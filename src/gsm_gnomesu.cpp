#include <config.h>

#include <glib.h>

#include "gsm_gnomesu.h"
#include "util.h"

gboolean (*gnomesu_exec) (const char *commandline);


static void
load_gnomesu (void)
{
  static gboolean init;

  if (init)
    return;

  init = TRUE;

  load_symbols ("libgnomesu.so.0",
                "gnomesu_exec", &gnomesu_exec,
                NULL);
}


gboolean
gsm_gnomesu_create_root_password_dialog (const char *command)
{
  return gnomesu_exec (command);
}


gboolean
procman_has_gnomesu (void)
{
  load_gnomesu ();
  return gnomesu_exec != NULL;
}
