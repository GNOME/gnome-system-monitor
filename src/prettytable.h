#ifndef _PROCMAN_PRETTYTABLE_H_
#define _PROCMAN_PRETTYTABLE_H_

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "procman.h"


void*		prettytable_load_async (void *data) G_GNUC_INTERNAL;
void		pretty_table_new (ProcData *procdata) G_GNUC_INTERNAL;
gint 		pretty_table_load_path (PrettyTable *pretty_table, gchar *path, 
					gboolean recursive) G_GNUC_INTERNAL; 
void 		pretty_table_add_table (PrettyTable *pretty_table, const gchar * const table[]) G_GNUC_INTERNAL;
gchar*		pretty_table_get_name (PrettyTable *pretty_table, const gchar *command) G_GNUC_INTERNAL;
GdkPixbuf*	pretty_table_get_icon (PrettyTable *pretty_table,
				       const gchar *command, gint pid) G_GNUC_INTERNAL;
void 		pretty_table_free (PrettyTable *pretty_table) G_GNUC_INTERNAL;

#endif /* _PROCMAN_PRETTYTABLE_H_ */
