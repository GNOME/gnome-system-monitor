#ifndef _PRETTYTABLE_H_
#define _PRETTYTABLE_H_

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "procman.h"


void*		prettytable_load_async (void *data);
PrettyTable*	pretty_table_new (void);
gint 		pretty_table_load_path (PrettyTable *pretty_table, gchar *path, 
					gboolean recursive); 
void 		pretty_table_add_table (PrettyTable *pretty_table, const gchar *table[]);
gchar*		pretty_table_get_name (PrettyTable *pretty_table, const gchar *command);
GdkPixbuf*	pretty_table_get_icon (PrettyTable *pretty_table, gchar *command);
void 		pretty_table_free (PrettyTable *pretty_table);

#endif /* _PRETTYTABLE_H_ */
