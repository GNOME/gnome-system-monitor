/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>

#include <glibtop/procopenfiles.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "procinfo.h"

#include "open-file.h"


#ifndef NI_IDN
const int NI_IDN = 0;
#endif


struct _GsmOpenFile {
  GObject parent_instance;

  ProcInfo *info;
  glibtop_open_files_entry *entry;

  pid_t pid;
  char *process;
  GdkPaintable *icon;

  int fd;
  char *type;
  char *object;
};


G_DEFINE_FINAL_TYPE (GsmOpenFile, gsm_open_file, G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_INFO,
  PROP_ENTRY,
  PROP_PID,
  PROP_PROCESS,
  PROP_ICON,
  PROP_FD,
  PROP_TYPE,
  PROP_OBJECT,
  N_PROPS
};
static GParamSpec *properties[N_PROPS];


static void
gsm_open_file_dispose (GObject *object)
{
  GsmOpenFile *self = GSM_OPEN_FILE (object);

  g_clear_pointer (&self->entry, g_free);
  g_clear_pointer (&self->process, g_free);
  g_clear_object (&self->icon);
  g_clear_pointer (&self->type, g_free);
  g_clear_pointer (&self->object, g_free);

  G_OBJECT_CLASS (gsm_open_file_parent_class)->dispose (object);
}


static void
gsm_open_file_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GsmOpenFile *self = GSM_OPEN_FILE (object);

  switch (property_id) {
    case PROP_INFO:
      g_value_set_pointer (value, self->info);
      break;
    case PROP_ENTRY:
      g_value_set_boxed (value, self->entry);
      break;
    case PROP_PID:
      g_value_set_int (value, self->pid);
      break;
    case PROP_PROCESS:
      g_value_set_string (value, self->process);
      break;
    case PROP_ICON:
      g_value_set_object (value, self->icon);
      break;
    case PROP_FD:
      g_value_set_int (value, self->fd);
      break;
    case PROP_TYPE:
      g_value_set_string (value, self->type);
      break;
    case PROP_OBJECT:
      g_value_set_string (value, self->object);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static inline void
set_info (GsmOpenFile *self, ProcInfo *info)
{
  g_return_if_fail (GSM_IS_OPEN_FILE (self));

  if (info == NULL) {
    self->pid = 0;
    g_clear_pointer (&self->process, g_free);
    g_clear_object (&self->icon);
    self->info = NULL;
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PID]);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROCESS]);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INFO]);
    return;
  }

  g_object_freeze_notify (G_OBJECT (self));

  if (self->pid != info->pid) {
    self->pid = info->pid;
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PID]);
  }

  if (g_set_str (&self->process, info->name.c_str ())) {
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROCESS]);
  }

  if (g_set_object (&self->icon, GDK_PAINTABLE (info->icon->gobj ()))) {
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
  }

  self->info = info;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INFO]);

  g_object_thaw_notify (G_OBJECT (self));
}


static void
gsm_open_file_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GsmOpenFile *self = GSM_OPEN_FILE (object);

  switch (property_id) {
    case PROP_INFO:
      set_info (self, static_cast<ProcInfo *>(g_value_get_pointer (value)));
      break;
    case PROP_ENTRY:
      gsm_open_file_set_entry (self,
                               static_cast<glibtop_open_files_entry *>(g_value_get_boxed (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
gsm_open_file_class_init (GsmOpenFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gsm_open_file_dispose;
  object_class->get_property = gsm_open_file_get_property;
  object_class->set_property = gsm_open_file_set_property;

  properties[PROP_INFO] =
    g_param_spec_pointer ("info", NULL, NULL,
                          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties[PROP_ENTRY] =
    g_param_spec_boxed ("entry", NULL, NULL,
                        glibtop_open_files_entry_get_type (),
                        (GParamFlags) (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties[PROP_PID] =
    g_param_spec_int ("pid", NULL, NULL,
                      0, G_MAXINT, 0,
                      (GParamFlags) (G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties[PROP_PROCESS] =
    g_param_spec_string ("process", NULL, NULL,
                         NULL,
                         (GParamFlags) (G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties[PROP_ICON] =
    g_param_spec_object ("icon", NULL, NULL,
                         GDK_TYPE_PAINTABLE,
                         (GParamFlags) (G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties[PROP_FD] =
    g_param_spec_int ("fd", NULL, NULL,
                      0, G_MAXINT, 0,
                      (GParamFlags) (G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties[PROP_TYPE] =
    g_param_spec_string ("type", NULL, NULL,
                         NULL,
                         (GParamFlags) (G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));
  properties[PROP_OBJECT] =
    g_param_spec_string ("object", NULL, NULL,
                         NULL,
                         (GParamFlags) (G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}


static void
gsm_open_file_init (GsmOpenFile *)
{
}


GsmOpenFile *
gsm_open_file_new (ProcInfo                 *info,
                   glibtop_open_files_entry *entry)
{
  return GSM_OPEN_FILE (g_object_new (GSM_TYPE_OPEN_FILE,
                                      "info", info,
                                      "entry", entry,
                                      NULL));
}


static const char *
get_type_name (enum glibtop_file_type t)
{
  switch (t) {
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


static char *
describe_entry (glibtop_open_files_entry *entry)
{
  switch (entry->type) {
    case GLIBTOP_FILE_TYPE_FILE:
      return g_strdup (entry->info.file.name);
    case GLIBTOP_FILE_TYPE_INET6SOCKET:
    case GLIBTOP_FILE_TYPE_INETSOCKET:
      return friendlier_hostname (entry->info.sock.dest_host,
                                  entry->info.sock.dest_port);
    case GLIBTOP_FILE_TYPE_LOCALSOCKET:
      return g_strdup (entry->info.localsock.name);
  }

  return NULL;
}


void
gsm_open_file_set_entry (GsmOpenFile              *self,
                         glibtop_open_files_entry *entry)
{
  g_autofree char *description = NULL;

  g_return_if_fail (GSM_IS_OPEN_FILE (self));

  if (entry == NULL && self->entry == NULL) {
    return;
  }

  if (entry == NULL) {
    self->fd = 0;
    g_clear_pointer (&self->entry, g_free);
    g_clear_pointer (&self->type, g_free);
    g_clear_pointer (&self->object, g_free);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FD]);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TYPE]);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_OBJECT]);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENTRY]);
    return;
  }

  description = describe_entry (entry);

  g_object_freeze_notify (G_OBJECT (self));

  if (self->fd != entry->fd) {
    self->fd = entry->fd;
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FD]);
  }

  if (g_set_str (&self->type,
                 get_type_name (static_cast<glibtop_file_type>(entry->type)))) {
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TYPE]);
  }

  if (g_set_str (&self->object, description)) {
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_OBJECT]);
  }

  g_free (self->entry);
  self->entry =
    static_cast<glibtop_open_files_entry *>(g_boxed_copy (glibtop_open_files_entry_get_type (),
                                                          static_cast<gconstpointer>(entry)));
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENTRY]);

  g_object_thaw_notify (G_OBJECT (self));
}
