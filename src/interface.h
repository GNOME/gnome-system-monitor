/* Procman - main window
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

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <procman.h>

gint		get_sys_pane_pos (void);
GtkWidget* 	create_main_window (ProcData *data);
GtkWidget*	create_simple_view_dialog (ProcData *procdata);
void		toggle_infoview (ProcData *data);
void		update_sensitivity (ProcData *data, gboolean sensitivity);
void            do_popup_menu(ProcData *data, GdkEventButton *event);
#endif
