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
#include <gnome.h>
#include <gconf/gconf-client.h>
#include <glibtop/cpu.h>

typedef struct _ProcConfig ProcConfig;
typedef struct _PrettyTable PrettyTable;
typedef struct _LoadGraph LoadGraph;
typedef struct _ProcInfo ProcInfo;
typedef struct _ProcData ProcData;

enum
{
	ALL_PROCESSES,
	MY_PROCESSES,
	ACTIVE_PROCESSES
};

#define NCPUSTATES 4

struct _ProcConfig
{
	gint		width;
	gint		height;
        gboolean        show_more_info;
        gboolean	show_kill_warning;
        gboolean	show_hide_message;
        gboolean	show_tree;
        gboolean	show_threads;
 	gint		update_interval;
 	gint		graph_update_interval;
 	gint		disks_update_interval;
	gint		whose_process;
	gint		current_tab;
	GdkColor	cpu_color[GLIBTOP_NCPU];
	GdkColor	mem_color;
	GdkColor	swap_color;
	GdkColor	bg_color;
	GdkColor	frame_color;
	gboolean	simple_view;
	gint		pane_pos;
	gint 		num_cpus;
};

struct _PrettyTable {
	GHashTable *app_hash;		/* apps gotten from libwnck */      
	GHashTable *default_hash; 	/* defined in defaulttable.h */
};

struct _LoadGraph {
    
    guint n;
    gint type;
    guint speed;
    guint draw_width, draw_height;
    guint num_points;
    guint num_cpus;

    guint allocated;

    GdkColor *colors;
    gfloat **data, **odata;
    guint data_size;
    guint *pos;

    gint colors_allocated;
    GtkWidget *main_widget;
    GtkWidget *disp;
    GtkWidget *cpu_labels[GLIBTOP_NCPU];
    GtkWidget *memused_label;
    GtkWidget *memtotal_label;
    GtkWidget *mempercent_label;
    GtkWidget *swapused_label;
    GtkWidget *swaptotal_label;
    GtkWidget *swappercent_label;
    GdkPixmap *pixmap;
    GdkGC *gc;
    int timer_index;
    
    gboolean draw;

    guint64 cpu_time [GLIBTOP_NCPU] [NCPUSTATES];
    guint64 cpu_last [GLIBTOP_NCPU] [NCPUSTATES];
    gboolean cpu_initialized;       
};

enum
{
	NEEDS_REMOVAL,
	NEEDS_ADDITION,
	NEEDS_NOTHING
};

struct _ProcInfo
{
	GtkTreeIter	node;
	GtkTreePath	*path;
	ProcInfo	*parent;
	GList		*children;

	GdkPixbuf	*pixbuf;
	gchar		*name;
	gchar		*user;
	gchar		*arguments;

	gchar		*status; /* shared, don't free it ! */
	gchar		*security_context;

	guint64		cpu_time_last;

	guint64		mem;
	guint64		vmsize;
	guint64		memres;
	guint64		memshared;
	guint64		memrss;
	guint64		memxserver;

	guint		pcpu; /* 0% - 100% */
	gint		nice;

	guint		pid;

	guint		queue		: 2;
	guint		is_visible	: 1;
	guint		is_running	: 1;
	guint		is_thread	: 1;
	guint		is_blacklisted	: 1;
};

struct _ProcData
{
	GtkWidget	*tree;
	GtkWidget	*infobox;
	GtkWidget	*disk_list;
	ProcConfig	config;
	LoadGraph	*cpu_graph;
	LoadGraph	*mem_graph;
	gint		cpu_label_fixed_width;
	ProcInfo	*selected_process;
	GtkTreeSelection *selection;
	gint		timeout;
	gint		disk_timeout;

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
};

void		procman_save_config (ProcData *data);
void		procman_save_tree_state (GConfClient *client, GtkWidget *tree, gchar *prefix);
gboolean	procman_get_tree_state (GConfClient *client, GtkWidget *tree, gchar *prefix);

#endif /* _PROCMAN_PROCMAN_H_ */
