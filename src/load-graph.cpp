#include <config.h>

#include <math.h>

#include <glib/gi18n.h>

#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/disk.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>

#include "application.h"
#include "load-graph.h"
#include "util.h"
#include "legacy/gsm_color_button.h"

constexpr int FRAME_WIDTH = 4;
constexpr unsigned GRAPH_MIN_HEIGHT = 40;

void
LoadGraph::clear_background ()
{
  gsm_graph_clear_background (GSM_GRAPH (disp));
}

bool
LoadGraph::is_logarithmic_scale () const
{
  // logarithmic scale is used only for memory graph
  return this->type == LOAD_GRAPH_MEM && GsmApplication::get ()->config.logarithmic_scale;
}

/*
 Translates y partial position to logarithmic position if set to logarithmic scale.
*/
float
LoadGraph::translate_to_log_partial_if_needed (float position_partial)
{
  if (this->is_logarithmic_scale ())
    position_partial = position_partial == 0 ? 0 : log10 (position_partial * 100) / 2;

  return position_partial;
}

static void
load_graph_rescale (LoadGraph *graph)
{
  ///org/gnome/desktop/interface/text-scaling-factor
  gsm_graph_set_font_size (GSM_GRAPH (graph->disp), 8 * graph->font_settings->get_double ("text-scaling-factor"));
}

static void
load_graph_draw (GtkDrawingArea* area,
                 cairo_t *cr,
                 int      width,
                 int      height,
                 gpointer data_ptr)
{
  LoadGraph * const graph = static_cast<LoadGraph*>(data_ptr);
  cairo_surface_t * background;
  guint frames_per_unit = gsm_graph_get_frames_per_unit (GSM_GRAPH (area));
  guint render_counter = gsm_graph_get_render_counter (GSM_GRAPH (area));
  double rmargin = gsm_graph_get_right_margin (GSM_GRAPH (area));
  guint num_points = gsm_graph_get_num_points (GSM_GRAPH (area));
  guint indent = gsm_graph_get_indent (GSM_GRAPH (area));
  graph->num_bars = gsm_graph_get_num_bars (GSM_GRAPH (graph->disp));
  guint dely = (height - 15) / graph->num_bars;   /* round to int to avoid AA blur */
  guint real_draw_height = dely * graph->num_bars;

  /* Initialize graph dimensions */
  width -= 2 * FRAME_WIDTH;
  height -= 2 * FRAME_WIDTH;

  gsm_graph_set_dely (GSM_GRAPH (area), dely);
  gsm_graph_set_real_draw_height (GSM_GRAPH (area), real_draw_height);

  /* Number of pixels wide for one sample point */
  const double x_step = double(width - rmargin - indent) / (num_points - 2);

  /* Lines start at the right edge of the drawing,
   * a bit outside the clip rectangle. */
  /* Adjustment for smooth movement between samples */
  double x_offset = width - rmargin + FRAME_WIDTH;

  /* Shift the x position of the most recent (shown rightmost) value outside of the clip area in order
     to be able to simulate continuous, smooth movement without the line being cut off at its ends */
  x_offset += x_step * (1 - render_counter / double(frames_per_unit));

  /* Draw background */
  if (!gsm_graph_is_background_set (GSM_GRAPH (graph->disp))) {
      gsm_graph_create_background (graph->disp, width, height);
  } 
  background = gsm_graph_get_background (GSM_GRAPH (graph->disp));

  cairo_set_source_surface (cr, background, 0, 0);
  cairo_paint (cr);

  /* Set the drawing style */
  cairo_set_line_width (cr, 1);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  /* Clip the drawing area to the inside of the drawn background */
  cairo_rectangle (cr,
                   indent + FRAME_WIDTH,
                   FRAME_WIDTH,
                   width - rmargin - indent,
                   real_draw_height);
  cairo_clip (cr);

  bool drawStacked = graph->type == LOAD_GRAPH_CPU && GsmApplication::get ()->config.draw_stacked;
  bool drawSmooth = GsmApplication::get ()->config.draw_smooth;
  gsm_graph_set_smooth_chart (GSM_GRAPH (area), drawSmooth);
  gsm_graph_set_stacked_chart (GSM_GRAPH (area), drawStacked);
  

  const double y_base = FRAME_WIDTH;

  /* Draw every graph */
  for (gint j = graph->n - 1; j >= 0; j--)
    {
      /* Set the color of the currently drawn graph */
      gdk_cairo_set_source_rgba (cr, &(graph->colors [j]));

      /* Start drawing on the right at the correct height */
      cairo_move_to (cr, x_offset, y_base + (1.0f - graph->data[0][j]) * real_draw_height);

      /* Draw the path of the line
         Loop starts at 1 because the curve accesses the 0th data point */
      for (guint i = 1; i < num_points; i++)
        {
          if (graph->data[i][j] == -1.0f)
            continue;

          if (drawSmooth)
            cairo_curve_to (cr,
                            x_offset - ((i - 0.5f) * x_step),
                            y_base + (1.0 - graph->data[i - 1][j]) * real_draw_height,
                            x_offset - ((i - 0.5f) * x_step),
                            y_base + (1.0 - graph->data[i][j]) * real_draw_height,
                            x_offset - (i * x_step),
                            y_base + (1.0 - graph->data[i][j]) * real_draw_height);
          else
            cairo_line_to (cr,
                           x_offset - (i * x_step),
                           y_base + (1.0 - graph->data[i][j]) * real_draw_height);
        }

      if (drawStacked)
        {
          /* Draw the remaining outline of the area */
          /* Left bottom corner */
          cairo_rel_line_to (cr, 0, y_base + real_draw_height);
          /* Right bottom corner.
             It's drawn far outside the visible area to avoid a weird bug
             where it's not filling the area it should completely */
          cairo_rel_line_to (cr, x_offset * 2, 0);

          cairo_close_path (cr);
          cairo_fill (cr);
        }
      else
        {
          cairo_stroke (cr);
        }
    }
}

void
load_graph_reset (LoadGraph *graph)
{
  std::fill (graph->data_block.begin (), graph->data_block.end (), -1.0);
}

static void
get_load (LoadGraph *graph)
{
  guint i;
  glibtop_cpu cpu;

  glibtop_get_cpu (&cpu);

  auto NOW = [&]() -> guint64 (&)[GLIBTOP_NCPU][N_CPU_STATES] {
               return graph->cpu.times[graph->cpu.now];
             };
  auto LAST = [&]() -> guint64 (&)[GLIBTOP_NCPU][N_CPU_STATES] {
                return graph->cpu.times[graph->cpu.now ^ 1];
              };

  if (graph->n == 1)
    {
      NOW ()[0][CPU_TOTAL] = cpu.total;
      NOW ()[0][CPU_USED] = cpu.user + cpu.nice + cpu.sys;
    }
  else
    {
      for (i = 0; i < graph->n; i++)
        {
          NOW ()[i][CPU_TOTAL] = cpu.xcpu_total[i];
          NOW ()[i][CPU_USED] = cpu.xcpu_user[i] + cpu.xcpu_nice[i]
                                + cpu.xcpu_sys[i];
        }
    }

  // on the first call, LAST is 0
  // which means data is set to the average load since boot
  // that value has no meaning, we just want all the
  // graphs to be aligned, so the CPU graph needs to start
  // immediately
  bool drawStacked = graph->type == LOAD_GRAPH_CPU && GsmApplication::get ()->config.draw_stacked;

  for (i = 0; i < graph->n; i++)
    {
      float load;
      float total, used;
      gchar *text;

      total = NOW ()[i][CPU_TOTAL] - LAST ()[i][CPU_TOTAL];
      used = NOW ()[i][CPU_USED] - LAST ()[i][CPU_USED];

      load = used / MAX (total, 1.0f);
      graph->data[0][i] = load;
      if (drawStacked)
        {
          graph->data[0][i] /= graph->n;
          if (i > 0)
            graph->data[0][i] += graph->data[0][i - 1];
        }

      /* Update label */
      // Translators: CPU usage percentage label: 95.7%
      text = g_strdup_printf (_("%.1f%%"), load * 100.0f);
      gtk_label_set_text (GTK_LABEL (graph->labels.cpu[i]), text);
      g_free (text);
    }

  graph->cpu.now ^= 1;
}

static void
set_memory_label_and_picker (GtkLabel      *label,
                             GsmColorButton*picker,
                             guint64        used,
                             guint64        cached,
                             guint64        total,
                             double         percent)
{
  char*used_text;
  char*cached_text;
  char*cached_label;
  char*total_text;
  char*text;

  used_text = format_byte_size (used, GsmApplication::get ()->config.resources_memory_in_iec);
  cached_text = format_byte_size (cached, GsmApplication::get ()->config.resources_memory_in_iec);
  total_text = format_byte_size (total, GsmApplication::get ()->config.resources_memory_in_iec);
  if (total == 0)
    {
      text = g_strdup (_("not available"));
    }
  else
    {
      // xgettext: "540MiB (53 %) of 1.0 GiB" or "540MB (53 %) of 1.0 GB"
      text = g_strdup_printf (_("%s (%.1f%%) of %s"), used_text, 100.0 * percent, total_text);

      if (cached != 0)
        {
          // xgettext: Used cache string, e.g.: "Cache 2.4GiB" or "Cache 2.4GB"
          cached_label = g_strdup_printf (_("Cache %s"), cached_text);
          text = g_strdup_printf ("%s\n%s", text, cached_label);
          g_free (cached_label);
        }
    }

  gtk_label_set_text (label, text);
  g_free (used_text);
  g_free (cached_text);
  g_free (total_text);
  g_free (text);

  if (picker)
    gsm_color_button_set_fraction (picker, percent);
}

static void
get_memory (LoadGraph *graph)
{
  float mempercent, swappercent;

  glibtop_mem mem;
  glibtop_swap swap;

  glibtop_get_mem (&mem);
  glibtop_get_swap (&swap);

  /* There's no swap on LiveCD : 0.0f is better than NaN :) */
  swappercent = (swap.total ? (float)swap.used / (float)swap.total : 0.0f);
  mempercent = (float)mem.user / (float)mem.total;
  set_memory_label_and_picker (GTK_LABEL (graph->labels.memory),
                               GSM_COLOR_BUTTON (graph->mem_color_picker),
                               mem.user, mem.cached, mem.total, mempercent);

  set_memory_label_and_picker (GTK_LABEL (graph->labels.swap),
                               GSM_COLOR_BUTTON (graph->swap_color_picker),
                               swap.used, 0, swap.total, swappercent);

  gtk_widget_set_sensitive (GTK_WIDGET (graph->swap_color_picker), swap.total > 0);

  graph->data[0][0] = graph->translate_to_log_partial_if_needed (mempercent);
  graph->data[0][1] = swap.total > 0 ? graph->translate_to_log_partial_if_needed (swappercent) : -1.0;
}

/* Nice Numbers for Graph Labels after Paul Heckbert
   nicenum: find a "nice" number approximately equal to x.
   Round the number if round=1, take ceiling if round=0    */

static double
nicenum (double x,
         int    round)
{
  int expv;                  /* exponent of x */
  double f;                  /* fractional part of x */
  double nf;                  /* nice, rounded fraction */

  expv = floor (log10 (x));
  f = x / pow (10.0, expv);       /* between 1 and 10 */
  if (round)
    {
      if (f < 1.5)
        nf = 1.0;
      else if (f < 3.0)
        nf = 2.0;
      else if (f < 7.0)
        nf = 5.0;
      else
        nf = 10.0;
    }
  else
    {
      if (f <= 1.0)
        nf = 1.0;
      else if (f <= 2.0)
        nf = 2.0;
      else if (f <= 5.0)
        nf = 5.0;
      else
        nf = 10.0;
    }
  return nf * pow (10.0, expv);
}

static void
dynamic_scale (LoadGraph             *graph,
               std::vector<unsigned> *values,
               guint64               *max,
               guint64                din,
               guint64                dout,
               gboolean               in_bits)
{
  graph->data[0][0] = 1.0f * din / *max;
  graph->data[0][1] = 1.0f * dout / *max;

  guint64 dmax = std::max (din, dout);

  if (graph->latest == 0)
    values->at (graph->num_points - 1) = dmax;
  else
    values->at (graph->latest - 1) = dmax;

  guint64 new_max;
  // both way, new_max is the greatest value
  if (dmax >= *max)
    new_max = dmax;
  else
    new_max = *std::max_element (&values->at (0),
                                 &values->at (graph->num_points - 1));

  //
  // Round maximum
  //

  const guint64 bak_max (new_max);

  if (in_bits)
    {
      // nice number is for the ticks
      unsigned ticks = graph->num_bars;

      if (graph->num_bars == 0)
        return;

      // gets messy at low values due to division by 8
      guint64 bit_max = std::max (new_max * 8, G_GUINT64_CONSTANT (10000));

      // our tick size leads to max
      double d = nicenum (bit_max / ticks, 0);
      bit_max = ticks * d;
      new_max = bit_max / 8;

      procman_debug ("bak*8 %" G_GUINT64_FORMAT ", ticks %d, d %f"
                     ", bit_max %" G_GUINT64_FORMAT ", new_max %" G_GUINT64_FORMAT,
                     bak_max * 8, ticks, d, bit_max, new_max);
    }
  else
    {
      // round up to get some extra space
      // yes, it can overflow
      new_max = 1.1 * new_max;
      // make sure max is not 0 to avoid / 0
      // default to 1 KiB
      new_max = std::max (new_max, G_GUINT64_CONSTANT (1024));

      // decompose new_max = coef10 * 2**(base10 * 10)
      // where coef10 and base10 are integers and coef10 < 2**10
      //
      // e.g: ceil(100.5 KiB) = 101 KiB = 101 * 2**(1 * 10)
      //      where base10 = 1, coef10 = 101, pow2 = 16

      guint64 pow2 = std::floor (log2 (new_max));
      guint64 base10 = pow2 / 10.0;
      guint64 coef10 = std::ceil (new_max / double (G_GUINT64_CONSTANT (1) << (base10 * 10)));
      g_assert (new_max <= (coef10 * (G_GUINT64_CONSTANT (1) << (base10 * 10))));

      // then decompose coef10 = x * 10**factor10
      // where factor10 is integer and x < 10
      // so we new_max has only 1 significant digit

      guint64 factor10 = std::pow (10.0, std::floor (std::log10 (coef10)));
      coef10 = std::ceil (coef10 / double (factor10)) * factor10;

      new_max = coef10 * (G_GUINT64_CONSTANT (1) << guint64 (base10 * 10));
      procman_debug ("bak %" G_GUINT64_FORMAT " new_max %" G_GUINT64_FORMAT
                     "pow2 %" G_GUINT64_FORMAT " coef10 %" G_GUINT64_FORMAT,
                     bak_max, new_max, pow2, coef10);
    }

  // if max is the same or has decreased but not so much, don't
  // do anything to avoid rescaling
  if ((0.8 * graph->net.max) < new_max && new_max <= graph->net.max)
    return;

  const double scale = 1.0f * *max / new_max;

  for (size_t i = 0; i < graph->num_points; i++)
    if (graph->data[i][0] >= 0.0f)
      {
        graph->data[i][0] *= scale;
        graph->data[i][1] *= scale;
      }

  procman_debug ("rescale dmax = %" G_GUINT64_FORMAT
                 " max = %" G_GUINT64_FORMAT
                 " new_max = %" G_GUINT64_FORMAT,
                 dmax, *max, new_max);

  *max = new_max;
  gsm_graph_set_max_value (graph->disp, new_max);

  // force the graph background to be redrawn now that scale has changed
  graph->clear_background ();
}

static guint64
get_hash64 (const gchar*c_str)
{
  // Fowler–Noll–Vo FNV-1 64-bit hash:

  guint64 hash = 0xcbf29ce484222325L;

  while (gchar c = *c_str++)
    {
      hash = (hash * 0x00000100000001B3L) ^ c;
    }

  return hash;
}

static void
handle_dynamic_max_value (LoadGraph             *graph,
                          std::vector<unsigned> *values,
                          guint64               *max,
                          guint64               *last_in,
                          guint64               *last_out,
                          guint64                in,
                          guint64                out,
                          guint64               *graph_time,
                          guint64                hash,
                          guint64               *graph_hash,
                          gboolean               in_bits,
                          gboolean               totals_in_bits,
                          GtkLabel              *label_in,
                          GtkLabel              *label_out,
                          GtkLabel              *label_in_total,
                          GtkLabel              *label_out_total)
{
  guint64 time = g_get_monotonic_time ();
  guint64 din, dout;

  if (in >= *last_in && out >= *last_out &&
      (graph_hash == NULL || hash == *graph_hash) &&
      *graph_time != 0)
    {
      float dtime = ((double) (time - *graph_time)) / G_USEC_PER_SEC;
      din = static_cast<guint64>((in - *last_in) / dtime);
      dout = static_cast<guint64>((out - *last_out) / dtime);
    }
  else
    {
      /* Don't calc anything if new data is less than old (interface
         removed, counters reset, ...) or if it is the first time */
      din = 0;
      dout = 0;
    }

  *last_in = in;
  *last_out = out;
  *graph_time = time;
  if (graph_hash != NULL)
    *graph_hash = hash;

  dynamic_scale (graph, values, max, din, dout, in_bits);

  gtk_label_set_text (GTK_LABEL (label_in), procman::format_rate (din, in_bits));
  gtk_label_set_text (GTK_LABEL (label_in_total), procman::format_volume (in, totals_in_bits));

  gtk_label_set_text (GTK_LABEL (label_out), procman::format_rate (dout, in_bits));
  gtk_label_set_text (GTK_LABEL (label_out_total), procman::format_volume (out, totals_in_bits));
}

static void
get_net (LoadGraph *graph)
{
  glibtop_netlist netlist;
  guint32 i;
  guint64 in = 0, out = 0;
  guint64 hash = 1;
  char **ifnames;

  ifnames = glibtop_get_netlist (&netlist);

  for (i = 0; i < netlist.number; ++i)
    {
      glibtop_netload netload;
      glibtop_get_netload (&netload, ifnames[i]);

      if (netload.if_flags & (1 << GLIBTOP_IF_FLAGS_LOOPBACK))
        continue;

      /* Skip interfaces without any IPv4/IPv6 address (or
         those with only a LINK ipv6 addr) However we need to
         be able to exclude these while still keeping the
         value so when they get online (with NetworkManager
         for example) we don't get a sudden peak.  Once we're
         able to get this, ignoring down interfaces will be
         possible too.  */
      if (not ((netload.flags & (1 << GLIBTOP_NETLOAD_ADDRESS6))
               and netload.scope6 != GLIBTOP_IF_IN6_SCOPE_LINK)
          and not (netload.flags & (1 << GLIBTOP_NETLOAD_ADDRESS)))
        continue;

      /* Don't skip interfaces that are down (GLIBTOP_IF_FLAGS_UP)
         to avoid spikes when they are brought up */

      in += netload.bytes_in;
      out += netload.bytes_out;
      hash += get_hash64 (ifnames[i]);
    }

  g_strfreev (ifnames);

  handle_dynamic_max_value (graph, &graph->net.values, &graph->net.max, &graph->net.last_in,
                            &graph->net.last_out, in, out, &graph->net.time,
                            hash, &graph->net.last_hash,
                            GsmApplication::get ()->config.network_in_bits,
                            GsmApplication::get ()->config.network_total_in_bits,
                            graph->labels.net_in, graph->labels.net_out,
                            graph->labels.net_in_total, graph->labels.net_out_total);
}

static void
get_disk (LoadGraph *graph)
{
  glibtop_disk disk;
  gint32 i;
  guint64 read = 0, write = 0;

  glibtop_get_disk (&disk);

  for (i = 0; i < glibtop_global_server->ndisk; i++)
    {
      read += disk.xdisk_sectors_read[i];
      write += disk.xdisk_sectors_write[i];
    }

  // These sectors are standard unix 512 byte sectors, not the actual disk sectors.
  read *= 512;
  write *= 512;

  handle_dynamic_max_value (graph, &graph->disk.values, &graph->disk.max, &graph->disk.last_read,
                            &graph->disk.last_write, read, write, &graph->disk.time, 0, NULL,
                            FALSE, FALSE, graph->labels.disk_read, graph->labels.disk_write,
                            graph->labels.disk_read_total, graph->labels.disk_write_total);
}

int
load_graph_update_data (LoadGraph *graph)
{
  // Rotate data one element down.
  std::rotate (graph->data.begin (),
               graph->data.end () - 1,
               graph->data.end ());

  // Update rotation counter.
  graph->latest = (graph->latest + 1) % graph->num_points;

  // Replace the 0th element
  switch (graph->type)
    {
      case LOAD_GRAPH_CPU:
        get_load (graph);
        break;

      case LOAD_GRAPH_MEM:
        get_memory (graph);
        break;

      case LOAD_GRAPH_NET:
        get_net (graph);
        break;

      case LOAD_GRAPH_DISK:
        get_disk (graph);
        break;

      default:
        g_assert_not_reached ();
    }
  return 0;
}

static void
load_graph_destroy (GtkWidget*,
                    gpointer data_ptr)
{
  LoadGraph * const graph = static_cast<LoadGraph*>(data_ptr);

  delete graph;
}

LoadGraph::LoadGraph(guint type)
  :
  indent (18.0),
  n (0),
  type (type),
  num_points (GsmApplication::get ()->config.graph_data_points + 2),
  latest (0),
  num_bars (0),
  colors (),
  data_block (),
  data (),
  main_widget (NULL),
  disp (NULL),
  labels (),
  mem_color_picker (NULL),
  swap_color_picker (NULL),
  font_settings (Gio::Settings::create (FONT_SETTINGS_SCHEMA)),
  cpu (),
  net (),
  disk ()
{
  font_settings->signal_changed (FONT_SETTING_SCALING).connect ([this](const Glib::ustring&) {
    load_graph_rescale (this);
  });

  switch (type)
    {
      case LOAD_GRAPH_CPU:
        cpu = CPU {};
        n = GsmApplication::get ()->config.num_cpus;

        for (guint i = 0; i < G_N_ELEMENTS (labels.cpu); ++i)
          labels.cpu[i] = make_tnum_label ();

        break;

      case LOAD_GRAPH_MEM:
        n = 2;
        labels.memory = init_tnum_label (10, GTK_ALIGN_START);
        labels.swap = init_tnum_label (10, GTK_ALIGN_START);
        break;

      case LOAD_GRAPH_NET:
        net = NET {};
        n = 2;
        net.max = 1;
        labels.net_in = init_tnum_label (10, GTK_ALIGN_END);
        labels.net_in_total = init_tnum_label (10, GTK_ALIGN_END);
        labels.net_out = init_tnum_label (10, GTK_ALIGN_END);
        labels.net_out_total = init_tnum_label (10, GTK_ALIGN_END);
        break;

      case LOAD_GRAPH_DISK:
        disk = DISK {};
        n = 2;
        disk.max = 1;
        labels.disk_read = init_tnum_label (16, GTK_ALIGN_END);
        labels.disk_read_total = init_tnum_label (16, GTK_ALIGN_END);
        labels.disk_write = init_tnum_label (10, GTK_ALIGN_END);
        labels.disk_write_total = init_tnum_label (10, GTK_ALIGN_END);
        break;
    }

  colors.resize (n);

  disp = GSM_GRAPH (gsm_graph_new ());
  gsm_graph_set_data_function (disp, (GSourceFunc)load_graph_update_data, this);
  gsm_graph_set_num_points (disp, num_points);
  gsm_graph_set_speed (disp, GsmApplication::get ()->config.graph_update_interval);
  gsm_graph_set_x_labels_function (disp, procman::format_duration);
  
  
  switch (type)
    {
      case LOAD_GRAPH_CPU:
        memcpy (&colors[0], GsmApplication::get ()->config.cpu_color,
                n * sizeof colors[0]);
        gsm_graph_set_max_value (disp, 100);
        gsm_graph_set_y_labels_function (disp, procman::format_percentage);
        break;

      case LOAD_GRAPH_MEM:
        colors[0] = GsmApplication::get ()->config.mem_color;
        colors[1] = GsmApplication::get ()->config.swap_color;
        mem_color_picker = gsm_color_button_new (&colors[0],
                                                 GSMCP_TYPE_PIE);
        swap_color_picker = gsm_color_button_new (&colors[1],
                                                  GSMCP_TYPE_PIE);
        gsm_graph_set_max_value (disp, 100);
        gsm_graph_set_y_labels_function (disp, procman::format_percentage);
        break;

      case LOAD_GRAPH_NET:
        net.values = std::vector<unsigned>(num_points);
        colors[0] = GsmApplication::get ()->config.net_in_color;
        colors[1] = GsmApplication::get ()->config.net_out_color;
        gsm_graph_set_max_value (disp, this->net.max);
        gsm_graph_set_y_labels_function (disp, procman::format_network_rate);
        break;

      case LOAD_GRAPH_DISK:
        disk.values = std::vector<unsigned>(num_points);
        colors[0] = GsmApplication::get ()->config.disk_read_color;
        colors[1] = GsmApplication::get ()->config.disk_write_color;
        gsm_graph_set_max_value (disp, this->disk.max);
        gsm_graph_set_y_labels_function (disp, (GsmLabelFunction)procman::format_rate);
        break;
    }

  main_widget = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
  gtk_widget_set_size_request (GTK_WIDGET (main_widget), -1, GRAPH_MIN_HEIGHT);

  gtk_widget_set_vexpand (GTK_WIDGET (disp), TRUE);
  gtk_widget_set_hexpand (GTK_WIDGET (disp), TRUE);
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (disp),
                                  load_graph_draw,
                                  this,
                                  NULL);
  g_signal_connect (G_OBJECT (disp), "destroy",
                    G_CALLBACK (load_graph_destroy), this);
  gtk_box_prepend (main_widget, GTK_WIDGET (disp));

  data = std::vector<double*>(num_points);
  /* Allocate data in a contiguous block */
  data_block = std::vector<double>(n * num_points, -1.0);

  for (guint i = 0; i < num_points; ++i)
    data[i] = &data_block[0] + i * n;

}

LoadGraph::~LoadGraph()
{
  load_graph_stop (this);
}

void
load_graph_start (LoadGraph *graph)
{
  gsm_graph_start (GSM_GRAPH (graph->disp));
}

void
load_graph_stop (LoadGraph *graph)
{
  /* don't draw anymore, but continue to poll */
  gsm_graph_stop (GSM_GRAPH (graph->disp));
}

void
load_graph_change_speed (LoadGraph *graph,
                         guint      new_speed)
{
  gsm_graph_set_speed (GSM_GRAPH (graph->disp), new_speed);
}

void
load_graph_change_num_points (LoadGraph *graph,
                              guint      new_num_points)
{
  // Don't do anything if the value didn't change.
  if (graph->num_points == new_num_points)
    return;

  // Sort the values in the data_block vector in the order they were accessed in by the pointers in data.
  std::rotate (graph->data_block.begin (),
               graph->data_block.begin () + (graph->num_points - graph->latest) * graph->n,
               graph->data_block.end ());

  // Reset rotation counter.
  graph->latest = 0;

  // Resize the vectors to the new amount of data points.
  // Fill the new values with -1.
  graph->data.resize (new_num_points);
  graph->data_block.resize (graph->n * new_num_points, -1.0);
  if (graph->type == LOAD_GRAPH_NET)
    graph->net.values.resize (new_num_points);
  else if (graph->type == LOAD_GRAPH_DISK)
    graph->disk.values.resize (new_num_points);

  // Replace the pointers in data, to match the new data_block values.
  for (guint i = 0; i < new_num_points; ++i)
    graph->data[i] = &graph->data_block[0] + i * graph->n;

  // Set the actual number of data points to be used by the graph.
  graph->num_points = new_num_points;
  gsm_graph_set_num_points (graph->disp, new_num_points);

}

LoadGraphLabels*
load_graph_get_labels (LoadGraph *graph)
{
  return &graph->labels;
}

GtkBox*
load_graph_get_widget (LoadGraph *graph)
{
  return graph->main_widget;
}

GsmColorButton*
load_graph_get_mem_color_picker (LoadGraph *graph)
{
  return graph->mem_color_picker;
}

GsmColorButton*
load_graph_get_swap_color_picker (LoadGraph *graph)
{
  return graph->swap_color_picker;
}
