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


#include <config.h>


#include <string.h>
#include <math.h>
#include <glib/gi18n.h>
#include <glibtop.h>
#include <glibtop/proclist.h>
#include <glibtop/procstate.h>
#include <glibtop/procmem.h>
#include <glibtop/proctime.h>
#include <glibtop/procuid.h>
#include <glibtop/procargs.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <sys/stat.h>
#include <pwd.h>

#include <libgnomevfs/gnome-vfs-utils.h>

#include "procman.h"
#include "proctable.h"
#include "callbacks.h"
#include "prettytable.h"
#include "util.h"
#include "infoview.h"
#include "interface.h"
#include "favorites.h"


#ifdef ENABLE_SELINUX
#include <selinux/selinux.h>
#endif


static guint64 total_time = 1;
static guint64 total_time_last = 1;


static gint
sort_ints (GtkTreeModel *model, GtkTreeIter *itera, GtkTreeIter *iterb, gpointer data)
{
	ProcInfo *infoa = NULL, *infob = NULL;
	const gint col = GPOINTER_TO_INT (data);

	gtk_tree_model_get (model, itera, COL_POINTER, &infoa, -1);
	gtk_tree_model_get (model, iterb, COL_POINTER, &infob, -1);
	g_return_val_if_fail (infoa, 0);
	g_return_val_if_fail (infob, 0);

	switch (col) {
	case COL_MEM:
		return PROCMAN_CMP(infoa->mem, infob->mem);

	case COL_VMSIZE:
		return PROCMAN_CMP(infoa->vmsize, infob->vmsize);

	case COL_MEMRES:
		return PROCMAN_CMP(infoa->memres, infob->memres);

	case COL_MEMSHARED:
		return PROCMAN_CMP(infoa->memshared, infob->memshared);

	case COL_MEMRSS:
		return PROCMAN_CMP(infoa->memrss, infob->memrss);

	case COL_MEMXSERVER:
		return PROCMAN_CMP(infoa->memxserver, infob->memxserver);

	case COL_CPU_TIME:
		return PROCMAN_CMP(infoa->cpu_time_last, infob->cpu_time_last);

	default:
		g_assert_not_reached();
		return 0;
	}
}



static gboolean
can_show_security_context_column (void)
{
#ifdef ENABLE_SELINUX
	switch (is_selinux_enabled()) {
	case 1: /* We're running on an SELinux kernel */ 
		return TRUE;

	default:
	case -1: 
		/* Error; hide the security context column */

	case 0: 
		/* We're not running on an SELinux kernel: hide the security context column */
		return FALSE;
	}
#else
	/* Support disabled; hide the security context column */
	return FALSE;
#endif
}

static void
get_process_selinux_context (ProcInfo *info)
{
#ifdef ENABLE_SELINUX
	/* Directly grab the SELinux security context: */
	security_context_t con;

	if (!getpidcon (info->pid, &con)) {
		info->security_context = g_strdup (con);
		freecon (con);
	}
#endif
}


static void
set_proctree_reorderable(ProcData *procdata)
{
	GList *columns, *col;
	GtkTreeView *proctree;

	proctree = GTK_TREE_VIEW(procdata->tree);

	columns = gtk_tree_view_get_columns (proctree);

	for(col = columns; col; col = col->next)
		gtk_tree_view_column_set_reorderable(col->data, TRUE);

	g_list_free(columns);
}


GtkWidget *
proctable_new (ProcData * const procdata)
{
	GtkWidget *proctree;
	GtkWidget *scrolled;
	GtkTreeStore *model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell_renderer;

	static const gchar *titles[] = {
		N_("Process Name"),
		N_("User"),
		N_("Status"),
		N_("Memory"),
		N_("VM Size"),
		N_("Resident Memory"),
		N_("Shared Memory"),
		N_("RSS Memory"),
		N_("X Server Memory"),
		/* xgettext:no-c-format */ N_("% CPU"),
		N_("CPU time"),
		N_("Nice"),
		N_("ID"),
		N_("Security Context"),
		N_("Arguments"),
		NULL,
		"POINTER"
	};

	gint i;

	PROCMAN_GETTEXT_ARRAY_INIT(titles);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	model = gtk_tree_store_new (NUM_COLUMNS,
				    G_TYPE_STRING,	/* Process Name */
				    G_TYPE_STRING,	/* User		*/
				    G_TYPE_STRING,	/* Status	*/
				    G_TYPE_STRING,      /* Memory	*/
				    G_TYPE_STRING,	/* VM Size	*/
				    G_TYPE_STRING,	/* Resident Memory */
				    G_TYPE_STRING,	/* Shared Memory */
				    G_TYPE_STRING,	/* RSS Memory	*/
				    G_TYPE_STRING,	/* X Server Memory */
				    G_TYPE_UINT,	/* % CPU	*/
				    G_TYPE_STRING,	/* CPU time	*/
				    G_TYPE_INT,		/* Nice		*/
				    G_TYPE_UINT,	/* ID		*/
				    G_TYPE_STRING,	/* Security Context */
				    G_TYPE_STRING,	/* Arguments	*/
				    GDK_TYPE_PIXBUF,	/* Icon		*/
				    G_TYPE_POINTER	/* ProcInfo	*/
		);

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

		switch(i)
		{
		case COL_MEM:
		case COL_VMSIZE:
		case COL_MEMRES:
		case COL_MEMSHARED:
		case COL_MEMRSS:
		case COL_MEMXSERVER:
		case COL_CPU:
		case COL_NICE:
		case COL_PID:
		case COL_CPU_TIME:
			g_object_set(G_OBJECT(cell_renderer),
				     "xalign", 1.0f,
				     NULL);
		}

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


	for(i = 0; i< NUM_COLUMNS - 2; i++)
	{
		switch(i)
		{
		case COL_MEM:
		case COL_VMSIZE:
		case COL_MEMRES:
		case COL_MEMSHARED:
		case COL_MEMRSS:
		case COL_MEMXSERVER:
		case COL_CPU_TIME:
			gtk_tree_sortable_set_sort_func (
				GTK_TREE_SORTABLE (model),
				i,
				sort_ints,
				GINT_TO_POINTER(i),
				NULL);
		}
	}

	procdata->tree = proctree;

	set_proctree_reorderable(procdata);

	procman_get_tree_state (procdata->client, proctree, "/apps/procman/proctree");

	/* Override column settings by hiding this column if it's meaningless: */
	if (!can_show_security_context_column ()) {
		GtkTreeViewColumn *column;
		column = gtk_tree_view_get_column (GTK_TREE_VIEW (proctree), COL_SECURITYCONTEXT);
		gtk_tree_view_column_set_visible (column, FALSE);
	}

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (proctree))),
			  "changed",
			  G_CALLBACK (cb_row_selected), procdata);
	g_signal_connect (G_OBJECT (proctree), "popup_menu",
			  G_CALLBACK (cb_tree_popup_menu), procdata);
	g_signal_connect (G_OBJECT (proctree), "button_press_event",
			  G_CALLBACK (cb_tree_button_pressed), procdata);

	return scrolled;
}


static void
proctable_free_info (ProcData *procdata, ProcInfo *info)
{
	g_return_if_fail(info != NULL);

	g_free (info->name);
	g_free (info->arguments);
	g_free (info->security_context);
	g_list_free (info->children);
	g_mem_chunk_free (procdata->procinfo_allocator, info);
}


static void
get_process_status (ProcInfo *info, const glibtop_proc_state *buf)
{

	switch(buf->state)
	{
	case GLIBTOP_PROCESS_RUNNING:
		info->status = _("Running");
		info->is_running = TRUE;
		break;

	case GLIBTOP_PROCESS_STOPPED:
		info->status = _("Stopped");
		info->is_running = FALSE;
		break;

	case GLIBTOP_PROCESS_ZOMBIE:
		info->status = _("Zombie");
		info->is_running = FALSE;
		break;

	case GLIBTOP_PROCESS_UNINTERRUPTIBLE:
		info->status = _("Uninterruptible");
		info->is_running = FALSE;
		break;

	default:
		info->status = _("Sleeping");
		info->is_running = FALSE;
		break;
	}
}


static void
get_process_name (ProcInfo *info,
		  const gchar *cmd, const gchar *args)
{

	/* strip the absolute path from the arguments
	 * if args is not null nor ""
	 */
	if (args && *args)
		info->name = g_path_get_basename (args);
	else
		info->name = g_strdup (cmd);
}


static void
get_process_memory_info(ProcInfo *info)
{
	glibtop_proc_mem procmem;
	WnckResourceUsage xresources;

	wnck_pid_read_resource_usage (gdk_screen_get_display (gdk_screen_get_default ()),
				      info->pid,
				      &xresources);

	glibtop_get_proc_mem(&procmem, info->pid);

	info->mem	= procmem.size;
	info->vmsize	= procmem.vsize;
	info->memres	= procmem.resident;
	info->memshared	= procmem.share;
	info->memrss	= procmem.rss;

	info->memxserver = xresources.total_bytes_estimate;
}

ProcInfo *
proctable_find_process (guint pid, ProcData *procdata)
{
	return g_hash_table_lookup(procdata->pids, GINT_TO_POINTER(pid));
}


static ProcInfo *
find_parent (ProcData *procdata, guint pid)
{
	/* ignore init as a parent process */
	if (pid <= 1)
		return NULL;

	return proctable_find_process(pid, procdata);
}



static char *
format_duration_for_display (double d)
{
	double minutes, seconds, centiseconds;

	centiseconds = 100.0 * modf(d, &seconds);

	if(seconds < 60.0)
	{
		minutes = 0.0;
	}
	else
	{
		minutes = seconds / 60.0;
		seconds = fmod(seconds, 60.0);
	}

	return g_strdup_printf(
		"%02u:%02u.%02u",
		(unsigned) minutes,
		(unsigned) seconds,
		(unsigned) centiseconds);
}


static void
update_info_mutable_cols(GtkTreeStore *store, ProcData *procdata, ProcInfo *info)
{
	gchar *mem, *vmsize, *memres, *memshared, *memrss, *memxserver, *cpu_time;

	mem	   = gnome_vfs_format_file_size_for_display (info->mem);
	vmsize	   = gnome_vfs_format_file_size_for_display (info->vmsize);
	memres	   = gnome_vfs_format_file_size_for_display (info->memres);
	memshared  = gnome_vfs_format_file_size_for_display (info->memshared);
	memrss	   = gnome_vfs_format_file_size_for_display (info->memrss);
	memxserver = gnome_vfs_format_file_size_for_display (info->memxserver);

	cpu_time = format_duration_for_display (info->cpu_time_last / procdata->frequency);

	gtk_tree_store_set (store, &info->node,
			    COL_STATUS, info->status,
			    COL_MEM, mem,
			    COL_VMSIZE, vmsize,
			    COL_MEMRES, memres,
			    COL_MEMSHARED, memshared,
			    COL_MEMRSS, memrss,
			    COL_MEMXSERVER, memxserver,
			    COL_CPU, info->pcpu,
			    COL_CPU_TIME, cpu_time,
			    COL_NICE, info->nice,
			    -1);

	/* We don't bother updating COL_SECURITYCONTEXT as it can never change */
	g_free (mem);
	g_free (vmsize);
	g_free (memres);
	g_free (memshared);
	g_free (memrss);
	g_free (memxserver);
	g_free (cpu_time);
}



void
insert_info_to_tree (ProcInfo *info, ProcData *procdata)
{
	GtkTreeModel *model;
	gchar *name;

	/* Don't show process if it is not running */
	if ((procdata->config.whose_process == ACTIVE_PROCESSES) &&
	    (!info->is_running))
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
		name = g_strdup_printf(_("%s (thread)"), info->name);
	else
		name = g_strdup (info->name);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
	if (info->parent && procdata->config.show_tree && info->parent->is_visible) {
		GtkTreePath *parent_node = gtk_tree_model_get_path (model, &info->parent->node);

		gtk_tree_store_insert (GTK_TREE_STORE (model), &info->node, &info->parent->node, 0);
		if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (procdata->tree), parent_node))
			gtk_tree_view_expand_row (GTK_TREE_VIEW (procdata->tree),
						  parent_node,
						  FALSE);

		gtk_tree_path_free (parent_node);
	}
	else
		gtk_tree_store_insert (GTK_TREE_STORE (model), &info->node, NULL, 0);

	/* COL_POINTER must be set first, because GtkTreeStore
	 * will call sort_ints as soon as we set the column
	 * that we're sorting on.
	 */

	gtk_tree_store_set (GTK_TREE_STORE (model), &info->node,
			    COL_POINTER, info,
			    COL_PIXBUF, info->pixbuf,
			    COL_NAME, name,
			    COL_ARGS, info->arguments,
			    COL_USER, info->user,
			    COL_PID, info->pid,
			    COL_SECURITYCONTEXT, info->security_context,
			    -1);

	update_info_mutable_cols(GTK_TREE_STORE (model), procdata, info);

	info->is_visible = TRUE;
}


/* Removing a node with children - make sure the children are queued
** to be readded.
*/
static void
remove_children_from_tree (ProcData *procdata, GtkTreeModel *model,
			   GtkTreeIter *parent, GPtrArray *addition_list)
{
	do {
		ProcInfo *child_info;
		GtkTreeIter child;

		if (gtk_tree_model_iter_children (model, &child, parent))
			remove_children_from_tree (procdata, model, &child, addition_list);

		gtk_tree_model_get (model, parent, COL_POINTER, &child_info, -1);
		if (child_info) {
			if (procdata->selected_process == child_info)
				procdata->selected_process = NULL;
			g_ptr_array_add(addition_list, child_info);
			child_info->is_visible = FALSE;
		}
	} while (gtk_tree_model_iter_next (model, parent));
}


void
remove_info_from_tree (ProcInfo *info, ProcData *procdata)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_if_fail (info);

	if (!info->is_visible)
		return;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
	iter = info->node;

	if (procdata->selected_process == info)
		procdata->selected_process = NULL;

	gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);

	info->is_visible = FALSE;
}


static void
remove_info_from_list (ProcInfo *info, ProcData *procdata)
{
	GList *child;
	ProcInfo * const parent = info->parent;

	/* Remove info from parent list */
	if (parent)
		parent->children = g_list_remove (parent->children, info);

	/* Add any children to parent list */
	for(child = info->children; child; child = g_list_next (child)) {
		ProcInfo *child_info = child->data;
		child_info->parent = parent;
	}

	if(parent) {
		parent->children = g_list_concat(parent->children,
						 info->children);
		info->children = NULL;
	}

	/* Remove from list */
	procdata->info = g_list_remove (procdata->info, info);
	g_hash_table_remove(procdata->pids, GINT_TO_POINTER(info->pid));
	proctable_free_info (procdata, info);
}


static void
update_info (ProcData *procdata, ProcInfo *info)
{
	glibtop_proc_state procstate;

	glibtop_get_proc_state (&procstate, info->pid);
	get_process_status (info, &procstate);

	if (procdata->config.whose_process == ACTIVE_PROCESSES)	{

		/* process started running */
		if (info->is_running && (!info->is_visible)) {
			insert_info_to_tree (info, procdata);
			return;
		}
		/* process was running but not anymore */
		else if ((!info->is_running) && info->is_visible) {
			remove_info_from_tree (info, procdata);
			return;
		}
		else if (!info->is_running)
			return;
	}

	if (info->is_visible) {
		GdkRectangle rect, vis_rect;
		GtkTreePath *path;

		GtkTreeModel *model;
		glibtop_proc_uid procuid;
		glibtop_proc_time proctime;

		glibtop_get_proc_uid (&procuid, info->pid);
		glibtop_get_proc_time (&proctime, info->pid);

		get_process_memory_info(info);

		info->pcpu = (proctime.rtime - info->cpu_time_last) * 100 / total_time;
		info->pcpu = MIN(info->pcpu, 100);

		info->cpu_time_last = proctime.rtime;
		info->nice = procuid.nice;


		model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
		path = gtk_tree_model_get_path (model, &info->node);
		gtk_tree_view_get_cell_area (GTK_TREE_VIEW (procdata->tree),
					     path, NULL, &rect);
		gtk_tree_view_get_visible_rect (GTK_TREE_VIEW (procdata->tree),
						&vis_rect);
		gtk_tree_path_free (path);

		/* Don't update if row is not visible.
		   Small performance improvement */
		if ((rect.y < 0) || (rect.y > vis_rect.height))
			return;

		update_info_mutable_cols(GTK_TREE_STORE (model), procdata, info);
	}
}


static ProcInfo *
get_info (ProcData *procdata, gint pid)
{
	ProcInfo *info;
	glibtop_proc_state procstate;
	glibtop_proc_time proctime;
	glibtop_proc_uid procuid;
	glibtop_proc_args procargs;
	struct passwd *pwd;
	gchar *arguments;
	char *username;

	info = g_chunk_new0(ProcInfo, procdata->procinfo_allocator);

	info->pid = pid;

	glibtop_get_proc_state (&procstate, pid);
	pwd = getpwuid (procstate.uid);
	glibtop_get_proc_uid (&procuid, pid);
	glibtop_get_proc_time (&proctime, pid);

	arguments = glibtop_get_proc_args (&procargs, pid, 0);
	get_process_name (info, procstate.cmd, arguments);

	if (arguments)
	{
		guint i;

		for (i = 0; i < procargs.size; i++)
		{
			if (!arguments[i])
				arguments[i] = ' ';
		}
		/* steals arguments */
		info->arguments = arguments;
	}
	else
		info->arguments = g_strdup ("");

	username = (pwd && pwd->pw_name ? pwd->pw_name : "");
	info->user = g_string_chunk_insert_const(procdata->users, username);

	info->pcpu = 0;
	info->cpu_time_last = proctime.rtime;
	info->nice = procuid.nice;

	get_process_memory_info(info);
	get_process_status (info, &procstate);
	get_process_selinux_context (info);

	info->pixbuf = pretty_table_get_icon (procdata->pretty_table, info->name, pid);

	info->parent = find_parent (procdata, procuid.ppid);
	if (info->parent) {
		info->parent->children = g_list_prepend (info->parent->children, info);
		if(g_strcasecmp (info->name, info->parent->name) == 0
		   && info->parent->mem == info->mem)
			info->is_thread = TRUE;
	}

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

		if (parentinfo->is_visible) {
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

	info->is_visible = FALSE;

	return info;
}



static void cb_exclude(ProcInfo* info, GPtrArray *addition)
{
	g_ptr_array_remove_fast (addition, info);
}


static void
refresh_list (ProcData *procdata, const unsigned *pid_list, const guint n)
{
	GHashTable *pid_hash;
	GList *list;
	GPtrArray *addition_list = g_ptr_array_new ();
	GPtrArray *removal_list = g_ptr_array_new ();
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
	guint i;

	/* Add or update processes */
	for(i = 0; i < n; ++i) {
		ProcInfo *info;

		info = proctable_find_process (pid_list[i], procdata);
		if (!info) {
			info = get_info (procdata, pid_list[i]);
			g_ptr_array_add(addition_list, info);
			procdata->info = g_list_append (procdata->info, info);
			g_hash_table_insert(procdata->pids, GINT_TO_POINTER(info->pid), info);
		}
		else {
			update_info (procdata, info);
		}
	}


	/* Remove processes */

	/* use a hash for fast lookup
	   !NULL as data for every key */

	pid_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
	for(i = 0; i < n; ++i)
		g_hash_table_insert(pid_hash,
				    GINT_TO_POINTER(pid_list[i]),
				    GINT_TO_POINTER(!NULL));


	for(list = procdata->info; list; list = g_list_next (list)) {
		ProcInfo * const info = list->data;

		if(!g_hash_table_lookup(pid_hash, GINT_TO_POINTER(info->pid)))
		{
			g_ptr_array_add (removal_list, info);
			/* remove all children from tree */
			if (info->is_visible) {
				GtkTreeIter child;
				if (gtk_tree_model_iter_children (model, &child, &info->node))
					remove_children_from_tree (procdata, model, &child, addition_list);
			}
		}
	}

	g_hash_table_destroy(pid_hash);

	g_ptr_array_foreach(removal_list, (GFunc) cb_exclude, addition_list);

	/* Add or remove processes from the tree */
	g_ptr_array_foreach(removal_list,  (GFunc) remove_info_from_tree, procdata);
	g_ptr_array_foreach(addition_list, (GFunc) insert_info_to_tree  , procdata);

	/* Remove processes from the internal GList */
	g_ptr_array_foreach(removal_list,  (GFunc) remove_info_from_list, procdata);

	g_ptr_array_free (addition_list, TRUE);
	g_ptr_array_free (removal_list, TRUE);
}


void
proctable_update_list (ProcData * const procdata)
{
	unsigned *pid_list;
	glibtop_proclist proclist;
	glibtop_cpu cpu;
	gint which, arg;

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

	/* FIXME: total cpu time elapsed should be calculated on an individual basis here
	** should probably have a total_time_last gint in the ProcInfo structure */
	glibtop_get_cpu (&cpu);
	total_time = cpu.total - total_time_last;
	total_time_last = cpu.total;

	refresh_list (procdata, pid_list, proclist.number);

	g_free (pid_list);

	/* proclist.number == g_list_length(procdata->info) == g_hash_table_size(procdata->pids) */
}


void
proctable_update_all (ProcData * const procdata)
{
	proctable_update_list (procdata);

	if (procdata->config.show_more_info)
		infoview_update (procdata);
}


void
proctable_clear_tree (ProcData * const procdata)
{
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));

	gtk_tree_store_clear (GTK_TREE_STORE (model));

	proctable_free_table (procdata);

	update_sensitivity (procdata, FALSE);
}


void
proctable_free_table (ProcData * const procdata)
{
	GList *list = procdata->info;

	while (list)
	{
		ProcInfo *info = list->data;
		proctable_free_info (procdata, info);
		list = g_list_next (list);
	}

	g_list_free (procdata->info);
	procdata->info = NULL;

	g_hash_table_destroy(procdata->pids);
	procdata->pids = g_hash_table_new(g_direct_hash, g_direct_equal);
}


void
proctable_search_table (ProcData *procdata, gchar *string)
{
	GList *list = procdata->info;
	GtkWidget *dialog;
	GtkTreeModel *model;
	static gint increment = 0;
	gint index;
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
		if (strstr (info->name, string) && info->is_visible)
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

		if (info->user && strstr (info->user, string) && info->is_visible)
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
	        /*translators: primary alert message*/
		dialog = gtk_message_dialog_new (GTK_WINDOW (procdata->app),
						 GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						 _("Could not find \"%s\""), string);
		gtk_message_dialog_format_secondary_text (
			GTK_MESSAGE_DIALOG (dialog),
			_("There are no processes containing the searched string. "
			  "Please note that the search is performed only on "
			  "processes shown in the process list."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	else {
		/* no more cases found. Start the search anew */
		increment = -1;
		proctable_search_table (procdata, string);
		return;
	}

	increment --;
}

