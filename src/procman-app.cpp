#include <config.h>

#include <glib/gi18n.h>
#include <glibtop.h>
#include <glibtop/close.h>

#include "procman-app.h"
#include "procdialogs.h"
#include "interface.h"
#include "proctable.h"
#include "callbacks.h"
#include "load-graph.h"
#include "settings-keys.h"
#include "argv.h"
#include "util.h"
#include "cgroups.h"
#include "lsof.h"

static void
mount_changed(const Glib::RefPtr<Gio::Mount>&, ProcmanApp *app)
{
    cb_update_disks(app);
}


static void
init_volume_monitor(ProcmanApp *app)
{
    using namespace Gio;
    using namespace Glib;

    RefPtr<VolumeMonitor> monitor = VolumeMonitor::get();

    monitor->signal_mount_added().connect(sigc::bind<ProcmanApp *>(sigc::ptr_fun(&mount_changed), app));
    monitor->signal_mount_changed().connect(sigc::bind<ProcmanApp *>(sigc::ptr_fun(&mount_changed), app));
    monitor->signal_mount_removed().connect(sigc::bind<ProcmanApp *>(sigc::ptr_fun(&mount_changed), app));
}

static void
cb_show_dependencies_changed (GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (app->tree),
                                      g_settings_get_boolean (settings, "show-dependencies"));

    proctable_clear_tree (app);
    proctable_update_all (app);
}

static void
solaris_mode_changed_cb(GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    app->config.solaris_mode = g_settings_get_boolean(settings, key);
    app->cpu_graph->clear_background();
    proctable_update_all (app);
}

static void
draw_stacked_changed_cb(GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    app->config.draw_stacked = g_settings_get_boolean(settings, key);
    app->cpu_graph->clear_background();
    load_graph_reset(app->cpu_graph);
}


static void
network_in_bits_changed_cb(GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    app->config.network_in_bits = g_settings_get_boolean(settings, key);
    // force scale to be redrawn
    app->net_graph->clear_background();
}

static void
cb_show_whose_processes_changed (GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    proctable_clear_tree (app);
    proctable_update_all (app);
}

static void
timeouts_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    if (g_str_equal (key, "update-interval")) {
        app->config.update_interval = g_settings_get_int (settings, key);
        app->config.update_interval =
            MAX (app->config.update_interval, 1000);

        app->smooth_refresh->reset();

        if(app->timeout) {
            g_source_remove (app->timeout);
            app->timeout = g_timeout_add (app->config.update_interval,
                                          cb_timeout,
                                          app);
        }
    }
    else if (g_str_equal (key, "graph-update-interval")){
        app->config.graph_update_interval = g_settings_get_int (settings, key);
        app->config.graph_update_interval =
            MAX (app->config.graph_update_interval,
                 250);
        load_graph_change_speed(app->cpu_graph,
                                app->config.graph_update_interval);
        load_graph_change_speed(app->mem_graph,
                                app->config.graph_update_interval);
        load_graph_change_speed(app->net_graph,
                                app->config.graph_update_interval);
    }
    else if (g_str_equal(key, "disks-interval")) {

        app->config.disks_update_interval = g_settings_get_int (settings, key);
        app->config.disks_update_interval =
            MAX (app->config.disks_update_interval, 1000);

        if(app->disk_timeout) {
            g_source_remove (app->disk_timeout);
            app->disk_timeout = \
                g_timeout_add (app->config.disks_update_interval,
                               cb_update_disks,
                               app);
        }
    }
    else {
        g_assert_not_reached();
    }
}

static void
apply_cpu_color_settings(GSettings *settings, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    GVariant *cpu_colors_var = g_settings_get_value(settings, "cpu-colors");
    gsize n = g_variant_n_children(cpu_colors_var);

    gchar *color;

    // Create builder to add the new colors if user has more than the number of cores with defaults.
    GVariantBuilder builder;
    GVariant* child;
    GVariant* full;

    g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

    for (guint i = 0; i < static_cast<guint>(app->config.num_cpus); i++) {
        if(i < n) {
            child = g_variant_get_child_value ( cpu_colors_var, i );
            g_variant_get_child( child, 1, "s", &color);
            g_variant_builder_add_value ( &builder, child);
            g_variant_unref (child);
        } else {
            color = g_strdup ("#f25915e815e8");
            g_variant_builder_add(&builder, "(us)", i, color);
        }
        gdk_rgba_parse(&app->config.cpu_color[i], color);
        g_free (color);
    }
    full = g_variant_builder_end(&builder);
    // if the user has more cores than colors stored in the gsettings, store the newly built gvariant in gsettings
    if (n < static_cast<guint>(app->config.num_cpus)) {
        g_settings_set_value(settings, "cpu-colors", full);
    } else {
        g_variant_unref(full);
    }

    g_variant_unref(cpu_colors_var);
}

static void
color_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    if (g_str_equal (key, "cpu-colors")) {
        apply_cpu_color_settings(settings, app);
        for (int i = 0; i < app->config.num_cpus; i++) {
            if(!gdk_rgba_equal(&app->cpu_graph->colors[i], &app->config.cpu_color[i])) {
                app->cpu_graph->colors[i] = app->config.cpu_color[i];
                break;
            }
        }
        return;
    }

    gchar *color = g_settings_get_string (settings, key);
    if (g_str_equal (key, "mem-color")) {
        gdk_rgba_parse (&app->config.mem_color, color);
        app->mem_graph->colors.at(0) = app->config.mem_color;
    }
    else if (g_str_equal (key, "swap-color")) {
        gdk_rgba_parse (&app->config.swap_color, color);
        app->mem_graph->colors.at(1) = app->config.swap_color;
    }
    else if (g_str_equal (key, "net-in-color")) {
        gdk_rgba_parse (&app->config.net_in_color, color);
        app->net_graph->colors.at(0) = app->config.net_in_color;
    }
    else if (g_str_equal (key, "net-out-color")) {
        gdk_rgba_parse (&app->config.net_out_color, color);
        app->net_graph->colors.at(1) = app->config.net_out_color;
    }
    else {
        g_assert_not_reached();
    }
    g_free (color);
}

static void
show_all_fs_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    app->config.show_all_fs = g_settings_get_boolean (settings, key);

    cb_update_disks (app);
}

void
ProcmanApp::load_settings()
{
    gchar *color;
    gint swidth, sheight;
    gint i;
    glibtop_cpu cpu;

    settings = g_settings_new (GSM_GSETTINGS_SCHEMA);

    config.width = g_settings_get_int (settings, "width");
    config.height = g_settings_get_int (settings, "height");
    config.xpos = g_settings_get_int (settings, "x-position");
    config.ypos = g_settings_get_int (settings, "y-position");
    config.maximized = g_settings_get_boolean (settings, "maximized");

    g_signal_connect (G_OBJECT(settings), "changed::show-dependencies", G_CALLBACK(cb_show_dependencies_changed), this);

    config.solaris_mode = g_settings_get_boolean(settings, procman::settings::solaris_mode.c_str());
    std::string detail_string("changed::" + procman::settings::solaris_mode);
    g_signal_connect(G_OBJECT(settings), detail_string.c_str(), G_CALLBACK(solaris_mode_changed_cb), this);

    config.draw_stacked = g_settings_get_boolean(settings, procman::settings::draw_stacked.c_str());
    detail_string = "changed::" + procman::settings::draw_stacked;
    g_signal_connect(G_OBJECT(settings), detail_string.c_str(), G_CALLBACK(draw_stacked_changed_cb), this);

    config.network_in_bits = g_settings_get_boolean(settings, procman::settings::network_in_bits.c_str());
    detail_string = "changed::" + procman::settings::network_in_bits;
    g_signal_connect(G_OBJECT(settings), detail_string.c_str(), G_CALLBACK(network_in_bits_changed_cb), this);

    config.update_interval = g_settings_get_int (settings, "update-interval");
    g_signal_connect (G_OBJECT(settings), "changed::update-interval", G_CALLBACK(timeouts_changed_cb), this);
    config.graph_update_interval = g_settings_get_int (settings,
                                                           "graph-update-interval");
    g_signal_connect (G_OBJECT(settings), "changed::graph-update-interval",
                      G_CALLBACK(timeouts_changed_cb), this);
    config.disks_update_interval = g_settings_get_int (settings, "disks-interval");
    g_signal_connect (G_OBJECT(settings), "changed::disks-interval", G_CALLBACK(timeouts_changed_cb), this);


    /* show_all_fs */
    config.show_all_fs = g_settings_get_boolean (settings, "show-all-fs");
    g_signal_connect (settings, "changed::show-all-fs", G_CALLBACK(show_all_fs_changed_cb), this);


    g_signal_connect (G_OBJECT(settings), "changed::show-whose-processes", G_CALLBACK(cb_show_whose_processes_changed), this);

    /* Determine number of cpus since libgtop doesn't really tell you*/
    config.num_cpus = 0;
    glibtop_get_cpu (&cpu);
    frequency = cpu.frequency;
    i=0;
    while (i < GLIBTOP_NCPU && cpu.xcpu_total[i] != 0) {
        config.num_cpus ++;
        i++;
    }
    if (config.num_cpus == 0)
        config.num_cpus = 1;

    apply_cpu_color_settings(settings, this);
    g_signal_connect (G_OBJECT(settings), "changed::cpu-colors",
                      G_CALLBACK(color_changed_cb), this);

    color = g_settings_get_string (settings, "mem-color");
    if (!color)
        color = g_strdup ("#000000ff0082");
    g_signal_connect (G_OBJECT(settings), "changed::mem-color",
                      G_CALLBACK(color_changed_cb), this);
    gdk_rgba_parse(&config.mem_color, color);

    g_free (color);

    color = g_settings_get_string (settings, "swap-color");
    if (!color)
        color = g_strdup ("#00b6000000ff");
    g_signal_connect (G_OBJECT(settings), "changed::swap-color",
                      G_CALLBACK(color_changed_cb), this);
    gdk_rgba_parse(&config.swap_color, color);
    g_free (color);

    color = g_settings_get_string (settings, "net-in-color");
    if (!color)
        color = g_strdup ("#000000f200f2");
    g_signal_connect (G_OBJECT(settings), "changed::net-in-color",
                      G_CALLBACK(color_changed_cb), this);
    gdk_rgba_parse(&config.net_in_color, color);
    g_free (color);

    color = g_settings_get_string (settings, "net-out-color");
    if (!color)
        color = g_strdup ("#00f2000000c1");
    g_signal_connect (G_OBJECT(settings), "changed::net-out-color",
                      G_CALLBACK(color_changed_cb), this);
    gdk_rgba_parse(&config.net_out_color, color);
    g_free (color);

    /* Sanity checks */
    swidth = gdk_screen_width ();
    sheight = gdk_screen_height ();
    config.width = CLAMP (config.width, 50, swidth);
    config.height = CLAMP (config.height, 50, sheight);
    config.update_interval = MAX (config.update_interval, 1000);
    config.graph_update_interval = MAX (config.graph_update_interval, 250);
    config.disks_update_interval = MAX (config.disks_update_interval, 1000);
}

ProcmanApp::ProcmanApp() : Gtk::Application("org.gnome.SystemMonitor", Gio::APPLICATION_HANDLES_COMMAND_LINE)
{
    Glib::set_application_name(_("System Monitor"));
}

Glib::RefPtr<ProcmanApp> ProcmanApp::get ()
{
    static Glib::RefPtr<ProcmanApp> singleton;

    if (!singleton) {
        singleton = Glib::RefPtr<ProcmanApp>(new ProcmanApp());
    }
    return singleton;
}

void ProcmanApp::on_activate()
{
    gtk_window_present (GTK_WINDOW (main_window));
}

static void
cb_header_menu_position_function (GtkMenu* menu, gint *x, gint *y, gboolean *push_in, gpointer data)
{
    GdkEventButton* event = (GdkEventButton *) data;
    gint wx, wy, ww, wh;
    gdk_window_get_geometry(event->window, &wx, &wy, &ww, &wh);
    gdk_window_get_origin(event->window, &wx, &wy);

    *x = wx + event->x;
    *y = wy + wh;
    *push_in = TRUE;
}

static gboolean
cb_column_header_clicked (GtkTreeViewColumn* column, GdkEvent* event, gpointer data)
{
    GtkMenu *menu = (GtkMenu *) data;

    if (event->button.button == GDK_BUTTON_SECONDARY) {
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, cb_header_menu_position_function, &(event->button), event->button.button, event->button.time);
        return TRUE;
    }

    return FALSE;
}

gboolean
procman_get_tree_state (GSettings *settings, GtkWidget *tree, const gchar *child_schema)
{
    GtkTreeModel *model;
    GList *columns, *it;
    gint sort_col;
    GtkSortType order;
    GtkWidget *header_menu;
   
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

    if(!g_strcmp0(child_schema, "proctree") ||
       !g_strcmp0(child_schema, "disktreenew"))
    {
        header_menu = gtk_menu_new();
        
        for(it = columns; it; it = it->next)
        {
            GtkTreeViewColumn *column;
            gint width;
            gboolean visible;
            int id;
            const gchar *title;
            gchar *key;
            GtkWidget* column_item;
            GtkWidget* button;

            column = static_cast<GtkTreeViewColumn*>(it->data);
            id = gtk_tree_view_column_get_sort_column_id (column);

            key = g_strdup_printf ("col-%d-width", id);
            g_settings_get (pt_settings, key, "i", &width);
            g_free (key);

            key = g_strdup_printf ("col-%d-visible", id);
            visible = g_settings_get_boolean (pt_settings, key);
            g_free (key);

            gtk_tree_view_column_set_visible (column, visible);
            /* ensure column is really visible */
            width = MAX(width, 50);
            gtk_tree_view_column_set_fixed_width(column, width);

            if ((id == COL_CGROUP) && (!cgroups_enabled()))
                continue;

            if ((id == COL_UNIT ||
                id == COL_SESSION ||
                id == COL_SEAT ||
                id == COL_OWNER)
#ifdef HAVE_SYSTEMD
                && !LOGIND_RUNNING()
#endif
                )
                continue;
            // set the states for the columns visibility menu available by header right click
            title = gtk_tree_view_column_get_title (column);

            if (!title)
                title = _("Icon");
            button = gtk_tree_view_column_get_button(column);
            g_signal_connect(G_OBJECT(button), "button_press_event", G_CALLBACK(cb_column_header_clicked), header_menu);

            key = g_strdup_printf ("col-%d-visible", id);
            column_item = gtk_check_menu_item_new_with_label(title);
            g_settings_bind(pt_settings, key, G_OBJECT(column_item), "active", G_SETTINGS_BIND_DEFAULT);
            g_settings_bind(pt_settings, key, G_OBJECT(column), "visible", G_SETTINGS_BIND_DEFAULT);
            g_free (key);

            gtk_menu_shell_append(GTK_MENU_SHELL(header_menu), column_item);
        }
        gtk_widget_show_all(header_menu);
        GVariant     *value;
        GVariantIter iter;
        int          sortIndex;

        GSList *order = NULL;

        value = g_settings_get_value(pt_settings, "columns-order");
        g_variant_iter_init(&iter, value);

        while (g_variant_iter_loop (&iter, "i", &sortIndex))
            order = g_slist_append(order, GINT_TO_POINTER(sortIndex));

        proctable_set_columns_order(GTK_TREE_VIEW(tree), order);

        g_variant_unref(value);
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

        g_variant_builder_unref(builder);
        g_slist_free(order);
    }

    g_list_free(columns);
}

void
ProcmanApp::save_config ()
{
    procman_save_tree_state (settings, tree, "proctree");
    procman_save_tree_state (settings, disk_list, "disktreenew");

    config.width  = gdk_window_get_width (gtk_widget_get_window (main_window));
    config.height = gdk_window_get_height(gtk_widget_get_window (main_window));
    gtk_window_get_position(GTK_WINDOW(main_window), &config.xpos, &config.ypos);

    config.maximized = gdk_window_get_state(gtk_widget_get_window (main_window)) & GDK_WINDOW_STATE_MAXIMIZED;

    g_settings_set_int (settings, "width", config.width);
    g_settings_set_int (settings, "height", config.height);
    g_settings_set_int (settings, "x-position", config.xpos);
    g_settings_set_int (settings, "y-position", config.ypos);
    g_settings_set_boolean (settings, "maximized", config.maximized);

    g_settings_sync ();
}

int ProcmanApp::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line)
{
    int argc = 0;
    char** argv = command_line->get_arguments(argc);

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

    g_strfreev(argv);

    if (option_group.show_processes_tab) {
        procman_debug("Starting with PROCMAN_TAB_PROCESSES by commandline request");
        g_settings_set_int (settings, "current-tab", PROCMAN_TAB_PROCESSES);
    } else if (option_group.show_resources_tab) {
        procman_debug("Starting with PROCMAN_TAB_RESOURCES by commandline request");
        g_settings_set_int (settings, "current-tab", PROCMAN_TAB_RESOURCES);
    } else if (option_group.show_file_systems_tab) {
        procman_debug("Starting with PROCMAN_TAB_DISKS by commandline request");
        g_settings_set_int (settings, "current-tab", PROCMAN_TAB_DISKS);
    } else if (option_group.print_version) {
        g_print("%s %s\n", _("GNOME System Monitor"), VERSION);
	exit (EXIT_SUCCESS);
	return 0;
    }

    on_activate ();

    return 0;
}

void
ProcmanApp::on_help_activate(const Glib::VariantBase&)
{
    GError* error = 0;
    if (!g_app_info_launch_default_for_uri("help:gnome-system-monitor", NULL, &error)) {
        g_warning("Could not display help : %s", error->message);
        g_error_free(error);
    }
}

void
ProcmanApp::on_lsof_activate(const Glib::VariantBase&)
{
    procman_lsof(this);
}

void
ProcmanApp::on_preferences_activate(const Glib::VariantBase&)
{
    procdialog_create_preferences_dialog (this);
}

void
ProcmanApp::on_quit_activate(const Glib::VariantBase&)
{
    shutdown ();
}

void
ProcmanApp::shutdown()
{
    save_config ();

    if (timeout)
        g_source_remove (timeout);
    if (disk_timeout)
        g_source_remove (disk_timeout);

    proctable_free_table (this);
    delete smooth_refresh;
    delete pretty_table;

    glibtop_close();

    quit();
}

void ProcmanApp::on_startup()
{
    Gtk::Application::on_startup();

    Glib::RefPtr<Gio::SimpleAction> action;

    action = Gio::SimpleAction::create("quit");
    action->signal_activate().connect(sigc::mem_fun(*this, &ProcmanApp::on_quit_activate));
    add_action(action);

    action = Gio::SimpleAction::create("help");
    action->signal_activate().connect(sigc::mem_fun(*this, &ProcmanApp::on_help_activate));
    add_action(action);

    action = Gio::SimpleAction::create("lsof");
    action->signal_activate().connect(sigc::mem_fun(*this, &ProcmanApp::on_lsof_activate));
    add_action(action);

    action = Gio::SimpleAction::create("preferences");
    action->signal_activate().connect(sigc::mem_fun(*this, &ProcmanApp::on_preferences_activate));
    add_action(action);

    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/org/gnome/gnome-system-monitor/data/menus.ui");

    Glib::RefPtr<Gio::Menu> menu = Glib::RefPtr<Gio::Menu>::cast_static(builder->get_object ("app-menu"));
    set_app_menu (menu);

    add_accelerator("<Primary>d", "win.show-dependencies", NULL);
    add_accelerator("<Primary>s", "win.send-signal-stop", NULL);
    add_accelerator("<Primary>c", "win.send-signal-cont", NULL);
    add_accelerator("<Primary>e", "win.send-signal-end", NULL);
    add_accelerator("<Primary>k", "win.send-signal-kill", NULL);
    add_accelerator("<Primary>m", "win.memory-maps", NULL);
    add_accelerator("<Primary>f", "win.open-files", NULL);

    Gtk::Window::set_default_icon_name ("utilities-system-monitor");

    glibtop_init ();

    load_settings ();

    pretty_table = new PrettyTable();
    smooth_refresh = new SmoothRefresh(settings);

    create_main_window (this);

    init_volume_monitor (this);

    add_accelerator ("<Alt>1", "win.show-page", g_variant_new_int32 (PROCMAN_TAB_PROCESSES));
    add_accelerator ("<Alt>2", "win.show-page", g_variant_new_int32 (PROCMAN_TAB_RESOURCES));
    add_accelerator ("<Alt>3", "win.show-page", g_variant_new_int32 (PROCMAN_TAB_DISKS));
    add_accelerator ("<Primary>r", "win.refresh", NULL);

    gtk_widget_show (main_window);
}

