/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include "gsm-module-loader.h"

#include "gsm_gksu.h"


static gboolean (*gksu_run) (const char *, GError **);


GSM_DEFINE_MODULE_LOADER (gsm, gksu, "libgksu2.so")


static inline GsmModuleInitResult
gsm_gksu_init_module (GModule *module)
{
  GSM_MODULE_REQUIRE_SYMBOL (module, "libgksu2.so", gksu_run);

  return GSM_MODULE_INIT_SUCCESS;
}


gboolean
gsm_gksu_create_root_password_dialog (const char *command)
{
  g_autoptr (GError) e = NULL;

  g_return_val_if_fail (gsm_gksu_get_or_load_module (), FALSE);

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
  return gsm_gksu_get_or_load_module ();
}
