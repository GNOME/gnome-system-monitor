/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

#include <glib/gi18n.h>
#include <glibtop/procopenfiles.h>

#include <sys/wait.h>

#include "application.h"
#include "open-file.h"
#include "procinfo.h"
#include "util.h"

#include "lsof.h"


struct _GsmLsof {
  AdwWindow parent;

  GtkWidget *search_bar;
  GtkWidget *search_entry;
  
  GListStore *lsof_store;
};


G_DEFINE_FINAL_TYPE (GsmLsof, gsm_lsof, ADW_TYPE_WINDOW)


static char *
format_title (G_GNUC_UNUSED GObject *object,
              guint                  n_items,
              const char            *search)
{
  if (search && search[0]) {
    return g_strdup_printf (g_dngettext (G_LOG_DOMAIN, "%d Matching Open File", "%d Matching Open Files", n_items), n_items);
  }

  return g_strdup_printf (g_dngettext (G_LOG_DOMAIN, "%d Open File", "%d Open Files", n_items), n_items);
}


static inline void
load_process_files (GPtrArray *files, ProcInfo *info)
{
  g_autofree glibtop_open_files_entry *entries = NULL;
  glibtop_proc_open_files buf;

  entries = glibtop_get_proc_open_files (&buf, info->pid);

  for (unsigned i = 0; i != buf.number; ++i) {
    if (entries[i].type & GLIBTOP_FILE_TYPE_FILE) {
      g_ptr_array_add (files, gsm_open_file_new (info, entries + i));
    }
  }
}


typedef struct _LoadFilesData LoadFilesData;
struct _LoadFilesData {
  GPtrArray *processes;
};


static void
load_files_data_free (gpointer data)
{
  LoadFilesData *self = static_cast<LoadFilesData *>(data);

  g_clear_pointer (&self->processes, g_ptr_array_unref);

  g_free (self);
}


static void
load_files_thread (GTask        *task,
                   gpointer      source_object,
                   gpointer      task_data,
                   GCancellable *cancellable)
{
  LoadFilesData *data = static_cast<LoadFilesData *>(task_data);
  g_autoptr (GPtrArray) files =
    g_ptr_array_new_null_terminated (data->processes->len * 10,
                                     g_object_unref,
                                     TRUE);

  for (size_t i = 0; i < data->processes->len; i++) {
    load_process_files (files,
                        static_cast<ProcInfo *>(g_ptr_array_index (data->processes, i)));
  }

  g_task_return_pointer (task,
                         g_steal_pointer (&files),
                         (GDestroyNotify) g_ptr_array_unref);
}


static GPtrArray *
load_files_finish (GsmLsof *self, GAsyncResult *result, GError **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);

  return static_cast<GPtrArray *>(g_task_propagate_pointer (G_TASK (result), error));
}


static void
load_files (GsmLsof             *self,
            GAsyncReadyCallback  callback,
            gpointer             callback_data)
{
  g_autoptr (GTask) task = g_task_new (self, NULL, callback, callback_data);
  LoadFilesData *data = g_new0 (LoadFilesData, 1);

  data->processes =
    g_ptr_array_new_null_terminated (1000, NULL, TRUE);

  g_task_set_name (task, "load_files");
  g_task_set_task_data (task, data, load_files_data_free);

  for (auto &v : GsmApplication::get().processes) {
    g_ptr_array_add (data->processes, &v.second);
  }

  g_task_run_in_thread (task, load_files_thread);
}


static void
did_load (GObject *source, GAsyncResult *result, gpointer user_data)
{
  GsmLsof *self = GSM_LSOF (source);
  g_autoptr (GError) error = NULL;
  g_autoptr (GPtrArray) files = load_files_finish (self, result, &error);
  guint old_length = g_list_model_get_n_items (G_LIST_MODEL (self->lsof_store));

  if (error) {
    g_warning ("Failed to refresh lsof: %s", error->message);
    return;
  }

  g_list_store_splice (self->lsof_store,
                       0,
                       old_length,
                       files->pdata,
                       files->len);
}


static void
refresh (GtkWidget                *widget,
         G_GNUC_UNUSED const char *action_name,
         G_GNUC_UNUSED GVariant   *target)
{
  load_files (GSM_LSOF (widget), did_load, NULL);
}


static void
gsm_lsof_class_init (GsmLsofClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gnome-system-monitor/data/lsof.ui");

  gtk_widget_class_bind_template_child (widget_class, GsmLsof, search_bar);
  gtk_widget_class_bind_template_child (widget_class, GsmLsof, search_entry);
  gtk_widget_class_bind_template_child (widget_class, GsmLsof, lsof_store);

  gtk_widget_class_bind_template_callback (widget_class, format_title);

  gtk_widget_class_install_action (widget_class, "lsof.refresh", NULL, refresh);

  g_type_ensure (GSM_TYPE_OPEN_FILE);
}


static void
gsm_lsof_init (GsmLsof *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_search_bar_connect_entry (GTK_SEARCH_BAR (self->search_bar),
                                GTK_EDITABLE (self->search_entry));
  gtk_search_bar_set_key_capture_widget (GTK_SEARCH_BAR (self->search_bar),
                                         GTK_WIDGET (self));

  gtk_widget_activate_action (GTK_WIDGET (self), "lsof.refresh", NULL);
}


GsmLsof *
gsm_lsof_new (GtkWindow *transient_for)
{
  return GSM_LSOF (g_object_new (GSM_TYPE_LSOF,
                                 "transient-for", transient_for,
                                 NULL));
}
