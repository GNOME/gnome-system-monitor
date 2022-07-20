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

static AdwPreferencesWindow *prefs_window = NULL;

static gboolean
prefs_window_close_request  (GtkWindow *self,
                             gpointer   data)
{
    prefs_window = NULL;
    return FALSE;
}

class SpinButtonUpdater
{
public:
    SpinButtonUpdater(const string&key)
        : key (key)
    {
    }

    /*static gboolean callback (GtkWidget     *widget,
                              GdkEventFocus *event,
                              gpointer       data)
    {
        SpinButtonUpdater*updater = static_cast<SpinButtonUpdater*>(data);
        gtk_spin_button_update (GTK_SPIN_BUTTON (widget));
        updater->update (GTK_SPIN_BUTTON (widget));
        return FALSE;
    }*/

private:

    void update (GtkSpinButton*spin)
    {
        int new_value = 1000 * gtk_spin_button_get_value (spin);

        GsmApplication::get ()->settings->set_int (this->key, new_value);

        procman_debug ("set %s to %d", this->key.c_str (), new_value);
    }

    const string key;
};

class ScaleUpdater
{
public:
    ScaleUpdater(const string&key)
        : key (key)
    {
    }

    static gboolean callback (GtkRange *range,
                              gpointer  data)
    {
        ScaleUpdater*updater = static_cast<ScaleUpdater*>(data);
        updater->update (range);
        return FALSE;
    }

private:

    void update (GtkRange*range)
    {
        int new_value = gtk_range_get_value (range);

        GsmApplication::get ()->settings->set_int (this->key, new_value);

        procman_debug ("set %s to %d", this->key.c_str (), new_value);
    }

    const string key;
};

static void
field_toggled (const gchar *gsettings_parent,
               gchar       *path_str,
               gpointer     data)
{
    GtkTreeModel *model = static_cast<GtkTreeModel*>(data);
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    gboolean toggled;
    auto settings = GsmApplication::get ()->settings->get_child (gsettings_parent);
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
field_row_activated (GtkTreeView       *tree,
                     GtkTreePath       *path,
                     GtkTreeViewColumn *column,
                     gpointer           data)
{
    GtkTreeModel *model = gtk_tree_view_get_model (tree);
    gchar *path_str = gtk_tree_path_to_string (path);
    field_toggled ((gchar*)data, path_str, model);
    g_free (path_str);
}

static void
proc_field_toggled (GtkCellRendererToggle *cell,
                    gchar                 *path_str,
                    gpointer               data)
{
    field_toggled ("proctree", path_str, data);
}

static void
disk_field_toggled (GtkCellRendererToggle *cell,
                    gchar                 *path_str,
                    gpointer               data)
{
    field_toggled ("disktreenew", path_str, data);
}

static void
create_field_page (GtkBuilder  *builder,
                   GtkTreeView *tree,
                   const gchar *widgetname)
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

    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (model));
    g_object_unref (G_OBJECT (model));

    column = gtk_tree_view_column_new ();

    cell = gtk_cell_renderer_toggle_new ();
    gtk_tree_view_column_pack_start (column, cell, FALSE);
    gtk_tree_view_column_set_attributes (column, cell,
                                         "active", 0,
                                         NULL);
    if (!g_strcmp0 (widgetname, "proctree"))
        g_signal_connect (G_OBJECT (cell), "toggled", G_CALLBACK (proc_field_toggled), model);
    else if (!g_strcmp0 (widgetname, "disktreenew"))
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

    for (it = columns; it; it = it->next) {
        GtkTreeViewColumn *column = static_cast<GtkTreeViewColumn*>(it->data);
        GtkTreeIter iter;
        const gchar *title;
        gboolean visible;
        gint column_id;

        title = gtk_tree_view_column_get_title (column);
        if (!title)
            title = _("Icon");

        column_id = gtk_tree_view_column_get_sort_column_id (column);
        if ((column_id == COL_CGROUP) && (!cgroups_enabled ()))
            continue;
        if ((column_id == COL_SECURITYCONTEXT) && (!can_show_security_context_column ()))
            continue;

        if ((column_id == COL_UNIT ||
             column_id == COL_SESSION ||
             column_id == COL_SEAT ||
             column_id == COL_OWNER)
            && !procman::systemd_logind_running ()
            )
            continue;

        visible = gtk_tree_view_column_get_visible (column);

        gtk_list_store_append (model, &iter);
        gtk_list_store_set (model, &iter, 0, visible, 1, title, 2, column,-1);
    }

    g_list_free (columns);
}

void
create_preferences_dialog (GsmApplication *app)
{
    static SpinButtonUpdater interval_updater ("update-interval");
    static SpinButtonUpdater graph_interval_updater ("graph-update-interval");
    static SpinButtonUpdater disks_interval_updater ("disks-interval");
    static ScaleUpdater graph_points_updater ("graph-data-points");

    GtkAdjustment *adjustment;
    GtkSpinButton *spin_button;
    GtkSwitch *check_switch;
    GtkSwitch *smooth_switch;
    GtkBuilder *builder;
    gfloat update;

    if (prefs_window)
        return;

    builder = gtk_builder_new ();
    GError *err = NULL;
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/preferences.ui", &err);
    if (err != NULL)
      g_error("%s", err->message);

    prefs_window = ADW_PREFERENCES_WINDOW (gtk_builder_get_object (builder, "preferences_dialog"));

    spin_button = GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "processes_interval_spinner"));

    update = (gfloat) app->config.update_interval;
    adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin_button));
    gtk_adjustment_configure (adjustment,
                              update / 1000.0,
                              MIN_UPDATE_INTERVAL / 1000,
                              MAX_UPDATE_INTERVAL / 1000,
                              0.25,
                              1.0,
                              0);
    /*g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK (SBU::callback), &interval_updater);*/

    smooth_switch = GTK_SWITCH (gtk_builder_get_object (builder, "smooth_switch"));
    g_settings_bind (app->settings->gobj (), SmoothRefresh::KEY.c_str (), smooth_switch, "active", G_SETTINGS_BIND_DEFAULT);

    check_switch = GTK_SWITCH (gtk_builder_get_object (builder, "check_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_SHOW_KILL_DIALOG,
                     check_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkSwitch *solaris_switch = GTK_SWITCH (gtk_builder_get_object (builder, "solaris_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_SOLARIS_MODE,
                     solaris_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkSwitch *proc_mem_in_iec_switch = GTK_SWITCH (gtk_builder_get_object (builder, "proc_mem_in_iec_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_PROCESS_MEMORY_IN_IEC,
                     proc_mem_in_iec_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkSwitch *logarithmic_scale_switch = GTK_SWITCH (gtk_builder_get_object (builder, "logarithmic_scale_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_LOGARITHMIC_SCALE,
                     logarithmic_scale_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkSwitch *draw_stacked_switch = GTK_SWITCH (gtk_builder_get_object (builder, "draw_stacked_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_DRAW_STACKED,
                     draw_stacked_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkSwitch *draw_smooth_switch = GTK_SWITCH (gtk_builder_get_object (builder, "draw_smooth_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_DRAW_SMOOTH,
                     draw_smooth_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkSwitch *res_mem_in_iec_switch = GTK_SWITCH (gtk_builder_get_object (builder, "res_mem_in_iec_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_RESOURCES_MEMORY_IN_IEC,
                     res_mem_in_iec_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    create_field_page (builder, GTK_TREE_VIEW (app->tree), "proctree");

    update = (gfloat) app->config.graph_update_interval;
    spin_button = GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "resources_interval_spinner"));
    adjustment = gtk_spin_button_get_adjustment (spin_button);
    gtk_adjustment_configure (adjustment, update / 1000.0, 0.05,
                              10.0, 0.05, 0.5, 0);
    /*g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK (SBU::callback),
                      &graph_interval_updater);*/

    update = (gfloat) app->config.graph_data_points;
    GtkRange*range = GTK_RANGE (gtk_builder_get_object (builder, "graph_data_points_scale"));
    adjustment = gtk_range_get_adjustment (range);
    gtk_adjustment_configure (adjustment, update, 30,
                              600, 10, 60, 0);
    g_signal_connect (G_OBJECT (range), "value-changed",
                      G_CALLBACK (ScaleUpdater::callback),
                      &graph_points_updater);

    GtkSwitch *bits_switch = GTK_SWITCH (gtk_builder_get_object (builder, "bits_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_NETWORK_IN_BITS,
                     bits_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkSwitch *bits_unit_switch = GTK_SWITCH (gtk_builder_get_object (builder, "bits_unit_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_NETWORK_TOTAL_UNIT,
                     bits_unit_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    GtkSwitch *bits_total_switch = GTK_SWITCH (gtk_builder_get_object (builder, "bits_total_switch"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_NETWORK_TOTAL_IN_BITS,
                     bits_total_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    update = (gfloat) app->config.disks_update_interval;
    spin_button = GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "devices_interval_spinner"));
    adjustment = gtk_spin_button_get_adjustment (spin_button);
    gtk_adjustment_configure (adjustment, update / 1000.0, 1.0,
                              100.0, 1.0, 1.0, 0);
    /*g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
                      G_CALLBACK (SBU::callback),
                      &disks_interval_updater);*/


    check_switch = GTK_SWITCH (gtk_builder_get_object (builder, "all_devices_check"));
    g_settings_bind (app->settings->gobj (), GSM_SETTING_SHOW_ALL_FS,
                     check_switch, "active",
                     G_SETTINGS_BIND_DEFAULT);

    create_field_page (builder, GTK_TREE_VIEW (app->disk_list), "disktreenew");

    gtk_window_set_transient_for (GTK_WINDOW (prefs_window), GTK_WINDOW (GsmApplication::get ()->main_window));
    gtk_window_set_modal (GTK_WINDOW (prefs_window), TRUE);

    g_signal_connect (G_OBJECT (prefs_window), "close-request",
                       G_CALLBACK (prefs_window_close_request), NULL);

    gtk_widget_show (GTK_WIDGET (prefs_window));

    g_object_unref (G_OBJECT (builder));
}
