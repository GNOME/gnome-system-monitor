/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>

#include "mem-map.h"


struct _GsmMemMap {
  GObject parent_instance;

  char     *filename;
  uint64_t  vm_start;
  uint64_t  vm_end;
  uint64_t  vm_size;
  char     *flags;
  uint64_t  vm_offset;
  uint64_t  private_clean;
  uint64_t  private_dirty;
  uint64_t  shared_clean;
  uint64_t  shared_dirty;
  char     *device;
  uint64_t  inode;
};


G_DEFINE_FINAL_TYPE (GsmMemMap, gsm_mem_map, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_FILENAME,
  PROP_VM_START,
  PROP_VM_END,
  PROP_VM_SIZE,
  PROP_FLAGS,
  PROP_VM_OFFSET,
  PROP_PRIVATE_CLEAN,
  PROP_PRIVATE_DIRTY,
  PROP_SHARED_CLEAN,
  PROP_SHARED_DIRTY,
  PROP_DEVICE,
  PROP_INODE,
  N_PROPS
};
static GParamSpec *properties[N_PROPS];


static void
gsm_mem_map_dispose (GObject *object)
{
  GsmMemMap *self = GSM_MEM_MAP (object);

  g_clear_pointer (&self->filename, g_free);
  g_clear_pointer (&self->flags, g_free);
  g_clear_pointer (&self->device, g_free);

  G_OBJECT_CLASS (gsm_mem_map_parent_class)->dispose (object);
}


static void
gsm_mem_map_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GsmMemMap *self = GSM_MEM_MAP (object);

  switch (property_id) {
    case PROP_FILENAME:
      g_value_set_string (value, self->filename);
      break;
    case PROP_VM_START:
      g_value_set_uint64 (value, self->vm_start);
      break;
    case PROP_VM_END:
      g_value_set_uint64 (value, self->vm_end);
      break;
    case PROP_VM_SIZE:
      g_value_set_uint64 (value, self->vm_size);
      break;
    case PROP_FLAGS:
      g_value_set_string (value, self->flags);
      break;
    case PROP_VM_OFFSET:
      g_value_set_uint64 (value, self->vm_offset);
      break;
    case PROP_PRIVATE_CLEAN:
      g_value_set_uint64 (value, self->private_clean);
      break;
    case PROP_PRIVATE_DIRTY:
      g_value_set_uint64 (value, self->private_dirty);
      break;
    case PROP_SHARED_CLEAN:
      g_value_set_uint64 (value, self->shared_clean);
      break;
    case PROP_SHARED_DIRTY:
      g_value_set_uint64 (value, self->shared_dirty);
      break;
    case PROP_DEVICE:
      g_value_set_string (value, self->device);
      break;
    case PROP_INODE:
      g_value_set_uint64 (value, self->inode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static inline void
set_str_prop (GObject       *object,
              GParamSpec    *pspec,
              char         **field,
              const GValue  *value)
{
  if (g_set_str (field, g_value_get_string (value))) {
    g_object_notify_by_pspec (object, pspec);
  }
}


static inline void
set_uint64_prop (GObject      *object,
                 GParamSpec   *pspec,
                 uint64_t     *field,
                 const GValue *value)
{
  uint64_t new_value = g_value_get_uint64 (value);

  if (*field == new_value) {
    return;
  }

  *field = new_value;
  g_object_notify_by_pspec (object, pspec);
}


static void
gsm_mem_map_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GsmMemMap *self = GSM_MEM_MAP (object);

  switch (property_id) {
    case PROP_FILENAME:
      set_str_prop (object, pspec, &self->filename, value);
      break;
    case PROP_VM_START:
      set_uint64_prop (object, pspec, &self->vm_start, value);
      break;
    case PROP_VM_END:
      set_uint64_prop (object, pspec, &self->vm_end, value);
      break;
    case PROP_VM_SIZE:
      set_uint64_prop (object, pspec, &self->vm_size, value);
      break;
    case PROP_FLAGS:
      set_str_prop (object, pspec, &self->flags, value);
      break;
    case PROP_VM_OFFSET:
      set_uint64_prop (object, pspec, &self->vm_offset, value);
      break;
    case PROP_PRIVATE_CLEAN:
      set_uint64_prop (object, pspec, &self->private_clean, value);
      break;
    case PROP_PRIVATE_DIRTY:
      set_uint64_prop (object, pspec, &self->private_dirty, value);
      break;
    case PROP_SHARED_CLEAN:
      set_uint64_prop (object, pspec, &self->shared_clean, value);
      break;
    case PROP_SHARED_DIRTY:
      set_uint64_prop (object, pspec, &self->shared_dirty, value);
      break;
    case PROP_DEVICE:
      set_str_prop (object, pspec, &self->device, value);
      break;
    case PROP_INODE:
      set_uint64_prop (object, pspec, &self->inode, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
gsm_mem_map_class_init (GsmMemMapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gsm_mem_map_dispose;
  object_class->get_property = gsm_mem_map_get_property;
  object_class->set_property = gsm_mem_map_set_property;

  properties [PROP_FILENAME] =
    g_param_spec_string ("filename", NULL, NULL,
                         NULL,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_VM_START] =
    g_param_spec_uint64 ("vm-start", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_VM_END] =
    g_param_spec_uint64 ("vm-end", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_VM_SIZE] =
    g_param_spec_uint64 ("vm-size", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_FLAGS] =
    g_param_spec_string ("flags", NULL, NULL,
                         NULL,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_VM_OFFSET] =
    g_param_spec_uint64 ("vm-offset", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_PRIVATE_CLEAN] =
    g_param_spec_uint64 ("private-clean", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_PRIVATE_DIRTY] =
    g_param_spec_uint64 ("private-dirty", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_SHARED_CLEAN] =
    g_param_spec_uint64 ("shared-clean", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_SHARED_DIRTY] =
    g_param_spec_uint64 ("shared-dirty", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_DEVICE] =
    g_param_spec_string ("device", NULL, NULL,
                         NULL,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties [PROP_INODE] =
    g_param_spec_uint64 ("inode", NULL, NULL,
                         0, G_MAXUINT64, 0,
                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}


static void
gsm_mem_map_init (GsmMemMap *)
{
}


GsmMemMap *
gsm_mem_map_new (const char *filename,
                 uint64_t    vm_start,
                 uint64_t    vm_end,
                 uint64_t    vm_size,
                 const char *flags,
                 uint64_t    vm_offset,
                 uint64_t    private_clean,
                 uint64_t    private_dirty,
                 uint64_t    shared_clean,
                 uint64_t    shared_dirty,
                 const char *device,
                 uint64_t    inode)
{
  return g_object_new (GSM_TYPE_MEM_MAP,
                       "filename", filename,
                       "vm-start", vm_start,
                       "vm-end", vm_end,
                       "vm-size", vm_size,
                       "flags", flags,
                       "vm-offset", vm_offset,
                       "private-clean", private_clean,
                       "private-dirty", private_dirty,
                       "shared-clean", shared_clean,
                       "shared-dirty", shared_dirty,
                       "device", device,
                       "inode", inode,
                       NULL);
}
