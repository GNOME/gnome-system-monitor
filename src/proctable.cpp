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
#include <glibtop/procstate.h>
#include <glibtop/procio.h>
#include <glibtop/procmem.h>
#include <glibtop/procmap.h>
#include <glibtop/proctime.h>
#include <glibtop/procuid.h>
#include <glibtop/procargs.h>
#include <glibtop/prockernel.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>

#include <set>
#include <list>

#include "application.h"
#include "proctable.h"
#include "prettytable.h"
#include "util.h"
#include "interface.h"
#include "procinfo.h"
#include "selinux.h"
#include "settings-keys.h"
#include "cgroups.h"
#include "legacy/treeview.h"
#include "systemd.h"

static void
cb_save_tree_state (gpointer,
                    gpointer data)
{
  GsmApplication * const app = static_cast<GsmApplication *>(data);

  gsm_tree_view_save_state (app->tree);
}

static void
cb_proctree_destroying (GtkTreeView *self,
                        gpointer     data)
{
  g_signal_handlers_disconnect_by_func (self,
                                        (gpointer) cb_save_tree_state,
                                        data);

  g_signal_handlers_disconnect_by_func (gtk_tree_view_get_model (self),
                                        (gpointer) cb_save_tree_state,
                                        data);
}

static gboolean
cb_tree_button_pressed (GtkGestureClick*,
                        gint,
                        gdouble         x,
                        gdouble         y,
                        GsmApplication *app)
{
  GtkTreePath *path = NULL;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (app->tree));
  int bwx, bwy;

  gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (app->tree), x, y, &bwx, &bwy);

  if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (app->tree), bwx, bwy, &path, NULL, NULL, NULL))
    return FALSE;

  if (!gtk_tree_selection_path_is_selected (selection, path))
    gtk_tree_selection_select_path (selection, path);
  gtk_tree_path_free (path);

  GdkRectangle rect = { (gint)x, (gint)y, 1, 1 };

  gtk_popover_set_pointing_to (GTK_POPOVER (app->proc_popover_menu), &rect);
  gtk_popover_popup (GTK_POPOVER (app->proc_popover_menu));

  return TRUE;
}

void
get_last_selected (GtkTreeModel *model,
                   GtkTreePath*,
                   GtkTreeIter  *iter,
                   gpointer      data)
{
  ProcInfo **info = (ProcInfo**) data;

  gtk_tree_model_get (model, iter, COL_POINTER, info, -1);
}

static void
cb_row_selected (GtkTreeSelection *selection,
                 gpointer          data)
{
  GsmApplication *app = (GsmApplication *) data;

  ProcInfo *selected_process = NULL;
  gint selected_count = gtk_tree_selection_count_selected_rows (selection);

  app->selection = selection;

  gchar *button_text = ngettext ("_End Process…", "_End Processes…", selected_count);

  gtk_button_set_label (GTK_BUTTON (app->end_process_button), button_text);

  /* get the most recent selected process and determine if there are
  ** no selected processes
  */
  gtk_tree_selection_selected_foreach (selection, get_last_selected,
                                       &selected_process);
  if (selected_process)
    {
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
  update_sensitivity (app);
}

static gint
cb_timeout (gpointer data)
{
  GsmApplication *app = (GsmApplication *) data;
  guint new_interval;

  proctable_update (app);

  if (app->smooth_refresh->get (new_interval))
    {
      app->timeout = g_timeout_add (new_interval,
                                    cb_timeout,
                                    app);
      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

static void
cb_refresh_icons (GtkIconTheme*,
                  gpointer data)
{
  GsmApplication *app = (GsmApplication *) data;

  for (auto&v : app->processes)
    app->pretty_table->set_icon (v.second);

  proctable_update (app);
}

static gboolean
iter_matches_search_key (GtkTreeModel *model,
                         GtkTreeIter  *iter,
                         const gchar  *search_text)
{
  char *name;
  char *user;
  pid_t pid;
  char *pids;
  char *args;
  gboolean found;
  char *search_pattern;
  char **keys;
  Glib::RefPtr<Glib::Regex> regex;

  gtk_tree_model_get (model, iter,
                      COL_NAME, &name,
                      COL_USER, &user,
                      COL_PID, &pid,
                      COL_ARGS, &args,
                      -1);

  pids = g_strdup_printf ("%d", pid);

  keys = g_strsplit_set (search_text, " |", -1);
  search_pattern = g_strjoinv ("|", keys);
  try {
      regex = Glib::Regex::create (search_pattern, Glib::Regex::CompileFlags::CASELESS);
    } catch (const Glib::Error&ex) {
      regex = Glib::Regex::create (Glib::Regex::escape_string (search_pattern), Glib::Regex::CompileFlags::CASELESS);
    }

  found = (name && regex->match (name)) || (user && regex->match (user))
          || (pids && regex->match (pids)) || (args && regex->match (args));

  g_strfreev (keys);
  g_free (search_pattern);
  g_free (name);
  g_free (user);
  g_free (args);
  g_free (pids);

  return found;
}

static gboolean
process_visibility_func (GtkTreeModel *model,
                         GtkTreeIter  *iter,
                         gpointer      data)
{
  GsmApplication * const app = static_cast<GsmApplication *>(data);
  const gchar *search_text = app->search_entry == NULL ? "" : gtk_editable_get_text (GTK_EDITABLE (app->search_entry));
  GtkTreePath *tree_path = gtk_tree_model_get_path (model, iter);

  if (strcmp (search_text, "") == 0)
    {
      gtk_tree_path_free (tree_path);
      return TRUE;
    }

  // in case we are in dependencies view, we show (and expand) rows not matching the text, but having a matching child
  gboolean match = false;

  if (app->settings->get_boolean (GSM_SETTING_SHOW_DEPENDENCIES))
    {
      GtkTreeIter child;
      if (gtk_tree_model_iter_children (model, &child, iter))
        {
          gboolean child_match = FALSE;
          do
            {
              child_match = process_visibility_func (model, &child, data);
            } while (gtk_tree_model_iter_next (model, &child) && !child_match);
          match = child_match;
        }

      match |= iter_matches_search_key (model, iter, search_text);
      if (match && (strlen (search_text) > 0))
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (app->tree), tree_path);
    }
  else
    {
      match = iter_matches_search_key (model, iter, search_text);
    }

  gtk_tree_path_free (tree_path);

  return match;
}

static void
proctable_clear_tree (GsmApplication * const app)
{
  GtkTreeModel *model;

  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (
                                             gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (
                                                                              gtk_tree_view_get_model (GTK_TREE_VIEW (app->tree))))));

  gtk_tree_store_clear (GTK_TREE_STORE (model));

  proctable_free_table (app);

  update_sensitivity (app);
}

static void
cb_show_dependencies_changed (Gio::Settings& settings,
                              Glib::ustring,
                              GsmApplication*app)
{
  if (app->timeout)
    {
      gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (app->tree),
                                        settings.get_boolean (GSM_SETTING_SHOW_DEPENDENCIES));

      proctable_clear_tree (app);
      proctable_update (app);
    }
}

static void
cb_show_whose_processes_changed (Gio::Settings&,
                                 Glib::ustring,
                                 GsmApplication *app)
{
  if (app->timeout)
    {
      proctable_clear_tree (app);
      proctable_update (app);
    }
}

GsmTreeView *
proctable_new (GsmApplication * const app)
{
  GsmTreeView *proctree;
  GtkTreeStore *model;
  GtkTreeModelFilter *model_filter;
  GtkTreeModelSort *model_sort;
  GtkTreeSelection *selection;
  GtkGesture *click_controller;
  GtkTreeViewColumn *column;
  GtkCellRenderer *cell_renderer;
  const gchar *titles[] = {
    N_("Name"),
    N_("User"),
    N_("Status"),
    N_("Virtual Memory"),
    N_("Resident Memory"),
    N_("Writable Memory"),
    N_("Shared Memory"),
    N_("X Server Memory"),
    /* xgettext:no-c-format */ N_("CPU"),
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
    N_("Disk Read Total"),
    N_("Disk Write Total"),
    N_("Disk Read"),
    N_("Disk Write"),
    N_("Priority"),
    NULL,
    "POINTER"
  };
  gint i;
  auto settings = g_settings_get_child (app->settings->gobj (), GSM_SETTINGS_CHILD_PROCESSES);

  model = gtk_tree_store_new (NUM_COLUMNS,
                              G_TYPE_STRING,        /* Process Name */
                              G_TYPE_STRING,        /* User         */
                              G_TYPE_UINT,          /* Status       */
                              G_TYPE_ULONG,         /* VM Size      */
                              G_TYPE_ULONG,         /* Resident Memory */
                              G_TYPE_ULONG,         /* Writable Memory */
                              G_TYPE_ULONG,         /* Shared Memory */
                              G_TYPE_ULONG,         /* X Server Memory */
                              G_TYPE_DOUBLE,        /* % CPU        */
                              G_TYPE_UINT64,        /* CPU time     */
                              G_TYPE_ULONG,         /* Started      */
                              G_TYPE_INT,           /* Nice         */
                              G_TYPE_UINT,          /* ID           */
                              G_TYPE_STRING,        /* Security Context */
                              G_TYPE_STRING,        /* Arguments    */
                              G_TYPE_ULONG,         /* Memory       */
                              G_TYPE_STRING,        /* wchan        */
                              G_TYPE_STRING,        /* Cgroup       */
                              G_TYPE_STRING,        /* Unit         */
                              G_TYPE_STRING,        /* Session      */
                              G_TYPE_STRING,        /* Seat         */
                              G_TYPE_STRING,        /* Owner        */
                              G_TYPE_UINT64,        /* Disk read total */
                              G_TYPE_UINT64,        /* Disk write total*/
                              G_TYPE_UINT64,        /* Disk read    */
                              G_TYPE_UINT64,        /* Disk write   */
                              G_TYPE_STRING,        /* Priority     */
                              GDK_TYPE_TEXTURE,     /* Icon         */
                              G_TYPE_POINTER,       /* ProcInfo     */
                              G_TYPE_STRING         /* Sexy tooltip */
                              );

  model_filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (model), NULL));
  gtk_tree_model_filter_set_visible_func (model_filter, process_visibility_func, app, NULL);

  model_sort = GTK_TREE_MODEL_SORT (gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (model_filter)));

  proctree = gsm_tree_view_new (settings, TRUE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (proctree), GTK_TREE_MODEL (model_sort));
  gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (proctree), COL_TOOLTIP);
  gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (proctree), app->settings->get_boolean (GSM_SETTING_SHOW_DEPENDENCIES));
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (proctree), FALSE);
  g_object_unref (G_OBJECT (model));

  column = gtk_tree_view_column_new ();

  cell_renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, cell_renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, cell_renderer,
                                       "texture", COL_ICON,
                                       NULL);

  cell_renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell_renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, cell_renderer,
                                       "text", COL_NAME,
                                       NULL);
  gtk_tree_view_column_set_title (column, _(titles[0]));

  gtk_tree_view_column_set_sort_column_id (column, COL_NAME);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_min_width (column, 1);
  gtk_tree_view_column_set_reorderable (column, TRUE);

  gsm_tree_view_append_and_bind_column (proctree, column);
  gtk_tree_view_set_expander_column (GTK_TREE_VIEW (proctree), column);

  gtk_tree_view_column_set_expand (column, TRUE);

  for (i = COL_USER; i <= COL_PRIORITY; i++)
    {
      GtkWidget *box;
      GtkWidget *title_label;
      GtkWidget *value_label;
      GtkTreeViewColumn *col;
      GtkCellRenderer *cell;
      PangoAttrList *attrs = NULL;

      if (i == COL_MEMWRITABLE)
        continue;

      cell = gtk_cell_renderer_text_new ();
      col = gtk_tree_view_column_new ();
      gtk_tree_view_column_pack_start (col, cell, TRUE);

      gtk_tree_view_column_set_title (col, _(titles[i]));

      switch (i) {
        case COL_CPU:
        case COL_MEM:
        case COL_VMSIZE:
        case COL_MEMRES:
        case COL_MEMWRITABLE:
        case COL_MEMSHARED:
        case COL_DISK_READ_TOTAL:
        case COL_DISK_WRITE_TOTAL:
        case COL_DISK_READ_CURRENT:
        case COL_DISK_WRITE_CURRENT:
          /* Insert a ‘total’ label */
          box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
          title_label = gtk_label_new (_(titles[i]));
          value_label = gtk_label_new ("");
          gtk_widget_add_css_class (value_label, "numeric");
          gtk_box_append (GTK_BOX (box), title_label);
          gtk_box_append (GTK_BOX (box), value_label);
          gtk_tree_view_column_set_widget (col, box);
          break;
        default:
          /* Don't insert a second label */
          break;
      }
      
      gtk_tree_view_column_set_resizable (col, TRUE);
      gtk_tree_view_column_set_sort_column_id (col, i);
      gtk_tree_view_column_set_reorderable (col, TRUE);
      gsm_tree_view_append_and_bind_column (proctree, col);

      // type
      switch (i)
        {
          case COL_VMSIZE:
          case COL_MEMRES:
          case COL_MEMSHARED:
          case COL_MEM:
            gtk_tree_view_column_set_cell_data_func (col, cell,
                                                     &procman::size_na_cell_data_func,
                                                     GUINT_TO_POINTER (i),
                                                     NULL);
            break;

          case COL_CPU:
            gtk_tree_view_column_set_cell_data_func (col, cell,
                                                     &procman::percentage_cell_data_func,
                                                     GUINT_TO_POINTER (i),
                                                     NULL);
            break;

          case COL_CPU_TIME:
            gtk_tree_view_column_set_cell_data_func (col, cell,
                                                     &procman::duration_cell_data_func,
                                                     GUINT_TO_POINTER (i),
                                                     NULL);
            break;

          case COL_START_TIME:
            gtk_tree_view_column_set_cell_data_func (col, cell,
                                                     &procman::time_cell_data_func,
                                                     GUINT_TO_POINTER (i),
                                                     NULL);
            break;

          case COL_STATUS:
            gtk_tree_view_column_set_cell_data_func (col, cell,
                                                     &procman::status_cell_data_func,
                                                     GUINT_TO_POINTER (i),
                                                     NULL);
            break;

          case COL_DISK_READ_TOTAL:
          case COL_DISK_WRITE_TOTAL:
            gtk_tree_view_column_set_cell_data_func (col, cell,
                                                     &procman::size_na_cell_data_func,
                                                     GUINT_TO_POINTER (i),
                                                     NULL);
            break;

          case COL_DISK_READ_CURRENT:
          case COL_DISK_WRITE_CURRENT:
            gtk_tree_view_column_set_cell_data_func (col, cell,
                                                     &procman::io_rate_cell_data_func,
                                                     GUINT_TO_POINTER (i),
                                                     NULL);

            break;

          case COL_PRIORITY:
            gtk_tree_view_column_set_cell_data_func (col, cell,
                                                     &procman::priority_cell_data_func,
                                                     GUINT_TO_POINTER (COL_NICE),
                                                     NULL);
            break;

          default:
            gtk_tree_view_column_set_attributes (col, cell, "text", i, NULL);
            break;
        }

      // Tabular Numbers
      switch (i)
        {
          case COL_PID:
          case COL_VMSIZE:
          case COL_MEMRES:
          case COL_MEMSHARED:
          case COL_MEM:
          case COL_CPU:
          case COL_CPU_TIME:
          case COL_DISK_READ_TOTAL:
          case COL_DISK_WRITE_TOTAL:
          case COL_DISK_READ_CURRENT:
          case COL_DISK_WRITE_CURRENT:
          case COL_START_TIME:
          case COL_NICE:
          case COL_WCHAN:
            attrs = make_tnum_attr_list ();
            g_object_set (cell, "attributes", attrs, NULL);
            g_clear_pointer (&attrs, pango_attr_list_unref);
            break;

          default:
            break;
        }

      // sorting
      switch (i)
        {
          case COL_VMSIZE:
          case COL_MEMRES:
          case COL_MEMSHARED:
          case COL_MEM:
          case COL_CPU:
          case COL_CPU_TIME:
          case COL_DISK_READ_TOTAL:
          case COL_DISK_WRITE_TOTAL:
          case COL_DISK_READ_CURRENT:
          case COL_DISK_WRITE_CURRENT:
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
      switch (i)
        {
          case COL_VMSIZE:
          case COL_MEMRES:
          case COL_MEMSHARED:
          case COL_CPU:
          case COL_NICE:
          case COL_PID:
          case COL_DISK_READ_TOTAL:
          case COL_DISK_WRITE_TOTAL:
          case COL_DISK_READ_CURRENT:
          case COL_DISK_WRITE_CURRENT:
          case COL_CPU_TIME:
          case COL_MEM:
            g_object_set (G_OBJECT (cell), "xalign", 1.0f, NULL);
            break;
        }

      gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
      // sizing
      switch (i)
        {
          case COL_ARGS:
            gtk_tree_view_column_set_min_width (col, 150);
            break;

          default:
            gtk_tree_view_column_set_min_width (column, 20);
            break;
        }

      if (i == COL_ARGS)
        gtk_tree_view_column_set_expand (col, TRUE);
      else
        gtk_tree_view_column_set_expand (col, FALSE);
    }
  app->tree = proctree;
  app->top_of_tree = NULL;
  app->last_vscroll_max = 0;
  app->last_vscroll_value = 0;

  if (!cgroups_enabled ())
    gsm_tree_view_add_excluded_column (proctree, COL_CGROUP);

  if (!procman::systemd_logind_running ())
    {
      gsm_tree_view_add_excluded_column (proctree, COL_UNIT);
      gsm_tree_view_add_excluded_column (proctree, COL_SESSION);
      gsm_tree_view_add_excluded_column (proctree, COL_SEAT);
      gsm_tree_view_add_excluded_column (proctree, COL_OWNER);
    }

  if (!can_show_security_context_column ())
    gsm_tree_view_add_excluded_column (proctree, COL_SECURITYCONTEXT);

  gsm_tree_view_load_state (proctree);

  GtkIconTheme*theme = gtk_icon_theme_get_for_display (gdk_display_get_default ());
  g_signal_connect (G_OBJECT (theme), "changed", G_CALLBACK (cb_refresh_icons), app);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (proctree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  app->selection = selection;

  click_controller = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click_controller), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller (GTK_WIDGET (proctree), GTK_EVENT_CONTROLLER (click_controller));

  g_signal_connect (G_OBJECT (click_controller), "pressed",
                    G_CALLBACK (cb_tree_button_pressed), app);

  g_signal_connect (G_OBJECT (selection),
                    "changed",
                    G_CALLBACK (cb_row_selected), app);

  g_signal_connect (G_OBJECT (proctree), "destroy",
                    G_CALLBACK (cb_proctree_destroying), app);

  g_signal_connect (G_OBJECT (proctree), "columns-changed",
                    G_CALLBACK (cb_save_tree_state), app);

  g_signal_connect (G_OBJECT (model_sort), "sort-column-changed",
                    G_CALLBACK (cb_save_tree_state), app);

  app->settings->signal_changed (GSM_SETTING_SHOW_DEPENDENCIES).connect ([app](const Glib::ustring&key) {
    cb_show_dependencies_changed (*app->settings.operator-> (), key, app);
  });

  app->settings->signal_changed (GSM_SETTING_SHOW_WHOSE_PROCESSES).connect ([app](const Glib::ustring&key) {
    cb_show_whose_processes_changed (*app->settings.operator-> (), key, app);
  });

  return proctree;
}

void
get_process_memory_writable (ProcInfo *info)
{
  glibtop_proc_map buf;
  glibtop_map_entry *maps;

  maps = glibtop_get_proc_map (&buf, info->pid);

  const bool use_private_dirty = buf.flags & (1 << GLIBTOP_MAP_ENTRY_PRIVATE_DIRTY);

  gulong memwritable = 0;
  const unsigned number = buf.number;

  for (unsigned i = 0; i < number; ++i)
    {
      if (use_private_dirty)
        // clang++ 3.4 is not smart enough to move this invariant out of the loop
        // but who cares ?
        memwritable += maps[i].private_dirty;
      else if (maps[i].perm & GLIBTOP_MAP_PERM_WRITE)
        memwritable += maps[i].size;
    }

  info->memwritable = memwritable;

  g_free (maps);
}

static void
get_process_memory_info (ProcInfo *info)
{
  glibtop_proc_mem procmem;

  glibtop_get_proc_mem (&procmem, info->pid);

  info->vmsize = procmem.vsize;
  info->memres = procmem.resident;
  info->memshared = procmem.share;

  info->mem = info->memres - info->memshared;
}

static void
update_info_mutable_cols (ProcInfo *info)
{
  GtkTreeModel *model;

  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (
                                             gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (
                                                                              gtk_tree_view_get_model (GTK_TREE_VIEW (GsmApplication::get ().tree))))));

  using procman::tree_store_update;

  tree_store_update (model, &info->node, COL_STATUS, info->status);
  tree_store_update (model, &info->node, COL_USER, info->user.c_str ());
  tree_store_update (model, &info->node, COL_VMSIZE, info->vmsize);
  tree_store_update (model, &info->node, COL_MEMRES, info->memres);
  tree_store_update (model, &info->node, COL_MEMSHARED, info->memshared);
  tree_store_update (model, &info->node, COL_CPU, info->pcpu);
  tree_store_update (model, &info->node, COL_CPU_TIME, info->cpu_time);
  tree_store_update (model, &info->node, COL_DISK_READ_TOTAL, info->disk_read_bytes_total);
  tree_store_update (model, &info->node, COL_DISK_WRITE_TOTAL, info->disk_write_bytes_total);
  tree_store_update (model, &info->node, COL_DISK_READ_CURRENT, info->disk_read_bytes_current);
  tree_store_update (model, &info->node, COL_DISK_WRITE_CURRENT, info->disk_write_bytes_current);
  tree_store_update (model, &info->node, COL_START_TIME, info->start_time);
  tree_store_update (model, &info->node, COL_NICE, info->nice);
  tree_store_update (model, &info->node, COL_MEM, info->mem);
  tree_store_update (model, &info->node, COL_WCHAN, info->wchan.c_str ());
  tree_store_update (model, &info->node, COL_CGROUP, info->cgroup_name.c_str ());
  tree_store_update (model, &info->node, COL_UNIT, info->unit.c_str ());
  tree_store_update (model, &info->node, COL_SESSION, info->session.c_str ());
  tree_store_update (model, &info->node, COL_SEAT, info->seat.c_str ());
  tree_store_update (model, &info->node, COL_OWNER, info->owner.c_str ());
}

static void
insert_info_to_tree (ProcInfo       *info,
                     GsmApplication *app,
                     bool            forced = false)
{
  GtkTreeModel *model;
  GtkTreeModel *filtered;
  GtkTreeModel *sorted;

  sorted = gtk_tree_view_get_model (GTK_TREE_VIEW (app->tree));
  filtered = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sorted));
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filtered));

  if (app->settings->get_boolean (GSM_SETTING_SHOW_DEPENDENCIES))
    {
      ProcInfo *parent = 0;

      if (not forced)
        parent = app->processes.find (info->ppid);

      if (parent)
        {
          GtkTreePath *parent_node = gtk_tree_model_get_path (model, &parent->node);
          gtk_tree_store_insert (GTK_TREE_STORE (model), &info->node, &parent->node, 0);

          GtkTreePath *filtered_parent = gtk_tree_model_filter_convert_child_path_to_path (GTK_TREE_MODEL_FILTER (filtered), parent_node);
          if (filtered_parent != NULL)
            {
              GtkTreePath *sorted_parent = gtk_tree_model_sort_convert_child_path_to_path (GTK_TREE_MODEL_SORT (sorted), filtered_parent);

              if (sorted_parent != NULL)
                {
                  if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (app->tree), sorted_parent)
#ifdef __linux__
                      // on linuxes we don't want to expand kthreadd by default (always has pid 2)
                      && (parent->pid != 2)
#endif
                      )
                    gtk_tree_view_expand_row (GTK_TREE_VIEW (app->tree), sorted_parent, FALSE);
                  gtk_tree_path_free (sorted_parent);
                }
              gtk_tree_path_free (filtered_parent);
            }
          gtk_tree_path_free (parent_node);
        }
      else
        {
          gtk_tree_store_insert (GTK_TREE_STORE (model), &info->node, NULL, 0);
        }
    }
  else
    {
      gtk_tree_store_insert (GTK_TREE_STORE (model), &info->node, NULL, 0);
    }

  gtk_tree_store_set (GTK_TREE_STORE (model), &info->node,
                      COL_POINTER, info,
                      COL_NAME, info->name.c_str (),
                      COL_ARGS, info->arguments.c_str (),
                      COL_TOOLTIP, info->tooltip.c_str (),
                      COL_PID, info->pid,
                      COL_SECURITYCONTEXT, info->security_context.c_str (),
                      -1);

  app->pretty_table->set_icon (*info);

  procman_debug ("inserted %d%s", info->pid, (forced ? " (forced)" : ""));
}

/* Removing a node with children - make sure the children are queued
** to be readded.
*/
template<typename List>
static void
remove_info_from_tree (GsmApplication *app,
                       GtkTreeModel   *model,
                       ProcInfo&       current,
                       List &          orphans,
                       unsigned        lvl = 0)
{
  GtkTreeIter child_node;

  if (std::find (orphans.begin (), orphans.end (), &current) != orphans.end ())
    {
      procman_debug ("[%u] %d already removed from tree", lvl, int(current.pid));
      return;
    }

  procman_debug ("[%u] pid %d, %d children", lvl, int(current.pid),
                 gtk_tree_model_iter_n_children (model, &current.node));

  // it is not possible to iterate&erase over a treeview so instead we
  // just pop one child after another and recursively remove it and
  // its children

  while (gtk_tree_model_iter_children (model, &child_node, &current.node))
    {
      ProcInfo *child = 0;
      gtk_tree_model_get (model, &child_node, COL_POINTER, &child, -1);
      remove_info_from_tree (app, model, *child, orphans, lvl + 1);
    }

  g_assert (not gtk_tree_model_iter_has_child (model, &current.node));

  orphans.push_back (&current);
  gtk_tree_store_remove (GTK_TREE_STORE (model), &current.node);
  procman::poison (current.node, 0x69);
}


static std::string
get_proc_kernel_wchan (glibtop_proc_kernel&obj)
{
  char buf[40] = { 0 };

  g_strlcpy (buf, obj.wchan, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';
  return buf;
}

static void
update_info (GsmApplication *app,
             ProcInfo       *info)
{
  glibtop_proc_state procstate;
  glibtop_proc_uid procuid;
  glibtop_proc_time proctime;
  glibtop_proc_kernel prockernel;
  glibtop_proc_io procio;
  gdouble update_interval_seconds = app->config.update_interval / 1000;

  glibtop_get_proc_kernel (&prockernel, info->pid);
  info->wchan = get_proc_kernel_wchan (prockernel);

  glibtop_get_proc_state (&procstate, info->pid);
  info->status = procstate.state;

  glibtop_get_proc_uid (&procuid, info->pid);
  glibtop_get_proc_time (&proctime, info->pid);
  glibtop_get_proc_io (&procio, info->pid);

  get_process_memory_info (info);

  info->set_user (procstate.uid);

  // if the cpu time has increased reset the status to running
  // regardless of kernel state (#606579)
  guint64 difference = proctime.rtime - info->cpu_time;

  if (difference > 0)
    info->status = GLIBTOP_PROCESS_RUNNING;

  guint cpu_scale = 100;

  if (not app->config.solaris_mode)
    cpu_scale *= app->config.num_cpus;

  info->pcpu = (gdouble)difference * cpu_scale / app->cpu_total_time;
  info->pcpu = MIN (info->pcpu, cpu_scale);

  app->processes.cpu_times[info->pid] = info->cpu_time = proctime.rtime;
  info->nice = procuid.nice;

  info->disk_write_bytes_current = (procio.disk_wbytes - info->disk_write_bytes_total) / update_interval_seconds;
  info->disk_read_bytes_current = (procio.disk_rbytes - info->disk_read_bytes_total) / update_interval_seconds;

  info->disk_write_bytes_total = procio.disk_wbytes;
  info->disk_read_bytes_total = procio.disk_rbytes;

  // set the ppid only if one can exist
  // i.e. pid=0 can never have a parent
  if (info->pid > 0)
    info->ppid = procuid.ppid;

  g_assert (info->pid != info->ppid);
  g_assert (info->ppid != -1 || info->pid == 0);

  /* get cgroup data */
  get_process_cgroup_info (*info);

  procman::get_process_systemd_info (info);
}

void
proctable_refresh_summary_headers(GsmApplication * app)
{
  GtkTreeIter iter;
  GList *columns, *it;
  double total_cpu = 0;
  uint64_t total_mem = 0;
  uint64_t total_memres = 0;
  uint64_t total_vmsize = 0;
  uint64_t total_memshared = 0;
  uint64_t total_memwritable = 0;
  uint64_t total_disk_read_bytes_total = 0;
  uint64_t total_disk_write_bytes_total = 0;
  uint64_t total_disk_read_bytes_current = 0;
  uint64_t total_disk_write_bytes_current = 0;
  std::function<void(GtkTreeIter&)> calc_summary;
  GtkTreeView *treeview = GTK_TREE_VIEW (app->tree);
  GtkTreeModel *model = GTK_TREE_MODEL(
    GTK_TREE_MODEL_FILTER (
      gtk_tree_model_sort_get_model (
        GTK_TREE_MODEL_SORT (
          gtk_tree_view_get_model (
            treeview
          )
        )
      )
    )
  );

  // Helper: convert bytes to readable units
  auto format_bytes = [app](uint64_t bytes) -> std::string
  {
    const GFormatSizeFlags flags =
      app->config.process_memory_in_iec ? G_FORMAT_SIZE_IEC_UNITS : G_FORMAT_SIZE_BITS;

    return make_string (g_format_size_full (bytes, flags));
  };

  // Helper: get second label from column header
  auto get_value_label = [](GtkTreeViewColumn* col) -> GtkLabel *
  {
    GtkWidget *box = gtk_tree_view_column_get_widget(col);
    if (!GTK_IS_BOX(box))
      return nullptr;

    GtkWidget *child = gtk_widget_get_first_child(box);
    if (!child)
      return nullptr;

    GtkWidget *second_child = gtk_widget_get_next_sibling(child);
    if (GTK_IS_LABEL(second_child))
    {
      return GTK_LABEL(second_child);
    }

    return nullptr;
  };

  // Accumulate totals
  calc_summary = [&model, 
                  &total_mem, 
                  &total_cpu,
                  &total_vmsize,
                  &total_memres,
                  &total_memshared,
                  &total_memwritable,
                  &total_disk_read_bytes_total,
                  &total_disk_write_bytes_total,
                  &total_disk_read_bytes_current,
                  &total_disk_write_bytes_current,
                  &calc_summary](GtkTreeIter &iter) 
  {
    GtkTreeIter child_iter;
    // take out value
    gpointer node_ptr;
    gtk_tree_model_get(model, &iter, COL_POINTER, &node_ptr, -1);

    ProcInfo *proc = static_cast<ProcInfo *>(node_ptr);
    if (!proc)
      return;

    total_mem += proc->mem;
    total_cpu += round(proc->pcpu * 10.0f) / 10.0f;
    total_memres += proc->memres;
    total_vmsize += proc->vmsize;
    total_memshared += proc->memshared;
    total_memwritable += proc->memwritable;
    total_disk_read_bytes_total += proc->disk_read_bytes_total;
    total_disk_write_bytes_total += proc->disk_write_bytes_total;
    total_disk_read_bytes_current += proc->disk_read_bytes_current;
    total_disk_write_bytes_current += proc->disk_write_bytes_current;

    // child
    if (gtk_tree_model_iter_has_child(model, &iter)) {
        gtk_tree_model_iter_children(model, &child_iter, &iter);
        calc_summary(child_iter);
    }

    // next sibling
    // recerssive check
    gboolean valid = gtk_tree_model_iter_next(model, &iter);
    if (!valid) return;
    else calc_summary(iter);
  };


  gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
  if(valid) calc_summary(iter);

  columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (treeview));

  for (it = columns; it; it = it->next)
  {
    std::string v_str;
    GtkTreeViewColumn *column = static_cast<GtkTreeViewColumn *>(it->data);
    GtkLabel *label = get_value_label(column);

    /* Not every column has a header */
    if (!label) {
      continue;
    }

    gint column_id = gtk_tree_view_column_get_sort_column_id(column);
    switch (column_id)
    {
    case COL_CPU:
      v_str = make_string (g_strdup_printf ("%.01f%%", total_cpu));
      break;
    case COL_MEM:
      v_str = format_bytes(total_mem);
      break;
    case COL_VMSIZE:
      v_str = format_bytes(total_vmsize);
      break;
    case COL_MEMRES:
      v_str = format_bytes(total_memres);
      break;
    case COL_MEMWRITABLE:
      v_str = format_bytes(total_memwritable);
      break;
    case COL_MEMSHARED:
      v_str = format_bytes(total_memshared);
      break;
    case COL_DISK_READ_TOTAL:
      v_str = format_bytes(total_disk_read_bytes_total);
      break;
    case COL_DISK_WRITE_TOTAL:
      v_str = format_bytes(total_disk_write_bytes_total);
      break;
    case COL_DISK_READ_CURRENT:
      v_str = format_bytes(total_disk_read_bytes_current);
      break;
    case COL_DISK_WRITE_CURRENT:
      v_str = format_bytes(total_disk_write_bytes_current);
      break;
    default:
      v_str = "";
    }
    gtk_label_set_text(label, v_str.c_str());
  }
}

static void
refresh_list (GsmApplication *app,
              const pid_t    *pid_list,
              const guint     n)
{
  typedef std::list<ProcInfo*> ProcList;
  ProcList addition;

  GtkTreeModel    *model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (
                                                              gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (
                                                                                               gtk_tree_view_get_model (GTK_TREE_VIEW (app->tree))))));
  guint i;

  // Add or update processes in the process list
  for (i = 0; i < n; ++i)
    {
      ProcInfo *info = app->processes.find (pid_list[i]);

      if (!info)
        {
          info = app->processes.add (pid_list[i]);
          addition.push_back (info);
        }

      update_info (app, info);
    }


  // Remove dead processes from the process list and from the
  // tree. children are queued to be readded at the right place
  // in the tree.

  const std::set<pid_t> pids (pid_list, pid_list + n);

  auto it = std::begin (app->processes);

  while (it != std::end (app->processes))
    {
      auto&info = it->second;
      if (pids.find (info.pid) == pids.end ())
        {
          procman_debug ("ripping %d", info.pid);
          remove_info_from_tree (app, model, info, addition);
          addition.remove (&info);
          it = app->processes.erase (it);
        }
      else
        {
          ++it;
        }
    }

  // INVARIANT
  // pid_list == ProcInfo::all + addition


  if (app->settings->get_boolean (GSM_SETTING_SHOW_DEPENDENCIES))
    {
      // insert process in the tree. walk through the addition list
      // (new process + process that have a new parent). This loop
      // handles the dependencies because we cannot insert a process
      // until its parent is in the tree.

      std::set<pid_t> in_tree (pids);

      for (ProcList::iterator it (addition.begin ()); it != addition.end (); ++it)
        in_tree.erase ((*it)->pid);


      while (not addition.empty ())
        {
          procman_debug ("looking for %d parents", int(addition.size ()));
          ProcList::iterator it (addition.begin ());

          while (it != addition.end ())
            {
              procman_debug ("looking for %d's parent with ppid %d",
                             int((*it)->pid), int((*it)->ppid));


              // inserts the process in the treeview if :
              // - it has no parent (ppid = -1),
              //   ie it is for example the [kernel] on FreeBSD
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


              if ((*it)->ppid <= 0 or in_tree.find ((*it)->ppid) != in_tree.end ())
                {
                  insert_info_to_tree (*it, app);
                  in_tree.insert ((*it)->pid);
                  it = addition.erase (it);
                  continue;
                }

              ProcInfo *parent = app->processes.find ((*it)->ppid);
              // if the parent is unreachable
              if (not parent)
                {
                  // or std::find(addition.begin(), addition.end(), parent) == addition.end()) {
                  insert_info_to_tree (*it, app, true);
                  in_tree.insert ((*it)->pid);
                  it = addition.erase (it);
                  continue;
                }

              ++it;
            }
        }
    }
  else
    {
      // don't care of the tree
      for (auto&v : addition)
        insert_info_to_tree (v, app);
    }

  // update Header
  proctable_refresh_summary_headers(app);

  for (auto&v : app->processes)
    update_info_mutable_cols (&v.second);
}

void
proctable_update (GsmApplication *app)
{
  pid_t*pid_list;
  glibtop_proclist proclist;
  glibtop_cpu cpu;
  int which = 0;
  int arg = 0;
  auto whose_processes = app->settings->get_string (GSM_SETTING_SHOW_WHOSE_PROCESSES);

  if (whose_processes == "all")
    {
      which = GLIBTOP_KERN_PROC_ALL;
      arg = 0;
    }
  else if (whose_processes == "active")
    {
      which = GLIBTOP_KERN_PROC_ALL | GLIBTOP_EXCLUDE_IDLE;
      arg = 0;
    }
  else if (whose_processes == "user")
    {
      which = GLIBTOP_KERN_PROC_UID;
      arg = getuid ();
    }

  pid_list = glibtop_get_proclist (&proclist, which, arg);

  /* FIXME: total cpu time elapsed should be calculated on an individual basis here
  ** should probably have a total_time_last gint in the ProcInfo structure */
  glibtop_get_cpu (&cpu);
  app->cpu_total_time = MAX (cpu.total - app->cpu_total_time_last, 1);
  app->cpu_total_time_last = cpu.total;

  // FIXME: not sure if glibtop always returns a sorted list of pid
  // but it is important otherwise refresh_list won't find the parent
  std::sort (pid_list, pid_list + proclist.number);
  refresh_list (app, pid_list, proclist.number);

  // juggling with tree scroll position to fix https://bugzilla.gnome.org/show_bug.cgi?id=92724
  GtkTreePath*current_top;

  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (app->tree), 0, 0, &current_top, NULL, NULL, NULL))
    {
      GtkAdjustment *vadjustment = GTK_ADJUSTMENT (gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (app->tree)));
      gdouble current_max = gtk_adjustment_get_upper (vadjustment);
      gdouble current_value = gtk_adjustment_get_value (vadjustment);

      if (app->top_of_tree)
        {
          // if the visible cell from the top of the tree is still the same, as last time
          if (gtk_tree_path_compare (app->top_of_tree, current_top) == 0)
            {
              //but something from the scroll parameters has changed compared to the last values
              if (app->last_vscroll_value == 0 && current_value != 0)
                // the tree was scrolled to top, and something has been added above the current top row
                gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (app->tree), -1, 0);
              else if (current_max > app->last_vscroll_max && app->last_vscroll_max == app->last_vscroll_value)
                // the tree was scrolled to bottom, something has been added below the current bottom row
                gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (app->tree), -1, current_max);
            }

          gtk_tree_path_free (app->top_of_tree);
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
  app->processes.clear ();
}

void
proctable_freeze (GsmApplication *app)
{
  if (app->timeout)
    {
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
