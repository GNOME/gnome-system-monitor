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
#include "memmaps.h"
#include "openfiles.h"
#include "procproperties.h"
#include "load-graph.h"
#include "disks.h"
#include "lsof.h"

void
cb_kill_sigstop(GtkAction *action, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    /* no confirmation */
    kill_process (app, SIGSTOP);
}




void
cb_kill_sigcont(GtkAction *action, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    /* no confirmation */
    kill_process (app, SIGCONT);
}



static void
kill_process_helper(ProcmanApp *app, int sig)
{
    if (app->config.show_kill_warning)
        procdialog_create_kill_dialog (app, sig);
    else
        kill_process (app, sig);
}


void
cb_edit_preferences (GtkAction *action, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    procdialog_create_preferences_dialog (app);
}


void
cb_renice (GtkAction *action, GtkRadioAction *current, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    gint selected = gtk_radio_action_get_current_value(current);

    if (selected == CUSTOM_PRIORITY)
    {
       procdialog_create_renice_dialog (app);
    } else {
       gint new_nice_value = 0;
       switch (selected) {
           case VERY_HIGH_PRIORITY: new_nice_value = -20; break;
           case HIGH_PRIORITY: new_nice_value = -5; break;
           case NORMAL_PRIORITY: new_nice_value = 0; break;
           case LOW_PRIORITY: new_nice_value = 5; break;
           case VERY_LOW_PRIORITY: new_nice_value = 19; break;
       }
       renice(app, new_nice_value);
    }
}


void
cb_end_process (GtkAction *action, gpointer data)
{
    kill_process_helper(static_cast<ProcmanApp *>(data), SIGTERM);
}


void
cb_kill_process (GtkAction *action, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);
    kill_process_helper(app, SIGKILL);
}


void
cb_show_memory_maps (GtkAction *action, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    create_memmaps_dialog (app);
}

void
cb_show_open_files (GtkAction *action, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    create_openfiles_dialog (app);
}

void
cb_show_process_properties (GtkAction *action, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);
    create_procproperties_dialog (app);
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
cb_end_process_button_pressed (GtkButton *button, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);
    kill_process_helper(app, SIGTERM);
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
        gint value;
        gint nice = app->selected_process->nice;
        if (nice < -7)
            value = VERY_HIGH_PRIORITY;
        else if (nice < -2)
            value = HIGH_PRIORITY;
        else if (nice < 3)
            value = NORMAL_PRIORITY;
        else if (nice < 7)
            value = LOW_PRIORITY;
        else
            value = VERY_LOW_PRIORITY;

        GtkRadioAction* normal = GTK_RADIO_ACTION(gtk_action_group_get_action(app->action_group, "Normal"));
        block_priority_changed_handlers(app, TRUE);
        gtk_radio_action_set_current_value(normal, value);
        block_priority_changed_handlers(app, FALSE);

    }
    update_sensitivity(app);
}


gboolean
cb_tree_button_pressed (GtkWidget *widget,
                        GdkEventButton *event,
                        gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
        do_popup_menu (app, event);
        return TRUE;
    }
    return FALSE;
}


gboolean
cb_tree_popup_menu (GtkWidget *widget, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    do_popup_menu (app, NULL);

    return TRUE;
}


void
cb_switch_page (GtkNotebook *nb, GtkWidget *page,
                gint num, gpointer data)
{
    cb_change_current_page (nb, num, data);
}

void
cb_change_current_page (GtkNotebook *nb, gint num, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    app->config.current_tab = num;


    if (num == PROCMAN_TAB_PROCESSES) {

        cb_timeout (app);

        if (!app->timeout)
            app->timeout = g_timeout_add (
                app->config.update_interval,
                cb_timeout, app);

        update_sensitivity(app);
    }
    else {
        if (app->timeout) {
            g_source_remove (app->timeout);
            app->timeout = 0;
        }

        update_sensitivity(app);
    }


    if (num == PROCMAN_TAB_RESOURCES) {
        load_graph_start (app->cpu_graph);
        load_graph_start (app->mem_graph);
        load_graph_start (app->net_graph);
    }
    else {
        load_graph_stop (app->cpu_graph);
        load_graph_stop (app->mem_graph);
        load_graph_stop (app->net_graph);
    }


    if (num == PROCMAN_TAB_DISKS) {

        cb_update_disks (app);

        if(!app->disk_timeout) {
            app->disk_timeout =
                g_timeout_add (app->config.disks_update_interval,
                               cb_update_disks,
                               app);
        }
    }
    else {
        if(app->disk_timeout) {
            g_source_remove (app->disk_timeout);
            app->disk_timeout = 0;
        }
    }

}



gint
cb_user_refresh (GtkAction*, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);
    proctable_update_all(app);
    return FALSE;
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
cb_radio_processes(GtkAction *action, GtkRadioAction *current, gpointer data)
{
    ProcmanApp * const app = static_cast<ProcmanApp *>(data);

    app->config.whose_process = gtk_radio_action_get_current_value(current);

    g_settings_set_int (app->settings, "view-as",
                        app->config.whose_process);
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

