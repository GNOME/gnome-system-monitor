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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glibtop/xmalloc.h>
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
#include "favorites.h"
#include "load-graph.h"
#include "cellrenderer.h"

void
cb_preferences_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	ProcData *procdata = user_data;
	
	procdialog_create_preferences_dialog (procdata);
}

void
cb_renice (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;

	procdialog_create_renice_dialog (procdata);

}

void
cb_end_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	if (procdata->config.show_kill_warning)
		procdialog_create_kill_dialog (procdata, SIGTERM);
	else
		kill_process (procdata, SIGTERM);
	
}

void
cb_kill_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	if (procdata->config.show_kill_warning)
		procdialog_create_kill_dialog (procdata, SIGKILL);
	else
		kill_process (procdata, SIGKILL);
	
}

void
cb_show_memory_maps (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	create_memmaps_dialog (procdata);
}

void		
cb_show_hidden_processes (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	create_blacklist_dialog (procdata);
}

void
cb_hide_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	if (procdata->config.show_hide_message)
		procdialog_create_hide_dialog (procdata);
	else
	{
		add_selected_to_blacklist (procdata);
	}
	
}


void
cb_about_activate (GtkMenuItem *menuitem, gpointer user_data)
{

	GtkWidget *about;
	GdkPixbuf *pixbuf;
	GError    *error = NULL;
	gchar     *file;


	const gchar *authors[] = {
				 _("Kevin Vandersloot (kfv101@psu.edu)"),
				 _("Jorgen Scheibengruber <mfcn@gmx.de> - nicer devices treeview"),
				NULL
				 };

	const gchar *documenters[] = {
				 NULL
	};

	const gchar *translator_credits = _("translator_credits");
	
	file = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, 
				          "procman.png", FALSE, NULL);
	pixbuf = gdk_pixbuf_new_from_file (file, &error);
	
	if (error) {
		g_warning (G_STRLOC ": cannot open %s: %s", file, error->message);
		g_error_free (error);
	}
	
	g_free (file);

			 
	about = gnome_about_new (_("System Monitor"), VERSION,
				 _("(C) 2001 Kevin Vandersloot"),
				 _("Simple process viewer using libgtop"),
				 authors,
				 documenters,
				 strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				 pixbuf);
	
	if (pixbuf) {
		g_object_unref (pixbuf);
	}	

			 
	gtk_widget_show (about);  
	
}


void
cb_app_exit (GtkObject *object, gpointer user_data)
{
	ProcData *procdata = user_data;

	cb_app_delete (NULL, NULL, procdata);
	
}

gboolean		
cb_app_delete (GtkWidget *window, GdkEventAny *event, gpointer data)
{
	ProcData *procdata = data;

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
#if 0
gboolean	
cb_close_simple_dialog (GnomeDialog *dialog, gpointer data)
{
	ProcData *procdata = data;
	
	if (procdata->timeout != -1)
		gtk_timeout_remove (procdata->timeout);
	
	gtk_main_quit ();
		
	return FALSE;

}
#endif
void
cb_all_process_menu_clicked 		(GtkWidget	*widget,
					 gpointer	data)
{
	ProcData *procdata = data;
	GConfClient *client = procdata->client;
	
	g_return_if_fail (data);
	procdata->config.whose_process = ALL_PROCESSES;
	gconf_client_set_int (client, "/apps/procman/view_as", 
			      procdata->config.whose_process, NULL);
	
}


void
cb_my_process_menu_clicked		(GtkWidget	*widget,
					 gpointer	data)
{
	ProcData *procdata = data;
	GConfClient *client = procdata->client;
	
	g_return_if_fail (data);
	procdata->config.whose_process = MY_PROCESSES;
	gconf_client_set_int (client, "/apps/procman/view_as", 
			      procdata->config.whose_process, NULL);
}

void
cb_running_process_menu_clicked		(GtkWidget	*widget,
					 gpointer	data)
{
	ProcData *procdata = data;
	GConfClient *client = procdata->client;
	
	g_return_if_fail (data);
	procdata->config.whose_process = RUNNING_PROCESSES;
	gconf_client_set_int (client, "/apps/procman/view_as", 
			      procdata->config.whose_process, NULL);
}				

void
popup_menu_renice (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	procdialog_create_renice_dialog (procdata);
}

void
popup_menu_show_memory_maps (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	create_memmaps_dialog (procdata);
}

void
popup_menu_hide_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	if (procdata->config.show_hide_message)
		procdialog_create_hide_dialog (procdata);
	else
	{
		add_selected_to_blacklist (procdata);
	}
	
}

void 
popup_menu_end_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;

        if (procdata->config.show_kill_warning)
		procdialog_create_kill_dialog (procdata, SIGTERM);
	else
		kill_process (procdata, SIGTERM);	
}

void 
popup_menu_kill_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;

        if (procdata->config.show_kill_warning)
		procdialog_create_kill_dialog (procdata, SIGKILL);
	else	
		kill_process (procdata, SIGKILL);
			
}
#if 0
void 
popup_menu_about_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info = NULL;
	gchar *name;
	
	if (!procdata->selected_node)
		return;
		
	info = e_tree_memory_node_get_data (procdata->memory, 
					    procdata->selected_node);
	g_return_if_fail (info != NULL);
	
	/* FIXME: this is lame. GNOME help browser sucks balls. There should be a way
	to first check man pages, then info pages and give a nice error message if nothing
	exists */			    
	name = g_strjoin (NULL, "man:", info->cmd, NULL);
	gnome_url_show (name);
	g_free (name);
					    
}
#endif

void
cb_end_process_button_pressed          (GtkButton       *button,
                                        gpointer         data)
{

	ProcData *procdata = data;

	if (procdata->config.show_kill_warning)
		procdialog_create_kill_dialog (procdata, SIGTERM);
	else
		kill_process (procdata, SIGTERM);
	
}

void
cb_info_button_pressed			(GtkButton	*button,
					 gpointer	user_data)
{
	ProcData *procdata = user_data;
	
	toggle_infoview (procdata);
		
}	

void		
cb_search (GtkEditable *editable, gpointer data)
{
	ProcData *procdata = data;
	gchar *text;
	
	text = gtk_editable_get_chars (editable, 0, -1);
	
	proctable_search_table (procdata, text);
	gtk_widget_grab_focus (GTK_WIDGET (editable));
	g_free (text);
}


void		
cb_cpu_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		      guint a, gpointer data)
{
	ProcData *procdata = data;
	GConfClient *client = procdata->client;
	gchar *color;
	
	color = g_strdup_printf("#%02x%02x%02x", r, g, b);
	gconf_client_set_string (client, "/apps/procman/cpu_color", color, NULL);
	g_free (color);
}

void		
cb_mem_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		      guint a, gpointer data)
{
	ProcData *procdata = data;
	GConfClient *client = procdata->client;
	gchar *color;

	color = g_strdup_printf("#%02x%02x%02x", r, g, b);
	gconf_client_set_string (client, "/apps/procman/mem_color", color, NULL);
	g_free (color);
}

void		
cb_swap_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		       guint a, gpointer data)
{
	ProcData *procdata = data;
	GConfClient *client = procdata->client;
	gchar *color;

	color = g_strdup_printf("#%02x%02x%02x", r, g, b);
	gconf_client_set_string (client, "/apps/procman/swap_color", color, NULL);
	g_free (color);
}

ProcInfo *selected_process = NULL;

static void
get_last_selected (GtkTreeModel *model, GtkTreePath *path, 
      		   GtkTreeIter *iter, gpointer data)
{
	ProcInfo *info;
	
	gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);
	g_return_if_fail (info);

	selected_process = info;
}

void
cb_row_selected (GtkTreeSelection *selection, gpointer data)
{
	ProcData *procdata = data;
	
	procdata->selection = selection;

	/* get the most recent selected process and determine if there are
	** no selected processes 
	*/
	selected_process = NULL;	
	gtk_tree_selection_selected_foreach (procdata->selection, get_last_selected, 
					     procdata);	
	
	if (selected_process) {
		procdata->selected_process = selected_process;
		if (procdata->config.show_more_info == TRUE)
			infoview_update (procdata);
		update_sensitivity (procdata, TRUE);
	}
	else {	
		procdata->selected_process = NULL;
		update_sensitivity (procdata, FALSE);
	}
	
}

gboolean
cb_tree_button_pressed (GtkWidget *widget, GdkEventButton *event, 
			gpointer data)
{
        ProcData *procdata = data;

	if (event->type == GDK_2BUTTON_PRESS) 
		toggle_infoview (procdata);
	else
        	do_popup_menu (procdata, event);

        return FALSE;

}

gint
cb_tree_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	ProcData *procdata = data;
		
	switch (event->keyval) {
	case GDK_Return:
	case GDK_space:
		toggle_infoview (procdata);
		break;
	default:
	}
		
	return FALSE;
		
}

void		
cb_switch_page (GtkNotebook *nb, GtkNotebookPage *page, 
		gint num, gpointer data)
{
	ProcData *procdata = data;
		
	procdata->config.current_tab = num;
	
	if (num == 0) {
		if (procdata->timeout == -1) 
			procdata->timeout = gtk_timeout_add (procdata->config.update_interval,
			 			     	     cb_timeout, procdata);
		load_graph_stop (procdata->cpu_graph);
		load_graph_stop (procdata->mem_graph);
		if (procdata->selected_process)
			update_sensitivity (procdata, TRUE);
	}
	else {
		if (procdata->timeout != -1 ) {
			gtk_timeout_remove (procdata->timeout);
			procdata->timeout = -1;
		}
		load_graph_start (procdata->cpu_graph);
		load_graph_start (procdata->mem_graph);
		if (procdata->selected_process)
			update_sensitivity (procdata, FALSE);
	}

}

GList *old_disks = NULL;

static gboolean
compare_disks (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	GHashTable *new_disks = data;
	GtkTreeIter *old_iter;
	glibtop_mountentry *entry = NULL;
	gchar *old_name;
	
	gtk_tree_model_get (model, iter, 1, &old_name, -1);
		
	entry = g_hash_table_lookup (new_disks, old_name);
	if (entry) {
		glibtop_fsusage usage;
		gchar *used, *total, *unused;
		float percentage, bused, bfree, btotal;
		
		glibtop_get_fsusage (&usage, entry->mountdir);

		btotal = (float)usage.blocks * 512;
		bfree = (float)usage.bfree * 512;
		bused = (float)(usage.blocks  - usage.bfree) * 512;
		percentage = (float) (usage.blocks - usage.bfree) / (float) usage.blocks;
		
		used = get_size_string (bused);
		total = get_size_string (btotal);
		unused = get_size_string (bfree);
		
		gtk_tree_store_set (GTK_TREE_STORE (model), iter,
				    4, total,
				    5, used,
					6, percentage,
					7, btotal,
					8, bfree, -1);
		g_hash_table_remove (new_disks, old_name);
		
		g_free (used);
		g_free (total);
		g_free (unused);

		g_free (old_name);
			
		return FALSE;
		
	}
	else {
		old_iter = gtk_tree_iter_copy (iter);
		old_disks = g_list_append (old_disks, old_iter);
	}
	
	g_free (old_name);
	return FALSE;
	
}

static GdkPixbuf*
get_icon_for_device(GnomeIconTheme *icontheme, char *mountpoint, char *type)
{
	GdkPixbuf *tmp, *pixbuf;
	char *i_type, *path;
	int size = 24;
	
	if (strstr(mountpoint,"/zip"))
		i_type = "gnome-dev-zipdisk";
	else 
	if (strstr(mountpoint,"/floppy"))
		i_type = "gnome-dev-floppy";
	else 
	if (strstr(type,"iso9660"))
		i_type = "gnome-dev-cdrom";
	else 
	if (strstr(type, "smbfs"))
		i_type = "gnome-fs-smb";
	else
	if (strstr(type, "nfs"))
		i_type = "gnome-fs-nfs";
	else 
		i_type = "gnome-dev-harddisk";

	if (!(path = gnome_icon_theme_lookup_icon(icontheme, i_type, 24, NULL, &size)))
		return NULL;

	if (!(tmp = gdk_pixbuf_new_from_file(path, NULL)))
		return NULL;

	g_free(path);
	
	if (size != 24)
	{	
		pixbuf = gdk_pixbuf_scale_simple (tmp, 24, 24, 
						GDK_INTERP_HYPER);
		g_object_unref(tmp);
		return pixbuf;
	}
	return tmp;
}

static void
add_new_disks (gpointer key, gpointer value, gpointer data)
{
	glibtop_mountentry *entry = value;
	GtkTreeModel *model = data;
	glibtop_fsusage usage;
	gchar *text[5];
	GdkPixbuf *pixbuf = NULL;
	GnomeIconTheme *icontheme;
	icontheme  = gnome_icon_theme_new();
	
	glibtop_get_fsusage (&usage, entry->mountdir);
	
	/* Hmm, usage.blocks == 0 seems to get rid of /proc and all
	** the other useless entries */
	if (usage.blocks != 0) {
		GtkTreeIter row;
		float percentage, btotal, bfree, bused;

		btotal = (float)usage.blocks * 512;
		bfree = (float)usage.bfree * 512;
		bused = (float)(usage.blocks  - usage.bfree) * 512;
		percentage = (float) (usage.blocks - usage.bfree) / (float) usage.blocks;
		
	/*  Load an icon corresponding to the type of the device */
		pixbuf = get_icon_for_device(icontheme, entry->mountdir, entry->type);
	
		text[0] = g_strdup (entry->devname);
		text[1] = g_strdup (entry->mountdir);
		text[2] = g_strdup (entry->type);
		text[3] = get_size_string (btotal);
		text[4] = get_size_string (bused);
		text[5] = get_size_string (bfree);
		
		gtk_tree_store_insert (GTK_TREE_STORE (model), &row, NULL, 0); 
		gtk_tree_store_set (GTK_TREE_STORE (model), &row,
					    	0, pixbuf,
						1, text[0],
					    	2, text[1],
					    	3, text[2],
					    	4, text[3],
					    	5, text[4],
						6, percentage,
						7, btotal,
						8, bfree, -1);
						
		g_free (text[0]);
		g_free (text[1]);
		g_free (text[2]);
		g_free (text[3]);
		g_free (text[4]);
		g_free (text[5]);
		if (pixbuf)
			g_object_unref (pixbuf);
	}
	
	g_object_unref (G_OBJECT (icontheme));
}

gint
cb_update_disks (gpointer data)
{
	ProcData *procdata = data;
	GtkTreeModel *model;
	glibtop_mountentry *entry;
	glibtop_mountlist mountlist;
	GHashTable *new_disks = NULL;
	gint i;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->disk_list));
	
	entry = glibtop_get_mountlist (&mountlist, 0);
	
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
	glibtop_free (entry);
	
	return TRUE;
}

gint
cb_timeout (gpointer data)
{
	ProcData *procdata = data;
	
	proctable_update_all (procdata);
	
	return TRUE;
}


