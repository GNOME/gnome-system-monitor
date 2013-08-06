/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef H_GNOME_SYSTEM_MONITOR_PKEXEC_H_
#define H_GNOME_SYSTEM_MONITOR_PKEXEC_H_

#include <glib.h>

gboolean
gsm_pkexec_create_root_password_dialog(const char *command);

gboolean
procman_has_pkexec(void) G_GNUC_CONST;

#endif /* H_GNOME_SYSTEM_MONITOR_PKEXEC_H_ */
