/* Procman - callbacks
 * Copyright (C) 2001 Kevin Vandersloot
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef _PROCMAN_CALLBACKS_H_
#define _PROCMAN_CALLBACKS_H_

#include <gtk/gtk.h>

gint            cb_update_disks (gpointer data);
gint            cb_timeout (gpointer data);

void            cb_column_resized (GtkWidget* column, GParamSpec* param, gpointer data);

gboolean        cb_column_header_clicked (GtkTreeViewColumn* column, 
                                          GdkEvent* event, 
                                          gpointer data);

#endif /* _PROCMAN_CALLBACKS_H_ */
