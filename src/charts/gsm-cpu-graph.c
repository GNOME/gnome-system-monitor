/* gsm-cpu-graph.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>

#include <dazzle.h>
#include "gsm-cpu-graph.h"
#include "gsm-cpu-model.h"

struct _GsmCpuGraph
{
  DzlGraphView parent_instance;

  gint64 timespan;
  guint  max_samples;
};

G_DEFINE_TYPE (GsmCpuGraph, gsm_cpu_graph, DZL_TYPE_GRAPH_VIEW)

enum {
  PROP_0,
  PROP_MAX_SAMPLES,
  PROP_TIMESPAN,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

static const gchar *colors[] = {
  "#73d216",
  "#f57900",
  "#3465a4",
  "#ef2929",
  "#75507b",
  "#ce5c00",
  "#c17d11",
  "#cc0000",
};

GtkWidget *
gsm_cpu_graph_new_full (gint64 timespan,
                        guint  max_samples)
{
  if (timespan <= 0)
    timespan = 60L * G_USEC_PER_SEC;

  if (max_samples < 1)
    max_samples = 120;

  return g_object_new (GSM_TYPE_CPU_GRAPH,
                       "max-samples", max_samples,
                       "timespan", timespan,
                       NULL);
}

static void
gsm_cpu_graph_constructed (GObject *object)
{
  static GsmCpuModel *model;
  GsmCpuGraph *self = (GsmCpuGraph *)object;
  guint n_columns;
  guint i;

  G_OBJECT_CLASS (gsm_cpu_graph_parent_class)->constructed (object);

  /*
   * Create a model, but allow it to be destroyed after the last
   * graph releases it. We will recreate it on demand.
   */
  if (model == NULL)
    {
      model = g_object_new (GSM_TYPE_CPU_MODEL,
                            "timespan", self->timespan,
                            "max-samples", self->max_samples + 1,
                            NULL);
      g_object_add_weak_pointer (G_OBJECT (model), (gpointer *)&model);
      dzl_graph_view_set_model (DZL_GRAPH_VIEW (self), DZL_GRAPH_MODEL (model));
      g_object_unref (model);
    }
  else
    {
      dzl_graph_view_set_model (DZL_GRAPH_VIEW (self), DZL_GRAPH_MODEL (model));
    }

  n_columns = dzl_graph_view_model_get_n_columns (DZL_GRAPH_MODEL (model));

  for (i = 0; i < n_columns; i++)
    {
      DzlGraphRenderer *renderer;

      renderer = g_object_new (DZL_TYPE_GRAPH_LINE_RENDERER,
                               "column", i,
                               "stroke-color", colors [i % G_N_ELEMENTS (colors)],
                               NULL);
      dzl_graph_view_add_renderer (DZL_GRAPH_VIEW (self), renderer);
      g_clear_object (&renderer);
    }
}

static void
gsm_cpu_graph_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GsmCpuGraph *self = GSM_CPU_GRAPH (object);

  switch (prop_id)
    {
    case PROP_MAX_SAMPLES:
      g_value_set_uint (value, self->max_samples);
      break;

    case PROP_TIMESPAN:
      g_value_set_int64 (value, self->timespan);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gsm_cpu_graph_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GsmCpuGraph *self = GSM_CPU_GRAPH (object);

  switch (prop_id)
    {
    case PROP_MAX_SAMPLES:
      self->max_samples = g_value_get_uint (value);
      break;

    case PROP_TIMESPAN:
      if (!(self->timespan = g_value_get_int64 (value)))
        self->timespan = 60L * G_USEC_PER_SEC;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gsm_cpu_graph_class_init (GsmCpuGraphClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gsm_cpu_graph_constructed;
  object_class->get_property = gsm_cpu_graph_get_property;
  object_class->set_property = gsm_cpu_graph_set_property;

  properties [PROP_TIMESPAN] =
    g_param_spec_int64 ("timespan",
                         "Timespan",
                         "Timespan",
                         0, G_MAXINT64,
                         60L * G_USEC_PER_SEC,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_MAX_SAMPLES] =
    g_param_spec_uint ("max-samples",
                       "Max Samples",
                       "Max Samples",
                       0, G_MAXUINT,
                       120,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
gsm_cpu_graph_init (GsmCpuGraph *self)
{
  self->max_samples = 120;
  self->timespan = 60L * G_USEC_PER_SEC;
}
