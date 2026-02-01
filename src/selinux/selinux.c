/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>

#include "selinux.h"


static int (*getpidcon) (pid_t, char **);
static void (*freecon) (char *);
static int (*is_selinux_enabled) (void);


static gboolean
load_selinux (void)
{
  static GModule *module = NULL;

  if (g_once_init_enter_pointer (&module)) {
    GModule *selinux = g_module_open ("libselinux.so.1",
                                      G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

    if (!g_module_symbol (selinux, "getpidcon", (gpointer *) &getpidcon)) {
      g_debug ("Could not load getpidcon from libselinux.so.0");
      return FALSE;
    } else {
      g_debug ("Loaded getpidcon from libselinux.so.0");
    }

    if (!g_module_symbol (selinux, "freecon", (gpointer *) &freecon)) {
      g_debug ("Could not load freecon from libselinux.so.0");
      return FALSE;
    } else {
      g_debug ("Loaded freecon from libselinux.so.0");
    }

    if (!g_module_symbol (selinux,
                          "is_selinux_enabled",
                          (gpointer *) &is_selinux_enabled)) {
      g_debug ("Could not load is_selinux_enabled from libselinux.so.0");
      return FALSE;
    } else {
      g_debug ("Loaded is_selinux_enabled from libselinux.so.0");
    }

    g_module_make_resident (selinux);

    g_once_init_leave_pointer (&module, g_steal_pointer (&selinux));
  }

  return module != NULL;
}


gboolean
gsm_selinux_is_enabled (void)
{
  if (!load_selinux ()) {
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

  if (!load_selinux ()) {
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
