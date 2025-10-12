/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

#include <glibtop/procmap.h>
#include <glibtop/mountlist.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#ifdef HAVE_DEV_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif
#ifdef HAVE_DEV_IN_TYPES
#include <sys/types.h>
#endif

#include <string>
#include <map>
#include <stdexcept>

using std::string;


#include "memmaps.h"
#include "memmaps-data.h"
#include "procinfo.h"
#include "proctable.h"
#include "settings-keys.h"
#include "util.h"


namespace
{
class OffsetFormater
{
const char *_format;

public:

OffsetFormater (const glibtop_map_entry &last_map)
{
  this->_format = (last_map.end <= G_MAXUINT32) ? "%08" G_GINT64_MODIFIER "x" : "%016" G_GINT64_MODIFIER "x";
}

string
format (guint64 v) const
{
  char buffer[17];

  g_snprintf (buffer, sizeof buffer, this->_format, v);
  return buffer;
}
};

}

struct _GsmMemMapsView
{
  AdwWindow parent_instance;

  GtkColumnView *column_view;
  AdwWindowTitle *window_title;
  GListStore *list_store;

  ProcInfo *info;
  GHashTable *devices;

  guint timer;
};

G_DEFINE_FINAL_TYPE (GsmMemMapsView, gsm_memmaps_view, ADW_TYPE_WINDOW)


enum {
  PROP_0,
  PROP_INFO,
  N_PROPS
};
static GParamSpec *properties [N_PROPS];


static void
gsm_memmaps_view_dispose (GObject *object)
{
  GsmMemMapsView *self = GSM_MEMMAPS_VIEW (object);

  g_clear_handle_id (&self->timer, g_source_remove);
  g_clear_pointer (&self->devices, g_hash_table_unref);

  G_OBJECT_CLASS (gsm_memmaps_view_parent_class)->dispose (object);
}


static void
gsm_memmaps_view_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GsmMemMapsView *self = GSM_MEMMAPS_VIEW (object);

  switch (prop_id)
    {
    case PROP_INFO:
      g_value_set_pointer (value, self->info);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
gsm_memmaps_view_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GsmMemMapsView *self = GSM_MEMMAPS_VIEW (object);

  switch (prop_id)
    {
    case PROP_INFO:
      self->info = (ProcInfo *) g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

struct glibtop_map_entry_cmp
{
  bool
  operator() (const glibtop_map_entry &a,
              const guint64            start) const
  {
    return a.start < start;
  }

  bool
  operator() (const guint64 &          start,
              const glibtop_map_entry &a) const
  {
    return start < a.start;
  }
};


static inline char *
dev_as_string (dev_t dev)
{
  return g_strdup_printf ("%02x:%02x", major (dev), minor (dev));
}


static char *
get_device_name (GsmMemMapsView *self, dev_t dev)
{
  g_autofree char *name = NULL;
  const char *found_name;

  if (dev == 0) {
    return NULL;
  }

  name = dev_as_string (dev);
  found_name =
    static_cast<const char *>(g_hash_table_lookup (self->devices, name));

  if (found_name) {
    return g_strdup (found_name);
  }

  return g_steal_pointer (&name);
}


static void
update_row (GsmMemMapsView          *self,
            guint                    position,
            gboolean                 newrow,
            const glibtop_map_entry *memmaps,
            OffsetFormater          *formater)
{
  guint64 size;
  string filename;
  string vmstart, vmend, vmoffset;
  char flags[5] = "----";
  g_autofree char *device = NULL;

  size = memmaps->end - memmaps->start;

  if (memmaps->perm & GLIBTOP_MAP_PERM_READ)
    flags [0] = 'r';
  if (memmaps->perm & GLIBTOP_MAP_PERM_WRITE)
    flags [1] = 'w';
  if (memmaps->perm & GLIBTOP_MAP_PERM_EXECUTE)
    flags [2] = 'x';
  if (memmaps->perm & GLIBTOP_MAP_PERM_SHARED)
    flags [3] = 's';
  if (memmaps->perm & GLIBTOP_MAP_PERM_PRIVATE)
    flags [3] = 'p';

  if (memmaps->flags & (1 << GLIBTOP_MAP_ENTRY_FILENAME))
    filename = memmaps->filename;

  vmstart = formater->format (memmaps->start);
  vmend = formater->format (memmaps->end);
  vmoffset = formater->format (memmaps->offset);
  device = get_device_name (self, memmaps->device);

  MemMapsData *memmaps_data;

  if (!newrow)
    {
      memmaps_data = memmaps_data_new (filename.c_str (),
                                       vmstart.c_str (),
                                       vmend.c_str (),
                                       size,
                                       flags,
                                       vmoffset.c_str (),
                                       memmaps->private_clean,
                                       memmaps->private_dirty,
                                       memmaps->shared_clean,
                                       memmaps->shared_dirty,
                                       device,
                                       memmaps->inode);

      g_list_store_insert (self->list_store, position, memmaps_data);
    }
  else
    {
      memmaps_data = MEMMAPS_DATA (g_list_model_get_object (G_LIST_MODEL (self->list_store), position));

      g_object_set (memmaps_data,
                    "filename", filename.c_str (),
                    "vmstart", vmstart.c_str (),
                    "vmend", vmend.c_str (),
                    "vmsize", size,
                    "flags", flags,
                    "vmoffset", vmoffset.c_str (),
                    "privateclean", memmaps->private_clean,
                    "privatedirty", memmaps->private_dirty,
                    "sharedclean", memmaps->shared_clean,
                    "shareddirty", memmaps->shared_dirty,
                    "device", device,
                    "inode", memmaps->inode,
                    NULL);
    }
}


static void
update_devices (GsmMemMapsView *self)
{
  g_hash_table_remove_all (self->devices);

  glibtop_mountlist list;
  g_autofree glibtop_mountentry *entries = glibtop_get_mountlist (&list, 1);

  for (size_t i = 0; i != list.number; ++i) {
    GStatBuf buf;

    if (g_stat (entries[i].devname, &buf) != -1) {
      g_hash_table_insert (self->devices,
                           dev_as_string (buf.st_rdev),
                           g_strdup (entries[i].devname));
    }
  }
}


static void
update_memmaps_dialog (GsmMemMapsView *self)
{
  glibtop_map_entry *memmaps;
  glibtop_proc_map procmap;

  memmaps = glibtop_get_proc_map (&procmap, self->info->pid);
  /* process has disappeared */
  if (!memmaps or procmap.number == 0)
    return;

  OffsetFormater formater (memmaps[procmap.number - 1]);

  guint position = 0;

  typedef std::map<guint64, guint> IterCache;
  IterCache iter_cache;

  /*
    removes the old maps and
    also fills a cache of start -> iter in order to speed
    up add
  */

  while (position < g_list_model_get_n_items (G_LIST_MODEL (self->list_store)))
    {
      char *vmstart;
      guint64 start;
      MemMapsData *memmaps_data;

      memmaps_data = MEMMAPS_DATA (g_list_model_get_object (G_LIST_MODEL (self->list_store), position));
      g_object_get (memmaps_data, "vmstart", &vmstart, NULL);

      try {
          std::istringstream (vmstart) >> std::hex >> start;
        } catch (std::logic_error &e) {
          g_warning ("Could not parse %s", vmstart);
          start = 0;
        }

      g_free (vmstart);

      bool found = std::binary_search (memmaps, memmaps + procmap.number,
                                       start, glibtop_map_entry_cmp ());

      if (found)
        {
          iter_cache[start] = position;
          position++;
        }
      else if (g_list_store_find (self->list_store, memmaps_data, &position))
        g_list_store_remove (self->list_store, position);
      else
        break;
    }

  update_devices (self);

  /*
    add the new maps
  */

  for (guint i = 0; i != procmap.number; i++)
    {
      IterCache::iterator it (iter_cache.find (memmaps[i].start));

      if (it != iter_cache.end ())
        {
          position = it->second;
          update_row (self, position, TRUE, &memmaps[i], &formater);
        }
      else
        update_row (self, position, FALSE, &memmaps[i], &formater);
    }

  g_free (memmaps);
}


static gboolean
memmaps_timer (gpointer data)
{
  GsmMemMapsView *self = GSM_MEMMAPS_VIEW (data);

  g_assert (self->list_store);

  update_memmaps_dialog (self);

  return G_SOURCE_CONTINUE;
}


static char *
format_size (gpointer,
             guint64 size)
{
  return g_format_size_full (size, G_FORMAT_SIZE_IEC_UNITS);
}

static void
gsm_memmaps_view_constructed (GObject *object)
{
  GsmMemMapsView *self = GSM_MEMMAPS_VIEW (object);

  // Translators: process name and id
  char *subtitle = g_strdup_printf (_("%s (PID %u)"),
                                    self->info->name.c_str (), self->info->pid);
  adw_window_title_set_subtitle (self->window_title, subtitle);
  g_free (subtitle);

  self->timer = g_timeout_add_seconds (5, memmaps_timer, self);
  g_source_set_name_by_id (self->timer, "refresh memmaps");
  update_memmaps_dialog (self);

  G_OBJECT_CLASS (gsm_memmaps_view_parent_class)->constructed (object);
}

static void
gsm_memmaps_view_class_init (GsmMemMapsViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gsm_memmaps_view_dispose;
  object_class->get_property = gsm_memmaps_view_get_property;
  object_class->set_property = gsm_memmaps_view_set_property;
  object_class->constructed = gsm_memmaps_view_constructed;

  properties[PROP_INFO] =
    g_param_spec_pointer ("info", NULL, NULL,
                          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gnome-system-monitor/data/memmaps.ui");

  gtk_widget_class_bind_template_child (widget_class, GsmMemMapsView, column_view);
  gtk_widget_class_bind_template_child (widget_class, GsmMemMapsView, window_title);
  gtk_widget_class_bind_template_child (widget_class, GsmMemMapsView, list_store);

  gtk_widget_class_bind_template_callback (widget_class, format_size);

  g_type_ensure (MEMMAPS_TYPE_DATA);
}


static void
gsm_memmaps_view_init (GsmMemMapsView *self)
{
  self->devices = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         g_free);

  gtk_widget_init_template (GTK_WIDGET (self));
}


GsmMemMapsView *
gsm_memmaps_view_new (ProcInfo *info)
{
  return GSM_MEMMAPS_VIEW (g_object_new (GSM_TYPE_MEMMAPS_VIEW,
                                         "info", info,
                                         NULL));
}
