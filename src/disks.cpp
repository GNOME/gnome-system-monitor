#include <config.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <glibtop/mountlist.h>
#include <glibtop/fsusage.h>
#include <glib/gi18n.h>

#include "disks.h"
#include "disks-data.h"
#include "application.h"
#include "util.h"
#include "settings-keys.h"

static void
cb_sort_changed (GListModel *,
                 gpointer data)
{
  GsmApplication *app = (GsmApplication *) data;

  // gsm_tree_view_save_state (GSM_TREE_VIEW (app->disk_list));
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

static GdkTexture *
get_icon_for_device (const char *mountpoint)
{
  GdkTexture *icon;
  GtkIconPaintable *icon_paintable;
  GtkIconTheme *icon_theme;
  const char *icon_name = get_icon_for_path (mountpoint);

  // FIXME: defaults to a safe value
  if (!strcmp (icon_name, ""))
    icon_name = "drive-harddisk";     // get_icon_for_path("/");

  icon_theme = gtk_icon_theme_get_for_display (gdk_display_get_default ());
  icon_paintable = gtk_icon_theme_lookup_icon (icon_theme, icon_name, NULL, 32, 1, GTK_TEXT_DIR_NONE, GTK_ICON_LOOKUP_PRELOAD);
  icon = gdk_texture_new_for_pixbuf (gdk_pixbuf_new_from_file_at_size (g_file_get_path (gtk_icon_paintable_get_file (icon_paintable)), 32, 32, NULL));

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
  GdkTexture *icon;
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
      data = disks_data_new (GDK_PAINTABLE (icon),
                             entry->devname,
                             entry->mountdir,
                             entry->type,
                             g_format_size (btotal),
                             g_format_size (bfree),
                             g_format_size (bavail),
                             g_format_size (bused),
                             percentage);

      g_list_store_insert (G_LIST_STORE (model), position, data);
    }
  else
    {
      data = DISKS_DATA (g_list_model_get_object (model, position));

      g_object_set (data,
                    "paintable", GDK_PAINTABLE (icon),
                    "device", entry->devname,
                    "directory", entry->mountdir,
                    "type", entry->type,
                    "total", g_format_size (btotal),
                    "free", g_format_size (bfree),
                    "available", g_format_size (bavail),
                    "used", g_format_size (bused),
                    "percentage", percentage,
                    NULL);
    }
}

static void
mount_changed (GVolumeMonitor*,
               GMount*,
               GsmApplication *app)
{
  disks_update (app);
}

static gboolean
cb_timeout (gpointer data)
{
  GsmApplication *app = (GsmApplication *) data;

  disks_update (app);

  return G_SOURCE_CONTINUE;
}

void
disks_update (GsmApplication *app)
{
  GListModel *model;
  glibtop_mountentry *entries;
  glibtop_mountlist mountlist;
  guint i;
  gboolean show_all_fs;

  model = gtk_sort_list_model_get_model (GTK_SORT_LIST_MODEL (
                                           gtk_single_selection_get_model (GTK_SINGLE_SELECTION (
                                                                             gtk_column_view_get_model (app->disk_list)))));
  show_all_fs = app->settings->get_boolean (GSM_SETTING_SHOW_ALL_FS);
  entries = glibtop_get_mountlist (&mountlist, show_all_fs);

  remove_old_disks (model, entries, mountlist.number);

  for (i = 0; i < mountlist.number; i++)
    add_disk (model, &entries[i], show_all_fs);

  g_free (entries);
}

static void
init_volume_monitor (GsmApplication *app)
{
  GVolumeMonitor *monitor = g_volume_monitor_get ();

  g_signal_connect (monitor, "mount-added", G_CALLBACK (mount_changed), app);
  g_signal_connect (monitor, "mount-changed", G_CALLBACK (mount_changed), app);
  g_signal_connect (monitor, "mount-removed", G_CALLBACK (mount_changed), app);
}

void
disks_freeze (GsmApplication *app)
{
  if (app->disk_timeout)
    {
      g_source_remove (app->disk_timeout);
      app->disk_timeout = 0;
    }
}

void
disks_thaw (GsmApplication *app)
{
  if (app->disk_timeout)
    return;

  app->disk_timeout = g_timeout_add (app->config.disks_update_interval,
                                     cb_timeout,
                                     app);
}

void
disks_reset_timeout (GsmApplication *app)
{
  disks_freeze (app);
  disks_thaw (app);
}

static void
cb_disk_columns_changed (GListModel *,
                         gpointer)
{
  // gsm_tree_view_save_state (GSM_TREE_VIEW (treeview));
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

  url = g_strdup_printf ("file://%s", dir);

  GError *error = 0;

  if (!g_app_info_launch_default_for_uri (url, NULL, &error))
    {
      g_warning ("Cannot open '%s' : %s\n", url, error->message);
      g_error_free (error);
    }

  g_free (url);
  g_free (dir);
}

static void
cb_disk_list_destroying (GtkWidget *self,
                         gpointer   data)
{
  GListModel *model;

  model = gtk_sort_list_model_get_model (GTK_SORT_LIST_MODEL (
                                           gtk_single_selection_get_model (GTK_SINGLE_SELECTION (
                                                                             gtk_column_view_get_model (GTK_COLUMN_VIEW (self))))));

  g_signal_handlers_disconnect_by_func (model, (gpointer) cb_disk_columns_changed, data);

  if (model != NULL)
    g_signal_handlers_disconnect_by_func (model,
                                          (gpointer) cb_sort_changed,
                                          data);
}


void
create_disk_view (GsmApplication *app,
                  GtkBuilder     *builder)
{
  GtkColumnView *columnview;
  GListModel *model;
  GListModel *selection;
  GListModel *columns;

  init_volume_monitor (app);

  columnview = GTK_COLUMN_VIEW (gtk_builder_get_object (builder, "disks_view"));
  model = G_LIST_MODEL (gtk_builder_get_object (builder, "disks_store"));
  selection = G_LIST_MODEL (gtk_builder_get_object (builder, "selection"));
  columns = gtk_column_view_get_columns (columnview);

  g_signal_connect (G_OBJECT (columnview), "activate", G_CALLBACK (open_dir), selection);
  app->disk_list = columnview;

  g_signal_connect (G_OBJECT (columnview), "destroy",
                    G_CALLBACK (cb_disk_list_destroying),
                    app);

  g_signal_connect (G_OBJECT (model), "items-changed",
                    G_CALLBACK (cb_disk_columns_changed), app);

  g_signal_connect (G_OBJECT (columns), "items-changed",
                    G_CALLBACK (cb_sort_changed), app);

  app->settings->signal_changed (GSM_SETTING_SHOW_ALL_FS).connect ([app](const Glib::ustring&) {
    disks_update (app);
    disks_reset_timeout (app);
  });

  gtk_widget_set_visible (GTK_WIDGET (columnview), TRUE);
}
