#ifndef LOAD_GRAPH_H__
#define LOAD_GRAPH_H__


enum
{
	CPU_GRAPH,
	MEM_GRAPH,
};

typedef struct _LoadGraph LoadGraph;

typedef void (*LoadGraphDataFunc) (int, int [], LoadGraph *);

#define NCPUSTATES 4

struct _LoadGraph {
    
    guint n;
    gint type;
    guint speed, size;
    guint draw_width, draw_height;
    guint num_points;

    guint allocated;

    GdkColor *colors;
    gfloat **data, **odata;
    guint data_size;
    guint *pos;

    gint colors_allocated;
    GtkWidget *main_widget;
    GtkWidget *disp;
    GtkWidget *label;
    GdkPixmap *pixmap;
    GdkGC *gc;
    int timer_index;
    
    gboolean draw;

    gint show_frame;

    long cpu_time [NCPUSTATES];
    long cpu_last [NCPUSTATES];
    int cpu_initialized;       
};

/* Create new load graph. */
LoadGraph *
load_graph_new (gint type);

/* Start load graph. */
void
load_graph_start (LoadGraph *g);

/* Stop load graph. */
void
load_graph_stop (LoadGraph *g);

#endif
