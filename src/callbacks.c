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

#include <gnome.h>
#include <signal.h>
#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "infoview.h"
#include "procdialogs.h"
#include "memmaps.h"
#include "favorites.h"





void
cb_properties_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
cb_preferences_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

void
cb_renice (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	procdialog_create_renice_dialog (procdata);
	
}

void
cb_show_memory_maps (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	create_memmaps_dialog (procdata);
}

void
cb_add_to_favorites (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	
	if (!procdata->selected_node)
		return;
	
	info = e_tree_memory_node_get_data (procdata->memory, procdata->selected_node);
	add_to_favorites (procdata, info->cmd);
	
}


void
cb_about_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

	GtkWidget *about;
	const gchar *authors[] = {
				 "Kevin Vandersloot (kfv101@psu.edu)",
				 "Erik Johnsson (zaphod@linux.nu)",
				 NULL
				 };
				 
	about = gnome_about_new (_("Process Manager"), VERSION,
				 _("(C) 2001 Kevin Vandersloot"),
				 authors,
				 _("Simple process viewer using libgtop"),
				 NULL);
				 
	gtk_widget_show (about);  
}


void
cb_app_destroy                        (GtkObject       *object,
                                       gpointer		user_data)
{
	ProcData *procdata = user_data;
	if (procdata)
	{
		proctable_save_state (procdata);
		procman_save_config (procdata);
	}
	gtk_main_quit ();
	
	/*FIXME: we need to free the Procdata and any other stuff here
	*/

}


void
cb_all_process_menu_clicked 		(GtkWidget	*widget,
					 gpointer	data)
{
	ProcData *procdata = data;
	
	procdata->config.whose_process = ALL_PROCESSES;
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
}


void
cb_my_process_menu_clicked		(GtkWidget	*widget,
					 gpointer	data)
{
	ProcData *procdata = data;
	
	procdata->config.whose_process = MY_PROCESSES;
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
}

void
cb_running_process_menu_clicked		(GtkWidget	*widget,
					 gpointer	data)
{
	ProcData *procdata = data;
	
	procdata->config.whose_process = RUNNING_PROCESSES;
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
}				


void
cb_favorites_menu_clicked (GtkWidget *widget, gpointer data)
{
	ProcData *procdata = data;
	
	procdata->config.whose_process = FAVORITE_PROCESSES;
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
}

void
cb_end_process_button_pressed          (GtkButton       *button,
                                        gpointer         data)
{

	ProcData *procdata = data;
	ProcInfo *info;
	GtkWidget *dialog;

	if (!procdata->selected_node)
		return;
		
	if (procdata->config.show_kill_warning)
	{	
		dialog = procdialog_create_kill_dialog (procdata);
		gtk_widget_show (dialog);
	}
	else
	{
		info = e_tree_memory_node_get_data (procdata->memory, 
						    procdata->selected_node);
		kill (info->pid, SIGTERM);
		proctable_update_all (procdata);
	}
			
	

}

void
cb_info_button_pressed			(GtkButton	*button,
					 gpointer	user_data)
{
	ProcData *procdata = user_data;
	
	toggle_infoview (procdata);

}	






void
cb_table_selected (ETree *tree, int row, ETreePath path, gpointer data)
{

	ProcData *procdata = data;
	ProcInfo *info;
	
	
	if (!tree)
		return;
	if (row == -1)
	{
		if (GTK_WIDGET_SENSITIVE (procdata->infobox))
			gtk_widget_set_sensitive (procdata->infobox, FALSE);
		update_sensitivity (procdata, FALSE);
		return;
	}
		
	
	info = e_tree_memory_node_get_data (procdata->memory, path);
	procdata->selected_pid = info->pid;
	procdata->selected_node = path;
	
		
	update_sensitivity (procdata, TRUE);
		
	if (procdata->config.show_more_info == TRUE)
		infoview_update (procdata);
	
	update_memmaps_dialog (procdata);
		 


}

void
cb_double_click (ETree *tree, int row, ETreePath path, int col, 
		 GdkEvent *event, gpointer data)
{
	ProcData *procdata = data;
	
	toggle_infoview (procdata);

}
	

gint
cb_timeout (gpointer data)
{
	ProcData *procdata = data;
	
	proctable_update_all (procdata);

	return TRUE;
}

