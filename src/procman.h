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
#ifndef _PROCMAN_H_
#define _PROCMAN_H_

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
    GtkWidget *swapused_label;
    GtkWidget *swaptotal_label;
    GdkPixmap *pixmap;
    GdkGC *gc;
    int timer_index;
    
    gboolean draw;

    long cpu_time [GLIBTOP_NCPU] [NCPUSTATES];
    long cpu_last [GLIBTOP_NCPU] [NCPUSTATES];
    int cpu_initialized;       
};

struct _ProcInfo
{
	GtkTreeIter 	node;
	GtkTreeIter 	parent_node;
	gboolean	visible;
	gboolean	has_parent;
	GdkPixbuf	*pixbuf;
	gchar		*name;
	gchar		*user;
	gchar		*arguments;
	gint		mem;
	gint		cpu;
	gint		pid;
	gint		parent_pid;
	gint		cpu_time_last;
	gint		nice;
	gint		vmsize;
	gint		memres;
	gint		memshared;
	gint		memrss;
        gint            memxserver;
	gchar		*status;
	gboolean	running;
	gboolean	is_thread;
	gboolean	is_blacklisted;
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
	GList		*info;
	PrettyTable	*pretty_table;
	GList		*blacklist;
	gint		blacklist_num;
	GConfClient	*client;
};

void		procman_save_config (ProcData *data);
void		procman_save_tree_state (GConfClient *client, GtkWidget *tree, gchar *prefix);
gboolean	procman_get_tree_state (GConfClient *client, GtkWidget *tree, gchar *prefix);

#endif
