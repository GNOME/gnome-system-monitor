/* Procman - main window
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
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>

#include "procman.h"
#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "procactions.h"
#include "load-graph.h"
#include "util.h"
#include "disks.h"

static void	cb_toggle_tree (GtkAction *action, gpointer data);
static void	cb_toggle_threads (GtkAction *action, gpointer data);

static const GtkActionEntry menu_entries[] =
{
	{ "File", NULL, N_("_File") },
	{ "Edit", NULL, N_("_Edit") },
	{ "View", NULL, N_("_View") },
	{ "Help", NULL, N_("_Help") },

	{ "Quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
	  N_("Quit the program"), G_CALLBACK (cb_app_exit) },


	{ "StopProcess", NULL, N_("_Stop Process"), "<control>S",
	  N_("Stop process"), G_CALLBACK(cb_kill_sigstop) },
	{ "ContProcess", NULL, N_("_Continue Process"), "<control>C",
	  N_("Continue process if stopped"), G_CALLBACK(cb_kill_sigcont) },

	{ "EndProcess", NULL, N_("End _Process"), "<control>E",
	  N_("Force process to finish normally"), G_CALLBACK (cb_end_process) },
	{ "KillProcess", NULL, N_("_Kill Process"), "<control>K",
	  N_("Force process to finish immediately"), G_CALLBACK (cb_kill_process) },
	{ "ChangePriority", NULL, N_("_Change Priority..."), "<control>R",
	  N_("Change the order of priority of process"), G_CALLBACK (cb_renice) },
	{ "Preferencies", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL,
	  N_("Configure the application"), G_CALLBACK (cb_edit_preferences) },

	{ "HideProcess", NULL, N_("_Hide Process"), "<control>H",
	  N_("Hide process from list"), G_CALLBACK (cb_hide_process) },
	{ "HiddenProcesses", NULL, N_("_Hidden Processes"), "<control>P",
	  N_("Open the list of currently hidden processes"), G_CALLBACK (cb_show_hidden_processes) },
	{ "MemoryMaps", NULL, N_("_Memory Maps"), "<control>M",
	  N_("Open the memory maps associated with a process"), G_CALLBACK (cb_show_memory_maps) },
	{ "OpenFiles", NULL, N_("Open _Files"), "<control>F",
	  N_("View the files opened by a process"), G_CALLBACK (cb_show_open_files) },

	{ "HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1",
	  N_("Open the manual"), G_CALLBACK (cb_help_contents) },
	{ "About", GTK_STOCK_ABOUT, N_("_About"), NULL,
	  N_("About this application"), G_CALLBACK (cb_about) }
};

static const GtkToggleActionEntry toggle_menu_entries[] =
{
	{ "ShowDependencies", NULL, N_("_Dependencies"), "<control>D",
	  N_("Show parent/child relationship between processes"),
	  G_CALLBACK (cb_toggle_tree), TRUE },
	{ "ShowThreads", NULL, N_("_Threads"), "<control>T",
	  N_("Show each thread as a separate process"),
	  G_CALLBACK (cb_toggle_threads), FALSE }
};


static const GtkRadioActionEntry radio_menu_entries[] =
{
  { "ShowActiveProcesses", NULL, N_("_Active Processes"), NULL,
    N_("Show active processes"), ACTIVE_PROCESSES },
  { "ShowAllProcesses", NULL, N_("A_ll Processes"), NULL,
    N_("Show all processes"), ALL_PROCESSES },
  { "ShowMyProcesses", NULL, N_("M_y Processes"), NULL,
    N_("Show user own process"), MY_PROCESSES }
};


static const char ui_info[] =
"  <menubar name=\"MenuBar\">"
"    <menu name=\"FileMenu\" action=\"File\">"
"      <menuitem name=\"FileQuitMenu\" action=\"Quit\" />"
"    </menu>"
"    <menu name=\"EditMenu\" action=\"Edit\">"
"      <menuitem name=\"EditStopProcessMenu\" action=\"StopProcess\" />"
"      <menuitem name=\"EditContProcessMenu\" action=\"ContProcess\" />"
"      <separator />"
"      <menuitem name=\"EditEndProcessMenu\" action=\"EndProcess\" />"
"      <menuitem name=\"EditKillProcessMenu\" action=\"KillProcess\" />"
"      <separator />"
"      <menuitem name=\"EditChangePriorityMenu\" action=\"ChangePriority\" />"
"      <separator />"
"      <menuitem name=\"EditPreferenciesMenu\" action=\"Preferencies\" />"
"    </menu>"
"    <menu name=\"ViewMenu\" action=\"View\">"
"      <menuitem name=\"ViewActiveProcesses\" action=\"ShowActiveProcesses\" />"
"      <menuitem name=\"ViewAllProcesses\" action=\"ShowAllProcesses\" />"
"      <menuitem name=\"ViewMyProcesses\" action=\"ShowMyProcesses\" />"
"      <separator />"
"      <menuitem name=\"ViewDependenciesMenu\" action=\"ShowDependencies\" />"
"      <menuitem name=\"ViewThreadsMenu\" action=\"ShowThreads\" />"
"      <separator />"
"      <menuitem name=\"ViewHideProcessMenu\" action=\"HideProcess\" />"
"      <menuitem name=\"ViewHiddenProcessesMenu\" action=\"HiddenProcesses\" />"
"      <separator />"
"      <menuitem name=\"ViewMemoryMapsMenu\" action=\"MemoryMaps\" />"
"      <menuitem name=\"ViewOpenFilesMenu\" action=\"OpenFiles\" />"
"    </menu>"
"    <menu name=\"HelpMenu\" action=\"Help\">"
"      <menuitem name=\"HelpContentsMenu\" action=\"HelpContents\" />"
"      <menuitem name=\"HelpAboutMenu\" action=\"About\" />"
"    </menu>"
"  </menubar>"
"  <popup name=\"PopupMenu\" action=\"Popup\">"
"    <menuitem action=\"StopProcess\" />"
"    <menuitem action=\"ContProcess\" />"
"    <separator />"
"    <menuitem action=\"EndProcess\" />"
"    <menuitem action=\"KillProcess\" />"
"    <separator />"
"    <menuitem action=\"ChangePriority\" />"
"    <separator />"
"    <menuitem action=\"HideProcess\" />"
"    <separator />"
"    <menuitem action=\"MemoryMaps\" />"
"    <menuitem action=\"OpenFiles\" />"
"  </popup>";


static GtkWidget *
create_proc_view (ProcData *procdata)
{
	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GtkWidget *scrolled;
	GtkWidget *hbox2;
	char* string;

	vbox1 = gtk_vbox_new (FALSE, 18);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 12);
	
	hbox1 = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);

	string = make_loadavg_string ();
	procdata->loadavg = gtk_label_new (string);
	g_free (string);
	gtk_box_pack_start (GTK_BOX (hbox1), procdata->loadavg, FALSE, FALSE, 0);

	gtk_widget_show_all (hbox1);

	scrolled = proctable_new (procdata);
	if (!scrolled)
		return NULL;
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                             GTK_SHADOW_IN);

	gtk_box_pack_start (GTK_BOX (vbox1), scrolled, TRUE, TRUE, 0);
	
	gtk_widget_show_all (scrolled);
	
	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, FALSE, 0);
	
	procdata->endprocessbutton = gtk_button_new_with_mnemonic (_("End _Process"));
	gtk_box_pack_end (GTK_BOX (hbox2), procdata->endprocessbutton, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (procdata->endprocessbutton), "clicked",
			  G_CALLBACK (cb_end_process_button_pressed), procdata);
	
	gtk_widget_show_all (hbox2);
	
	/* create popup_menu */
 	procdata->popup_menu = gtk_ui_manager_get_widget (procdata->uimanager, "/PopupMenu");

        return vbox1;
}


GtkWidget *
make_title_label (const char *text)
{
  GtkWidget *label;
  char *full;

  full = g_strdup_printf ("<span weight=\"bold\">%s</span>", text);
  label = gtk_label_new (full);
  g_free (full);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

  return label;
}

/* Make sure the cpu labels don't jump around. From the clock applet */
static void
cpu_size_request (GtkWidget *box, GtkRequisition *req, ProcData *procdata)
{
	req->width = procdata->cpu_label_fixed_width = \
		MAX(req->width, procdata->cpu_label_fixed_width);
}

static void
net_size_request (GtkWidget *label, GtkRequisition *req, ProcData *procdata)
{
	req->width = procdata->net_label_fixed_width = \
		MAX(req->width, procdata->net_label_fixed_width);
}




static GtkWidget *
create_sys_view (ProcData *procdata)
{
	GtkWidget *vbox, *hbox;
	GtkWidget *cpu_box, *mem_box, *net_box;
	GtkWidget *cpu_hbox, *mem_hbox, *net_hbox;
	GtkWidget *cpu_graph_box, *mem_graph_box, *net_graph_box;
	GtkWidget *label,*cpu_label, *spacer;
	GtkWidget *table;
	GtkWidget *color_picker;
	GtkSizeGroup *sizegroup;
	LoadGraph *cpu_graph, *mem_graph, *net_graph;
	gint i;


	vbox = gtk_vbox_new (FALSE, 18);

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

	/* The CPU BOX */

	cpu_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), cpu_box, TRUE, TRUE, 0);

	label = make_title_label (_("CPU History"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (cpu_box), label, FALSE, FALSE, 0);
	
	cpu_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (cpu_box), cpu_hbox, TRUE, TRUE, 0);

	spacer = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (cpu_hbox), spacer, FALSE, FALSE, 0);

	cpu_graph_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (cpu_hbox), cpu_graph_box, TRUE, TRUE, 0);

	cpu_graph = load_graph_new (LOAD_GRAPH_CPU, procdata);
	gtk_box_pack_start (GTK_BOX (cpu_graph_box),
			    load_graph_get_widget(cpu_graph),
			    TRUE,
			    TRUE,
			    0);

	sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (cpu_graph_box), hbox, FALSE, FALSE, 0);

	for (i=0;i<procdata->config.num_cpus; i++) {
		GtkWidget *temp_hbox;
		gchar *text;
		
		temp_hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), temp_hbox, FALSE, FALSE, 0);
		gtk_size_group_add_widget (sizegroup, temp_hbox);
		g_signal_connect (G_OBJECT (temp_hbox), "size_request",
					 G_CALLBACK (cpu_size_request), procdata);

		color_picker = gtk_color_button_new_with_color (
			&load_graph_get_colors(cpu_graph)[2+i]);

		g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_cpu_color_changed), GINT_TO_POINTER (i));
		gtk_box_pack_start (GTK_BOX (temp_hbox), color_picker, FALSE, FALSE, 0);
		
		if(procdata->config.num_cpus == 1) {
			text = g_strdup (_("CPU:"));
		}
		else {
			text = g_strdup_printf (_("CPU%d:"), i+1);
		}
		label = gtk_label_new (text);
		gtk_box_pack_start (GTK_BOX (temp_hbox), label, FALSE, FALSE, 6);
		g_free (text);
		
		cpu_label = gtk_label_new (NULL);
		gtk_box_pack_start (GTK_BOX (temp_hbox), cpu_label, FALSE, FALSE, 0);
		load_graph_get_labels(cpu_graph)->cpu[i] = cpu_label;
		
	}
	
	
	
	procdata->cpu_graph = cpu_graph;


	mem_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), mem_box, TRUE, TRUE, 0);

	label = make_title_label (_("Memory and Swap History"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (mem_box), label, FALSE, FALSE, 0);
	
	mem_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (mem_box), mem_hbox, TRUE, TRUE, 0);

	spacer = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (mem_hbox), spacer, FALSE, FALSE, 0);

	mem_graph_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (mem_hbox), mem_graph_box, TRUE, TRUE, 0);


	mem_graph = load_graph_new (LOAD_GRAPH_MEM, procdata);
	gtk_box_pack_start (GTK_BOX (mem_graph_box),
			    load_graph_get_widget(mem_graph),
			    TRUE,
			    TRUE,
			    0);
	
	table = gtk_table_new (2, 7, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_box_pack_start (GTK_BOX (mem_graph_box), table, 
			    FALSE, FALSE, 0);

	color_picker = gtk_color_button_new_with_color (
		&load_graph_get_colors(mem_graph)[2]);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_mem_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 0, 1, 0, 1, 0, 0, 0, 0);
	
	label = gtk_label_new (_("User memory:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
	
	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(mem_graph)->memused),
				0.0,
				0.5);
	gtk_table_attach (GTK_TABLE (table),
			  load_graph_get_labels(mem_graph)->memused,
			  2,
			  3,
			  0,
			  1,
			  GTK_FILL,
			  0,
			  0,
			  0);
	
	label = gtk_label_new (_("of"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);
	

	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(mem_graph)->memtotal),
				0.0,
				0.5);

	gtk_table_attach (GTK_TABLE (table),
			  load_graph_get_labels(mem_graph)->memtotal,
			  4,
			  5,
			  0,
			  1,
			  GTK_FILL,
			  0,
			  0,
			  0);
	

	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(mem_graph)->mempercent),
					  1.0,
					  0.5);
	gtk_table_attach (GTK_TABLE (table),
			  load_graph_get_labels(mem_graph)->mempercent,
			  5,
			  6,
			  0,
			  1,
			  GTK_FILL,
			  0,
			  0,
			  0);

	color_picker = gtk_color_button_new_with_color (
		&load_graph_get_colors(mem_graph)[3]);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_swap_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 0, 1, 1, 2, 0, 0, 0, 0);
			  
	label = gtk_label_new (_("Used swap:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
	

	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(mem_graph)->swapused ),
				0.0,
				0.5);
	gtk_table_attach (GTK_TABLE (table),
			  load_graph_get_labels(mem_graph)->swapused,
			  2,
			  3,
			  1,
			  2,
			  GTK_FILL,
			  0,
			  0,
			  0);
			  
	label = gtk_label_new (_("of"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);
	

	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(mem_graph)->swaptotal),
				0.0,
				0.5);
	gtk_table_attach (GTK_TABLE (table),
			  load_graph_get_labels(mem_graph)->swaptotal,
			  4,
			  5,
			  1,
			  2,
			  GTK_FILL,
			  0,
			  0,
			  0);


	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(mem_graph)->swappercent),
				1.0,
				0.5);
	gtk_table_attach (GTK_TABLE (table),
			  load_graph_get_labels(mem_graph)->swappercent,
			  5,
			  6,
			  1,
			  2,
			  GTK_FILL,
			  0,
			  0,
			  0);

	procdata->mem_graph = mem_graph;

	/* The net box */
	net_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), net_box, TRUE, TRUE, 0);

	label = make_title_label (_("Network History"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (net_box), label, FALSE, FALSE, 0);

	net_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (net_box), net_hbox, TRUE, TRUE, 0);

	spacer = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (net_hbox), spacer, FALSE, FALSE, 0);

	net_graph_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (net_hbox), net_graph_box, TRUE, TRUE, 0);

	net_graph = load_graph_new (LOAD_GRAPH_NET, procdata);
	gtk_box_pack_start (GTK_BOX (net_graph_box),
			    load_graph_get_widget(net_graph),
			    TRUE,
			    TRUE,
			    0);

	table = gtk_table_new (2, 5, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_box_pack_start (GTK_BOX (net_graph_box), table,
			    FALSE, FALSE, 0);

	color_picker = gtk_color_button_new_with_color (
		&load_graph_get_colors(net_graph)[2]);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_net_in_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 0, 1, 0, 1, 0, 0, 0, 0);

	label = gtk_label_new (_("Received:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	g_signal_connect (G_OBJECT (hbox), "size_request",
			  G_CALLBACK (net_size_request), procdata);


	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(net_graph)->net_in),
				0.0,
				0.5);
	gtk_box_pack_start (GTK_BOX (hbox),
			    load_graph_get_labels(net_graph)->net_in,
			    FALSE,
			    FALSE,
			    0);

	gtk_table_attach (GTK_TABLE (table), hbox, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Total:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(net_graph)->net_in_total),
				0.0,
				0.5);
	gtk_table_attach (GTK_TABLE (table),
			  load_graph_get_labels(net_graph)->net_in_total,
			  4,
			  5,
			  0,
			  1,
			  GTK_FILL,
			  0,
			  0,
			  0);

	color_picker = gtk_color_button_new_with_color (
		&load_graph_get_colors(net_graph)[3]);
	g_signal_connect (G_OBJECT (color_picker), "color_set",
			    G_CALLBACK (cb_net_out_color_changed), procdata);
	gtk_table_attach (GTK_TABLE (table), color_picker, 0, 1, 1, 2, 0, 0, 0, 0);

	label = gtk_label_new (_("Sent:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	g_signal_connect (G_OBJECT (hbox), "size_request",
			  G_CALLBACK (net_size_request), procdata);

	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(net_graph)->net_out),
				0.0,
				0.5);
	gtk_box_pack_start (GTK_BOX (hbox),
			    load_graph_get_labels(net_graph)->net_out,
			    FALSE,
			    FALSE,
			    0);

	gtk_table_attach (GTK_TABLE (table), hbox, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Total:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);

	gtk_misc_set_alignment (GTK_MISC (load_graph_get_labels(net_graph)->net_out_total),
				0.0,
				0.5);
	gtk_table_attach (GTK_TABLE (table),
			  load_graph_get_labels(net_graph)->net_out_total,
			  4,
			  5,
			  1,
			  2,
			  GTK_FILL,
			  0,
			  0,
			  0);

	procdata->net_graph = net_graph;


	gtk_widget_show_all (vbox);

	return vbox;
}

static void
menu_item_select_cb (GtkMenuItem *proxy,
                     ProcData *procdata)
{
	GtkAction *action;
	char *message;

	action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");
	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
	if (message)
	{
		gtk_statusbar_push (GTK_STATUSBAR (procdata->statusbar),
				    procdata->tip_message_cid, message);
		g_free (message);
	}
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy,
                       ProcData *procdata)
{
	gtk_statusbar_pop (GTK_STATUSBAR (procdata->statusbar),
			   procdata->tip_message_cid);
}

static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction *action,
                  GtkWidget *proxy,
                  ProcData *procdata)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), procdata);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), procdata);
	}
}

static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction *action,
                     GtkWidget *proxy,
                     ProcData *procdata)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), procdata);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), procdata);
	}
}

void
create_main_window (ProcData *procdata)
{
	gint width, height;
	GtkWidget *app;
	GtkAction *action;
	GtkWidget *menubar;
	GtkWidget *main_box;
	GtkWidget *notebook;
	GtkWidget *tab_label1, *tab_label2, *tab_label3;
	GtkWidget *vbox1;
	GtkWidget *sys_box, *devices_box;

	app = gnome_app_new ("procman", _("System Monitor"));

	main_box = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), main_box);
	gtk_widget_show (main_box);
	
	width = procdata->config.width;
	height = procdata->config.height;
	gtk_window_set_default_size (GTK_WINDOW (app), width, height);
	gtk_window_set_resizable (GTK_WINDOW (app), TRUE);
	
	/* create the menubar */
	procdata->uimanager = gtk_ui_manager_new ();

	/* show tooltips in the statusbar */
	g_signal_connect (procdata->uimanager, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), procdata);
	g_signal_connect (procdata->uimanager, "disconnect_proxy",
			 G_CALLBACK (disconnect_proxy_cb), procdata);

	gtk_window_add_accel_group (GTK_WINDOW (app),
				    gtk_ui_manager_get_accel_group (procdata->uimanager));

	if (!gtk_ui_manager_add_ui_from_string (procdata->uimanager,
	                                        ui_info,
	                                        -1,
	                                        NULL)) {
		g_error("building menus failed");
	}

	procdata->action_group = gtk_action_group_new ("ProcmanActions");
	gtk_action_group_set_translation_domain (procdata->action_group, NULL);
	gtk_action_group_add_actions (procdata->action_group,
	                              menu_entries,
	                              G_N_ELEMENTS (menu_entries),
	                              procdata);
	gtk_action_group_add_toggle_actions (procdata->action_group,
	                                     toggle_menu_entries,
	                                     G_N_ELEMENTS (toggle_menu_entries),
	                                     procdata);

	gtk_action_group_add_radio_actions (procdata->action_group,
					    radio_menu_entries,
					    G_N_ELEMENTS (radio_menu_entries),
					    procdata->config.whose_process,
					    G_CALLBACK(cb_radio_processes),
					    procdata);

	gtk_ui_manager_insert_action_group (procdata->uimanager,
	                                    procdata->action_group,
	                                    0);

	menubar = gtk_ui_manager_get_widget (procdata->uimanager, "/MenuBar");
	gtk_box_pack_start (GTK_BOX (main_box), menubar, FALSE, FALSE, 0);


	/* create the main notebook */
	notebook = gtk_notebook_new ();
  	gtk_box_pack_start (GTK_BOX (main_box), 
	                    notebook, 
	                    TRUE, 
	                    TRUE, 
	                    0);
	gtk_widget_show (notebook);

	vbox1 = create_proc_view (procdata);
	gtk_widget_show (vbox1);
	tab_label1 = gtk_label_new (_("Processes"));
	gtk_widget_show (tab_label1);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox1, tab_label1);
	
	sys_box = create_sys_view (procdata);
	gtk_widget_show (sys_box);
	tab_label2 = gtk_label_new (_("Resources"));
	gtk_widget_show (tab_label2);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sys_box, tab_label2);

	devices_box = create_disk_view (procdata);
	gtk_widget_show(devices_box);
	tab_label3 = gtk_label_new (_("Devices"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), devices_box, tab_label3);

	g_signal_connect (G_OBJECT (notebook), "switch-page",
			  G_CALLBACK (cb_switch_page), procdata);
	g_signal_connect (G_OBJECT (notebook), "change-current-page",
			  G_CALLBACK (cb_change_current_page), procdata);


	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), procdata->config.current_tab);
	cb_change_current_page (GTK_NOTEBOOK (notebook), procdata->config.current_tab, procdata);
	g_signal_connect (G_OBJECT (app), "delete_event",
                          G_CALLBACK (cb_app_delete),
                          procdata);


	/* create the statusbar */
	procdata->statusbar = gtk_statusbar_new();
	gnome_app_set_statusbar (GNOME_APP (app), procdata->statusbar);
	procdata->tip_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (procdata->statusbar), "tip_message");


	action = gtk_action_group_get_action (procdata->action_group, "ShowDependencies");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      procdata->config.show_tree);

	action = gtk_action_group_get_action (procdata->action_group, "ShowThreads");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      procdata->config.show_threads);


	procdata->app = app;
}

void
do_popup_menu (ProcData *procdata, GdkEventButton *event)
{
	gint button;
	gint event_time;

	if (event) {
		button = event->button;
		event_time = event->time;
	}
	else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (procdata->popup_menu), NULL, NULL,
			NULL, NULL, button, event_time);
}

void
update_sensitivity (ProcData *data, gboolean sensitivity)
{
	GtkAction *action;

	if(data->endprocessbutton) {
		/* avoid error on startup if endprocessbutton
		   has not been built yet */
		gtk_widget_set_sensitive (data->endprocessbutton, sensitivity);
	}

	action = gtk_action_group_get_action (data->action_group, "StopProcess");
	gtk_action_set_sensitive (action, sensitivity);
	action = gtk_action_group_get_action (data->action_group, "ContProcess");
	gtk_action_set_sensitive (action, sensitivity);

	action = gtk_action_group_get_action (data->action_group, "EndProcess");
	gtk_action_set_sensitive (action, sensitivity);
	action = gtk_action_group_get_action (data->action_group, "KillProcess");
	gtk_action_set_sensitive (action, sensitivity);
	action = gtk_action_group_get_action (data->action_group, "ChangePriority");
	gtk_action_set_sensitive (action, sensitivity);
	action = gtk_action_group_get_action (data->action_group, "HideProcess");
	gtk_action_set_sensitive (action, sensitivity);
	action = gtk_action_group_get_action (data->action_group, "HiddenProcesses");
	gtk_action_set_sensitive (action, sensitivity);
	action = gtk_action_group_get_action (data->action_group, "MemoryMaps");
	gtk_action_set_sensitive (action, sensitivity);
	action = gtk_action_group_get_action (data->action_group, "OpenFiles");
	gtk_action_set_sensitive (action, sensitivity);
}

static void		
cb_toggle_tree (GtkAction *action, gpointer data)
{
	ProcData *procdata = data;
	GConfClient *client = procdata->client;
	gboolean show;

	show = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	if (show == procdata->config.show_tree)
		return;

	gconf_client_set_bool (client, "/apps/procman/show_tree", show, NULL);
}

static void		
cb_toggle_threads (GtkAction *action, gpointer data)
{
	ProcData *procdata = data;
	GConfClient *client = procdata->client;
	gboolean show;

	show = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	if (show == procdata->config.show_threads)
		return;

	gconf_client_set_bool (client, "/apps/procman/show_threads", show, NULL);
}
