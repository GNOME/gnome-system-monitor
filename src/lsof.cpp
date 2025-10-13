/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

 #include <config.h>

#include <glib/gi18n.h>
#include <glibtop/procopenfiles.h>

#include <sys/wait.h>

#include <set>
#include <iterator>

#include "application.h"
#include "procinfo.h"
#include "lsof-data.h"
#include "util.h"

#include "lsof.h"


namespace
{

class EntryHolder {
  glibtop_open_files_entry *ptr;

public:
  EntryHolder (glibtop_open_files_entry *entry) : ptr(entry) {
    this->ptr =
      static_cast<glibtop_open_files_entry *>(g_boxed_copy (glibtop_open_files_entry_get_type (),
                                                            static_cast<gconstpointer>(entry)));
  }

  EntryHolder(const EntryHolder &that) {
    this->ptr =
      static_cast<glibtop_open_files_entry *>(g_boxed_copy (glibtop_open_files_entry_get_type (),
                                                            static_cast<gconstpointer>(that.ptr)));
  };

  EntryHolder& operator=(const EntryHolder &) = delete;

  ~EntryHolder () {
    g_free (this->ptr);
  }

  glibtop_open_files_entry *operator* () {
    return this->ptr;
  }

  operator glibtop_open_files_entry* () const {
    return this->ptr;
  }
};


class Lsof
{

public:
template<typename OutputIterator>
void
search (const ProcInfo *info,
        OutputIterator  out) const
{
  glibtop_open_files_entry *entries;
  glibtop_proc_open_files buf;

  entries = glibtop_get_proc_open_files (&buf, info->pid);

  for (unsigned i = 0; i != buf.number; ++i) {
    if (entries[i].type & GLIBTOP_FILE_TYPE_FILE) {
      *out++ = entries + i;
    }
  }

  g_free (entries);
}
};


struct GUI: private procman::NonCopyable
{
  GListStore *model;
  GtkWindow *dialog;
  GsmApplication *app;


  GUI(
    GListStore *model_,
    GtkWindow *dialog_,
    GsmApplication *app_
  )
    : model (model_),
    dialog (dialog_),
    app (app_)
  {
    procman_debug ("New Lsof GUI %p", (void *)this);
  }

  ~GUI()
  {
    procman_debug ("Destroying Lsof GUI %p", (void *) this);
  }

  void
  search ()
  {
    typedef std::set<EntryHolder> MatchSet;

    g_list_store_remove_all (this->model);

    try
      {
        g_autoptr (GPtrArray) files =
          g_ptr_array_new_null_terminated (20000, g_object_unref, TRUE);
        Lsof lsof;

        for (auto&v : app->processes)
          {
            auto info = &v.second;
            MatchSet matches;
            lsof.search (info, std::inserter (matches, matches.begin ()));

            for (auto entry : matches)
              {
                g_autoptr (LsofData) data =
                  lsof_data_new (GDK_PAINTABLE (info->icon->gobj ()),
                                 info->name.c_str (),
                                 info->pid,
                                 (* entry)->info.file.name);

                g_ptr_array_add (files, g_steal_pointer (&data));
              }
          }

        g_list_store_splice (this->model, 0, 0, files->pdata, files->len);
      }
    catch (Glib::RegexError &error)
      {
      }
  }
};
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

  // wil be deleted by the close button or delete-event
  GUI *gui = new GUI(model, dialog, app);

  gtk_widget_set_visible (GTK_WIDGET (dialog), TRUE);
  gui->search ();
}
