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

static GnomeUIInfo file1_menu_uiinfo[] =
{
	GNOMEUIINFO_MENU_EXIT_ITEM (cb_app_destroy, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo edit1_menu_uiinfo[] =
{
 	{
 	  GNOME_APP_UI_ITEM, N_("Renice..."), "",
	 cb_renice, NULL, NULL, 0, 0,
	 'r', GDK_CONTROL_MASK
	},
	{
	 GNOME_APP_UI_ITEM, N_("Hide Process"), "",
	 cb_hide_process, NULL, NULL, 0, 0,
	 'h', GDK_CONTROL_MASK
	}, 
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Hidden Processes..."), "",
	 cb_show_hidden_processes, NULL, NULL, 0, 0,
	 'p', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_END
};

static GnomeUIInfo view1_menu_uiinfo[] =
{
	{
	 GNOME_APP_UI_ITEM, N_("Memory Maps..."), "",
	 cb_show_memory_maps, NULL, NULL, 0, 0,
	 'm', GDK_CONTROL_MASK
	},
	GNOMEUIINFO_END
};

#if 0
static GnomeUIInfo favorites1_menu_uiinfo[] =
{
	{
	 GNOME_APP_UI_ITEM, N_("Add to favorites"), "",
	 cb_add_to_favorites, NULL, NULL, 0, 0,
	 'a', 0
	},
	GNOMEUIINFO_END
};
#endif

static GnomeUIInfo settings1_menu_uiinfo[] =
{
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
GtkAccelGroup *accel;

void
popup_menu_renice (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	procdialog_create_renice_dialog (procdata);
	
	gtk_menu_popdown (GTK_MENU (popup_menu));
}

void
popup_menu_show_memory_maps (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	
	create_memmaps_dialog (procdata);
	
	gtk_menu_popdown (GTK_MENU (popup_menu));
}

void
popup_menu_hide_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	
	if (!procdata->selected_node)
		return;
	
	if (procdata->config.show_hide_message)
	{	
		GtkWidget *dialog;
		dialog = procdialog_create_hide_dialog (procdata);
		gtk_widget_show (dialog);
	}
	else
	{
		info = e_tree_memory_node_get_data (procdata->memory, 
						    procdata->selected_node);
		add_to_blacklist (procdata, info->cmd);
		proctable_update_all (procdata);
	}
	
	gtk_menu_popdown (GTK_MENU (popup_menu));
	
}

void popup_menu_kill_process (GtkMenuItem *menuitem, gpointer data)
{
	ProcData *procdata = data;
        ProcInfo *info;

        if (!procdata->selected_node)
		return;
	
	else
	{
		info = e_tree_memory_node_get_data (procdata->memory, 
						    procdata->selected_node);
		kill (info->pid, SIGKILL);
		proctable_update_all (procdata);
	}
	
	gtk_menu_popdown (GTK_MENU (popup_menu));	
}

GtkWidget*
create_main_window (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *app;
	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GtkWidget *search_label;
	GtkWidget *search_entry;
	GtkWidget *optionmenu1;
	GtkWidget *optionmenu1_menu;
	GtkWidget *glade_menuitem;
	GtkWidget *scrolled;
	GtkWidget *label;
	GtkWidget *hbox2;
	GtkWidget *appbar1;
	GtkWidget *status_hbox;
	GtkWidget *status_frame;
	GtkWidget *infobox;
	GtkWidget *meter_hbox;
	GtkWidget *cpumeter, *memmeter, *swapmeter;
	GtkWidget *lbl_hide;
        GtkWidget *lbl_kill;
        GtkWidget *lbl_renice;
	GtkWidget *lbl_mem_maps;
	GtkWidget *sep, *an_sep;
	guint key;

	app = gnome_app_new ("procman", NULL);
	accel = gtk_accel_group_new ();
	gtk_accel_group_attach (accel, GTK_OBJECT (app));
	gtk_accel_group_unref (accel);

	gtk_window_set_default_size (GTK_WINDOW (app), 460, 475);
	gtk_window_set_policy (GTK_WINDOW (app), FALSE, TRUE, TRUE);

	gnome_app_create_menus_with_data (GNOME_APP (app), menubar1_uiinfo, procdata);
	
	vbox1 = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), vbox1);

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
#if 0  	
  	glade_menuitem = gtk_menu_item_new_with_label (_("Favorites"));
  	gtk_widget_show (glade_menuitem);
  	gtk_signal_connect (GTK_OBJECT (glade_menuitem), "activate",
  			    GTK_SIGNAL_FUNC (cb_favorites_menu_clicked), procdata);
  	gtk_menu_append (GTK_MENU (optionmenu1_menu), glade_menuitem);
#endif  	
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

	/* This is just to get rid of some GNome-UI criticals - just
	** hide the appbar
	*/
	appbar1 = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar (GNOME_APP (app), appbar1);
	gtk_widget_hide (appbar1);
	
	status_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox1), status_hbox, FALSE, FALSE, 0);
	
	status_frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (status_hbox), status_frame, TRUE, TRUE, 0);
	meter_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (meter_hbox), 1);
	gtk_container_add (GTK_CONTAINER (status_frame), meter_hbox);
	label = gtk_label_new (_("CPU :"));
	gtk_box_pack_start (GTK_BOX (meter_hbox), label, FALSE, FALSE, GNOME_PAD_SMALL);
	cpumeter = gtk_progress_bar_new ();
	gtk_widget_set_usize (cpumeter, 60, -2);
	gtk_box_pack_start (GTK_BOX (meter_hbox), cpumeter, TRUE, TRUE, GNOME_PAD_SMALL);
	procdata->cpumeter = cpumeter;
	
	status_frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (status_hbox), status_frame, TRUE, TRUE, 0);
	meter_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (meter_hbox), 1);
	gtk_container_add (GTK_CONTAINER (status_frame), meter_hbox);
	label = gtk_label_new (_("Memory Used :"));
	gtk_box_pack_start (GTK_BOX (meter_hbox), label, FALSE, FALSE, GNOME_PAD_SMALL);
	memmeter = gtk_progress_bar_new ();
	gtk_widget_set_usize (memmeter, 60, -2);
	gtk_box_pack_start (GTK_BOX (meter_hbox), memmeter, TRUE, TRUE, GNOME_PAD_SMALL);
	procdata->memmeter = memmeter;
	
	status_frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (status_hbox), status_frame, TRUE, TRUE, 0);
	meter_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (meter_hbox), 1);
	gtk_container_add (GTK_CONTAINER (status_frame), meter_hbox);
	label = gtk_label_new (_("Swap Used :"));
	gtk_box_pack_start (GTK_BOX (meter_hbox), label, FALSE, FALSE, GNOME_PAD_SMALL);
	swapmeter = gtk_progress_bar_new ();
	gtk_widget_set_usize (swapmeter, 60, -2);
	gtk_box_pack_start (GTK_BOX (meter_hbox), swapmeter, TRUE, TRUE, GNOME_PAD_SMALL);
	procdata->swapmeter = swapmeter;
	
	gtk_widget_show_all (status_hbox);
	
	gtk_signal_connect (GTK_OBJECT (app), "destroy",
                            GTK_SIGNAL_FUNC (cb_app_destroy),
                            procdata);
	gnome_app_install_menu_hints (GNOME_APP (app), menubar1_uiinfo);

	gtk_signal_connect (GTK_OBJECT (endprocessbutton), "clicked",
			    GTK_SIGNAL_FUNC (cb_end_process_button_pressed), procdata);
	gtk_signal_connect (GTK_OBJECT (infobutton), "clicked",
			    GTK_SIGNAL_FUNC (cb_info_button_pressed),
			    procdata);
        gtk_signal_connect (GTK_OBJECT (procdata->tree), "right_click",
                            GTK_SIGNAL_FUNC (cb_right_click), procdata);
	gtk_signal_connect (GTK_OBJECT (procdata->tree), "cursor_activated",
			    GTK_SIGNAL_FUNC (cb_table_selected), procdata);
	gtk_signal_connect (GTK_OBJECT (procdata->tree), "double_click",
			    GTK_SIGNAL_FUNC (cb_double_click), procdata);
        gtk_signal_connect (GTK_OBJECT (procdata->tree), "right_click",
                            GTK_SIGNAL_FUNC (cb_right_click), procdata);


	
#if 1
	procdata->timeout = gtk_timeout_add (procdata->config.update_interval,
			 cb_timeout, procdata);
	procdata->meter_timeout = gtk_timeout_add (500, cb_progress_meter_timeout, procdata);
#endif		
	gtk_widget_show (vbox1);	 
	gtk_widget_show (app);

	/* Makes sure everything that should be insensitive is at start */
	gtk_signal_emit_by_name (GTK_OBJECT (procdata->tree), 
					 "cursor_activated",
					  -1, NULL);
					  
 	/* We cheat and force it to set up the labels */
 	procdata->config.show_more_info = !procdata->config.show_more_info;
 	toggle_infoview (procdata);	
 	
 	/* create popup_menu */
 	popup_menu = gtk_menu_new ();

	/* Create new menu items */
	lbl_mem_maps = gtk_menu_item_new_with_label (_("Memory Maps ..."));
	gtk_widget_show (lbl_mem_maps);
	lbl_hide = gtk_menu_item_new_with_label (_("Hide Process"));
	gtk_widget_show (lbl_hide);
        lbl_kill = gtk_menu_item_new_with_label (_("End Process"));
        gtk_widget_show (lbl_kill);
        lbl_renice = gtk_menu_item_new_with_label (_("Renice Process ..."));
        gtk_widget_show (lbl_renice);
	sep = gtk_menu_item_new();
	gtk_widget_show (sep);
	an_sep = gtk_menu_item_new();
	gtk_widget_show (an_sep);

	/* Associate the items to the menu and make them visible */
        gtk_menu_append (GTK_MENU (popup_menu), lbl_renice);
        gtk_menu_append (GTK_MENU (popup_menu), lbl_mem_maps);
	gtk_menu_append (GTK_MENU (popup_menu), sep);
	gtk_menu_append (GTK_MENU (popup_menu), lbl_hide);
        gtk_menu_append (GTK_MENU (popup_menu), an_sep);
	gtk_menu_append (GTK_MENU (popup_menu), lbl_kill);
        
	/* Add activiate signal associations to these */
	gtk_signal_connect (GTK_OBJECT (lbl_mem_maps),"activate",
			    GTK_SIGNAL_FUNC(popup_menu_show_memory_maps),
			    data);

	gtk_signal_connect (GTK_OBJECT (lbl_hide),"activate",
                            GTK_SIGNAL_FUNC(popup_menu_hide_process),
                            data);

        gtk_signal_connect (GTK_OBJECT (lbl_renice),"activate",
                            GTK_SIGNAL_FUNC(popup_menu_renice),
                            data);
	gtk_signal_connect (GTK_OBJECT (lbl_kill),"activate",
			    GTK_SIGNAL_FUNC(popup_menu_kill_process),
			    data);

	/* Make the menu visible */
        gtk_widget_show (popup_menu);
	
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
	gtk_widget_set_sensitive (data->infobox, sensitivity);
	gtk_widget_set_sensitive (edit1_menu_uiinfo[0].widget, sensitivity);
	gtk_widget_set_sensitive (edit1_menu_uiinfo[1].widget, sensitivity);
	gtk_widget_set_sensitive (view1_menu_uiinfo[0].widget, sensitivity);
}	

