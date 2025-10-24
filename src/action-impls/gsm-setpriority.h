/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <sys/types.h>

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

G_MODULE_EXPORT
void        gsm_setpriority            (pid_t                 pid,
                                        int                   priority,
                                        GError              **error);

G_END_DECLS
