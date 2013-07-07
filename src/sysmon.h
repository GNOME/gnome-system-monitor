#ifndef __SMONCPU_H__
#define __SMONCPU_H__

#include <gdk/gdkx.h>
#include <glib.h>

#include "uber-line-graph.h"

typedef struct
{
	guint       len;
	gdouble    *total;
	gdouble    *freq;
	glong      *last_user;
	glong      *last_idle;
	glong      *last_system;
	glong      *last_nice;
	GtkWidget **labels;
} CpuInfo;

typedef struct
{
	gdouble total_in;
	gdouble total_out;
	gdouble last_total_in;
	gdouble last_total_out;
} NetInfo;

extern CpuInfo      cpu_info;
extern NetInfo      net_info;

gboolean smon_cpu_has_freq_scaling (gint cpu);

gdouble
smon_get_cpu_info (UberLineGraph *graph,     /* IN */
              guint          line,      /* IN */
              gpointer       user_data);

gdouble
smon_get_net_info (UberLineGraph *graph,     /* IN */
              guint          line,      /* IN */
              gpointer       user_data);

void
smon_next_cpu_info (void);

void
smon_next_cpu_freq_info (void);

void
smon_next_net_info (void);

#endif /* __SMONCPU_H__ */
