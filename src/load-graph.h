#ifndef _PROCMAN_LOAD_GRAPH_H_
#define _PROCMAN_LOAD_GRAPH_H_

#include "procman.h"

enum
{
	LOAD_GRAPH_CPU,
	LOAD_GRAPH_MEM
};


/* Create new load graph. */
LoadGraph *
load_graph_new (gint type, ProcData *procdata) G_GNUC_INTERNAL;

/* Force a drawing update */
void
load_graph_draw (LoadGraph *g) G_GNUC_INTERNAL;

/* Start load graph. */
void
load_graph_start (LoadGraph *g) G_GNUC_INTERNAL;

/* Stop load graph. */
void
load_graph_stop (LoadGraph *g) G_GNUC_INTERNAL;

#endif /* _PROCMAN_LOAD_GRAPH_H_ */
