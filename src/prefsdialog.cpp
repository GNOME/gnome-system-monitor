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
prefs_window_close_request (GtkWindow*,
                            gpointer)
{
  prefs_window = NULL;
  return FALSE;
}

static void
spin_button_value_changed (AdwSpinRow *self,
                           gpointer    data)
{
  GString *key = (GString *) data;
  int new_value = 1000 * adw_spin_row_get_value (self);

  GsmApplication::get ()->settings->set_int (key->str, new_value);

  procman_debug ("set %s to %d", key->str, new_value);
}

static void
range_value_changed (GtkRange *self,
                     gpointer  data)
{
  GString *key = (GString *) data;
  int new_value = gtk_range_get_value (self);

  GsmApplication::get ()->settings->set_int (key->str, new_value);

  procman_debug ("set %s to %d", key->str, new_value);
}

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

  if (!path)
    return;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, 2, &column, -1);
  gtk_tree_model_get (model, &iter, 0, &toggled, -1);

  toggled = !toggled;

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, toggled, -1);
  gtk_tree_view_column_set_visible (column, toggled);

  auto id = gtk_tree_view_column_get_sort_column_id (column);
  auto key = Glib::ustring::compose ("col-%1-visible", id);
  auto settings = GsmApplication::get ()->settings->get_child (gsettings_parent);
  settings->set_boolean (key, toggled);

  gtk_tree_path_free (path);
}

static void
field_row_activated (GtkTreeView       *tree,
                     GtkTreePath       *path,
                     GtkTreeViewColumn*,
                     gpointer           data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  gchar *path_str = gtk_tree_path_to_string (path);

  field_toggled ((gchar*)data, path_str, model);
  g_free (path_str);
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
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  column = gtk_tree_view_column_new ();
  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, FALSE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text", 1,
                                       NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  g_signal_connect (G_OBJECT (GTK_TREE_VIEW (treeview)), "row-activated",
                    G_CALLBACK (field_row_activated), (gpointer)widgetname);

  columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));

  for (it = columns; it; it = it->next)
    {
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
      gtk_list_store_set (model, &iter, 0, visible, 1, title, 2, column, -1);
    }

  g_list_free (columns);
}

static void
switch_preferences_page (GtkBuilder   *builder,
                         AdwViewStack *stack)
{
  const gchar *stack_name = adw_view_stack_get_visible_child_name (stack);

  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (gtk_builder_get_object (builder, stack_name));

  if (page != NULL)
    adw_preferences_window_set_visible_page (prefs_window, page);
}

void
create_preferences_dialog (GsmApplication *app)
{
  GtkAdjustment *adjustment;
  AdwSpinRow *spin_button;
  AdwSwitchRow *check_switch;
  AdwSwitchRow *smooth_switch;
  GtkBuilder *builder;
  gfloat update;
  GError* err = NULL;

  if (prefs_window)
    return;

  builder = gtk_builder_new ();
  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/preferences.ui", &err);
  if (err != NULL) {
    procman_debug ("problem loading preferences ui %s", err->message);
    g_error_free (err);
  }

  prefs_window = ADW_PREFERENCES_WINDOW (gtk_builder_get_object (builder, "preferences_dialog"));

  spin_button = ADW_SPIN_ROW (gtk_builder_get_object (builder, "processes_interval_spinner"));
  update = (gfloat) app->config.update_interval;
  adjustment = adw_spin_row_get_adjustment (ADW_SPIN_ROW (spin_button));
  gtk_adjustment_configure (adjustment,
                            update / 1000.0,
                            MIN_UPDATE_INTERVAL / 1000,
                            MAX_UPDATE_INTERVAL / 1000,
                            0.25,
                            1.0,
                            0);
  g_signal_connect (G_OBJECT (spin_button), "changed",
                    G_CALLBACK (spin_button_value_changed), g_string_new ("update-interval"));

  smooth_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "smooth_switch"));
  g_settings_bind (app->settings->gobj (), SmoothRefresh::KEY.c_str (), smooth_switch, "active", G_SETTINGS_BIND_DEFAULT);

  check_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "check_switch"));
  g_settings_bind (app->settings->gobj (), GSM_SETTING_SHOW_KILL_DIALOG,
                   check_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  AdwSwitchRow *solaris_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "solaris_switch"));

  g_settings_bind (app->settings->gobj (), GSM_SETTING_SOLARIS_MODE,
                   solaris_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  AdwSwitchRow *proc_mem_in_iec_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "proc_mem_in_iec_switch"));

  g_settings_bind (app->settings->gobj (), GSM_SETTING_PROCESS_MEMORY_IN_IEC,
                   proc_mem_in_iec_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  AdwSwitchRow *logarithmic_scale_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "logarithmic_scale_switch"));

  g_settings_bind (app->settings->gobj (), GSM_SETTING_LOGARITHMIC_SCALE,
                   logarithmic_scale_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  AdwSwitchRow *draw_stacked_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "draw_stacked_switch"));

  g_settings_bind (app->settings->gobj (), GSM_SETTING_DRAW_STACKED,
                   draw_stacked_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  AdwSwitchRow *draw_smooth_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "draw_smooth_switch"));

  g_settings_bind (app->settings->gobj (), GSM_SETTING_DRAW_SMOOTH,
                   draw_smooth_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  AdwSwitchRow *res_mem_in_iec_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "res_mem_in_iec_switch"));

  g_settings_bind (app->settings->gobj (), GSM_SETTING_RESOURCES_MEMORY_IN_IEC,
                   res_mem_in_iec_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  create_field_page (builder, GTK_TREE_VIEW (app->tree), GSM_SETTINGS_CHILD_PROCESSES);

  update = (gfloat) app->config.graph_update_interval;
  spin_button = ADW_SPIN_ROW (gtk_builder_get_object (builder, "resources_interval_spinner"));
  adjustment = adw_spin_row_get_adjustment (spin_button);
  gtk_adjustment_configure (adjustment, update / 1000.0, 0.05,
                            10.0, 0.05, 0.5, 0);

  g_signal_connect (G_OBJECT (spin_button), "changed",
                    G_CALLBACK (spin_button_value_changed), g_string_new ("graph-update-interval"));

  update = (gfloat) app->config.graph_data_points;
  GtkRange*range = GTK_RANGE (gtk_builder_get_object (builder, "graph_data_points_scale"));

  adjustment = gtk_range_get_adjustment (range);
  gtk_adjustment_configure (adjustment, update, 30,
                            600, 10, 60, 0);
  g_signal_connect (G_OBJECT (range), "value-changed",
                    G_CALLBACK (range_value_changed), g_string_new ("graph-data-points"));

  AdwSwitchRow *bits_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "bits_switch"));

  g_settings_bind (app->settings->gobj (), GSM_SETTING_NETWORK_IN_BITS,
                   bits_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  AdwSwitchRow *bits_total_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "bits_total_switch"));

  g_settings_bind (app->settings->gobj (), GSM_SETTING_NETWORK_TOTAL_IN_BITS,
                   bits_total_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  update = (gfloat) app->config.disks_update_interval;
  spin_button = ADW_SPIN_ROW (gtk_builder_get_object (builder, "devices_interval_spinner"));
  adjustment = adw_spin_row_get_adjustment (spin_button);
  gtk_adjustment_configure (adjustment, update / 1000.0, 1.0,
                            100.0, 1.0, 1.0, 0);
  g_signal_connect (G_OBJECT (spin_button), "changed",
                    G_CALLBACK (spin_button_value_changed), g_string_new ("disks-interval"));

  check_switch = ADW_SWITCH_ROW (gtk_builder_get_object (builder, "all_devices_check"));
  g_settings_bind (app->settings->gobj (), GSM_SETTING_SHOW_ALL_FS,
                   check_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  create_field_page (builder, GTK_TREE_VIEW (app->disk_list), GSM_SETTINGS_CHILD_DISKS);

  gtk_window_set_transient_for (GTK_WINDOW (prefs_window), GTK_WINDOW (GsmApplication::get ()->main_window));
  gtk_window_set_modal (GTK_WINDOW (prefs_window), TRUE);

  g_signal_connect (G_OBJECT (prefs_window), "close-request",
                    G_CALLBACK (prefs_window_close_request), NULL);

  switch_preferences_page (builder, app->stack);

  gtk_window_present (GTK_WINDOW (prefs_window));

  g_object_unref (G_OBJECT (builder));
}
