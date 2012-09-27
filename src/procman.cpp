/* Procman
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

#include <stdlib.h>

#include <locale.h>

#include <gtkmm.h>
#include <glib/gi18n.h>

#include "procman-app.h"
#include "procman.h"
#include "proctable.h"

ProcData::ProcData()
    : tree(NULL),
      cpu_graph(NULL),
      mem_graph(NULL),
      net_graph(NULL),
      selected_process(NULL),
      timeout(0),
      disk_timeout(0),
      cpu_total_time(1),
      cpu_total_time_last(1)
{ }


ProcData* ProcData::get_instance()
{
    static ProcData instance;
    return &instance;
}

gboolean
procman_get_tree_state (GSettings *settings, GtkWidget *tree, const gchar *child_schema)
{
    GtkTreeModel *model;
    GList *columns, *it;
    gint sort_col;
    GtkSortType order;

    g_assert(tree);
    g_assert(child_schema);

    GSettings *pt_settings = g_settings_get_child (settings, child_schema);

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

    sort_col = g_settings_get_int (pt_settings, "sort-col");

    order = static_cast<GtkSortType>(g_settings_get_int (pt_settings, "sort-order"));

    if (sort_col != -1)
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
                                              sort_col,
                                              order);

    columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));

    if(!g_strcmp0(child_schema, "proctree"))
    {
        for(it = columns; it; it = it->next)
        {
            GtkTreeViewColumn *column;
            gint width;
            gboolean visible;
            int id;
            gchar *key;

            column = static_cast<GtkTreeViewColumn*>(it->data);
            id = gtk_tree_view_column_get_sort_column_id (column);

            key = g_strdup_printf ("col-%d-width", id);
            g_settings_get (pt_settings, key, "i", &width);
            g_free (key);

            key = g_strdup_printf ("col-%d-visible", id);
            visible = g_settings_get_boolean (pt_settings, key);
            g_free (key);

            column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), id);
            if(!column) continue;
            gtk_tree_view_column_set_visible (column, visible);
            if (visible) {
                /* ensure column is really visible */
                width = MAX(width, 10);
                gtk_tree_view_column_set_fixed_width(column, width);
            }
        }
    }

    if(!g_strcmp0(child_schema, "proctree") ||
       !g_strcmp0(child_schema, "disktreenew"))
    {
        GVariant     *value;
        GVariantIter iter;
        int          sortIndex;

        GSList *order = NULL;

        value = g_settings_get_value(pt_settings, "columns-order");
        g_variant_iter_init(&iter, value);

        while (g_variant_iter_loop (&iter, "i", &sortIndex))
            order = g_slist_append(order, GINT_TO_POINTER(sortIndex));

        proctable_set_columns_order(GTK_TREE_VIEW(tree), order);

        g_slist_free(order);
    }

    g_object_unref(pt_settings);
    pt_settings = NULL;

    g_list_free(columns);

    return TRUE;
}

void
procman_save_tree_state (GSettings *settings, GtkWidget *tree, const gchar *child_schema)
{
    GtkTreeModel *model;
    GList *columns;
    gint sort_col;
    GtkSortType order;

   if (ProcData::get_instance()->terminating)
        return;

    g_assert(tree);
    g_assert(child_schema);

    GSettings *pt_settings = g_settings_get_child (settings, child_schema);

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
    if (gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model), &sort_col,
                                              &order)) {
        g_settings_set_int (pt_settings, "sort-col", sort_col);
        g_settings_set_int (pt_settings, "sort-order", order);
    }

    columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));

    if(!g_strcmp0(child_schema, "proctree") || !g_strcmp0(child_schema, "disktreenew"))
    {
        GSList *order;
        GSList *order_node;
        GVariantBuilder *builder;
        GVariant *order_variant;

        order = proctable_get_columns_order(GTK_TREE_VIEW(tree));

        builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);

        for(order_node = order; order_node; order_node = order_node->next)
            g_variant_builder_add(builder, "i", GPOINTER_TO_INT(order_node->data));

        order_variant = g_variant_new("ai", builder);
        g_settings_set_value(pt_settings, "columns-order", order_variant);

        g_slist_free(order);
    }

    g_list_free(columns);
}

void
procman_save_config (ProcData *data)
{
    GSettings *settings = data->settings;

    g_assert(data);

    procman_save_tree_state (data->settings, data->tree, "proctree");
    procman_save_tree_state (data->settings, data->disk_list, "disktreenew");

    data->config.maximized = gdk_window_get_state(gtk_widget_get_window (data->app)) & GDK_WINDOW_STATE_MAXIMIZED;

    if (!data->config.maximized) {
        // we only want to store/overwrite size and position info with non-maximized state info
        data->config.width  = gdk_window_get_width (gtk_widget_get_window (data->app));
        data->config.height = gdk_window_get_height(gtk_widget_get_window (data->app));

        gtk_window_get_position(GTK_WINDOW(data->app), &data->config.xpos, &data->config.ypos);

        g_settings_set_int (settings, "width", data->config.width);
        g_settings_set_int (settings, "height", data->config.height);
        g_settings_set_int (settings, "x-position", data->config.xpos);
        g_settings_set_int (settings, "y-position", data->config.ypos);
    }
    g_settings_set_boolean (settings, "maximized", data->config.maximized);

    g_settings_set_int (settings, "current-tab", data->config.current_tab);

    g_settings_sync ();
}


int
main (int argc, char *argv[])
{
    bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
    setlocale (LC_ALL, "");

    Glib::RefPtr<ProcmanApp> application = ProcmanApp::create();
    return application->run (argc, argv);
}

