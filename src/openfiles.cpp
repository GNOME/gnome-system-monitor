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
#include "openfiles-data.h"
#include "procinfo.h"
#include "proctable.h"
#include "util.h"
#include "settings-keys.h"

#ifndef NI_IDN
const int NI_IDN = 0;
#endif

static const char*
get_type_name (enum glibtop_file_type t)
{
  switch (t)
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
friendlier_hostname (const char *addr_str,
                     int         port)
{
  struct addrinfo hints = { };
  struct addrinfo *res = NULL;
  char hostname[NI_MAXHOST];
  char service[NI_MAXSERV];
  char port_str[6];

  if (!addr_str[0])
    return g_strdup ("");

  snprintf (port_str, sizeof port_str, "%d", port);

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo (addr_str, port_str, &hints, &res))
    goto failsafe;

  if (getnameinfo (res->ai_addr, res->ai_addrlen, hostname,
                   sizeof hostname, service, sizeof service, NI_IDN))
    goto failsafe;

  if (res)
    freeaddrinfo (res);
  return g_strdup_printf ("%s, TCP port %d (%s)", hostname, port, service);

failsafe:
  if (res)
    freeaddrinfo (res);
  return g_strdup_printf ("%s, TCP port %d", addr_str, port);
}

static void
add_new_files (gpointer,
               gpointer value,
               gpointer data)
{
  glibtop_open_files_entry *openfiles;
  GListStore *store;
  char *object;

  openfiles = static_cast<glibtop_open_files_entry*>(value);
  store = static_cast<GListStore*>(data);

  switch (openfiles->type)
    {
      case GLIBTOP_FILE_TYPE_FILE:
        object = g_strdup (openfiles->info.file.name);
        break;

      case GLIBTOP_FILE_TYPE_INET6SOCKET:
      case GLIBTOP_FILE_TYPE_INETSOCKET:
        object = friendlier_hostname (openfiles->info.sock.dest_host,
                                      openfiles->info.sock.dest_port);
        break;

      case GLIBTOP_FILE_TYPE_LOCALSOCKET:
        object = g_strdup (openfiles->info.localsock.name);
        break;

      default:
        object = g_strdup ("");
    }

  OpenFilesData *openfiles_data;

  openfiles_data = openfiles_data_new (openfiles->fd,
                                       get_type_name (static_cast<glibtop_file_type>(openfiles->type)),
                                       object,
                                       g_memdup2 (openfiles, sizeof (*openfiles)));

  g_list_store_insert (store, 0, openfiles_data);

  g_free (object);
}

static GList *old_maps = NULL;

static void
classify_openfiles (GListStore *store,
                    guint       position,
                    GHashTable *new_maps)
{
  OpenFilesData *data;
  glibtop_open_files_entry *openfiles;
  gchar *old_name;

  data = OPENFILES_DATA (g_list_model_get_object (G_LIST_MODEL (store), position));
  g_object_get (data, "object", &old_name, NULL);

  openfiles = static_cast<glibtop_open_files_entry*>(g_hash_table_lookup (new_maps, old_name));

  if (openfiles)
    g_hash_table_remove (new_maps, old_name);

  old_maps = g_list_append (old_maps, data);
  g_free (old_name);
}

static gboolean
compare_open_files (gconstpointer a,
                    gconstpointer b)
{
  const glibtop_open_files_entry *o1 = static_cast<const glibtop_open_files_entry *>(a);
  const glibtop_open_files_entry *o2 = static_cast<const glibtop_open_files_entry *>(b);

  /* Falta manejar los diferentes tipos! */
  return (o1->fd == o2->fd) && (o1->type == o2->type);   /* XXX! */
}

static void
update_openfiles_dialog (GListStore *store)
{
  ProcInfo *info;
  glibtop_open_files_entry *openfiles;
  glibtop_proc_open_files procmap;
  GHashTable *new_maps;
  guint i;

  pid_t pid = GPOINTER_TO_UINT (static_cast<pid_t*>(g_object_get_data (G_OBJECT (store), "selected_info")));

  info = GsmApplication::get ().processes.find (pid);

  if (!info)
    return;

  openfiles = glibtop_get_proc_open_files (&procmap, info->pid);

  if (!openfiles)
    return;

  new_maps = static_cast<GHashTable *>(g_hash_table_new_full (g_str_hash, compare_open_files,
                                                              NULL, NULL));
  for (i = 0; i < procmap.number; i++)
    g_hash_table_insert (new_maps, openfiles + i, openfiles + i);

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (store)); i++)
    classify_openfiles (store, i, new_maps);

  g_hash_table_foreach (new_maps, add_new_files, store);

  while (old_maps)
    {
      OpenFilesData *data;
      guint position;

      data = static_cast<OpenFilesData*>(old_maps->data);
      openfiles = NULL;

      g_object_get (data, "pointer", &openfiles, NULL);

      if (g_list_store_find (store, data, &position))
        g_list_store_remove (store, position);
      g_free (openfiles);

      old_maps = g_list_next (old_maps);
    }

  g_hash_table_destroy (new_maps);
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
close_openfiles_dialog (GtkWindow *dialog,
                        gpointer)
{
  guint timer;

  timer = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (dialog), "timer"));
  g_source_remove (timer);

  gtk_window_destroy (dialog);
}

static gboolean
openfiles_timer (gpointer data)
{
  GListStore *store = static_cast<GListStore *>(data);

  g_assert (store);

  update_openfiles_dialog (store);

  return TRUE;
}

static void
create_single_openfiles_dialog (GtkTreeModel *model,
                                GtkTreePath*,
                                GtkTreeIter  *iter,
                                gpointer)
{
  AdwWindow *openfilesdialog;
  AdwWindowTitle *window_title;
  GtkColumnView *column_view;
  GListStore *store;
  ProcInfo *info;
  GSimpleActionGroup *action_group;
  guint timer;

  gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

  if (!info)
    return;

  GtkBuilder *builder = gtk_builder_new ();
  GError *err = NULL;

  gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/openfiles.ui", &err);
  if (err != NULL)
    g_error ("%s", err->message);

  openfilesdialog = ADW_WINDOW (gtk_builder_get_object (builder, "openfiles_dialog"));
  window_title = ADW_WINDOW_TITLE (gtk_builder_get_object (builder, "window_title"));
  column_view = GTK_COLUMN_VIEW (gtk_builder_get_object (builder, "openfiles_view"));
  store = G_LIST_STORE (gtk_builder_get_object (builder, "openfiles_store"));

  // Translators: process name and id
  adw_window_title_set_subtitle (window_title, g_strdup_printf (_("%s (PID %u)"), info->name.c_str (), info->pid));

  g_object_set_data (G_OBJECT (store), "selected_info", GUINT_TO_POINTER (info->pid));

  action_group = g_simple_action_group_new ();

  GSimpleAction *close_action = g_simple_action_new ("close", NULL);
  g_signal_connect (close_action, "activate", G_CALLBACK (close_dialog_action), openfilesdialog);
  g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (close_action));

  gtk_widget_insert_action_group (GTK_WIDGET (openfilesdialog), "openfiles", G_ACTION_GROUP (action_group));

  g_signal_connect (G_OBJECT (openfilesdialog), "close-request",
                    G_CALLBACK (close_openfiles_dialog), column_view);

  gtk_window_present (GTK_WINDOW (openfilesdialog));

  timer = g_timeout_add_seconds (5, openfiles_timer, store);
  g_object_set_data (G_OBJECT (openfilesdialog), "timer", GUINT_TO_POINTER (timer));

  update_openfiles_dialog (store);

  g_object_unref (G_OBJECT (builder));
}

void
create_openfiles_dialog (GsmApplication *app)
{
  gtk_tree_selection_selected_foreach (app->selection, create_single_openfiles_dialog,
                                       app);
}
