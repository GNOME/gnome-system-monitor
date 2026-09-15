/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>

#include "gsm-selinux.h"


static int (*getpidcon) (pid_t, char **);
static void (*freecon) (char *);
static int (*is_selinux_enabled) (void);


static inline GModule *
load_selinux_module (void)
{
  g_autoptr (GError) error = NULL;
  GModule *module =
    g_module_open_full ("libselinux.so.1",
                        G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL,
                        &error);

  if (error) {
    g_debug ("Could not load libselinux.so.1: %s", error->message);
    goto fail;
  }

  if (!module) {
    g_debug ("Could not load libselinux.so.1");
    goto fail;
  }

  if (!g_module_symbol (module, "getpidcon", (gpointer *) &getpidcon)) {
    g_debug ("Could not load getpidcon from libselinux.so.1");
    goto fail;
  } else {
    g_debug ("Loaded getpidcon from libselinux.so.1");
  }

  if (!g_module_symbol (module, "freecon", (gpointer *) &freecon)) {
    g_debug ("Could not load freecon from libselinux.so.1");
    goto fail;
  } else {
    g_debug ("Loaded freecon from libselinux.so.1");
  }
  
  if (!g_module_symbol (module,
                        "is_selinux_enabled",
                        (gpointer *) &is_selinux_enabled)) {
    g_debug ("Could not load is_selinux_enabled from libselinux.so.1");
    goto fail;
  } else {
    g_debug ("Loaded is_selinux_enabled from libselinux.so.1");
  }

  g_module_make_resident (module);

  return g_steal_pointer (&module);

fail:
  g_clear_pointer (&module, g_module_close);

  return NULL;
}


static inline GModule *
get_selinux_module (void)
{
  static GModule *module = NULL;

  if (g_once_init_enter_pointer (&module)) {
    GModule *gksu = load_selinux_module ();

    g_once_init_leave_pointer (&module, g_steal_pointer (&gksu));
  }

  return module;
}


gboolean
gsm_selinux_is_enabled (void)
{
  if (get_selinux_module () == NULL) {
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

  if (get_selinux_module () == NULL) {
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
