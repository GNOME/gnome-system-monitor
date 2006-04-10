#include <config.h>

/* FIXME: hack the Makefile.am */

#if HAVE_LIBGKSU

#include <gksu.h>
#include <gksuui-dialog.h>

#include "procman.h"
#include "procman_gksu.h"



gboolean
procman_gksu_create_root_password_dialog(char * command)
{
       static GksuContext *context;
       GError *error = NULL;
       gboolean ret = TRUE;

       if (!context) {
	       context = gksu_context_new();
	       gksu_context_set_debug(context, TRUE);
       }

       gksu_context_set_command(context, command);

       if (gksu_context_try_need_password(context)) {
	       GtkWidget *dialog;
	       gint response;

	       dialog = gksuui_dialog_new();
	       response = gtk_dialog_run(GTK_DIALOG(dialog));

	       if (response == GTK_RESPONSE_OK) {
		       char *password;
		       password = gksuui_dialog_get_password(GKSUUI_DIALOG(dialog));
		       gksu_context_set_password(context, password);
		       memset(password, g_random_int(), strlen(password));
		       g_free(password);
	       }

	       gtk_widget_destroy(dialog);

	       if (response != GTK_RESPONSE_OK) {
		       ret = FALSE;
		       goto out;
	       }
       }

       if (gksu_context_sudo_run(context, &error) == -1) {
	       g_warning("error: %s - command: '%s' - user: '%s'\n",
			 error->message,
			 gksu_context_get_command(context),
			 gksu_context_get_user(context));
	       g_error_free(error);
	       ret = FALSE;
       }

 out:
       return ret;
}


#endif /* HAVE_LIBGKSU */
