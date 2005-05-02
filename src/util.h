#ifndef _PROCMAN_UTIL_H_
#define _PROCAMN_UTIL_H_

#include <glib.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <stddef.h>

void _procman_array_gettext_init(const char * strings[], size_t n) G_GNUC_INTERNAL;

#define PROCMAN_GETTEXT_ARRAY_INIT(A) G_STMT_START { \
static gboolean is_init = FALSE; \
if(!is_init) { \
  _procman_array_gettext_init((A), G_N_ELEMENTS(A)); \
  is_init = TRUE; \
 } \
} G_STMT_END

#define PROCMAN_CMP(X, Y) (((X) == (Y)) ? 0 : (((X) < (Y)) ? -1 : 1))


GtkWidget*
procman_make_label_for_mmaps_or_ofiles(const char *format,
					     const char *process_name,
					     unsigned pid) G_GNUC_INTERNAL;


gchar*
SI_gnome_vfs_format_file_size_for_display (GnomeVFSFileSize size) G_GNUC_INTERNAL;


#endif /* _PROCAMN_UTIL_H_ */
