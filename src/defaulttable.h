#ifndef _PROCMAN_DEFAULTTABLE_H_
#define _PROCMAN_DEFAULTTABLE_H_

#include <string>
#include <tr1/memory>
#include <pcrecpp.h>

/* This file contains prettynames and icons for well-known applications, that by default has no .desktop entry */

struct PrettyTableItem
{
  std::tr1::shared_ptr<pcrecpp::RE> command;
  std::string icon;
};

#define ITEM(CMD, ICON) { std::tr1::shared_ptr<pcrecpp::RE>(new pcrecpp::RE((CMD "$"))), (ICON) }

/* The current table is only a test */
static const PrettyTableItem default_table[] = {
	ITEM("(ba|z|tc|c|k)sh", "utilities-terminal"),
	ITEM("(k|sys|u)logd|logger", "internet-news-reader"),
	ITEM("X(org)?", "display"),
	ITEM("(ksoftirqd|aio|events|kondemand|reiserfs|kswapd|kblockd)/?\\d+", "applications-system"),
	ITEM("kthread|khelper|khubd|kfand|ksuspend_usbd|pdflush", "applications-system"),
	ITEM("apache2?|httpd|lighttpd", "internet-web-browser"),
	ITEM("applet(-?2)?", "gnome-applets"),
	ITEM("atd|cron|CRON|ntpd", "date"),
	ITEM("cupsd|lpd?", "printer"),
	ITEM("cvsd|mtn|git|svn", "file-manager"),
	ITEM("emacs(server|\\d+)?", "gnome-emacs"),
	ITEM("evolution.*", "internet-mail"),
	ITEM("famd|gam_server", "file-manager"),
	ITEM("gconfd-2", "preferences-desktop"),
	ITEM("getty", "input-keyboard"),
	ITEM("metacity", "gnome-window-manager"),
	ITEM("sendmail|exim\\d?", "internet-mail"),
	ITEM("squid", "proxy"),
	ITEM("ssh(d|-agent)", "ssh-askpass-gnome"),
	ITEM("top|vmstat", "system-monitor"),
	ITEM("vim?", "vim"),
	ITEM("x?inetd", "internet-web-browser"),
	ITEM("vino.*", "gnome-remote-desktop")
};

#undef ITEM

#endif /* _PROCMAN_DEFAULTTABLE_H_ */
