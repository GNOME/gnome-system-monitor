#include <config.h>

#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <gconf/gconf-client.h>
#include <glibtop.h>
#include <glibtop/proctime.h>
#include <glibtop/cpu.h>

#include "smooth_refresh.h"
#include "procman.h"
#include "util.h"

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
	gboolean active;
	guint connection;

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



static void status_changed(GConfClient *client,
			   guint cnxn_id,
			   GConfEntry *entry,
			   gpointer user_data)
{
	SmoothRefresh *sm = static_cast<SmoothRefresh *>(user_data);
	GConfValue *value = gconf_entry_get_value(entry);

	if (value)
		sm->active = gconf_value_get_bool(value);
	else
		sm->active = SMOOTH_REFRESH_KEY_DEFAULT;

	smooth_refresh_reset(sm);

	if (sm->active)
		procman_debug("smooth_refresh is enabled");
}


SmoothRefresh*
smooth_refresh_new(const guint * config_interval)
{
	SmoothRefresh *sm;
	GConfValue *value;

	sm = g_new(SmoothRefresh, 1);
	sm->config_interval = config_interval;

	smooth_refresh_reset(sm);
	value = gconf_client_get_without_default(gconf_client_get_default(),
						 SMOOTH_REFRESH_KEY,
						 NULL);

	if (value) {
		sm->active = gconf_value_get_bool(value);
		gconf_value_free(value);
	} else
		sm->active = SMOOTH_REFRESH_KEY_DEFAULT;

	sm->connection = gconf_client_notify_add(gconf_client_get_default(),
						 SMOOTH_REFRESH_KEY,
						 status_changed,
						 sm,
						 NULL,
						 NULL);

	if (sm->active)
		procman_debug("smooth_refresh is enabled");

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
	if (sm->connection)
		gconf_client_notify_remove(gconf_client_get_default(),
					   sm->connection);
	g_free(sm);
}



gboolean
smooth_refresh_get(SmoothRefresh *sm, guint *new_interval)
{
	gboolean changed;
	float pcpu;

	if (!sm->active)
		return FALSE;

	pcpu = get_own_cpu_usage(sm);
/*
  invariant: MAX_UPDATE_INTERVAL >= interval >= config_interval >= MIN_UPDATE_INTERVAL

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

	*new_interval = CLAMP(*new_interval,
			      MIN_UPDATE_INTERVAL,
			      MAX_UPDATE_INTERVAL);

	changed = sm->interval != *new_interval;

	if(changed)
	{
		sm->interval = *new_interval;
	}

	sm->last_pcpu = pcpu;


	if(changed) {
		procman_debug("CPU usage is %3.1f%%, changed refresh_interval to %u (config %u)",
			      sm->last_pcpu,
			      sm->interval,
			      *sm->config_interval);
	}

	g_assert(sm->interval == *new_interval);
	g_assert(sm->interval >= *sm->config_interval);

	return changed;
}

