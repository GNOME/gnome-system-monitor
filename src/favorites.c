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

#include <gal/e-table/e-tree-memory.h>
#include <gal/e-table/e-tree-memory-callbacks.h>
#include <gal/e-table/e-tree-scrolled.h>
#include <gal/e-table/e-cell-text.h>
#include "favorites.h"

#define SPEC "<ETableSpecification cursor-mode=\"line\" selection-mode=\"browse\" draw-focus=\"true\" no-headers=\"true\">                    	       \
  <ETableColumn model_col=\"0\" _title=\" \"   expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"blacklist\" compare=\"string\"/> \
  	<ETableState> \
        	<column source=\"0\"/> \
	        <grouping> <leaf column=\"0\" ascending=\"true\"/> </grouping>    \
        </ETableState> \
</ETableSpecification>"

GtkWidget *blacklist_dialog = NULL;
GtkWidget *tree;
ETreeMemory *memory;
ETreePath root_node;
gint initial_blacklist_num; /* defined in order to prune off entries from config file */

void
add_to_favorites (ProcData *procdata, gchar *name)
{
	gchar *favorite = g_strdup (name);
	procdata->favorites = g_list_append (procdata->favorites, favorite);

}


void
add_to_blacklist (ProcData *procdata, gchar *name)
{
	gchar *process = g_strdup (name);
	procdata->blacklist = g_list_append (procdata->blacklist, process);
	procdata->blacklist_num++;
	
}

void
remove_from_favorites (ProcData *procdata, gchar *name)
{


}

void
remove_from_blacklist (ProcData *procdata, gchar *name)
{


	procdata->blacklist = g_list_remove (procdata->blacklist, name);
	procdata->blacklist_num --;
}


gboolean
is_process_a_favorite (ProcData *procdata, gchar *name)
{
	GList *list = procdata->favorites;
	
	if (!list)
	{
		return FALSE;
	}
	
	while (list)
	{
		gchar *favorite = list->data;
		if (!g_strcasecmp (favorite, name))
			return TRUE;
		
		list = g_list_next (list);
	}
	
	return FALSE;

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

void save_favorites (ProcData *procdata)
{

	GList *list = procdata->favorites;
	gint i = 0;
	
	while (list)
	{
		gchar *name = list->data;
		gchar *config = g_strdup_printf ("%s%d", "procman/Favorites/favorite", i);
		gnome_config_set_string (config, name);
		g_free (config); 
		i++;
		list = g_list_next (list);
	}
}


void save_blacklist (ProcData *procdata)
{

	GList *list = procdata->blacklist;
	gint i = 0;
	
	while (list)
	{
		gchar *name = list->data;
		gchar *config = g_strdup_printf ("%s%d", "procman/Blacklist/process", i);
		gnome_config_set_string (config, name);
		g_free (config); 
		i++;
		list = g_list_next (list);
	}
	
	for (i = initial_blacklist_num; i >= procdata->blacklist_num; i--)
	{
		gchar *config = g_strdup_printf ("%s%d", "procman/Blacklist/process", i);
		gnome_config_clean_key (config);
		g_free (config);
	} 
}


void get_favorites (ProcData *procdata)
{
	gint i = 0;
	gboolean done = FALSE;
	
	while (!done)
	{
		gchar *config = g_strdup_printf ("%s%d", "procman/Favorites/favorite", i);
		gchar *favorite;
		
		favorite = gnome_config_get_string (config);
		if (favorite)
			add_to_favorites (procdata, favorite);
		else
			done = TRUE;
		i++;
	}
	
} 

void get_blacklist (ProcData *procdata)
{
	gint i = 0;
	gboolean done = FALSE;
	
	while (!done)
	{
		gchar *config = g_strdup_printf ("%s%d", "procman/Blacklist/process", i);
		gchar *process;
		
		process = gnome_config_get_string (config);
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


static GdkPixbuf *
get_icon (ETreeModel *etm, ETreePath path, void *data)
{
	/* No icon, since the cell tree renderer takes care of the +/- icons itself. */
	return NULL;
}

static int
get_columns (ETreeModel *table, void *data)
{
	return 1;
}


static void *
get_value (ETreeModel *model, ETreePath path, int column, void *data)
{
	gchar *string;
	
	string = e_tree_memory_node_get_data (memory, path);

	return string;
}

static void
set_value (ETreeModel *model, ETreePath path, int col, const void *value, void *data)
{

}	

static gboolean
get_editable (ETreeModel *model, ETreePath path, int column, void *data)
{
	return FALSE;
}

static void *
duplicate_value (ETreeModel *model, int column, const void *value, void *data)
{
	return g_strdup (value);
	
}

static void
free_value (ETreeModel *model, int column, void *value, void *data)
{
	g_free (value);
	
}

static void *
initialize_value (ETreeModel *model, int column, void *data)
{
	return g_strdup ("");
	
}

static gboolean
value_is_empty (ETreeModel *model, int column, const void *value, void *data)
{
	return !(value && *(char *)value);
	
}

static char *
value_to_string (ETreeModel *model, int column, const void *value, void *data)
{
	return g_strdup (value);
	
}

static void 
fill_tree_with_info (ProcData *procdata)
{
	GList *blacklist = procdata->blacklist;
	
	if (!memory)
		return;
		
	root_node = e_tree_memory_node_insert (memory, NULL, 0, NULL);
	e_tree_root_node_set_visible (E_TREE(tree), FALSE);
	
	/* add the blacklist */
	while (blacklist)
	{
		e_tree_memory_node_insert (memory, root_node, 0, blacklist->data);
		blacklist = g_list_next (blacklist);
	}
}

static ETableExtras *
new_extras ()
{
	ETableExtras *extras;
	ECell *cell;
	
	extras = e_table_extras_new ();
	
	cell = e_cell_text_new (NULL, GTK_JUSTIFY_LEFT);
	e_table_extras_add_cell (extras, "blacklist", cell);
	
	return extras;
}

static GtkWidget *
create_tree (ProcData *procdata)
{
	GtkWidget *scrolled;
	GtkWidget *e_tree;
	ETableExtras *extras;
	ETreeMemory *etmm;
	ETreeModel *model;

	model = e_tree_memory_callbacks_new (get_icon,
					     get_columns,
					     NULL,
					     NULL,
					     NULL,
					     NULL,
					     get_value,
					     set_value,
					     get_editable,
				    	     duplicate_value,
				    	     free_value,
				    	     initialize_value,
				    	     value_is_empty,
				    	     value_to_string,
				    	     NULL);
	
					    	     
	etmm = E_TREE_MEMORY(model);
	memory = etmm;
	
	extras = new_extras ();

	scrolled = e_tree_scrolled_new (model, extras, SPEC, NULL);

	e_tree = GTK_WIDGET (e_tree_scrolled_get_tree (E_TREE_SCROLLED (scrolled)));
	tree = e_tree;

	fill_tree_with_info (procdata);

	return scrolled;

}

static void
remove_item (ETreePath node, gpointer data)
{
	ProcData *procdata = data;
	gchar *process;
	
	process = e_tree_memory_node_get_data (memory, node);
	remove_from_blacklist (procdata, process);
	
}

static void
remove_button_clicked (GtkButton *button, gpointer data)
{
	ProcData *procdata = data;
	
	e_tree_selected_path_foreach (E_TREE (tree), remove_item, procdata);
	
	if (root_node)
	{
		e_tree_memory_node_remove (memory, root_node);
		fill_tree_with_info (procdata);
	}
		
}

static gboolean
close_blacklist_dialog (GnomeDialog *dialog, gpointer data)
{
	blacklist_dialog = NULL;
	
	return FALSE;
}

static void
close_button_pressed (GnomeDialog *dialog, gint button, gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG (blacklist_dialog));
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

		blacklist_dialog = gnome_dialog_new (_("Manage Hidden Processes"), 
					     GNOME_STOCK_BUTTON_CLOSE, NULL);
		gtk_window_set_policy (GTK_WINDOW (blacklist_dialog), FALSE, TRUE, FALSE);
		gtk_window_set_default_size (GTK_WINDOW (blacklist_dialog), 320, 375);
		
		main_vbox = GNOME_DIALOG (blacklist_dialog)->vbox;
	
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
  	
  		button = gtk_button_new_with_label (_("Remove From List"));
  		gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  		gtk_container_set_border_width (GTK_CONTAINER (button), GNOME_PAD_SMALL);
  	
  		gtk_signal_connect (GTK_OBJECT (button), "clicked",
  			    GTK_SIGNAL_FUNC (remove_button_clicked), procdata);
  		gtk_signal_connect (GTK_OBJECT (blacklist_dialog), "clicked",
			    GTK_SIGNAL_FUNC (close_button_pressed), procdata);
		gtk_signal_connect (GTK_OBJECT (blacklist_dialog), "close",
			    GTK_SIGNAL_FUNC (close_blacklist_dialog), procdata);
  	
  		gtk_widget_show_all (blacklist_dialog);
	}
  	
}
