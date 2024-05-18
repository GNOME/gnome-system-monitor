#pragma once

#include <gio/gio.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define PROCTABLE_TYPE_DATA (proctable_data_get_type())

G_DECLARE_FINAL_TYPE (ProctableData, proctable_data, PROCTABLE, DATA, GObject)

ProctableData *proctable_data_new (GdkPaintable *icon,
                                   const gchar  *name,
                                   const gchar  *user,
                                   const gchar  *status,
                                   const gchar  *vmsize,
                                   const gchar  *resident_memory,
                                   gulong        writable_memory,
                                   const gchar  *shared_memory,
                                   gulong        x_server_memory,
                                   const gchar  *cpu,
                                   const gchar  *cpu_time,
                                   const gchar  *started,
                                   gint          nice,
                                   guint         id,
                                   const gchar  *security_context,
                                   const gchar  *args,
                                   const gchar  *memory,
                                   const gchar  *waiting_channel,
                                   const gchar  *control_group,
                                   const gchar  *unit,
                                   const gchar  *session,
                                   const gchar  *seat,
                                   const gchar  *owner,
                                   const gchar  *disk_read_total,
                                   const gchar  *disk_write_total,
                                   const gchar  *disk_read,
                                   const gchar  *disk_write,
                                   const gchar  *priority,
                                   const gchar  *tooltip,
                                   gpointer      pointer);

G_END_DECLS
