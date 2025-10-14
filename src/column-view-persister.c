/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include "settings-keys.h"

#include "column-view-persister.h"


struct _GsmColumnViewPersister {
  GObject parent;

  char *name;
  GtkColumnView *view;

  GSettings *settings;
  GSettings *view_settings;

  GSignalGroup *column_signals;
  GSignalGroup *sorter_signals;
  GBindingGroup *view_binds;
};


G_DEFINE_FINAL_TYPE (GsmColumnViewPersister, gsm_column_view_persister, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_NAME,
  PROP_VIEW,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
gsm_column_view_persister_dispose (GObject *object)
{
  GsmColumnViewPersister *self = GSM_COLUMN_VIEW_PERSISTER (object);

  g_clear_pointer (&self->name, g_free);
  g_clear_weak_pointer (&self->view);

  g_clear_object (&self->settings);
  g_clear_object (&self->view_settings);

  g_clear_object (&self->column_signals);
  g_clear_object (&self->sorter_signals);
  g_clear_object (&self->view_binds);

  G_OBJECT_CLASS (gsm_column_view_persister_parent_class)->dispose (object);
}


static void
gsm_column_view_persister_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GsmColumnViewPersister *self = GSM_COLUMN_VIEW_PERSISTER (object);

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_VIEW:
      g_value_set_object (value, self->view);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
gsm_column_view_persister_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GsmColumnViewPersister *self = GSM_COLUMN_VIEW_PERSISTER (object);

  switch (property_id) {
    case PROP_NAME:
      if (g_set_str (&self->name, g_value_get_string (value))) {
        g_clear_object (&self->view_settings);
        self->view_settings = g_settings_get_child (self->settings, self->name);
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    case PROP_VIEW:
      if (g_set_weak_pointer (&self->view, g_value_get_object (value))) {
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static inline gboolean
configure_column (GsmColumnViewPersister *self,
                  GtkColumnViewColumn    *column,
                  const char             *target,
                  size_t                  pos)
{
  const char *column_id = gtk_column_view_column_get_id (column);

  if (g_strcmp0 (column_id, target) == 0) {
    g_autofree char *visible_col =
      g_strdup_printf ("col-%s-visible", column_id);
    g_autofree char *width_col =
      g_strdup_printf ("col-%s-width", column_id);

    gtk_column_view_insert_column (self->view, pos, column);

    if (g_strcmp0 (g_settings_get_string (self->view_settings, "sort-col"), column_id) == 0) {
      GtkSortType direction = g_settings_get_int (self->view_settings, "sort-order") ?
                                  GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
      gtk_column_view_sort_by_column (self->view, column, direction);
    }

    g_settings_bind (self->view_settings, visible_col,
                     G_OBJECT (column), "visible",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (self->view_settings, width_col,
                     G_OBJECT (column), "fixed-width",
                     G_SETTINGS_BIND_DEFAULT);

    return TRUE;
  }

  return FALSE;
}


static void
columns_changed (GListModel          *columns,
                 G_GNUC_UNUSED guint  position,
                 G_GNUC_UNUSED guint  removed,
                 G_GNUC_UNUSED guint  added,
                 gpointer             user_data)
{
  GsmColumnViewPersister *self = GSM_COLUMN_VIEW_PERSISTER (user_data);
  g_auto (GStrv) order = NULL;
  guint num_columns;

  num_columns = g_list_model_get_n_items (columns);
  order = g_new0 (char *, num_columns + 1);

  for (size_t pos = 0; pos < num_columns; pos++) {
    GtkColumnViewColumn *column =
      GTK_COLUMN_VIEW_COLUMN (g_list_model_get_object (columns, pos));
    order[pos] = g_strdup (gtk_column_view_column_get_id (column));
  }

  g_settings_set_strv (self->view_settings,
                       "columns-order",
                       (const char * const *) order);
}


static void
sort_changed (GtkSorter                     *sorter,
              G_GNUC_UNUSED GtkSorterChange  change,
              gpointer                       user_data)
{
  GsmColumnViewPersister *self = GSM_COLUMN_VIEW_PERSISTER (user_data);
  GtkColumnViewColumn *column;
  GtkSortType sort_type;

  column =
    gtk_column_view_sorter_get_primary_sort_column (GTK_COLUMN_VIEW_SORTER (sorter));

  g_settings_set_string (self->view_settings,
                         "sort-col",
                         gtk_column_view_column_get_id (column));
  sort_type =
    gtk_column_view_sorter_get_primary_sort_order (GTK_COLUMN_VIEW_SORTER (sorter));

  g_settings_set_int (self->view_settings,
                      "sort-order",
                      sort_type == GTK_SORT_ASCENDING ? 0 : 1);
}


static void
gsm_column_view_persister_constructed (GObject *object)
{
  GsmColumnViewPersister *self = GSM_COLUMN_VIEW_PERSISTER (object);
  GListModel *columns;
  g_auto (GStrv) order = NULL;

  G_OBJECT_CLASS (gsm_column_view_persister_parent_class)->constructed (object);

  columns = gtk_column_view_get_columns (self->view);
  order = g_settings_get_strv (self->view_settings, "columns-order");

  g_signal_group_block (self->column_signals);
  g_signal_group_block (self->sorter_signals);

  for (size_t pos = 0; order[pos] != NULL; pos++) {
    for (size_t column_pos = 0;
         column_pos < g_list_model_get_n_items (columns);
         column_pos++) {
      GtkColumnViewColumn *column =
        GTK_COLUMN_VIEW_COLUMN (g_list_model_get_object (columns, column_pos));

      if (configure_column (self, column, order[pos], pos)) {
        break;
      }
    }
  }

  g_signal_group_unblock (self->sorter_signals);
  g_signal_group_unblock (self->column_signals);
}


static void
gsm_column_view_persister_class_init (GsmColumnViewPersisterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gsm_column_view_persister_dispose;
  object_class->get_property = gsm_column_view_persister_get_property;
  object_class->set_property = gsm_column_view_persister_set_property;
  object_class->constructed = gsm_column_view_persister_constructed;

  pspecs[PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  pspecs[PROP_VIEW] =
    g_param_spec_object ("view", NULL, NULL,
                         GTK_TYPE_COLUMN_VIEW,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
gsm_column_view_persister_init (GsmColumnViewPersister *self)
{
  self->settings = g_settings_new (GSM_GSETTINGS_SCHEMA);

  self->column_signals = g_signal_group_new (G_TYPE_LIST_MODEL);
  g_signal_group_connect_object (self->column_signals, "items-changed",
                                 G_CALLBACK (columns_changed), self,
                                 G_CONNECT_DEFAULT);

  self->sorter_signals = g_signal_group_new (GTK_TYPE_SORTER);
  g_signal_group_connect_object (self->sorter_signals, "changed",
                                 G_CALLBACK (sort_changed), self,
                                 G_CONNECT_DEFAULT);

  self->view_binds = g_binding_group_new ();
  g_binding_group_bind (self->view_binds, "columns",
                        self->column_signals, "target",
                        G_BINDING_SYNC_CREATE);
  g_binding_group_bind (self->view_binds, "sorter",
                        self->sorter_signals, "target",
                        G_BINDING_SYNC_CREATE);

  g_object_bind_property (self, "view",
                          self->view_binds, "source",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
}
