AM_CPPFLAGS = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DGSM_LIBEXEC_DIR=\""$(pkglibexecdir)"\" \
	$(GSM_LEGACY_CFLAGS) \
	$(SYSTEMD_CFLAGS) \
	$(WNCK_CFLAGS)

libgsmlegacy_c_files = \
	e_date.c \
	gsm_color_button.c \
	treeview.c

libgsmlegacy_h_files = \
	$(libgsmlegacy_c_files:.c=.h)

noinst_LTLIBRARIES = libgsmlegacy.la
libgsmlegacy_la_SOURCES = \
	$(libgsmlegacy_c_files) \
	$(libgsmlegacy_h_files)


-include $(top_srcdir)/git.mk

