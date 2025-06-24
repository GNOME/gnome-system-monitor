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
  
  // internal usage for animations
  
  guint render_counter;
  guint frames_per_unit;
  guint redraw_timeout;

  // used for displaying, configurable

  // font size for labels
  double fontsize;
  // refresh timeout
  guint speed;
  // draw y axis with logarithmic scale instead of default linear
  gboolean logarithmic_scale;
  // number of chart data points
  guint num_points;
  // draw stacked chart
  gboolean stacked;
  // draw smooth chart
  gboolean smooth;
  // maximum value for y axis
  guint64 max_value;

  // used for displaying, internal

  // right margin
  double rmargin;
  // cached background surface
  cairo_surface_t *background;
  guint indent;
  
  // function to calculate the latest set of data to display
  GSourceFunc data_function;
  // the data to be passed to the data_function
  gpointer update_data;
};

struct _GsmGraphClass
{
  GtkDrawingAreaClass parent_class;

  void (* css_changed) (GsmGraph *graph);
};

GType               gsm_graph_get_type (void);
GsmGraph *          gsm_graph_new (void);

void                gsm_graph_dispose (GObject*);
void                gsm_graph_finalize (GObject*);

void                gsm_graph_start (GsmGraph*);
void                gsm_graph_stop (GsmGraph*);
void                gsm_graph_force_refresh (GsmGraph*);

// public setter methods
void                gsm_graph_set_logarithmic_scale (GsmGraph*, gboolean);
void                gsm_graph_set_smooth_chart (GsmGraph*, gboolean);
void                gsm_graph_set_stacked_chart (GsmGraph*, gboolean);
void                gsm_graph_set_speed (GsmGraph*, guint);
void                gsm_graph_set_data_function (GsmGraph*, GSourceFunc, gpointer);
void                gsm_graph_set_font_size (GsmGraph*, double);
void                gsm_graph_set_num_points (GsmGraph*, guint);
void                gsm_graph_set_max_value (GsmGraph*, guint64);

// temporary exported setter methods
void                gsm_graph_clear_background (GsmGraph*);
void                gsm_graph_set_background (GsmGraph*, cairo_surface_t*);
void                gsm_graph_set_frames_per_unit (GsmGraph*, guint);

// public getter methods
gboolean            gsm_graph_is_logarithmic_scale (GsmGraph*);
guint               gsm_graph_get_speed (GsmGraph*);
double              gsm_graph_get_font_size (GsmGraph*);
gboolean            gsm_graph_is_started (GsmGraph*);
gboolean            gsm_graph_is_smooth_chart (GsmGraph*);
gboolean            gsm_graph_is_stacked_chart (GsmGraph*);
guint               gsm_graph_get_num_points (GsmGraph*);
guint64             gsm_graph_get_max_value (GsmGraph*);

// temporary exported methods for the migration
gboolean            gsm_graph_is_background_set (GsmGraph*);
cairo_surface_t *   gsm_graph_get_background (GsmGraph*);
guint               gsm_graph_get_frames_per_unit (GsmGraph*);
guint               gsm_graph_get_render_counter (GsmGraph*);
guint               gsm_graph_get_indent (GsmGraph*);
double              gsm_graph_get_right_margin (GsmGraph*);
guint               gsm_graph_get_num_bars (GsmGraph*, gint);


G_END_DECLS

// not intended to be exported
void                _gsm_graph_set_draw (GsmGraph*, gboolean);

#endif /* _GSM_GRAPH_H_ */
