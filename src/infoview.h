/* Procman - more info view
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

#ifndef _INFOVIEW_H_
#define _INFOVIEW_H_

#include <gtk/gtk.h>
#include <glib/gtypes.h>
#include "procman.h"

typedef struct _Infoview Infoview;

struct _Infoview
{
	GtkWidget	*expander;
	GtkWidget	*box;
	GtkWidget	*cmd_label;
	GtkWidget	*status_label;
	GtkWidget	*nice_label;
	GtkWidget	*memtotal_label;
	GtkWidget	*memrss_label;
	GtkWidget	*memshared_label;
};

void infoview_create (ProcData *data) G_GNUC_INTERNAL;
void	infoview_update (ProcData *data) G_GNUC_INTERNAL;


#endif
