#ifndef _DEFAULTTABLE_H_
#define _DEFAULTTABLE_H_

/* This file contains prettynames and icons for well-known applications, that by default has no .desktop entry */

/* The current table is only a test */
static const gchar * const default_table[] = {
	/* "0", "gnome-run.png", */
	"X", "gnome-mdi.png",

	"aio" G_DIR_SEPARATOR_S "0", "gnome-run.png",
	"aio" G_DIR_SEPARATOR_S "1", "gnome-run.png",
	"aio" G_DIR_SEPARATOR_S "2", "gnome-run.png",
	"aio" G_DIR_SEPARATOR_S "3", "gnome-run.png",

	"apache", "other" G_DIR_SEPARATOR_S "SMB-Server-alt.png",
	"apache2", "other" G_DIR_SEPARATOR_S "SMB-Server-alt.png",

	"atd", "gnome-set-time.png",

	"bash", "gnome-term.png",
	/* "-bash", "gnome-term.png", */

	/* "bonobo-activation-server", "other" G_DIR_SEPARATOR_S "Computer-Ximian_1.png", */

	"cron", "gnome-set-time.png",
	"CRON", "gnome-set-time.png",

	"cupsd", "other" G_DIR_SEPARATOR_S "Printer.png",
	/* "cvsd", "other" G_DIR_SEPARATOR_S "RDF2.png", */
	"dbus-daemon-1", "other" G_DIR_SEPARATOR_S "Reading.png",

	"emacs", "gnome-emacs.png",
	"emacsserver", "gnome-emacs.png",
	"emacs21", "gnome-emacs.png",

	"events" G_DIR_SEPARATOR_S "0", "other" G_DIR_SEPARATOR_S "Dialog-Warning5.png",
	"events" G_DIR_SEPARATOR_S "1", "other" G_DIR_SEPARATOR_S "Dialog-Warning5.png",
	"events" G_DIR_SEPARATOR_S "2", "other" G_DIR_SEPARATOR_S "Dialog-Warning5.png",
	"events" G_DIR_SEPARATOR_S "3", "other" G_DIR_SEPARATOR_S "Dialog-Warning5.png",

	"evolution-alarm-notify", "other" G_DIR_SEPARATOR_S "Button-Apps.png",
	"evolution-wombat", "other" G_DIR_SEPARATOR_S "Button-Apps.png",

	"exim4", "other" G_DIR_SEPARATOR_S "Envelope.png",
	/* "famd", "other" G_DIR_SEPARATOR_S "Button-GNOME.png", */
	"gconfd-2", "other" G_DIR_SEPARATOR_S "Control-Center2.png",
	"gdm", "gdm.png",
	"getty", "gksu-icon.png",
	"gnome-session", "gnome-logo-icon-transparent.png",
	"inetd", "other" G_DIR_SEPARATOR_S "Networking.png",
	/* "logger", "other" G_DIR_SEPARATOR_S "Harddisk.png", */

	"kblockd" G_DIR_SEPARATOR_S "0", "other" G_DIR_SEPARATOR_S "Disks.png",
	"kblockd" G_DIR_SEPARATOR_S "1", "other" G_DIR_SEPARATOR_S "Disks.png",
	"kblockd" G_DIR_SEPARATOR_S "2", "other" G_DIR_SEPARATOR_S "Disks.png",
	"kblockd" G_DIR_SEPARATOR_S "3", "other" G_DIR_SEPARATOR_S "Disks.png",

	/* "khelper", "other" G_DIR_SEPARATOR_S  "Help.png", */
	"kirqd", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png",
	/* "klogd", "other" G_DIR_SEPARATOR_S "Harddisk.png", */

	"ksoftirqd" G_DIR_SEPARATOR_S "0", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png",
	"ksoftirqd" G_DIR_SEPARATOR_S "1", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png",
	"ksoftirqd" G_DIR_SEPARATOR_S "2", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png",
	"ksoftirqd" G_DIR_SEPARATOR_S "3", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png",

	/* "kswapd0", "other" G_DIR_SEPARATOR_S "Harddisk.png", */
	"metacity", "metacity-properties.png",

	"migration" G_DIR_SEPARATOR_S "0" ,"other" G_DIR_SEPARATOR_S "Bird.png",
	"migration" G_DIR_SEPARATOR_S "1" ,"other" G_DIR_SEPARATOR_S "Bird.png",
	"migration" G_DIR_SEPARATOR_S "2" ,"other" G_DIR_SEPARATOR_S "Bird.png",
	"migration" G_DIR_SEPARATOR_S "3" ,"other" G_DIR_SEPARATOR_S "Bird.png",

	"mysqld", "other" G_DIR_SEPARATOR_S "MySQL.png",
	"pdflush", "gnome-run.png",
	"portmap","other" G_DIR_SEPARATOR_S "Connection-Ethernet.png",
	"powernowd", "battstat.png",
	/* "pppoe", "other" G_DIR_SEPARATOR_S "Globe.png", */

	"reiserfs" G_DIR_SEPARATOR_S "0","other" G_DIR_SEPARATOR_S "Disks.png",
	"reiserfs" G_DIR_SEPARATOR_S "1","other" G_DIR_SEPARATOR_S "Disks.png",
	"reiserfs" G_DIR_SEPARATOR_S "2","other" G_DIR_SEPARATOR_S "Disks.png",
	"reiserfs" G_DIR_SEPARATOR_S "3","other" G_DIR_SEPARATOR_S "Disks.png",

	"setiathome", "other" G_DIR_SEPARATOR_S "WPicon2.png",
	"sh", "gnome-term.png",
	"squid", "Proxy-Config.png",

	"sshd", "ssh-askpass-gnome.png",
	"ssh", "ssh-askpass-gnome.png",
	"ssh-agent", "ssh-askpass-gnome.png",

	"syslogd", "gnome-log.png",
	"tail", "other" G_DIR_SEPARATOR_S "Tail.png",
	/* "top", "other" G_DIR_SEPARATOR_S "Control-Center2.png", */
	"xfs", "other" G_DIR_SEPARATOR_S "Font-Capplet.png",
	"xscreensaver", "xscreensaver.xpm",
	"xterm", "gnome-xterm.png",
	NULL
};

#endif /* _DEFAULTTABLE_H_ */
