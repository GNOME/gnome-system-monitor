#include <config.h>
#include <libgnome/libgnome.h>
#ifdef USE_WNCK
  #define WNCK_I_KNOW_THIS_IS_UNSTABLE 
  #include <libwnck/libwnck.h>
#endif
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
#ifdef USE_WNCK
  WnckScreen *screen = NULL;
#endif

static gboolean
compare_strings (gconstpointer a, gconstpointer b)
{
	const gchar *str1 = a;
	const gchar *str2 = b;
	
	return g_strcasecmp (str1, str2) == 0;
}

void *
prettytable_load_async (void *data)
{
	ProcData *procdata = (ProcData *)data;
	PrettyTable *table;
	
	table = pretty_table_new (procdata);
	
	procdata->desktop_load_finished = TRUE;
	
	return (void *)table;
	
}

#ifdef USE_WNCK
static void
new_application (WnckScreen *screen, WnckApplication *app, gpointer data)
{
	ProcData *procdata = data;
	GHashTable *hash = procdata->pretty_table->cmdline_to_prettyicon;
	gint pid;
	GdkPixbuf *icon = NULL;
	GList *windows = NULL;
	WnckWindow *window;
	
	windows =  wnck_application_get_windows (app);
	g_return_if_fail (windows);
	window = windows->data;
	
	g_print ("new app \n");
	pid = wnck_window_get_pid (window);
	g_print ("%d \n", pid);
	if (pid == 0)
		return;
		
	icon = wnck_window_get_icon (window);
	
	if (icon) {
		g_hash_table_insert (hash, g_strdup_printf ("%d", pid), icon);
	}
	/* Ugh this is ugly */
	procdata->desktop_load_finished = TRUE;
}	

static void
application_finished (WnckScreen *screen, WnckApplication *app, gpointer data)
{
	ProcData *procdata = data;
	GHashTable *hash = procdata->pretty_table->cmdline_to_prettyicon;
	gint pid;
	GdkPixbuf *icon;
	GList *windows = NULL;
	WnckWindow *window;
	return;
	windows =  wnck_application_get_windows (app);
	window = windows->data;
	
	g_print ("new app closed\n");
	pid = wnck_window_get_pid (window);
	g_print ("%d \n", pid);
	if (pid == 0)
		return;
	
	g_hash_table_remove (hash, &pid);
	
}
#endif

PrettyTable *pretty_table_new (ProcData *procdata) {
	PrettyTable *pretty_table = NULL;
	gchar *path;
	
#ifdef USE_WNCK
	screen = wnck_screen_get (0);
#endif	

	pretty_table = g_malloc (sizeof (PrettyTable));
	
	pretty_table->cmdline_to_prettyicon = g_hash_table_new (g_str_hash, g_str_equal);
	
#ifdef USE_WNCK	
	g_signal_connect (G_OBJECT (screen), "application_opened",
			  G_CALLBACK (new_application), procdata);
	g_signal_connect (G_OBJECT (screen), "application_closed",
			  G_CALLBACK (application_finished), procdata);
#endif

	return pretty_table;
}

void pretty_table_add_table (PrettyTable *pretty_table, const gchar *table[]) {
	/* Table format:

	const gchar *table[] = {
			"X", "X Window System", "x.png",
			"bash" "Bourne Again Shell", "bash.png",
			NULL};
	*/
	gint i = 0;
	gchar *command, *prettyname, *prettyicon;
	gchar *text;
	
	g_hash_table_freeze (pretty_table->cmdline_to_prettyname);

	while (table[i] && table[i + 1] && table[i + 2]) {
		/* pretty_table_free frees all string in the tables */
		command = g_strdup (table[i]); 
		/* pretty_table_free frees all string in the tables */
		prettyname = g_strdup (gettext(table[i + 1]));
		text = gnome_datadir_file ("pixmaps/");
		if (text) {
			prettyicon = g_malloc (strlen (table[i + 2]) + strlen (text) + 1);
			sprintf (prettyicon, "%s%s", text, table[i + 2]);
			g_hash_table_insert (pretty_table->cmdline_to_prettyicon, 
					     command, prettyicon);
			g_free (text);
		}
		g_hash_table_insert (pretty_table->cmdline_to_prettyname, command, prettyname);
		
		i += 3;
	}

	g_hash_table_thaw (pretty_table->cmdline_to_prettyname);

	return;
}

gchar *pretty_table_get_name (PrettyTable *pretty_table, const gchar *command) {
	gchar *pretty_name;

	if (!pretty_table)
		return NULL;
		
	pretty_name = g_hash_table_lookup (pretty_table->cmdline_to_prettyname, command);
	if (pretty_name) 
		return g_strdup (pretty_name); 

	pretty_name = g_hash_table_lookup (pretty_table->name_to_prettyname, command);
	if (pretty_name) 
		return g_strdup (pretty_name);

	return NULL;
}

GdkPixbuf *pretty_table_get_icon (PrettyTable *pretty_table, gchar *command, gint pid) 
{
	GdkPixbuf *icon = NULL, *tmp_pixbuf = NULL;
	gchar *icon_path = NULL;
	GError *error;
	gchar *text;
	gint test;
	
	if (!pretty_table) 
		return NULL;
#if 0
	icon_path = g_hash_table_lookup (pretty_table->cmdline_to_prettyicon, command);
	if (!icon_path) {
		gchar *tmp1;
		
		tmp1 = g_strdup (command);
		g_strdown (tmp1);
		
		icon_path = g_hash_table_lookup (pretty_table->name_to_prettyicon, tmp1);
		
		g_free (tmp1);
	}

	if (!icon_path) {
		GList *windows = NULL;
		if (!screen)
			g_print ("no screen yet \n");
		windows = wnck_screen_get_windows (screen);
		
		if (!windows)
			g_print ("No windows \n");
		while (windows) {
			int wpid = 0;
			WnckWindow *window = windows->data;
			
			wpid = wnck_window_get_pid (window);
			g_print ("window PID %d \n", wpid);
			
			icon_path = "hello";	
			if (wpid == pid) {
				tmp_pixbuf = wnck_window_get_icon (window);
			}
			windows = g_list_next (windows);
		}		
	}
	
	if (!icon_path)
		return NULL;
#endif	

#ifdef USE_WNCK
	text = g_strdup_printf ("%d", pid);
	tmp_pixbuf = g_hash_table_lookup (pretty_table->cmdline_to_prettyicon, 
		     text);
	g_free (text);
	
	/*if (!tmp_pixbuf)
		tmp_pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);*/
	if (!tmp_pixbuf) 
		return NULL;
	
	icon = gdk_pixbuf_scale_simple (tmp_pixbuf, 16, 16, GDK_INTERP_HYPER);
	
	return icon;
#else
	return NULL;
#endif

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
	g_hash_table_foreach (pretty_table->cmdline_to_prettyname, free_entry, NULL);
	g_hash_table_destroy (pretty_table->cmdline_to_prettyname);
	g_hash_table_foreach (pretty_table->cmdline_to_prettyicon, free_value, NULL);
	g_hash_table_destroy (pretty_table->cmdline_to_prettyicon);
	g_hash_table_foreach (pretty_table->name_to_prettyicon, free_key, NULL);
	g_hash_table_destroy (pretty_table->name_to_prettyicon);
	g_hash_table_destroy (pretty_table->name_to_prettyname);

	g_free (pretty_table);
}


