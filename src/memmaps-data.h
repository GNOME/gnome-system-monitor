#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define MEMMAPS_TYPE_DATA (memmaps_data_get_type())

G_DECLARE_FINAL_TYPE (MemMapsData, memmaps_data, MEMMAPS, DATA, GObject)

MemMapsData *memmaps_data_new (const gchar *filename,
                               const gchar *vmstart,
                               const gchar *vmend,
                               guint64      vmsize,
                               const gchar *flags,
                               const gchar *vmoffset,
                               guint64      privateclean,
                               guint64      privatedirty,
                               guint64      sharedclean,
                               guint64      shareddirty,
                               const gchar *device,
                               guint64      inode);

G_END_DECLS
