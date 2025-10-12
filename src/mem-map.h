/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <glib-object.h>
#include <stdint.h>

G_BEGIN_DECLS

#define GSM_TYPE_MEM_MAP (gsm_mem_map_get_type ())
G_DECLARE_FINAL_TYPE (GsmMemMap, gsm_mem_map, GSM, MEM_MAP, GObject)

GsmMemMap *gsm_mem_map_new (const char *filename,
                            uint64_t    vm_start,
                            uint64_t    vm_end,
                            uint64_t    vm_size,
                            const char *flags,
                            uint64_t    vm_offset,
                            uint64_t    private_clean,
                            uint64_t    private_dirty,
                            uint64_t    shared_clean,
                            uint64_t    shared_dirty,
                            const char *device,
                            uint64_t    inode);

G_END_DECLS
