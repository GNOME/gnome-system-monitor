#include <config.h>

#include <glib.h>

#include "selinux.h"
#include "procman.h"

#if HAVE_SELINUX

#include <selinux/selinux.h>


void
get_process_selinux_context (ProcInfo *info)
{
	/* Directly grab the SELinux security context: */
	security_context_t con;

	if (!getpidcon (info->pid, &con)) {
		info->security_context = g_strdup (con);
		freecon (con);
	}
}



gboolean
can_show_security_context_column (void)
{
	switch (is_selinux_enabled()) {
	case 1:
		/* We're running on an SELinux kernel */
		return TRUE;

	case -1:
		/* Error; hide the security context column */

	case 0:
		/* We're not running on an SELinux kernel:
		   hide the security context column */

	default:
		return FALSE;
	}
}

#endif
