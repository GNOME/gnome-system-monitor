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


#include <gnome.h>
#include <gal/e-table/e-table.h>
#include <gal/e-table/e-tree.h>
#include <gal/e-table/e-tree-memory.h>


typedef struct _ProcConfig ProcConfig;
typedef struct _PrettyTable PrettyTable;
typedef struct _LoadGraph LoadGraph;
typedef struct _ProcInfo ProcInfo;
typedef struct _ProcData ProcData;

enum
{
	ALL_PROCESSES,
	MY_PROCESSES,
	RUNNING_PROCESSES,
	FAVORITE_PROCESSES,
};

#define NCPUSTATES 4

struct _ProcConfig
{
        gboolean        show_more_info;
        gboolean	show_kill_warning;
        gboolean	show_hide_message;
        gboolean	show_tree;
        gboolean	load_desktop_files;
        gboolean	delay_load;
        gboolean	show_pretty_names;
        gboolean	show_threads;
 	gint		update_interval;
 	gint		graph_update_interval;
	gint		whose_process;
	gchar		*tree_state_file;
	gchar		*memmaps_state_file;
	gint		current_tab;
	GdkColor	cpu_color;
	GdkColor	mem_color;
	GdkColor	swap_color;
	GdkColor	bg_color;
	GdkColor	frame_color;
};

struct _PrettyTable {
	GHashTable *cmdline_to_prettyname;
	GHashTable *cmdline_to_prettyicon;
	GHashTable *name_to_prettyicon; /* lower case */
	GHashTable *name_to_prettyname;
};

struct _LoadGraph {
    
    guint n;
    gint type;
    guint speed;
    guint draw_width, draw_height;
    guint num_points;

    guint allocated;

    GdkColor *colors;
    gfloat **data, **odata;
    guint data_size;
    guint *pos;

    gint colors_allocated;
    GtkWidget *main_widget;
    GtkWidget *disp;
    GtkWidget *label;
    GtkWidget *mem_label;
    GtkWidget *swap_label;
    GdkPixmap *pixmap;
    GdkGC *gc;
    int timer_index;
    
    gboolean draw;

    long cpu_time [NCPUSTATES];
    long cpu_last [NCPUSTATES];
    int cpu_initialized;       
};

struct _ProcInfo
{
	ETreePath	node;
	GdkPixbuf	*pixbuf;
	gchar		*name;
	gchar		*name_utf8;
	gchar		*user;
	gchar		*cmd;
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
	gchar		*status;
	gboolean	running;
	gboolean	is_blacklisted;	
};

struct _ProcData
{
	GtkWidget	*tree;
	ETreeModel	*model;
	ETreeMemory	*memory;
	GtkWidget	*infobox;
	ProcConfig	config;
	LoadGraph	*cpu_graph;
	LoadGraph	*mem_graph;
	ETreePath	selected_node;
	gint		selected_pid;
	gint		timeout;
	gint		disk_timeout;
	GList		*info;
	gint		proc_num;
	GtkWidget 	*cpumeter, *memmeter, *swapmeter;
	PrettyTable	*pretty_table;
	GList		*favorites;
	GList		*blacklist;
	gint		blacklist_num;
};

void		procman_save_config (ProcData *data);

#endif
