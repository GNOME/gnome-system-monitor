#ifndef _PRETTYTABLE_H_
#define _PRETTYTABLE_H_

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct _PrettyTable {
	GHashTable *cmdline_to_prettyname;
	GHashTable *cmdline_to_prettyicon;
	GHashTable *name_to_prettyicon; /* lower case */
	
	/* private */
	
	/*GHashTable *cache;*/
} PrettyTable;

typedef struct _CachedPixbuf {
	GdkPixbuf *pixbuf;
	gint age;
	gint max_age;
} CachedPixbuf;

PrettyTable *pretty_table_new (void);
gint pretty_table_load_path (PrettyTable *pretty_table, gchar *path, gboolean recursive); /* No ending slash in the path */
void pretty_table_add_table (PrettyTable *pretty_table, const gchar *table[]);
gchar *pretty_table_get_name (PrettyTable *pretty_table, const gchar *command);
GdkPixbuf *pretty_table_get_icon (PrettyTable *pretty_table, gchar *command);
void pretty_table_free (PrettyTable *pretty_table);

CachedPixbuf *cached_pixbuf_new (void);
CachedPixbuf *cached_pixbuf_new_with_pixbuf_and_max_age (GdkPixbuf *pixbuf, gint max_age);
gboolean cached_pixbuf_age (CachedPixbuf *cached_pixbuf); /* Will free the cached pixbuf and return TRUE if age is greater than max_age */
void cached_pixbuf_renew (CachedPixbuf *cached_pixbuf); /* Sets the age to 0 */
void cached_pixbuf_free (CachedPixbuf *cached_pixbuf);

#endif /* _PRETTYTABLE_H_ */
