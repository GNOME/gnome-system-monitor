/*
 * GNOME System Monitor graph
 * Copyright (C) 2022 Ondřej Míchal <harrymichal@seznam.cz>
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSM_TYPE_GRAPH  (gsm_graph_get_type ())
#define GSM_GRAPH(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSM_TYPE_GRAPH, GsmGraph))

typedef struct _GsmGraph GsmGraph;
typedef struct _GsmGraphClass GsmGraphClass;

struct _GsmGraph
{
  GtkDrawingArea parent_instance;
};

struct _GsmGraphClass
{
  GtkDrawingAreaClass parent_class;

  void (* css_changed) (GsmGraph *graph);
};

GType      gsm_graph_get_type (void);
GsmGraph * gsm_graph_new (void);

G_END_DECLS
