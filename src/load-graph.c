#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <config.h>
#include <gnome.h>
#include <gdk/gdkx.h>
#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include "procman.h"
#include "load-graph.h"


static GList *object_list = NULL;

#define FRAME_WIDTH GNOME_PAD_SMALL

/* Redraws the backing pixmap for the load graph and updates the window */
void
load_graph_draw (LoadGraph *g)
{
    guint i, j;
    
    if (!g->disp->window) 
    	return;
    
    g->draw_width = g->disp->allocation.width - 2 * FRAME_WIDTH;
    g->draw_height = g->disp->allocation.height - 2 * FRAME_WIDTH - 2;
    
    if (!g->pixmap)
	g->pixmap = gdk_pixmap_new (g->disp->window,
				    g->draw_width, g->draw_height,
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

	g->colors_allocated = 1;
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
    			g->draw_width,
    			g->disp->allocation.height - 2 * FRAME_WIDTH);
			
    for (i = 0; i < g->num_points; i++)
	g->pos [i] = g->draw_height;

    /* FIXME: try to do some averaging here to smooth out the graph */
    for (j = 0; j < g->n; j++) {
        float delx = (float)g->draw_width / ( g->num_points - 1);
        gdk_gc_set_foreground (g->gc, &(g->colors [j + 2]));

	for (i = 0; i < g->num_points - 1; i++) {
	    
	    gint x1 = i * delx - FRAME_WIDTH;
	    gint x2 = (i + 1) * delx - FRAME_WIDTH;
	    gint y1 = g->data[i][j] * g->draw_height - FRAME_WIDTH -1;
	    gint y2 = g->data[i+1][j] * g->draw_height - FRAME_WIDTH - 1;
	    
	    if ((g->data[i][j] != -1) && (g->data[i+1][j] != -1))
	    	gdk_draw_line (g->pixmap, g->gc,
			       g->draw_width - x2, g->pos[i + 1] - y2,
			       g->draw_width - x1, g->pos[i] - y1);
	    g->pos [i] -= g->data [i][j];
	}
	g->pos[g->num_points - 1] -= g->data [g->num_points - 1] [j];
    }
    
    
	
    gdk_draw_pixmap (g->disp->window,
		     g->disp->style->fg_gc [GTK_WIDGET_STATE(g->disp)],
		     g->pixmap,
		     0, 0,
		     0, 0,
		     g->disp->allocation.width,
		     g->disp->allocation.height);

    
}

static gchar *
get_size_string (gint size)
{
	gfloat fsize;

	fsize = (gfloat) size;
	if (fsize < 1024.0)
		return g_strdup_printf ("%d bytes", (int)fsize);
	fsize /= 1024.0;
	if (fsize < 1024.0) 
		return g_strdup_printf ("%d K", (int)fsize);
		
	fsize /= 1024.0;
	if (fsize < 1024.0)
		return g_strdup_printf ("%.0f MB", fsize);
	
	fsize /= 1024.0;
	return g_strdup_printf ("%.0f GB", fsize);

}

static void
get_load (gfloat data [2], LoadGraph *g)
{
    float usr, nice, sys, free;
    float total;
    gchar *text;

    glibtop_cpu cpu;
	
    glibtop_get_cpu (&cpu);
	
    
    g->cpu_time [0] = cpu.user;
    g->cpu_time [1] = cpu.nice;
    g->cpu_time [2] = cpu.sys;
    g->cpu_time [3] = cpu.idle;

    if (!g->cpu_initialized) {
	memcpy (g->cpu_last, g->cpu_time, sizeof (g->cpu_last));
	g->cpu_initialized = 1;
    }

    usr  = g->cpu_time [0] - g->cpu_last [0];
    nice = g->cpu_time [1] - g->cpu_last [1];
    sys  = g->cpu_time [2] - g->cpu_last [2];
    free = g->cpu_time [3] - g->cpu_last [3];

    total = usr + nice + sys + free;

    g->cpu_last [0] = g->cpu_time [0];
    g->cpu_last [1] = g->cpu_time [1];
    g->cpu_last [2] = g->cpu_time [2];
    g->cpu_last [3] = g->cpu_time [3];

    if (!total) total = 1.0;

    usr  = usr  / total;
    nice = nice / total;
    sys  = sys  / total;
    free = free / total;

    data[0] = usr + sys + nice;
    
    text = g_strdup_printf ("%.2f %s", 100.0 *(usr + sys + nice), "%");
    gtk_label_set_text (GTK_LABEL (g->label), text);
    g_free (text);
    
}

static void
get_memory (gfloat data [1], LoadGraph *g)
{
    float user, shared, buffer, free;
    float swap_used;
    gchar *text1, *text2, *text;

    glibtop_mem mem;
    glibtop_swap swap;
	
    glibtop_get_mem (&mem);
    glibtop_get_swap (&swap);
	
    user = mem.used - mem.buffer - mem.shared;
    
    swap_used = (float)swap.used / (float)swap.total;
	
    user    = user   / (float)mem.total;
    shared  = mem.shared / mem.total;
    buffer  = mem.buffer / mem.total;
    free    = mem.free / mem.total;
    
    text1 = get_size_string (mem.total);
    text2 = get_size_string (mem.used);
    text = g_strdup_printf ("%s / %s", text2, text1);
    gtk_label_set_text (GTK_LABEL (g->mem_label), text);
    g_free (text);
    g_free (text1);
    g_free (text2);
    
    text1 = get_size_string (swap.total);
    text2 = get_size_string (swap.used);
    text = g_strdup_printf ("%s / %s", text2, text1);
    gtk_label_set_text (GTK_LABEL (g->swap_label), text);
    g_free (text);
    g_free (text1);
    g_free (text2);
    
    data [0] = user;
    data [1] = swap_used;
}

/* Updates the load graph when the timeout expires */
static int
load_graph_update (LoadGraph *g)
{
    guint i, j;

    for (i = 0; i < g->num_points; i++)
	memcpy (g->odata [i], g->data [i], g->data_size);

    switch (g->type) {
    case CPU_GRAPH:
    	get_load (g->data [0], g);
    	break;
    case MEM_GRAPH:
    	get_memory (g->data [0], g);
    	break;
    }

    for (i=0; i < g->num_points-1; i++)
	for (j=0; j < g->n; j++)
	    g->data [i+1][j] = g->odata [i][j];

    if (g->draw)
    	load_graph_draw (g);
    	
    return TRUE;
}

static void
load_graph_unalloc (LoadGraph *g)
{
    int i;

    if (!g->allocated)
	return;

    for (i = 0; i < g->num_points; i++) {
	g_free (g->data [i]);
	g_free (g->odata [i]);
    }

    g_free (g->data);
    g_free (g->odata);
    g_free (g->pos);

    g->pos = NULL;
    g->data = g->odata = NULL;

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

    if (g->allocated)
	return;

    g->data = g_new0 (gfloat *, g->num_points);
    g->odata = g_new0 (gfloat *, g->num_points);
    g->pos = g_new0 (guint, g->num_points);

    g->data_size = sizeof (gfloat) * g->n;

    for (i = 0; i < g->num_points; i++) {
	g->data [i] = g_malloc0 (g->data_size);
	g->odata [i] = g_malloc0 (g->data_size);
    }
    
    for (j = 0; j < g->n; j++) {
    	for (i = 0; i < g->num_points; i++) {
    		g->data [i][j] = -1;
    	}
    }

    g->allocated = TRUE;
}

static gint
load_graph_configure (GtkWidget *widget, GdkEventConfigure *event,
		      gpointer data_ptr)
{
    LoadGraph *c = (LoadGraph *) data_ptr;

    if (!c->draw)
    	return TRUE;
    	
    if (c->pixmap) {
	gdk_pixmap_unref (c->pixmap);
	c->pixmap = NULL;
    }
    
    if (!c->pixmap)
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
    return TRUE;
    event = NULL;
}

static gint
load_graph_expose (GtkWidget *widget, GdkEventExpose *event,
		   gpointer data_ptr)
{
    LoadGraph *g = (LoadGraph *) data_ptr;
	
    gdk_draw_pixmap (widget->window,
		     widget->style->fg_gc [GTK_WIDGET_STATE(widget)],
		     g->pixmap,
		     event->area.x, event->area.y,
		     event->area.x, event->area.y,
		     event->area.width, event->area.height);
    return FALSE;
}

static void
load_graph_destroy (GtkWidget *widget, gpointer data_ptr)
{
    LoadGraph *g = (LoadGraph *) data_ptr;

    load_graph_stop (g);

    object_list = g_list_remove (object_list, g);
    
    if (g->timer_index != -1)
    	gtk_timeout_remove (g->timer_index);
    
    return;
    widget = NULL;
}

LoadGraph *
load_graph_new (gint type, ProcData *procdata)
{
    LoadGraph *g;

    g = g_new0 (LoadGraph, 1);

    g->type = type;
    switch (type) {
    case CPU_GRAPH:
    	g->n = 1;
    	break;
    case MEM_GRAPH:
    	g->n = 2;
    	break;
    }
	
    g->speed  = 500;
    g->num_points = 200;
    
    g->colors = g_new0 (GdkColor, g->n + 2);

    g->colors[0] = procdata->config.bg_color;
    g->colors[1] = procdata->config.frame_color;
    switch (type) {
    case CPU_GRAPH:
    	g->colors[2] = procdata->config.cpu_color;
    	break;
    case MEM_GRAPH:
    	g->colors[2] = procdata->config.mem_color;
    	g->colors[3] = procdata->config.swap_color;
    	break;
    }
    	    
    g->timer_index = -1;
    
    g->draw = FALSE;
	
    g->main_widget = gtk_vbox_new (FALSE, FALSE);
    gtk_widget_show (g->main_widget);

    g->disp = gtk_drawing_area_new ();
    gtk_widget_show (g->disp);
    gtk_signal_connect (GTK_OBJECT (g->disp), "expose_event",
			(GtkSignalFunc)load_graph_expose, g);
    gtk_signal_connect (GTK_OBJECT(g->disp), "configure_event",
			(GtkSignalFunc)load_graph_configure, g);
    gtk_signal_connect (GTK_OBJECT(g->disp), "destroy",
			(GtkSignalFunc)load_graph_destroy, g);
    			
    gtk_widget_set_events (g->disp, GDK_EXPOSURE_MASK);

    gtk_box_pack_start (GTK_BOX (g->main_widget), g->disp, TRUE, TRUE, 0);
    
    load_graph_alloc (g);
    
    object_list = g_list_append (object_list, g);

    gtk_widget_show_all (g->main_widget);
    
    g->timer_index = gtk_timeout_add (g->speed,
				      (GtkFunction) load_graph_update, g);

    return g;
}

void
load_graph_start (LoadGraph *g)
{
    if (g->timer_index == -1)
        g->timer_index = gtk_timeout_add (g->speed,
				      (GtkFunction) load_graph_update, g);
				      
    g->draw = TRUE;
}

void
load_graph_stop (LoadGraph *g)
{
    /*if (g->timer_index != -1)
	gtk_timeout_remove (g->timer_index);
    
    g->timer_index = -1;*/
    g->draw = FALSE;
}
