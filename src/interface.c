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
#include <gnome.h>
#include <gdk/gdkkeysyms.h>
#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "infoview.h"
#include "prettytable.h"
#include "procdialogs.h"
#include "memmaps.h"
#include "favorites.h"
#include "procactions.h"
#include "load-graph.h"
#include <gal/e-table/e-tree-memory.h>
#include <gal/e-table/e-tree-memory-callbacks.h>
#include <gal/e-table/e-tree-scrolled.h>

static GnomeUIInfo file1_menu_uiinfo[] =
{
	GNOMEUIINFO_MENU_EXIT_ITEM (cb_app_exit, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo edit1_menu_uiinfo[] =
{
 	{
 	  GNOME_APP_UI_ITEM, N_("_Change Priority..."), N_("Change the priority of a process"),
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

#if 0
static GnomeUIInfo favorites1_menu_uiinfo[] =
{
	{
	 GNOME_APP_UI_ITEM, N_("Add to favorites"), NULL,
	 cb_add_to_favorites, NULL, NULL, 0, 0,
	 'a', 0
	},
	GNOMEUIINFO_END
};
#endif

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
#if 0
	{
		GNOME_APP_UI_SUBTREE,
		N_("F_avorites"),
      		NULL,
             	&favorites1_menu_uiinfo, NULL, NULL,
            	GNOME_APP_PIXMAP_NONE, NULL,
            	0, 0, NULL	
	},
#endif
	GNOMEUIINFO_MENU_SETTINGS_TREE (settings1_menu_uiinfo),
	GNOMEUIINFO_MENU_HELP_TREE (help1_menu_uiinfo),
	GNOMEUIINFO_END
};

gchar *moreinfolabel = N_("More _Info >>");
gchar *lessinfolabel = N_("<< Less _Info");

GtkWidget *infobutton;
GtkWidget *endprocessbutton;
GtkWidget *popup_menu;
GtkWidget *sys_pane;
GtkAccelGroup *accel;

gint
get_sys_pane_pos (void)
{
	return GTK_PANED (sys_pane)->child1_size;
}

static void
disk_list_resize (GtkCList *clist, gint column, gint width, gpointer data)
{
	ProcData *procdata = data;
	
	switch (column) {
	case 0:
		procdata->config.disk_width1 = width;
		break;
	case 1:
		procdata->config.disk_width2 = width;
		break;
	case 2:
		procdata->config.disk_width3 = width;
		break;
	case 3:
		procdata->config.disk_width4 = width;
		break;
	default:
		break;
	}
		
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
	GtkWidget *glade_menuitem;
	GtkWidget *scrolled;
	GtkWidget *infobox;
	GtkWidget *label;
	GtkWidget *hbox2;
	GtkWidget *lbl_hide;
        GtkWidget *lbl_kill;
        GtkWidget *lbl_renice;
	GtkWidget *lbl_mem_maps;
	GtkWidget *sep;
	GtkWidget *menuitem;
	guint key;	
	
	vbox1 = gtk_vbox_new (FALSE, 0);
	
	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox1), GNOME_PAD_SMALL);
	
	search_label = gtk_label_new (NULL);
	key = gtk_label_parse_uline (GTK_LABEL (search_label), _("Sea_rch :"));
	gtk_box_pack_start (GTK_BOX (hbox1), search_label, FALSE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (search_label), GNOME_PAD_SMALL, 0);
	
	search_entry = gtk_entry_new ();
	gtk_widget_add_accelerator (search_entry, "grab_focus",
				    accel,
				    key,
				    GDK_MOD1_MASK,
				    0);
				    
	gtk_box_pack_start (GTK_BOX (hbox1), search_entry, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (search_entry), "activate",
			    GTK_SIGNAL_FUNC (cb_search), procdata);

	optionmenu1 = gtk_option_menu_new ();
	gtk_box_pack_end (GTK_BOX (hbox1), optionmenu1, FALSE, FALSE, 0);
  	optionmenu1_menu = gtk_menu_new ();
  	
  	glade_menuitem = gtk_menu_item_new_with_label (_("All Processes"));
  	gtk_widget_show (glade_menuitem);
  	gtk_signal_connect (GTK_OBJECT (glade_menuitem), "activate",
  			    GTK_SIGNAL_FUNC (cb_all_process_menu_clicked), procdata);
  	gtk_menu_append (GTK_MENU (optionmenu1_menu), glade_menuitem);
  	
  	glade_menuitem = gtk_menu_item_new_with_label (_("My Processes"));
  	gtk_widget_show (glade_menuitem);
  	gtk_signal_connect (GTK_OBJECT (glade_menuitem), "activate",
  			    GTK_SIGNAL_FUNC (cb_my_process_menu_clicked), procdata);
  	gtk_menu_append (GTK_MENU (optionmenu1_menu), glade_menuitem);
  	
  	glade_menuitem = gtk_menu_item_new_with_label (_("Running Processes"));
  	gtk_widget_show (glade_menuitem);
  	gtk_signal_connect (GTK_OBJECT (glade_menuitem), "activate",
  			    GTK_SIGNAL_FUNC (cb_running_process_menu_clicked), procdata);
  	gtk_menu_append (GTK_MENU (optionmenu1_menu), glade_menuitem);
 	
  	gtk_menu_set_active (GTK_MENU (optionmenu1_menu), procdata->config.whose_process);
  	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), optionmenu1_menu);
  	
  	label = gtk_label_new (NULL);
	key = gtk_label_parse_uline (GTK_LABEL (label), _("Vie_w"));
	gtk_widget_add_accelerator (optionmenu1, "grab_focus",
				    accel,
				    key,
				    GDK_MOD1_MASK,
				    0);
	gtk_box_pack_end (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	
	gtk_widget_show_all (hbox1);
	
	scrolled = proctable_new (procdata);
	if (!scrolled)
		return NULL;
	/*gtk_widget_set_usize (scrolled, 400, 400);*/
	gtk_box_pack_start (GTK_BOX (vbox1), scrolled, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled), GNOME_PAD_SMALL);
	
	gtk_widget_show_all (scrolled);
	
	infobox = infoview_create (procdata);
	gtk_box_pack_start (GTK_BOX (vbox1), infobox, FALSE, FALSE, 0);
	
	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, FALSE, 0);
	
	endprocessbutton = gtk_button_new ();
  	label = gtk_label_new (NULL);
	key = gtk_label_parse_uline (GTK_LABEL (label), _("End _Process"));
	gtk_widget_add_accelerator (endprocessbutton, "clicked",
				    accel,
				    key,
				    GDK_MOD1_MASK,
				    0);
	gtk_container_add (GTK_CONTAINER (endprocessbutton), label);
	gtk_box_pack_start (GTK_BOX (hbox2), endprocessbutton, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (endprocessbutton), GNOME_PAD_SMALL);
	gtk_misc_set_padding (GTK_MISC (GTK_BIN (endprocessbutton)->child), 
			      GNOME_PAD_SMALL, 1);

	infobutton = gtk_button_new_with_label ("");
	
	gtk_box_pack_end (GTK_BOX (hbox2), infobutton, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (infobutton), GNOME_PAD_SMALL);
	gtk_misc_set_padding (GTK_MISC (GTK_BIN (infobutton)->child), GNOME_PAD_SMALL, 1);
	
	gtk_widget_show_all (hbox2);
	
	gtk_signal_connect (GTK_OBJECT (endprocessbutton), "clicked",
			    GTK_SIGNAL_FUNC (cb_end_process_button_pressed), procdata);
	gtk_signal_connect (GTK_OBJECT (infobutton), "clicked",
			    GTK_SIGNAL_FUNC (cb_info_button_pressed),
			    procdata);
			    
	/* create popup_menu */
 	popup_menu = gtk_menu_new ();

	/* Create new menu items */
	lbl_renice = gtk_menu_item_new_with_label (_("Change Priority ..."));
        gtk_widget_show (lbl_renice);
        gtk_signal_connect (GTK_OBJECT (lbl_renice),"activate",
                            GTK_SIGNAL_FUNC(popup_menu_renice),
                            procdata);
        gtk_menu_append (GTK_MENU (popup_menu), lbl_renice);
	lbl_mem_maps = gtk_menu_item_new_with_label (_("Memory Maps"));
	gtk_widget_show (lbl_mem_maps);
	gtk_signal_connect (GTK_OBJECT (lbl_mem_maps),"activate",
			    GTK_SIGNAL_FUNC(popup_menu_show_memory_maps),
			    procdata);
	gtk_menu_append (GTK_MENU (popup_menu), lbl_mem_maps);
	sep = gtk_menu_item_new();
	gtk_widget_show (sep);
	gtk_menu_append (GTK_MENU (popup_menu), sep);
	lbl_hide = gtk_menu_item_new_with_label (_("Hide Process"));
	gtk_widget_show (lbl_hide);
	gtk_signal_connect (GTK_OBJECT (lbl_hide),"activate",
                            GTK_SIGNAL_FUNC(popup_menu_hide_process),
                            procdata);
	gtk_menu_append (GTK_MENU (popup_menu), lbl_hide);
	sep = gtk_menu_item_new();
	gtk_widget_show (sep);
	gtk_menu_append (GTK_MENU (popup_menu), sep);
        lbl_kill = gtk_menu_item_new_with_label (_("End Process"));
        gtk_widget_show (lbl_kill);
        gtk_signal_connect (GTK_OBJECT (lbl_kill),"activate",
			    GTK_SIGNAL_FUNC(popup_menu_end_process),
			    procdata);
        gtk_menu_append (GTK_MENU (popup_menu), lbl_kill);
        menuitem = gtk_menu_item_new_with_label (_("Kill Process"));
        gtk_widget_show (menuitem);
        gtk_signal_connect (GTK_OBJECT (menuitem),"activate",
			    GTK_SIGNAL_FUNC(popup_menu_kill_process),
			    procdata);
        gtk_menu_append (GTK_MENU (popup_menu), menuitem);
        /*sep = gtk_menu_item_new();
	gtk_widget_show (sep);
	gtk_menu_append (GTK_MENU (popup_menu), sep);
        menuitem = gtk_menu_item_new_with_label (_("About This Process"));
        gtk_widget_show (menuitem);
        gtk_signal_connect (GTK_OBJECT (menuitem),"activate",
			    GTK_SIGNAL_FUNC(popup_menu_about_process),
			    procdata);
        gtk_menu_append (GTK_MENU (popup_menu), menuitem);*/
	
	/* Make the menu visible */
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
	GtkWidget *scrolled, *clist;
	gchar *titles[5] = {_("Name"),
			    _("Directory"),
			    _("Used Space"),
			    _("Total Space")
			    };
	LoadGraph *cpu_graph, *mem_graph;
	
	vpane = gtk_vpaned_new ();
	sys_pane = vpane;
	gtk_paned_set_position (GTK_PANED (vpane), procdata->config.pane_pos);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_paned_add1 (GTK_PANED (vpane), vbox);
	
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
	gtk_signal_connect (GTK_OBJECT (color_picker), "color_set",
			    GTK_SIGNAL_FUNC (cb_cpu_color_changed), procdata);
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
	gtk_signal_connect (GTK_OBJECT (color_picker), "color_set",
			    GTK_SIGNAL_FUNC (cb_mem_color_changed), procdata);
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
	gtk_signal_connect (GTK_OBJECT (color_picker), "color_set",
			    GTK_SIGNAL_FUNC (cb_swap_color_changed), procdata);
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
	gtk_paned_add2 (GTK_PANED (vpane), disk_frame);
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled), GNOME_PAD_SMALL);
  	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), 
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (disk_frame), scrolled);
	 
	clist = gtk_clist_new_with_titles (4, titles);
	gtk_container_add (GTK_CONTAINER (scrolled), clist);
  	
  	gtk_clist_set_column_width (GTK_CLIST (clist), 0, procdata->config.disk_width1);
  	gtk_clist_set_column_width (GTK_CLIST (clist), 1, procdata->config.disk_width2);
  	gtk_clist_set_column_width (GTK_CLIST (clist), 2, procdata->config.disk_width3);
  	gtk_clist_set_column_width (GTK_CLIST (clist), 3, procdata->config.disk_width4);
  	
  	gtk_clist_set_column_justification (GTK_CLIST (clist), 2, GTK_JUSTIFY_CENTER);
  	gtk_clist_set_column_justification (GTK_CLIST (clist), 3, GTK_JUSTIFY_CENTER);
  	
  	gtk_signal_connect (GTK_OBJECT (clist), "resize_column",
  			    GTK_SIGNAL_FUNC (disk_list_resize), procdata);
  	
  	gtk_widget_show_all (disk_frame);
  	
  	procdata->disk_clist = clist;	
	  	
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

	app = gnome_app_new ("procman", _("Procman System Monitor"));
	accel = gtk_accel_group_new ();
	gtk_accel_group_attach (accel, GTK_OBJECT (app));
	gtk_accel_group_unref (accel);

	width = procdata->config.width;
	height = procdata->config.height;
	gtk_window_set_default_size (GTK_WINDOW (app), width, height);
	gtk_window_set_policy (GTK_WINDOW (app), FALSE, TRUE, TRUE);
	
	gnome_app_create_menus_with_data (GNOME_APP (app), menubar1_uiinfo, procdata);
	
	notebook = gtk_notebook_new ();
	gtk_container_set_border_width (GTK_CONTAINER (notebook), GNOME_PAD_SMALL);
	
	vbox1 = create_proc_view (procdata);
	tab_label1 = gtk_label_new (_("Process Listing"));
	gtk_widget_show (tab_label1);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox1, tab_label1);
	
	sys_box = create_sys_view (procdata);
	gtk_widget_show (sys_box);
	tab_label2 = gtk_label_new (_("System Monitor"));
	gtk_widget_show (tab_label2);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sys_box, tab_label2);
	
	appbar1 = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar (GNOME_APP (app), appbar1);
	
	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
                            GTK_SIGNAL_FUNC (cb_app_delete),
                            procdata);
	gnome_app_install_menu_hints (GNOME_APP (app), menubar1_uiinfo);


        gtk_signal_connect (GTK_OBJECT (procdata->tree), "cursor_activated",
			    GTK_SIGNAL_FUNC (cb_table_selected), procdata);
	gtk_signal_connect (GTK_OBJECT (procdata->tree), "double_click",
			    GTK_SIGNAL_FUNC (cb_double_click), procdata);
        gtk_signal_connect (GTK_OBJECT (procdata->tree), "right_click",
                            GTK_SIGNAL_FUNC (cb_right_click), procdata);
	gtk_signal_connect (GTK_OBJECT (procdata->tree), "key_press",
			    GTK_SIGNAL_FUNC (cb_tree_key_press), procdata);
			    
	gtk_signal_connect (GTK_OBJECT (notebook), "switch-page",
			    GTK_SIGNAL_FUNC (cb_switch_page), procdata);
	
	if (procdata->config.current_tab == 0)		    
		procdata->timeout = gtk_timeout_add (procdata->config.update_interval,
			 			     cb_timeout, procdata);
	else {
		load_graph_start (procdata->cpu_graph);
		load_graph_start (procdata->mem_graph);
	}
	 	
	gtk_widget_show (vbox1);
	gnome_app_set_contents (GNOME_APP (app), notebook);
	
	/* Makes sure everything that should be insensitive is at start */
	gtk_signal_emit_by_name (GTK_OBJECT (procdata->tree), 
					 "cursor_activated",
					  -1, NULL);
					  
 	/* We cheat and force it to set up the labels */
 	procdata->config.show_more_info = !procdata->config.show_more_info;
 	toggle_infoview (procdata);	
 	
 	gtk_notebook_set_page (GTK_NOTEBOOK (notebook), procdata->config.current_tab);
 	
 	return app;

}

static void
simple_dialog_button_pressed (GnomeDialog *dialog, gint button, gpointer data)
{
	gnome_dialog_close (dialog);
}

GtkWidget*
create_simple_view_dialog (ProcData *procdata)
{
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
	gtk_accel_group_attach (accel, GTK_OBJECT (app));
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
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (cb_end_process_button_pressed), procdata);
	gtk_signal_connect (GTK_OBJECT (app), "clicked",
			    GTK_SIGNAL_FUNC (simple_dialog_button_pressed), procdata);
	gtk_signal_connect (GTK_OBJECT (app), "close",
			    GTK_SIGNAL_FUNC (cb_close_simple_dialog), procdata);
	gtk_signal_connect (GTK_OBJECT (procdata->tree), "cursor_activated",
			    GTK_SIGNAL_FUNC (cb_table_selected), procdata);
			    
	/* Makes sure everything that should be insensitive is at start */
	gtk_signal_emit_by_name (GTK_OBJECT (procdata->tree), "cursor_activated",
				 -1, NULL);
				 
	procdata->timeout = gtk_timeout_add (procdata->config.update_interval,
			 		     cb_timeout, procdata);
			 		    
	
			    
	gtk_widget_show_all (main_vbox);
	gtk_widget_show (app);
	
	return app;
}

void
toggle_infoview (ProcData *data)
{
	ProcData *procdata = data;
	GtkLabel *label;
	static guint more_key = 0;
	static guint less_key = 0;

	label = GTK_LABEL (GTK_BUTTON (infobutton)->child);
		
	if (procdata->config.show_more_info == FALSE)
	{
		infoview_update (procdata);
		gtk_widget_show_all (procdata->infobox);
		procdata->config.show_more_info = TRUE;	
 		less_key = gtk_label_parse_uline(GTK_LABEL (label),
 						 _(lessinfolabel));
 		if (more_key != 0)
 			gtk_widget_remove_accelerator (infobutton,
 						       accel,
 						       more_key,
 						       GDK_MOD1_MASK);
 		gtk_widget_add_accelerator (infobutton, "clicked",
 					    accel,
 					    less_key, GDK_MOD1_MASK,
 					    0);
		
	}			
	else
	{
		gtk_widget_hide (procdata->infobox);
		procdata->config.show_more_info = FALSE;
 		more_key = gtk_label_parse_uline(GTK_LABEL (label),
 						 _(moreinfolabel));
 		if (less_key != 0)
 			gtk_widget_remove_accelerator (infobutton,
 						       accel,
 						       less_key,
 						       GDK_MOD1_MASK);
 		gtk_widget_add_accelerator (infobutton, "clicked",
 					    accel,
 					    more_key, GDK_MOD1_MASK,
 					    0);		
	}
}

void do_popup_menu (ProcData *data, GdkEvent *event)
{

	/* Define all variables */
        GdkEventButton *event_button;

	/* Make the menu visible one right-button click*/
        if (event->type == GDK_BUTTON_PRESS)
        {
                event_button = (GdkEventButton *) event;
                if (event_button->button == 3)
                {
                        gtk_menu_popup (GTK_MENU (popup_menu), NULL, NULL,
                                        NULL, NULL, event_button->button,
                                        event_button->time);
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
		gtk_widget_set_sensitive (edit1_menu_uiinfo[3].widget, sensitivity);
		gtk_widget_set_sensitive (edit1_menu_uiinfo[4].widget, sensitivity);
	}
}	

