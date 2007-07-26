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
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>

#include <set>
#include <list>

#include <libgnomevfs/gnome-vfs-utils.h>

#include "procman.h"
#include "selection.h"
#include "proctable.h"
#include "callbacks.h"
#include "prettytable.h"
#include "util.h"
#include "interface.h"
#include "selinux.h"
extern "C" {
#include "e_date.h"
}

static guint64 total_time = 1;
static guint64 total_time_last = 1;



ProcInfo::List ProcInfo::all;


ProcInfo* ProcInfo::find(pid_t pid)
{
  Iterator it(ProcInfo::all.find(pid));
  return (it == ProcInfo::all.end() ? NULL : it->second);
}




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
	case COL_CPU_TIME:
		return PROCMAN_RCMP(infoa->cpu_time_last, infob->cpu_time_last);

	case COL_START_TIME:
		return PROCMAN_CMP(infoa->start_time, infob->start_time);

	case COL_CPU:
		return PROCMAN_RCMP(infoa->pcpu, infob->pcpu);
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
		/* translators:no-c-format */ N_("% CPU"),
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
				    G_TYPE_ULONG,	/* VM Size	*/
				    G_TYPE_ULONG,	/* Resident Memory */
				    G_TYPE_ULONG,	/* Writable Memory */
				    G_TYPE_ULONG,	/* Shared Memory */
				    G_TYPE_ULONG,	/* X Server Memory */
				    G_TYPE_UINT,	/* % CPU	*/
				    G_TYPE_STRING,	/* CPU time	*/
				    G_TYPE_STRING,	/* Started	*/
				    G_TYPE_INT,		/* Nice		*/
				    G_TYPE_UINT,	/* ID		*/
				    G_TYPE_STRING,	/* Security Context */
				    G_TYPE_STRING,	/* Arguments	*/
				    G_TYPE_ULONG,	/* Memory       */
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

		GtkCellRenderer *cell;
		GtkTreeViewColumn *col;

		cell = gtk_cell_renderer_text_new();
		col = gtk_tree_view_column_new();
		gtk_tree_view_column_pack_start(col, cell, TRUE);
		gtk_tree_view_column_set_title(col, _(titles[i]));
		gtk_tree_view_column_set_resizable(col, TRUE);
		gtk_tree_view_column_set_sort_column_id(col, i);
		gtk_tree_view_column_set_reorderable(col, TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(proctree), col);

		// type
		switch (i) {
		case COL_VMSIZE:
		case COL_MEMRES:
		case COL_MEMWRITABLE:
		case COL_MEMSHARED:
		case COL_MEMXSERVER:
		case COL_MEM:
		  gtk_tree_view_column_set_cell_data_func(col, cell,
							  &procman::size_cell_data_func,
							  GUINT_TO_POINTER(i),
							  NULL);
		  break;

		default:
		  gtk_tree_view_column_set_attributes(col, cell, "text", i, NULL);
		  break;
		}

		// xaling
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
			g_object_set(G_OBJECT(cell), "xalign", 1.0f, NULL);
			break;
		}

		// sizing
		switch (i) {
		case COL_ARGS:
		  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
		  gtk_tree_view_column_set_min_width(col, 150);
		  break;
		default:
		  gtk_tree_view_column_set_min_width(column, 20);
		  break;
		}
	}

	gtk_container_add (GTK_CONTAINER (scrolled), proctree);


	for(i = COL_NAME; i <= COL_MEM; i++)
	{
		switch(i)
		{
		case COL_CPU_TIME:
		case COL_START_TIME:
		case COL_CPU:
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


ProcInfo::~ProcInfo()
{
  g_free(this->name);
  g_free(this->tooltip);
  g_free(this->arguments);
  g_free(this->security_context);
}


static void
get_process_status (ProcInfo *info, const glibtop_proc_state *buf)
{

	switch(buf->state)
	{
	case GLIBTOP_PROCESS_RUNNING:
		info->status = _("Running");
		break;

	case GLIBTOP_PROCESS_STOPPED:
		info->status = _("Stopped");
		break;

	case GLIBTOP_PROCESS_ZOMBIE:
		info->status = _("Zombie");
		break;

	case GLIBTOP_PROCESS_UNINTERRUPTIBLE:
		info->status = _("Uninterruptible");
		break;

	default:
		info->status = _("Sleeping");
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
		/* translators: weeks, days */
		return g_strdup_printf(_("%uw%ud"), weeks, days);

	if (days)
		/* translators: days, hours (0 -> 23) */
		return g_strdup_printf(_("%ud%02uh"), days, hours);

	if (hours)
		/* translators: hours (0 -> 23), minutes, seconds */
		return g_strdup_printf(_("%u:%02u:%02u"), hours, minutes, seconds);

	/* translators: minutes, seconds, centiseconds */
	return g_strdup_printf(_("%u:%02u.%02u"), minutes, seconds, centiseconds);
}


static void
update_info_mutable_cols(ProcInfo *info)
{
	ProcData * const procdata = ProcData::get_instance();
	GtkTreeModel *model;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(procdata->tree));

	gchar *cpu_time, *start_time;

	// expects centiseconds
	cpu_time = format_duration_for_display(100 * info->cpu_time_last / procdata->frequency);

	/* FIXME: does it worths it to display relative to $now date ?
	   absolute date wouldn't required to be updated on every refresh */
	start_time = procman_format_date_for_display(info->start_time);

	gtk_tree_store_set(GTK_TREE_STORE(model), &info->node,
			    COL_STATUS, info->status,
			    COL_USER, info->user,
			    COL_VMSIZE, info->vmsize,
			    COL_MEMRES, info->memres,
			    COL_MEMWRITABLE, info->memwritable,
			    COL_MEMSHARED, info->memshared,
			    COL_MEMXSERVER, info->memxserver,
			    COL_CPU, guint(info->pcpu),
			    COL_CPU_TIME, cpu_time,
			    COL_START_TIME, start_time,
			    COL_NICE, gint(info->nice),
			    COL_MEM, info->mem,
			    -1);

	/* FIXME: We don't bother updating COL_SECURITYCONTEXT as it can never change.
	   even on fork ? */
	g_free (cpu_time);
	g_free(start_time);
}



static void
insert_info_to_tree (ProcInfo *info, ProcData *procdata, bool forced = false)
{
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));

	if (procdata->config.show_tree) {

	  ProcInfo *parent = 0;

	  if (not forced)
	    parent = ProcInfo::find(info->ppid);

	  if (parent) {
	    GtkTreePath *parent_node = gtk_tree_model_get_path(model, &parent->node);
	    gtk_tree_store_insert(GTK_TREE_STORE(model), &info->node, &parent->node, 0);

	    if (!gtk_tree_view_row_expanded(GTK_TREE_VIEW(procdata->tree), parent_node))
	      gtk_tree_view_expand_row(GTK_TREE_VIEW(procdata->tree), parent_node, FALSE);
	    gtk_tree_path_free(parent_node);
	  } else
	    gtk_tree_store_insert(GTK_TREE_STORE(model), &info->node, NULL, 0);
	}
	else
		gtk_tree_store_insert (GTK_TREE_STORE (model), &info->node, NULL, 0);

	/* COL_POINTER must be set first, because GtkTreeStore
	 * will call sort_ints as soon as we set the column
	 * that we're sorting on.
	 */

	gtk_tree_store_set (GTK_TREE_STORE (model), &info->node,
			    COL_POINTER, info,
			    COL_NAME, info->name,
			    COL_ARGS, info->arguments,
			    COL_TOOLTIP, info->tooltip,
			    COL_PID, info->pid,
			    COL_SECURITYCONTEXT, info->security_context,
			    -1);

	procdata->pretty_table.set_icon(*info);

	procman_debug("inserted %d%s", info->pid, (forced ? " (forced)" : ""));
}


/* Removing a node with children - make sure the children are queued
** to be readded.
*/
template<typename List>
static void
remove_info_from_tree (ProcData *procdata, GtkTreeModel *model,
		       ProcInfo *current, List &orphans, unsigned lvl = 0)
{
  GtkTreeIter child_node;

  if (std::find(orphans.begin(), orphans.end(), current) != orphans.end()) {
    procman_debug("[%u] %d already removed from tree", lvl, int(current->pid));
    return;
  }

  procman_debug("[%u] pid %d, %d children", lvl, int(current->pid),
		gtk_tree_model_iter_n_children(model, &current->node));

  // it is not possible to iterate&erase over a treeview so instead we
  // just pop one child after another and recursively remove it and
  // its children

  while (gtk_tree_model_iter_children(model, &child_node, &current->node)) {
    ProcInfo *child = 0;
    gtk_tree_model_get(model, &child_node, COL_POINTER, &child, -1);
    remove_info_from_tree(procdata, model, child, orphans, lvl + 1);
  }

  g_assert(not gtk_tree_model_iter_has_child(model, &current->node));

  if (procdata->selected_process == current)
    procdata->selected_process = NULL;

  orphans.push_back(current);
  gtk_tree_store_remove(GTK_TREE_STORE(model), &current->node);
  procman::poison(current->node, 0x69);
}



static void
update_info (ProcData *procdata, ProcInfo *info)
{
	glibtop_proc_state procstate;
	glibtop_proc_uid procuid;
	glibtop_proc_time proctime;

	glibtop_get_proc_state (&procstate, info->pid);
	glibtop_get_proc_uid (&procuid, info->pid);
	glibtop_get_proc_time (&proctime, info->pid);

	get_process_status (info, &procstate);

	get_process_memory_info(info);

	get_process_user(procdata, info, procstate.uid);

	info->pcpu = (proctime.rtime - info->cpu_time_last) * 100 / total_time;
	info->pcpu = MIN(info->pcpu, 100);

	if (procdata->config.solaris_mode)
	  info->pcpu /= procdata->config.num_cpus;

	info->cpu_time_last = proctime.rtime;
	info->nice = procuid.nice;
	info->ppid = procuid.ppid;
}


ProcInfo::ProcInfo(pid_t pid)
  : tooltip(NULL),
    name(NULL),
    user(NULL),
    arguments(NULL),
    status(NULL),
    security_context(NULL),
    pid(pid),
    uid(-1)
{
	ProcInfo * const info = this;
	glibtop_proc_state procstate;
	glibtop_proc_time proctime;
	glibtop_proc_args procargs;
	gchar** arguments;

	glibtop_get_proc_state (&procstate, pid);
	glibtop_get_proc_time (&proctime, pid);
	arguments = glibtop_get_proc_argv (&procargs, pid, 0);

	/* FIXME : wrong. name and arguments may change with exec* */
	get_process_name (info, procstate.cmd, arguments[0]);

	info->tooltip = g_strjoinv(" ", arguments);
	info->arguments = g_strescape(info->tooltip, "\\\"");
	g_strfreev(arguments);

	info->cpu_time_last = proctime.rtime;
	info->start_time = proctime.start_time;

	get_process_selinux_context (info);
}




static void
refresh_list (ProcData *procdata, const pid_t* pid_list, const guint n)
{
  typedef std::list<ProcInfo*> ProcList;
  ProcList addition;

	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (procdata->tree));
	guint i;

	// Add or update processes in the process list
	for(i = 0; i < n; ++i) {
		ProcInfo *info = ProcInfo::find(pid_list[i]);

		if (!info) {
			info = new ProcInfo(pid_list[i]);
			ProcInfo::all[info->pid] = info;
			addition.push_back(info);
		}

		update_info (procdata, info);
	}


	// Remove dead processes from the process list and from the
	// tree. children are queued to be readded at the right place
	// in the tree.

	const std::set<pid_t> pids(pid_list, pid_list + n);

	ProcInfo::Iterator it(ProcInfo::begin());

	while (it != ProcInfo::end()) {
	  ProcInfo * const info = it->second;
	  ProcInfo::Iterator next(it);
	  ++next;

	  if (pids.find(info->pid) == pids.end()) {
	    procman_debug("ripping %d", info->pid);
	    remove_info_from_tree(procdata, model, info, addition);
	    addition.remove(info);
	    ProcInfo::all.erase(it);
	    delete info;
	  }

	  it = next;
	}

	// INVARIANT
	// pid_list == ProcInfo::all + addition


	if (procdata->config.show_tree) {

	// insert process in the tree. walk through the addition list
	// (new process + process that have a new parent). This loop
	// handles the dependencies because we cannot insert a process
	// until its parent is in the tree.

	std::set<pid_t> in_tree(pids);

	for (ProcList::iterator it(addition.begin()); it != addition.end(); ++it)
	  in_tree.erase((*it)->pid);


	while (not addition.empty()) {
	  procman_debug("looking for %d parents", int(addition.size()));
	  ProcList::iterator it(addition.begin());

	  while (it != addition.end()) {
	    procman_debug("looking for %d's parent with ppid %d",
			  int((*it)->pid), int((*it)->ppid));


	    // inserts the process in the treeview if :
	    // - it is init
	    // - its parent is already in tree
	    // - its parent is unreachable
	    //
	    // rounds == 2 means that addition contains processes with
	    // unreachable parents
	    //
	    // FIXME: this is broken if the unreachable parent becomes active
	    // i.e. it gets active or changes ower
	    // so we just clear the tree on __each__ update
	    // see proctable_update_list (ProcData * const procdata)


	    if ((*it)->ppid == 0 or in_tree.find((*it)->ppid) != in_tree.end()) {
	      insert_info_to_tree(*it, procdata);
	      in_tree.insert((*it)->pid);
	      it = addition.erase(it);
	      continue;
	    }

	    ProcInfo *parent = ProcInfo::find((*it)->ppid);
	    // if the parent is unreachable
	    if (not parent) {
		// or std::find(addition.begin(), addition.end(), parent) == addition.end()) {
		insert_info_to_tree(*it, procdata, true);
		in_tree.insert((*it)->pid);
		it = addition.erase(it);
		continue;
	    }

	    ++it;
	  }
	}
	}
	else { 
	  // don't care of the tree
	  for (ProcList::iterator it(addition.begin()); it != addition.end(); ++it)
	    insert_info_to_tree(*it, procdata);
	}


	for (ProcInfo::Iterator it(ProcInfo::begin()); it != ProcInfo::end(); ++it)
		update_info_mutable_cols(it->second);
}


void
proctable_update_list (ProcData * const procdata)
{
	pid_t* pid_list;
	glibtop_proclist proclist;
	glibtop_cpu cpu;
	gint which, arg;
	procman::SelectionMemento selection;

	switch (procdata->config.whose_process) {
	case ALL_PROCESSES:
		which = GLIBTOP_KERN_PROC_ALL;
		arg = 0;
		break;

	case ACTIVE_PROCESSES:
		which = GLIBTOP_KERN_PROC_ALL | GLIBTOP_EXCLUDE_IDLE;
		arg = 0;
		if (procdata->config.show_tree)
		  {
		    selection.save(procdata->tree);
		    proctable_clear_tree(procdata);
		  }
		break;

	default:
		which = GLIBTOP_KERN_PROC_UID;
		arg = getuid ();
		if (procdata->config.show_tree)
		  {
		    selection.save(procdata->tree);
		    proctable_clear_tree(procdata);
		  }
		break;
	}

	pid_list = glibtop_get_proclist (&proclist, which, arg);

	/* FIXME: total cpu time elapsed should be calculated on an individual basis here
	** should probably have a total_time_last gint in the ProcInfo structure */
	glibtop_get_cpu (&cpu);
	total_time = MAX(cpu.total - total_time_last, 1);
	total_time_last = cpu.total;

	refresh_list (procdata, pid_list, proclist.number);

	selection.restore(procdata->tree);

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
  for (ProcInfo::Iterator it(ProcInfo::begin()); it != ProcInfo::end(); ++it)
    delete it->second;

  ProcInfo::all.clear();
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



void
ProcInfo::set_icon(Glib::RefPtr<Gdk::Pixbuf> icon)
{
  this->pixbuf = icon;

  GtkTreeModel *model;
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(ProcData::get_instance()->tree));
  gtk_tree_store_set(GTK_TREE_STORE(model), &this->node,
		     COL_PIXBUF, this->pixbuf->gobj(),
		     -1);
}
