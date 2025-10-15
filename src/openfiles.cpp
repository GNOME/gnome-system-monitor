/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

#include <glib/gi18n.h>

#include <glibtop/procopenfiles.h>

#include "open-file.h"
#include "procinfo.h"

#include "openfiles.h"


struct _GsmOpenFiles {
  AdwWindow parent;

  ProcInfo *info;

  GtkWidget *window_title;

  GListStore *store;

  GHashTable *known_files;
  guint timer;
};


G_DEFINE_FINAL_TYPE (GsmOpenFiles, gsm_open_files, ADW_TYPE_WINDOW)


enum {
  PROP_0,
  PROP_INFO,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
gsm_open_files_dispose (GObject *object)
{
  GsmOpenFiles *self = GSM_OPEN_FILES (object);

  g_clear_pointer (&self->known_files, g_hash_table_unref);
  g_clear_handle_id (&self->timer, g_source_remove);

  G_OBJECT_CLASS (gsm_open_files_parent_class)->dispose (object);
}


static void
gsm_open_files_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GsmOpenFiles *self = GSM_OPEN_FILES (object);

  switch (property_id) {
    case PROP_INFO:
      g_value_set_pointer (value, self->info);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static inline GHashTable *
get_current_keys (GsmOpenFiles *self)
{
  /* the strings in the hash set are borrowed from the hash table, so
   * we don't need to free them */
  g_autoptr (GHashTable) old_keys =
    g_hash_table_new (g_str_hash, g_str_equal);
  GHashTableIter iter;
  const char *key;

  g_hash_table_iter_init (&iter, self->known_files);
  while (g_hash_table_iter_next (&iter, (gpointer *) &key, NULL)) {
    g_hash_table_add (old_keys, (gpointer *) key);
  }

  return g_steal_pointer (&old_keys);
}


static inline void
drop_leftover_keys (GsmOpenFiles *self, GHashTable *leftover_keys)
{
  GHashTableIter iter;
  const char *key;

  g_hash_table_iter_init (&iter, leftover_keys);
  while (g_hash_table_iter_next (&iter, (gpointer *) &key, NULL)) {
    g_autoptr (GsmOpenFile) file = NULL;
    g_autofree char *stolen_key = NULL;
    guint position;
    
    if (!g_hash_table_steal_extended (self->known_files,
                                      key,
                                      (gpointer *) &stolen_key,
                                      (gpointer *) &file)) {
      continue;
    }

    if (g_list_store_find (self->store, file, &position)) {
      g_list_store_remove (self->store, position);
    }
  }
}


static void
update_dialog (GsmOpenFiles *self)
{
  g_autofree glibtop_open_files_entry *openfiles = NULL;
  glibtop_proc_open_files procmap;
  g_autoptr (GHashTable) leftover_keys = NULL;
  g_autoptr (GPtrArray) new_files =
    g_ptr_array_new_null_terminated (100, g_object_unref, TRUE);


  // Translators: process name and id
  char *subtitle = g_strdup_printf (_("%s (PID %u)"), self->info->name.c_str (), self->info->pid);
  adw_window_title_set_subtitle (ADW_WINDOW_TITLE (self->window_title), subtitle);
  g_free (subtitle);

  openfiles = glibtop_get_proc_open_files (&procmap, self->info->pid);

  if (!openfiles) {
    g_hash_table_remove_all (self->known_files);
    g_list_store_remove_all (self->store);
    return;
  }

  /* Get a copy of the keys we already know about so we can
   * derive which keys closed in the mean time, and drop them */
  leftover_keys = get_current_keys (self);

  /* Update or insert files */
  for (size_t i = 0; i < procmap.number; i++) {
    glibtop_open_files_entry *entry = openfiles + i;
    g_autofree char *key = g_strdup_printf ("%d#%d", entry->fd, entry->type);
    GsmOpenFile *existing_file;

    /* If this was already a known key, save it from being cleaned up */
    g_hash_table_remove (leftover_keys, key);

    existing_file = static_cast<GsmOpenFile *>(g_hash_table_lookup (self->known_files, key));
    if (existing_file) {
      gsm_open_file_set_entry (existing_file, entry);
    } else {
      g_autoptr (GsmOpenFile) file = gsm_open_file_new (self->info, entry);

      g_hash_table_insert (self->known_files,
                           g_steal_pointer (&key),
                           g_object_ref (file));
      g_ptr_array_add (new_files, g_steal_pointer (&file));
    }
  }

  g_list_store_splice (self->store, 0, 0, new_files->pdata, new_files->len);

  /* Kill closed files */
  drop_leftover_keys (self, leftover_keys);
}


static void
gsm_open_files_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GsmOpenFiles *self = GSM_OPEN_FILES (object);

  switch (property_id) {
    case PROP_INFO:
      self->info = static_cast<ProcInfo *>(g_value_get_pointer (value));
      update_dialog (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
gsm_open_files_class_init (GsmOpenFilesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gsm_open_files_dispose;
  object_class->get_property = gsm_open_files_get_property;
  object_class->set_property = gsm_open_files_set_property;

  pspecs[PROP_INFO] =
    g_param_spec_pointer ("info", NULL, NULL,
                          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gnome-system-monitor/data/openfiles.ui");

  gtk_widget_class_bind_template_child (widget_class, GsmOpenFiles, window_title);
  gtk_widget_class_bind_template_child (widget_class, GsmOpenFiles, store);

  g_type_ensure (GSM_TYPE_OPEN_FILE);
}


static gboolean
openfiles_timer (gpointer data)
{
  GsmOpenFiles *self = GSM_OPEN_FILES (data);

  update_dialog (self);

  return G_SOURCE_CONTINUE;
}


static void
gsm_open_files_init (GsmOpenFiles *self)
{
  self->known_files = g_hash_table_new_full (g_str_hash,
                                             g_str_equal,
                                             g_free,
                                             g_object_unref);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->timer = g_timeout_add_seconds (5, openfiles_timer, self);
  g_source_set_name_by_id (self->timer, "refresh openfiles");
}


GsmOpenFiles *
gsm_open_files_new (GtkApplication *app, ProcInfo *info)
{
  return GSM_OPEN_FILES (g_object_new (GSM_TYPE_OPEN_FILES,
                                       "application", app,
                                       "info", info,
                                       NULL));
}
