#include <config.h>

#include <glib/gi18n.h>
#include <glibtop.h>
#include <glibtop/close.h>

#include "procman-app.h"
#include "interface.h"
#include "proctable.h"
#include "callbacks.h"
#include "load-graph.h"
#include "settings-keys.h"
#include "argv.h"
#include "util.h"

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
tree_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    app->config.show_tree = g_settings_get_boolean(settings, key);

    g_object_set(G_OBJECT(app->tree),
                 "show-expanders", app->config.show_tree,
                 NULL);

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
view_as_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    app->config.whose_process = g_settings_get_int (settings, key);
    app->config.whose_process = CLAMP (app->config.whose_process, 0, 2);
    proctable_clear_tree (app);
    proctable_update_all (app);
}

static void
warning_changed_cb (GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    if (g_str_equal (key, "kill-dialog")) {
        app->config.show_kill_warning = g_settings_get_boolean (settings, key);
    }
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

    config.show_tree = g_settings_get_boolean (settings, "show-tree");
    g_signal_connect (G_OBJECT(settings), "changed::show-tree", G_CALLBACK(tree_changed_cb), this);

    config.solaris_mode = g_settings_get_boolean(settings, procman::settings::solaris_mode.c_str());
    std::string detail_string("changed::" + procman::settings::solaris_mode);
    g_signal_connect(G_OBJECT(settings), detail_string.c_str(), G_CALLBACK(solaris_mode_changed_cb), this);

    config.draw_stacked = g_settings_get_boolean(settings, procman::settings::draw_stacked.c_str());
    detail_string = "changed::" + procman::settings::draw_stacked;
    g_signal_connect(G_OBJECT(settings), detail_string.c_str(), G_CALLBACK(draw_stacked_changed_cb), this);

    config.network_in_bits = g_settings_get_boolean(settings, procman::settings::network_in_bits.c_str());
    detail_string = "changed::" + procman::settings::network_in_bits;
    g_signal_connect(G_OBJECT(settings), detail_string.c_str(), G_CALLBACK(network_in_bits_changed_cb), this);

    config.show_kill_warning = g_settings_get_boolean (settings, "kill-dialog");
    g_signal_connect (G_OBJECT(settings), "changed::kill-dialog", G_CALLBACK(warning_changed_cb), this);
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


    config.whose_process = g_settings_get_int (settings, "view-as");
    g_signal_connect (G_OBJECT(settings), "changed::view-as", G_CALLBACK(view_as_changed_cb), this);
    config.current_tab = g_settings_get_int (settings, "current-tab");

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
    config.whose_process = CLAMP (config.whose_process, 0, 2);
    config.current_tab = CLAMP(config.current_tab,
                               PROCMAN_TAB_PROCESSES,
                               PROCMAN_TAB_DISKS);
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
set_tab(GtkNotebook* notebook, gint tab, ProcmanApp *app)
{
    gtk_notebook_set_current_page(notebook, tab);
    cb_change_current_page(notebook, tab, app);
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

    g_settings_set_int (settings, "current-tab", config.current_tab);

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

    if (option_group.show_processes_tab) {
        procman_debug("Starting with PROCMAN_TAB_PROCESSES by commandline request");
        set_tab(GTK_NOTEBOOK(notebook), PROCMAN_TAB_PROCESSES, this);
    } else if (option_group.show_resources_tab) {
        procman_debug("Starting with PROCMAN_TAB_RESOURCES by commandline request");
        set_tab(GTK_NOTEBOOK(notebook), PROCMAN_TAB_RESOURCES, this);
    } else if (option_group.show_file_systems_tab) {
        procman_debug("Starting with PROCMAN_TAB_DISKS by commandline request");
        set_tab(GTK_NOTEBOOK(notebook), PROCMAN_TAB_DISKS, this);
    }

    on_activate ();

    return 0;
}

void
ProcmanApp::on_help_activate(const Glib::VariantBase&)
{
    cb_help_contents (NULL, this);
}

void
ProcmanApp::on_lsof_activate(const Glib::VariantBase&)
{
    cb_show_lsof (NULL, this);
}

void
ProcmanApp::on_preferences_activate(const Glib::VariantBase&)
{
    cb_edit_preferences (NULL, this);
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

    char* filename = g_build_filename (GSM_DATA_DIR, "menus.ui", NULL);
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file(filename);
    g_free (filename);

    Glib::RefPtr<Gio::Menu> menu = Glib::RefPtr<Gio::Menu>::cast_static(builder->get_object ("app-menu"));
    set_app_menu (menu);

    Gtk::Window::set_default_icon_name ("utilities-system-monitor");

    glibtop_init ();

    load_settings ();

    pretty_table = new PrettyTable();
    smooth_refresh = new SmoothRefresh(settings);

    create_main_window (this);

    init_volume_monitor (this);

    gtk_widget_show (main_window);
}

