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


void		cb_properties_activate (GtkMenuItem *menuitem, gpointer user_data) G_GNUC_INTERNAL;

void		cb_show_memory_maps (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;
void		cb_show_open_files (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;
void		cb_renice (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;
void		cb_add_to_favorites (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;
void		cb_end_process (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;
void		cb_kill_process (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;
void		cb_hide_process (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;
void		cb_show_hidden_processes (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;
void		cb_preferences_activate (GtkMenuItem *menuitem, gpointer user_data) G_GNUC_INTERNAL;

void		cb_about_activate (GtkMenuItem *menuitem, gpointer user_data) G_GNUC_INTERNAL;

void		cb_app_exit (GtkObject *object, gpointer user_data) G_GNUC_INTERNAL;
gboolean	cb_app_delete (GtkWidget *window, GdkEventAny *event, gpointer data) G_GNUC_INTERNAL;

void		cb_proc_combo_changed (GtkComboBox *combo, gpointer data) G_GNUC_INTERNAL;

void		cb_end_process_button_pressed (GtkButton *button, gpointer data) G_GNUC_INTERNAL;
void		cb_logout (GtkButton *button, gpointer data) G_GNUC_INTERNAL;

void		popup_menu_about_process (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;
void		popup_menu_show_open_files (GtkMenuItem *menuitem, gpointer data) G_GNUC_INTERNAL;

void		cb_info_button_pressed (GtkButton *button, gpointer user_data) G_GNUC_INTERNAL;
void		cb_search (GtkEditable *editable, gpointer data) G_GNUC_INTERNAL;


void		cb_cpu_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;
void		cb_mem_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;
void		cb_swap_color_changed (GtkColorButton *widget, gpointer user_data) G_GNUC_INTERNAL;
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


#endif /* _PROCMAN_CALLBACKS_H_ */
