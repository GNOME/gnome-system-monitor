
#include <gnome.h>
#include <gal/e-table/e-tree-memory.h>
#include <gal/e-table/e-tree-memory-callbacks.h>
#include <gal/e-table/e-tree-scrolled.h>
#include <glibtop/procmap.h>
#include <glibtop/xmalloc.h>

#include "procman.h"
#include "memmaps.h"

GtkWidget *memmapsdialog = NULL;
GtkWidget *cmd_label;
GtkWidget *tree = NULL;
ETreeModel *model = NULL;
ETreeMemory *memory = NULL;
ETreePath *root_node = NULL;
gint timer = 0;
GList *memmaps_list = NULL;

static GdkPixbuf *
memmaps_get_icon (ETreeModel *etm, ETreePath path, void *data)
{
	/* No icon, since the cell tree renderer takes care of the +/- icons itself. */
	return NULL;
}

static int
memmaps_get_columns (ETreeModel *table, void *data)
{
	return 7;
}


static void *
memmaps_get_value (ETreeModel *model, ETreePath path, int column, void *data)
{
	MemmapsInfo *info;
	
	info = e_tree_memory_node_get_data (memory, path);
	if (!info) g_print ("Null info \n");
	
	switch (column) {
	case COL_FILENAME:
		return info->filename;
	case COL_VMSTART:
		return info->vmstart;
	case COL_VMEND:
		return info->vmend;
	case COL_FLAGS:
		return info->flags;
	case COL_VMOFFSET:
		return info->vmoffset;
	case COL_DEVICE:
		return info->device;
	case COL_INODE:
		return info->inode;
	
	}
	g_assert_not_reached ();
	return NULL;
	
}

static void
memmaps_set_value (ETreeModel *model, ETreePath path, int col, const void *value, void *data)
{

}	

static gboolean
memmaps_get_editable (ETreeModel *model, ETreePath path, int column, void *data)
{
	return FALSE;
}

static void *
memmaps_duplicate_value (ETreeModel *model, int column, const void *value, void *data)
{

	return g_strdup (value);

}

static void
memmaps_free_value (ETreeModel *model, int column, void *value, void *data)
{


	g_free (value);

		
}

static void *
memmaps_initialize_value (ETreeModel *model, int column, void *data)
{
	return g_strdup ("");

}

static gboolean
memmaps_value_is_empty (ETreeModel *model, int column, const void *value, void *data)
{

	return !(value && *(char *)value);

}

static char *
memmaps_value_to_string (ETreeModel *model, int column, const void *value, void *data)
{
	return g_strdup (value);

}


static void
get_memmaps_list (ProcData *procdata, ProcInfo *info)
{
	glibtop_map_entry *memmaps;
	glibtop_proc_map procmap;
	gint i;
	
	memmaps = glibtop_get_proc_map (&procmap, info->pid);
	
	if (!memmaps)
		return;
		
	for (i = 0; i < procmap.number; i++)
	{
		MemmapsInfo *info = g_new0 (MemmapsInfo, 1);
		gchar *format = (sizeof (void*) == 8) ? "%016lx" : "%08lx";
		unsigned long vmstart;
		unsigned long vmend;
		char flags[5];
		unsigned long vmoffset;
		short dev_major;
        	short dev_minor;
        	unsigned long inode;
		
		
		vmstart = memmaps[i].start;
		vmend = memmaps[i].end;
		vmoffset = memmaps[i].offset;
		dev_minor = memmaps [i].device & 255;
		dev_major = (memmaps [i].device >> 8) & 255;
		inode = memmaps[i].inode;
		flags [0] =
                	(memmaps[i].perm & GLIBTOP_MAP_PERM_READ) ? 'r' : '-';
            	flags [1] =
                	(memmaps[i].perm & GLIBTOP_MAP_PERM_WRITE) ? 'w' : '-';
            	flags [2] =
                	(memmaps[i].perm & GLIBTOP_MAP_PERM_EXECUTE) ? 'x' : '-';
            	flags [3] =
                	(memmaps[i].perm & GLIBTOP_MAP_PERM_SHARED) ? 's' : '-';
           	if (memmaps[i].perm & GLIBTOP_MAP_PERM_PRIVATE)
                	flags [3] = 'p';

            	flags [4] = 0;
            	
            	if (memmaps [i].flags & (1 << GLIBTOP_MAP_ENTRY_FILENAME))
                	info->filename = (gchar *)glibtop_strdup (memmaps [i].filename);
                else
                	info->filename = glibtop_strdup ("");
                	
                info->vmstart = g_strdup_printf (format, vmstart);
                info->vmend = g_strdup_printf (format, vmend);
                info->flags = g_strdup (flags);
                info->vmoffset = g_strdup_printf (format, vmoffset);
                info->device = g_strdup_printf ("%02hx:%02hx", dev_major, dev_minor);
                info->inode = g_strdup_printf ("%ld", inode);
                
                	
                
                e_tree_memory_node_insert (memory, root_node, 0, info);
                memmaps_list = g_list_append (memmaps_list, info);
	
	}
	


}

static void
clear_memmaps (ProcData *procdata)
{
	e_tree_memory_node_remove (memory, root_node);
	root_node = NULL;
	
	while (memmaps_list)
	{
		MemmapsInfo *info = memmaps_list->data;
		g_free (info->filename);
		g_free (info->vmstart);
		g_free (info->vmend);
		g_free (info->device);
		g_free (info->vmoffset);
		g_free (info->inode);
		g_free (info);
		memmaps_list = g_list_next (memmaps_list);
	}
	g_list_free (memmaps_list);
	memmaps_list = NULL;
	

}


void
update_memmaps_dialog (ProcData *procdata)
{

	ProcInfo *info;
	
	if (!memmapsdialog)
		return;
		
	if (!procdata->selected_node)
		return;

	info = e_tree_memory_node_get_data (procdata->memory, procdata->selected_node);
	if (!info)
		return;

	gtk_label_set_text (GTK_LABEL (cmd_label), info->name);
	
	if (memmaps_list)
		clear_memmaps (procdata);
	
	if (!root_node)
	{
		root_node = e_tree_memory_node_insert (memory, NULL, 0, NULL);
		e_tree_root_node_set_visible (E_TREE(tree), FALSE);
	}
		
	get_memmaps_list (procdata, info);
}

static void
save_memmaps_tree_state (ProcData *procdata)
{
	e_tree_save_state (E_TREE (tree), procdata->config.memmaps_state_file);
}


static void
close_memmaps_dialog (gpointer data)
{
	ProcData *procdata = data;
	save_memmaps_tree_state (procdata);
	gnome_dialog_close (GNOME_DIALOG (memmapsdialog));
	memmapsdialog = NULL;
	gtk_timeout_remove (timer);
}

static void
close_button_pressed (GtkButton *button, gpointer data)
{
	ProcData *procdata = data;
	close_memmaps_dialog (procdata);
}


/* Do this to prevent selection of a row */
static gint
tree_clicked (ETree *tree, int row, ETreePath *node, int col, GdkEvent *event)
{
	return TRUE;
}

static GtkWidget *
create_memmaps_tree (ProcData *procdata)
{
	GtkWidget *scrolled;
	
	model = e_tree_memory_callbacks_new (memmaps_get_icon,
					     memmaps_get_columns,
					     NULL,
					     NULL,
					     NULL,
					     NULL,
					     memmaps_get_value,
					     memmaps_set_value,
					     memmaps_get_editable,
				    	     memmaps_duplicate_value,
				    	     memmaps_free_value,
				    	     memmaps_initialize_value,
				    	     memmaps_value_is_empty,
				    	     memmaps_value_to_string,
				    	     procdata);
				    	     
	memory = E_TREE_MEMORY(model);
	
	scrolled = e_tree_scrolled_new (model, NULL, MEMMAPSSPEC, NULL);
	
	tree = GTK_WIDGET (e_tree_scrolled_get_tree (E_TREE_SCROLLED (scrolled)));
	
	e_tree_load_state (E_TREE (tree), procdata->config.memmaps_state_file);
	
	/* Connect here to prevent row selection - a bit of a hack indeed */
	gtk_signal_connect (GTK_OBJECT (tree), "click",
			    GTK_SIGNAL_FUNC (tree_clicked), NULL);
	
	return scrolled;

}


static gint
memmaps_timer (gpointer data)
{
	ProcData *procdata = data;
	
	update_memmaps_dialog (procdata);
	
	return TRUE;
}

void create_memmaps_dialog (ProcData *procdata)
{
	GtkWidget *dialog_vbox;
	GtkWidget *alignment;
	GtkWidget *cmd_hbox;
	GtkWidget *label;
	GtkWidget *dialog_action_area;
	GtkWidget *closebutton;
	GtkWidget *scrolled;

	memmapsdialog = gnome_dialog_new (_("Memory Maps"), NULL);
	gtk_window_set_policy (GTK_WINDOW (memmapsdialog), FALSE, TRUE, FALSE);
	gtk_widget_set_usize (memmapsdialog, 575, 400);
	
	dialog_vbox = GNOME_DIALOG (memmapsdialog)->vbox;
	
	alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), alignment, FALSE, FALSE, 0);
	
	cmd_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment), cmd_hbox);
	
	label = gtk_label_new (_("Process Name : "));
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (cmd_hbox),label, FALSE, FALSE, 0);
	
	cmd_label = gtk_label_new ("");
	gtk_misc_set_padding (GTK_MISC (cmd_label), GNOME_PAD_SMALL, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (cmd_hbox),cmd_label, FALSE, FALSE, 0);
	
	gtk_widget_show_all (alignment);
	
	scrolled = create_memmaps_tree (procdata);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), scrolled, TRUE, TRUE, 0);
	gtk_widget_show_all (scrolled);
		
	dialog_action_area = GNOME_DIALOG (memmapsdialog)->action_area;
	gnome_dialog_append_button (GNOME_DIALOG (memmapsdialog), GNOME_STOCK_BUTTON_CLOSE);
	closebutton = GTK_WIDGET (g_list_last (GNOME_DIALOG (memmapsdialog)->buttons)->data);
	GTK_WIDGET_SET_FLAGS (closebutton, GTK_CAN_DEFAULT);
	
	gtk_signal_connect (GTK_OBJECT (closebutton), "clicked",
			    GTK_SIGNAL_FUNC (close_button_pressed), procdata);
	gtk_signal_connect (GTK_OBJECT (memmapsdialog), "destroy",
			    GTK_SIGNAL_FUNC (close_memmaps_dialog), procdata);
	
	gtk_widget_show (memmapsdialog);
#if 1
	timer = gtk_timeout_add (procdata->config.update_interval, memmaps_timer, procdata);
#endif
	update_memmaps_dialog (procdata);
	
}
