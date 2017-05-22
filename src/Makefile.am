## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DGSM_LIBEXEC_DIR=\""$(pkglibexecdir)"\" \
	-DGSM_RESOURCE_FILE=\""$(gsmsharedir)/$(gsmshare_DATA)"\" \
	$(GSM_CFLAGS) \
	$(SYSTEMD_CFLAGS) \
	$(WNCK_CFLAGS)

bin_PROGRAMS = gnome-system-monitor

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
	systemd.cpp \
	cgroups.cpp \
	gsm_gnomesu.cpp \
	gsm_gksu.cpp \
	gsm_pkexec.cpp \
	lsof.cpp \
	prefsdialog.cpp \
	application.cpp


gnome_system_monitor_h_files = \
	$(gnome_system_monitor_cpp_files:.cpp=.h) \
	defaulttable.h \
	settings-keys.h

gnome_system_monitor_SOURCES = \
	$(gnome_system_monitor_h_files) \
	$(gnome_system_monitor_cpp_files) \
	main.cpp

LDADD = \
	legacy/libgsmlegacy.la \
	$(GSM_LIBS) \
	$(SYSTEMD_LIBS) \
	$(WNCK_LIBS)


# GSettings schemas, enum files and conversion file
gsettings_ENUM_NAMESPACE = org.gnome.gnome-system-monitor
gsettings_ENUM_FILES = $(top_srcdir)/src/*.h $(top_srcdir)/src/legacy/*.h
gsettings_in_file = org.gnome.gnome-system-monitor.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

dist_noinst_DATA = \
	$(gsettings_in_file) \
	$(top_srcdir)/src/gsm.gresource.xml

gsm.gresource: $(top_srcdir)/src/gsm.gresource.xml $(shell glib-compile-resources --sourcedir=$(top_srcdir) --generate-dependencies $(top_srcdir)/src/gsm.gresource.xml)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(top_srcdir) $(top_srcdir)/src/gsm.gresource.xml


gsmsharedir = $(datadir)/@PACKAGE@
gsmshare_DATA = \
	gsm.gresource

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	$(gsmshare_DATA)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

SUBDIRS = legacy

-include $(top_srcdir)/git.mk
