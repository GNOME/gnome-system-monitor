#include <config.h>
#include <stdlib.h>

#ifdef HAVE_SYSTEMD
#include <systemd/sd-login.h>
#endif

#include "systemd.h"
#include "procinfo.h"


bool
procman::systemd_logind_running ()
{
#ifdef HAVE_SYSTEMD
  static bool init;
  static bool is_running;

  if (!init)
    {
      /* check if logind is running */
      if (access ("/run/systemd/seats/", F_OK) >= 0)
        is_running = true;
      init = true;
    }

  return is_running;
#else
  return false;
#endif
}

void
procman::get_process_systemd_info (ProcInfo *info)
{
#ifdef HAVE_SYSTEMD
  uid_t uid;

  if (!systemd_logind_running ())
    return;

  char*unit = NULL;
  sd_pid_get_unit (info->pid, &unit);
  info->unit = make_string (unit);

  char*session = NULL;
  sd_pid_get_session (info->pid, &session);
  info->session = make_string (session);

  if (!info->session.empty ())
    {
      char*seat = NULL;
      sd_session_get_seat (info->session.c_str (), &seat);
      info->seat = make_string (seat);
    }
  if (sd_pid_get_owner_uid (info->pid, &uid) >= 0)
    info->owner = info->lookup_user (uid);
  else
    info->owner = "";
#endif
}
