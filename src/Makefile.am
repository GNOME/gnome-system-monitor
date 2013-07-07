## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPROCMAN_DATADIR=\""$(datadir)/procman/"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATADIR=\""$(datadir)"\" \
	@PROCMAN_CFLAGS@ \
	@SYSTEMD_CFLAGS@

bin_PROGRAMS = gnome-system-monitor nextmon

nextmon_c_files = \
    sysmon.c \
    system-monitor.c

nextmon_h_files = \
    sysmon.h

nextmon_SOURCES = \
    $(nextmon_h_files) \
    $(nextmon_c_files)

nextmon_LDADD = @PROCMAN_LIBS@ $(top_builddir)/uber/libuber-1.0.la -ldl

nextmon_CPPFLAGS = \
	-I$(top_srcdir)	\
	-I$(top_builddir)/uber \
	-DGSM_DATA_DIR=\""$(pkgdatadir)"\"

gnome_system_monitor_cpp_files = \
	argv.cpp \
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
	procman_pkexec.cpp \
	lsof.cpp \
	settings-keys.cpp \
	iconthemewrapper.cpp \
	procman-app.cpp

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
  $(gnome_system_monitor_c_files) \
  procman.cpp

gnome_system_monitor_LDADD = @PROCMAN_LIBS@ @SYSTEMD_LIBS@

gnome_system_monitor_CPPFLAGS = -DGSM_DATA_DIR=\""$(pkgdatadir)"\" \
                                -DGSM_LIBEXEC_DIR=\""$(pkglibexecdir)"\"

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
