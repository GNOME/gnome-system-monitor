/* Procman - main window
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

#include <gnome.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>

#include "procman.h"
#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "infoview.h"
#include "procactions.h"
#include "load-graph.h"
#include "util.h"

static void	cb_toggle_tree (GtkMenuItem *menuitem, gpointer data);
static void	cb_toggle_threads (GtkMenuItem *menuitem, gpointer data);


static GnomeUIInfo file1_menu_uiinfo[] =
{
	GNOMEUIINFO_MENU_QUIT_ITEM (cb_app_exit, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo edit1_menu_uiinfo[] =
{
	{
	 GNOME_APP_UI_ITEM, N_("End _Process"), N_("Force process to finish normally"),
	 cb_end_process, NULL, NULL, 0, NULL,
	 'e', GDK_CONTROL_MASK
	},
	{
	 GNOME_APP_UI_ITEM, N_("_Kill Process"), N_("Force process to finish immediately"),
	 cb_kill_process, NULL, NULL, 0, NULL,
	 'k', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_SEPARATOR,
 	{
 	  GNOME_APP_UI_ITEM, N_("_Change Priority..."), N_("Change the order of priority of process"),
	 cb_renice, NULL, NULL, 0, NULL,
	 'r', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_PREFERENCES_ITEM (cb_preferences_activate, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo view1_menu_uiinfo[] =
{
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Dependencies"), N_("Show parent/child relationship between processes"),
	 cb_toggle_tree, NULL, NULL, 0, NULL,
	 'd', GDK_CONTROL_MASK
	}, 
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Threads"), N_("Show each thread as a separate process"),
	 cb_toggle_threads, NULL, NULL, 0, NULL,
	 't', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Hide Process"), N_("Hide process from list"),
	 cb_hide_process, NULL, NULL, 0, NULL,
	 'h', GDK_CONTROL_MASK
	}, 
	{
	 GNOME_APP_UI_ITEM, N_("_Hidden Processes"), N_("Open the list of currently hidden processes"),
	 cb_show_hidden_processes, NULL, NULL, 0, NULL,
	 'p', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Memory Maps"), N_("Open the memory maps associated with a process"),
	 cb_show_memory_maps, NULL, NULL, 0, NULL,
	 'm', GDK_CONTROL_MASK
	},
	{
	 GNOME_APP_UI_ITEM, N_("Open _Files"), N_("View the files opened by a process"),
	 cb_show_open_files, NULL, NULL, 0, 0,
	 'f', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_END
};

static GnomeUIInfo help1_menu_uiinfo[] =
{
	GNOMEUIINFO_HELP("gnome-system-monitor"),
	GNOMEUIINFO_MENU_ABOUT_ITEM (cb_about_activate, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo menubar1_uiinfo[] =
{
	GNOMEUIINFO_MENU_FILE_TREE (file1_menu_uiinfo),
	GNOMEUIINFO_MENU_EDIT_TREE (edit1_menu_uiinfo),
	GNOMEUIINFO_MENU_VIEW_TREE (view1_menu_uiinfo),
	GNOMEUIINFO_MENU_HELP_TREE (help1_menu_uiinfo),
	GNOMEUIINFO_END
};

static GnomeUIInfo popup_menu_uiinfo[] =
{
	{
 	  GNOME_APP_UI_ITEM, N_("_End Process"), N_("Force a process to finish normally"),
	 cb_end_process, NULL, NULL, 0, NULL,
	 0, 0
	},
	{
 	  GNOME_APP_UI_ITEM, N_("_Kill Process"), N_("Force a process to finish immediately"),
	 cb_kill_process, NULL, NULL, 0, NULL,
	 0, 0
	},
	GNOMEUIINFO_SEPARATOR,
	{
 	  GNOME_APP_UI_ITEM, N_("_Change Priority..."), N_("Change the order of priority of process"),
	 cb_renice, NULL, NULL, 0, NULL,
	 0, 0
	},
	GNOMEUIINFO_SEPARATOR,
	{
 	  GNOME_APP_UI_ITEM, N_("_Hide Process"), N_("Hide process from list"),
	 cb_hide_process, NULL, NULL, 0, NULL,
	 0, 0
	},
	GNOMEUIINFO_SEPARATOR,
	{
 	  GNOME_APP_UI_ITEM, N_("_Memory Maps"), N_("Open the memory maps associated with the process"),
	 cb_show_memory_maps, NULL, NULL, 0, NULL,
	 0, 0
	},
	{
 	  GNOME_APP_UI_ITEM, N_("Open _Files"), N_("View the files opened by the process"),
	 popup_menu_show_open_files, NULL, NULL, 0, 0,
	 0, 0
	},
	GNOMEUIINFO_END
};


static GtkWidget *
create_proc_view (ProcData *procdata)
{
	GtkWidget *vbox1, *vbox2;
	GtkWidget *hbox1;
	GtkWidget *search_label;
	GtkWidget *search_entry;
	GtkWidget *proc_combo;
	GtkWidget *scrolled;
	GtkWidget *label;
	GtkWidget *hbox2;

	vbox1 = gtk_vbox_new (FALSE, 18);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 12);
	
	hbox1 = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);

	search_label = gtk_label_new_with_mnemonic (_("Sea_rch:"));
	gtk_box_pack_start (GTK_BOX (hbox1), search_label, FALSE, FALSE, 0);
	
	search_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox1), search_entry, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (search_entry), "activate",
			  G_CALLBACK (cb_search), procdata);
	gtk_label_set_mnemonic_widget (GTK_LABEL (search_label), search_entry);

	proc_combo = gtk_combo_box_new_text ();
	gtk_box_pack_end (GTK_BOX (hbox1), proc_combo, FALSE, FALSE, 0);

	gtk_combo_box_insert_text (GTK_COMBO_BOX (proc_combo), ALL_PROCESSES, _("All processes"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (proc_combo), MY_PROCESSES, _("My processes"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (proc_combo), ACTIVE_PROCESSES, _("Active processes"));

	gtk_combo_box_set_active (GTK_COMBO_BOX (proc_combo), procdata->config.whose_process);

	g_signal_connect (G_OBJECT (proc_combo), "changed",
			  G_CALLBACK (cb_proc_combo_changed), procdata);

  	label = gtk_label_new_with_mnemonic (_("Sho_w:"));
  	gtk_label_set_mnemonic_widget (GTK_LABEL (label), proc_combo);
	gtk_box_pack_end (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

	gtk_widget_show_all (hbox1);

	scrolled = proctable_new (procdata);
	if (!scrolled)
		return NULL;
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                             GTK_SHADOW_IN);

	gtk_box_pack_start (GTK_BOX (vbox1), scrolled, TRUE, TRUE, 0);
	
	gtk_widget_show_all (scrolled);
	
	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, FALSE, 0);

	infoview_create (procdata);
	gtk_box_pack_start (GTK_BOX (hbox2), procdata->infoview.expander, FALSE, FALSE, 0);
	gtk_expander_set_expanded (GTK_EXPANDER(procdata->infoview.expander),
				   procdata->config.show_more_info);
	gtk_widget_show (procdata->infoview.expander);
	

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox2), vbox2, FALSE, FALSE, 0);
	procdata->endprocessbutton = gtk_button_new_with_mnemonic (_("End _Process"));
	gtk_box_pack_end (GTK_BOX (vbox2), procdata->endprocessbutton, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (procdata->endprocessbutton), "clicked",
			  G_CALLBACK (cb_end_process_button_pressed), procdata);
	
	gtk_widget_show_all (hbox2);
	
	/* create popup_menu */
 	procdata->popup_menu = gtk_menu_new ();
 	gnome_app_fill_menu_with_data (GTK_MENU_SHELL (procdata->popup_menu), popup_menu_uiinfo,
  			               NULL, TRUE, 0, procdata);
	gtk_widget_show (procdata->popup_menu);

        return vbox1;
}

static int
sort_bytes (GtkTreeModel *model, GtkTreeIter *itera, GtkTreeIter *iterb, gpointer data)
{
	int col = GPOINTER_TO_INT (data);
	float btotal1, btotal2, bfree1, bfree2;

	btotal1 = btotal2 = bfree1 = bfree2 = 0.0f;
	gtk_tree_model_get (model, itera, 8, &btotal1, 9, &bfree1, -1);
	gtk_tree_model_get (model, iterb, 8, &btotal2, 9, &bfree2, -1);

	switch (col)
	{
		case 4:
			return PROCMAN_CMP(btotal1, btotal2);

		case 5:
			return PROCMAN_CMP(bfree1, bfree2);

		case 6:
			return PROCMAN_CMP(btotal1 - bfree1, btotal2 - bfree2);

		default:
			g_assert_not_reached();
			return 0;
	}
}

GtkWidget *
make_title_label (const char *text)
{
  GtkWidget *label;
  char *full;

  full = g_strdup_printf ("<span weight=\"bold\">%s</span>", text);
  label = gtk_label_new (full);
  g_free (full);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

  return label;
}

/* Make sure the cpu labels don't jump around. From the clock applet */
static void
cpu_size_request (GtkWidget *box, GtkRequisition *req, ProcData *procdata)
{
	req->width = procdata->cpu_label_fixed_width = \
		MAX(req->width, procdata->cpu_label_fixed_width);
}




static GtkWidget*
create_disk_view (ProcData *procdata)
{
	GtkWidget *disk_box, *disk_hbox;
	GtkWidget *label, *spacer;
	GtkWidget *scrolled;
	GtkWidget *disk_tree;
	GtkTreeStore *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;
	gint i;

	static const gchar *titles[] = {
	  N_("Device"),
	  N_("Directory"),
	  N_("Type"),
	  N_("Total"),
	  N_("Free"),
	  N_("Used"),
	};

	PROCMAN_GETTEXT_ARRAY_INIT(titles);

	disk_box = gtk_vbox_new (FALSE, 6);

	gtk_container_set_border_width (GTK_CONTAINER (disk_box), 6);

	label = make_title_label (_("Devices"));
	gtk_box_pack_start (GTK_BOX (disk_box), label, FALSE, FALSE, 0);

	disk_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (disk_box), disk_hbox, TRUE, TRUE, 0);

	spacer = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (disk_hbox), spacer, FALSE, FALSE, 0);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), 
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                             GTK_SHADOW_IN);

	gtk_box_pack_start (GTK_BOX (disk_hbox), scrolled, TRUE, TRUE, 0);


	model = gtk_tree_store_new (10,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, /* device name */
				    G_TYPE_STRING, /* directory */
				    G_TYPE_STRING, /* type */
				    G_TYPE_STRING, /* total */
				    G_TYPE_STRING, /* free */
				    G_TYPE_STRING, /* used */
				    G_TYPE_FLOAT,
				    G_TYPE_FLOAT,
				    G_TYPE_FLOAT);

	disk_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	procdata->disk_list = disk_tree;
	gtk_container_add (GTK_CONTAINER (scrolled), disk_tree);
  	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (disk_tree), TRUE);
  	g_object_unref (G_OBJECT (model));
  	
 	col = gtk_tree_view_column_new ();
  	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (col, cell, FALSE);
	gtk_tree_view_column_set_attributes (col, cell,
					     "pixbuf", 0,
					     NULL);
		
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, FALSE);
	gtk_tree_view_column_set_attributes (col, cell,
					     "text", 1,
					     NULL);
	gtk_tree_view_column_set_title (col, titles[0]);
	gtk_tree_view_column_set_sort_column_id (col, 1);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (disk_tree), col);
								
  	for (i = 1; i < 5	; i++) {
  		cell = gtk_cell_renderer_text_new ();
  		col = gtk_tree_view_column_new_with_attributes (titles[i],
						    		cell,
						     		"text", i + 1,
						     		NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_sort_column_id (col, i + 1);
		gtk_tree_view_append_column (GTK_TREE_VIEW (disk_tree), col);
	}
	
	col = gtk_tree_view_column_new ();	

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, FALSE);
	gtk_tree_view_column_set_attributes (col, cell,
					     "text", 6,
					     NULL);
	gtk_tree_view_column_set_title (col, titles[5]);
		
	
	cell = gtk_cell_renderer_progress_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	gtk_tree_view_column_set_attributes (col, cell,
					     "value", 7,
					     NULL);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW (disk_tree), col);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_sort_column_id (col, 6);
	
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 4, sort_bytes, GINT_TO_POINTER (4), NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 5, sort_bytes, GINT_TO_POINTER (5), NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 6, sort_bytes, GINT_TO_POINTER (6), NULL);


	gtk_widget_show_all (disk_box);
  	
  	procman_get_tree_state (procdata->client, disk_tree, "/apps/procman/disktreenew");
  	
  	cb_update_disks (procdata);
  	procdata->disk_timeout = gtk_timeout_add (procdata->config.disks_update_interval,
  						  cb_update_disks, procdata);


	return disk_box;
}







static GtkWidget *
create_sys_view (ProcData *procdata)
{
	GtkWidget *vbox, *hbox;
	GtkWidget *cpu_box, *mem_box;
	GtkWidget *cpu_hbox, *mem_hbox;
	GtkWidget *cpu_graph_box, *mem_graph_box;
	GtkWidget *label,*cpu_label, *spacer;
	GtkWidget *table;
	GtkWidget *color_picker;
	GtkSizeGroup *sizegroup;
	GdkColor color;
	LoadGraph *cpu_graph, *mem_graph;
	gint i;


	vbox = gtk_vbox_new (FALSE, 18);

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

	/* The CPU BOX */

	cpu_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), cpu_box, TRUE, TRUE, 0);

	label = make_title_label (_("CPU History"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (cpu_box), label, FALSE, FALSE, 0);
	
	cpu_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (cpu_box), cpu_hbox, TRUE, TRUE, 0);

	spacer = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (cpu_hbox), spacer, FALSE, FALSE, 0);

	cpu_graph_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (cpu_hbox), cpu_graph_box, TRUE, TRUE, 0);

	cpu_graph = load_graph_new (LOAD_GRAPH_CPU, procdata);
	gtk_box_pack_start (GTK_BOX (cpu_graph_box), cpu_graph->main_widget, TRUE, TRUE, 0);

	sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (cpu_graph_box), hbox, FALSE, FALSE, 0);

	for (i=0;i<procdata->config.num_cpus; i++) {
		GtkWidget *temp_hbox;
		gchar *text;
		
		temp_hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), temp_hbox, FALSE, FALSE, 0);
		gtk_size_group_add_widget (sizegroup, temp_hbox);
		g_signal_connect (G_OBJECT (temp_hbox), "size_request",
					 G_CALLBACK (cpu_size_request), procdata);

		color.red   = cpu_graph->colors[2+i].red;
		color.green = cpu_graph->colors[2+i].green;
		color.blue  = cpu_graph->colors[2+i].blue;
		color_picker = gtk_color_button_new_with_color (&color);

		g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_cpu_color_changed), GINT_TO_POINTER (i));
		gtk_box_pack_start (GTK_BOX (temp_hbox), color_picker, FALSE, FALSE, 0);
		
		text = g_strdup_printf (_("CPU%d:"), i+1);
		label = gtk_label_new (text);
		gtk_box_pack_start (GTK_BOX (temp_hbox), label, FALSE, FALSE, 0);
		g_free (text);
		
		cpu_label = gtk_label_new (NULL);
		gtk_box_pack_start (GTK_BOX (temp_hbox), cpu_label, FALSE, FALSE, 0);
		cpu_graph->cpu_labels[i] = cpu_label;
		
	}
	
	
	
	procdata->cpu_graph = cpu_graph;


	mem_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), mem_box, TRUE, TRUE, 0);

	label = make_title_label (_("Memory and Swap History"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (mem_box), label, FALSE, FALSE, 0);
	
	mem_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (mem_box), mem_hbox, TRUE, TRUE, 0);

	spacer = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (mem_hbox), spacer, FALSE, FALSE, 0);

	mem_graph_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (mem_hbox), mem_graph_box, TRUE, TRUE, 0);


	mem_graph = load_graph_new (LOAD_GRAPH_MEM, procdata);
	gtk_box_pack_start (GTK_BOX (mem_graph_box), mem_graph->main_widget, 
			    TRUE, TRUE, 0);
	
	table = gtk_table_new (2, 7, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_box_pack_start (GTK_BOX (mem_graph_box), table, 
			    FALSE, FALSE, 0);


	color.red   = mem_graph->colors[2].red;
	color.green = mem_graph->colors[2].green;
	color.blue  = mem_graph->colors[2].blue;
	color_picker = gtk_color_button_new_with_color (&color);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_mem_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 0, 1, 0, 1, 0, 0, 0, 0);
	
	label = gtk_label_new (_("User memory:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
	
	mem_graph->memused_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (mem_graph->memused_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->memused_label, 2, 3, 0, 1, 
			  GTK_FILL, 0, 0, 0);
	
	label = gtk_label_new (_("of"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);
	
	mem_graph->memtotal_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (mem_graph->memtotal_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->memtotal_label, 4, 5, 0, 1, 
			  GTK_FILL, 0, 0, 0);
	
	mem_graph->mempercent_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (mem_graph->mempercent_label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->mempercent_label, 5, 6, 0, 1, 
			  GTK_FILL, 0, 0, 0);
			  
	color.red   = mem_graph->colors[3].red;
	color.green = mem_graph->colors[3].green;
	color.blue  = mem_graph->colors[3].blue;
	color_picker = gtk_color_button_new_with_color (&color);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_swap_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 0, 1, 1, 2, 0, 0, 0, 0);
			  
	label = gtk_label_new (_("Used swap:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
	
	mem_graph->swapused_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (mem_graph->swapused_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->swapused_label, 2, 3, 1, 2, 
			  GTK_FILL, 0, 0, 0);
			  
	label = gtk_label_new (_("of"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);
	
	mem_graph->swaptotal_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (mem_graph->swaptotal_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->swaptotal_label, 4, 5, 1, 2, 
			  GTK_FILL, 0, 0, 0);	
	
	mem_graph->swappercent_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (mem_graph->swappercent_label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->swappercent_label, 5, 6, 1, 2, 
			  GTK_FILL, 0, 0, 0);
			  
	procdata->mem_graph = mem_graph;
	gtk_widget_show_all (vbox);

	return vbox;
}


void
create_main_window (ProcData *procdata)
{
	gint width, height;
	GtkWidget *app;
	GtkWidget *notebook;
	GtkWidget *tab_label1, *tab_label2, *tab_label3;
	GtkWidget *vbox1;
	GtkWidget *sys_box, *devices_box;
	GtkWidget *appbar1;
	
	app = gnome_app_new ("procman", _("System Monitor"));
	
	width = procdata->config.width;
	height = procdata->config.height;
	gtk_window_set_default_size (GTK_WINDOW (app), width, height);
	gtk_window_set_resizable (GTK_WINDOW (app), TRUE);
	
	gnome_app_create_menus_with_data (GNOME_APP (app), menubar1_uiinfo, procdata);
	
	notebook = gtk_notebook_new ();
	
	vbox1 = create_proc_view (procdata);
	tab_label1 = gtk_label_new (_("Processes"));
	gtk_widget_show (tab_label1);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox1, tab_label1);
	
	sys_box = create_sys_view (procdata);
	gtk_widget_show (sys_box);
	tab_label2 = gtk_label_new (_("Resources"));
	gtk_widget_show (tab_label2);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sys_box, tab_label2);

	devices_box = create_disk_view (procdata);
	gtk_widget_show(devices_box);
	tab_label3 = gtk_label_new (_("Devices"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), devices_box, tab_label3);


	appbar1 = gtk_statusbar_new();
	gnome_app_set_statusbar (GNOME_APP (app), appbar1);
	
	g_signal_connect (G_OBJECT (app), "delete_event",
                          G_CALLBACK (cb_app_delete),
                          procdata);
	
	gnome_app_install_menu_hints (GNOME_APP (app), menubar1_uiinfo);
		    
	g_signal_connect (G_OBJECT (notebook), "switch-page",
			    G_CALLBACK (cb_switch_page), procdata);

	gtk_widget_show (vbox1);
	gnome_app_set_contents (GNOME_APP (app), notebook);
	gtk_widget_show (notebook);

 	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), procdata->config.current_tab);
 	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (view1_menu_uiinfo[0].widget),
							  procdata->config.show_tree);
 	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (view1_menu_uiinfo[1].widget),
							  procdata->config.show_threads);
	
	procdata->app = app;

}

void
do_popup_menu (ProcData *procdata, GdkEventButton *event)
{
	gint button;
	gint event_time;

	if (event) {
		button = event->button;
		event_time = event->time;
	}
	else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (procdata->popup_menu), NULL, NULL,
			NULL, NULL, button, event_time);
}

void
update_sensitivity (ProcData *data, gboolean sensitivity)
{
	if(data->endprocessbutton) {
		/* avoid error on startup if endprocessbutton
		   has not been built yet */
		gtk_widget_set_sensitive (data->endprocessbutton, sensitivity);
	}

	gtk_widget_set_sensitive (data->infoview.box, sensitivity);
	/*Edit->End Process*/
	gtk_widget_set_sensitive (edit1_menu_uiinfo[0].widget, sensitivity);
	/*Edit->Kill Process*/
	gtk_widget_set_sensitive (edit1_menu_uiinfo[1].widget, sensitivity);
	/*Edit->Change Priority*/
	gtk_widget_set_sensitive (edit1_menu_uiinfo[3].widget, sensitivity);
	/*View->Hide Process*/
	gtk_widget_set_sensitive (view1_menu_uiinfo[3].widget, sensitivity);
	/*View->Hidden Processes*/
	gtk_widget_set_sensitive (view1_menu_uiinfo[4].widget, sensitivity);
	/*View->Memory Maps*/
	gtk_widget_set_sensitive (view1_menu_uiinfo[6].widget, sensitivity);
	/*View->Open Files*/
	gtk_widget_set_sensitive (view1_menu_uiinfo[7].widget, sensitivity);	
}

static void		
cb_toggle_tree (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	GtkCheckMenuItem *menu = GTK_CHECK_MENU_ITEM (view1_menu_uiinfo[0].widget);
	GConfClient *client = procdata->client;
	gboolean show;
	
	show = gtk_check_menu_item_get_active (menu);
	if (show == procdata->config.show_tree)
		return;
		
	gconf_client_set_bool (client, "/apps/procman/show_tree", show, NULL);
	
}

static void		
cb_toggle_threads (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	GtkCheckMenuItem *menu = GTK_CHECK_MENU_ITEM (view1_menu_uiinfo[1].widget);
	GConfClient *client = procdata->client;
	gboolean show;
	
	show = gtk_check_menu_item_get_active (menu);
	if (show == procdata->config.show_threads)
		return;
		
	gconf_client_set_bool (client, "/apps/procman/show_threads", show, NULL);
	
}
