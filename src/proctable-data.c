#include <glib/gi18n.h>

#include "proctable-data.h"

struct _ProctableData
{
  GObject parent_instance;

  GdkPaintable *icon;
  gchar        *name;
  gchar        *user;
  gchar        *status;
  gchar        *vmsize;
  gchar        *resident_memory;
  gulong        writable_memory;
  gchar        *shared_memory;
  gulong        x_server_memory;
  gchar        *cpu;
  gchar        *cpu_time;
  gchar        *started;
  gint          nice;
  guint         id;
  gchar        *security_context;
  gchar        *args;
  gchar        *memory;
  gchar        *waiting_channel;
  gchar        *control_group;
  gchar        *unit;
  gchar        *session;
  gchar        *seat;
  gchar        *owner;
  gchar        *disk_read_total;
  gchar        *disk_write_total;
  gchar        *disk_read;
  gchar        *disk_write;
  gchar        *priority;
  gchar        *tooltip;
  gpointer      pointer;
};

G_DEFINE_TYPE (ProctableData, proctable_data, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_ICON,
  PROP_NAME,
  PROP_USER,
  PROP_STATUS,
  PROP_VMSIZE,
  PROP_RESIDENT_MEMORY,
  PROP_WRITABLE_MEMORY,
  PROP_SHARED_MEMORY,
  PROP_X_SERVER_MEMORY,
  PROP_CPU,
  PROP_CPU_TIME,
  PROP_STARTED,
  PROP_NICE,
  PROP_ID,
  PROP_SECURITY_CONTEXT,
  PROP_ARGS,
  PROP_MEMORY,
  PROP_WAITING_CHANNEL,
  PROP_CONTROL_GROUP,
  PROP_UNIT,
  PROP_SESSION,
  PROP_SEAT,
  PROP_OWNER,
  PROP_DISK_READ_TOTAL,
  PROP_DISK_WRITE_TOTAL,
  PROP_DISK_READ,
  PROP_DISK_WRITE,
  PROP_PRIORITY,
  PROP_TOOLTIP,
  PROP_POINTER,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

ProctableData *
proctable_data_new (GdkPaintable *icon,
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
                    gpointer      pointer)
{
  return g_object_new (PROCTABLE_TYPE_DATA,
                       "icon", icon,
                       "name", name,
                       "user", user,
                       "status", status,
                       "vmsize", vmsize,
                       "resident-memory", resident_memory,
                       "writable-memory", writable_memory,
                       "shared-memory", shared_memory,
                       "x-server-memory", x_server_memory,
                       "cpu", cpu,
                       "cpu-time", cpu_time,
                       "started", started,
                       "nice", nice,
                       "id", id,
                       "security-context", security_context,
                       "args", args,
                       "memory", memory,
                       "waiting_channel", waiting_channel,
                       "control-group", control_group,
                       "unit", unit,
                       "session", session,
                       "seat", seat,
                       "owner", owner,
                       "disk-read-total", disk_read_total,
                       "disk-write-total", disk_write_total,
                       "disk-read", disk_read,
                       "disk-write", disk_write,
                       "priority", priority,
                       "tooltip", tooltip,
                       "pointer", pointer,
                       NULL);
}

static void
proctable_data_finalize (GObject *object)
{
  G_OBJECT_CLASS (proctable_data_parent_class)->finalize (object);
}

static void
proctable_data_get_property (GObject    *object,
                             guint       pid,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ProctableData *self = PROCTABLE_DATA(object);

  switch (pid)
    {
    case PROP_ICON:
      g_value_set_object (value, self->icon);
      break;
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_USER:
      g_value_set_string (value, self->user);
      break;
    case PROP_STATUS:
      g_value_set_string (value, self->status);
      break;
    case PROP_VMSIZE:
      g_value_set_string (value, self->vmsize);
      break;
    case PROP_RESIDENT_MEMORY:
      g_value_set_string (value, self->resident_memory);
      break;
    case PROP_WRITABLE_MEMORY:
      g_value_set_ulong (value, self->writable_memory);
      break;
    case PROP_SHARED_MEMORY:
      g_value_set_string (value, self->shared_memory);
      break;
    case PROP_X_SERVER_MEMORY:
      g_value_set_ulong (value, self->x_server_memory);
      break;
    case PROP_CPU:
      g_value_set_string (value, self->cpu);
      break;
    case PROP_CPU_TIME:
      g_value_set_string (value, self->cpu_time);
      break;
    case PROP_STARTED:
      g_value_set_string (value, self->started);
      break;
    case PROP_NICE:
      g_value_set_int (value, self->nice);
      break;
    case PROP_ID:
      g_value_set_uint (value, self->id);
      break;
    case PROP_SECURITY_CONTEXT:
      g_value_set_string (value, self->security_context);
      break;
    case PROP_ARGS:
      g_value_set_string (value, self->args);
      break;
    case PROP_MEMORY:
      g_value_set_string (value, self->memory);
      break;
    case PROP_WAITING_CHANNEL:
      g_value_set_string (value, self->waiting_channel);
      break;
    case PROP_CONTROL_GROUP:
      g_value_set_string (value, self->control_group);
      break;
    case PROP_UNIT:
      g_value_set_string (value, self->unit);
      break;
    case PROP_SESSION:
      g_value_set_string (value, self->session);
      break;
    case PROP_SEAT:
      g_value_set_string (value, self->seat);
      break;
    case PROP_OWNER:
      g_value_set_string (value, self->owner);
      break;
    case PROP_DISK_READ_TOTAL:
      g_value_set_string (value, self->disk_read_total);
      break;
    case PROP_DISK_WRITE_TOTAL:
      g_value_set_string (value, self->disk_write_total);
      break;
    case PROP_DISK_READ:
      g_value_set_string (value, self->disk_read);
      break;
    case PROP_DISK_WRITE:
      g_value_set_string (value, self->disk_write);
      break;
    case PROP_PRIORITY:
      g_value_set_string (value, self->priority);
      break;
    case PROP_TOOLTIP:
      g_value_set_string (value, self->tooltip);
      break;
    case PROP_POINTER:
      g_value_set_pointer (value, self->pointer);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
proctable_data_set_property (GObject      *object,
                             guint         pid,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ProctableData *self = PROCTABLE_DATA(object);

  switch (pid)
    {
    case PROP_ICON:
      self->icon = g_value_get_object (value);
      break;
    case PROP_NAME:
      self->name = g_value_dup_string (value);
      break;
    case PROP_USER:
      self->user = g_value_dup_string (value);
      break;
    case PROP_STATUS:
      self->status = g_value_dup_string (value);
      break;
    case PROP_VMSIZE:
      self->vmsize = g_value_dup_string (value);
      break;
    case PROP_RESIDENT_MEMORY:
      self->resident_memory = g_value_dup_string (value);
      break;
    case PROP_WRITABLE_MEMORY:
      self->writable_memory = g_value_get_ulong (value);
      break;
    case PROP_SHARED_MEMORY:
      self->shared_memory = g_value_dup_string (value);
      break;
    case PROP_X_SERVER_MEMORY:
      self->x_server_memory = g_value_get_ulong (value);
      break;
    case PROP_CPU:
      self->cpu = g_value_dup_string (value);
      break;
    case PROP_CPU_TIME:
      self->cpu_time = g_value_dup_string (value);
      break;
    case PROP_STARTED:
      self->started = g_value_dup_string (value);
      break;
    case PROP_NICE:
      self->nice = g_value_get_int (value);
      break;
    case PROP_ID:
      self->id = g_value_get_uint (value);
      break;
    case PROP_SECURITY_CONTEXT:
      self->security_context = g_value_dup_string (value);
      break;
    case PROP_ARGS:
      self->args = g_value_dup_string (value);
      break;
    case PROP_MEMORY:
      self->memory = g_value_dup_string (value);
      break;
    case PROP_WAITING_CHANNEL:
      self->waiting_channel = g_value_dup_string (value);
      break;
    case PROP_CONTROL_GROUP:
      self->control_group = g_value_dup_string (value);
      break;
    case PROP_UNIT:
      self->unit = g_value_dup_string (value);
      break;
    case PROP_SESSION:
      self->session = g_value_dup_string (value);
      break;
    case PROP_SEAT:
      self->seat = g_value_dup_string (value);
      break;
    case PROP_OWNER:
      self->owner = g_value_dup_string (value);
      break;
    case PROP_DISK_READ_TOTAL:
      self->disk_read_total = g_value_dup_string (value);
      break;
    case PROP_DISK_WRITE_TOTAL:
      self->disk_write_total = g_value_dup_string (value);
      break;
    case PROP_DISK_READ:
      self->disk_read = g_value_dup_string (value);
      break;
    case PROP_DISK_WRITE:
      self->disk_write = g_value_dup_string (value);
      break;
    case PROP_PRIORITY:
      self->priority = g_value_dup_string (value);
      break;
    case PROP_TOOLTIP:
      self->tooltip = g_value_dup_string (value);
      break;
    case PROP_POINTER:
      self->pointer = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, pid, pspec);
    }
}

static void
proctable_data_class_init (ProctableDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = proctable_data_finalize;
  object_class->get_property = proctable_data_get_property;
  object_class->set_property = proctable_data_set_property;

  properties [PROP_ICON] =
    g_param_spec_object ("icon",
                         "Icon",
                         "Process Icon",
                         GDK_TYPE_PAINTABLE,
                         G_PARAM_READWRITE);

  properties [PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "Process Name",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_USER] =
    g_param_spec_string ("user",
                         "User",
                         "Process User",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_STATUS] =
    g_param_spec_string ("status",
                         "Status",
                         "Process Status",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_VMSIZE] =
    g_param_spec_string ("vmsize",
                         "Vmsize",
                         "Process VM Size",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_RESIDENT_MEMORY] =
    g_param_spec_string ("resident-memory",
                         "Resident-memory",
                         "Process Resident Memory",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_WRITABLE_MEMORY] =
    g_param_spec_ulong ("writable-memory",
                        "Writable-memory",
                        "Process Writable Memory",
                        0, G_MAXULONG, 0,
                        G_PARAM_READWRITE);

  properties [PROP_SHARED_MEMORY] =
    g_param_spec_string ("shared-memory",
                         "Shared-memory",
                         "Process Shared Memory",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_X_SERVER_MEMORY] =
    g_param_spec_ulong ("x-server-memory",
                        "X-server-memory",
                        "Process X Server Memory",
                        0, G_MAXULONG, 0,
                        G_PARAM_READWRITE);

  properties [PROP_CPU] =
    g_param_spec_string ("cpu",
                         "Cpu",
                         "Process CPU",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_CPU_TIME] =
    g_param_spec_string ("cpu-time",
                         "Cpu-time",
                         "Process CPU Time",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_STARTED] =
    g_param_spec_string ("started",
                         "Started",
                         "Process Started",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_NICE] =
    g_param_spec_int ("nice",
                      "Nice",
                      "Process Priority",
                      -20, 20, 0,
                      G_PARAM_READWRITE);

  properties [PROP_ID] =
    g_param_spec_uint ("id",
                       "Id",
                       "Process ID",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE);

  properties [PROP_SECURITY_CONTEXT] =
    g_param_spec_string ("security-context",
                         "Security-context",
                         "Process Security Context",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_ARGS] =
    g_param_spec_string ("args",
                         "Args",
                         "Process Args",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_MEMORY] =
    g_param_spec_string ("memory",
                         "Memory",
                         "Process Memory",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_WAITING_CHANNEL] =
    g_param_spec_string ("waiting-channel",
                         "Waiting-channel",
                         "Process Waiting Channel",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_CONTROL_GROUP] =
    g_param_spec_string ("control-group",
                         "Control-group",
                         "Process Control Group",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_UNIT] =
    g_param_spec_string ("unit",
                         "Unit",
                         "Process Unit",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_SESSION] =
    g_param_spec_string ("session",
                         "Session",
                         "Process Session",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_SEAT] =
    g_param_spec_string ("seat",
                         "Seat",
                         "Process Seat",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_OWNER] =
    g_param_spec_string ("owner",
                         "Owner",
                         "Process Owner",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_DISK_READ_TOTAL] =
    g_param_spec_string ("disk-read-total",
                         "Disk-read-total",
                         "Process Disk Read Total",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_DISK_WRITE_TOTAL] =
    g_param_spec_string ("disk-write-total",
                         "Disk-write-total",
                         "Process Disk Write Total",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_DISK_READ] =
    g_param_spec_string ("disk-read",
                         "Disk-read",
                         "Process Disk Read",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_DISK_WRITE] =
    g_param_spec_string ("disk-write",
                         "Disk-write",
                         "Process Disk Write",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_PRIORITY] =
    g_param_spec_string ("priority",
                         "Priority",
                         "Process Priority",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_TOOLTIP] =
    g_param_spec_string ("tooltip",
                         "Tooltip",
                         "Process Tooltip",
                         NULL,
                         G_PARAM_READWRITE);

  properties [PROP_POINTER] =
    g_param_spec_pointer ("pointer",
                          "Pointer",
                          "Process Pointer",
                          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
proctable_data_init (ProctableData *)
{
}
