// -*- c++ -*-

#ifndef _PROCMAN_PRETTYTABLE_H_
#define _PROCMAN_PRETTYTABLE_H_

#include <gtkmm/icontheme.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtkmm.h>

#include <map>
#include <string>

extern "C" {
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
}


using std::string;


class IconThemeWrapper
{
 public:
  IconThemeWrapper(Glib::RefPtr<Gtk::IconTheme> a_theme = Gtk::IconTheme::get_default())
    : theme(a_theme)
  { }

  Glib::RefPtr<Gdk::Pixbuf>
  load_icon(const Glib::ustring& icon_name, int size, Gtk::IconLookupFlags flags) const;

  const IconThemeWrapper* operator->() const
  { return this; }

 private:
  Glib::RefPtr<Gtk::IconTheme> theme;
};


class PrettyTable
{
 public:
  PrettyTable();
  ~PrettyTable();

  Glib::RefPtr<Gdk::Pixbuf> get_icon(const gchar* command, pid_t pid);

private:

  static void on_application_opened(WnckScreen* screen, WnckApplication* app, gpointer data);
  static void on_application_closed(WnckScreen* screen, WnckApplication* app, gpointer data);

  void register_application(pid_t pid, Glib::RefPtr<Gdk::Pixbuf> icon);
  void unregister_application(pid_t pid);


  Glib::RefPtr<Gdk::Pixbuf> get_icon_from_theme(pid_t pid, const gchar* command);
  Glib::RefPtr<Gdk::Pixbuf> get_icon_from_default(pid_t pid, const gchar* command);
  Glib::RefPtr<Gdk::Pixbuf> get_icon_from_wnck(pid_t pid, const gchar* command);
  Glib::RefPtr<Gdk::Pixbuf> get_icon_from_name(pid_t pid, const gchar* command);
  Glib::RefPtr<Gdk::Pixbuf> get_icon_dummy(pid_t pid, const gchar* command);

  bool get_default_icon_name(const string &cmd, string &name);

  typedef std::map<string, Glib::RefPtr<Gdk::Pixbuf> > IconCache;
  typedef std::map<pid_t, Glib::RefPtr<Gdk::Pixbuf> > IconsForPID;

  IconsForPID apps;
  IconCache defaults;
  IconThemeWrapper theme;
};


#endif /* _PROCMAN_PRETTYTABLE_H_ */
