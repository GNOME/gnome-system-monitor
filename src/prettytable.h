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


  void load_default_table();

  // return a new reference to GdkPixbuf*
  GdkPixbuf* get_icon_from_theme(pid_t pid, const gchar* command);
  GdkPixbuf* get_icon_from_default(pid_t pid, const gchar* command);
  GdkPixbuf* get_icon_from_wnck(pid_t pid, const gchar* command);


  typedef std::map<string, GdkPixbuf*> IconsForCommand;
  typedef std::map<pid_t, GdkPixbuf*> IconsForPID;

  IconsForPID apps;
  IconsForCommand defaults;
  GtkIconTheme* theme;
};


#endif /* _PROCMAN_PRETTYTABLE_H_ */
