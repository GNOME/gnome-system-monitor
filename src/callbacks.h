/* Procman - callbacks
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


#ifndef _PROCMAN_CALLBACKS_H_
#define _PROCMAN_CALLBACKS_H_

#include <gtk/gtk.h>
#include "procman.h"
#include <libgnomevfs/gnome-vfs.h>

G_BEGIN_DECLS

void		cb_show_memory_maps (GtkAction *action, gpointer data) G_GNUC_INTERNAL;
void		cb_show_open_files (GtkAction *action, gpointer data) G_GNUC_INTERNAL;
void		cb_show_lsof(GtkAction *action, gpointer data) G_GNUC_INTERNAL;
void		cb_renice (GtkAction *action, gpointer data) G_GNUC_INTERNAL;
void		cb_end_process (GtkAction *action, gpointer data) G_GNUC_INTERNAL;
void		cb_kill_process (GtkAction *action, gpointer data) G_GNUC_INTERNAL;
void		cb_hide_process (GtkAction *action, gpointer data) G_GNUC_INTERNAL;
void		cb_show_hidden_processes (GtkAction *action, gpointer data) G_GNUC_INTERNAL;
void		cb_edit_preferences (GtkAction *action, gpointer data) G_GNUC_INTERNAL;

void		cb_help_contents (GtkAction *action, gpointer data) G_GNUC_INTERNAL;
void		cb_about (GtkAction *action, gpointer data) G_GNUC_INTERNAL;

void		cb_app_exit (GtkAction *action, gpointer data) G_GNUC_INTERNAL;
gboolean	cb_app_delete (GtkWidget *window, GdkEventAny *event, gpointer data) G_GNUC_INTERNAL;

void		cb_end_process_button_pressed (GtkButton *button, gpointer data) G_GNUC_INTERNAL;
void		cb_logout (GtkButton *button, gpointer data) G_GNUC_INTERNAL;

void		cb_info_button_pressed (GtkButton *button, gpointer user_data) G_GNUC_INTERNAL;

void		cb_cpu_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;
void		cb_mem_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;
void		cb_swap_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;
void		cb_net_in_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;
void		cb_net_out_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;
void		cb_bg_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;
void		cb_frame_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;

void		cb_row_selected (GtkTreeSelection *selection, gpointer data) G_GNUC_INTERNAL;

gboolean	cb_tree_popup_menu (GtkWidget *widget, gpointer data) G_GNUC_INTERNAL;
gboolean	cb_tree_button_pressed (GtkWidget *widget, GdkEventButton *event,
					gpointer data) G_GNUC_INTERNAL;


void		cb_change_current_page (GtkNotebook *nb,
					gint num, gpointer data) G_GNUC_INTERNAL;
void		cb_switch_page (GtkNotebook *nb, GtkNotebookPage *page,
				gint num, gpointer data) G_GNUC_INTERNAL;

gint		cb_update_disks (gpointer data) G_GNUC_INTERNAL;
gint		cb_timeout (gpointer data) G_GNUC_INTERNAL;

void		cb_volume_mounted_or_unmounted(GnomeVFSVolumeMonitor *vfsvolumemonitor,
					       GnomeVFSVolume *vol,
					       gpointer procdata) G_GNUC_INTERNAL;

void		cb_radio_processes(GtkAction *action,
				   GtkRadioAction *current,
				   gpointer data) G_GNUC_INTERNAL;



void		cb_kill_sigstop(GtkAction *action,
				gpointer data) G_GNUC_INTERNAL;

void		cb_kill_sigcont(GtkAction *action,
				gpointer data) G_GNUC_INTERNAL;
G_END_DECLS

#endif /* _PROCMAN_CALLBACKS_H_ */
