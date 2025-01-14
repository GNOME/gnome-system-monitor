/* Procman - dialogs
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

#include <glib/gi18n.h>

#include <signal.h>
#include <string.h>
#include <stdio.h>

#include "procdialogs.h"
#include "application.h"
#include "proctable.h"
#include "prettytable.h"
#include "procactions.h"
#include "procinfo.h"
#include "util.h"
#include "gsm_gnomesu.h"
#include "gsm_gksu.h"
#include "gsm_pkexec.h"
#include "cgroups.h"

static AdwAlertDialog *renice_dialog = NULL;
static gint new_nice_value = 0;

static GtkWindow *
get_active_window ()
{
  GListModel *toplevels = gtk_window_get_toplevels ();

  guint n_toplevels = g_list_model_get_n_items (toplevels);
  for (guint i = 0; i < n_toplevels; i++)
    {
      GtkWindow *window = GTK_WINDOW (g_list_model_get_item (toplevels, i));

      if (gtk_window_is_active (window))
        return window;
    }

  g_object_unref (toplevels);

  return NULL;
}

static void
kill_dialog_choose (GObject      *dialog,
                    GAsyncResult *res,
                    gpointer      data)
{
  struct ProcActionArgs *kargs = static_cast<ProcActionArgs*>(data);

  const gchar *res_id = adw_alert_dialog_choose_finish (ADW_ALERT_DIALOG (dialog), res);

  if (g_str_equal (res_id, adw_alert_dialog_get_close_response (ADW_ALERT_DIALOG (dialog))))
    return;

  kill_process (kargs->app, kargs->arg_value, -1);

  proctable_thaw (kargs->app);
  proctable_update (kargs->app);
  g_free (kargs);
}

void
procdialog_create_kill_dialog (GsmApplication *app,
                               int             signal,
                               gint32          proc)
{
  AdwAlertDialog *kill_alert_dialog;
  struct ProcActionArgs *kargs;
  gchar *primary, *secondary, *button_text;
  GCancellable *cancellable;

  proctable_freeze (app);
  kargs = g_new (ProcActionArgs, 1);
  kargs->app = app;
  kargs->arg_value = signal;
  gint selected_count = gtk_tree_selection_count_selected_rows (app->selection);

  if (selected_count == 1 || proc != -1)
    {
      ProcInfo *selected_process = NULL;

      if (proc == -1)
        {
          // get the last selected row
          gtk_tree_selection_selected_foreach (app->selection, get_last_selected,
                                               &selected_process);
        }
      else
        {
          selected_process = app->processes.find (proc);
        }

      std::string *process_name = &selected_process->name;
      std::string short_process_name = process_name->substr (0, process_name->find (" -"));

      switch (signal)
        {
          case SIGKILL:
            /*xgettext: primary alert message for killing single process*/
            primary = g_strdup_printf (_("Force Stop %s?"),
                                       short_process_name.c_str ());
            break;

          case SIGTERM:
            /*xgettext: primary alert message for ending single process*/
            primary = g_strdup_printf (_("End %s?"),
                                       short_process_name.c_str ());
            break;

          default:   // SIGSTOP
            /*xgettext: primary alert message for stopping single process*/
            primary = g_strdup_printf (_("Stop %s?"),
                                       short_process_name.c_str ());
            break;
        }
    }
  else
    {
      switch (signal)
        {
          case SIGKILL:
            /*xgettext: primary alert message for killing multiple processes*/
            primary = g_strdup_printf (ngettext ("Force Stop Selected Process?",
                                                 "Force Stop %d Selected Processes?", selected_count),
                                       selected_count);
            break;

          case SIGTERM:
            /*xgettext: primary alert message for ending multiple processes*/
            primary = g_strdup_printf (ngettext ("End Selected Process?",
                                                 "End %d Selected Processes?", selected_count),
                                       selected_count);
            break;

          default:   // SIGSTOP
            /*xgettext: primary alert message for stopping multiple processes*/
            primary = g_strdup_printf (ngettext ("Stop Selected Process?",
                                                 "Stop %d Selected Processes?", selected_count),
                                       selected_count);
            break;
        }
    }

  switch (signal)
    {
      case SIGKILL:
        /*xgettext: secondary alert message*/
        secondary = _("Force stopping processes can result in data "
                      "loss, crashes, and system failures");
        button_text = _("_Force Stop");
        break;

      case SIGTERM:
        /*xgettext: secondary alert message*/
        secondary = _("Ending processes can result in data loss, "
                      "crashes, and system failures");
        button_text = _("_End");
        break;

      default:   // SIGSTOP
        /*xgettext: secondary alert message*/
        secondary = _("Stopping processes can result in data loss, "
                      "crashes, and system failures");
        button_text = _("_Stop");
        break;
    }

  kill_alert_dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (primary, secondary));
  g_free (primary);

  adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (kill_alert_dialog),
                                  "cancel", _("_Cancel"),
                                  "apply", button_text,
                                  NULL);
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (kill_alert_dialog), "cancel");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (kill_alert_dialog), "cancel");
  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (kill_alert_dialog), "apply", ADW_RESPONSE_DESTRUCTIVE);

  cancellable = g_cancellable_new ();

  adw_alert_dialog_choose (ADW_ALERT_DIALOG (kill_alert_dialog),
                           GTK_WIDGET (get_active_window ()),
                           cancellable,
                           kill_dialog_choose,
                           kargs);
}

static void
renice_adjustment_value_changed (GtkAdjustment *adjustment,
                                 gpointer       data)
{
  GtkLabel *label = GTK_LABEL (data);

  new_nice_value = int(gtk_adjustment_get_value (adjustment));

  gchar *text = g_strdup (procman::get_nice_level_with_priority (new_nice_value));
  gtk_label_set_text (label, text);

  g_free (text);
}

static void
renice_dialog_response (AdwAlertDialog*,
                        gchar   *response,
                        gpointer data)
{
  GsmApplication *app = static_cast<GsmApplication *>(data);

  if (g_str_equal (response, "change"))
    renice (app, new_nice_value);

  renice_dialog = NULL;
}

void
procdialog_create_renice_dialog (GsmApplication *app)
{
  GtkAdjustment *renice_adjustment;
  GtkBuilder *builder;
  GtkLabel *priority_label;
  ProcInfo *info;
  gchar     *dialog_title;

  if (renice_dialog)
    return;

  gtk_tree_selection_selected_foreach (app->selection, get_last_selected,
                                       &info);
  gint selected_count = gtk_tree_selection_count_selected_rows (app->selection);

  if (!info)
    return;

  new_nice_value = info->nice;

  builder = gtk_builder_new ();
  GError *err = NULL;

  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/renice.ui", &err);
  if (err != NULL)
    g_error ("%s", err->message);

  priority_label = GTK_LABEL (gtk_builder_get_object (builder, "priority_label"));
  gtk_label_set_label (priority_label, procman::get_nice_level_with_priority (info->nice));

  renice_adjustment = GTK_ADJUSTMENT (gtk_builder_get_object (builder, "renice_adjustment"));
  gtk_adjustment_configure (GTK_ADJUSTMENT (renice_adjustment), info->nice, RENICE_VAL_MIN, RENICE_VAL_MAX, 1, 1, 0);
  g_signal_connect (G_OBJECT (renice_adjustment),
                    "value_changed",
                    G_CALLBACK (renice_adjustment_value_changed),
                    priority_label);

  renice_dialog = ADW_ALERT_DIALOG (gtk_builder_get_object (builder, "renice_dialog"));

  if (selected_count == 1)
    dialog_title = g_strdup_printf (_("Change Priority of %s (PID %u)"),
                                    info->name.c_str (), info->pid);
  else
    dialog_title = g_strdup_printf (ngettext ("Change Priority of the Selected Process", "Change Priority of %d Selected Processes", selected_count),
                                    selected_count);
  adw_alert_dialog_set_heading (renice_dialog, dialog_title);
  g_free (dialog_title);

  g_signal_connect (G_OBJECT (renice_dialog),
                    "response",
                    G_CALLBACK (renice_dialog_response),
                    app);

  adw_dialog_present (ADW_DIALOG (renice_dialog), GTK_WIDGET (app->main_window));

  g_object_unref (G_OBJECT (builder));
}

static char *
procman_action_to_command (ProcmanActionType type,
                           gint              pid,
                           gint              extra_value)
{
  switch (type)
    {
      case PROCMAN_ACTION_KILL:
        return g_strdup_printf ("kill -s %d %d", extra_value, pid);

      case PROCMAN_ACTION_RENICE:
        return g_strdup_printf ("renice %d %d", extra_value, pid);

      default:
        g_assert_not_reached ();
    }
}


gboolean
multi_root_check (char *command)
{
  if (procman_has_pkexec ())
    return gsm_pkexec_create_root_password_dialog (command);

  if (procman_has_gksu ())
    return gsm_gksu_create_root_password_dialog (command);

  if (procman_has_gnomesu ())
    return gsm_gnomesu_create_root_password_dialog (command);

  return FALSE;
}

/*
 * type determines whether if dialog is for killing process or renice.
 * type == PROCMAN_ACTION_KILL,   extra_value -> signal to send
 * type == PROCMAN_ACTION_RENICE, extra_value -> new priority.
 */
gboolean
procdialog_create_root_password_dialog (ProcmanActionType type,
                                        GsmApplication*,
                                        gint              pid,
                                        gint              extra_value)
{
  char *command;
  gboolean ret = FALSE;

  command = procman_action_to_command (type, pid, extra_value);

  procman_debug ("Trying to run '%s' as root", command);

  if (procman_has_pkexec ())
    ret = gsm_pkexec_create_root_password_dialog (command);
  else if (procman_has_gksu ())
    ret = gsm_gksu_create_root_password_dialog (command);
  else if (procman_has_gnomesu ())
    ret = gsm_gnomesu_create_root_password_dialog (command);

  g_free (command);
  return ret;
}
