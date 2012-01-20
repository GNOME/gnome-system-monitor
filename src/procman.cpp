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

#include <locale.h>

#include <gtkmm/main.h>
#include <giomm/volumemonitor.h>
#include <giomm/init.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <bacon-message-connection.h>
#include <glibtop.h>
#include <glibtop/close.h>
#include <glibtop/loadavg.h>

#include "load-graph.h"
#include "procman.h"
#include "interface.h"
#include "proctable.h"
#include "prettytable.h"
#include "callbacks.h"
#include "smooth_refresh.h"
#include "util.h"
#include "settings-keys.h"
#include "argv.h"


ProcData::ProcData()
    : tree(NULL),
      cpu_graph(NULL),
      mem_graph(NULL),
      net_graph(NULL),
      selected_process(NULL),
      timeout(0),
      disk_timeout(0),
      cpu_total_time(1),
      cpu_total_time_last(1)
{ }


ProcData* ProcData::get_instance()
{
    static ProcData instance;
    return &instance;
}


static void
tree_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);

    procdata->config.show_tree = g_settings_get_boolean(settings, key);

    g_object_set(G_OBJECT(procdata->tree),
                 "show-expanders", procdata->config.show_tree,
                 NULL);

    proctable_clear_tree (procdata);
    proctable_update_all (procdata);
}

static void
solaris_mode_changed_cb(GSettings *settings, const gchar *key, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);

    procdata->config.solaris_mode = g_settings_get_boolean(settings, key);
    proctable_update_all (procdata);
}


static void
network_in_bits_changed_cb(GSettings *settings, const gchar *key, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);

    procdata->config.network_in_bits = g_settings_get_boolean(settings, key);
    // force scale to be redrawn
    procdata->net_graph->clear_background();
}

static void
view_as_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);

    procdata->config.whose_process = g_settings_get_int (settings, key);
    procdata->config.whose_process = CLAMP (procdata->config.whose_process, 0, 2);
    proctable_clear_tree (procdata);
    proctable_update_all (procdata);
}

static void
warning_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);

    if (g_str_equal (key, "kill-dialog")) {
        procdata->config.show_kill_warning = g_settings_get_boolean (settings, key);
    }
}

static void
timeouts_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcData *procdata = static_cast<ProcData*>(data);

    if (g_str_equal (key, "update-interval")) {
        procdata->config.update_interval = g_settings_get_int (settings, key);
        procdata->config.update_interval =
            MAX (procdata->config.update_interval, 1000);

        procdata->smooth_refresh->reset();

        if(procdata->timeout) {
            g_source_remove (procdata->timeout);
            procdata->timeout = g_timeout_add (procdata->config.update_interval,
                                               cb_timeout,
                                               procdata);
        }
    }
    else if (g_str_equal (key, "graph-update-interval")){
        procdata->config.graph_update_interval = g_settings_get_int (settings, key);
        procdata->config.graph_update_interval =
            MAX (procdata->config.graph_update_interval,
                 250);
        load_graph_change_speed(procdata->cpu_graph,
                                procdata->config.graph_update_interval);
        load_graph_change_speed(procdata->mem_graph,
                                procdata->config.graph_update_interval);
        load_graph_change_speed(procdata->net_graph,
                                procdata->config.graph_update_interval);
    }
    else if (g_str_equal(key, "disks-interval")) {

        procdata->config.disks_update_interval = g_settings_get_int (settings, key);
        procdata->config.disks_update_interval =
            MAX (procdata->config.disks_update_interval, 1000);

        if(procdata->disk_timeout) {
            g_source_remove (procdata->disk_timeout);
            procdata->disk_timeout = \
                g_timeout_add (procdata->config.disks_update_interval,
                               cb_update_disks,
                               procdata);
        }
    }
    else {
        g_assert_not_reached();
    }
}

static void
apply_cpu_color_settings(GSettings *settings, ProcData * const procdata)
{
    GVariant *cpu_colors_var = g_settings_get_value(settings, "cpu-colors");
    gsize n = g_variant_n_children(cpu_colors_var);

    gchar *color;

    // Create builder to add the new colors if user has more than the number of cores with defaults.
    GVariantBuilder builder;
    GVariant* child;
    GVariant* full;

    g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

    for (guint i = 0; i < static_cast<guint>(procdata->config.num_cpus); i++) {
        if(i < n) {
            child = g_variant_get_child_value ( cpu_colors_var, i );
            g_variant_get_child( child, 1, "s", &color);
            g_variant_builder_add_value ( &builder, child);
        } else {
            color = g_strdup ("#f25915e815e8");
            g_variant_builder_add(&builder, "(us)", i, color);
        }
        gdk_color_parse(color, &procdata->config.cpu_color[i]);
        g_free (color);
    }
    full = g_variant_builder_end(&builder);
    // if the user has more cores than colors stored in the gsettings, store the newly built gvariant in gsettings
    if (n < static_cast<guint>(procdata->config.num_cpus)) {
        g_settings_set_value(settings, "cpu-colors", full);
    }
}

static void
color_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcData * const procdata = static_cast<ProcData*>(data);

    if (g_str_equal (key, "cpu-colors")) {
        apply_cpu_color_settings(settings, procdata);
        for (int i = 0; i < procdata->config.num_cpus; i++) {
            if(!gdk_color_equal(&procdata->cpu_graph->colors[i], &procdata->config.cpu_color[i])) {
                procdata->cpu_graph->colors[i] = procdata->config.cpu_color[i];
                break;
            }
        }
        return;
    }

    const gchar *color = g_settings_get_string (settings, key);
    if (g_str_equal (key, "mem-color")) {
        gdk_color_parse (color, &procdata->config.mem_color);
        procdata->mem_graph->colors.at(0) = procdata->config.mem_color;
    }
    else if (g_str_equal (key, "swap-color")) {
        gdk_color_parse (color, &procdata->config.swap_color);
        procdata->mem_graph->colors.at(1) = procdata->config.swap_color;
    }
    else if (g_str_equal (key, "net-in-color")) {
        gdk_color_parse (color, &procdata->config.net_in_color);
        procdata->net_graph->colors.at(0) = procdata->config.net_in_color;
    }
    else if (g_str_equal (key, "net-out-color")) {
        gdk_color_parse (color, &procdata->config.net_out_color);
        procdata->net_graph->colors.at(1) = procdata->config.net_out_color;
    }
    else {
        g_assert_not_reached();
    }
}

static void
show_all_fs_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcData * const procdata = static_cast<ProcData*>(data);

    procdata->config.show_all_fs = g_settings_get_boolean (settings, key);

    cb_update_disks (data);
}

static ProcData *
procman_data_new (GSettings *settings)
{
    ProcData *pd;
    gchar *color;
    gint swidth, sheight;
    gint i;
    glibtop_cpu cpu;

    pd = ProcData::get_instance();

    pd->config.width = g_settings_get_int (settings, "width");
    pd->config.height = g_settings_get_int (settings, "height");
    pd->config.xpos = g_settings_get_int (settings, "x-position");
    pd->config.ypos = g_settings_get_int (settings, "y-position");

    pd->config.show_tree = g_settings_get_boolean (settings, "show-tree");
    g_signal_connect (G_OBJECT(settings), "changed::show-tree", G_CALLBACK(tree_changed_cb), pd);

    pd->config.solaris_mode = g_settings_get_boolean(settings, procman::settings::solaris_mode.c_str());
    std::string detail_string("changed::" + procman::settings::solaris_mode);
    g_signal_connect(G_OBJECT(settings), detail_string.c_str(), G_CALLBACK(solaris_mode_changed_cb), pd);

    pd->config.network_in_bits = g_settings_get_boolean(settings, procman::settings::network_in_bits.c_str());
    detail_string = "changed::" + procman::settings::network_in_bits;
    g_signal_connect(G_OBJECT(settings), detail_string.c_str(), G_CALLBACK(network_in_bits_changed_cb), pd);

    pd->config.show_kill_warning = g_settings_get_boolean (settings, "kill-dialog");
    g_signal_connect (G_OBJECT(settings), "changed::kill-dialog", G_CALLBACK(warning_changed_cb), pd);
    pd->config.update_interval = g_settings_get_int (settings, "update-interval");
    g_signal_connect (G_OBJECT(settings), "changed::update-interval", G_CALLBACK(timeouts_changed_cb), pd);
    pd->config.graph_update_interval = g_settings_get_int (settings,
                                                           "graph-update-interval");
    g_signal_connect (G_OBJECT(settings), "changed::graph-update-interval",
                      G_CALLBACK(timeouts_changed_cb), pd);
    pd->config.disks_update_interval = g_settings_get_int (settings, "disks-interval");
    g_signal_connect (G_OBJECT(settings), "changed::disks-interval", G_CALLBACK(timeouts_changed_cb), pd);


    /* show_all_fs */
    pd->config.show_all_fs = g_settings_get_boolean (settings, "show-all-fs");
    g_signal_connect (settings, "changed::show-all-fs", G_CALLBACK(show_all_fs_changed_cb), pd);


    pd->config.whose_process = g_settings_get_int (settings, "view-as");
    g_signal_connect (G_OBJECT(settings), "changed::view-as", G_CALLBACK(view_as_changed_cb),pd);
    pd->config.current_tab = g_settings_get_int (settings, "current-tab");

    /* Determine number of cpus since libgtop doesn't really tell you*/
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

    apply_cpu_color_settings(settings, pd);
    g_signal_connect (G_OBJECT(settings), "changed::cpu-colors",
                      G_CALLBACK(color_changed_cb), pd);

    color = g_settings_get_string (settings, "mem-color");
    if (!color)
        color = g_strdup ("#000000ff0082");
    g_signal_connect (G_OBJECT(settings), "changed::mem-color",
                      G_CALLBACK(color_changed_cb), pd);
    gdk_color_parse(color, &pd->config.mem_color);

    g_free (color);

    color = g_settings_get_string (settings, "swap-color");
    if (!color)
        color = g_strdup ("#00b6000000ff");
    g_signal_connect (G_OBJECT(settings), "changed::swap-color",
                      G_CALLBACK(color_changed_cb), pd);
    gdk_color_parse(color, &pd->config.swap_color);
    g_free (color);

    color = g_settings_get_string (settings, "net-in-color");
    if (!color)
        color = g_strdup ("#000000f200f2");
    g_signal_connect (G_OBJECT(settings), "changed::net-in-color",
                      G_CALLBACK(color_changed_cb), pd);
    gdk_color_parse(color, &pd->config.net_in_color);
    g_free (color);

    color = g_settings_get_string (settings, "net-out-color");
    if (!color)
        color = g_strdup ("#00f2000000c1");
    g_signal_connect (G_OBJECT(settings), "changed::net-out-color",
                      G_CALLBACK(color_changed_cb), pd);
    gdk_color_parse(color, &pd->config.net_out_color);
    g_free (color);

    /* Sanity checks */
    swidth = gdk_screen_width ();
    sheight = gdk_screen_height ();
    pd->config.width = CLAMP (pd->config.width, 50, swidth);
    pd->config.height = CLAMP (pd->config.height, 50, sheight);
    pd->config.update_interval = MAX (pd->config.update_interval, 1000);
    pd->config.graph_update_interval = MAX (pd->config.graph_update_interval, 250);
    pd->config.disks_update_interval = MAX (pd->config.disks_update_interval, 1000);
    pd->config.whose_process = CLAMP (pd->config.whose_process, 0, 2);
    pd->config.current_tab = CLAMP(pd->config.current_tab,
                                   PROCMAN_TAB_SYSINFO,
                                   PROCMAN_TAB_DISKS);

    // delayed initialization as SmoothRefresh() needs ProcData
    // i.e. we can't call ProcData::get_instance
    pd->smooth_refresh = new SmoothRefresh(settings);

    return pd;

}

static void
procman_free_data (ProcData *procdata)
{

    proctable_free_table (procdata);
    delete procdata->smooth_refresh;
}


gboolean
procman_get_tree_state (GSettings *settings, GtkWidget *tree, const gchar *child_schema)
{
    GtkTreeModel *model;
    GList *columns, *it;
    gint sort_col;
    GtkSortType order;

    g_assert(tree);
    g_assert(child_schema);

    GSettings *pt_settings = g_settings_get_child (settings, child_schema);

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

    sort_col = g_settings_get_int (pt_settings, "sort-col");

    order = static_cast<GtkSortType>(g_settings_get_int (pt_settings, "sort-order"));

    if (sort_col != -1)
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
                                              sort_col,
                                              order);

    columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));

    if(!g_strcmp0(child_schema, "proctree"))
    {
        for(it = columns; it; it = it->next)
        {
            GtkTreeViewColumn *column;
            gint width;
            gboolean visible;
            int id;
            gchar *key;

            column = static_cast<GtkTreeViewColumn*>(it->data);
            id = gtk_tree_view_column_get_sort_column_id (column);

            key = g_strdup_printf ("col-%d-width", id);
            g_settings_get (pt_settings, key, "i", &width);
            g_free (key);

            key = g_strdup_printf ("col-%d-visible", id);
            visible = g_settings_get_boolean (pt_settings, key);
            g_free (key);

            column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), id);
            if(!column) continue;
            gtk_tree_view_column_set_visible (column, visible);
            if (visible) {
                /* ensure column is really visible */
                width = MAX(width, 10);
                gtk_tree_view_column_set_fixed_width(column, width);
            }
        }
    }

    if(!g_strcmp0(child_schema, "proctree") ||
       !g_strcmp0(child_schema, "disktreenew"))
    {
        GVariant     *value;
        GVariantIter iter;
        int          sortIndex;

        GSList *order = NULL;

        value = g_settings_get_value(pt_settings, "columns-order");
        g_variant_iter_init(&iter, value);

        while (g_variant_iter_loop (&iter, "i", &sortIndex))
            order = g_slist_append(order, GINT_TO_POINTER(sortIndex));

        proctable_set_columns_order(GTK_TREE_VIEW(tree), order);

        g_slist_free(order);
    }

    g_object_unref(pt_settings);
    pt_settings = NULL;

    g_list_free(columns);

    return TRUE;
}

void
procman_save_tree_state (GSettings *settings, GtkWidget *tree, const gchar *child_schema)
{
    GtkTreeModel *model;
    GList *columns;
    gint sort_col;
    GtkSortType order;

    g_assert(tree);
    g_assert(child_schema);

    GSettings *pt_settings = g_settings_get_child (settings, child_schema);

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
    if (gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model), &sort_col,
                                              &order)) {
        g_settings_set_int (pt_settings, "sort-col", sort_col);
        g_settings_set_int (pt_settings, "sort-order", order);
    }

    columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));

    if(!g_strcmp0(child_schema, "proctree") || !g_strcmp0(child_schema, "disktreenew"))
    {
        GSList *order;
        GSList *order_node;
        GVariantBuilder *builder;
        GVariant *order_variant;

        order = proctable_get_columns_order(GTK_TREE_VIEW(tree));

        builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);

        for(order_node = order; order_node; order_node = order_node->next)
            g_variant_builder_add(builder, "i", GPOINTER_TO_INT(order_node->data));

        order_variant = g_variant_new("ai", builder);
        g_settings_set_value(pt_settings, "columns-order", order_variant);

        g_slist_free(order);
    }

    g_list_free(columns);
}

void
procman_save_config (ProcData *data)
{
    GSettings *settings = data->settings;

    g_assert(data);

    procman_save_tree_state (data->settings, data->tree, "proctree");
    procman_save_tree_state (data->settings, data->disk_list, "disktreenew");

    data->config.width  = gdk_window_get_width (gtk_widget_get_window (data->app));
    data->config.height = gdk_window_get_height(gtk_widget_get_window (data->app));
    gtk_window_get_position(GTK_WINDOW(data->app), &data->config.xpos, &data->config.ypos);

    g_settings_set_int (settings, "width", data->config.width);
    g_settings_set_int (settings, "height", data->config.height);
    g_settings_set_int (settings, "x-position", data->config.xpos);
    g_settings_set_int (settings, "y-position", data->config.ypos);

    g_settings_set_int (settings, "current-tab", data->config.current_tab);

    g_settings_sync ();
}

static guint32
get_startup_timestamp ()
{
    const gchar *startup_id_env;
    gchar *startup_id = NULL;
    gchar *time_str;
    gulong retval = 0;

    /* we don't unset the env, since startup-notification
     * may still need it */
    startup_id_env = g_getenv ("DESKTOP-STARTUP-ID");
    if (startup_id_env == NULL)
        goto out;

    startup_id = g_strdup (startup_id_env);

    time_str = g_strrstr (startup_id, "_TIME");
    if (time_str == NULL)
        goto out;

    /* Skip past the "_TIME" part */
    time_str += 5;

    retval = strtoul (time_str, NULL, 0);

  out:
    g_free (startup_id);

    return retval;
}

static void
set_tab(GtkNotebook* notebook, gint tab, ProcData* procdata)
{
    gtk_notebook_set_current_page(notebook, tab);
    cb_change_current_page(notebook, tab, procdata);
}

static void
cb_server (const gchar *msg, gpointer user_data)
{
    GdkWindow *window;
    ProcData *procdata;
    guint32 timestamp = 0;

    window = gdk_get_default_root_window ();

    procdata = *(ProcData**)user_data;
    g_assert (procdata != NULL);

    procman_debug("cb_server(%s)", msg);
    if (msg != NULL) {
        if (procman::SHOW_SYSTEM_TAB_CMD == msg) {
            procman_debug("Changing to PROCMAN_TAB_SYSINFO via bacon message");
            set_tab(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_SYSINFO, procdata);
        } else if (procman::SHOW_PROCESSES_TAB_CMD == msg) {
            procman_debug("Changing to PROCMAN_TAB_PROCESSES via bacon message");
            set_tab(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_PROCESSES, procdata);
        } else if (procman::SHOW_RESOURCES_TAB_CMD == msg) {
            procman_debug("Changing to PROCMAN_TAB_RESOURCES via bacon message");
            set_tab(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_RESOURCES, procdata);
        } else if (procman::SHOW_FILE_SYSTEMS_TAB_CMD == msg) {
            procman_debug("Changing to PROCMAN_TAB_DISKS via bacon message");
            set_tab(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_DISKS, procdata);
        }
    } else
        timestamp = strtoul(msg, NULL, 0);

    if (timestamp == 0)
    {
        /* fall back to rountripping to X */
        timestamp = gdk_x11_get_server_time (window);
    }

    gdk_x11_window_set_user_time (window, timestamp);

    gtk_window_present (GTK_WINDOW(procdata->app));
}




static void
mount_changed(const Glib::RefPtr<Gio::Mount>&)
{
    cb_update_disks(ProcData::get_instance());
}


static void
init_volume_monitor(ProcData *procdata)
{
    using namespace Gio;
    using namespace Glib;

    RefPtr<VolumeMonitor> monitor = VolumeMonitor::get();

    monitor->signal_mount_added().connect(sigc::ptr_fun(&mount_changed));
    monitor->signal_mount_changed().connect(sigc::ptr_fun(&mount_changed));
    monitor->signal_mount_removed().connect(sigc::ptr_fun(&mount_changed));
}


namespace procman
{
    const std::string SHOW_SYSTEM_TAB_CMD("SHOWSYSTAB");
    const std::string SHOW_PROCESSES_TAB_CMD("SHOWPROCTAB");
    const std::string SHOW_RESOURCES_TAB_CMD("SHOWRESTAB");
    const std::string SHOW_FILE_SYSTEMS_TAB_CMD("SHOWFSTAB");
}


int
main (int argc, char *argv[])
{
    guint32 startup_timestamp;
    GSettings *settings;
    ProcData *procdata;
    BaconMessageConnection *conn;

    bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
    setlocale (LC_ALL, "");

    startup_timestamp = get_startup_timestamp();

    Glib::OptionContext context;
    context.set_summary(_("A simple process and system monitor."));
    context.set_ignore_unknown_options(true);
    procman::OptionGroup option_group;
    context.set_main_group(option_group);

    try {
        context.parse(argc, argv);
    } catch (const Glib::Error& ex) {
        g_error("Arguments parse error : %s", ex.what().c_str());
    }

    Gio::init();
    Gtk::Main kit(&argc, &argv);
    procman_debug("post gtk_init");

    conn = bacon_message_connection_new ("gnome-system-monitor");
    if (!conn) g_error("Couldn't connect to gnome-system-monitor");

    if (bacon_message_connection_get_is_server (conn))
    {
        bacon_message_connection_set_callback (conn, cb_server, &procdata);
    }
    else /* client */
    {
        char *timestamp;

        timestamp = g_strdup_printf ("%" G_GUINT32_FORMAT, startup_timestamp);

        if (option_group.show_system_tab)
            bacon_message_connection_send(conn, procman::SHOW_SYSTEM_TAB_CMD.c_str());

        if (option_group.show_processes_tab)
            bacon_message_connection_send(conn, procman::SHOW_PROCESSES_TAB_CMD.c_str());

        if (option_group.show_resources_tab)
            bacon_message_connection_send(conn, procman::SHOW_RESOURCES_TAB_CMD.c_str());

        if (option_group.show_file_systems_tab)
            bacon_message_connection_send(conn, procman::SHOW_FILE_SYSTEMS_TAB_CMD.c_str());

        bacon_message_connection_send (conn, timestamp);

        gdk_notify_startup_complete ();

        g_free (timestamp);
        bacon_message_connection_free (conn);

        exit (0);
    }

    g_type_init ();

    gtk_window_set_default_icon_name ("utilities-system-monitor");
    g_set_application_name(_("System Monitor"));

    settings = g_settings_new (GSM_GSETTINGS_SCHEMA);

    glibtop_init ();

    procman_debug("end init");

    procdata = procman_data_new (settings);
    procdata->settings = g_settings_new(GSM_GSETTINGS_SCHEMA);

    procman_debug("begin create_main_window");
    create_main_window (procdata);
    procman_debug("end create_main_window");

    // proctable_update_all (procdata);

    init_volume_monitor (procdata);

    g_assert(procdata->app);

    if (option_group.show_system_tab) {
        procman_debug("Starting with PROCMAN_TAB_SYSINFO by commandline request");
        set_tab(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_SYSINFO, procdata);
    } else if (option_group.show_processes_tab) {
        procman_debug("Starting with PROCMAN_TAB_PROCESSES by commandline request");
        set_tab(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_PROCESSES, procdata);
    } else if (option_group.show_resources_tab) {
        procman_debug("Starting with PROCMAN_TAB_RESOURCES by commandline request");
        set_tab(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_RESOURCES, procdata);
    } else if (option_group.show_file_systems_tab) {
        procman_debug("Starting with PROCMAN_TAB_DISKS by commandline request");
        set_tab(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_DISKS, procdata);
    }

    gtk_widget_set_name(procdata->app, "gnome-system-monitor");
    gtk_widget_show(procdata->app);

    procman_debug("begin gtk_main");
    kit.run();

    procman_free_data (procdata);

    glibtop_close ();

    return 0;
}

