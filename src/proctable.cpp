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
#include <glibtop/loadavg.h>
#include <glibtop/proclist.h>
#include <glibtop/procstate.h>
#include <glibtop/procmem.h>
#include <glibtop/procmap.h>
#include <glibtop/proctime.h>
#include <glibtop/procuid.h>
#include <glibtop/procargs.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
extern "C" {
#include <libwnck/libwnck.h>
}
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>

#include <libgnomevfs/gnome-vfs-utils.h>

#include "procman.h"
#include "proctable.h"
#include "callbacks.h"
#include "prettytable.h"
#include "util.h"
#include "interface.h"
#include "favorites.h"
#include "selinux.h"
extern "C" {
#include "e_date.h"
}

static guint64 total_time = 1;
static guint64 total_time_last = 1;


static gint
sort_ints (GtkTreeModel *model, GtkTreeIter *itera, GtkTreeIter *iterb, gpointer data)
{
	ProcInfo *infoa = NULL, *infob = NULL;
	const gint col = GPOINTER_TO_INT (data);

	gtk_tree_model_get (model, itera, COL_POINTER, &infoa, -1);
	gtk_tree_model_get (model, iterb, COL_POINTER, &infob, -1);

	g_assert(infoa);
	g_assert(infob);

	switch (col) {
	case COL_VMSIZE:
		return PROCMAN_RCMP(infoa->vmsize, infob->vmsize);

	case COL_MEMRES:
		return PROCMAN_RCMP(infoa->memres, infob->memres);

	case COL_MEMWRITABLE:
		return PROCMAN_RCMP(infoa->memwritable, infob->memwritable);

	case COL_MEMSHARED:
		return PROCMAN_RCMP(infoa->memshared, infob->memshared);

	case COL_MEMXSERVER:
		return PROCMAN_RCMP(infoa->memxserver, infob->memxserver);

	case COL_CPU_TIME:
		return PROCMAN_RCMP(infoa->cpu_time_last, infob->cpu_time_last);

	case COL_START_TIME:
		return PROCMAN_CMP(infoa->start_time, infob->start_time);

	case COL_CPU:
		return PROCMAN_RCMP(infoa->pcpu, infob->pcpu);

	case COL_MEM:
		return PROCMAN_RCMP(infoa->mem, infob->mem);

	default:
		g_assert_not_reached();
		return 0;
	}
}



static GtkWidget *
create_proctree(GtkTreeModel *model)
{
	GtkWidget *proctree;
	GtkWidget* (*sexy_new)(void);
	void (*sexy_set_column)(void*, guint);


	if (FALSE && load_symbols("libsexy.so",
			 "sexy_tree_view_new", &sexy_new,
			 "sexy_tree_view_set_tooltip_label_column", &sexy_set_column,
			 NULL)) {
		proctree = sexy_new();
		gtk_tree_view_set_model(GTK_TREE_VIEW(proctree), model);
		sexy_set_column(proctree, COL_TOOLTIP);
	} else {
		proctree = gtk_tree_view_new_with_model(model);
	}

	return proctree;
}





static void
set_proctree_reorderable(ProcData *procdata)
{
	GList *columns, *col;
	GtkTreeView *proctree;

	proctree = GTK_TREE_VIEW(procdata->tree);

	columns = gtk_tree_view_get_columns (proctree);

	for(col = columns; col; col = col->next)
		gtk_tree_view_column_set_reorderable(static_cast<GtkTreeViewColumn*>(col->data), TRUE);

	g_list_free(columns);
}


static void
cb_columns_changed(GtkTreeView *treeview, gpointer user_data)
{
	ProcData * const procdata = static_cast<ProcData*>(user_data);

	procman_save_tree_state(procdata->client,
				GTK_WIDGET(treeview),
				"/apps/procman/proctree");
}


static GtkTreeViewColumn*
my_gtk_tree_view_get_column_with_sort_column_id(GtkTreeView *treeview, int id)
{
	GList *columns, *it;
	GtkTreeViewColumn *col = NULL;

	columns = gtk_tree_view_get_columns(treeview);

	for(it = columns; it; it = it->next)
	{
		if(gtk_tree_view_column_get_sort_column_id(static_cast<GtkTreeViewColumn*>(it->data)) == id)
		{
			col = static_cast<GtkTreeViewColumn*>(it->data);
			break;
		}
	}

	g_list_free(columns);

	return col;
}


void
proctable_set_columns_order(GtkTreeView *treeview, GSList *order)
{
	GtkTreeViewColumn* last = NULL;
	GSList *it;

	for(it = order; it; it = it->next)
	{
		int id;
		GtkTreeViewColumn *cur;

		id = GPOINTER_TO_INT(it->data);

		g_assert(id >= 0 && id < NUM_COLUMNS);

		cur = my_gtk_tree_view_get_column_with_sort_column_id(treeview, id);

		if(cur && cur != last)
		{
			gtk_tree_view_move_column_after(treeview, cur, last);
			last = cur;
		}
	}
}



GSList*
proctable_get_columns_order(GtkTreeView *treeview)
{
	GList *columns, *col;
	GSList *order = NULL;

	columns = gtk_tree_view_get_columns(treeview);

	for(col = columns; col; col = col->next)
	{
		int id;

		id = gtk_tree_view_column_get_sort_column_id(static_cast<GtkTreeViewColumn*>(col->data));
		order = g_slist_prepend(order, GINT_TO_POINTER(id));
	}

	g_list_free(columns);

	order = g_slist_reverse(order);

	return order;
}


static gboolean
search_equal_func(GtkTreeModel *model,
		  gint column,
		  const gchar *key,
		  GtkTreeIter *iter,
		  gpointer search_data)
{
	char* name;
	char* user;
	gboolean found;

	gtk_tree_model_get(model, iter,
			   COL_NAME, &name,
			   COL_USER, &user,
			   -1);

	found = !((name && strstr(name, key))
		  || (user && strstr(user, key)));

	g_free(name);
	g_free(user);

	return found;
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

	const gchar *titles[] = {
		N_("Process Name"),
		N_("User"),
		N_("Status"),
		N_("Virtual Memory"),
		N_("Resident Memory"),
		N_("Writable Memory"),
		N_("Shared Memory"),
		N_("X Server Memory"),
		/* xgettext:no-c-format */ N_("% CPU"),
		N_("CPU Time"),
		N_("Started"),
		N_("Nice"),
		N_("ID"),
		N_("Security Context"),
		N_("Command Line"),
		N_("Memory"),
		NULL,
		"POINTER"
	};

	g_assert(COL_MEM == 15);

	gint i;

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	model = gtk_tree_store_new (NUM_COLUMNS,
				    G_TYPE_STRING,	/* Process Name */
				    G_TYPE_STRING,	/* User		*/
				    G_TYPE_STRING,	/* Status	*/
				    G_TYPE_STRING,	/* VM Size	*/
				    G_TYPE_STRING,	/* Resident Memory */
				    G_TYPE_STRING,	/* Writable Memory */
				    G_TYPE_STRING,	/* Shared Memory */
				    G_TYPE_STRING,	/* X Server Memory */
				    G_TYPE_UINT,	/* % CPU	*/
				    G_TYPE_STRING,	/* CPU time	*/
				    G_TYPE_STRING,	/* Started	*/
				    G_TYPE_INT,		/* Nice		*/
				    G_TYPE_UINT,	/* ID		*/
				    G_TYPE_STRING,	/* Security Context */
				    G_TYPE_STRING,	/* Arguments	*/
				    G_TYPE_STRING,	/* Memory       */
				    GDK_TYPE_PIXBUF,	/* Icon		*/
				    G_TYPE_POINTER,	/* ProcInfo	*/
				    G_TYPE_STRING	/* Sexy tooltip */
		);

	proctree = create_proctree(GTK_TREE_MODEL(model));
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (proctree),
					     search_equal_func,
					     NULL,
					     NULL);
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
	gtk_tree_view_column_set_title (column, _(titles[0]));
	gtk_tree_view_column_set_sort_column_id (column, COL_NAME);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (column, 1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (proctree), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (proctree), column);


	for (i = COL_USER; i <= COL_MEM; i++) {
		cell_renderer = gtk_cell_renderer_text_new ();

		switch(i)
		{
		case COL_VMSIZE:
		case COL_MEMRES:
		case COL_MEMWRITABLE:
		case COL_MEMSHARED:
		case COL_MEMXSERVER:
		case COL_CPU:
		case COL_NICE:
		case COL_PID:
		case COL_CPU_TIME:
		case COL_MEM:
			g_object_set(G_OBJECT(cell_renderer),
				     "xalign", 1.0f,
				     NULL);
		}

		column = gtk_tree_view_column_new ();
		gtk_tree_view_column_pack_start (column, cell_renderer, FALSE);
		gtk_tree_view_column_set_attributes (column, cell_renderer,
						     "text", i,
						     NULL);
		gtk_tree_view_column_set_title (column, _(titles[i]));
		gtk_tree_view_column_set_sort_column_id (column, i);
		gtk_tree_view_column_set_resizable (column, TRUE);

		switch (i) {
		case COL_ARGS:
			gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
			break;
		}

		gtk_tree_view_column_set_min_width (column, 1);
		gtk_tree_view_append_column (GTK_TREE_VIEW (proctree), column);
	}

	gtk_container_add (GTK_CONTAINER (scrolled), proctree);


	for(i = COL_NAME; i <= COL_MEM; i++)
	{
		switch(i)
		{
		case COL_VMSIZE:
		case COL_MEMRES:
		case COL_MEMWRITABLE:
		case COL_MEMSHARED:
		case COL_MEMXSERVER:
		case COL_CPU_TIME:
		case COL_START_TIME:
		case COL_CPU:
		case COL_MEM:
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

	g_signal_connect (G_OBJECT(proctree), "columns-changed",
			  G_CALLBACK(cb_columns_changed), procdata);

	return scrolled;
}


static void
proctable_free_info(ProcInfo *info)
{
	g_assert(info != NULL);

	if(info->pixbuf) {
		g_object_unref (info->pixbuf);
	}

	g_free (info->name);
	g_free(info->tooltip);
	g_free (info->arguments);
	g_free (info->security_context);
	g_slist_free (info->children);
	g_slice_free(ProcInfo, info);
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
	if (args && *args)
	{
		char* basename;
		basename = g_path_get_basename (args);

		if(g_str_has_prefix (basename, cmd))
		{
			info->name = basename;
			return;
		}

		g_free (basename);
	}

	info->name = g_strdup (cmd);
}



static void
get_process_user(ProcData* procdata, ProcInfo* info, uid_t uid)
{
	struct passwd* pwd;
	char* username;

	if(G_LIKELY(info->uid == uid))
		return;

	info->uid = uid;

	pwd = getpwuid(uid);

	if(pwd && pwd->pw_name)
		username = g_strdup(pwd->pw_name);
	else
		username = g_strdup_printf("%u", (unsigned)uid);

	/* don't free, because info->user belongs to procdata->users */
	info->user = g_string_chunk_insert_const(procdata->users, username);

	g_free(username);
}



static void get_process_memory_writable(ProcInfo *info)
{
	glibtop_proc_map buf;
	glibtop_map_entry *maps;
	unsigned i;

	info->memwritable = 0;

	maps = glibtop_get_proc_map(&buf, info->pid);

	for (i = 0; i < buf.number; ++i) {
#ifdef __linux__
		info->memwritable += maps[i].private_dirty;
#else
		if (maps[i].perm & GLIBTOP_MAP_PERM_WRITE)
			info->memwritable += maps[i].size;
#endif
	}

	g_free(maps);
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

	info->vmsize	= procmem.vsize;
	info->memres	= procmem.resident;
	info->memshared	= procmem.share;

	info->memxserver = xresources.total_bytes_estimate;

	get_process_memory_writable(info);

	info->mem = info->memxserver + info->memwritable;
}



ProcInfo *
proctable_find_process (guint pid, ProcData *procdata)
{
	return static_cast<ProcInfo*>(g_hash_table_lookup(procdata->pids, GINT_TO_POINTER(pid)));
}


static ProcInfo *
find_parent (ProcData *procdata, guint pid)
{
	/* ignore init as a parent process */
	if (pid <= 1)
		return NULL;

	return proctable_find_process(pid, procdata);
}


static inline unsigned divide(unsigned *q, unsigned *r, unsigned d)
{
	*q = *r / d;
	*r = *r % d;
	return *q != 0;
}

/*
 * @param d: duration in centiseconds
 * @type d: unsigned
 */
static char *
format_duration_for_display(unsigned centiseconds)
{
	unsigned weeks = 0, days = 0, hours = 0, minutes = 0, seconds = 0;

	(void)(divide(&seconds, &centiseconds, 100)
	       && divide(&minutes, &seconds, 60)
	       && divide(&hours, &minutes, 60)
	       && divide(&days, &hours, 24)
	       && divide(&weeks, &days, 7));

	if (weeks)
		/* xgettext: weeks, days */
		return g_strdup_printf(_("%uw%ud"), weeks, days);

	if (days)
		/* xgettext: days, hours (0 -> 23) */
		return g_strdup_printf(_("%ud%02uh"), days, hours);

	if (hours)
		/* xgettext: hours (0 -> 23), minutes, seconds */
		return g_strdup_printf(_("%u:%02u:%02u"), hours, minutes, seconds);

	/* xgettext: minutes, seconds, centiseconds */
	return g_strdup_printf(_("%u:%02u.%02u"), minutes, seconds, centiseconds);
}


static void
update_info_mutable_cols(GtkTreeStore *store, ProcData *procdata, ProcInfo *info)
{
	gchar *vmsize, *memres, *memwritable, *memshared, *memxserver,
		*cpu_time, *start_time, *mem;

	vmsize	   = SI_gnome_vfs_format_file_size_for_display (info->vmsize);
	memres	   = SI_gnome_vfs_format_file_size_for_display (info->memres);
	memwritable = SI_gnome_vfs_format_file_size_for_display (info->memwritable);
	memshared  = SI_gnome_vfs_format_file_size_for_display (info->memshared);
	memxserver = SI_gnome_vfs_format_file_size_for_display (info->memxserver);
	mem = SI_gnome_vfs_format_file_size_for_display(info->mem);

	cpu_time = format_duration_for_display(100 * info->cpu_time_last / procdata->frequency);

	/* FIXME: does it worths it to display relative to $now date ?
	   absolute date wouldn't required to be updated on every refresh */
	start_time = procman_format_date_for_display(info->start_time);

	gtk_tree_store_set (store, &info->node,
			    COL_STATUS, info->status,
			    COL_USER, info->user,
			    COL_VMSIZE, vmsize,
			    COL_MEMRES, memres,
			    COL_MEMWRITABLE, memwritable,
			    COL_MEMSHARED, memshared,
			    COL_MEMXSERVER, memxserver,
			    COL_CPU, guint(info->pcpu),
			    COL_CPU_TIME, cpu_time,
			    COL_START_TIME, start_time,
			    COL_NICE, gint(info->nice),
			    COL_MEM, mem,
			    -1);

	/* FIXME: We don't bother updating COL_SECURITYCONTEXT as it can never change.
	   even on fork ? */
	g_free (vmsize);
	g_free (memres);
	g_free (memwritable);
	g_free (memshared);
	g_free (memxserver);
	g_free (cpu_time);
	g_free(start_time);
	g_free(mem);
}



void
insert_info_to_tree (ProcInfo *info, ProcData *procdata)
{
	GtkTreeModel *model;

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
			    COL_NAME, info->name,
			    COL_ARGS, info->arguments,
			    COL_TOOLTIP, info->tooltip,
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

	g_assert(info);

	if (!info->is_visible)
		return;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));

	if (procdata->selected_process == info)
		procdata->selected_process = NULL;

	gtk_tree_store_remove (GTK_TREE_STORE (model), &info->node);

	info->is_visible = FALSE;
}


static void
remove_info_from_list (ProcInfo *info, ProcData *procdata)
{
	GSList *child;
	ProcInfo * const parent = info->parent;

	/* Remove info from parent list */
	if (parent)
		parent->children = g_slist_remove (parent->children, info);

	/* Add any children to parent list */
	for(child = info->children; child; child = g_slist_next (child)) {
		ProcInfo *child_info = static_cast<ProcInfo*>(child->data);
		child_info->parent = parent;
	}

	if(parent) {
		parent->children = g_slist_concat(parent->children,
						  info->children);
		info->children = NULL;
	}

	/* Remove from list */
	procdata->info = g_list_remove (procdata->info, info);
	g_hash_table_remove(procdata->pids, GINT_TO_POINTER(info->pid));
	proctable_free_info(info);
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
		GtkTreeModel *model;
		glibtop_proc_uid procuid;
		glibtop_proc_time proctime;

		glibtop_get_proc_uid (&procuid, info->pid);
		glibtop_get_proc_time (&proctime, info->pid);

		get_process_memory_info(info);
		get_process_user(procdata, info, procstate.uid);

		info->pcpu = (proctime.rtime - info->cpu_time_last) * 100 / total_time;
		info->pcpu = MIN(info->pcpu, 100);

		info->cpu_time_last = proctime.rtime;
		info->nice = procuid.nice;


		model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));

		update_info_mutable_cols(GTK_TREE_STORE (model), procdata, info);
	}
}


static ProcInfo *
procinfo_new (ProcData *procdata, gint pid)
{
	ProcInfo *info;
	glibtop_proc_state procstate;
	glibtop_proc_time proctime;
	glibtop_proc_uid procuid;
	glibtop_proc_args procargs;
	gchar** arguments;

	info = g_slice_new0(ProcInfo);

	info->pid = pid;
	info->uid = -1;

	glibtop_get_proc_state (&procstate, pid);
	glibtop_get_proc_uid (&procuid, pid);
	glibtop_get_proc_time (&proctime, pid);
	arguments = glibtop_get_proc_argv (&procargs, pid, 0);

	/* FIXME : wrong. name and arguments may change with exec* */
	get_process_name (info, procstate.cmd, arguments[0]);
	get_process_user (procdata, info, procstate.uid);

	info->tooltip = g_strjoinv(" ", arguments);
	info->arguments = g_strescape(info->tooltip, "\\\"");
	g_strfreev(arguments);

	info->pcpu = 0;
	info->cpu_time_last = proctime.rtime;
	info->start_time = proctime.start_time;
	info->nice = procuid.nice;

	get_process_memory_info(info);
	get_process_status (info, &procstate);
	get_process_selinux_context (info);

	info->pixbuf = pretty_table_get_icon (procdata->pretty_table, info->name, pid);

	info->parent = find_parent (procdata, procuid.ppid);
	if (info->parent) {
		info->parent->children = g_slist_prepend (info->parent->children, info);
	}

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
			info = procinfo_new (procdata, pid_list[i]);
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

	pid_hash = g_hash_table_new(NULL, NULL);
	for(i = 0; i < n; ++i)
		g_hash_table_insert(pid_hash,
				    GINT_TO_POINTER(pid_list[i]),
				    GINT_TO_POINTER(1 /* !NULL */));


	for(list = procdata->info; list; list = g_list_next (list)) {
		ProcInfo * const info = static_cast<ProcInfo*>(list->data);

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
	total_time = MAX(cpu.total - total_time_last, 1);
	total_time_last = cpu.total;

	refresh_list (procdata, pid_list, proclist.number);

	g_free (pid_list);

	/* proclist.number == g_list_length(procdata->info) == g_hash_table_size(procdata->pids) */
}


void
proctable_update_all (ProcData * const procdata)
{
	char* string;

	string = make_loadavg_string();
	gtk_label_set_text (GTK_LABEL(procdata->loadavg), string);
	g_free (string);

	proctable_update_list (procdata);
}


void
proctable_clear_tree (ProcData * const procdata)
{
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));

	gtk_tree_store_clear (GTK_TREE_STORE (model));

	proctable_free_table (procdata);

	update_sensitivity(procdata);
}


void
proctable_free_table (ProcData * const procdata)
{
	GList *list = procdata->info;

	while (list)
	{
		ProcInfo *info = static_cast<ProcInfo*>(list->data);
		proctable_free_info(info);
		list = g_list_next (list);
	}

	g_list_free (procdata->info);
	procdata->info = NULL;

	g_hash_table_destroy(procdata->pids);
	procdata->pids = g_hash_table_new(NULL, NULL);
}



char*
make_loadavg_string(void)
{
	glibtop_loadavg buf;

	glibtop_get_loadavg(&buf);

	return g_strdup_printf(
		_("Load averages for the last 1, 5, 15 minutes: "
		  "%0.2f, %0.2f, %0.2f"),
		buf.loadavg[0],
		buf.loadavg[1],
		buf.loadavg[2]);
}
