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

void
renice (int pid, int nice)
{
	int error;
	gchar *error_msg;
	GtkWidget *dialog;
	
	errno = 0;
	error = setpriority (PRIO_PROCESS, pid, nice);
	if (error == -1)
	{
		switch (errno) {
			case ESRCH:
				error_msg = g_strdup_printf (_("No such process."));
				dialog = gnome_error_dialog (error_msg);
				gnome_dialog_run(GNOME_DIALOG (dialog));
				g_free (error_msg);
				break;
			case EPERM:
				error_msg = g_strdup_printf (_("You do not have permission to change the priority of this process."));
				dialog = gnome_error_dialog (error_msg);
				gnome_dialog_run(GNOME_DIALOG (dialog));
				g_free(error_msg);
				break;
			case EACCES:
				error_msg = g_strdup_printf (_("You must be root to renice a process lower than 0."));
				dialog = gnome_error_dialog (error_msg);
				gnome_dialog_run(GNOME_DIALOG (dialog));
				g_free(error_msg);
				break;
			default:
				break;
		}
	}
	
}

void
kill_process (ProcData *procdata, int sig)
{

	ProcInfo *info;
	int error;
	GtkWidget *dialog;
        gchar *error_msg;
	gchar *error_critical;
	gint retval;


	if (!procdata->selected_node)
		return;
		
	info = e_tree_memory_node_get_data (procdata->memory, 
					    procdata->selected_node);
	/* Author:  Tige Chastian
	   Date:  8/18/01 
	   Added dialogs for errors on kill.  
	   Added sigterm fail over to sigkill 
	*/
        error = kill (info->pid, sig);
	if (error == -1)
	{
		switch (errno) {
			case ESRCH:
				break;
			case EPERM:
				error_msg = g_strdup_printf (_("You do not have permission to end this process.\n Would you like to enter the superuser (root) password\n to gain the necessary permission?"));
				dialog = gnome_ok_cancel_dialog (error_msg, NULL, NULL);
				retval = gnome_dialog_run(GNOME_DIALOG (dialog));
				g_free(error_msg);
				if (!retval) 
					procdialog_create_root_password_dialog (0, procdata, 
										info->pid, 0);
				break;	
			default: 
				error = kill (info->pid, SIGKILL);
				if (error == -1)
				{
					switch (errno) {
					case ESRCH:
						break;
					default:
						error_critical = g_strdup_printf (_("An error occured while killing the process."));
						dialog = gnome_error_dialog (error_critical);
						g_free (error_critical);
					}
				}
			
                	}
	}

	proctable_update_all (procdata);		
	
}
