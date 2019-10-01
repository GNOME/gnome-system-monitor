/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _GSM_APPLICATION_H_
#define _GSM_APPLICATION_H_

#include <gtkmm.h>
#include <glibtop/cpu.h>

#include <algorithm>

struct LoadGraph;

#include "smooth_refresh.h"
#include "prettytable.h"
#include "legacy/treeview.h"
#include "util.h"

static const unsigned MIN_UPDATE_INTERVAL =   1 * 1000;
static const unsigned MAX_UPDATE_INTERVAL = 100 * 1000;


enum ProcmanTab
{
    PROCMAN_TAB_PROCESSES,
    PROCMAN_TAB_RESOURCES,
    PROCMAN_TAB_DISKS
};


struct ProcConfig
  : private procman::NonCopyable
{
    ProcConfig()
      : update_interval(0),
	graph_update_interval(0),
	disks_update_interval(0),
	mem_color(),
	swap_color(),
	net_in_color(),
	net_out_color(),
	bg_color(),
	frame_color(),
	num_cpus(0),
	solaris_mode(false),
	draw_stacked(false),
	draw_smooth(true),
	network_in_bits(false)
    {
      std::fill(&this->cpu_color[0], &this->cpu_color[GLIBTOP_NCPU], GdkRGBA());
    }

    int             update_interval;
    int             graph_update_interval;
    int             disks_update_interval;
    GdkRGBA         cpu_color[GLIBTOP_NCPU];
    GdkRGBA         mem_color;
    GdkRGBA         swap_color;
    GdkRGBA         net_in_color;
    GdkRGBA         net_out_color;
    GdkRGBA         bg_color;
    GdkRGBA         frame_color;
    gint            num_cpus;
    bool solaris_mode;
    bool draw_stacked;
    bool draw_smooth;
    bool network_in_bits;
};



struct MutableProcInfo
  : private procman::NonCopyable
{

MutableProcInfo()
  : vmsize(0UL),
    memres(0UL),
    memshared(0UL),
    memwritable(0UL),
    mem(0UL),
#ifdef HAVE_WNCK
    memxserver(0UL),
#endif
    start_time(0UL),
    cpu_time(0ULL),
    disk_read_bytes_total(0ULL),
    disk_write_bytes_total(0ULL),
    disk_read_bytes_current(0ULL),
    disk_write_bytes_current(0ULL),
    status(0U),
    pcpu(0U),
    nice(0)
    {
    }

    std::string user;

    std::string wchan;

    // all these members are filled with libgtop which uses
    // guint64 (to have fixed size data) but we don't need more
    // than an unsigned long (even for 32bit apps on a 64bit
    // kernel) as these data are amounts, not offsets.
    gulong vmsize;
    gulong memres;
    gulong memshared;
    gulong memwritable;
    gulong mem;

#ifdef HAVE_WNCK
    // wnck gives an unsigned long
    gulong memxserver;
#endif

    gulong start_time;
    guint64 cpu_time;
    guint64 disk_read_bytes_total;
    guint64 disk_write_bytes_total;
    guint64 disk_read_bytes_current;
    guint64 disk_write_bytes_current;
    guint status;
    guint pcpu;
    gint nice;
    std::string cgroup_name;

    std::string unit;
    std::string session;
    std::string seat;

    std::string owner;
};


class ProcInfo
: public MutableProcInfo
{
  public:
    ProcInfo& operator=(const ProcInfo&) = delete;
    ProcInfo(const ProcInfo&) = delete;
    ProcInfo(pid_t pid);
    // adds one more ref to icon
    void set_icon(Glib::RefPtr<Gdk::Pixbuf> icon);
    void set_user(guint uid);
    std::string lookup_user(guint uid);

    GtkTreeIter     node;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    std::string     tooltip;
    std::string     name;
    std::string     arguments;

    std::string     security_context;

    const pid_t     pid;
    pid_t           ppid;
    guint           uid;
};

class ProcList {
    // TODO: use a set instead
    // sorted by pid. The map has a nice property : it is sorted
    // by pid so this helps a lot when looking for the parent node
    // as ppid is nearly always < pid.
    typedef std::map<pid_t, ProcInfo> List;
    List data;
    std::mutex data_lock;
public:
    std::map<pid_t, unsigned long> cpu_times;
    typedef List::iterator Iterator;
    Iterator begin() { return std::begin(data); }
    Iterator end() { return std::end(data); }
    Iterator erase(Iterator it) {
        std::lock_guard<std::mutex> lg(data_lock);
        return data.erase(it);
    }
    ProcInfo* add(pid_t pid) { return &data.emplace(std::piecewise_construct, std::forward_as_tuple(pid), std::forward_as_tuple(pid)).first->second; }
    void clear() { return data.clear(); }

    ProcInfo* find(pid_t pid);
};

class GsmApplication : public Gtk::Application, private procman::NonCopyable

{
private:
    void load_settings();
    void load_resources();

    void on_preferences_activate(const Glib::VariantBase&);
    void on_lsof_activate(const Glib::VariantBase&);
    void on_help_activate(const Glib::VariantBase&);
    void on_quit_activate(const Glib::VariantBase&);
protected:
    GsmApplication();
public:
    static Glib::RefPtr<GsmApplication> get ();

    void save_config();
    void shutdown();

    ProcList         processes;
    GsmTreeView      *tree;
    GtkRevealer      *proc_actionbar_revealer;
    GtkMenu          *popup_menu;
    GsmTreeView      *disk_list;
    GtkStack         *stack;
    GtkButton        *refresh_button;
    GtkMenuButton    *process_menu_button;
    GtkMenuButton    *window_menu_button;
    GtkButton        *end_process_button;
    GtkButton        *search_button;
    GtkSearchEntry   *search_entry;
    GtkSearchBar     *search_bar;
    ProcConfig        config;
    LoadGraph        *cpu_graph;
    LoadGraph        *mem_graph;
    LoadGraph        *net_graph;
    gint              cpu_label_fixed_width;
    gint              net_label_fixed_width;
    GtkTreeSelection *selection;
    guint             timeout;
    guint             disk_timeout;

    GtkTreePath      *top_of_tree;
    gdouble          last_vscroll_max;
    gdouble          last_vscroll_value;

    PrettyTable      *pretty_table;

    Glib::RefPtr<Gio::Settings> settings;
    GtkApplicationWindow *main_window;

    unsigned         frequency;

    SmoothRefresh    *smooth_refresh;

    guint64           cpu_total_time;
    guint64           cpu_total_time_last;

protected:
    virtual void on_activate();
    virtual int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line);
    virtual void on_startup();
};

#endif /* _GSM_APPLICATION_H_ */
