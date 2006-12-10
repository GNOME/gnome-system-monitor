#ifndef PROCMAN_SELINUX_H_20050525
#define PROCMAN_SELINUX_H_20050525

#include <glib.h>

#include "procman.h"

G_BEGIN_DECLS

void
get_process_selinux_context (ProcInfo *info) G_GNUC_INTERNAL;

gboolean
can_show_security_context_column (void) G_GNUC_INTERNAL G_GNUC_CONST;

G_END_DECLS

#endif /* PROCMAN_SELINUX_H_20050525 */
