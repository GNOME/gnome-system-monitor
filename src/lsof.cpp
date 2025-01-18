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
#include "lsof.h"
#include "lsof-data.h"
#include "procinfo.h"
#include "util.h"


using std::string;


namespace
{
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
search (const ProcInfo &info,
        OutputIterator  out) const
{
  glibtop_open_files_entry *entries;
  glibtop_proc_open_files buf;

  entries = glibtop_get_proc_open_files (&buf, info.pid);

  for (unsigned i = 0; i != buf.number; ++i)
    if (entries[i].type & GLIBTOP_FILE_TYPE_FILE)
      {
        const string filename (entries[i].info.file.name);
        if (this->matches (filename))
          *out++ = filename;
      }

  g_free (entries);
}
};


struct GUI: private procman::NonCopyable
{
  GListStore *model;
  GtkSearchEntry *entry;
  GtkWindow *dialog;
  GtkLabel *count;
  GsmApplication *app;
  bool case_insensitive;


  GUI()
    : model (NULL),
    entry (NULL),
    dialog (NULL),
    count (NULL),
    app (NULL),
    case_insensitive ()
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
    typedef std::set<string> MatchSet;

    g_list_store_remove_all (this->model);

    try
      {
        Lsof lsof (this->pattern (), this->case_insensitive);

        unsigned count = 0;

        for (const auto&v : app->processes)
          {
            const auto&info = v.second;
            MatchSet matches;
            lsof.search (info, std::inserter (matches, matches.begin ()));
            count += matches.size ();

            for (const auto&match : matches)
              {
                LsofData *data;

                data = lsof_data_new (GDK_PAINTABLE (info.icon->gobj ()), info.name.c_str(), info.pid, match.c_str());

                g_list_store_append (this->model, data);

                g_object_unref (data);
              }
          }

        this->update_count (count);
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

  GUI *gui = new GUI;   // wil be deleted by the close button or delete-event

  gui->app = app;
  gui->model = model;
  gui->dialog = dialog;
  gui->entry = search_entry;
  gui->case_insensitive = gtk_check_button_get_active (case_button);

  g_signal_connect (G_OBJECT (search_entry), "search-changed",
                    G_CALLBACK (GUI::search_changed), gui);
  g_signal_connect (G_OBJECT (case_button), "toggled",
                    G_CALLBACK (GUI::case_button_checked), gui);

  gtk_widget_set_visible (GTK_WIDGET (dialog), TRUE);
  gui->search ();

  g_object_unref (G_OBJECT (builder));
}
