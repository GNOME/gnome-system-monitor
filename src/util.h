
#ifndef _PROCMAN_UTIL_H_
#define _PROCAMN_UTIL_H_

#include <gnome.h>
#include <stddef.h>

/* -1 means wrong password, 0 means another error, 1 means great */
gint 		su_run_with_password (gchar *exec_path, gchar *password);

gchar*		get_size_string (gfloat size);


#define PROCMAN_GETTEXT_ARRAY_INIT(A) G_STMT_START { \
static gboolean is_init = FALSE; \
if(!is_init) { \
  size_t i; \
  for(i = 0; i < G_N_ELEMENTS(A); ++i) if((A)[i]) ((A)[i]) = _(((A)[i])); \
  is_init = TRUE; \
 } \
} G_STMT_END

#endif /* _PROCAMN_UTIL_H_ */
