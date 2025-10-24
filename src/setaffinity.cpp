/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: Copyright (C) 2020 Jacob Barkdull
 */

#include "config.h"

#include <glib/gi18n.h>

#include <sys/types.h>
#include <unistd.h>
#include <glibtop/procaffinity.h>
#include <sys/stat.h>

#include "gsm-setaffinity.h"

#include "application.h"
#include "procdialogs.h"
#include "procinfo.h"
#include "proctable.h"
#include "util.h"
#include "setaffinity.h"

namespace
{
class SetAffinityData
{
public:
GtkWidget  *dialog;
pid_t pid;
GtkWidget **buttons;
GtkWidget *all_threads_row;
guint32 cpu_count;
gboolean toggle_single_blocked;
gboolean toggle_all_blocked;
};
}

static gboolean
all_toggled (SetAffinityData *affinity)
{
  guint32 i;

  /* Check if any CPU's aren't set for this process */
  for (i = 1; i <= affinity->cpu_count; i++)
    /* If so, return FALSE */
    if (!adw_switch_row_get_active (ADW_SWITCH_ROW (affinity->buttons[i])))
      return FALSE;

  return TRUE;
}

static void
affinity_toggler_single (GObject    *self,
                         GParamSpec *,
                         gpointer    data)
{
  SetAffinityData *affinity = static_cast<SetAffinityData *>(data);
  gboolean toggle_all_state = FALSE;

  /* Return void if toggle single is blocked */
  if (affinity->toggle_single_blocked == TRUE)
    return;

  /* Set toggle all state based on whether all are toggled */
  if (adw_switch_row_get_active (ADW_SWITCH_ROW (self)))
    toggle_all_state = all_toggled (affinity);

  /* Block toggle all signal */
  affinity->toggle_all_blocked = TRUE;

  /* Set toggle all check box state */
  adw_switch_row_set_active (ADW_SWITCH_ROW (affinity->buttons[0]),
                             toggle_all_state);

  /* Unblock toggle all signal */
  affinity->toggle_all_blocked = FALSE;
}

static void
affinity_toggle_all (GObject    *self,
                     GParamSpec *,
                     gpointer    data)
{
  SetAffinityData *affinity = static_cast<SetAffinityData *>(data);

  guint32 i;
  gboolean state;

  /* Return void if toggle all is blocked */
  if (affinity->toggle_all_blocked == TRUE)
    return;

  /* Set individual CPU toggles based on toggle all state */
  state = adw_switch_row_get_active (ADW_SWITCH_ROW (self));

  /* Block toggle single signal */
  affinity->toggle_single_blocked = TRUE;

  /* Set all CPU check boxes to specified state */
  for (i = 1; i <= affinity->cpu_count; i++)
    adw_switch_row_set_active (
      ADW_SWITCH_ROW (affinity->buttons[i]),
      state);

  /* Unblock toggle single signal */
  affinity->toggle_single_blocked = FALSE;
}


static void
set_affinity_error (GError *error)
{
  AdwDialog *dialog;

  /* Create error alert dialog */
  dialog = adw_alert_dialog_new (_("GNU CPU Affinity error"),
                                   NULL);

  if (error) {
    adw_alert_dialog_set_body (ADW_ALERT_DIALOG (dialog), error->message);
  } else {
    adw_alert_dialog_format_body (ADW_ALERT_DIALOG (dialog), "%s", g_strerror (errno));
  }

  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "close", _("_Close"));

  /* Show the dialog */
  adw_dialog_present (dialog, GTK_WIDGET (GsmApplication::get ().main_window));
}


static void
execute_taskset_command (gchar  **cpu_list,
                         pid_t    pid,
                         gboolean child_threads)
{
#ifdef __linux__
  gchar *pc;
  gchar *command;

  /* Join CPU number strings by commas for taskset command CPU list */
  pc = g_strjoinv (",", cpu_list);

  /* Construct taskset command */
  command = g_strdup_printf ("taskset -pc%c %s %d", child_threads ? 'a' : ' ', pc, pid);

  /* Execute taskset command; show error on failure */
  if (!multi_root_check (command))
    set_affinity_error (NULL);

  /* Free memory for taskset command */
  g_free (command);
  g_free (pc);
#endif
}

static void
set_affinity (GtkCheckButton*,
              gpointer data)
{
  g_autoptr (GError) error = NULL;
  SetAffinityData *affinity = static_cast<SetAffinityData *>(data);

  glibtop_proc_affinity get_affinity;

  gchar   **cpu_list;
  GArray   *cpuset;
  guint16  *cpus;
  guint32 i;
  gint taskset_cpu = 0;
  gboolean all_threads;

  /* Create string array for taskset command CPU list */
  cpu_list = g_new0 (gchar *, affinity->cpu_count);

  /* Check whether we can get process's current affinity */
  cpus = glibtop_get_proc_affinity (&get_affinity, affinity->pid);
  if (cpus != NULL)
    {
      g_free (cpus);

      /* If so, create array for CPU numbers */
      cpuset = g_array_new (FALSE, FALSE, sizeof (guint16));

      /* Run through all CPUs set for this process */
      for (i = 0; i < affinity->cpu_count; i++)
        /* Check if CPU check box button is active */
        if (adw_switch_row_get_active (ADW_SWITCH_ROW (affinity->buttons[i + 1])))
          {
            /* If so, get its CPU number as 16bit integer */
            guint16 n = i;

            /* Add its CPU for process affinity */
            g_array_append_val (cpuset, n);

            /* Store CPU number as string for taskset command CPU list */
            cpu_list[taskset_cpu] = g_strdup_printf ("%i", i);
            taskset_cpu++;
          }

      all_threads = adw_switch_row_get_active (ADW_SWITCH_ROW (affinity->all_threads_row));

      /* Set process affinity; Show message dialog upon error */
      gsm_setaffinity (affinity->pid,
                       cpuset->len,
                       (uint16_t *) cpuset->data,
                       all_threads,
                       &error);
      if (error) {
        /* If so, check whether an access error occurred */
        if (g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_PERM) ||
            g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_ACCES)) {
          /* If so, attempt to run taskset as root, show error on failure */
          execute_taskset_command (cpu_list, affinity->pid, all_threads);
        } else {
          /* If not, show error immediately */
          set_affinity_error (error);
        }
      }

      /* Free memory for CPU strings */
      for (i = 0; i < affinity->cpu_count; i++)
        g_free (cpu_list[i]);

      /* Free CPU array memory */
      g_array_free (cpuset, TRUE);
    }
  else
    {
      /* If not, show error message dialog */
      set_affinity_error (NULL);
    }

  /* Destroy dialog window */
  gtk_window_destroy (GTK_WINDOW (affinity->dialog));
}

static void
create_single_set_affinity_dialog (GtkTreeModel *model,
                                   GtkTreePath*,
                                   GtkTreeIter  *iter,
                                   gpointer      data)
{
  GsmApplication *app = static_cast<GsmApplication *>(data);

  ProcInfo        *info;
  SetAffinityData *affinity_data;
  AdwWindowTitle  *window_title;
  GtkWidget       *cancel_button;
  GtkWidget       *apply_button;
  GtkListBox      *cpulist_box;

  guint16               *affinity_cpus;
  guint16 affinity_cpu;
  glibtop_proc_affinity affinity;
  guint32 affinity_i;
  gint button_n;
  gchar                 *button_text;

  /* Get selected process information */
  gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

  /* Return void if process information comes back not true */
  if (!info)
    return;

  /* Create affinity data object */
  affinity_data = new SetAffinityData ();

  /* Set initial check box array */
  affinity_data->buttons = g_new (GtkWidget *, app->config.num_cpus);

  GtkBuilder *builder = gtk_builder_new ();
  GError *err = NULL;

  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/setaffinity.ui", &err);
  if (err != NULL)
    g_error ("%s", err->message);

  affinity_data->dialog = GTK_WIDGET (gtk_builder_get_object (builder, "setaffinity_dialog"));
  window_title = ADW_WINDOW_TITLE (gtk_builder_get_object (builder, "window_title"));
  cpulist_box = GTK_LIST_BOX (gtk_builder_get_object (builder, "cpulist_box"));
  affinity_data->buttons[0] = GTK_WIDGET (gtk_builder_get_object (builder, "allcpus_row"));
  cancel_button = GTK_WIDGET (gtk_builder_get_object (builder, "cancel_button"));
  apply_button = GTK_WIDGET (gtk_builder_get_object (builder, "apply_button"));
  affinity_data->all_threads_row = GTK_WIDGET (gtk_builder_get_object (builder, "all_threads_row"));

  // Translators: process name and id
  char *subtitle = g_strdup_printf (_("%s (PID %u)"), info->name.c_str (), info->pid);
  adw_window_title_set_subtitle (window_title, subtitle);
  g_free (subtitle);

  /* Add selected process pid to affinity data */
  affinity_data->pid = info->pid;

  /* Add CPU count to affinity data */
  affinity_data->cpu_count = app->config.num_cpus;

  /* Set default toggle signal block states */
  affinity_data->toggle_single_blocked = FALSE;
  affinity_data->toggle_all_blocked = FALSE;

  /* Get process's current affinity */
  affinity_cpus = glibtop_get_proc_affinity (&affinity, info->pid);

  /* Set toggle all check box based on whether all CPUs are set for this process */
  adw_switch_row_set_active (
    ADW_SWITCH_ROW (affinity_data->buttons[0]),
    affinity.all);

  /* Run through all CPU buttons */
  for (button_n = 1; button_n < app->config.num_cpus + 1; button_n++)
    {
      /* Set check box label value to CPU [1..2048] */
      button_text = g_strdup_printf (_("CPU%d"), button_n);

      /* Create check box button for current CPU */
      affinity_data->buttons[button_n] = adw_switch_row_new ();
      adw_preferences_row_set_title (ADW_PREFERENCES_ROW (affinity_data->buttons[button_n]), button_text);

      /* Add check box to CPU list */
      gtk_list_box_insert (cpulist_box, affinity_data->buttons[button_n], button_n);

      /* Connect check box to toggler function */
      g_signal_connect (affinity_data->buttons[button_n],
                        "notify::active",
                        G_CALLBACK (affinity_toggler_single),
                        affinity_data);

      /* Free check box label value gchar */
      g_free (button_text);
    }

  /* Run through all CPUs set for this process */
  for (affinity_i = 0; affinity_i < affinity.number; affinity_i++)
    {
      /* Get CPU button index */
      affinity_cpu = affinity_cpus[affinity_i] + 1;

      /* Set CPU check box active */
      adw_switch_row_set_active (
        ADW_SWITCH_ROW (affinity_data->buttons[affinity_cpu]),
        TRUE);
    }

  /* Swap click signal on "Cancel" button */
  g_signal_connect_swapped (cancel_button,
                            "clicked",
                            G_CALLBACK (gtk_window_destroy),
                            affinity_data->dialog);

  /* Connect click signal on "Apply" button */
  g_signal_connect (apply_button,
                    "clicked",
                    G_CALLBACK (set_affinity),
                    affinity_data);

  /* Connect toggle all check box to (de)select all function */
  g_signal_connect (affinity_data->buttons[0],
                    "notify::active",
                    G_CALLBACK (affinity_toggle_all),
                    affinity_data);

  /* Show dialog window */
  gtk_window_present (GTK_WINDOW (affinity_data->dialog));

  g_object_unref (G_OBJECT (builder));

  g_free (affinity_cpus);
}

void
create_set_affinity_dialog (GsmApplication *app)
{
  /* Create a dialog window for each selected process */
  gtk_tree_selection_selected_foreach (app->selection,
                                       create_single_set_affinity_dialog,
                                       app);
}
