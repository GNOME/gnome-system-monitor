/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <adwaita.h>

#include "procinfo.h"

G_BEGIN_DECLS

#define GSM_TYPE_OPEN_FILES (gsm_open_files_get_type ())
G_DECLARE_FINAL_TYPE (GsmOpenFiles, gsm_open_files, GSM, OPEN_FILES, AdwWindow)

GsmOpenFiles *gsm_open_files_new (GtkApplication *app, ProcInfo *info);

G_END_DECLS
