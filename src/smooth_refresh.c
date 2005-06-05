#include <config.h>

#include <sys/types.h>
#include <unistd.h>

#include <glib.h>

#include <glibtop.h>
#include <glibtop/proctime.h>
#include <glibtop/cpu.h>

#include "smooth_refresh.h"



/*
  -self : procman's PID (so we call getpid() only once)

  -interval : current refresh interval

  -config_interval : pointer to the configuration refresh interval.
		     Used to watch configuration changes

  -interval >= -config_interval

  -last_pcpu : to avoid spikes, the last CPU%. See PCPU_{LO,HI}
	       Storead as float for more precision (where per-process pcpu
	       is just a guint)

  -last_total_time:
  -last_cpu_time: Save last cpu and process times to compute CPU%
*/


struct _SmoothRefresh
{
	pid_t self;

	guint interval;

	const guint *config_interval;

	float last_pcpu;

	guint64 last_total_time;
	guint64 last_cpu_time;
};



/*
  fuzzy logic:
   - decrease refresh interval only if current CPU% and last CPU%
     are higher than PCPU_LO
   - increase refresh interval only if current CPU% and last CPU%
     are higher than PCPU_HI

*/

enum
{
	PCPU_HI = 10,
	PCPU_LO = 8
};




static float
get_own_cpu_usage(SmoothRefresh *sm)
{
	glibtop_cpu cpu;
	glibtop_proc_time proctime;
	guint64 elapsed;
	float usage;

	glibtop_get_cpu (&cpu);
	elapsed = cpu.total - sm->last_total_time;

	glibtop_get_proc_time (&proctime, sm->self);
	usage = (proctime.rtime - sm->last_cpu_time) * 100.0f / elapsed;
	usage = CLAMP(usage, 0.0f, 100.0f);

	sm->last_total_time = cpu.total;
	sm->last_cpu_time = proctime.rtime;

	return usage;
}



SmoothRefresh*
smooth_refresh_new(const guint * config_interval)
{
	SmoothRefresh *sm;

	sm = g_new(SmoothRefresh, 1);
	sm->config_interval = config_interval;

	smooth_refresh_reset(sm);

	return sm;
}



void
smooth_refresh_reset(SmoothRefresh *sm)
{
	glibtop_cpu cpu;
	glibtop_proc_time proctime;

	sm->self = getpid();

	glibtop_get_cpu(&cpu);
	glibtop_get_proc_time(&proctime, sm->self);

	sm->interval = *sm->config_interval;
	sm->last_pcpu = PCPU_LO;
	sm->last_total_time = cpu.total;
	sm->last_cpu_time = proctime.rtime;
}



void
smooth_refresh_destroy(SmoothRefresh *sm)
{
	g_free(sm);
}



gboolean
smooth_refresh_get(SmoothRefresh *sm, guint *new_interval)
{
	gboolean changed;
	float pcpu;

	pcpu = get_own_cpu_usage(sm);
/*
  invariant: interval >= config_interval >= 1000

  i see 3 cases:

	a) interval is too big (CPU usage < 10%)
		-> increase interval

	b) interval is too small (CPU usage > 10%) AND interval != config_interval
								>
		-> decrease interval

        c) interval is config_interval (start or interval is perfect)

*/
	g_assert(sm->interval >= *sm->config_interval);

	if(pcpu > PCPU_HI
	   && sm->last_pcpu > PCPU_HI)
	{
		*new_interval = sm->interval * 1.1f;
	}
	else if(sm->interval != *sm->config_interval
		&& pcpu < PCPU_LO
		&& sm->last_pcpu < PCPU_LO)
	{
		*new_interval = MAX(*sm->config_interval, sm->interval * 0.9f);
	}
	else
	{
		*new_interval = sm->interval;
	}


	changed = sm->interval != *new_interval;

	if(changed)
	{
		sm->interval = *new_interval;
	}

	sm->last_pcpu = pcpu;


	if(changed) {
		printf("CPU %3.1f%% current %u (config %u)\n",
		       sm->last_pcpu,
		       sm->interval,
		       *sm->config_interval);
	}

	g_assert(sm->interval == *new_interval);
	g_assert(sm->interval >= *sm->config_interval);

	return changed;
}

