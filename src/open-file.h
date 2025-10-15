/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <glib-object.h>

#include <glibtop/procopenfiles.h>

#include "procinfo.h"

G_BEGIN_DECLS

#define GSM_TYPE_OPEN_FILE (gsm_open_file_get_type ())
G_DECLARE_FINAL_TYPE (GsmOpenFile, gsm_open_file, GSM, OPEN_FILE, GObject)

GsmOpenFile *gsm_open_file_new       (ProcInfo                 *info,
                                      glibtop_open_files_entry *entry);
void         gsm_open_file_set_entry (GsmOpenFile              *self,
                                      glibtop_open_files_entry *entry);

G_END_DECLS
