#include <config.h>

#include <glibtop/procmap.h>
#include <glibtop/mountlist.h>
#include <sys/stat.h>
#include <glib/gi18n.h>

#include <string>
#include <map>
#include <sstream>
#include <iomanip>
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
string format;

public:

void
set (const glibtop_map_entry &last_map)
{
  this->format = (last_map.end <= G_MAXUINT32) ? "%08" G_GINT64_MODIFIER "x" : "%016" G_GINT64_MODIFIER "x";
}

string
operator() (guint64 v) const
{
  char buffer[17];

  g_snprintf (buffer, sizeof buffer, this->format.c_str (), v);
  return buffer;
}
};

class InodeDevices
{
typedef std::map<guint16, string> Map;
Map devices;

public:

void
update ()
{
  this->devices.clear ();

  glibtop_mountlist list;
  glibtop_mountentry *entries = glibtop_get_mountlist (&list, 1);

  for (unsigned i = 0; i != list.number; ++i)
    {
      struct stat buf;

      if (stat (entries[i].devname, &buf) != -1)
        this->devices[buf.st_rdev] = entries[i].devname;
    }

  g_free (entries);
}

string
get (guint64 dev64)
{
  if (dev64 == 0)
    return "";

  guint16 dev = dev64 & 0xffff;

  if (dev != dev64)
    g_warning ("weird device %" G_GINT64_MODIFIER "x", dev64);

  Map::iterator it (this->devices.find (dev));

  if (it != this->devices.end ())
    return it->second;

  guint8 major, minor;

  major = dev >> 8;
  minor = dev;

  std::ostringstream out;

  out << std::hex
      << std::setfill ('0')
      << std::setw (2) << unsigned(major)
      << ':'
      << std::setw (2) << unsigned(minor);

  this->devices[dev] = out.str ();
  return out.str ();
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
  OffsetFormater format;
  mutable InodeDevices devices;

  guint timer;
};

G_DEFINE_TYPE (GsmMemMapsView, gsm_memmaps_view, ADW_TYPE_WINDOW)

enum {
  PROP_0,
  PROP_INFO,
  PROP_TIMER,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

GsmMemMapsView *
gsm_memmaps_view_new (ProcInfo *info)
{
  return GSM_MEMMAPS_VIEW (g_object_new (GSM_TYPE_MEMMAPS_VIEW,
                                         "info", info,
                                         NULL));
}

static void
gsm_memmaps_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (gsm_memmaps_view_parent_class)->finalize (object);
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
    case PROP_TIMER:
      g_value_set_uint (value, self->timer);
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
    case PROP_TIMER:
      self->timer = g_value_get_uint (value);
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


static void
update_row (GsmMemMapsView          *self,
            guint                    position,
            gboolean                 newrow,
            const glibtop_map_entry *memmaps)
{
  guint64 size;
  string filename, device;
  string vmstart, vmend, vmoffset;
  char flags[5] = "----";

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

  vmstart = self->format (memmaps->start);
  vmend = self->format (memmaps->end);
  vmoffset = self->format (memmaps->offset);
  device = self->devices.get (memmaps->device);

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
                                       device.c_str (),
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
                    "device", device.c_str (),
                    "inode", memmaps->inode,
                    NULL);
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

  self->format.set (memmaps[procmap.number - 1]);

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

  self->devices.update ();

  /*
    add the new maps
  */

  for (guint i = 0; i != procmap.number; i++)
    {
      IterCache::iterator it (iter_cache.find (memmaps[i].start));

      if (it != iter_cache.end ())
        {
          position = it->second;
          update_row (self, position, TRUE, &memmaps[i]);
        }
      else
        update_row (self, position, FALSE, &memmaps[i]);
    }

  g_free (memmaps);
}


static void
dialog_action (GSimpleAction*,
               GVariant*,
               GsmMemMapsView* self)
{
  g_source_remove (self->timer);

  gtk_window_destroy (GTK_WINDOW (self));
}


static void
dialog_response (GsmMemMapsView *self,
                 gpointer)
{
  g_source_remove (self->timer);

  gtk_window_destroy (GTK_WINDOW (self));
}


static gboolean
memmaps_timer (gpointer data)
{
  GsmMemMapsView *self = GSM_MEMMAPS_VIEW (data);

  g_assert (self->list_store);

  update_memmaps_dialog (self);

  return TRUE;
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
  adw_window_title_set_subtitle (self->window_title, g_strdup_printf (_("%s (PID %u)"),
                                   self->info->name.c_str (), self->info->pid));

  self->timer = g_timeout_add_seconds (5, memmaps_timer, self);
  update_memmaps_dialog (self);

  G_OBJECT_CLASS (gsm_memmaps_view_parent_class)->constructed (object);
}

static void
gsm_memmaps_view_class_init (GsmMemMapsViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gsm_memmaps_view_finalize;
  object_class->get_property = gsm_memmaps_view_get_property;
  object_class->set_property = gsm_memmaps_view_set_property;
  object_class->constructed = gsm_memmaps_view_constructed;

  properties [PROP_INFO] =
    g_param_spec_pointer ("info",
                          "Info",
                          "Info",
                          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  properties [PROP_TIMER] =
    g_param_spec_uint ("timer",
                       "Timer",
                       "Timer",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gnome-system-monitor/data/memmaps.ui");

  gtk_widget_class_bind_template_child (widget_class, GsmMemMapsView, column_view);
  gtk_widget_class_bind_template_child (widget_class, GsmMemMapsView, window_title);
  gtk_widget_class_bind_template_child (widget_class, GsmMemMapsView, list_store);

  gtk_widget_class_bind_template_callback (widget_class, format_size);
}

static void
gsm_memmaps_view_init (GsmMemMapsView *self)
{
  GSimpleActionGroup *action_group;

  g_type_ensure (MEMMAPS_TYPE_DATA);

  gtk_widget_init_template (GTK_WIDGET (self));

  action_group = g_simple_action_group_new ();

  GSimpleAction *close_action = g_simple_action_new ("close", NULL);
  g_signal_connect (close_action, "activate", G_CALLBACK (dialog_action), self);
  g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (close_action));

  gtk_widget_insert_action_group (GTK_WIDGET (self), "memmaps", G_ACTION_GROUP (action_group));

  g_signal_connect (G_OBJECT (self), "close-request",
                    G_CALLBACK (dialog_response), NULL);
}
