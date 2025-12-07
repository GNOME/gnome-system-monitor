/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

G_MODULE_EXPORT
gboolean   gsm_gksu_create_root_password_dialog (const char *command);
G_MODULE_EXPORT
gboolean   procman_has_gksu                     (void) G_GNUC_CONST;

G_END_DECLS
