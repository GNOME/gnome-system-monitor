#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define MEMMAPS_TYPE_INFO (memmaps_info_get_type())

G_DECLARE_FINAL_TYPE (MemMapsInfo, memmaps_info, MEMMAPS, INFO, GObject)

MemMapsInfo *memmaps_info_new (const gchar *filename,
                               const gchar *vmstart,
                               const gchar *vmend,
                               const gchar *vmsize,
                               const gchar *flags,
                               const gchar *vmoffset,
                               const gchar *privateclean,
                               const gchar *privatedirty,
                               const gchar *sharedclean,
                               const gchar *shareddirty,
                               const gchar *device,
                               guint64      inode);

G_END_DECLS
