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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "procman.h"
#include "infoview.h"
#include "procdialogs.h"
#include "memmaps.h"


GtkWidget *infobox;
GtkWidget *main_frame;
GtkWidget *cmd_event_box;
GtkWidget *cmd_label;
GtkWidget *status_label;
GtkWidget *nice_label;
GtkWidget *memtotal_label;
GtkWidget *memrss_label;
GtkWidget *memshared_label;
GtkTooltips *cmd_tooltip;

GtkWidget *
infoview_create (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *separator;
	GtkWidget *infobox;
	GtkWidget *main_hbox;
	GtkWidget *info_frame, *mem_frame;
	GtkWidget *info_table, *mem_table;
	GtkWidget *label;

	
	infobox = gtk_vbox_new (FALSE, 0);
	
	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (infobox), separator, FALSE, FALSE, 0);
	
	
	main_frame = gtk_frame_new (NULL);
	gtk_frame_set_label_align (GTK_FRAME (main_frame), 0.0, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (main_frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (infobox), main_frame, FALSE, FALSE, 0);
	

	main_hbox = gtk_hbox_new (TRUE, 0);
	gtk_container_add (GTK_CONTAINER (main_frame), main_hbox);
	
	info_frame = gtk_frame_new (_("Process Info"));
	gtk_frame_set_label_align (GTK_FRAME (info_frame), 0.5, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (info_frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (main_hbox), info_frame, TRUE, TRUE, 0);
	
	info_table = gtk_table_new (3, 2, FALSE);
	gtk_container_add (GTK_CONTAINER (info_frame), info_table);
	
	label = gtk_label_new (_("Command : "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), label, 0, 1, 0, 1, GTK_FILL, 
			  0,GNOME_PAD_SMALL, 0);
	label = gtk_label_new (_("Status : "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), label, 0, 1, 1, 2, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
	label = gtk_label_new (_("Nice Value : "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), label, 0, 1, 2, 3, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
	
	cmd_event_box = gtk_event_box_new ();
	gtk_table_attach (GTK_TABLE (info_table), cmd_event_box, 1, 2, 0, 1, GTK_FILL,
			  0, GNOME_PAD_SMALL, 0);
	
	cmd_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (cmd_label), 0.0, 0.5);
	gtk_container_add (GTK_CONTAINER (cmd_event_box), cmd_label);
	
	cmd_tooltip = gtk_tooltips_new ();
	
	status_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (status_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), status_label, 1, 2, 1, 2, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
	nice_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (nice_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), nice_label, 1, 2, 2, 3, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
	
	mem_frame = gtk_frame_new (_("Memory Usage"));
	gtk_frame_set_label_align (GTK_FRAME (mem_frame), 0.5, 0.5);
	gtk_container_set_border_width (GTK_CONTAINER (mem_frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (main_hbox), mem_frame, TRUE, TRUE, 0);
	
	mem_table = gtk_table_new (3, 2, FALSE);
	gtk_container_add (GTK_CONTAINER (mem_frame), mem_table);
	
	
	label = gtk_label_new (_("Total : "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), label, 0, 1, 0, 1, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
	label = gtk_label_new (_("RSS : "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), label, 0, 1, 1, 2, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
	label = gtk_label_new (_("Shared : "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), label, 0, 1, 2, 3, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
			  
	memtotal_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (memtotal_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), memtotal_label, 1, 2, 0, 1, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
	memrss_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (memrss_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), memrss_label, 1, 2, 1, 2, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
	memshared_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (memshared_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), memshared_label, 1, 2, 2, 3, GTK_FILL, 
			  0, GNOME_PAD_SMALL, 0);
	
	procdata->infobox = infobox;

		
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
	gchar *string;
	gchar *command;
	
	if (!procdata->selected_node)
		return;

	info = e_tree_memory_node_get_data (procdata->memory, procdata->selected_node);	
	
	if (!info)
	{
		if (GTK_WIDGET_SENSITIVE (procdata->infobox))
			gtk_widget_set_sensitive (procdata->infobox, FALSE);
		return;
	}
	
	gtk_frame_set_label (GTK_FRAME (main_frame), info->name);
	gtk_label_set_text (GTK_LABEL (status_label), info->status);
	/* ugh. don't update the tooltip if the selected process has not changed.
	** This is here so that when the user has to tooltip showing and we do an
	** update here, the tooltip will go away if we call gtk_tooltip_set_tip
	*/
	gtk_label_get (GTK_LABEL (cmd_label), &command);
	if (g_strcasecmp (info->cmd, command))
		gtk_tooltips_set_tip (cmd_tooltip, cmd_event_box, info->arguments, NULL);
	gtk_label_set_text (GTK_LABEL (cmd_label), info->cmd);
	string = g_strdup_printf ("%d", info->nice);
	gtk_label_set_text (GTK_LABEL (nice_label), string);
	g_free (string); 
	string = get_size_string (info->mem);
	gtk_label_set_text (GTK_LABEL (memtotal_label), string);
	g_free (string);
	string = get_size_string (info->memrss);
	gtk_label_set_text (GTK_LABEL (memrss_label), string);
	g_free (string);
	string = get_size_string (info->memshared);
	gtk_label_set_text (GTK_LABEL (memshared_label), string);
	g_free (string);
	
	
}
