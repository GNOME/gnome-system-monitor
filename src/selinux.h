#ifndef _GSM_SELINUX_H_
#define _GSM_SELINUX_H_

#include <glib.h>

#include "procinfo.h"

void
get_process_selinux_context (ProcInfo *info);

gboolean
can_show_security_context_column (void) G_GNUC_CONST;

#endif /* _GSM_SELINUX_H_ */
