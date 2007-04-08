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
/* "0", "gnome-run", */
	{"X", "gnome-mdi"},
	{"Xorg", "gnome-mdi"},

	{"aio" G_DIR_SEPARATOR_S "0", "gnome-run"},
	{"aio" G_DIR_SEPARATOR_S "1", "gnome-run"},
	{"aio" G_DIR_SEPARATOR_S "2", "gnome-run"},
	{"aio" G_DIR_SEPARATOR_S "3", "gnome-run"},

	{"apache", "other" G_DIR_SEPARATOR_S "SMB-Server-alt"},
	{"apache2", "other" G_DIR_SEPARATOR_S "SMB-Server-alt"},

	{"atd", "gnome-set-time"},

	{"bash", "gnome-term"},

	{"cron", "gnome-set-time"},
	{"CRON", "gnome-set-time"},

	{"cupsd", "other" G_DIR_SEPARATOR_S "Printer"},
	{"cvsd",  "other" G_DIR_SEPARATOR_S "SMB-Server-alt"},
	{"dbus-daemon-1", "other" G_DIR_SEPARATOR_S "Reading"},

	{"emacs", "gnome-emacs"},
	{"emacsserver", "gnome-emacs"},
	{"emacs21", "gnome-emacs"},

	{"events" G_DIR_SEPARATOR_S "0", "other" G_DIR_SEPARATOR_S "Dialog-Warning5"},
	{"events" G_DIR_SEPARATOR_S "1", "other" G_DIR_SEPARATOR_S "Dialog-Warning5"},
	{"events" G_DIR_SEPARATOR_S "2", "other" G_DIR_SEPARATOR_S "Dialog-Warning5"},
	{"events" G_DIR_SEPARATOR_S "3", "other" G_DIR_SEPARATOR_S "Dialog-Warning5"},

	{"evolution-alarm-notify", "other" G_DIR_SEPARATOR_S "Button-Apps"},
	{"evolution-wombat", "other" G_DIR_SEPARATOR_S "Button-Apps"},

	{"exim4", "other" G_DIR_SEPARATOR_S "Envelope"},

	{"famd", "other" G_DIR_SEPARATOR_S "Find-Files2"},
	{"gam_server", "other" G_DIR_SEPARATOR_S "Find-Files2"},

	{"gconfd-2", "other" G_DIR_SEPARATOR_S "Control-Center2"},
	{"gdm", "gdm"},
	{"getty", "gksu-icon"},
	{"gnome-session", "gnome-logo-icon-transparent"},
	{"inetd", "other" G_DIR_SEPARATOR_S "Networking"},

	{"logger", "gnome-log"},

	{"kblockd" G_DIR_SEPARATOR_S "0", "other" G_DIR_SEPARATOR_S "Disks"},
	{"kblockd" G_DIR_SEPARATOR_S "1", "other" G_DIR_SEPARATOR_S "Disks"},
	{"kblockd" G_DIR_SEPARATOR_S "2", "other" G_DIR_SEPARATOR_S "Disks"},
	{"kblockd" G_DIR_SEPARATOR_S "3", "other" G_DIR_SEPARATOR_S "Disks"},

	/* "khelper", "other" G_DIR_SEPARATOR_S  "Help", */
	{"kirqd", "other" G_DIR_SEPARATOR_S "Connection-Ethernet"},
	{"klogd", "gnome-log"},

	{"ksoftirqd" G_DIR_SEPARATOR_S "0", "other" G_DIR_SEPARATOR_S "Connection-Ethernet"},
	{"ksoftirqd" G_DIR_SEPARATOR_S "1", "other" G_DIR_SEPARATOR_S "Connection-Ethernet"},
	{"ksoftirqd" G_DIR_SEPARATOR_S "2", "other" G_DIR_SEPARATOR_S "Connection-Ethernet"},
	{"ksoftirqd" G_DIR_SEPARATOR_S "3", "other" G_DIR_SEPARATOR_S "Connection-Ethernet"},

	/* "kswapd0", "other" G_DIR_SEPARATOR_S "Harddisk", */

	{"mc", "mc"},

	{"metacity", "metacity-properties"},

	{"migration" G_DIR_SEPARATOR_S "0" ,"other" G_DIR_SEPARATOR_S "Bird"},
	{"migration" G_DIR_SEPARATOR_S "1" ,"other" G_DIR_SEPARATOR_S "Bird"},
	{"migration" G_DIR_SEPARATOR_S "2" ,"other" G_DIR_SEPARATOR_S "Bird"},
	{"migration" G_DIR_SEPARATOR_S "3" ,"other" G_DIR_SEPARATOR_S "Bird"},

	{"mysqld", "other" G_DIR_SEPARATOR_S "MySQL"},
	{"mutt", "mutt"},
	{"pdflush", "gnome-run"},
	{"portmap","other" G_DIR_SEPARATOR_S "Connection-Ethernet"},
	{"powernowd", "battstat"},

	{"pppoe", "other" G_DIR_SEPARATOR_S "Modem"},

	{"reiserfs" G_DIR_SEPARATOR_S "0","other" G_DIR_SEPARATOR_S "Disks"},
	{"reiserfs" G_DIR_SEPARATOR_S "1","other" G_DIR_SEPARATOR_S "Disks"},
	{"reiserfs" G_DIR_SEPARATOR_S "2","other" G_DIR_SEPARATOR_S "Disks"},
	{"reiserfs" G_DIR_SEPARATOR_S "3","other" G_DIR_SEPARATOR_S "Disks"},

	{"sendmail", "other" G_DIR_SEPARATOR_S "Envelope"},
	{"setiathome", "other" G_DIR_SEPARATOR_S "WPicon2"},
	{"sh", "gnome-term"},
	{"squid", "other" G_DIR_SEPARATOR_S "Proxy-Config"},

	{"sshd", "ssh-askpass-gnome"},
	{"ssh", "ssh-askpass-gnome"},
	{"ssh-agent", "ssh-askpass-gnome"},

	{"syslogd", "gnome-log"},

	{"tail", "other" G_DIR_SEPARATOR_S "Tail"},

	{"top", "gnome-ccperiph"},

	{"xfs", "other" G_DIR_SEPARATOR_S "Font-Capplet"},
	{"xscreensaver", "xscreensaver"},
	{"xterm", "gnome-xterm"},

	{"vim", "vim"},


	/* GNOME applets */

	{"accessx-status-applet",	"gnome-applets"},
	{"battstat-applet-2",		"gnome-applets"},
	{"charpick_applet2",		"gnome-applets"},
	{"cpufreq-applet",		"gnome-applets"},
	{"drivemount_applet2",		"gnome-applets"},
	{"geyes_applet2",		"gnome-applets"},
	{"gnome-keyboard-applet",       "gnome-applets"},
	{"gtik2_applet2",		"gnome-applets"},
	{"gweather-applet-2",		"gnome-applets"},
	{"mini_commander_applet",       "gnome-applets"},
	{"mixer_applet2",		"gnome-applets"},
	{"modem_applet",		"gnome-applets"},
	{"multiload-applet-2",		"gnome-applets"},
	{"netspeed-applet-2",		"gnome-applets"},
	{"null_applet",			"gnome-applets"},
	{"rhythmbox-applet",		"gnome-applets"},
	{"stickynotes_applet",		"gnome-applets"},
	{"trashapplet",			"gnome-applets"},

	{"clock-applet",		"gnome-applets"},
	{"notification-area-applet",	"gnome-applets"},
	{"wnck-applet",			"gnome-applets"}
};

#endif /* _PROCMAN_DEFAULTTABLE_H_ */
