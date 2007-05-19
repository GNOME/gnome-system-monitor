#ifndef _PROCMAN_PRETTYTABLE_H_
#define _PROCMAN_PRETTYTABLE_H_

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <map>
#include <string>

extern "C" {
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
}


using std::string;


class PrettyTable
{
 public:
  PrettyTable();
  ~PrettyTable();

  // gets a new reference to GdkPixbuf*
  GdkPixbuf* get_icon(const gchar* command, pid_t pid);

private:

  static void on_application_opened(WnckScreen* screen, WnckApplication* app, gpointer data);
  static void on_application_closed(WnckScreen* screen, WnckApplication* app, gpointer data);

  void register_application(pid_t pid, GdkPixbuf* icon);
  void unregister_application(pid_t pid);


  // return a new reference to GdkPixbuf*
  GdkPixbuf* get_icon_from_theme(pid_t pid, const gchar* command);
  GdkPixbuf* get_icon_from_default(pid_t pid, const gchar* command);
  GdkPixbuf* get_icon_from_wnck(pid_t pid, const gchar* command);
  GdkPixbuf* get_icon_from_name(pid_t pid, const gchar* command);
  GdkPixbuf* get_icon_dummy(pid_t pid, const gchar* command);

  bool get_default_icon_name(const string &cmd, string &name);

  typedef std::map<string, GdkPixbuf*> IconCache;
  typedef std::map<pid_t, GdkPixbuf*> IconsForPID;

  IconsForPID apps;
  IconCache defaults;
  GtkIconTheme* theme;
};


#endif /* _PROCMAN_PRETTYTABLE_H_ */
