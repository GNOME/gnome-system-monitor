#include <config.h>
#include <libgnome/libgnome.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "prettytable.h"
#include "defaulttable.h"
#include "proctable.h"


static void
new_application (WnckScreen *screen, WnckApplication *app, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	GHashTable * const hash = procdata->pretty_table->app_hash;
	gint pid;
	GdkPixbuf *icon = NULL, *tmp = NULL;
	GList *list = NULL;
	WnckWindow *window;

	pid = wnck_application_get_pid (app);
	if (pid == 0)
		return;

	/* Check to see if the pid has already been added */
	if (g_hash_table_lookup (hash, GINT_TO_POINTER(pid)))
		return;

/* Hack - wnck_application_get_icon always returns the default icon */
#if 0
	tmp = wnck_application_get_mini_icon (app);
#else
	list = wnck_application_get_windows (app);
	if (!list)
		return;
	window = list->data;
	tmp = wnck_window_get_icon (window);
#endif
	if (!tmp)
		return;

	icon =  gdk_pixbuf_scale_simple (tmp, 16, 16, GDK_INTERP_HYPER);
	if (!icon)
		return;

	/* If process already exists then set the icon. Otherwise put into hash
	** table to be added later */
	info = proctable_find_process (pid, NULL, procdata);

	if (info) {
	/* Don't update the icon if there already is one */
		if (info->pixbuf)
			return;
		info->pixbuf = icon;
		if (info->visible) {
			GtkTreeModel *model;
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
			gtk_tree_store_set (GTK_TREE_STORE (model), &info->node,
					    COL_PIXBUF, info->pixbuf, -1);
		}
	}

	g_hash_table_insert (hash, GINT_TO_POINTER(pid), icon);
}


static void
application_finished (WnckScreen *screen, WnckApplication *app, gpointer data)
{
	ProcData * const procdata = data;
	GHashTable * const hash = procdata->pretty_table->app_hash;
	gint pid;
	gpointer orig_pid, icon;

	pid =  wnck_application_get_pid (app);
	if (pid == 0)
		return;

	if (g_hash_table_lookup_extended (hash, GINT_TO_POINTER(pid),
					  &orig_pid, &icon)) {
		g_hash_table_remove (hash, orig_pid);
		if (icon)
			g_object_unref (G_OBJECT (icon));
	}
}


PrettyTable *pretty_table_new (ProcData *procdata)
{
	WnckScreen *screen;
	PrettyTable *pretty_table = g_new(PrettyTable, 1);

	pretty_table->app_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	pretty_table->default_hash = g_hash_table_new (g_str_hash, g_str_equal);

	screen = wnck_screen_get_default ();

	g_signal_connect (G_OBJECT (screen), "application_opened",
			  G_CALLBACK (new_application), procdata);
	g_signal_connect (G_OBJECT (screen), "application_closed",
			  G_CALLBACK (application_finished), procdata);

	pretty_table_add_table (pretty_table, default_table);

	return pretty_table;
}


void pretty_table_add_table (PrettyTable *pretty_table, const gchar * const table[])
{
	/* Table format:

	const gchar *table[] = {
		"X", "x.png",
		"bash", "bash.png",
		NULL};
	*/

	size_t i;
	gchar *datadir;

	datadir = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_DATADIR, "pixmaps/",
					     TRUE, NULL);

	for(i = 0; table[i] && table[i + 1]; i += 2) {
		const char * const process  = table[i];
		const char * const icon = table[i + 1];
		GdkPixbuf *tmp;
		gchar *prettyicon;

		prettyicon = g_build_filename (datadir, icon, NULL);
		tmp = gdk_pixbuf_new_from_file (prettyicon, NULL);
		g_free (prettyicon);

		if (tmp) {
			GdkPixbuf *icon = gdk_pixbuf_scale_simple (tmp, 16, 16,
								   GDK_INTERP_HYPER);
			g_object_unref (G_OBJECT (tmp));

			if (icon) {
	/* pretty_table_free frees all string in the tables */
				gchar *command = g_strdup (process);
				g_hash_table_insert (pretty_table->default_hash,
						     command, icon);
			}
		}
	}

	g_free(datadir);

}

GdkPixbuf *pretty_table_get_icon (PrettyTable *pretty_table,
				  const gchar *command, gint pid)
{
	GdkPixbuf *icon;

	if (!pretty_table)
		return NULL;


	icon = g_hash_table_lookup (pretty_table->default_hash, command);
	if (icon)
		return icon;

	icon = g_hash_table_lookup (pretty_table->app_hash, GINT_TO_POINTER(pid));

	return icon;
}

#if 0
static void free_entry (gpointer key, gpointer value, gpointer data) {
	g_free (key);
	g_free (value);
}

static void free_value (gpointer key, gpointer value, gpointer data) {
	g_free (value);
}

static void free_key (gpointer key, gpointer value, gpointer data) {
	g_free (key);
}
#endif


void pretty_table_free (PrettyTable *pretty_table) {
	if (!pretty_table)
		return;
	return;
#if 0
	g_hash_table_foreach (pretty_table->cmdline_to_prettyname, free_entry, NULL);
	g_hash_table_destroy (pretty_table->cmdline_to_prettyname);
	g_hash_table_foreach (pretty_table->cmdline_to_prettyicon, free_value, NULL);
	g_hash_table_destroy (pretty_table->cmdline_to_prettyicon);
	g_hash_table_foreach (pretty_table->name_to_prettyicon, free_key, NULL);
	g_hash_table_destroy (pretty_table->name_to_prettyicon);
	g_hash_table_destroy (pretty_table->name_to_prettyname);
#endif
	g_free (pretty_table);
}

#if 0
static gchar** load_table_from_file(const gchar *path)
{
	GPtrArray *strings = g_ptr_array_new();
	FILE *f;
	char line[1024]; /* long enough */

	if(!(f = fopen(path, "r"))) goto out;

	while(fgets(line, sizeof line, f))
	{
		char ** const tokens = g_strsplit(line, "=", 2);

		if(tokens && tokens[0] && tokens[1])
		{
			char *app, *icon;

			app  = g_strdup( g_strstrip(tokens[0]) );
			icon = g_strdup( g_strstrip(tokens[1]) );

			g_ptr_array_add(strings, app);
			g_ptr_array_add(strings, icon);
		}

		g_strfreev(tokens);
	}

	fclose(f);

 out:
	return (gchar**) g_ptr_array_free(strings, FALSE);
}
#endif

