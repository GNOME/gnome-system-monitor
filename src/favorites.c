/* Procman - ability to show favorite processes
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


#include "favorites.h"


void
add_to_favorites (ProcData *procdata, gchar *name)
{
	gchar *favorite = g_strdup (name);
	procdata->favorites = g_list_append (procdata->favorites, favorite);

}


void
remove_from_favorites (ProcData *procdata)
{

}


gboolean
is_process_a_favorite (ProcData *procdata, gchar *name)
{
	GList *list = procdata->favorites;
	
	if (!list)
	{
		g_print ("no favorites \n");
		return FALSE;
	}
	
	while (list)
	{
		gchar *favorite = list->data;
		if (!g_strcasecmp (favorite, name))
			return TRUE;
		
		list = g_list_next (list);
	}
	
	return FALSE;

}

void save_favorites (ProcData *procdata)
{

	GList *list = procdata->favorites;
	gint i = 0;
	
	while (list)
	{
		gchar *name = list->data;
		gchar *config = g_strdup_printf ("%s%d", "procman/Favorites/favorite", i);
		gnome_config_set_string (config, name);
		g_free (config); 
		i++;
		list = g_list_next (list);
	}
}


void get_favorites (ProcData *procdata)
{
	gint i = 0;
	gboolean done = FALSE;
	
	while (!done)
	{
		gchar *config = g_strdup_printf ("%s%d", "procman/Favorites/favorite", i);
		gchar *favorite;
		
		favorite = gnome_config_get_string (config);
		if (favorite)
			add_to_favorites (procdata, favorite);
		else
			done = TRUE;
		i++;
	}
	
} 
