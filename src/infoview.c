/* Procman - more info view
 * Copyright (C) 2001 Kevin Vandersloot
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#include "procman.h"
#include "infoview.h"
#include "procdialogs.h"
#include "memmaps.h"


static void
add_separator (GtkWidget *box)
{
	GtkWidget *separator;
	
	separator = gtk_vseparator_new ();
	gtk_box_pack_start (GTK_BOX (box), separator, TRUE, FALSE, 0);
}

static void
memmaps_button_clicked (GtkButton *button, gpointer data)
{
	ProcData *procdata = data;
	
	create_memmaps_dialog (procdata);
}

static void
nice_button_clicked (GtkButton *button, gpointer data)
{

	ProcData *procdata = data;
	procdialog_create_renice_dialog (procdata);
	
}



GtkWidget *
infoview_create (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *infobox;
	GtkWidget *separator;
	GtkWidget *main_frame;
	GtkWidget *main_vbox;
	GtkWidget *info_hbox, *mem_hbox;
	GtkWidget *info_frame, *mem_frame;
	GtkWidget *nice_button;
	GtkWidget *label;
	GtkWidget *cmd_entry;
	GtkWidget *status_entry;
	GtkWidget *nice_entry;
	GtkWidget *memtotal_entry;
	GtkWidget *memrss_entry;
	GtkWidget *memshared_entry;
	GtkWidget *memmaps_button;
	
	infobox = gtk_vbox_new (FALSE, 0);
	
	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (infobox), separator, FALSE, FALSE, 0);
	
	
	main_frame = gtk_frame_new ("");
	gtk_frame_set_label_align (GTK_FRAME (main_frame), 0.0, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (main_frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (infobox), main_frame, FALSE, FALSE, 0);
	
	
	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (main_frame), main_vbox);
	
	info_frame = gtk_frame_new (_("Process Info"));
	gtk_frame_set_label_align (GTK_FRAME (info_frame), 0.5, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (info_frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (main_vbox), info_frame, FALSE, FALSE, 0);
	
	info_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (info_frame), info_hbox);
	
	label = gtk_label_new (_("Command : "));
	gtk_box_pack_start (GTK_BOX (info_hbox), label, TRUE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, GNOME_PAD_SMALL);
	
	cmd_entry = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (cmd_entry), FALSE);
	gtk_widget_set_usize (cmd_entry, 60, -2);
	gtk_box_pack_start (GTK_BOX (info_hbox), cmd_entry, TRUE, FALSE, 0);
	
	add_separator (info_hbox);
	
	label = gtk_label_new (_("Status : "));
	gtk_box_pack_start (GTK_BOX (info_hbox), label, TRUE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD_SMALL, GNOME_PAD_SMALL);
	
	status_entry = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (status_entry), FALSE);
	gtk_widget_set_usize (status_entry, 60, -2);
	gtk_box_pack_start (GTK_BOX (info_hbox), status_entry, TRUE, FALSE, 0);
	
	add_separator (info_hbox);
	
	label = gtk_label_new (_("Nice Value : "));
	gtk_box_pack_start (GTK_BOX (info_hbox), label, TRUE, FALSE, 0);
	
	nice_entry = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (nice_entry), FALSE);
	gtk_widget_set_usize (nice_entry, 35, -2);
	gtk_box_pack_start (GTK_BOX (info_hbox), nice_entry, TRUE, FALSE, 0);
	
	nice_button = gtk_button_new_with_label (_("Renice ..."));
	gtk_box_pack_start (GTK_BOX (info_hbox), nice_button, TRUE, FALSE, 0);
	
		
	mem_frame = gtk_frame_new (_("Memory Usage"));
	gtk_frame_set_label_align (GTK_FRAME (mem_frame), 0.5, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (mem_frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (main_vbox), mem_frame, FALSE, FALSE, 0);
	
	mem_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (mem_frame), mem_hbox);
	
	label = gtk_label_new (_("Total : "));
	gtk_box_pack_start (GTK_BOX (mem_hbox), label, TRUE, FALSE, 0);
	
	memtotal_entry = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (memtotal_entry), FALSE);
	gtk_widget_set_usize (memtotal_entry, 50, -2);
	gtk_box_pack_start (GTK_BOX (mem_hbox), memtotal_entry, TRUE, FALSE, 0);
	
	add_separator (mem_hbox);
	
	label = gtk_label_new (_("RSS : "));
	gtk_box_pack_start (GTK_BOX (mem_hbox), label, TRUE, FALSE, 0);
	
	memrss_entry = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (memrss_entry), FALSE);
	gtk_widget_set_usize (memrss_entry, 50, -2);
	gtk_box_pack_start (GTK_BOX (mem_hbox), memrss_entry, TRUE, FALSE, 0);
	
	add_separator (mem_hbox);
	
	label = gtk_label_new (_("Shared : "));
	gtk_box_pack_start (GTK_BOX (mem_hbox), label, TRUE, FALSE, 0);
	
	memshared_entry = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (memshared_entry), FALSE);
	gtk_widget_set_usize (memshared_entry, 50, -2);
	gtk_box_pack_start (GTK_BOX (mem_hbox), memshared_entry, TRUE, FALSE, 0);
	
	add_separator (mem_hbox);
	
	memmaps_button = gtk_button_new_with_label (_("Memory Maps ..."));
	gtk_box_pack_start (GTK_BOX (mem_hbox), memmaps_button, TRUE, FALSE, 0);
	
	procdata->infoview->main_frame = main_frame;
	procdata->infoview->infobox = infobox;
	procdata->infoview->cmd_entry = cmd_entry;
	procdata->infoview->status_entry = status_entry;
	procdata->infoview->nice_entry = nice_entry;
	procdata->infoview->memtotal_entry = memtotal_entry;
	procdata->infoview->memrss_entry = memrss_entry;
	procdata->infoview->memshared_entry = memshared_entry;
	
	if (procdata->selected_pid == -1)
		gtk_widget_set_sensitive (infobox, FALSE);
	
	gtk_signal_connect (GTK_OBJECT (nice_button), "clicked",
			    GTK_SIGNAL_FUNC (nice_button_clicked), procdata);
			   
	gtk_signal_connect (GTK_OBJECT (memmaps_button), "clicked",
			    GTK_SIGNAL_FUNC (memmaps_button_clicked), procdata);
	
	return infobox;
	

}


/* stolen from gal */
static gchar *
get_size_string (gint size)
{
	gfloat fsize;

	if (size < 1024) 
	{
		return g_strdup_printf ("%d bytes", size);

	} 
	else 
	{
		fsize = ((gfloat) size) / 1024.0;
		if (fsize < 1024.0) 
		{
			return g_strdup_printf ("%d K", (int)fsize);
		} 
		else 
		{
			fsize /= 1024.0;
			return g_strdup_printf ("%.1f MB", fsize);
		}
      	}



}


void
infoview_update (ProcData *data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	InfoView *infoview = procdata->infoview;
	gchar *string;
	
	if (!procdata->selected_node)
		return;


	info = e_tree_memory_node_get_data (procdata->memory, procdata->selected_node);	
	
	if (!info)
	{
		if (GTK_WIDGET_SENSITIVE (procdata->infoview->infobox))
			gtk_widget_set_sensitive (procdata->infoview->infobox, FALSE);
		return;
	}
	
	gtk_frame_set_label (GTK_FRAME (infoview->main_frame), info->name);
	gtk_entry_set_text (GTK_ENTRY (infoview->status_entry), info->status);
	gtk_entry_set_text (GTK_ENTRY (infoview->cmd_entry), info->cmd);
	string = g_strdup_printf ("%d", info->nice);
	gtk_entry_set_text (GTK_ENTRY (infoview->nice_entry), string);
	g_free (string); 
	string = get_size_string (info->mem);
	gtk_entry_set_text (GTK_ENTRY (infoview->memtotal_entry), string);
	g_free (string);
	string = get_size_string (info->memrss);
	gtk_entry_set_text (GTK_ENTRY (infoview->memrss_entry), string);
	g_free (string);
	string = get_size_string (info->memshared);
	gtk_entry_set_text (GTK_ENTRY (infoview->memshared_entry), string);
	g_free (string);
}
