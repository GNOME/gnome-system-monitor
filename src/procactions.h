/* Procman process actions
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
#ifndef _GSM_PROCACTIONS_H_
#define _GSM_PROCACTIONS_H_

#include "application.h"

void renice (GsmApplication *app,
             int             nice);
void kill_process (GsmApplication *app,
                   int             sig);

struct ProcActionArgs
{
  GsmApplication *app;
  int arg_value;
};

#endif /* _GSM_PROCACTIONS_H_ */
