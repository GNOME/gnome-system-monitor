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
#include <gconf/gconf-client.h>
#include <glibtop.h>
#include <pthread.h>
#include "procman.h"
#include "interface.h"
#include "proctable.h"
#include "prettytable.h"
#if 0
#include "favorites.h"
#endif

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


#if 0
static void
procman_get_save_files (ProcData *procdata)
{
	gchar *homedir;
	
	homedir = g_get_home_dir ();
	procdata->config.tree_state_file = g_strconcat (homedir, "/.gnome/", 
							"procman_header_state", NULL);
	procdata->config.memmaps_state_file = g_strconcat (homedir, "/.gnome/",
							   "procman_memmaps_state", NULL);

}

static gint
idle_func (gpointer data)
{
	ProcData *procdata = data;
	
	if (!procdata->pretty_table)
	{
		if (pthread_create (&thread, NULL, (void*) prettytable_load_async,
				   (void*) procdata)) 
			g_print ("error creating new thread \n");
	}
			
	return FALSE;
	
}

static gint
icon_load_finished (gpointer data)
{
	ProcData *procdata = data;
	PrettyTable *table = NULL;
	
	if (procdata->desktop_load_finished)
	{
		if (pthread_join (thread, (void *)&table))
			g_print ("error joining thread \n");
		procdata->pretty_table = (PrettyTable *)table;
		proctable_update_all (procdata);
		return FALSE;
	}
	
	return TRUE;
}
#endif

static void
load_desktop_files (ProcData *pd)
{
	/* delay load the .desktop files. Procman will display icons and app names
	** if the pd->pretty_table structure is not NULL. The timeout will monitor
	** whether or not that becomes the case and update the table if so. There is 
	** undoubtedly a better solution, but I am not too knowlegabe about threads.
	** Basically what needs to be done is to update the table when the thread is
	** finished 
	*/
	#if 0
	if (pd->config.load_desktop_files && pd->config.delay_load)
	{
		pd->pretty_table = NULL;
		pd->desktop_load_finished = FALSE;
		gtk_idle_add_priority (2000, idle_func, pd);
		/*idle_func (pd);*/
		gtk_timeout_add (500, icon_load_finished, pd);
	}
	else if (pd->config.load_desktop_files && !pd->config.delay_load) 
	#endif
		pd->pretty_table = pretty_table_new (pd);
	/*else
		pd->pretty_table = NULL;*/
}

static gint
initial_update_tree (gpointer data)
{
	ProcData *procdata = data;
	
	load_desktop_files (procdata);
	
	proctable_update_all (procdata);
	
	return FALSE;
}

static gint 
get_gconf_int_with_default (gchar *key, gint def)
{
	GConfValue *value = NULL;
	gint retval;
	
	value = gconf_client_get (client, key, NULL);
	if (!value) {
		return def;
	}
	else {
		retval = gconf_value_get_int(value);
		return retval;
	}
		
}

static gboolean
get_gconf_bool_with_default (gchar *key, gboolean def)
{
	GConfValue *value = NULL;
	gboolean retval;
	
	value = gconf_client_get (client, key, NULL);
	if (!value) {
		return def;
	}
	else {
		retval = gconf_value_get_bool(value);
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
	pd->proc_num = 0;
	pd->selected_pid = -1;
	pd->selected_process = NULL;
	pd->timeout = -1;
	pd->favorites = NULL;
	pd->blacklist = NULL;
	pd->cpu_graph = NULL;
	pd->mem_graph = NULL;
	pd->disk_timeout = -1;
	
	pd->config.width = get_gconf_int_with_default ("/apps/procman/width", 440);
	pd->config.height = get_gconf_int_with_default ("/apps/procman/height", 495);
	pd->config.show_more_info = get_gconf_bool_with_default 
		("/apps/procman/more_info", FALSE);
	pd->config.show_tree = get_gconf_bool_with_default 
		("/apps/procman/show_tree", TRUE);
	pd->config.show_kill_warning = get_gconf_bool_with_default 
		("/apps/procman/kill_dialog", TRUE);
	pd->config.show_hide_message = get_gconf_bool_with_default 
		("/apps/procman/hide_message", TRUE);
	pd->config.delay_load = get_gconf_bool_with_default 
		("/apps/procman/delay_load", TRUE);
	pd->config.load_desktop_files = get_gconf_bool_with_default 
		("/apps/procman/load_desktop_files", TRUE);
	pd->config.show_pretty_names = get_gconf_bool_with_default 
		("/apps/procman/show_app_names", FALSE);
	pd->config.show_threads = get_gconf_bool_with_default 
		("/apps/procman/show_threads", FALSE);
	pd->config.update_interval = get_gconf_int_with_default 
		("/apps/procman/update_interval", 3000);
	pd->config.graph_update_interval = get_gconf_int_with_default 
		("/apps/procman/graph_update_interval", 1000);
	pd->config.disks_update_interval = get_gconf_int_with_default 
		("/apps/procman/disks_interval", 5000);
	pd->config.whose_process = get_gconf_int_with_default ("/apps/procman/view_as", 1);
	pd->config.current_tab = get_gconf_int_with_default ("/apps/procman/current_tab", 1);
	pd->config.pane_pos = get_gconf_int_with_default 
		("/apps/procman/pane_pos", 300);
	pd->config.bg_color.red = get_gconf_int_with_default
		("/apps/procman/bg_red", 0);
	pd->config.bg_color.green = get_gconf_int_with_default
		("/apps/procman/bg_green", 0);
	pd->config.bg_color.blue= get_gconf_int_with_default
		("/apps/procman/bg_blue", 0);
	pd->config.frame_color.red = get_gconf_int_with_default
		("/apps/procman/frame_red", 20409);
	pd->config.frame_color.green = get_gconf_int_with_default
		("/apps/procman/frame_green",32271);
	pd->config.frame_color.blue = get_gconf_int_with_default
		("/apps/procman/frame_blue", 17781);
	pd->config.cpu_color.red = get_gconf_int_with_default
		("/apps/procman/cpu_red", 65535);
	pd->config.cpu_color.green = get_gconf_int_with_default
		("/apps/procman/cpu_green", 591);
	pd->config.cpu_color.blue = get_gconf_int_with_default
		("/apps/procman/cpu_blue", 0);
	pd->config.mem_color.red = get_gconf_int_with_default
		("/apps/procman/mem_red", 65535);
	pd->config.mem_color.green = get_gconf_int_with_default
		("/apps/procman/mem_green", 591);
	pd->config.mem_color.blue = get_gconf_int_with_default
		("/apps/procman/mem_blue", 0);
	pd->config.swap_color.red = get_gconf_int_with_default
		("/apps/procman/swap_red", 1363);
	pd->config.swap_color.green = get_gconf_int_with_default
		("/apps/procman/swap_green", 52130);
	pd->config.swap_color.blue = get_gconf_int_with_default
		("/apps/procman/swap_blue", 18595);
		
	pd->config.whose_process = 0;
	pd->config.show_more_info = FALSE;
#if 0
	procman_get_save_files (pd);

	get_blacklist (pd);
#endif	
	pd->config.simple_view = FALSE;	
	if (pd->config.simple_view) {
		pd->config.width = 325;
		pd->config.height = 400;
		pd->config.whose_process = 1;
		pd->config.show_more_info = FALSE;
		pd->config.show_tree = FALSE;
		pd->config.show_kill_warning = TRUE;
		pd->config.show_pretty_names = FALSE;
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


void
procman_save_config (ProcData *data)
{
	gint width, height, pane_pos;

	if (!data)
		return;
		
	if (data->config.simple_view)
		return;
#if 0		
	proctable_save_state (data);
#endif		
	gdk_window_get_size (app->window, &width, &height);
	data->config.width = width;
	data->config.height = height;
	
	pane_pos = get_sys_pane_pos ();
	data->config.pane_pos = pane_pos;
	g_print ("save width %d \n", width);
	gconf_client_set_int (client, "/apps/procman/width", data->config.width, NULL);
	gconf_client_set_int (client, "/apps/procman/height", data->config.height, NULL);
	gconf_client_set_int (client, "/apps/procman/view_as", data->config.whose_process, NULL);
	gconf_client_set_bool (client, "/apps/procman/more_info", data->config.show_more_info, NULL);
	gconf_client_set_bool (client, "/apps/procman/kill_dialog", data->config.show_kill_warning, NULL);
	gconf_client_set_bool (client, "/apps/procman/hide_message", data->config.show_hide_message, NULL);
	gconf_client_set_bool (client, "/apps/procman/show_tree", data->config.show_tree, NULL);
	gconf_client_set_bool (client, "/apps/procman/delay_load", data->config.delay_load, NULL);
	gconf_client_set_bool (client, "/apps/procman/load_desktop_files", 
			       data->config.load_desktop_files, NULL);
	gconf_client_set_bool (client, "/apps/procman/show_app_names", 
			       data->config.show_pretty_names, NULL);
	gconf_client_set_bool (client, "/apps/procman/show_threads", data->config.show_threads, NULL);
	gconf_client_set_int (client, "/apps/procman/update_interval", data->config.update_interval, NULL);
	gconf_client_set_int (client, "/apps/procman/graph_update_interval", 
			      data->config.graph_update_interval, NULL);
	gconf_client_set_int (client, "/apps/procman/disks_interval", 
			      data->config.disks_update_interval, NULL);
	gconf_client_set_int (client, "/apps/procman/current_tab", data->config.current_tab, NULL);
	gconf_client_set_int (client, "/apps/procman/pane_pos", data->config.pane_pos, NULL);
	gconf_client_set_int (client, "/apps/procman/bg_red", data->config.bg_color.red, NULL);
	gconf_client_set_int (client, "/apps/procman/bg_green", data->config.bg_color.green, NULL);
	gconf_client_set_int (client, "/apps/procman/bg_blue", data->config.bg_color.blue, NULL);
	gconf_client_set_int (client, "/apps/procman/frame_red", data->config.frame_color.red, NULL);
	gconf_client_set_int (client, "/apps/procman/frame_green", data->config.frame_color.green, NULL);
	gconf_client_set_int (client, "/apps/procman/frame_blue", data->config.frame_color.blue, NULL);
	gconf_client_set_int (client, "/apps/procman/cpu_red", data->config.cpu_color.red, NULL);
	gconf_client_set_int (client, "/apps/procman/cpu_green", data->config.cpu_color.green, NULL);
	gconf_client_set_int (client, "/apps/procman/cpu_blue", data->config.cpu_color.blue, NULL);
	gconf_client_set_int (client, "/apps/procman/mem_red", data->config.mem_color.red, NULL);
	gconf_client_set_int (client, "/apps/procman/mem_green", data->config.mem_color.green, NULL);
	gconf_client_set_int (client, "/apps/procman/mem_blue", data->config.mem_color.blue, NULL);
	gconf_client_set_int (client, "/apps/procman/swap_red", data->config.swap_color.red, NULL);
	gconf_client_set_int (client, "/apps/procman/swap_green", data->config.swap_color.green, NULL);
	gconf_client_set_int (client, "/apps/procman/swap_blue", data->config.swap_color.blue, NULL);
	
	gconf_client_suggest_sync (client, NULL);
	
#if 0	
	save_blacklist (data);
#endif
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
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif

	procman = gnome_program_init ("procman", VERSION, LIBGNOMEUI_MODULE, argc, argv, 
			    	      GNOME_PARAM_POPT_TABLE, options, NULL);
	/*gnome_init ("procman", VERSION, argc, argv);*/
	g_print ("gconf init \n");
	g_timer_start (timer);		    
	gconf_init (argc, argv, NULL);
			    
	client = gconf_client_get_default ();
	gconf_client_add_dir(client,
                       "/apps/procman",
                       GCONF_CLIENT_PRELOAD_NONE,
                       NULL);
        		    
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
	/* update in idle to satisfy libwnck */
	//gtk_idle_add_priority (200, initial_update_tree, procdata); 
	g_timer_stop (timer);
	g_print ("table updated %f \n", g_timer_elapsed (timer, NULL));
	g_timer_destroy (timer);
	//gtk_tree_view_expand_all (GTK_TREE_VIEW (procdata->tree));
	if (!app)
		return 0;  
			
 	gtk_widget_show (app);	
 	
	gtk_main ();
#if 0	
	procman_free_data (procdata);
#endif	
	return 0;
}

