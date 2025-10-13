#include <config.h>

#include <glib/gi18n.h>
#include <glibtop/procopenfiles.h>

#include <sys/wait.h>

#include <set>
#include <string>
#include <sstream>
#include <iterator>

#include <glibmm/regex.h>

#include "application.h"
#include "procinfo.h"
#include "lsof-data.h"
#include "util.h"

#include "lsof.h"


using std::string;


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
Glib::RefPtr<Glib::Regex> re;

bool
matches (const string &filename) const
{
  return this->re->match (filename.c_str ());
}

public:
Lsof(const string &pattern,
     bool          caseless)
{
  Glib::Regex::CompileFlags compile_flags = static_cast<Glib::Regex::CompileFlags>(0);
  Glib::Regex::MatchFlags match_flags = static_cast<Glib::Regex::MatchFlags>(0);

  if (caseless)
    compile_flags |= Glib::Regex::CompileFlags::CASELESS;

  this->re = Glib::Regex::create (pattern.c_str (), compile_flags, match_flags);
}


template<typename OutputIterator>
void
search (const ProcInfo *info,
        OutputIterator  out) const
{
  glibtop_open_files_entry *entries;
  glibtop_proc_open_files buf;

  entries = glibtop_get_proc_open_files (&buf, info->pid);

  for (unsigned i = 0; i != buf.number; ++i)
    if (entries[i].type & GLIBTOP_FILE_TYPE_FILE)
      {
        const string filename (entries[i].info.file.name);
        if (this->matches (filename))
          *out++ = entries + i;
      }

  g_free (entries);
}
};


struct GUI: private procman::NonCopyable
{
  GListStore *model;
  GtkSearchEntry *entry;
  GtkWindow *dialog;
  GsmApplication *app;
  bool case_insensitive;


  GUI(
    GListStore *model_,
    GtkSearchEntry *entry_,
    GtkWindow *dialog_,
    GsmApplication *app_,
    bool case_insensitive_
  )
    : model (model_),
    entry (entry_),
    dialog (dialog_),
    app (app_),
    case_insensitive (case_insensitive_)
  {
    procman_debug ("New Lsof GUI %p", (void *)this);
  }

  ~GUI()
  {
    procman_debug ("Destroying Lsof GUI %p", (void *) this);
  }


  void
  update_count (unsigned count)
  {
    gchar *title;

    if (this->pattern ().length () == 0)
      title = g_strdup_printf (ngettext ("%d Open File", "%d Open Files", count), count);
    else
      title = g_strdup_printf (ngettext ("%d Matching Open File", "%d Matching Open Files", count), count);
    gtk_window_set_title (GTK_WINDOW (this->dialog), title);
    g_free (title);
  }

  string
  pattern () const
  {
    return gtk_editable_get_text (GTK_EDITABLE (this->entry));
  }


  void
  search ()
  {
    typedef std::set<EntryHolder> MatchSet;

    g_list_store_remove_all (this->model);

    try
      {
        Lsof lsof (this->pattern (), this->case_insensitive);
        g_autoptr (GPtrArray) files =
          g_ptr_array_new_null_terminated (20000, g_object_unref, TRUE);

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

        this->update_count (files->len);
      }
    catch (Glib::RegexError &error)
      {
      }
  }

  static void
  search_changed (GtkSearchEntry *,
                  gpointer data)
  {
    static_cast<GUI*>(data)->search ();
  }

  static void
  case_button_checked (GtkCheckButton *button,
                       gpointer        data)
  {
    bool state = gtk_check_button_get_active (button);

    static_cast<GUI*>(data)->case_insensitive = state;
  }
};
}

void
procman_lsof (GsmApplication *app)
{
  GtkBuilder *builder = gtk_builder_new ();
  GError *err = NULL;

  g_type_ensure (LSOF_TYPE_DATA);

  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/lsof.ui", &err);
  if (err != NULL)
    g_error ("%s", err->message);

  GtkWindow *dialog = GTK_WINDOW (gtk_builder_get_object (builder, "lsof_dialog"));
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (app->main_window));
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  GtkSearchBar *search_bar = GTK_SEARCH_BAR (gtk_builder_get_object (builder, "search_bar"));
  GtkSearchEntry *search_entry = GTK_SEARCH_ENTRY (gtk_builder_get_object (builder, "search_entry"));
  GtkCheckButton *case_button = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "case_button"));
  GListStore *model = G_LIST_STORE (gtk_builder_get_object (builder, "lsof_store"));

  gtk_search_bar_connect_entry (search_bar, GTK_EDITABLE (search_entry));
  gtk_search_bar_set_key_capture_widget (search_bar, GTK_WIDGET (dialog));

  // wil be deleted by the close button or delete-event
  GUI *gui = new GUI(
    model,
    search_entry,
    dialog,
    app,
    gtk_check_button_get_active (case_button)
  );

  g_signal_connect (G_OBJECT (search_entry), "search-changed",
                    G_CALLBACK (GUI::search_changed), gui);
  g_signal_connect (G_OBJECT (case_button), "toggled",
                    G_CALLBACK (GUI::case_button_checked), gui);

  gtk_widget_set_visible (GTK_WIDGET (dialog), TRUE);
  gui->search ();

  g_object_unref (G_OBJECT (builder));
}
