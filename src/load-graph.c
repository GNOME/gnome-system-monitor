#include <config.h>
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

#include <libgnomevfs/gnome-vfs-utils.h>

#include "procman.h"
#include "load-graph.h"
#include "util.h"


struct _LoadGraph {

	guint n;
	gint type;
	guint speed;
	guint draw_width, draw_height;
	guint num_points;
	guint num_cpus;

	guint allocated;

	GdkColor *colors;
	gfloat **data;
	guint data_size;

	gboolean colors_allocated;
	GtkWidget *main_widget;
	GtkWidget *disp;

	cairo_surface_t *buffer;

	guint timer_index;

	gboolean draw;

	LoadGraphLabels labels;

	guint64 cpu_time [GLIBTOP_NCPU] [NCPUSTATES];
	guint64 cpu_last [GLIBTOP_NCPU] [NCPUSTATES];
	gboolean cpu_initialized;

	guint64 net_last_in, net_last_out;
	float net_max;
	GTimeVal net_time;
};


#define FRAME_WIDTH 4


/* Redraws the backing buffer for the load graph and updates the window */
void
load_graph_draw (LoadGraph *g)
{
	cairo_t *cr;
	int dely;
	double delx;
	int real_draw_height;
	gint i, j;

	/* Allocate colors. */
	if (!g->colors_allocated) {
		GdkColormap *colormap;

		colormap = gdk_window_get_colormap (g->disp->window);
		for (i = 0; i < g->n + 2; i++) {
			gdk_color_alloc (colormap, &(g->colors [i]));
		}

		g->colors_allocated = TRUE;
	}

	cr = cairo_create (g->buffer);

	/* clear */
	cairo_set_source_rgb (cr, 0., 0., 0.);
	cairo_paint (cr);

	/* draw frame */
	cairo_translate (cr, FRAME_WIDTH, FRAME_WIDTH);
	gdk_cairo_set_source_color (cr, &(g->colors [1]));
	cairo_set_line_width (cr, 1.0);

	dely = (g->draw_height) / 5; /* round to int to avoid AA blur */
	real_draw_height = dely * 5;

	for (i = 0; i <= 5; ++i) {
		cairo_move_to (cr, 0.5, i * dely + 0.5);
		cairo_line_to (cr, g->draw_width - 0.5, i * dely + 0.5);
	}

	cairo_move_to (cr, 0.5, 0.5);
	cairo_line_to (cr, 0.5, real_draw_height + 0.5);

	cairo_move_to (cr, g->draw_width - 0.5, 0.5);
	cairo_line_to (cr, g->draw_width - 0.5, real_draw_height + 0.5);

	cairo_stroke (cr);

	/* draw the graph */
	cairo_set_line_width (cr, 2.0);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);

	delx = (double)(g->draw_width) / (double)g->num_points;

	for (j = 0; j < g->n; ++j) {
		cairo_move_to (cr,
			       g->draw_width - 1,
			       (1 - g->data[0][j]) * real_draw_height);
		gdk_cairo_set_source_color (cr, &(g->colors [j + 2]));

		for (i = 1; i < g->num_points - 1; ++i) {
			if (g->data[i][j] == -1)
				continue;

			cairo_line_to (cr,
				       g->draw_width -1 - i * delx,
				       (1 - g->data[i][j]) * real_draw_height);
		}

		cairo_stroke (cr);
	}

	cairo_destroy (cr);

	/* repaint */
	gtk_widget_queue_draw (g->disp);
}

static gboolean
load_graph_configure (GtkWidget *widget,
		      GdkEventConfigure *event,
		      gpointer data_ptr)
{
	LoadGraph * const g = data_ptr;
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

	load_graph_draw (g);

	return TRUE;
}

static gboolean
load_graph_expose (GtkWidget *widget,
		   GdkEventExpose *event,
		   gpointer data_ptr)
{
	LoadGraph * const g = data_ptr;
	cairo_t *cr;

	cr = gdk_cairo_create(widget->window);

	cairo_set_source_surface(cr, g->buffer, 0, 0);
	cairo_rectangle(cr,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	cairo_fill(cr);

	cairo_destroy(cr);

	return TRUE;
}

static void
get_load (gfloat data [2], LoadGraph *g)
{
	guint i;

	glibtop_cpu cpu;

	glibtop_get_cpu (&cpu);

	if (g->n == 1) {
	  g->cpu_time [0][0] = cpu.user;
	  g->cpu_time [0][1] = cpu.nice;
	  g->cpu_time [0][2] = cpu.sys;
	  g->cpu_time [0][3] = cpu.idle;
	  g->cpu_time [0][4] = cpu.iowait + cpu.irq + cpu.softirq;
	} else {
	  for (i=0; i<g->n; i++) {
	    g->cpu_time [i][0] = cpu.xcpu_user[i];
	    g->cpu_time [i][1] = cpu.xcpu_nice[i];
	    g->cpu_time [i][2] = cpu.xcpu_sys[i];
	    g->cpu_time [i][3] = cpu.xcpu_idle[i];
	    g->cpu_time [0][4] = cpu.xcpu_iowait[i]
		    + cpu.xcpu_irq[i] + cpu.xcpu_softirq[i];
	  }
	}

	if (!g->cpu_initialized) {
		for (i=0; i<g->n; i++) {
			memcpy(g->cpu_last[i], g->cpu_time[i],
			       sizeof g->cpu_last[i]);

			data[i] = -1;
		}
		g->cpu_initialized = TRUE;
		return;
	}

	for (i=0; i<g->n; i++) {
		float load;
		float usr, nice, sys, free, iowait;
		float total;
		gchar *text;

		usr  = g->cpu_time [i][0] - g->cpu_last [i][0];
		nice = g->cpu_time [i][1] - g->cpu_last [i][1];
		sys  = g->cpu_time [i][2] - g->cpu_last [i][2];
		free = g->cpu_time [i][3] - g->cpu_last [i][3];
		iowait = g->cpu_time[i][4] - g->cpu_last[i][4];
		memcpy(g->cpu_last [i], g->cpu_time [i], sizeof g->cpu_last[i]);

		total = MAX(usr + nice + sys + free + iowait, 1.0f);

		usr  = usr  / total;
		nice = nice / total;
		sys  = sys  / total;
		free = free / total;

		data[i] = usr + sys + nice;

		load = CLAMP(100.0f*data[i], 0.0f, 100.0f);

		text = g_strdup_printf ("%.1f%%", load);
		gtk_label_set_text (GTK_LABEL (g->labels.cpu[i]), text);
		g_free (text);
	}
}

static void
get_memory (gfloat data [1], LoadGraph *g)
{
	float mempercent, swappercent;
	gchar *text1, *text2, *text3;

	glibtop_mem mem;
	glibtop_swap swap;

	glibtop_get_mem (&mem);
	glibtop_get_swap (&swap);

	/* There's no swap on LiveCD : 0.0f is better than NaN :) */
	swappercent = (swap.total ? (float)swap.used / (float)swap.total : 0.0f);
	mempercent  = (float)mem.user  / (float)mem.total;

	text1 = SI_gnome_vfs_format_file_size_for_display (mem.total);
	text2 = SI_gnome_vfs_format_file_size_for_display (mem.user);
	text3 = g_strdup_printf ("  %.1f %%", mempercent * 100.0f);
	gtk_label_set_text (GTK_LABEL (g->labels.memused), text2);
	gtk_label_set_text (GTK_LABEL (g->labels.memtotal), text1);
	gtk_label_set_text (GTK_LABEL (g->labels.mempercent), text3);
	g_free (text1);
	g_free (text2);
	g_free (text3);

	text1 = SI_gnome_vfs_format_file_size_for_display (swap.total);
	text2 = SI_gnome_vfs_format_file_size_for_display (swap.used);
	text3 = g_strdup_printf ("  %.1f %%", swappercent * 100.0f);
	gtk_label_set_text (GTK_LABEL (g->labels.swapused), text2);
	gtk_label_set_text (GTK_LABEL (g->labels.swaptotal), text1);
	gtk_label_set_text (GTK_LABEL (g->labels.swappercent), text3);
	g_free (text1);
	g_free (text2);
	g_free (text3);

	data [0] = mempercent;
	data [1] = swappercent;
}

static void
net_scale (LoadGraph *g, float new_max)
{
	gint i;
	float old_max = MAX (g->net_max, 1.0f), scale;

	if (new_max <= old_max)
		return;

	scale = old_max / new_max;

	for (i = 0; i < g->num_points; i++) {
		if (g->data[i][0] >= 0.0f) {
			g->data[i][0] *= scale;
			g->data[i][1] *= scale;
		}
	}

	g->net_max = new_max;
}

static void
get_net (float data [2], LoadGraph *g)
{
	glibtop_netlist netlist;
	char **ifnames;
	guint32 i;
	guint64 in = 0, out = 0;
	GTimeVal time;
	float din, dout;
	gchar *text1, *text2;

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

	if (in >= g->net_last_in && out >= g->net_last_out &&
	    g->net_time.tv_sec != 0) {
		float dtime;
		dtime = time.tv_sec - g->net_time.tv_sec +
			(float) (time.tv_usec - g->net_time.tv_usec) / G_USEC_PER_SEC;
		din   = (in - g->net_last_in) / dtime;
		dout  = (out - g->net_last_out) / dtime;
	} else {
		/* Don't calc anything if new data is less than old (interface
		   removed, counters reset, ...) or if it is the first time */
		din  = 0.0f;
		dout = 0.0f;
	}

	g->net_last_in  = in;
	g->net_last_out = out;
	g->net_time     = time;

	data [0] = din / MAX (g->net_max, 1.0f);
	data [1] = dout / MAX (g->net_max, 1.0f);

	net_scale (g, MAX (din, dout));

	text1 = SI_gnome_vfs_format_file_size_for_display (din);
	text2 = g_strdup_printf (_("%s/s"), text1);
	gtk_label_set_text (GTK_LABEL (g->labels.net_in), text2);
	g_free (text1);
	g_free (text2);

	text1 = SI_gnome_vfs_format_file_size_for_display (in);
	gtk_label_set_text (GTK_LABEL (g->labels.net_in_total), text1);
	g_free (text1);

	text1 = SI_gnome_vfs_format_file_size_for_display (dout);
	text2 = g_strdup_printf (_("%s/s"), text1);
	gtk_label_set_text (GTK_LABEL (g->labels.net_out), text2);
	g_free (text1);
	g_free (text2);

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

	/* data[g->num_points - 1] becomes data[0] */
	last_data = g->data[g->num_points - 1];

	/* data[i+1] = data[i] */
	for(i = g->num_points - 1; i != 0; --i)
		g->data[i] = g->data[i-1];

	g->data[0] = last_data;
}

/* Updates the load graph when the timeout expires */
static gboolean
load_graph_update (gpointer user_data)
{
	LoadGraph * const g = user_data;

	shift_right(g);

	switch (g->type) {
	case LOAD_GRAPH_CPU:
		get_load (g->data[0], g);
		break;
	case LOAD_GRAPH_MEM:
		get_memory (g->data [0], g);
		break;
	case LOAD_GRAPH_NET:
		get_net (g->data[0], g);
		break;
	}

	if (g->draw)
		load_graph_draw (g);

	return TRUE;
}

static void
load_graph_unalloc (LoadGraph *g)
{
	int i;

	g_assert(g->allocated);

	for (i = 0; i < g->num_points; i++) {
		g_free (g->data [i]);
	}

	g_free (g->data);

	g->data = NULL;

	if (g->buffer) {
		cairo_surface_destroy (g->buffer);
		g->buffer = NULL;
	}

	g->allocated = FALSE;
}

static void
load_graph_alloc (LoadGraph *g)
{
	int i, j;

	g_assert(!g->allocated);

	g->data = g_new (gfloat *, g->num_points);

	g->data_size = sizeof (gfloat) * g->n;

	for (i = 0; i < g->num_points; i++) {

		g->data [i] = g_malloc (g->data_size);
		for(j = 0; j < g->n; j++) g->data [i][j] = -1;
	}

	g->allocated = TRUE;
}

static gboolean
load_graph_destroy (GtkWidget *widget, gpointer data_ptr)
{
	LoadGraph * const g = data_ptr;

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
	gint i = 0;

	g = g_new0 (LoadGraph, 1);

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
		g->labels.memused = gtk_label_new(NULL);
		g->labels.memtotal = gtk_label_new(NULL);
		g->labels.mempercent = gtk_label_new(NULL);
		g->labels.swapused = gtk_label_new(NULL);
		g->labels.swaptotal = gtk_label_new(NULL);
		g->labels.swappercent = gtk_label_new(NULL);
		break;

	case LOAD_GRAPH_NET:
		g->n = 2;
		g->labels.net_in = gtk_label_new(NULL);
		g->labels.net_in_total = gtk_label_new(NULL);
		g->labels.net_out = gtk_label_new(NULL);
		g->labels.net_out_total = gtk_label_new(NULL);
		break;
	}

	g->speed  = procdata->config.graph_update_interval;
	g->num_points = 100;

	g->colors = g_new0 (GdkColor, g->n + 2);

	g->colors[0] = procdata->config.bg_color;
	g->colors[1] = procdata->config.frame_color;
	switch (type) {
	case LOAD_GRAPH_CPU:
		for (i=0;i<g->n;i++)
			g->colors[2+i] = procdata->config.cpu_color[i];
		break;
	case LOAD_GRAPH_MEM:
		g->colors[2] = procdata->config.mem_color;
		g->colors[3] = procdata->config.swap_color;
		break;
	case LOAD_GRAPH_NET:
		g->colors[2] = procdata->config.net_in_color;
		g->colors[3] = procdata->config.net_out_color;
		break;
	}

	g->timer_index = 0;

	g->draw = FALSE;

	g->main_widget = gtk_vbox_new (FALSE, FALSE);
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

	return g;
}

void
load_graph_start (LoadGraph *g)
{
	if(!g->timer_index) {

		load_graph_update(g);

		g->timer_index = g_timeout_add (g->speed,
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
	g->speed = new_speed;

	if(g->timer_index) {
		g_source_remove (g->timer_index);
		g->timer_index = g_timeout_add (g->speed,
						load_graph_update,
						g);
	}
}

GdkColor*
load_graph_get_colors (LoadGraph *g)
{
	return g->colors;
}

void
load_graph_reset_colors (LoadGraph *g)
{
	g->colors_allocated = FALSE;
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
