
#ifndef _UTIL_H_
#define _UTIL_H_

#include <gnome.h>

/* -1 means wrong password, 0 means another error, 1 means great */
gint 		su_run_with_password (gchar *exec_path, gchar *password);

gchar*		get_size_string (gint size);
#endif
