#ifndef _GSM_APPLICATION_H_
#define _GSM_APPLICATION_H_

#include <gtkmm.h>
#include <glibtop/cpu.h>
#include <adwaita.h>

#include <algorithm>
#include <mutex>

struct LoadGraph;

#include "legacy/treeview.h"
#include "argv.h"
#include "disks.h"
#include "prettytable.h"
#include "procinfo.h"
#include "proclist.h"
#include "smooth_refresh.h"
#include "util.h"

struct ProcConfig
  : private procman::NonCopyable
{
  ProcConfig ()
    : current_tab (),
    update_interval (0),
    graph_update_interval (0),
    graph_data_points (0),
    mem_color (),
    swap_color (),
    net_in_color (),
    net_out_color (),
    disk_read_color (),
    disk_write_color (),
    bg_color (),
    frame_color (),
    num_cpus (0),
    solaris_mode (false),
    process_memory_in_iec (true),
    logarithmic_scale (false),
    draw_stacked (false),
    draw_smooth (true),
    resources_memory_in_iec (true),
    network_in_bits (false),
    network_total_in_bits (false)
  {
    std::fill (&this->cpu_color[0], &this->cpu_color[GLIBTOP_NCPU], GdkRGBA ());
  }

  Glib::ustring current_tab;
  int update_interval;
  int graph_update_interval;
  int graph_data_points;
  GdkRGBA cpu_color[GLIBTOP_NCPU];
  GdkRGBA mem_color;
  GdkRGBA swap_color;
  GdkRGBA net_in_color;
  GdkRGBA net_out_color;
  GdkRGBA disk_read_color;
  GdkRGBA disk_write_color;
  GdkRGBA bg_color;
  GdkRGBA frame_color;
  gint num_cpus;
  bool solaris_mode;
  bool process_memory_in_iec;
  bool logarithmic_scale;
  bool draw_stacked;
  bool draw_smooth;
  bool resources_memory_in_iec;
  bool network_in_bits;
  bool network_total_in_bits;
};

class GsmApplication final: public Gtk::Application, private procman::NonCopyable
{
private:
  void load_settings ();
  void load_command_line_options();

  void on_preferences_activate (const Glib::VariantBase&);
  void on_lsof_activate (const Glib::VariantBase&);
  void on_help_activate (const Glib::VariantBase&);
  void on_quit_activate (const Glib::VariantBase&);
  int handle_local_options (const Glib::RefPtr<Glib::VariantDict> &);

  GsmApplication();

  void on_activate () override;
  int on_command_line (const Glib::RefPtr<Gio::ApplicationCommandLine>&command_line) override;
  void on_startup () override;
public:
  static GsmApplication& get ();

  void save_config ();
  void shutdown ();

  Glib::RefPtr<Gio::Settings> settings;
  AdwApplicationWindow *main_window;
  AdwViewStack *stack;
  GtkMenuButton *app_menu_button;

  GMenuModel *generic_window_menu_model;
  GMenuModel *process_window_menu_model;

  ProcList processes;
  ProcConfig config;
  PrettyTable *pretty_table;
  GsmTreeView *tree;
  GtkRevealer *proc_actionbar_revealer;
  GtkPopover *proc_popover_menu;
  GtkButton *refresh_button;
  GtkButton *end_process_button;
  GtkToggleButton *search_button;
  GtkSearchEntry *search_entry;
  GtkSearchBar *search_bar;
  GtkTreePath *top_of_tree;

  gdouble last_vscroll_max;
  gdouble last_vscroll_value;
  guint timeout;
  guint64 cpu_total_time;
  guint64 cpu_total_time_last;
  unsigned frequency;

  LoadGraph *cpu_graph;
  LoadGraph *mem_graph;
  LoadGraph *net_graph;
  LoadGraph *disk_graph;

  GsmDisksView *disk_list;

  GtkTreeSelection *selection;

  SmoothRefresh *smooth_refresh;
  procman::OptionGroup option_group;
};

#endif /* _GSM_APPLICATION_H_ */
