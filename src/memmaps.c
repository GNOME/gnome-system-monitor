#include <config.h>

#include <glibtop/procmap.h>
#include <sys/stat.h>
#include <glib/gi18n.h>

#include <libgnomevfs/gnome-vfs-utils.h>

#include "procman.h"
#include "memmaps.h"
#include "proctable.h"
#include "util.h"



/* be careful with this enum, you could break the column names */
enum
{
	MMAP_COL_FILENAME = 0,
	MMAP_COL_VMSTART,
	MMAP_COL_VMEND,
	MMAP_COL_VMSZ,
	MMAP_COL_FLAGS,
	MMAP_COL_VMOFFSET,
	MMAP_COL_DEVICE,
	MMAP_COL_INODE = 7,

	MMAP_COL_VMSZ_INT,
	MMAP_COL_INODE_INT,
	MMAP_COL_MAX
};

#define MMAP_COL_MAX_VISIBLE (MMAP_COL_INODE + 1)


typedef struct _MemmapsInfo MemmapsInfo;

struct _MemmapsInfo
{
	gchar *filename;
	gchar *vmstart;
	gchar *vmend;
	gchar *vmsize;
	gchar *flags;
	gchar *vmoffset;
	gchar *device;
	gchar *inode;
};



/*
 * Returns a new allocated string representing @v
 */
static gchar* vmoff_tostring(guint64 v)
{
	return g_strdup_printf((sizeof (void*) == 8) ? "%016llx" : "%08llx",
			       (sizeof (void*) == 8) ?  v : v & 0xffffffff);
}



static void
add_new_maps (gpointer key, gpointer value, gpointer data)
{
	glibtop_map_entry * const memmaps = value;
	GtkTreeModel * const model = data;
	GtkTreeIter row;
	MemmapsInfo info;
	guint64 vmsize;
	char flags[5] = "----";
	unsigned short dev_major, dev_minor;

	vmsize = memmaps->end - memmaps->start;
	dev_minor = memmaps->device & 255;
	dev_major = (memmaps->device >> 8) & 255;

	if(memmaps->perm & GLIBTOP_MAP_PERM_READ)    flags [0] = 'r';
	if(memmaps->perm & GLIBTOP_MAP_PERM_WRITE)   flags [1] = 'w';
	if(memmaps->perm & GLIBTOP_MAP_PERM_EXECUTE) flags [2] = 'x';
	if(memmaps->perm & GLIBTOP_MAP_PERM_SHARED)  flags [3] = 's';
	if(memmaps->perm & GLIBTOP_MAP_PERM_PRIVATE) flags [3] = 'p';

	if (memmaps->flags & (1 << GLIBTOP_MAP_ENTRY_FILENAME))
		info.filename = g_strdup (memmaps->filename);
	else
		info.filename = g_strdup ("");

	info.vmstart  = vmoff_tostring (memmaps->start);
	info.vmend    = vmoff_tostring (memmaps->end);
	info.vmsize   = gnome_vfs_format_file_size_for_display (vmsize);
	info.flags    = g_strdup (flags);
	info.vmoffset = vmoff_tostring (memmaps->offset);
	info.device   = g_strdup_printf ("%02hx:%02hx", dev_major, dev_minor);
	info.inode    = g_strdup_printf ("%llu", memmaps->inode);

	gtk_list_store_insert (GTK_LIST_STORE (model), &row, 0);
	gtk_list_store_set (GTK_LIST_STORE (model), &row,
			    MMAP_COL_FILENAME, info.filename,
			    MMAP_COL_VMSTART, info.vmstart,
			    MMAP_COL_VMEND, info.vmend,
			    MMAP_COL_VMSZ, info.vmsize,
			    MMAP_COL_FLAGS, info.flags,
			    MMAP_COL_VMOFFSET, info.vmoffset,
			    MMAP_COL_DEVICE, info.device,
			    MMAP_COL_INODE, info.inode,
			    MMAP_COL_VMSZ_INT, vmsize,
			    MMAP_COL_INODE_INT, memmaps->inode,
			    -1);

	g_free (info.filename);
	g_free (info.vmstart);
	g_free (info.vmend);
	g_free (info.vmsize);
	g_free (info.flags);
	g_free (info.device);
	g_free (info.vmoffset);
	g_free (info.inode);
}


static GList *old_maps = NULL;


static gboolean
compare_memmaps (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	GHashTable * const new_maps = data;
	GtkTreeIter *old_iter;
	glibtop_map_entry *memmaps;
	gchar *old_name;

	gtk_tree_model_get (model, iter, 1, &old_name, -1);

	memmaps = g_hash_table_lookup (new_maps, old_name);
	if (memmaps) {
		g_hash_table_remove (new_maps, old_name);
		g_free (old_name);
		return FALSE;
	}

	old_iter = gtk_tree_iter_copy (iter);
	old_maps = g_list_prepend (old_maps, old_iter);
	g_free (old_name);
	return FALSE;
}


static void
update_memmaps_dialog (GtkWidget *tree)
{
	ProcInfo *info;
	GtkTreeModel *model;
	glibtop_map_entry *memmaps;
	glibtop_proc_map procmap;
	GHashTable *new_maps;
	gint i;

	info = g_object_get_data (G_OBJECT (tree), "selected_info");
	g_return_if_fail (info);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

	memmaps = glibtop_get_proc_map (&procmap, info->pid);

	if (!memmaps)
		return;

	new_maps = g_hash_table_new_full (g_str_hash, g_str_equal,
					  g_free, NULL);
	for (i=0; i < procmap.number; i++) {
		gchar *vmstart;
		vmstart = vmoff_tostring (memmaps[i].start);
		g_hash_table_insert (new_maps, vmstart, &memmaps[i]);
	}

	gtk_tree_model_foreach (model, compare_memmaps, new_maps);

	g_hash_table_foreach (new_maps, add_new_maps, model);

	while (old_maps) {
		GtkTreeIter *iter = old_maps->data;

		gtk_list_store_remove (GTK_LIST_STORE (model), iter);
		gtk_tree_iter_free (iter);

		old_maps = g_list_next (old_maps);

	}

	g_hash_table_destroy (new_maps);
	g_free (memmaps);
}


static void
close_memmaps_dialog (GtkDialog *dialog, gint id, gpointer data)
{
	GtkWidget * const tree = data;
	GConfClient *client;
	gint timer;

	client = g_object_get_data (G_OBJECT (tree), "client");
	procman_save_tree_state (client, tree, "/apps/procman/memmapstree2");

	timer = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tree), "timer"));
	gtk_timeout_remove (timer);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}


static gint
sort_guint64 (GtkTreeModel *model, GtkTreeIter *itera, GtkTreeIter *iterb, gpointer data)
{
	const gint selected_col = GPOINTER_TO_INT (data);
	gint data_col;
	guint64 a, b;

	switch(selected_col)
	{
	case MMAP_COL_VMSZ:
		data_col = MMAP_COL_VMSZ_INT;
		break;

	case MMAP_COL_INODE:
		data_col = MMAP_COL_INODE_INT;
		break;

	default:
		g_assert_not_reached();
		return 0;
	}

	gtk_tree_model_get (model, itera, data_col, &a, -1);
	gtk_tree_model_get (model, iterb, data_col, &b, -1);

	return PROCMAN_CMP(a, b);
}



static GtkWidget *
create_memmaps_tree (ProcData *procdata)
{
	GtkWidget *tree;
	GtkListStore *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	guint i;

	static const gchar *titles[] = {
		N_("Filename"),
		N_("VM Start"),
		N_("VM End"),
		N_("VM Size"),
		N_("Flags"),
		N_("VM Offset"),
		N_("Device"),
		N_("Inode")
	};

	PROCMAN_GETTEXT_ARRAY_INIT(titles);


	model = gtk_list_store_new (MMAP_COL_MAX,
				    G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_UINT64, G_TYPE_UINT64 /* these are invisible */
				    );

	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);
	g_object_unref (G_OBJECT (model));

	for (i = 0; i < MMAP_COL_MAX_VISIBLE; i++) {
		cell = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes (titles[i],
								   cell,
								   "text", i,
								   NULL);
		gtk_tree_view_column_set_sort_column_id (column, i);
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	}


	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 MMAP_COL_VMSZ,
					 sort_guint64,
					 GINT_TO_POINTER (MMAP_COL_VMSZ),
					 NULL);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 MMAP_COL_INODE,
					 sort_guint64,
					 GINT_TO_POINTER (MMAP_COL_INODE),
					 NULL);

	procman_get_tree_state (procdata->client, tree, "/apps/procman/memmapstree2");

	return tree;
}


static gint
memmaps_timer (gpointer data)
{
	GtkWidget * const tree = data;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	g_return_val_if_fail (model, FALSE);

	update_memmaps_dialog (tree);

	return TRUE;
}


static void
create_single_memmaps_dialog (GtkTreeModel *model, GtkTreePath *path,
			      GtkTreeIter *iter, gpointer data)
{
	ProcData * const procdata = data;
	GtkWidget *memmapsdialog;
	GtkWidget *dialog_vbox, *vbox;
	GtkWidget *label;
	GtkWidget *scrolled;
	GtkWidget *tree;
	ProcInfo *info;
	gint timer;

	gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);
	g_return_if_fail (info);

	memmapsdialog = gtk_dialog_new_with_buttons (_("Memory Maps"), GTK_WINDOW (procdata->app),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						     NULL);
	/*FIXME: maybe the window title could be "Memory Maps - PROCESS_NAME" ?*/
	gtk_window_set_resizable (GTK_WINDOW (memmapsdialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (memmapsdialog), 575, 400);
	gtk_dialog_set_has_separator (GTK_DIALOG (memmapsdialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (memmapsdialog), 6);

	vbox = GTK_DIALOG (memmapsdialog)->vbox;
	gtk_box_set_spacing (GTK_BOX (vbox), 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	dialog_vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 5);
	gtk_box_pack_start (GTK_BOX (vbox), dialog_vbox, TRUE, TRUE, 0);


	label = procman_make_label_for_mmaps_or_ofiles (
		_("_Memory maps for process \"%s\" (PID %d):"),
		info->name,
		info->pid);

	gtk_box_pack_start (GTK_BOX (dialog_vbox), label, FALSE, TRUE, 0);


	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
					     GTK_SHADOW_IN);

	tree = create_memmaps_tree (procdata);
	gtk_container_add (GTK_CONTAINER (scrolled), tree);
	g_object_set_data (G_OBJECT (tree), "selected_info", info);
	g_object_set_data (G_OBJECT (tree), "client", procdata->client);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), tree);

	gtk_box_pack_start (GTK_BOX (dialog_vbox), scrolled, TRUE, TRUE, 0);
	gtk_widget_show_all (scrolled);

	g_signal_connect (G_OBJECT (memmapsdialog), "response",
			  G_CALLBACK (close_memmaps_dialog), tree);

	gtk_widget_show_all (memmapsdialog);

	timer = gtk_timeout_add (5000, memmaps_timer, tree);
	g_object_set_data (G_OBJECT (tree), "timer", GINT_TO_POINTER (timer));

	update_memmaps_dialog (tree);

}


void
create_memmaps_dialog (ProcData *procdata)
{
	gtk_tree_selection_selected_foreach (procdata->selection, create_single_memmaps_dialog,
					     procdata);
}
