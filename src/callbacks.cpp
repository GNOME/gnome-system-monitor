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

#include <config.h>

#include <giomm.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <signal.h>

#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "util.h"
#include "procactions.h"
#include "procman-app.h"
#include "procdialogs.h"
#include "load-graph.h"
#include "disks.h"
#include "lsof.h"

gint
cb_timeout (gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);
    guint new_interval;


    proctable_update_all (app);

    if (app->smooth_refresh->get(new_interval))
    {
        app->timeout = g_timeout_add(new_interval,
                                     cb_timeout,
                                     app);
        return FALSE;
    }

    return TRUE;
}
