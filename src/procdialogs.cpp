/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
#include "proctable.h"
#include "prettytable.h"
#include "procactions.h"
#include "util.h"
#include "gsm_gnomesu.h"
#include "gsm_gksu.h"
#include "gsm_pkexec.h"
#include "cgroups.h"

static GtkDialog *renice_dialog = NULL;
static gint new_nice_value = 0;


static void
kill_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
    struct ProcActionArgs *kargs = static_cast<ProcActionArgs*>(data);

    gtk_widget_destroy (GTK_WIDGET (dialog));

    if (id == GTK_RESPONSE_OK)
        kill_process (kargs->app, kargs->arg_value);

    g_free (kargs);
}

void
procdialog_create_kill_dialog (GsmApplication *app, int signal)
{
    GtkMessageDialog *kill_alert_dialog;
    GtkWidget *confirm_button;

    gchar *primary, *secondary, *button_text;
    struct ProcActionArgs *kargs;

    kargs = g_new(ProcActionArgs, 1);
    kargs->app = app;
    kargs->arg_value = signal;
    gint selected_count = gtk_tree_selection_count_selected_rows (app->selection);

    if ( selected_count == 1 ) {
        ProcInfo *selected_process = NULL;
        // get the last selected row
        gtk_tree_selection_selected_foreach (app->selection, get_last_selected,
                                         &selected_process);

	std::string *process_name = &selected_process->name;
	std::string short_process_name = process_name->substr(0, process_name->find(" -"));

        switch (signal) {
            case SIGKILL:
                /*xgettext: primary alert message for killing single process*/
                primary = g_strdup_printf (_("Are you sure you want to kill the selected process “%s” (PID: %u)?"),
                                           short_process_name.c_str(),
                                           selected_process->pid);
                break;
            case SIGTERM:
                /*xgettext: primary alert message for ending single process*/
                primary = g_strdup_printf (_("Are you sure you want to end the selected process “%s” (PID: %u)?"),
                                           short_process_name.c_str(),
                                           selected_process->pid);
                break;
            default: // SIGSTOP
                /*xgettext: primary alert message for stopping single process*/
                primary = g_strdup_printf (_("Are you sure you want to stop the selected process “%s” (PID: %u)?"),
                                           short_process_name.c_str(),
                                           selected_process->pid);
                break;
        }
    } else {
        switch (signal) {
            case SIGKILL:
                /*xgettext: primary alert message for killing multiple processes*/
                primary = g_strdup_printf (ngettext("Are you sure you want to kill the selected process?", 
                                                    "Are you sure you want to kill the %d selected processes?", selected_count),
                                           selected_count);
            break;
            case SIGTERM:
                /*xgettext: primary alert message for ending multiple processes*/
                primary = g_strdup_printf (ngettext("Are you sure you want to end the selected process?", 
                                                    "Are you sure you want to end the %d selected processes?", selected_count),
                                           selected_count);
            break;
            default: // SIGSTOP
                /*xgettext: primary alert message for stopping multiple processes*/
                primary = g_strdup_printf (ngettext("Are you sure you want to stop the selected process?", 
                                                    "Are you sure you want to stop the %d selected processes?", selected_count),
                                           selected_count);
            break;
        }
    }
    
    switch (signal) {
        case SIGKILL:
            /*xgettext: secondary alert message*/
            secondary = _("Killing a process may destroy data, break the "
                          "session or introduce a security risk. "
                          "Only unresponsive processes should be killed.");
            button_text = ngettext("_Kill Process", "_Kill Processes", selected_count);
            break;
        case SIGTERM:
            /*xgettext: secondary alert message*/
            secondary = _("Ending a process may destroy data, break the "
                          "session or introduce a security risk. "
                          "Only unresponsive processes should be ended.");
            button_text = ngettext("_End Process", "_End Processes", selected_count);
            break;
        default: // SIGSTOP
            /*xgettext: secondary alert message*/
            secondary = _("Stopping a process may destroy data, break the "
                          "session or introduce a security risk. "
                          "Only unresponsive processes should be stopped.");
            button_text = ngettext("_Stop Process", "_Stop Processes", selected_count);
            break;
    }

    kill_alert_dialog = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (GTK_WINDOW (app->main_window),
                                                static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                GTK_MESSAGE_WARNING,
                                                GTK_BUTTONS_NONE,
                                                "%s",
                                                primary));
    g_free (primary);

    gtk_message_dialog_format_secondary_text (kill_alert_dialog,
                                              "%s",
                                              secondary);

    gtk_dialog_add_button (GTK_DIALOG (kill_alert_dialog),
                           _("_Cancel"), GTK_RESPONSE_CANCEL);

    confirm_button = gtk_dialog_add_button (GTK_DIALOG (kill_alert_dialog),
                                            button_text, GTK_RESPONSE_OK);
    gtk_style_context_add_class (gtk_widget_get_style_context (confirm_button),
                                 GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);

    gtk_dialog_set_default_response (GTK_DIALOG (kill_alert_dialog),
                                     GTK_RESPONSE_CANCEL);

    g_signal_connect (G_OBJECT (kill_alert_dialog), "response",
                      G_CALLBACK (kill_dialog_button_pressed), kargs);

    gtk_widget_show_all (GTK_WIDGET (kill_alert_dialog));
}

static void
renice_scale_changed (GtkAdjustment *adj, gpointer data)
{
    GtkLabel *label = GTK_LABEL (data);

    new_nice_value = int(gtk_adjustment_get_value (adj));
    gchar* text = g_strdup(procman::get_nice_level_with_priority (new_nice_value));
    gtk_label_set_text (label, text);
    g_free(text);

}

static void
renice_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);
    if (id == 100) {
        if (new_nice_value == -100)
            return;
        renice(app, new_nice_value);
    }

    gtk_widget_destroy (GTK_WIDGET (dialog));
    renice_dialog = NULL;
}

void
procdialog_create_renice_dialog (GsmApplication *app)
{
    ProcInfo *info;
    
    GtkLabel *label;
    GtkLabel *priority_label;
    GtkAdjustment *renice_adj;
    GtkBuilder *builder;
    gchar     *text;
    gchar     *dialog_title;

    if (renice_dialog)
        return;

    gtk_tree_selection_selected_foreach (app->selection, get_last_selected,
                                         &info);
    gint selected_count = gtk_tree_selection_count_selected_rows (app->selection);
    if (!info)
        return;

    builder = gtk_builder_new();
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/renice.ui", NULL);

    renice_dialog = GTK_DIALOG (gtk_builder_get_object (builder, "renice_dialog"));
    if ( selected_count == 1 ) {
        dialog_title = g_strdup_printf (_("Change Priority of Process “%s” (PID: %u)"),
                                        info->name.c_str(), info->pid);
    } else {
        dialog_title = g_strdup_printf (ngettext("Change Priority of the selected process", "Change Priority of %d selected processes", selected_count),
                                        selected_count);
    }

    gtk_window_set_title (GTK_WINDOW(renice_dialog), dialog_title);
    
    g_free (dialog_title);

    gtk_dialog_set_default_response (GTK_DIALOG (renice_dialog), 100);
    new_nice_value = -100;

    renice_adj = GTK_ADJUSTMENT (gtk_builder_get_object (builder, "renice_adj"));
    gtk_adjustment_configure( GTK_ADJUSTMENT(renice_adj), info->nice, RENICE_VAL_MIN, RENICE_VAL_MAX, 1, 1, 0);

    new_nice_value = 0;
    
    priority_label =  GTK_LABEL (gtk_builder_get_object (builder, "priority_label"));
    gtk_label_set_label (priority_label, procman::get_nice_level_with_priority (info->nice));

    text = g_strconcat("<small><i><b>", _("Note:"), "</b> ",
                       _("The priority of a process is given by its nice value. A lower nice value corresponds to a higher priority."),
                       "</i></small>", NULL);
    label = GTK_LABEL (gtk_builder_get_object (builder, "note_label"));
    gtk_label_set_label (label, _(text));
    gtk_label_set_line_wrap (label, TRUE);
    g_free (text);

    g_signal_connect (G_OBJECT (renice_dialog), "response",
                      G_CALLBACK (renice_dialog_button_pressed), app);
    g_signal_connect (G_OBJECT (renice_adj), "value_changed",
                      G_CALLBACK (renice_scale_changed), priority_label);

    gtk_window_set_transient_for (GTK_WINDOW (renice_dialog), GTK_WINDOW (GsmApplication::get()->main_window));
    gtk_widget_show_all (GTK_WIDGET (renice_dialog));

    gtk_builder_connect_signals (builder, NULL);

    g_object_unref (G_OBJECT (builder));
}



static char *
procman_action_to_command(ProcmanActionType type,
                          gint pid,
                          gint extra_value)
{
    switch (type) {
        case PROCMAN_ACTION_KILL:
            return g_strdup_printf("kill -s %d %d", extra_value, pid);
        case PROCMAN_ACTION_RENICE:
            return g_strdup_printf("renice %d %d", extra_value, pid);
        default:
            g_assert_not_reached();
    }
}


/*
 * type determines whether if dialog is for killing process or renice.
 * type == PROCMAN_ACTION_KILL,   extra_value -> signal to send
 * type == PROCMAN_ACTION_RENICE, extra_value -> new priority.
 */
gboolean
procdialog_create_root_password_dialog(ProcmanActionType type,
                                       GsmApplication *app,
                                       gint pid,
                                       gint extra_value)
{
    char * command;
    gboolean ret = FALSE;

    command = procman_action_to_command(type, pid, extra_value);

    procman_debug("Trying to run '%s' as root", command);

    if (procman_has_pkexec())
        ret = gsm_pkexec_create_root_password_dialog(command);
    else if (procman_has_gksu())
        ret = gsm_gksu_create_root_password_dialog(command);
    else if (procman_has_gnomesu())
        ret = gsm_gnomesu_create_root_password_dialog(command);

    g_free(command);
    return ret;
}
