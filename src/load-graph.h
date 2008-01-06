#ifndef _PROCMAN_LOAD_GRAPH_H_
#define _PROCMAN_LOAD_GRAPH_H_

#include <glib/gtypes.h>
#include <glibtop/cpu.h>

struct LoadGraph;
typedef struct _LoadGraphLabels LoadGraphLabels;

#include "procman.h"

enum
{
	LOAD_GRAPH_CPU,
	LOAD_GRAPH_MEM,
	LOAD_GRAPH_NET
};


struct _LoadGraphLabels
{
	GtkWidget *cpu[GLIBTOP_NCPU];
	GtkWidget *memory;
	GtkWidget *swap;
	GtkWidget *net_in;
	GtkWidget *net_in_total;
	GtkWidget *net_out;
	GtkWidget *net_out_total;
};


/* Create new load graph. */
LoadGraph *
load_graph_new (gint type, ProcData *procdata);

/* Force a drawing update */
void
load_graph_draw (LoadGraph *g);

/* Start load graph. */
void
load_graph_start (LoadGraph *g);

/* Stop load graph. */
void
load_graph_stop (LoadGraph *g);

/* Change load graph speed and restart it if it has been previously started */
void
load_graph_change_speed (LoadGraph *g,
			 guint new_speed);

GdkColor*
load_graph_get_colors (LoadGraph *g) G_GNUC_CONST;

LoadGraphLabels*
load_graph_get_labels (LoadGraph *g) G_GNUC_CONST;


GtkWidget*
load_graph_get_widget (LoadGraph *g) G_GNUC_CONST;

GtkWidget*
load_graph_get_mem_color_picker(LoadGraph *g) G_GNUC_CONST;

GtkWidget*
load_graph_get_swap_color_picker(LoadGraph *g) G_GNUC_CONST;


#endif /* _PROCMAN_LOAD_GRAPH_H_ */
