/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _GSM_APPLICATION_H_
#define _GSM_APPLICATION_H_

#include <gtkmm.h>
#include <glibtop/cpu.h>

struct LoadGraph;

#include "smooth_refresh.h"
#include "prettytable.h"

static const unsigned MIN_UPDATE_INTERVAL =   1 * 1000;
static const unsigned MAX_UPDATE_INTERVAL = 100 * 1000;


enum ProcmanTab
{
    PROCMAN_TAB_PROCESSES,
    PROCMAN_TAB_RESOURCES,
    PROCMAN_TAB_DISKS
};


struct ProcConfig
{
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





class GsmApplication : public Gtk::Application
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
