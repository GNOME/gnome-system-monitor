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
	
	if (!procdata->selected_process)
		return;
	if (procdata->config.show_kill_warning)
		procdialog_create_kill_dialog (procdata, SIGTERM);
	else
		kill_process (procdata, SIGTERM);
	
}

void
cb_kill_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	if (!procdata->selected_process)
		return;
	
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
	ProcInfo *info;
	
	if (!procdata->selected_process)
		return;
	
	if (procdata->config.show_hide_message)
		procdialog_create_hide_dialog (procdata);
	else
	{
		add_to_blacklist (procdata, procdata->selected_process->cmd);
		proctable_update_all (procdata);
	}
	
}


void
cb_about_activate (GtkMenuItem *menuitem, gpointer user_data)
{

	GtkWidget *about;
	const gchar *authors[] = {
				 _("Kevin Vandersloot (kfv101@psu.edu)"),
				 _("Erik Johnsson (zaphod@linux.nu) - icon support"),
				 NULL
				 };
				 
	about = gnome_about_new (_("Process Manager"), VERSION,
				 _("(C) 2001 Kevin Vandersloot"),
				 _("Simple process viewer using libgtop"),
				 authors,
				 NULL, NULL, NULL);
				 
	gtk_widget_show (about);  
	
}


void
cb_app_exit (GtkObject *object, gpointer user_data)
{
	ProcData *procdata = user_data;
	
	cb_app_delete (NULL, NULL, procdata);
	
}

void		
cb_app_delete (GtkWidget *window, GdkEventAny *ev, gpointer data)
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
	g_return_if_fail (data);
	procdata->config.whose_process = ALL_PROCESSES;
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
}


void
cb_my_process_menu_clicked		(GtkWidget	*widget,
					 gpointer	data)
{
	ProcData *procdata = data;
	g_return_if_fail (data);
	procdata->config.whose_process = MY_PROCESSES;
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
}

void
cb_running_process_menu_clicked		(GtkWidget	*widget,
					 gpointer	data)
{
	ProcData *procdata = data;
	g_return_if_fail (data);
	procdata->config.whose_process = RUNNING_PROCESSES;
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
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
	
	if (!procdata->selected_process)
		return;
	
	if (procdata->config.show_hide_message)
		procdialog_create_hide_dialog (procdata);
	else
	{
		add_to_blacklist (procdata, procdata->selected_process->cmd);
		proctable_update_all (procdata);
	}
	
}

void 
popup_menu_end_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;

        if (!procdata->selected_process)
		return;
	
	if (procdata->config.show_kill_warning)
		procdialog_create_kill_dialog (procdata, SIGTERM);
	else
		kill_process (procdata, SIGTERM);	
}

void 
popup_menu_kill_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;

        if (!procdata->selected_process)
		return;
		
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

	if (!procdata->selected_process)
		return;
		
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
	
	g_free (text);
}


void		
cb_cpu_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		      guint a, gpointer data)
{
	ProcData *procdata = data;
	
	procdata->cpu_graph->colors[2].red = r;
	procdata->cpu_graph->colors[2].green = g;
	procdata->cpu_graph->colors[2].blue = b;
	procdata->config.cpu_color.red = r;
	procdata->config.cpu_color.green = g;
	procdata->config.cpu_color.blue = b;	
	procdata->cpu_graph->colors_allocated = FALSE;

}

void		
cb_mem_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		      guint a, gpointer data)
{
	ProcData *procdata = data;
	
	procdata->mem_graph->colors[2].red = r;
	procdata->mem_graph->colors[2].green = g;
	procdata->mem_graph->colors[2].blue = b;
	procdata->config.mem_color.red = r;
	procdata->config.mem_color.green = g;
	procdata->config.mem_color.blue = b;	
	procdata->mem_graph->colors_allocated = FALSE;

}

void		
cb_swap_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		       guint a, gpointer data)
{
	ProcData *procdata = data;
	
	procdata->mem_graph->colors[3].red = r;
	procdata->mem_graph->colors[3].green = g;
	procdata->mem_graph->colors[3].blue = b;
	procdata->config.swap_color.red = r;
	procdata->config.swap_color.green = g;
	procdata->config.swap_color.blue = b;	
	procdata->mem_graph->colors_allocated = FALSE;

}

void
cb_row_selected (GtkTreeSelection *selection, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean selected = FALSE;
	
	selected = gtk_tree_selection_get_selected (selection, NULL, &iter);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
	
	if (selected) {
		gtk_tree_model_get (model, &iter, COL_POINTER, &info, -1);
		if (info)
			procdata->selected_process = info;
		if (procdata->config.show_more_info == TRUE)
			infoview_update (procdata);
		update_sensitivity (procdata, TRUE);
		
		/*update_memmaps_dialog (procdata);*/
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

gint
cb_update_disks (gpointer data)
{
	ProcData *procdata = data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	glibtop_mountentry *entry;
	glibtop_mountlist mountlist;
	gboolean selected;
	gchar *selected_disk = NULL;
	gint i;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->disk_list));
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (procdata->disk_list));
	selected = gtk_tree_selection_get_selected (selection, NULL, &iter);
	
	if (selected) {
		gtk_tree_model_get (model, &iter, 0, &selected_disk, -1);
	}
	
	gtk_list_store_clear (GTK_LIST_STORE (model));
	
	entry = glibtop_get_mountlist (&mountlist, 0);
	for (i=0; i < mountlist.number; i++) {
		glibtop_fsusage usage;
		gchar *text[4];
				
		glibtop_get_fsusage (&usage, entry[i].mountdir);
		text[0] = g_strdup (entry[i].devname);
		text[1] = g_strdup (entry[i].mountdir);
		text[2] = get_size_string ((float)(usage.blocks - usage.bfree) * 512);
		text[3] = get_size_string ((float) usage.blocks * 512);
		/* Hmm, usage.blocks == 0 seems to get rid of /proc and all
		** the other useless entries */
		if (usage.blocks != 0) {
			GtkTreeIter row;
			
			gtk_list_store_insert (GTK_LIST_STORE (model), &row, 0); 
			gtk_list_store_set (GTK_LIST_STORE (model), &row,
					    0, text[0],
					    1, text[1],
					    2, text[2],
					    3, text[3], -1);
			if (selected_disk && !g_strcasecmp (selected_disk, text[0])) {
				GtkTreePath *path;
				g_print ("selected disk %s \n", text[0]);
				path = gtk_tree_model_get_path (model, &row);
				gtk_tree_view_set_cursor (GTK_TREE_VIEW (procdata->disk_list),
							  path,
							  NULL,
							  FALSE);
			}
		}
		
		
		
		g_free (text[0]);
		g_free (text[1]);
		g_free (text[2]);
		g_free (text[3]);
	}
	
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


