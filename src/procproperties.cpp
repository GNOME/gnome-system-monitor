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
#if defined (__linux__)
#include <asm/param.h>
#elif defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#include "application.h"
#include "procinfo.h"
#include "procproperties.h"
#include "proctable.h"
#include "util.h"

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
fill_proc_properties (GtkWidget *tree, ProcInfo *info)
{
    guint i;
    GtkListStore *store;
    
    if (!info)
        return;

    get_process_memory_writable (info);

#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    struct clockinfo cinf;
    size_t size = sizeof (cinf);
    int HZ;
    int mib[] = { CTL_KERN, KERN_CLOCKRATE };

    if (sysctl (mib, G_N_ELEMENTS (mib), &cinf, &size, NULL, 0) == -1)
        HZ = 100;
    else
        HZ = (cinf.stathz ? cinf.stathz : cinf.hz);
#endif
#ifdef __GNU__
    int HZ;
    HZ = sysconf(_SC_CLK_TCK);
    if (HZ < 0)
        HZ = 100;
#endif
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
        { N_("CPU Time"), g_strdup_printf(ngettext("%lld second", "%lld seconds", info->cpu_time/HZ), (unsigned long long)info->cpu_time/HZ) },
        { N_("Started"), g_strdup_printf("%s", ctime((const time_t*)(&info->start_time)))},
        { N_("Nice"), g_strdup_printf("%d", info->nice)},
        { N_("Priority"), g_strdup_printf("%s", procman::get_nice_level(info->nice)) },
        { N_("ID"), g_strdup_printf("%d", info->pid)},
        { N_("Security Context"), info->security_context?g_strdup_printf("%s", info->security_context):g_strdup(_("N/A"))},
        { N_("Command Line"), g_strdup_printf("%s", info->arguments)},
        { N_("Waiting Channel"), g_strdup_printf("%s", info->wchan)},
        { N_("Control Group"), info->cgroup_name?g_strdup_printf("%s", info->cgroup_name):g_strdup(_("N/A"))},
        { NULL, NULL}
    };

    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
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
update_procproperties_dialog (GtkWidget *tree)
{
    ProcInfo *info;

    pid_t pid = GPOINTER_TO_UINT(static_cast<pid_t*>(g_object_get_data (G_OBJECT (tree), "selected_info")));
    info = ProcInfo::find(pid);

    fill_proc_properties(tree, info);
}

static void
close_procprop_dialog (GtkDialog *dialog, gint id, gpointer data)
{
    GtkWidget *tree = static_cast<GtkWidget*>(data);
    guint timer;

    timer = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (tree), "timer"));
    g_source_remove (timer);

    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static GtkWidget *
create_procproperties_tree (GsmApplication *app, ProcInfo *info)
{
    GtkWidget *tree;
    GtkListStore *model;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    gint i;

    model = gtk_list_store_new (NUM_COLS,
                                G_TYPE_STRING,	/* Property */
                                G_TYPE_STRING	/* Value */
        );

    tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);
    g_object_unref (G_OBJECT (model));

    for (i = 0; i < NUM_COLS; i++) {
        cell = gtk_cell_renderer_text_new ();

        column = gtk_tree_view_column_new_with_attributes (NULL,
                                                           cell,
                                                           "text", i,
                                                           NULL);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
    }

    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(tree), FALSE);
    fill_proc_properties(tree, info);

    return tree;
}

static gboolean
procprop_timer (gpointer data)
{
    GtkWidget *tree = static_cast<GtkWidget*>(data);
    GtkTreeModel *model;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
    g_assert(model);

    update_procproperties_dialog (tree);

    return TRUE;
}

static void
create_single_procproperties_dialog (GtkTreeModel *model, GtkTreePath *path,
                                     GtkTreeIter *iter, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);

    GtkWidget *procpropdialog;
    GtkWidget *dialog_vbox, *vbox;
    GtkWidget *cmd_hbox;
    gchar *label;
    GtkWidget *scrolled;
    GtkWidget *tree;
    ProcInfo *info;
    guint timer;

    gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

    if (!info)
        return;

    procpropdialog = GTK_WIDGET (g_object_new (GTK_TYPE_DIALOG, 
                                               "use-header-bar", TRUE, NULL));

    label = g_strdup_printf( _("%s (PID %u)"), info->name, info->pid);
    gtk_window_set_title (GTK_WINDOW (procpropdialog), label);
    g_free (label);

    gtk_window_set_destroy_with_parent (GTK_WINDOW (procpropdialog), TRUE);

    gtk_window_set_resizable (GTK_WINDOW (procpropdialog), TRUE);
    gtk_window_set_default_size (GTK_WINDOW (procpropdialog), 575, 400);
    gtk_container_set_border_width (GTK_CONTAINER (procpropdialog), 5);

    vbox = gtk_dialog_get_content_area (GTK_DIALOG (procpropdialog));
    gtk_box_set_spacing (GTK_BOX (vbox), 2);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

    dialog_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 5);
    gtk_box_pack_start (GTK_BOX (vbox), dialog_vbox, TRUE, TRUE, 0);

    cmd_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), cmd_hbox, FALSE, FALSE, 0);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                         GTK_SHADOW_IN);

    tree = create_procproperties_tree (app, info);
    gtk_container_add (GTK_CONTAINER (scrolled), tree);
    g_object_set_data (G_OBJECT (tree), "selected_info", GUINT_TO_POINTER (info->pid));

    gtk_box_pack_start (GTK_BOX (dialog_vbox), scrolled, TRUE, TRUE, 0);
    gtk_widget_show_all (scrolled);

    g_signal_connect (G_OBJECT (procpropdialog), "response",
                      G_CALLBACK (close_procprop_dialog), tree);

    gtk_widget_show_all (procpropdialog);

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
