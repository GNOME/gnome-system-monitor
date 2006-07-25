## Process this file with automake to produce Makefile.in

SUBDIRS = po src help

EXTRA_DIST = \
	gnome-system-monitor.desktop.in \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	omf.make \
	xmldocs.make

Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-system-monitor.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

distuninstallcheck_listfiles = find . -type f -print | grep -v 'scrollkeeper'

DISTCLEANFILES = \
  intltool-extract \
  intltool-merge \
  intltool-update \
  gnome-system-monitor.desktop
