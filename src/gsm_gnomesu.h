#ifndef _GSM_GSM_GNOMESU_H_
#define _GSM_GSM_GNOMESU_H_

#include <glib.h>

gboolean
gsm_gnomesu_create_root_password_dialog (const char *message);

gboolean
procman_has_gnomesu (void) G_GNUC_CONST;

#endif /* _GSM_GSM_GNOMESU_H_ */
