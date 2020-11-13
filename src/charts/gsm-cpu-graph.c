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
  GHashTable*  renderers;
  GsmCpuModel* model;
};

G_DEFINE_TYPE (GsmCpuGraph, gsm_cpu_graph, DZL_TYPE_GRAPH_VIEW)

enum {
  PROP_0,
  PROP_MODEL,
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

static void
_gsm_cpu_graph_initialize_with_model(GsmCpuGraph *self);

GtkWidget *
gsm_cpu_graph_new_full (GsmCpuModel *cpu_model)
{
  return g_object_new (GSM_TYPE_CPU_GRAPH,
                       "model", cpu_model,
                       NULL);
}

void
gsm_cpu_graph_renderer_set_color (GsmCpuGraph *cpu_graph, gchar* renderer_name, GdkRGBA *color)
{
  DzlGraphRenderer *renderer = DZL_GRAPH_RENDERER (g_hash_table_lookup (cpu_graph->renderers, renderer_name));
  if (renderer == NULL) return;
  g_object_set (renderer,
                "stroke-color-rgba", color,
                "line-width", 0.5,
                NULL);
}

static void
gsm_cpu_graph_constructed (GObject *object)
{
  GsmCpuGraph *self = (GsmCpuGraph *)object;

  G_OBJECT_CLASS (gsm_cpu_graph_parent_class)->constructed (object);

  /*
   * Create a model, but allow it to be destroyed after the last
   * graph releases it. We will recreate it on demand.
   */
  if (self->model == NULL)
    {
      self->model = g_object_new (GSM_TYPE_CPU_MODEL,
                            "timespan", 3*G_TIME_SPAN_MINUTE,
                            "max-samples", 180 + 1,
                            NULL);
    }

  self->renderers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  _gsm_cpu_graph_initialize_with_model(self);
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
    case PROP_MODEL:
      g_value_set_object (value, self->model);
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
    case PROP_MODEL:
      self->model = g_value_get_object (value);
      _gsm_cpu_graph_initialize_with_model (self);
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

  properties [PROP_MODEL] =
    g_param_spec_object ("model",
                         "Model",
                         "Model",
                         GSM_TYPE_CPU_MODEL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
gsm_cpu_graph_init (GsmCpuGraph *self)
{
}

static void
_gsm_cpu_graph_initialize_with_model(GsmCpuGraph *self)
{
  guint n_columns;
  guint i;
  dzl_graph_view_set_model (DZL_GRAPH_VIEW (self), DZL_GRAPH_MODEL (self->model));

  n_columns = dzl_graph_view_model_get_n_columns (DZL_GRAPH_MODEL (self->model));
  for (i = 0; i < n_columns; i++)
    {
      DzlGraphRenderer *renderer;

      renderer = g_object_new (DZL_TYPE_GRAPH_LINE_RENDERER,
                               "column", i,
                               "stroke-color", colors [i % G_N_ELEMENTS (colors)],
                               NULL);
      dzl_graph_view_add_renderer (DZL_GRAPH_VIEW (self), renderer);
      g_hash_table_insert (self->renderers, g_strdup_printf("cpu%d", i), renderer);
    }
}
