## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DGSM_LIBEXEC_DIR=\""$(pkglibexecdir)"\" \
	$(PROCMAN_CFLAGS) \
	$(SYSTEMD_CFLAGS) \
	$(WNCK_CFLAGS)

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
        $(PROCMAN_CFLAGS) \
	-I$(top_srcdir)	\
	-I$(top_builddir)/uber \
	-DGSM_DATA_DIR=\""$(pkgdatadir)"\"

BUILT_SOURCES = gsm-resources.c

gnome_system_monitor_cpp_files = \
	argv.cpp \
	interface.cpp \
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
	$(BUILT_SOURCES) \
	$(gnome_system_monitor_h_files) \
	$(gnome_system_monitor_cpp_files) \
	$(gnome_system_monitor_c_files) \
	procman.cpp

LDADD = \
	$(PROCMAN_LIBS) \
	$(SYSTEMD_LIBS) \
	$(WNCK_LIBS)

gsm-resources.c: gsm.gresource.xml $(shell glib-compile-resources --sourcedir=$(top_srcdir) --generate-dependencies $(srcdir)/gsm.gresource.xml)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(top_srcdir) --generate-source $(srcdir)/gsm.gresource.xml


# GSettings schemas, enum files and conversion file
gsettings_ENUM_NAMESPACE = org.gnome.gnome-system-monitor
gsettings_ENUM_FILES = $(top_srcdir)/src/*.h
gsettings_in_file = org.gnome.gnome-system-monitor.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

dist_noinst_DATA = \
	$(gsettings_in_file)

EXTRA_DIST = \
	gsm.gresource.xml

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	$(BUILT_SOURCES)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)
