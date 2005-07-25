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
	guint *pos;

	gboolean colors_allocated;
	GtkWidget *main_widget;
	GtkWidget *disp;

	GdkPixmap *pixmap;
	GdkGC *gc;
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



/* Redraws the backing pixmap for the load graph and updates the window */
void
load_graph_draw (LoadGraph *g)
{
	guint i, j;
	gfloat dely;

	if (!g->disp->window)
		return;

	g->draw_width = g->disp->allocation.width - 2 * FRAME_WIDTH;
	g->draw_height = g->disp->allocation.height - 2 * FRAME_WIDTH;

	if (!g->pixmap)
		g->pixmap = gdk_pixmap_new (g->disp->window,
					    g->disp->allocation.width,
					    g->disp->allocation.height,
					    gtk_widget_get_visual (g->disp)->depth);

/* Create GC if necessary. */
	if (!g->gc) {
		g->gc = gdk_gc_new (g->disp->window);
		gdk_gc_copy (g->gc, g->disp->style->white_gc);
	}

/* Allocate colors. */
	if (!g->colors_allocated) {
		GdkColormap *colormap;

		colormap = gdk_window_get_colormap (g->disp->window);
		for (i = 0; i < g->n + 2; i++) {
			gdk_color_alloc (colormap, &(g->colors [i]));
		}

		g->colors_allocated = TRUE;
	}

/* Erase Rectangle */
	gdk_gc_set_foreground (g->gc, &(g->colors [0]));
	gdk_draw_rectangle (g->pixmap,
			    g->gc,
			    TRUE, 0, 0,
			    g->disp->allocation.width,
			    g->disp->allocation.height);

/* draw frame */
	gdk_gc_set_foreground (g->gc, &(g->colors [1]));
	gdk_draw_rectangle (g->pixmap,
			    g->gc,
			    FALSE, FRAME_WIDTH, FRAME_WIDTH,
			    g->draw_width - 1,
			    g->draw_height - 1);

	dely = (gfloat)(g->draw_height - 1) / 5;
	for (i = 1; i <5; i++) {
		gint y1 = FRAME_WIDTH + i * dely;
		gdk_draw_line (g->pixmap, g->gc,
			       FRAME_WIDTH, y1, FRAME_WIDTH + g->draw_width - 1, y1);
	}

	for (i = 0; i < g->num_points; i++)
		g->pos [i] = g->draw_height;

	gdk_gc_set_line_attributes (g->gc, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER );
/* FIXME: try to do some averaging here to smooth out the graph */
	for (j = 0; j < g->n; j++) {
		gfloat delx = (gfloat)(g->draw_width - 1) / (g->num_points - 1);
		gdk_gc_set_foreground (g->gc, &(g->colors [j + 2]));

		for (i = 0; i < g->num_points - 1; i++) {

			gint x1 = g->draw_width - i * delx + FRAME_WIDTH - 1;
			gint x2 = g->draw_width - (i + 1) * delx + FRAME_WIDTH - 1;
			gint y1 = g->pos[i] - g->data[i][j] * (g->draw_height - 1) +
				  FRAME_WIDTH - 1;
			gint y2 = g->pos[i+1] - g->data[i+1][j] * (g->draw_height - 1) +
				  FRAME_WIDTH - 1;

			if ((g->data[i][j] != -1) && (g->data[i+1][j] != -1))
				gdk_draw_line (g->pixmap, g->gc, x2, y2, x1, y1);

			g->pos [i] -= g->data [i][j];
		}
		g->pos[g->num_points - 1] -= g->data [g->num_points - 1] [j];
	}

	gdk_gc_set_line_attributes (g->gc, 1, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER );

	gdk_draw_pixmap (g->disp->window,
			 g->disp->style->fg_gc [GTK_WIDGET_STATE(g->disp)],
			 g->pixmap,
			 0, 0,
			 0, 0,
			 g->disp->allocation.width,
			 g->disp->allocation.height);


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
	} else {
	  for (i=0; i<g->n; i++) {
	    g->cpu_time [i][0] = cpu.xcpu_user[i];
	    g->cpu_time [i][1] = cpu.xcpu_nice[i];
	    g->cpu_time [i][2] = cpu.xcpu_sys[i];
	    g->cpu_time [i][3] = cpu.xcpu_idle[i];
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
		float usr, nice, sys, free;
		float total;
		gchar *text;

		usr  = g->cpu_time [i][0] - g->cpu_last [i][0];
		nice = g->cpu_time [i][1] - g->cpu_last [i][1];
		sys  = g->cpu_time [i][2] - g->cpu_last [i][2];
		free = g->cpu_time [i][3] - g->cpu_last [i][3];

		memcpy(g->cpu_last [i], g->cpu_time [i], sizeof g->cpu_last[i]);

		total = MAX(usr + nice + sys + free, 1.0f);

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

	g_return_if_fail(g->allocated);

	for (i = 0; i < g->num_points; i++) {
		g_free (g->data [i]);
	}

	g_free (g->data);
	g_free (g->pos);

	g->pos = NULL;
	g->data = NULL;

	if (g->pixmap) {
		gdk_pixmap_unref (g->pixmap);
		g->pixmap = NULL;
	}

	g->allocated = FALSE;
}

static void
load_graph_alloc (LoadGraph *g)
{
	int i, j;

	g_return_if_fail(!g->allocated);

	g->data = g_new (gfloat *, g->num_points);
	g->pos = g_new0 (guint, g->num_points);

	g->data_size = sizeof (gfloat) * g->n;

	for (i = 0; i < g->num_points; i++) {

		g->data [i] = g_malloc (g->data_size);
		for(j = 0; j < g->n; j++) g->data [i][j] = -1;
	}

	g->allocated = TRUE;
}


static gboolean
load_graph_configure (GtkWidget *widget, GdkEventConfigure *event,
		      gpointer data_ptr)
{
	LoadGraph * const c = data_ptr;

	if (c->pixmap) {
		gdk_pixmap_unref (c->pixmap);
	}

	c->pixmap = gdk_pixmap_new (widget->window,
				    widget->allocation.width,
				    widget->allocation.height,
				    gtk_widget_get_visual (c->disp)->depth);

	gdk_draw_rectangle (c->pixmap,
			    widget->style->black_gc,
			    TRUE, 0,0,
			    widget->allocation.width,
			    widget->allocation.height);
	gdk_draw_pixmap (widget->window,
			 c->disp->style->fg_gc [GTK_WIDGET_STATE(widget)],
			 c->pixmap,
			 0, 0,
			 0, 0,
			 c->disp->allocation.width,
			 c->disp->allocation.height);

	load_graph_draw (c);

	return TRUE;
}


static gboolean
load_graph_expose (GtkWidget *widget, GdkEventExpose *event,
		   gpointer data_ptr)
{
	LoadGraph * const g = data_ptr;

	gdk_draw_pixmap (widget->window,
			 widget->style->fg_gc [GTK_WIDGET_STATE(widget)],
			 g->pixmap,
			 event->area.x, event->area.y,
			 event->area.x, event->area.y,
			 event->area.width, event->area.height);
	return FALSE;
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
