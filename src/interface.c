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
#include "procman.h"
#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "infoview.h"
#include "procactions.h"
#if 0
#include "prettytable.h"
#include "procdialogs.h"
#include "memmaps.h"
#include "favorites.h"
#endif
#include "load-graph.h"

static GnomeUIInfo file1_menu_uiinfo[] =
{
	GNOMEUIINFO_MENU_EXIT_ITEM (cb_app_exit, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo edit1_menu_uiinfo[] =
{
 	{
 	  GNOME_APP_UI_ITEM, N_("_Change Priority..."), N_("Change the importance (nice value) of a process"),
	 cb_renice, NULL, NULL, 0, 0,
	 'r', GDK_CONTROL_MASK
	},
	{
	 GNOME_APP_UI_ITEM, N_("_Hide Process"), N_("Hide a process"),
	 cb_hide_process, NULL, NULL, 0, 0,
	 'h', GDK_CONTROL_MASK
	}, 
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("End _Process"), N_("Force a process to finish."),
	 cb_end_process, NULL, NULL, 0, 0,
	 'e', GDK_CONTROL_MASK
	},
	{
	 GNOME_APP_UI_ITEM, N_("_Kill Process"), N_("Force a process to finish now."),
	 cb_kill_process, NULL, NULL, 0, 0,
	 'k', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_END
};

static GnomeUIInfo view1_menu_uiinfo[] =
{
	{
	 GNOME_APP_UI_ITEM, N_("_Memory Maps"), N_("View the memory maps associated with a process"),
	 cb_show_memory_maps, NULL, NULL, 0, 0,
	 'm', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_END
};

static GnomeUIInfo settings1_menu_uiinfo[] =
{
	{
	 GNOME_APP_UI_ITEM, N_("_Hidden Processes"), N_("View and edit your list of hidden processes"),
	 cb_show_hidden_processes, NULL, NULL, 0, 0,
	 'p', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_MENU_PREFERENCES_ITEM (cb_preferences_activate, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo help1_menu_uiinfo[] =
{
	GNOMEUIINFO_MENU_ABOUT_ITEM (cb_about_activate, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo menubar1_uiinfo[] =
{
	GNOMEUIINFO_MENU_FILE_TREE (file1_menu_uiinfo),
	GNOMEUIINFO_MENU_EDIT_TREE (edit1_menu_uiinfo),
	GNOMEUIINFO_MENU_VIEW_TREE (view1_menu_uiinfo),
	GNOMEUIINFO_MENU_SETTINGS_TREE (settings1_menu_uiinfo),
	GNOMEUIINFO_MENU_HELP_TREE (help1_menu_uiinfo),
	GNOMEUIINFO_END
};

static GnomeUIInfo view_optionmenu[] = 
{
	{
 	  GNOME_APP_UI_ITEM, N_("All Processes"), N_("View processes being run by all users"),
	 cb_all_process_menu_clicked, NULL, NULL, 0, 0,
	 't', GDK_CONTROL_MASK
	},
	{
 	  GNOME_APP_UI_ITEM, N_("My Processes"), N_("View processes being run by you"),
	 cb_my_process_menu_clicked, NULL, NULL, 0, 0,
	 'p', GDK_CONTROL_MASK
	},
	{
 	  GNOME_APP_UI_ITEM, N_("Active Processes"), N_("View only active processes"),
	 cb_running_process_menu_clicked, NULL, NULL, 0, 0,
	 'o', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_END
};

static GnomeUIInfo popup_menu_uiinfo[] =
{
	{
 	  GNOME_APP_UI_ITEM, N_("Change Priority ..."), N_("Change the importance (nice value) of a process"),
	 popup_menu_renice, NULL, NULL, 0, 0,
	 0, 0
	},
	{
 	  GNOME_APP_UI_ITEM, N_("Memory Maps"), N_("View the memory maps associated with a process"),
	 popup_menu_show_memory_maps, NULL, NULL, 0, 0,
	 0, 0
	},
	GNOMEUIINFO_SEPARATOR,
	{
 	  GNOME_APP_UI_ITEM, N_("Hide Process"), N_("Hide a process"),
	 popup_menu_hide_process, NULL, NULL, 0, 0,
	 0, 0
	},
	GNOMEUIINFO_SEPARATOR,
	{
 	  GNOME_APP_UI_ITEM, N_("End Process"), N_("Force a process to finish"),
	 popup_menu_end_process, NULL, NULL, 0, 0,
	 0, 0
	},
	{
 	  GNOME_APP_UI_ITEM, N_("Kill Process"), N_("Force a process to finish now"),
	 popup_menu_kill_process, NULL, NULL, 0, 0,
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
	GtkWidget *optionmenu1;
	GtkWidget *optionmenu1_menu;
	GtkWidget *scrolled;
	GtkWidget *infobox;
	GtkWidget *label;
	GtkWidget *hbox2;
	GTimer *timer = g_timer_new ();
	
	vbox1 = gtk_vbox_new (FALSE, 0);
	
	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox1), GNOME_PAD_SMALL);
	
	search_label = gtk_label_new_with_mnemonic (_("Sea_rch :"));
	gtk_box_pack_start (GTK_BOX (hbox1), search_label, FALSE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (search_label), GNOME_PAD_SMALL, 0);
	
	search_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox1), search_entry, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (search_entry), "activate",
			  G_CALLBACK (cb_search), procdata);
	gtk_label_set_mnemonic_widget (GTK_LABEL (search_label), search_entry);
	
	g_timer_start (timer);
	optionmenu1 = gtk_option_menu_new ();
	gtk_box_pack_end (GTK_BOX (hbox1), optionmenu1, FALSE, FALSE, 0);
  	optionmenu1_menu = gtk_menu_new ();

  	gnome_app_fill_menu_with_data (GTK_MENU_SHELL (optionmenu1_menu), view_optionmenu,
  			               NULL, TRUE, 0, procdata);

	gtk_menu_set_active (GTK_MENU (optionmenu1_menu), procdata->config.whose_process);
  	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), optionmenu1_menu);
  	g_timer_stop (timer);
  	g_print ("optionmenu done %f \n", g_timer_elapsed (timer, NULL));
  	
  	label = gtk_label_new_with_mnemonic (_("Vie_w"));
  	gtk_label_set_mnemonic_widget (GTK_LABEL (label), optionmenu1);
	gtk_box_pack_end (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	
	gtk_widget_show_all (hbox1);
	
	g_timer_start (timer);
	scrolled = proctable_new (procdata);
	if (!scrolled)
		return NULL;
	
	gtk_box_pack_start (GTK_BOX (vbox1), scrolled, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled), GNOME_PAD_SMALL);
	g_timer_stop (timer);
	g_print ("table created %f \n", g_timer_elapsed (timer, NULL));
	
	gtk_widget_show_all (scrolled);
	
	g_timer_start (timer);
	infobox = infoview_create (procdata);
	gtk_box_pack_start (GTK_BOX (vbox1), infobox, FALSE, FALSE, 0);

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, FALSE, 0);
	
	endprocessbutton = gtk_button_new_with_mnemonic (_("End _Process"));
  	gtk_box_pack_end (GTK_BOX (hbox2), endprocessbutton, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (endprocessbutton), GNOME_PAD_SMALL);
	gtk_misc_set_padding (GTK_MISC (GTK_BIN (endprocessbutton)->child), 
			      GNOME_PAD_SMALL, 1);
	g_signal_connect (G_OBJECT (endprocessbutton), "clicked",
			  G_CALLBACK (cb_end_process_button_pressed), procdata);

	infolabel = gtk_label_new_with_mnemonic (_("More _Info"));
	infobutton = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (infobutton), infolabel);
	gtk_box_pack_start (GTK_BOX (hbox2), infobutton, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (infobutton), GNOME_PAD_SMALL);
	gtk_misc_set_padding (GTK_MISC (GTK_BIN (infobutton)->child), GNOME_PAD_SMALL, 1);
	g_signal_connect (G_OBJECT (infobutton), "clicked",
			  G_CALLBACK (cb_info_button_pressed), procdata);
	
	gtk_widget_show_all (hbox2);
	g_timer_stop (timer);
	g_print ("info bottm butonss new %f \n", g_timer_elapsed (timer, NULL));
	
	/* create popup_menu */
 	popup_menu = gtk_menu_new ();
 	gnome_app_fill_menu_with_data (GTK_MENU_SHELL (popup_menu), popup_menu_uiinfo,
  			               NULL, TRUE, 0, procdata);
	gtk_widget_show (popup_menu);
      
        return vbox1;
}


static GtkWidget *
create_sys_view (ProcData *procdata)
{
	GtkWidget *vbox;
	GtkWidget *vpane;
	GtkWidget *cpu_frame, *mem_frame;
	GtkWidget *label,*cpu_label;
	GtkWidget *hbox, *table;
	GtkWidget *disk_frame;
	GtkWidget *color_picker;
	GtkWidget *scrolled;
	GtkWidget *disk_tree;
	GtkTreeStore *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;
	gchar *titles[5] = {_("Name"),
			    _("Directory"),
			    _("Used Space"),
			    _("Total Space")
			    };
	LoadGraph *cpu_graph, *mem_graph;
	gint i;
	
	vpane = gtk_vpaned_new ();
	sys_pane = vpane;
	gtk_paned_set_position (GTK_PANED (vpane), procdata->config.pane_pos);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_paned_pack1 (GTK_PANED (vpane), vbox, TRUE, FALSE);
	
	cpu_frame = gtk_frame_new (_("% CPU Usage History"));
	gtk_widget_show (cpu_frame);
	gtk_container_set_border_width (GTK_CONTAINER (cpu_frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), cpu_frame, TRUE, TRUE, 0);
	
	mem_frame = gtk_frame_new (_("% Memory / Swap Usage History"));
	gtk_container_set_border_width (GTK_CONTAINER (mem_frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), mem_frame, TRUE, TRUE, 0);
	
	cpu_graph = load_graph_new (CPU_GRAPH, procdata);
	gtk_container_add (GTK_CONTAINER (cpu_frame), cpu_graph->main_widget);
	gtk_container_set_border_width (GTK_CONTAINER (cpu_graph->main_widget), 
					GNOME_PAD_SMALL);
					
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (cpu_graph->main_widget), hbox, FALSE, FALSE, 0);

	color_picker = gnome_color_picker_new ();
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (color_picker), 
				    cpu_graph->colors[2].red,
				    cpu_graph->colors[2].green,
				    cpu_graph->colors[2].blue, 0);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_cpu_color_changed), procdata);
	gtk_box_pack_start (GTK_BOX (hbox), color_picker, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("CPU Used :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	cpu_label = gtk_label_new ("");
	gtk_misc_set_padding (GTK_MISC (cpu_label), GNOME_PAD_SMALL, 0);
	gtk_box_pack_start (GTK_BOX (hbox), cpu_label, FALSE, FALSE, 0);
	cpu_graph->label = cpu_label;
	
	procdata->cpu_graph = cpu_graph;
	
	mem_graph = load_graph_new (MEM_GRAPH, procdata);
	gtk_container_add (GTK_CONTAINER (mem_frame), mem_graph->main_widget);
	gtk_container_set_border_width (GTK_CONTAINER (mem_graph->main_widget), 
					GNOME_PAD_SMALL);
	
	
	table = gtk_table_new (2, 6, FALSE);
	gtk_box_pack_start (GTK_BOX (mem_graph->main_widget), table, FALSE, FALSE, 0);
	
	color_picker = gnome_color_picker_new ();
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (color_picker), 
				    mem_graph->colors[2].red,
				    mem_graph->colors[2].green,
				    mem_graph->colors[2].blue, 0);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_mem_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 0, 1, 0, 1, 0, 0, 0, 0);
	
	label = gtk_label_new (_("Memory"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
	
	label = gtk_label_new (_("Used :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);
	
	mem_graph->memused_label = gtk_label_new ("");
	gtk_misc_set_padding (GTK_MISC (mem_graph->memused_label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (mem_graph->memused_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->memused_label, 3, 4, 0, 1, 
			  GTK_FILL, 0, 0, 0);
	
	label = gtk_label_new (_("Total :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 4, 5, 0, 1, GTK_FILL, 0, 0, 0);
	
	mem_graph->memtotal_label = gtk_label_new ("");
	gtk_misc_set_padding (GTK_MISC (mem_graph->memtotal_label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (mem_graph->memtotal_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->memtotal_label, 5, 6, 0, 1, 
			  GTK_FILL, 0, 0, 0);
	
	color_picker = gnome_color_picker_new ();
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (color_picker), 
				    mem_graph->colors[3].red,
				    mem_graph->colors[3].green,
				    mem_graph->colors[3].blue, 0);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_swap_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 0, 1, 1, 2, 0, 0, 0, 0);
			  
	label = gtk_label_new (_("Swap"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
	
	label = gtk_label_new (_("Used :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);
	
	mem_graph->swapused_label = gtk_label_new ("");
	gtk_misc_set_padding (GTK_MISC (mem_graph->swapused_label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (mem_graph->swapused_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->swapused_label, 3, 4, 1, 2, 
			  GTK_FILL, 0, 0, 0);
			  
	label = gtk_label_new (_("Total :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 4, 5, 1, 2, GTK_FILL, 0, 0, 0);
	
	mem_graph->swaptotal_label = gtk_label_new ("");
	gtk_misc_set_padding (GTK_MISC (mem_graph->swaptotal_label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (mem_graph->swaptotal_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), mem_graph->swaptotal_label, 5, 6, 1, 2, 
			  GTK_FILL, 0, 0, 0);	
	
	procdata->mem_graph = mem_graph;
	gtk_widget_show_all (vbox);
				
	disk_frame = gtk_frame_new (_("Devices"));
	gtk_container_set_border_width (GTK_CONTAINER (disk_frame), GNOME_PAD_SMALL);

	gtk_paned_pack2 (GTK_PANED (vpane), disk_frame, TRUE, TRUE);
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), 
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (disk_frame), scrolled);
	 
	model = gtk_tree_store_new (5, G_TYPE_STRING, G_TYPE_STRING,
				       G_TYPE_STRING, G_TYPE_STRING,
				       G_TYPE_STRING); 
				       
	disk_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	procdata->disk_list = disk_tree;
	gtk_container_add (GTK_CONTAINER (scrolled), disk_tree);
  	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (disk_tree), TRUE);
  	g_object_unref (G_OBJECT (model));
  	
  	for (i = 0; i < 4; i++) {
  		cell = gtk_cell_renderer_text_new ();
  		col = gtk_tree_view_column_new_with_attributes (titles[i],
						    		cell,
						     		"text", i,
						     		NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (disk_tree), col);
	}
  	
	gtk_widget_show_all (disk_frame);
  	
  	procman_get_tree_state (disk_tree, "/apps/procman/disktree");
  	
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
	GTimer *timer = g_timer_new ();

	g_timer_start (timer);
	app = gnome_app_new ("procman", _("Procman System Monitor"));
	
	g_timer_stop (timer);
	g_print ("app new %f \n", g_timer_elapsed (timer, NULL));

	width = procdata->config.width;
	height = procdata->config.height;
	gtk_window_set_default_size (GTK_WINDOW (app), width, height);
	gtk_window_set_policy (GTK_WINDOW (app), FALSE, TRUE, TRUE);
	
	g_timer_start (timer);
	gnome_app_create_menus_with_data (GNOME_APP (app), menubar1_uiinfo, procdata);
	g_timer_stop (timer);
	g_print ("menus new %f \n", g_timer_elapsed (timer, NULL));
	
	notebook = gtk_notebook_new ();
	gtk_container_set_border_width (GTK_CONTAINER (notebook), GNOME_PAD_SMALL);

	g_timer_start (timer);
	vbox1 = create_proc_view (procdata);
	tab_label1 = gtk_label_new_with_mnemonic (_("Process _Listing"));
	gtk_widget_show (tab_label1);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox1, tab_label1);
	g_timer_stop (timer);
	g_print ("proc view created %f \n", g_timer_elapsed (timer, NULL));
	
	g_timer_start (timer);
	sys_box = create_sys_view (procdata);
	gtk_widget_show (sys_box);
	tab_label2 = gtk_label_new_with_mnemonic (_("System _Monitor"));
	gtk_widget_show (tab_label2);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sys_box, tab_label2);
	g_timer_stop (timer);
	g_print ("sys view created %f \n", g_timer_elapsed (timer, NULL));
	
	g_timer_start (timer);
	appbar1 = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar (GNOME_APP (app), appbar1);
	g_timer_stop (timer);
	g_print ("appbar created %f \n", g_timer_elapsed (timer, NULL));
	
	g_signal_connect (G_OBJECT (app), "delete_event",
                          G_CALLBACK (cb_app_delete),
                          procdata);
	g_timer_start (timer);
	gnome_app_install_menu_hints (GNOME_APP (app), menubar1_uiinfo);
	g_timer_stop (timer);
	g_print ("menu hints %f \n", g_timer_elapsed (timer, NULL));
	g_timer_destroy (timer);
		    
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
	
	accel = gtk_accel_group_new ();
	gtk_accel_group_attach (accel, G_OBJECT (app));
	gtk_accel_group_unref (accel);
	
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

void do_popup_menu (ProcData *data, GdkEventButton *event)
{

	if (event->type == GDK_BUTTON_PRESS)
        {
                if (event->button == 3)
                {
                	gtk_menu_popup (GTK_MENU (popup_menu), NULL, NULL,
                                        NULL, NULL, event->button,
                                        event->time);
                }
        }
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

