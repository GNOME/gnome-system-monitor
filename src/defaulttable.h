#ifndef _GSM_DEFAULT_TABLE_H_
#define _GSM_DEFAULT_TABLE_H_

#include <string>
#include <glibmm/refptr.h>
#include <glibmm/regex.h>

/* This file contains prettynames and icons for well-known applications, that by default has no .desktop entry */

struct PrettyTableItem
{
  Glib::RefPtr<Glib::Regex> command;
  std::string icon;

  PrettyTableItem (const std::string &a_command,
                   const std::string &a_icon)
    : command (Glib::Regex::create (("^(" + a_command + ")$").c_str ())),
    icon (a_icon)
  {
  }
};

#define ITEM PrettyTableItem

static const PrettyTableItem default_table[] = {
  /* GNOME services */
  ITEM (".*applet(-?2)?|gnome-panel", "gnome-panel"),
  ITEM ("evolution.*", "emblem-mail"),
  ITEM ("gconfd-2|dconf-service", "preferences-desktop"),
  ITEM ("metacity|gnome-shell", "gnome-window-manager"),
  ITEM ("vino.*", "gnome-remote-desktop"),
  /* Other processes */
  ITEM ("(ba|z|tc|c|k)?sh", "utilities-terminal"),
  ITEM ("(k|sys|u)logd|logger", "internet-news-reader"),
  ITEM ("X(org)?", "display"),
  ITEM ("apache2?|httpd|lighttpd", "internet-web-browser"),
  ITEM ("atd|cron|CRON|ntpd", "date"),
  ITEM ("cupsd|lpd?", "printer"),
  ITEM ("cvsd|mtn|git|svn", "file-manager"),
  ITEM ("emacs(server|\\d+)?", "gnome-emacs"),
  ITEM ("famd|gam_server", "file-manager"),
  ITEM ("getty", "input-keyboard"),
  ITEM ("gdb|((gcc|g\\+\\+)(-.*)?)|ar|ld|make", "applications-development"),
  ITEM ("sendmail|exim\\d?", "internet-mail"),
  ITEM ("squid", "proxy"),
  ITEM ("ssh(d|-agent)", "ssh-askpass-gnome"),
  ITEM ("top|vmstat", "system-monitor"),
  ITEM ("vim?", "vim"),
  ITEM ("x?inetd", "internet-web-browser")
};

#undef ITEM

#endif /* _GSM_DEFAULT_TABLE_H_ */
