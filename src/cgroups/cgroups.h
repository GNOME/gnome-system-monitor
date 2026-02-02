/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <sys/types.h>

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

G_MODULE_EXPORT
gboolean    gsm_cgroups_is_enabled       (void);
G_MODULE_EXPORT
const char *gsm_cgroups_extract_name     (const char *file_text);
G_MODULE_EXPORT
char       *gsm_cgroups_get_name         (pid_t       pid);

G_END_DECLS
