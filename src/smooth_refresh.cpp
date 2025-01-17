#include <config.h>

#include <glib.h>

#include <glibtop.h>
#include <glibtop/proctime.h>
#include <glibtop/cpu.h>

#include "smooth_refresh.h"
#include "application.h"
#include "settings-keys.h"
#include "update_interval.h"
#include "util.h"


const string SmoothRefresh::KEY (GSM_SETTING_SMOOTH_REFRESH);



unsigned
SmoothRefresh::get_own_cpu_usage ()
{
  glibtop_cpu cpu;
  glibtop_proc_time proctime;
  guint64 elapsed;
  unsigned usage = PCPU_LO;

  glibtop_get_cpu (&cpu);
  elapsed = cpu.total - this->last_total_time;

  if (elapsed)     // avoid division by 0
    {
      glibtop_get_proc_time (&proctime, getpid ());
      usage = (proctime.rtime - this->last_cpu_time) * 100 / elapsed;
      this->last_cpu_time = proctime.rtime;
    }

  usage = CLAMP (usage, 0, 100);

  this->last_total_time = cpu.total;

  return usage;
}

void
SmoothRefresh::load_settings_value (Glib::ustring key)
{
  this->active = this->settings->get_boolean (key);

  if (this->active)
    procman_debug ("smooth_refresh is enabled");
}


SmoothRefresh::SmoothRefresh(Glib::RefPtr<Gio::Settings> a_settings)
  :
  settings (a_settings),
  active (false),
  interval (0),
  last_pcpu (0),
  last_total_time (0ULL),
  last_cpu_time (0ULL)
{
  this->connection = a_settings->signal_changed ("smooth_refresh").connect ([this](const Glib::ustring&key) {
    this->load_settings_value (key);
  });

  this->reset ();
  this->load_settings_value (KEY);
}



void
SmoothRefresh::reset ()
{
  glibtop_cpu cpu;
  glibtop_proc_time proctime;

  glibtop_get_cpu (&cpu);
  glibtop_get_proc_time (&proctime, getpid ());

  this->interval = GsmApplication::get ().config.update_interval;
  this->last_pcpu = PCPU_LO;
  this->last_total_time = cpu.total;
  this->last_cpu_time = proctime.rtime;
}



SmoothRefresh::~SmoothRefresh()
{
  if (this->connection)
    this->connection.disconnect ();
}



bool
SmoothRefresh::get (guint &new_interval)
{
  const unsigned config_interval = GsmApplication::get ().config.update_interval;

  g_assert (this->interval >= config_interval);

  if (not this->active)
    return false;


  const unsigned pcpu = this->get_own_cpu_usage ();

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

  if (pcpu > PCPU_HI && this->last_pcpu > PCPU_HI)
    new_interval = this->interval * 11 / 10;
  else if (this->interval != config_interval && pcpu < PCPU_LO && this->last_pcpu < PCPU_LO)
    new_interval = this->interval * 9 / 10;
  else
    new_interval = this->interval;

  new_interval = CLAMP (new_interval, config_interval, config_interval * 2);
  new_interval = CLAMP (new_interval, MIN_UPDATE_INTERVAL, MAX_UPDATE_INTERVAL);

  bool changed = this->interval != new_interval;

  if (changed)
    this->interval = new_interval;

  this->last_pcpu = pcpu;


  if (changed)
    procman_debug ("CPU usage is %3u%%, changed refresh_interval to %u (config %u)",
                   this->last_pcpu,
                   this->interval,
                   config_interval);

  g_assert (this->interval == new_interval);
  g_assert (this->interval >= config_interval);

  return changed;
}
