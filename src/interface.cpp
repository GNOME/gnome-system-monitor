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

#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "procactions.h"
#include "load-graph.h"
#include "util.h"
#include "disks.h"
#include "gsm_color_button.h"

static void     cb_toggle_tree (GtkAction *action, gpointer data);
static void     cb_proc_goto_tab (gint tab);

static const GtkActionEntry menu_entries[] =
{
    { "View", NULL, N_("_View") },

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

    { "Refresh", NULL, N_("_Refresh"), "<control>R",
      N_("Refresh the process list"), G_CALLBACK(cb_user_refresh) },

    { "MemoryMaps", NULL, N_("_Memory Maps"), "<control>M",
      N_("Open the memory maps associated with a process"), G_CALLBACK (cb_show_memory_maps) },
    // Translators: this means 'Files that are open' (open is no verb here)
    { "OpenFiles", NULL, N_("Open _Files"), "<control>F",
      N_("View the files opened by a process"), G_CALLBACK (cb_show_open_files) },
    { "ProcessProperties", NULL, N_("_Properties"), NULL,
      N_("View additional information about a process"), G_CALLBACK (cb_show_process_properties) },
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
create_proc_view(ProcmanApp *app, GtkBuilder * builder)
{
    GtkWidget *proctree;
    GtkWidget *scrolled;
    GtkWidget *viewmenu;
    GtkWidget *button;
    GtkAction *action;
    char* string;

    /* create the processes tab */
    string = make_loadavg_string ();
    app->loadavg = GTK_WIDGET (gtk_builder_get_object (builder, "load_avg_label"));
    gtk_label_set_text (GTK_LABEL (app->loadavg), string);
    g_free (string);

    proctree = proctable_new (app);
    scrolled = GTK_WIDGET (gtk_builder_get_object (builder, "processes_scrolled"));

    gtk_container_add (GTK_CONTAINER (scrolled), proctree);

    app->endprocessbutton = GTK_WIDGET (gtk_builder_get_object (builder, "endprocessbutton"));
    g_signal_connect (G_OBJECT (app->endprocessbutton), "clicked",
                      G_CALLBACK (cb_end_process_button_pressed), app);

    button = GTK_WIDGET (gtk_builder_get_object (builder, "viewmenubutton"));
    viewmenu = gtk_ui_manager_get_widget (app->uimanager, "/ViewMenu");
    gtk_widget_set_halign (viewmenu, GTK_ALIGN_END);
    gtk_menu_button_set_popup (GTK_MENU_BUTTON (button), viewmenu);

    button = GTK_WIDGET (gtk_builder_get_object (builder, "refreshbutton"));
    action = gtk_action_group_get_action (app->action_group, "Refresh");
    gtk_activatable_set_related_action (GTK_ACTIVATABLE (button), action);

    /* create popup_menu for the processes tab */
    app->popup_menu = gtk_ui_manager_get_widget (app->uimanager, "/PopupMenu");
}

static void
create_sys_view (ProcmanApp *app, GtkBuilder * builder)
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
    for (i=0;i<app->config.num_cpus; i++) {
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
        if(app->config.num_cpus == 1) {
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

    app->cpu_graph = cpu_graph;

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
                      G_CALLBACK (cb_mem_color_changed), app);
    title_text = g_strdup_printf(title_template, _("Memory"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);

    label = GTK_WIDGET(gtk_builder_get_object(builder, "memory_label"));

    gtk_grid_attach_next_to (GTK_GRID (table), color_picker, label, GTK_POS_LEFT, 1, 2);
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(mem_graph)->memory, label, GTK_POS_BOTTOM, 1, 1);

    color_picker = load_graph_get_swap_color_picker(mem_graph);
    g_signal_connect (G_OBJECT (color_picker), "color_set",
                      G_CALLBACK (cb_swap_color_changed), app);
    title_text = g_strdup_printf(title_template, _("Swap"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);

    label = GTK_WIDGET(gtk_builder_get_object(builder, "swap_label"));

    gtk_grid_attach_next_to (GTK_GRID (table), color_picker, label, GTK_POS_LEFT, 1, 2);
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(mem_graph)->swap, label, GTK_POS_BOTTOM, 1, 1);

    app->mem_graph = mem_graph;

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
                      G_CALLBACK (cb_net_in_color_changed), app);
    title_text = g_strdup_printf(title_template, _("Receiving"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);
    picker_alignment = GTK_WIDGET (gtk_builder_get_object (builder, "receiving_picker_alignment"));
    gtk_container_add (GTK_CONTAINER (picker_alignment), color_picker);

    //gtk_widget_set_size_request(GTK_WIDGET(load_graph_get_labels(net_graph)->net_in), 100, -1);
    label = GTK_WIDGET (gtk_builder_get_object(builder, "receiving_label"));
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(net_graph)->net_in, label, GTK_POS_RIGHT, 1, 1);
    label = GTK_WIDGET (gtk_builder_get_object(builder, "total_received_label"));
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(net_graph)->net_in_total, label, GTK_POS_RIGHT, 1, 1);

    color_picker = gsm_color_button_new (
        &net_graph->colors.at(1), GSMCP_TYPE_NETWORK_OUT);
    g_signal_connect (G_OBJECT (color_picker), "color_set",
                      G_CALLBACK (cb_net_out_color_changed), app);
    title_text = g_strdup_printf(title_template, _("Sending"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);

    picker_alignment = GTK_WIDGET (gtk_builder_get_object (builder, "sending_picker_alignment"));
    gtk_container_add (GTK_CONTAINER (picker_alignment), color_picker);

    //gtk_widget_set_size_request(GTK_WIDGET(load_graph_get_labels(net_graph)->net_out), 100, -1);
    label = GTK_WIDGET (gtk_builder_get_object(builder, "sending_label"));
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(net_graph)->net_out, label, GTK_POS_RIGHT, 1, 1);
    label = GTK_WIDGET (gtk_builder_get_object(builder, "total_sent_label"));
    gtk_grid_attach_next_to (GTK_GRID (table),  load_graph_get_labels(net_graph)->net_out_total, label, GTK_POS_RIGHT, 1, 1);

    app->net_graph = net_graph;
    g_free(title_template);

}

static void
on_activate_about (GSimpleAction *, GVariant *, gpointer data)
{
    cb_about (NULL, data);
}

void
create_main_window (ProcmanApp *app)
{
    gint i;
    gint width, height, xpos, ypos;
    GtkWidget *main_window;
    GtkAction *action;
    GtkWidget *notebook;

    gchar* filename = g_build_filename (GSM_DATA_DIR, "interface.ui", NULL);

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, filename, NULL);
    g_free (filename);

    main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
    gtk_window_set_application (GTK_WINDOW (main_window), app->gobj());
    gtk_widget_set_name (main_window, "gnome-system-monitor");

    GActionEntry win_action_entries[] = {
        { "about", on_activate_about, NULL, NULL, NULL }
    };

    g_action_map_add_action_entries (G_ACTION_MAP (main_window),
                                     win_action_entries,
                                     G_N_ELEMENTS (win_action_entries),
                                     app);

    GdkScreen* screen = gtk_widget_get_screen(main_window);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);

    /* use visual, if available */
    if (visual)
        gtk_widget_set_visual(main_window, visual);

    width = app->config.width;
    height = app->config.height;
    xpos = app->config.xpos;
    ypos = app->config.ypos;
    gtk_window_set_default_size (GTK_WINDOW (main_window), width, height);
    gtk_window_move(GTK_WINDOW (main_window), xpos, ypos);
    if (app->config.maximized) {
        gtk_window_maximize(GTK_WINDOW(main_window));
    }

    app->uimanager = gtk_ui_manager_new ();

    gtk_window_add_accel_group (GTK_WINDOW (main_window),
                                gtk_ui_manager_get_accel_group (app->uimanager));

    filename = g_build_filename (GSM_DATA_DIR, "popups.ui", NULL);

    if (!gtk_ui_manager_add_ui_from_file (app->uimanager,
                                            filename,
                                            NULL)) {
        g_error("building menus failed");
    }
    g_free (filename);
    app->action_group = gtk_action_group_new ("ProcmanActions");
    gtk_action_group_set_translation_domain (app->action_group, NULL);
    gtk_action_group_add_actions (app->action_group,
                                  menu_entries,
                                  G_N_ELEMENTS (menu_entries),
                                  app);
    gtk_action_group_add_toggle_actions (app->action_group,
                                         toggle_menu_entries,
                                         G_N_ELEMENTS (toggle_menu_entries),
                                         app);

    gtk_action_group_add_radio_actions (app->action_group,
                                        radio_menu_entries,
                                        G_N_ELEMENTS (radio_menu_entries),
                                        app->config.whose_process,
                                        G_CALLBACK(cb_radio_processes),
                                        app);

    gtk_action_group_add_radio_actions (app->action_group,
                                        priority_menu_entries,
                                        G_N_ELEMENTS (priority_menu_entries),
                                        NORMAL_PRIORITY,
                                        G_CALLBACK(cb_renice),
                                        app);

    gtk_ui_manager_insert_action_group (app->uimanager,
                                        app->action_group,
                                        0);

    /* create the main notebook */
    app->notebook = notebook = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));


    create_proc_view(app, builder);

    create_sys_view (app, builder);
    
    create_disk_view (app, builder);


    g_signal_connect (G_OBJECT (notebook), "switch-page",
                      G_CALLBACK (cb_switch_page), app);
    g_signal_connect (G_OBJECT (notebook), "change-current-page",
                      G_CALLBACK (cb_change_current_page), app);

    gtk_widget_show_all(notebook); // need to make page switch work
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), app->config.current_tab);
    cb_change_current_page (GTK_NOTEBOOK (notebook), app->config.current_tab, app);
    g_signal_connect (G_OBJECT (main_window), "delete_event",
                      G_CALLBACK (cb_main_window_delete),
                      app);

    GtkAccelGroup *accel_group;
    GClosure *goto_tab_closure[4];
    accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW(main_window), accel_group);
    for (i = 0; i < 4; ++i) {
        goto_tab_closure[i] = g_cclosure_new_swap (G_CALLBACK (cb_proc_goto_tab),
                                                   GINT_TO_POINTER (i), NULL);
        gtk_accel_group_connect (accel_group, '0'+(i+1),
                                 GDK_MOD1_MASK, GTK_ACCEL_VISIBLE,
                                 goto_tab_closure[i]);
    }
    action = gtk_action_group_get_action (app->action_group, "ShowDependencies");
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                  app->config.show_tree);

    
    gtk_builder_connect_signals (builder, NULL);

    gtk_widget_show_all(main_window);
    app->main_window = main_window;

    g_object_unref (G_OBJECT (builder));
}

void
do_popup_menu (ProcmanApp *app, GdkEventButton *event)
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

    gtk_menu_popup (GTK_MENU (app->popup_menu), NULL, NULL,
                    NULL, NULL, button, event_time);
}

void
update_sensitivity(ProcmanApp *app)
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

    processes_sensitivity = (app->config.current_tab == PROCMAN_TAB_PROCESSES);
    selected_sensitivity = (processes_sensitivity && app->selected_process != NULL);

    if(app->endprocessbutton) {
        /* avoid error on startup if endprocessbutton
           has not been built yet */
        gtk_widget_set_sensitive(app->endprocessbutton, selected_sensitivity);
    }

    for (i = 0; i != G_N_ELEMENTS(processes_actions); ++i) {
        action = gtk_action_group_get_action(app->action_group,
                                             processes_actions[i]);
        gtk_action_set_sensitive(action, processes_sensitivity);
    }

    for (i = 0; i != G_N_ELEMENTS(selected_actions); ++i) {
        action = gtk_action_group_get_action(app->action_group,
                                             selected_actions[i]);
        gtk_action_set_sensitive(action, selected_sensitivity);
    }
}

void
block_priority_changed_handlers(ProcmanApp *app, bool block)
{
    gint i;
    if (block) {
        for (i = 0; i != G_N_ELEMENTS(priority_menu_entries); ++i) {
            GtkRadioAction *action = GTK_RADIO_ACTION(gtk_action_group_get_action(app->action_group,
                                             priority_menu_entries[i].name));
            g_signal_handlers_block_by_func(action, (gpointer)cb_renice, app);
        }
    } else {
        for (i = 0; i != G_N_ELEMENTS(priority_menu_entries); ++i) {
            GtkRadioAction *action = GTK_RADIO_ACTION(gtk_action_group_get_action(app->action_group,
                                             priority_menu_entries[i].name));
            g_signal_handlers_unblock_by_func(action, (gpointer)cb_renice, app);
        }
    }
}

static void
cb_toggle_tree (GtkAction *action, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);
    GSettings *settings = app->settings;
    gboolean show;

    show = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
    if (show == app->config.show_tree)
        return;

    g_settings_set_boolean (settings, "show-tree", show);
}

static void
cb_proc_goto_tab (gint tab)
{
    Glib::RefPtr<ProcmanApp> app = ProcmanApp::get();
    gtk_notebook_set_current_page (GTK_NOTEBOOK (app->notebook), tab);
}
