#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <glibtop/procmap.h>
#include <sys/stat.h>

#include "procman.h"
#include "memmaps.h"
#include "proctable.h"
#include "util.h"


static void
add_new_maps (gpointer key, gpointer value, gpointer data)
{
	glibtop_map_entry *memmaps = value;
	GtkTreeModel *model = data;
	GtkTreeIter row;
	MemmapsInfo *info = g_new0 (MemmapsInfo, 1);
	gchar *format = (sizeof (void*) == 8) ? "%016lx" : "%08lx";
	gchar *vmsize_fmt = "%luK";
	unsigned long vmstart;
	unsigned long vmend;
	unsigned long vmsize;
	char flags[5];
	unsigned long vmoffset;
	short dev_major;
       	short dev_minor;
       	unsigned long inode;
		
		
	vmstart = memmaps->start;
	vmend = memmaps->end;
	vmsize = (memmaps->end - memmaps->start)/1024;
	vmoffset = memmaps->offset;
	dev_minor = memmaps->device & 255;
	dev_major = (memmaps->device >> 8) & 255;
	inode = memmaps->inode;
	flags [0] =
               	(memmaps->perm & GLIBTOP_MAP_PERM_READ) ? 'r' : '-';
       	flags [1] =
               	(memmaps->perm & GLIBTOP_MAP_PERM_WRITE) ? 'w' : '-';
       	flags [2] =
               	(memmaps->perm & GLIBTOP_MAP_PERM_EXECUTE) ? 'x' : '-';
        flags [3] =
               	(memmaps->perm & GLIBTOP_MAP_PERM_SHARED) ? 's' : '-';
        if (memmaps->perm & GLIBTOP_MAP_PERM_PRIVATE)
               	flags [3] = 'p';

        flags [4] = 0;
            	
        if (memmaps->flags & (1 << GLIBTOP_MAP_ENTRY_FILENAME))
               	info->filename = g_strdup (memmaps->filename);
        else
               	info->filename = g_strdup ("");
                	
        info->vmstart = g_strdup_printf (format, vmstart);
        info->vmend = g_strdup_printf (format, vmend);
        info->vmsize = g_strdup_printf (vmsize_fmt, vmsize);
	info->flags = g_strdup (flags);
        info->vmoffset = g_strdup_printf (format, vmoffset);
        info->device = g_strdup_printf ("%02hx:%02hx", dev_major, dev_minor);
        info->inode = g_strdup_printf ("%ld", inode);
                
        gtk_list_store_insert (GTK_LIST_STORE (model), &row, 0);	
        gtk_list_store_set (GTK_LIST_STORE (model), &row, 
                		    0, info->filename, 
				    1, info->vmstart,
				    2, info->vmend,
				    3, info->vmsize,
				    4, info->flags,
				    5, info->vmoffset,
				    6, info->device,
				    7, info->inode,
				    8, vmsize,
			            9, inode,
				    -1);
        g_free (info->filename);
	g_free (info->vmstart);
	g_free (info->vmend);
	g_free (info->vmsize);
	g_free (info->flags);
	g_free (info->device);
	g_free (info->vmoffset);
	g_free (info->inode);
	g_free (info);
}

static GList *old_maps = NULL;

static gboolean
compare_memmaps (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	GHashTable *new_maps = data;
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
	old_maps = g_list_append (old_maps, old_iter);
	g_free (old_name);
	return FALSE;
	
}

static void
key_destroy (gpointer key)
{
	if (key)
		g_free (key);
		
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
					  key_destroy, NULL);
	for (i=0; i < procmap.number; i++) {
		gchar *format = (sizeof (void*) == 8) ? "%016lx" : "%08lx";
		gchar *vmstart;
		
		vmstart = g_strdup_printf (format,  memmaps[i].start);
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
	GtkWidget *tree = data;	
	GConfClient *client;
	gint timer;
	
	client = g_object_get_data (G_OBJECT (tree), "client");
	procman_save_tree_state (client, tree, "/apps/procman/memmapstree2");
	
	timer = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tree), "timer"));
	gtk_timeout_remove (timer);
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
		
	return ;
}

static gint
sort_ints (GtkTreeModel *model, GtkTreeIter *itera, GtkTreeIter *iterb, gpointer data)
{
	gint col = GPOINTER_TO_INT (data);
	gint a=0, b=0;
	
	switch (col) {
	case COL_VMSZ:
		gtk_tree_model_get (model, itera, 8, &a, -1);
		gtk_tree_model_get (model, iterb, 8, &b, -1);
		break;
	case COL_INODE:
		gtk_tree_model_get (model, itera, 9, &a, -1);
		gtk_tree_model_get (model, iterb, 9, &b, -1);
		break;
	default:
		a = 0;
		b = 0;
		break;
	}

	if (a > b)
		return -1;
	else if (a < b)
		return 1;
        else
                return 0;	
}

static GtkWidget *
create_memmaps_tree (ProcData *procdata)
{
	GtkWidget *tree;
	GtkListStore *model;
	GtkTreeViewColumn *column;
  	GtkCellRenderer *cell;
  	gint i;

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

	                             	
        model = gtk_list_store_new (NUM_MMAP_COL+2, 
				    G_TYPE_STRING, G_TYPE_STRING,
                                    G_TYPE_STRING, G_TYPE_STRING,
                                    G_TYPE_STRING, G_TYPE_STRING,
                                    G_TYPE_STRING, G_TYPE_STRING,
				     G_TYPE_INT, G_TYPE_INT);
				    
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);
  	g_object_unref (G_OBJECT (model));
  	
  	for (i = 0; i < NUM_MMAP_COL; i++) {
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
					 COL_VMSZ,
					 sort_ints,
					 GINT_TO_POINTER (COL_VMSZ),
					 NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
					 COL_INODE,
					 sort_ints,
					 GINT_TO_POINTER (COL_INODE),
					 NULL);
	/*gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					        0,
					        GTK_SORT_ASCENDING);*/
					      
	procman_get_tree_state (procdata->client, tree, "/apps/procman/memmapstree2");
	
	return tree;
		
}


static gint
memmaps_timer (gpointer data)
{
	GtkWidget *tree = data;
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
	ProcData *procdata = data;
	GtkWidget *memmapsdialog;
	GtkWidget *dialog_vbox, *vbox;
	GtkWidget *cmd_hbox;
	GtkWidget *label;
	GtkWidget *scrolled;
	GtkWidget *tree;
	ProcInfo *info;
	gint timer;

	gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);
	g_return_if_fail (info);

	memmapsdialog = gtk_dialog_new_with_buttons (_("Memory Maps"), NULL,
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						     NULL);
	gtk_window_set_resizable (GTK_WINDOW (memmapsdialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (memmapsdialog), 575, 400);
	gtk_dialog_set_has_separator (GTK_DIALOG (memmapsdialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (memmapsdialog), 5);
	
	vbox = GTK_DIALOG (memmapsdialog)->vbox;
	gtk_box_set_spacing (GTK_BOX (vbox), 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	
	dialog_vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 12);
	gtk_box_pack_start (GTK_BOX (vbox), dialog_vbox, TRUE, TRUE, 0);
	
	cmd_hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), cmd_hbox, FALSE, FALSE, 0);
		
	label = gtk_label_new (_("Process name:"));
	gtk_box_pack_start (GTK_BOX (cmd_hbox),label, FALSE, FALSE, 0);
	
	label = gtk_label_new ("");
	gtk_label_set_text (GTK_LABEL (label), info->name);
	gtk_box_pack_start (GTK_BOX (cmd_hbox),label, FALSE, FALSE, 0);
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  	GTK_POLICY_AUTOMATIC,
                                  	GTK_POLICY_AUTOMATIC);
	
	tree = create_memmaps_tree (procdata);
	gtk_container_add (GTK_CONTAINER (scrolled), tree);
	g_object_set_data (G_OBJECT (tree), "selected_info", info);
	g_object_set_data (G_OBJECT (tree), "client", procdata->client);
	
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
