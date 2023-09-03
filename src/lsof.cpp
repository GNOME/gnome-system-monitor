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

enum ProcmanLsof
{
  PROCMAN_LSOF_COL_TEXTURE,
  PROCMAN_LSOF_COL_PROCESS,
  PROCMAN_LSOF_COL_PID,
  PROCMAN_LSOF_COL_FILENAME,
  PROCMAN_LSOF_NCOLS
};


struct GUI: private procman::NonCopyable
{
  GtkListStore *model;
  GtkSearchEntry *entry;
  GtkWindow *dialog;
  GtkLabel *count;
  GsmApplication *app;
  bool case_insensitive;
  bool regex_error_displayed;


  GUI()
    : model (NULL),
    entry (NULL),
    dialog (NULL),
    count (NULL),
    app (NULL),
    case_insensitive (),
    regex_error_displayed (false)
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
      title = g_strdup_printf (ngettext ("%d open file", "%d open files", count), count);
    else
      title = g_strdup_printf (ngettext ("%d matching open file", "%d matching open files", count), count);
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

    bool regex_error = false;

    gtk_list_store_clear (this->model);

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
                GtkTreeIter file;
                gtk_list_store_append (this->model, &file);
                gtk_list_store_set (this->model, &file,
                                    PROCMAN_LSOF_COL_TEXTURE, info.icon->gobj (),
                                    PROCMAN_LSOF_COL_PROCESS, info.name.c_str (),
                                    PROCMAN_LSOF_COL_PID, info.pid,
                                    PROCMAN_LSOF_COL_FILENAME, match.c_str (),
                                    -1);
              }
          }

        this->update_count (count);
      }
    catch (Glib::RegexError &error)
      {
        regex_error = true;
      }

    if (regex_error && !this->regex_error_displayed)
      this->regex_error_displayed = true;
    else if (!regex_error && this->regex_error_displayed)
      this->regex_error_displayed = false;
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
  GtkListStore *model = \
    gtk_list_store_new (PROCMAN_LSOF_NCOLS,
                        GDK_TYPE_TEXTURE,   // PROCMAN_LSOF_COL_TEXTURE
                        G_TYPE_STRING,      // PROCMAN_LSOF_COL_PROCESS
                        G_TYPE_UINT,        // PROCMAN_LSOF_COL_PID
                        G_TYPE_STRING       // PROCMAN_LSOF_COL_FILENAME
                        );

  GtkTreeView *tree = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (model)));

  g_object_unref (model);

  // TEXTURE / PROCESS

  GtkTreeViewColumn *column = gtk_tree_view_column_new ();
  GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new ();

  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "texture", PROCMAN_LSOF_COL_TEXTURE,
                                       NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "text", PROCMAN_LSOF_COL_PROCESS,
                                       NULL);

  gtk_tree_view_column_set_title (column, _("Process"));
  gtk_tree_view_column_set_sort_column_id (column, PROCMAN_LSOF_COL_PROCESS);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_min_width (column, 10);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), PROCMAN_LSOF_COL_PROCESS,
                                        GTK_SORT_ASCENDING);

  // PID
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("PID"), renderer,
                                                     "text", PROCMAN_LSOF_COL_PID,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, PROCMAN_LSOF_COL_PID);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  // FILENAME
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Filename"), renderer,
                                                     "text", PROCMAN_LSOF_COL_FILENAME,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, PROCMAN_LSOF_COL_FILENAME);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  GtkBuilder *builder = gtk_builder_new ();
  GError *err = NULL;

  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/lsof.ui", &err);
  if (err != NULL)
    g_error ("%s", err->message);

  GtkWindow *dialog = GTK_WINDOW (gtk_builder_get_object (builder, "lsof_dialog"));
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (app->main_window));

  GtkSearchBar *search_bar = GTK_SEARCH_BAR (gtk_builder_get_object (builder, "search_bar"));
  GtkSearchEntry *search_entry = GTK_SEARCH_ENTRY (gtk_builder_get_object (builder, "search_entry"));
  GtkCheckButton *case_button = GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "case_button"));
  GtkWidget *scrolled = GTK_WIDGET (gtk_builder_get_object (builder, "scrolled"));

  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), GTK_WIDGET (tree));
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

  gtk_widget_show (GTK_WIDGET (dialog));
  gui->search ();

  g_object_unref (G_OBJECT (builder));
}
