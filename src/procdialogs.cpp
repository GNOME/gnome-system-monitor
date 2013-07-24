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
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
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
#include "load-graph.h"
#include "settings-keys.h"
#include "procman_gnomesu.h"
#include "procman_gksu.h"
#include "procman_pkexec.h"
#include "cgroups.h"

static GtkWidget *renice_dialog = NULL;
static GtkWidget *prefs_dialog = NULL;
static gint new_nice_value = 0;


static void
kill_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
    struct KillArgs *kargs = static_cast<KillArgs*>(data);

    gtk_widget_destroy (GTK_WIDGET (dialog));

    if (id == GTK_RESPONSE_OK)
        kill_process (kargs->app, kargs->signal);

    g_free (kargs);
}

void
procdialog_create_kill_dialog (ProcmanApp *app, int signal)
{
    GtkWidget *kill_alert_dialog;
    gchar *primary, *secondary, *button_text;
    struct KillArgs *kargs;

    kargs = g_new(KillArgs, 1);
    kargs->app = app;
    kargs->signal = signal;


    if (signal == SIGKILL) {
        /*xgettext: primary alert message*/
        primary = g_strdup_printf (_("Kill the selected process “%s” (PID: %u)?"),
                                   app->selected_process->name,
                                   app->selected_process->pid);
        /*xgettext: secondary alert message*/
        secondary = _("Killing a process may destroy data, break the "
                      "session or introduce a security risk. "
                      "Only unresponsive processes should be killed.");
        button_text = _("_Kill Process");
    }
    else {
        /*xgettext: primary alert message*/
        primary = g_strdup_printf (_("End the selected process “%s” (PID: %u)?"),
                                   app->selected_process->name,
                                   app->selected_process->pid);
        /*xgettext: secondary alert message*/
        secondary = _("Ending a process may destroy data, break the "
                      "session or introduce a security risk. "
                      "Only unresponsive processes should be ended.");
        button_text = _("_End Process");
    }

    kill_alert_dialog = gtk_message_dialog_new (GTK_WINDOW (app->main_window),
                                                static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                GTK_MESSAGE_WARNING,
                                                GTK_BUTTONS_NONE,
                                                "%s",
                                                primary);
    g_free (primary);

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (kill_alert_dialog),
                                              "%s",
                                              secondary);

    gtk_dialog_add_buttons (GTK_DIALOG (kill_alert_dialog),
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            button_text, GTK_RESPONSE_OK,
                            NULL);

    gtk_dialog_set_default_response (GTK_DIALOG (kill_alert_dialog),
                                     GTK_RESPONSE_CANCEL);

    g_signal_connect (G_OBJECT (kill_alert_dialog), "response",
                      G_CALLBACK (kill_dialog_button_pressed), kargs);

    gtk_widget_show_all (kill_alert_dialog);
}

static void
renice_scale_changed (GtkAdjustment *adj, gpointer data)
{
    GtkWidget *label = GTK_WIDGET (data);

    new_nice_value = int(gtk_adjustment_get_value (adj));
    gchar* text = g_strdup(procman::get_nice_level_with_priority (new_nice_value));
    gtk_label_set_text (GTK_LABEL (label), text);
    g_free(text);

}

static void
renice_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);
    if (id == 100) {
        if (new_nice_value == -100)
            return;
        renice(app, new_nice_value);
    }

    gtk_widget_destroy (GTK_WIDGET (dialog));
    renice_dialog = NULL;
}

void
procdialog_create_renice_dialog (ProcmanApp *app)
{
    ProcInfo *info = app->selected_process;
    GtkWidget *label;
    GtkWidget *priority_label;
    GtkAdjustment *renice_adj;
    GtkBuilder *builder;
    gchar     *text;
    gchar     *dialog_title;

    if (renice_dialog)
        return;

    if (!info)
        return;

    builder = gtk_builder_new();
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/renice.ui", NULL);

    renice_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "renice_dialog"));

    dialog_title = g_strdup_printf (_("Change Priority of Process “%s” (PID: %u)"),
                                    info->name, info->pid);

    gtk_window_set_title (GTK_WINDOW(renice_dialog), dialog_title);
    
    g_free (dialog_title);

    gtk_dialog_set_default_response (GTK_DIALOG (renice_dialog), 100);
    new_nice_value = -100;

    renice_adj = GTK_ADJUSTMENT (gtk_builder_get_object (builder, "renice_adj"));
    gtk_adjustment_configure( GTK_ADJUSTMENT(renice_adj), info->nice, RENICE_VAL_MIN, RENICE_VAL_MAX, 1, 1, 0);

    new_nice_value = 0;
    
    priority_label =  GTK_WIDGET (gtk_builder_get_object (builder, "priority_label"));
    gtk_label_set_label (GTK_LABEL(priority_label), procman::get_nice_level_with_priority (info->nice));

    text = g_strconcat("<small><i><b>", _("Note:"), "</b> ",
                       _("The priority of a process is given by its nice value. A lower nice value corresponds to a higher priority."),
                       "</i></small>", NULL);
    label = GTK_WIDGET (gtk_builder_get_object (builder, "note_label"));
    gtk_label_set_label (GTK_LABEL(label), _(text));
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    g_free (text);

    g_signal_connect (G_OBJECT (renice_dialog), "response",
                      G_CALLBACK (renice_dialog_button_pressed), app);
    g_signal_connect (G_OBJECT (renice_adj), "value_changed",
                      G_CALLBACK (renice_scale_changed), priority_label);

    gtk_widget_show_all (renice_dialog);

    gtk_builder_connect_signals (builder, NULL);

    g_object_unref (G_OBJECT (builder));
}

static void
prefs_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
    if (id == GTK_RESPONSE_HELP)
    {
        GError* error = 0;
        if (!g_app_info_launch_default_for_uri("help:gnome-system-monitor/gnome-system-monitor-prefs", NULL, &error))
        {
            g_warning("Could not display preferences help : %s", error->message);
            g_error_free(error);
        }
    }
    else
    {
        gtk_widget_destroy (GTK_WIDGET (dialog));
        prefs_dialog = NULL;
    }
}


class SpinButtonUpdater
{
public:
    SpinButtonUpdater(const string& key)
        : key(key)
    { }

    static gboolean callback(GtkWidget *widget, GdkEventFocus *event, gpointer data)
    {
        SpinButtonUpdater* updater = static_cast<SpinButtonUpdater*>(data);
        gtk_spin_button_update(GTK_SPIN_BUTTON(widget));
        updater->update(GTK_SPIN_BUTTON(widget));
        return FALSE;
    }

private:

    void update(GtkSpinButton* spin)
    {
        int new_value = int(1000 * gtk_spin_button_get_value(spin));

        g_settings_set_int(ProcmanApp::get()->settings,
                           this->key.c_str(), new_value);

        procman_debug("set %s to %d", this->key.c_str(), new_value);
    }

    const string key;
};

static void
field_toggled (const gchar *gconf_parent, GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    GtkTreeModel *model = static_cast<GtkTreeModel*>(data);
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    gboolean toggled;
    GSettings *settings = g_settings_get_child (ProcmanApp::get()->settings, gconf_parent);
    gchar *key;
    int id;

    if (!path)
        return;

    gtk_tree_model_get_iter (model, &iter, path);

    gtk_tree_model_get (model, &iter, 2, &column, -1);
    toggled = gtk_cell_renderer_toggle_get_active (cell);

    gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, !toggled, -1);
    gtk_tree_view_column_set_visible (column, !toggled);

    id = gtk_tree_view_column_get_sort_column_id (column);

    key = g_strdup_printf ("col-%d-visible", id);
    g_settings_set_boolean (settings, key, !toggled);
    g_free (key);

    gtk_tree_path_free (path);

}

static void
proc_field_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    field_toggled("proctree", cell, path_str, data);
}

static void
disk_field_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    field_toggled("disktreenew", cell, path_str, data);
}

static void
create_field_page(GtkBuilder* builder, GtkWidget *tree, const gchar *widgetname)
{
    GtkTreeView *treeview;
    GList *it, *columns;
    GtkListStore *model;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    gchar *full_widgetname;

    full_widgetname = g_strdup_printf ("%s_columns", widgetname);
    treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, full_widgetname));
    g_free (full_widgetname);

    model = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);

    gtk_tree_view_set_model (GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(model));
    g_object_unref (G_OBJECT (model));

    column = gtk_tree_view_column_new ();

    cell = gtk_cell_renderer_toggle_new ();
    gtk_tree_view_column_pack_start (column, cell, FALSE);
    gtk_tree_view_column_set_attributes (column, cell,
                                         "active", 0,
                                         NULL);
    if(!g_strcmp0(widgetname, "proctree")) 
        g_signal_connect (G_OBJECT (cell), "toggled", G_CALLBACK (proc_field_toggled), model);
    else if(!g_strcmp0(widgetname, "disktreenew")) 
        g_signal_connect (G_OBJECT (cell), "toggled", G_CALLBACK (disk_field_toggled), model);

    gtk_tree_view_column_set_clickable (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    column = gtk_tree_view_column_new ();

    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, cell, FALSE);
    gtk_tree_view_column_set_attributes (column, cell,
                                         "text", 1,
                                         NULL);

    gtk_tree_view_column_set_title (column, "Not Shown");
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));

    for(it = columns; it; it = it->next)
    {
        GtkTreeViewColumn *column = static_cast<GtkTreeViewColumn*>(it->data);
        GtkTreeIter iter;
        const gchar *title;
        gboolean visible;
        gint column_id;

        title = gtk_tree_view_column_get_title (column);
        if (!title)
            title = _("Icon");

        column_id = gtk_tree_view_column_get_sort_column_id(column);
        if ((column_id == COL_CGROUP) && (!cgroups_enabled()))
            continue;

        if ((column_id == COL_UNIT ||
             column_id == COL_SESSION ||
             column_id == COL_SEAT ||
             column_id == COL_OWNER)
#ifdef HAVE_SYSTEMD
            && !LOGIND_RUNNING()
#endif
                )
            continue;

        visible = gtk_tree_view_column_get_visible (column);

        gtk_list_store_append (model, &iter);
        gtk_list_store_set (model, &iter, 0, visible, 1, title, 2, column,-1);
    }

    g_list_free(columns);

}

void
procdialog_create_preferences_dialog (ProcmanApp *app)
{
    typedef SpinButtonUpdater SBU;

    static SBU interval_updater("update-interval");
    static SBU graph_interval_updater("graph-update-interval");
    static SBU disks_interval_updater("disks-interval");

    GtkWidget *notebook;
    GtkAdjustment *adjustment;
    GtkWidget *spin_button;
    GtkWidget *check_button;
    GtkWidget *smooth_button;
    GtkBuilder *builder;
    gfloat update;

    if (prefs_dialog)
        return;

    builder = gtk_builder_new();
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/preferences.ui", NULL);

    prefs_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "preferences_dialog"));

    notebook = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));

    spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "processes_interval_spinner"));

    update = (gfloat) app->config.update_interval;
    adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0,
                                                      MIN_UPDATE_INTERVAL / 1000,
                                                      MAX_UPDATE_INTERVAL / 1000,
                                                      0.25,
                                                      1.0,
                                                      0);

    gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON(spin_button), adjustment);
    g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK (SBU::callback), &interval_updater);

    smooth_button = GTK_WIDGET (gtk_builder_get_object (builder, "smooth_button"));
    g_settings_bind(app->settings, SmoothRefresh::KEY.c_str(), smooth_button, "active", G_SETTINGS_BIND_DEFAULT);

    check_button = GTK_WIDGET (gtk_builder_get_object (builder, "check_button"));
    g_settings_bind(app->settings, "kill-dialog", check_button, "active", G_SETTINGS_BIND_DEFAULT);

    GtkWidget *solaris_button = GTK_WIDGET (gtk_builder_get_object (builder, "solaris_button"));
    g_settings_bind(app->settings, procman::settings::solaris_mode.c_str(), solaris_button, "active", G_SETTINGS_BIND_DEFAULT);

    GtkWidget *draw_stacked_button = GTK_WIDGET (gtk_builder_get_object (builder, "draw_stacked_button"));
    g_settings_bind(app->settings, procman::settings::draw_stacked.c_str(), draw_stacked_button, "active", G_SETTINGS_BIND_DEFAULT);

    create_field_page (builder, app->tree, "proctree");

    update = (gfloat) app->config.graph_update_interval;
    adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0, 0.25,
                                                      100.0, 0.25, 1.0, 0);
    spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "resources_interval_spinner"));
    gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON(spin_button), adjustment);
    g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK(SBU::callback),
                      &graph_interval_updater);

    GtkWidget *bits_button = GTK_WIDGET (gtk_builder_get_object (builder, "bits_button"));
    g_settings_bind(app->settings, procman::settings::network_in_bits.c_str(), bits_button, "active", G_SETTINGS_BIND_DEFAULT);

    update = (gfloat) app->config.disks_update_interval;
    adjustment = (GtkAdjustment *) gtk_adjustment_new (update / 1000.0, 1.0,
                                                       100.0, 1.0, 1.0, 0);
    spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "devices_interval_spinner"));
    gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON(spin_button), adjustment);
    g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK(SBU::callback),
                      &disks_interval_updater);


    check_button = GTK_WIDGET (gtk_builder_get_object (builder, "all_devices_check"));
    g_settings_bind(app->settings, "show-all-fs", check_button, "active", G_SETTINGS_BIND_DEFAULT);

    create_field_page (builder, app->disk_list, "disktreenew");

    gtk_widget_show_all (prefs_dialog);
    g_signal_connect (G_OBJECT (prefs_dialog), "response",
                      G_CALLBACK (prefs_dialog_button_pressed), app);

    switch (g_settings_get_int (app->settings, "current-tab")) {
        case PROCMAN_TAB_PROCESSES:
            gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
            break;
        case PROCMAN_TAB_RESOURCES:
            gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 1);
            break;
        case PROCMAN_TAB_DISKS:
            gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 2);
            break;

    }
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
                                       ProcmanApp *app,
                                       gint pid,
                                       gint extra_value)
{
    char * command;
    gboolean ret = FALSE;

    command = procman_action_to_command(type, pid, extra_value);

    procman_debug("Trying to run '%s' as root", command);

    if (procman_has_pkexec())
        ret = procman_pkexec_create_root_password_dialog(command);
    else if (procman_has_gksu())
        ret = procman_gksu_create_root_password_dialog(command);
    else if (procman_has_gnomesu())
        ret = procman_gnomesu_create_root_password_dialog(command);

    g_free(command);
    return ret;
}
