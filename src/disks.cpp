#include <config.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <glibtop/mountlist.h>
#include <glibtop/fsusage.h>
#include <glib/gi18n.h>

#include "disks.h"
#include "disks-data.h"
#include "util.h"
#include "settings-keys.h"

struct _GsmDisksView
{
  GtkWidget parent_instance;

  GtkColumnView *column_view;
  GtkSingleSelection *selection;
  GListStore *list_store;

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
cb_sort_changed (GtkSorter *,
                 GtkSorterChange,
                 gpointer data)
{
  GtkColumnView *column_view = static_cast<GtkColumnView *>(data);

  save_sort_state (column_view, GSM_SETTINGS_CHILD_DISKS);
}

static void
fsusage_stats (const glibtop_fsusage *buf,
               guint64               *bused,
               guint64               *bfree,
               guint64               *bavail,
               guint64               *btotal,
               gint                  *percentage)
{
  guint64 total = buf->blocks * buf->block_size;

  if (!total)
    {
      /* not a real device */
      *btotal = *bfree = *bavail = *bused = 0ULL;
      *percentage = 0;
    }
  else
    {
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

static const char*
get_icon_for_path (const char*path)
{
  GVolumeMonitor *monitor;
  GList *mounts;
  uint i;
  GMount *mount;
  GIcon *icon;
  const char*name = "";

  monitor = g_volume_monitor_get ();
  mounts = g_volume_monitor_get_mounts (monitor);

  for (i = 0; i < g_list_length (mounts); i++)
    {
      mount = G_MOUNT (g_list_nth_data (mounts, i));
      if (strcmp (g_mount_get_name (mount), path))
        continue;

      icon = g_mount_get_icon (mount);

      if (!icon)
        continue;
      name = g_icon_to_string (icon);
      g_object_unref (icon);
    }

  g_list_free_full (mounts, g_object_unref);
  return name;
}

static GdkPaintable *
get_icon_for_device (const char *mountpoint)
{
  GdkPaintable *icon;
  GtkIconTheme *icon_theme;
  const char *icon_name = get_icon_for_path (mountpoint);

  // FIXME: defaults to a safe value
  if (!strcmp (icon_name, ""))
    icon_name = "drive-harddisk";     // get_icon_for_path("/");

  icon_theme = gtk_icon_theme_get_for_display (gdk_display_get_default ());
  icon = GDK_PAINTABLE (gtk_icon_theme_lookup_icon (icon_theme, icon_name, NULL, 32, 1, GTK_TEXT_DIR_NONE, GTK_ICON_LOOKUP_PRELOAD));

  return icon;
}


static gboolean
find_disk_in_model (GListModel *model,
                    const char *mountpoint,
                    guint      *position)
{
  guint pos = *position;
  gboolean found = FALSE;

  while (!found && pos < g_list_model_get_n_items (model))
    {
      DisksData *data;
      gchar *dir;

      data = DISKS_DATA (g_list_model_get_object (model, pos));
      g_object_get (data, "directory", &dir, NULL);

      if (dir && !strcmp (dir, mountpoint))
          found = TRUE;
      else
          pos++;

      g_free (dir);
    }

  *position = pos;

  return found;
}

static void
remove_old_disks (GListModel               *model,
                  const glibtop_mountentry *entries,
                  guint                     n)
{
  guint position = 0;

  while (position < g_list_model_get_n_items (model))
    {
      DisksData *data;
      char *dir;
      guint i;
      gboolean found = FALSE;

      data = DISKS_DATA (g_list_model_get_object (model, position));
      g_object_get (data, "directory", &dir, NULL);

      for (i = 0; i != n; ++i)
        if (!strcmp (dir, entries[i].mountdir))
          {
            found = TRUE;
            break;
          }

      g_free (dir);

      if (!found)
        {
          if (g_list_store_find (G_LIST_STORE (model), G_OBJECT (data), &position))
            g_list_store_remove (G_LIST_STORE (model), position);
          else
            break;
        }

      position++;
    }
}



static void
add_disk (GListModel               *model,
          const glibtop_mountentry *entry,
          bool                      show_all_fs)
{
  GdkPaintable *icon;
  glibtop_fsusage usage;
  guint64 bused, bfree, bavail, btotal;
  gint percentage;
  guint position = 0;

  glibtop_get_fsusage (&usage, entry->mountdir);

  if (not show_all_fs and usage.blocks == 0)
    {
      if (find_disk_in_model (model, entry->mountdir, &position))
        g_list_store_remove (G_LIST_STORE (model), position);
      return;
    }

  fsusage_stats (&usage, &bused, &bfree, &bavail, &btotal, &percentage);
  icon = get_icon_for_device (entry->mountdir);

  DisksData *data;

  /* if we can find a row with the same mountpoint, we get it but we
     still need to update all the fields.
     This makes selection persistent.
  */
  if (!find_disk_in_model (model, entry->mountdir, &position))
    {
      data = disks_data_new (icon,
                             entry->devname,
                             entry->mountdir,
                             entry->type,
                             btotal,
                             bfree,
                             bavail,
                             bused,
                             percentage);

      g_list_store_insert (G_LIST_STORE (model), position, data);
    }
  else
    {
      data = DISKS_DATA (g_list_model_get_object (model, position));

      g_object_set (data,
                    "paintable", icon,
                    "device", entry->devname,
                    "directory", entry->mountdir,
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

  for (i = 0; i < mountlist.number; i++)
    add_disk (G_LIST_MODEL (self->list_store), &entries[i], self->show_all_fs);

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
cb_disk_columns_changed (GListModel *,
                         guint,
                         guint,
                         guint,
                         gpointer data)
{
  GtkColumnView *column_view = static_cast<GtkColumnView *>(data);

  save_columns_state (column_view, GSM_SETTINGS_CHILD_DISKS);
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

static void
open_dir (GtkColumnView *,
          guint position,
          gpointer data)
{
  GListModel *selection = static_cast<GListModel *>(data);
  DisksData *disksdata;
  gchar *dir, *url;

  disksdata = DISKS_DATA (g_list_model_get_object (selection, position));
  g_object_get (disksdata, "directory", &dir, NULL);

  url = g_filename_to_uri (dir, NULL, NULL);

  GError *error = 0;

  if (!g_app_info_launch_default_for_uri (url, NULL, &error))
    {
      g_warning ("Cannot open '%s' : %s\n", url, error->message);
      g_error_free (error);
    }

  g_free (url);
  g_free (dir);
}

static char *
format_size (gpointer,
             guint64 size)
{
  return g_format_size (size);
}

static char*
format_percentage (gpointer,
                   gint percentage)
{
  return g_strdup_printf ("%i%%", percentage);
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

  gtk_widget_class_bind_template_callback (widget_class, format_size);
  gtk_widget_class_bind_template_callback (widget_class, format_percentage);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
gsm_disks_view_init (GsmDisksView *self)
{
  GSettings *settings;
  GListModel *columns;
  GtkSorter *sorter;

  gtk_widget_init_template (GTK_WIDGET (self));

  settings = g_settings_new (GSM_GSETTINGS_SCHEMA);
  columns = gtk_column_view_get_columns (self->column_view);
  sorter = gtk_column_view_get_sorter (self->column_view);

  init_volume_monitor (self);

  load_state (GTK_COLUMN_VIEW (self->column_view), GSM_SETTINGS_CHILD_DISKS);

  g_settings_bind (settings, GSM_SETTING_DISKS_UPDATE_INTERVAL,
                   G_OBJECT (self), "update-interval", G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, GSM_SETTING_SHOW_ALL_FS,
                   G_OBJECT (self), GSM_SETTING_SHOW_ALL_FS, G_SETTINGS_BIND_DEFAULT);

  g_signal_connect (G_OBJECT (settings), "changed::%s" GSM_SETTING_DISKS_UPDATE_INTERVAL,
                    G_CALLBACK (cb_timeout_changed), self);

  g_signal_connect (G_OBJECT (settings), "changed::%s" GSM_SETTING_SHOW_ALL_FS,
                    G_CALLBACK (cb_show_all_fs), self);

  g_object_unref (settings);

  g_signal_connect (G_OBJECT (self->column_view), "activate",
                    G_CALLBACK (open_dir), self->selection);

  g_signal_connect (G_OBJECT (columns), "items-changed",
                    G_CALLBACK (cb_disk_columns_changed), self->column_view);

  g_signal_connect (G_OBJECT (sorter), "changed",
                    G_CALLBACK (cb_sort_changed), self->column_view);
}
