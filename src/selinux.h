/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _GSM_SELINUX_H_
#define _GSM_SELINUX_H_

#include <glib.h>

#include "application.h"

void
get_process_selinux_context (ProcInfo *info);

gboolean
can_show_security_context_column (void) G_GNUC_CONST;

#endif /* _GSM_SELINUX_H_ */
