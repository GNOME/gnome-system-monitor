/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

#include <glib/gi18n.h>
#include <glibtop/procopenfiles.h>

#include <sys/wait.h>

#include "application.h"
#include "procinfo.h"
#include "lsof-data.h"
#include "util.h"

#include "lsof.h"


static inline void
load_process_files (GPtrArray *files, ProcInfo *info)
{
  g_autofree glibtop_open_files_entry *entries = NULL;
  glibtop_proc_open_files buf;

  entries = glibtop_get_proc_open_files (&buf, info->pid);

  for (unsigned i = 0; i != buf.number; ++i) {
    if (entries[i].type & GLIBTOP_FILE_TYPE_FILE) {
      g_ptr_array_add (files,
                       lsof_data_new (GDK_PAINTABLE (info->icon->gobj ()),
                                      info->name.c_str (),
                                      info->pid,
                                      entries[i].info.file.name));
    }
  }
}


static void
load_files (GsmApplication *app, GListStore *model)
{
  guint old_length = g_list_model_get_n_items (G_LIST_MODEL (model));
  g_autoptr (GPtrArray) files =
    g_ptr_array_new_null_terminated (MAX (old_length, 20000), g_object_unref, TRUE);

  for (auto &v : app->processes) {
    load_process_files (files, &v.second);
  }

  g_list_store_splice (model, 0, old_length, files->pdata, files->len);
}


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


void
procman_lsof (GsmApplication *app)
{
  g_autoptr (GtkBuilderScope) scope = gtk_builder_cscope_new ();
  g_autoptr (GtkBuilder) builder = gtk_builder_new ();
  GError *err = NULL;

  g_type_ensure (LSOF_TYPE_DATA);

  gtk_builder_cscope_add_callback (scope, format_title);
  gtk_builder_set_scope (builder, scope);

  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/lsof.ui", &err);
  if (err != NULL)
    g_error ("%s", err->message);

  GtkWindow *dialog = GTK_WINDOW (gtk_builder_get_object (builder, "lsof_dialog"));
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (app->main_window));
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  GtkSearchBar *search_bar = GTK_SEARCH_BAR (gtk_builder_get_object (builder, "search_bar"));
  GtkSearchEntry *search_entry = GTK_SEARCH_ENTRY (gtk_builder_get_object (builder, "search_entry"));
  GListStore *model = G_LIST_STORE (gtk_builder_get_object (builder, "lsof_store"));

  gtk_search_bar_connect_entry (search_bar, GTK_EDITABLE (search_entry));
  gtk_search_bar_set_key_capture_widget (search_bar, GTK_WIDGET (dialog));

  gtk_widget_set_visible (GTK_WIDGET (dialog), TRUE);

  load_files (app, model);
}
