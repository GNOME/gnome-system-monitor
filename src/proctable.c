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

#include <gal/e-table/e-cell-number.h>
#include <gal/e-table/e-cell-size.h>
#include <gal/e-table/e-tree-memory.h>
#include <gal/e-table/e-tree-memory-callbacks.h>
#include <gal/e-table/e-tree-scrolled.h>
#include <glibtop.h>
#include <glibtop/proclist.h>
#include <glibtop/xmalloc.h>
#include <glibtop/procstate.h>
#include <glibtop/procmem.h>
#include <glibtop/proctime.h>
#include <glibtop/procuid.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <pwd.h>
#include "procman.h"
#include "proctable.h"
#include "infoview.h"
#include "memmaps.h"

/*
gchar * titles[NUM_COLUMNS] = 
{
	N_("Name"),
	N_("User"),
	N_("Memory"),
	N_("% cpu"),
	N_("PID")
};
*/

#define NUM_COLUMNS 11


#define SPEC "<ETableSpecification cursor-mode=\"line\" selection-mode=\"single\" draw-focus=\"true\">                    	       \
  <ETableColumn model_col=\"0\" _title=\"Process Name\"   expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"tree-string\" compare=\"string\"/> \
  <ETableColumn model_col=\"1\" _title=\"User\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\"      compare=\"string\"/> \
  <ETableColumn model_col=\"2\" _title=\"Memory\"     expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"memory\"      compare=\"integer\"/> \
  <ETableColumn model_col=\"3\" _title=\"% CPU\"      expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"cpu\"      compare=\"integer\"/> \
  <ETableColumn model_col=\"4\" _title=\"ID\"      expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"pid\"      compare=\"integer\"/> \
  <ETableColumn model_col=\"5\" _title=\"VM Size\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"vmsize\"      compare=\"integer\"/> \
  <ETableColumn model_col=\"6\" _title=\"Resident Memory\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"memres\"      compare=\"integer\"/> \
  <ETableColumn model_col=\"7\" _title=\"Shared Memory\"      expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"memshared\"      compare=\"integer\"/> \
  <ETableColumn model_col=\"8\" _title=\"RSS Memory\"      expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"memrss\"      compare=\"integer\"/> \
  <ETableColumn model_col=\"9\" _title=\"Nice\"      expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"nice\"      compare=\"integer\"/> \
  <ETableColumn model_col=\"10\" _title=\" \"      expansion=\"1.0\" minimum_width=\"16\" resizable=\"false\" cell=\"pixbuf\"      compare=\"string\"/> \
  
        <ETableState> \
        	<column source=\"10\"/> \
        	<column source=\"0\"/> \
	        <column source=\"1\"/> \
	        <column source=\"2\"/> \
	        <column source=\"3\"/> \
	        <column source=\"4\"/> \
	        <grouping> <leaf column=\"0\" ascending=\"true\"/> </grouping>    \
        </ETableState> \
</ETableSpecification>"

gint total_time = 0;
gint total_time_last = 0;
gint cpu_time_last = 0;
gint cpu_time = 0;


static GdkPixbuf *
proctable_get_icon (ETreeModel *etm, ETreePath path, void *data)
{
	/* No icon, since the cell tree renderer takes care of the +/- icons itself. */
	return NULL;
}

static int
proctable_get_columns (ETreeModel *table, void *data)
{
	return NUM_COLUMNS;
}


static void *
proctable_get_value (ETreeModel *model, ETreePath path, int column, void *data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	
	info = e_tree_memory_node_get_data (procdata->memory, path);

	switch (column) {
	case COL_NAME: {
		return info->name;
	}
	case COL_USER: {
		return info->user;
	}
	case COL_PIXBUF: {
		return info->pixbuf;
	}
	case COL_MEM: {
		return GINT_TO_POINTER (info->mem);
	}
	case COL_CPU: {
		return GINT_TO_POINTER (info->cpu);
	}
	case COL_PID: {
		return GINT_TO_POINTER (info->pid);
	}		
	case COL_VMSIZE: {
		return GINT_TO_POINTER (info->vmsize);
	}
	case COL_MEMRES: {
		return GINT_TO_POINTER (info->memres);
	}
	case COL_MEMSHARED: {
		return GINT_TO_POINTER (info->memshared);
	}
	case COL_MEMRSS: {
		return GINT_TO_POINTER (info->memrss);
	}
	case COL_NICE: {
		return GINT_TO_POINTER (info->nice);
	}
	}
	g_assert_not_reached ();
	return NULL;

	
}

static void
proctable_set_value (ETreeModel *model, ETreePath path, int col, const void *value, void *data)
{

}	

static gboolean
proctable_get_editable (ETreeModel *model, ETreePath path, int column, void *data)
{
	return FALSE;
}

static void *
proctable_duplicate_value (ETreeModel *model, int column, const void *value, void *data)
{
	switch (column) {
	case COL_NAME:
	case COL_USER:
		return g_strdup (value);
	case COL_MEM:
	case COL_CPU:
	case COL_PID:
	case COL_VMSIZE:
	case COL_MEMRES:
	case COL_MEMSHARED:
	case COL_MEMRSS:
	case COL_NICE:
		return (void *)value;
	
	}
		
	g_assert_not_reached ();
	
	return NULL;
}

static void
proctable_free_value (ETreeModel *model, int column, void *value, void *data)
{

	switch (column) {
	case COL_NAME:
	case COL_USER:
		g_free (value);
	case COL_PID:
	case COL_MEM:
	case COL_CPU:
	case COL_VMSIZE:
	case COL_MEMRES:
	case COL_MEMSHARED:
	case COL_MEMRSS:
	case COL_NICE:
		break;
	
	}
		
}

static void *
proctable_initialize_value (ETreeModel *model, int column, void *data)
{
	switch (column) {
	case COL_NAME:
	case COL_USER:
		return g_strdup ("");
	case COL_MEM:
	case COL_CPU:
	case COL_PID:
	case COL_VMSIZE:
	case COL_MEMRES:
	case COL_MEMSHARED:
	case COL_MEMRSS:
	case COL_NICE:
	case COL_PIXBUF:
		return NULL;
	}
		
	g_assert_not_reached ();
	return NULL;
}

static gboolean
proctable_value_is_empty (ETreeModel *model, int column, const void *value, void *data)
{
	switch (column) {
	case COL_NAME:
	case COL_USER:
		return !(value && *(char *)value);
	case COL_MEM:
	case COL_CPU:
	case COL_PID:
	case COL_VMSIZE:
	case COL_MEMRES:
	case COL_MEMSHARED:
	case COL_MEMRSS:
	case COL_NICE:
		return value == NULL;
	default:
		g_assert_not_reached ();
		return FALSE;
	}
}

static char *
proctable_value_to_string (ETreeModel *model, int column, const void *value, void *data)
{
	switch (column) {
	case COL_NAME:
	case COL_USER:
		return g_strdup (value);
	case COL_MEM:
	case COL_CPU:
	case COL_PID:
	case COL_VMSIZE:
	case COL_MEMRES:
	case COL_MEMSHARED:
	case COL_MEMRSS:
	case COL_NICE:
		return g_strdup_printf ("%d",(int) value);
	}
	
	g_assert_not_reached ();
	return NULL;
}

static ETableExtras *
proctable_new_extras ()
{
	ETableExtras *extras;
	ECell *cell;
	
	extras = e_table_extras_new ();
	
	cell = e_cell_size_new (NULL, GTK_JUSTIFY_RIGHT);
	e_table_extras_add_cell (extras, "memory", cell);
	cell = e_cell_number_new (NULL, GTK_JUSTIFY_RIGHT);
	e_table_extras_add_cell (extras, "cpu", cell);
	cell = e_cell_number_new (NULL, GTK_JUSTIFY_RIGHT);
	e_table_extras_add_cell (extras, "pid", cell);
	cell = e_cell_size_new (NULL, GTK_JUSTIFY_RIGHT);
	e_table_extras_add_cell (extras, "vmsize", cell);
	cell = e_cell_size_new (NULL, GTK_JUSTIFY_RIGHT);
	e_table_extras_add_cell (extras, "memres", cell);
	cell = e_cell_size_new (NULL, GTK_JUSTIFY_RIGHT);
	e_table_extras_add_cell (extras, "memshared", cell);
	cell = e_cell_size_new (NULL, GTK_JUSTIFY_RIGHT);
	e_table_extras_add_cell (extras, "memrss", cell);
	cell = e_cell_number_new (NULL, GTK_JUSTIFY_RIGHT);
	e_table_extras_add_cell (extras, "nice", cell);
	
	return extras;
}

GtkWidget *
proctable_new (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *proctree;
	GtkWidget *scrolled;
	ETreeMemory *etmm;
	ETreeModel *model = NULL;
	ETableExtras *extras;

	model = e_tree_memory_callbacks_new (proctable_get_icon,
					     proctable_get_columns,
					     NULL,
					     NULL,
					     NULL,
					     NULL,
					     proctable_get_value,
					     proctable_set_value,
					     proctable_get_editable,
				    	     proctable_duplicate_value,
				    	     proctable_free_value,
				    	     proctable_initialize_value,
				    	     proctable_value_is_empty,
				    	     proctable_value_to_string,
				    	     procdata);
	extras = proctable_new_extras ();
	
	etmm = E_TREE_MEMORY(model);
	
	scrolled = e_tree_scrolled_new (model, extras, SPEC, NULL);
	
	proctree = GTK_WIDGET (e_tree_scrolled_get_tree (E_TREE_SCROLLED (scrolled)));
	
	e_tree_load_state (E_TREE (proctree), procdata->config.tree_state_file);
	
	procdata->tree = proctree;
	procdata->model = model;
	procdata->memory = etmm;
	
	
	return scrolled;

}



static void
proctable_free_info (ProcInfo *info)
{
	if (!info)
		return;
	if (info->name)
		g_free (info->name);
	if (info->cmd)
		g_free (info->cmd);
	if (info->user)
		g_free (info->user);
	if (info->status)
		g_free (info->status);
	if (info->pixbuf)
		g_free (info->pixbuf);
	g_free (info);
}



static gchar *
get_process_status (char *state)
{

	if (!g_strcasecmp (state, "r"))
		return g_strdup_printf (_("Running"));
	else if (!g_strcasecmp (state, "t"))
		return g_strdup_printf (_("Stopped"));
	else
		return g_strdup_printf (_("Sleeping"));

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


static void
update_info (ProcInfo *info, gint pid)
{
	ProcInfo *newinfo = info;
	glibtop_proc_state procstate;
	glibtop_proc_mem procmem;
	glibtop_proc_uid procuid;
	glibtop_proc_time proctime;
	gint newcputime;
	
	glibtop_get_proc_state (&procstate, pid);
	glibtop_get_proc_mem (&procmem, pid);
	glibtop_get_proc_uid (&procuid, pid);
	glibtop_get_proc_time (&proctime, pid);
	newcputime = proctime.utime + proctime.stime;
	
	newinfo->mem = procmem.size;
	newinfo->vmsize = procmem.vsize;
	newinfo->memres = procmem.resident;
	newinfo->memshared = procmem.share;
	newinfo->memrss = procmem.rss;
	if (cpu_time)
		newinfo->cpu = ( newcputime - info->cpu_time_last ) * 100 / total_time;
	else 
		newinfo->cpu = 0;
	newinfo->cpu_time_last = newcputime;
	newinfo->nice = procuid.nice;
	newinfo->status = get_process_status (&procstate.state);
}

static ProcInfo *
get_info (ProcData *procdata, gint pid)
{
	ProcInfo *info = g_new0 (ProcInfo, 1);
	glibtop_proc_state procstate;
	glibtop_proc_time proctime;
	glibtop_proc_mem procmem;
	glibtop_proc_uid procuid;
	struct passwd *pwd;
	gint newcputime;
	
	
	glibtop_get_proc_state (&procstate, pid);
	pwd = getpwuid (procstate.uid);
	glibtop_get_proc_time (&proctime, pid);
	glibtop_get_proc_mem (&procmem, pid);
	glibtop_get_proc_uid (&procuid, pid);
	glibtop_get_proc_time (&proctime, pid);
	newcputime = proctime.utime + proctime.stime;
	
	info->pixbuf = pretty_table_get_icon (procdata->pretty_table, procstate.cmd);
	info->name = pretty_table_get_name (procdata->pretty_table, procstate.cmd);
	info->cmd = g_strdup_printf ("%s", procstate.cmd);
	info->user = g_strdup_printf ("%s", pwd->pw_name);
	info->mem = procmem.size;
	info->vmsize = procmem.vsize;
	info->memres = procmem.resident;
	info->memshared = procmem.share;
	info->memrss = procmem.rss;
	info->cpu = 0;
	info->pid = pid;
	info->parent_pid = procuid.ppid;
	info->cpu_time_last = newcputime;
	info->nice = procuid.nice;
	info->status = get_process_status (&procstate.state);
	
	return info;
}

static ETreePath *
insert_info_to_tree (ProcInfo *info, ProcData *procdata, ETreePath *root_node)
{
	ProcInfo *parentinfo = NULL;
	ETreePath *node;
	
	#if 1 /* do the tree */
	parentinfo = find_parent (procdata, info->parent_pid);
	if (parentinfo)
	{
		node = e_tree_memory_node_insert (procdata->memory, 
						  parentinfo->node, 0, info);
		/* Ha Ha - don't expand different threads - check to see if parent has
		** same name - I don't know if this is too smart though.
		*/
		if (g_strcasecmp (info->name, parentinfo->name))
			e_tree_node_set_expanded (E_TREE (procdata->tree),
					  	  parentinfo->node, TRUE);
	}
	else
	#endif
		node = e_tree_memory_node_insert (procdata->memory, root_node,
						  0, info);
	
	return node;
}

static void
refresh_list (ProcData *data, unsigned *pid_list, gint n)
{
	ProcData *procdata = data;
	GList *list = procdata->info;
	gint i = 0;
	ETreePath *root_node;
	
	//e_tree_memory_freeze (procdata->memory);
	root_node = e_tree_model_get_root (procdata->model);
	
	
	while (i < n)
	{
		ProcInfo *oldinfo;
		/* New process with higher pid than any previous one */
		if (!list)
		{
			ProcInfo *info;
			ETreePath *node;
			
			info = get_info (procdata, pid_list[i]);
			node = insert_info_to_tree (info, procdata, root_node);
			info->node = node;
			procdata->info = g_list_append (procdata->info, info);
			i++;
			continue;
		}
		else
			oldinfo = list->data;
		/* new process */	
		if (pid_list[i] < oldinfo->pid)
		{
			ProcInfo *info;
			ETreePath *node;
			
			info = get_info (procdata, pid_list[i]);
			node = insert_info_to_tree (info, procdata, root_node);
			info->node = node;
			procdata->info = g_list_insert (procdata->info, info, i);
			i++;
		}
		/* existing process */
		else if (pid_list[i] == oldinfo->pid)
		{
			update_info (oldinfo, oldinfo->pid);
			e_tree_model_node_data_changed (procdata->model,oldinfo->node);
			list = g_list_next (list);
			i++;
		}
		/* process no longer exists */
		else if (pid_list[i] > oldinfo->pid)
		{
			if (oldinfo->node == procdata->selected_node)
			{
				procdata->selected_node = NULL;
				/* simulate a "unselected" signal */
				gtk_signal_emit_by_name (GTK_OBJECT (procdata->tree), 
							 "cursor_activated",
					  		 -1, NULL); 
			}
			e_tree_memory_node_remove (procdata->memory, oldinfo->node);
			list = g_list_next (list);
			procdata->info = g_list_remove (procdata->info, oldinfo);
			proctable_free_info (oldinfo);
						
		}
	}
	
	
	/* Get rid of any tasks at end of list that have ended */
	while (list)
	{
		ProcInfo *oldinfo;
		oldinfo = list->data;
		e_tree_memory_node_remove (procdata->memory, oldinfo->node);
		list = g_list_next (list);
		procdata->info = g_list_remove (procdata->info, oldinfo);
		proctable_free_info (oldinfo);
	}
	
	//e_tree_memory_thaw (procdata->memory);


}

void
proctable_update_list (ProcData *data)
{
	ProcData *procdata = data;
	unsigned *pid_list;
	glibtop_proclist proclist;
	glibtop_cpu cpu;
	glibtop_mem mem;
	glibtop_swap swap;
	float memtotal, memused, swaptotal, swapused;
	float pcpu;
	gint which, arg;
	gint n;
	ETreePath *root_node;
	
	
	root_node = e_tree_model_get_root (procdata->model);

	if (procdata->config.whose_process == ALL_PROCESSES)
	{
		which = GLIBTOP_KERN_PROC_ALL;
		arg = 0;
	}
	else
	{
		which = GLIBTOP_KERN_PROC_UID;
		arg = getuid ();
	}

	pid_list = glibtop_get_proclist (&proclist, which, arg);
	n = proclist.number;
	procdata->proc_num = n;
	
	/* FIXME: should make sure that we don't divide by zero here */
	glibtop_get_cpu (&cpu);
	cpu_time = cpu.user + cpu.sys - cpu_time_last;
	cpu_time_last = cpu.user + cpu.sys ;
	total_time = cpu.total - total_time_last;
	pcpu = (float) cpu_time / (total_time);
	total_time_last = cpu.total;
	gtk_progress_bar_update (GTK_PROGRESS_BAR (procdata->cpumeter), pcpu);
	
	glibtop_get_mem (&mem);
	memused = (float) mem.used;
	memtotal = (float) mem.total;
	gtk_progress_bar_update (GTK_PROGRESS_BAR (procdata->memmeter), memused / memtotal);
	
	glibtop_get_swap (&swap);
	swapused = (float) swap.used;
	swaptotal = (float) swap.total;
	gtk_progress_bar_update (GTK_PROGRESS_BAR (procdata->swapmeter), 
				  swapused / swaptotal);

	

	refresh_list (procdata, pid_list, n);
	
	glibtop_free (pid_list);
	
}


void
proctable_update_all (ProcData *data)
{
	ProcData *procdata = data;
	ETreeModel *model = procdata->model;
	ETreePath *root_node;

	
	root_node = e_tree_model_get_root (model);
	/* create a root node if it don't exist */
	if (!root_node)
	{
		root_node = e_tree_memory_node_insert (procdata->memory, NULL, 0, NULL);
		e_tree_root_node_set_visible (E_TREE(procdata->tree), FALSE);	
	}
	
	proctable_update_list (procdata);
	
	
	if (procdata->config.show_more_info)
		infoview_update (procdata);
		
	update_memmaps_dialog (procdata);


}

void 
proctable_clear_tree (ProcData *data)
{
	ProcData *procdata = data;
	ETreePath *rootnode;
	
	rootnode = e_tree_model_get_root (procdata->model);
	while (procdata->info)
	{
		ProcInfo *info = procdata->info->data;
		/*if (info->node)
			e_tree_memory_node_remove (procdata->memory, info->node);*/
		proctable_free_info (info);
		procdata->info = g_list_remove (procdata->info, info);
	}
	
	g_list_free (procdata->info);
	procdata->info = NULL;
	
	gtk_signal_emit_by_name (GTK_OBJECT (procdata->tree), "cursor_activated", -1, NULL);
	procdata->selected_node = NULL;
	procdata->selected_pid = -1;
	
	e_tree_memory_node_remove (procdata->memory, rootnode);
	
	
}


void
proctable_save_state (ProcData *data)
{
	ProcData *procdata = data;
	
	e_tree_save_state (E_TREE (procdata->tree), procdata->config.tree_state_file);
	
}


	
