/* Procman tree view and process updating
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

#include <string.h>
#include <glibtop.h>
#include <glibtop/proclist.h>
#include <glibtop/procstate.h>
#include <glibtop/procmem.h>
#include <glibtop/proctime.h>
#include <glibtop/procuid.h>
#include <glibtop/procargs.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <glibtop/procmap.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <sys/stat.h>
#include <pwd.h>
#include "procman.h"
#include "proctable.h"
#include "callbacks.h"
#include "prettytable.h"
#include "util.h"
#include "infoview.h"
#include "interface.h"
#include "memmaps.h"
#include "favorites.h"



static gint total_time = 0;
static gint total_time_last = 0;


static gint
sort_ints (GtkTreeModel *model, GtkTreeIter *itera, GtkTreeIter *iterb, gpointer data)
{
	ProcInfo *infoa = NULL, *infob = NULL;
	gint col = GPOINTER_TO_INT (data);
	gint a, b;

	gtk_tree_model_get (model, itera, COL_POINTER, &infoa, -1);
	gtk_tree_model_get (model, iterb, COL_POINTER, &infob, -1);
	g_return_val_if_fail (infoa, 0);
	g_return_val_if_fail (infob, 0);

	switch (col) {
	case COL_MEM:
		a = infoa->mem;
		b = infob->mem;
		break;
	case COL_CPU:
		a = infoa->cpu;
		b = infob->cpu;
		break;
	case COL_PID:
		a = infoa->pid;
		b = infob->pid;
		break;
	case COL_NICE:
		a = infoa->nice;
		b = infob->nice;
		break;
	case COL_VMSIZE:
		a = infoa->vmsize;
		b = infob->vmsize;
		break;
	case COL_MEMRES:
		a = infoa->memres;
		b = infob->memres;
		break;
	case COL_MEMSHARED:
		a = infoa->memshared;
		b = infob->memshared;
		break;
	case COL_MEMRSS:
		a = infoa->memrss;
		b = infob->memrss;
		break;
	case COL_MEMXSERVER:
		a = infoa->memxserver;
		b = infob->memxserver;
		break;
	default:
		return 0;
		break;
	}

	if (a > b)
		return -1;
	else if (a < b)
		return 1;
	else
		return 0;
}


GtkWidget *
proctable_new (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *proctree;
	GtkWidget *scrolled = NULL;
	GtkTreeStore *model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell_renderer;

	static const gchar *titles[] = {
		N_("Process Name"),
		N_("Arguments"),
		N_("User"),
		N_("Status"),
		N_("Memory"),
		N_("VM Size"),
		N_("Resident Memory"),
		N_("Shared Memory"),
		N_("RSS Memory"),
		N_("X Server Memory"),
		/* xgettext:no-c-format */ N_("% CPU"),
		N_("Nice"),
		N_("ID"),
		NULL,
		"POINTER"
	};

	gint i;

	PROCMAN_GETTEXT_ARRAY_INIT(titles);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	model = gtk_tree_store_new (NUM_COLUMNS,  G_TYPE_STRING,
				    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
				    GDK_TYPE_PIXBUF, G_TYPE_POINTER);

	proctree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (proctree), TRUE);
	g_object_unref (G_OBJECT (model));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (proctree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	column = gtk_tree_view_column_new ();

	cell_renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, cell_renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, cell_renderer,
					     "pixbuf", COL_PIXBUF,
					     NULL);

	cell_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, cell_renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, cell_renderer,
					     "text", COL_NAME,
					     NULL);
	gtk_tree_view_column_set_title (column, titles[0]);
	gtk_tree_view_column_set_sort_column_id (column, COL_NAME);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (proctree), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (proctree), column);

	for (i = 1; i < NUM_COLUMNS - 2; i++) {
		cell_renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new ();
		gtk_tree_view_column_pack_start (column, cell_renderer, FALSE);
		gtk_tree_view_column_set_attributes (column, cell_renderer,
						     "text", i,
						     NULL);
		gtk_tree_view_column_set_title (column, titles[i]);
		gtk_tree_view_column_set_sort_column_id (column, i);
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_column_set_min_width (column, 1);
		gtk_tree_view_append_column (GTK_TREE_VIEW (proctree), column);
	}

	gtk_container_add (GTK_CONTAINER (scrolled), proctree);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_MEM,
					 sort_ints,
					 GINT_TO_POINTER (COL_MEM),
					 NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_CPU,
					 sort_ints,
					 GINT_TO_POINTER (COL_CPU),
					 NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_PID,
					 sort_ints,
					 GINT_TO_POINTER (COL_PID),
					 NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_NICE,
					 sort_ints,
					 GINT_TO_POINTER (COL_NICE),
					 NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_VMSIZE,
					 sort_ints,
					 GINT_TO_POINTER (COL_VMSIZE),
					 NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_MEMRES,
					 sort_ints,
					 GINT_TO_POINTER (COL_MEMRES),
					 NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_MEMSHARED,
					 sort_ints,
					 GINT_TO_POINTER (COL_MEMSHARED),
					 NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_MEMRSS,
					 sort_ints,
					 GINT_TO_POINTER (COL_MEMRSS),
					 NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_MEMXSERVER,
					 sort_ints,
					 GINT_TO_POINTER (COL_MEMXSERVER),
					 NULL);
	procdata->tree = proctree;

	procman_get_tree_state (procdata->client, proctree, "/apps/procman/proctree");

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (proctree))),
			  "changed",
			  G_CALLBACK (cb_row_selected), procdata);
	g_signal_connect (G_OBJECT (proctree), "row_activated",
			  G_CALLBACK (cb_tree_row_activated), procdata);
	g_signal_connect (G_OBJECT (proctree), "popup_menu",
			  G_CALLBACK (cb_tree_popup_menu), procdata);
	g_signal_connect (G_OBJECT (proctree), "button_press_event",
			  G_CALLBACK (cb_tree_button_pressed), procdata);

	return scrolled;
}


static void
proctable_free_info (ProcInfo *info)
{
	if (!info)
		return;
	if (info->name)
		g_free (info->name);
	if (info->arguments)
		g_free (info->arguments);
	if (info->user)
		g_free (info->user);
	if (info->status)
		g_free (info->status);
	g_free (info);
}


static void
get_process_status (ProcInfo *info, const glibtop_proc_state *buf)
{
	g_free (info->status);

	switch(buf->state)
	{
	case GLIBTOP_PROCESS_RUNNING:
		info->status = g_strdup (_("Running"));
		info->running = TRUE;
		break;

	case GLIBTOP_PROCESS_STOPPED:
		info->status = g_strdup (_("Stopped"));
		info->running = FALSE;
		break;

	case GLIBTOP_PROCESS_ZOMBIE:
		info->status = g_strdup (_("Zombie"));
		info->running = FALSE;
		break;

	case GLIBTOP_PROCESS_UNINTERRUPTIBLE:
		info->status = g_strdup (_("Uninterruptible"));
		info->running = FALSE;
		break;

	default:
		info->status = g_strdup (_("Sleeping"));
		info->running = FALSE;
		break;
	}
}


static void
get_process_name (ProcData *procdata, ProcInfo *info, gchar *cmd, gchar *args)
{
	gchar *command = NULL;
	gint i, n = 0, len, newlen;
	gboolean done = FALSE;

	/* strip the absolute path from the arguments */
	if (args && (len = strlen(args)))
	{
		i = len;
		while (!done)
		{
			/* no / in string */
			if (i == 0) {
				n = 0;
				done = TRUE;
			}
			if (args[i] == '/') {
				done = TRUE;
				n = i + 1;
			}
			i--;
		}
		newlen = len - n;
		command = g_new (gchar, newlen + 1);
		for (i = 0; i < newlen; i++) {
			command[i] = args[i + n];
		}
		command[newlen] = '\0';
	}

	if (command)
		info->name = command;
	else
		info->name = g_strdup (cmd);
}


ProcInfo *
proctable_find_process (gint pid, gchar *name, ProcData *procdata)
{

	GList *list = procdata->info;

	while (list) {
		ProcInfo *info = list->data;

		if (pid == info->pid)
			return info;

		if (name) {
			if (g_strcasecmp (name, info->name) == 0)
				return info;
		}

		list = g_list_next (list);
	}

	return NULL;
}


static ProcInfo *
find_parent (ProcData *data, gint pid)
{
	GList *list = data->info;


	/* ignore init as a parent process */
	if (pid <= 1)
		return NULL;

	while (list)
	{
		ProcInfo *info = list->data;

		if (pid == info->pid)
			return info;

		list = g_list_next (list);
	}

	return NULL;
}


void
insert_info_to_tree (ProcInfo *info, ProcData *procdata)
{
	GtkTreeModel *model;
	GtkTreeIter row;
	gchar *name;
	gchar *mem, *vmsize, *memres, *memshared, *memrss, *memxserver;

	/* Don't show process if it is not running */
	if ((procdata->config.whose_process == ACTIVE_PROCESSES) &&
	    (!info->running))
		return;

	/* Don't show processes that user has blacklisted */
	if (is_process_blacklisted (procdata, info->name))
	{
		info->is_blacklisted = TRUE;
		return;
	}
	info->is_blacklisted = FALSE;

	/* Don't show threads */
	if (!procdata->config.show_threads && info->is_thread)
		return;
	else if (info->is_thread)
		name = g_strconcat (info->name, _(" (thread)"), NULL);
	else
		name = g_strdup (info->name);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
	if (info->parent && procdata->config.show_tree && info->parent->visible) {
		GtkTreePath *parent_node = gtk_tree_model_get_path (model, &info->parent->node);

		gtk_tree_store_insert (GTK_TREE_STORE (model), &row, &info->parent->node, 0);
		if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (procdata->tree), parent_node))
			gtk_tree_view_expand_row (GTK_TREE_VIEW (procdata->tree),
						  parent_node,
						  FALSE);

		gtk_tree_path_free (parent_node);
	}
	else
		gtk_tree_store_insert (GTK_TREE_STORE (model), &row, NULL, 0);

	mem = get_size_string (info->mem);
	vmsize = get_size_string (info->vmsize);
	memres = get_size_string (info->memres);
	memshared = get_size_string (info->memshared);
	memrss = get_size_string (info->memrss);
	memxserver = get_size_string (info->memxserver);

	/* COL_POINTER must be set first, because GtkTreeStore
	 * will call sort_ints as soon as we set the column
	 * that we're sorting on.
	 */

	gtk_tree_store_set (GTK_TREE_STORE (model), &row,
			    COL_POINTER, info,
			    COL_PIXBUF, info->pixbuf,
			    COL_NAME, name,
			    COL_ARGS, info->arguments,
			    COL_USER, info->user,
			    COL_STATUS, info->status,
			    COL_MEM, mem,
			    COL_VMSIZE, vmsize,
			    COL_MEMRES, memres,
			    COL_MEMSHARED, memshared,
			    COL_MEMRSS, memrss,
			    COL_MEMXSERVER, memxserver,
			    COL_CPU, info->cpu,
			    COL_PID, info->pid,
			    COL_NICE, info->nice,
			    -1);
	g_free (mem);
	g_free (vmsize);
	g_free (memres);
	g_free (memshared);
	g_free (memrss);
	g_free (memxserver);
	g_free (name);

	info->node = row;
	info->visible = TRUE;
}


/* Removing a node with children - make sure the children are queued
** to be readded.
*/
static void
remove_children_from_tree (ProcData *procdata, GtkTreeModel *model,
			   GtkTreeIter *parent)
{
	ProcInfo *child_info;

	do {
		GtkTreeIter child;

		if (gtk_tree_model_iter_children (model, &child, parent))
			remove_children_from_tree (procdata, model, &child);

		gtk_tree_model_get (model, parent, COL_POINTER, &child_info, -1);
		if (child_info) {
			if (procdata->selected_process == child_info)
				procdata->selected_process = NULL;
			child_info ->queue = NEEDS_ADDITION;
			child_info->visible = FALSE;
		}
	} while (gtk_tree_model_iter_next (model, parent));
}


void
remove_info_from_tree (ProcInfo *info, ProcData *procdata)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_if_fail (info);

	if (!info->visible)
		return;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
	iter = info->node;

	if (procdata->selected_process == info)
		procdata->selected_process = NULL;

	gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);

	info->visible = FALSE;
}


static void
remove_info_from_list (ProcInfo *info, ProcData *procdata)
{
	GList *children = info->children;

	/* Remove info from parent list */
	if (info->parent)
		info->parent->children = g_list_remove (info->parent->children, info);

	/* Add any children to parent list */
	while (children) {
		ProcInfo *child_info = children->data;

		if (info->parent)
			info->parent->children = g_list_append (info->parent->children, child_info);
		child_info->parent = info->parent;

		children = g_list_next (children);
	}

	/* Remove from list */
	procdata->info = g_list_remove (procdata->info, info);
	proctable_free_info (info);

}


static void
update_info (ProcData *procdata, ProcInfo *info, gint pid)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	glibtop_proc_state procstate;
	glibtop_proc_mem procmem;
	glibtop_proc_uid procuid;
	glibtop_proc_time proctime;
	gchar *mem, *vmsize, *memres, *memshared, *memrss, *memxserver;
	gint newcputime;
	gint pcpu;
	WnckResourceUsage xresources;

	glibtop_get_proc_state (&procstate, pid);
	glibtop_get_proc_mem (&procmem, pid);
	glibtop_get_proc_uid (&procuid, pid);
	glibtop_get_proc_time (&proctime, pid);
	newcputime = proctime.utime + proctime.stime;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));

	wnck_pid_read_resource_usage (gdk_screen_get_display (gdk_screen_get_default ()),
				      pid,
				      &xresources);

	info->mem = procmem.size;
	info->vmsize = procmem.vsize;
	info->memres = procmem.resident;
	info->memshared = procmem.share;
	info->memrss = procmem.rss;
	info->memxserver = xresources.total_bytes_estimate;

	/* Possible enhancement - also SUBTRACT this value
	 * from the memory of the X server itself.
	 */
	info->mem += info->memxserver;

	if (total_time != 0)
		pcpu = ( newcputime - info->cpu_time_last ) * 100 / total_time;
	else
		pcpu = 0;
	info->cpu_time_last = newcputime;
	info->cpu = pcpu;
	info->nice = procuid.nice;

	get_process_status (info, &procstate);

	if (procdata->config.whose_process == ACTIVE_PROCESSES)	{

	/* process started running */
		if (info->running && (!info->visible)) {
			insert_info_to_tree (info, procdata);
			return;
		}
	/* process was running but not anymore */
		else if ((!info->running) && info->visible) {
			remove_info_from_tree (info, procdata);
			return;
		}
		else if (!info->running)
			return;
	}

	if (info->visible) {
		GdkRectangle rect, vis_rect;
		path = gtk_tree_model_get_path (model, &info->node);
		gtk_tree_view_get_cell_area (GTK_TREE_VIEW (procdata->tree),
					     path, 	NULL, &rect);
		gtk_tree_view_get_visible_rect (GTK_TREE_VIEW (procdata->tree),
						&vis_rect);
		gtk_tree_path_free (path);
	/* Don't update if row is not visible. Small performance improvement */
		if ((rect.y < 0) || (rect.y > vis_rect.height))
			return;
		mem = get_size_string (info->mem);
		vmsize = get_size_string (info->vmsize);
		memres = get_size_string (info->memres);
		memshared = get_size_string (info->memshared);
		memrss = get_size_string (info->memrss);
		memxserver = get_size_string (info->memxserver);

		gtk_tree_store_set (GTK_TREE_STORE (model), &info->node,
				    COL_STATUS, info->status,
				    COL_MEM, mem,
				    COL_VMSIZE, vmsize,
				    COL_MEMRES, memres,
				    COL_MEMSHARED, memshared,
				    COL_MEMRSS, memrss,
				    COL_MEMXSERVER, memxserver,
				    COL_CPU, info->cpu,
				    COL_NICE, info->nice,
				    -1);
		g_free (mem);
		g_free (vmsize);
		g_free (memres);
		g_free (memshared);
		g_free (memrss);
		g_free (memxserver);
	}
}


static ProcInfo *
get_info (ProcData *procdata, gint pid)
{
	ProcInfo *info = g_new0 (ProcInfo, 1);
	glibtop_proc_state procstate;
	glibtop_proc_time proctime;
	glibtop_proc_mem procmem;
	glibtop_proc_uid procuid;
	glibtop_proc_args procargs;
	struct passwd *pwd;
	gchar *arguments;
	gint newcputime, i;
	WnckResourceUsage xresources;

	glibtop_get_proc_state (&procstate, pid);
	pwd = getpwuid (procstate.uid);
	glibtop_get_proc_time (&proctime, pid);
	glibtop_get_proc_mem (&procmem, pid);
	glibtop_get_proc_uid (&procuid, pid);
	glibtop_get_proc_time (&proctime, pid);
	newcputime = proctime.utime + proctime.stime;

	wnck_pid_read_resource_usage (gdk_screen_get_display (gdk_screen_get_default ()),
				      pid,
				      &xresources);

	arguments = glibtop_get_proc_args (&procargs, pid, 0);
	get_process_name (procdata, info, procstate.cmd, arguments);

	if (arguments)
	{
		for (i = 0; i < procargs.size; i++)
		{
			if (!arguments[i])
				arguments[i] = ' ';
		}
		info->arguments = arguments;
	}
	else
		info->arguments = g_strdup ("");

	if (pwd && pwd->pw_name)
		info->user = g_strdup(pwd->pw_name);
	else
		info->user = NULL;

	info->mem = procmem.size;
	info->vmsize = procmem.vsize;
	info->memres = procmem.resident;
	info->memshared = procmem.share;
	info->memrss = procmem.rss;
	info->memxserver = xresources.total_bytes_estimate;

	/* Possible enhancement - also SUBTRACT this value
	 * from the memory of the X server itself.
	 */
	info->mem += info->memxserver;

	info->cpu = 0;
	info->pid = pid;
	info->parent_pid = procuid.ppid;
	info->cpu_time_last = newcputime;
	info->nice = procuid.nice;
	get_process_status (info, &procstate);

	info->pixbuf = pretty_table_get_icon (procdata->pretty_table, info->name, pid);

	info->parent = find_parent (procdata, info->parent_pid);
	if (info->parent)
		info->parent->children = g_list_append (info->parent->children, info);

#if 0
	if (parentinfo) {
	/* Ha Ha - don't expand different threads - check to see if parent has
	** same name and same mem usage - I don't know if this is too smart though.
	*/

		if (!g_strcasecmp (info->name, parentinfo->name) &&
		    ( parentinfo->mem == info->mem))
		{
			info->is_thread = TRUE;
		}
		else

			info->is_thread = FALSE;

		if (parentinfo->visible) {
			info->parent_node = parentinfo->node;
			info->has_parent = TRUE;
		}
		else
			info->has_parent = FALSE;
	}
	else {
		info->has_parent = FALSE;
		info->is_thread = FALSE;
	}
#endif

	info->visible = FALSE;

	return info;
}


static gboolean
find_match_in_new_list (gint pid, unsigned *pid_list, gint n)
{
	gint i = 0;

	for (i=0; i<n; i++){
		if (pid_list[i] == pid) return TRUE;
	}

	return FALSE;
}


static void
refresh_list (ProcData *data, unsigned *pid_list, gint n)
{
	ProcData *procdata = data;
	GList *list, *removal_list = NULL;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
	gint i = 0;

	/* Add or update processes */
	while (i < n) {
		ProcInfo *info;

		info = proctable_find_process (pid_list[i], NULL, procdata);
		if (!info) {
			info = get_info (procdata, pid_list[i]);
			info->queue = NEEDS_ADDITION;
			procdata->info = g_list_append (procdata->info, info);
		}
		else {
			update_info (procdata, info, info->pid);
		}

		i++;
	}

	/* Remove processes */
	list = procdata->info;
	while (list) {
		ProcInfo *info = list->data;
		GtkTreeIter child;

		if (!find_match_in_new_list (info->pid, pid_list, n))  {
			removal_list  = g_list_append (removal_list, info);
			info->queue = NEEDS_REMOVAL;
			/* remove all children from tree */
			if (info->visible) {
				if (gtk_tree_model_iter_children (model, &child, &info->node))
					remove_children_from_tree (procdata, model, &child);
			}
		}
		list = g_list_next (list);
	}

	/* Add or remove processes from the tree */
	list = procdata->info;
	while (list) {
		ProcInfo *info = list->data;

		if (info->queue == NEEDS_REMOVAL) {
			remove_info_from_tree (info, procdata);
			info->queue = NEEDS_NOTHING;
		} else if (info->queue == NEEDS_ADDITION) {
			insert_info_to_tree (info, procdata);
			info->queue = NEEDS_NOTHING;
		}

		list = g_list_next (list);
	}

	/* Remove processes from the internal GList */
	while(removal_list) {
		ProcInfo *info = removal_list->data;

		remove_info_from_list (info, procdata);

		removal_list = g_list_next (removal_list);
	}

	g_list_free (removal_list);

}


void
proctable_update_list (ProcData *data)
{
	ProcData *procdata = data;
	unsigned *pid_list;
	glibtop_proclist proclist;
	glibtop_cpu cpu;
	gint which, arg;
	gint n;

	switch (procdata->config.whose_process) {
	case ALL_PROCESSES:
	case ACTIVE_PROCESSES:
		which = GLIBTOP_KERN_PROC_ALL;
		arg = 0;
		break;
	default:
		which = GLIBTOP_KERN_PROC_UID;
		arg = getuid ();
		break;
	}

	pid_list = glibtop_get_proclist (&proclist, which, arg);
	n = proclist.number;

	/* FIXME: total cpu time elapsed should be calculated on an individual basis here
	** should probably have a total_time_last gint in the ProcInfo structure */
	glibtop_get_cpu (&cpu);
	total_time = cpu.total - total_time_last;
	total_time_last = cpu.total;

	refresh_list (procdata, pid_list, n);

	g_free (pid_list);
}


void
proctable_update_all (ProcData *data)
{
	ProcData *procdata = data;

	proctable_update_list (procdata);

	if (procdata->config.show_more_info)
		infoview_update (procdata);
}


void
proctable_clear_tree (ProcData *data)
{
	ProcData *procdata = data;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));

	gtk_tree_store_clear (GTK_TREE_STORE (model));

	proctable_free_table (procdata);

	update_sensitivity (procdata, FALSE);
}


void
proctable_free_table (ProcData *procdata)
{
	GList *list = procdata->info;

	while (list)
	{
		ProcInfo *info = list->data;
		proctable_free_info (info);
		list = g_list_next (list);
	}

	g_list_free (procdata->info);
	procdata->info = NULL;
}


void
proctable_search_table (ProcData *procdata, gchar *string)
{
	GList *list = procdata->info;
	GtkWidget *dialog;
	GtkTreeModel *model;
	gchar *error;
	static gint increment = 0, index;
	static gchar *last = NULL;

	if (!g_strcasecmp (string, ""))
		return;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));

	if (!last)
		last = g_strdup (string);
	else if (g_strcasecmp (string, last)) {
		increment = 0;
		g_free (last);
		last = g_strdup (string);
	}
	else
		increment ++;

	index = increment;

	while (list)
	{
		ProcInfo *info = list->data;
		if (strstr (info->name, string) && info->visible)
		{
			GtkTreePath *node = gtk_tree_model_get_path (model, &info->node);

			if (index == 0) {
				gtk_tree_view_set_cursor (GTK_TREE_VIEW (procdata->tree),
							  node,
							  NULL,
							  FALSE);
				return;
			}
			else
				index --;

			gtk_tree_path_free (node);
		}

		if (info->user && strstr (info->user, string) && info->visible)
		{
			GtkTreePath *node = gtk_tree_model_get_path (model, &info->node);

			if (index == 0) {
				gtk_tree_view_set_cursor (GTK_TREE_VIEW (procdata->tree),
							  node,
							  NULL,
							  FALSE);
				return;
			}
			else
				index --;

			gtk_tree_path_free (node);
		}

		list = g_list_next (list);
	}

	if (index == increment) {
		error = g_strdup_printf (_("%s could not be found."), string);
		dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						 "%s", error, NULL);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_free (error);
	}
	else {
		/* no more cases found. Start the search anew */
		increment = -1;
		proctable_search_table (procdata, string);
		return;
	}

	increment --;
}

