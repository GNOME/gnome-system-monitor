/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <errno.h>
#include <sys/resource.h>
#include <sys/types.h>

#include "gsm-setpriority.h"


void
gsm_setpriority (pid_t    pid,
                 int      priority,
                 GError **error)
{
  int err;

  if (setpriority (PRIO_PROCESS, pid, priority) >= 0) {
    return;
  }

  err = errno;

  g_set_error (error,
               G_FILE_ERROR,
               g_file_error_from_errno (err),
               _("Priority failed on %i: %s"),
               pid,
               g_strerror (err));
}
