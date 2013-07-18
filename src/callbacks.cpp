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

#include <config.h>

#include <giomm.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <signal.h>

#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "util.h"
#include "procactions.h"
#include "procman-app.h"
#include "procdialogs.h"
#include "load-graph.h"
#include "disks.h"
#include "lsof.h"

void
cb_edit_preferences (GtkAction *action, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    procdialog_create_preferences_dialog (app);
}


void
cb_show_lsof(GtkAction *action, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);
    procman_lsof(app);
}


void
cb_about (GtkAction *action, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    const gchar * const authors[] = {
        "Kevin Vandersloot",
        "Erik Johnsson",
        "Jorgen Scheibengruber",
        "Benoît Dejean",
        "Paolo Borelli",
        "Karl Lattimer",
        "Chris Kühl",
        "Robert Roth",
        NULL
    };

    const gchar * const documenters[] = {
        "Bill Day",
        "Sun Microsystems",
        NULL
    };

    const gchar * const artists[] = {
        "Baptiste Mille-Mathias",
        NULL
    };

    gtk_show_about_dialog (
        GTK_WINDOW (app->main_window),
        "name",                 _("System Monitor"),
        "comments",             _("View current processes and monitor "
                                  "system state"),
        "version",              VERSION,
        "copyright",            "Copyright \xc2\xa9 2001-2004 Kevin Vandersloot\n"
                                "Copyright \xc2\xa9 2005-2007 Benoît Dejean\n"
                                "Copyright \xc2\xa9 2011 Chris Kühl",
        "logo-icon-name",       "utilities-system-monitor",
        "authors",              authors,
        "artists",              artists,
        "documenters",          documenters,
        "translator-credits",   _("translator-credits"),
        "license",              "GPL 2+",
        "wrap-license",         TRUE,
        NULL
        );
}


void
cb_help_contents (GtkAction *action, gpointer data)
{
    GError* error = 0;
    if (!g_app_info_launch_default_for_uri("help:gnome-system-monitor", NULL, &error)) {
        g_warning("Could not display help : %s", error->message);
        g_error_free(error);
    }
}


gboolean
cb_main_window_delete (GtkWidget *window, GdkEvent *event, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    app->shutdown ();

    return TRUE;
}

void
cb_cpu_color_changed (GSMColorButton *cp, gpointer data)
{
    guint cpu_i = GPOINTER_TO_UINT (data);
    GSettings *settings = g_settings_new (GSM_GSETTINGS_SCHEMA);

    /* Get current values */
    GVariant *cpu_colors_var = g_settings_get_value(settings, "cpu-colors");
    gsize children_n = g_variant_n_children(cpu_colors_var);

    /* Create builder to contruct new setting with updated value for cpu i */
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

    for (guint i = 0; i < children_n; i++) {
        if(cpu_i == i) {
            gchar *color;
            GdkRGBA button_color;
            gsm_color_button_get_color(cp, &button_color);
            color = gdk_rgba_to_string (&button_color);
            g_variant_builder_add(&builder, "(us)", i, color);
            g_free (color);
        } else {
            g_variant_builder_add_value(&builder,
                                        g_variant_get_child_value(cpu_colors_var, i));
        }
    }

    /* Just set the value and let the changed::cpu-colors signal callback do the rest. */
    g_settings_set_value(settings, "cpu-colors", g_variant_builder_end(&builder));
}

static void change_settings_color(GSettings *settings, const char *key,
                                  GSMColorButton *cp)
{
    GdkRGBA c;
    char *color;

    gsm_color_button_get_color(cp, &c);
    color = gdk_rgba_to_string (&c);
    g_settings_set_string (settings, key, color);
    g_free (color);
}

void
cb_mem_color_changed (GSMColorButton *cp, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);
    change_settings_color(app->settings, "mem-color", cp);
}


void
cb_swap_color_changed (GSMColorButton *cp, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);
    change_settings_color(app->settings, "swap-color", cp);
}

void
cb_net_in_color_changed (GSMColorButton *cp, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);
    change_settings_color(app->settings, "net-in-color", cp);
}

void
cb_net_out_color_changed (GSMColorButton *cp, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);
    change_settings_color(app->settings, "net-out-color", cp);
}

static void
get_last_selected (GtkTreeModel *model, GtkTreePath *path,
                   GtkTreeIter *iter, gpointer data)
{
    ProcInfo **info = static_cast<ProcInfo**>(data);

    gtk_tree_model_get (model, iter, COL_POINTER, info, -1);
}


void
cb_row_selected (GtkTreeSelection *selection, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    app->selection = selection;

    app->selected_process = NULL;

    /* get the most recent selected process and determine if there are
    ** no selected processes
    */
    gtk_tree_selection_selected_foreach (app->selection, get_last_selected,
                                         &app->selected_process);
    if (app->selected_process) {
        GVariant *priority;
        gint nice = app->selected_process->nice;
        if (nice < -7)
            priority = g_variant_new_string ("very-high");
        else if (nice < -2)
            priority = g_variant_new_string ("high");
        else if (nice < 3)
            priority = g_variant_new_string ("normal");
        else if (nice < 7)
            priority = g_variant_new_string ("low");
        else
            priority = g_variant_new_string ("very-low");

        GAction *action = g_action_map_lookup_action (G_ACTION_MAP (app->main_window),
                                                      "priority");

        g_action_change_state (action, priority);
    }
    update_sensitivity(app);
}


gint
cb_timeout (gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);
    guint new_interval;


    proctable_update_all (app);

    if (app->smooth_refresh->get(new_interval))
    {
        app->timeout = g_timeout_add(new_interval,
                                     cb_timeout,
                                     app);
        return FALSE;
    }

    return TRUE;
}

void
cb_refresh_icons (GtkIconTheme *theme, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);
    if(app->timeout) {
        g_source_remove (app->timeout);
    }

    for (ProcInfo::Iterator it(ProcInfo::begin()); it != ProcInfo::end(); ++it) {
        app->pretty_table->set_icon(*(it->second));
    }

    cb_timeout(app);
}


void
cb_column_resized(GtkWidget *widget, GParamSpec* param, gpointer data)
{
    GSettings * settings = static_cast<GSettings *>(data);
    GtkTreeViewColumn *column = GTK_TREE_VIEW_COLUMN(widget);
    gint width;
    gchar *key;
    int id;
    
    gint saved_width;

    id = gtk_tree_view_column_get_sort_column_id (column);
    width = gtk_tree_view_column_get_width (column);
    key = g_strdup_printf ("col-%d-width", id);

    g_settings_get (settings, key, "i", &saved_width);
    if (saved_width!=width)
    {
        g_settings_set_int(settings, key, width);
    }
    g_free (key);
}


static void
cb_header_menu_position_function(GtkMenu* menu, gint *x, gint *y, gboolean *push_in, gpointer data)
{
    GdkEventButton* event = static_cast<GdkEventButton*>(data);
    gint wx, wy, ww, wh;
    gdk_window_get_geometry(event->window, &wx, &wy, &ww, &wh);
    gdk_window_get_origin(event->window, &wx, &wy);
    
    *x = wx + event->x;
    *y = wy + wh;
    *push_in = TRUE;
}

gboolean
cb_column_header_clicked (GtkTreeViewColumn* column, GdkEvent* event, gpointer data)
{
    GtkMenu *menu = static_cast<GtkMenu*>(data);
    if (event->button.button == GDK_BUTTON_SECONDARY) {
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, cb_header_menu_position_function, &(event->button), event->button.button, event->button.time);
        return TRUE;
    }

    return FALSE;
    
}

