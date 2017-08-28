## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = po scripts src help

ui_files = \
	data/preferences.ui \
	data/renice.ui \
	data/lsof.ui \
	data/openfiles.ui \
	data/interface.ui \
	data/menus.ui

pixmap_files = \
	pixmaps/download.svg \
	pixmaps/upload.svg

org.gnome.gnome-system-monitor.policy.in: org.gnome.gnome-system-monitor.policy.in.in Makefile
	$(AM_V_GEN) sed -e "s|\@pkglibexecdir\@|$(pkglibexecdir)|" $< > $@

EXTRA_DIST = \
	$(ui_files) \
	$(appdata_in_files) \
	$(pixmap_files) \
	org.gnome.gnome-system-monitor.policy.in.in \
	gnome-system-monitor.desktop.in \
	gnome-system-monitor-kde.desktop.in \
	gnome-system-monitor.doap

polkit_policydir = $(datadir)/polkit-1/actions
polkit_policy_in_files = org.gnome.gnome-system-monitor.policy.in
polkit_policy_DATA = $(polkit_policy_in_files:.policy.in=.policy)
$(polkit_policy_DATA): %.policy: %.policy.in
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-system-monitor.desktop.in gnome-system-monitor-kde.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)

appdatadir = $(datadir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = gnome-system-monitor.appdata.xml.in
$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

distuninstallcheck_listfiles = find . -type f -print | grep -v 'scrollkeeper'

if MAINTAINER_MODE
dist-hook: dist-changelog
else
dist-hook:
endif

.PHONY: dist-changelog

dist-changelog:
	$(AM_V_at)if git --git-dir=$(top_srcdir)/.git --work-tree=$(top_srcdir) \
		log --no-merges --date=short --pretty='tformat:%cd  %an  <%ae>%n%n%s%n%n%b' a5b4b868024f89e07fb9e1bf35d94df60568d841.. | \
	 $(SED)	-e '/^[12]...-[01].-[0123].  [^<>]*  <[^<>]*>$$/,/^$$/ b' \
		-e '/[^	 ]/,/^[	 ]*$$/ !d' \
		-e 's/^[	 ]*/	/' \
		-e 's/^[	 ]*$$//' >.ChangeLog.tmp; \
	then mv -f .ChangeLog.tmp "$(top_distdir)/ChangeLog"; \
	else rm -f .ChangeLog.tmp; exit 1; fi

dist_noinst_DATA = \
	ChangeLog.pre-git

check-local: $(appdata_DATA) $(Applications_DATA) \
             $(APPDATA_VALIDATE) $(appdata_DATA) \
             $(DESKTOP_FILE_VALIDATE) $(Applications_DATA)

DISTCLEANFILES = \
    $(appdata_DATA) \
	gnome-system-monitor.desktop  \
	gnome-system-monitor-kde.desktop  \
	org.gnome.gnome-system-monitor.policy \
    org.gnome.gnome-system-monitor.policy.in 

GITIGNOREFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
    *.make \
    m4

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

-include $(top_srcdir)/git.mk
