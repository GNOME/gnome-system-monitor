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

#include <gnome.h>

#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "infoview.h"

static GnomeUIInfo file1_menu_uiinfo[] =
{
	GNOMEUIINFO_MENU_EXIT_ITEM (cb_app_destroy, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo edit1_menu_uiinfo[] =
{
 	GNOMEUIINFO_MENU_PROPERTIES_ITEM (cb_properties_activate, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo view1_menu_uiinfo[] =
{
	GNOMEUIINFO_END
};

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
	GNOMEUIINFO_MENU_SETTINGS_TREE (settings1_menu_uiinfo),
	GNOMEUIINFO_MENU_HELP_TREE (help1_menu_uiinfo),
	GNOMEUIINFO_END
};

GtkWidget *infobutton;
GtkWidget *endprocessbutton;

static void
cb_test (ETree *tree, int row, ETreePath *node, gpointer data)
{

	g_print ("change row %d \n",row);
}

GtkWidget*
create_main_window (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *app;
	GtkWidget *vbox1;
	GtkWidget *alignment1;
	GtkWidget *hbox1;
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

	app = gnome_app_new ("procman", NULL);
	gtk_window_set_default_size (GTK_WINDOW (app), 460, 475);
	gtk_window_set_policy (GTK_WINDOW (app), FALSE, TRUE, TRUE);

	gnome_app_create_menus_with_data (GNOME_APP (app), menubar1_uiinfo, procdata);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), vbox1);

	alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_box_pack_start (GTK_BOX (vbox1), alignment1, FALSE, FALSE, 0);
	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment1), hbox1);
	gtk_container_set_border_width (GTK_CONTAINER (hbox1), GNOME_PAD_SMALL);

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
  	gtk_menu_set_active (GTK_MENU (optionmenu1_menu), procdata->config.whose_process);
  	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), optionmenu1_menu);
  	
  	label = gtk_label_new (_("View:"));
	gtk_box_pack_end (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	
	gtk_widget_show_all (alignment1);
	
	scrolled = proctable_new (procdata);
	/*gtk_widget_set_usize (scrolled, 400, 400);*/
	gtk_box_pack_start (GTK_BOX (vbox1), scrolled, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled), GNOME_PAD_SMALL);
	
	gtk_widget_show_all (scrolled);
	
	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, FALSE, 0);
	endprocessbutton = gtk_button_new_with_label (_("End Process"));
	gtk_widget_set_sensitive (endprocessbutton, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox2), endprocessbutton, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (endprocessbutton), GNOME_PAD_SMALL);

	infobutton = gtk_button_new_with_label (_("More Info"));
	gtk_box_pack_end (GTK_BOX (hbox2), infobutton, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (infobutton), GNOME_PAD_SMALL);
	gtk_misc_set_padding (GTK_MISC (GTK_BIN (infobutton)->child), 2, 2);
	
	gtk_widget_show_all (hbox2);

	/* This is just to get rid of some GNome-UI criticals - just
	** hide the appbar
	*/
	appbar1 = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar (GNOME_APP (app), appbar1);
	gtk_widget_hide (appbar1);
	
	infobox = infoview_create (procdata);
	gtk_box_pack_start (GTK_BOX (vbox1), infobox, FALSE, FALSE, 0);
	
	if (procdata->config.show_more_info == TRUE)
		gtk_widget_show_all (infobox);
	
	status_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox1), status_hbox, FALSE, FALSE, 0);

	status_frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (status_hbox), status_frame, TRUE, TRUE, 0);
	meter_hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (status_frame), meter_hbox);
	label = gtk_label_new (_("CPU :"));
	gtk_box_pack_start (GTK_BOX (meter_hbox), label, FALSE, FALSE, 0);
	cpumeter = gtk_progress_bar_new ();
	gtk_widget_set_usize (cpumeter, 60, -2);
	gtk_box_pack_start (GTK_BOX (meter_hbox), cpumeter, FALSE, FALSE, 0);
	procdata->cpumeter = cpumeter;
	
	status_frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (status_hbox), status_frame, TRUE, TRUE, 0);
	meter_hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (status_frame), meter_hbox);
	label = gtk_label_new (_("Memory Used :"));
	gtk_box_pack_start (GTK_BOX (meter_hbox), label, FALSE, FALSE, 0);
	memmeter = gtk_progress_bar_new ();
	gtk_widget_set_usize (memmeter, 60, -2);
	gtk_box_pack_start (GTK_BOX (meter_hbox), memmeter, FALSE, FALSE, 0);
	procdata->memmeter = memmeter;
	
	status_frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (status_hbox), status_frame, TRUE, TRUE, 0);
	meter_hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (status_frame), meter_hbox);
	label = gtk_label_new (_("Swap Used :"));
	gtk_box_pack_start (GTK_BOX (meter_hbox), label, FALSE, FALSE, 0);
	swapmeter = gtk_progress_bar_new ();
	gtk_widget_set_usize (swapmeter, 60, -2);
	gtk_box_pack_start (GTK_BOX (meter_hbox), swapmeter, FALSE, FALSE, 0);
	procdata->swapmeter = swapmeter;
	
	gtk_widget_show_all (status_hbox);
	
	gtk_signal_connect (GTK_OBJECT (app), "destroy",
                            GTK_SIGNAL_FUNC (cb_app_destroy),
                            procdata);
	gnome_app_install_menu_hints (GNOME_APP (app), menubar1_uiinfo);

	gtk_signal_connect (GTK_OBJECT (endprocessbutton), "clicked",
			    GTK_SIGNAL_FUNC (cb_end_process_button_pressed), procdata);
	gtk_signal_connect (GTK_OBJECT (infobutton), "pressed",
			    GTK_SIGNAL_FUNC (cb_info_button_pressed),
			    procdata);

	gtk_signal_connect (GTK_OBJECT (procdata->tree), "cursor_activated",
			    GTK_SIGNAL_FUNC (cb_table_selected), procdata);
	gtk_signal_connect (GTK_OBJECT (procdata->tree), "double_click",
			    GTK_SIGNAL_FUNC (cb_double_click), procdata);
	gtk_signal_connect (GTK_OBJECT (procdata->tree), "cursor_change",
			    GTK_SIGNAL_FUNC (cb_test), procdata);

#if 1
	procdata->timeout = gtk_timeout_add (procdata->config.update_interval,
			 cb_timeout, procdata);
#endif		
	gtk_widget_show (vbox1);	 
	gtk_widget_show (app);

		
	
	
	return app;
}


void
toggle_infoview (ProcData *data)
{
	ProcData *procdata = data;
	GtkLabel *label;
	
	label = GTK_LABEL (GTK_BUTTON (infobutton)->child);
		
	if (procdata->config.show_more_info == FALSE)
	{
		gtk_widget_show_all (procdata->infobox);
		procdata->config.show_more_info = TRUE;	
		infoview_update (procdata);
		gtk_label_set_text (label, _("Less Info"));
		
	}			
	else
	{
		gtk_widget_hide (procdata->infobox);
		procdata->config.show_more_info = FALSE;
		gtk_label_set_text (label, _("More Info"));
	}
}


void
update_sensitivity (ProcData *data, gboolean sensitivity)
{

	gtk_widget_set_sensitive (endprocessbutton, sensitivity);
	gtk_widget_set_sensitive (data->infobox, sensitivity);
}	

