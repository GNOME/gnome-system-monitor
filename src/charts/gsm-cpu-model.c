/* gsm-cpu-model.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#if defined(__FreeBSD__)
# include <errno.h>
# include <sys/resource.h>
# include <sys/sysctl.h>
# include <sys/types.h>
#endif

#include "gsm-cpu-model.h"
#include <dazzle.h>

#ifdef __linux__
# define PROC_STAT_BUF_SIZE 4096
#endif

typedef struct
{
  gdouble total;
  gdouble freq;
  glong   last_user;
  glong   last_idle;
  glong   last_system;
  glong   last_nice;
  glong   last_iowait;
  glong   last_irq;
  glong   last_softirq;
  glong   last_steal;
  glong   last_guest;
  glong   last_guest_nice;
} CpuInfo;

struct _GsmCpuModel
{
  DzlGraphModel  parent_instance;

  GArray  *cpu_info;
  guint    n_cpu;

#ifdef __linux__
  gint     stat_fd;
  gchar   *stat_buf;
#endif

  guint    poll_source;
  guint    poll_interval_msec;
};

G_DEFINE_TYPE (GsmCpuModel, gsm_cpu_model, DZL_TYPE_GRAPH_MODEL)

#ifdef __linux__
static gboolean
read_stat (GsmCpuModel *self)
{
  gssize len;

  g_assert (self != NULL);
  g_assert (self->stat_fd != -1);
  g_assert (self->stat_buf != NULL);

  if (lseek (self->stat_fd, 0, SEEK_SET) != 0)
    return FALSE;

  len = read (self->stat_fd, self->stat_buf, PROC_STAT_BUF_SIZE);
  if (len <= 0)
    return FALSE;

  if (len < PROC_STAT_BUF_SIZE)
    self->stat_buf[len] = 0;
  else
    self->stat_buf[PROC_STAT_BUF_SIZE-1] = 0;

  return TRUE;
}

static void
gsm_cpu_model_poll (GsmCpuModel *self)
{
  gchar cpu[64] = { 0 };
  glong user;
  glong sys;
  glong nice;
  glong idle;
  glong iowait;
  glong irq;
  glong softirq;
  glong steal;
  glong guest;
  glong guest_nice;
  glong user_calc;
  glong system_calc;
  glong nice_calc;
  glong idle_calc;
  glong iowait_calc;
  glong irq_calc;
  glong softirq_calc;
  glong steal_calc;
  glong guest_calc;
  glong guest_nice_calc;
  glong total;
  gchar *line;
  gint ret;
  gint id;

  if (read_stat (self))
    {
      line = self->stat_buf;

      for (gsize i = 0; self->stat_buf[i]; i++)
        {
          if (self->stat_buf[i] == '\n') {
            self->stat_buf[i] = '\0';

            if (strncmp (line, "cpu", 3) == 0)
              {
                if (isdigit (line[3]))
                  {
                    CpuInfo *cpu_info;

                    user = nice = sys = idle = id = 0;
                    ret = sscanf (line, "%s %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
                                  cpu, &user, &nice, &sys, &idle,
                                  &iowait, &irq, &softirq, &steal, &guest, &guest_nice);
                    if (ret != 11)
                      goto next;

                    ret = sscanf(cpu, "cpu%d", &id);

                    if (ret != 1 || id < 0 || id >= (gint)self->n_cpu)
                      goto next;

                    cpu_info = &g_array_index (self->cpu_info, CpuInfo, id);

                    user_calc = user - cpu_info->last_user;
                    nice_calc = nice - cpu_info->last_nice;
                    system_calc = sys - cpu_info->last_system;
                    idle_calc = idle - cpu_info->last_idle;
                    iowait_calc = iowait - cpu_info->last_iowait;
                    irq_calc = irq - cpu_info->last_irq;
                    softirq_calc = softirq - cpu_info->last_softirq;
                    steal_calc = steal - cpu_info->last_steal;
                    guest_calc = guest - cpu_info->last_guest;
                    guest_nice_calc = guest_nice - cpu_info->last_guest_nice;

                    total = user_calc + nice_calc + system_calc + idle_calc + iowait_calc + irq_calc + softirq_calc + steal_calc + guest_calc + guest_nice_calc;
                    cpu_info->total = ((total - idle_calc) / (gdouble)total) * 100.0;

                    cpu_info->last_user = user;
                    cpu_info->last_nice = nice;
                    cpu_info->last_idle = idle;
                    cpu_info->last_system = sys;
                    cpu_info->last_iowait = iowait;
                    cpu_info->last_irq = irq;
                    cpu_info->last_softirq = softirq;
                    cpu_info->last_steal = steal;
                    cpu_info->last_guest = guest;
                    cpu_info->last_guest_nice = guest_nice;
                  }
              } else {
                /* CPU info comes first. Skip further lines. */
                break;
              }

            next:
              line = &self->stat_buf[i + 1];
            }
        }
    }
}

#elif defined(__FreeBSD__)

static void
gsm_cpu_model_poll (GsmCpuModel *self)
{
  static gint mib_cp_times[2];
  static gsize len_cp_times = 2;

  if (mib_cp_times[0] == 0 || mib_cp_times[1] == 0)
    {
      if (sysctlnametomib ("kern.cp_times", mib_cp_times, &len_cp_times) == -1)
        {
          g_critical ("Cannot convert sysctl name kern.cp_times to a mib array: %s",
                      g_strerror (errno));
          return;
        }
    }

  gsize cp_times_size = sizeof (glong) * CPUSTATES * self->n_cpu;
  glong *cp_times = g_malloc (cp_times_size);

  if (sysctl (mib_cp_times, 2, cp_times, &cp_times_size, NULL, 0) == -1)
    {
      g_critical ("Cannot get CPU usage by sysctl kern.cp_times: %s",
                  g_strerror (errno));
      g_free (cp_times);
      return;
    }

  for (guint i = 0, j = 0; i < self->n_cpu; i++, j += CPUSTATES)
    {
      CpuInfo *cpu_info = &g_array_index (self->cpu_info, CpuInfo, i);

      glong user = cp_times[j + CP_USER];
      glong nice = cp_times[j + CP_NICE];
      glong sys = cp_times[j + CP_SYS];
      glong irq = cp_times[j + CP_INTR];
      glong idle = cp_times[j + CP_IDLE];

      glong user_calc = user - cpu_info->last_user;
      glong nice_calc = nice - cpu_info->last_nice;
      glong system_calc = sys - cpu_info->last_system;
      glong irq_calc = irq - cpu_info->last_irq;
      glong idle_calc = idle - cpu_info->last_idle;

      glong total = user_calc + nice_calc + system_calc + irq_calc + idle_calc;
      cpu_info->total = ((total - idle_calc) / (gdouble)total) * 100.0;

      cpu_info->last_user = user;
      cpu_info->last_nice = nice;
      cpu_info->last_system = sys;
      cpu_info->last_irq = irq;
      cpu_info->last_idle = idle;
    }
  g_free (cp_times);
}
#else
static void
gsm_cpu_model_poll (GsmCpuModel *self)
{
  /*
   * TODO: calculate cpu info for OpenBSD/etc.
   *
   * While we are at it, we should make the Linux code above non-shitty.
   */
}
#endif

static gboolean
gsm_cpu_model_poll_cb (gpointer user_data)
{
  GsmCpuModel *self = user_data;
  DzlGraphModelIter iter;
  guint i;

  gsm_cpu_model_poll (self);

  dzl_graph_view_model_push (DZL_GRAPH_MODEL (self), &iter, g_get_monotonic_time ());

  for (i = 0; i < self->cpu_info->len; i++)
    {
      CpuInfo *cpu_info;

      cpu_info = &g_array_index (self->cpu_info, CpuInfo, i);
      dzl_graph_view_model_iter_set (&iter, i, cpu_info->total, -1);
    }

  return G_SOURCE_CONTINUE;
}

static void
gsm_cpu_model_constructed (GObject *object)
{
  GsmCpuModel *self = (GsmCpuModel *)object;
  gint64 timespan;
  guint max_samples;

  G_OBJECT_CLASS (gsm_cpu_model_parent_class)->constructed (object);

  max_samples = dzl_graph_view_model_get_max_samples (DZL_GRAPH_MODEL (self));
  timespan = dzl_graph_view_model_get_timespan (DZL_GRAPH_MODEL (self));

  self->poll_interval_msec = (gdouble)timespan / (gdouble)(max_samples - 1) / 1000L;

  if (self->poll_interval_msec == 0)
    {
      g_critical ("Implausible timespan/max_samples combination for graph.");
      self->poll_interval_msec = 1000;
    }

  self->n_cpu = g_get_num_processors ();

  for (guint i = 0; i < self->n_cpu; i++)
    {
      CpuInfo cpu_info = { 0 };
      DzlGraphColumn *column;
      gchar *name;

      name = g_strdup_printf ("CPU %d", i + 1);
      column = dzl_graph_view_column_new (name, G_TYPE_DOUBLE);

      dzl_graph_view_model_add_column (DZL_GRAPH_MODEL (self), column);
      g_array_append_val (self->cpu_info, cpu_info);

      g_object_unref (column);
      g_free (name);
    }

  gsm_cpu_model_poll (self);

  self->poll_source = g_timeout_add (self->poll_interval_msec, gsm_cpu_model_poll_cb, self);
}

static void
gsm_cpu_model_finalize (GObject *object)
{
  GsmCpuModel *self = (GsmCpuModel *)object;

#ifdef __linux__
  g_clear_pointer (&self->stat_buf, g_free);
  if (self->stat_fd != -1)
    close (self->stat_fd);
#endif

  dzl_clear_source (&self->poll_source);
  g_clear_pointer (&self->cpu_info, g_array_unref);

  G_OBJECT_CLASS (gsm_cpu_model_parent_class)->finalize (object);
}

static void
gsm_cpu_model_class_init (GsmCpuModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gsm_cpu_model_constructed;
  object_class->finalize = gsm_cpu_model_finalize;
}

static void
gsm_cpu_model_init (GsmCpuModel *self)
{
  self->cpu_info = g_array_new (FALSE, FALSE, sizeof (CpuInfo));

#ifdef __linux__
  self->stat_fd = open ("/proc/stat", O_RDONLY);
  self->stat_buf = g_malloc (PROC_STAT_BUF_SIZE);
#endif

  g_object_set (self,
                "value-min", 0.0,
                "value-max", 100.0,
                NULL);
}

DzlGraphModel *
gsm_cpu_model_new (void)
{
  return g_object_new (GSM_TYPE_CPU_MODEL, NULL);
}
