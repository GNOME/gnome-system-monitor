#include <config.h>

#ifdef HAVE_WNCK
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#endif

#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <glibtop/procstate.h>
#include <giomm/error.h>
#include <giomm/file.h>
#include <glibmm/miscutils.h>
#include <iostream>

#include <vector>

#include "prettytable.h"
#include "defaulttable.h"
#include "proctable.h"
#include "util.h"

namespace
{
const unsigned APP_ICON_SIZE = 16;
}


PrettyTable::PrettyTable()
{
    // init GIO apps cache
    std::vector<std::string> dirs = Glib::get_system_data_dirs ();
    for (std::vector<std::string>::iterator it = dirs.begin (); it != dirs.end (); ++it) {
        std::string path = (*it).append ("/applications");
        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path (path);
        Glib::RefPtr<Gio::FileMonitor> monitor = file->monitor_directory ();
        monitor->set_rate_limit (1000); // 1 second

        // TODO: Properly handle signal change
        // monitor->signal_changed ().connect (sigc::mem_fun (this, &PrettyTable::file_monitor_event));
        monitors[path] = monitor;
    }

    this->init_gio_app_cache ();
}


PrettyTable::~PrettyTable()
{
}

#ifdef HAVE_WNCK
void
PrettyTable::on_application_opened (WnckScreen     *screen,
                                    WnckApplication*app,
                                    gpointer        data)
{
    PrettyTable * const that = static_cast<PrettyTable*>(data);

    pid_t pid = wnck_application_get_pid (app);

    if (pid == 0)
        return;

    const char*icon_name = wnck_application_get_icon_name (app);

    Glib::RefPtr<Gtk::IconTheme> icon_theme;
    Glib::RefPtr<Gtk::IconPaintable> icon_paintable;
    Glib::RefPtr<Gdk::Texture> icon;

    icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

    icon_paintable = Gtk::IconTheme::lookup_icon (
        icon_name.c_str (),
        APP_ICON_SIZE,
        1,
        Gtk::TextDirection::NONE,
        Gtk::IconLookupFlags::PRELOAD);

    icon = Gdk::Texture::create_from_file (icon_paintable.get_file ());

    if (not icon) {
        icon = Glib::wrap (wnck_application_get_icon (app), /* take_copy */ true);
        icon = icon->scale_simple (APP_ICON_SIZE, APP_ICON_SIZE, Gdk::INTERP_HYPER);
    }

    if (not icon)
        return;

    that->register_application (pid, icon);
}



void
PrettyTable::register_application (pid_t                     pid,
                                   Glib::RefPtr<Gdk::Pixbuf> icon)
{
    /* If process already exists then set the icon. Otherwise put into hash
    ** table to be added later */
    if (ProcInfo*info = GsmApplication::get ()->processes.find (pid)) {
        info->set_icon (icon);
        // move the ref to the map
        this->apps[pid] = icon;
        procman_debug ("WNCK OK for %u", unsigned(pid));
    }
}



void
PrettyTable::on_application_closed (WnckScreen     *screen,
                                    WnckApplication*app,
                                    gpointer        data)
{
    pid_t pid = wnck_application_get_pid (app);

    if (pid == 0)
        return;

    static_cast<PrettyTable*>(data)->unregister_application (pid);
}



void
PrettyTable::unregister_application (pid_t pid)
{
    IconsForPID::iterator it (this->apps.find (pid));

    if (it != this->apps.end ())
        this->apps.erase (it);
}
#endif // HAVE_WNCK

void PrettyTable::init_gio_app_cache ()
{
    this->gio_apps.clear ();

    std::vector<Glib::RefPtr<Gio::AppInfo> > apps = Gio::AppInfo::get_all ();
    for (std::vector<Glib::RefPtr<Gio::AppInfo> >::const_iterator it = apps.begin ();
         it != apps.end (); ++it) {
        Glib::RefPtr<Gio::AppInfo> app = *it;
        std::string executable = app->get_executable ();
        if (executable != "sh" &&
            executable != "env")
            this->gio_apps[executable] = app;
    }
}

void PrettyTable::file_monitor_event (Glib::RefPtr<Gio::File>,
                                      Glib::RefPtr<Gio::File>,
                                      Gio::FileMonitor::Event)
{
    this->init_gio_app_cache ();
}

Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_from_theme (const ProcInfo &info)
{
    Glib::RefPtr<Gtk::IconTheme> icon_theme;
    Glib::RefPtr<Gtk::IconPaintable> icon_paintable;

    icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

    icon_paintable = icon_theme->lookup_icon (
        info.name.c_str (),
        APP_ICON_SIZE,
        1,
        Gtk::TextDirection::NONE,
        Gtk::IconLookupFlags::PRELOAD);

    return Gdk::Texture::create_from_file (icon_paintable->get_file ());
}


bool PrettyTable::get_default_icon_name (const string &cmd,
                                         string &      name)
{
    for (size_t i = 0; i != G_N_ELEMENTS (default_table); ++i) {
        if (default_table[i].command->match (cmd.c_str ())) {
            name = default_table[i].icon;
            return true;
        }
    }

    return false;
}

/*
  Try to get an icon from the default_table
  If it's not in defaults, try to load it.
  If there is no default for a command, store NULL in defaults
  so we don't have to lookup again.
*/

Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_from_default (const ProcInfo &info)
{
    Glib::RefPtr<Gtk::IconTheme> icon_theme;
    Glib::RefPtr<Gtk::IconPaintable> icon_paintable;
    Glib::RefPtr<Gdk::Texture> texture;
    string name;

    if (this->get_default_icon_name (info.name, name)) {
        IconCache::iterator it (this->defaults.find (name));

        if (it == this->defaults.end ()) {
            icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

            icon_paintable = icon_theme->lookup_icon (
                name.c_str (),
                APP_ICON_SIZE,
                1,
                Gtk::TextDirection::NONE,
                Gtk::IconLookupFlags::PRELOAD);

            texture = Gdk::Texture::create_from_file (icon_paintable->get_file ());

            if (texture)
                this->defaults[name] = texture;
        } else {
            texture = it->second;
        }
    }

    return texture;
}

Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_from_gio (const ProcInfo &info)
{
    gchar **cmdline = g_strsplit (info.name.c_str (), " ", 2);
    const gchar *executable = cmdline[0];
    Glib::RefPtr<Gtk::IconTheme> icon_theme;
    Glib::RefPtr<Gtk::IconPaintable> icon_paintable;
    Glib::RefPtr<Gdk::Texture> icon;

    if (executable) {
        Glib::RefPtr<Gio::AppInfo> app = this->gio_apps[executable];
        Glib::RefPtr<Gio::Icon> gicon;
        Glib::RefPtr<Gtk::IconPaintable> info;

        if (app)
            gicon = app->get_icon ();

        icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

        if (gicon)
            icon_paintable = icon_theme->lookup_icon (
                gicon,
                APP_ICON_SIZE,
                1,
                Gtk::TextDirection::NONE,
                Gtk::IconLookupFlags::PRELOAD);

        if (info)
            icon = Gdk::Texture::create_from_file (info->get_file ());
    }

    g_strfreev (cmdline);
    return icon;
}

#ifdef HAVE_WNCK
Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_from_wnck (const ProcInfo &info)
{
    Glib::RefPtr<Gdk::Texture> icon;

    IconsForPID::iterator it (this->apps.find (info.pid));

    if (it != this->apps.end ())
        icon = it->second;

    return icon;
}
#endif


Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_from_name (const ProcInfo &info)
{
    Glib::RefPtr<Gtk::IconTheme> icon_theme;
    Glib::RefPtr<Gtk::IconPaintable> icon_paintable;

    icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

    icon_paintable = icon_theme->lookup_icon (
        info.name.c_str (),
        APP_ICON_SIZE,
        1,
        Gtk::TextDirection::NONE,
        Gtk::IconLookupFlags::PRELOAD);

    return Gdk::Texture::create_from_file (icon_paintable->get_file ());
}


Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_dummy (const ProcInfo &)
{
    Glib::RefPtr<Gtk::IconTheme> icon_theme;
    Glib::RefPtr<Gtk::IconPaintable> icon_paintable;

    icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

    icon_paintable = icon_theme->lookup_icon (
        "application-x-executable",
        APP_ICON_SIZE,
        1,
        Gtk::TextDirection::NONE,
        Gtk::IconLookupFlags::PRELOAD);

    return Gdk::Texture::create_from_file (icon_paintable->get_file ());
}


namespace
{
bool has_kthreadd ()
{
    glibtop_proc_state buf;
    glibtop_get_proc_state (&buf, 2);

    return buf.cmd == string ("kthreadd");
}

// @pre: has_kthreadd
bool is_kthread (const ProcInfo &info)
{
    return info.pid == 2 or info.ppid == 2;
}
}


Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_for_kernel (const ProcInfo &info)
{
    if (is_kthread (info)) {
        Glib::RefPtr<Gtk::IconTheme> icon_theme;
        Glib::RefPtr<Gtk::IconPaintable> icon_paintable;

        icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

        icon_paintable = icon_theme->lookup_icon (
            "application-x-executable",
            APP_ICON_SIZE,
            1,
            Gtk::TextDirection::NONE,
            Gtk::IconLookupFlags::PRELOAD);

        return Gdk::Texture::create_from_file (icon_paintable->get_file ());
    }

    return Glib::RefPtr<Gdk::Texture> ();
}


void
PrettyTable::set_icon (ProcInfo &info)
{
    typedef Glib::RefPtr<Gdk::Texture> (PrettyTable::*Getter) (const ProcInfo &);

    static std::vector<Getter> getters;

    if (getters.empty ()) {
        getters.push_back (&PrettyTable::get_icon_from_gio);
#ifdef HAVE_WNCK
        getters.push_back (&PrettyTable::get_icon_from_wnck);
#endif
        getters.push_back (&PrettyTable::get_icon_from_theme);
        getters.push_back (&PrettyTable::get_icon_from_default);
        getters.push_back (&PrettyTable::get_icon_from_name);
        if (has_kthreadd ()) {
            procman_debug ("kthreadd is running with PID 2");
            getters.push_back (&PrettyTable::get_icon_for_kernel);
        }
        getters.push_back (&PrettyTable::get_icon_dummy);
    }

    Glib::RefPtr<Gdk::Texture> icon;

    for (size_t i = 0; not icon and i < getters.size (); ++i) {
        try {
            icon = (this->*getters[i])(info);
        }
        catch (std::exception&e) {
            g_warning ("Failed to load icon for %s(%u) : %s", info.name.c_str (), info.pid, e.what ());
            continue;
        }
    }

    info.set_icon (icon);
}
