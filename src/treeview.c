/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <config.h>

#include "treeview.h"

typedef struct
{
    GSettings  *settings;
    gboolean    store_column_order;
    GHashTable *excluded_columns;
} GsmTreeViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GsmTreeView, gsm_tree_view, GTK_TYPE_TREE_VIEW)

static void
gsm_tree_view_finalize (GObject *object)
{
    GsmTreeViewPrivate *priv = gsm_tree_view_get_instance_private (GSM_TREE_VIEW (object));

    g_hash_table_destroy (priv->excluded_columns);
    priv->excluded_columns = NULL;

    G_OBJECT_CLASS (gsm_tree_view_parent_class)->finalize (object);
}

static void
gsm_tree_view_class_init (GsmTreeViewClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = gsm_tree_view_finalize;
}

static void
gsm_tree_view_init (GsmTreeView *self)
{
    GsmTreeViewPrivate *priv = gsm_tree_view_get_instance_private (self);

    priv->excluded_columns = g_hash_table_new (g_direct_hash, g_direct_equal);
}

void
gsm_tree_view_save_state (GsmTreeView *tree_view)
{
    GsmTreeViewPrivate *priv;

    g_return_if_fail (GSM_IS_TREE_VIEW (tree_view));

    priv = gsm_tree_view_get_instance_private (tree_view);
    GtkTreeModel *model;
    gint sort_col;
    GtkSortType sort_type;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));

    if (gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
                                              &sort_col,
                                              &sort_type)) {
        g_settings_set_int (priv->settings, "sort-col", sort_col);
        g_settings_set_int (priv->settings, "sort-order", sort_type);
    }

    if (priv->store_column_order) {
        GList *columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree_view));
        GList *iter;
        GVariantBuilder builder;

        g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);

        for (iter = columns; iter != NULL; iter = iter->next) {
            gint id = gtk_tree_view_column_get_sort_column_id (GTK_TREE_VIEW_COLUMN (iter->data));
            g_variant_builder_add (&builder, "i", id);
        }

        g_settings_set_value (priv->settings, "columns-order",
                              g_variant_new ("ai", builder));

        g_list_free (columns);
    }
}

GtkTreeViewColumn *
gsm_tree_view_get_column_from_id (GsmTreeView *tree_view, gint sort_id)
{
    GList *columns;
    GList *iter;
    GtkTreeViewColumn *col = NULL;

    g_return_val_if_fail (GSM_IS_TREE_VIEW (tree_view), NULL);

    columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree_view));

    for (iter = columns; iter != NULL; iter = iter->next) {
        col = GTK_TREE_VIEW_COLUMN (iter->data);
        if (gtk_tree_view_column_get_sort_column_id (col) == sort_id)
            break;
    }

    g_list_free (columns);

    return col;
}

static gboolean
cb_column_header_clicked (GtkTreeViewColumn* column, GdkEvent* event, gpointer data)
{
    GtkMenu *menu = GTK_MENU (data);

    if (event->button.button == GDK_BUTTON_SECONDARY) {
        gtk_menu_popup (menu, NULL, NULL,
                        NULL,
                        &(event->button),
                        event->button.button,
                        event->button.time);
        return TRUE;
    }

    return FALSE;
}

void
gsm_tree_view_load_state (GsmTreeView *tree_view)
{
    GsmTreeViewPrivate *priv;
    GtkTreeModel *model;
    gint sort_col;
    GtkSortType sort_type;

    g_return_if_fail (GSM_IS_TREE_VIEW (tree_view));

    priv = gsm_tree_view_get_instance_private (tree_view);
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));

    sort_col = g_settings_get_int (priv->settings, "sort-col");
    sort_type = g_settings_get_int (priv->settings, "sort-order");

    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
                                          sort_col,
                                          sort_type);

    if (priv->store_column_order) {
        GtkWidget *header_menu = gtk_menu_new ();
        GList *columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree_view));
        GList *iter;
        GVariantIter *var_iter;
        GtkTreeViewColumn *col, *last;
        gint sort_id;

        for (iter = columns; iter != NULL; iter = iter->next) {
            const char *title;
            GtkWidget *button;
            GtkWidget *column_item;

            col = GTK_TREE_VIEW_COLUMN (iter->data);
            sort_id = gtk_tree_view_column_get_sort_column_id (col);

            if (priv->excluded_columns &&
                g_hash_table_contains (priv->excluded_columns, GINT_TO_POINTER (sort_id)))
                continue;

            title = gtk_tree_view_column_get_title (col);

            button = gtk_tree_view_column_get_button (col);
            g_signal_connect (button, "button-press-event",
                              G_CALLBACK (cb_column_header_clicked),
                              header_menu);

            column_item = gtk_check_menu_item_new_with_label (title);
            g_object_bind_property (col, "visible",
                                    column_item, "active",
                                    G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

            gtk_menu_shell_append (GTK_MENU_SHELL (header_menu), column_item);
        }

        gtk_widget_show_all (header_menu);

        g_settings_get (priv->settings, "columns-order", "ai", &var_iter);
        last = NULL;
        while (g_variant_iter_loop (var_iter, "i", &sort_id)) {
            col = gsm_tree_view_get_column_from_id (tree_view, sort_id);

            if (col != NULL && col != last) {
                gtk_tree_view_move_column_after (GTK_TREE_VIEW (tree_view),
                                                 col, last);
                last = col;
            }
        }
        g_variant_iter_free (var_iter);
    }
}

void
gsm_tree_view_add_excluded_column (GsmTreeView *tree_view, gint column_id)
{
    GsmTreeViewPrivate *priv;

    g_return_if_fail (GSM_IS_TREE_VIEW (tree_view));

    priv = gsm_tree_view_get_instance_private (tree_view);
    g_hash_table_add (priv->excluded_columns, GINT_TO_POINTER (column_id));
}

void
gsm_tree_view_append_and_bind_column (GsmTreeView *tree_view, GtkTreeViewColumn *column)
{
    GsmTreeViewPrivate *priv;
    gchar *key;
    gint column_id;

    g_return_if_fail (GSM_IS_TREE_VIEW (tree_view));
    g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (column));

    priv = gsm_tree_view_get_instance_private (tree_view);

    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view),
                                 column);

    column_id = gtk_tree_view_column_get_sort_column_id (column);

    key = g_strdup_printf ("col-%d-width", column_id);
    g_settings_bind (priv->settings, key, column, "fixed-width", G_SETTINGS_BIND_DEFAULT);
    g_free (key);

    key = g_strdup_printf ("col-%d-visible", column_id);
    g_settings_bind (priv->settings, key, column, "visible", G_SETTINGS_BIND_DEFAULT);
    g_free (key);
}


GtkWidget *
gsm_tree_view_new (GSettings *settings, gboolean store_column_order)
{
    GsmTreeView *self = g_object_new (GSM_TYPE_TREE_VIEW, NULL);
    GsmTreeViewPrivate *priv = gsm_tree_view_get_instance_private (self);

    priv->settings = settings;
    priv->store_column_order = store_column_order;

    return GTK_WIDGET (self);
}
