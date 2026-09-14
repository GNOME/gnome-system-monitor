/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include "gsm_gksu.h"


static gboolean (*gksu_run) (const char *, GError **);


static inline gboolean
load_gksu (void)
{
  static GModule *module = NULL;

  if (g_once_init_enter_pointer (&module)) {
    GModule *gksu =
      g_module_open ("libgksu2.so", G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

    if (!gksu) {
      g_debug ("Could not load libgksu2.so");
      return FALSE;
    }

    if (!g_module_symbol (gksu, "gksu_run", (gpointer *) &gksu_run)) {
      g_debug ("Could not load gksu_run from libgksu2.so");
      return FALSE;
    } else {
      g_debug ("Loaded gksu_run from libgksu2.so");
    }

    g_once_init_leave_pointer (&module, g_steal_pointer (&gksu));
  }

  return module != NULL;
}


gboolean
gsm_gksu_create_root_password_dialog (const char *command)
{
  g_autoptr (GError) e = NULL;

  /* Returns FALSE or TRUE on success, depends on version ... */
  gksu_run (command, &e);

  if (e) {
    g_critical ("Could not run gksu_run(\"%s\") : %s\n",
                command,
                e->message);
    return FALSE;
  }

  g_message ("gksu_run did fine\n");
  return TRUE;
}


gboolean
procman_has_gksu (void)
{
  return load_gksu ();
}
