/* Procman process actions
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
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "procactions.h"
#include "procman.h"
#include "proctable.h"
#include "procdialogs.h"
#include "callbacks.h"

int nice_value;
int kill_signal;

static void
renice_single_process (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info = NULL;
	gint error;
	gchar *error_msg;
	GtkWidget *dialog;
	
	gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);
	g_return_if_fail (info);
	
	errno = 0;
	error = setpriority (PRIO_PROCESS, info->pid, nice_value);
	if (error == -1)
	{
		switch (errno) {
			case ESRCH:
				error_msg = g_strdup_printf (_("No such process."));
				dialog = gtk_message_dialog_new (NULL,
							         GTK_DIALOG_DESTROY_WITH_PARENT,
                                  			         GTK_MESSAGE_ERROR,
                                  			         GTK_BUTTONS_OK,
                                  			         "%s",
                                  			         error_msg,
                                  			         NULL); 
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				g_free (error_msg);
				break;
			case EPERM:
				error_msg = g_strdup_printf (_("Process Name: %s \n\nYou do not have permission to change the priority of this process. You can enter the root password to gain the necessary permission."), info->name);
				procdialog_create_root_password_dialog (1, procdata, 
									info->pid, nice_value,
									error_msg);
				g_free (error_msg);
				break;
			case EACCES:
				error_msg = g_strdup_printf (_("Process Name: %s \n\nYou must be root to renice a process lower than 0. You can enter the root password to gain the necessary permission."), info->name);
				procdialog_create_root_password_dialog (1, procdata, 
									info->pid, nice_value,
									error_msg);
				g_free (error_msg);
				break;
			default:
				break;
		}
	}		
	
}

void
renice (ProcData *procdata, int pid, int nice)
{
	nice_value = nice;
	
	/*if (!procdata->selection)
		return;*/
		
	gtk_tree_selection_selected_foreach (procdata->selection, renice_single_process, 
					     procdata);
	proctable_update_all (procdata);
	
}

static void
kill_single_process (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	int error;
	GtkWidget *dialog;
        gchar *error_msg;
	gchar *error_critical;
	
	gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);
	g_return_if_fail (info);
		
	/* Author:  Tige Chastian
	   Date:  8/18/01 
	   Added dialogs for errors on kill.  
	   Added sigterm fail over to sigkill 
	*/
        error = kill (info->pid, kill_signal);
	if (error == -1)
	{
		switch (errno) {
			case ESRCH:
				break;
			case EPERM:
				error_msg = g_strdup_printf (_("Process Name: %s \n\nYou do not have permission to end this process. You can enter the root password to gain the necessary permission."), info->name);
				procdialog_create_root_password_dialog (0, procdata, 
									info->pid, kill_signal,
									error_msg);
				g_free (error_msg);
				break;	
			default: 
				error = kill (info->pid, SIGKILL);
				if (error == -1)
				{
					switch (errno) {
					case ESRCH:
						break;
					default:
						dialog = gtk_message_dialog_new (NULL,
							       GTK_DIALOG_DESTROY_WITH_PARENT,
                                  			       GTK_MESSAGE_ERROR,
                                  			       GTK_BUTTONS_OK,
                                  			       "%s",
                                  			      _("An error occured while killing the process."),
                                  			      NULL); 
						gtk_dialog_run (GTK_DIALOG (dialog));
						gtk_widget_destroy (dialog);
					}
				}
			
                	}
	}

	
}

void
kill_process (ProcData *procdata, int sig)
{
	/* EEEK - ugly hack - make sure the table is not updated as a crash
	** occurs if you first kill a process and the tree node is removed while
	** still in the foreach function
	*/
	gtk_timeout_remove (procdata->timeout);	
	
	kill_signal = sig;	
	
	gtk_tree_selection_selected_foreach (procdata->selection, kill_single_process, 
					     procdata);
	
	procdata->timeout = gtk_timeout_add (procdata->config.update_interval,
					     cb_timeout, procdata);	    
	proctable_update_all (procdata);
	
}
