#include <config.h>

/* FIXME: hack the Makefile.am */

#if HAVE_LIBGKSU

#include <gksu.h>

#include "procman.h"
#include "procman_gksu.h"



gboolean
procman_gksu_create_root_password_dialog(char * command)
{
       GksuContext *context;
       GError *error = NULL;
       gboolean ret = TRUE;

       context = gksu_context_new();
       gksu_context_set_debug(context, TRUE);

       gksu_context_set_command(context, command);

       if (gksu_context_sudo_run(context, &error) == -1) {
	       g_warning("error: %s - command: '%s' - user: '%s'\n",
			 error->message,
			 gksu_context_get_command(context),
			 gksu_context_get_user(context));
	       g_error_free(error);
	       ret = FALSE;
       }

       gksu_context_free(context);

       return ret;
}


#endif /* HAVE_LIBGKSU */
