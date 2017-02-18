/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* Process properties dialog
 * Copyright (C) 2010 Krishnan Parthasarathi <krishnan.parthasarathi@gmail.com>
 *                    Robert Ancell <robert.ancell@canonical.com>
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
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

#include <glib/gi18n.h>
#include <glibtop/procmem.h>
#include <glibtop/procmap.h>
#include <glibtop/procstate.h>

#include "application.h"
#include "procproperties.h"
#include "proctable.h"
#include "util.h"
#include "legacy/e_date.h"

enum
{
    COL_PROP = 0,
    COL_VAL,
    NUM_COLS,
};

typedef struct _proc_arg {
    const gchar *prop;
    gchar *val;
} proc_arg;

static gchar*
format_memsize(guint64 size)
{
    if (size == 0)
        return g_strdup(_("N/A"));
    else
        return g_format_size_full(size, G_FORMAT_SIZE_IEC_UNITS);
}

static void
fill_proc_properties (GtkTreeView *tree, ProcInfo *info)
{
    guint i;
    GtkListStore *store;
    
    if (!info)
        return;

    get_process_memory_writable (info);

    proc_arg proc_props[] = {
        { N_("Process Name"), g_strdup_printf("%s", info->name)},
        { N_("User"), g_strdup_printf("%s (%d)", info->user.c_str(), info->uid)},
        { N_("Status"), g_strdup(format_process_state(info->status))},
        { N_("Memory"), format_memsize(info->mem)},
        { N_("Virtual Memory"), format_memsize(info->vmsize)},
        { N_("Resident Memory"), format_memsize(info->memres)},
        { N_("Writable Memory"), format_memsize(info->memwritable)},
        { N_("Shared Memory"), format_memsize(info->memshared)},
#ifdef HAVE_WNCK
        { N_("X Server Memory"), format_memsize(info->memxserver)},
#endif
        { N_("CPU"), g_strdup_printf("%d%%", info->pcpu)},
        { N_("CPU Time"), procman::format_duration_for_display(100 * info->cpu_time / GsmApplication::get()->frequency) },
        { N_("Started"), procman_format_date_for_display(info->start_time) },
        { N_("Nice"), g_strdup_printf("%d", info->nice)},
        { N_("Priority"), g_strdup_printf("%s", procman::get_nice_level(info->nice)) },
        { N_("ID"), g_strdup_printf("%d", info->pid)},
        { N_("Security Context"), info->security_context?g_strdup_printf("%s", info->security_context):g_strdup(_("N/A"))},
        { N_("Command Line"), g_strdup_printf("%s", info->arguments)},
        { N_("Waiting Channel"), g_strdup_printf("%s", info->wchan)},
        { N_("Control Group"), info->cgroup_name?g_strdup_printf("%s", info->cgroup_name):g_strdup(_("N/A"))},
        { NULL, NULL}
    };

    store = GTK_LIST_STORE(gtk_tree_view_get_model(tree));
    for (i = 0; proc_props[i].prop; i++) {
        GtkTreeIter iter;

        if (!gtk_tree_model_iter_nth_child (GTK_TREE_MODEL(store), &iter, NULL, i)) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COL_PROP, gettext(proc_props[i].prop), -1);
        }

        gtk_list_store_set(store, &iter, COL_VAL, g_strstrip(proc_props[i].val), -1);
        g_free(proc_props[i].val);
    }
}

static void
update_procproperties_dialog (GtkTreeView *tree)
{
    ProcInfo *info;

    pid_t pid = GPOINTER_TO_UINT(static_cast<pid_t*>(g_object_get_data (G_OBJECT (tree), "selected_info")));
    info = ProcInfo::find(pid);

    fill_proc_properties(tree, info);
}

static void
close_procprop_dialog (GtkDialog *dialog, gint id, gpointer data)
{
    GtkTreeView *tree = static_cast<GtkTreeView*>(data);
    guint timer;

    timer = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (tree), "timer"));
    g_source_remove (timer);

    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static GtkTreeView *
create_procproperties_tree (GsmApplication *app, ProcInfo *info)
{
    GtkTreeView *tree;
    GtkListStore *model;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    gint i;

    model = gtk_list_store_new (NUM_COLS,
                                G_TYPE_STRING,	/* Property */
                                G_TYPE_STRING	/* Value */
        );

    tree = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (model)));
    g_object_unref (G_OBJECT (model));

    for (i = 0; i < NUM_COLS; i++) {
        cell = gtk_cell_renderer_text_new ();

        column = gtk_tree_view_column_new_with_attributes (NULL,
                                                           cell,
                                                           "text", i,
                                                           NULL);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (tree, column);
    }

    gtk_tree_view_set_headers_visible (tree, FALSE);
    fill_proc_properties(tree, info);

    return tree;
}

static gboolean
procprop_timer (gpointer data)
{
    GtkTreeView *tree = static_cast<GtkTreeView*>(data);
    GtkTreeModel *model;

    model = gtk_tree_view_get_model (tree);
    g_assert(model);

    update_procproperties_dialog (tree);

    return TRUE;
}

static void
create_single_procproperties_dialog (GtkTreeModel *model, GtkTreePath *path,
                                     GtkTreeIter *iter, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);

    GtkDialog *procpropdialog;
    GtkBox *dialog_vbox, *vbox;
    GtkBox *cmd_hbox;
    gchar *label;
    GtkScrolledWindow *scrolled;
    GtkTreeView *tree;
    ProcInfo *info;
    guint timer;

    gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

    if (!info)
        return;

    procpropdialog = GTK_DIALOG (g_object_new (GTK_TYPE_DIALOG, 
                                               "use-header-bar", TRUE, NULL));

    label = g_strdup_printf( _("%s (PID %u)"), info->name, info->pid);
    gtk_window_set_title (GTK_WINDOW (procpropdialog), label);
    g_free (label);

    gtk_window_set_destroy_with_parent (GTK_WINDOW (procpropdialog), TRUE);

    gtk_window_set_resizable (GTK_WINDOW (procpropdialog), TRUE);
    gtk_window_set_default_size (GTK_WINDOW (procpropdialog), 575, 400);
    gtk_container_set_border_width (GTK_CONTAINER (procpropdialog), 5);

    vbox = GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (procpropdialog)));
    gtk_box_set_spacing (vbox, 2);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

    dialog_vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
    gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 5);
    gtk_box_pack_start (vbox, GTK_WIDGET (dialog_vbox), TRUE, TRUE, 0);

    cmd_hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12));
    gtk_box_pack_start (dialog_vbox, GTK_WIDGET (cmd_hbox), FALSE, FALSE, 0);

    scrolled = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
    gtk_scrolled_window_set_policy (scrolled,
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (scrolled,
                                         GTK_SHADOW_IN);

    tree = create_procproperties_tree (app, info);
    gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (tree));
    g_object_set_data (G_OBJECT (tree), "selected_info", GUINT_TO_POINTER (info->pid));

    gtk_box_pack_start (dialog_vbox, GTK_WIDGET (scrolled), TRUE, TRUE, 0);
    gtk_widget_show_all (GTK_WIDGET (scrolled));

    g_signal_connect (G_OBJECT (procpropdialog), "response",
                      G_CALLBACK (close_procprop_dialog), tree);

    gtk_window_set_transient_for (GTK_WINDOW (procpropdialog), GTK_WINDOW (GsmApplication::get()->main_window));
    gtk_widget_show_all (GTK_WIDGET (procpropdialog));

    timer = g_timeout_add_seconds (5, procprop_timer, tree);
    g_object_set_data (G_OBJECT (tree), "timer", GUINT_TO_POINTER (timer));

    update_procproperties_dialog (tree);
}

void
create_procproperties_dialog (GsmApplication *app)
{
    gtk_tree_selection_selected_foreach (app->selection, create_single_procproperties_dialog,
                                         app);
}
