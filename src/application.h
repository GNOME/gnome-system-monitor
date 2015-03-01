/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _GSM_APPLICATION_H_
#define _GSM_APPLICATION_H_

#include <gtkmm.h>
#include <glibtop/cpu.h>

#include <algorithm>

class ProcInfo;
struct LoadGraph;

#include "smooth_refresh.h"
#include "prettytable.h"
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
    bool network_in_bits;
};



struct MutableProcInfo
  : private procman::NonCopyable
{

MutableProcInfo()
  : user(),
    vmsize(0UL),
    memres(0UL),
    memshared(0UL),
    memwritable(0UL),
    mem(0UL),
#ifdef HAVE_WNCK
    memxserver(0UL),
#endif
    start_time(0UL),
    cpu_time(0ULL),
    status(0U),
    pcpu(0U),
    nice(0),
    cgroup_name(NULL),
    unit(NULL),
    session(NULL),
    seat(NULL),
    owner()
    {
      wchan[0] = '\0';
    }

    std::string user;

    gchar wchan[40];

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
    guint status;
    guint pcpu;
    gint nice;
    gchar *cgroup_name;

    gchar *unit;
    gchar *session;
    gchar *seat;

    std::string owner;
};


class ProcInfo
: public MutableProcInfo
{
    /* undefined */ ProcInfo& operator=(const ProcInfo&);
    /* undefined */ ProcInfo(const ProcInfo&);

    typedef std::map<guint, std::string> UserMap;
    /* cached username */
    static UserMap users;

  public:

    // TODO: use a set instead
    // sorted by pid. The map has a nice property : it is sorted
    // by pid so this helps a lot when looking for the parent node
    // as ppid is nearly always < pid.
    typedef std::map<pid_t, ProcInfo*> List;
    typedef List::iterator Iterator;

    static List all;

    static ProcInfo* find(pid_t pid);
    static Iterator begin() { return ProcInfo::all.begin(); }
    static Iterator end() { return ProcInfo::all.end(); }


    ProcInfo(pid_t pid);
    ~ProcInfo();
    // adds one more ref to icon
    void set_icon(Glib::RefPtr<Gdk::Pixbuf> icon);
    void set_user(guint uid);
    std::string lookup_user(guint uid);

    GtkTreeIter     node;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    gchar           *tooltip;
    gchar           *name;
    gchar           *arguments;

    gchar           *security_context;

    const pid_t     pid;
    pid_t           ppid;
    guint           uid;

// private:
    // tracks cpu time per process keeps growing because if a
    // ProcInfo is deleted this does not mean that the process is
    // not going to be recreated on the next update.  For example,
    // if dependencies + (My or Active), the proclist is cleared
    // on each update.  This is a workaround
    static std::map<pid_t, guint64> cpu_times;
};



class GsmApplication : public Gtk::Application, private procman::NonCopyable

{
private:
    void load_settings();

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

    GtkWidget        *tree;
    GtkWidget        *proc_actionbar_revealer;
    GtkWidget        *popup_menu;
    GtkWidget        *disk_list;
    GtkWidget        *stack;
    GtkWidget        *refresh_button;
    GtkWidget        *process_menu_button;
    GtkWidget        *end_process_button;
    GtkWidget        *search_button;
    GtkWidget        *search_entry;
    GtkWidget        *search_bar;
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

    GSettings        *settings;
    GtkWidget        *main_window;

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
