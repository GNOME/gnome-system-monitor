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
#include "defaulttable.h"

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


static ProcData *
procman_data_new (void)
{

	ProcData *procdata;

	procdata = g_new0 (ProcData, 1);

	procdata->tree = NULL;
	procdata->model = NULL;
	procdata->memory = NULL;
	procdata->infobox = NULL;
	procdata->info = NULL;
	procdata->proc_num = 0;
	procdata->selected_pid = -1;
	procdata->selected_node = NULL;
	procdata->timeout = -1;
	
	procdata->config.show_more_info = 
		gnome_config_get_bool ("procman/Config/more_info=FALSE");
	procdata->config.show_tree = 
		gnome_config_get_bool ("procman/Config/show_tree=FALSE");
	procdata->config.show_kill_warning = 
		gnome_config_get_bool ("procman/Config/kill_dialog=TRUE");
	procdata->config.update_interval = 
		gnome_config_get_int ("procman/Config/update_interval=3000");
	procdata->config.whose_process = gnome_config_get_int ("procman/Config/view_as=0");
	
	
	procman_get_save_files (procdata);
	

	return procdata;

}


void
procman_save_config (ProcData *data)
{

	if (!data)
		return;
	gnome_config_set_int ("procman/Config/view_as",data->config.whose_process);
	gnome_config_set_bool ("procman/Config/more_info", data->config.show_more_info);
	gnome_config_set_bool ("procman/Config/kill_dialog", data->config.show_kill_warning);
	gnome_config_set_bool ("procman/Config/show_tree", data->config.show_tree);
	gnome_config_set_int ("procman/Config/update_interval", data->config.update_interval);
	

	gnome_config_sync ();

}

int
main (int argc, char *argv[])
{
	GtkWidget *app1;
	ProcData *procdata;
	gchar *path;

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif

	gnome_init ("procman", VERSION, argc, argv);
		
	e_cursors_init ();

/*	gtk_widget_push_visual (gdk_rgb_get_visual ());
	gtk_widget_push_colormap (gdk_rgb_get_cmap ());*/

	glibtop_init ();

	procdata = procman_data_new ();

	procdata->pretty_table = pretty_table_new ();

	path = gnome_datadir_file ("gnome/apps");
	pretty_table_load_path (procdata->pretty_table, path, TRUE);
	g_free (path);
	path = gnome_datadir_file ("gnome/ximian");
	pretty_table_load_path (procdata->pretty_table, path, TRUE);
	g_free (path);
	
	pretty_table_add_table (procdata->pretty_table, default_table);

	app1 = create_main_window (procdata);
  	proctable_update_all (procdata);
  	
	gtk_main ();
	
	e_cursors_shutdown ();
	
	pretty_table_free (procdata->pretty_table);
		

	
	return 0;
}

