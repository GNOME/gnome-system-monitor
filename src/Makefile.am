## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPROCMAN_DATADIR=\"$(datadir)/procman/\" \
	@PROCMAN_CFLAGS@ -DGTK_DISABLE_DEPRECATED=1 \
	-DGNOME_DISABLE_DEPRECATED

bin_PROGRAMS = procman

procman_SOURCES = \
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
	favorites.c favorites.h \
	defaulttable.h

procman_LDADD = @PROCMAN_LIBS@

specdir = $(datadir)/procman

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = \
	procman.png

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = procman.schemas	

EXTRA_DIST = \
	README.hacking \
	$(pixmap_DATA) \
	$(schema_DATA)

install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	gconftool-2 --makefile-install-rule $(schema_DATA)	  	
