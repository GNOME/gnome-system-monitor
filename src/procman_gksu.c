#include <config.h>

#include "procman.h"
#include "procman_gksu.h"


static gboolean (*gksu_run)(const char *, GError **);


static void load_gksu(void)
{
	static gboolean init;
	GModule *gksu;

	if (init)
		return;

	init = TRUE;

	gksu = g_module_open("libgksu2.so",
			     G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

	if (gksu) {
		if (g_module_symbol(gksu, "gksu_run", &gksu_run)) {

			g_module_make_resident(gksu);
			g_print("Found libgksu2\n");
		} else {
			g_module_close(gksu);
		}
	}
}





gboolean procman_gksu_create_root_password_dialog(const char *command)
{
	GError *e = NULL;

	/* Returns FALSE or TRUE on success, depends on version ... */
	gksu_run(command, &e);

	if (e) {
		g_error("Could not run gksu_run(\"%s\") : %s\n",
			command, e->message);
		g_error_free(e);
		return FALSE;
	}

	g_message("gksu_run did fine\n");
	return TRUE;
}



gboolean
procman_has_gksu(void)
{
	load_gksu();
	return gksu_run != NULL;
}

