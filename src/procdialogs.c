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

GtkWidget *renice_dialog;
gint new_nice_value = 0;

static void
cb_show_hide_message_toggled (GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = data;
	gboolean toggle_state;
	
	toggle_state = gtk_toggle_button_get_active (button);
	
	procdata->config.show_hide_message = toggle_state;	

}

static void
cb_hide_process_clicked (GtkButton *button, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	
	info = e_tree_memory_node_get_data (procdata->memory, 
					    procdata->selected_node);
					   
	add_to_blacklist (procdata, info->cmd);
	proctable_update_all (procdata);

}

static void
cb_hide_cancel_clicked (GtkButton *button, gpointer data)
{

}


GtkWidget *
procdialog_create_hide_dialog (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *messagebox1;
	GtkWidget *dialog_vbox1;
  	GtkWidget *hbox1;
  	GtkWidget *checkbutton1;
  	GtkWidget *button5;
  	GtkWidget *button6;
  	GtkWidget *dialog_action_area1;
  	gchar *text = _("This action will block a process from being displayed. \n To reshow a process choose Hidden Processes in the Edit menu");

  	/* We create it with an OK button, and then remove the button, to work
     	around a bug in gnome-libs. */
 	messagebox1 = gnome_message_box_new (text,
                              		     GNOME_MESSAGE_BOX_WARNING,
                              		     GNOME_STOCK_BUTTON_OK, NULL);
  	gtk_container_remove (GTK_CONTAINER (GNOME_DIALOG (messagebox1)->action_area), 
  			      GNOME_DIALOG (messagebox1)->buttons->data);
  	GNOME_DIALOG (messagebox1)->buttons = NULL;
  	
  	gtk_window_set_title (GTK_WINDOW (messagebox1), _("Hide Process"));
  	gtk_window_set_modal (GTK_WINDOW (messagebox1), TRUE);
  	gtk_window_set_policy (GTK_WINDOW (messagebox1), FALSE, FALSE, FALSE);
  
    	dialog_vbox1 = GNOME_DIALOG (messagebox1)->vbox;
  	

  	hbox1 = gtk_hbox_new (FALSE, 0);
  	gtk_widget_show (hbox1);
  	gtk_box_pack_end (GTK_BOX (dialog_vbox1), hbox1, TRUE, TRUE, 0);

  	checkbutton1 = gtk_check_button_new_with_label (_("Show this dialog next time."));
  	gtk_widget_show (checkbutton1);
  	gtk_box_pack_end (GTK_BOX (hbox1), checkbutton1, FALSE, FALSE, 0);
    	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);

  	gnome_dialog_append_button (GNOME_DIALOG (messagebox1), _("Hide Process"));
  	button5 = GTK_WIDGET (g_list_last (GNOME_DIALOG (messagebox1)->buttons)->data);
  	gtk_widget_show (button5);
  	GTK_WIDGET_SET_FLAGS (button5, GTK_CAN_DEFAULT);

  	gnome_dialog_append_button (GNOME_DIALOG (messagebox1), GNOME_STOCK_BUTTON_CANCEL);
  	button6 = GTK_WIDGET (g_list_last (GNOME_DIALOG (messagebox1)->buttons)->data);
  	gtk_widget_show (button6);
    	GTK_WIDGET_SET_FLAGS (button6, GTK_CAN_DEFAULT);

  	dialog_action_area1 = GNOME_DIALOG (messagebox1)->action_area;
  	

  	gtk_signal_connect (GTK_OBJECT (checkbutton1), "toggled",
                      	    GTK_SIGNAL_FUNC (cb_show_hide_message_toggled),
                      	    procdata);
        gtk_signal_connect (GTK_OBJECT (button5), "clicked",
                      	    GTK_SIGNAL_FUNC (cb_hide_process_clicked),
                      	    procdata);
  	gtk_signal_connect (GTK_OBJECT (button6), "clicked",
                      	    GTK_SIGNAL_FUNC (cb_hide_cancel_clicked),
                      	    NULL);


  	gtk_widget_grab_default (button6);
  	return messagebox1;

}

static void
cb_show_kill_warning_toggled (GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = data;
	gboolean toggle_state;
	
	toggle_state = gtk_toggle_button_get_active (button);
	
	procdata->config.show_kill_warning = toggle_state;	

}

static void
cb_kill_process_clicked (GtkButton *button, gpointer data)
{
	ProcData *procdata = data;
	
	kill_process (procdata, SIGTERM);

}

static void
cb_kill_cancel_clicked (GtkButton *button, gpointer data)
{

}


void
procdialog_create_kill_dialog (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *messagebox1;
	GtkWidget *dialog_vbox1;
  	GtkWidget *hbox1;
  	GtkWidget *checkbutton1;
  	GtkWidget *button5;
  	GtkWidget *button6;
  	GtkWidget *dialog_action_area1;

  	/* We create it with an OK button, and then remove the button, to work
     	around a bug in gnome-libs. */
 	messagebox1 = gnome_message_box_new (_("Unsaved data will be lost."),
                              		     GNOME_MESSAGE_BOX_WARNING,
                              		     GNOME_STOCK_BUTTON_OK, NULL);
  	gtk_container_remove (GTK_CONTAINER (GNOME_DIALOG (messagebox1)->action_area), 
  			      GNOME_DIALOG (messagebox1)->buttons->data);
  	GNOME_DIALOG (messagebox1)->buttons = NULL;
  	
  	gtk_window_set_title (GTK_WINDOW (messagebox1), _("End Process"));
  	gtk_window_set_modal (GTK_WINDOW (messagebox1), TRUE);
  	gtk_window_set_policy (GTK_WINDOW (messagebox1), FALSE, FALSE, FALSE);
  
    	dialog_vbox1 = GNOME_DIALOG (messagebox1)->vbox;
  	

  	hbox1 = gtk_hbox_new (FALSE, 0);
  	gtk_widget_show (hbox1);
  	gtk_box_pack_end (GTK_BOX (dialog_vbox1), hbox1, TRUE, TRUE, 0);

  	checkbutton1 = gtk_check_button_new_with_label (_("Show this dialog next time."));
  	gtk_widget_show (checkbutton1);
  	gtk_box_pack_end (GTK_BOX (hbox1), checkbutton1, FALSE, FALSE, 0);
    	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);

  	gnome_dialog_append_button (GNOME_DIALOG (messagebox1), _("End Process"));
  	button5 = GTK_WIDGET (g_list_last (GNOME_DIALOG (messagebox1)->buttons)->data);
  	gtk_widget_show (button5);
  	GTK_WIDGET_SET_FLAGS (button5, GTK_CAN_DEFAULT);

  	gnome_dialog_append_button (GNOME_DIALOG (messagebox1), GNOME_STOCK_BUTTON_CANCEL);
  	button6 = GTK_WIDGET (g_list_last (GNOME_DIALOG (messagebox1)->buttons)->data);
  	gtk_widget_show (button6);
    	GTK_WIDGET_SET_FLAGS (button6, GTK_CAN_DEFAULT);

  	dialog_action_area1 = GNOME_DIALOG (messagebox1)->action_area;
  	

  	gtk_signal_connect (GTK_OBJECT (checkbutton1), "toggled",
                      	    GTK_SIGNAL_FUNC (cb_show_kill_warning_toggled),
                      	    procdata);
        gtk_signal_connect (GTK_OBJECT (button5), "clicked",
                      	    GTK_SIGNAL_FUNC (cb_kill_process_clicked),
                      	    procdata);
  	gtk_signal_connect (GTK_OBJECT (button6), "clicked",
                      	    GTK_SIGNAL_FUNC (cb_kill_cancel_clicked),
                      	    NULL);


  	gtk_widget_grab_default (button6);
  	
  	gtk_widget_show (messagebox1);

}

static gboolean
renice_close_dialog (GnomeDialog *dialog, gpointer data)
{
	renice_dialog = NULL;
	
	return FALSE;
}

static void
renice_close (GtkButton *button, gpointer *data)
{
	gnome_dialog_close (GNOME_DIALOG (renice_dialog));

}

static void
renice_accept (GtkButton *button, gpointer data)
{
	ProcData *procdata = data;
	
	renice (procdata, procdata->selected_pid, new_nice_value);
	
	gnome_dialog_close (GNOME_DIALOG (renice_dialog));	
	
}

static void
renice_scale_changed (GtkAdjustment *adj, gpointer data)
{
	new_nice_value = adj->value;		
	
}

void
procdialog_create_renice_dialog (ProcData *data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	GtkWidget *dialog = NULL;
	GtkWidget *dialog_vbox;
	GtkWidget *vbox;
 	GtkWidget *hbox;
  	GtkWidget *label;
  	GtkWidget *frame;
  	GtkObject *renice_adj;
  	GtkWidget *hscale;
  	GtkWidget *renicebutton;
  	GtkWidget *cancelbutton;
  	GtkWidget *dialog_action_area;
  	gchar *text = 
  	      _("The priority of a process is given by its nice value. A lower nice value corresponds to a higher priority.");
	
	if (!procdata->selected_node)
		return;
		
	info = e_tree_memory_node_get_data (procdata->memory, procdata->selected_node);
	if (!info)
		return;
		
	if (renice_dialog)
		return;
		
	dialog = gnome_dialog_new (_("Change Priority"), NULL);
  	renice_dialog = dialog;
  	  
    	dialog_vbox = GNOME_DIALOG (dialog)->vbox;
    	
    	vbox = gtk_vbox_new (FALSE, GNOME_PAD);
    	gtk_box_pack_start (GTK_BOX (dialog_vbox), vbox, TRUE, TRUE, 0);
    	
    	label = gtk_label_new (text);
    	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD, 0);
    	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    	
    	frame = gtk_frame_new (_("Nice Value"));				
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), GNOME_PAD_SMALL);
	
	renice_adj = gtk_adjustment_new (info->nice, -20, 20, 1, 1, 0);
	new_nice_value = info->nice;
	hscale = gtk_hscale_new (GTK_ADJUSTMENT (renice_adj));
	gtk_scale_set_digits (GTK_SCALE (hscale), 0);
	gtk_box_pack_start (GTK_BOX (hbox), hscale, TRUE, TRUE, 0);
	
	dialog_action_area = GNOME_DIALOG (dialog)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area), 8);
	
	gnome_dialog_append_button (GNOME_DIALOG (dialog), _("Change Priority"));
  	renicebutton = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog)->buttons)->data);
  	gnome_dialog_append_button (GNOME_DIALOG (dialog), GNOME_STOCK_BUTTON_CANCEL);
  	cancelbutton = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog)->buttons)->data);
  	
  	GTK_WIDGET_SET_FLAGS (cancelbutton, GTK_CAN_DEFAULT);
  	
  	gtk_signal_connect (GTK_OBJECT (cancelbutton), "clicked",
  			    GTK_SIGNAL_FUNC (renice_close), dialog);
  	gtk_signal_connect (GTK_OBJECT (renicebutton), "clicked",
  			    GTK_SIGNAL_FUNC (renice_accept), procdata);
  	gtk_signal_connect (GTK_OBJECT (renice_adj), "value_changed",
  			    GTK_SIGNAL_FUNC (renice_scale_changed), procdata);
  	gtk_signal_connect (GTK_OBJECT (dialog), "close",
  			    GTK_SIGNAL_FUNC (renice_close_dialog), NULL);
    	
    	gtk_widget_show_all (dialog);
    	
    	
}

static void
preferences_close_button_pressed (GnomeDialog *dialog, gint button, gpointer data)
{
	gnome_dialog_close (dialog);
}

static void
show_tree_toggled (GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = data;
	gboolean toggled;
	
	toggled = gtk_toggle_button_get_active (button);
	
	procdata->config.show_tree = toggled;

	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
}

static void
show_commands_toggled (GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = data;
	gboolean toggled;
	
	toggled = gtk_toggle_button_get_active (button);
	
	procdata->config.show_pretty_names = toggled;

	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
	
}

#if 0
static void
show_icons_toggled (GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = data;
	gboolean toggled;
	
	toggled = gtk_toggle_button_get_active (button);
	
	procdata->config.load_desktop_files = !toggled;
	
	if (!procdata->pretty_table && !toggled)
		procdata->pretty_table = pretty_table_new ();
		
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
	
}
#endif

static void
show_threads_toggled (GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = data;
	gboolean toggled;
	
	toggled = gtk_toggle_button_get_active (button);
	
	procdata->config.show_threads = toggled;
	
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
	
}


static void
update_update_interval (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	ProcData *procdata = data;
	gfloat value;
	
	value = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (widget));
	procdata->config.update_interval = value * 1000;
	
	gtk_timeout_remove (procdata->timeout);
	procdata->timeout = gtk_timeout_add (procdata->config.update_interval, cb_timeout,
					     procdata);
	
}

static void
update_graph_update_interval (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	ProcData *procdata = data;
	gfloat value;
	
	value = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (widget));
	procdata->config.graph_update_interval = value * 1000;
	
	gtk_timeout_remove (procdata->cpu_graph->timer_index);
	procdata->cpu_graph->timer_index = -1;
	procdata->cpu_graph->speed = procdata->config.graph_update_interval;
	gtk_timeout_remove (procdata->mem_graph->timer_index);
	procdata->mem_graph->timer_index = -1;
	procdata->mem_graph->speed = procdata->config.graph_update_interval;
	
	load_graph_start (procdata->cpu_graph);
	load_graph_start (procdata->mem_graph);					     
	
}

static void
update_disks_update_interval (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	ProcData *procdata = data;
	gfloat value;
	
	value = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (widget));
	procdata->config.disks_update_interval = value * 1000;
	
	gtk_timeout_remove (procdata->disk_timeout);
	
	procdata->disk_timeout = gtk_timeout_add (procdata->config.disks_update_interval,
  						   cb_update_disks, procdata);		     
	
}

static void		
bg_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		  guint a, gpointer data)
{
	ProcData *procdata = data;
	
	procdata->cpu_graph->colors[0].red = r;
	procdata->cpu_graph->colors[0].green = g;
	procdata->cpu_graph->colors[0].blue = b;
	procdata->mem_graph->colors[0].red = r;
	procdata->mem_graph->colors[0].green = g;
	procdata->mem_graph->colors[0].blue = b;
	procdata->config.bg_color.red = r;
	procdata->config.bg_color.green = g;
	procdata->config.bg_color.blue = b;	
	procdata->cpu_graph->colors_allocated = FALSE;
	procdata->mem_graph->colors_allocated = FALSE;

}

static void		
frame_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
		  guint a, gpointer data)
{
	ProcData *procdata = data;
	
	procdata->cpu_graph->colors[1].red = r;
	procdata->cpu_graph->colors[1].green = g;
	procdata->cpu_graph->colors[1].blue = b;
	procdata->mem_graph->colors[1].red = r;
	procdata->mem_graph->colors[1].green = g;
	procdata->mem_graph->colors[1].blue = b;
	procdata->config.frame_color.red = r;
	procdata->config.frame_color.green = g;
	procdata->config.frame_color.blue = b;	
	procdata->cpu_graph->colors_allocated = FALSE;
	procdata->mem_graph->colors_allocated = FALSE;

}

void
procdialog_create_preferences_dialog (ProcData *procdata)
{
	GtkWidget *dialog;
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
	
	dialog = gnome_dialog_new (_("Preferences"), GNOME_STOCK_BUTTON_CLOSE, NULL);
	
	main_vbox = GNOME_DIALOG (dialog)->vbox;
	
	notebook = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);
	
	proc_box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (proc_box), GNOME_PAD_SMALL);
	tab_label = gtk_label_new (_("Process Viewer"));
	gtk_widget_show (tab_label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), proc_box, tab_label);
	
	frame = gtk_frame_new (_("General"));
	gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
	gtk_box_pack_start (GTK_BOX (proc_box), frame, FALSE, FALSE, GNOME_PAD_SMALL);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("Update Speed ( seconds ) :"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	update = (gfloat) procdata->config.update_interval;
	adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0, 0.25, 100.0, 0.25, 1.0, 1.0);
	spin_button = gtk_spin_button_new (adjustment, 1.0, 2);
	gtk_signal_connect (GTK_OBJECT (spin_button), "focus_out_event",
			    GTK_SIGNAL_FUNC (update_update_interval), procdata);
	gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);
	
	check_button = gtk_check_button_new_with_label (_("Show Process Dependencies"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), 
				    procdata->config.show_tree);
	gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
			    GTK_SIGNAL_FUNC (show_tree_toggled), procdata);
	gtk_box_pack_start (GTK_BOX (vbox), check_button, FALSE, FALSE, 0);
	
	frame = gtk_frame_new (_("Advanced"));
	gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
	gtk_box_pack_start (GTK_BOX (proc_box), frame, FALSE, FALSE, GNOME_PAD_SMALL);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	check_button = gtk_check_button_new_with_label (_("Show Application Names"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), 
				    procdata->config.show_pretty_names);
	gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
			    GTK_SIGNAL_FUNC (show_commands_toggled), procdata);
	gtk_box_pack_start (GTK_BOX (vbox), check_button, FALSE, FALSE, 0);
	
#if 0
	check_button = gtk_check_button_new_with_label (_("Never Show Icons or Application Names \n ( faster startup time )"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), 
				    !procdata->config.load_desktop_files);
	gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
			    GTK_SIGNAL_FUNC (show_icons_toggled), procdata);			    
	gtk_box_pack_start (GTK_BOX (vbox), check_button, FALSE, FALSE, 0);
#endif
	
	check_button = gtk_check_button_new_with_label (_("Show Threads"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), 
				    procdata->config.show_threads);
	gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
			    GTK_SIGNAL_FUNC (show_threads_toggled), procdata);
	gtk_box_pack_start (GTK_BOX (vbox), check_button, FALSE, FALSE, 0);
	
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (preferences_close_button_pressed), NULL);
			    
			    
	sys_box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (sys_box), GNOME_PAD_SMALL);
	tab_label = gtk_label_new (_("System Monitor"));
	gtk_widget_show (tab_label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sys_box, tab_label);
	
	frame = gtk_frame_new (_("Graphs"));
	gtk_box_pack_start (GTK_BOX (sys_box), frame, FALSE, FALSE, 0);
	
	table = gtk_table_new (2, 3, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);
	
	label = gtk_label_new (_("Update Speed  ( seconds ) :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 
			  GTK_FILL|GTK_EXPAND, 0, 0, GNOME_PAD_SMALL);
			  
	update = (gfloat) procdata->config.graph_update_interval;
	adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0, 0.25, 100.0, 0.25, 1.0, 1.0);
	spin_button = gtk_spin_button_new (adjustment, 1.0, 2);
	gtk_signal_connect (GTK_OBJECT (spin_button), "focus_out_event",
			    GTK_SIGNAL_FUNC (update_graph_update_interval), procdata);
	gtk_table_attach (GTK_TABLE (table), spin_button, 1, 2, 0, 1,
			  0, 0, 0, GNOME_PAD_SMALL);
	
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
	gtk_signal_connect (GTK_OBJECT (color_picker), "color_set",
			    GTK_SIGNAL_FUNC (bg_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 1, 2, 1, 2, 
			  GTK_FILL|GTK_EXPAND,0, 0, GNOME_PAD_SMALL);
			  
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
	gtk_signal_connect (GTK_OBJECT (color_picker), "color_set",
			    GTK_SIGNAL_FUNC (frame_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 1, 2, 2, 3, 
			  GTK_FILL|GTK_EXPAND,0, 0, GNOME_PAD_SMALL);
			  
	
	frame = gtk_frame_new (_("Disks"));
	gtk_box_pack_start (GTK_BOX (sys_box), frame, FALSE, FALSE, 0);
	
	table = gtk_table_new (2, 1, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);
			  
	label = gtk_label_new (_("Update Speed  ( seconds ) :"));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 
			  GTK_FILL|GTK_EXPAND, 0, 0, GNOME_PAD_SMALL);
			  
	update = (gfloat) procdata->config.disks_update_interval;
	adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0, 0.25, 100.0, 0.25, 1.0, 1.0);
	spin_button = gtk_spin_button_new (adjustment, 1.0, 2);
	gtk_signal_connect (GTK_OBJECT (spin_button), "focus_out_event",
			    GTK_SIGNAL_FUNC (update_disks_update_interval), procdata);
	gtk_table_attach (GTK_TABLE (table), spin_button, 1, 2, 0, 1,
			  0, 0, 0, GNOME_PAD_SMALL);

	gtk_widget_show_all (dialog);
	
	gtk_notebook_set_page (GTK_NOTEBOOK (notebook), procdata->config.current_tab);
}

/*
** type determines whether if dialog is for killing process (type=0) or renice (type=other).
** extra_value is not used for killing and is priority for renice
*/
void procdialog_create_root_password_dialog (gint type, ProcData *procdata, gint pid, 
					     gint extra_value, gchar *text)
{
	GtkWidget *dialog;
	GtkWidget *main_vbox;
	GtkWidget *hbox;
	GtkWidget *entry;
	GtkWidget *label;
	gchar *title = NULL;
	gchar *command;
	gchar *password, *blank;
	gint retval;
	
	if (type == 0)
		title = g_strdup (_("End Process"));
	else
		title = g_strdup (_("Change Priority"));
		
	dialog = gnome_dialog_new (title, title, 
				   GNOME_STOCK_BUTTON_CANCEL, NULL);
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
	
	if (retval == 0) {
		password = gtk_entry_get_text (GTK_ENTRY (entry));
		
		if (!password)
			password = "";
		blank = g_strdup (password);
		if (strlen (blank))
			memset (blank, ' ', strlen (blank));

		gtk_entry_set_text (GTK_ENTRY (entry), blank);
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		g_free (blank);
		
		if (type == 0)
			command = g_strdup_printf ("kill %d", pid);
		else
			command = g_strdup_printf ("renice %d %d", extra_value, pid);
			
		su_run_with_password (command, password);
		g_free (command);
	}
	
	
}

