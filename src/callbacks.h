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
 
 
#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

#include <gnome.h>
#include "procman.h"
#include <gal/e-table/e-table.h>



void
cb_properties_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
cb_show_memory_maps (GtkMenuItem *menuitem, gpointer data);

void
cb_renice (GtkMenuItem *menuitem, gpointer data);

void
cb_preferences_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
cb_about_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
cb_app_destroy                        (GtkObject       *object,
                                        gpointer         user_data);
                                        
void
cb_all_process_menu_clicked 		(GtkWidget	*widget,
					 gpointer	data);	
					 
void
cb_my_process_menu_clicked		(GtkWidget	*widget,
					 gpointer	data);  
					 
void	cb_running_process_menu_clicked	(GtkWidget *widget, gpointer data);

void
cb_end_process_button_pressed          (GtkButton       *button,
                                        gpointer         data);

void
cb_info_button_pressed			(GtkButton	*button,
					gpointer	user_data);
				
void
cb_table_selected			(ETree		*tree,
					int		row,
					ETreePath	path,
					gpointer	data);
					 
void 
cb_double_click				(ETree		*tree,
					 int		row,
					 ETreePath	path,
					 int		col,
					 GdkEvent	*event,
					 gpointer	data);
					 


gint
cb_timeout 				(gpointer data);


#endif
