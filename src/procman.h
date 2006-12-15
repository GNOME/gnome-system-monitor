/* Procman
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
#ifndef _PROCMAN_PROCMAN_H_
#define _PROCMAN_PROCMAN_H_



#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <glibtop/cpu.h>

#include <time.h>

typedef struct _ProcConfig ProcConfig;
typedef struct _PrettyTable PrettyTable;
typedef struct _ProcInfo ProcInfo;
typedef struct _ProcData ProcData;

#include "smooth_refresh.h"

#include "load-graph.h"

enum
{
	ALL_PROCESSES,
	MY_PROCESSES,
	ACTIVE_PROCESSES
};

enum
{
	MIN_UPDATE_INTERVAL = 1 * 1000,
	MAX_UPDATE_INTERVAL = 100 * 1000
};


enum ProcmanTab
{
	PROCMAN_TAB_SYSINFO,
	PROCMAN_TAB_PROCESSES,
	PROCMAN_TAB_RESOURCES,
	PROCMAN_TAB_DISKS
};


struct _ProcConfig
{
	gint		width;
	gint		height;
        gboolean	show_kill_warning;
        gboolean	show_hide_message;
        gboolean	show_tree;
	gboolean	show_all_fs;
	guint		update_interval;
 	gint		graph_update_interval;
 	gint		disks_update_interval;
	gint		whose_process;
	gint		current_tab;
	GdkColor	cpu_color[GLIBTOP_NCPU];
	GdkColor	mem_color;
	GdkColor	swap_color;
	GdkColor	net_in_color;
	GdkColor	net_out_color;
	GdkColor	bg_color;
	GdkColor	frame_color;
	gint 		num_cpus;
};


struct _ProcInfo
{
	GtkTreeIter	node;
	GtkTreePath	*path;
	ProcInfo	*parent;
	GSList		*children;

	GdkPixbuf	*pixbuf;
	gchar		*tooltip;
	gchar		*name;
	gchar		*user; /* allocated with g_string_chunk, don't free it ! */
	gchar		*arguments;

	gchar		*status; /* shared, don't free it ! */
	gchar		*security_context;

	time_t		start_time;
	guint64		cpu_time_last;

	guint64		vmsize;
	guint64		memres;
	guint64		memwritable;
	guint64		memshared;
	guint64		mem; /* estimated memory usage */
	unsigned long	memxserver;

	guint		pid;
	guint		uid;

	guint8		pcpu; /* 0% - 100% */
	gint8		nice;

	guint		is_visible	: 1;
	guint		is_running	: 1;
	guint		is_blacklisted	: 1;
};

struct _ProcData
{
	GtkUIManager	*uimanager;
	GtkActionGroup	*action_group;
	GtkWidget	*statusbar;
	gint		tip_message_cid;
	GtkWidget	*tree;
	GtkWidget	*loadavg;
	GtkWidget	*endprocessbutton;
	GtkWidget	*popup_menu;
	GtkWidget	*disk_list;
	ProcConfig	config;
	LoadGraph	*cpu_graph;
	LoadGraph	*mem_graph;
	LoadGraph	*net_graph;
	gint		cpu_label_fixed_width;
	gint		net_label_fixed_width;
	ProcInfo	*selected_process;
	GtkTreeSelection *selection;
	guint		timeout;
	guint		disk_timeout;

/*
   'info' is GList, which has very slow lookup. On each update, glibtop
   retrieves the full system pid list. To update the table,

   foreach pid in glibtop pid list:
	- if there's not ProcInfo with pid, add a new ProcInfo to 'info'
	- else, update the current ProcInfo

   which is very inefficient, because if a process is running at time t,
   it's very likely to be running at t+1.
   So if is length('info') = N, N lookups are performed on each update.
   Average lookup iterates over N / 2 ProcInfo to find that a pid already
   has a ProcInfo.
   -> cost = (N x N) / 2

   With 200 processes, an update costs about 20000 g_list_next(), which
   is huge because we only want to know if we have this pid or not.

   So 'pids' is a hastable to perform fast lookups.

   TODO: 'info' and 'pids' could be merged
*/

	GList		*info;
	GHashTable	*pids;

	PrettyTable	*pretty_table;
	GList		*blacklist;
	gint		blacklist_num;

	GConfClient	*client;
	GtkWidget	*app;
	GtkUIManager	*menu;


	/* cached username */
	GStringChunk	*users;


	/* libgtop uses guint64 but we use a float because
	   frequency is ~always == 100
	   and because we display cpu_time as %.1f seconds
	*/
	float		frequency;

	SmoothRefresh  *smooth_refresh;
};

void		procman_save_config (ProcData *data) G_GNUC_INTERNAL;
void		procman_save_tree_state (GConfClient *client, GtkWidget *tree, const gchar *prefix) G_GNUC_INTERNAL;
gboolean	procman_get_tree_state (GConfClient *client, GtkWidget *tree, const gchar *prefix) G_GNUC_INTERNAL;





struct ReniceArgs
{
	ProcData *procdata;
	int nice_value;
};


struct KillArgs
{
	ProcData *procdata;
	int signal;
};


#endif /* _PROCMAN_PROCMAN_H_ */
