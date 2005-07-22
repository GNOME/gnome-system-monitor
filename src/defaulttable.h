#ifndef _PROCMAN_DEFAULTTABLE_H_
#define _PROCMAN_DEFAULTTABLE_H_

/* This file contains prettynames and icons for well-known applications, that by default has no .desktop entry */

typedef struct _PrettyTableItem PrettyTableItem;

struct _PrettyTableItem
{
	const char *command;
	const char *icon;
};

/* The current table is only a test */
static const PrettyTableItem default_table[] = {
/* "0", "gnome-run.png", */
	{"X", "gnome-mdi.png"},
	{"Xorg", "gnome-mdi.png"},

	{"aio" G_DIR_SEPARATOR_S "0", "gnome-run.png"},
	{"aio" G_DIR_SEPARATOR_S "1", "gnome-run.png"},
	{"aio" G_DIR_SEPARATOR_S "2", "gnome-run.png"},
	{"aio" G_DIR_SEPARATOR_S "3", "gnome-run.png"},

	{"apache", "other" G_DIR_SEPARATOR_S "SMB-Server-alt.png"},
	{"apache2", "other" G_DIR_SEPARATOR_S "SMB-Server-alt.png"},

	{"atd", "gnome-set-time.png"},

	{"bash", "gnome-term.png"},

	{"cron", "gnome-set-time.png"},
	{"CRON", "gnome-set-time.png"},

	{"cupsd", "other" G_DIR_SEPARATOR_S "Printer.png"},
	{"cvsd",  "other" G_DIR_SEPARATOR_S "SMB-Server-alt.png"},
	{"dbus-daemon-1", "other" G_DIR_SEPARATOR_S "Reading.png"},

	{"emacs", "gnome-emacs.png"},
	{"emacsserver", "gnome-emacs.png"},
	{"emacs21", "gnome-emacs.png"},

	{"events" G_DIR_SEPARATOR_S "0", "other" G_DIR_SEPARATOR_S "Dialog-Warning5.png"},
	{"events" G_DIR_SEPARATOR_S "1", "other" G_DIR_SEPARATOR_S "Dialog-Warning5.png"},
	{"events" G_DIR_SEPARATOR_S "2", "other" G_DIR_SEPARATOR_S "Dialog-Warning5.png"},
	{"events" G_DIR_SEPARATOR_S "3", "other" G_DIR_SEPARATOR_S "Dialog-Warning5.png"},

	{"evolution-alarm-notify", "other" G_DIR_SEPARATOR_S "Button-Apps.png"},
	{"evolution-wombat", "other" G_DIR_SEPARATOR_S "Button-Apps.png"},

	{"exim4", "other" G_DIR_SEPARATOR_S "Envelope.png"},

	{"famd", "other" G_DIR_SEPARATOR_S "Find-Files2.png"},
	{"gam_server", "other" G_DIR_SEPARATOR_S "Find-Files2.png"},

	{"gconfd-2", "other" G_DIR_SEPARATOR_S "Control-Center2.png"},
	{"gdm", "gdm.png"},
	{"getty", "gksu-icon.png"},
	{"gnome-session", "gnome-logo-icon-transparent.png"},
	{"inetd", "other" G_DIR_SEPARATOR_S "Networking.png"},

	{"logger", "gnome-log.png"},

	{"kblockd" G_DIR_SEPARATOR_S "0", "other" G_DIR_SEPARATOR_S "Disks.png"},
	{"kblockd" G_DIR_SEPARATOR_S "1", "other" G_DIR_SEPARATOR_S "Disks.png"},
	{"kblockd" G_DIR_SEPARATOR_S "2", "other" G_DIR_SEPARATOR_S "Disks.png"},
	{"kblockd" G_DIR_SEPARATOR_S "3", "other" G_DIR_SEPARATOR_S "Disks.png"},

	/* "khelper", "other" G_DIR_SEPARATOR_S  "Help.png", */
	{"kirqd", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png"},
	{"klogd", "gnome-log.png"},

	{"ksoftirqd" G_DIR_SEPARATOR_S "0", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png"},
	{"ksoftirqd" G_DIR_SEPARATOR_S "1", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png"},
	{"ksoftirqd" G_DIR_SEPARATOR_S "2", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png"},
	{"ksoftirqd" G_DIR_SEPARATOR_S "3", "other" G_DIR_SEPARATOR_S "Connection-Ethernet.png"},

	/* "kswapd0", "other" G_DIR_SEPARATOR_S "Harddisk.png", */

	{"mc", "mc.xpm"},

	{"metacity", "metacity-properties.png"},

	{"migration" G_DIR_SEPARATOR_S "0" ,"other" G_DIR_SEPARATOR_S "Bird.png"},
	{"migration" G_DIR_SEPARATOR_S "1" ,"other" G_DIR_SEPARATOR_S "Bird.png"},
	{"migration" G_DIR_SEPARATOR_S "2" ,"other" G_DIR_SEPARATOR_S "Bird.png"},
	{"migration" G_DIR_SEPARATOR_S "3" ,"other" G_DIR_SEPARATOR_S "Bird.png"},

	{"mysqld", "other" G_DIR_SEPARATOR_S "MySQL.png"},
	{"mutt", "mutt.xpm"},
	{"pdflush", "gnome-run.png"},
	{"portmap","other" G_DIR_SEPARATOR_S "Connection-Ethernet.png"},
	{"powernowd", "battstat.png"},

	{"pppoe", "other" G_DIR_SEPARATOR_S "Modem.png"},

	{"reiserfs" G_DIR_SEPARATOR_S "0","other" G_DIR_SEPARATOR_S "Disks.png"},
	{"reiserfs" G_DIR_SEPARATOR_S "1","other" G_DIR_SEPARATOR_S "Disks.png"},
	{"reiserfs" G_DIR_SEPARATOR_S "2","other" G_DIR_SEPARATOR_S "Disks.png"},
	{"reiserfs" G_DIR_SEPARATOR_S "3","other" G_DIR_SEPARATOR_S "Disks.png"},

	{"sendmail", "other" G_DIR_SEPARATOR_S "Envelope.png"},
	{"setiathome", "other" G_DIR_SEPARATOR_S "WPicon2.png"},
	{"sh", "gnome-term.png"},
	{"squid", "other" G_DIR_SEPARATOR_S "Proxy-Config.png"},

	{"sshd", "ssh-askpass-gnome.png"},
	{"ssh", "ssh-askpass-gnome.png"},
	{"ssh-agent", "ssh-askpass-gnome.png"},

	{"syslogd", "gnome-log.png"},

	{"tail", "other" G_DIR_SEPARATOR_S "Tail.png"},

	{"top", "gnome-ccperiph.png"},

	{"xfs", "other" G_DIR_SEPARATOR_S "Font-Capplet.png"},
	{"xscreensaver", "xscreensaver.xpm"},
	{"xterm", "gnome-xterm.png"},

	{"vim", "vim.svg"},


	/* GNOME applets */

	{"accessx-status-applet",	"gnome-applets.png"},
	{"battstat-applet-2",		"gnome-applets.png"},
	{"charpick_applet2",		"gnome-applets.png"},
	{"cpufreq-applet",		"gnome-applets.png"},
	{"drivemount_applet2",		"gnome-applets.png"},
	{"geyes_applet2",		"gnome-applets.png"},
	{"gnome-keyboard-applet",       "gnome-applets.png"},
	{"gtik2_applet2",		"gnome-applets.png"},
	{"gweather-applet-2",		"gnome-applets.png"},
	{"mini_commander_applet",       "gnome-applets.png"},
	{"mixer_applet2",		"gnome-applets.png"},
	{"modem_applet",		"gnome-applets.png"},
	{"multiload-applet-2",		"gnome-applets.png"},
	{"netspeed-applet-2",		"gnome-applets.png"},
	{"null_applet",			"gnome-applets.png"},
	{"rhythmbox-applet",		"gnome-applets.png"},
	{"stickynotes_applet",		"gnome-applets.png"},
	{"trashapplet",			"gnome-applets.png"},

	{"clock-applet",		"gnome-applets.png"},
	{"notification-area-applet",	"gnome-applets.png"},
	{"wnck-applet",			"gnome-applets.png"}
};

#endif /* _PROCMAN_DEFAULTTABLE_H_ */
