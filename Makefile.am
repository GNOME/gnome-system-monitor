## Process this file with automake to produce Makefile.in

SUBDIRS = intl po macros src

EXTRA_DIST = \
	procman.desktop.in \
	procman.spec \
	xml-i18n-extract.in \
	xml-i18n-merge.in \
	xml-i18n-update.in \
	HACKING 

Applicationsdir = $(datadir)/gnome/apps/System
Applications_in_files = procman.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@XML_I18N_MERGE_DESKTOP_RULE@

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

