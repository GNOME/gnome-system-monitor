/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* sysmon.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <ctype.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glibtop/netlist.h>
#include <glibtop/netload.h>

#include "sysmon.h"

CpuInfo      cpu_info         = { 0 };
NetInfo      net_info         = { 0 };

gboolean
smon_cpu_has_freq_scaling (gint cpu)
{
	gboolean ret;
	gchar *path;

	path = g_strdup_printf("/sys/devices/system/cpu/cpu%d/cpufreq", cpu);
	ret = g_file_test(path, G_FILE_TEST_IS_DIR);
	g_free(path);
	return ret;
}

gsize
smon_get_cpu_count (void)
{
   return get_nprocs();
}

gsize
smon_get_net_interface_count (void)
{  
   if (G_UNLIKELY(!net_info.len)) {
       glibtop_netlist dummy;
       return g_strv_length(glibtop_get_netlist(&dummy));
   }
   return net_info.len;
}

gdouble
smon_get_cpu_info (UberLineGraph *graph,     /* IN */
              guint          line,      /* IN */
              gpointer       user_data) /* IN */
{
	gchar *text;
	gint i;
	gdouble value = UBER_LINE_GRAPH_NO_VALUE;

	g_assert_cmpint(line, >, 0);
	g_assert_cmpint(line, <=, cpu_info.len);

    i = line-1;
    value = cpu_info.total[i] * cpu_info.freq[i] / 100.0;
	/* Update label text. */
	text = g_strdup_printf("CPU%d  %0.1f %%", i + 1, value);
	uber_label_set_text(UBER_LABEL(cpu_info.labels[i]), text);
	g_free(text);
	return value;
}

gdouble
smon_get_net_info (UberLineGraph *graph,     /* IN */
              guint          line,      /* IN */
              gpointer       user_data) /* IN */
{
	gdouble value = UBER_LINE_GRAPH_NO_VALUE;
	switch (line % 2) {
	case 0:
        if (net_info.total_in!=NULL)
		value = net_info.total_in[line / 2];
		break;
	case 1:
        if (net_info.total_out !=NULL)
		value = net_info.total_out[line / 2];
		break;
	default:
		g_assert_not_reached();
	}
	return value;
}

void
smon_next_cpu_info (void)
{
	gchar cpu[64] = { 0 };
	glong user;
	glong system;
	glong nice;
	glong idle;
	glong user_calc;
	glong system_calc;
	glong nice_calc;
	glong idle_calc;
	gchar *buf = NULL;
	glong total;
	gchar *line;
	gint ret;
	gint id;
	gint i;

	if (G_UNLIKELY(!cpu_info.len)) {
#if __linux__
		cpu_info.len = get_nprocs();
#else
#error "Your platform is not supported"
#endif
		g_assert(cpu_info.len);
		/*
		 * Allocate data for sampling.
		 */
		cpu_info.total = g_new0(gdouble, cpu_info.len);
		cpu_info.last_user = g_new0(glong, cpu_info.len);
		cpu_info.last_idle = g_new0(glong, cpu_info.len);
		cpu_info.last_system = g_new0(glong, cpu_info.len);
		cpu_info.last_nice = g_new0(glong, cpu_info.len);
		cpu_info.labels = g_new0(GtkWidget*, cpu_info.len);
	}

	if (G_LIKELY(g_file_get_contents("/proc/stat", &buf, NULL, NULL))) {
		line = buf;
		for (i = 0; buf[i]; i++) {
			if (buf[i] == '\n') {
				buf[i] = '\0';
				if (g_str_has_prefix(line, "cpu")) {
					if (isdigit(line[3])) {
						user = nice = system = idle = id = 0;
						ret = sscanf(line, "%s %ld %ld %ld %ld",
						             cpu, &user, &nice, &system, &idle);
						if (ret != 5) {
							goto next;
						}
						ret = sscanf(cpu, "cpu%d", &id);
						if (ret != 1 || id < 0 || id >= cpu_info.len) {
							goto next;
						}
						user_calc = user - cpu_info.last_user[id];
						nice_calc = nice - cpu_info.last_nice[id];
						system_calc = system - cpu_info.last_system[id];
						idle_calc = idle - cpu_info.last_idle[id];
						total = user_calc + nice_calc + system_calc + idle_calc;
						cpu_info.total[id] = (user_calc + nice_calc + system_calc) / (gfloat)total * 100.;
						cpu_info.last_user[id] = user;
						cpu_info.last_nice[id] = nice;
						cpu_info.last_idle[id] = idle;
						cpu_info.last_system[id] = system;
					}
				} else {
					/* CPU info comes first. Skip further lines. */
					break;
				}
			  next:
				line = &buf[i + 1];
			}
		}
	}

	g_free(buf);
}

void
smon_next_cpu_freq_info (void)
{
	glong max;
	glong cur;
	gboolean ret;
	gchar *buf;
	gchar *path;
	gint i;

	g_return_if_fail(cpu_info.len > 0);

	/*
	 * Initialize.
	 */
	if (!cpu_info.freq) {
		cpu_info.freq = g_new0(gdouble, cpu_info.len);
	}

	/*
	 * Get current frequencies.
	 */
	for (i = 0; i < cpu_info.len; i++) {
		/*
		 * Get max frequency.
		 */
		path = g_strdup_printf("/sys/devices/system/cpu/cpu%d"
		                       "/cpufreq/scaling_max_freq", i);
		ret = g_file_get_contents(path, &buf, NULL, NULL);
		g_free(path);
		if (!ret) {
			continue;
		}
		max = atoi(buf);
		g_free(buf);

		/*
		 * Get current frequency.
		 */
		path = g_strdup_printf("/sys/devices/system/cpu/cpu%d/"
		                       "cpufreq/scaling_cur_freq", i);
		ret = g_file_get_contents(path, &buf, NULL, NULL);
		g_free(path);
		if (!ret) {
			continue;
		}
		cur = atoi(buf);
		g_free(buf);

		/*
		 * Store frequency percentage.
		 */
		cpu_info.freq[i] = (gfloat)cur / (gfloat)max * 100.;
	}
}

void
smon_next_net_info (void)
{
	gulong total_in = 0;
	gulong total_out = 0;
	gsize len;
    gsize count = 0;
	gint i;

   if (G_UNLIKELY(!net_info.len)) {
        glibtop_netlist netlist;

        net_info.ifnames = glibtop_get_netlist(&netlist);
        
        net_info.len = g_strv_length(net_info.ifnames);

        g_assert(net_info.len);
		/*
		 * Allocate data for sampling.
		 */
		net_info.labels = g_new0(GtkWidget*, 2*net_info.len);
        net_info.total_out = g_new0(gdouble, net_info.len);
		net_info.total_in = g_new0(gdouble, net_info.len);
		net_info.last_total_out = g_new0(gdouble, net_info.len);
		net_info.last_total_in = g_new0(gdouble, net_info.len);

	}

    len = net_info.len;

	for (i = 0; i < len; i++) {
        glibtop_netload netload;
        glibtop_get_netload (&netload, net_info.ifnames[i]);
        if (netload.if_flags & (1 << GLIBTOP_IF_FLAGS_LOOPBACK))
            continue;

        /* Skip interfaces without any IPv4/IPv6 address (or
           those with only a LINK ipv6 addr) However we need to
           be able to exclude these while still keeping the
           value so when they get online (with NetworkManager
           for example) we don't get a suddent peak.  Once we're
           able to get this, ignoring down interfaces will be
           possible too.  */
        if (!(netload.flags & (1 << GLIBTOP_NETLOAD_ADDRESS6)
                 & (netload.scope6 != GLIBTOP_IF_IN6_SCOPE_LINK))
            & !(netload.flags & (1 << GLIBTOP_NETLOAD_ADDRESS)))
            continue;
		total_in = netload.bytes_in;
		total_out = netload.bytes_out;
        if ((net_info.last_total_in[i] != 0.) && (net_info.last_total_out[count] != 0.)) {
		    net_info.total_in[count] = (total_in - net_info.last_total_in[count]);
		    net_info.total_out[count] = (total_out - net_info.last_total_out[count]);
	    }
        net_info.ifnames[count] = net_info.ifnames[i];
        net_info.last_total_in[count] = total_in;
        net_info.last_total_out[count] = total_out;
        count++;
	}
    net_info.len = count;

}
