#include <libgnome/libgnome.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <config.h>
#include <pthread.h>
#include "prettytable.h"
#include "defaulttable.h"
#include "proctable.h"

void free_entry (gpointer key, gpointer value, gpointer data);
void free_value (gpointer key, gpointer value, gpointer data);
void free_key (gpointer key, gpointer value, gpointer data);


static gboolean
compare_strings (gconstpointer a, gconstpointer b)
{
	const gchar *str1 = a;
	const gchar *str2 = b;
	
	return g_strcasecmp (str1, str2) == 0;
}

static void
thread_func (void *data)
{
	ProcData *procdata = (ProcData *)data;
	PrettyTable *table;
	
	table = pretty_table_new ();
	
	procdata->pretty_table = table;
	
	/*g_print ("clear \n");
	proctable_clear_tree (procdata);
	g_print ("update");
	proctable_update_all (procdata);*/
}

void
prettytable_load_async (ProcData *procdata)
{
	pthread_t thread;
	
	pthread_create (&thread, NULL, (void*)thread_func, (void*)procdata);
	
}

PrettyTable *pretty_table_new (void) {
	PrettyTable *pretty_table = NULL;
	gchar *path;
	
	
	pretty_table = g_malloc (sizeof (PrettyTable));

	pretty_table->cmdline_to_prettyname = g_hash_table_new (g_str_hash, compare_strings);
	pretty_table->cmdline_to_prettyicon = g_hash_table_new (g_str_hash, compare_strings);
	pretty_table->name_to_prettyicon = g_hash_table_new (g_str_hash, compare_strings);
	pretty_table->name_to_prettyname = g_hash_table_new (g_str_hash, compare_strings);
	
	path = gnome_datadir_file ("gnome/apps");
	pretty_table_load_path (pretty_table, path, TRUE);
	g_free (path);
	path = gnome_datadir_file ("gnome/ximian");
	pretty_table_load_path (pretty_table, path, TRUE);
	g_free (path);
	
	pretty_table_add_table (pretty_table, default_table);

	return pretty_table;
}

gint pretty_table_load_path (PrettyTable *pretty_table, gchar *path, gboolean recursive) { /* No ending slash in the path */
	DIR *dh;
	struct dirent *file;
	struct stat filestat;
	gchar *full_path, *tmp1, *tmp2, *tmp3, *tmp4;
	GnomeDesktopEntry *entry;

	g_return_val_if_fail (path, 0);
	g_return_val_if_fail (dh = opendir (path), 0);

	g_hash_table_freeze (pretty_table->cmdline_to_prettyname);
	g_hash_table_freeze (pretty_table->cmdline_to_prettyicon);
	g_hash_table_freeze (pretty_table->name_to_prettyicon);

	while ((file = readdir (dh))) {
		if (file->d_name[0] != '.') { /* This does not only filter out the "."- and ".."-directories , but also all hidden files (It is probably a good idea). */
			full_path = g_malloc (strlen (file->d_name) + strlen (path) + 2);
			sprintf (full_path, "%s/%s", path, file->d_name);
			lstat (full_path, &filestat);
			if (S_ISDIR(filestat.st_mode)) {
				if (recursive)
					pretty_table_load_path (pretty_table, full_path, 1);
			}
			entry = gnome_desktop_entry_load (full_path);
			if (entry && entry->exec) {
				tmp1 = g_strdup (entry->exec[0]);
				if (strlen (tmp1) > 15) /* libgtop seems to report only 15 characters of the command */
					tmp1[15] = '\0';

				tmp2 = g_strdup (entry->name);
				tmp3 = g_strdup (entry->icon);
				g_hash_table_insert (pretty_table->cmdline_to_prettyname, tmp1, tmp2);
				g_hash_table_insert (pretty_table->cmdline_to_prettyicon, tmp1, tmp3);
				tmp4 = g_strdup (tmp2);
				g_strdown (tmp4);
				g_hash_table_insert (pretty_table->name_to_prettyicon, tmp4, tmp3);
				g_hash_table_insert (pretty_table->name_to_prettyname, tmp4, tmp2);
				gnome_desktop_entry_free (entry);
			}

			g_free (full_path);
		}
	}

	g_hash_table_thaw (pretty_table->cmdline_to_prettyname);
	g_hash_table_thaw (pretty_table->cmdline_to_prettyicon);
	g_hash_table_thaw (pretty_table->name_to_prettyicon);
	
	closedir (dh);

	return 1;
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
	
	g_hash_table_freeze (pretty_table->cmdline_to_prettyname);

	while (table[i] && table[i + 1] && table[i + 2]) {
		command = g_strdup (table[i]); /* pretty_table_free frees all string in the tables */
		prettyname = g_strdup (table[i + 1]); /* pretty_table_free frees all string in the tables */
		prettyicon = g_malloc (strlen (table[i + 2]) + strlen (gnome_datadir_file ("pixmaps/")) + 1);
		sprintf (prettyicon, "%s%s", gnome_datadir_file ("pixmaps/"), table[i + 2]);
		g_hash_table_insert (pretty_table->cmdline_to_prettyname, command, prettyname);
		g_hash_table_insert (pretty_table->cmdline_to_prettyicon, command, prettyicon);

		i += 3;
	}

	g_hash_table_thaw (pretty_table->cmdline_to_prettyname);

	return;
}

gchar *pretty_table_get_name (PrettyTable *pretty_table, const gchar *command) {
	gchar *pretty_name;

	if (!pretty_table)
		return g_strdup (command);
	pretty_name = g_hash_table_lookup (pretty_table->cmdline_to_prettyname, command);
	if (pretty_name)
		return g_strdup (pretty_name);

	pretty_name = g_hash_table_lookup (pretty_table->name_to_prettyname, command);
	if (pretty_name)
		return g_strdup (pretty_name);

	return g_strdup (command);
}

GdkPixbuf *pretty_table_get_icon (PrettyTable *pretty_table, gchar *command) {
	GdkPixbuf *icon = NULL, *tmp_pixbuf = NULL;
	gchar *icon_path = NULL;
	
	if (!pretty_table)
		return NULL;

	icon_path = g_hash_table_lookup (pretty_table->cmdline_to_prettyicon, command);
	if (!icon_path) {
		gchar *tmp1;
		
		tmp1 = g_strdup (command);
		g_strdown (tmp1);
		
		icon_path = g_hash_table_lookup (pretty_table->name_to_prettyicon, tmp1);
		
		g_free (tmp1);
	}

	if (!icon_path)
		return NULL;

	tmp_pixbuf = gdk_pixbuf_new_from_file (icon_path);
	if (!tmp_pixbuf)
		return NULL;

	icon = gdk_pixbuf_scale_simple (tmp_pixbuf, 16, 16, GDK_INTERP_NEAREST);
	gdk_pixbuf_unref (tmp_pixbuf);

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

	g_hash_table_foreach (pretty_table->cmdline_to_prettyname, free_entry, NULL);
	g_hash_table_destroy (pretty_table->cmdline_to_prettyname);
	g_hash_table_foreach (pretty_table->cmdline_to_prettyicon, free_value, NULL);
	g_hash_table_destroy (pretty_table->cmdline_to_prettyicon);
	g_hash_table_foreach (pretty_table->name_to_prettyicon, free_key, NULL);
	g_hash_table_destroy (pretty_table->name_to_prettyicon);
	g_hash_table_destroy (pretty_table->name_to_prettyname);

	g_free (pretty_table);
}


