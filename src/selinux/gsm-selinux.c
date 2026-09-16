/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>

#include "gsm-module-loader.h"

#include "gsm-selinux.h"


static int (*getpidcon) (pid_t, char **);
static void (*freecon) (char *);
static int (*is_selinux_enabled) (void);


GSM_DEFINE_MODULE_LOADER (gsm, selinux, "libselinux.so.1")


static inline GsmModuleInitResult
gsm_selinux_init_module (GModule *module)
{
  GSM_MODULE_REQUIRE_SYMBOL (module, "libselinux.so.1", getpidcon);
  GSM_MODULE_REQUIRE_SYMBOL (module, "libselinux.so.1", freecon);
  GSM_MODULE_REQUIRE_SYMBOL (module, "libselinux.so.1", is_selinux_enabled);

  g_module_make_resident (module);

  return GSM_MODULE_INIT_SUCCESS;
}


gboolean
gsm_selinux_is_enabled (void)
{
  if (!gsm_selinux_get_or_load_module ()) {
    return FALSE;
  }

  switch (is_selinux_enabled ()) {
    case 1:
      /* We're running on an SELinux kernel */
      return TRUE;
    case -1:
      /* Error; hide the security context column */
    case 0:
      /* We're not running on an SELinux kernel:
         hide the security context column */
    default:
      g_debug ("SELinux was found but is not enabled.\n");
      return FALSE;
  }
}


char *
gsm_selinux_get_context (pid_t pid)
{
  g_autofree char *result = NULL;
  char *con = NULL;

  if (!gsm_selinux_get_or_load_module ()) {
    goto out;
  }

  if (getpidcon (pid, &con) != 0) {
    goto out;
  }

  /* Copy into glib memory */
  g_set_str (&result, con);

out:
  /* This comes from selinux's alloc */
  g_clear_pointer (&con, freecon);

  return g_steal_pointer (&result);
}
