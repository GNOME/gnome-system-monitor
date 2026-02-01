/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include "procinfo.h"

#include <map>
#include <string>

#include <glib.h>
#include <glib/gprintf.h>
#include <glibtop/procargs.h>
#include <glibtop/procstate.h>
#include <glibtop/proctime.h>
#include <pwd.h>
#include <sys/types.h>

#include "application.h"
#include "cgroups.h"
#include "proctable.h"
#include "selinux.h"

#ifdef HAVE_SYSTEMD
#include "systemd.h"
#endif


std::string
ProcInfo::lookup_user (guint uid)
{
  static std::map<guint, std::string> users;
  const auto [it, inserted] = users.try_emplace (uid, "");

  // procman_debug("User lookup for uid %u: %s", uid, (inserted ? "MISS" : "HIT"));

  if (inserted)
    {
      struct passwd*pwd;
      pwd = getpwuid (uid);

      if (pwd && pwd->pw_name)
        {
          it->second = pwd->pw_name;
        }
      else
        {
          char username[16];
          g_sprintf (username, "%u", uid);
          it->second = username;
        }
    }

  return it->second;
}

void
ProcInfo::set_user (guint uid)
{
  if (G_LIKELY (this->uid == uid))
    return;

  this->uid = uid;
  this->user = lookup_user (uid);
}

static void
get_process_name (ProcInfo    *info,
                  const gchar *cmd,
                  const GStrv  args)
{
  if (args)
    {
      // look for /usr/bin/very_long_name
      // and also /usr/bin/interpreter /usr/.../very_long_name
      // which may have use prctl to alter 'cmd' name
      for (int i = 0; i != 2 && args[i]; ++i)
        {
          char*basename;
          basename = g_path_get_basename (args[i]);

          if (g_str_has_prefix (basename, cmd))
            {
              info->name = make_string (basename);
              return;
            }

          g_free (basename);
        }
    }
  info->name = cmd;
}

ProcInfo::ProcInfo(pid_t pid)
  : node (),
  icon (),
  pid (pid),
  ppid (-1),
  uid (-1)
{
  ProcInfo * const info = this;
  glibtop_proc_state procstate;
  glibtop_proc_time proctime;
  glibtop_proc_args procargs;
  gchar**arguments;

  glibtop_get_proc_state (&procstate, pid);
  glibtop_get_proc_time (&proctime, pid);
  arguments = glibtop_get_proc_argv (&procargs, pid, 0);

  /* FIXME : wrong. name and arguments may change with exec* */
  get_process_name (info, procstate.cmd, static_cast<const GStrv>(arguments));

  std::string tooltip = make_string (g_strjoinv (" ", arguments));

  if (tooltip.empty ())
    tooltip = procstate.cmd;

  info->tooltip = make_string (g_markup_escape_text (tooltip.c_str (), -1));

  info->arguments = make_string (g_strescape (tooltip.c_str (), "\\\""));
  g_strfreev (arguments);

  guint64 cpu_time = proctime.rtime;
  auto it = GsmApplication::get ().processes.cpu_times.find (pid);

  if (it != GsmApplication::get ().processes.cpu_times.end ())
    if (proctime.rtime >= it->second)
      cpu_time = it->second;
  info->cpu_time = cpu_time;
  info->start_time = proctime.start_time;

  get_process_cgroup_info (*info);

  gsm_proc_info_load_selinux (info);
  gsm_proc_info_load_systemd (info);
}


void
ProcInfo::set_icon (Glib::RefPtr<Gdk::Texture> icon)
{
  this->icon = icon;

  GtkTreeModel *model;

  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (
                                             gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (
                                                                              gtk_tree_view_get_model (GTK_TREE_VIEW (GsmApplication::get ().tree))))));
  gtk_tree_store_set (GTK_TREE_STORE (model), &this->node,
                      COL_ICON, (this->icon ? this->icon->gobj () : NULL),
                      -1);
}


void
gsm_proc_info_load_selinux (ProcInfo *self)
{
  g_autofree char *context = NULL;

  g_return_if_fail (self);

  context = gsm_selinux_get_context (self->pid);

  self->security_context = make_string (g_steal_pointer (&context));
}


void
gsm_proc_info_load_systemd (ProcInfo *self)
{
#ifdef HAVE_SYSTEMD
  g_autofree char *sd_unit = NULL;
  g_autofree char *sd_session = NULL;
  g_autofree char *sd_seat = NULL;
  uid_t sd_owner;

  g_return_if_fail (self);

  if (gsm_systemd_get_process_info (self->pid,
                                    &sd_unit,
                                    &sd_session,
                                    &sd_seat,
                                    &sd_owner)) {
    self->unit = make_string (g_steal_pointer (&sd_unit));
    self->session = make_string (g_steal_pointer (&sd_session));
    self->seat = make_string (g_steal_pointer (&sd_seat));
    self->owner = self->lookup_user (sd_owner);
  }
#else
  g_return_if_fail (self);
#endif
}
