#pragma once

#include <gio/gio.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define LSOF_TYPE_DATA (lsof_data_get_type())

G_DECLARE_FINAL_TYPE (LsofData, lsof_data, LSOF, DATA, GObject)

LsofData *lsof_data_new (GdkPaintable *paintable,
                         const gchar  *process,
                         guint         pid,
                         const gchar  *filename);

G_END_DECLS
