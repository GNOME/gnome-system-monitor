#ifndef _PROCMAN_PRETTYTABLE_H_
#define _PROCMAN_PRETTYTABLE_H_

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "procman.h"


void		pretty_table_new (ProcData *procdata) G_GNUC_INTERNAL;

GdkPixbuf*	pretty_table_get_icon (PrettyTable *pretty_table,
				       const gchar *command, guint pid) G_GNUC_INTERNAL;
void 		pretty_table_free (PrettyTable *pretty_table) G_GNUC_INTERNAL;

#endif /* _PROCMAN_PRETTYTABLE_H_ */
