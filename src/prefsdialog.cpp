/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "prefsdialog.h"

#include "cgroups.h"
#include "proctable.h"
#include "selinux.h"
#include "settings-keys.h"
#include "systemd.h"
#include "util.h"

static GtkDialog *prefs_dialog = NULL;

static void
prefs_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
    gtk_widget_destroy (GTK_WIDGET (dialog));
    prefs_dialog = NULL;
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

        GsmApplication::get()->settings->set_int(this->key, new_value);

        procman_debug("set %s to %d", this->key.c_str(), new_value);
    }

    const string key;
};

static void
field_toggled (const gchar *gsettings_parent, gchar *path_str, gpointer data)
{
    GtkTreeModel *model = static_cast<GtkTreeModel*>(data);
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    gboolean toggled;
    auto settings = GsmApplication::get()->settings->get_child (gsettings_parent);
    int id;

    if (!path)
        return;

    gtk_tree_model_get_iter (model, &iter, path);

    gtk_tree_model_get (model, &iter, 2, &column, -1);

    gtk_tree_model_get (model, &iter, 0, &toggled, -1);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, !toggled, -1);
    gtk_tree_view_column_set_visible (column, !toggled);

    id = gtk_tree_view_column_get_sort_column_id (column);

    auto key = Glib::ustring::compose ("col-%1-visible", id);
    settings->set_boolean (key, !toggled);

    gtk_tree_path_free (path);

}

static void 
field_row_activated ( GtkTreeView *tree, GtkTreePath *path, 
                      GtkTreeViewColumn *column, gpointer data)
{
    GtkTreeModel * model = gtk_tree_view_get_model (tree);
    gchar * path_str = gtk_tree_path_to_string (path);
    field_toggled((gchar*)data, path_str, model );
    g_free (path_str);
}

static void
proc_field_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    field_toggled("proctree", path_str, data);
}

static void
disk_field_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    field_toggled("disktreenew", path_str, data);
}

static void
create_field_page(GtkBuilder* builder, GtkTreeView *tree, const gchar *widgetname)
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

    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL(model));
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
        
    g_signal_connect (G_OBJECT (GTK_TREE_VIEW (treeview)), "row-activated", G_CALLBACK (field_row_activated), (gpointer)widgetname);
    
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
        if ((column_id == COL_SECURITYCONTEXT) && (!can_show_security_context_column ()))
            continue;

        if ((column_id == COL_UNIT ||
             column_id == COL_SESSION ||
             column_id == COL_SEAT ||
             column_id == COL_OWNER)
            && !procman::systemd_logind_running()
            )
            continue;

        visible = gtk_tree_view_column_get_visible (column);

        gtk_list_store_append (model, &iter);
        gtk_list_store_set (model, &iter, 0, visible, 1, title, 2, column,-1);
    }

    g_list_free(columns);

}

void
create_preferences_dialog (GsmApplication *app)
{
    typedef SpinButtonUpdater SBU;

    static SBU interval_updater("update-interval");
    static SBU graph_interval_updater("graph-update-interval");
    static SBU disks_interval_updater("disks-interval");

    GtkNotebook *notebook;
    GtkAdjustment *adjustment;
    GtkSpinButton *spin_button;
    GtkCheckButton *check_button;
    GtkCheckButton *smooth_button;
    GtkBuilder *builder;
    gfloat update;

    if (prefs_dialog)
        return;

    builder = gtk_builder_new();
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/preferences.ui", NULL);

    prefs_dialog = GTK_DIALOG (gtk_builder_get_object (builder, "preferences_dialog"));

    notebook = GTK_NOTEBOOK (gtk_builder_get_object (builder, "notebook"));

    spin_button = GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "processes_interval_spinner"));

    update = (gfloat) app->config.update_interval;
    adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(spin_button));
    gtk_adjustment_configure (adjustment,
                              update / 1000.0,
                              MIN_UPDATE_INTERVAL / 1000,
                              MAX_UPDATE_INTERVAL / 1000,
                              0.25,
                              1.0,
                              0);
    g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK (SBU::callback), &interval_updater);

    smooth_button = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "smooth_button"));
    g_settings_bind(app->settings->gobj (), SmoothRefresh::KEY.c_str(), smooth_button, "active", G_SETTINGS_BIND_DEFAULT);

    check_button = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "check_button"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_SHOW_KILL_DIALOG,
                     check_button, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkCheckButton *solaris_button = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "solaris_button"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_SOLARIS_MODE,
                     solaris_button, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkCheckButton *draw_stacked_button = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "draw_stacked_button"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_DRAW_STACKED,
                     draw_stacked_button, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkCheckButton *draw_smooth_button = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "draw_smooth_button"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_DRAW_SMOOTH,
                     draw_smooth_button, "active",
                     G_SETTINGS_BIND_DEFAULT);

    create_field_page (builder, GTK_TREE_VIEW (app->tree), "proctree");

    update = (gfloat) app->config.graph_update_interval;
    spin_button = GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "resources_interval_spinner"));                                                 
    adjustment = gtk_spin_button_get_adjustment (spin_button);
    gtk_adjustment_configure (adjustment, update / 1000.0, 0.25,
                              100.0, 0.25, 1.0, 0);
    g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK(SBU::callback),
                      &graph_interval_updater);

    GtkCheckButton *bits_button = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "bits_button"));
    g_settings_bind(app->settings->gobj (), GSM_SETTING_NETWORK_IN_BITS,
                    bits_button, "active",
                    G_SETTINGS_BIND_DEFAULT);

    update = (gfloat) app->config.disks_update_interval;
    spin_button = GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "devices_interval_spinner"));
    adjustment = gtk_spin_button_get_adjustment (spin_button);
    gtk_adjustment_configure (adjustment, update / 1000.0, 1.0,
                              100.0, 1.0, 1.0, 0);
    g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK(SBU::callback),
                      &disks_interval_updater);


    check_button = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "all_devices_check"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_SHOW_ALL_FS,
                     check_button, "active",
                     G_SETTINGS_BIND_DEFAULT);

    create_field_page (builder, GTK_TREE_VIEW (app->disk_list), "disktreenew");

    gtk_window_set_transient_for (GTK_WINDOW (prefs_dialog), GTK_WINDOW (GsmApplication::get()->main_window));

    gtk_widget_show_all (GTK_WIDGET (prefs_dialog));
    g_signal_connect (G_OBJECT (prefs_dialog), "response",
                      G_CALLBACK (prefs_dialog_button_pressed), app);

    auto current_tab = app->settings->get_string(GSM_SETTING_CURRENT_TAB);
    if (current_tab == "processes")
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
    else if (current_tab == "resources")
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 1);
    else if (current_tab == "disks")
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 2);

    gtk_builder_connect_signals (builder, NULL);
    g_object_unref (G_OBJECT (builder));
}

