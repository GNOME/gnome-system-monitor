#ifndef LOAD_GRAPH_H__
#define LOAD_GRAPH_H__

#include "procman.h"

enum
{
	CPU_GRAPH,
	MEM_GRAPH,
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

#endif
