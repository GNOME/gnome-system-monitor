/* Procman - dialogs
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
#ifndef _PROCDIALOGS_H_
#define _PROCDIALOGS_H_


#include <glib.h>
#include "procman.h"

extern int kill_signal G_GNUC_INTERNAL;

void			procdialog_create_hide_dialog (ProcData *data) G_GNUC_INTERNAL;
void			procdialog_create_kill_dialog (ProcData *data, int signal) G_GNUC_INTERNAL;
void 			procdialog_create_renice_dialog (ProcData *data) G_GNUC_INTERNAL;
gboolean		procdialog_create_root_password_dialog (gint type,
								ProcData *procdata,
								gint pid, gint extra_value) G_GNUC_INTERNAL;
void 			procdialog_create_memmaps_dialog (ProcData *data) G_GNUC_INTERNAL;
void			procdialog_create_preferences_dialog (ProcData *data) G_GNUC_INTERNAL;

#endif

