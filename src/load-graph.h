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

#endif /* _PROCMAN_LOAD_GRAPH_H_ */
