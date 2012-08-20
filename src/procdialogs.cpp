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

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include "procdialogs.h"
#include "proctable.h"
#include "callbacks.h"
#include "prettytable.h"
#include "procactions.h"
#include "util.h"
#include "load-graph.h"
#include "settings-keys.h"
#include "procman_gnomesu.h"
#include "procman_gksu.h"
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
        kill_process (kargs->procdata, kargs->signal);

    g_free (kargs);
}

void
procdialog_create_kill_dialog (ProcData *procdata, int signal)
{
    GtkWidget *kill_alert_dialog;
    gchar *primary, *secondary, *button_text;
    struct KillArgs *kargs;

    kargs = g_new(KillArgs, 1);
    kargs->procdata = procdata;
    kargs->signal = signal;


    if (signal == SIGKILL) {
        /*xgettext: primary alert message*/
        primary = g_strdup_printf (_("Kill the selected process »%s« (PID: %u)?"),
                                   procdata->selected_process->name,
                                   procdata->selected_process->pid);
        /*xgettext: secondary alert message*/
        secondary = _("Killing a process may destroy data, break the "
                      "session or introduce a security risk. "
                      "Only unresponsive processes should be killed.");
        button_text = _("_Kill Process");
    }
    else {
        /*xgettext: primary alert message*/
        primary = g_strdup_printf (_("End the selected process »%s« (PID: %u)?"),
                                   procdata->selected_process->name,
                                   procdata->selected_process->pid);
        /*xgettext: secondary alert message*/
        secondary = _("Ending a process may destroy data, break the "
                      "session or introduce a security risk. "
                      "Only unresponsive processes should be ended.");
        button_text = _("_End Process");
    }

    kill_alert_dialog = gtk_message_dialog_new (GTK_WINDOW (procdata->app),
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
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
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
    gchar* text = g_strdup_printf(_("(%s Priority)"), procman::get_nice_level (new_nice_value));
    gtk_label_set_text (GTK_LABEL (label), text);
    g_free(text);

}

static void
renice_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);

    if (id == 100) {
        if (new_nice_value == -100)
            return;
        renice(procdata, new_nice_value);
    }

    gtk_widget_destroy (GTK_WIDGET (dialog));
    renice_dialog = NULL;
}

void
procdialog_create_renice_dialog (ProcData *procdata)
{
    ProcInfo *info = procdata->selected_process;
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

    gchar* filename = g_build_filename (GSM_DATA_DIR, "renice.ui", NULL);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, filename, NULL);

    renice_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "renice_dialog"));

    dialog_title = g_strdup_printf (_("Change Priority of Process »%s« (PID: %u)"),
                                    info->name, info->pid);

    gtk_window_set_title (GTK_WINDOW(renice_dialog), dialog_title);
    
    g_free (dialog_title);

    gtk_dialog_set_default_response (GTK_DIALOG (renice_dialog), 100);
    new_nice_value = -100;

    renice_adj = GTK_ADJUSTMENT (gtk_builder_get_object (builder, "renice_adj"));
    gtk_adjustment_configure( GTK_ADJUSTMENT(renice_adj), info->nice, RENICE_VAL_MIN, RENICE_VAL_MAX, 1, 1, 0);

    new_nice_value = 0;
    
    priority_label =  GTK_WIDGET (gtk_builder_get_object (builder, "priority_label"));
    gtk_label_set_label (GTK_LABEL(priority_label), procman::get_nice_level (info->nice));

    text = g_strconcat("<small><i><b>", _("Note:"), "</b> ",
                       _("The priority of a process is given by its nice value. A lower nice value corresponds to a higher priority."),
                       "</i></small>", NULL);
    label = GTK_WIDGET (gtk_builder_get_object (builder, "note_label"));
    gtk_label_set_label (GTK_LABEL(label), _(text));
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    g_free (text);

    g_signal_connect (G_OBJECT (renice_dialog), "response",
                      G_CALLBACK (renice_dialog_button_pressed), procdata);
    g_signal_connect (G_OBJECT (renice_adj), "value_changed",
                      G_CALLBACK (renice_scale_changed), priority_label);

    gtk_widget_show_all (renice_dialog);

    gtk_builder_connect_signals (builder, NULL);

    g_object_unref (G_OBJECT (builder));
    g_free (filename);
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


static void
show_kill_dialog_toggled (GtkToggleButton *button, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);
    GSettings *settings = procdata->settings;

    gboolean toggled;

    toggled = gtk_toggle_button_get_active (button);

    g_settings_set_boolean (settings, "kill-dialog", toggled);
}



static void
solaris_mode_toggled(GtkToggleButton *button, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);
    GSettings *settings = procdata->settings;

    gboolean toggled;
    toggled = gtk_toggle_button_get_active(button);
    g_settings_set_boolean(settings, procman::settings::solaris_mode.c_str(), toggled);
}


static void
network_in_bits_toggled(GtkToggleButton *button, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);
    GSettings *settings = procdata->settings;

    gboolean toggled;
    toggled = gtk_toggle_button_get_active(button);
    g_settings_set_boolean(settings, procman::settings::network_in_bits.c_str(), toggled);
}



static void
smooth_refresh_toggled(GtkToggleButton *button, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);
    GSettings *settings = procdata->settings;

    gboolean toggled;

    toggled = gtk_toggle_button_get_active(button);

    g_settings_set_boolean(settings, SmoothRefresh::KEY.c_str(), toggled);
}



static void
show_all_fs_toggled (GtkToggleButton *button, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);
    GSettings *settings = procdata->settings;

    gboolean toggled;

    toggled = gtk_toggle_button_get_active (button);

    g_settings_set_boolean (settings, "show-all-fs", toggled);
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

        g_settings_set_int(ProcData::get_instance()->settings,
                           this->key.c_str(), new_value);

        procman_debug("set %s to %d", this->key.c_str(), new_value);
    }

    const string key;
};




static void
field_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    GtkTreeModel *model = static_cast<GtkTreeModel*>(data);
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    gboolean toggled;
    GSettings *settings = g_settings_get_child (ProcData::get_instance()->settings, "proctree");
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
create_field_page(GtkBuilder* builder, GtkWidget *tree, const gchar *widgetname)
{
    GtkTreeView *treeview;
    GList *it, *columns;
    GtkListStore *model;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;

    treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, widgetname));
    model = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);

    gtk_tree_view_set_model (GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(model));
    g_object_unref (G_OBJECT (model));

    column = gtk_tree_view_column_new ();

    cell = gtk_cell_renderer_toggle_new ();
    gtk_tree_view_column_pack_start (column, cell, FALSE);
    gtk_tree_view_column_set_attributes (column, cell,
                                         "active", 0,
                                         NULL);
    g_signal_connect (G_OBJECT (cell), "toggled", G_CALLBACK (field_toggled), model);
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
            && sd_booted() <= 0
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
procdialog_create_preferences_dialog (ProcData *procdata)
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

    gchar* filename = g_build_filename (GSM_DATA_DIR, "preferences.ui", NULL);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, filename, NULL);

    prefs_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "preferences_dialog"));

    notebook = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));

    spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "processes_interval_spinner"));

    update = (gfloat) procdata->config.update_interval;
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
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(smooth_button),
                                 g_settings_get_boolean(procdata->settings,
                                                        SmoothRefresh::KEY.c_str()));
    g_signal_connect(G_OBJECT(smooth_button), "toggled",
                     G_CALLBACK(smooth_refresh_toggled), procdata);

    check_button = GTK_WIDGET (gtk_builder_get_object (builder, "check_button"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                  procdata->config.show_kill_warning);
    g_signal_connect (G_OBJECT (check_button), "toggled",
                      G_CALLBACK (show_kill_dialog_toggled), procdata);

    GtkWidget *solaris_button = GTK_WIDGET (gtk_builder_get_object (builder, "solaris_button"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(solaris_button),
                                 g_settings_get_boolean(procdata->settings,
                                                        procman::settings::solaris_mode.c_str()));
    g_signal_connect(G_OBJECT(solaris_button), "toggled",
                     G_CALLBACK(solaris_mode_toggled), procdata);

    create_field_page (builder, procdata->tree, "processes_columns_treeview");

    update = (gfloat) procdata->config.graph_update_interval;
    adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0, 0.25,
                                                      100.0, 0.25, 1.0, 0);
    spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "resources_interval_spinner"));
    gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON(spin_button), adjustment);
    g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK(SBU::callback),
                      &graph_interval_updater);

    GtkWidget *bits_button = GTK_WIDGET (gtk_builder_get_object (builder, "bits_button"));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bits_button),
                                 g_settings_get_boolean(procdata->settings,
                                                        procman::settings::network_in_bits.c_str()));
    g_signal_connect(G_OBJECT(bits_button), "toggled",
                     G_CALLBACK(network_in_bits_toggled), procdata);

    update = (gfloat) procdata->config.disks_update_interval;
    adjustment = (GtkAdjustment *) gtk_adjustment_new (update / 1000.0, 1.0,
                                                       100.0, 1.0, 1.0, 0);
    spin_button = GTK_WIDGET (gtk_builder_get_object (builder, "devices_interval_spinner"));
    gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON(spin_button), adjustment);
    g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK(SBU::callback),
                      &disks_interval_updater);


    check_button = GTK_WIDGET (gtk_builder_get_object (builder, "all_devices_check"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                  procdata->config.show_all_fs);
    g_signal_connect (G_OBJECT (check_button), "toggled",
                      G_CALLBACK (show_all_fs_toggled), procdata);

    create_field_page (builder, procdata->disk_list, "devices_columns_treeview");

    gtk_widget_show_all (prefs_dialog);
    g_signal_connect (G_OBJECT (prefs_dialog), "response",
                      G_CALLBACK (prefs_dialog_button_pressed), procdata);

    switch (procdata->config.current_tab) {
        case PROCMAN_TAB_SYSINFO:
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
    g_free (filename);
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
                                       ProcData *procdata,
                                       gint pid,
                                       gint extra_value)
{
    char * command;
    gboolean ret = FALSE;

    command = procman_action_to_command(type, pid, extra_value);

    procman_debug("Trying to run '%s' as root", command);

    if (procman_has_gksu())
        ret = procman_gksu_create_root_password_dialog(command);
    else if (procman_has_gnomesu())
        ret = procman_gnomesu_create_root_password_dialog(command);

    g_free(command);
    return ret;
}
