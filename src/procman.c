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

#include <config.h>

#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gnome.h>
#include <bacon-message-connection.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>
#include <glibtop.h>
#include <glibtop/close.h>
#include <glibtop/loadavg.h>
#include "load-graph.h"
#include "procman.h"
#include "interface.h"
#include "proctable.h"
#include "prettytable.h"
#include "favorites.h"
#include "callbacks.h"
#include "smooth_refresh.h"


static void
tree_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	ProcData *procdata = data;
	GConfValue *value = gconf_entry_get_value (entry);
	
	procdata->config.show_tree = gconf_value_get_bool (value);
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
	
}

static void
threads_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	ProcData *procdata = data;
	GConfValue *value = gconf_entry_get_value (entry);
	
	procdata->config.show_threads = gconf_value_get_bool (value);
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
	
}

static void
view_as_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	ProcData *procdata = data;
	GConfValue *value = gconf_entry_get_value (entry);
	
	procdata->config.whose_process = gconf_value_get_int (value);
	procdata->config.whose_process = CLAMP (procdata->config.whose_process, 0, 2);
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
	
}

static void
warning_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	ProcData *procdata = data;
	const gchar *key = gconf_entry_get_key (entry);
	GConfValue *value = gconf_entry_get_value (entry);
	
	if (g_str_equal (key, "/apps/procman/kill_dialog")) {
		procdata->config.show_kill_warning = gconf_value_get_bool (value);
	}
	else {
		procdata->config.show_hide_message = gconf_value_get_bool (value);
	}
}

static void
timeouts_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	ProcData *procdata = data;
	const gchar *key = gconf_entry_get_key (entry);
	GConfValue *value = gconf_entry_get_value (entry);

	if (g_str_equal (key, "/apps/procman/update_interval")) {
		procdata->config.update_interval = gconf_value_get_int (value);
		procdata->config.update_interval = 
			MAX (procdata->config.update_interval, 1000);

		smooth_refresh_reset(procdata->smooth_refresh);

		gtk_timeout_remove (procdata->timeout);
		procdata->timeout = gtk_timeout_add (procdata->config.update_interval, 
						     cb_timeout, procdata);
	}
	else if (g_str_equal (key, "/apps/procman/graph_update_interval")){
		procdata->config.graph_update_interval = gconf_value_get_int (value);
		procdata->config.graph_update_interval = 
			MAX (procdata->config.graph_update_interval, 
			     250);
		gtk_timeout_remove (procdata->cpu_graph->timer_index);
		procdata->cpu_graph->timer_index = -1;
		procdata->cpu_graph->speed = procdata->config.graph_update_interval;
		
	
		gtk_timeout_remove (procdata->mem_graph->timer_index);
		procdata->mem_graph->timer_index = -1;
		procdata->mem_graph->speed = procdata->config.graph_update_interval;
	
		load_graph_start (procdata->cpu_graph);
		load_graph_start (procdata->mem_graph);	
	}
	else {
		
		procdata->config.disks_update_interval = gconf_value_get_int (value);
		procdata->config.disks_update_interval = 
			MAX (procdata->config.disks_update_interval, 1000);	
		gtk_timeout_remove (procdata->disk_timeout);
		procdata->disk_timeout = 
			gtk_timeout_add (procdata->config.disks_update_interval,
  					 cb_update_disks, procdata);	
		
	}
}

static void
color_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	ProcData * const procdata = data;
	const gchar *key = gconf_entry_get_key (entry);
	GConfValue *value = gconf_entry_get_value (entry);
	const gchar *color = gconf_value_get_string (value);

	if (g_str_equal (key, "/apps/procman/bg_color")) {
		gdk_color_parse (color, &procdata->config.bg_color);
		procdata->cpu_graph->colors[0] = procdata->config.bg_color;
		procdata->mem_graph->colors[0] = procdata->config.bg_color;
	}
	else if (g_str_equal (key, "/apps/procman/frame_color")) {
		gdk_color_parse (color, &procdata->config.frame_color);
		procdata->cpu_graph->colors[1] = procdata->config.frame_color;
		procdata->mem_graph->colors[1] = procdata->config.frame_color;
	}
	else if (g_str_equal (key, "/apps/procman/cpu_color")) {
		gdk_color_parse (color, &procdata->config.cpu_color[0]);
		procdata->cpu_graph->colors[2] = procdata->config.cpu_color[0];
	}
	else if (g_str_has_prefix (key, "/apps/procman/cpu_color")) {
		gint i;

		for (i=1;i<GLIBTOP_NCPU;i++) {
			gchar *cpu_key;
			cpu_key = g_strdup_printf ("/apps/procman/cpu_color%d",i);
			if (g_str_equal (key, cpu_key)) {
				gdk_color_parse (color, &procdata->config.cpu_color[i]);
				procdata->cpu_graph->colors[i+2] = procdata->config.cpu_color[i];
			}
			g_free (cpu_key);
		}
	}
	else if (g_str_equal (key, "/apps/procman/mem_color")) {
		gdk_color_parse (color, &procdata->config.mem_color);
		procdata->mem_graph->colors[2] = procdata->config.mem_color;
	}
	else if (g_str_equal (key, "/apps/procman/swap_color")) {
		gdk_color_parse (color, &procdata->config.swap_color);
		procdata->mem_graph->colors[3] = procdata->config.swap_color;
	}
		
	procdata->cpu_graph->colors_allocated = FALSE;
	procdata->mem_graph->colors_allocated = FALSE;
		
}



static void
show_all_fs_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	ProcData * const procdata = data;
	GConfValue *value = gconf_entry_get_value (entry);

	procdata->config.show_all_fs = gconf_value_get_bool (value);

	cb_update_disks (data);
}


static ProcData *
procman_data_new (GConfClient *client)
{

	ProcData *pd;
	gchar *color;
	gint swidth, sheight;
	gint i;
	glibtop_cpu cpu;
	glibtop_loadavg loadavg;
	gsize nprocs;

	pd = g_new0 (ProcData, 1);
	
	pd->tree = NULL;
	pd->info = NULL;
	pd->pids = g_hash_table_new(g_direct_hash, g_direct_equal);
	pd->selected_process = NULL;
	pd->timeout = -1;
	pd->blacklist = NULL;
	pd->cpu_graph = NULL;
	pd->mem_graph = NULL;
	pd->disk_timeout = -1;

	glibtop_get_loadavg(&loadavg);
	nprocs = (loadavg.nr_tasks ? 1.2f * loadavg.nr_tasks : 100);
	pd->procinfo_allocator = g_mem_chunk_create(ProcInfo, nprocs, G_ALLOC_AND_FREE);

	/* username is usually 8 chars long
	   for caching, we create chunks of 128 chars */
	pd->users = g_string_chunk_new(128);
	/* push empty string */
	g_string_chunk_insert_const(pd->users, "");

	pd->config.width = gconf_client_get_int (client, "/apps/procman/width", NULL);
	pd->config.height = gconf_client_get_int (client, "/apps/procman/height", NULL);
	pd->config.show_more_info = gconf_client_get_bool (client, "/apps/procman/more_info", NULL);
	pd->config.show_tree = gconf_client_get_bool (client, "/apps/procman/show_tree", NULL);
	gconf_client_notify_add (client, "/apps/procman/show_tree", tree_changed_cb,
				 pd, NULL, NULL);
	pd->config.show_kill_warning = gconf_client_get_bool (client, "/apps/procman/kill_dialog", 
							      NULL);
	gconf_client_notify_add (client, "/apps/procman/kill_dialog", warning_changed_cb,
				 pd, NULL, NULL);
	pd->config.show_hide_message = gconf_client_get_bool (client, "/apps/procman/hide_message",
							      NULL);
	gconf_client_notify_add (client, "/apps/procman/hide_message", warning_changed_cb,
				 pd, NULL, NULL);
	pd->config.show_threads = gconf_client_get_bool (client, "/apps/procman/show_threads", NULL);
	gconf_client_notify_add (client, "/apps/procman/show_threads", threads_changed_cb,
				 pd, NULL, NULL);
	pd->config.update_interval = gconf_client_get_int (client, "/apps/procman/update_interval", 
							   NULL);
	gconf_client_notify_add (client, "/apps/procman/update_interval", timeouts_changed_cb,
				 pd, NULL, NULL);
	pd->config.graph_update_interval = gconf_client_get_int (client,
						"/apps/procman/graph_update_interval",
						NULL);
	gconf_client_notify_add (client, "/apps/procman/graph_update_interval", timeouts_changed_cb,
				 pd, NULL, NULL);
	pd->config.disks_update_interval = gconf_client_get_int (client,
								 "/apps/procman/disks_interval",
								 NULL);
	gconf_client_notify_add (client, "/apps/procman/disks_interval", timeouts_changed_cb,
				 pd, NULL, NULL);


	/* /apps/procman/show_all_fs */
	pd->config.show_all_fs = gconf_client_get_bool (
		client, "/apps/procman/show_all_fs",
		NULL);
	gconf_client_notify_add
		(client, "/apps/procman/show_all_fs",
		 show_all_fs_changed_cb, pd, NULL, NULL);


	pd->config.whose_process = gconf_client_get_int (client, "/apps/procman/view_as", NULL);
	gconf_client_notify_add (client, "/apps/procman/view_as", view_as_changed_cb,
				 pd, NULL, NULL);
	pd->config.current_tab = gconf_client_get_int (client, "/apps/procman/current_tab", NULL);

	color = gconf_client_get_string (client, "/apps/procman/bg_color", NULL);
	if (!color)
		color = g_strdup ("#000000");
	gconf_client_notify_add (client, "/apps/procman/bg_color", 
			  	 color_changed_cb, pd, NULL, NULL);
	gdk_color_parse(color, &pd->config.bg_color);
	g_free (color);
	
	color = gconf_client_get_string (client, "/apps/procman/frame_color", NULL);
	if (!color)
		color = g_strdup ("#231e89aa2805");
	gconf_client_notify_add (client, "/apps/procman/frame_color", 
			  	 color_changed_cb, pd, NULL, NULL);
	gdk_color_parse(color, &pd->config.frame_color);
	g_free (color);
	
	color = gconf_client_get_string (client, "/apps/procman/cpu_color", NULL);
	if (!color)
		color = g_strdup ("#0000007200b3");
	gconf_client_notify_add (client, "/apps/procman/cpu_color", 
			  	 color_changed_cb, pd, NULL, NULL);
	gdk_color_parse(color, &pd->config.cpu_color[0]);
	g_free (color);
	
	for (i=1;i<GLIBTOP_NCPU;i++) {
		gchar *key;
		key = g_strdup_printf ("/apps/procman/cpu_color%d", i);
		
		color = gconf_client_get_string (client, key, NULL);
		if (!color)
			color = g_strdup ("#f25915e815e8");
		gconf_client_notify_add (client, key, 
			  	 color_changed_cb, pd, NULL, NULL);
		gdk_color_parse(color, &pd->config.cpu_color[i]);
		g_free (color);
		g_free (key);
	}
	color = gconf_client_get_string (client, "/apps/procman/mem_color", NULL);
	if (!color)
		color = g_strdup ("#000000b3005b");
	gconf_client_notify_add (client, "/apps/procman/mem_color", 
			  	 color_changed_cb, pd, NULL, NULL);
	gdk_color_parse(color, &pd->config.mem_color);
	
	g_free (color);
	
	color = gconf_client_get_string (client, "/apps/procman/swap_color", NULL);
	if (!color)
		color = g_strdup ("#008b000000c3");
	gconf_client_notify_add (client, "/apps/procman/swap_color", 
			  	 color_changed_cb, pd, NULL, NULL);
	gdk_color_parse(color, &pd->config.swap_color);
	g_free (color);
	
	get_blacklist (pd, client);
	
	/* Sanity checks */
	swidth = gdk_screen_width ();
	sheight = gdk_screen_height ();
	pd->config.width = CLAMP (pd->config.width, 50, swidth);
	pd->config.height = CLAMP (pd->config.height, 50, sheight);
	pd->config.update_interval = MAX (pd->config.update_interval, 1000);
	pd->config.graph_update_interval = MAX (pd->config.graph_update_interval, 250);
	pd->config.disks_update_interval = MAX (pd->config.disks_update_interval, 1000);
	pd->config.whose_process = CLAMP (pd->config.whose_process, 0, 2);
	pd->config.current_tab = CLAMP (pd->config.current_tab, 0, 2);
	
	/* Determinie number of cpus since libgtop doesn't really tell you*/
	pd->config.num_cpus = 0;
	glibtop_get_cpu (&cpu);
	pd->frequency = cpu.frequency;
	i=0;
    	while (i < GLIBTOP_NCPU && cpu.xcpu_total[i] != 0) {
    	    pd->config.num_cpus ++;
    	    i++;
    	}
    	if (pd->config.num_cpus == 0)
    		pd->config.num_cpus = 1;

	pd->smooth_refresh = smooth_refresh_new(&pd->config.update_interval);

	return pd;

}

static void
procman_free_data (ProcData *procdata)
{

	proctable_free_table (procdata);
	g_hash_table_destroy(procdata->pids);
	/* pretty_table_free (procdata->pretty_table); */
	smooth_refresh_destroy(procdata->smooth_refresh);
	g_free (procdata);
	
}


gboolean
procman_get_tree_state (GConfClient *client, GtkWidget *tree, const gchar *prefix)
{
	GtkTreeModel *model;
	gint sort_col;
	GtkSortType order;
	gchar *key;
	gint i;
	
	g_return_val_if_fail (tree, FALSE);
	g_return_val_if_fail (prefix, FALSE);
	
	if (!gconf_client_dir_exists (client, prefix, NULL)) 
		return FALSE;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	
	key = g_strdup_printf ("%s/sort_col", prefix);
	sort_col = gconf_client_get_int (client, key, NULL);
	g_free (key);
	
	key = g_strdup_printf ("%s/sort_order", prefix);
	order = gconf_client_get_int (client, key, NULL);
	g_free (key);
	
	if (sort_col != -1)
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					      	      sort_col,
					              order);
	
	for(i = 0; i < NUM_COLUMNS; ++i) {
		GtkTreeViewColumn *column;
		GConfValue *value = NULL;
		gint width;
		gboolean visible;
		
		
		key = g_strdup_printf ("%s/col_%d_width", prefix, i);
		value = gconf_client_get (client, key, NULL);
		g_free (key);
		
		if (value != NULL) {
			width = gconf_value_get_int(value);
			gconf_value_free (value);
		
			key = g_strdup_printf ("%s/col_%d_visible", prefix, i);
			visible = gconf_client_get_bool (client, key, NULL);
			g_free (key);
		
			column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), i);
			if(!column) continue;
			gtk_tree_view_column_set_visible (column, visible);
			if (width > 0)
				gtk_tree_view_column_set_fixed_width (column, width);
		}
	}
	
	return TRUE;
}

void
procman_save_tree_state (GConfClient *client, GtkWidget *tree, const gchar *prefix)
{
	GtkTreeModel *model;
	GList *columns;
	gint sort_col;
	GtkSortType order;
	gint i = 0;
	
	g_return_if_fail (tree);
	g_return_if_fail (prefix);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	if (gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model), &sort_col,
					          &order)) {
		gchar *key;
		
		key = g_strdup_printf ("%s/sort_col", prefix);
		gconf_client_set_int (client, key, sort_col, NULL);
		g_free (key);
		
		key = g_strdup_printf ("%s/sort_order", prefix);
		gconf_client_set_int (client, key, order, NULL);
		g_free (key);
	}			       
	
	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));
	
	while (columns) {
		GtkTreeViewColumn *column = columns->data;
		gboolean visible;
		gint width;
		gchar *key;
		
		visible = gtk_tree_view_column_get_visible (column);
		width = gtk_tree_view_column_get_width (column);
		
		key = g_strdup_printf ("%s/col_%d_width", prefix, i);
		gconf_client_set_int (client, key, width, NULL);
		g_free (key);
		
		key = g_strdup_printf ("%s/col_%d_visible", prefix, i);
		gconf_client_set_bool (client, key, visible, NULL);
		g_free (key);
		
		columns = g_list_next (columns);
		i++;
	}
	
}

void
procman_save_config (ProcData *data)
{
	GConfClient *client = data->client;
	gint width, height;

	g_return_if_fail(data != NULL);
		
	procman_save_tree_state (data->client, data->tree, "/apps/procman/proctree");
	procman_save_tree_state (data->client, data->disk_list, "/apps/procman/disktreenew");
		
	gdk_window_get_size (data->app->window, &width, &height);
	data->config.width = width;
	data->config.height = height;
	
	gconf_client_set_int (client, "/apps/procman/width", data->config.width, NULL);
	gconf_client_set_int (client, "/apps/procman/height", data->config.height, NULL);	
	gconf_client_set_bool (client, "/apps/procman/more_info", data->config.show_more_info, NULL);
	gconf_client_set_int (client, "/apps/procman/current_tab", data->config.current_tab, NULL);
	
	save_blacklist (data, client);

	gconf_client_suggest_sync (client, NULL);
	

	
}


static void
cb_server (const gchar *msg, gpointer user_data)
{
	GdkWindow *window;
	ProcData *procdata;
	guint32 timestamp;

	window = gdk_get_default_root_window ();

	procdata = *(ProcData**)user_data;
	g_assert (procdata != NULL);

	timestamp = strtoul(msg, NULL, 0);

	if(timestamp)
	{
		gdk_x11_window_set_user_time (window, timestamp);
	}
	else
	{
		g_warning ("Couldn't get timestamp");
	}

	gtk_window_present (GTK_WINDOW(procdata->app));
}


static void
init_volume_monitor(ProcData *procdata)
{
	GnomeVFSVolumeMonitor *mon;
	mon = gnome_vfs_get_volume_monitor();

	g_signal_connect(mon, "volume_mounted",
			 G_CALLBACK(cb_volume_mounted_or_unmounted), procdata);

	g_signal_connect(mon, "volume_unmounted",
			 G_CALLBACK(cb_volume_mounted_or_unmounted), procdata);
}


int
main (int argc, char *argv[])
{
	GnomeProgram *procman;
	GConfClient *client;
	ProcData *procdata;
	BaconMessageConnection *conn;

	conn = bacon_message_connection_new ("gnome-system-monitor");
	if (!conn) g_error("Couldn't connect to gnome-system-monitor");

	if (bacon_message_connection_get_is_server (conn))
	{
		bacon_message_connection_set_callback (conn, cb_server, &procdata);
	}
	else /* client */
	{
		const char *desktop_startup_id, *timestamp;

		timestamp = NULL;
		desktop_startup_id = g_getenv("DESKTOP_STARTUP_ID");

		if(desktop_startup_id)
		{
			timestamp = strstr(desktop_startup_id, "_TIME");
		}

		if(!timestamp)
		{
			g_warning ("Couldn't get $DESKTOP_STARTUP_ID");
			timestamp = "0";
		}

		bacon_message_connection_send (conn, timestamp);
		bacon_message_connection_free (conn);
		return 1;
	}

	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	procman = gnome_program_init ("gnome-system-monitor", VERSION, 
				       LIBGNOMEUI_MODULE, argc, argv,
				      GNOME_PARAM_APP_DATADIR,DATADIR, NULL);
	gtk_window_set_default_icon_name ("gnome-monitor");
		    
	gconf_init (argc, argv, NULL);
			    
	client = gconf_client_get_default ();
	gconf_client_add_dir(client, "/apps/procman", GCONF_CLIENT_PRELOAD_NONE, NULL);

	gnome_vfs_init ();
	glibtop_init ();
	
	procdata = procman_data_new (client);
	procdata->client = client;
	pretty_table_new (procdata);

	create_main_window (procdata);
	
	proctable_update_all (procdata);

	init_volume_monitor (procdata);

	g_return_val_if_fail(procdata->app != NULL, 1);
			
 	gtk_widget_show(procdata->app);
 	
	gtk_main ();
	
	procman_free_data (procdata);

	glibtop_close ();
	gnome_vfs_shutdown ();

	return 0;
}

