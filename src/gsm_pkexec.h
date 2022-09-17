#ifndef _GSM_GSM_PKEXEC_H_
#define _GSM_GSM_PKEXEC_H_

#include <glib.h>

gboolean
gsm_pkexec_create_root_password_dialog (const char *command);

gboolean
procman_has_pkexec (void) G_GNUC_CONST;

#endif /* _GSM_PKEXEC_H_ */
