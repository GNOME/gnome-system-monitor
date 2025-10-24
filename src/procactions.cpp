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

#include "config.h"

#include <glib/gi18n.h>

#include "application.h"
#include "gsm-actions.h"
#include "procdialogs.h"
#include "procinfo.h"
#include "proctable.h"
#include "settings-keys.h"

#include "procactions.h"


typedef struct _PriorityData PriorityData;
struct _PriorityData {
  GPtrArray *infos;
  int priority;
  GtkWidget *parent;
  GsmApplication *app;
};


G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
build_priority_list (GtkTreeModel              *model,
                     G_GNUC_UNUSED GtkTreePath *path,
                     GtkTreeIter               *iter,
                     gpointer                   user_data)
{
  PriorityData *data = static_cast<PriorityData *>(user_data);
  ProcInfo *info;

  gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

  if (!info) {
    return;
  }

  g_ptr_array_add (data->infos, info);
}
G_GNUC_END_IGNORE_DEPRECATIONS


static void
did_priority (GObject *source, GAsyncResult *result, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  PriorityData *data = static_cast<PriorityData *>(user_data);

  gsm_actions_set_priority_finish (GSM_ACTIONS (source), result, &error);

  if (error) {
    AdwDialog *dialog =
      adw_alert_dialog_new (_("Cannot Change Priority"),
                            error->message);

    adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog),
                                   "close",
                                   _("_Close"));

    adw_dialog_present (dialog, data->parent);
  }

  if (data->infos->len > 0) {
    ProcInfo *info =
      static_cast<ProcInfo *>(g_ptr_array_steal_index (data->infos, 0));

    gsm_actions_set_priority (GSM_ACTIONS (source),
                              info->pid,
                              data->priority,
                              NULL,
                              did_priority,
                              data);

    return;
  }

  proctable_thaw (data->app);
  proctable_update (data->app);

  g_clear_pointer (&data->infos, g_ptr_array_unref);
  g_clear_object (&data->parent);
  g_free (data);
}


void
renice (GsmApplication *app,
        int             nice)
{
  g_autoptr (GsmActions) actions =
    GSM_ACTIONS (g_object_new (GSM_TYPE_ACTIONS, NULL));
  PriorityData *data = g_new0 (PriorityData, 1);
  ProcInfo *info = NULL;

  /* EEEK - ugly hack - make sure the table is not updated as a crash
  ** occurs if you first kill a process and the tree node is removed while
  ** still in the foreach function
  */
  proctable_freeze (app);

  data->priority = nice;
  data->infos = g_ptr_array_new_null_terminated (10, NULL, TRUE);
  g_set_object (&data->parent, GTK_WIDGET (app->main_window));
  data->app = app;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_tree_selection_selected_foreach (app->selection,
                                       build_priority_list,
                                       data);
G_GNUC_END_IGNORE_DEPRECATIONS

  info = static_cast<ProcInfo *>(g_ptr_array_steal_index (data->infos, 0));
  gsm_actions_set_priority (actions,
                            info->pid,
                            nice,
                            NULL,
                            did_priority,
                            data);
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


typedef struct _KillData KillData;
struct _KillData {
  GPtrArray *infos;
  int signal;
  GtkWidget *parent;
  GsmApplication *app;
};


G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
build_kill_list (GtkTreeModel              *model,
                 G_GNUC_UNUSED GtkTreePath *path,
                 GtkTreeIter               *iter,
                 gpointer                   user_data)
{
  KillData *data = static_cast<KillData *>(user_data);
  ProcInfo *info;

  gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

  if (!info) {
    return;
  }

  g_ptr_array_add (data->infos, info);
}
G_GNUC_END_IGNORE_DEPRECATIONS


static void
did_kill (GObject *source, GAsyncResult *result, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  KillData *data = static_cast<KillData *>(user_data);

  gsm_actions_kill_finish (GSM_ACTIONS (source), result, &error);

  if (error) {
    AdwDialog *dialog =
      adw_alert_dialog_new (_("Cannot Kill Process"),
                            error->message);

    adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog),
                                   "close",
                                   _("_Close"));

    adw_dialog_present (dialog, data->parent);
  }

  if (data->infos->len > 0) {
    ProcInfo *info =
      static_cast<ProcInfo *>(g_ptr_array_steal_index (data->infos, 0));

    gsm_actions_kill (GSM_ACTIONS (source),
                      info->pid,
                      data->signal,
                      NULL,
                      did_kill,
                      data);

    return;
  }

  proctable_thaw (data->app);
  proctable_update (data->app);

  g_clear_pointer (&data->infos, g_ptr_array_unref);
  g_clear_object (&data->parent);
  g_free (data);
}


void
kill_process (GsmApplication *app,
              int             sig,
              gint32          proc)
{
  g_autoptr (GsmActions) actions =
    GSM_ACTIONS (g_object_new (GSM_TYPE_ACTIONS, NULL));
  KillData *data = g_new0 (KillData, 1);
  ProcInfo *info = NULL;

  /* EEEK - ugly hack - make sure the table is not updated as a crash
  ** occurs if you first kill a process and the tree node is removed while
  ** still in the foreach function
  */
  proctable_freeze (app);

  data->signal = sig;
  data->infos = g_ptr_array_new_null_terminated (10, NULL, TRUE);
  g_set_object (&data->parent, GTK_WIDGET (app->main_window));
  data->app = app;

  if (proc == -1) {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gtk_tree_selection_selected_foreach (app->selection,
                                         build_kill_list,
                                         data);
G_GNUC_END_IGNORE_DEPRECATIONS
  } else {
    g_ptr_array_add (data->infos, app->processes.find (proc));
  }

  info = static_cast<ProcInfo *>(g_ptr_array_steal_index (data->infos, 0));
  gsm_actions_kill (actions,
                    info->pid,
                    sig,
                    NULL,
                    did_kill,
                    data);
}
