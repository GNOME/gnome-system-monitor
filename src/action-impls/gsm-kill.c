/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <errno.h>
#include <signal.h>
#include <sys/types.h>

#include "gsm-kill.h"


void
gsm_kill (pid_t    pid,
          int      signal,
          GError **error)
{
  int err;

  if (kill (pid, signal) >= 0) {
    return;
  }

  err = errno;

  g_set_error (error,
               G_FILE_ERROR,
               g_file_error_from_errno (err),
               _("Kill failed on %i: %s"),
               pid,
               g_strerror (err));
}
