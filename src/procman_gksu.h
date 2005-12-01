#ifndef H_GNOME_SYSTEM_MONITOR_GKSU_H_1132171928
#define H_GNOME_SYSTEM_MONITOR_GKSU_H_1132171928

#include <glib/gtypes.h>

#if HAVE_LIBGKSU

gboolean
procman_gksu_create_root_password_dialog(char * command);

#else

static inline gboolean
procman_gksu_create_root_password_dialog(char * command)
{
	return FALSE;
}

#endif


#endif /* H_GNOME_SYSTEM_MONITOR_GKSU_H_1132171928 */
