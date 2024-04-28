#include <glib/gi18n.h>

#include "openfiles-data.h"

struct _OpenFilesData
{
  GObject parent_instance;

  gint      fd;
  gchar    *type;
  gchar    *object;
  gpointer  pointer;
};

G_DEFINE_TYPE (OpenFilesData, openfiles_data, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_FD,
  PROP_TYPE,
  PROP_OBJECT,
  PROP_POINTER,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

OpenFilesData *
openfiles_data_new (gint          fd,
                    const gchar  *type,
                    const gchar  *object,
                    gpointer      pointer)
{
  return g_object_new (OPENFILES_TYPE_DATA,
                       "fd", fd,
                       "type", type,
                       "object", object,
                       "pointer", pointer,
                       NULL);
}

static void
openfiles_data_finalize (GObject *object)
{
  G_OBJECT_CLASS (openfiles_data_parent_class)->finalize (object);
}

static void
openfiles_data_get_property (GObject    *object,
                             guint       pid,
                             GValue     *value,
                             GParamSpec *pspec)
{
  OpenFilesData *self = OPENFILES_DATA(object);

  switch (pid)
    {
    case PROP_FD:
      g_value_set_int (value, self->fd);
      break;
    case PROP_TYPE:
      g_value_set_string (value, self->type);
      break;
    case PROP_OBJECT:
      g_value_set_string (value, self->object);
      break;
    case PROP_POINTER:
      g_value_set_pointer (value, self->pointer);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
openfiles_data_set_property (GObject      *object,
                             guint         pid,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  OpenFilesData *self = OPENFILES_DATA(object);

  switch (pid)
    {
    case PROP_FD:
      self->fd = g_value_get_int (value);
      break;
    case PROP_TYPE:
      self->type = g_value_dup_string (value);
      break;
    case PROP_OBJECT:
      self->object = g_value_dup_string (value);
      break;
    case PROP_POINTER:
      self->pointer = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
openfiles_data_class_init (OpenFilesDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openfiles_data_finalize;
  object_class->get_property = openfiles_data_get_property;
  object_class->set_property = openfiles_data_set_property;

  properties [PROP_FD] =
    g_param_spec_int ("fd",
                      "Fd",
                      "FD",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE);

  properties [PROP_TYPE] =
    g_param_spec_string ("type",
                         "Type",
                         "Type",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_OBJECT] =
    g_param_spec_string ("object",
                         "Object",
                         "Object",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_POINTER] =
    g_param_spec_pointer ("pointer",
                          "Pointer",
                          "Pointer",
                          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
openfiles_data_init (OpenFilesData *)
{
}
