/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include "gsm_gnomesu.h"


gboolean (*gnomesu_exec) (const char *commandline);


static inline GModule *
load_gnomesu_module (void)
{
  g_autoptr (GError) error = NULL;
  GModule *module =
    g_module_open_full ("libgnomesu.so.0",
                        G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL,
                        &error);

  if (error) {
    g_debug ("Could not load libgnomesu.so.0: %s", error->message);
    goto fail;
  }

  if (!module) {
    g_debug ("Could not load libgnomesu.so.0");
    goto fail;
  }

  if (!g_module_symbol (module,
                        "gnomesu_exec",
                        (gpointer *) &gnomesu_exec)) {
    g_debug ("Could not load gnomesu_exec from libgnomesu.so.0");
    goto fail;
  }

  g_debug ("Loaded gnomesu_exec from libgnomesu.so.0");

  return g_steal_pointer (&module);

fail:
  g_clear_pointer (&module, g_module_close);

  return NULL;
}


static inline GModule *
get_gnomesu_module (void)
{
  static GModule *module = NULL;

  if (g_once_init_enter_pointer (&module)) {
    GModule *gksu = load_gnomesu_module ();

    g_once_init_leave_pointer (&module, g_steal_pointer (&gksu));
  }

  return module;
}


gboolean
gsm_gnomesu_create_root_password_dialog (const char *command)
{
  g_return_val_if_fail (get_gnomesu_module () != NULL, FALSE);

  return gnomesu_exec (command);
}


gboolean
procman_has_gnomesu (void)
{
  return get_gnomesu_module () != NULL;
}
