#include <config.h>

#include "gsm_pkexec.h"
#include "util.h"

gboolean
gsm_pkexec_create_root_password_dialog (const char *command)
{
  gboolean ret = FALSE;
  gint *exit_status = NULL;
  GError *error = NULL;
  gchar *command_line = g_strdup_printf ("pkexec --disable-internal-agent %s/gsm-%s",
                                         GSM_LIBEXEC_DIR, command);

  if (!g_spawn_command_line_sync (command_line, NULL, NULL, exit_status, &error))
    {
      g_critical ("Could not run pkexec(\"%s\") : %s\n",
                  command, error->message);
      g_error_free (error);
    }
  else
    {
      g_debug ("pkexec did fine\n");
      ret = TRUE;
    }

  g_free (command_line);

  return ret;
}



gboolean
procman_has_pkexec (void)
{
  return g_file_test ("/usr/bin/pkexec", G_FILE_TEST_EXISTS);
}
