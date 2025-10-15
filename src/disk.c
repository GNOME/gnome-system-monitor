/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>

#include <gio/gunixmounts.h>

#include "disk.h"


struct _GsmDisk {
  GObject parent_instance;

  GIcon *icon;
  GFile *device;
  GFile *directory;
  char *type;
  uint64_t total;
  uint64_t free;
  uint64_t available;
  uint64_t used;
  int percentage;
  char *display_device;
  char *display_directory;
};


G_DEFINE_FINAL_TYPE (GsmDisk, gsm_disk, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_ICON,
  PROP_DEVICE,
  PROP_DIRECTORY,
  PROP_TYPE,
  PROP_TOTAL,
  PROP_FREE,
  PROP_AVAILABLE,
  PROP_USED,
  PROP_PERCENTAGE,
  PROP_DISPLAY_DEVICE,
  PROP_DISPLAY_DIRECTORY,
  N_PROPS
};
static GParamSpec *properties[N_PROPS];


static void
gsm_disk_dispose (GObject *object)
{
  GsmDisk *self = GSM_DISK (object);

  g_clear_object (&self->icon);
  g_clear_object (&self->device);
  g_clear_object (&self->directory);
  g_clear_pointer (&self->type, g_free);
  g_clear_pointer (&self->display_device, g_free);
  g_clear_pointer (&self->display_directory, g_free);

  G_OBJECT_CLASS (gsm_disk_parent_class)->dispose (object);
}


static void
gsm_disk_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  GsmDisk *self = GSM_DISK (object);

  switch (property_id) {
    case PROP_ICON:
      g_value_set_object (value, self->icon);
      break;
    case PROP_DEVICE:
      g_value_set_object (value, self->device);
      break;
    case PROP_DIRECTORY:
      g_value_set_object (value, self->directory);
      break;
    case PROP_TYPE:
      g_value_set_string (value, self->type);
      break;
    case PROP_TOTAL:
      g_value_set_uint64 (value, self->total);
      break;
    case PROP_FREE:
      g_value_set_uint64 (value, self->free);
      break;
    case PROP_AVAILABLE:
      g_value_set_uint64 (value, self->available);
      break;
    case PROP_USED:
      g_value_set_uint64 (value, self->used);
      break;
    case PROP_PERCENTAGE:
      g_value_set_int (value, self->percentage);
      break;
    case PROP_DISPLAY_DEVICE:
      g_value_set_string (value, self->display_device);
      break;
    case PROP_DISPLAY_DIRECTORY:
      g_value_set_string (value, self->display_directory);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static inline void
set_uint64_prop (GObject      *object,
                 GParamSpec   *pspec,
                 uint64_t     *field,
                 const GValue *value)
{
  uint64_t new_value = g_value_get_uint64 (value);

  if (*field == new_value) {
    return;
  }

  *field = new_value;
  g_object_notify_by_pspec (object, pspec);
}


static void
gsm_disk_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  GsmDisk *self = GSM_DISK (object);

  switch (property_id) {
    case PROP_DEVICE:
      gsm_disk_set_device (self, g_value_get_object (value));
      break;
    case PROP_DIRECTORY:
      gsm_disk_set_directory (self, g_value_get_object (value));
      break;
    case PROP_TYPE:
      if (g_set_str (&self->type, g_value_get_string (value))) {
        g_object_notify_by_pspec (object, pspec);
      }
      break;
    case PROP_TOTAL:
      set_uint64_prop (object, pspec, &self->total, value);
      break;
    case PROP_FREE:
      set_uint64_prop (object, pspec, &self->free, value);
      break;
    case PROP_AVAILABLE:
      set_uint64_prop (object, pspec, &self->available, value);
      break;
    case PROP_USED:
      set_uint64_prop (object, pspec, &self->used, value);
      break;
    case PROP_PERCENTAGE:
      self->percentage = g_value_get_int (value);
      g_object_notify_by_pspec (object, pspec);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
gsm_disk_class_init (GsmDiskClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gsm_disk_dispose;
  object_class->get_property = gsm_disk_get_property;
  object_class->set_property = gsm_disk_set_property;

  properties[PROP_ICON] =
    g_param_spec_object ("icon", NULL, NULL,
                         G_TYPE_ICON,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_DEVICE] =
    g_param_spec_object ("device", NULL, NULL,
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_DIRECTORY] =
    g_param_spec_object ("directory", NULL, NULL,
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_TYPE] =
    g_param_spec_string ("type", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_TOTAL] =
    g_param_spec_uint64 ("total", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_FREE] =
    g_param_spec_uint64 ("free", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_AVAILABLE] =
    g_param_spec_uint64 ("available", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_USED] =
    g_param_spec_uint64 ("used", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_PERCENTAGE] =
    g_param_spec_int ("percentage", NULL, NULL,
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_DISPLAY_DEVICE] =
    g_param_spec_string ("display-device", NULL, NULL,
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_DISPLAY_DIRECTORY] =
    g_param_spec_string ("display-directory", NULL, NULL,
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}


static void
gsm_disk_init (GsmDisk *self)
{
  self->icon = g_themed_icon_new_with_default_fallbacks ("drive-harddisk");
}


GsmDisk *
gsm_disk_new (GFile      *device,
              GFile      *directory,
              const char *type,
              uint64_t    total,
              uint64_t    free,
              uint64_t    available,
              uint64_t    used,
              int         percentage)
{
  return g_object_new (GSM_TYPE_DISK,
                       "device", device,
                       "directory", directory,
                       "type", type,
                       "total", total,
                       "free", free,
                       "available", available,
                       "used", used,
                       "percentage", percentage,
                       NULL);
}


void
gsm_disk_set_device (GsmDisk *self, GFile *device)
{
  g_autofree char *path = NULL;

  g_return_if_fail (GSM_IS_DISK (self));
  g_return_if_fail (device == NULL || G_IS_FILE (device));

  if (self->device && device && g_file_equal (self->device, device)) {
    return;
  }

  if (!g_set_object (&self->device, device)) {
    return;
  }

  path = g_file_get_path (device);
  g_clear_pointer (&self->display_device, g_free);
  self->display_device = g_filename_display_name (path);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DEVICE]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DISPLAY_DEVICE]);
}


static void
found_mount (GObject *source, GAsyncResult *result, gpointer user_data)
{
  g_autoptr (GsmDisk) self = GSM_DISK (user_data);
  g_autoptr (GError) error = NULL;
  g_autoptr (GMount) mount =
    g_file_find_enclosing_mount_finish (G_FILE (source), result, &error);
  g_autoptr (GIcon) icon = NULL;

  if (error) {
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_FAILED)) {
      g_warning ("%s", error->message);
    }
  } else {
    icon = g_mount_get_icon (mount);
  }

  if (!icon) {
    g_autofree char *path = g_file_get_path (self->directory);
    g_autoptr (GUnixMountEntry) entry =
      g_unix_mount_entry_at (path, NULL);
    if (entry) {
      icon = g_unix_mount_entry_guess_icon (entry);
    }
  }

  if (!icon) {
    icon = g_themed_icon_new_with_default_fallbacks ("drive-harddisk");
  }

  if (g_set_object (&self->icon, icon)) {
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
  }
}


void
gsm_disk_set_directory (GsmDisk *self, GFile *directory)
{
  g_return_if_fail (GSM_IS_DISK (self));
  g_return_if_fail (directory == NULL || G_IS_FILE (directory));

  if (self->directory && directory &&
      g_file_equal (self->directory, directory)) {
    return;
  }

  if (!g_set_object (&self->directory, directory)) {
    return;
  }

  g_clear_pointer (&self->display_directory, g_free);

  if (self->directory) {
    g_autofree char *path = g_file_get_path (directory);
    self->display_directory = g_filename_display_name (path);

    g_file_find_enclosing_mount_async (self->directory,
                                       G_PRIORITY_HIGH_IDLE,
                                       NULL,
                                       found_mount,
                                       g_object_ref (self));
  } else {
    g_autoptr (GIcon) falback =
      g_themed_icon_new_with_default_fallbacks ("drive-harddisk");

    if (!g_icon_equal (self->icon, falback)) {
      g_set_object (&self->icon, falback);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
    }
  }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DIRECTORY]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DISPLAY_DIRECTORY]);
}


static void
did_launch (GObject      *source,
            GAsyncResult *result,
            gpointer      user_data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GTask) task = G_TASK (user_data);

  gtk_file_launcher_launch_finish (GTK_FILE_LAUNCHER (source), result, &error);

  if (error) {
    GFile *file = NULL;
    g_autofree char *uri = NULL;

    file = gtk_file_launcher_get_file (GTK_FILE_LAUNCHER (source));
    uri = g_file_get_uri (file);

    g_task_return_prefixed_error (task,
                                  g_steal_pointer (&error),
                                  "‘%s’: ",
                                  uri);
    return;
  }

  g_task_return_int (task, 0);
}


void
gsm_disk_open_root (GsmDisk             *self,
                    GtkWindow           *parent,
                    GCancellable        *cancellable,
                    GAsyncReadyCallback  callback,
                    gpointer             callback_data)
{
  g_autoptr (GTask) task = NULL;
  g_autoptr (GtkFileLauncher) launcher = NULL;
  GCancellable *task_cancellable;

  g_return_if_fail (GSM_IS_DISK (self));

  task = g_task_new (self, cancellable, callback, callback_data);

  launcher = gtk_file_launcher_new (self->directory);
  task_cancellable = g_task_get_cancellable (task);
  gtk_file_launcher_launch (launcher,
                            parent,
                            task_cancellable,
                            did_launch,
                            g_steal_pointer (&task));
}


void
gsm_disk_open_root_finish (GsmDisk       *self,
                           GAsyncResult  *result,
                           GError       **error)
{
  g_return_if_fail (GSM_IS_DISK (self));
  g_return_if_fail (g_task_is_valid (result, self));

  g_task_propagate_int (G_TASK (result), error);
}
