/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include "gsm_gksu.h"


static gboolean (*gksu_run) (const char *, GError **);


static inline GModule *
load_gksu_module (void)
{
  g_autoptr (GError) error = NULL;
  GModule *module =
    g_module_open_full ("libgksu2.so",
                        G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL,
                        &error);

  if (error) {
    g_debug ("Could not load libgksu2.so: %s", error->message);
    goto fail;
  }

  if (!module) {
    g_debug ("Could not load libgksu2.so");
    goto fail;
  }

  if (!g_module_symbol (module, "gksu_run", (gpointer *) &gksu_run)) {
    g_debug ("Could not load gksu_run from libgksu2.so");
    goto fail;
  }

  g_debug ("Loaded gksu_run from libgksu2.so");

  return g_steal_pointer (&module);

fail:
  g_clear_pointer (&module, g_module_close);

  return NULL;
}


static inline GModule *
get_gksu_module (void)
{
  static GModule *module = NULL;

  if (g_once_init_enter_pointer (&module)) {
    GModule *gksu = load_gksu_module ();

    g_once_init_leave_pointer (&module, g_steal_pointer (&gksu));
  }

  return module;
}


gboolean
gsm_gksu_create_root_password_dialog (const char *command)
{
  g_autoptr (GError) e = NULL;

  g_return_val_if_fail (get_gksu_module () != NULL, FALSE);

  /* Returns FALSE or TRUE on success, depends on version ... */
  gksu_run (command, &e);

  if (e) {
    g_critical ("Could not run gksu_run(\"%s\") : %s\n",
                command,
                e->message);
    return FALSE;
  }

  g_message ("gksu_run did fine");

  return TRUE;
}


gboolean
procman_has_gksu (void)
{
  return get_gksu_module () != NULL;
}
