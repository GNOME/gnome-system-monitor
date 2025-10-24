/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <gio/gio.h>
#include <gmodule.h>

G_BEGIN_DECLS


#define GSM_TYPE_ACTIONS (gsm_actions_get_type ())
G_MODULE_EXPORT
G_DECLARE_FINAL_TYPE (GsmActions, gsm_actions, GSM, ACTIONS, GObject)

G_MODULE_EXPORT
void          gsm_actions_kill                           (GsmActions           *self,
                                                          pid_t                 pid,
                                                          int                   signal,
                                                          GCancellable         *cancellable,
                                                          GAsyncReadyCallback   callback,
                                                          gpointer              callback_data);
G_MODULE_EXPORT
void          gsm_actions_kill_finish                    (GsmActions           *self,
                                                          GAsyncResult         *result,
                                                          GError              **error);
G_MODULE_EXPORT
void          gsm_actions_set_affinity                   (GsmActions           *self,
                                                          pid_t                 pid,
                                                          size_t                n_cpus,
                                                          uint16_t              cpus[],
                                                          gboolean              apply_to_threads,
                                                          GCancellable         *cancellable,
                                                          GAsyncReadyCallback   callback,
                                                          gpointer              callback_data);
G_MODULE_EXPORT
void          gsm_actions_set_affinity_finish            (GsmActions           *self,
                                                          GAsyncResult         *result,
                                                          GError              **error);
G_MODULE_EXPORT
void          gsm_actions_set_priority                   (GsmActions           *self,
                                                          pid_t                 pid,
                                                          int                   priority,
                                                          GCancellable         *cancellable,
                                                          GAsyncReadyCallback   callback,
                                                          gpointer              callback_data);
G_MODULE_EXPORT
void          gsm_actions_set_priority_finish            (GsmActions           *self,
                                                          GAsyncResult         *result,
                                                          GError              **error);

G_END_DECLS
