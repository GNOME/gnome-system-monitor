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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

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
#include "cellrenderer.h"

void	cb_toggle_tree (GtkMenuItem *menuitem, gpointer data);
void	cb_toggle_threads (GtkMenuItem *menuitem, gpointer data);


static GnomeUIInfo file1_menu_uiinfo[] =
{
	GNOMEUIINFO_MENU_QUIT_ITEM (cb_app_exit, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo edit1_menu_uiinfo[] =
{
 	{
 	  GNOME_APP_UI_ITEM, N_("_Change Priority..."), N_("Change the importance (nice value) of a process"),
	 cb_renice, NULL, NULL, 0, NULL,
	 'r', GDK_CONTROL_MASK
	},
	{
	 GNOME_APP_UI_ITEM, N_("_Hide Process"), N_("Hide a process"),
	 cb_hide_process, NULL, NULL, 0, NULL,
	 'h', GDK_CONTROL_MASK
	}, 
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("End _Process"), N_("Force a process to finish."),
	 cb_end_process, NULL, NULL, 0, NULL,
	 'e', GDK_CONTROL_MASK
	},
	{
	 GNOME_APP_UI_ITEM, N_("_Kill Process"), N_("Force a process to finish now."),
	 cb_kill_process, NULL, NULL, 0, NULL,
	 'k', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Hidden Processes"), 
	 		    N_("View and edit your list of hidden processes"),
	 cb_show_hidden_processes, NULL, NULL, 0, NULL,
	 'p', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_MENU_PREFERENCES_ITEM (cb_preferences_activate, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo view1_menu_uiinfo[] =
{
	{
	 GNOME_APP_UI_ITEM, N_("_Memory Maps"), N_("View the memory maps associated with a process"),
	 cb_show_memory_maps, NULL, NULL, 0, NULL,
	 'm', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("Process _Dependencies"), N_("Display a tree showing process dependencies"),
	 cb_toggle_tree, NULL, NULL, 0, NULL,
	 'd', GDK_CONTROL_MASK
	}, 
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Threads"), N_("Display threads (subprocesses)"),
	 cb_toggle_threads, NULL, NULL, 0, NULL,
	 't', GDK_CONTROL_MASK
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
 	  GNOME_APP_UI_ITEM, N_("_Change Priority..."), N_("Change the importance (nice value) of a process"),
	 popup_menu_renice, NULL, NULL, 0, NULL,
	 0, 0
	},
	{
 	  GNOME_APP_UI_ITEM, N_("_Memory Maps"), N_("View the memory maps associated with a process"),
	 popup_menu_show_memory_maps, NULL, NULL, 0, NULL,
	 0, 0
	},
	GNOMEUIINFO_SEPARATOR,
	{
 	  GNOME_APP_UI_ITEM, N_("_Hide Process"), N_("Hide a process"),
	 popup_menu_hide_process, NULL, NULL, 0, NULL,
	 0, 0
	},
	GNOMEUIINFO_SEPARATOR,
	{
 	  GNOME_APP_UI_ITEM, N_("_End Process"), N_("Force a process to finish"),
	 popup_menu_end_process, NULL, NULL, 0, NULL,
	 0, 0
	},
	{
 	  GNOME_APP_UI_ITEM, N_("_Kill Process"), N_("Force a process to finish now"),
	 popup_menu_kill_process, NULL, NULL, 0, NULL,
	 0, 0
	},
	GNOMEUIINFO_END
};

gchar *moreinfolabel = N_("More _Info >>");
gchar *lessinfolabel = N_("<< Less _Info");

GtkWidget *infobutton;
GtkWidget *infolabel;
GtkWidget *endprocessbutton;
GtkWidget *popup_menu;
GtkWidget *sys_pane;

gint
get_sys_pane_pos (void)
{
	return GTK_PANED (sys_pane)->child1_size;
}

static GtkWidget *
create_proc_view (ProcData *procdata)
{
	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GtkWidget *search_label;
	GtkWidget *search_entry;
	GtkWidget *proc_combo;
	GtkWidget *scrolled;
	GtkWidget *infobox;
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

	gtk_combo_box_insert_text (GTK_COMBO_BOX (proc_combo), ALL_PROCESSES, _("All Processes"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (proc_combo), MY_PROCESSES, _("My Processes"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (proc_combo), ACTIVE_PROCESSES, _("Active Processes"));

	gtk_combo_box_set_active (GTK_COMBO_BOX (proc_combo), procdata->config.whose_process);

	g_signal_connect (G_OBJECT (proc_combo), "changed",
			  G_CALLBACK (cb_proc_combo_changed), procdata);

  	label = gtk_label_new_with_mnemonic (_("Vie_w:"));
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
	
	infobox = infoview_create (procdata);
	gtk_box_pack_start (GTK_BOX (vbox1), infobox, FALSE, FALSE, 0);

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, FALSE, 0);
	
	endprocessbutton = gtk_button_new_with_mnemonic (_("End _Process"));
  	gtk_box_pack_end (GTK_BOX (hbox2), endprocessbutton, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (endprocessbutton), "clicked",
			  G_CALLBACK (cb_end_process_button_pressed), procdata);

	infolabel = gtk_label_new_with_mnemonic (_("More _Info"));
	infobutton = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (infobutton), infolabel);
	gtk_box_pack_start (GTK_BOX (hbox2), infobutton, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (infobutton), "clicked",
			  G_CALLBACK (cb_info_button_pressed), procdata);
	
	gtk_widget_show_all (hbox2);
	
	/* create popup_menu */
 	popup_menu = gtk_menu_new ();
 	gnome_app_fill_menu_with_data (GTK_MENU_SHELL (popup_menu), popup_menu_uiinfo,
  			               NULL, TRUE, 0, procdata);
	gtk_widget_show (popup_menu);

        return vbox1;
}

static int
sort_bytes (GtkTreeModel *model, GtkTreeIter *itera, GtkTreeIter *iterb, gpointer data)
{
	int col = GPOINTER_TO_INT (data);
	float btotal1, btotal2, bfree1, bfree2;
	float a, b;
	
	btotal1 = btotal2 = bfree1 = bfree2 = 0.0;
	gtk_tree_model_get (model, itera, 7, &btotal1, 8, &bfree1, -1);
	gtk_tree_model_get (model, iterb, 7, &btotal2, 8, &bfree2, -1);

	switch (col)
	{
		case 4: 
			a = btotal1;
			b = btotal2;
			break;
		case 5: 
			a = btotal1 - bfree1;
			b = btotal2 - bfree2;
			break;
		case 6: 
			a = bfree1;
			b = bfree2;
			break;
		default:
			a = 0;
			b = 0;
	}

	if (a > b)
		return -1;
	else if (a < b)
		return 1;
	return 0;	
}

static GtkWidget *
make_title_label (const char *text)
{
  GtkWidget *label;
  char *full;

  full = g_strdup_printf ("<span weight=\"bold\">%s</span>", text);
  label = gtk_label_new (full);
  g_free (full);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

  return label;
}

/* Make sure the cpu labels don't jump around. From the clock applet */
static void
cpu_size_request (GtkWidget *box, GtkRequisition *req, ProcData *procdata)
{
	if (req->width > procdata->cpu_label_fixed_width)
		procdata->cpu_label_fixed_width = req->width;
		
	req->width = procdata->cpu_label_fixed_width;
	
}

static GtkWidget *
create_sys_view (ProcData *procdata)
{
	GtkWidget *vbox;
	GtkWidget *vpane;
	GtkWidget *cpu_box, *mem_box, *disk_box;
	GtkWidget *cpu_hbox, *mem_hbox, *disk_hbox;
	GtkWidget *cpu_graph_box, *mem_graph_box;
	GtkWidget *label,*cpu_label, *spacer;
	GtkWidget *hbox, *table;
	GtkWidget *color_picker;
	GtkWidget *scrolled;
	GtkWidget *disk_tree;
	GtkTreeStore *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;
	GtkSizeGroup *sizegroup;
	gchar *titles[5] = {_("Name"),
			    _("Directory"),
				_("Type"),
			    _("Total"),
			    _("Used"),
				};
	LoadGraph *cpu_graph, *mem_graph;
	gint i;
	
	vpane = gtk_vpaned_new ();
	sys_pane = vpane;
	gtk_paned_set_position (GTK_PANED (vpane), procdata->config.pane_pos);
	
	gtk_container_set_border_width (GTK_CONTAINER (vpane), 6);

	vbox = gtk_vbox_new (FALSE, 18);
	gtk_paned_pack1 (GTK_PANED (vpane), vbox, TRUE, FALSE);

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

	cpu_graph = load_graph_new (CPU_GRAPH, procdata);
	gtk_box_pack_start (GTK_BOX (cpu_graph_box), cpu_graph->main_widget, TRUE, TRUE, 0);

	sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	for (i=0;i<procdata->config.num_cpus; i++) {
		GtkWidget *temp_hbox;
		gchar *text;
		/* Two per row */
		if (fabs(fmod(i,2) - 0) < .01) {
			hbox = gtk_hbox_new (FALSE, 12);
			gtk_box_pack_start (GTK_BOX (cpu_graph_box), hbox, FALSE, FALSE, 0);
		}
		
		temp_hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), temp_hbox, FALSE, FALSE, 0);
		gtk_size_group_add_widget (sizegroup, temp_hbox);
		g_signal_connect (G_OBJECT (temp_hbox), "size_request",
					 G_CALLBACK (cpu_size_request), procdata);
		
		color_picker = gnome_color_picker_new ();
		gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (color_picker), 
				    cpu_graph->colors[2+i].red,
				    cpu_graph->colors[2+i].green,
				    cpu_graph->colors[2+i].blue, 0);
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


	mem_graph = load_graph_new (MEM_GRAPH, procdata);
	gtk_box_pack_start (GTK_BOX (mem_graph_box), mem_graph->main_widget, 
			    TRUE, TRUE, 0);
	
	table = gtk_table_new (2, 6, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_box_pack_start (GTK_BOX (mem_graph_box), table, 
			    FALSE, FALSE, 0);

	color_picker = gnome_color_picker_new ();
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (color_picker), 
				    mem_graph->colors[2].red,
				    mem_graph->colors[2].green,
				    mem_graph->colors[2].blue, 0);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_mem_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 0, 1, 0, 1, 0, 0, 0, 0);
	
	label = gtk_label_new (_("Used memory:"));
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
	
	color_picker = gnome_color_picker_new ();
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (color_picker), 
				    mem_graph->colors[3].red,
				    mem_graph->colors[3].green,
				    mem_graph->colors[3].blue, 0);
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
	
	procdata->mem_graph = mem_graph;
	gtk_widget_show_all (vbox);


	/*****************************************/

	disk_box = gtk_vbox_new (FALSE, 6);

	gtk_container_set_border_width (GTK_CONTAINER (disk_box), 6);

	gtk_paned_pack2 (GTK_PANED (vpane), disk_box, TRUE, TRUE);

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

	 
	model = gtk_tree_store_new (9, GDK_TYPE_PIXBUF, G_TYPE_STRING, 
					   G_TYPE_STRING, G_TYPE_STRING,
				       G_TYPE_STRING, G_TYPE_STRING,
					G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT);
				       
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
								
  	for (i = 1; i < 4	; i++) {
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
					     "text", 5,
					     NULL);
	gtk_tree_view_column_set_title (col, _(titles[4]));
		
	
	cell = procman_cell_renderer_progress_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	gtk_tree_view_column_set_attributes (col, cell,
					     "value", 6,
					     NULL);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW (disk_tree), col);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_sort_column_id (col, 5);
	
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 4, sort_bytes, GINT_TO_POINTER (4), NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 5, sort_bytes, GINT_TO_POINTER (5), NULL);
	
	gtk_widget_show_all (disk_box);
  	
  	procman_get_tree_state (procdata->client, disk_tree, "/apps/procman/disktreenew");
  	
  	cb_update_disks (procdata);
  	procdata->disk_timeout = gtk_timeout_add (procdata->config.disks_update_interval,
  						  cb_update_disks, procdata);


	return vpane;
}

GtkWidget*
create_main_window (ProcData *procdata)
{
	gint width, height;
	GtkWidget *app;
	GtkWidget *notebook;
	GtkWidget *tab_label1, *tab_label2;
	GtkWidget *vbox1;
	GtkWidget *sys_box;
	GtkWidget *appbar1;
	
	app = gnome_app_new ("procman", _("System Monitor"));
	
	width = procdata->config.width;
	height = procdata->config.height;
	gtk_window_set_default_size (GTK_WINDOW (app), width, height);
	gtk_window_set_resizable (GTK_WINDOW (app), TRUE);
	
	gnome_app_create_menus_with_data (GNOME_APP (app), menubar1_uiinfo, procdata);
	
	notebook = gtk_notebook_new ();
	
	vbox1 = create_proc_view (procdata);
	tab_label1 = gtk_label_new (_("Process Listing"));
	gtk_widget_show (tab_label1);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox1, tab_label1);
	
	sys_box = create_sys_view (procdata);
	gtk_widget_show (sys_box);
	tab_label2 = gtk_label_new (_("Resource Monitor"));
	gtk_widget_show (tab_label2);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sys_box, tab_label2);
	
	appbar1 = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar (GNOME_APP (app), appbar1);
	
	g_signal_connect (G_OBJECT (app), "delete_event",
                          G_CALLBACK (cb_app_delete),
                          procdata);
	
	gnome_app_install_menu_hints (GNOME_APP (app), menubar1_uiinfo);
		    
	g_signal_connect (G_OBJECT (notebook), "switch-page",
			    G_CALLBACK (cb_switch_page), procdata);

	if (procdata->config.current_tab == 0)
		procdata->timeout = gtk_timeout_add (procdata->config.update_interval,
			 		     cb_timeout, procdata);
	else {
		load_graph_start (procdata->cpu_graph);
		load_graph_start (procdata->mem_graph);
	}
	 	
	gtk_widget_show (vbox1);
	gnome_app_set_contents (GNOME_APP (app), notebook);
	gtk_widget_show (notebook);

	update_sensitivity (procdata, FALSE);
	
	/* We cheat and force it to set up the labels */
 	procdata->config.show_more_info = !procdata->config.show_more_info;
 	toggle_infoview (procdata);

 	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), procdata->config.current_tab);
 	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (view1_menu_uiinfo[2].widget),
							  procdata->config.show_tree);
 	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (view1_menu_uiinfo[3].widget),
							  procdata->config.show_threads);
	
 	return app;

}

GtkWidget*
create_simple_view_dialog (ProcData *procdata)
{
#if 0
	GtkWidget *app = NULL;
	GtkWidget *main_vbox;
	GtkWidget *scrolled;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *frame;
	guint key;
	
	app = gnome_dialog_new (_("Application Manager"), GNOME_STOCK_BUTTON_CANCEL, NULL);
	gtk_window_set_policy (GTK_WINDOW (app), FALSE, TRUE, FALSE);
	gtk_window_set_default_size (GTK_WINDOW (app), 350, 425);
	
	main_vbox = GNOME_DIALOG (app)->vbox;
	
	frame = gtk_frame_new (_("Running Applications"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
	
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	scrolled = proctable_new (procdata);
	if (!scrolled)
		return NULL;
	gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	button = gtk_button_new ();
	endprocessbutton = button;
  	label = gtk_label_new (NULL);
	key = gtk_label_parse_uline (GTK_LABEL (label), _("_Close Application"));
	gtk_widget_add_accelerator (button, "clicked",
				    accel,
				    key,
				    GDK_MOD1_MASK,
				    0);
	gtk_container_add (GTK_CONTAINER (button), label);
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button), GNOME_PAD_SMALL);
	gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 
			      GNOME_PAD_SMALL, 1);
	g_signal_connect (G_OBJECT (button), "clicked",
			    G_CALLBACK (cb_end_process_button_pressed), procdata);

	g_signal_connect (G_OBJECT (app), "close",
			    G_CALLBACK (cb_close_simple_dialog), procdata);
	g_signal_connect (G_OBJECT (procdata->tree), "cursor_activated",
			    G_CALLBACK (cb_table_selected), procdata);
			    
	/* Makes sure everything that should be insensitive is at start */
	gtk_signal_emit_by_name (G_OBJECT (procdata->tree), "cursor_activated",
				 -1, NULL);
				 
	procdata->timeout = gtk_timeout_add (procdata->config.update_interval,
			 		     cb_timeout, procdata);
			 		    
	
			    
	gtk_widget_show_all (main_vbox);
	gtk_widget_show (app);
	
	return app;
#endif
	return NULL;
}

void
toggle_infoview (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *label;
	
	label = infolabel;
		
	if (procdata->config.show_more_info == FALSE)
	{
		infoview_update (procdata);
		gtk_widget_show_all (procdata->infobox);
		procdata->config.show_more_info = TRUE;	
		gtk_label_set_text_with_mnemonic (GTK_LABEL (label),_(lessinfolabel)); 
 				
	}			
	else
	{
		gtk_widget_hide (procdata->infobox);
		procdata->config.show_more_info = FALSE;
 		gtk_label_set_text_with_mnemonic (GTK_LABEL (label),_(moreinfolabel)); 
	}
}

void
do_popup_menu (ProcData *data, GdkEventButton *event)
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

	gtk_menu_popup (GTK_MENU (popup_menu), NULL, NULL,
			NULL, NULL, button, event_time);
}

void
update_sensitivity (ProcData *data, gboolean sensitivity)
{
	gtk_widget_set_sensitive (endprocessbutton, sensitivity);
	
	if (!data->config.simple_view) {
		gtk_widget_set_sensitive (data->infobox, sensitivity);
		gtk_widget_set_sensitive (edit1_menu_uiinfo[0].widget, sensitivity);
		gtk_widget_set_sensitive (edit1_menu_uiinfo[1].widget, sensitivity);
		gtk_widget_set_sensitive (view1_menu_uiinfo[0].widget, sensitivity);
		gtk_widget_set_sensitive (edit1_menu_uiinfo[2].widget, sensitivity);
		gtk_widget_set_sensitive (edit1_menu_uiinfo[3].widget, sensitivity);
		gtk_widget_set_sensitive (edit1_menu_uiinfo[4].widget, sensitivity);
	}
}

void		
cb_toggle_tree (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	GtkCheckMenuItem *menu = GTK_CHECK_MENU_ITEM (view1_menu_uiinfo[2].widget);
	GConfClient *client = procdata->client;
	gboolean show;
	
	show = gtk_check_menu_item_get_active (menu);
	if (show == procdata->config.show_tree)
		return;
		
	gconf_client_set_bool (client, "/apps/procman/show_tree", show, NULL);
	
}

void		
cb_toggle_threads (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	GtkCheckMenuItem *menu = GTK_CHECK_MENU_ITEM (view1_menu_uiinfo[3].widget);
	GConfClient *client = procdata->client;
	gboolean show;
	
	show = gtk_check_menu_item_get_active (menu);
	if (show == procdata->config.show_threads)
		return;
		
	gconf_client_set_bool (client, "/apps/procman/show_threads", show, NULL);
	
}
