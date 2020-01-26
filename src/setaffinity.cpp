/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/**
 * Copyright (C) 2020 Jacob Barkdull
 *
 * This program is part of GNOME System Monitor.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
**/


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <sys/stat.h>
#include <glib/gi18n.h>

#include "proctable.h"
#include "procdialogs.h"
#include "util.h"
#include "setaffinity.h"

namespace
{
    class SetAffinityData
    {
        public:
            GtkWidget  *dialog;
            pid_t       pid;
            GtkWidget **buttons;
            gdouble     cpu_count;
            gboolean    toggle_single_blocked;
            gboolean    toggle_all_blocked;
    };
}

static gboolean
all_toggled (SetAffinityData *affinity)
{
    gint i;

    /* Check if any CPU's aren't set for this process */
    for (i = 1; i <= affinity->cpu_count; i++) {
        /* If so, return FALSE */
        if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (affinity->buttons[i]))) {
            return FALSE;
        }
    }

    return TRUE;
}

static void
affinity_toggler_single (GtkToggleButton *button,
                         gpointer         data)
{
    SetAffinityData *affinity = static_cast<SetAffinityData *>(data);
    gboolean         toggle_all_state = FALSE;

    /* Return void if toggle single is blocked */
    if (affinity->toggle_single_blocked == TRUE) {
        return;
    }

    /* Set toggle all state based on whether all are toggled */
    if (gtk_toggle_button_get_active (button)) {
        toggle_all_state = all_toggled (affinity);
    }

    /* Block toggle all signal */
    affinity->toggle_all_blocked = TRUE;

    /* Set toggle all check box state */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (affinity->buttons[0]),
                                  toggle_all_state);

    /* Unblock toggle all signal */
    affinity->toggle_all_blocked = FALSE;
}

static void
affinity_toggle_all (GtkToggleButton *toggle_all_button,
                     gpointer         data)
{
    SetAffinityData *affinity = static_cast<SetAffinityData *>(data);

    gint     i;
    gboolean state;

    /* Return void if toggle all is blocked */
    if (affinity->toggle_all_blocked == TRUE) {
        return;
    }

    /* Set individual CPU toggles based on toggle all state */
    state = gtk_toggle_button_get_active (toggle_all_button);

    /* Block toggle single signal */
    affinity->toggle_single_blocked = TRUE;

    /* Set all CPU check boxes to specified state */
    for (i = 1; i <= affinity->cpu_count; i++) {
        gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (affinity->buttons[i]),
            state
        );
    }

    /* Unblock toggle single signal */
    affinity->toggle_single_blocked = FALSE;
}

static void
set_affinity_error (void)
{
    GtkWidget *dialog;

    /* Create error message dialog */
    dialog = gtk_message_dialog_new (GTK_WINDOW (GsmApplication::get()->main_window),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "GNU CPU Affinity error: %s",
                                     g_strerror (errno));

    /* Set dialog as modal */
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

    /* Show the dialog */
    gtk_widget_show (dialog);

    /* Connect response signal to GTK widget destroy function */
    g_signal_connect_swapped (dialog,
                              "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
}

static void
set_affinity (GtkToggleButton *button,
              gpointer         data)
{
    SetAffinityData *affinity = static_cast<SetAffinityData *>(data);

    cpu_set_t   cpuset;
    gint        i;
    gint        taskset_cpu = 0;
    gchar     **cpu_list;
    gchar      *pc;
    gchar      *command;

    /* Create emtpy struct type */
    cpu_list = g_new0 (gchar *, affinity->cpu_count);

    /* Check whether we can get process's current affinity */
    if (sched_getaffinity (affinity->pid, sizeof (cpu_set_t), &cpuset) != -1) {
        /* If so, run throguh all CPUs set for this process */
        for (i = 0; i < affinity->cpu_count; i++) {
            /* Check if CPU check box button is active */
            if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (affinity->buttons[i + 1]))) {
                /* If so, add its CPU for process affinity */
                CPU_SET (i, &cpuset);

                /* Store CPU for taskset command in case root is needed */
                cpu_list[taskset_cpu] = g_strdup_printf ("%i", i);
                taskset_cpu++;
            } else {
                /* If not, remove its CPU from process affinity */
                CPU_CLR (i, &cpuset);
            }
        }

        /* Construct taskset command */
        pc = g_strjoinv (",", cpu_list);
        command = g_strdup_printf ("taskset -pc %s %d", pc, affinity->pid);

        /* Set process affinity; Show message dialog upon error */
        if (sched_setaffinity (affinity->pid, sizeof (cpu_set_t), &cpuset) == -1) {
            /* If so, check whether an access error occurred */
            if (errno == EPERM or errno == EACCES) {
                /* If so, attempt to run taskset as root, show error on failure */
                if (!multi_root_check (command)) {
                    set_affinity_error ();
                }
            } else {
                /* If not, show error immediately */
                set_affinity_error ();
            }
        }

        /* Free memory for taskset command */
        g_free (command);
        g_free (pc);
    } else {
        /* If not, show error message dialog */
        set_affinity_error ();
    }

    /* Destroy dialog window */
    gtk_widget_destroy (affinity->dialog);
}

static void
create_single_set_affinity_dialog (GtkTreeModel *model,
                                   GtkTreePath  *path,
                                   GtkTreeIter  *iter,
                                   gpointer      data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);

    ProcInfo        *info;
    SetAffinityData *affinity_data;
    GtkWidget       *cancel_button;
    GtkWidget       *apply_button;
    GtkWidget       *dialog_vbox;
    GtkWidget       *label;
    GtkWidget       *scrolled;
    GtkStyleContext *scrolled_style;
    GtkGrid         *cpulist_grid;

    cpu_set_t        cpuset;
    gint             i;
    gint             button_n;
    gchar           *button_text;

    /* Get selected process information */
    gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

    /* Return void if process information comes back not true */
    if (!info) {
        return;
    }

    /* Create affinity data object */
    affinity_data = new SetAffinityData ();

    /* Set initial check box array */
    affinity_data->buttons = g_new (GtkWidget *, app->config.num_cpus);

    /* Create dialog window */
    affinity_data->dialog = GTK_WIDGET (g_object_new (GTK_TYPE_DIALOG,
                                                      "title", _("Set Affinity"),
                                                      "use-header-bar", TRUE,
                                                      "destroy-with-parent", TRUE,
                                                      NULL));

    /* Add cancel button to header bar */
    cancel_button = gtk_dialog_add_button (GTK_DIALOG (affinity_data->dialog),
                                           _("_Cancel"),
                                           GTK_RESPONSE_CANCEL);

    /* Add apply button to header bar */
    apply_button = gtk_dialog_add_button (GTK_DIALOG (affinity_data->dialog),
                                          _("_Apply"),
                                          GTK_RESPONSE_APPLY);

    /* Set dialog window "transient for" */
    gtk_window_set_transient_for (GTK_WINDOW (affinity_data->dialog),
                                  GTK_WINDOW (GsmApplication::get()->main_window));

    /* Set dialog window to be resizable */
    gtk_window_set_resizable (GTK_WINDOW (affinity_data->dialog), TRUE);

    /* Set default dialog window size */
    gtk_widget_set_size_request (affinity_data->dialog, 600, 430);

    /* Set dialog box padding ("border") */
    gtk_container_set_border_width (GTK_CONTAINER (affinity_data->dialog), 5);

    /* Get dialog content area VBox */
    dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG (affinity_data->dialog));

    /* Set dialog VBox padding ("border") */
    gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 10);

    /* Set dialog VBox spacing */
    gtk_box_set_spacing (GTK_BOX (dialog_vbox), 10);

    /* Add selected process pid to affinity data */
    affinity_data->pid = info->pid;

    /* Add CPU count to affinity data */
    affinity_data->cpu_count = app->config.num_cpus;

    /* Set default toggle signal block states */
    affinity_data->toggle_single_blocked = FALSE;
    affinity_data->toggle_all_blocked = FALSE;

    /* Create a label describing the dialog windows intent */
    label = GTK_WIDGET (procman_make_label_for_mmaps_or_ofiles (
        _("Select CPUs \"%s\" (PID %u) is allowed to run on:"),
        info->name.c_str(),
        info->pid
    ));

    /* Add label to dialog VBox */
    gtk_box_pack_start (GTK_BOX (dialog_vbox), label, FALSE, TRUE, 0);

    /* Create scrolled box ("window") */
    scrolled = gtk_scrolled_window_new (NULL, NULL);

    /* Add view class to scrolled box style */
    scrolled_style = gtk_widget_get_style_context (scrolled);
    gtk_style_context_add_class (scrolled_style, "view");

    /* Set scrolled box vertical and horizontal policies */
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);

    /* Set scrolled box shadow */
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                         GTK_SHADOW_IN);

    /* Create grid for CPU list */
    cpulist_grid = GTK_GRID (gtk_grid_new ());

    /* Set CPU list grid padding ("border") */
    gtk_container_set_border_width (GTK_CONTAINER (cpulist_grid), 10);

    /* Set grid row spacing */
    gtk_grid_set_row_spacing (cpulist_grid, 10);

    /* Create toggle all check box */
    affinity_data->buttons[0] = gtk_check_button_new_with_label ("Run on all CPUs");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (affinity_data->buttons[0]), TRUE);
    gtk_widget_set_hexpand (affinity_data->buttons[0], TRUE);

    /* Get process's current affinity */
    sched_getaffinity (info->pid, sizeof (cpu_set_t), &cpuset);

    /* Check if any CPU's aren't set for this process */
    for (i = 0; i < app->config.num_cpus; i++) {
        if (!CPU_ISSET (i, &cpuset)) {
            /* If so, set the check box inactive */
            gtk_toggle_button_set_active (
                GTK_TOGGLE_BUTTON (affinity_data->buttons[0]),
                FALSE
            );

            break;
        }
    }

    /* Add toggle all check box to CPU grid */
    gtk_grid_attach (cpulist_grid, affinity_data->buttons[0], 0, 0, 1, 1);

    /* Run through all CPU buttons */
    for (i = 0, button_n = 1; i < app->config.num_cpus; i++, button_n++) {
        /* Set check box label value to CPU [1..2048] */
        button_text = g_strdup_printf (_("CPU %d"), button_n);

        /* Create check box button for current CPU */
        affinity_data->buttons[button_n] = gtk_check_button_new_with_label (button_text);
        gtk_widget_set_hexpand (affinity_data->buttons[button_n], TRUE);

        /* Check if this CPU is set for this process */
        if (CPU_ISSET (i, &cpuset)) {
            /* If so, set the check box active */
            gtk_toggle_button_set_active (
                GTK_TOGGLE_BUTTON (affinity_data->buttons[button_n]),
                TRUE
            );
        }

        /* Add check box to CPU grid */
        gtk_grid_attach (cpulist_grid, affinity_data->buttons[button_n], 0, button_n, 1, 1);

        /* Connect check box to toggler function */
        g_signal_connect (affinity_data->buttons[button_n],
                          "toggled",
                          G_CALLBACK (affinity_toggler_single),
                          affinity_data);

        /* Free check box label value gchar */
        g_free (button_text);
    }

    /* Add CPU grid to scrolled box */
    gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (cpulist_grid));

    /* Add scrolled box to dialog VBox */
    gtk_box_pack_start (GTK_BOX (dialog_vbox), scrolled, TRUE, TRUE, 0);

    /* Swap click signal on "Cancel" button */
    g_signal_connect_swapped (cancel_button,
                              "clicked",
                              G_CALLBACK (gtk_widget_destroy),
                              affinity_data->dialog);

    /* Connect click signal on "Apply" button */
    g_signal_connect (apply_button,
                      "clicked",
                      G_CALLBACK (set_affinity),
                      affinity_data);

    /* Connect toggle all check box to (de)select all function */
    g_signal_connect (affinity_data->buttons[0],
                      "toggled",
                      G_CALLBACK (affinity_toggle_all),
                      affinity_data);

    /* Show dialog window */
    gtk_widget_show_all (affinity_data->dialog);
}

void
create_set_affinity_dialog (GsmApplication *app)
{
    /* Create a dialog window for each selected process */
    gtk_tree_selection_selected_foreach (app->selection,
                                         create_single_set_affinity_dialog,
                                         app);
}
