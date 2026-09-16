/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include "gsm-module-loader.h"

#include "gsm_gnomesu.h"


gboolean (*gnomesu_exec) (const char *commandline);


GSM_DEFINE_MODULE_LOADER (gsm, gnomesu, "libgnomesu.so.0")


static inline GsmModuleInitResult
gsm_gnomesu_init_module (GModule *module)
{
  GSM_MODULE_REQUIRE_SYMBOL (module, "libgnomesu.so.0", gnomesu_exec);

  return GSM_MODULE_INIT_SUCCESS;
}


gboolean
gsm_gnomesu_create_root_password_dialog (const char *command)
{
  g_return_val_if_fail (gsm_gnomesu_get_or_load_module (), FALSE);

  return gnomesu_exec (command);
}


gboolean
procman_has_gnomesu (void)
{
  return gsm_gnomesu_get_or_load_module ();
}
