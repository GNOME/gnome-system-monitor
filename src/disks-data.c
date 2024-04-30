#include <glib/gi18n.h>

#include "disks-data.h"

struct _DisksData
{
  GObject parent_instance;

  GdkPaintable *paintable;
  gchar        *device;
  gchar        *directory;
  gchar        *type;
  guint64       total;
  guint64       free;
  guint64       available;
  guint64       used;
  gint          percentage;
};

G_DEFINE_TYPE (DisksData, disks_data, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_PAINTABLE,
  PROP_DEVICE,
  PROP_DIRECTORY,
  PROP_TYPE,
  PROP_TOTAL,
  PROP_FREE,
  PROP_AVAILABLE,
  PROP_USED,
  PROP_PERCENTAGE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

DisksData *
disks_data_new (GdkPaintable *paintable,
                const gchar  *device,
                const gchar  *directory,
                const gchar  *type,
                guint64       total,
                guint64       free,
                guint64       available,
                guint64       used,
                gint          percentage)
{
  return g_object_new (DISKS_TYPE_DATA,
                       "paintable", paintable,
                       "device", device,
                       "directory", directory,
                       "type", type,
                       "total", total,
                       "free", free,
                       "available", available,
                       "used", used,
                       "percentage", percentage,
                       NULL);
}

static void
disks_data_finalize (GObject *object)
{
  G_OBJECT_CLASS (disks_data_parent_class)->finalize (object);
}

static void
disks_data_get_property (GObject    *object,
                         guint       pid,
                         GValue     *value,
                         GParamSpec *pspec)
{
  DisksData *self = DISKS_DATA(object);

  switch (pid)
    {
    case PROP_PAINTABLE:
      g_value_set_object (value, self->paintable);
      break;
    case PROP_DEVICE:
      g_value_set_string (value, self->device);
      break;
    case PROP_DIRECTORY:
      g_value_set_string (value, self->directory);
      break;
    case PROP_TYPE:
      g_value_set_string (value, self->type);
      break;
    case PROP_TOTAL:
      g_value_set_uint64 (value, self->total);
      break;
    case PROP_FREE:
      g_value_set_uint64 (value, self->free);
      break;
    case PROP_AVAILABLE:
      g_value_set_uint64 (value, self->available);
      break;
    case PROP_USED:
      g_value_set_uint64 (value, self->used);
      break;
    case PROP_PERCENTAGE:
      g_value_set_int (value, self->percentage);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
disks_data_set_property (GObject      *object,
                         guint         pid,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  DisksData *self = DISKS_DATA(object);

  switch (pid)
    {
    case PROP_PAINTABLE:
      self->paintable = g_value_get_object (value);
      break;
    case PROP_DEVICE:
      self->device = g_value_dup_string (value);
      break;
    case PROP_DIRECTORY:
      self->directory = g_value_dup_string (value);
      break;
    case PROP_TYPE:
      self->type = g_value_dup_string (value);
      break;
    case PROP_TOTAL:
      self->total = g_value_get_uint64 (value);
      break;
    case PROP_FREE:
      self->free = g_value_get_uint64 (value);
      break;
    case PROP_AVAILABLE:
      self->available = g_value_get_uint64 (value);
      break;
    case PROP_USED:
      self->used = g_value_get_uint64 (value);
      break;
    case PROP_PERCENTAGE:
      self->percentage = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
disks_data_class_init (DisksDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = disks_data_finalize;
  object_class->get_property = disks_data_get_property;
  object_class->set_property = disks_data_set_property;

  properties [PROP_PAINTABLE] =
    g_param_spec_object ("paintable",
                         "Paintable",
                         "Disk icon",
                         GDK_TYPE_PAINTABLE,
                         G_PARAM_READWRITE);

  properties [PROP_DEVICE] =
    g_param_spec_string ("device",
                         "Device",
                         "Disk device",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_DIRECTORY] =
    g_param_spec_string ("directory",
                         "Directory",
                         "Disk directory",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_TYPE] =
    g_param_spec_string ("type",
                         "Type",
                         "Disk type",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_TOTAL] =
    g_param_spec_uint64 ("total",
                         "Total",
                         "Disk total",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  properties [PROP_FREE] =
    g_param_spec_uint64 ("free",
                         "Free",
                         "Disk free",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  properties [PROP_AVAILABLE] =
    g_param_spec_uint64 ("available",
                         "Available",
                         "Disk available",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  properties [PROP_USED] =
    g_param_spec_uint64 ("used",
                         "Used",
                         "Disk used",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  properties [PROP_PERCENTAGE] =
    g_param_spec_int ("percentage",
                      "Percentage",
                      "Disk percentage",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
disks_data_init (DisksData *)
{
}
