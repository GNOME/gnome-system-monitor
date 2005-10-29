#include <config.h>

#include <glib/gi18n.h>
#include <glibtop/procopenfiles.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "procman.h"
#include "openfiles.h"
#include "proctable.h"
#include "util.h"

enum
{
	COL_FD,
	COL_TYPE,
	COL_OBJECT,
	COL_OPENFILE_STRUCT,
	NUM_OPENFILES_COL
};


static const char*
get_type_name(enum glibtop_file_type t)
{
	switch(t)
	{
	case GLIBTOP_FILE_TYPE_FILE:
		return _("file");
	case GLIBTOP_FILE_TYPE_PIPE:
		return _("pipe");
	case GLIBTOP_FILE_TYPE_INETSOCKET:
		return _("network connection");
	case GLIBTOP_FILE_TYPE_LOCALSOCKET:
		return _("local socket");
	default:
		return _("unknown type");
	}
}



static char *
friendlier_hostname(const char *dotted_quad, int port)
{
	struct in_addr addr4;
	struct hostent *host;

	if(inet_pton(AF_INET, dotted_quad, &addr4) <= 0)
		goto failsafe;


	host = gethostbyaddr(&addr4, sizeof addr4, AF_INET);

	if(!host)
		goto failsafe;

	return g_strdup_printf("%s:%d", host->h_name, port);

 failsafe:
	if(!dotted_quad[0]) return g_strdup("");
	return g_strdup_printf("%s:%d", dotted_quad, port);
}



static void
add_new_files (gpointer key, gpointer value, gpointer data)
{
	glibtop_open_files_entry *openfiles = value;

	GtkTreeModel *model = data;
	GtkTreeIter row;

	char *object;

	switch(openfiles->type)
	{
	case GLIBTOP_FILE_TYPE_FILE:
		object = g_strdup(openfiles->info.file.name);
		break;

	case GLIBTOP_FILE_TYPE_INETSOCKET:
		object = friendlier_hostname(openfiles->info.sock.dest_host,
					     openfiles->info.sock.dest_port);
		break;

	case GLIBTOP_FILE_TYPE_LOCALSOCKET:
		object = g_strdup(openfiles->info.localsock.name);
		break;

	default:
		object = g_strdup("");
	}

	gtk_list_store_insert (GTK_LIST_STORE (model), &row, 0);
	gtk_list_store_set (GTK_LIST_STORE (model), &row,
			    COL_FD, openfiles->fd,
			    COL_TYPE, get_type_name(openfiles->type),
			    COL_OBJECT, object,
			    COL_OPENFILE_STRUCT, g_memdup(openfiles, sizeof(*openfiles)),
			    -1);

	g_free(object);
}

static GList *old_maps = NULL;

static gboolean
classify_openfiles (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	GHashTable *new_maps = data;
	GtkTreeIter *old_iter;
	glibtop_open_files_entry *openfiles;
	gchar *old_name;

	gtk_tree_model_get (model, iter, 1, &old_name, -1);

	openfiles = g_hash_table_lookup (new_maps, old_name);
	if (openfiles) {
		g_hash_table_remove (new_maps, old_name);
		g_free (old_name);
		return FALSE;

	}

	old_iter = gtk_tree_iter_copy (iter);
	old_maps = g_list_append (old_maps, old_iter);
	g_free (old_name);
	return FALSE;

}


static gboolean
compare_open_files(gconstpointer a, gconstpointer b)
{
	const glibtop_open_files_entry *o1 = a;
	const glibtop_open_files_entry *o2 = b;

	/* Falta manejar los diferentes tipos! */
	return (o1->fd == o2->fd) && (o1->type == o1->type); /* XXX! */
}


static void
update_openfiles_dialog (GtkWidget *tree)
{
	ProcInfo *info;
	GtkTreeModel *model;
	glibtop_open_files_entry *openfiles;
	glibtop_proc_open_files procmap;
	GHashTable *new_maps;
	gint i;

	info = g_object_get_data (G_OBJECT (tree), "selected_info");

	if (!info)
		return;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

	openfiles = glibtop_get_proc_open_files (&procmap, info->pid);

	if (!openfiles)
		return;

	new_maps = g_hash_table_new_full (g_str_hash, compare_open_files,
					  NULL, NULL);
	for (i=0; i < procmap.number; i++)
		g_hash_table_insert (new_maps, openfiles + i, openfiles + i);

	gtk_tree_model_foreach (model, classify_openfiles, new_maps);

	g_hash_table_foreach (new_maps, add_new_files, model);

	while (old_maps) {
		GtkTreeIter *iter = old_maps->data;
		glibtop_open_files_entry *openfiles = NULL;

		gtk_tree_model_get (model, iter,
				    COL_OPENFILE_STRUCT, &openfiles,
				    -1);

		gtk_list_store_remove (GTK_LIST_STORE (model), iter);
		gtk_tree_iter_free (iter);
		g_free (openfiles);

		old_maps = g_list_next (old_maps);

	}

	g_hash_table_destroy (new_maps);
	g_free (openfiles);
}

static void
close_openfiles_dialog (GtkDialog *dialog, gint id, gpointer data)
{
	GtkWidget *tree = data;
	GConfClient *client;
	guint timer;

	client = g_object_get_data (G_OBJECT (tree), "client");
	procman_save_tree_state (client, tree, "/apps/procman/openfilestree2");

	timer = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (tree), "timer"));
	g_source_remove (timer);

	gtk_widget_destroy (GTK_WIDGET (dialog));

	return ;
}


static GtkWidget *
create_openfiles_tree (ProcData *procdata)
{
	GtkWidget *tree;
	GtkListStore *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	gint i;

	static const gchar *title[] = {
		/* Translators: "FD" here means "File Descriptor". Please use
                   a very short translation if possible, and at most
                   2-3 characters for it to be able to fit in the UI. */
		N_("FD"),
		N_("Type"),
		N_("Object")
	};

	PROCMAN_GETTEXT_ARRAY_INIT(title);

	model = gtk_list_store_new (NUM_OPENFILES_COL,
				    G_TYPE_INT,		/* FD */
				    G_TYPE_STRING,	/* Type */
				    G_TYPE_STRING,	/* Object */
				    G_TYPE_POINTER	/* open_files_entry */
		);

	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);
	g_object_unref (G_OBJECT (model));

	for (i = 0; i < NUM_OPENFILES_COL-1; i++) {
		cell = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (title[i],
								   cell,
								   "text", i,
								   NULL);
		gtk_tree_view_column_set_sort_column_id (column, i);
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	}

#if 0
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_VMSZ,
					 sort_ints,
					 GINT_TO_POINTER (COL_FD),
					 NULL);
/*gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
  0,
  GTK_SORT_ASCENDING);*/
#endif

	procman_get_tree_state (procdata->client, tree, "/apps/procman/openfilestree");

	return tree;

}


static gboolean
openfiles_timer (gpointer data)
{
	GtkWidget *tree = data;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	g_assert(model);

	update_openfiles_dialog (tree);

	return TRUE;
}


static void
create_single_openfiles_dialog (GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, gpointer data)
{
	ProcData *procdata = data;
	GtkWidget *openfilesdialog;
	GtkWidget *dialog_vbox, *vbox;
	GtkWidget *cmd_hbox;
	GtkWidget *label;
	GtkWidget *scrolled;
	GtkWidget *tree;
	ProcInfo *info;
	guint timer;

	gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

	if (!info)
		return;

	openfilesdialog = gtk_dialog_new_with_buttons (_("Open Files"), NULL,
						       GTK_DIALOG_DESTROY_WITH_PARENT,
						       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						       NULL);
	gtk_window_set_resizable (GTK_WINDOW (openfilesdialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (openfilesdialog), 575, 400);
	gtk_dialog_set_has_separator (GTK_DIALOG (openfilesdialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (openfilesdialog), 5);

	vbox = GTK_DIALOG (openfilesdialog)->vbox;
	gtk_box_set_spacing (GTK_BOX (vbox), 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	dialog_vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 5);
	gtk_box_pack_start (GTK_BOX (vbox), dialog_vbox, TRUE, TRUE, 0);

	cmd_hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), cmd_hbox, FALSE, FALSE, 0);


	label = procman_make_label_for_mmaps_or_ofiles (
		_("_Files opened by process \"%s\" (PID %u):"),
		info->name,
		info->pid);

	gtk_box_pack_start (GTK_BOX (cmd_hbox),label, FALSE, FALSE, 0);


	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                             GTK_SHADOW_IN);

	tree = create_openfiles_tree (procdata);
	gtk_container_add (GTK_CONTAINER (scrolled), tree);
	g_object_set_data (G_OBJECT (tree), "selected_info", info);
	g_object_set_data (G_OBJECT (tree), "client", procdata->client);

	gtk_box_pack_start (GTK_BOX (dialog_vbox), scrolled, TRUE, TRUE, 0);
	gtk_widget_show_all (scrolled);

	g_signal_connect (G_OBJECT (openfilesdialog), "response",
			  G_CALLBACK (close_openfiles_dialog), tree);

	gtk_widget_show_all (openfilesdialog);

	timer = g_timeout_add (5000, openfiles_timer, tree);
	g_object_set_data (G_OBJECT (tree), "timer", GUINT_TO_POINTER (timer));

	update_openfiles_dialog (tree);

}


void
create_openfiles_dialog (ProcData *procdata)
{
	gtk_tree_selection_selected_foreach (procdata->selection, create_single_openfiles_dialog,
					     procdata);
}
