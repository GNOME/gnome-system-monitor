/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* Procman tree view and process updating
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
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <config.h>


#include <string.h>
#include <math.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glibtop.h>
#include <glibtop/proclist.h>
#include <time.h>

#include <set>
#include <list>

#include "application.h"
#include "procinfo.h"
#include "proctable.h"
#include "prettytable.h"
#include "util.h"
#include "interface.h"
#include "selinux.h"
#include "settings-keys.h"
#include "cgroups.h"
#include "treeview.h"


static void
cb_save_tree_state(gpointer, gpointer data)
{
    GsmApplication * const app = static_cast<GsmApplication *>(data);

    gsm_tree_view_save_state (GSM_TREE_VIEW (app->tree));
}

static void
cb_proctree_destroying (GtkTreeView *self, gpointer data)
{
    g_signal_handlers_disconnect_by_func (self,
                                          (gpointer) cb_save_tree_state,
                                          data);

    g_signal_handlers_disconnect_by_func (gtk_tree_view_get_model (self),
                                          (gpointer) cb_save_tree_state,
                                          data);
}

static gboolean
cb_tree_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    GtkTreePath *path;

    if (!gdk_event_triggers_context_menu ((GdkEvent *) event))
        return FALSE;

    if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (app->tree), event->x, event->y, &path, NULL, NULL, NULL))
        return FALSE;

    if (!gtk_tree_selection_path_is_selected (app->selection, path)) {
        if (!(event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)))
            gtk_tree_selection_unselect_all (app->selection);
        gtk_tree_selection_select_path (app->selection, path);
    }

    gtk_tree_path_free (path);

    gtk_menu_popup (GTK_MENU (app->popup_menu),
                    NULL, NULL, NULL, NULL,
                    event->button, event->time);
    return TRUE;
}

static gboolean
cb_tree_popup_menu (GtkWidget *widget, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    gtk_menu_popup (GTK_MENU (app->popup_menu),
                    NULL, NULL, NULL, NULL,
                    0, gtk_get_current_event_time ());

    return TRUE;
}

void
get_last_selected (GtkTreeModel *model, GtkTreePath *path,
                   GtkTreeIter *iter, gpointer data)
{
    ProcInfo **info = (ProcInfo**) data;

    gtk_tree_model_get (model, iter, COL_POINTER, info, -1);
}

void
cb_row_selected (GtkTreeSelection *selection, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    app->selection = selection;

    ProcInfo *selected_process = NULL;

    /* get the most recent selected process and determine if there are
    ** no selected processes
    */
    gtk_tree_selection_selected_foreach (app->selection, get_last_selected,
                                         &selected_process);
    if (selected_process) {
        GVariant *priority;
        gint nice = selected_process->nice;
        if (nice < -7)
            priority = g_variant_new_int32 (-20);
        else if (nice < -2)
            priority = g_variant_new_int32 (-5);
        else if (nice < 3)
            priority = g_variant_new_int32 (0);
        else if (nice < 7)
            priority = g_variant_new_int32 (5);
        else
            priority = g_variant_new_int32 (19);

        GAction *action = g_action_map_lookup_action (G_ACTION_MAP (app->main_window),
                                                      "priority");

        g_action_change_state (action, priority);
    }
    update_sensitivity(app);
}

static gint
cb_timeout (gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    guint new_interval;

    proctable_update (app);

    if (app->smooth_refresh->get(new_interval)) {
        app->timeout = g_timeout_add(new_interval,
                                     cb_timeout,
                                     app);
        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}


static void
treemodel_update_icon (GsmApplication *app, ProcInfo *info)
{
    GtkTreeModel *model;

    model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (
            gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(
            gtk_tree_view_get_model (GTK_TREE_VIEW(app->tree))))));

    gtk_tree_store_set(GTK_TREE_STORE(model), &info->node,
                       COL_PIXBUF, (info->pixbuf ? info->pixbuf->gobj() : NULL),
                       -1);
}

static void
cb_refresh_icons (GtkIconTheme *theme, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    if(app->timeout) {
        g_source_remove (app->timeout);
    }

    for (ProcInfo::Iterator it(ProcInfo::begin()); it != ProcInfo::end(); ++it) {
        app->pretty_table->set_icon(*(it->second));
        treemodel_update_icon (app, it->second);
    }

    cb_timeout(app);
}

static gboolean
iter_matches_search_key (GtkTreeModel *model, GtkTreeIter *iter, const gchar *key)
{
    char *name;
    char *user;
    gboolean found;

    gtk_tree_model_get (model, iter,
                        COL_NAME, &name,
                        COL_USER, &user,
                        -1);

    found = (name && strcasestr (name, key)) || (user && strcasestr (user, key));

    g_free (name);
    g_free (user);

    return found;
}

static gboolean
process_visibility_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    GsmApplication * const app = static_cast<GsmApplication *>(data);
    const gchar * search_text = app->search_entry == NULL ? "" : gtk_entry_get_text (GTK_ENTRY (app->search_entry));
    GtkTreePath *tree_path = gtk_tree_model_get_path (model, iter);

    if (strcmp (search_text, "") == 0)
        return TRUE;

	// in case we are in dependencies view, we show (and expand) rows not matching the text, but having a matching child
    gboolean match = false;
    if (g_settings_get_boolean (app->settings, GSM_SETTING_SHOW_DEPENDENCIES)) {
        GtkTreeIter child;
        if (gtk_tree_model_iter_children (model, &child, iter)) {
            gboolean child_match = FALSE;
            do {
                child_match = process_visibility_func (model, &child, data);
            } while (gtk_tree_model_iter_next (model, &child) && !child_match);
            match = child_match;
        }

        match |= iter_matches_search_key (model, iter, search_text);
        // TODO auto-expand items not matching the search string but having matching children
        // complicated because of treestore nested in treemodelfilter nested in treemodelsort
        // expand to path requires the path string in the treemodelsort, but tree_path is the path in the double nested treestore
        //if (match && (strlen (search_text) > 0)) {
        //    gtk_tree_view_expand_to_path (GTK_TREE_VIEW (app->tree), tree_path);
        //}

    } else {
        match = iter_matches_search_key (model, iter, search_text);
    }
        
    gtk_tree_path_free (tree_path);

    return match;
}

static void
proctable_clear_tree (GsmApplication * const app)
{
    GtkTreeModel *model;

    model =  gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (
             gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT (
             gtk_tree_view_get_model (GTK_TREE_VIEW(app->tree))))));

    gtk_tree_store_clear (GTK_TREE_STORE (model));

    proctable_free_table (app);

    update_sensitivity(app);
}
                                                        
static void
cb_show_dependencies_changed (GSettings *settings, const gchar *key, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    if (app->timeout) {
        gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (app->tree),
                                      g_settings_get_boolean (settings, GSM_SETTING_SHOW_DEPENDENCIES));

        proctable_clear_tree (app);
        proctable_update (app);
    }
}

static void
cb_show_whose_processes_changed (GSettings *settings, const gchar *key, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    if (app->timeout) {
        proctable_clear_tree (app);
        proctable_update (app);
    }
}

GtkWidget *
proctable_new (GsmApplication * const app)
{
    GtkWidget *proctree;
    GtkTreeStore *model;
    GtkTreeModelFilter *model_filter;
    GtkTreeModelSort *model_sort;
    
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell_renderer;
    const gchar *titles[] = {
        N_("Process Name"),
        N_("User"),
        N_("Status"),
        N_("Virtual Memory"),
        N_("Resident Memory"),
        N_("Writable Memory"),
        N_("Shared Memory"),
        N_("X Server Memory"),
        /* xgettext:no-c-format */ N_("% CPU"),
        N_("CPU Time"),
        N_("Started"),
        N_("Nice"),
        N_("ID"),
        N_("Security Context"),
        N_("Command Line"),
        N_("Memory"),
        /* xgettext: combined noun, the function the process is waiting in, see wchan ps(1) */
        N_("Waiting Channel"),
        N_("Control Group"),
        N_("Unit"),
        N_("Session"),
        /* TRANSLATORS: Seat = i.e. the physical seat the session of the process belongs to, only
	for multi-seat environments. See http://en.wikipedia.org/wiki/Multiseat_configuration */
        N_("Seat"),
        N_("Owner"),
        N_("Priority"),
        NULL,
        "POINTER"
    };

    gint i;
    GSettings * settings = g_settings_get_child (app->settings, GSM_SETTINGS_CHILD_PROCESSES);
    model = gtk_tree_store_new (NUM_COLUMNS,
                                G_TYPE_STRING,      /* Process Name */
                                G_TYPE_STRING,      /* User         */
                                G_TYPE_UINT,        /* Status       */
                                G_TYPE_ULONG,       /* VM Size      */
                                G_TYPE_ULONG,       /* Resident Memory */
                                G_TYPE_ULONG,       /* Writable Memory */
                                G_TYPE_ULONG,       /* Shared Memory */
                                G_TYPE_ULONG,       /* X Server Memory */
                                G_TYPE_UINT,        /* % CPU        */
                                G_TYPE_UINT64,      /* CPU time     */
                                G_TYPE_ULONG,       /* Started      */
                                G_TYPE_INT,         /* Nice         */
                                G_TYPE_UINT,        /* ID           */
                                G_TYPE_STRING,      /* Security Context */
                                G_TYPE_STRING,      /* Arguments    */
                                G_TYPE_ULONG,       /* Memory       */
                                G_TYPE_STRING,      /* wchan        */
                                G_TYPE_STRING,      /* Cgroup       */
                                G_TYPE_STRING,      /* Unit         */
                                G_TYPE_STRING,      /* Session      */
                                G_TYPE_STRING,      /* Seat         */
                                G_TYPE_STRING,      /* Owner        */
                                G_TYPE_STRING,      /* Priority     */
                                GDK_TYPE_PIXBUF,    /* Icon         */
                                G_TYPE_POINTER,     /* ProcInfo     */
                                G_TYPE_STRING       /* Sexy tooltip */
        );

    model_filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (model), NULL));
        
    gtk_tree_model_filter_set_visible_func(model_filter, process_visibility_func, app, NULL);
    
    model_sort = GTK_TREE_MODEL_SORT (gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (model_filter)));
    
    proctree = gsm_tree_view_new (settings, TRUE);
    gtk_tree_view_set_model (GTK_TREE_VIEW (proctree), GTK_TREE_MODEL (model_sort));

    gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (proctree), COL_TOOLTIP);
    gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (proctree),
                                      g_settings_get_boolean (app->settings, GSM_SETTING_SHOW_DEPENDENCIES));
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (proctree), FALSE);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (proctree), TRUE);
    g_object_unref (G_OBJECT (model));

    column = gtk_tree_view_column_new ();

    cell_renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, cell_renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, cell_renderer,
                                         "pixbuf", COL_PIXBUF,
                                         NULL);

    cell_renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, cell_renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, cell_renderer,
                                         "text", COL_NAME,
                                         NULL);
    gtk_tree_view_column_set_title (column, _(titles[0]));

    gtk_tree_view_column_set_sort_column_id (column, COL_NAME);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_min_width (column, 1);
    gtk_tree_view_column_set_reorderable(column, TRUE);

    gsm_tree_view_append_and_bind_column (GSM_TREE_VIEW (proctree), column);
    gtk_tree_view_set_expander_column (GTK_TREE_VIEW (proctree), column);

    for (i = COL_USER; i <= COL_PRIORITY; i++) {
        GtkTreeViewColumn *col;
        GtkCellRenderer *cell;

#ifndef HAVE_WNCK
        if (i == COL_MEMXSERVER)
          continue;
#endif

        if (i == COL_MEMWRITABLE)
            continue;

        cell = gtk_cell_renderer_text_new();
        col = gtk_tree_view_column_new();
        gtk_tree_view_column_pack_start(col, cell, TRUE);
        gtk_tree_view_column_set_title(col, _(titles[i]));
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_sort_column_id(col, i);
        gtk_tree_view_column_set_reorderable(col, TRUE);
        gsm_tree_view_append_and_bind_column (GSM_TREE_VIEW (proctree), col);

        // type
        switch (i) {
#ifdef HAVE_WNCK
            case COL_MEMXSERVER:
                gtk_tree_view_column_set_cell_data_func(col, cell,
                                                        &procman::size_cell_data_func,
                                                        GUINT_TO_POINTER(i),
                                                        NULL);
                break;
#endif
            case COL_VMSIZE:
            case COL_MEMRES:
            case COL_MEMSHARED:
            case COL_MEM:
                gtk_tree_view_column_set_cell_data_func(col, cell,
                                                        &procman::size_na_cell_data_func,
                                                        GUINT_TO_POINTER(i),
                                                        NULL);
                break;

            case COL_CPU_TIME:
                gtk_tree_view_column_set_cell_data_func(col, cell,
                                                        &procman::duration_cell_data_func,
                                                        GUINT_TO_POINTER(i),
                                                        NULL);
                break;

            case COL_START_TIME:
                gtk_tree_view_column_set_cell_data_func(col, cell,
                                                        &procman::time_cell_data_func,
                                                        GUINT_TO_POINTER(i),
                                                        NULL);
                break;

            case COL_STATUS:
                gtk_tree_view_column_set_cell_data_func(col, cell,
                                                        &procman::status_cell_data_func,
                                                        GUINT_TO_POINTER(i),
                                                        NULL);
                break;
            case COL_PRIORITY:
                gtk_tree_view_column_set_cell_data_func(col, cell,
                                                        &procman::priority_cell_data_func,
                                                        GUINT_TO_POINTER(COL_NICE),
                                                        NULL);
                break;
            default:
                gtk_tree_view_column_set_attributes(col, cell, "text", i, NULL);
                break;
        }

        // sorting
        switch (i) {
#ifdef HAVE_WNCK
            case COL_MEMXSERVER:
#endif
            case COL_VMSIZE:
            case COL_MEMRES:
            case COL_MEMSHARED:
            case COL_MEM:
            case COL_CPU:
            case COL_CPU_TIME:
            case COL_START_TIME:
                gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model_sort), i,
                                                 procman::number_compare_func,
                                                 GUINT_TO_POINTER (i),
                                                 NULL);
                break;
            case COL_PRIORITY:
                gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model_sort), i,
                                                 procman::priority_compare_func,
                                                 GUINT_TO_POINTER (COL_NICE), NULL);
                break;
            default:
                break;
        }

        // xalign
        switch(i)
        {
            case COL_VMSIZE:
            case COL_MEMRES:
            case COL_MEMSHARED:
#ifdef HAVE_WNCK
            case COL_MEMXSERVER:
#endif
            case COL_CPU:
            case COL_NICE:
            case COL_PID:
            case COL_CPU_TIME:
            case COL_MEM:
                g_object_set(G_OBJECT(cell), "xalign", 1.0f, NULL);
                break;
        }

        gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
        // sizing
        switch (i) {
            case COL_ARGS:
                gtk_tree_view_column_set_min_width(col, 150);
                break;
            default:
                gtk_tree_view_column_set_min_width(column, 20);
                break;
        }
    }
    app->tree = proctree;
    app->top_of_tree = NULL;
    app->last_vscroll_max = 0;
    app->last_vscroll_value = 0;

    if (!cgroups_enabled ())
        gsm_tree_view_add_excluded_column (GSM_TREE_VIEW (proctree), COL_CGROUP);

#ifdef HAVE_SYSTEMD
    if (!LOGIND_RUNNING ())
#endif
    {
        gsm_tree_view_add_excluded_column (GSM_TREE_VIEW (proctree), COL_UNIT);
        gsm_tree_view_add_excluded_column (GSM_TREE_VIEW (proctree), COL_SESSION);
        gsm_tree_view_add_excluded_column (GSM_TREE_VIEW (proctree), COL_SEAT);
        gsm_tree_view_add_excluded_column (GSM_TREE_VIEW (proctree), COL_OWNER);
    }

    if (!can_show_security_context_column ())
        gsm_tree_view_add_excluded_column (GSM_TREE_VIEW (proctree), COL_SECURITYCONTEXT);

    gsm_tree_view_load_state (GSM_TREE_VIEW (proctree));

    GtkIconTheme* theme = gtk_icon_theme_get_default();
    g_signal_connect(G_OBJECT (theme), "changed", G_CALLBACK (cb_refresh_icons), app);

    app->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (proctree));
    gtk_tree_selection_set_mode (app->selection, GTK_SELECTION_MULTIPLE);
    
    g_signal_connect (G_OBJECT (app->selection),
                      "changed",
                      G_CALLBACK (cb_row_selected), app);
    g_signal_connect (G_OBJECT (proctree), "popup_menu",
                      G_CALLBACK (cb_tree_popup_menu), app);
    g_signal_connect (G_OBJECT (proctree), "button_press_event",
                      G_CALLBACK (cb_tree_button_pressed), app);

    g_signal_connect (G_OBJECT (proctree), "destroy",
                      G_CALLBACK (cb_proctree_destroying),
                      app);

    g_signal_connect (G_OBJECT (proctree), "columns-changed",
                      G_CALLBACK (cb_save_tree_state), app);

    g_signal_connect (G_OBJECT (model_sort), "sort-column-changed",
                      G_CALLBACK (cb_save_tree_state), app);

    g_signal_connect (app->settings, "changed::" GSM_SETTING_SHOW_DEPENDENCIES,
                      G_CALLBACK (cb_show_dependencies_changed), app);

    g_signal_connect (app->settings, "changed::" GSM_SETTING_SHOW_WHOSE_PROCESSES,
                      G_CALLBACK (cb_show_whose_processes_changed), app);

    gtk_widget_show (proctree);

    return proctree;
}

static void
update_info_mutable_cols(ProcInfo *info)
{
    GtkTreeModel *model;
    model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (
            gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(
            gtk_tree_view_get_model (GTK_TREE_VIEW(GsmApplication::get()->tree))))));

    using procman::tree_store_update;

    tree_store_update(model, &info->node, COL_STATUS, info->status);
    tree_store_update(model, &info->node, COL_USER, info->user.c_str());
    tree_store_update(model, &info->node, COL_VMSIZE, info->vmsize);
    tree_store_update(model, &info->node, COL_MEMRES, info->memres);
    tree_store_update(model, &info->node, COL_MEMSHARED, info->memshared);
#ifdef HAVE_WNCK
    tree_store_update(model, &info->node, COL_MEMXSERVER, info->memxserver);
#endif
    tree_store_update(model, &info->node, COL_CPU, info->pcpu);
    tree_store_update(model, &info->node, COL_CPU_TIME, info->cpu_time);
    tree_store_update(model, &info->node, COL_START_TIME, info->start_time);
    tree_store_update(model, &info->node, COL_NICE, info->nice);
    tree_store_update(model, &info->node, COL_MEM, info->mem);
    tree_store_update(model, &info->node, COL_WCHAN, info->wchan);
    tree_store_update(model, &info->node, COL_CGROUP, info->cgroup_name);
    tree_store_update(model, &info->node, COL_UNIT, info->unit);
    tree_store_update(model, &info->node, COL_SESSION, info->session);
    tree_store_update(model, &info->node, COL_SEAT, info->seat);
    tree_store_update(model, &info->node, COL_OWNER, info->owner.c_str());
}

static void
insert_info_to_tree (ProcInfo *info, GsmApplication *app, bool forced = false)
{
    GtkTreeModel *model;
    GtkTreeModel *filtered;
    GtkTreeModel *sorted;
    sorted = gtk_tree_view_get_model (GTK_TREE_VIEW(app->tree));
    filtered = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(sorted));
    model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filtered));
    
    if (g_settings_get_boolean (app->settings, GSM_SETTING_SHOW_DEPENDENCIES)) {

        ProcInfo *parent = 0;

        if (not forced)
            parent = ProcInfo::find(info->ppid);

        if (parent) {
            GtkTreePath *parent_node = gtk_tree_model_get_path(model, &parent->node);
            gtk_tree_store_insert(GTK_TREE_STORE(model), &info->node, &parent->node, 0);
            
            GtkTreePath *filtered_parent = gtk_tree_model_filter_convert_child_path_to_path (GTK_TREE_MODEL_FILTER (filtered), parent_node);
            if (filtered_parent != NULL) {
                GtkTreePath *sorted_parent = gtk_tree_model_sort_convert_child_path_to_path (GTK_TREE_MODEL_SORT (sorted), filtered_parent);
            
                if (sorted_parent != NULL) {
                    if (!gtk_tree_view_row_expanded(GTK_TREE_VIEW(app->tree), sorted_parent)
#ifdef __linux__
                        // on linuxes we don't want to expand kthreadd by default (always has pid 2)
                        && (parent->pid != 2)
#endif
                    )
                        gtk_tree_view_expand_row(GTK_TREE_VIEW(app->tree), sorted_parent, FALSE);
                    gtk_tree_path_free (sorted_parent);
                }
                gtk_tree_path_free (filtered_parent);
            }
            gtk_tree_path_free (parent_node);
        } else
            gtk_tree_store_insert(GTK_TREE_STORE(model), &info->node, NULL, 0);
    }
    else
        gtk_tree_store_insert (GTK_TREE_STORE (model), &info->node, NULL, 0);

    gtk_tree_store_set (GTK_TREE_STORE (model), &info->node,
                        COL_POINTER, info,
                        COL_NAME, info->name,
                        COL_ARGS, info->arguments,
                        COL_TOOLTIP, info->tooltip,
                        COL_PID, info->pid,
                        COL_SECURITYCONTEXT, info->security_context,
                        -1);

    app->pretty_table->set_icon(*info);
    treemodel_update_icon (app, info);

    procman_debug("inserted %d%s", info->pid, (forced ? " (forced)" : ""));
}

/* Removing a node with children - make sure the children are queued
** to be readded.
*/
template<typename List>
static void
remove_info_from_tree (GsmApplication *app, GtkTreeModel *model,
                       ProcInfo *current, List &orphans, unsigned lvl = 0)
{
    GtkTreeIter child_node;

    if (std::find(orphans.begin(), orphans.end(), current) != orphans.end()) {
        procman_debug("[%u] %d already removed from tree", lvl, int(current->pid));
        return;
    }

    procman_debug("[%u] pid %d, %d children", lvl, int(current->pid),
                  gtk_tree_model_iter_n_children(model, &current->node));

    // it is not possible to iterate&erase over a treeview so instead we
    // just pop one child after another and recursively remove it and
    // its children

    while (gtk_tree_model_iter_children(model, &child_node, &current->node)) {
        ProcInfo *child = 0;
        gtk_tree_model_get(model, &child_node, COL_POINTER, &child, -1);
        remove_info_from_tree(app, model, child, orphans, lvl + 1);
    }

    g_assert(not gtk_tree_model_iter_has_child(model, &current->node));

    orphans.push_back(current);
    gtk_tree_store_remove(GTK_TREE_STORE(model), &current->node);
    procman::poison(current->node, 0x69);
}

static void
refresh_list (GsmApplication *app, const pid_t* pid_list, const guint n)
{
    typedef std::list<ProcInfo*> ProcList;
    ProcList addition;

    GtkTreeModel    *model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (
                             gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT (
                             gtk_tree_view_get_model (GTK_TREE_VIEW(app->tree))))));
    guint i;

    // Add or update processes in the process list
    for(i = 0; i < n; ++i) {
        ProcInfo *info = ProcInfo::find(pid_list[i]);

        if (!info) {
            info = new ProcInfo(pid_list[i]);
            ProcInfo::all[info->pid] = info;
            addition.push_back(info);
        }

        info->update (app);
    }


    // Remove dead processes from the process list and from the
    // tree. children are queued to be readded at the right place
    // in the tree.

    const std::set<pid_t> pids(pid_list, pid_list + n);

    ProcInfo::Iterator it(ProcInfo::begin());

    while (it != ProcInfo::end()) {
        ProcInfo * const info = it->second;
        ProcInfo::Iterator next(it);
        ++next;

        if (pids.find(info->pid) == pids.end()) {
            procman_debug("ripping %d", info->pid);
            remove_info_from_tree(app, model, info, addition);
            addition.remove(info);
            ProcInfo::all.erase(it);
            delete info;
        }

        it = next;
    }

    // INVARIANT
    // pid_list == ProcInfo::all + addition


    if (g_settings_get_boolean (app->settings, GSM_SETTING_SHOW_DEPENDENCIES)) {

        // insert process in the tree. walk through the addition list
        // (new process + process that have a new parent). This loop
        // handles the dependencies because we cannot insert a process
        // until its parent is in the tree.

        std::set<pid_t> in_tree(pids);

        for (ProcList::iterator it(addition.begin()); it != addition.end(); ++it)
            in_tree.erase((*it)->pid);


        while (not addition.empty()) {
            procman_debug("looking for %d parents", int(addition.size()));
            ProcList::iterator it(addition.begin());

            while (it != addition.end()) {
                procman_debug("looking for %d's parent with ppid %d",
                              int((*it)->pid), int((*it)->ppid));


                // inserts the process in the treeview if :
                // - it is init
                // - its parent is already in tree
                // - its parent is unreachable
                //
                // rounds == 2 means that addition contains processes with
                // unreachable parents
                //
                // FIXME: this is broken if the unreachable parent becomes active
                // i.e. it gets active or changes ower
                // so we just clear the tree on __each__ update
                // see proctable_update (ProcData * const procdata)


                if ((*it)->ppid == 0 or in_tree.find((*it)->ppid) != in_tree.end()) {
                    insert_info_to_tree(*it, app);
                    in_tree.insert((*it)->pid);
                    it = addition.erase(it);
                    continue;
                }

                ProcInfo *parent = ProcInfo::find((*it)->ppid);
                // if the parent is unreachable
                if (not parent) {
                    // or std::find(addition.begin(), addition.end(), parent) == addition.end()) {
                    insert_info_to_tree(*it, app, true);
                    in_tree.insert((*it)->pid);
                    it = addition.erase(it);
                    continue;
                }

                ++it;
            }
        }
    }
    else {
        // don't care of the tree
        for (ProcList::iterator it(addition.begin()); it != addition.end(); ++it)
            insert_info_to_tree(*it, app);
    }


    for (ProcInfo::Iterator it(ProcInfo::begin()); it != ProcInfo::end(); ++it)
        update_info_mutable_cols(it->second);
}

void
proctable_update (GsmApplication *app)
{
    pid_t* pid_list;
    glibtop_proclist proclist;
    glibtop_cpu cpu;
    int which = 0;
    int arg = 0;
    char *whose_processes = g_settings_get_string (app->settings, GSM_SETTING_SHOW_WHOSE_PROCESSES);
    if (strcmp (whose_processes, "all") == 0) {
        which = GLIBTOP_KERN_PROC_ALL;
        arg = 0;
    } else if (strcmp (whose_processes, "active") == 0) {
        which = GLIBTOP_KERN_PROC_ALL | GLIBTOP_EXCLUDE_IDLE;
        arg = 0;
    } else if (strcmp (whose_processes, "user") == 0) {
      which = GLIBTOP_KERN_PROC_UID;
      arg = getuid ();
    }
    g_free (whose_processes);

    pid_list = glibtop_get_proclist (&proclist, which, arg);

    /* FIXME: total cpu time elapsed should be calculated on an individual basis here
    ** should probably have a total_time_last gint in the ProcInfo structure */
    glibtop_get_cpu (&cpu);
    app->cpu_total_time = MAX(cpu.total - app->cpu_total_time_last, 1);
    app->cpu_total_time_last = cpu.total;
    
    refresh_list (app, pid_list, proclist.number);

    // juggling with tree scroll position to fix https://bugzilla.gnome.org/show_bug.cgi?id=92724
    GtkTreePath* current_top;
    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app->tree), 0,0, &current_top, NULL, NULL, NULL)) {
        GtkAdjustment *vadjustment = GTK_ADJUSTMENT (gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (app->tree)));
        gdouble current_max = gtk_adjustment_get_upper(vadjustment);
        gdouble current_value = gtk_adjustment_get_value(vadjustment);

        if (app->top_of_tree) {
            // if the visible cell from the top of the tree is still the same, as last time 
            if (gtk_tree_path_compare (app->top_of_tree, current_top) == 0) {
                //but something from the scroll parameters has changed compared to the last values
                if (app->last_vscroll_value == 0 && current_value != 0) {
                    // the tree was scrolled to top, and something has been added above the current top row
                    gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(app->tree), -1, 0);
                } else if (current_max > app->last_vscroll_max && app->last_vscroll_max == app->last_vscroll_value) {
                    // the tree was scrolled to bottom, something has been added below the current bottom row
                    gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(app->tree), -1, current_max);
                }
            }

            gtk_tree_path_free(app->top_of_tree);
        }

        app->top_of_tree = current_top;
        app->last_vscroll_value = current_value;
        app->last_vscroll_max = current_max;
    }

    g_free (pid_list);

    /* proclist.number == g_list_length(procdata->info) == g_hash_table_size(procdata->pids) */
}

void
proctable_free_table (GsmApplication * const app)
{
    for (ProcInfo::Iterator it(ProcInfo::begin()); it != ProcInfo::end(); ++it)
        delete it->second;

    ProcInfo::all.clear();
}

void
proctable_freeze (GsmApplication *app)
{
    if (app->timeout) {
      g_source_remove (app->timeout);
      app->timeout = 0;
    }
}

void
proctable_thaw (GsmApplication *app)
{
    if (app->timeout)
        return;

    app->timeout = g_timeout_add (app->config.update_interval,
                                  cb_timeout,
                                  app);
}

void
proctable_reset_timeout (GsmApplication *app)
{
    proctable_freeze (app);
    proctable_thaw (app);
}
