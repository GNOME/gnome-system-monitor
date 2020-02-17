/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <config.h>

#include <glib/gi18n.h>
#include <glibtop/procopenfiles.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "application.h"
#include "openfiles.h"
#include "proctable.h"
#include "util.h"
#include "settings-keys.h"
#include "legacy/treeview.h"

#ifndef NI_IDN
const int NI_IDN = 0;
#endif

enum
{
    COL_FD,
    COL_TYPE,
    COL_OBJECT,
    COL_OPENFILE_STRUCT,
    NUM_OPENFILES_COL
};


static const char*
get_type_name(enum glibtop_file_type t)
{
    switch(t)
    {
        case GLIBTOP_FILE_TYPE_FILE:
            return _("file");
        case GLIBTOP_FILE_TYPE_PIPE:
            return _("pipe");
        case GLIBTOP_FILE_TYPE_INET6SOCKET:
            return _("IPv6 network connection");
        case GLIBTOP_FILE_TYPE_INETSOCKET:
            return _("IPv4 network connection");
        case GLIBTOP_FILE_TYPE_LOCALSOCKET:
            return _("local socket");
        default:
            return _("unknown type");
    }
}



static char *
friendlier_hostname(const char *addr_str, int port)
{
    struct addrinfo hints = { };
    struct addrinfo *res = NULL;
    char hostname[NI_MAXHOST];
    char service[NI_MAXSERV];
    char port_str[6];

    if (!addr_str[0]) return g_strdup("");

    snprintf(port_str, sizeof port_str, "%d", port);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(addr_str, port_str, &hints, &res))
        goto failsafe;

    if (getnameinfo(res->ai_addr, res->ai_addrlen, hostname,
                    sizeof hostname, service, sizeof service, NI_IDN))
        goto failsafe;

    if (res) freeaddrinfo(res);
    return g_strdup_printf("%s, TCP port %d (%s)", hostname, port, service);

  failsafe:
    if (res) freeaddrinfo(res);
    return g_strdup_printf("%s, TCP port %d", addr_str, port);
}



static void
add_new_files (gpointer key, gpointer value, gpointer data)
{
    glibtop_open_files_entry *openfiles = static_cast<glibtop_open_files_entry*>(value);

    GtkTreeModel *model = static_cast<GtkTreeModel*>(data);
    GtkTreeIter row;

    char *object;

    switch(openfiles->type)
    {
        case GLIBTOP_FILE_TYPE_FILE:
            object = g_strdup(openfiles->info.file.name);
            break;

        case GLIBTOP_FILE_TYPE_INET6SOCKET:
        case GLIBTOP_FILE_TYPE_INETSOCKET:
            object = friendlier_hostname(openfiles->info.sock.dest_host,
                                         openfiles->info.sock.dest_port);
            break;

        case GLIBTOP_FILE_TYPE_LOCALSOCKET:
            object = g_strdup(openfiles->info.localsock.name);
            break;

        default:
            object = g_strdup("");
    }

    gtk_list_store_insert (GTK_LIST_STORE (model), &row, 0);
    gtk_list_store_set (GTK_LIST_STORE (model), &row,
                        COL_FD, openfiles->fd,
                        COL_TYPE, get_type_name(static_cast<glibtop_file_type>(openfiles->type)),
                        COL_OBJECT, object,
                        COL_OPENFILE_STRUCT, g_memdup(openfiles, sizeof(*openfiles)),
                        -1);

    g_free(object);
}

static GList *old_maps = NULL;

static gboolean
classify_openfiles (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    GHashTable *new_maps = static_cast<GHashTable*>(data);
    GtkTreeIter *old_iter;
    glibtop_open_files_entry *openfiles;
    gchar *old_name;

    gtk_tree_model_get (model, iter, 1, &old_name, -1);

    openfiles = static_cast<glibtop_open_files_entry*>(g_hash_table_lookup (new_maps, old_name));
    if (openfiles) {
        g_hash_table_remove (new_maps, old_name);
        g_free (old_name);
        return FALSE;

    }

    old_iter = gtk_tree_iter_copy (iter);
    old_maps = g_list_append (old_maps, old_iter);
    g_free (old_name);
    return FALSE;

}


static gboolean
compare_open_files(gconstpointer a, gconstpointer b)
{
    const glibtop_open_files_entry *o1 = static_cast<const glibtop_open_files_entry *>(a);
    const glibtop_open_files_entry *o2 = static_cast<const glibtop_open_files_entry *>(b);

    /* Falta manejar los diferentes tipos! */
    return (o1->fd == o2->fd) && (o1->type == o2->type); /* XXX! */
}


static void
update_openfiles_dialog (GsmTreeView *tree)
{
    ProcInfo *info;
    GtkTreeModel *model;
    glibtop_open_files_entry *openfiles;
    glibtop_proc_open_files procmap;
    GHashTable *new_maps;
    guint i;

    pid_t pid = GPOINTER_TO_UINT(static_cast<pid_t*>(g_object_get_data (G_OBJECT (tree), "selected_info")));
    info = GsmApplication::get()->processes.find(pid);


    if (!info)
        return;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

    openfiles = glibtop_get_proc_open_files (&procmap, info->pid);

    if (!openfiles)
        return;

    new_maps = static_cast<GHashTable *>(g_hash_table_new_full (g_str_hash, compare_open_files,
                                                                NULL, NULL));
    for (i=0; i < procmap.number; i++)
        g_hash_table_insert (new_maps, openfiles + i, openfiles + i);

    gtk_tree_model_foreach (model, classify_openfiles, new_maps);

    g_hash_table_foreach (new_maps, add_new_files, model);

    while (old_maps) {
        GtkTreeIter *iter = static_cast<GtkTreeIter*>(old_maps->data);
        glibtop_open_files_entry *openfiles = NULL;

        gtk_tree_model_get (model, iter,
                            COL_OPENFILE_STRUCT, &openfiles,
                            -1);

        gtk_list_store_remove (GTK_LIST_STORE (model), iter);
        gtk_tree_iter_free (iter);
        g_free (openfiles);

        old_maps = g_list_next (old_maps);

    }

    g_hash_table_destroy (new_maps);
    g_free (openfiles);
}

static void
close_openfiles_dialog (GtkDialog *dialog, gint id, gpointer data)
{
    GsmTreeView *tree = static_cast<GsmTreeView*>(data);
    guint timer;

    gsm_tree_view_save_state (tree);

    timer = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (tree), "timer"));
    g_source_remove (timer);

    gtk_widget_destroy (GTK_WIDGET (dialog));

    return ;
}


static GsmTreeView *
create_openfiles_tree (GsmApplication *app)
{
    GsmTreeView *tree;
    GtkListStore *model;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    gint i;

    const gchar * const titles[] = {
        /* Translators: "FD" here means "File Descriptor". Please use
           a very short translation if possible, and at most
           2-3 characters for it to be able to fit in the UI. */
        N_("FD"),
        N_("Type"),
        N_("Object")
    };

    model = gtk_list_store_new (NUM_OPENFILES_COL,
                                G_TYPE_INT,         /* FD */
                                G_TYPE_STRING,      /* Type */
                                G_TYPE_STRING,      /* Object */
                                G_TYPE_POINTER      /* open_files_entry */
        );

    auto settings = g_settings_get_child (app->settings->gobj (), GSM_SETTINGS_CHILD_OPEN_FILES);

    tree = gsm_tree_view_new (settings, FALSE);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (model));
    g_object_unref (G_OBJECT (model));

    for (i = 0; i < NUM_OPENFILES_COL-1; i++) {
        cell = gtk_cell_renderer_text_new ();

        switch (i) {
            case COL_FD:
                g_object_set(cell, "xalign", 1.0f, NULL);
                break;
        }

        column = gtk_tree_view_column_new_with_attributes (_(titles[i]),
                                                           cell,
                                                           "text", i,
                                                           NULL);
        gtk_tree_view_column_set_sort_column_id (column, i);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
    }

    gsm_tree_view_load_state (GSM_TREE_VIEW (tree));

    return tree;

}


static gboolean
openfiles_timer (gpointer data)
{
    GsmTreeView* tree = static_cast<GsmTreeView*>(data);
    GtkTreeModel *model;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
    g_assert(model);

    update_openfiles_dialog (tree);

    return TRUE;
}


static void
create_single_openfiles_dialog (GtkTreeModel *model, GtkTreePath *path,
                                GtkTreeIter *iter, gpointer data)
{
    GsmApplication *app = static_cast<GsmApplication *>(data);
    GtkDialog *openfilesdialog;
    GtkGrid *cmd_grid;
    GtkLabel *label;
    GtkScrolledWindow *scrolled;
    GsmTreeView *tree;
    ProcInfo *info;
    guint timer;

    gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

    if (!info)
        return;

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/openfiles.ui", NULL);

    openfilesdialog = GTK_DIALOG (gtk_builder_get_object (builder, "openfiles_dialog"));

    cmd_grid = GTK_GRID (gtk_builder_get_object (builder, "cmd_grid"));


    label = procman_make_label_for_mmaps_or_ofiles (
        _("_Files opened by process “%s” (PID %u):"),
        info->name.c_str(),
        info->pid);

    gtk_container_add (GTK_CONTAINER (cmd_grid), GTK_WIDGET (label));

    scrolled = GTK_SCROLLED_WINDOW (gtk_builder_get_object (builder, "scrolled"));

    tree = create_openfiles_tree (app);
    gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (tree));
    g_object_set_data (G_OBJECT (tree), "selected_info", GUINT_TO_POINTER (info->pid));

    g_signal_connect (G_OBJECT (openfilesdialog), "response",
                      G_CALLBACK (close_openfiles_dialog), tree);

    gtk_builder_connect_signals (builder, NULL);

    gtk_window_set_transient_for (GTK_WINDOW (openfilesdialog), GTK_WINDOW (GsmApplication::get()->main_window));
    gtk_widget_show_all (GTK_WIDGET (openfilesdialog));

    timer = g_timeout_add_seconds (5, openfiles_timer, tree);
    g_object_set_data (G_OBJECT (tree), "timer", GUINT_TO_POINTER (timer));

    update_openfiles_dialog (tree);

    g_object_unref (G_OBJECT (builder));
}


void
create_openfiles_dialog (GsmApplication *app)
{
    gtk_tree_selection_selected_foreach (app->selection, create_single_openfiles_dialog,
                                         app);
}
