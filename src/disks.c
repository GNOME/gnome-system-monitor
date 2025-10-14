/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <glibtop/mountlist.h>
#include <glibtop/fsusage.h>
#include <adwaita.h>

#include "column-view-persister.h"
#include "disk.h"
#include "settings-keys.h"

#include "disks.h"


struct _GsmDisksView {
  GtkWidget parent_instance;

  GtkColumnView *column_view;
  GtkSingleSelection *selection;
  GListStore *list_store;
  /* We don't actually use this from C,
   * but if we don't bind it it'll be disposed */
  GsmColumnViewPersister *persister;

  int update_interval;
  gboolean show_all_fs;
  guint timeout;
};

G_DEFINE_TYPE (GsmDisksView, gsm_disks_view, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_UPDATE_INTERVAL,
  PROP_SHOW_ALL_FS,
  PROP_TIMEOUT,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

GsmDisksView *
gsm_disks_view_new (void)
{
  return GSM_DISKS_VIEW (g_object_new (GSM_TYPE_DISKS_VIEW, NULL));
}

static void
gsm_disks_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (gsm_disks_view_parent_class)->finalize (object);
}

static void
gsm_disks_view_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GsmDisksView *self = GSM_DISKS_VIEW (object);

  switch (prop_id)
    {
    case PROP_UPDATE_INTERVAL:
      g_value_set_int (value, self->update_interval);
      break;
    case PROP_SHOW_ALL_FS:
      g_value_set_boolean (value, self->show_all_fs);
      break;
    case PROP_TIMEOUT:
      g_value_set_uint (value, self->timeout);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gsm_disks_view_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GsmDisksView *self = GSM_DISKS_VIEW (object);

  switch (prop_id)
    {
    case PROP_UPDATE_INTERVAL:
      self->update_interval = g_value_get_int (value);
      break;
    case PROP_SHOW_ALL_FS:
      self->show_all_fs = g_value_get_boolean (value);
      break;
    case PROP_TIMEOUT:
      self->timeout = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

guint
gsm_disks_view_get_timeout (GsmDisksView *self)
{
  return self->timeout;
}

GtkColumnView *
gsm_disks_view_get_column_view (GsmDisksView *self)
{
  return self->column_view;
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


static gboolean
find_disk_in_model (GsmDisksView *self,
                    GFile        *directory,
                    guint        *position)
{
  for (guint pos = 0;
       pos < g_list_model_get_n_items (G_LIST_MODEL (self->list_store));
       pos++) {
    g_autoptr (GsmDisk) data =
      GSM_DISK (g_list_model_get_object (G_LIST_MODEL (self->list_store), pos));

    if (gsm_disk_is_for_directory (data, directory)) {
      *position = pos;

      return TRUE;
    }
  }

  return FALSE;
}


static void
remove_old_disks (GListModel               *model,
                  const glibtop_mountentry *entries,
                  guint                     n)
{
  guint position = 0;

  while (position < g_list_model_get_n_items (model)) {
    g_autoptr (GsmDisk) data =
      GSM_DISK (g_list_model_get_object (model, position));
    gboolean found = FALSE;

    for (guint i = 0; i != n; ++i) {
      g_autoptr (GFile) directory = g_file_new_for_path (entries[i].mountdir);

      if (gsm_disk_is_for_directory (data, directory)) {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      if (g_list_store_find (G_LIST_STORE (model), G_OBJECT (data), &position)) {
        g_list_store_remove (G_LIST_STORE (model), position);
      } else {
        break;
      }
    }

    position++;
  }
}


static inline void
add_disk (GsmDisksView             *self,
          const glibtop_mountentry *entry)
{
  glibtop_fsusage usage;
  uint64_t bused, bfree, bavail, btotal;
  int percentage;
  guint position = 0;
  g_autoptr (GsmDisk) data = NULL;
  g_autoptr (GFile) device = g_file_new_for_path (entry->devname);
  g_autoptr (GFile) directory = g_file_new_for_path (entry->mountdir);

  glibtop_get_fsusage (&usage, entry->mountdir);

  if (!self->show_all_fs && usage.blocks == 0) {
    if (find_disk_in_model (self, directory, &position)) {
      g_list_store_remove (self->list_store, position);
    }

    return;
  }

  fsusage_stats (&usage, &bused, &bfree, &bavail, &btotal, &percentage);

  /* if we can find a row with the same mountpoint, we get it but we
   * still need to update all the fields.
   * This makes selection persistent.
   */
  if (!find_disk_in_model (self, directory, &position)) {
    data = gsm_disk_new (device,
                         directory,
                         entry->type,
                         btotal,
                         bfree,
                         bavail,
                         bused,
                         percentage);
    g_list_store_insert (self->list_store, position, data);
  } else {
    data = GSM_DISK (g_list_model_get_object (G_LIST_MODEL (self->list_store), position));

    gsm_disk_set_device (data, device);
    gsm_disk_set_directory (data, directory);
    g_object_set (data,
                  "type", entry->type,
                  "total", btotal,
                  "free", bfree,
                  "available", bavail,
                  "used", bused,
                  "percentage", percentage,
                  NULL);
  }
}

static void
mount_changed (GVolumeMonitor*,
               GMount*,
               gpointer data)
{
  GsmDisksView *self = (GsmDisksView *) data;

  disks_update (self);
}

static gboolean
cb_timeout (gpointer data)
{
  GsmDisksView *self = (GsmDisksView *) data;

  disks_update (self);

  return G_SOURCE_CONTINUE;
}

void
disks_update (GsmDisksView *self)
{
  glibtop_mountentry *entries;
  glibtop_mountlist mountlist;
  guint i;

  entries = glibtop_get_mountlist (&mountlist, self->show_all_fs);

  remove_old_disks (G_LIST_MODEL (self->list_store), entries, mountlist.number);

  for (i = 0; i < mountlist.number; i++) {
    add_disk (self, &entries[i]);
  }

  g_free (entries);
}

static void
init_volume_monitor (GsmDisksView *self)
{
  GVolumeMonitor *monitor = g_volume_monitor_get ();

  g_signal_connect (monitor, "mount-added", G_CALLBACK (mount_changed), self);
  g_signal_connect (monitor, "mount-changed", G_CALLBACK (mount_changed), self);
  g_signal_connect (monitor, "mount-removed", G_CALLBACK (mount_changed), self);
}

void
disks_freeze (GsmDisksView *self)
{
  if (self->timeout)
    {
      g_source_remove (self->timeout);
      self->timeout = 0;
    }
}

void
disks_thaw (GsmDisksView *self)
{
  if (self->timeout)
    return;

  self->timeout = g_timeout_add (self->update_interval,
                                 cb_timeout,
                                 self);
}

void
disks_reset_timeout (GsmDisksView *self)
{
  disks_freeze (self);
  disks_thaw (self);
}


static void
cb_show_all_fs (GSettings *,
                gchar     *,
                gpointer   data)
{
  GsmDisksView *self = (GsmDisksView *) data;

  disks_update (self);
  disks_reset_timeout (self);
}

static void
cb_timeout_changed (GSettings *,
                    gchar     *,
                    gpointer   data)
{
  GsmDisksView *self = (GsmDisksView *) data;

  disks_reset_timeout (self);
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

  object_class->finalize = gsm_disks_view_finalize;
  object_class->get_property = gsm_disks_view_get_property;
  object_class->set_property = gsm_disks_view_set_property;

  properties [PROP_UPDATE_INTERVAL] =
    g_param_spec_int ("update-interval",
                      "Update interval",
                      "Update interval",
                      0, G_MAXINT32, 0,
                      G_PARAM_READWRITE);

  properties [PROP_SHOW_ALL_FS] =
    g_param_spec_boolean ("show-all-fs",
                          "Show all fs",
                          "Show all fs",
                           FALSE,
                           G_PARAM_READWRITE);

  properties [PROP_TIMEOUT] =
    g_param_spec_uint ("timeout",
                       "Timeout",
                       "Timeout",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gnome-system-monitor/data/disks.ui");

  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, column_view);
  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, selection);
  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, list_store);
  gtk_widget_class_bind_template_child (widget_class, GsmDisksView, persister);

  gtk_widget_class_bind_template_callback (widget_class, format_size);
  gtk_widget_class_bind_template_callback (widget_class, format_percentage);
  gtk_widget_class_bind_template_callback (widget_class, activate_disk);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  g_type_ensure (GSM_TYPE_COLUMN_VIEW_PERSISTER);
}


static void
gsm_disks_view_init (GsmDisksView *self)
{
  GSettings *settings;

  gtk_widget_init_template (GTK_WIDGET (self));

  settings = g_settings_new (GSM_GSETTINGS_SCHEMA);

  init_volume_monitor (self);

  g_settings_bind (settings, GSM_SETTING_DISKS_UPDATE_INTERVAL,
                   G_OBJECT (self), "update-interval", G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, GSM_SETTING_SHOW_ALL_FS,
                   G_OBJECT (self), GSM_SETTING_SHOW_ALL_FS, G_SETTINGS_BIND_DEFAULT);

  g_signal_connect (G_OBJECT (settings), "changed::%s" GSM_SETTING_DISKS_UPDATE_INTERVAL,
                    G_CALLBACK (cb_timeout_changed), self);

  g_signal_connect (G_OBJECT (settings), "changed::%s" GSM_SETTING_SHOW_ALL_FS,
                    G_CALLBACK (cb_show_all_fs), self);

  g_object_unref (settings);
}
