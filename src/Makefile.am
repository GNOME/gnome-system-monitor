## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	-DPROCMAN_DATADIR=\"$(datadir)/procman/\" \
	@PROCMAN_CFLAGS@ \
	$(LIBGTOP_INCS)

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
	favorites.c favorites.h 

procman_LDADD =  \
	@PROCMAN_LIBS@ $(INTLLIBS) $(LIBGTOP_LIBS) 

specdir = $(datadir)/procman

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = \
	procman.png
	
EXTRA_DIST = \
	README.hacking \
	$(pixmap_DATA)
