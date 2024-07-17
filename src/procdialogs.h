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
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _GSM_PROCDIALOGS_H_
#define _GSM_PROCDIALOGS_H_


#include <glib.h>
#include "application.h"

/* These are the actual range of settable values. Values outside this range
   are scaled back to these limits. So show these limits in the slider
*/
#ifdef __linux__
const int RENICE_VAL_MIN = -20;
const int RENICE_VAL_MAX = 19;
#else /* ! linux */
const int RENICE_VAL_MIN = -20;
const int RENICE_VAL_MAX = 20;
#endif


typedef enum
{
  PROCMAN_ACTION_RENICE,
  PROCMAN_ACTION_KILL
} ProcmanActionType;


void     procdialog_create_kill_dialog (GsmApplication *app,
                                        int             signal,
                                        gint32          proc);
void     procdialog_create_renice_dialog (GsmApplication *app);
gboolean multi_root_check (char *command);
gboolean procdialog_create_root_password_dialog (ProcmanActionType type,
                                                 GsmApplication   *app,
                                                 gint              pid,
                                                 gint              extra_value);

#endif /* _GSM_PROCDIALOGS_H_ */
