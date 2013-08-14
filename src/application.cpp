/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <config.h>

#include <glib/gi18n.h>
#include <glibtop.h>
#include <glibtop/close.h>
#include <signal.h>

#include "application.h"
#include "procdialogs.h"
#include "prefsdialog.h"
#include "interface.h"
#include "proctable.h"
#include "load-graph.h"
#include "settings-keys.h"
#include "argv.h"
#include "util.h"
#include "lsof.h"
#include "disks.h"

static void
cb_solaris_mode_changed (GSettings *settings, const gchar *key, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);

    app->config.solaris_mode = g_settings_get_boolean(settings, key);
    app->cpu_graph->clear_background();
    proctable_update (app);
}

static void
cb_draw_stacked_changed (GSettings *settings, const gchar *key, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);

    app->config.draw_stacked = g_settings_get_boolean(settings, key);
    app->cpu_graph->clear_background();
    load_graph_reset(app->cpu_graph);
}


static void
cb_network_in_bits_changed (GSettings *settings, const gchar *key, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);

    app->config.network_in_bits = g_settings_get_boolean(settings, key);
    // force scale to be redrawn
    app->net_graph->clear_background();
}

static void
cb_timeouts_changed (GSettings *settings, const gchar *key, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);

    if (strcmp (key, GSM_SETTING_PROCESS_UPDATE_INTERVAL) == 0) {
        app->config.update_interval = g_settings_get_int (settings, key);

        app->smooth_refresh->reset();

        proctable_reset_timeout (app);
    } else if (strcmp (key, GSM_SETTING_GRAPH_UPDATE_INTERVAL) == 0) {
        app->config.graph_update_interval = g_settings_get_int (settings, key);
        load_graph_change_speed(app->cpu_graph,
                                app->config.graph_update_interval);
        load_graph_change_speed(app->mem_graph,
                                app->config.graph_update_interval);
        load_graph_change_speed(app->net_graph,
                                app->config.graph_update_interval);
    } else if (strcmp (key, GSM_SETTING_DISKS_UPDATE_INTERVAL) == 0) {
        app->config.disks_update_interval = g_settings_get_int (settings, key);
        disks_reset_timeout (app);
    }
}

static void
apply_cpu_color_settings(GSettings *settings, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);

    GVariant *cpu_colors_var = g_settings_get_value (settings, GSM_SETTING_CPU_COLORS);
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
        g_settings_set_value(settings, GSM_SETTING_CPU_COLORS, full);
    } else {
        g_variant_unref(full);
    }

    g_variant_unref(cpu_colors_var);
}

static void
cb_color_changed (GSettings *settings, const gchar *key, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);

    if (strcmp (key, GSM_SETTING_CPU_COLORS) == 0) {
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
    if (strcmp (key, GSM_SETTING_MEM_COLOR) == 0) {
        gdk_rgba_parse (&app->config.mem_color, color);
        app->mem_graph->colors.at(0) = app->config.mem_color;
    } else if (strcmp (key, GSM_SETTING_SWAP_COLOR) == 0) {
        gdk_rgba_parse (&app->config.swap_color, color);
        app->mem_graph->colors.at(1) = app->config.swap_color;
    } else if (strcmp (key, GSM_SETTING_NET_IN_COLOR) == 0) {
        gdk_rgba_parse (&app->config.net_in_color, color);
        app->net_graph->colors.at(0) = app->config.net_in_color;
    } else if (strcmp (key, GSM_SETTING_NET_OUT_COLOR) == 0) {
        gdk_rgba_parse (&app->config.net_out_color, color);
        app->net_graph->colors.at(1) = app->config.net_out_color;
    }
    g_free (color);
}

void
GsmApplication::load_settings()
{
    gchar *color;
    gint i;
    glibtop_cpu cpu;

    settings = g_settings_new (GSM_GSETTINGS_SCHEMA);

    config.solaris_mode = g_settings_get_boolean (settings, GSM_SETTING_SOLARIS_MODE);
    g_signal_connect (settings, "changed::" GSM_SETTING_SOLARIS_MODE,
                      G_CALLBACK (cb_solaris_mode_changed), this);

    config.draw_stacked = g_settings_get_boolean (settings, GSM_SETTING_DRAW_STACKED);
    g_signal_connect (settings, "changed::" GSM_SETTING_DRAW_STACKED,
                      G_CALLBACK (cb_draw_stacked_changed), this);

    config.network_in_bits = g_settings_get_boolean (settings, GSM_SETTING_NETWORK_IN_BITS);
    g_signal_connect (settings, "changed::" GSM_SETTING_NETWORK_IN_BITS,
                      G_CALLBACK (cb_network_in_bits_changed), this);

    config.update_interval = g_settings_get_int (settings, GSM_SETTING_PROCESS_UPDATE_INTERVAL);
    g_signal_connect (settings, "changed::" GSM_SETTING_PROCESS_UPDATE_INTERVAL,
                      G_CALLBACK (cb_timeouts_changed), this);
    config.graph_update_interval = g_settings_get_int (settings, GSM_SETTING_GRAPH_UPDATE_INTERVAL);
    g_signal_connect (settings, "changed::" GSM_SETTING_GRAPH_UPDATE_INTERVAL,
                      G_CALLBACK (cb_timeouts_changed), this);
    config.disks_update_interval = g_settings_get_int (settings, GSM_SETTING_DISKS_UPDATE_INTERVAL);
    g_signal_connect (settings, "changed::" GSM_SETTING_DISKS_UPDATE_INTERVAL,
                      G_CALLBACK (cb_timeouts_changed), this);

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

    apply_cpu_color_settings (settings, this);
    g_signal_connect (settings, "changed::" GSM_SETTING_CPU_COLORS,
                      G_CALLBACK (cb_color_changed), this);

    color = g_settings_get_string (settings, GSM_SETTING_MEM_COLOR);
    if (!color)
        color = g_strdup ("#000000ff0082");
    g_signal_connect (settings, "changed::" GSM_SETTING_MEM_COLOR,
                      G_CALLBACK (cb_color_changed), this);
    gdk_rgba_parse (&config.mem_color, color);
    g_free (color);

    color = g_settings_get_string (settings, GSM_SETTING_SWAP_COLOR);
    if (!color)
        color = g_strdup ("#00b6000000ff");
    g_signal_connect (settings, "changed::" GSM_SETTING_SWAP_COLOR,
                      G_CALLBACK (cb_color_changed), this);
    gdk_rgba_parse (&config.swap_color, color);
    g_free (color);

    color = g_settings_get_string (settings, GSM_SETTING_NET_IN_COLOR);
    if (!color)
        color = g_strdup ("#000000f200f2");
    g_signal_connect (settings, "changed::" GSM_SETTING_NET_IN_COLOR,
                      G_CALLBACK (cb_color_changed), this);
    gdk_rgba_parse (&config.net_in_color, color);
    g_free (color);

    color = g_settings_get_string (settings, GSM_SETTING_NET_OUT_COLOR);
    if (!color)
        color = g_strdup ("#00f2000000c1");
    g_signal_connect (settings, "changed::" GSM_SETTING_NET_OUT_COLOR,
                      G_CALLBACK (cb_color_changed), this);
    gdk_rgba_parse (&config.net_out_color, color);
    g_free (color);
}

GsmApplication::GsmApplication() : Gtk::Application("org.gnome.SystemMonitor", Gio::APPLICATION_HANDLES_COMMAND_LINE)
{
    Glib::set_application_name(_("System Monitor"));
}

Glib::RefPtr<GsmApplication> GsmApplication::get ()
{
    static Glib::RefPtr<GsmApplication> singleton;

    if (!singleton) {
        singleton = Glib::RefPtr<GsmApplication>(new GsmApplication());
    }
    return singleton;
}

void GsmApplication::on_activate()
{
    gtk_window_present (GTK_WINDOW (main_window));
}

void
GsmApplication::save_config ()
{
    int width, height, xpos, ypos;
    gboolean maximized;

    width  = gdk_window_get_width (gtk_widget_get_window (main_window));
    height = gdk_window_get_height (gtk_widget_get_window (main_window));
    gtk_window_get_position (GTK_WINDOW (main_window), &xpos, &ypos);

    maximized = gdk_window_get_state (gtk_widget_get_window (main_window)) & GDK_WINDOW_STATE_MAXIMIZED;

    g_settings_set (settings, GSM_SETTING_WINDOW_STATE, "(iiii)",
                    width, height, xpos, ypos);

    g_settings_set_boolean (settings, GSM_SETTING_MAXIMIZED, maximized);
}

int GsmApplication::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line)
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

    if (option_group.print_version) {
        g_print("%s %s\n", _("GNOME System Monitor"), VERSION);
        exit (EXIT_SUCCESS);
    }

    if (option_group.show_processes_tab)
        g_settings_set_string (settings, GSM_SETTING_CURRENT_TAB, "processes");
    else if (option_group.show_resources_tab)
        g_settings_set_string (settings, GSM_SETTING_CURRENT_TAB, "resources");
    else if (option_group.show_file_systems_tab)
        g_settings_set_string (settings, GSM_SETTING_CURRENT_TAB, "disks");
    else if (option_group.print_version)

    on_activate ();

    return 0;
}

void
GsmApplication::on_help_activate(const Glib::VariantBase&)
{
    GError* error = 0;
    if (!g_app_info_launch_default_for_uri("help:gnome-system-monitor", NULL, &error)) {
        g_warning("Could not display help : %s", error->message);
        g_error_free(error);
    }
}

void
GsmApplication::on_lsof_activate(const Glib::VariantBase&)
{
    procman_lsof(this);
}

void
GsmApplication::on_preferences_activate(const Glib::VariantBase&)
{
    create_preferences_dialog (this);
}

void
GsmApplication::on_quit_activate(const Glib::VariantBase&)
{
    shutdown ();
}

void
GsmApplication::shutdown()
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

void GsmApplication::on_startup()
{
    Gtk::Application::on_startup();

    Glib::RefPtr<Gio::SimpleAction> action;

    action = Gio::SimpleAction::create("quit");
    action->signal_activate().connect(sigc::mem_fun(*this, &GsmApplication::on_quit_activate));
    add_action(action);

    action = Gio::SimpleAction::create("help");
    action->signal_activate().connect(sigc::mem_fun(*this, &GsmApplication::on_help_activate));
    add_action(action);

    action = Gio::SimpleAction::create("lsof");
    action->signal_activate().connect(sigc::mem_fun(*this, &GsmApplication::on_lsof_activate));
    add_action(action);

    action = Gio::SimpleAction::create("preferences");
    action->signal_activate().connect(sigc::mem_fun(*this, &GsmApplication::on_preferences_activate));
    add_action(action);

    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/org/gnome/gnome-system-monitor/data/menus.ui");

    Glib::RefPtr<Gio::Menu> menu = Glib::RefPtr<Gio::Menu>::cast_static(builder->get_object ("app-menu"));
    set_app_menu (menu);

    add_accelerator("<Primary>d", "win.show-dependencies", NULL);
    add_accelerator("<Primary>s", "win.send-signal-stop", g_variant_new_int32(SIGSTOP));
    add_accelerator("<Primary>c", "win.send-signal-cont", g_variant_new_int32(SIGCONT));
    add_accelerator("<Primary>e", "win.send-signal-end", g_variant_new_int32(SIGTERM));
    add_accelerator("<Primary>k", "win.send-signal-kill", g_variant_new_int32 (SIGKILL));
    add_accelerator("<Primary>m", "win.memory-maps", NULL);
    add_accelerator("<Primary>o", "win.open-files", NULL);
    add_accelerator("<Alt>Return", "win.process-properties", NULL);
    add_accelerator("<Primary>f", "win.search", NULL);

    Gtk::Window::set_default_icon_name ("utilities-system-monitor");

    glibtop_init ();

    load_settings ();

    pretty_table = new PrettyTable();
    smooth_refresh = new SmoothRefresh(settings);

    create_main_window (this);

    add_accelerator ("<Alt>1", "win.show-page", g_variant_new_string ("processes"));
    add_accelerator ("<Alt>2", "win.show-page", g_variant_new_string ("resources"));
    add_accelerator ("<Alt>3", "win.show-page", g_variant_new_string ("disks"));
    add_accelerator ("<Primary>r", "win.refresh", NULL);

    gtk_widget_show (main_window);
}

