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

#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "procdialogs.h"

GtkWidget *renice_spinbutton;
GtkWidget *renice_dialog;

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
	ProcInfo *info;
	
	info = e_tree_memory_node_get_data (procdata->memory, 
					    procdata->selected_node);
	kill (info->pid, SIGTERM);

}

static void
cb_kill_cancel_clicked (GtkButton *button, gpointer data)
{

}


GtkWidget *
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
 	messagebox1 = gnome_message_box_new (_("Unsaved data will be lost"),
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

  	gnome_dialog_append_button (GNOME_DIALOG (messagebox1), _("Cancel"));
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
  	return messagebox1;

}


static void
renice_close (GtkButton *button, gpointer *data)
{
	GtkWidget *dialog = GTK_WIDGET (data);
	
	gnome_dialog_close (GNOME_DIALOG (dialog));

}

static void
renice (int pid, int nice)
{
	int error;
	/* FIXME: give a message box with the error messages */
	error = setpriority (PRIO_PROCESS, pid, nice);
	gnome_dialog_close (GNOME_DIALOG (renice_dialog));
}

static void
renice_accept (GtkButton *button, gpointer *data)
{
	ProcData *procdata = (ProcData *)data;
	gint nice;
	
	if (!renice_spinbutton)
		return;
		
	nice = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (renice_spinbutton));
	renice (procdata->selected_pid, nice);
	g_print ("%d \n", nice);
}
	
	



void
procdialog_create_renice_dialog (ProcData *data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	GtkWidget *dialog;
	GtkWidget *dialog_vbox;
	GtkWidget *vbox;
  	GtkWidget *hbox;
  	GtkWidget *label;
  	GtkWidget *spinbutton;
  	GtkObject *adjustment;
  	GtkWidget *renicebutton;
  	GtkWidget *cancelbutton;
  	GtkWidget *dialog_action_area;
  	gchar *text = 
  	      _("The nice value is the priority");
	
	if (!procdata->selected_node)
		return;
		
	info = e_tree_memory_node_get_data (procdata->memory, procdata->selected_node);
	if (!info)
		return;

  	dialog = gnome_dialog_new (NULL, NULL);
  	renice_dialog = dialog;
  	gtk_window_set_title (GTK_WINDOW (dialog), _("Change Priority"));
  	gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
  	  
    	dialog_vbox = GNOME_DIALOG (dialog)->vbox;
    	
    	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
    	gtk_box_pack_start (GTK_BOX (dialog_vbox), vbox, TRUE, TRUE, 0);
    	
    	label = gtk_label_new (text);
    	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("Nice :"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	adjustment = gtk_adjustment_new (info->nice, -20, 20, 1, 10, 10);
	renice_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
	gtk_box_pack_start (GTK_BOX (hbox), renice_spinbutton, FALSE, FALSE, 0);
	
	dialog_action_area = GNOME_DIALOG (dialog)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area), 8);
	
	gnome_dialog_append_button (GNOME_DIALOG (dialog), _("Renice"));
  	renicebutton = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog)->buttons)->data);
  	gnome_dialog_append_button (GNOME_DIALOG (dialog), _("Cancel"));
  	cancelbutton = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog)->buttons)->data);
  	
  	GTK_WIDGET_SET_FLAGS (cancelbutton, GTK_CAN_DEFAULT);
  	
  	gtk_signal_connect (GTK_OBJECT (cancelbutton), "clicked",
  			    GTK_SIGNAL_FUNC (renice_close), dialog);
  	gtk_signal_connect (GTK_OBJECT (renicebutton), "clicked",
  			    GTK_SIGNAL_FUNC (renice_accept), procdata);
    	
    	gtk_widget_show_all (dialog);
    	
    	
    	
}

