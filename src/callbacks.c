/* Procman - callbacks
 * Copyright (C) 2001 Kevin Vandersloot
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <config.h>

#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glibtop/mountlist.h>
#include <glibtop/fsusage.h>
#include <signal.h>
#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "util.h"
#include "infoview.h"
#include "procactions.h"
#include "procdialogs.h"
#include "memmaps.h"
#include "openfiles.h"
#include "favorites.h"
#include "load-graph.h"


static void
kill_process_helper(ProcData *procdata, int sig)
{
	if (procdata->config.show_kill_warning)
		procdialog_create_kill_dialog (procdata, sig);
	else
		kill_process (procdata, sig);
}


void
cb_preferences_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	ProcData * const procdata = user_data;

	procdialog_create_preferences_dialog (procdata);
}


void
cb_renice (GtkMenuItem *menuitem, gpointer data)
{
	ProcData * const procdata = data;

	procdialog_create_renice_dialog (procdata);
}


void
cb_end_process (GtkMenuItem *menuitem, gpointer data)
{
	kill_process_helper(data, SIGTERM);
}


void
cb_kill_process (GtkMenuItem *menuitem, gpointer data)
{
	kill_process_helper(data, SIGKILL);
}


void
cb_show_memory_maps (GtkMenuItem *menuitem, gpointer data)
{
	ProcData * const procdata = data;

	create_memmaps_dialog (procdata);
}

void
cb_show_open_files (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	create_openfiles_dialog (procdata);
}

void		
cb_show_hidden_processes (GtkMenuItem *menuitem, gpointer data)
{
	ProcData * const procdata = data;

	create_blacklist_dialog (procdata);
}


void
cb_hide_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData * const procdata = data;

	if (procdata->config.show_hide_message)
		procdialog_create_hide_dialog (procdata);
	else
		add_selected_to_blacklist (procdata);
}


void
cb_about_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	static const gchar *authors[] = {
		"Kevin Vandersloot",
		N_("Jorgen Scheibengruber - nicer devices treeview"),
		N_("Benoît Dejean - maintainer"),
		NULL
	};

	static const gchar *documenters[] = {
		"Bill Day",
		"Sun Microsystems",
		NULL
	};

	PROCMAN_GETTEXT_ARRAY_INIT(authors);

	gtk_show_about_dialog (
		NULL,
		"name",			_("System Monitor"),
		"comments",		_("View current processes and monitor "
					  "system state"),
		"version",		VERSION,
		"copyright",		"Copyright \xc2\xa9 2001-2004 "
					"Kevin Vandersloot",
		"logo-icon-name",	"gnome-monitor",
		"authors",		authors,
		"documenters",		documenters,
		"translator-credits",	_("translator-credits"),
		NULL
		);
}


void
cb_app_exit (GtkObject *object, gpointer user_data)
{
	ProcData * const procdata = user_data;

	cb_app_delete (NULL, NULL, procdata);
}


gboolean
cb_app_delete (GtkWidget *window, GdkEventAny *event, gpointer data)
{
	ProcData * const procdata = data;

	procman_save_config (procdata);
	if (procdata->timeout != -1)
		gtk_timeout_remove (procdata->timeout);
	if (procdata->cpu_graph)
		gtk_timeout_remove (procdata->cpu_graph->timer_index);
	if (procdata->mem_graph)
		gtk_timeout_remove (procdata->mem_graph->timer_index);
	if (procdata->disk_timeout != -1)
		gtk_timeout_remove (procdata->disk_timeout);

	gtk_main_quit ();

	return TRUE;
}


void
cb_proc_combo_changed (GtkComboBox *combo, gpointer data)
{
	ProcData * const procdata = data;
	GConfClient *client;

	g_return_if_fail (procdata);

	client = procdata->client;

	procdata->config.whose_process = gtk_combo_box_get_active (combo);
	gconf_client_set_int (client, "/apps/procman/view_as",
			      procdata->config.whose_process, NULL);
}


void
cb_end_process_button_pressed (GtkButton *button, gpointer data)
{
	kill_process_helper(data, SIGTERM);
}


void
popup_menu_show_open_files (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	create_openfiles_dialog (procdata);
}


void
cb_search (GtkEditable *editable, gpointer data)
{
	ProcData * const procdata = data;
	gchar *text;

	text = gtk_editable_get_chars (editable, 0, -1);

	proctable_search_table (procdata, text);
	gtk_widget_grab_focus (GTK_WIDGET (editable));
	g_free (text);
}


static void change_gconf_color(GConfClient *client, const char *key,
			       GtkColorButton *cp)
{
	GdkColor c;
	char color[24]; /* color should be 1 + 3*4 + 1 = 15 chars -> 24 */

	gtk_color_button_get_color(cp, &c);
	g_snprintf(color, sizeof color, "#%04x%04x%04x", c.red, c.green, c.blue);
	gconf_client_set_string (client, key, color, NULL);
}


void
cb_cpu_color_changed (GtkColorButton *cp, gpointer data)
{
	char key[80];
	gint i = GPOINTER_TO_INT (data);
	GConfClient *client = gconf_client_get_default ();

	if (i == 0) {
		strcpy(key, "/apps/procman/cpu_color");
	}
	else {
		g_snprintf(key, sizeof key, "/apps/procman/cpu_color%d", i);
	}

	change_gconf_color(client, key, cp);
}

void
cb_mem_color_changed (GtkColorButton *cp, gpointer data)
{
	ProcData * const procdata = data;
	change_gconf_color(procdata->client, "/apps/procman/mem_color", cp);
}


void
cb_swap_color_changed (GtkColorButton *cp, gpointer data)
{
	ProcData * const procdata = data;
	change_gconf_color(procdata->client, "/apps/procman/swap_color", cp);
}


void
cb_bg_color_changed (GtkColorButton *cp, gpointer data)
{
	ProcData * const procdata = data;
	change_gconf_color(procdata->client, "/apps/procman/bg_color", cp);
}

void
cb_frame_color_changed (GtkColorButton *cp, gpointer data)
{
	ProcData * const procdata = data;
	change_gconf_color(procdata->client, "/apps/procman/frame_color", cp);
}


static void
get_last_selected (GtkTreeModel *model, GtkTreePath *path,
		   GtkTreeIter *iter, gpointer data)
{
	ProcInfo **info = data;

	gtk_tree_model_get (model, iter, COL_POINTER, info, -1);
}


void
cb_row_selected (GtkTreeSelection *selection, gpointer data)
{
	ProcData * const procdata = data;

	procdata->selection = selection;

	/* get the most recent selected process and determine if there are
	** no selected processes
	*/
	gtk_tree_selection_selected_foreach (procdata->selection, get_last_selected,
					     &procdata->selected_process);

	if (procdata->selected_process) {
		if (procdata->config.show_more_info)
			infoview_update (procdata);

		update_sensitivity (procdata, TRUE);
	}
	else {
		update_sensitivity (procdata, FALSE);
	}
}


gboolean
cb_tree_button_pressed (GtkWidget *widget,
			GdkEventButton *event,
			gpointer data)
{
	ProcData * const procdata = data;

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
		do_popup_menu (procdata, event);

	return FALSE;
}


gboolean
cb_tree_popup_menu (GtkWidget *widget, gpointer data)
{
	ProcData * const procdata = data;

	do_popup_menu (procdata, NULL);

	return TRUE;
}


void
cb_switch_page (GtkNotebook *nb, GtkNotebookPage *page,
		gint num, gpointer data)
{
	cb_change_current_page (nb, num, data);
}

void
cb_change_current_page (GtkNotebook *nb, gint num, gpointer data)
{
	ProcData * const procdata = data;

	procdata->config.current_tab = num;

	if (num == 0) {
		if (procdata->timeout == -1)
			procdata->timeout = gtk_timeout_add (
				procdata->config.update_interval,
				cb_timeout, procdata);

		if (procdata->selected_process)
			update_sensitivity (procdata, TRUE);
	}
	else {
		if (procdata->timeout != -1 ) {
			gtk_timeout_remove (procdata->timeout);
			procdata->timeout = -1;
		}

		if (procdata->selected_process)
			update_sensitivity (procdata, FALSE);
	}


	if (num == 1) {
		load_graph_start (procdata->cpu_graph);
		load_graph_start (procdata->mem_graph);
		load_graph_draw (procdata->cpu_graph);
		load_graph_draw (procdata->mem_graph);
	}
	else {
		load_graph_stop (procdata->cpu_graph);
		load_graph_stop (procdata->mem_graph);
	}
}


static GList *old_disks = NULL;


static void
fsusage_stats(const glibtop_fsusage *buf,
	      float *bused, float *bfree, float *btotal,
	      float *percentage)
{
	*btotal = buf->blocks * buf->block_size;
	*bfree  = buf->bfree  * buf->block_size;
	*bused  = *btotal - *bfree;
	*percentage = CLAMP(100.0f * *bused / *btotal, 0.0f, 100.0f);
}


static void
update_disk(GtkTreeModel *model, GtkTreeIter *iter, const char *mountdir)
{
	glibtop_fsusage usage;
	gchar *used_str, *total_str, *free_str;
	float percentage, bused, bfree, btotal;

	glibtop_get_fsusage (&usage, mountdir);

	fsusage_stats(&usage, &bused, &bfree, &btotal, &percentage);

	used_str  = gnome_vfs_format_file_size_for_display (bused);
	free_str  = gnome_vfs_format_file_size_for_display (bfree);
	total_str = gnome_vfs_format_file_size_for_display (btotal);

	gtk_tree_store_set (GTK_TREE_STORE (model), iter,
			    4, total_str,
			    5, free_str,
			    6, used_str,
			    7, percentage,
			    8, btotal,
			    9, bfree,
			    -1);

	g_free (used_str);
	g_free (free_str);
	g_free (total_str);
}


static gboolean
compare_disks (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	GHashTable * const new_disks = data;

	GtkTreeIter *old_iter;
	glibtop_mountentry *entry;
	gchar *old_name;

	gtk_tree_model_get (model, iter, 1, &old_name, -1);

	entry = g_hash_table_lookup (new_disks, old_name);
	if (entry) {
		update_disk(model, iter, entry->mountdir);
		g_hash_table_remove (new_disks, old_name);
	}
	else {
		old_iter = gtk_tree_iter_copy (iter);
		old_disks = g_list_prepend (old_disks, old_iter);
	}

	g_free (old_name);
	return FALSE;
}


static GdkPixbuf*
get_icon_for_device(const char *mountpoint)
{
	GdkPixbuf *pixbuf;
	GtkIconTheme *icon_theme;
	GnomeVFSVolumeMonitor* monitor;
	GnomeVFSVolume* volume;
	char *icon_name;

	monitor = gnome_vfs_get_volume_monitor();
	volume = gnome_vfs_volume_monitor_get_volume_for_path(monitor, mountpoint);

	if(!volume)
	{
		g_warning("Cannont get volume for mount point '%s'", mountpoint);
		return NULL;
	}

	icon_name = gnome_vfs_volume_get_icon(volume);
	icon_theme = gtk_icon_theme_get_default ();
	pixbuf = gtk_icon_theme_load_icon (icon_theme, icon_name, 24, 0, NULL);

	gnome_vfs_volume_unref(volume);
	g_free (icon_name);

	return pixbuf;
}


static void
add_new_disks (gpointer key, gpointer value, gpointer data)
{
	glibtop_mountentry * const entry = value;
	GtkTreeModel * const model = data;
	GdkPixbuf *pixbuf;
	GtkTreeIter row;

	/*  Load an icon corresponding to the type of the device */
	pixbuf = get_icon_for_device(entry->mountdir);


	gtk_tree_store_insert (GTK_TREE_STORE (model), &row, NULL, 0);
	gtk_tree_store_set (GTK_TREE_STORE (model), &row,
			    0, pixbuf,
			    1, entry->devname,
			    2, entry->mountdir,
			    3, entry->type,
			    -1);

	update_disk(model, &row, entry->mountdir);


	if(pixbuf) g_object_unref (pixbuf);
}


gint
cb_update_disks (gpointer data)
{
	ProcData * const procdata = data;

	GtkTreeModel *model;
	glibtop_mountentry *entry;
	glibtop_mountlist mountlist;
	GHashTable *new_disks = NULL;
	guint i;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->disk_list));

	entry = glibtop_get_mountlist (&mountlist, FALSE);

	new_disks = g_hash_table_new (g_str_hash, g_str_equal);
	for (i=0; i < mountlist.number; i++) {
		g_hash_table_insert (new_disks, entry[i].devname, &entry[i]);
	}

	gtk_tree_model_foreach (model, compare_disks, new_disks);

	g_hash_table_foreach (new_disks, add_new_disks, model);

	while (old_disks) {
		GtkTreeIter *iter = old_disks->data;

		gtk_tree_store_remove (GTK_TREE_STORE (model), iter);
		gtk_tree_iter_free (iter);

		old_disks = g_list_next (old_disks);
	}

	g_hash_table_destroy (new_disks);
	g_free (entry);

	return TRUE;
}


gint
cb_timeout (gpointer data)
{
	ProcData * const procdata = data;

	proctable_update_all (procdata);

	return TRUE;
}
