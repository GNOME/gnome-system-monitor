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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <glibtop.h>
#include <gal/widgets/e-cursors.h>
#include "procman.h"
#include "proctable.h"
#include "interface.h"
#include "favorites.h"
#include "prettytable.h"

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
		prettytable_load_async (procdata);
	}
	return FALSE;
}

static gint
icon_load_finished (gpointer data)
{
	ProcData *procdata = data;
	
	if (procdata->pretty_table)
	{
		proctable_update_all (procdata);
		return FALSE;
	}
	
	return TRUE;
}

static ProcData *
procman_data_new (void)
{

	ProcData *pd;

	pd = g_new0 (ProcData, 1);

	pd->tree = NULL;
	pd->model = NULL;
	pd->memory = NULL;
	pd->infobox = NULL;
	pd->info = NULL;
	pd->proc_num = 0;
	pd->selected_pid = -1;
	pd->selected_node = NULL;
	pd->timeout = -1;
	pd->favorites = NULL;
	pd->blacklist = NULL;
	
	pd->config.show_more_info = 
		gnome_config_get_bool ("procman/Config/more_info=FALSE");
	pd->config.show_tree = 
		gnome_config_get_bool ("procman/Config/show_tree=TRUE");
	pd->config.show_kill_warning = 
		gnome_config_get_bool ("procman/Config/kill_dialog=TRUE");
	pd->config.show_hide_message = 
		gnome_config_get_bool ("procman/Config/hide_message=TRUE");
	pd->config.delay_load = 
		gnome_config_get_bool ("procman/Config/delay_load=TRUE");
	pd->config.load_desktop_files = 
		gnome_config_get_bool ("procman/Config/load_desktop_files=TRUE");
	pd->config.show_pretty_names = 
		gnome_config_get_bool ("procman/Config/show_pretty_names=TRUE");
	pd->config.show_threads = 
		gnome_config_get_bool ("procman/Config/show_threads=FALSE");
	pd->config.update_interval = 
		gnome_config_get_int ("procman/Config/update_interval=3000");
	pd->config.graph_update_interval = 
		gnome_config_get_int ("procman/Config/graph_update_interval=500");
	pd->config.disks_update_interval = 
		gnome_config_get_int ("procman/Config/disks_update_interval=5000");
	pd->config.whose_process = gnome_config_get_int ("procman/Config/view_as=1");
	pd->config.current_tab = gnome_config_get_int ("procman/Config/current_tab=0");
	pd->config.bg_color.red = gnome_config_get_int
		("procman/Config/bg_red=0");
	pd->config.bg_color.green = gnome_config_get_int
		("procman/Config/bg_green=0");
	pd->config.bg_color.blue= gnome_config_get_int
		("procman/Config/bg_blue=0");
	pd->config.frame_color.red = gnome_config_get_int
		("procman/Config/frame_red=20409");
	pd->config.frame_color.green = gnome_config_get_int
		("procman/Config/frame_green=32271");
	pd->config.frame_color.blue = gnome_config_get_int
		("procman/Config/frame_blue=17781");
	pd->config.cpu_color.red = gnome_config_get_int
		("procman/Config/cpu_red=65535");
	pd->config.cpu_color.green = gnome_config_get_int
		("procman/Config/cpu_green=591");
	pd->config.cpu_color.blue = gnome_config_get_int
		("procman/Config/cpu_blue=0");
	pd->config.mem_color.red = gnome_config_get_int
		("procman/Config/mem_red=65535");
	pd->config.mem_color.green = gnome_config_get_int
		("procman/Config/mem_green=591");
	pd->config.mem_color.blue = gnome_config_get_int
		("procman/Config/mem_blue=0");
	pd->config.swap_color.red = gnome_config_get_int
		("procman/Config/swap_red=1363");
	pd->config.swap_color.green = gnome_config_get_int
		("procman/Config/swap_green=52130");
	pd->config.swap_color.blue = gnome_config_get_int
		("procman/Config/swap_blue=18595");
	
	procman_get_save_files (pd);
#if 0	
	get_favorites (pd);
#endif

	get_blacklist (pd);	
	
	/* delay load the .desktop files. Procman will display icons and app names
	** if the pd->pretty_table structure is not NULL. The timeout will monitor
	** whether or not that becomes the case and update the table if so. There is 
	** undoubtedly a better solution, but I am not too knowlegabe about threads.
	** Basically what needs to be done is to update the table when the thread is
	** finished 
	*/
	if (pd->config.load_desktop_files && pd->config.delay_load)
	{
		pd->pretty_table = NULL;
		gtk_idle_add_priority (800, idle_func, pd);
		gtk_timeout_add (500, icon_load_finished, pd);
	}
	else if (pd->config.load_desktop_files && !pd->config.delay_load)
		pd->pretty_table = pretty_table_new ();
	else
		pd->pretty_table = NULL;	
	

	return pd;

}

static void
procman_free_data (ProcData *procdata)
{

	proctable_free_table (procdata);
	
	pretty_table_free (procdata->pretty_table);
	
	g_free (procdata);
	
}

void
procman_save_config (ProcData *data)
{

	if (!data)
		return;
	gnome_config_set_int ("procman/Config/view_as",data->config.whose_process);
	gnome_config_set_bool ("procman/Config/more_info", data->config.show_more_info);
	gnome_config_set_bool ("procman/Config/kill_dialog", data->config.show_kill_warning);
	gnome_config_set_bool ("procman/Config/hide_message", data->config.show_hide_message);
	gnome_config_set_bool ("procman/Config/show_tree", data->config.show_tree);
	gnome_config_set_bool ("procman/Config/delay_load", data->config.delay_load);
	gnome_config_set_bool ("procman/Config/load_desktop_files", data->config.load_desktop_files);
	gnome_config_set_bool ("procman/Config/show_pretty_names", data->config.show_pretty_names);
	gnome_config_set_bool ("procman/Config/show_threads", data->config.show_threads);
	gnome_config_set_int ("procman/Config/update_interval", data->config.update_interval);
	gnome_config_set_int ("procman/Config/graph_update_interval", 
			      data->config.graph_update_interval);
	gnome_config_set_int ("procman/Config/disks_update_interval", 
			      data->config.disks_update_interval);
	gnome_config_set_int ("procman/Config/current_tab", data->config.current_tab);
	/*gnome_config_set_string ("procman/Config/bg_color", data->config.bg_color);
	gnome_config_set_string ("procman/Config/cpu_color", data->config.cpu_color);
	gnome_config_set_string ("procman/Config/mem_color", data->config.mem_color);
	gnome_config_set_string ("procman/Config/swap_color", data->config.swap_color);
	gnome_config_set_string ("procman/Config/frame_color", data->config.frame_color);*/
	
	gnome_config_set_int ("procman/Config/bg_red", data->config.bg_color.red);
	gnome_config_set_int ("procman/Config/bg_green", data->config.bg_color.green);
	gnome_config_set_int ("procman/Config/bg_blue", data->config.bg_color.blue);
	gnome_config_set_int ("procman/Config/frame_red", data->config.frame_color.red);
	gnome_config_set_int ("procman/Config/frame_green", data->config.frame_color.green);
	gnome_config_set_int ("procman/Config/frame_blue", data->config.frame_color.blue);
	gnome_config_set_int ("procman/Config/cpu_red", data->config.cpu_color.red);
	gnome_config_set_int ("procman/Config/cpu_green", data->config.cpu_color.green);
	gnome_config_set_int ("procman/Config/cpu_blue", data->config.cpu_color.blue);
	gnome_config_set_int ("procman/Config/mem_red", data->config.mem_color.red);
	gnome_config_set_int ("procman/Config/mem_green", data->config.mem_color.green);
	gnome_config_set_int ("procman/Config/mem_blue", data->config.mem_color.blue);
	gnome_config_set_int ("procman/Config/swap_red", data->config.swap_color.red);
	gnome_config_set_int ("procman/Config/swap_green", data->config.swap_color.green);
	gnome_config_set_int ("procman/Config/swap_blue", data->config.swap_color.blue);
	
	save_blacklist (data);
	gnome_config_sync ();

}

int
main (int argc, char *argv[])
{
	GtkWidget *app1;
	ProcData *procdata;
	
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif

	gnome_init ("procman", VERSION, argc, argv);
		
	e_cursors_init ();

	glibtop_init ();

	procdata = procman_data_new ();

	app1 = create_main_window (procdata);
	if (!app1)
		return 0;
		
  	proctable_update_all (procdata);
 	
	gtk_main ();
	
	e_cursors_shutdown ();
	
	procman_free_data (procdata);
	
	return 0;
}

