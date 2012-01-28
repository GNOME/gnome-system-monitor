## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPROCMAN_DATADIR=\""$(datadir)/procman/"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATADIR=\""$(datadir)"\" \
	@PROCMAN_CFLAGS@ \
	@SYSTEMD_CFLAGS@

bin_PROGRAMS = gnome-system-monitor

gnome_system_monitor_cpp_files = \
	argv.cpp \
	procman.cpp \
	interface.cpp \
	callbacks.cpp \
	load-graph.cpp \
	proctable.cpp \
	prettytable.cpp \
	util.cpp \
	procactions.cpp \
	procdialogs.cpp \
	memmaps.cpp \
	openfiles.cpp \
	procproperties.cpp \
	smooth_refresh.cpp \
	disks.cpp \
	selinux.cpp \
	cgroups.cpp \
	procman_gnomesu.cpp \
	procman_gksu.cpp \
	sysinfo.cpp \
	lsof.cpp \
	selection.cpp \
	settings-keys.cpp \
	iconthemewrapper.cpp

gnome_system_monitor_c_files = \
	e_date.c \
	gsm_color_button.c

gnome_system_monitor_h_files = \
	$(gnome_system_monitor_cpp_files:.cpp=.h) \
	$(gnome_system_monitor_c_files:.c=.h) \
	defaulttable.h

gnome_system_monitor_SOURCES = \
  $(gnome_system_monitor_h_files) \
  $(gnome_system_monitor_cpp_files) \
  $(gnome_system_monitor_c_files)

gnome_system_monitor_LDADD = @PROCMAN_LIBS@ @SYSTEMD_LIBS@ libbacon.la

gnome_system_monitor_CPPFLAGS = -DGSM_DATA_DIR=\""$(pkgdatadir)"\"

noinst_LTLIBRARIES = libbacon.la
libbacon_la_SOURCES = \
	bacon-message-connection.c \
	bacon-message-connection.h

specdir = $(datadir)/procman

# GSettings schemas, enum files and conversion file
gsettings_ENUM_NAMESPACE = org.gnome.gnome-system-monitor
gsettings_ENUM_FILES = $(top_srcdir)/src/*.h
gsettings_in_file = org.gnome.gnome-system-monitor.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

dist_noinst_DATA = \
	$(gsettings_in_file)

CLEANFILES = \
	$(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)
