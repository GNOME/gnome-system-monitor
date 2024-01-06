#ifndef _GSM_PRETTY_TABLE_H_
#define _GSM_PRETTY_TABLE_H_

#include <glib.h>
#include <gio/gio.h>
#include <giomm.h>
#include <glibmm/refptr.h>
#include <giomm/filemonitor.h>
#include <gdkmm/texture.h>

#include <map>
#include <string>

class ProcInfo;

using std::string;

class PrettyTable
{
public:
PrettyTable();
~PrettyTable();

void                       set_icon (ProcInfo &);

private:
Glib::RefPtr<Gdk::Texture> get_icon_from_theme (const ProcInfo &);
Glib::RefPtr<Gdk::Texture> get_icon_from_default (const ProcInfo &);
Glib::RefPtr<Gdk::Texture> get_icon_from_gio (const ProcInfo &);
Glib::RefPtr<Gdk::Texture> get_icon_from_name (const ProcInfo &);
Glib::RefPtr<Gdk::Texture> get_icon_for_kernel (const ProcInfo &);
Glib::RefPtr<Gdk::Texture> get_icon_dummy (const ProcInfo &);

bool                       get_default_icon_name (const string &cmd,
                                                  string &      name);
void file_monitor_event (Glib::RefPtr<Gio::File>,
                         Glib::RefPtr<Gio::File>,
                         Gio::FileMonitor::Event);
void init_gio_app_cache ();

typedef std::map<string, Glib::RefPtr<Gdk::Texture> > IconCache;
typedef std::map<pid_t, Glib::RefPtr<Gdk::Texture> > IconsForPID;
typedef std::map<string, Glib::RefPtr<Gio::AppInfo> > AppCache;
typedef std::map<string, Glib::RefPtr<Gio::FileMonitor> > DesktopDirMonitors;

IconsForPID apps;
IconCache defaults;
DesktopDirMonitors monitors;
AppCache gio_apps;
};

#endif /* _GSM_PRETTY_TABLE_H_ */
