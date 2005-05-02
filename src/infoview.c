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

#include <libgnomevfs/gnome-vfs-utils.h>
#include <glib/gi18n.h>
#include "procman.h"
#include "infoview.h"
#include "interface.h"
#include "util.h"


static const char *
expander_get_label (gboolean more_info)
{
	return (more_info ? _("Less _Info") : _("More _Info"));
}


static void
expander_callback (GObject    *object,
		   GParamSpec *param_spec,
		   gpointer    user_data)
{
	ProcData *procdata = user_data;
	GtkExpander *expander;

	expander = GTK_EXPANDER (object);

	if (gtk_expander_get_expanded (expander))
	{
		procdata->config.show_more_info = TRUE;

		infoview_update (procdata);
		gtk_widget_show (procdata->infoview.box);
	}
	else
	{
		procdata->config.show_more_info = FALSE;

		gtk_widget_hide (procdata->infoview.box);
	}

	gtk_expander_set_label (
		expander,
		expander_get_label(procdata->config.show_more_info)
		);
}



void
infoview_create (ProcData *procdata)
{
	Infoview * const infoview = &procdata->infoview;

	GtkWidget *main_hbox;
	GtkWidget *info_table, *mem_table;
	GtkWidget *label;

	GtkWidget *info_title, *info_box;
	GtkWidget *mem_title, *mem_box;
	GtkWidget *spacer, *info_hbox, *mem_hbox;

	infoview->box = gtk_vbox_new (FALSE, 0);

	main_hbox = gtk_hbox_new (FALSE, 18);
	gtk_box_pack_start (GTK_BOX (infoview->box), main_hbox, FALSE, TRUE, 0);

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

	infoview->cmd_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (infoview->cmd_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), infoview->cmd_label, 1, 2, 0, 1, GTK_FILL,
			  0, 0, 0);

	infoview->status_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (infoview->status_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), infoview->status_label, 1, 2, 1, 2, GTK_FILL,
			  0, 0, 0);
	infoview->nice_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (infoview->nice_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (info_table), infoview->nice_label, 1, 2, 2, 3, GTK_FILL,
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

	infoview->memtotal_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (infoview->memtotal_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), infoview->memtotal_label, 1, 2, 0, 1, GTK_FILL,
			  0, 0, 0);
	infoview->memrss_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (infoview->memrss_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), infoview->memrss_label, 1, 2, 1, 2, GTK_FILL,
			  0, 0, 0);
	infoview->memshared_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (infoview->memshared_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (mem_table), infoview->memshared_label, 1, 2, 2, 3, GTK_FILL,
			  0, 0, 0);

	infoview->expander = gtk_expander_new_with_mnemonic (
		expander_get_label(procdata->config.show_more_info));
	g_signal_connect (infoview->expander, "notify::expanded",
			  G_CALLBACK (expander_callback), procdata);

	gtk_container_add (GTK_CONTAINER(infoview->expander), infoview->box);
}

void
infoview_update (ProcData *procdata)
{
	ProcInfo *info;
	gchar *string;

	if (!procdata->selected_process) {
		update_sensitivity (procdata, FALSE);
		return;
	}

	info = procdata->selected_process;

	gtk_label_set_text (GTK_LABEL (procdata->infoview.cmd_label), info->name);
	gtk_label_set_text (GTK_LABEL (procdata->infoview.status_label), info->status);

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
	gtk_label_set_text (GTK_LABEL (procdata->infoview.nice_label), string);
	g_free (string);

	string = SI_gnome_vfs_format_file_size_for_display (info->mem);
	gtk_label_set_text (GTK_LABEL (procdata->infoview.memtotal_label), string);
	g_free (string);

	string = SI_gnome_vfs_format_file_size_for_display (info->memrss);
	gtk_label_set_text (GTK_LABEL (procdata->infoview.memrss_label), string);
	g_free (string);

	string = SI_gnome_vfs_format_file_size_for_display (info->memshared);
	gtk_label_set_text (GTK_LABEL (procdata->infoview.memshared_label), string);
	g_free (string);
}
