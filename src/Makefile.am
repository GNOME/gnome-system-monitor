## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	-DPROCMAN_DATADIR=\"$(datadir)/procman/\" \
	$(GNOME_INCLUDEDIR) \
	$(LIBGTOP_INCS) \
	$(GAL_CFLAGS)

bin_PROGRAMS = procman

procman_SOURCES = \
	procman.c  procman.h\
	interface.c interface.h \
	callbacks.c callbacks.h \
	proctable.c proctable.h \
	infoview.c infoview.h \
	procdialogs.c procdialogs.h \
	prettytable.c prettytable.h \
	memmaps.c memmaps.h \
	favorites.c favorites.h \
	e-clipped-label.c e-clipped-label.h \
	procactions.c procactions.h \
	defaulttable.h

procman_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) \
	$(INTLLIBS) $(LIBGTOP_LIBS) $(GAL_LIBS) -lpthread

specdir = $(datadir)/procman
spec_DATA = \
	proctable.etspec \
	memmaps.etspec

EXTRA_DIST = \
	README.hacking \
	$(spec_DATA)
