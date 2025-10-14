/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <stdint.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSM_TYPE_DISK (gsm_disk_get_type ())
G_DECLARE_FINAL_TYPE (GsmDisk, gsm_disk, GSM, DISK, GObject)

GsmDisk    *gsm_disk_new               (GFile                *device,
                                        GFile                *directory,
                                        const char           *type,
                                        uint64_t              total,
                                        uint64_t              free,
                                        uint64_t              available,
                                        uint64_t              used,
                                        int                   percentage);
void        gsm_disk_set_device        (GsmDisk              *self,
                                        GFile                *device);
void        gsm_disk_set_directory     (GsmDisk              *self,
                                        GFile                *directory);
gboolean    gsm_disk_is_for_directory  (GsmDisk              *self,
                                        GFile               *directory);
void        gsm_disk_open_root         (GsmDisk              *self,
                                        GtkWindow            *parent,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              callback_data);
void        gsm_disk_open_root_finish  (GsmDisk              *self,
                                        GAsyncResult         *result,
                                        GError              **error);

G_END_DECLS
