/* Procman - dialogs
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

#include <signal.h>
#include "procdialogs.h"
#include "favorites.h"
#include "proctable.h"
#include "callbacks.h"
#include "prettytable.h"
#include "procactions.h"
#include "util.h"
#include "load-graph.h"

GtkWidget *renice_dialog = NULL;
GtkWidget *prefs_dialog = NULL;
gint new_nice_value = 0;
int kill_signal = SIGTERM;

static void
cb_show_hide_message_toggled (GtkToggleButton *button, gpointer data)
{
	GConfClient *client = gconf_client_get_default ();
	gboolean toggle_state;
	
	toggle_state = gtk_toggle_button_get_active (button);
	
	gconf_client_set_bool (client, "/apps/procman/hide_message", toggle_state, NULL);	

}

static void
hide_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
	ProcData *procdata = data;
	
	if (id == 100) {
		add_selected_to_blacklist (procdata);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
		
}

void
procdialog_create_hide_dialog (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *messagebox1;
	GtkWidget *dialog_vbox1;
  	GtkWidget *hbox1;
  	GtkWidget *checkbutton1;
  	gchar *text = _("Are you sure you want to hide this process?\n"
  			"(Choose 'Hidden Processes' in the Settings menu to reshow)");
  			
  	messagebox1 = gtk_message_dialog_new (NULL,
 					      GTK_DIALOG_MODAL,
 					      GTK_MESSAGE_WARNING,
 					      GTK_BUTTONS_CANCEL,
 					      text);
  	
  	gtk_window_set_title (GTK_WINDOW (messagebox1), _("Hide Process"));
  	gtk_window_set_policy (GTK_WINDOW (messagebox1), FALSE, FALSE, FALSE);
  
    	dialog_vbox1 = GTK_DIALOG (messagebox1)->vbox;

  	hbox1 = gtk_hbox_new (FALSE, 0);
  	gtk_widget_show (hbox1);
  	gtk_box_pack_end (GTK_BOX (dialog_vbox1), hbox1, TRUE, TRUE, 0);

  	checkbutton1 = gtk_check_button_new_with_label (_("Show this dialog next time."));
  	gtk_widget_show (checkbutton1);
  	gtk_box_pack_end (GTK_BOX (hbox1), checkbutton1, FALSE, FALSE, 0);
    	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);

  	gtk_dialog_add_button (GTK_DIALOG (messagebox1), _("_Hide Process"), 100);
  	
  	g_signal_connect (G_OBJECT (checkbutton1), "toggled",
                      	  G_CALLBACK (cb_show_hide_message_toggled), procdata);
        g_signal_connect (G_OBJECT (messagebox1), "response",
        		  G_CALLBACK (hide_dialog_button_pressed), procdata);
  	
  	gtk_widget_show_all (messagebox1);

}

static void
cb_show_kill_warning_toggled (GtkToggleButton *button, gpointer data)
{
	GConfClient *client = gconf_client_get_default ();
	gboolean toggle_state;
	
	toggle_state = gtk_toggle_button_get_active (button);
	
	gconf_client_set_bool (client, "/apps/procman/kill_dialog", toggle_state, NULL);	

}

static void
kill_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
	ProcData *procdata = data;
	
	if (id == 100) 
		kill_process (procdata, kill_signal);
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
		
}

void
procdialog_create_kill_dialog (ProcData *data, int signal)
{
	ProcData *procdata = data;
	GtkWidget *messagebox1;
	GtkWidget *dialog_vbox1;
  	GtkWidget *hbox1;
  	GtkWidget *checkbutton1;
  	gchar *text, *title;
  	
  	kill_signal = signal;
  	
  	if (signal == SIGKILL) {
  		title = _("Kill Process");
  		text = _("_Kill Process");
  	}
  	else {
  		title = _("End Process");
  		text = _("_End Process");
  	}

  	messagebox1 = gtk_message_dialog_new (NULL,
 					      GTK_DIALOG_MODAL,
 					      GTK_MESSAGE_WARNING,
 					      GTK_BUTTONS_CANCEL,
 					      _("Unsaved data will be lost."));
  	
  	gtk_window_set_title (GTK_WINDOW (messagebox1), _(title));
  	gtk_window_set_modal (GTK_WINDOW (messagebox1), TRUE);
  	gtk_window_set_policy (GTK_WINDOW (messagebox1), FALSE, FALSE, FALSE);
  
    	dialog_vbox1 = GTK_DIALOG (messagebox1)->vbox;
  	
	hbox1 = gtk_hbox_new (FALSE, 0);
  	gtk_widget_show (hbox1);
  	gtk_box_pack_end (GTK_BOX (dialog_vbox1), hbox1, TRUE, TRUE, 0);

  	checkbutton1 = gtk_check_button_new_with_label (_("Show this dialog next time."));
  	gtk_widget_show (checkbutton1);
  	gtk_box_pack_end (GTK_BOX (hbox1), checkbutton1, FALSE, FALSE, 0);
    	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);

	gtk_dialog_add_button (GTK_DIALOG (messagebox1), _(text), 100);
					    
  	g_signal_connect (G_OBJECT (checkbutton1), "toggled",
                      	  G_CALLBACK (cb_show_kill_warning_toggled),
                      	  procdata);
        g_signal_connect (G_OBJECT (messagebox1), "response",
        		  G_CALLBACK (kill_dialog_button_pressed), procdata);
        
        gtk_widget_show_all (messagebox1);
        
        
}

static gchar *
get_nice_level (gint nice)
{

	if (nice < -7)
		return _("( Very High Priority )");
	else if (nice < -2)
		return _("( High Priority )");
	else if (nice < 3)
		return _("( Normal Priority )");
	else if (nice < 7)
		return _("( Low Priority )");
	else
		return _("( Very Low Priority)");
	
}

static void
renice_scale_changed (GtkAdjustment *adj, gpointer data)
{
	GtkWidget *label = GTK_WIDGET (data);
	
	new_nice_value = adj->value;
	gtk_label_set_text (GTK_LABEL (label), get_nice_level (new_nice_value));		
	
}

static void
renice_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
	ProcData *procdata = data;
	
	if (id == 100) {
		if (new_nice_value == -100)
			return;		
		renice (procdata, -2, new_nice_value);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	renice_dialog = NULL;
}

void
procdialog_create_renice_dialog (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *dialog = NULL;
	GtkWidget *dialog_vbox;
	GtkWidget *vbox;
  	GtkWidget *label;
  	GtkWidget *priority_label;
  	GtkWidget *table;
  	GtkObject *renice_adj;
  	GtkWidget *hscale;
  	gchar *text = 
  	      _("The priority of a process is given by its nice value. A lower nice value corresponds to a higher priority.");

	if (renice_dialog)
		return;
		
	dialog = gtk_dialog_new_with_buttons (_("Change Priority"), NULL,
				              GTK_DIALOG_DESTROY_WITH_PARENT,
				              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				              _("Change _Priority"), 100,
				              NULL);
  	renice_dialog = dialog;
  	
  	new_nice_value = -100;
  	  
    	dialog_vbox = GTK_DIALOG (dialog)->vbox;
    	
    	vbox = gtk_vbox_new (FALSE, GNOME_PAD);
    	gtk_box_pack_start (GTK_BOX (dialog_vbox), vbox, TRUE, TRUE, 0);
    	
    	label = gtk_label_new (text);
    	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD, 0);
    	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    	
    	table = gtk_table_new (2, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
	
	label = gtk_label_new (_("Nice Value :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 2,
			  0, 0, 0, 0);
	
	renice_adj = gtk_adjustment_new (0, -20, 20, 1, 1, 0);
	new_nice_value = 0;
	hscale = gtk_hscale_new (GTK_ADJUSTMENT (renice_adj));
	gtk_scale_set_digits (GTK_SCALE (hscale), 0);
	gtk_table_attach (GTK_TABLE (table), hscale, 1, 2, 0, 1,
			  GTK_EXPAND | GTK_FILL, 0, 0, 0);
			  
	priority_label = gtk_label_new (get_nice_level (0));
	gtk_table_attach (GTK_TABLE (table), priority_label, 1, 2, 1, 2,
			  GTK_FILL, 0, 0, 0);
	
	g_signal_connect (G_OBJECT (dialog), "response",
  			  G_CALLBACK (renice_dialog_button_pressed), procdata);
  	g_signal_connect (G_OBJECT (renice_adj), "value_changed",
  			    G_CALLBACK (renice_scale_changed), priority_label);
  	
    	gtk_widget_show_all (dialog);
    	
    	
}

static void
prefs_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
	
	prefs_dialog = NULL;
}

static void
show_tree_toggled (GtkToggleButton *button, gpointer data)
{
	GConfClient *client;
	gboolean toggled;
	
	client = gconf_client_get_default ();
	
	toggled = gtk_toggle_button_get_active (button);
	gconf_client_set_bool (client, "/apps/procman/show_tree", toggled, NULL);

}

static void
show_threads_toggled (GtkToggleButton *button, gpointer data)
{
	GConfClient *client;
	gboolean toggled;
	
	client = gconf_client_get_default ();
	
	toggled = gtk_toggle_button_get_active (button);
	
	gconf_client_set_bool (client, "/apps/procman/show_threads", toggled, NULL);
		
}


static void
update_update_interval (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	ProcData *procdata = data;
	GConfClient *client = gconf_client_get_default ();
	gdouble value;
	
	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
	
	if (1000 * value == procdata->config.update_interval)
		return;
		
	gconf_client_set_int (client, "/apps/procman/update_interval", value * 1000, NULL);
		
}

static void
update_graph_update_interval (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	ProcData *procdata = data;
	GConfClient *client = gconf_client_get_default ();
	gdouble value = 0;
	
	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
	
	if (1000 * value == procdata->config.graph_update_interval)
		return;
	
	gconf_client_set_int (client, "/apps/procman/graph_update_interval", value * 1000, NULL);
	
}

static void
update_disks_update_interval (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	ProcData *procdata = data;
	GConfClient *client = gconf_client_get_default ();
	gdouble value;
	
	value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
	
	if (1000 * value == procdata->config.disks_update_interval)
		return;
		
	gconf_client_set_int (client, "/apps/procman/disks_interval", value * 1000, NULL);
	
}

static void		
bg_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		  guint a, gpointer data)
{
	GConfClient *client = gconf_client_get_default ();
	
	gconf_client_set_int (client, "/apps/procman/bg_red", r, NULL);
	gconf_client_set_int (client, "/apps/procman/bg_green", g, NULL);
	gconf_client_set_int (client, "/apps/procman/bg_blue", b, NULL);

}

static void		
frame_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		  guint a, gpointer data)
{
	GConfClient *client = gconf_client_get_default ();
	
	gconf_client_set_int (client, "/apps/procman/frame_red", r, NULL);
	gconf_client_set_int (client, "/apps/procman/frame_green", g, NULL);
	gconf_client_set_int (client, "/apps/procman/frame_blue", b, NULL);

}

static void
proc_field_toggled (GtkToggleButton *button, gpointer data)
{
	GtkTreeViewColumn *column = data;
	gboolean toggled;
	
	toggled = gtk_toggle_button_get_active (button);

	gtk_tree_view_column_set_visible (column, toggled);
	
}

static GtkWidget *
create_proc_field_page (ProcData *procdata)
{
	GtkWidget *vbox;
	GtkWidget *tree = procdata->tree;
	GList *columns = NULL;
	GtkWidget *check_button;
	
	vbox = gtk_vbox_new (TRUE, GNOME_PAD_SMALL);
	
	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));
	
	while (columns) {
		GtkTreeViewColumn *column = columns->data;
		const gchar *title;
		gboolean visible;
		
		title = gtk_tree_view_column_get_title (column);
		if (!title) 
			title = _("Icon");
		
		visible = gtk_tree_view_column_get_visible (column);
		
		check_button = gtk_check_button_new_with_label (title);
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), 
				              visible);
		g_signal_connect (G_OBJECT (check_button), "toggled",
			          G_CALLBACK (proc_field_toggled), column);
			    
		gtk_box_pack_start (GTK_BOX (vbox), check_button, FALSE, FALSE, 0);
		
		columns = g_list_next (columns);
	}
		
	
	return vbox;
}

void
procdialog_create_preferences_dialog (ProcData *procdata)
{
	static GtkWidget *dialog = NULL;
	GtkWidget *notebook;
	GtkWidget *proc_box;
	GtkWidget *sys_box;
	GtkWidget *main_vbox;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkAdjustment *adjustment;
	GtkWidget *spin_button;
	GtkWidget *check_button;
	GtkWidget *table;
	GtkWidget *tab_label;
	GtkWidget *color_picker;
	gfloat update;
	
	if (prefs_dialog)
		return;
	
	dialog = gtk_dialog_new_with_buttons (_("Preferences"), NULL,
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);
	prefs_dialog = dialog;
	
	main_vbox = GTK_DIALOG (dialog)->vbox;
	
	notebook = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);
	
	proc_box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (proc_box), GNOME_PAD_SMALL);
	tab_label = gtk_label_new_with_mnemonic (_("Process _Listing"));
	gtk_widget_show (tab_label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), proc_box, tab_label);
	
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (proc_box), vbox, FALSE, FALSE, GNOME_PAD_SMALL);
	
	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("Update Interval ( seconds ) :"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	update = (gfloat) procdata->config.update_interval;
	adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0, 1.0, 
							  100.0, 0.25, 1.0, 1.0);
	spin_button = gtk_spin_button_new (adjustment, 1.0, 2);
	g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
			    G_CALLBACK (update_update_interval), procdata);
	gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);
	
	check_button = gtk_check_button_new_with_label (_("Show Process Dependencies"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), 
				    procdata->config.show_tree);
	g_signal_connect (G_OBJECT (check_button), "toggled",
			    G_CALLBACK (show_tree_toggled), procdata);
	gtk_box_pack_start (GTK_BOX (vbox), check_button, FALSE, FALSE, 0);
	
	check_button = gtk_check_button_new_with_label (_("Show Threads"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), 
				    procdata->config.show_threads);
	g_signal_connect (G_OBJECT (check_button), "toggled",
			    G_CALLBACK (show_threads_toggled), procdata);
	gtk_box_pack_start (GTK_BOX (vbox), check_button, FALSE, FALSE, 0);
	
	vbox = create_proc_field_page (procdata);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	tab_label = gtk_label_new_with_mnemonic (_("Process _Fields"));
	gtk_widget_show (tab_label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, tab_label);
	
	sys_box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (sys_box), GNOME_PAD_SMALL);
	tab_label = gtk_label_new_with_mnemonic (_("System _Monitor"));
	gtk_widget_show (tab_label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sys_box, tab_label);
	
	frame = gtk_frame_new (_("Graphs"));
	gtk_box_pack_start (GTK_BOX (sys_box), frame, FALSE, FALSE, GNOME_PAD_SMALL);
	
	table = gtk_table_new (2, 3, FALSE);
	gtk_container_add (GTK_CONTAINER (frame), table);
	
	label = gtk_label_new (_("Update Speed ( seconds ) :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 
			  GTK_FILL|GTK_EXPAND, 0, 0, GNOME_PAD_SMALL);
			  
	update = (gfloat) procdata->config.graph_update_interval;
	adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0, 0.25, 
							  100.0, 0.25, 1.0, 1.0);
	spin_button = gtk_spin_button_new (adjustment, 1.0, 2);
	g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
			    G_CALLBACK (update_graph_update_interval), procdata);
	gtk_table_attach (GTK_TABLE (table), spin_button, 1, 2, 0, 1,
			  0, 0, GNOME_PAD_SMALL, GNOME_PAD_SMALL);
	
	label = gtk_label_new (_("Background Color :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			  GTK_FILL, 0, 0, GNOME_PAD_SMALL);
	
	color_picker = gnome_color_picker_new ();
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (color_picker), 
				    procdata->config.bg_color.red,
				    procdata->config.bg_color.green,
				    procdata->config.bg_color.blue, 0);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (bg_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 1, 2, 1, 2, 
			  GTK_FILL|GTK_EXPAND,0, GNOME_PAD_SMALL, GNOME_PAD_SMALL);
			  
	label = gtk_label_new (_("Grid Color :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			  GTK_FILL, 0, 0, GNOME_PAD_SMALL);
	
	color_picker = gnome_color_picker_new ();
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (color_picker), 
				    procdata->config.frame_color.red,
				    procdata->config.frame_color.green,
				    procdata->config.frame_color.blue, 0);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (frame_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 1, 2, 2, 3, 
			  GTK_FILL|GTK_EXPAND,0, GNOME_PAD_SMALL, GNOME_PAD_SMALL);		  
	
	frame = gtk_frame_new (_("Devices"));
	gtk_box_pack_start (GTK_BOX (sys_box), frame, FALSE, FALSE, GNOME_PAD_SMALL);
	
	table = gtk_table_new (2, 1, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);
			  
	label = gtk_label_new (_("Update Speed ( seconds ) :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 
			  GTK_FILL|GTK_EXPAND, 0, 0, GNOME_PAD_SMALL);
			  
	update = (gfloat) procdata->config.disks_update_interval;
	adjustment = (GtkAdjustment *) gtk_adjustment_new (update / 1000.0, 1.0, 
							   100.0, 1.0, 1.0, 1.0);
	spin_button = gtk_spin_button_new (adjustment, 1.0, 0);
	g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
			    G_CALLBACK (update_disks_update_interval), procdata);
	gtk_table_attach (GTK_TABLE (table), spin_button, 1, 2, 0, 1,
			  0, 0, 0, GNOME_PAD_SMALL);
			  
	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (prefs_dialog_button_pressed), procdata);
	
	gtk_widget_show_all (dialog);
	
	if (procdata->config.current_tab == 0)
		gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);
	else
		gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 2);
}

/*
** type determines whether if dialog is for killing process (type=0) or renice (type=other).
** extra_value is not used for killing and is priority for renice
*/
void procdialog_create_root_password_dialog (gint type, ProcData *procdata, gint pid, 
					     gint extra_value, gchar *text)
{
	GtkWidget *dialog;
	GtkWidget *error_dialog;
	GtkWidget *main_vbox;
	GtkWidget *hbox;
	GtkWidget *entry;
	GtkWidget *label;
	gchar *title = NULL;
	gchar *command;
	gchar *password, *blank;
	gint retval;
	
	if (type == 0) {
		if (extra_value == SIGKILL)
			title = g_strdup (_("Kill Process"));
		else
			title = g_strdup (_("End Process"));
	}
	else
		title = g_strdup (_("Change Priority"));
		
	dialog = gnome_dialog_new (title, GNOME_STOCK_BUTTON_CANCEL, title, NULL);
	gnome_dialog_close_hides (GNOME_DIALOG (dialog), TRUE);
	
	main_vbox = GNOME_DIALOG (dialog)->vbox;
	
	label = gtk_label_new (_(text));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD, 2 * GNOME_PAD);
	gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("Root Password :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
	
	entry = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, FALSE, 0);
		
	gtk_widget_show_all (main_vbox);
	
	gnome_dialog_set_default (GNOME_DIALOG (dialog), 0);
	gtk_widget_grab_focus (entry);
		
	if (title)
		g_free (title);	
	
	gnome_dialog_editable_enters (GNOME_DIALOG (dialog), GTK_EDITABLE (entry));	
	retval = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
	
	if (retval == 1) {
		password = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
		
		if (!password)
			password = "";
		blank = g_strdup (password);
		if (strlen (blank))
			memset (blank, ' ', strlen (blank));
			
		gtk_entry_set_text (GTK_ENTRY (entry), blank);
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		g_free (blank);
		
		if (type == 0)
			command = g_strdup_printf ("kill -s %d %d", extra_value, pid);
		else
			command = g_strdup_printf ("renice %d %d", extra_value, pid);
			
		if (su_run_with_password (command, password) == -1) {
			error_dialog = gtk_message_dialog_new (NULL,
							       GTK_DIALOG_DESTROY_WITH_PARENT,
                                  			       GTK_MESSAGE_ERROR,
                                  			       GTK_BUTTONS_OK,
                                  			       "%s",
                                  			      _("Wrong Password."),
                                  			      NULL); 
			gtk_dialog_run (GTK_DIALOG (error_dialog));
			gtk_widget_destroy (error_dialog);
		}
		g_free (command);
		
	}
	
}

