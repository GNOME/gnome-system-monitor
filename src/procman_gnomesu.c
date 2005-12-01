#include <config.h>

#include <glib.h>

#include "procman.h"
#include "procman_gnomesu.h"


typedef gboolean (*GnomesuExecFunc)(char *commandline);


static GnomesuExecFunc G_GNUC_CONST
procman_gnomesu_get_exec(void)
{
	static gboolean is_init = FALSE;
	static gpointer gnomesu_exec = NULL;

	if (!is_init) {
		GModule *libgnomesu;

		libgnomesu = g_module_open("libgnomesu.so.0",
					   G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

		if (libgnomesu) {
			if (g_module_symbol(libgnomesu, "gnomesu_exec", &gnomesu_exec))
				g_module_make_resident(libgnomesu);
			else {
				g_warning("Cannot found gnomesu_exec : %s", g_module_error());
				g_module_close(libgnomesu);
			}
		}

		is_init = TRUE;
	}

	return (GnomesuExecFunc)gnomesu_exec;
}



gboolean
procman_gnomesu_create_root_password_dialog(char * command)
{
	GnomesuExecFunc gnomesu_exec;

	gnomesu_exec = procman_gnomesu_get_exec();

	if (gnomesu_exec)
		return gnomesu_exec(command);

	return FALSE;
}

