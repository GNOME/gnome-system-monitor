#include <config.h>

#include <glib/gi18n.h>
#include <glibtop/procopenfiles.h>

#include <set>
#include <string>

extern "C" {
  #include "procman.h"
  #include "lsof.h"
  #include "util.h"
}

#include <pcrecpp.h>


using std::set;
using std::string;


namespace {

  class Lsof {
    typedef set<string> ResultSet;
  public:
    typedef ResultSet::const_iterator iterator;

    Lsof(const string &pattern)
      : re(pattern, pcrecpp::RE_Options().set_caseless(true).set_utf8(true)) { }

    iterator begin() const { return this->result_set.begin(); }
    iterator end() const { return this->result_set.end(); }

    void clear() { this->result_set.clear(); }

    void search(const ProcInfo &info);

  private:
    pcrecpp::RE re;
    ResultSet result_set;
  };


  void Lsof::search(const ProcInfo &info)
  { 
    glibtop_open_files_entry *entries;
    glibtop_proc_open_files buf;

    entries = glibtop_get_proc_open_files(&buf, info.pid);

    for (unsigned i = 0; i != buf.number; ++i) {
      if (entries[i].type & GLIBTOP_FILE_TYPE_FILE) {
        const string filename(entries[i].info.file.name);
	if (this->re.PartialMatch(filename))
          this->result_set.insert(filename);
      }
    }

    g_free(entries);
  }



  void clear(ProcData *, GtkListStore *tree)
  {
    gtk_list_store_clear(tree);
  }


  void search(ProcData *data, const string &string_pattern, GtkListStore *tree)
  {
    clear(data, tree);

    Lsof pattern(string_pattern);

    for (GList *i = data->info; i; i = i->next) {
      ProcInfo &info(*static_cast<ProcInfo*>(i->data));
      pattern.search(info);

      for (Lsof::iterator it(pattern.begin()), end(pattern.end()); it != end; ++it) {
	GtkTreeIter file;
	gtk_list_store_append(tree, &file);
	gtk_list_store_set(tree, &file,
			   PROCMAN_LSOF_COL_PIXBUF, info.pixbuf,
			   PROCMAN_LSOF_COL_PROCESS, info.name,
			   PROCMAN_LSOF_COL_PID, info.pid,
			   PROCMAN_LSOF_COL_FILENAME, it->c_str(),
			   -1);
      }

      pattern.clear();
    }
  }


  // GUI stuff

  void close_dialog(GtkDialog *dialog, gint, gpointer)
  {
    gtk_widget_destroy(GTK_WIDGET(dialog));
  }


  struct ButtonClickedData {
    GtkListStore *model;
    GtkEntry *entry;
    ProcData *procdata;
  };

  static ButtonClickedData bcd;


  void search_button_clicked(GtkButton *button, gpointer data)
  {
    ButtonClickedData *bcd = static_cast<ButtonClickedData*>(data);
    search(bcd->procdata, gtk_entry_get_text(bcd->entry), bcd->model);
  }


  void clear_button_clicked(GtkButton *button, gpointer data)
  {
    ButtonClickedData *bcd = static_cast<ButtonClickedData*>(data);
    clear(bcd->procdata, bcd->model);
    gtk_entry_set_text(bcd->entry, "");
  }
}




void procman_lsof(ProcData *data)
{
  GtkListStore *model = \
    gtk_list_store_new(PROCMAN_LSOF_NCOLS,
		       GDK_TYPE_PIXBUF,	// PROCMAN_LSOF_COL_PIXBUF
		       G_TYPE_STRING,	// PROCMAN_LSOF_COL_PROCESS
		       G_TYPE_UINT,	// PROCMAN_LSOF_COL_PID
		       G_TYPE_STRING	// PROCMAN_LSOF_COL_FILENAME
		       );

  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
  g_object_unref(model);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);


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
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
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
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
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


  GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Lsof"), NULL,
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						  NULL);
  g_signal_connect(G_OBJECT(dialog), "response",
		   G_CALLBACK(close_dialog), NULL);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 575, 400);
  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 2);
  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 5);

  GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
  gtk_box_pack_start(GTK_BOX(vbox), vbox, TRUE, TRUE, 0);

  // Label, entry and search button

  GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);
  GtkWidget *label = gtk_label_new(_("Filename pattern to search:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  GtkWidget *entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  GtkWidget *search_button = gtk_button_new_from_stock(GTK_STOCK_FIND);
  gtk_box_pack_start(GTK_BOX(hbox), search_button, FALSE, FALSE, 0);
  GtkWidget *clear_button = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
  gtk_box_pack_start(GTK_BOX(hbox), clear_button, FALSE, FALSE, 0);

  g_object_set(G_OBJECT(entry), "activates_default", TRUE, NULL);
  gtk_window_set_default(GTK_WINDOW(dialog), GTK_WIDGET(search_button));

  bcd.procdata = data;
  bcd.model = model;
  bcd.entry = GTK_ENTRY(entry);

  g_signal_connect(G_OBJECT(clear_button), "clicked",
		   G_CALLBACK(clear_button_clicked), &bcd);
  g_signal_connect(G_OBJECT(search_button), "clicked",
		   G_CALLBACK(search_button_clicked), &bcd);


  // Scrolled TreeView
  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
				      GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(scrolled), tree);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolled, TRUE, TRUE, 0);


  gtk_widget_show_all(dialog);
}

