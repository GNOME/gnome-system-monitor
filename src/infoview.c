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


#include <config.h>


#include "procman.h"
#include "infoview.h"
#include "interface.h"
#include "util.h"
#if 0
#include "procdialogs.h"
#include "memmaps.h"
#include "e-clipped-label.h"
#endif

GtkWidget *infobox;
GtkWidget *cmd_label;
GtkWidget *status_label;
GtkWidget *nice_label;
GtkWidget *memtotal_label;
GtkWidget *memrss_label;
GtkWidget *memshared_label;

static GtkWidget *
make_title_label (const char *text)
{
  GtkWidget *label;
  char *full;

  full = g_strdup_printf ("<span weight=\"bold\">%s</span>", text);
  label = gtk_label_new (full);
  g_free (full);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

  return label;
}

GtkWidget *
infoview_create (ProcData *data)
{
	ProcData *procdata = data;
	GtkWidget *infobox;
	GtkWidget *main_hbox;
	GtkWidget *info_table, *mem_table;
	GtkWidget *label;

	GtkWidget *info_title, *info_box;
	GtkWidget *mem_title, *mem_box;
	GtkWidget *spacer, *info_hbox, *mem_hbox;
	
	infobox = gtk_vbox_new (FALSE, 0);
		
	main_hbox = gtk_hbox_new (FALSE, 18);
	gtk_box_pack_start (GTK_BOX (infobox), main_hbox, FALSE, TRUE, 0);
	
	/* The Process Info Area */
	
	info_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_hbox), info_box, TRUE, TRUE, 0);

	info_title= make_title_label (_("Process Info"));
	gtk_box_pack_start (GTK_BOX (info_box), info_title, FALSE, FALSE, 0);

	info_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (info_box), info_hbox, TRUE, TRUE, 0);

	spacer = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (info_hbox), spacer, FALSE, FALSE, 0);

	info_table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (info_table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (info_table), 12);
	gtk_box_pack_start (GTK_BOX (info_hbox), info_table, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("Command:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), label, 0, 1, 0, 1, GTK_FILL, 
			  0, 0, 0);
	label = gtk_label_new (_("Status:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), label, 0, 1, 1, 2, GTK_FILL, 
			  0, 0, 0);
	label = gtk_label_new (_("Priority:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), label, 0, 1, 2, 3, GTK_FILL, 
			  0, 0, 0);
	
	cmd_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (cmd_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), cmd_label, 1, 2, 0, 1, GTK_FILL, 
			  0, 0, 0);
	
	status_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (status_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), status_label, 1, 2, 1, 2, GTK_FILL, 
			  0, 0, 0);
	nice_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (nice_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), nice_label, 1, 2, 2, 3, GTK_FILL, 
			  0, 0, 0);

	/*The Memory usage Area*/

	mem_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_end (GTK_BOX (main_hbox), mem_box, TRUE, TRUE, 0);

	mem_title= make_title_label (_("Memory Usage"));
	gtk_box_pack_start (GTK_BOX (mem_box), mem_title, FALSE, FALSE, 0);

	mem_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (mem_box), mem_hbox, TRUE, TRUE, 0);

	spacer = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (mem_hbox), spacer, FALSE, FALSE, 0);
	
	mem_table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (mem_table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (mem_table), 12);
	gtk_box_pack_start (GTK_BOX (mem_hbox), mem_table, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("Total:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), label, 0, 1, 0, 1, GTK_FILL, 
			  0, 0, 0);
	label = gtk_label_new (_("RSS:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), label, 0, 1, 1, 2, GTK_FILL, 
			  0, 0, 0);
	label = gtk_label_new (_("Shared:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), label, 0, 1, 2, 3, GTK_FILL, 
			  0, 0, 0);
			  
	memtotal_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (memtotal_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), memtotal_label, 1, 2, 0, 1, GTK_FILL, 
			  0, 0, 0);
	memrss_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (memrss_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), memrss_label, 1, 2, 1, 2, GTK_FILL, 
			  0, 0, 0);
	memshared_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (memshared_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), memshared_label, 1, 2, 2, 3, GTK_FILL, 
			  0, 0, 0);
			  
	procdata->infobox = infobox;

		
	return infobox;
	

}

void
infoview_update (ProcData *data)
{
	ProcData *procdata = data;
	ProcInfo *info;
	gchar *string;

	if (!procdata->selected_process) {
		update_sensitivity (procdata, FALSE);
		return;
	}

	info = procdata->selected_process;	

	gtk_label_set_text (GTK_LABEL (cmd_label), info->name);
	gtk_label_set_text (GTK_LABEL (status_label), info->status);
	
	if (info->nice < -7)
		string = g_strdup_printf (_("Very high - nice %d"), info->nice);
	else if (info->nice < -2)
		string = g_strdup_printf (_("High - nice %d"), info->nice);
	else if (info->nice < 3)
		string = g_strdup_printf (_("Normal - nice %d"), info->nice);
	else if (info->nice < 7)
		string = g_strdup_printf (_("Low - nice %d"), info->nice);
	else
		string = g_strdup_printf (_("Very low - nice %d"), info->nice);
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
