/* Procman - ability to show favorite processes blacklisted processes
 * For now the favorite processes will not be compiled in.
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

#include "favorites.h"
#include "proctable.h"

GtkWidget *blacklist_dialog = NULL;
GtkWidget *proctree = NULL;
gint initial_blacklist_num = 0; /* defined in order to prune off entries from config file */

void
add_to_blacklist (ProcData *procdata, gchar *name)
{
	gchar *process = g_strdup (name);
	procdata->blacklist = g_list_append (procdata->blacklist, process);
	procdata->blacklist_num++;
	
	if (blacklist_dialog) {
		GtkTreeModel *model;
		GtkTreeIter row;
		
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (proctree));
		gtk_tree_store_insert (GTK_TREE_STORE (model), &row, NULL, 0);
		gtk_tree_store_set (GTK_TREE_STORE (model), &row, 0, name, -1);
	}
		
}

static void
add_single_to_blacklist (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info = NULL;
	
	gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);
	g_return_if_fail (info);
	
	add_to_blacklist (procdata, info->cmd);
	
}

void
add_selected_to_blacklist (ProcData *procdata)
{
	if (!procdata->selection)
		return;
		
	gtk_tree_selection_selected_foreach (procdata->selection, 
					     add_single_to_blacklist, procdata);
}

void
remove_from_blacklist (ProcData *procdata, gchar *name)
{
	GList *list = procdata->blacklist;
	
	while (list) {
		if (!g_strcasecmp (list->data, name)) {
			procdata->blacklist = g_list_remove (procdata->blacklist, list->data);
			procdata->blacklist_num --;
			return;
		}
		
		list = g_list_next (list);
	}
	
}

gboolean
is_process_blacklisted (ProcData *procdata, gchar *name)
{
	GList *list = procdata->blacklist;
	
	if (!list)
	{
		return FALSE;
	}
	
	while (list)
	{
		gchar *process = list->data;
		if (!g_strcasecmp (process, name))
			return TRUE;
		
		list = g_list_next (list);
	}
	
	return FALSE;

}

void 
save_blacklist (ProcData *procdata, GConfClient *client)
{

	GList *list = procdata->blacklist;
	gint i = 0;
	
	while (list)
	{
		gchar *name = list->data;
		gchar *config = g_strdup_printf ("%s%d", "/apps/procman/process", i);
		gconf_client_set_string (client, config, name, NULL);
		g_free (config); 
		i++;
		
		list = g_list_next (list);
	}
	
	for (i = initial_blacklist_num; i >= procdata->blacklist_num; i--)
	{
		gchar *config = g_strdup_printf ("%s%d", "/apps/procman/process", i);
		gconf_client_unset (client, config, NULL);
		g_free (config);
	} 

}

void get_blacklist (ProcData *procdata, GConfClient *client)
{
	gint i = 0;
	gboolean done = FALSE;
	
	while (!done)
	{
		gchar *config = g_strdup_printf ("%s%d", "/apps/procman/process", i);
		gchar *process;
		
		process = gconf_client_get_string (client, config, NULL);
		g_free (config);
		if (process)
		{
			add_to_blacklist (procdata, process);
			g_free (process);
		}
		else
			done = TRUE;
		i++;
	}
	
	procdata->blacklist_num = i - 1;
	initial_blacklist_num = i - 1;

}

static void 
fill_tree_with_info (ProcData *procdata, GtkWidget *tree)
{
	GList *blacklist = procdata->blacklist;
	GtkTreeModel *model;
	GtkTreeIter row;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	
	/* add the blacklist */
	while (blacklist)
	{
		gtk_tree_store_insert (GTK_TREE_STORE (model), &row, NULL, 0);
		gtk_tree_store_set (GTK_TREE_STORE (model), &row, 0, blacklist->data, -1);
		blacklist = g_list_next (blacklist);
	}
}


static GtkWidget *
create_tree (ProcData *procdata)
{
	GtkWidget *scrolled;
	GtkWidget *tree;
	GtkTreeStore *model;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
  	GtkCellRenderer *cell_renderer;
		
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  	GTK_POLICY_AUTOMATIC,
                                  	GTK_POLICY_AUTOMATIC);
	
	model = gtk_tree_store_new (1, G_TYPE_STRING);
	
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	g_object_unref (G_OBJECT (model));
  	
  	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  	
  	cell_renderer = gtk_cell_renderer_text_new ();
  	column = gtk_tree_view_column_new_with_attributes ("hello",
						    	   cell_renderer,
						     	   "text", 0,
						     	   NULL);
	gtk_tree_view_column_set_sort_column_id (column, 0);
					     	   
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	
	gtk_container_add (GTK_CONTAINER (scrolled), tree);
	
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					      0,
					      GTK_SORT_ASCENDING);
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
	  	
	fill_tree_with_info (procdata, tree);

	proctree = tree;
	
	return scrolled;

}

GList *removed_iters = NULL;

static void
remove_item (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	ProcData *procdata = data;
	GtkTreeIter *iter_copy;
	gchar *process = NULL;
	
	gtk_tree_model_get (model, iter, 0, &process, -1);
	
	if (process)
		remove_from_blacklist (procdata, process);
	
	iter_copy = gtk_tree_iter_copy (iter);	
	removed_iters = g_list_append (removed_iters, iter_copy);
}

static void
remove_button_clicked (GtkButton *button, gpointer data)
{
	ProcData *procdata = data;
	GtkTreeModel *model;
	GtkTreeSelection *selection = NULL;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (proctree));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (proctree));
	
	gtk_tree_selection_selected_foreach (selection, remove_item, procdata); 
	
	while (removed_iters) {
		GtkTreeIter *iter = removed_iters->data;
		
		gtk_tree_store_remove (GTK_TREE_STORE (model), iter);
		gtk_tree_iter_free (iter);
		
		removed_iters = g_list_next (removed_iters);
	}
	
	proctable_update_all (procdata);
	
}

static void
close_blacklist_dialog (GtkDialog *dialog, gint id, gpointer data)
{
	gtk_widget_destroy (blacklist_dialog);
	
	blacklist_dialog = NULL;
}

void create_blacklist_dialog (ProcData *procdata)
{
	GtkWidget *frame;
	GtkWidget *main_vbox;
	GtkWidget *inner_vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *scrolled;
	GtkWidget *label;
	GtkWidget *dialog;
	gchar *message;
	

	if (procdata->blacklist_num == 0 )
	{
		message = g_strdup_printf(_("No processes are currently hidden."));
		dialog = gnome_error_dialog (message);
		gnome_dialog_run(GNOME_DIALOG (dialog));
		g_free (message);
	}
	
	else
	{

		if (blacklist_dialog)
		{
			gdk_window_raise(blacklist_dialog->window);
      			return;
   		}

		blacklist_dialog = gtk_dialog_new_with_buttons (_("Manage Hidden Processes"), 
								NULL,
								GTK_DIALOG_DESTROY_WITH_PARENT,
						     		GTK_STOCK_CLOSE, 
						     		GTK_RESPONSE_CLOSE,
						     		NULL);
		gtk_window_set_policy (GTK_WINDOW (blacklist_dialog), FALSE, TRUE, FALSE);
		gtk_window_set_default_size (GTK_WINDOW (blacklist_dialog), 320, 375);
		
		main_vbox = GTK_DIALOG (blacklist_dialog)->vbox;
	
		label = gtk_label_new (_("These are the processes you have chosen to hide. You can reshow a process by removing it from this list."));
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
		
		frame = gtk_frame_new (_("Hidden Processes"));
  		gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  	
  		inner_vbox = gtk_vbox_new (FALSE, 0);
  		gtk_container_add (GTK_CONTAINER (frame), inner_vbox);
  	
  		scrolled = create_tree (procdata);
  		gtk_box_pack_start (GTK_BOX (inner_vbox), scrolled, TRUE, TRUE, 0);
  		gtk_container_set_border_width (GTK_CONTAINER (scrolled), GNOME_PAD_SMALL);
  	
  		hbox = gtk_hbox_new (FALSE, 0);
  		gtk_box_pack_end (GTK_BOX (inner_vbox), hbox, FALSE, FALSE, 0);
  	
  		button = gtk_button_new_with_mnemonic (_("_Remove From List"));
  		/*gtk_misc_set_padding (GTK_MISC (GTK_BUTTON (button)->child), 
  				      GNOME_PAD_SMALL, -2);*/
  		gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  		gtk_container_set_border_width (GTK_CONTAINER (button), GNOME_PAD_SMALL);
  	
  		g_signal_connect (G_OBJECT (button), "clicked",
  			    	  G_CALLBACK (remove_button_clicked), procdata);
  		g_signal_connect (G_OBJECT (blacklist_dialog), "response",
			    	  G_CALLBACK (close_blacklist_dialog), procdata);
  	
  		gtk_widget_show_all (blacklist_dialog);
	}
  	
}
