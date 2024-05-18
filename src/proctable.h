/* Procman - tree view
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
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _GSM_PROCTABLE_H_
#define _GSM_PROCTABLE_H_

#include <glib.h>
#include <gtk/gtk.h>
#include "application.h"

void proctable_new (GsmApplication *app, GtkColumnView *proctable);
void proctable_update (GsmApplication *app);
void proctable_free_table (GsmApplication *app);
void proctable_freeze (GsmApplication *app);
void proctable_thaw (GsmApplication *app);
void proctable_reset_timeout (GsmApplication *app);

void get_process_memory_writable (ProcInfo *info);
void get_last_selected (GListModel *model,
                        guint       position,
                        gpointer    data);

#endif /* _GSM_PROCTABLE_H_ */
