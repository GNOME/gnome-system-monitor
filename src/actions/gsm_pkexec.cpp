/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include "gsm_pkexec.h"


gboolean
gsm_pkexec_create_root_password_dialog (const char *command)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *command_line = NULL;
  gboolean ret = FALSE;
  int *exit_status = NULL;

  command_line = g_strdup_printf ("pkexec --disable-internal-agent %s/gsm-%s",
                                  GSM_LIBEXEC_DIR,
                                  command);

  if (!g_spawn_command_line_sync (command_line, NULL, NULL, exit_status, &error)) {
    g_critical ("Could not run pkexec(\"%s\") : %s\n",
                command,
                error->message);
  } else {
    g_debug ("pkexec did fine\n");
    ret = TRUE;
  }

  return ret;
}


gboolean
procman_has_pkexec (void)
{
  return g_file_test ("/usr/bin/pkexec", G_FILE_TEST_EXISTS);
}
