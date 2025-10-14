#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "prefsdialog.h"

#include "application.h"
#include "cgroups.h"
#include "proctable.h"
#include "selinux.h"
#include "settings-keys.h"
#include "systemd.h"
#include "util.h"
#include "update_interval.h"

static AdwPreferencesDialog *prefs_dialog = NULL;

static gboolean
prefs_dialog_close_request (AdwDialog*,
                            gpointer)
{
  prefs_dialog = NULL;
  return FALSE;
}

static void
spin_button_value_changed (AdwSpinRow *self,
                           gpointer    data)
{
  GString *key = (GString *) data;
  int new_value = 1000 * adw_spin_row_get_value (self);

  GsmApplication::get ().settings->set_int (key->str, new_value);

  procman_debug ("set %s to %d", key->str, new_value);
}

static void
range_value_changed (GtkRange *self,
                     gpointer  data)
{
  GString *key = (GString *) data;
  int new_value = gtk_range_get_value (self);

  GsmApplication::get ().settings->set_int (key->str, new_value);

  procman_debug ("set %s to %d", key->str, new_value);
}

static void
create_field_page (GtkBuilder  *builder,
                   GtkTreeView *tree,
                   const gchar *widgetname)
{
  GList *it, *columns;
  AdwPreferencesGroup *group;

  group = ADW_PREFERENCES_GROUP (gtk_builder_get_object (builder, widgetname));

  columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));

  auto get_value_label = [](GtkTreeViewColumn *col) -> const gchar *
  {
    GtkWidget *box = gtk_tree_view_column_get_widget(col);
    if (!GTK_IS_BOX(box))
      return nullptr;

    GtkWidget *child = gtk_widget_get_first_child(box);
    if (GTK_IS_LABEL(child))
      return gtk_label_get_text(GTK_LABEL(child));

    return nullptr;
  };

  for (it = columns; it; it = it->next)
    {
      GtkTreeViewColumn *column = static_cast<GtkTreeViewColumn*>(it->data);
      const gchar *title;
      gint column_id;
      GtkWidget *row;

      title = get_value_label(column);

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

      row = adw_switch_row_new ();
      adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);

      auto settings = GsmApplication::get ().settings->get_child (widgetname);
      auto key = Glib::ustring::compose ("col-%1-visible", column_id);

      g_settings_bind (settings->gobj (), key.c_str (), G_OBJECT (column), "visible", G_SETTINGS_BIND_DEFAULT);
      g_settings_bind (settings->gobj (), key.c_str (), G_OBJECT (row), "active", G_SETTINGS_BIND_DEFAULT);

      adw_preferences_group_add (group, GTK_WIDGET (row));
    }

  g_list_free (columns);
}

static void
create_field_page (GtkBuilder    *builder,
                   GtkColumnView *column_view,
                   const gchar   *widgetname)
{
  GListModel *columns;
  AdwPreferencesGroup *group;

  columns = gtk_column_view_get_columns (column_view);
  group = ADW_PREFERENCES_GROUP (gtk_builder_get_object (builder, widgetname));
  auto settings = GsmApplication::get ().settings->get_child (widgetname);

  for (guint i = 0; i < g_list_model_get_n_items (columns); i++)
    {
      GtkColumnViewColumn *column = GTK_COLUMN_VIEW_COLUMN (g_list_model_get_object (columns, i));
      const gchar *title;
      const gchar *column_id;
      GtkWidget *row;

      title = gtk_column_view_column_get_title (column);
      if (!title)
        title = _("Icon");

      column_id = gtk_column_view_column_get_id (column);
      row = adw_switch_row_new ();
      adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);

      auto key = Glib::ustring::compose ("col-%1-visible", column_id);

      g_settings_bind (settings->gobj (), key.c_str (), G_OBJECT (column), "visible", G_SETTINGS_BIND_DEFAULT);
      g_settings_bind (settings->gobj (), key.c_str (), G_OBJECT (row), "active", G_SETTINGS_BIND_DEFAULT);

      adw_preferences_group_add (group, GTK_WIDGET (row));
    }
}

static void
switch_preferences_page (GtkBuilder   *builder,
                         AdwViewStack *stack)
{
  const gchar *stack_name = adw_view_stack_get_visible_child_name (stack);

  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (gtk_builder_get_object (builder, stack_name));

  if (page != NULL)
    adw_preferences_dialog_set_visible_page (prefs_dialog, page);
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
  GError*err = NULL;

  if (prefs_dialog)
    return;

  builder = gtk_builder_new ();
  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/preferences.ui", &err);
  if (err != NULL)
    {
      procman_debug ("problem loading preferences ui %s", err->message);
      g_error_free (err);
    }

  prefs_dialog = ADW_PREFERENCES_DIALOG (gtk_builder_get_object (builder, "preferences_dialog"));

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
                            100.0, 0.05, 0.5, 0);

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

  update = (gfloat) g_settings_get_int (app->settings->gobj (), GSM_SETTING_DISKS_UPDATE_INTERVAL);
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

  create_field_page (builder, gsm_disks_view_get_column_view (app->disk_list),
                     GSM_SETTINGS_CHILD_DISKS);

  g_signal_connect (G_OBJECT (prefs_dialog), "closed",
                    G_CALLBACK (prefs_dialog_close_request), NULL);

  switch_preferences_page (builder, app->stack);

  adw_dialog_present (ADW_DIALOG (prefs_dialog), GTK_WIDGET (GsmApplication::get ().main_window));

  g_object_unref (G_OBJECT (builder));
}
