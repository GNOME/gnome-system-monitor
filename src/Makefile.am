## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPROCMAN_DATADIR=\"$(datadir)/procman/\" \
	-DDATADIR=\""$(datadir)"\" \
	@PROCMAN_CFLAGS@ $(LIBGKSU_CFLAGS)

bin_PROGRAMS = gnome-system-monitor

gnome_system_monitor_SOURCES = \
	procman.c  procman.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	load-graph.c load-graph.h \
	proctable.c proctable.h \
	prettytable.c prettytable.h \
	util.c util.h \
	procactions.c procactions.h \
	procdialogs.c procdialogs.h \
	memmaps.c memmaps.h \
	openfiles.c openfiles.h \
	favorites.c favorites.h \
	smooth_refresh.c smooth_refresh.h \
	defaulttable.h \
	disks.c disks.h \
	selinux.h selinux.c \
	procman_gnomesu.h procman_gnomesu.c \
	procman_gksu.h procman_gksu.c \
	e_date.c e_date.h


gnome_system_monitor_LDADD = @PROCMAN_LIBS@ $(SELINUX_LIBS) $(LIBGKSU_LIBS) libbacon.la 


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
