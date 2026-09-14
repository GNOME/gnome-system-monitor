/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include "gsm_gnomesu.h"


gboolean (*gnomesu_exec) (const char *commandline);


static inline gboolean
load_gnomesu (void)
{
  static GModule *module = NULL;

  if (g_once_init_enter_pointer (&module)) {
    GModule *gnomesu =
      g_module_open ("libgnomesu.so.0", G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

    if (!gnomesu) {
      g_debug ("Could not load libgnomesu.so.0");
      return FALSE;
    }

    if (!g_module_symbol (gnomesu, "gnomesu_exec", (gpointer *) &gnomesu_exec)) {
      g_debug ("Could not load gnomesu_exec from libgnomesu.so.0");
      return FALSE;
    } else {
      g_debug ("Loaded gnomesu_exec from libgnomesu.so.0");
    }

    g_once_init_leave_pointer (&module, g_steal_pointer (&gnomesu));
  }

  return module != NULL;
}


gboolean
gsm_gnomesu_create_root_password_dialog (const char *command)
{
  return gnomesu_exec (command);
}


gboolean
procman_has_gnomesu (void)
{
  return load_gnomesu ();
}
