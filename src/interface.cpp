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

#include <glib/gi18n.h>
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
#include "sysinfo.h"
#include "gsm_color_button.h"

static void     cb_toggle_tree (GtkAction *action, gpointer data);
static void     cb_proc_goto_tab (gint tab);

static const GtkActionEntry menu_entries[] =
{
    // xgettext: noun, top level menu.
    // "File" did not make sense for system-monitor
    { "Monitor", NULL, N_("_Monitor") },
    { "Edit", NULL, N_("_Edit") },
    { "View", NULL, N_("_View") },
    { "Help", NULL, N_("_Help") },

    { "Lsof", GTK_STOCK_FIND, N_("Search for _Open Files"), "<control>O",
      N_("Search for open files"), G_CALLBACK(cb_show_lsof) },
    { "Quit", GTK_STOCK_QUIT, NULL, NULL,
      N_("Quit the program"), G_CALLBACK (cb_app_exit) },


    { "StopProcess", NULL, N_("_Stop Process"), "<control>S",
      N_("Stop process"), G_CALLBACK(cb_kill_sigstop) },
    { "ContProcess", NULL, N_("_Continue Process"), "<control>C",
      N_("Continue process if stopped"), G_CALLBACK(cb_kill_sigcont) },

    { "EndProcess", NULL, N_("_End Process"), "<control>E",
      N_("Force process to finish normally"), G_CALLBACK (cb_end_process) },
    { "KillProcess", NULL, N_("_Kill Process"), "<control>K",
      N_("Force process to finish immediately"), G_CALLBACK (cb_kill_process) },
    { "ChangePriority", NULL, N_("_Change Priority"), NULL,
      N_("Change the order of priority of process"), NULL },
    { "Preferences", GTK_STOCK_PREFERENCES, NULL, NULL,
      N_("Configure the application"), G_CALLBACK (cb_edit_preferences) },

    { "Refresh", GTK_STOCK_REFRESH, N_("_Refresh"), "<control>R",
      N_("Refresh the process list"), G_CALLBACK(cb_user_refresh) },

    { "MemoryMaps", NULL, N_("_Memory Maps"), "<control>M",
      N_("Open the memory maps associated with a process"), G_CALLBACK (cb_show_memory_maps) },
    // Translators: this means 'Files that are open' (open is no verb here)
    { "OpenFiles", NULL, N_("Open _Files"), "<control>F",
      N_("View the files opened by a process"), G_CALLBACK (cb_show_open_files) },
    { "ProcessProperties", NULL, N_("_Properties"), NULL,
      N_("View additional information about a process"), G_CALLBACK (cb_show_process_properties) },


    { "HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1",
      N_("Open the manual"), G_CALLBACK (cb_help_contents) },
    { "About", GTK_STOCK_ABOUT, NULL, NULL,
      N_("About this application"), G_CALLBACK (cb_about) }
};

static const GtkToggleActionEntry toggle_menu_entries[] =
{
    { "ShowDependencies", NULL, N_("_Dependencies"), "<control>D",
      N_("Show parent/child relationship between processes"),
      G_CALLBACK (cb_toggle_tree), TRUE },
};


static const GtkRadioActionEntry radio_menu_entries[] =
{
    { "ShowActiveProcesses", NULL, N_("_Active Processes"), NULL,
      N_("Show active processes"), ACTIVE_PROCESSES },
    { "ShowAllProcesses", NULL, N_("A_ll Processes"), NULL,
      N_("Show all processes"), ALL_PROCESSES },
    { "ShowMyProcesses", NULL, N_("M_y Processes"), NULL,
      N_("Show only user-owned processes"), MY_PROCESSES }
};

static const GtkRadioActionEntry priority_menu_entries[] =
{
    { "VeryHigh", NULL, N_("Very High"), NULL,
      N_("Set process priority to very high"), VERY_HIGH_PRIORITY },
    { "High", NULL, N_("High"), NULL,
      N_("Set process priority to high"), HIGH_PRIORITY },
    { "Normal", NULL, N_("Normal"), NULL,
      N_("Set process priority to normal"), NORMAL_PRIORITY },
    { "Low", NULL, N_("Low"), NULL,
      N_("Set process priority to low"), LOW_PRIORITY },
    { "VeryLow", NULL, N_("Very Low"), NULL,
      N_("Set process priority to very low"), VERY_LOW_PRIORITY },
    { "Custom", NULL, N_("Custom"), NULL,
      N_("Set process priority manually"), CUSTOM_PRIORITY }
};


static const char ui_info[] =
    "  <menubar name=\"MenuBar\">"
    "    <menu name=\"MonitorMenu\" action=\"Monitor\">"
    "      <menuitem name=\"MonitorLsofMenu\" action=\"Lsof\" />"
    "      <menuitem name=\"MonitorQuitMenu\" action=\"Quit\" />"
    "    </menu>"
    "    <menu name=\"EditMenu\" action=\"Edit\">"
    "      <menuitem name=\"EditStopProcessMenu\" action=\"StopProcess\" />"
    "      <menuitem name=\"EditContProcessMenu\" action=\"ContProcess\" />"
    "      <separator />"
    "      <menuitem name=\"EditEndProcessMenu\" action=\"EndProcess\" />"
    "      <menuitem name=\"EditKillProcessMenu\" action=\"KillProcess\" />"
    "      <separator />"
    "      <menu name=\"EditChangePriorityMenu\" action=\"ChangePriority\" >"
    "        <menuitem action=\"VeryHigh\" />"
    "        <menuitem action=\"High\" />"
    "        <menuitem action=\"Normal\" />"
    "        <menuitem action=\"Low\" />"
    "        <menuitem action=\"VeryLow\" />"
    "        <separator />"
    "        <menuitem action=\"Custom\"/>"
    "      </menu>"
    "      <separator />"
    "      <menuitem name=\"EditPreferencesMenu\" action=\"Preferences\" />"
    "    </menu>"
    "    <menu name=\"ViewMenu\" action=\"View\">"
    "      <menuitem name=\"ViewActiveProcesses\" action=\"ShowActiveProcesses\" />"
    "      <menuitem name=\"ViewAllProcesses\" action=\"ShowAllProcesses\" />"
    "      <menuitem name=\"ViewMyProcesses\" action=\"ShowMyProcesses\" />"
    "      <separator />"
    "      <menuitem name=\"ViewDependenciesMenu\" action=\"ShowDependencies\" />"
    "      <separator />"
    "      <menuitem name=\"ViewMemoryMapsMenu\" action=\"MemoryMaps\" />"
    "      <menuitem name=\"ViewOpenFilesMenu\" action=\"OpenFiles\" />"
    "      <separator />"
    "      <menuitem name=\"ViewProcessPropertiesMenu\" action=\"ProcessProperties\" />"
    "      <separator />"
    "      <menuitem name=\"ViewRefresh\" action=\"Refresh\" />"
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
    "    <menu name=\"ChangePriorityMenu\" action=\"ChangePriority\" >"
    "      <menuitem action=\"VeryHigh\" />"
    "      <menuitem action=\"High\" />"
    "      <menuitem action=\"Normal\" />"
    "      <menuitem action=\"Low\" />"
    "      <menuitem action=\"VeryLow\" />"
    "      <separator />"
    "      <menuitem action=\"Custom\"/>"
    "    </menu>"
    "    <separator />"
    "    <menuitem action=\"MemoryMaps\" />"
    "    <menuitem action=\"OpenFiles\" />"
    "    <separator />"
    "    <menuitem action=\"ProcessProperties\" />"

    "  </popup>";


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

static void 
create_proc_view(ProcData *procdata, GtkBuilder * builder) 
{
    GtkWidget *proctree;
    GtkWidget *scrolled;
    char* string;

    /* create the processes tab */
    string = make_loadavg_string ();
    procdata->loadavg = GTK_WIDGET (gtk_builder_get_object (builder, "load_avg_label"));
    gtk_label_set_text (GTK_LABEL (procdata->loadavg), string);
    g_free (string);

    proctree = proctable_new (procdata);
    scrolled = GTK_WIDGET (gtk_builder_get_object (builder, "processes_scrolled"));

    gtk_container_add (GTK_CONTAINER (scrolled), proctree);

    procdata->endprocessbutton = GTK_WIDGET (gtk_builder_get_object (builder, "endprocessbutton"));
    g_signal_connect (G_OBJECT (procdata->endprocessbutton), "clicked",
                      G_CALLBACK (cb_end_process_button_pressed), procdata);

    /* create popup_menu for the processes tab */
    procdata->popup_menu = gtk_ui_manager_get_widget (procdata->uimanager, "/PopupMenu");
}

static void
create_sys_view (ProcData *procdata, GtkBuilder * builder)
{
    GtkWidget *cpu_graph_box, *mem_graph_box, *net_graph_box;
    GtkWidget *label,*cpu_label;
    GtkWidget *table;
    GtkWidget *color_picker;
    GtkWidget *picker_alignment;
    LoadGraph *cpu_graph, *mem_graph, *net_graph;

    gint i;
    gchar *title_text;
    gchar *label_text;
    gchar *title_template;

    // Translators: color picker title, %s is CPU, Memory, Swap, Receiving, Sending
    title_template = g_strdup(_("Pick a Color for '%s'"));

    /* The CPU BOX */

    cpu_graph_box = GTK_WIDGET (gtk_builder_get_object (builder, "cpu_graph_box"));

    cpu_graph = new LoadGraph(LOAD_GRAPH_CPU);
    gtk_box_pack_start (GTK_BOX (cpu_graph_box),
                        load_graph_get_widget(cpu_graph),
                        TRUE,
                        TRUE,
                        0);

    GtkWidget* cpu_table = GTK_WIDGET (gtk_builder_get_object (builder, "cpu_table"));
    gint cols = 4;
    for (i=0;i<procdata->config.num_cpus; i++) {
        GtkWidget *temp_hbox;

        temp_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        if (i < cols) {
            gtk_grid_insert_column(GTK_GRID(cpu_table), i%cols);
        }
        if ((i+1)%cols ==cols) {
            gtk_grid_insert_row(GTK_GRID(cpu_table), (i+1)/cols);
        }
        gtk_grid_attach(GTK_GRID (cpu_table), temp_hbox, i%cols, i/cols, 1, 1);
        //gtk_size_group_add_widget (sizegroup, temp_hbox);
        /*g_signal_connect (G_OBJECT (temp_hbox), "size_request",
          G_CALLBACK(size_request), &cpu_size);
        */
        color_picker = gsm_color_button_new (&cpu_graph->colors.at(i), GSMCP_TYPE_CPU);
        g_signal_connect (G_OBJECT (color_picker), "color_set",
                          G_CALLBACK (cb_cpu_color_changed), GINT_TO_POINTER (i));
        gtk_box_pack_start (GTK_BOX (temp_hbox), color_picker, FALSE, TRUE, 0);
        gtk_widget_set_size_request(GTK_WIDGET(color_picker), 32, -1);
        if(procdata->config.num_cpus == 1) {
            label_text = g_strdup (_("CPU"));
        } else {
            label_text = g_strdup_printf (_("CPU%d"), i+1);
        }
        title_text = g_strdup_printf(title_template, label_text);
        label = gtk_label_new (label_text);
        gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
        g_free(title_text);
        gtk_box_pack_start (GTK_BOX (temp_hbox), label, FALSE, FALSE, 6);
        g_free (label_text);

        cpu_label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (cpu_label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (temp_hbox), cpu_label, FALSE, FALSE, 0);
        load_graph_get_labels(cpu_graph)->cpu[i] = cpu_label;

    }

    procdata->cpu_graph = cpu_graph;

    /** The memory box */
    mem_graph_box = GTK_WIDGET (gtk_builder_get_object (builder, "mem_graph_box"));

    mem_graph = new LoadGraph(LOAD_GRAPH_MEM);
    gtk_box_pack_start (GTK_BOX (mem_graph_box),
                        load_graph_get_widget(mem_graph),
                        TRUE,
                        TRUE,
                        0);

    table = GTK_WIDGET (gtk_builder_get_object (builder, "mem_table"));

    color_picker = load_graph_get_mem_color_picker(mem_graph);
    g_signal_connect (G_OBJECT (color_picker), "color_set",
                      G_CALLBACK (cb_mem_color_changed), procdata);
    title_text = g_strdup_printf(title_template, _("Memory"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);
    
    gtk_grid_attach (GTK_GRID (table), color_picker, 0, 0, 1, 2);

    gtk_grid_attach (GTK_GRID (table), load_graph_get_labels(mem_graph)->memory, 1, 1, 1, 1);

    color_picker = load_graph_get_swap_color_picker(mem_graph);
    g_signal_connect (G_OBJECT (color_picker), "color_set",
                      G_CALLBACK (cb_swap_color_changed), procdata);
    title_text = g_strdup_printf(title_template, _("Swap"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);
    gtk_grid_attach (GTK_GRID (table), color_picker, 2, 0, 1, 2);

    gtk_grid_attach (GTK_GRID (table),  load_graph_get_labels(mem_graph)->swap, 3, 1, 1, 1);

    procdata->mem_graph = mem_graph;

    /* The net box */
    net_graph_box = GTK_WIDGET (gtk_builder_get_object (builder, "net_graph_box"));

    net_graph = new LoadGraph(LOAD_GRAPH_NET);
    gtk_box_pack_start (GTK_BOX (net_graph_box),
                        load_graph_get_widget(net_graph),
                        TRUE,
                        TRUE,
                        0);

    table = GTK_WIDGET (gtk_builder_get_object (builder, "net_table"));

    color_picker = gsm_color_button_new (
        &net_graph->colors.at(0), GSMCP_TYPE_NETWORK_IN);
    g_signal_connect (G_OBJECT (color_picker), "color_set",
                      G_CALLBACK (cb_net_in_color_changed), procdata);
    title_text = g_strdup_printf(title_template, _("Receiving"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);
    picker_alignment = GTK_WIDGET (gtk_builder_get_object (builder, "receiving_picker_alignment"));
    gtk_container_add (GTK_CONTAINER (picker_alignment), color_picker);

    //gtk_widget_set_size_request(GTK_WIDGET(load_graph_get_labels(net_graph)->net_in), 100, -1);
    gtk_grid_attach (GTK_GRID (table), load_graph_get_labels(net_graph)->net_in, 2, 0, 1, 1);

    gtk_grid_attach (GTK_GRID (table), load_graph_get_labels(net_graph)->net_in_total, 2, 1, 1, 1);

    color_picker = gsm_color_button_new (
        &net_graph->colors.at(1), GSMCP_TYPE_NETWORK_OUT);
    g_signal_connect (G_OBJECT (color_picker), "color_set",
                      G_CALLBACK (cb_net_out_color_changed), procdata);
    title_text = g_strdup_printf(title_template, _("Sending"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);

    picker_alignment = GTK_WIDGET (gtk_builder_get_object (builder, "sending_picker_alignment"));
    gtk_container_add (GTK_CONTAINER (picker_alignment), color_picker);

    //gtk_widget_set_size_request(GTK_WIDGET(load_graph_get_labels(net_graph)->net_out), 100, -1);
    
    gtk_grid_attach (GTK_GRID (table), load_graph_get_labels(net_graph)->net_out, 6, 0, 1, 1);

    gtk_grid_attach (GTK_GRID (table),  load_graph_get_labels(net_graph)->net_out_total, 6, 1, 1, 1);

    procdata->net_graph = net_graph;
    g_free(title_template);

}

void
create_main_window (ProcData *procdata)
{
    gint i;
    gint width, height, xpos, ypos;
    GtkWidget *app;
    GtkAction *action;
    GtkWidget *menubar;
    GtkWidget *main_box;
    GtkWidget *notebook;

    gchar* filename = g_build_filename (GSM_DATA_DIR, "interface.ui", NULL);

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, filename, NULL);

    app = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
    main_box = GTK_WIDGET (gtk_builder_get_object (builder, "main_box"));
    GdkScreen* screen = gtk_widget_get_screen(app);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);

    /* use visual, if available */
    if (visual)
        gtk_widget_set_visual(app, visual);

    width = procdata->config.width;
    height = procdata->config.height;
    xpos = procdata->config.xpos;
    ypos = procdata->config.ypos;
    gtk_window_set_default_size (GTK_WINDOW (app), width, height);
    gtk_window_move(GTK_WINDOW (app), xpos, ypos);

    /* create the menubar */
    procdata->uimanager = gtk_ui_manager_new ();

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

    gtk_action_group_add_radio_actions (procdata->action_group,
                                        priority_menu_entries,
                                        G_N_ELEMENTS (priority_menu_entries),
                                        NORMAL_PRIORITY,
                                        G_CALLBACK(cb_renice),
                                        procdata);

    gtk_ui_manager_insert_action_group (procdata->uimanager,
                                        procdata->action_group,
                                        0);

    menubar = gtk_ui_manager_get_widget (procdata->uimanager, "/MenuBar");
    gtk_box_pack_start (GTK_BOX (main_box), menubar, FALSE, FALSE, 0);


    /* create the main notebook */
    procdata->notebook = notebook = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));

    create_proc_view(procdata, builder);

    create_sys_view (procdata, builder);
    
    create_disk_view (procdata, builder);
    
    g_signal_connect (G_OBJECT (notebook), "switch-page",
                      G_CALLBACK (cb_switch_page), procdata);
    g_signal_connect (G_OBJECT (notebook), "change-current-page",
                      G_CALLBACK (cb_change_current_page), procdata);

    gtk_widget_show_all(notebook); // need to make page switch work
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), procdata->config.current_tab);
    cb_change_current_page (GTK_NOTEBOOK (notebook), procdata->config.current_tab, procdata);
    g_signal_connect (G_OBJECT (app), "delete_event",
                      G_CALLBACK (cb_app_delete),
                      procdata);

    GtkAccelGroup *accel_group;
    GClosure *goto_tab_closure[4];
    accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW(app), accel_group);
    for (i = 0; i < 4; ++i) {
        goto_tab_closure[i] = g_cclosure_new_swap (G_CALLBACK (cb_proc_goto_tab),
                                                   GINT_TO_POINTER (i), NULL);
        gtk_accel_group_connect (accel_group, '0'+(i+1),
                                 GDK_MOD1_MASK, GTK_ACCEL_VISIBLE,
                                 goto_tab_closure[i]);
    }
    action = gtk_action_group_get_action (procdata->action_group, "ShowDependencies");
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                  procdata->config.show_tree);

    
    gtk_builder_connect_signals (builder, NULL);

    gtk_widget_show_all(app);
    procdata->app = app;

    g_object_unref (G_OBJECT (builder));
    g_free (filename);

}

void
do_popup_menu (ProcData *procdata, GdkEventButton *event)
{
    guint button;
    guint32 event_time;

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
update_sensitivity(ProcData *data)
{
    const char * const selected_actions[] = { "StopProcess",
                                              "ContProcess",
                                              "EndProcess",
                                              "KillProcess",
                                              "ChangePriority",
                                              "MemoryMaps",
                                              "OpenFiles",
                                              "ProcessProperties" };

    const char * const processes_actions[] = { "ShowActiveProcesses",
                                               "ShowAllProcesses",
                                               "ShowMyProcesses",
                                               "ShowDependencies",
                                               "Refresh"
    };

    size_t i;
    gboolean processes_sensitivity, selected_sensitivity;
    GtkAction *action;

    processes_sensitivity = (data->config.current_tab == PROCMAN_TAB_PROCESSES);
    selected_sensitivity = (processes_sensitivity && data->selected_process != NULL);

    if(data->endprocessbutton) {
        /* avoid error on startup if endprocessbutton
           has not been built yet */
        gtk_widget_set_sensitive(data->endprocessbutton, selected_sensitivity);
    }

    for (i = 0; i != G_N_ELEMENTS(processes_actions); ++i) {
        action = gtk_action_group_get_action(data->action_group,
                                             processes_actions[i]);
        gtk_action_set_sensitive(action, processes_sensitivity);
    }

    for (i = 0; i != G_N_ELEMENTS(selected_actions); ++i) {
        action = gtk_action_group_get_action(data->action_group,
                                             selected_actions[i]);
        gtk_action_set_sensitive(action, selected_sensitivity);
    }
}

void
block_priority_changed_handlers(ProcData *data, bool block)
{
    gint i;
    if (block) {
        for (i = 0; i != G_N_ELEMENTS(priority_menu_entries); ++i) {
            GtkRadioAction *action = GTK_RADIO_ACTION(gtk_action_group_get_action(data->action_group,
                                             priority_menu_entries[i].name));
            g_signal_handlers_block_by_func(action, (gpointer)cb_renice, data);
        }
    } else {
        for (i = 0; i != G_N_ELEMENTS(priority_menu_entries); ++i) {
            GtkRadioAction *action = GTK_RADIO_ACTION(gtk_action_group_get_action(data->action_group,
                                             priority_menu_entries[i].name));
            g_signal_handlers_unblock_by_func(action, (gpointer)cb_renice, data);
        }
    }
}

static void
cb_toggle_tree (GtkAction *action, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);
    GSettings *settings = procdata->settings;       gboolean show;

    show = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
    if (show == procdata->config.show_tree)
        return;

    g_settings_set_boolean (settings, "show-tree", show);
}

static void
cb_proc_goto_tab (gint tab)
{
    ProcData *data = ProcData::get_instance ();
    gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook), tab);
}
