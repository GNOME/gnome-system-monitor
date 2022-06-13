/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* Procman process actions
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
#include <errno.h>

#include <glib/gi18n.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "procactions.h"
#include "application.h"
#include "proctable.h"
#include "procdialogs.h"


static void
renice_single_process (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    const struct ProcActionArgs * const args = static_cast<ProcActionArgs*>(data);

    ProcInfo *info = NULL;
    gint error;
    int saved_errno;
    gchar *error_msg;
    GtkMessageDialog *dialog;

    gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

    if (!info)
        return;
    if (info->nice == args->arg_value)
        return;
    error = setpriority (PRIO_PROCESS, info->pid, args->arg_value);

    /* success */
    if(error != -1) return;

    saved_errno = errno;

    /* need to be root */
    if(errno == EPERM || errno == EACCES) {
        gboolean success;

        success = procdialog_create_root_password_dialog (
            PROCMAN_ACTION_RENICE, args->app, info->pid,
            args->arg_value);

        if(success) return;

        if(errno) {
            saved_errno = errno;
        }
    }

    /* failed */
    error_msg = g_strdup_printf (
        _("Cannot change the priority of process with PID %d to %d.\n"
          "%s"),
        info->pid, args->arg_value, g_strerror(saved_errno));

    dialog = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
        NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", error_msg));

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (GTK_WIDGET (dialog));
    g_free (error_msg);
}


void
renice (GsmApplication *app, int nice)
{
    struct ProcActionArgs args = { app, nice };

    /* EEEK - ugly hack - make sure the table is not updated as a crash
    ** occurs if you first kill a process and the tree node is removed while
    ** still in the foreach function
    */
    proctable_freeze (app);

    gtk_tree_selection_selected_foreach(app->selection, renice_single_process,
                                        &args);

    proctable_thaw (app);

    proctable_update (app);
}




static void
kill_single_process (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    const struct ProcActionArgs * const args = static_cast<ProcActionArgs*>(data);
    char *error_msg;
    ProcInfo *info;
    int error;
    int saved_errno;
    GtkMessageDialog *dialog;

    gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

    if (!info)
        return;

    error = kill (info->pid, args->arg_value);

    /* success */
    if(error != -1) return;

    saved_errno = errno;

    /* need to be root */
    if(errno == EPERM) {
        gboolean success;

        success = procdialog_create_root_password_dialog (
            PROCMAN_ACTION_KILL, args->app, info->pid,
            args->arg_value);

        if(success) return;

        if(errno) {
            saved_errno = errno;
        }
    }

    /* failed */
    error_msg = g_strdup_printf (
        _("Cannot kill process with PID %d with signal %d.\n"
          "%s"),
        info->pid, args->arg_value, g_strerror(saved_errno));

    dialog = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
        NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", error_msg));

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (GTK_WIDGET (dialog));
    g_free (error_msg);
}


void
kill_process (GsmApplication *app, int sig)
{
    struct ProcActionArgs args = { app, sig };

    /* EEEK - ugly hack - make sure the table is not updated as a crash
    ** occurs if you first kill a process and the tree node is removed while
    ** still in the foreach function
    */
    proctable_freeze (app);

    gtk_tree_selection_selected_foreach (app->selection, kill_single_process,
                                         &args);

    proctable_thaw (app);

    proctable_update (app);
}
