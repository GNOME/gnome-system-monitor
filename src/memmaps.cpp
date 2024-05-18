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


#include "application.h"
#include "memmaps.h"
#include "memmaps-info.h"
#include "proctable.h"
#include "proctable-data.h"
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


class MemMapsData
{
public:
guint timer;
GtkColumnView *column_view;
ProcInfo *info;
OffsetFormater format;
mutable InodeDevices devices;
GListStore *store;

MemMapsData(GtkColumnView *a_column_view)
  : timer (),
  column_view (a_column_view),
  info (NULL),
  format (),
  devices (),
  store ()
{
}
};
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
update_row (guint                    position,
            gboolean                 newrow,
            const MemMapsData &      mm,
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

  vmstart = mm.format (memmaps->start);
  vmend = mm.format (memmaps->end);
  vmoffset = mm.format (memmaps->offset);
  device = mm.devices.get (memmaps->device);

  MemMapsInfo *memmaps_info;

  if (!newrow)
    {
      memmaps_info = memmaps_info_new (filename.c_str (),
                                       vmstart.c_str (),
                                       vmend.c_str (),
                                       g_format_size_full (size, G_FORMAT_SIZE_IEC_UNITS),
                                       flags,
                                       vmoffset.c_str (),
                                       g_format_size_full (memmaps->private_clean, G_FORMAT_SIZE_IEC_UNITS),
                                       g_format_size_full (memmaps->private_dirty, G_FORMAT_SIZE_IEC_UNITS),
                                       g_format_size_full (memmaps->shared_clean, G_FORMAT_SIZE_IEC_UNITS),
                                       g_format_size_full (memmaps->shared_dirty, G_FORMAT_SIZE_IEC_UNITS),
                                       device.c_str (),
                                       memmaps->inode);

      g_list_store_insert (mm.store, position, memmaps_info);
    }
  else
    {
      memmaps_info = MEMMAPS_INFO (g_list_model_get_object (G_LIST_MODEL (mm.store), position));

      g_object_set (memmaps_info,
                    "filename", filename.c_str (),
                    "vmstart", vmstart.c_str (),
                    "vmend", vmend.c_str (),
                    "vmsize", g_format_size_full (size, G_FORMAT_SIZE_IEC_UNITS),
                    "flags", flags,
                    "vmoffset", vmoffset.c_str (),
                    "privateclean", g_format_size_full (memmaps->private_clean, G_FORMAT_SIZE_IEC_UNITS),
                    "privatedirty", g_format_size_full (memmaps->private_dirty, G_FORMAT_SIZE_IEC_UNITS),
                    "sharedclean", g_format_size_full (memmaps->shared_clean, G_FORMAT_SIZE_IEC_UNITS),
                    "shareddirty", g_format_size_full (memmaps->shared_dirty, G_FORMAT_SIZE_IEC_UNITS),
                    "device", device.c_str (),
                    "inode", memmaps->inode,
                    NULL);
    }
}


static void
update_memmaps_dialog (MemMapsData *mmdata)
{
  glibtop_map_entry *memmaps;
  glibtop_proc_map procmap;

  memmaps = glibtop_get_proc_map (&procmap, mmdata->info->pid);
  /* process has disappeared */
  if (!memmaps or procmap.number == 0)
    return;

  mmdata->format.set (memmaps[procmap.number - 1]);

  guint position = 0;

  typedef std::map<guint64, guint> IterCache;
  IterCache iter_cache;

  /*
    removes the old maps and
    also fills a cache of start -> iter in order to speed
    up add
  */

  while (position < g_list_model_get_n_items (G_LIST_MODEL (mmdata->store)))
    {
      char *vmstart;
      guint64 start;
      MemMapsInfo *memmaps_info;

      memmaps_info = MEMMAPS_INFO (g_list_model_get_object (G_LIST_MODEL (mmdata->store), position));
      g_object_get (memmaps_info, "vmstart", &vmstart, NULL);

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
      else if (g_list_store_find (mmdata->store, memmaps_info, &position))
        g_list_store_remove (mmdata->store, position);
      else
        break;
    }

  mmdata->devices.update ();

  /*
    add the new maps
  */

  for (guint i = 0; i != procmap.number; i++)
    {
      IterCache::iterator it (iter_cache.find (memmaps[i].start));

      if (it != iter_cache.end ())
        {
          position = it->second;
          update_row (position, TRUE, *mmdata, &memmaps[i]);
        }
      else
        update_row (position, FALSE, *mmdata, &memmaps[i]);
    }

  g_free (memmaps);
}


static void
dialog_action (GSimpleAction*,
               GVariant*,
               GtkWindow* dialog)
{
  MemMapsData *mmdata;
  guint timer;

  mmdata = static_cast<MemMapsData *>(g_object_get_data (G_OBJECT (dialog), "mmdata"));

  timer = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (dialog), "timer"));
  g_source_remove (timer);

  delete mmdata;
  gtk_window_destroy (dialog);
}


static void
dialog_response (AdwWindow *dialog,
                 gpointer   data)
{
  MemMapsData * const mmdata = static_cast<MemMapsData*>(data);

  g_source_remove (mmdata->timer);

  delete mmdata;
  gtk_window_destroy (GTK_WINDOW (dialog));
}


static gboolean
memmaps_timer (gpointer data)
{
  MemMapsData * const mmdata = static_cast<MemMapsData*>(data);

  g_assert (mmdata->store);

  update_memmaps_dialog (mmdata);

  return TRUE;
}


static void
create_single_memmaps_dialog (GListModel *model,
                              guint       position,
                              gpointer)
{
  ProctableData *data;
  MemMapsData *mmdata;
  AdwWindow  *memmapsdialog;
  AdwWindowTitle *window_title;
  GtkColumnView *column_view;
  ProcInfo *info;
  GListStore *store;
  GSimpleActionGroup *action_group;

  data = PROCTABLE_DATA (g_list_model_get_object (model, position));
  g_object_get (data, "pointer", &info, NULL);

  if (!info)
    return;

  GtkBuilder *builder = gtk_builder_new ();
  GError *err = NULL;

  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/memmaps.ui", &err);
  if (err != NULL)
    g_error ("%s", err->message);

  memmapsdialog = ADW_WINDOW (gtk_builder_get_object (builder, "memmaps_dialog"));
  window_title = ADW_WINDOW_TITLE (gtk_builder_get_object (builder, "window_title"));
  column_view = GTK_COLUMN_VIEW (gtk_builder_get_object (builder, "openfiles_view"));
  store = G_LIST_STORE (gtk_builder_get_object (builder, "memmaps_store"));

  adw_window_title_set_subtitle (window_title, g_strdup_printf ("%s (PID %u)", info->name.c_str (), info->pid));

  mmdata = new MemMapsData (column_view);
  mmdata->info = info;
  mmdata->store = store;

  action_group = g_simple_action_group_new ();

  GSimpleAction *close_action = g_simple_action_new ("close", NULL);
  g_signal_connect (close_action, "activate", G_CALLBACK (dialog_action), memmapsdialog);
  g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (close_action));

  gtk_widget_insert_action_group (GTK_WIDGET (memmapsdialog), "memmaps", G_ACTION_GROUP (action_group));

  g_signal_connect (G_OBJECT (memmapsdialog), "close-request",
                    G_CALLBACK (dialog_response), mmdata);

  mmdata->timer = g_timeout_add_seconds (5, memmaps_timer, mmdata);
  g_object_set_data (G_OBJECT (memmapsdialog), "mmdata", mmdata);
  g_object_set_data (G_OBJECT (memmapsdialog), "timer", GUINT_TO_POINTER (mmdata->timer));
  update_memmaps_dialog (mmdata);

  gtk_window_present (GTK_WINDOW (memmapsdialog));

  g_object_unref (G_OBJECT (builder));
}


void
create_memmaps_dialog (GsmApplication *app)
{
  GListModel *model;
  GtkBitsetIter iter;
  GtkBitset *selection;
  guint position;

  selection = gtk_selection_model_get_selection (gtk_column_view_get_model (app->column_view));

  model = gtk_sort_list_model_get_model (GTK_SORT_LIST_MODEL (
                                           gtk_multi_selection_get_model (GTK_MULTI_SELECTION (
                                                                            gtk_column_view_get_model (app->column_view)))));

  /* TODO: do we really want to open multiple dialogs ? */
  for (gtk_bitset_iter_init_first (&iter, selection, &position);
       gtk_bitset_iter_is_valid (&iter);
       gtk_bitset_iter_next (&iter, &position))
    create_single_memmaps_dialog (model, position, app);
}
