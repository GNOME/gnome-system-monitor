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

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <gconf/gconf-client.h>
#include <glibtop.h>
#include <pthread.h>
#include "procman.h"
#include "interface.h"
#include "proctable.h"
#include "prettytable.h"
#include "favorites.h"
#include "callbacks.h"

GtkWidget *app;
GConfClient *client;
static int simple_view = FALSE;
pthread_t thread;


struct poptOption options[] = {
  {
    "simple",
    's',
    POPT_ARG_NONE,
    &simple_view,
    0,
    N_("show simple dialog to end processes and logout"),
    NULL,
  },
  {
    NULL,
    '\0',
    0,
    NULL,
    0,
    NULL,
    NULL
  }
};

static void
load_desktop_files (ProcData *pd)
{
	pd->pretty_table = pretty_table_new (pd);
}

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
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
	
}

static void
warning_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	ProcData *procdata = data;
	const gchar *key = gconf_entry_get_key (entry);
	GConfValue *value = gconf_entry_get_value (entry);
	
	if (!g_strcasecmp (key, "/apps/procman/kill_dialog")) {
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
	
	if (!g_strcasecmp (key, "/apps/procman/update_interval")) {
		g_print ("main timeout \n");
		procdata->config.update_interval = gconf_value_get_int (value);
		gtk_timeout_remove (procdata->timeout);
		procdata->timeout = gtk_timeout_add (procdata->config.update_interval, cb_timeout,
					     procdata);
	}
	else if (!g_strcasecmp (key, "/apps/procman/graph_update_interval")){
		g_print ("garph timeout \n");
		gtk_timeout_remove (procdata->cpu_graph->timer_index);
		procdata->cpu_graph->timer_index = -1;
		procdata->cpu_graph->speed = gconf_value_get_int (value);
		gtk_timeout_remove (procdata->mem_graph->timer_index);
		procdata->mem_graph->timer_index = -1;
		procdata->mem_graph->speed = gconf_value_get_int (value);
	
		load_graph_start (procdata->cpu_graph);
		load_graph_start (procdata->mem_graph);	
	}
	else {
		g_print ("disk timeout \n");
		
		procdata->config.disks_update_interval = gconf_value_get_int (value);
		gtk_timeout_remove (procdata->disk_timeout);
		procdata->disk_timeout = gtk_timeout_add (procdata->config.disks_update_interval,
  						   cb_update_disks, procdata);	
		
	}
}

static void
color_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	ProcData *procdata = data;
	const gchar *key = gconf_entry_get_key (entry);
	GConfValue *value = gconf_entry_get_value (entry);
	gint color = gconf_value_get_int (value);
	
	if (!g_strcasecmp (key, "/apps/procman/bg_red")) {
		procdata->cpu_graph->colors[0].red = color;
		procdata->mem_graph->colors[0].red = color;
		procdata->config.bg_color.red = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/bg_green")) {
		procdata->cpu_graph->colors[0].green = color;
		procdata->mem_graph->colors[0].green = color;
		procdata->config.bg_color.green = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/bg_blue")) {
		procdata->cpu_graph->colors[0].blue = color;
		procdata->mem_graph->colors[0].blue = color;
		procdata->config.bg_color.blue = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/frame_red")) {
		procdata->cpu_graph->colors[1].red = color;
		procdata->mem_graph->colors[1].red = color;
		procdata->config.frame_color.red = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/frame_green")) {
		procdata->cpu_graph->colors[1].green = color;
		procdata->mem_graph->colors[1].green = color;
		procdata->config.frame_color.green = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/frame_blue")) {
		procdata->cpu_graph->colors[1].blue = color;
		procdata->mem_graph->colors[1].blue = color;
		procdata->config.frame_color.blue = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/cpu_red")) {
		procdata->cpu_graph->colors[2].red = color;
		procdata->config.cpu_color.red = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/cpu_green")) {
		procdata->cpu_graph->colors[2].green = color;
		procdata->config.cpu_color.green = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/cpu_blue")) {
		procdata->cpu_graph->colors[2].blue = color;
		procdata->config.cpu_color.blue = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/mem_red")) {
		procdata->mem_graph->colors[2].red = color;
		procdata->config.mem_color.red = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/mem_green")) {
		procdata->mem_graph->colors[2].green = color;
		procdata->config.mem_color.green = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/mem_blue")) {
		procdata->mem_graph->colors[2].blue = color;
		procdata->config.mem_color.blue = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/swap_red")) {
		procdata->mem_graph->colors[3].red = color;
		procdata->config.swap_color.red = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/swap_green")) {
		procdata->mem_graph->colors[3].green = color;
		procdata->config.swap_color.green = color;
	}
	else if (!g_strcasecmp (key, "/apps/procman/swap_blue")) {
		procdata->mem_graph->colors[3].blue = color;
		procdata->config.swap_color.blue = color;
	}
	
	procdata->cpu_graph->colors_allocated = FALSE;
	procdata->mem_graph->colors_allocated = FALSE;
	
	
}

static gint 
gconf_client_get_int_with_default (gchar *key, gint def)
{
	GConfValue *value = NULL;
	gint retval;
	
	value = gconf_client_get (client, key, NULL);
	if (!value) {
		return def;
	}
	else {
		retval = gconf_value_get_int(value);
		gconf_value_free (value);
		return retval;
	}
		
}

static gboolean
gconf_client_get_bool_with_default (gchar *key, gboolean def)
{
	GConfValue *value = NULL;
	gboolean retval;
	
	value = gconf_client_get (client, key, NULL);
	if (!value) {
		return def;
	}
	else {
		retval = gconf_value_get_bool(value);
		gconf_value_free (value);
		return retval;
	}
		
}

static ProcData *
procman_data_new (void)
{

	ProcData *pd;
	
	pd = g_new0 (ProcData, 1);
	
	pd->tree = NULL;
	pd->infobox = NULL;
	pd->info = NULL;
	pd->selected_process = NULL;
	pd->timeout = -1;
	pd->blacklist = NULL;
	pd->cpu_graph = NULL;
	pd->mem_graph = NULL;
	pd->disk_timeout = -1;
	
	pd->config.width = gconf_client_get_int_with_default ("/apps/procman/width", 440);
	pd->config.height = gconf_client_get_int_with_default ("/apps/procman/height", 495);
	pd->config.show_more_info = gconf_client_get_bool_with_default 
		("/apps/procman/more_info", FALSE);
	pd->config.show_tree = gconf_client_get_bool_with_default 
		("/apps/procman/show_tree", TRUE);
	gconf_client_notify_add (client, "/apps/procman/show_tree", tree_changed_cb,
				 pd, NULL, NULL);
	pd->config.show_kill_warning = gconf_client_get_bool_with_default 
		("/apps/procman/kill_dialog", TRUE);
	gconf_client_notify_add (client, "/apps/procman/kill_dialog", warning_changed_cb,
				 pd, NULL, NULL);
	pd->config.show_hide_message = gconf_client_get_bool_with_default 
		("/apps/procman/hide_message", TRUE);
	gconf_client_notify_add (client, "/apps/procman/hide_message", warning_changed_cb,
				 pd, NULL, NULL);
	pd->config.show_threads = gconf_client_get_bool_with_default 
		("/apps/procman/show_threads", FALSE);
	gconf_client_notify_add (client, "/apps/procman/show_threads", threads_changed_cb,
				 pd, NULL, NULL);
	pd->config.update_interval = gconf_client_get_int_with_default 
		("/apps/procman/update_interval", 3000);
	gconf_client_notify_add (client, "/apps/procman/update_interval", timeouts_changed_cb,
				 pd, NULL, NULL);
	pd->config.graph_update_interval = gconf_client_get_int_with_default 
		("/apps/procman/graph_update_interval", 1000);
	gconf_client_notify_add (client, "/apps/procman/graph_update_interval", timeouts_changed_cb,
				 pd, NULL, NULL);
	pd->config.disks_update_interval = gconf_client_get_int_with_default 
		("/apps/procman/disks_interval", 5000);
	gconf_client_notify_add (client, "/apps/procman/disks_interval", timeouts_changed_cb,
				 pd, NULL, NULL);
	pd->config.whose_process = gconf_client_get_int_with_default ("/apps/procman/view_as", 1);
	gconf_client_notify_add (client, "/apps/procman/view_as", view_as_changed_cb,
				 pd, NULL, NULL);
	pd->config.current_tab = gconf_client_get_int_with_default ("/apps/procman/current_tab", 1);
	pd->config.pane_pos = gconf_client_get_int_with_default 
		("/apps/procman/pane_pos", 300);
	pd->config.bg_color.red = gconf_client_get_int_with_default
		("/apps/procman/bg_red", 0);
	gconf_client_notify_add (client, "/apps/procman/bg_red", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.bg_color.green = gconf_client_get_int_with_default
		("/apps/procman/bg_green", 0);
	gconf_client_notify_add (client, "/apps/procman/bg_green", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.bg_color.blue= gconf_client_get_int_with_default
		("/apps/procman/bg_blue", 0);
	gconf_client_notify_add (client, "/apps/procman/bg_blue", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.frame_color.red = gconf_client_get_int_with_default
		("/apps/procman/frame_red", 20409);
	gconf_client_notify_add (client, "/apps/procman/frame_red", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.frame_color.green = gconf_client_get_int_with_default
		("/apps/procman/frame_green",32271);
	gconf_client_notify_add (client, "/apps/procman/frame_green", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.frame_color.blue = gconf_client_get_int_with_default
		("/apps/procman/frame_blue", 17781);
	gconf_client_notify_add (client, "/apps/procman/frame_blue", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.cpu_color.red = gconf_client_get_int_with_default
		("/apps/procman/cpu_red", 65535);
	gconf_client_notify_add (client, "/apps/procman/cpu_red", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.cpu_color.green = gconf_client_get_int_with_default
		("/apps/procman/cpu_green", 591);
	gconf_client_notify_add (client, "/apps/procman/cpu_green", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.cpu_color.blue = gconf_client_get_int_with_default
		("/apps/procman/cpu_blue", 0);
	gconf_client_notify_add (client, "/apps/procman/cpu_blue", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.mem_color.red = gconf_client_get_int_with_default
		("/apps/procman/mem_red", 65535);
	gconf_client_notify_add (client, "/apps/procman/mem_red", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.mem_color.green = gconf_client_get_int_with_default
		("/apps/procman/mem_green", 591);
	gconf_client_notify_add (client, "/apps/procman/mem_green", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.mem_color.blue = gconf_client_get_int_with_default
		("/apps/procman/mem_blue", 0);
	gconf_client_notify_add (client, "/apps/procman/mem_blue", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.swap_color.red = gconf_client_get_int_with_default
		("/apps/procman/swap_red", 1363);
	gconf_client_notify_add (client, "/apps/procman/swap_red", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.swap_color.green = gconf_client_get_int_with_default
		("/apps/procman/swap_green", 52130);
	gconf_client_notify_add (client, "/apps/procman/swap_green", color_changed_cb,
				 pd, NULL, NULL);
	pd->config.swap_color.blue = gconf_client_get_int_with_default
		("/apps/procman/swap_blue", 18595);
	gconf_client_notify_add (client, "/apps/procman/swap_blue", color_changed_cb,
				 pd, NULL, NULL);
		
	get_blacklist (pd, client);

	pd->config.simple_view = FALSE;	
	if (pd->config.simple_view) {
		pd->config.width = 325;
		pd->config.height = 400;
		pd->config.whose_process = 1;
		pd->config.show_more_info = FALSE;
		pd->config.show_tree = FALSE;
		pd->config.show_kill_warning = TRUE;
		pd->config.show_threads = FALSE;
		pd->config.current_tab = 0;
	}	

	return pd;

}
#if 0
static void
procman_free_data (ProcData *procdata)
{

	proctable_free_table (procdata);
	
	pretty_table_free (procdata->pretty_table);
	
	g_free (procdata);
	
}
#endif

gboolean
procman_get_tree_state (GtkWidget *tree, gchar *prefix)
{
	GtkTreeModel *model;
	gint sort_col;
	GtkSortType order;
	gchar *key;
	gint i = 0;
	gboolean done = FALSE;
	
	g_return_val_if_fail (tree, FALSE);
	g_return_val_if_fail (prefix, FALSE);
	if (!gconf_client_dir_exists (client, prefix, NULL)) {
		g_print ("dir don't exist \n");
		return FALSE;
	}
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	
	key = g_strdup_printf ("%s/sort_col", prefix);
	sort_col = gconf_client_get_int_with_default (key, -1);
	g_free (key);
	
	key = g_strdup_printf ("%s/sort_order", prefix);
	order = gconf_client_get_int_with_default (key, -1);
	g_free (key);
	
	if (sort_col != -1)
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					      	      sort_col,
					              order);
	
	while (!done) {
		GtkTreeViewColumn *column;
		gint width;
		gboolean visible;
		
		key = g_strdup_printf ("%s/col_%d_width", prefix, i);
		width = gconf_client_get_int_with_default (key, -1);
		g_free (key);
		
		key = g_strdup_printf ("%s/col_%d_visible", prefix, i);
		visible = gconf_client_get_bool_with_default (key, FALSE);
		g_free (key);
		
		if (width != -1) {
			column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), i);
			gtk_tree_view_column_set_visible (column, visible);
			if (width > 0)
				gtk_tree_view_column_set_fixed_width (column, width);
		}
		else
			done = TRUE;
		
		i++;
	}
	
	return TRUE;
}

void
procman_save_tree_state (GtkWidget *tree, gchar *prefix)
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
	gint width, height, pane_pos;

	if (!data)
		return;
		
	if (data->config.simple_view)
		return;
		
	procman_save_tree_state (data->tree, "/apps/procman/proctree");
	procman_save_tree_state (data->disk_list, "/apps/procman/disktree");
		
	gdk_window_get_size (app->window, &width, &height);
	data->config.width = width;
	data->config.height = height;
	
	pane_pos = get_sys_pane_pos ();
	data->config.pane_pos = pane_pos;
	
	gconf_client_set_int (client, "/apps/procman/width", data->config.width, NULL);
	gconf_client_set_int (client, "/apps/procman/height", data->config.height, NULL);	
	gconf_client_set_bool (client, "/apps/procman/more_info", data->config.show_more_info, NULL);
	gconf_client_set_int (client, "/apps/procman/current_tab", data->config.current_tab, NULL);
	gconf_client_set_int (client, "/apps/procman/pane_pos", data->config.pane_pos, NULL);
	
	save_blacklist (data, client);

	gconf_client_suggest_sync (client, NULL);
	

	
}


int
main (int argc, char *argv[])
{
	GnomeProgram *procman;
	ProcData *procdata;
	GValue value = {0,};
	poptContext pctx;
	char **args;
	GTimer *timer = g_timer_new ();
	
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	procman = gnome_program_init ("procman", VERSION, LIBGNOMEUI_MODULE, argc, argv, 
			    	      GNOME_PARAM_POPT_TABLE, options, NULL);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/procman.png");
	/*gnome_init ("procman", VERSION, argc, argv);*/
	g_print ("gconf init \n");
	g_timer_start (timer);		    
	gconf_init (argc, argv, NULL);
			    
	client = gconf_client_get_default ();
	gconf_client_add_dir(client, "/apps/procman", GCONF_CLIENT_PRELOAD_NONE, NULL);
        		    
	/*g_value_init (&value, G_TYPE_POINTER);
  	g_object_get_property (G_OBJECT(procman),
                               GNOME_PARAM_POPT_CONTEXT, &value);
        (poptContext)pctx = g_value_get_pointer (&value);
                               
	args = (char**) poptGetArgs (pctx);
	poptFreeContext(pctx);*/
	g_timer_stop (timer);
	g_print ("gconf done %f \n", g_timer_elapsed (timer, NULL));
	g_timer_start (timer);
	glibtop_init ();
	g_timer_stop (timer);
	g_print ("glibtop done %f \n", g_timer_elapsed (timer, NULL));
	g_timer_start (timer);
	procdata = procman_data_new ();
	g_timer_stop (timer);
	g_print ("data done %f \n", g_timer_elapsed (timer, NULL));
	g_timer_start (timer);
	if (procdata->config.simple_view) 
		app = create_simple_view_dialog (procdata);
	else 
		app = create_main_window (procdata);
	g_timer_stop (timer);
	g_print ("UI doene %f \n", g_timer_elapsed (timer, NULL));
	g_timer_start (timer);
	load_desktop_files (procdata);
	g_print ("desktop files %f \n", g_timer_elapsed (timer, NULL));
	g_timer_start (timer);
	proctable_update_all (procdata);
		 
	g_timer_stop (timer);
	g_print ("table updated %f \n", g_timer_elapsed (timer, NULL));
	g_timer_destroy (timer);
	
	if (!app)
		return 0;  
			
 	gtk_widget_show (app);	
 	
	gtk_main ();
#if 0	
	procman_free_data (procdata);
#endif	
	return 0;
}

