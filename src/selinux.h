#ifndef PROCMAN_SELINUX_H_20050525
#define PROCMAN_SELINUX_H_20050525

#include <glib.h>

#include "procman.h"


#if HAVE_SELINUX

void
get_process_selinux_context (ProcInfo *info) G_GNUC_INTERNAL;

gboolean
can_show_security_context_column (void) G_GNUC_INTERNAL;

#else /* ! HAVE_SELINUX */

static inline void
get_process_selinux_context (ProcInfo *info) { }

static inline gboolean
can_show_security_context_column (void) { return FALSE; }

#endif /* HAVE_SELINUX */


#endif /* PROCMAN_SELINUX_H_20050525 */
