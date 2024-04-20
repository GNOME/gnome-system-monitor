#include <glib/gi18n.h>

#include "lsof-data.h"

struct _LsofData
{
  GObject parent_instance;

  GdkPaintable *paintable;
  gchar        *process;
  guint         pid;
  gchar        *filename;
};

G_DEFINE_TYPE (LsofData, lsof_data, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_PAINTABLE,
  PROP_PROCESS,
  PROP_PID,
  PROP_FILENAME,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

LsofData *
lsof_data_new (GdkPaintable *paintable,
               const gchar  *process,
               guint         pid,
               const gchar  *filename)
{
  return g_object_new (LSOF_TYPE_DATA,
                       "paintable", paintable,
                       "process", process,
                       "pid", pid,
                       "filename", filename,
                       NULL);
}

static void
lsof_data_finalize (GObject *object)
{
  G_OBJECT_CLASS (lsof_data_parent_class)->finalize (object);
}

static void
lsof_data_get_property (GObject    *object,
                        guint       pid,
                        GValue     *value,
                        GParamSpec *pspec)
{
  LsofData *self = LSOF_DATA(object);

  switch (pid)
    {
    case PROP_PAINTABLE:
      g_value_set_object (value, self->paintable);
      break;
    case PROP_PROCESS:
      g_value_set_string (value, self->process);
      break;
    case PROP_PID:
      g_value_set_uint (value, self->pid);
      break;
    case PROP_FILENAME:
      g_value_set_string (value, self->filename);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
lsof_data_set_property (GObject      *object,
                        guint         pid,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LsofData *self = LSOF_DATA(object);

  switch (pid)
    {
    case PROP_PAINTABLE:
      self->paintable = g_value_get_object (value);
      break;
    case PROP_PROCESS:
      self->process = g_value_dup_string (value);
      break;
    case PROP_PID:
      self->pid = g_value_get_uint (value);
      break;
    case PROP_FILENAME:
      self->filename = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
lsof_data_class_init (LsofDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = lsof_data_finalize;
  object_class->get_property = lsof_data_get_property;
  object_class->set_property = lsof_data_set_property;

  properties [PROP_PAINTABLE] =
    g_param_spec_object ("paintable",
                         "Paintable",
                         "Process icon",
                         GDK_TYPE_PAINTABLE,
                         G_PARAM_READWRITE);

  properties [PROP_PROCESS] =
    g_param_spec_string ("process",
                         "Process",
                         "Process name",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_PID] =
    g_param_spec_uint ("pid",
                       "Pid",
                       "Process PID",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE);

  properties [PROP_FILENAME] =
    g_param_spec_string ("filename",
                         "Filename",
                         "Process filename",
                         NULL,
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
lsof_data_init (LsofData *)
{
}
