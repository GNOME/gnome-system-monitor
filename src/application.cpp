/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <config.h>

#include <glib/gi18n.h>
#include <glibtop.h>
#include <glibtop/close.h>
#include <glibtop/cpu.h>
#include <glibtop/sysinfo.h>
#include <handy.h>
#include <signal.h>
#include <stdlib.h>

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
cb_solaris_mode_changed (Gio::Settings& settings,
                         Glib::ustring  key,
                         GsmApplication*app)
{
  app->config.solaris_mode = settings.get_boolean (key);
  app->cpu_graph->clear_background ();
  if (app->timeout)
    proctable_update (app);
}

static void
cb_process_memory_in_iec_changed (Gio::Settings& settings,
                                  Glib::ustring  key,
                                  GsmApplication*app)
{
  app->config.process_memory_in_iec = settings.get_boolean (key);
  app->cpu_graph->clear_background ();
  if (app->timeout)
    proctable_update (app);
}

static void
cb_logarithmic_scale_changed (Gio::Settings& settings,
                              Glib::ustring  key,
                              GsmApplication*app)
{
  app->config.logarithmic_scale = settings.get_boolean (key);
  app->mem_graph->clear_background ();
  load_graph_reset (app->mem_graph);
}

static void
cb_draw_stacked_changed (Gio::Settings& settings,
                         Glib::ustring  key,
                         GsmApplication*app)
{
  app->config.draw_stacked = settings.get_boolean (key);
  app->cpu_graph->clear_background ();
  load_graph_reset (app->cpu_graph);
}

static void
cb_draw_smooth_changed (Gio::Settings& settings,
                        Glib::ustring  key,
                        GsmApplication*app)
{
  app->config.draw_smooth = settings.get_boolean (key);
  app->cpu_graph->clear_background ();
  app->mem_graph->clear_background ();
  app->net_graph->clear_background ();
}

static void
cb_resources_memory_in_iec_changed (Gio::Settings& settings,
                                    Glib::ustring  key,
                                    GsmApplication*app)
{
  app->config.resources_memory_in_iec = settings.get_boolean (key);
  app->cpu_graph->clear_background ();
  if (app->timeout)
    proctable_update (app);
}

static void
cb_network_in_bits_changed (Gio::Settings& settings,
                            Glib::ustring  key,
                            GsmApplication*app)
{
  app->config.network_in_bits = settings.get_boolean (key);
  if (app->config.network_total_unit == FALSE)
    app->config.network_total_in_bits = app->config.network_in_bits;
  // force scale to be redrawn
  app->net_graph->clear_background ();
}

static void
cb_network_total_in_unit_changed (Gio::Settings& settings,
                                  Glib::ustring  key,
                                  GsmApplication*app)
{
  app->config.network_total_unit = settings.get_boolean (key);
  if (app->config.network_total_unit == FALSE)
    app->config.network_total_in_bits = app->config.network_in_bits;
  else
    app->config.network_total_in_bits = app->settings->get_boolean (GSM_SETTING_NETWORK_TOTAL_IN_BITS);
  // force scale to be redrawn
  app->net_graph->clear_background ();
}

static void
cb_network_total_in_bits_changed (Gio::Settings& settings,
                                  Glib::ustring  key,
                                  GsmApplication*app)
{
  app->config.network_total_in_bits = settings.get_boolean (key);
  // force scale to be redrawn
  app->net_graph->clear_background ();
}

static void
cb_timeouts_changed (Gio::Settings& settings,
                     Glib::ustring  key,
                     GsmApplication*app)
{
  if (key == GSM_SETTING_PROCESS_UPDATE_INTERVAL)
    {
      app->config.update_interval = settings.get_int (key);

      app->smooth_refresh->reset ();
      if (app->timeout)
        proctable_reset_timeout (app);
    }
  else if (key == GSM_SETTING_GRAPH_UPDATE_INTERVAL)
    {
      app->config.graph_update_interval = settings.get_int (key);
      load_graph_change_speed (app->cpu_graph,
                               app->config.graph_update_interval);
      load_graph_change_speed (app->mem_graph,
                               app->config.graph_update_interval);
      load_graph_change_speed (app->net_graph,
                               app->config.graph_update_interval);
    }
  else if (key == GSM_SETTING_DISKS_UPDATE_INTERVAL)
    {
      app->config.disks_update_interval = settings.get_int (key);
      disks_reset_timeout (app);
    }
}

static void
cb_data_points_changed (Gio::Settings& settings,
                        Glib::ustring  key,
                        GsmApplication*app)
{
  app->config.graph_data_points = settings.get_int (key);
  unsigned points = app->config.graph_data_points + 2;

  load_graph_change_num_points (app->cpu_graph, points);
  load_graph_change_num_points (app->mem_graph, points);
  load_graph_change_num_points (app->net_graph, points);
}

static void
apply_cpu_color_settings (Gio::Settings& settings,
                          GsmApplication*app)
{
  GVariant *cpu_colors_var = g_settings_get_value (settings.gobj (), GSM_SETTING_CPU_COLORS);
  gsize n = g_variant_n_children (cpu_colors_var);

  gchar *color;

  // Create builder to add the new colors if user has more than the number of cores with defaults.
  GVariantBuilder builder;
  GVariant*child;
  GVariant*full;

  g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);

  std::vector<std::string> random_colors = procman::generate_colors (app->config.num_cpus);

  for (guint i = 0; i < static_cast<guint>(app->config.num_cpus); i++)
    {
      if (i < n)
        {
          child = g_variant_get_child_value (cpu_colors_var, i);
          g_variant_get_child (child, 1, "s", &color);
          g_variant_builder_add_value (&builder, child);
          g_variant_unref (child);
        }
      else
        {
          color = g_strdup (random_colors[i].c_str ());
          g_variant_builder_add (&builder, "(us)", i, color);
        }
      gdk_rgba_parse (&app->config.cpu_color[i], color);
      g_free (color);
    }
  full = g_variant_builder_end (&builder);
  // if the user has more cores than colors stored in the gsettings, store the newly built gvariant in gsettings
  if (n < static_cast<guint>(app->config.num_cpus))
    g_settings_set_value (settings.gobj (), GSM_SETTING_CPU_COLORS, full);
  else
    g_variant_unref (full);

  g_variant_unref (cpu_colors_var);
}

static void
cb_color_changed (Gio::Settings& settings,
                  Glib::ustring  key,
                  GsmApplication*app)
{
  if (key == GSM_SETTING_CPU_COLORS)
    {
      apply_cpu_color_settings (settings, app);
      for (int i = 0; i < app->config.num_cpus; i++)
        if (!gdk_rgba_equal (&app->cpu_graph->colors[i], &app->config.cpu_color[i]))
          {
            app->cpu_graph->colors[i] = app->config.cpu_color[i];
            break;
          }
      return;
    }

  auto color = settings.get_string (key);

  if (key == GSM_SETTING_MEM_COLOR)
    {
      gdk_rgba_parse (&app->config.mem_color, color.c_str ());
      app->mem_graph->colors.at (0) = app->config.mem_color;
    }
  else if (key == GSM_SETTING_SWAP_COLOR)
    {
      gdk_rgba_parse (&app->config.swap_color, color.c_str ());
      app->mem_graph->colors.at (1) = app->config.swap_color;
    }
  else if (key == GSM_SETTING_NET_IN_COLOR)
    {
      gdk_rgba_parse (&app->config.net_in_color, color.c_str ());
      app->net_graph->colors.at (0) = app->config.net_in_color;
    }
  else if (key == GSM_SETTING_NET_OUT_COLOR)
    {
      gdk_rgba_parse (&app->config.net_out_color, color.c_str ());
      app->net_graph->colors.at (1) = app->config.net_out_color;
    }
}

void
GsmApplication::load_settings ()
{
  glibtop_cpu cpu;

  this->settings = Gio::Settings::create (GSM_GSETTINGS_SCHEMA);

  config.solaris_mode = this->settings->get_boolean (GSM_SETTING_SOLARIS_MODE);
  this->settings->signal_changed (GSM_SETTING_SOLARIS_MODE).connect ([this](const Glib::ustring&key) {
    cb_solaris_mode_changed (*this->settings.operator-> (), key, this);
  });

  config.process_memory_in_iec = this->settings->get_boolean (GSM_SETTING_PROCESS_MEMORY_IN_IEC);
  this->settings->signal_changed (GSM_SETTING_PROCESS_MEMORY_IN_IEC).connect ([this](const Glib::ustring&key) {
    cb_process_memory_in_iec_changed (*this->settings.operator-> (), key, this);
  });

  config.logarithmic_scale = this->settings->get_boolean (GSM_SETTING_LOGARITHMIC_SCALE);
  this->settings->signal_changed (GSM_SETTING_LOGARITHMIC_SCALE).connect ([this](const Glib::ustring&key) {
    cb_logarithmic_scale_changed (*this->settings.operator-> (), key, this);
  });

  config.draw_stacked = this->settings->get_boolean (GSM_SETTING_DRAW_STACKED);
  this->settings->signal_changed (GSM_SETTING_DRAW_STACKED).connect ([this](const Glib::ustring&key) {
    cb_draw_stacked_changed (*this->settings.operator-> (), key, this);
  });

  config.draw_smooth = this->settings->get_boolean (GSM_SETTING_DRAW_SMOOTH);
  this->settings->signal_changed (GSM_SETTING_DRAW_SMOOTH).connect ([this](const Glib::ustring&key) {
    cb_draw_smooth_changed (*this->settings.operator-> (), key, this);
  });

  config.resources_memory_in_iec = this->settings->get_boolean (GSM_SETTING_RESOURCES_MEMORY_IN_IEC);
  this->settings->signal_changed (GSM_SETTING_RESOURCES_MEMORY_IN_IEC).connect ([this](const Glib::ustring&key) {
    cb_resources_memory_in_iec_changed (*this->settings.operator-> (), key, this);
  });

  config.network_in_bits = this->settings->get_boolean (GSM_SETTING_NETWORK_IN_BITS);
  this->settings->signal_changed (GSM_SETTING_NETWORK_IN_BITS).connect ([this](const Glib::ustring&key) {
    cb_network_in_bits_changed (*this->settings.operator-> (), key, this);
  });

  config.network_total_unit = this->settings->get_boolean (GSM_SETTING_NETWORK_TOTAL_UNIT);
  this->settings->signal_changed (GSM_SETTING_NETWORK_TOTAL_UNIT).connect ([this](const Glib::ustring&key) {
    cb_network_total_in_unit_changed (*this->settings.operator-> (), key, this);
  });

  if (config.network_total_unit == FALSE)
    {
      config.network_total_in_bits = config.network_in_bits;
      this->settings->set_boolean (GSM_SETTING_NETWORK_TOTAL_IN_BITS, config.network_in_bits);
    }
  else
    {
      config.network_total_in_bits = this->settings->get_boolean (GSM_SETTING_NETWORK_TOTAL_IN_BITS);
    }
  this->settings->signal_changed (GSM_SETTING_NETWORK_TOTAL_IN_BITS).connect ([this](const Glib::ustring&key) {
    cb_network_total_in_bits_changed (*this->settings.operator-> (), key, this);
  });

  auto cbtc = [this](const Glib::ustring&key) {
                cb_timeouts_changed (*this->settings.operator-> (), key, this);
              };

  config.update_interval = this->settings->get_int (GSM_SETTING_PROCESS_UPDATE_INTERVAL);
  this->settings->signal_changed (GSM_SETTING_PROCESS_UPDATE_INTERVAL).connect (cbtc);
  config.graph_update_interval = this->settings->get_int (GSM_SETTING_GRAPH_UPDATE_INTERVAL);
  this->settings->signal_changed (GSM_SETTING_GRAPH_UPDATE_INTERVAL).connect (cbtc);
  config.disks_update_interval = this->settings->get_int (GSM_SETTING_DISKS_UPDATE_INTERVAL);
  this->settings->signal_changed (GSM_SETTING_DISKS_UPDATE_INTERVAL).connect (cbtc);

  config.graph_data_points = this->settings->get_int (GSM_SETTING_GRAPH_DATA_POINTS);
  this->settings->signal_changed (GSM_SETTING_GRAPH_DATA_POINTS).connect ([this](const Glib::ustring&key) {
    cb_data_points_changed (*this->settings.operator-> (), key, this);
  });

  glibtop_get_cpu (&cpu);
  frequency = cpu.frequency;

  config.num_cpus = glibtop_get_sysinfo ()->ncpu;  // or server->ncpu + 1

  apply_cpu_color_settings (*this->settings.operator-> (), this);

  auto mem_color = this->settings->get_string (GSM_SETTING_MEM_COLOR);

  gdk_rgba_parse (&config.mem_color, mem_color.empty () ? "#000000ff0082" : mem_color.c_str ());

  auto swap_color = this->settings->get_string (GSM_SETTING_SWAP_COLOR);

  gdk_rgba_parse (&config.swap_color, swap_color.empty () ? "#00b6000000ff" : swap_color.c_str ());

  auto net_in_color = this->settings->get_string (GSM_SETTING_NET_IN_COLOR);

  gdk_rgba_parse (&config.net_in_color, net_in_color.empty () ? "#000000f200f2" : net_in_color.c_str ());

  auto net_out_color = this->settings->get_string (GSM_SETTING_NET_OUT_COLOR);

  gdk_rgba_parse (&config.net_out_color, net_out_color.empty () ? "#00f2000000c1" : net_out_color.c_str ());

  auto cbcc = [this](const Glib::ustring&key) {
                cb_color_changed (*this->settings.operator-> (), key, this);
              };

  for (auto k : { GSM_SETTING_CPU_COLORS, GSM_SETTING_MEM_COLOR, GSM_SETTING_SWAP_COLOR, GSM_SETTING_NET_IN_COLOR, GSM_SETTING_NET_OUT_COLOR })
    this->settings->signal_changed (k).connect (cbcc);
}


GsmApplication::GsmApplication()
  : Gtk::Application ("org.gnome.SystemMonitor", Gio::APPLICATION_HANDLES_COMMAND_LINE),
  tree (NULL),
  proc_actionbar_revealer (NULL),
  popup_menu (NULL),
  disk_list (NULL),
  stack (NULL),
  refresh_button (NULL),
  process_menu_button (NULL),
  window_menu_button (NULL),
  end_process_button (NULL),
  search_button (NULL),
  search_entry (NULL),
  search_bar (NULL),
  config (),
  cpu_graph (NULL),
  mem_graph (NULL),
  net_graph (NULL),
  selection (NULL),
  timeout (0U),
  disk_timeout (0U),
  top_of_tree (NULL),
  last_vscroll_max (0.0),
  last_vscroll_value (0.0),
  pretty_table (NULL),
  settings (NULL),
  main_window (NULL),
  frequency (0U),
  smooth_refresh (NULL),
  cpu_total_time (0ULL),
  cpu_total_time_last (0ULL)
{
  Glib::set_application_name (_("System Monitor"));
}

Glib::RefPtr<GsmApplication>
GsmApplication::get ()
{
  static Glib::RefPtr<GsmApplication> singleton;

  if (!singleton)
    singleton = Glib::RefPtr<GsmApplication>(new GsmApplication ());
  return singleton;
}

void
GsmApplication::on_activate ()
{
  gtk_window_present (GTK_WINDOW (main_window));
}

void
GsmApplication::save_config ()
{
  int width, height, xpos, ypos;
  gboolean maximized;

  gtk_window_get_size (GTK_WINDOW (main_window), &width, &height);
  gtk_window_get_position (GTK_WINDOW (main_window), &xpos, &ypos);

  maximized = gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (main_window))) & GDK_WINDOW_STATE_MAXIMIZED;

  g_settings_set (settings->gobj (), GSM_SETTING_WINDOW_STATE, "(iiii)",
                  width, height, xpos, ypos);

  settings->set_boolean (GSM_SETTING_MAXIMIZED, maximized);
}

int
GsmApplication::on_command_line (const Glib::RefPtr<Gio::ApplicationCommandLine>&command_line)
{
  int argc = 0;
  char**argv = command_line->get_arguments (argc);

  Glib::OptionContext context;

  context.set_summary (_("A simple process and system monitor."));
  context.set_ignore_unknown_options (true);

  Glib::OptionGroup gtkgroup (gtk_get_option_group (true));
  procman::OptionGroup option_group;

  context.set_main_group (option_group);
  context.add_group (gtkgroup);

  try {
      context.parse (argc, argv);
    } catch (const Glib::Error&ex) {
      g_error ("Arguments parse error : %s", ex.what ().c_str ());
    }

  g_strfreev (argv);

  if (option_group.print_version)
    {
      g_print ("%s %s\n", _("GNOME System Monitor"), VERSION);
      exit (EXIT_SUCCESS);
    }

  if (option_group.show_processes_tab)
    this->settings->set_string (GSM_SETTING_CURRENT_TAB, "processes");
  else if (option_group.show_resources_tab)
    this->settings->set_string (GSM_SETTING_CURRENT_TAB, "resources");
  else if (option_group.show_file_systems_tab)
    this->settings->set_string (GSM_SETTING_CURRENT_TAB, "disks");
  else if (option_group.print_version)

    on_activate ();

  return 0;
}

void
GsmApplication::on_help_activate (const Glib::VariantBase&)
{
  GError*error = 0;

  if (!g_app_info_launch_default_for_uri ("help:gnome-system-monitor", NULL, &error))
    {
      g_warning ("Could not display help : %s", error->message);
      g_error_free (error);
    }
}

void
GsmApplication::on_lsof_activate (const Glib::VariantBase&)
{
  procman_lsof (this);
}

void
GsmApplication::on_preferences_activate (const Glib::VariantBase&)
{
  create_preferences_dialog (this);
}

void
GsmApplication::on_quit_activate (const Glib::VariantBase&)
{
  shutdown ();
}

void
GsmApplication::shutdown ()
{
  save_config ();

  if (timeout)
    g_source_remove (timeout);
  if (disk_timeout)
    g_source_remove (disk_timeout);

  proctable_free_table (this);
  delete smooth_refresh;
  delete pretty_table;

  glibtop_close ();

  quit ();
}

void
GsmApplication::on_startup ()
{
  HdyStyleManager *style_manager;

  Gtk::Application::on_startup ();

  hdy_init ();

  load_resources ();

  style_manager = hdy_style_manager_get_default ();
  hdy_style_manager_set_color_scheme (style_manager, HDY_COLOR_SCHEME_PREFER_LIGHT);

  Glib::RefPtr<Gio::SimpleAction> action;

  action = Gio::SimpleAction::create ("quit");
  action->signal_activate ().connect (sigc::mem_fun (*this, &GsmApplication::on_quit_activate));
  add_action (action);

  action = Gio::SimpleAction::create ("help");
  action->signal_activate ().connect (sigc::mem_fun (*this, &GsmApplication::on_help_activate));
  add_action (action);

  action = Gio::SimpleAction::create ("lsof");
  action->signal_activate ().connect (sigc::mem_fun (*this, &GsmApplication::on_lsof_activate));
  add_action (action);

  action = Gio::SimpleAction::create ("preferences");
  action->signal_activate ().connect (sigc::mem_fun (*this, &GsmApplication::on_preferences_activate));
  add_action (action);

  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource ("/org/gnome/gnome-system-monitor/data/menus.ui");

  Glib::RefPtr<Gio::Menu> menu = Glib::RefPtr<Gio::Menu>::cast_static (builder->get_object ("app-menu"));

  set_app_menu (menu);

  add_accelerator ("<Primary>d", "win.show-dependencies", NULL);
  add_accelerator ("<Primary>q", "app.quit", NULL);
  add_accelerator ("<Primary>s", "win.send-signal-stop", g_variant_new_int32 (SIGSTOP));
  add_accelerator ("<Primary>c", "win.send-signal-cont", g_variant_new_int32 (SIGCONT));
  add_accelerator ("<Primary>t", "win.send-signal-term", g_variant_new_int32 (SIGTERM));
  add_accelerator ("<Primary>k", "win.send-signal-kill", g_variant_new_int32 (SIGKILL));
  add_accelerator ("<Alt>s", "win.set-affinity", NULL);
  add_accelerator ("<Primary>m", "win.memory-maps", NULL);
  add_accelerator ("<Primary>o", "win.open-files", NULL);
  add_accelerator ("<Alt>Return", "win.process-properties", NULL);
  add_accelerator ("<Primary>f", "win.search", g_variant_new_boolean (TRUE));
  add_accelerator ("F1", "app.help", NULL);
  add_accelerator ("<Primary>question", "win.show-help-overlay", NULL);
  add_accelerator ("<Primary>comma", "app.preferences", NULL);

  Gtk::Window::set_default_icon_name ("org.gnome.SystemMonitor");

  glibtop_init ();

  load_settings ();

  pretty_table = new PrettyTable ();
  smooth_refresh = new SmoothRefresh (settings);

  create_main_window (this);

  add_accelerator ("<Alt>1", "win.show-page", g_variant_new_string ("processes"));
  add_accelerator ("<Alt>2", "win.show-page", g_variant_new_string ("resources"));
  add_accelerator ("<Alt>3", "win.show-page", g_variant_new_string ("disks"));
  add_accelerator ("<Primary>r", "win.refresh", NULL);

  gtk_widget_show (GTK_WIDGET (main_window));
}


void
GsmApplication::load_resources ()
{
  auto res = Gio::Resource::create_from_file (GSM_RESOURCE_FILE);

  res->register_global ();
}
