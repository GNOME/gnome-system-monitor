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

static guint signals[NUM_SIGNALS] = {
  0
};

G_DEFINE_TYPE_WITH_CODE (GsmGraph, gsm_graph, GTK_TYPE_DRAWING_AREA, G_ADD_PRIVATE(GsmGraph))

static void
gsm_graph_css_changed (GtkWidget *widget,
                       GtkCssStyleChange*)
{
  g_signal_emit (G_OBJECT (widget), signals[CSS_CHANGED], 0);
}

static void
gsm_graph_class_init (GsmGraphClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->css_changed = gsm_graph_css_changed;

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
}

void
gsm_graph_dispose (GsmGraph *self)
{
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (GSM_GRAPH (self));

  if (priv->background)
    cairo_surface_destroy (priv->background);
  G_OBJECT_CLASS (gsm_graph_parent_class)->dispose (self);
}

void
gsm_graph_finalize (GsmGraph *self)
{
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (GSM_GRAPH (self));

  priv->background = NULL;
  G_OBJECT_CLASS (gsm_graph_parent_class)->finalize (self);
}

static void
gsm_graph_init (GsmGraph *self)
{
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  priv->draw = FALSE;
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

void
gsm_graph_start (GsmGraph *self)
{
  _gsm_graph_set_draw (self, TRUE);
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
gsm_graph_set_background (GsmGraph *self, cairo_surface_t *background)
{
  g_return_if_fail (GSM_IS_GRAPH (self));
  GsmGraphPrivate *priv = gsm_graph_get_instance_private (self);
  if (priv->background != background) {
    if (priv->background != NULL) {
      cairo_surface_destroy (priv->background);
    }
    priv->background = background;
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
