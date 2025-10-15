/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <glibtop/mountlist.h>
#include <glibtop/fsusage.h>
#include <adwaita.h>

#include "column-view-persister.h"
#include "disk.h"
#include "settings-keys.h"

#include "disks.h"


struct _GsmDisksView {
  AdwBin parent_instance;

  GtkColumnView *column_view;
  GtkSingleSelection *selection;
  GListStore *list_store;
  /* We don't actually use this from C,
   * but if we don't bind it it'll be disposed */
  GsmColumnViewPersister *persister;
  GSignalGroup *root_signals;
  GSignalGroup *monitor_signals;

  int update_interval;
  gboolean show_all_fs;

  guint timeout;
  GHashTable *known_directories;
  GSettings *settings;
  GVolumeMonitor *monitor;
};


G_DEFINE_FINAL_TYPE (GsmDisksView, gsm_disks_view, ADW_TYPE_BIN)


enum {
  PROP_0,
  PROP_UPDATE_INTERVAL,
  PROP_SHOW_ALL_FS,
  N_PROPS
};
static GParamSpec *properties[N_PROPS];


static inline void
clear_timeout (GsmDisksView *self)
{
  g_debug ("disks: clear timeout");
  g_clear_handle_id (&self->timeout, g_source_remove);
}


static void
gsm_disks_view_dispose (GObject *object)
{
  GsmDisksView *self = GSM_DISKS_VIEW (object);

  clear_timeout (self);
  g_clear_pointer (&self->known_directories, g_hash_table_unref);
  g_clear_object (&self->settings);
  g_clear_object (&self->monitor);

  G_OBJECT_CLASS (gsm_disks_view_parent_class)->dispose (object);
}


static void
gsm_disks_view_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GsmDisksView *self = GSM_DISKS_VIEW (object);

  switch (prop_id) {
    case PROP_UPDATE_INTERVAL:
      g_value_set_int (value, self->update_interval);
      break;
    case PROP_SHOW_ALL_FS:
      g_value_set_boolean (value, self->show_all_fs);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
fsusage_stats (const glibtop_fsusage *buf,
               uint64_t              *bused,
               uint64_t              *bfree,
               uint64_t              *bavail,
               uint64_t              *btotal,
               int                   *percentage)
{
  uint64_t total = buf->blocks * buf->block_size;

  if (!total) {
    /* not a real device */
    *btotal = *bfree = *bavail = *bused = 0ULL;
    *percentage = 0;
  } else {
    int percent;
    *btotal = total;
    *bfree = buf->bfree * buf->block_size;
    *bavail = buf->bavail * buf->block_size;
    *bused = *btotal - *bfree;
    /* percent = 100.0f * *bused / *btotal; */
    percent = 100 * *bused / (*bused + *bavail);
    *percentage = CLAMP (percent, 0, 100);
  }
}


static inline GHashTable *
get_current_directories (GsmDisksView *self)
{
  g_autoptr (GHashTable) current_directories =
    g_hash_table_new_full (g_file_hash,
                           (GEqualFunc) g_file_equal,
                           g_object_unref,
                           NULL);
  GHashTableIter iter;
  GFile *directory;

  /* Produce a hashset of the current keys */
  g_hash_table_iter_init (&iter, self->known_directories);
  while (g_hash_table_iter_next (&iter, (gpointer *) &directory, NULL)) {
    g_hash_table_add (current_directories, g_object_ref (directory));
  }

  return g_steal_pointer (&current_directories);
}


static inline void
drop_leftover_directories (GsmDisksView *self,
                           GHashTable   *leftover_directories)
{
  GHashTableIter iter;
  GFile *directory;

  g_hash_table_iter_init (&iter, leftover_directories);
  while (g_hash_table_iter_next (&iter, (gpointer *) &directory, NULL)) {
    g_autoptr (GFile) stolen_key = NULL;
    g_autoptr (GsmDisk) disk = NULL;
    guint position;

    if (!g_hash_table_steal_extended (self->known_directories,
                                      directory,
                                      (gpointer *) &stolen_key,
                                      (gpointer *) &disk)) {
      continue;
    }

    if (g_list_store_find (self->list_store, disk, &position)) {
      g_list_store_remove (self->list_store, position);
    }
  }
}


static inline void
handle_entry (GsmDisksView             *self,
              GPtrArray                *new_disks,
              GHashTable               *leftover_directories,
              const glibtop_mountentry *entry)
{
  glibtop_fsusage usage;
  uint64_t bused, bfree, bavail, btotal;
  int percentage;
  g_autoptr (GFile) device = g_file_new_for_path (entry->devname);
  g_autoptr (GFile) directory = g_file_new_for_path (entry->mountdir);
  GsmDisk *existing_disk = NULL;

  glibtop_get_fsusage (&usage, entry->mountdir);

  /* If this was already a known directory, save it from being cleaned up */
  g_hash_table_remove (leftover_directories, directory);

  if (!self->show_all_fs && usage.blocks == 0) {
    return;
  }

  fsusage_stats (&usage, &bused, &bfree, &bavail, &btotal, &percentage);

  /* if we can find a row with the same mountpoint, we get it but we
   * still need to update all the fields.
   * This makes selection persistent.
   */
  existing_disk = g_hash_table_lookup (self->known_directories, directory);
  if (existing_disk) {
    gsm_disk_set_device (existing_disk, device);
    g_object_set (existing_disk,
                  "type", entry->type,
                  "total", btotal,
                  "free", bfree,
                  "available", bavail,
                  "used", bused,
                  "percentage", percentage,
                  NULL);
  } else {
    g_autoptr (GsmDisk) disk = gsm_disk_new (device,
                                             directory,
                                             entry->type,
                                             btotal,
                                             bfree,
                                             bavail,
                                             bused,
                                             percentage);

    g_hash_table_insert (self->known_directories,
                         g_steal_pointer (&directory),
                         g_object_ref (disk));
    g_ptr_array_add (new_disks, g_steal_pointer (&disk));
  }
}


static void
disks_update (GsmDisksView *self)
{
  g_autoptr (GHashTable) leftover_directories =
    get_current_directories (self);
  g_autofree glibtop_mountentry *entries;
  g_autoptr (GPtrArray) new_disks =
    g_ptr_array_new_null_terminated (10, g_object_unref, TRUE);
  glibtop_mountlist mountlist;

  entries = glibtop_get_mountlist (&mountlist, self->show_all_fs);

  for (guint i = 0; i < mountlist.number; i++) {
    handle_entry (self, new_disks, leftover_directories, &entries[i]);
  }

  g_list_store_splice (self->list_store,
                       0,
                       0,
                       new_disks->pdata,
                       new_disks->len);

  drop_leftover_directories (self, leftover_directories);
}


static gboolean
disks_timeout (gpointer data)
{
  disks_update (GSM_DISKS_VIEW (data));

  return G_SOURCE_CONTINUE;
}


static inline void
setup_timeout (GsmDisksView *self, gboolean reset)
{
  if (self->timeout && !reset) {
    return;
  }

  clear_timeout (self);

  g_debug ("disks: start timeout");
  g_clear_handle_id (&self->timeout, g_source_remove);
  self->timeout = g_timeout_add (self->update_interval, disks_timeout, self);
  g_source_set_name_by_id (self->timeout, "refresh disks");
  disks_update (self);
}


static void
gsm_disks_view_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GsmDisksView *self = GSM_DISKS_VIEW (object);

  switch (prop_id) {
    case PROP_UPDATE_INTERVAL:
      {
        int new_value = g_value_get_int (value);

        if (new_value == self->update_interval) {
          return;
        }

        self->update_interval = new_value;
        setup_timeout (self, TRUE);
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    case PROP_SHOW_ALL_FS:
      {
        gboolean new_value = g_value_get_boolean (value);

        if (new_value == self->show_all_fs) {
          return;
        }

        self->show_all_fs = new_value;
        disks_update (self);
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
gsm_disks_view_map (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (gsm_disks_view_parent_class)->map (widget);

  setup_timeout (GSM_DISKS_VIEW (widget), FALSE);
}


static void
gsm_disks_view_unmap (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (gsm_disks_view_parent_class)->unmap (widget);

  clear_timeout (GSM_DISKS_VIEW (widget));
}


static void
mount_changed (G_GNUC_UNUSED GVolumeMonitor *monitor,
               G_GNUC_UNUSED GMount         *mount,
               gpointer                      data)
{
  disks_update (GSM_DISKS_VIEW (data));
}


static char *
format_size (G_GNUC_UNUSED GObject *object,
             uint64_t               size)
{
  return g_format_size (size);
}


static char *
format_percentage (G_GNUC_UNUSED GObject *object,
                   int                    percentage)
{
  return g_strdup_printf ("%i%%", percentage);
}


static void
did_open (GObject      *source,
          GAsyncResult *result,
          gpointer      user_data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GsmDisksView) self = GSM_DISKS_VIEW (user_data);

  gsm_disk_open_root_finish (GSM_DISK (source), result, &error);

  if (error) {
    AdwDialog *dialog = adw_alert_dialog_new (_("Open Failed"), error->message);

    adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog),
                                   "close",
                                   _("_Close"));

    adw_dialog_present (dialog, GTK_WIDGET (self));
  }
}


static void
activate_disk (GtkColumnView *view, guint position, gpointer user_data)
{
  GsmDisksView *self = GSM_DISKS_VIEW (user_data);
  GListModel *selection = G_LIST_MODEL (self->selection);
  g_autoptr (GsmDisk) disk =
    GSM_DISK (g_list_model_get_object (selection, position));

  gsm_disk_open_root (disk,
                      GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (view))),
                      NULL,
                      did_open,
                      g_object_ref (self));
}


static void
gsm_disks_view_class_init (GsmDisksViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gsm_disks_view_dispose;
  object_class->get_property = gsm_disks_view_get_property;
  object_class->set_property = gsm_disks_view_set_property;

  widget_class->map = gsm_disks_view_map;
  widget_class->unmap = gsm_disks_view_unmap;

  properties[PROP_UPDATE_INTERVAL] =
    g_param_spec_int ("update-interval", NULL, NULL,
                      0, G_MAXINT32, 0,
                      G_PARAM_READWRITE);
  properties[PROP_SHOW_ALL_FS] =
    g_param_spec_boolean ("show-all-fs", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gnome-system-monitor/data/disks.ui");

  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, column_view);
  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, selection);
  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, list_store);
  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, persister);
  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, root_signals);
  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, monitor_signals);

  gtk_widget_class_bind_template_callback (widget_class, format_size);
  gtk_widget_class_bind_template_callback (widget_class, format_percentage);
  gtk_widget_class_bind_template_callback (widget_class, activate_disk);

  g_type_ensure (GSM_TYPE_COLUMN_VIEW_PERSISTER);
}


static void
suspended_notified (GtkWindow                *window,
                    G_GNUC_UNUSED GParamSpec *pspec,
                    gpointer                  user_data)
{
  GsmDisksView *self = GSM_DISKS_VIEW (user_data);

  if (gtk_window_is_suspended (window)) {
    clear_timeout (self);
  } else if (gtk_widget_get_mapped (GTK_WIDGET (window))) {
    setup_timeout (self, FALSE);
  }
}


static void
gsm_disks_view_init (GsmDisksView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->known_directories = g_hash_table_new_full (g_file_hash,
                                                   (GEqualFunc) g_file_equal,
                                                   g_object_unref,
                                                   g_object_unref);

  self->settings = g_settings_new (GSM_GSETTINGS_SCHEMA);

  self->monitor = g_volume_monitor_get ();

  g_signal_group_connect (self->monitor_signals,
                          "mount-added", G_CALLBACK (mount_changed),
                          self);
  g_signal_group_connect (self->monitor_signals,
                          "mount-changed", G_CALLBACK (mount_changed),
                          self);
  g_signal_group_connect (self->monitor_signals,
                          "mount-removed", G_CALLBACK (mount_changed),
                          self);
  g_signal_group_set_target (self->monitor_signals, self->monitor);

  g_settings_bind (self->settings, GSM_SETTING_DISKS_UPDATE_INTERVAL,
                   G_OBJECT (self), "update-interval", G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (self->settings, GSM_SETTING_SHOW_ALL_FS,
                   G_OBJECT (self), GSM_SETTING_SHOW_ALL_FS, G_SETTINGS_BIND_DEFAULT);

  g_signal_group_connect (self->root_signals,
                          "notify::suspended",
                          G_CALLBACK (suspended_notified),
                          self);
}


GListModel *
gsm_disks_view_get_columns (GsmDisksView *self)
{
  g_return_val_if_fail (GSM_IS_DISKS_VIEW (self), NULL);

  return gtk_column_view_get_columns (self->column_view);
}
