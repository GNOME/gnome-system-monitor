/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "gsm-kill.h"
#include "gsm-setaffinity.h"
#include "gsm-setpriority.h"

#include "gsm_gnomesu.h"
#include "gsm_gksu.h"
#include "gsm_pkexec.h"

#include "gsm-actions.h"


struct _GsmActions {
  GObject parent;
};


G_DEFINE_FINAL_TYPE (GsmActions, gsm_actions, G_TYPE_OBJECT)


static void
gsm_actions_class_init (G_GNUC_UNUSED GsmActionsClass *klass)
{
}


static void
gsm_actions_init (G_GNUC_UNUSED GsmActions *self)
{
}


static inline gboolean
multi_root_check (const char *command)
{
  if (procman_has_pkexec ()) {
    return gsm_pkexec_create_root_password_dialog (command);
  }

  if (procman_has_gksu ()) {
    return gsm_gksu_create_root_password_dialog (command);
  }

  if (procman_has_gnomesu ()) {
    return gsm_gnomesu_create_root_password_dialog (command);
  }

  return FALSE;
}


typedef struct _Kill Kill;
struct _Kill {
  pid_t pid;
  int signal;
};


static void
kill_thread (GTask                      *task,
             G_GNUC_UNUSED gpointer      source_object,
             gpointer                    task_data,
             G_GNUC_UNUSED GCancellable *cancellable)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *command = NULL;
  Kill *data = task_data;

  gsm_kill (data->pid, data->signal, &error);
  if (!error) {
    g_task_return_int (task, 0);
    return;
  }

  /* We'll handle permission errors by attempting root */
  if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_PERM)) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  /* Now's a good time to bail out if appropriate */
  if (g_task_return_error_if_cancelled (task)) {
    return;
  }

  command = g_strdup_printf ("kill -s %d %d", data->signal, data->pid);
  if (!multi_root_check (command)) {
    g_task_return_new_error (task,
                             G_FILE_ERROR,
                             G_FILE_ERROR_FAILED,
                             _("Kill helper failed"));
    return;
  }

  g_task_return_int (task, 0);
}


void
gsm_actions_kill (GsmActions          *self,
                  pid_t                pid,
                  int                  signal,
                  GCancellable        *cancellable,
                  GAsyncReadyCallback  callback,
                  gpointer             callback_data)
{
  g_autoptr (GTask) task =
    g_task_new (self, cancellable, callback, callback_data);
  Kill *data;

  g_return_if_fail (GSM_IS_ACTIONS (self));

  data = g_new0 (Kill, 1);
  data->pid = pid;
  data->signal = signal;

  g_task_set_source_tag (task, gsm_actions_kill);
  g_task_set_task_data (task, data, g_free);

  g_task_run_in_thread (task, kill_thread);
}


void
gsm_actions_kill_finish (GsmActions    *self,
                         GAsyncResult  *result,
                         GError       **error)
{
  g_return_if_fail (GSM_IS_ACTIONS (self));
  g_return_if_fail (g_task_is_valid (result, self));

  g_task_propagate_int (G_TASK (result), error);
}


typedef struct _SetAffinity SetAffinity;
struct _SetAffinity {
  pid_t pid;
  size_t n_cpus;
  uint16_t *cpus;
  gboolean apply_to_threads;
};


static void
set_affinity_destroy (gpointer user_data)
{
  SetAffinity *self = user_data;

  g_clear_pointer (&self->cpus, g_free);

  g_free (self);
}


static inline gboolean
execute_taskset_command (size_t     n_cpus,
                         uint16_t   cpus[],
                         pid_t      pid,
                         gboolean   child_threads)
{
#ifdef __linux__
  g_autoptr (GStrvBuilder) list = g_strv_builder_new ();
  g_auto (GStrv) cpu_list = NULL;
  g_autofree char *pc = NULL;
  g_autofree char *command = NULL;

  for (size_t i = 0; i < n_cpus; i++) {
    g_strv_builder_take (list, g_strdup_printf ("%i", cpus[i]));
  }
  cpu_list = g_strv_builder_end (list);

  /* Join CPU number strings by commas for taskset command CPU list */
  pc = g_strjoinv (",", cpu_list);

  /* Construct taskset command */
  command = g_strdup_printf ("taskset -pc%c %s %d", child_threads ? 'a' : ' ', pc, pid);

  /* Execute taskset command; show error on failure */
  if (!multi_root_check (command)) {
    return FALSE;
  }

  return TRUE;
#else
  return FALSE;
#endif
}


static void
set_affinity_thread (GTask                      *task,
                     G_GNUC_UNUSED gpointer      source_object,
                     gpointer                    task_data,
                     G_GNUC_UNUSED GCancellable *cancellable)
{
  g_autoptr (GError) error = NULL;
  SetAffinity *data = task_data;

  gsm_setaffinity (data->pid,
                   data->n_cpus,
                   data->cpus,
                   data->apply_to_threads,
                   &error);
  if (!error) {
    g_task_return_int (task, 0);
    return;
  }

  /* Check whether an access error occurred */
  if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_PERM) &&
      !g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_ACCES)) {
    g_task_return_error (task, g_steal_pointer (&error));
  }

  /* Now's a good time to bail out if appropriate */
  if (g_task_return_error_if_cancelled (task)) {
    return;
  }

  /* Attempt to run taskset as root, show error on failure */
  if (!execute_taskset_command (data->n_cpus,
                                data->cpus,
                                data->pid,
                                data->apply_to_threads)) {
    g_task_return_new_error (task,
                             G_FILE_ERROR,
                             G_FILE_ERROR_FAILED,
                             _("Affinity helper failed"));
    return;
  }

  g_task_return_int (task, 0);
}


void
gsm_actions_set_affinity (GsmActions          *self,
                          pid_t                pid,
                          size_t               n_cpus,
                          uint16_t             cpus[],
                          gboolean             apply_to_threads,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             callback_data)
{
  g_autoptr (GTask) task =
    g_task_new (self, cancellable, callback, callback_data);
  SetAffinity *data;

  g_return_if_fail (GSM_IS_ACTIONS (self));

  data = g_new0 (SetAffinity, 1);
  data->pid = pid;
  data->n_cpus = n_cpus;
  data->cpus = g_memdup2 (cpus, sizeof (uint16_t) * n_cpus);
  data->apply_to_threads = apply_to_threads;

  g_task_set_source_tag (task, gsm_actions_set_affinity);
  g_task_set_task_data (task, data, set_affinity_destroy);

  g_task_run_in_thread (task, set_affinity_thread);
}


void
gsm_actions_set_affinity_finish (GsmActions    *self,
                                 GAsyncResult  *result,
                                 GError       **error)
{
  g_return_if_fail (GSM_IS_ACTIONS (self));
  g_return_if_fail (g_task_is_valid (result, self));

  g_task_propagate_int (G_TASK (result), error);
}


typedef struct _Priority Priority;
struct _Priority {
  pid_t pid;
  int priority;
};


static void
set_priority_thread (GTask                      *task,
                     G_GNUC_UNUSED gpointer      source_object,
                     gpointer                    task_data,
                     G_GNUC_UNUSED GCancellable *cancellable)
{
  g_autoptr (GError) error = NULL;
  Priority *data = task_data;
  g_autofree char *command = NULL;

  gsm_setpriority (data->pid, data->priority, &error);
  if (!error) {
    g_task_return_int (task, 0);
    return;
  }

  /* We'll handle permission errors by attempting root */
  if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_PERM) &&
      !g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_ACCES)) {
    g_task_return_error (task, g_steal_pointer (&error));
    return;
  }

  /* Now's a good time to bail out if appropriate */
  if (g_task_return_error_if_cancelled (task)) {
    return;
  }

  command = g_strdup_printf ("renice %d %d", data->priority, data->pid);
  if (!multi_root_check (command)) {
    g_task_return_new_error (task,
                             G_FILE_ERROR,
                             G_FILE_ERROR_FAILED,
                             _("Priority helper failed"));
    return;
  }

  g_task_return_int (task, 0);
}


void
gsm_actions_set_priority (GsmActions          *self,
                          pid_t                pid,
                          int                  priority,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             callback_data)
{
  g_autoptr (GTask) task =
    g_task_new (self, cancellable, callback, callback_data);
  Priority *data;

  g_return_if_fail (GSM_IS_ACTIONS (self));

  data = g_new0 (Priority, 1);
  data->pid = pid;
  data->priority = priority;

  g_task_set_source_tag (task, gsm_actions_set_priority);
  g_task_set_task_data (task, data, g_free);

  g_task_run_in_thread (task, set_priority_thread);
}


void
gsm_actions_set_priority_finish (GsmActions    *self,
                                 GAsyncResult  *result,
                                 GError       **error)
{
  g_return_if_fail (GSM_IS_ACTIONS (self));
  g_return_if_fail (g_task_is_valid (result, self));

  g_task_propagate_int (G_TASK (result), error);
}
