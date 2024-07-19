#include <glib/gi18n.h>

#include "memmaps-data.h"

struct _MemMapsData
{
  GObject parent_instance;

  gchar   *filename;
  gchar   *vmstart;
  gchar   *vmend;
  guint64  vmsize;
  gchar   *flags;
  gchar   *vmoffset;
  guint64  privateclean;
  guint64  privatedirty;
  guint64  sharedclean;
  guint64  shareddirty;
  gchar   *device;
  guint64  inode;
};

G_DEFINE_TYPE (MemMapsData, memmaps_data, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_FILENAME,
  PROP_VMSTART,
  PROP_VMEND,
  PROP_VMSIZE,
  PROP_FLAGS,
  PROP_VMOFFSET,
  PROP_PRIVATECLEAN,
  PROP_PRIVATEDIRTY,
  PROP_SHAREDCLEAN,
  PROP_SHAREDDIRTY,
  PROP_DEVICE,
  PROP_INODE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

MemMapsData *
memmaps_data_new (const gchar *filename,
                  const gchar *vmstart,
                  const gchar *vmend,
                  guint64      vmsize,
                  const gchar *flags,
                  const gchar *vmoffset,
                  guint64      privateclean,
                  guint64      privatedirty,
                  guint64      sharedclean,
                  guint64      shareddirty,
                  const gchar *device,
                  guint64      inode)
{
  return g_object_new (MEMMAPS_TYPE_DATA,
                       "filename", filename,
                       "vmstart", vmstart,
                       "vmend", vmend,
                       "vmsize", vmsize,
                       "flags", flags,
                       "vmoffset", vmoffset,
                       "privateclean", privateclean,
                       "privatedirty", privatedirty,
                       "sharedclean", sharedclean,
                       "shareddirty", shareddirty,
                       "device", device,
                       "inode", inode,
                       NULL);
}

static void
memmaps_data_finalize (GObject *object)
{
  G_OBJECT_CLASS (memmaps_data_parent_class)->finalize (object);
}

static void
memmaps_data_get_property (GObject    *object,
                           guint       pid,
                           GValue     *value,
                           GParamSpec *pspec)
{
  MemMapsData *self = MEMMAPS_DATA (object);

  switch (pid)
    {
    case PROP_FILENAME:
      g_value_set_string (value, self->filename);
      break;
    case PROP_VMSTART:
      g_value_set_string (value, self->vmstart);
      break;
    case PROP_VMEND:
      g_value_set_string (value, self->vmend);
      break;
    case PROP_VMSIZE:
      g_value_set_uint64 (value, self->vmsize);
      break;
    case PROP_FLAGS:
      g_value_set_string (value, self->flags);
      break;
    case PROP_VMOFFSET:
      g_value_set_string (value, self->vmoffset);
      break;
    case PROP_PRIVATECLEAN:
      g_value_set_uint64 (value, self->privateclean);
      break;
    case PROP_PRIVATEDIRTY:
      g_value_set_uint64 (value, self->privatedirty);
      break;
    case PROP_SHAREDCLEAN:
      g_value_set_uint64 (value, self->sharedclean);
      break;
    case PROP_SHAREDDIRTY:
      g_value_set_uint64 (value, self->shareddirty);
      break;
    case PROP_DEVICE:
      g_value_set_string (value, self->device);
      break;
    case PROP_INODE:
      g_value_set_uint64 (value, self->inode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
memmaps_data_set_property (GObject      *object,
                           guint         pid,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  MemMapsData *self = MEMMAPS_DATA (object);

  switch (pid)
    {
    case PROP_FILENAME:
      self->filename = g_value_dup_string (value);
      break;
    case PROP_VMSTART:
      self->vmstart = g_value_dup_string (value);
      break;
    case PROP_VMEND:
      self->vmend = g_value_dup_string (value);
      break;
    case PROP_VMSIZE:
      self->vmsize = g_value_get_uint64 (value);
      break;
    case PROP_FLAGS:
      self->flags = g_value_dup_string (value);
      break;
    case PROP_VMOFFSET:
      self->vmoffset = g_value_dup_string (value);
      break;
    case PROP_PRIVATECLEAN:
      self->privateclean = g_value_get_uint64 (value);
      break;
    case PROP_PRIVATEDIRTY:
      self->privatedirty = g_value_get_uint64 (value);
      break;
    case PROP_SHAREDCLEAN:
      self->sharedclean = g_value_get_uint64 (value);
      break;
    case PROP_SHAREDDIRTY:
      self->shareddirty = g_value_get_uint64 (value);
      break;
    case PROP_DEVICE:
      self->device = g_value_dup_string (value);
      break;
    case PROP_INODE:
      self->inode = g_value_get_uint64 (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
memmaps_data_class_init (MemMapsDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = memmaps_data_finalize;
  object_class->get_property = memmaps_data_get_property;
  object_class->set_property = memmaps_data_set_property;

  properties [PROP_FILENAME] =
    g_param_spec_string ("filename",
                         "Filename",
                         "Filename",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_VMSTART] =
    g_param_spec_string ("vmstart",
                         "Vmstart",
                         "VMStart",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_VMEND] =
    g_param_spec_string ("vmend",
                         "Vmend",
                         "VMEnd",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_VMSIZE] =
    g_param_spec_uint64 ("vmsize",
                         "Vmsize",
                         "VMSize",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  properties [PROP_FLAGS] =
    g_param_spec_string ("flags",
                         "Flags",
                         "Flags",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_VMOFFSET] =
    g_param_spec_string ("vmoffset",
                         "Vmoffset",
                         "VMOffset",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_PRIVATECLEAN] =
    g_param_spec_uint64 ("privateclean",
                         "Privateclean",
                         "PrivateClean",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  properties [PROP_PRIVATEDIRTY] =
    g_param_spec_uint64 ("privatedirty",
                         "Privatedirty",
                         "PrivateDirty",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  properties [PROP_SHAREDCLEAN] =
    g_param_spec_uint64 ("sharedclean",
                         "Sharedclean",
                         "SharedClean",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  properties [PROP_SHAREDDIRTY] =
    g_param_spec_uint64 ("shareddirty",
                         "Shareddirty",
                         "SharedDirty",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  properties [PROP_DEVICE] =
    g_param_spec_string ("device",
                         "Device",
                         "Device",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_INODE] =
    g_param_spec_uint64 ("inode",
                         "Inode",
                         "Inode",
                         0, G_MAXUINT64, 0,
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
memmaps_data_init (MemMapsData *)
{
}
