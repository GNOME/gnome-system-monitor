#include <config.h>

#include <glib/gi18n.h>
#include <glibtop.h>
#include <glibtop/close.h>
#include <glibtop/cpu.h>
#include <glibtop/sysinfo.h>
#include <adwaita.h>
#include <signal.h>
#include <stdlib.h>
#include <glib.h>
#include <sigc++/functors/functors.h>

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

extern "C" {
#include "gsm-resources.h"
}

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
  app->disk_graph->clear_background ();
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
      load_graph_change_speed (app->disk_graph,
                               app->config.graph_update_interval);
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
  load_graph_change_num_points (app->disk_graph, points);
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
  else if (key == GSM_SETTING_DISK_READ_COLOR)
    {
      gdk_rgba_parse (&app->config.disk_read_color, color.c_str ());
      app->disk_graph->colors.at (0) = app->config.disk_read_color;
    }
  else if (key == GSM_SETTING_DISK_WRITE_COLOR)
    {
      gdk_rgba_parse (&app->config.disk_write_color, color.c_str ());
      app->disk_graph->colors.at (1) = app->config.disk_write_color;
    }
}

void
GsmApplication::load_settings ()
{
  glibtop_cpu cpu;

  this->settings = Gio::Settings::create (GSM_GSETTINGS_SCHEMA);

  config.current_tab = this->settings->get_string (GSM_SETTING_CURRENT_TAB);

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

  config.network_total_in_bits = this->settings->get_boolean (GSM_SETTING_NETWORK_TOTAL_IN_BITS);
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

  auto disk_read_color = this->settings->get_string (GSM_SETTING_DISK_READ_COLOR);

  gdk_rgba_parse (&config.disk_read_color, disk_read_color.empty () ? "#000000f200f2" : disk_read_color.c_str ());

  auto disk_write_color = this->settings->get_string (GSM_SETTING_DISK_WRITE_COLOR);

  gdk_rgba_parse (&config.disk_write_color, disk_write_color.empty () ? "#00f2000000c1" : disk_write_color.c_str ());


  auto cbcc = [this](const Glib::ustring&key) {
                cb_color_changed (*this->settings.operator-> (), key, this);
              };

  for (auto k : { GSM_SETTING_CPU_COLORS, GSM_SETTING_MEM_COLOR, GSM_SETTING_SWAP_COLOR, GSM_SETTING_NET_IN_COLOR, GSM_SETTING_NET_OUT_COLOR, GSM_SETTING_DISK_READ_COLOR, GSM_SETTING_DISK_WRITE_COLOR })
    this->settings->signal_changed (k).connect (cbcc);
}

void GsmApplication::load_command_line_options ()
{
  for ( auto entry : option_group.entries() ) 
  {
    this->add_main_option_entry (
      OptionType::BOOL, 
      entry.get_long_name(),
      entry.get_short_name(),
      entry.get_description(),
      entry.get_arg_description(),
      entry.get_flags()
    );
  }
}


GsmApplication::GsmApplication()
  : Gtk::Application (APP_ID, Gio::Application::Flags::HANDLES_COMMAND_LINE),
  settings (NULL),
  main_window (NULL),
  stack (NULL),
  app_menu_button (NULL),

  config (),
  pretty_table (NULL),
  tree (NULL),
  proc_actionbar_revealer (NULL),
  proc_popover_menu (NULL),
  refresh_button (NULL),
  end_process_button (NULL),
  search_button (NULL),
  search_entry (NULL),
  search_bar (NULL),
  top_of_tree (NULL),

  last_vscroll_max (0.0),
  last_vscroll_value (0.0),
  timeout (0U),
  cpu_total_time (0ULL),
  cpu_total_time_last (0ULL),
  frequency (0U),

  cpu_graph (NULL),
  mem_graph (NULL),
  net_graph (NULL),
  disk_graph (NULL),

  disk_list (NULL),

  selection (NULL),

  smooth_refresh (NULL)
{
  Glib::set_application_name (_("System Monitor"));
  this->set_option_context_summary (_("A simple process and system monitor."));
  this->signal_handle_local_options().connect(sigc::mem_fun(*this, &GsmApplication::handle_local_options), false);
  load_command_line_options ();
}

GsmApplication&
GsmApplication::get ()
{
  static GsmApplication singleton;
  return singleton;
}

void
GsmApplication::on_activate ()
{
  if (main_window == NULL)
      create_main_window (this);

  gtk_window_present (GTK_WINDOW (main_window));
}

int
GsmApplication::handle_local_options (const Glib::RefPtr<Glib::VariantDict> &content)
{
  option_group.load ( content );

  if (option_group.print_version)
  {
    g_print ("%s %s\n", _("GNOME System Monitor"), VERSION);
    return 0;
  }

  return -1;
}

int
GsmApplication::on_command_line (const Glib::RefPtr<Gio::ApplicationCommandLine>&command_line)
{
  option_group.load ( command_line->get_options_dict () );

  const char*tab = NULL;
  if (option_group.show_processes_tab)
    tab = "processes";
  else if (option_group.show_resources_tab)
    tab = "resources";
  else if (option_group.show_file_systems_tab)
    tab = "disks";
  if (tab)
    adw_view_stack_set_visible_child_name (this->stack, tab);

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
  GsmLsof *window = gsm_lsof_new (GTK_WINDOW (this->main_window));

  gtk_window_present (GTK_WINDOW (window));
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
  if (timeout)
    g_source_remove (timeout);

  guint disks_timeout = gsm_disks_view_get_timeout (disk_list);

  if (disks_timeout)
    g_source_remove (disks_timeout);

  proctable_free_table (this);
  delete smooth_refresh;
  delete pretty_table;

  glibtop_close ();

  quit ();
}

void
GsmApplication::on_startup ()
{
  AdwStyleManager *style_manager;
  GtkCssProvider *css_provider;

  g_resources_register (gsm_get_resource ());

  Gtk::Application::on_startup ();

  glibtop_init ();

  style_manager = adw_style_manager_get_default ();
  adw_style_manager_set_color_scheme (style_manager, ADW_COLOR_SCHEME_PREFER_LIGHT);

  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (css_provider, "/org/gnome/gnome-system-monitor/style.css");
  gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                              GTK_STYLE_PROVIDER (css_provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  Gtk::Window::set_default_icon_name (APP_ID);

  using SignalFunction = void (GsmApplication::*)(const Glib::VariantBase&);
  auto make_action = [&](const Glib::ustring& name, const SignalFunction function) {
    auto action = Gio::SimpleAction::create (name);
    action->signal_activate ().connect (sigc::mem_fun (*this, function));
    return action;
  };

  add_action (make_action("quit", &GsmApplication::on_quit_activate));
  add_action (make_action("help", &GsmApplication::on_help_activate));
  add_action (make_action("lsof", &GsmApplication::on_lsof_activate));
  add_action (make_action("preferences", &GsmApplication::on_preferences_activate));

  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource ("/org/gnome/gnome-system-monitor/data/menus.ui");

  set_accel_for_action ("win.show-dependencies", "<Primary>d");
  set_accel_for_action ("app.quit", "<Primary>q");
  set_accel_for_action ("window.close", "<Primary>w");
  set_accel_for_action ("win.send-signal-stop", "<Primary>s");
  set_accel_for_action ("win.send-signal-cont", "<Primary>c");
  set_accel_for_action ("win.send-signal-term", "<Primary>t");
  set_accel_for_action ("win.send-signal-kill", "<Primary>k");
  set_accel_for_action ("win.set-affinity", "<Alt>s");
  set_accel_for_action ("win.memory-maps", "<Control>m");
  set_accel_for_action ("win.open-files", "<Control>o");
  set_accel_for_action ("win.process-properties", "<Alt>Return");
  set_accel_for_action ("win.search", "<Control>f");
  set_accel_for_action ("app.help", "F1");
  set_accel_for_action ("win.show-help-overlay", "<Control>question");
  set_accel_for_action ("app.preferences", "<Primary>comma");
  set_accel_for_action ("win.show-page('resources')", "<Alt>1");
  set_accel_for_action ("win.show-page('processes')", "<Alt>2");
  set_accel_for_action ("win.show-page('disks')", "<Alt>3");
  set_accel_for_action ("win.refresh", "<Control>r");

  load_settings ();

  pretty_table = new PrettyTable ();
  smooth_refresh = new SmoothRefresh (settings);
}
