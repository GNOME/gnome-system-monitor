#ifndef _GSM_GSM_GKSU_H_
#define _GSM_GSM_GKSU_H_

#include <glib.h>

gboolean
gsm_gksu_create_root_password_dialog (const char *command);

gboolean
procman_has_gksu (void) G_GNUC_CONST;

#endif /* _GSM_GSM_GKSU_H_ */
