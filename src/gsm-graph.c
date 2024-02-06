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

#include "gsm-graph.h"

#include <glib/gi18n.h>

enum
{
  CSS_CHANGED,
  NUM_SIGNALS
};

enum
{
  PROP_LOGARITHMIC_SCALE = 1,
  PROP_FONT_SIZE,
  PROP_NUM_POINTS,
  PROP_SMOOTH_CHART,
  PROP_STACKED_CHART,
  PROP_MAX_VALUE,
  PROP_X_LABELS_FUNCTION,
  PROP_Y_LABELS_FUNCTION,
  NUM_PROPS
};

const double BORDER_ALPHA = 0.7;
const double GRID_ALPHA = BORDER_ALPHA / 2.0;
const int FRAME_WIDTH = 4;

static guint signals[NUM_SIGNALS] = {
  0
};

static GParamSpec* obj_properties[NUM_PROPS] = {
  NULL,
};

G_DEFINE_TYPE_WITH_CODE (GsmGraph, gsm_graph, GTK_TYPE_DRAWING_AREA, G_ADD_PRIVATE(GsmGraph))

static void
gsm_graph_css_changed (GtkWidget *widget,
                       GtkCssStyleChange*)
{
  g_signal_emit (G_OBJECT (widget), signals[CSS_CHANGED], 0);
}

static void
gsm_graph_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GsmGraph *self = GSM_GRAPH (object);

  switch (property_id)
    {
    case PROP_LOGARITHMIC_SCALE:
      gsm_graph_set_logarithmic_scale (self, g_value_get_boolean (value));
      break;
    case PROP_SMOOTH_CHART:
      gsm_graph_set_smooth_chart (self, g_value_get_boolean (value));
      break;
    case PROP_STACKED_CHART:
      gsm_graph_set_stacked_chart (self, g_value_get_boolean (value));
      break;
    case PROP_FONT_SIZE:
      gsm_graph_set_font_size (self, g_value_get_double (value));
      break;
    case PROP_NUM_POINTS:
      gsm_graph_set_num_points (self, g_value_get_uint (value));
      break;
    case PROP_MAX_VALUE:
      gsm_graph_set_max_value (self, g_value_get_uint64 (value));
      break;
    case PROP_X_LABELS_FUNCTION:
      gsm_graph_set_x_labels_function (self, g_value_get_pointer (value));
      break;
    case PROP_Y_LABELS_FUNCTION:
      gsm_graph_set_y_labels_function (self, g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gsm_graph_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GsmGraph *self = GSM_GRAPH (object);

  switch (property_id)
    {
    case PROP_LOGARITHMIC_SCALE:
      g_value_set_boolean (value, gsm_graph_is_logarithmic_scale (self));
      break;
    case PROP_SMOOTH_CHART:
      g_value_set_boolean (value, gsm_graph_is_smooth_chart (self));
      break;
    case PROP_STACKED_CHART:
      g_value_set_boolean (value, gsm_graph_is_stacked_chart (self));
      break;
    case PROP_FONT_SIZE:
      g_value_set_double (value, gsm_graph_get_font_size (self));
      break;
    case PROP_NUM_POINTS:
      g_value_set_double (value, gsm_graph_get_font_size (self));
      break;
    case PROP_MAX_VALUE:
      g_value_set_uint64 (value, gsm_graph_get_max_value (self));
      break;
    case PROP_X_LABELS_FUNCTION:
      g_value_set_pointer (value, gsm_graph_get_x_labels_function (self));
      break;
    case PROP_Y_LABELS_FUNCTION:
      g_value_set_pointer (value, gsm_graph_get_y_labels_function (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gsm_graph_class_init (GsmGraphClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->css_changed = gsm_graph_css_changed;

  object_class->set_property = gsm_graph_set_property;
  object_class->get_property = gsm_graph_get_property;

  object_class->dispose = gsm_graph_dispose;
  object_class->finalize = gsm_graph_finalize;

  /**
   * GsmGraph::css-changed:
   * @self:
   */
  signals[CSS_CHANGED] = g_signal_new ("css-changed",
                                       G_TYPE_FROM_CLASS (object_class),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (GsmGraphClass, css_changed),
                                       NULL, NULL,
                                       NULL,
                                       G_TYPE_NONE,
                                       0);
  obj_properties[PROP_LOGARITHMIC_SCALE] =
                        g_param_spec_boolean ("logarithmic-scale", NULL, NULL, FALSE, G_PARAM_READWRITE);
  obj_properties[PROP_FONT_SIZE] =
                        g_param_spec_double ("font-size", NULL, NULL, 8.0, 48.0, 8.0, G_PARAM_READWRITE);
  obj_properties[PROP_NUM_POINTS] =
                        g_param_spec_uint ("num-points", NULL, NULL, 10, 600, 60, G_PARAM_READWRITE);
  obj_properties[PROP_SMOOTH_CHART] =
                        g_param_spec_boolean ("smooth-chart", NULL, NULL, TRUE, G_PARAM_READWRITE);
  obj_properties[PROP_STACKED_CHART] =
                        g_param_spec_boolean ("stacked-chart", NULL, NULL, FALSE, G_PARAM_READWRITE);
  obj_properties[PROP_MAX_VALUE] =
                        g_param_spec_uint64 ("max-value", NULL, NULL, 0, G_MAXUINT64, 100, G_PARAM_READWRITE);
  obj_properties[PROP_X_LABELS_FUNCTION] =
                        g_param_spec_pointer ("x-labels-function", NULL, NULL, G_PARAM_READWRITE);
  obj_properties[PROP_Y_LABELS_FUNCTION] =
                        g_param_spec_pointer ("y-labels-function", NULL, NULL, G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     G_N_ELEMENTS (obj_properties),
                                     obj_properties);

}

void
gsm_graph_dispose (GObject *self)
{
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (GSM_GRAPH (self));

  if (priv->background)
    cairo_surface_destroy (priv->background);

  if (priv->redraw_timeout)
    g_source_remove (priv->redraw_timeout);

  G_OBJECT_CLASS (gsm_graph_parent_class)->dispose (G_OBJECT (self));
}

void
gsm_graph_finalize (GObject *self)
{
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (GSM_GRAPH (self));

  priv->background = NULL;
  G_OBJECT_CLASS (gsm_graph_parent_class)->finalize (G_OBJECT (self));
}

static void
_gsm_graph_state_flags_changed (GtkWidget *widget,
                                GtkStateFlags*,
                                gpointer   self)
{
  GsmGraph *graph = GSM_GRAPH (self);

  gsm_graph_force_refresh (graph);
  if (gtk_widget_is_visible (widget))
    gsm_graph_start (graph);
  else
    gsm_graph_stop (graph);
}

static gboolean
_gsm_graph_update (GsmGraph *self)
{
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  if (priv->render_counter == priv->frames_per_unit - 1)
    priv->data_function (priv->update_data);

  if (gsm_graph_is_started (self))
    gtk_widget_queue_draw (GTK_WIDGET (self));

  priv->render_counter++;

  if (priv->render_counter >= priv->frames_per_unit)
    priv->render_counter = 0;

  return TRUE;
}

static void
gsm_graph_init (GsmGraph *self)
{
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  priv->draw = FALSE;
  priv->background = NULL;
  priv->logarithmic_scale = FALSE;
  
  // FIXME:
  // on configure, frames_per_unit = graph->draw_width/num_points;
  // knock FRAMES down to 10 until cairo gets faster
  priv->frames_per_unit = 10;
  priv->render_counter = priv->frames_per_unit - 1;
  priv->fontsize = 8.0;
  priv->num_points = 60;
  priv->indent = 18;
  priv->rmargin = 6 * priv->fontsize;
  priv->smooth = TRUE;
  priv->stacked = FALSE;
  priv->max_value = 100;

  g_signal_connect (G_OBJECT (self), "resize",
                    G_CALLBACK (gsm_graph_force_refresh), self);
  g_signal_connect (G_OBJECT (self), "css-changed",
                    G_CALLBACK (gsm_graph_force_refresh), self);
  g_signal_connect (G_OBJECT (self), "state-flags-changed",
                    G_CALLBACK (_gsm_graph_state_flags_changed), self);
}

GsmGraph *
gsm_graph_new (void)
{
  return g_object_new (GSM_TYPE_GRAPH, NULL);
}

void _gsm_graph_set_draw (GsmGraph *self, gboolean draw)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->draw != draw)
    priv->draw = draw;
}

void gsm_graph_set_font_size (GsmGraph *self, double fontsize)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->fontsize != fontsize)
  {
    priv->fontsize = fontsize;
    priv->rmargin = 6 * fontsize;
    gsm_graph_force_refresh (self);
  }
}

void gsm_graph_set_num_points (GsmGraph *self, guint num_points)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->num_points != num_points)
  {
    priv->num_points = num_points;
    gsm_graph_force_refresh (self);
  }
}

void gsm_graph_set_max_value (GsmGraph *self, guint64 max_value)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->max_value != max_value)
  {
    priv->max_value = max_value;
    gsm_graph_force_refresh (self);
  }
}

void gsm_graph_set_speed (GsmGraph *self, guint speed)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->speed != speed) {
    priv->speed = speed;
    if (priv->redraw_timeout) {
      g_source_remove (priv->redraw_timeout);
      priv->redraw_timeout = g_timeout_add (priv->speed,
                                            (GSourceFunc)_gsm_graph_update,
                                            self);
      gsm_graph_clear_background (self);
    }
  }
}

void gsm_graph_set_data_function (GsmGraph *self, GSourceFunc function, gpointer data)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->redraw_timeout) {
    g_source_remove (priv->redraw_timeout);
  }
  
  priv->data_function = function;
  priv->update_data = data;
  
  if (gsm_graph_is_started (self)) {
      priv->redraw_timeout = g_timeout_add (priv->speed,
                                            (GSourceFunc)_gsm_graph_update,
                                            self);
  }
}

void
gsm_graph_start (GsmGraph *self)
{
  _gsm_graph_set_draw (self, TRUE);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  g_return_if_fail ( priv->data_function != NULL);

  if (priv->redraw_timeout == 0)
    {
      // Update the data two times so the graph
      // doesn't wait one cycle to start drawing.
      priv->data_function (priv->update_data);
      _gsm_graph_update (self);

      priv->redraw_timeout = g_timeout_add (priv->speed,
                                            (GSourceFunc)_gsm_graph_update,
                                            self);
  }
}

void
gsm_graph_force_refresh (GsmGraph *self)
{
  gsm_graph_clear_background (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
gsm_graph_clear_background (GsmGraph *self)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  if (priv->background) {
    cairo_surface_destroy (priv->background);
    priv->background = NULL;
  }
}

void
gsm_graph_set_background (GsmGraph *self, cairo_surface_t * background)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  if (priv->background != background) {
    if (priv->background != NULL)
      cairo_surface_destroy (priv->background);
    priv->background = background;
  }
}

void
gsm_graph_create_background (GsmGraph *self, guint width, guint height)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  if (priv->background != NULL) {
    cairo_surface_destroy (priv->background);
  }
  
  GdkRGBA fg_color;
  GdkRGBA grid_color;
  PangoContext *pango_context;
  PangoFontDescription *font_desc;
  PangoLayout *layout;
  cairo_t *cr;
  cairo_surface_t *surface;
  graphene_rect_t * bounds;
  const guint num_sections = 7;
  guint num_bars = gsm_graph_get_num_bars (self);

  /* Graph length */
  const unsigned total_seconds = priv->speed * (priv->num_points - 2) / 1000 * priv->frames_per_unit;

  bounds = graphene_rect_alloc ();
  g_return_val_if_fail (gtk_widget_compute_bounds (GTK_WIDGET (self), gtk_widget_get_parent (GTK_WIDGET (self)), bounds), NULL);
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        graphene_rect_get_width (bounds),
                                        graphene_rect_get_height(bounds));
  graphene_rect_free (bounds);
  cr = cairo_create (surface);

  /* Create grid label layout */
  layout = pango_cairo_create_layout (cr);

  /* Set font for graph labels */
  pango_context = gtk_widget_get_pango_context (GTK_WIDGET (self));
  font_desc = pango_context_get_font_description (pango_context);
  pango_font_description_set_size (font_desc, 0.8 * priv->fontsize * PANGO_SCALE);
  pango_layout_set_font_description (layout, font_desc);

  /* draw frame */
  cairo_translate (cr, FRAME_WIDTH, FRAME_WIDTH);

  /* Draw background rectangle */
  /* When a user uses a dark theme, the hard-coded
   * white background in GSM is a lone white on the
   * display, which makes the user unhappy. To fix
   * this, here we offer the user a chance to set
   * his favorite background color. */
  GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (self));

  gtk_style_context_save (context);

  /* Here we specify the name of the class. Now in
   * the theme's CSS we can specify the own colors
   * for this class. */
  gtk_style_context_add_class (context, "loadgraph");

  /* Get foreground color */
  gtk_style_context_get_color (context, &fg_color);
  /* The grid color is the same as the foreground color but has sometimes
   * different alpha. Keep it separate. */
  grid_color = fg_color;

  /* Why not use the new features of the
   * GTK instead of cairo_rectangle ?! :) */
  gtk_render_background (context, cr, priv->indent, 0.0,
                         width - priv->rmargin - priv->indent,
                         priv->real_draw_height);

  gtk_style_context_restore (context);

  cairo_set_line_width (cr, 0.25);

  /* Horizontal grid lines */
  for (guint i = 0; i <= num_bars; i++)
    {
      PangoRectangle extents;

      /* Label alignment */
      double y;
      if (i == 0)
        /* Below the line */
        y = 0.5 + priv->fontsize / 2.0;
      else if (i == num_bars)
        /* Above the line */
        y = i * priv->dely + 0.5;
      else
        /* Next to the line */
        y = i * priv->dely + priv->fontsize / 2.0;

      if (priv->y_labels_function != NULL)
      {
        /* Draw the label */
        /* Prepare the text */
        float caption_value = gsm_graph_get_y_label_value (self, i);
        gchar *caption = priv->y_labels_function ((guint64)caption_value);

        pango_layout_set_text (layout, caption, -1);
        pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
        pango_layout_get_extents (layout, NULL, &extents);

        /* Create y axis position modifier */
        double label_y_offset_modifier = i == 0 ? 0.5
                                : i == num_bars
                                    ? 1.0
                                    : 0.85;

        /* Set the label position */
        cairo_move_to (cr,
                       width - priv->indent - 23,
                       y - label_y_offset_modifier * extents.height / PANGO_SCALE);

        /* Set the color */
        gdk_cairo_set_source_rgba (cr, &fg_color);

        /* Paint the grid label */
        pango_cairo_show_layout (cr, layout);
        g_free (caption);
      }

      /* Set the grid line alpha */
      if (i == 0 || i == num_bars)
        grid_color.alpha = BORDER_ALPHA;
      else
        grid_color.alpha = GRID_ALPHA;

      /* Draw the line */
      /* Set the color */
      gdk_cairo_set_source_rgba (cr, &grid_color);

      /* Set the grid line path */
      cairo_move_to (cr, priv->indent, i * priv->dely);
      cairo_line_to (cr,
                     width - priv->rmargin + 4,
                     i * priv->dely);
    }

  /* Vertical grid lines */
  for (unsigned int i = 0; i < num_sections; i++)
    {
      PangoRectangle extents;

      /* Prepare the x position */
      double x = ceil (i * (width - priv->rmargin - priv->indent) / (num_sections - 1));

      if (priv->x_labels_function != NULL)
      {
        /* Draw the label */
        /* Prepare the text */
        gchar *caption = priv->x_labels_function (total_seconds - i * total_seconds / (num_sections - 1));
        pango_layout_set_text (layout, caption, -1);
        pango_layout_get_extents (layout, NULL, &extents);

        /* Create x axis position modifier */
        double label_x_offset_modifier = i == 0 ? 0
                                           : i == (num_sections - 1)
                                              ? 1.0
                                              : 0.5;

        /* Set the label position */
        cairo_move_to (cr,
                       x + priv->indent - label_x_offset_modifier * extents.width / PANGO_SCALE + 1.0,
                       height - 1.0 * extents.height / PANGO_SCALE + 5);

        /* Set the color */
        gdk_cairo_set_source_rgba (cr, &fg_color);

        /* Paint the grid label */
        pango_cairo_show_layout (cr, layout);
        g_free (caption);
      }
      /* Set the grid line alpha */
      if (i == 0 || i == (num_sections - 1))
        grid_color.alpha = BORDER_ALPHA;
      else
        grid_color.alpha = GRID_ALPHA;

      /* Draw the line */
      /* Set the color */
      gdk_cairo_set_source_rgba (cr, &grid_color);

      /* Set the grid line path */
      cairo_move_to (cr, x + priv->indent, 0);
      cairo_line_to (cr, x + priv->indent, priv->real_draw_height + 4);
    }

  /* Paint */
  cairo_stroke (cr);

  g_object_unref (layout);
  cairo_destroy (cr);

  priv->background = surface;
}

void
gsm_graph_set_logarithmic_scale (GsmGraph *self, gboolean logarithmic)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  if (priv->logarithmic_scale != logarithmic) {
    priv->logarithmic_scale = logarithmic;
  }
}

void
gsm_graph_set_smooth_chart (GsmGraph *self, gboolean smooth)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  if (priv->smooth != smooth) {
    priv->smooth = smooth;
  }
}

void
gsm_graph_set_stacked_chart (GsmGraph *self, gboolean stacked)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  if (priv->stacked != stacked) {
    priv->stacked = stacked;
  }
}

void gsm_graph_set_dely (GsmGraph *self, guint dely)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->dely != dely) {
    priv->dely = dely;
  }
}

void gsm_graph_set_x_labels_function (GsmGraph *self, GsmLabelFunction x_labels_function)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->x_labels_function != x_labels_function) {
    priv->x_labels_function = x_labels_function;
  }
}

void gsm_graph_set_y_labels_function (GsmGraph *self, GsmLabelFunction y_labels_function)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->y_labels_function != y_labels_function) {
    priv->y_labels_function = y_labels_function;
  }
}

void gsm_graph_set_real_draw_height (GsmGraph *self, guint real_draw_height)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  if (priv->real_draw_height != real_draw_height) {
    priv->real_draw_height = real_draw_height;
  }
}

void
gsm_graph_stop (GsmGraph *self)
{
  _gsm_graph_set_draw (self, TRUE);
}

gboolean
gsm_graph_is_background_set (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), FALSE);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->background != NULL;
}

gboolean
gsm_graph_is_started (GsmGraph *self)
{   
  g_return_val_if_fail (GSM_IS_GRAPH (self), FALSE);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->draw;
}

cairo_surface_t*
gsm_graph_get_background (GsmGraph *self)
{   
  g_return_val_if_fail (GSM_IS_GRAPH (self), NULL);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->background;
}

guint
gsm_graph_get_speed (GsmGraph *self)
{   
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->speed;
}

guint
gsm_graph_get_frames_per_unit (GsmGraph *self)
{   
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->frames_per_unit;
}

guint
gsm_graph_get_render_counter (GsmGraph *self)
{   
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->render_counter;
}

double
gsm_graph_get_font_size (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->fontsize;
}

guint
gsm_graph_get_num_points (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->num_points;
}

guint64
gsm_graph_get_max_value (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->max_value;
}

guint
gsm_graph_get_indent (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->indent;
}

guint
gsm_graph_get_dely (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->dely;
}

guint
gsm_graph_get_real_draw_height (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->real_draw_height;
}

double
gsm_graph_get_right_margin (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->rmargin;
}

GsmLabelFunction
gsm_graph_get_x_labels_function (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), NULL);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->x_labels_function;
}

GsmLabelFunction
gsm_graph_get_y_labels_function (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), NULL);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->y_labels_function;
}

gboolean
gsm_graph_is_logarithmic_scale (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), FALSE);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->logarithmic_scale;
}

gboolean
gsm_graph_is_smooth_chart (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), FALSE);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->smooth;
}

gboolean
gsm_graph_is_stacked_chart (GsmGraph *self)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), FALSE);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);

  return priv->stacked;
}

guint
gsm_graph_get_num_bars (GsmGraph *self)
{
  
  guint n;
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);

  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  guint height = gtk_widget_get_height (GTK_WIDGET (self));

  // keep 100 % num_bars == 0
  switch ((int)(height / (priv->fontsize + 14)))
    {
      case 0:
      case 1:
        n = 1;
        break;

      case 2:
      case 3:
        n = 2;
        break;

      case 4:
        n = 4;
        break;

      case 5:
        n = 5;
        if (priv->logarithmic_scale)
          n = 4;
        break;

      default:
        n = 5;
        if (priv->logarithmic_scale)
          n = 6;
    }

  return n;
}

float
gsm_graph_get_y_label_value (GsmGraph *self, guint index)
{
  g_return_val_if_fail (GSM_IS_GRAPH (self), 0);
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  
  guint num_bars = gsm_graph_get_num_bars (self);
  // operation orders matters so it's 0 if index == num_bars
  float caption_percentage = (float)priv->max_value - index * (float)priv->max_value / num_bars;

  if (priv->logarithmic_scale)
    return caption_percentage == 0 ? 0 : pow (100, caption_percentage / priv->max_value);
  return caption_percentage;
}
