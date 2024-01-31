#ifndef _GSM_GRAPH_H_
#define _GSM_GRAPH_H_
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
#define GSM_IS_GRAPH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSM_TYPE_GRAPH))

typedef struct _GsmGraph GsmGraph;
typedef struct _GsmGraphClass GsmGraphClass;
typedef struct _GsmGraphPrivate GsmGraphPrivate;

struct _GsmGraph
{
  GtkDrawingArea parent_instance;
};

struct _GsmGraphPrivate
{
  gboolean draw;
  gboolean logarithmic_scale;
  cairo_surface_t *background;
};

struct _GsmGraphClass
{
  GtkDrawingAreaClass parent_class;

  void (* css_changed) (GsmGraph *graph);
};

GType               gsm_graph_get_type (void);
GsmGraph *          gsm_graph_new (void);

void                gsm_graph_dispose (GsmGraph*);
void                gsm_graph_finalize (GsmGraph*);

void                gsm_graph_start (GsmGraph*);
void                gsm_graph_stop (GsmGraph*);
void                gsm_graph_force_refresh (GsmGraph*);

void                gsm_graph_clear_background (GsmGraph*);
void                gsm_graph_set_background (GsmGraph*, cairo_surface_t*);
void                gsm_graph_set_logarithmic_scale (GsmGraph*, gboolean);

gboolean            gsm_graph_is_logarithmic_scale (GsmGraph*);
gboolean            gsm_graph_is_started (GsmGraph*);
gboolean            gsm_graph_is_background_set (GsmGraph*);
cairo_surface_t *   gsm_graph_get_background (GsmGraph*);

G_END_DECLS

#endif /* _GSM_GRAPH_H_ */
