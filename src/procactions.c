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
#include "procman.h"
#include "proctable.h"

void
renice (int pid, int nice)
{
	int error;
	
	/* FIXME: give a message box with the error messages */
	error = setpriority (PRIO_PROCESS, pid, nice);
	
}

void
kill_process (ProcData *procdata)
{

	ProcInfo *info;
	int error;

	if (!procdata->selected_node)
		return;
		
	info = e_tree_memory_node_get_data (procdata->memory, 
					    procdata->selected_node);
	/* FIXME: if SIGTERM don't work the try SIGKILL. Give a error dialog
	** if still an error (like permission denied
	*/
	error = kill (info->pid, SIGTERM);
	if (error == -1)
	{
		switch (errno) {
		case EPERM:
			g_print ("acces denied \n");
			break;
		default:
			g_print ("error \n");
		}
	}
	proctable_update_all (procdata);		
	
}
