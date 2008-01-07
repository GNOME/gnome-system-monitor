#include <config.h>

#include <gtkmm.h>

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <gdk/gdkx.h>

#include <glib/gi18n.h>

#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>
#include <math.h>

#include <libgnomevfs/gnome-vfs-utils.h>

#include <algorithm>

#include "procman.h"
#include "load-graph.h"
#include "util.h"
#include "gsm_color_button.h"

#define NUM_POINTS 62
#define GRAPH_MIN_HEIGHT 40
#define FRAMES 10

enum {
	CPU_TOTAL,
	CPU_USED,
	N_CPU_STATES
};

struct LoadGraph {


	unsigned num_bars() const;

	double fontsize;
	double rmargin;
	double indent;

	guint n;
	gint type;
	guint speed;
	guint draw_width, draw_height;
	gint render_counter;

	GdkColor *colors;
	GtkWidget *notebook;

	gfloat* data_block;
	gfloat* data[NUM_POINTS];

	GtkWidget *main_widget;
	GtkWidget *disp;

	cairo_surface_t *buffer;
	cairo_surface_t *graph_buffer;
	cairo_surface_t *background_buffer;

	guint timer_index;

	gboolean draw;

	LoadGraphLabels labels;
	GtkWidget *mem_color_picker;
	GtkWidget *swap_color_picker;

	/* union { */
		struct {
			guint now; /* 0 -> current, 1 -> last
				    now ^ 1 each time */
			/* times[now], times[now ^ 1] is last */
			guint64 times[2][GLIBTOP_NCPU][N_CPU_STATES];
		} cpu;

		struct {
			guint64 last_in, last_out;
			GTimeVal time;
			unsigned int max;
			unsigned values[NUM_POINTS];
			size_t cur;
		} net;
	/* }; */
};



unsigned LoadGraph::num_bars() const
{
	unsigned n;

	// keep 100 % num_bars == 0
	switch (static_cast<int>(this->draw_height / (this->fontsize + 14)))
	{
	case 0:
	case 1:
		n = 1;
		break;
	case 2:
	case 3:
		n = 2;
		break;
	case 4:
		n = 4;
		break;
	default:
		n = 5;
	}

	return n;
}



#define FRAME_WIDTH 4
void draw_background(LoadGraph *g) {
	double dash[2] = { 1.0, 2.0 };
	cairo_t *cr;
	cairo_t* tmp_cr;
	int dely;
	int real_draw_height;
	guint i;
	unsigned num_bars;
	char *caption;
	cairo_text_extents_t extents;


	num_bars = g->num_bars();
	dely = (g->draw_height - 15) / num_bars; /* round to int to avoid AA blur */
	real_draw_height = dely * num_bars;
	
	cr = cairo_create (g->buffer);
	g->background_buffer = cairo_surface_create_similar (cairo_get_target (cr),
							     CAIRO_CONTENT_COLOR_ALPHA,
							     g->draw_width + (2*FRAME_WIDTH),
							     g->draw_height + FRAME_WIDTH);
	
	tmp_cr = cairo_create (g->background_buffer);

	/* draw frame */
	cairo_translate (tmp_cr, FRAME_WIDTH, FRAME_WIDTH);
	
	/* Draw background rectangle */
	cairo_set_source_rgb (tmp_cr, 1.0, 1.0, 1.0);
	cairo_rectangle (tmp_cr, g->rmargin + g->indent, 0,
			 g->draw_width - g->rmargin - g->indent, real_draw_height);
	cairo_fill(tmp_cr);
	
	cairo_set_line_width (tmp_cr, 1.0);
	cairo_set_dash (tmp_cr, dash, 2, 0);
	cairo_set_font_size (tmp_cr, g->fontsize);

	for (i = 0; i <= num_bars; ++i) {
		double y;

		if (i == 0)
		  y = 0.5 + g->fontsize / 2.0;
		else if (i == num_bars)
		  y = i * dely + 0.5;
		else
		  y = i * dely + g->fontsize / 2.0;
		cairo_set_source_rgba (tmp_cr, 0, 0, 0, 1);
		if (g->type == LOAD_GRAPH_NET) {
			// operation orders matters so it's 0 if i == num_bars
			unsigned rate = g->net.max - (i * g->net.max / num_bars);
			const std::string caption(procman::format_rate(rate));
			cairo_text_extents (tmp_cr, caption.c_str(), &extents);
			cairo_move_to (tmp_cr, g->indent - extents.width + 20, y);
			cairo_show_text (tmp_cr, caption.c_str());
		} else {
			// operation orders matters so it's 0 if i == num_bars
			caption = g_strdup_printf("%d %%", 100 - i * (100 / num_bars));
			cairo_text_extents (tmp_cr, caption, &extents);
			cairo_move_to (tmp_cr, g->indent - extents.width + 20, y);
			cairo_show_text (tmp_cr, caption);
			g_free (caption);
		}

		cairo_set_source_rgba (tmp_cr, 0, 0, 0, 0.75);
		cairo_move_to (tmp_cr, g->rmargin + g->indent - 3, i * dely + 0.5);
		cairo_line_to (tmp_cr, g->draw_width - 0.5, i * dely + 0.5);
	}
	cairo_stroke (tmp_cr);

	cairo_set_dash (tmp_cr, dash, 2, 1.5);

	const unsigned total_seconds = g->speed * (NUM_POINTS - 2) / 1000;

	for (unsigned int i = 0; i < 7; i++) {
		double x = (i) * (g->draw_width - g->rmargin - g->indent) / 6;
		cairo_set_source_rgba (tmp_cr, 0, 0, 0, 0.75);
		cairo_move_to (tmp_cr, (ceil(x) + 0.5) + g->rmargin + g->indent, 0.5);
		cairo_line_to (tmp_cr, (ceil(x) + 0.5) + g->rmargin + g->indent, real_draw_height + 4.5);
		cairo_stroke(tmp_cr);
		unsigned seconds = total_seconds - i * total_seconds / 6;
		const char* format;
		if (i == 0)
			format = dngettext(GETTEXT_PACKAGE, "%u second", "%u seconds", seconds);
		else
			format = "%u";
		caption = g_strdup_printf(format, seconds);
		cairo_text_extents (tmp_cr, caption, &extents);
		cairo_move_to (tmp_cr, ((ceil(x) + 0.5) + g->rmargin + g->indent) - (extents.width/2), g->draw_height);
		cairo_set_source_rgba (tmp_cr, 0, 0, 0, 1);
		cairo_show_text (tmp_cr, caption);
		g_free (caption);
	}

	cairo_stroke (tmp_cr);
	cairo_destroy (tmp_cr);
	cairo_destroy (cr); 
}

/* Redraws the backing buffer for the load graph and updates the window */
void
load_graph_draw (LoadGraph *g)
{
	cairo_t *cr;
	int dely;
	double delx;
	int real_draw_height;
	guint i, j;
	unsigned num_bars;

	num_bars = g->num_bars();

	cr = cairo_create (g->buffer);
	GtkStyle *style = gtk_widget_get_style (g->notebook);
	gdk_cairo_set_source_color (cr, &style->bg[GTK_STATE_NORMAL]);
	cairo_paint (cr);

	dely = (g->draw_height - 15) / num_bars; /* round to int to avoid AA blur */
	delx = (g->draw_width - 2.0 - g->rmargin - g->indent) / (NUM_POINTS - 3);
	real_draw_height = dely * num_bars;

	/* draw the graph */
	if ((g->render_counter == 0) || (g->background_buffer == NULL)) {
		cairo_surface_destroy(g->graph_buffer);
		cairo_t* tmp_cr;

		g->graph_buffer = cairo_surface_create_similar (cairo_get_target (cr),
							 	CAIRO_CONTENT_COLOR_ALPHA,
								g->draw_width,
								g->draw_height);
		tmp_cr = cairo_create (g->graph_buffer);

		cairo_set_line_width (tmp_cr, 1.5);
		cairo_set_line_cap (tmp_cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join (tmp_cr, CAIRO_LINE_JOIN_ROUND);

		for (j = 0; j < g->n; ++j) {
			cairo_move_to (tmp_cr,
				       g->draw_width - 2.0,
				       (1.0f - g->data[0][j]) * real_draw_height);
			gdk_cairo_set_source_color (tmp_cr, &(g->colors [j]));

			for (i = 1; i < NUM_POINTS; ++i) {
				if (g->data[i][j] == -1.0f)
					continue;

				cairo_curve_to (tmp_cr, 
					       (g->draw_width - (i-1) * delx) - (delx/2),
					       (1.0f - g->data[i-1][j]) * real_draw_height,
					       (g->draw_width - i * delx) + (delx/2),
					       (1.0f - g->data[i][j]) * real_draw_height,
					       g->draw_width - i * delx,
					       (1.0f - g->data[i][j]) * real_draw_height);
			}

			cairo_stroke (tmp_cr);

		}
		cairo_destroy (tmp_cr);
	} 

	/* Composite and clip the surfaces together */
	if (g->background_buffer == NULL) {
		draw_background(g);
	}
	cairo_set_source_surface (cr, g->background_buffer, 0, 0);
	cairo_paint (cr);

	cairo_set_source_surface (cr, g->graph_buffer, -1*((g->render_counter) * (delx/FRAMES)) + (1.5*delx) + FRAME_WIDTH, FRAME_WIDTH);
	cairo_rectangle (cr, g->rmargin + g->indent + FRAME_WIDTH + 1, FRAME_WIDTH - 1,
			 g->draw_width - g->rmargin - g->indent - 1 , real_draw_height +FRAME_WIDTH - 1);
	cairo_fill (cr);
	cairo_destroy (cr);

	/* repaint */
	gtk_widget_queue_draw (g->disp);
}

static gboolean
load_graph_configure (GtkWidget *widget,
		      GdkEventConfigure *event,
		      gpointer data_ptr)
{
	LoadGraph * const g = static_cast<LoadGraph*>(data_ptr);
	cairo_t *cr;

	g->draw_width = widget->allocation.width - 2 * FRAME_WIDTH;
	g->draw_height = widget->allocation.height - 2 * FRAME_WIDTH;

	cr = gdk_cairo_create (widget->window);

	if (g->buffer)
		cairo_surface_destroy (g->buffer);

	g->buffer = cairo_surface_create_similar (cairo_get_target (cr),
						  CAIRO_CONTENT_COLOR,
						  widget->allocation.width,
						  widget->allocation.height);

	cairo_destroy (cr);

	if (g->background_buffer != NULL) {
		cairo_surface_destroy(g->background_buffer);
		g->background_buffer = NULL;
	}

	load_graph_draw (g);

	return TRUE;
}

static gboolean
load_graph_expose (GtkWidget *widget,
		   GdkEventExpose *event,
		   gpointer data_ptr)
{
	LoadGraph * const g = static_cast<LoadGraph*>(data_ptr);
	cairo_t *cr;

	cr = gdk_cairo_create(widget->window);

	cairo_set_source_surface(cr, g->buffer, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);

	return TRUE;
}

static void
get_load (LoadGraph *g)
{
	guint i;
	glibtop_cpu cpu;

	glibtop_get_cpu (&cpu);

#undef NOW
#undef LAST
#define NOW  (g->cpu.times[g->cpu.now])
#define LAST (g->cpu.times[g->cpu.now ^ 1])

	if (g->n == 1) {
		NOW[0][CPU_TOTAL] = cpu.total;
		NOW[0][CPU_USED] = cpu.user + cpu.nice + cpu.sys;
	} else {
		for (i = 0; i < g->n; i++) {
			NOW[i][CPU_TOTAL] = cpu.xcpu_total[i];
			NOW[i][CPU_USED] = cpu.xcpu_user[i] + cpu.xcpu_nice[i]
				+ cpu.xcpu_sys[i];
		}
	}

	// on the first call, LAST is 0
	// which means data is set to the average load since boot
	// that value has no meaning, we just want all the
	// graphs to be aligned, so the CPU graph needs to start
	// immediately

	for (i = 0; i < g->n; i++) {
		float load;
		float total, used;
		gchar *text;

		total = NOW[i][CPU_TOTAL] - LAST[i][CPU_TOTAL];
		used  = NOW[i][CPU_USED]  - LAST[i][CPU_USED];

		load = used / MAX(total, 1.0f);
		g->data[0][i] = load;

		/* Update label */
		text = g_strdup_printf("%.1f%%", load * 100.0f);
		gtk_label_set_text(GTK_LABEL(g->labels.cpu[i]), text);
		g_free(text);
	}

	g->cpu.now ^= 1;

#undef NOW
#undef LAST
}


namespace
{

  void set_memory_label_and_picker(GtkLabel* label, GSMColorButton* picker,
				   guint64 used, guint64 total, double percent)
  {
    char* used_text;
    char* total_text;
    char* text;

    used_text = SI_gnome_vfs_format_file_size_for_display(used);
    total_text = SI_gnome_vfs_format_file_size_for_display(total);
    // xgettext: 540MiB (53 %) of 1.0 GiB
    text = g_strdup_printf(_("%s (%.1f %%) of %s"), used_text, 100.0 * percent, total_text);
    gtk_label_set_text(label, text);
    g_free(used_text);
    g_free(total_text);
    g_free(text);

    if (picker)
      gsm_color_button_set_fraction(picker, percent);
  }
}

static void
get_memory (LoadGraph *g)
{
	float mempercent, swappercent;

	glibtop_mem mem;
	glibtop_swap swap;

	glibtop_get_mem (&mem);
	glibtop_get_swap (&swap);

	/* There's no swap on LiveCD : 0.0f is better than NaN :) */
	swappercent = (swap.total ? (float)swap.used / (float)swap.total : 0.0f);
	mempercent  = (float)mem.user  / (float)mem.total;

	set_memory_label_and_picker(GTK_LABEL(g->labels.memory),
				    GSM_COLOR_BUTTON(g->mem_color_picker),
				    mem.user, mem.total, mempercent);

	set_memory_label_and_picker(GTK_LABEL(g->labels.swap),
				    GSM_COLOR_BUTTON(g->swap_color_picker),
				    swap.used, swap.total, swappercent);

	g->data[0][0] = mempercent;
	g->data[0][1] = swappercent;
}

static void
net_scale (LoadGraph *g, unsigned din, unsigned dout)
{
	g->data[0][0] = 1.0f * din / g->net.max;
	g->data[0][1] = 1.0f * dout / g->net.max;

	unsigned dmax = std::max(din, dout);
	g->net.values[g->net.cur] = dmax;
	g->net.cur = (g->net.cur + 1) % NUM_POINTS;

	unsigned new_max;
	// both way, new_max is the greatest value
	if (dmax >= g->net.max)
		new_max = dmax;
	else
		new_max = *std::max_element(&g->net.values[0],
					    &g->net.values[NUM_POINTS]);

	//
	// Round network maximum
	//

	const unsigned bak_max(new_max);

	// round up to get some extra space
	new_max = 11U * new_max / 10U;
	// make sure max is not 0 to avoid / 0
	// default to 1 KiB
	new_max = std::max(new_max, 1024U);

	// decompose new_max = coef10 * 2**(base10 * 10)
	// where coef10 and base10 are integers and coef10 < 2**10
	//
	// e.g: ceil(100.5 KiB) = 101 KiB = 101 * 2**(1 * 10)
	//      where base10 = 1, coef10 = 101, pow2 = 16

	unsigned pow2 = std::floor(log2(new_max));
	unsigned base10 = pow2 / 10;
	unsigned coef10 = std::ceil(new_max / double(1UL << (base10 * 10)));
	g_assert(new_max <= (coef10 * (1UL << (base10 * 10))));

	// then decompose coef10 = x * 10**factor10
	// where factor10 is integer and x < 10
	// so we new_max has only 1 significant digit

	unsigned factor10 = std::pow(10.0, std::floor(std::log10(coef10)));
	coef10 = std::ceil(coef10 / double(factor10)) * factor10;

	// then make coef10 divisible by num_bars
	if (coef10 % g->num_bars() != 0)
		coef10 = coef10 + (g->num_bars() - coef10 % g->num_bars());
	g_assert(coef10 % g->num_bars() == 0);

	new_max = coef10 * (1UL << (base10 * 10));
	procman_debug("bak %u new_max %u pow2 %u coef10 %u", bak_max, new_max, pow2, coef10);
	g_assert(bak_max <= new_max);

	if (new_max == g->net.max)
		return;

	// if max has decreased but not so much, don't do anything
	// to avoid rescaling
	if ((8U * g->net.max / 10U) < new_max && new_max < g->net.max)
		return;

	const float scale = 1.0f * g->net.max / new_max;

	for (size_t i = 0; i < NUM_POINTS; i++) {
		if (g->data[i][0] >= 0.0f) {
			g->data[i][0] *= scale;
			g->data[i][1] *= scale;
		}
	}

	procman_debug("dmax = %u max = %u new_max = %u", dmax, g->net.max, new_max);

	g->net.max = new_max;

	// force the graph background to be redrawn now that scale has changed
	if (g->background_buffer != NULL) {
		cairo_surface_destroy(g->background_buffer);
		g->background_buffer = NULL;
	}
}

static void
get_net (LoadGraph *g)
{
	glibtop_netlist netlist;
	char **ifnames;
	guint32 i;
	guint64 in = 0, out = 0;
	GTimeVal time;
	unsigned din, dout;
	gchar *text1;

	ifnames = glibtop_get_netlist(&netlist);

	for (i = 0; i < netlist.number; ++i)
	{
		glibtop_netload netload;
		glibtop_get_netload (&netload, ifnames[i]);

		if (netload.if_flags & (1 << GLIBTOP_IF_FLAGS_LOOPBACK))
			continue;

		/* Don't skip interfaces that are down (GLIBTOP_IF_FLAGS_UP)
		   to avoid spikes when they are brought up */

		in  += netload.bytes_in;
		out += netload.bytes_out;
	}

	g_strfreev(ifnames);

	g_get_current_time (&time);

	if (in >= g->net.last_in && out >= g->net.last_out &&
	    g->net.time.tv_sec != 0) {
		float dtime;
		dtime = time.tv_sec - g->net.time.tv_sec +
			(float) (time.tv_usec - g->net.time.tv_usec) / G_USEC_PER_SEC;
		din   = static_cast<unsigned>((in  - g->net.last_in)  / dtime);
		dout  = static_cast<unsigned>((out - g->net.last_out) / dtime);
	} else {
		/* Don't calc anything if new data is less than old (interface
		   removed, counters reset, ...) or if it is the first time */
		din  = 0;
		dout = 0;
	}

	g->net.last_in  = in;
	g->net.last_out = out;
	g->net.time     = time;

	net_scale(g, din, dout);


	gtk_label_set_text (GTK_LABEL (g->labels.net_in), procman::format_rate(din).c_str());

	text1 = SI_gnome_vfs_format_file_size_for_display (in);
	gtk_label_set_text (GTK_LABEL (g->labels.net_in_total), text1);
	g_free (text1);

	gtk_label_set_text (GTK_LABEL (g->labels.net_out), procman::format_rate(dout).c_str());

	text1 = SI_gnome_vfs_format_file_size_for_display (out);
	gtk_label_set_text (GTK_LABEL (g->labels.net_out_total), text1);
	g_free (text1);
}


/*
  Shifts data right

  data[i+1] = data[i]

  data[i] are float*, so we just move the pointer, not the data.
  But moving data loses data[n-1], so we save data[n-1] and reuse
  it as new data[0]. In fact, we rotate data[].

*/

static void
shift_right(LoadGraph *g)
{
	unsigned i;
	float* last_data;

	/* data[NUM_POINTS - 1] becomes data[0] */
	last_data = g->data[NUM_POINTS - 1];

	/* data[i+1] = data[i] */
	for(i = NUM_POINTS - 1; i != 0; --i)
		g->data[i] = g->data[i-1];

	g->data[0] = last_data;
}

/* Updates the load graph when the timeout expires */
static gboolean
load_graph_update (gpointer user_data)
{
	LoadGraph * const g = static_cast<LoadGraph*>(user_data);

	if (g->render_counter == 0) {
		shift_right(g);

		switch (g->type) {
		case LOAD_GRAPH_CPU:
			get_load(g);
			break;
		case LOAD_GRAPH_MEM:
			get_memory(g);
			break;
		case LOAD_GRAPH_NET:
			get_net(g);
			break;
		default:
			g_assert_not_reached();
		}
	}

	if (g->draw)
		load_graph_draw (g);

	g->render_counter++;

	if (g->render_counter > FRAMES)
		g->render_counter = 0;

	return TRUE;
}

static void
load_graph_unalloc (LoadGraph *g)
{
	g_free(g->data_block);

	if (g->buffer) {
		cairo_surface_destroy (g->buffer);
		g->buffer = NULL;
	}
}

static void
load_graph_alloc (LoadGraph *g)
{
	guint i, j;

	/* Allocate data in a contiguous block */
	g->data_block = g_new(float, g->n * NUM_POINTS);

	for (i = 0; i < NUM_POINTS; ++i) {
		g->data[i] = g->data_block + i * g->n;
		for (j = 0; j < g->n; ++j)
			g->data[i][j] = -1.0f;
	}
}

static gboolean
load_graph_destroy (GtkWidget *widget, gpointer data_ptr)
{
	LoadGraph * const g = static_cast<LoadGraph*>(data_ptr);

	load_graph_stop (g);

	if (g->timer_index)
		g_source_remove (g->timer_index);

	load_graph_unalloc(g);

	return FALSE;
}

LoadGraph *
load_graph_new (gint type, ProcData *procdata)
{
	LoadGraph *g;
	guint i = 0;

	g = g_new0 (LoadGraph, 1);


	g->fontsize = 8.0;
	g->rmargin = 3.5 * g->fontsize;
	g->indent = 24.0;

	g->type = type;
	switch (type) {
	case LOAD_GRAPH_CPU:
		if (procdata->config.num_cpus == 0)
			g->n = 1;
		else
			g->n = procdata->config.num_cpus;

		for(i = 0; i < G_N_ELEMENTS(g->labels.cpu); ++i)
			g->labels.cpu[i] = gtk_label_new(NULL);

		break;

	case LOAD_GRAPH_MEM:
		g->n = 2;
		g->labels.memory = gtk_label_new(NULL);
		g->labels.swap = gtk_label_new(NULL);
		break;

	case LOAD_GRAPH_NET:
		g->n = 2;
		g->net.max = 1;
		g->labels.net_in = gtk_label_new(NULL);
		g->labels.net_in_total = gtk_label_new(NULL);
		g->labels.net_out = gtk_label_new(NULL);
		g->labels.net_out_total = gtk_label_new(NULL);
		break;
	}

	g->speed  = procdata->config.graph_update_interval;

	g->colors = g_new0 (GdkColor, g->n);

	switch (type) {
	case LOAD_GRAPH_CPU:
		memcpy(g->colors, procdata->config.cpu_color,
		       g->n * sizeof g->colors[0]);
		break;
	case LOAD_GRAPH_MEM:
		g->colors[0] = procdata->config.mem_color;
		g->colors[1] = procdata->config.swap_color;
		g->mem_color_picker = gsm_color_button_new (&g->colors[0], 
							    GSMCP_TYPE_PIE);
		g->swap_color_picker = gsm_color_button_new (&g->colors[1], 
							     GSMCP_TYPE_PIE);
		break;
	case LOAD_GRAPH_NET:
		g->colors[0] = procdata->config.net_in_color;
		g->colors[1] = procdata->config.net_out_color;
		break;
	}

	g->timer_index = 0;
	g->render_counter = 10;
	g->background_buffer = NULL;
	g->graph_buffer = NULL;
	g->draw = FALSE;

	g->main_widget = gtk_vbox_new (FALSE, FALSE);
	gtk_widget_set_size_request(g->main_widget, -1, GRAPH_MIN_HEIGHT);
	gtk_widget_show (g->main_widget);

	g->disp = gtk_drawing_area_new ();
	gtk_widget_show (g->disp);
	g_signal_connect (G_OBJECT (g->disp), "expose_event",
			  G_CALLBACK (load_graph_expose), g);
	g_signal_connect (G_OBJECT(g->disp), "configure_event",
			  G_CALLBACK (load_graph_configure), g);
	g_signal_connect (G_OBJECT(g->disp), "destroy",
			  G_CALLBACK (load_graph_destroy), g);

	gtk_widget_set_events (g->disp, GDK_EXPOSURE_MASK);

	gtk_box_pack_start (GTK_BOX (g->main_widget), g->disp, TRUE, TRUE, 0);

	load_graph_alloc (g);

	gtk_widget_show_all (g->main_widget);

        /* GtkStyle gives us the theme background color, we also have
         * to trawl through parent widgets to get to GtkNotebook
         * *sigh*
         */
        g->notebook = g->main_widget;
        while (g_ascii_strncasecmp(gtk_widget_get_name(g->notebook),
                                   "GtkNotebook", 12)) {
                g->notebook = gtk_widget_get_parent(g->notebook);
                if (g->notebook == NULL) {
                        // Fall back to using the main_widget for styles
                        g->notebook = g->main_widget;
                        break;
                }
        }
        
	load_graph_start(g);
	load_graph_stop(g);
	
	return g;
}

void
load_graph_start (LoadGraph *g)
{
	if(!g->timer_index) {

		load_graph_update(g);

		g->timer_index = g_timeout_add (g->speed / FRAMES,
						load_graph_update,
						g);
	}

	g->draw = TRUE;
}

void
load_graph_stop (LoadGraph *g)
{
	/* don't draw anymore, but continue to poll */
	g->draw = FALSE;
}

void
load_graph_change_speed (LoadGraph *g,
			 guint new_speed)
{
	if (g->speed == new_speed)
		return;

	g->speed = new_speed;

	g_assert(g->timer_index);

	if(g->timer_index) {
		g_source_remove (g->timer_index);
		g->timer_index = g_timeout_add (g->speed / FRAMES,
						load_graph_update,
						g);
	}
	if (g->background_buffer != NULL) {
		cairo_surface_destroy(g->background_buffer);
		g->background_buffer = NULL;
	}
}

GdkColor*
load_graph_get_colors (LoadGraph *g)
{
	return g->colors;
}


LoadGraphLabels*
load_graph_get_labels (LoadGraph *g)
{
	return &g->labels;
}

GtkWidget*
load_graph_get_widget (LoadGraph *g)
{
	return g->main_widget;
}

GtkWidget*
load_graph_get_mem_color_picker(LoadGraph *g)
{
	return g->mem_color_picker;
}

GtkWidget*
load_graph_get_swap_color_picker(LoadGraph *g)
{
	return g->swap_color_picker;
}
