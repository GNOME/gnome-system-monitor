/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

G_MODULE_EXPORT
void        gsm_setaffinity            (pid_t                 pid,
                                        size_t                n_cpus,
                                        uint16_t              cpus[],
                                        gboolean              apply_to_threads,
                                        GError              **error);

G_END_DECLS
