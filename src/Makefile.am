## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPROCMAN_DATADIR=\""$(datadir)/procman/"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATADIR=\""$(datadir)"\" \
	@PROCMAN_CFLAGS@

bin_PROGRAMS = gnome-system-monitor

gnome_system_monitor_SOURCES = \
	argv.h argv.cpp \
	procman.cpp  procman.h \
	interface.cpp interface.h \
	callbacks.cpp callbacks.h \
	load-graph.cpp load-graph.h \
	proctable.cpp proctable.h \
	prettytable.cpp prettytable.h \
	util.cpp util.h \
	procactions.cpp procactions.h \
	procdialogs.cpp procdialogs.h \
	memmaps.cpp memmaps.h \
	openfiles.cpp openfiles.h \
	smooth_refresh.cpp smooth_refresh.h \
	defaulttable.h \
	disks.cpp disks.h \
	selinux.h selinux.cpp \
	procman_gnomesu.h procman_gnomesu.cpp \
	procman_gksu.h procman_gksu.cpp \
	sysinfo.cpp sysinfo.h \
	lsof.cpp lsof.h \
	selection.cpp selection.h \
	gconf-keys.cpp gconf-keys.h \
	iconthemewrapper.cpp iconthemewrapper.h \
	e_date.c e_date.h \
        gsm_color_button.c gsm_color_button.h


gnome_system_monitor_LDADD = @PROCMAN_LIBS@ libbacon.la


noinst_LTLIBRARIES = libbacon.la
libbacon_la_SOURCES = \
	bacon-message-connection.c \
	bacon-message-connection.h


specdir = $(datadir)/procman

schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schema_ins  = gnome-system-monitor.schemas.in
schema_DATA = $(schema_ins:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST = \
	$(schema_ins)

CLEANFILES = \
	$(schema_DATA)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)"; then \
	  for p in $(schema_DATA); do \
	    GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	      gconftool-2 --makefile-install-rule $(top_builddir)/src/$$p; \
	  done; \
	fi
else
install-data-local:
endif
