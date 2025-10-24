/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#define _GNU_SOURCE

#include <glib/gi18n-lib.h>

#include <errno.h>
#include <sched.h>
#include <sys/types.h>

#include "gsm-setaffinity.h"


#ifdef __linux__
static inline gboolean
is_task (const char *tasks_dir, const char *name)
{
  g_autofree char *path = g_build_filename (tasks_dir, name, NULL);

  return g_file_test (path, G_FILE_TEST_IS_DIR);
}


static inline void
threads_setaffinity (pid_t       pid,
                     size_t      sizeof_set,
                     cpu_set_t  *set,
                     GError    **error)
{
  g_autoptr (GError) local_error = NULL;
  g_autofree char *tasks = g_strdup_printf ("/proc/%d/task", pid);
  g_autoptr (GDir) dir = g_dir_open (tasks, 0, &local_error);

  if (local_error) {
    g_propagate_prefixed_error (error,
                                local_error,
                                "Failed to find threads: %s",
                                local_error->message);
    return;
  }

  while (TRUE) {
    const char *name = g_dir_read_name (dir);
    int64_t task_no;

    if (!name) {
      break;
    }

    if (!is_task (tasks, name)) {
      continue;
    }

    g_ascii_string_to_signed (name,
                              10,
                              0,
                              G_MAXINT,
                              &task_no,
                              &local_error);
    if (local_error) {
      g_autofree char *display = g_filename_display_name (name);
      g_warning ("‘%s’ doesn't look like an ID: %s",
                 display,
                 local_error->message);
      continue;
    }

    /* We let this fail silently */
    sched_setaffinity ((pid_t) task_no, sizeof_set, set);
  }
}
#endif


void
gsm_setaffinity (pid_t      pid,
                 size_t     n_cpus,
                 uint16_t   cpus[],
                 gboolean   apply_to_threads,
                 GError   **error)
{
#ifdef __linux__
  cpu_set_t set;
  size_t sizeof_set;

  CPU_ZERO (&set);

  for (size_t i = 0; i < n_cpus; i++) {
    CPU_SET (cpus[i], &set);
  }

  sizeof_set = sizeof (set);
  if (sched_setaffinity (pid, sizeof_set, &set) < 0) {
    int err = errno;

    g_set_error (error,
                 G_FILE_ERROR,
                 g_file_error_from_errno (err),
                 _("Affinity failed on %i: %s"),
                 pid,
                 g_strerror (err));

    return;
  }

  /* set child threads if required */
  if (apply_to_threads) {
    threads_setaffinity (pid, sizeof_set, &set, error);
  }
#else
  g_set_error (error,
               G_FILE_ERROR,
               G_FILE_ERROR_FAILED,
               _("Affinity unsupported"));
#endif
}
