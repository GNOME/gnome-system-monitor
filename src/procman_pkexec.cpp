#include <config.h>

#include "procman-app.h"
#include "procman_pkexec.h"
#include "util.h"

gboolean procman_pkexec_create_root_password_dialog(const char *command)
{
    gint *exit_status = NULL;
    GError *error = NULL;
    if (!g_spawn_command_line_sync( g_strdup_printf("pkexec %s", command), NULL, NULL, exit_status, &error)) {
        g_critical("Could not run pkexec(\"%s\") : %s\n",
                   command, error->message);
        g_error_free(error);
        return FALSE;
    }

    g_message("pkexec did fine\n");
    return TRUE;
}



gboolean
procman_has_pkexec(void)
{
    return g_file_test("/usr/bin/pkexec", G_FILE_TEST_EXISTS);
}

