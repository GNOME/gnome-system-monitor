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

void free_entry (gpointer key, gpointer value, gpointer data);
void free_value (gpointer key, gpointer value, gpointer data);
void free_key (gpointer key, gpointer value, gpointer data);

WnckScreen *screen = NULL;


static void
new_application (WnckScreen *screen, WnckApplication *app, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	GHashTable *hash = procdata->pretty_table->app_hash;
	gint pid;
	GdkPixbuf *icon = NULL, *tmp = NULL;
	GList *list = NULL;
	WnckWindow *window;
	gchar *text;
	
	pid = wnck_application_get_pid (app);
	if (pid == 0)
		return;
	
	/* Check to see if the pid has already been added */	
	text = g_strdup_printf ("%d", pid);
	if (g_hash_table_lookup (hash, text)) {
		g_free (text);
		return;
	}
	g_free (text);
		
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
						
	g_hash_table_insert (hash, g_strdup_printf ("%d", pid), icon);
		
	
}	

static void
application_finished (WnckScreen *screen, WnckApplication *app, gpointer data)
{

	ProcData *procdata = data;
	GHashTable *hash = procdata->pretty_table->app_hash;
	gint pid;
	gpointer p1, p2;
	GdkPixbuf *icon;
	WnckWindow *window;
	gchar *id, *orig_id;

	pid =  wnck_application_get_pid (app);
	if (pid == 0)
		return;
	
	id = g_strdup_printf ("%d", pid);
	if (g_hash_table_lookup_extended (hash, id, &p1, &p2)) {
		orig_id = p1;
		icon = p2;
		g_hash_table_remove (hash, orig_id);
		if (orig_id) 
			g_free (orig_id);
		if (icon)
			g_object_unref (G_OBJECT (icon));
	}
	
	
	g_free (id);

	
}

PrettyTable *pretty_table_new (ProcData *procdata) 
{
	PrettyTable *pretty_table = NULL;
	GList *list = NULL;
	
	pretty_table = g_malloc (sizeof (PrettyTable));
	
	pretty_table->app_hash = g_hash_table_new (g_str_hash, g_str_equal);
	pretty_table->default_hash = g_hash_table_new (g_str_hash, g_str_equal);

	screen = wnck_screen_get_default ();
		
	g_signal_connect (G_OBJECT (screen), "application_opened",
			  G_CALLBACK (new_application), procdata);
	g_signal_connect (G_OBJECT (screen), "application_closed",
			  G_CALLBACK (application_finished), procdata);

	pretty_table_add_table (pretty_table, default_table);
	
	return pretty_table;
}

void pretty_table_add_table (PrettyTable *pretty_table, const gchar *table[]) 
{
	/* Table format:

	const gchar *table[] = {
			"X", "X Window System", "x.png",
			"bash" "Bourne Again Shell", "bash.png",
			NULL};
	*/
	gint i = 0;
	gchar *command, *prettyicon;
	gchar *text;
	
	while (table[i] && table[i + 1] && table[i + 2]) {
		/* pretty_table_free frees all string in the tables */
		command = g_strdup (table[i]); 
		/* pretty_table_free frees all string in the tables */
		text = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_DATADIR, "pixmaps/",
						  TRUE, NULL);
		if (text) {
			GdkPixbuf *tmp = NULL, *icon = NULL;
			prettyicon = g_malloc (strlen (table[i + 2]) + strlen (text) + 1);
			sprintf (prettyicon, "%s%s", text, table[i + 2]);
			if (prettyicon) {
				tmp = gdk_pixbuf_new_from_file (prettyicon, NULL);
				g_free (prettyicon);		
			}
			if (tmp) {
				icon =  gdk_pixbuf_scale_simple (tmp, 16, 16, 
										 GDK_INTERP_HYPER);
				g_object_unref (G_OBJECT (tmp));
			}
			if (icon) {
				g_hash_table_insert (pretty_table->default_hash, 
					                       command, icon);
			};
			g_free (text);
		}
		i += 3;
	}

	return;
}

GdkPixbuf *pretty_table_get_icon (PrettyTable *pretty_table, gchar *command, gint pid) 
{
	GdkPixbuf *icon = NULL, *tmp_pixbuf = NULL;
	gchar *icon_path = NULL;
	gchar *text;
	
	if (!pretty_table) 
		return NULL;


	icon = g_hash_table_lookup (pretty_table->default_hash, command);
	if (icon)
		return icon;
	
	text = g_strdup_printf ("%d", pid);
	icon = g_hash_table_lookup (pretty_table->app_hash, text);
	g_free (text);
	
	return icon;
	
}

void free_entry (gpointer key, gpointer value, gpointer data) {
	if (key)
		g_free (key);
	
	if (value)
		g_free (value);
}

void free_value (gpointer key, gpointer value, gpointer data) {
	if (value)
		g_free (value);
}

void free_key (gpointer key, gpointer value, gpointer data) {
	if (key)
		g_free (key);
}


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


