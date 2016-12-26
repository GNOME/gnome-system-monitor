/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <config.h>

#include <gtkmm/messagedialog.h>
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

        bool matches(const string &filename) const
        {
            return this->re->match(filename);
        }

    public:

        Lsof(const string &pattern, bool caseless)
        {
            Glib::RegexCompileFlags flags = static_cast<Glib::RegexCompileFlags>(0);

            if (caseless)
                flags |= Glib::REGEX_CASELESS;

            this->re = Glib::Regex::create(pattern, flags);
        }


        template<typename OutputIterator>
        void search(const ProcInfo &info, OutputIterator out) const
        {
            glibtop_open_files_entry *entries;
            glibtop_proc_open_files buf;

            entries = glibtop_get_proc_open_files(&buf, info.pid);

            for (unsigned i = 0; i != buf.number; ++i) {
                if (entries[i].type & GLIBTOP_FILE_TYPE_FILE) {
                    const string filename(entries[i].info.file.name);
                    if (this->matches(filename))
                        *out++ = filename;
                }
            }

            g_free(entries);
        }
    };



    // GUI Stuff


    enum ProcmanLsof {
        PROCMAN_LSOF_COL_PIXBUF,
        PROCMAN_LSOF_COL_PROCESS,
        PROCMAN_LSOF_COL_PID,
        PROCMAN_LSOF_COL_FILENAME,
        PROCMAN_LSOF_NCOLS
    };


    struct GUI : private procman::NonCopyable {

        GtkListStore *model;
        GtkSearchEntry *entry;
        GtkWindow *window;
        GtkLabel *count;
        GsmApplication *app;
        bool case_insensitive;
        bool regex_error_displayed;


        GUI()
            : model(NULL),
              entry(NULL),
              window(NULL),
              count(NULL),
              app(NULL),
              case_insensitive(),
              regex_error_displayed(false)
        {
            procman_debug("New Lsof GUI %p", this);
        }


        ~GUI()
        {
            procman_debug("Destroying Lsof GUI %p", this);
        }


        void update_count(unsigned count)
        {
            gchar *title;
            if (this->pattern().length() == 0) {
                title = g_strdup_printf (ngettext("%d open file", "%d open files", count), count);
            } else {
                title = g_strdup_printf (ngettext("%d matching open file", "%d matching open files", count), count);
            }
            gtk_window_set_title(this->window, title);
            g_free (title);
        }


        string pattern() const
        {
            return gtk_entry_get_text(GTK_ENTRY (this->entry));
        }


        void search()
        {
            typedef std::set<string> MatchSet;

            bool regex_error = false;

            gtk_list_store_clear(this->model);
            try {
                Lsof lsof(this->pattern(), this->case_insensitive);

                unsigned count = 0;

                for (const auto& v : ProcInfo::all) {
                    const auto& info = v.second;
                    MatchSet matches;
                    lsof.search(info, std::inserter(matches, matches.begin()));
                    count += matches.size();

                    for (const auto& match : matches) {
                        GtkTreeIter file;
                        gtk_list_store_append(this->model, &file);
                        gtk_list_store_set(this->model, &file,
                                           PROCMAN_LSOF_COL_PIXBUF, info.pixbuf->gobj(),
                                           PROCMAN_LSOF_COL_PROCESS, info.name,
                                           PROCMAN_LSOF_COL_PID, info.pid,
                                           PROCMAN_LSOF_COL_FILENAME, match.c_str(),
                                           -1);
                    }
                }

                this->update_count(count);
            }
            catch (Glib::RegexError& error) {
                regex_error = true;
            }

            if (regex_error && !this->regex_error_displayed) {
                this->regex_error_displayed = true;
                gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(entry)), GTK_STYLE_CLASS_ERROR);
            }
            else if (!regex_error && this->regex_error_displayed) {
                this->regex_error_displayed = false;
                gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(entry)), GTK_STYLE_CLASS_ERROR);
            }
        }


        static void search_changed(GtkSearchEntry *, gpointer data)
        {
            static_cast<GUI*>(data)->search();
        }


        static void close_button_clicked(GtkButton *, gpointer data)
        {
            GUI *gui = static_cast<GUI*>(data);
            gtk_widget_destroy(GTK_WIDGET(gui->window));
            delete gui;
        }


        static void case_button_toggled(GtkToggleButton *button, gpointer data)
        {
            bool state = gtk_toggle_button_get_active(button);
            static_cast<GUI*>(data)->case_insensitive = state;
        }


        static gboolean window_delete_event(GtkWidget *, GdkEvent *, gpointer data)
        {
            delete static_cast<GUI*>(data);
            return FALSE;
        }

    };
}




void procman_lsof(GsmApplication *app)
{
    GtkListStore *model = \
        gtk_list_store_new(PROCMAN_LSOF_NCOLS,
                           GDK_TYPE_PIXBUF, // PROCMAN_LSOF_COL_PIXBUF
                           G_TYPE_STRING,   // PROCMAN_LSOF_COL_PROCESS
                           G_TYPE_UINT,     // PROCMAN_LSOF_COL_PID
                           G_TYPE_STRING    // PROCMAN_LSOF_COL_FILENAME
            );

    GtkTreeView *tree = GTK_TREE_VIEW (gtk_tree_view_new_with_model(GTK_TREE_MODEL(model)));
    g_object_unref(model);

    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    // PIXBUF / PROCESS

    column = gtk_tree_view_column_new();

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "pixbuf", PROCMAN_LSOF_COL_PIXBUF,
                                        NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text", PROCMAN_LSOF_COL_PROCESS,
                                        NULL);

    gtk_tree_view_column_set_title(column, _("Process"));
    gtk_tree_view_column_set_sort_column_id(column, PROCMAN_LSOF_COL_PROCESS);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
    gtk_tree_view_column_set_min_width(column, 10);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), PROCMAN_LSOF_COL_PROCESS,
                                         GTK_SORT_ASCENDING);


    // PID
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("PID"), renderer,
                                                      "text", PROCMAN_LSOF_COL_PID,
                                                      NULL);
    gtk_tree_view_column_set_sort_column_id(column, PROCMAN_LSOF_COL_PID);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);


    // FILENAME
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Filename"), renderer,
                                                      "text", PROCMAN_LSOF_COL_FILENAME,
                                                      NULL);
    gtk_tree_view_column_set_sort_column_id(column, PROCMAN_LSOF_COL_FILENAME);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);


    GtkWindow *dialog;

    GtkBuilder *builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/lsof.ui", NULL);

    dialog = GTK_WINDOW (gtk_builder_get_object (builder, "lsof_dialog"));

    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (app->main_window));

    GtkSearchEntry *entry =  GTK_SEARCH_ENTRY (gtk_builder_get_object (builder, "entry"));

    GtkCheckButton *case_button =  GTK_CHECK_BUTTON (gtk_builder_get_object (builder, "case_button"));

    // Scrolled TreeView
    GtkScrolledWindow *scrolled = GTK_SCROLLED_WINDOW (gtk_builder_get_object (builder, "scrolled"));

    gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET (tree));

    GUI *gui = new GUI; // wil be deleted by the close button or delete-event
    gui->app = app;
    gui->model = model;
    gui->window = dialog;
    gui->entry = entry;
    gui->case_insensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (case_button));

    g_signal_connect(G_OBJECT(entry), "search-changed",
                     G_CALLBACK(GUI::search_changed), gui);
    g_signal_connect(G_OBJECT(case_button), "toggled",
                     G_CALLBACK(GUI::case_button_toggled), gui);
    g_signal_connect(G_OBJECT(dialog), "delete-event",
                     G_CALLBACK(GUI::window_delete_event), gui);

    gtk_builder_connect_signals (builder, NULL);

    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (GsmApplication::get()->main_window));
    gtk_widget_show_all(GTK_WIDGET (dialog));
    gui->search ();
    
    g_object_unref (G_OBJECT (builder));
}

