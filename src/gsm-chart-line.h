#ifndef _GSM_CHART_LINE_H_
#define _GSM_CHART_LINE_H_

/*
 * GNOME System Monitor chart line
 * Copyright (C) 2024 System Monitor developers
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with the software; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define GSM_TYPE_CHART_LINE  (gsm_chart_line_get_type ())
#define GSM_CHART_LINE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSM_TYPE_CHART_LINE, GsmChartLine))
#define GSM_IS_CHART_LINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSM_TYPE_CHART_LINE))

typedef struct _GsmChartLine GsmChartLine;
typedef struct _GsmChartLineClass GsmChartLineClass;
typedef struct _GsmChartLinePrivate GsmChartLinePrivate;

struct _GsmChartLine
{
  GObject parent_instance;
};

struct _GsmChartLinePrivate
{
  GArray *values;
  guint next_position;
  guint count;
  guint size;
};

struct _GsmChartLineClass
{
  GObjectClass parent_class;
};


GType               gsm_chart_line_get_type (void);
GsmChartLine *      gsm_chart_line_new (guint);

void                gsm_chart_line_dispose (GObject*);
void                gsm_chart_line_finalize (GObject*);

// void                gsm_chart_line_resize (GsmChartLine*, guint);
void                gsm_chart_line_push_value (GsmChartLine*, double);
// double              gsm_chart_line_get_value (GsmChartLine*, guint);

G_END_DECLS

#endif /* _GSM_CHART_LINE_H_ */
