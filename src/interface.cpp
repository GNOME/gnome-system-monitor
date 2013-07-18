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

static const GtkActionEntry menu_entries[] =
{
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

    { "MemoryMaps", NULL, N_("_Memory Maps"), "<control>M",
      N_("Open the memory maps associated with a process"), G_CALLBACK (cb_show_memory_maps) },
    // Translators: this means 'Files that are open' (open is no verb here)
    { "OpenFiles", NULL, N_("Open _Files"), "<control>F",
      N_("View the files opened by a process"), G_CALLBACK (cb_show_open_files) },
    { "ProcessProperties", NULL, N_("_Properties"), NULL,
      N_("View additional information about a process"), G_CALLBACK (cb_show_process_properties) },
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

static void 
create_proc_view(ProcmanApp *app, GtkBuilder * builder)
{
    GtkWidget *proctree;
    GtkWidget *scrolled;
    GtkWidget *viewmenu;
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

static void
on_activate_refresh (GSimpleAction *, GVariant *, gpointer data)
{
    ProcmanApp *app = (ProcmanApp *) data;
    proctable_update_all (app);
}


static void
on_activate_radio (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    g_action_change_state (G_ACTION (action), parameter);
}

static void
on_activate_toggle (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    GVariant *state = g_action_get_state (G_ACTION (action));
    g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
    g_variant_unref (state);
}

static void
change_show_page_state (GSimpleAction *action, GVariant *state, gpointer data)
{
    ProcmanApp *app = (ProcmanApp *) data;

    g_simple_action_set_state (action, state);
    g_settings_set_value (app->settings, "current-tab", state);
}

static void
change_show_processes_state (GSimpleAction *action, GVariant *state, gpointer data)
{
    ProcmanApp *app = (ProcmanApp *) data;

    g_simple_action_set_state (action, state);
    g_settings_set_value (app->settings, "show-whose-processes", state);
}

static void
change_show_dependencies_state (GSimpleAction *action, GVariant *state, gpointer data)
{
    ProcmanApp *app = (ProcmanApp *) data;

    g_simple_action_set_state (action, state);
    g_settings_set_value (app->settings, "show-dependencies", state);
}

void
update_page_activities (ProcmanApp *app)
{
    int current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (app->notebook));

    if (current_page == PROCMAN_TAB_PROCESSES) {
        cb_timeout (app);

        if (!app->timeout) {
            app->timeout = g_timeout_add (app->config.update_interval,
                                          cb_timeout, app);
        }

        update_sensitivity (app);

        gtk_widget_grab_focus (app->tree);
    } else {
        if (app->timeout) {
            g_source_remove (app->timeout);
            app->timeout = 0;
        }

        update_sensitivity (app);
    }

    if (current_page == PROCMAN_TAB_RESOURCES) {
        load_graph_start (app->cpu_graph);
        load_graph_start (app->mem_graph);
        load_graph_start (app->net_graph);
    } else {
        load_graph_stop (app->cpu_graph);
        load_graph_stop (app->mem_graph);
        load_graph_stop (app->net_graph);
    }

    if (current_page == PROCMAN_TAB_DISKS) {
        cb_update_disks (app);

        if (!app->disk_timeout) {
            app->disk_timeout = g_timeout_add (app->config.disks_update_interval,
                                               cb_update_disks,
                                               app);
        }
    } else {
        if (app->disk_timeout) {
            g_source_remove (app->disk_timeout);
            app->disk_timeout = 0;
        }
    }
}

static void
cb_change_current_page (GtkNotebook *notebook, GParamSpec *pspec, gpointer data)
{
    update_page_activities ((ProcmanApp *)data);
}

void
create_main_window (ProcmanApp *app)
{
    gint i;
    gint width, height, xpos, ypos;
    GtkWidget *main_window;
    GtkWidget *notebook;

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/interface.ui", NULL);

    main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
    gtk_window_set_application (GTK_WINDOW (main_window), app->gobj());
    gtk_widget_set_name (main_window, "gnome-system-monitor");
    app->main_window = main_window;

    GActionEntry win_action_entries[] = {
        { "about", on_activate_about, NULL, NULL, NULL },
        { "refresh", on_activate_refresh, NULL, NULL, NULL },
        { "show-page", on_activate_radio, "i", "0", change_show_page_state },
        { "show-whose-processes", on_activate_radio, "s", "'all'", change_show_processes_state },
        { "show-dependencies", on_activate_toggle, NULL, "false", change_show_dependencies_state }
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

    if (!gtk_ui_manager_add_ui_from_resource (app->uimanager,
                                              "/org/gnome/gnome-system-monitor/data/popups.ui",
                                              NULL)) {
        g_error("building menus failed");
    }

    app->action_group = gtk_action_group_new ("ProcmanActions");
    gtk_action_group_set_translation_domain (app->action_group, NULL);
    gtk_action_group_add_actions (app->action_group,
                                  menu_entries,
                                  G_N_ELEMENTS (menu_entries),
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

    g_settings_bind (app->settings, "current-tab", notebook, "page", G_SETTINGS_BIND_DEFAULT);

    g_signal_connect (G_OBJECT (notebook), "notify::page",
                      G_CALLBACK (cb_change_current_page), app);
    update_page_activities (app);

    g_signal_connect (G_OBJECT (main_window), "delete_event",
                      G_CALLBACK (cb_main_window_delete),
                      app);

    GAction *action;
    action = g_action_map_lookup_action (G_ACTION_MAP (main_window),
                                         "show-dependencies");
    g_action_change_state (action,
                           g_settings_get_value (app->settings, "show-dependencies"));


    action = g_action_map_lookup_action (G_ACTION_MAP (main_window),
                                         "show-whose-processes");
    g_action_change_state (action,
                           g_settings_get_value (app->settings, "show-whose-processes"));

    gtk_builder_connect_signals (builder, NULL);

    gtk_widget_show_all(main_window);

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

    const char * const processes_actions[] = { "refresh",
                                               "show-whose-processes",
                                               "show-dependencies" };

    size_t i;
    gboolean processes_sensitivity, selected_sensitivity;
    GtkAction *action;
    GAction *gaction;

    processes_sensitivity = (g_settings_get_int (app->settings, "current-tab") == PROCMAN_TAB_PROCESSES);
    selected_sensitivity = (processes_sensitivity && app->selected_process != NULL);

    if(app->endprocessbutton) {
        /* avoid error on startup if endprocessbutton
           has not been built yet */
        gtk_widget_set_sensitive(app->endprocessbutton, selected_sensitivity);
    }

    for (i = 0; i != G_N_ELEMENTS(processes_actions); ++i) {
        gaction = g_action_map_lookup_action (G_ACTION_MAP (app->main_window),
                                              processes_actions[i]);
        g_simple_action_set_enabled (G_SIMPLE_ACTION (gaction), processes_sensitivity);
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
