/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <sys/types.h>

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

G_MODULE_EXPORT
gboolean  gsm_systemd_is_running       (void);
G_MODULE_EXPORT
gboolean  gsm_systemd_get_process_info (pid_t     pid,
                                        char    **sd_unit,
                                        char    **sd_session,
                                        char    **sd_seat,
                                        uid_t    *sd_owner);

G_END_DECLS
