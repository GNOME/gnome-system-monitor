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

#include "gsm-chart-line.h"

G_DEFINE_TYPE_WITH_PRIVATE (GsmChartLine, gsm_chart_line, G_TYPE_OBJECT)

static void
gsm_chart_line_class_init (GsmChartLineClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gsm_chart_line_dispose;
  object_class->finalize = gsm_chart_line_finalize;

}

static void
gsm_chart_line_init (GsmChartLine *)
{
  // GsmChartLinePrivate *priv = gsm_chart_line_get_instance_private (self);
}

void
gsm_chart_line_dispose (GObject *self)
{
  GsmChartLinePrivate *priv = gsm_chart_line_get_instance_private (GSM_CHART_LINE (self));

  if (priv->values)
    g_array_free (priv->values, TRUE);
  priv->count = 0;
  priv->next_position = 0;
  G_OBJECT_CLASS (gsm_chart_line_parent_class)->dispose (G_OBJECT (self));
}

void
gsm_chart_line_finalize (GObject *self)
{
  // GsmChartLinePrivate *priv = gsm_chart_line_get_instance_private (GSM_CHART_LINE (self));

  G_OBJECT_CLASS (gsm_chart_line_parent_class)->finalize (G_OBJECT (self));
}

GsmChartLine *
gsm_chart_line_new (guint size)
{
  GsmChartLine * result = g_object_new (GSM_TYPE_CHART_LINE, NULL);
  GsmChartLinePrivate *priv = gsm_chart_line_get_instance_private (GSM_CHART_LINE (result));
  priv->values = g_array_sized_new (TRUE, TRUE, sizeof(double), size);
  priv->size = size;
  return result;
}

void
gsm_chart_line_push_value (GsmChartLine* self, double new_value)
{
  GsmChartLinePrivate *priv = gsm_chart_line_get_instance_private (GSM_CHART_LINE (self));
  if (priv->count < priv->size) {
    g_array_append_val (priv->values, new_value);
    priv->next_position++;
    priv->count++;
  } else {
    g_array_insert_val (priv->values, priv->next_position, new_value);
    priv->next_position = (priv->next_position + 1) % priv->count;
  } 
}
