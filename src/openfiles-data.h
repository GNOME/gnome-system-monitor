#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define OPENFILES_TYPE_DATA (openfiles_data_get_type())

G_DECLARE_FINAL_TYPE (OpenFilesData, openfiles_data, OPENFILES, DATA, GObject)

OpenFilesData *openfiles_data_new (gint         fd,
                                   const gchar *type,
                                   const gchar *object,
                                   gpointer     pointer);

G_END_DECLS
