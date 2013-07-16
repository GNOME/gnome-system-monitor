#ifndef _PROCMAN_PRETTYTABLE_H_
#define _PROCMAN_PRETTYTABLE_H_

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glibmm/refptr.h>
#include <giomm/filemonitor.h>

#include <gdkmm/pixbuf.h>

#include <map>
#include <string>

#ifdef HAVE_WNCK
extern "C" {
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
}
#endif

#include "iconthemewrapper.h"

class ProcInfo;

using std::string;



class PrettyTable
{
public:
    PrettyTable();
    ~PrettyTable();

    void set_icon(ProcInfo &);

private:

#ifdef HAVE_WNCK
    static void on_application_opened(WnckScreen* screen, WnckApplication* app, gpointer data);
    static void on_application_closed(WnckScreen* screen, WnckApplication* app, gpointer data);

    void register_application(pid_t pid, Glib::RefPtr<Gdk::Pixbuf> icon);
    void unregister_application(pid_t pid);
#endif

    Glib::RefPtr<Gdk::Pixbuf> get_icon_from_theme(const ProcInfo &);
    Glib::RefPtr<Gdk::Pixbuf> get_icon_from_default(const ProcInfo &);
    Glib::RefPtr<Gdk::Pixbuf> get_icon_from_gio(const ProcInfo &);
#ifdef HAVE_WNCK
    Glib::RefPtr<Gdk::Pixbuf> get_icon_from_wnck(const ProcInfo &);
#endif
    Glib::RefPtr<Gdk::Pixbuf> get_icon_from_name(const ProcInfo &);
    Glib::RefPtr<Gdk::Pixbuf> get_icon_for_kernel(const ProcInfo &);
    Glib::RefPtr<Gdk::Pixbuf> get_icon_dummy(const ProcInfo &);

    bool get_default_icon_name(const string &cmd, string &name);
    void file_monitor_event (Glib::RefPtr<Gio::File>,
                             Glib::RefPtr<Gio::File>,
                             Gio::FileMonitorEvent);
    void init_gio_app_cache ();

    typedef std::map<string, Glib::RefPtr<Gdk::Pixbuf> > IconCache;
    typedef std::map<pid_t, Glib::RefPtr<Gdk::Pixbuf> > IconsForPID;
    typedef std::map<string, Glib::RefPtr<Gio::AppInfo> > AppCache;
    typedef std::map<string, Glib::RefPtr<Gio::FileMonitor> > DesktopDirMonitors;

    IconsForPID apps;
    IconCache defaults;
    DesktopDirMonitors monitors;
    AppCache gio_apps;
    procman::IconThemeWrapper theme;
};


#endif /* _PROCMAN_PRETTYTABLE_H_ */
