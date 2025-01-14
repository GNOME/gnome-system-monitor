#include <config.h>

#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <glibtop/procstate.h>
#include <gdkmm/pixbuf.h>
#include <giomm/error.h>
#include <giomm/file.h>
#include <glibmm/miscutils.h>

#include <iostream>
#include <vector>

#include "prettytable.h"
#include "defaulttable.h"
#include "procinfo.h"
#include "proctable.h"
#include "util.h"

const unsigned APP_ICON_SIZE = 16;

PrettyTable::PrettyTable()
{
  // init GIO apps cache
  std::vector<std::string> dirs = Glib::get_system_data_dirs ();

  for (std::vector<std::string>::iterator it = dirs.begin (); it != dirs.end (); ++it)
    {
      std::string path = (*it).append ("/applications");
      Glib::RefPtr<Gio::File> file = Gio::File::create_for_path (path);
      Glib::RefPtr<Gio::FileMonitor> monitor = file->monitor_directory ();

      monitor->set_rate_limit (1000); // 1 second

      monitor->signal_changed ().connect (sigc::mem_fun (*this, &PrettyTable::file_monitor_event));
      monitors[path] = monitor;
    }

  this->init_gio_app_cache ();
}


PrettyTable::~PrettyTable()
{
}

void
PrettyTable::init_gio_app_cache ()
{
  this->gio_apps.clear ();

  std::vector<Glib::RefPtr<Gio::AppInfo> > apps = Gio::AppInfo::get_all ();

  for (std::vector<Glib::RefPtr<Gio::AppInfo> >::const_iterator it = apps.begin ();
       it != apps.end (); ++it)
    {
      Glib::RefPtr<Gio::AppInfo> app = *it;
      Glib::RefPtr<Gio::DesktopAppInfo> desktop = std::dynamic_pointer_cast<Gio::DesktopAppInfo>(app);
      std::string executable = app->get_executable ();
      std::string flatpak = desktop->get_string ("X-Flatpak");
      bool visible = app->should_show ();

      if (!flatpak.empty ())
        this->gio_apps[flatpak] = app;
      else if (executable != "sh" &&
               executable != "env" &&
               visible)
        this->gio_apps[executable] = app;
    }
}

void
PrettyTable::file_monitor_event (Glib::RefPtr<Gio::File>,
                                 Glib::RefPtr<Gio::File>,
                                 Gio::FileMonitor::Event)
{
  this->init_gio_app_cache ();
}

Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_from_theme (const ProcInfo &info)
{
  Glib::RefPtr<Gtk::IconTheme> icon_theme;
  Glib::RefPtr<Gdk::Texture> icon;

  icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

  if (icon_theme->has_icon (info.name))
    {
      Glib::RefPtr<Gtk::IconPaintable> icon_paintable;

      // Because g-s-m still uses GtkCellView which does not support GdkPaintable
      // the easiest way to get GdkPixbuf or GdkTexture is to get the icon, get
      // the file behind it, make it into a sized GdkPixbuf and turn it into
      // GdkTexture.
      // Once g-s-m uses GtkColumnView, GdkPaintable could be used instead.
      icon_paintable = icon_theme->lookup_icon (
        info.name,
        APP_ICON_SIZE,
        1,
        Gtk::TextDirection::NONE,
        Gtk::IconLookupFlags::PRELOAD);

      icon = Gdk::Texture::create_for_pixbuf (Gdk::Pixbuf::create_from_file (icon_paintable->get_file ()->get_path (), APP_ICON_SIZE, APP_ICON_SIZE));
    }

  return icon;
}

bool
PrettyTable::get_default_icon_name (const string &cmd,
                                    string &      name)
{
  for (size_t i = 0; i != G_N_ELEMENTS (default_table); ++i)
    if (default_table[i].command->match (cmd.c_str ()))
      {
        name = default_table[i].icon;
        return true;
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
  Glib::RefPtr<Gdk::Texture> icon;
  string name;

  if (!this->get_default_icon_name (info.name, name))
    return icon;

  IconCache::iterator it (this->defaults.find (name));
  if (it == this->defaults.end ())
    {
      Glib::RefPtr<Gtk::IconPaintable> icon_paintable;

      icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

      if (!icon_theme->has_icon (name))
        return icon;

      // Because g-s-m still uses GtkCellView which does not support GdkPaintable
      // the easiest way to get GdkPixbuf or GdkTexture is to get the icon, get
      // the file behind it, make it into a sized GdkPixbuf and turn it into
      // GdkTexture.
      // Once g-s-m uses GtkColumnView, GdkPaintable could be used instead.
      icon_paintable = icon_theme->lookup_icon (
        name,
        APP_ICON_SIZE,
        1,
        Gtk::TextDirection::NONE,
        Gtk::IconLookupFlags::PRELOAD);

      icon = Gdk::Texture::create_for_pixbuf (Gdk::Pixbuf::create_from_file (icon_paintable->get_file ()->get_path (), APP_ICON_SIZE, APP_ICON_SIZE));

      this->defaults[name] = icon;
    }
  else
    {
      icon = it->second;
    }

  return icon;
}

Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_from_gio (const ProcInfo &info)
{
  Glib::RefPtr<Gdk::Texture> icon;
  Glib::RefPtr<Gio::Icon> gicon;
  Glib::RefPtr<Gtk::IconTheme> icon_theme;
  Glib::RefPtr<Gtk::IconPaintable> icon_paintable;
  const auto executable = info.name.substr (0, info.name.find (' '));

  Glib::RefPtr<Gio::AppInfo> app = this->gio_apps[executable];

  if (!app)
    {
      std::string flatpak;
      size_t last_dash, second_last_dash;

      // Extract flatpak id from cgroup_name
      flatpak = info.cgroup_name.c_str ();
      flatpak = flatpak.substr (flatpak.find_last_of ('/') + 1);
      last_dash = flatpak.find_last_of ('-');
      second_last_dash = flatpak.find_last_of ('-', flatpak.find_last_of ('-') - 1);
      flatpak = flatpak.substr (second_last_dash + 1, last_dash - second_last_dash - 1);

      app = this->gio_apps[flatpak];

      if (!app)
        return icon;
    }

  gicon = app->get_icon ();
  if (!gicon)
    return icon;

  icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

  icon_paintable = icon_theme->lookup_icon (
    gicon,
    APP_ICON_SIZE,
    1,
    Gtk::TextDirection::NONE,
    Gtk::IconLookupFlags::PRELOAD);

  std::string path = icon_paintable->get_file ()->get_path ();
  icon = Gdk::Texture::create_for_pixbuf (Gdk::Pixbuf::create_from_file (path, APP_ICON_SIZE, APP_ICON_SIZE));

  return icon;
}

Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_from_name (const ProcInfo &info)
{
  Glib::RefPtr<Gtk::IconTheme> icon_theme;
  Glib::RefPtr<Gtk::IconPaintable> icon_paintable;
  Glib::RefPtr<Gdk::Texture> icon;

  icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());

  if (!icon_theme->has_icon (info.name))
    return icon;

  icon_paintable = icon_theme->lookup_icon (
    info.name,
    APP_ICON_SIZE,
    1,
    Gtk::TextDirection::NONE,
    Gtk::IconLookupFlags::PRELOAD);
  icon = Gdk::Texture::create_for_pixbuf (Gdk::Pixbuf::create_from_file (icon_paintable->get_file ()->get_path (), APP_ICON_SIZE, APP_ICON_SIZE));

  return icon;
}


Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_dummy (const ProcInfo &)
{
  Glib::RefPtr<Gtk::IconTheme> icon_theme;
  Glib::RefPtr<Gtk::IconPaintable> icon_paintable;
  Glib::RefPtr<Gdk::Texture> icon;

  icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());
  icon_paintable = icon_theme->lookup_icon (
    "application-x-executable",
    APP_ICON_SIZE,
    1,
    Gtk::TextDirection::NONE,
    Gtk::IconLookupFlags::PRELOAD);
  icon = Gdk::Texture::create_for_pixbuf (Gdk::Pixbuf::create_from_file (icon_paintable->get_file ()->get_path (), APP_ICON_SIZE, APP_ICON_SIZE));

  return icon;
}

namespace
{
bool
has_kthreadd ()
{
  glibtop_proc_state buf;

  glibtop_get_proc_state (&buf, 2);

  return buf.cmd == string ("kthreadd");
}

// @pre: has_kthreadd
bool
is_kthread (const ProcInfo &info)
{
  return info.pid == 2 or info.ppid == 2;
}
}

Glib::RefPtr<Gdk::Texture>
PrettyTable::get_icon_for_kernel (const ProcInfo &info)
{
  Glib::RefPtr<Gdk::Texture> icon;
  Glib::RefPtr<Gtk::IconTheme> icon_theme;
  Glib::RefPtr<Gtk::IconPaintable> icon_paintable;

  if (!is_kthread (info))
    return icon;

  icon_theme = Gtk::IconTheme::get_for_display (Gdk::Display::get_default ());
  icon_paintable = icon_theme->lookup_icon (
    "application-x-executable",
    APP_ICON_SIZE,
    1,
    Gtk::TextDirection::NONE,
    Gtk::IconLookupFlags::PRELOAD);
  icon = Gdk::Texture::create_for_pixbuf (Gdk::Pixbuf::create_from_file (icon_paintable->get_file ()->get_path (), APP_ICON_SIZE, APP_ICON_SIZE));

  return icon;
}

void
PrettyTable::set_icon (ProcInfo &info)
{
  typedef Glib::RefPtr<Gdk::Texture> (PrettyTable::*Getter) (const ProcInfo &);

  static std::vector<Getter> getters;

  if (getters.empty ())
    {
      getters.push_back (&PrettyTable::get_icon_from_gio);
      getters.push_back (&PrettyTable::get_icon_from_theme);
      getters.push_back (&PrettyTable::get_icon_from_default);
      getters.push_back (&PrettyTable::get_icon_from_name);
      if (has_kthreadd ())
        {
          procman_debug ("kthreadd is running with PID 2");
          getters.push_back (&PrettyTable::get_icon_for_kernel);
        }
      getters.push_back (&PrettyTable::get_icon_dummy);
    }

  Glib::RefPtr<Gdk::Texture> icon;

  for (size_t i = 0; not icon and i < getters.size (); ++i)
    {
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
