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


typedef struct
{
	int timer;
	GtkWidget *tree;
	GPtrArray *old_maps;

	/*
	  { VMStart -> glibtop_map_entry* }
	*/
	GHashTable *new_maps;

	GConfClient *client;
	ProcInfo *info;

} MemMapsData;



/*
 * Returns a new allocated string representing @v
 */
static gchar* vmoff_tostring(guint64 v)
{
	return g_strdup_printf((sizeof (void*) == 8) ? "%016llx" : "%08llx",
			       (sizeof (void*) == 8) ?  v : v & 0xffffffff);
}



/*
 * Dummy callback used to clear a GHashTable
 */
static gboolean
ghashtable_clear_callback (gpointer key, gpointer value, gpointer user_data)
{
	return TRUE;
}


/*
 * Utility function to clear a GHashTable without destroying it
 * then creating a new one.
 */

static void
ghashtable_clear(GHashTable *hash)
{
	g_hash_table_foreach_remove (hash, ghashtable_clear_callback, NULL);
}





static void
add_new_maps (gpointer key, gpointer value, gpointer data)
{
	glibtop_map_entry * const memmaps = value;
	GtkTreeModel * const model = data;
	GtkTreeIter row;
	guint64 size;
	char *filename;
	char *vmstart;
	char *vmend;
	char *vmsize;
	char *vmoffset;
	char *device;
	char *inode;
	char flags[5] = "----";
	unsigned short dev_major, dev_minor;

	size = memmaps->end - memmaps->start;
	dev_minor = memmaps->device & 255;
	dev_major = (memmaps->device >> 8) & 255;

	if(memmaps->perm & GLIBTOP_MAP_PERM_READ)    flags [0] = 'r';
	if(memmaps->perm & GLIBTOP_MAP_PERM_WRITE)   flags [1] = 'w';
	if(memmaps->perm & GLIBTOP_MAP_PERM_EXECUTE) flags [2] = 'x';
	if(memmaps->perm & GLIBTOP_MAP_PERM_SHARED)  flags [3] = 's';
	if(memmaps->perm & GLIBTOP_MAP_PERM_PRIVATE) flags [3] = 'p';

	if (memmaps->flags & (1 << GLIBTOP_MAP_ENTRY_FILENAME))
		filename = g_strdup (memmaps->filename);
	else
		filename = g_strdup ("");

	vmstart  = vmoff_tostring (memmaps->start);
	vmend    = vmoff_tostring (memmaps->end);
	vmsize   = SI_gnome_vfs_format_file_size_for_display (size);
	vmoffset = vmoff_tostring (memmaps->offset);
	device   = g_strdup_printf ("%02hx:%02hx", dev_major, dev_minor);
	inode    = g_strdup_printf ("%llu", memmaps->inode);

	gtk_list_store_insert (GTK_LIST_STORE (model), &row, 0);
	gtk_list_store_set (GTK_LIST_STORE (model), &row,
			    MMAP_COL_FILENAME, filename,
			    MMAP_COL_VMSTART, vmstart,
			    MMAP_COL_VMEND, vmend,
			    MMAP_COL_VMSZ, vmsize,
			    MMAP_COL_FLAGS, flags,
			    MMAP_COL_VMOFFSET, vmoffset,
			    MMAP_COL_DEVICE, device,
			    MMAP_COL_INODE, inode,
			    MMAP_COL_VMSZ_INT, size,
			    MMAP_COL_INODE_INT, memmaps->inode,
			    -1);

	g_free (filename);
	g_free (vmstart);
	g_free (vmend);
	g_free (vmsize);
	g_free (device);
	g_free (vmoffset);
	g_free (inode);
}




static gboolean
compare_memmaps (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	MemMapsData * const mmdata = data;

	glibtop_map_entry *memmaps;
	gchar *old_name;

	gtk_tree_model_get (model, iter, MMAP_COL_VMSTART, &old_name, -1);

	g_return_val_if_fail(old_name != NULL, FALSE);

	/* If the glibtop_map_entry is still there, we remove it
	   from the new_maps so it's going to be updated only.
	   Else we add it to the old_maps to be removed.
	*/
	/* TODO: lookup is based on vmstart which seems totally broken */
	memmaps = g_hash_table_lookup (mmdata->new_maps, old_name);
	if (memmaps) {
		g_hash_table_remove (mmdata->new_maps, old_name);
	}
	else
	{
		GtkTreeIter *old_iter;
		old_iter = gtk_tree_iter_copy (iter);
		g_ptr_array_add (mmdata->old_maps, old_iter);
	}

	g_free (old_name);
	return FALSE;
}


static void
update_memmaps_dialog (MemMapsData *mmdata)
{
	GtkTreeModel *model;
	glibtop_map_entry *memmaps;
	glibtop_proc_map procmap;
	gint i;

	memmaps = glibtop_get_proc_map (&procmap, mmdata->info->pid);
	/* process has disappeared */
	if(!memmaps) return;

	/* fill mmdata->new_maps with { VMstart -> memmaps[i] */

	g_assert(g_hash_table_size(mmdata->new_maps) == 0);

	for (i=0; i < procmap.number; i++) {
		gchar *vmstart;
		vmstart = vmoff_tostring (memmaps[i].start);
		g_hash_table_insert (mmdata->new_maps, vmstart, &memmaps[i]);
	}


	/* update the mmaps */

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (mmdata->tree));
	gtk_tree_model_foreach (model, compare_memmaps, mmdata);

	g_hash_table_foreach (mmdata->new_maps, add_new_maps, model);

	ghashtable_clear (mmdata->new_maps);


	/* remove and clear the old mmaps*/

	for(i = 0; i < mmdata->old_maps->len; ++i)
	{
		GtkTreeIter *iter = g_ptr_array_index (mmdata->old_maps, i);

		gtk_list_store_remove (GTK_LIST_STORE (model), iter);
		gtk_tree_iter_free (iter);
	}

	g_ptr_array_set_size (mmdata->old_maps, 0);


	g_free (memmaps);
}


static void
close_memmaps_dialog (GtkDialog *dialog, gint id, gpointer data)
{
	MemMapsData * const mmdata = data;

	procman_save_tree_state (mmdata->client, mmdata->tree, "/apps/procman/memmapstree2");

	gtk_timeout_remove (mmdata->timer);

	gtk_widget_destroy (GTK_WIDGET (dialog));

	g_ptr_array_free (mmdata->old_maps, TRUE);
	g_hash_table_destroy (mmdata->new_maps);
	g_free (mmdata);
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



static MemMapsData*
create_memmapsdata (ProcData *procdata)
{
	MemMapsData *mmdata;
	GtkWidget *tree;
	GtkListStore *model;
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
				    G_TYPE_STRING, /* MMAP_COL_FILENAME  */
				    G_TYPE_STRING, /* MMAP_COL_VMSTART	 */
				    G_TYPE_STRING, /* MMAP_COL_VMEND	 */
				    G_TYPE_STRING, /* MMAP_COL_VMSZ	 */
				    G_TYPE_STRING, /* MMAP_COL_FLAGS	 */
				    G_TYPE_STRING, /* MMAP_COL_VMOFFSET  */
				    G_TYPE_STRING, /* MMAP_COL_DEVICE	 */
				    G_TYPE_STRING, /* MMAP_COL_INODE	 */
				    /* hidden */
				    G_TYPE_UINT64, /* MMAP_COL_VMSZ_INT  */
				    G_TYPE_UINT64  /* MMAP_COL_INODE_INT */
				    );

	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);
	g_object_unref (G_OBJECT (model));

	for (i = 0; i < MMAP_COL_MAX_VISIBLE; i++) {
		GtkCellRenderer *cell;
		GtkTreeViewColumn *column;

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


	mmdata = g_new(MemMapsData, 1);

	mmdata->timer = -1;
	mmdata->client = procdata->client;
	mmdata->tree = tree;
	mmdata->old_maps = g_ptr_array_new ();
	mmdata->new_maps = g_hash_table_new_full (g_str_hash, g_str_equal,
						  g_free, NULL);

	return mmdata;
}


static gint
memmaps_timer (gpointer data)
{
	MemMapsData * const mmdata = data;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (mmdata->tree));
	g_return_val_if_fail (model, FALSE);

	update_memmaps_dialog (mmdata);

	return TRUE;
}


static void
create_single_memmaps_dialog (GtkTreeModel *model, GtkTreePath *path,
			      GtkTreeIter *iter, gpointer data)
{
	ProcData * const procdata = data;
	MemMapsData *mmdata;
	GtkWidget *memmapsdialog;
	GtkWidget *dialog_vbox, *vbox;
	GtkWidget *label;
	GtkWidget *scrolled;
	ProcInfo *info;

	gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);
	g_return_if_fail (info);

	mmdata = create_memmapsdata (procdata);
	mmdata->info = info;

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
		_("_Memory maps for process \"%s\" (PID %u):"),
		info->name,
		info->pid);

	gtk_box_pack_start (GTK_BOX (dialog_vbox), label, FALSE, TRUE, 0);


	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
					     GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (scrolled), mmdata->tree);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), mmdata->tree);

	gtk_box_pack_start (GTK_BOX (dialog_vbox), scrolled, TRUE, TRUE, 0);
	gtk_widget_show_all (scrolled);

	g_signal_connect (G_OBJECT (memmapsdialog), "response",
			  G_CALLBACK (close_memmaps_dialog), mmdata);

	gtk_widget_show_all (memmapsdialog);

	mmdata->timer = gtk_timeout_add (5000, memmaps_timer, mmdata);

	update_memmaps_dialog (mmdata);
}


void
create_memmaps_dialog (ProcData *procdata)
{
	/* TODO: do we really want to open multiple dialogs ? */
	gtk_tree_selection_selected_foreach (procdata->selection, create_single_memmaps_dialog,
					     procdata);
}
