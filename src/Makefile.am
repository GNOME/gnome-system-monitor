## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPROCMAN_DATADIR=\"$(datadir)/procman/\" \
	-DDATADIR=\""$(datadir)"\" \
	@PROCMAN_CFLAGS@

bin_PROGRAMS = gnome-system-monitor

gnome_system_monitor_SOURCES = \
	procman.c  procman.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	load-graph.c load-graph.h \
	proctable.c proctable.h \
	prettytable.c prettytable.h \
	util.c util.h \
	infoview.c infoview.h \
	procactions.c procactions.h \
	procdialogs.c procdialogs.h \
	memmaps.c memmaps.h \
	openfiles.c openfiles.h \
	favorites.c favorites.h \
	defaulttable.h

if ENABLE_SELINUX
gnome_system_monitor_LDADD = @PROCMAN_LIBS@ -lselinux
else
gnome_system_monitor_LDADD = @PROCMAN_LIBS@
endif

specdir = $(datadir)/procman

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_ins  = gnome-system-monitor.schemas.in
schema_DATA = $(schema_ins:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST = \
	README.hacking \
	$(schema_ins)

CLEANFILES = \
	$(schema_DATA)

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	gconftool-2 --makefile-install-rule $(schema_DATA)	  	
