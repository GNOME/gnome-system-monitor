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
#include "procactions.h"
#include "procproperties.h"
#include "procinfo.h"
#include "proctable.h"
#include "util.h"
#include "legacy/e_date.h"

typedef struct _proc_arg
{
  const gchar *prop;
  gchar *val;
} proc_arg;

static char *
format_memsize (guint64 size)
{
  if (size == 0)
    return g_strdup ("—");
  else
    return g_format_size_full (size, G_FORMAT_SIZE_IEC_UNITS);
}

static void
fill_proc_properties (GtkBuilder *builder,
                      ProcInfo   *info)
{
  if (!info)
    return;

  get_process_memory_writable (info);

  GtkLabel *label;
  AdwActionRow *row;
  char *text;

  label = GTK_LABEL (gtk_builder_get_object (builder, "pid_label"));
  text = g_strdup_printf ("%d", info->pid);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "user_label"));
  text = g_strdup_printf ("%s (%d)", info->user.c_str (), info->uid);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "started_label"));
  text = procman_format_date_for_display (info->start_time);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "priority_label"));
  text = g_strdup_printf ("%s (%d)", procman::get_nice_level (info->nice), info->nice);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "status_label"));
  gtk_label_set_label (label, format_process_state (info->status));

  label = GTK_LABEL (gtk_builder_get_object (builder, "cpu_label"));
  text = g_strdup_printf ("%.2f%%", info->pcpu);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "memory_label"));
  text = format_memsize (info->mem);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "cputime_label"));
  text = procman::format_duration_for_display (100 * info->cpu_time / GsmApplication::get ().frequency);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "vmemory_label"));
  text = format_memsize (info->vmsize);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "rmemory_label"));
  text = format_memsize (info->memres);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "wmemory_label"));
  text = format_memsize (info->memwritable);
  gtk_label_set_label (label, text);
  g_free (text);

  label = GTK_LABEL (gtk_builder_get_object (builder, "smemory_label"));
  text = format_memsize (info->memshared);
  gtk_label_set_label (label, text);
  g_free (text);

  row = ADW_ACTION_ROW (gtk_builder_get_object (builder, "securitycontext_row"));
  text = not info->security_context.empty ()
            ? g_strdup_printf ("%s", info->security_context.c_str ())
            : g_strdup ("—");
  adw_action_row_set_subtitle (row, text);
  g_free (text);

  row = ADW_ACTION_ROW (gtk_builder_get_object (builder, "commandline_row"));
  text = g_strdup_printf ("%s", info->arguments.c_str ());
  adw_action_row_set_subtitle (row, text);
  g_free (text);

  row = ADW_ACTION_ROW (gtk_builder_get_object (builder, "waitingchannel_row"));
  text = g_strdup_printf ("%s", info->wchan.c_str ());
  adw_action_row_set_subtitle (row, text);
  g_free (text);

  row = ADW_ACTION_ROW (gtk_builder_get_object (builder, "controlgroup_row"));
  text = not info->cgroup_name.empty ()
            ? g_strdup_printf ("%s", info->cgroup_name.c_str ())
            : g_strdup ("—");
  adw_action_row_set_subtitle (row, text);
  g_free (text);
}

static void
update_procproperties_dialog (GtkBuilder *builder)
{
  ProcInfo *info;
  GtkWidget *widget;

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "procprop_dialog"));

  pid_t pid = GPOINTER_TO_UINT (static_cast<pid_t*>(g_object_get_data (G_OBJECT (widget), "selected_info")));

  info = GsmApplication::get ().processes.find (pid);

  fill_proc_properties (builder, info);
}

static void
close_dialog_action (GSimpleAction*,
                     GVariant*,
                     GtkWindow *dialog)
{
  guint timer;

  timer = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (dialog), "timer"));
  g_source_remove (timer);

  gtk_window_destroy (dialog);
}

static void
close_procprop_dialog (GObject*,
                       gpointer data)
{
  GtkWindow *window = static_cast<GtkWindow*>(data);
  guint timer;

  timer = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (window), "timer"));
  g_source_remove (timer);

  gtk_window_destroy (window);
}

static gboolean
procprop_timer (gpointer data)
{
  GtkBuilder *builder = static_cast<GtkBuilder*>(data);

  update_procproperties_dialog (builder);

  return TRUE;
}

static void
create_single_procproperties_dialog (GtkTreeModel *model,
                                     GtkTreePath*,
                                     GtkTreeIter  *iter,
                                     gpointer      data)
{
  GsmApplication *app = static_cast<GsmApplication *>(data);

  ProcInfo *info;
  guint timer;
  GAction *action;
  GSimpleActionGroup *action_group;
  GMenuItem *item;

  gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

  if (!info)
    return;

  GtkBuilder *builder = gtk_builder_new ();
  GError *err = NULL;

  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/procproperties.ui", &err);
  if (err != NULL)
    g_error ("%s", err->message);

  GtkWindow *procpropdialog = GTK_WINDOW (gtk_builder_get_object (builder, "procprop_dialog"));
  GtkButton *stop_button = GTK_BUTTON (gtk_builder_get_object (builder, "stop_button"));
  GtkButton *force_stop_button = GTK_BUTTON (gtk_builder_get_object (builder, "force_stop_button"));
  GMenu *menu = G_MENU (gtk_builder_get_object (builder, "menu"));

  gtk_window_set_title (GTK_WINDOW (procpropdialog), info->name.c_str ());

  fill_proc_properties (builder, info);
  g_object_set_data (G_OBJECT (procpropdialog), "selected_info", GUINT_TO_POINTER (info->pid));

  action_group = g_simple_action_group_new ();

  GActionEntry win_action_entries[] = {
    { "send-signal-stop", on_activate_send_signal, "i", NULL, NULL, { 0, 0, 0 } },
    { "send-signal-cont", on_activate_send_signal, "i", NULL, NULL, { 0, 0, 0 } },
    { "send-signal-term", on_activate_send_signal, "i", NULL, NULL, { 0, 0, 0 } },
    { "send-signal-kill", on_activate_send_signal, "i", NULL, NULL, { 0, 0, 0 } },
  };

  gtk_actionable_set_action_target_value (GTK_ACTIONABLE (stop_button), g_variant_new_int32 (info->pid));
  gtk_actionable_set_action_target_value (GTK_ACTIONABLE (force_stop_button), g_variant_new_int32 (info->pid));

  item = g_menu_item_new (_("_Terminate…"), "procprop.send-signal-term");
  g_menu_item_set_attribute_value (item, G_MENU_ATTRIBUTE_TARGET, g_variant_new_int32 (info->pid));
  g_menu_append_item (menu, item);
  item = g_menu_item_new (_("_Resume"), "procprop.send-signal-cont");
  g_menu_item_set_attribute_value (item, G_MENU_ATTRIBUTE_TARGET, g_variant_new_int32 (info->pid));
  g_menu_append_item (menu, item);

  g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                   win_action_entries,
                                   G_N_ELEMENTS (win_action_entries),
                                   app);

  GSimpleAction *close_action = g_simple_action_new ("close", NULL);
  g_signal_connect (close_action, "activate", G_CALLBACK (close_dialog_action), procpropdialog);
  g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (close_action));

  gtk_widget_insert_action_group (GTK_WIDGET (procpropdialog), "procprop", G_ACTION_GROUP (action_group));

  g_signal_connect (procpropdialog, "close-request",
                    G_CALLBACK (close_procprop_dialog), procpropdialog);

  gtk_window_present (GTK_WINDOW (procpropdialog));

  timer = g_timeout_add_seconds (5, procprop_timer, builder);
  g_object_set_data (G_OBJECT (procpropdialog), "timer", GUINT_TO_POINTER (timer));

  update_procproperties_dialog (builder);
}

void
create_procproperties_dialog (GsmApplication *app)
{
  gtk_tree_selection_selected_foreach (app->selection, create_single_procproperties_dialog,
                                       app);
}
