#include <glib/gi18n.h>
#include <glibtop.h>
#include <glibtop/close.h>

#include "procman-app.h"
#include "procman.h"
#include "interface.h"
#include "proctable.h"
#include "callbacks.h"
#include "load-graph.h"
#include "settings-keys.h"
#include "argv.h"
#include "util.h"

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

    pd->terminating = FALSE;

    return pd;

}

static void
procman_free_data (ProcData *procdata)
{

    proctable_free_table (procdata);
    delete procdata->smooth_refresh;
}

ProcmanApp::ProcmanApp() : Gtk::Application("org.gnome.SystemMonitor", Gio::APPLICATION_HANDLES_COMMAND_LINE)
{
    Glib::set_application_name(_("System Monitor"));
}

Glib::RefPtr<ProcmanApp> ProcmanApp::create ()
{
    return Glib::RefPtr<ProcmanApp>(new ProcmanApp());
}

void ProcmanApp::on_activate()
{
    gtk_window_present (GTK_WINDOW (procdata->app));
}

static void
set_tab(GtkNotebook* notebook, gint tab, ProcData* procdata)
{
    gtk_notebook_set_current_page(notebook, tab);
    cb_change_current_page(notebook, tab, procdata);
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

    on_activate ();

    return 0;
}

void ProcmanApp::on_startup()
{
    Gtk::Application::on_startup();

    GSettings *settings;

    Gtk::Window::set_default_icon_name ("utilities-system-monitor");

    settings = g_settings_new (GSM_GSETTINGS_SCHEMA);

    glibtop_init ();

    procdata = procman_data_new (settings);
    procdata->settings = g_settings_new(GSM_GSETTINGS_SCHEMA);

    create_main_window (procdata);
    init_volume_monitor (procdata);

    Gtk::Window *window = Glib::wrap(GTK_WINDOW(procdata->app));
    window->show();
    window->set_name ("gnome-system-monitor");

    add_window (*window);
}

void ProcmanApp::on_shutdown()
{
    procman_free_data(procdata);
    glibtop_close();
}
