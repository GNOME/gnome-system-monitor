#include <config.h>
#include <libgnome/gnome-program.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "prettytable.h"
#include "defaulttable.h"
#include "proctable.h"


#define APP_ICON_SIZE 16


struct _PrettyTable {
	GHashTable	*app_hash;	/* WNCK */
	GHashTable	*default_hash;	/* defined in defaulttable.h */
	gchar		*datadir;
	GtkIconTheme	*theme;
};



static void
load_default_table(PrettyTable *pretty_table,
		   const PrettyTableItem table[], size_t n);



static void
new_application (WnckScreen *screen, WnckApplication *app, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	GHashTable * const hash = procdata->pretty_table->app_hash;
	guint pid;
	GdkPixbuf *icon = NULL, *tmp = NULL;
	GList *list = NULL;
	WnckWindow *window;

	pid = wnck_application_get_pid (app);
	if (pid == 0)
		return;

	/* Check to see if the pid has already been added */
	if (g_hash_table_lookup (hash, GUINT_TO_POINTER(pid)))
		return;

	/* don't free list, we don't own it */
	list = wnck_application_get_windows (app);
	if (!list)
		return;
	window = list->data;
	tmp = wnck_window_get_icon (window);

	if (!tmp)
		return;

	icon =  gdk_pixbuf_scale_simple (tmp, APP_ICON_SIZE, APP_ICON_SIZE, GDK_INTERP_HYPER);
	if (!icon)
		return;

	/* If process already exists then set the icon. Otherwise put into hash
	** table to be added later */
	info = proctable_find_process (pid, procdata);

	if (info) {
	/* Don't update the icon if there already is one */
		if (info->pixbuf) {
			g_object_unref(icon);
			return;
		}
		info->pixbuf = icon;
		g_object_ref(info->pixbuf);
		if (info->is_visible) {
			GtkTreeModel *model;
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
			gtk_tree_store_set (GTK_TREE_STORE (model), &info->node,
					    COL_PIXBUF, info->pixbuf, -1);
		}
	}

	g_assert(g_hash_table_lookup(hash, GUINT_TO_POINTER(pid)) == NULL);
	g_hash_table_insert (hash, GUINT_TO_POINTER(pid), icon);
}


static void
application_finished (WnckScreen *screen, WnckApplication *app, gpointer data)
{
	ProcData * const procdata = data;
	GHashTable * const hash = procdata->pretty_table->app_hash;
	guint pid;
	gpointer orig_pid, icon;

	pid =  wnck_application_get_pid (app);
	if (pid == 0)
		return;

	if (g_hash_table_lookup_extended (hash, GUINT_TO_POINTER(pid),
					  &orig_pid, &icon)) {
		g_hash_table_remove (hash, orig_pid);
		if (icon)
			g_object_unref (icon);
	}
}


void pretty_table_new (ProcData *procdata)
{
	WnckScreen *screen;
	PrettyTable *pretty_table = g_new(PrettyTable, 1);

	pretty_table->app_hash = g_hash_table_new (NULL, NULL);
	pretty_table->default_hash = g_hash_table_new_full (g_str_hash,
							    g_str_equal,
							    g_free,
							    g_object_unref);

	screen = wnck_screen_get_default ();

	g_signal_connect (G_OBJECT (screen), "application_opened",
			  G_CALLBACK (new_application), procdata);
	g_signal_connect (G_OBJECT (screen), "application_closed",
			  G_CALLBACK (application_finished), procdata);


	pretty_table->datadir = gnome_program_locate_file (
		NULL, GNOME_FILE_DOMAIN_DATADIR, "pixmaps/", TRUE, NULL);

	load_default_table(pretty_table, default_table, G_N_ELEMENTS(default_table));

	procdata->pretty_table = pretty_table;

	pretty_table->theme = gtk_icon_theme_get_default ();
}





static GdkPixbuf *
get_icon_from_theme (PrettyTable *pretty_table,
		     guint pid,
		     const gchar *command)
{
	return gtk_icon_theme_load_icon (pretty_table->theme,
					 command,
					 APP_ICON_SIZE,
					 0,
					 NULL);
}



static GdkPixbuf *
get_icon_from_default (PrettyTable *pretty_table,
		       guint pid,
		       const gchar *command)
{
	GdkPixbuf *icon;

	icon = g_hash_table_lookup (pretty_table->default_hash,
				    command);

	if (icon) {
		g_object_ref (icon);
		return icon;
	}

	return NULL;
}



static GdkPixbuf *
get_icon_from_wnck (PrettyTable *pretty_table,
		    guint pid,
		    const gchar *command)
{
	GdkPixbuf *icon;

	icon = g_hash_table_lookup (pretty_table->app_hash,
				    GUINT_TO_POINTER(pid));

	if (icon) {
		g_object_ref (icon);
		return icon;
	}

	return NULL;
}



GdkPixbuf *pretty_table_get_icon (PrettyTable *pretty_table,
				  const gchar *command, guint pid)
{
	typedef GdkPixbuf * (*IconGetter) (PrettyTable *pretty_table,
					   guint pid,
					   const gchar *command);


	const IconGetter getters[] = {
		get_icon_from_theme,
		get_icon_from_wnck,
		get_icon_from_default
	};


	size_t i;

	for (i = 0; i < G_N_ELEMENTS (getters); ++i) {
		GdkPixbuf *icon;

		icon = (getters[i]) (pretty_table, pid, command);

		if (icon) {
#if 0
			g_print("Get icon method %lu for '%s'\n",
				(gulong)i,
				command);
#endif
			return icon;
		}
	}

	return NULL;
}



void pretty_table_free (PrettyTable *pretty_table) {
	g_hash_table_destroy (pretty_table->default_hash);
	g_hash_table_destroy (pretty_table->app_hash);
	g_free (pretty_table->datadir);
	g_free (pretty_table);
}







static GdkPixbuf *
create_scaled_icon(const char *iconpath)
{
	GError *error = NULL;
	GdkPixbuf *scaled;

	scaled = gdk_pixbuf_new_from_file_at_scale(iconpath,
						   APP_ICON_SIZE, APP_ICON_SIZE,
						   TRUE,
						   &error);

	if(error) {
	  if(!(error->domain == G_FILE_ERROR
	       && error->code == G_FILE_ERROR_NOENT))
		    g_warning(error->message);

	  g_error_free(error);
	  return NULL;
	}

	return scaled;
}



static void
load_icon_for_commands(gpointer key, gpointer value, gpointer userdata)
{
	char *icon = key;
	GPtrArray *commands = value;
	PrettyTable * pretty_table = userdata;

	GdkPixbuf *scaled;
	char *iconpath;

	iconpath = g_build_filename(pretty_table->datadir, icon, NULL);
	scaled = create_scaled_icon(iconpath);
	g_free(iconpath);

	if(scaled) {
		guint i;

		for(i = 0; i < commands->len; ++i)
		{
			g_object_ref(scaled);
			g_hash_table_insert(pretty_table->default_hash,
					    g_strdup(g_ptr_array_index(commands, i)),
					    scaled);
		}

		g_object_unref(scaled);
	}
}



static void
cb_g_free(gpointer value, gpointer userdata)
{
	g_free(value);
}

static void
cb_g_ptr_array_free(gpointer value)
{
	g_ptr_array_foreach(value, cb_g_free, NULL);
	g_ptr_array_free(value, TRUE);
}


static void
load_default_table(PrettyTable *pretty_table,
		   const PrettyTableItem table[], size_t n)
{
	size_t i;
	GHashTable *multimap;

	/*
	 * regroup commands using the same icon
	 * icon -> [command1, ..., commandN]
	 */
	multimap = g_hash_table_new_full(g_str_hash, g_str_equal,
					 g_free, cb_g_ptr_array_free);

	for(i = 0; i < n; ++i)
	{
		const char *command = table[i].command;
		const char *icon    = table[i].icon;

		GPtrArray *commands;

		commands = g_hash_table_lookup(multimap, icon);

		if(!commands)
		{
			commands = g_ptr_array_new();
			g_hash_table_insert(multimap, g_strdup(icon), commands);
		}

		g_ptr_array_add(commands, g_strdup(command));
	}

	/*
	 * load once the icon
	 * then and add all command -> icon in pretty_table->default_table
	 */
	g_hash_table_foreach(multimap, load_icon_for_commands, pretty_table);

	g_hash_table_destroy(multimap);
}
