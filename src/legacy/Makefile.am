AM_CPPFLAGS = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DGSM_LIBEXEC_DIR=\""$(pkglibexecdir)"\" \
	$(GSM_LEGACY_CFLAGS) \
	$(SYSTEMD_CFLAGS) \
	$(WNCK_CFLAGS)

BUILT_SOURCES = gsm-resources.c

libgsmlegacy_c_files = \
	e_date.c \
	gsm_color_button.c \
	treeview.c

libgsmlegacy_h_files = \
	$(libgsmlegacy_c_files:.c=.h)

noinst_LTLIBRARIES = libgsmlegacy.la
libgsmlegacy_la_SOURCES = \
	$(libgsmlegacy_c_files) \
	$(libgsmlegacy_h_files) \
	$(BUILT_SOURCES)

gsm-resources.c: $(top_srcdir)/src/gsm.gresource.xml $(shell glib-compile-resources --sourcedir=$(top_srcdir) --generate-dependencies $(top_srcdir)/src/gsm.gresource.xml)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(top_srcdir) --generate-source $(top_srcdir)/src/gsm.gresource.xml



CLEANFILES = \
	$(BUILT_SOURCES)

-include $(top_srcdir)/git.mk

