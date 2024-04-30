#pragma once

#include <gio/gio.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define DISKS_TYPE_DATA (disks_data_get_type())

G_DECLARE_FINAL_TYPE (DisksData, disks_data, DISKS, DATA, GObject)

DisksData *disks_data_new (GdkPaintable *paintable,
                           const gchar  *device,
                           const gchar  *directory,
                           const gchar  *type,
                           const gchar  *total,
                           const gchar  *free,
                           const gchar  *available,
                           const gchar  *used,
                           gint          percentage);

G_END_DECLS
