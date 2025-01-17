/* Procman process actions
 * Copyright (C) 2001 Kevin Vandersloot
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <config.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "procactions.h"
#include "application.h"
#include "procinfo.h"
#include "proctable.h"
#include "procdialogs.h"
#include "settings-keys.h"


static void
renice_single_process (GtkTreeModel *model,
                       GtkTreePath*,
                       GtkTreeIter  *iter,
                       gpointer      data)
{
  const struct ProcActionArgs * const args = static_cast<ProcActionArgs*>(data);

  ProcInfo *info = NULL;
  gint error;
  int saved_errno;
  gchar *error_msg;
  AdwAlertDialog *dialog;

  gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

  if (!info)
    return;
  if (info->nice == args->arg_value)
    return;
  error = setpriority (PRIO_PROCESS, info->pid, args->arg_value);

  /* success */
  if (error != -1)
    return;

  saved_errno = errno;

  /* need to be root */
  if (errno == EPERM || errno == EACCES)
    {
      gboolean success;

      success = procdialog_create_root_password_dialog (
        PROCMAN_ACTION_RENICE, args->app, info->pid,
        args->arg_value);

      if (success)
        return;

      if (errno)
        saved_errno = errno;
    }

  /* failed */
  error_msg = g_strdup_printf (
    _("Cannot change the priority of process with PID %d to %d.\n"
      "%s"),
    info->pid, args->arg_value, g_strerror (saved_errno));

  dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (
                               NULL,
                               NULL));

  adw_alert_dialog_format_body (dialog, "%s", error_msg);

  adw_alert_dialog_add_response (dialog, "ok", _("_OK"));

  g_free (error_msg);

  adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET (GsmApplication::get ().main_window));
}


void
renice (GsmApplication *app,
        int             nice)
{
  struct ProcActionArgs args = { app, nice };

  /* EEEK - ugly hack - make sure the table is not updated as a crash
  ** occurs if you first kill a process and the tree node is removed while
  ** still in the foreach function
  */
  proctable_freeze (app);

  gtk_tree_selection_selected_foreach (app->selection, renice_single_process,
                                       &args);

  proctable_thaw (app);

  proctable_update (app);
}

static void
kill_process_action (ProcInfo                    *info,
                     const struct ProcActionArgs *const args)
{
  int error;
  int saved_errno;
  char *error_msg;
  AdwAlertDialog *dialog;

  if (!info)
    return;

  error = kill (info->pid, args->arg_value);

  /* success */
  if (error != -1)
    return;

  saved_errno = errno;

  /* need to be root */
  if (errno == EPERM)
    {
      gboolean success;

      success = procdialog_create_root_password_dialog (
        PROCMAN_ACTION_KILL, args->app, info->pid,
        args->arg_value);

      if (success)
        return;

      if (errno)
        saved_errno = errno;
    }

  /* failed */
  error_msg = g_strdup_printf (
    _("Cannot kill process with PID %d with signal %d.\n"
      "%s"),
    info->pid, args->arg_value, g_strerror (saved_errno));

  dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (
                               NULL,
                               NULL));

  adw_alert_dialog_format_body (dialog, "%s", error_msg);

  adw_alert_dialog_add_response (dialog, "ok", _("_OK"));

  g_free (error_msg);

  adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET (GsmApplication::get ().main_window));
}

static void
kill_single_process (GtkTreeModel *model,
                     GtkTreePath*,
                     GtkTreeIter  *iter,
                     gpointer      data)
{
  const struct ProcActionArgs * const args = static_cast<ProcActionArgs*>(data);
  ProcInfo *info;

  gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

  kill_process_action (info, args);
}

void
on_activate_send_signal (GSimpleAction *action,
                         GVariant      *value,
                         gpointer       data)
{
  GsmApplication *app = (GsmApplication *) data;

  const gchar *action_name = g_action_get_name (G_ACTION (action));
  gint32 proc = -1;

  if (value)
    proc = g_variant_get_int32 (value);

  if (g_strcmp0 (action_name, "send-signal-cont") == 0)
    kill_process (app, SIGCONT, proc);
  else if (g_strcmp0 (action_name, "send-signal-stop") == 0)
    kill_process_with_confirmation (app, SIGSTOP, proc);
  else if (g_strcmp0 (action_name, "send-signal-term") == 0)
    kill_process_with_confirmation (app, SIGTERM, proc);
  else if (g_strcmp0 (action_name, "send-signal-kill") == 0)
    kill_process_with_confirmation (app, SIGKILL, proc);
}

void
kill_process_with_confirmation (GsmApplication *app,
                                int             signal,
                                gint32          proc)
{
  gboolean kill_dialog = app->settings->get_boolean (GSM_SETTING_SHOW_KILL_DIALOG);

  if (kill_dialog)
    procdialog_create_kill_dialog (app, signal, proc);
  else
    kill_process (app, signal, proc);
}

void
kill_process (GsmApplication *app,
              int             sig,
              gint32          proc)
{
  struct ProcActionArgs args = { app, sig };

  /* EEEK - ugly hack - make sure the table is not updated as a crash
  ** occurs if you first kill a process and the tree node is removed while
  ** still in the foreach function
  */
  proctable_freeze (app);

  if (proc == -1)
    {
      gtk_tree_selection_selected_foreach (app->selection, kill_single_process,
                                           &args);
    }
  else
    {
      ProcInfo *info = app->processes.find (proc);

      kill_process_action (info, &args);
    }

  proctable_thaw (app);

  proctable_update (app);
}
