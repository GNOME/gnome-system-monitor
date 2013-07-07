#include <ctype.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <gtk/gtk.h>

#include "sysmon.h"

UIInfo       ui_info          = { 0 };
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

void
smon_gdk_event_hook (GdkEvent *event, /* IN */
                gpointer  data)  /* IN */
{
	ui_info.gdk_event_count++;
	gtk_main_do_event(event);
}

gdouble
smon_get_xevent_info (UberLineGraph *graph,     /* IN */
                 guint          line,      /* IN */
                 gpointer       user_data) /* IN */
{
	gdouble value = UBER_LINE_GRAPH_NO_VALUE;

	switch (line) {
	case 1:
		value = ui_info.gdk_event_count;
		ui_info.gdk_event_count = 0;
		break;
	case 2:
		value = ui_info.x_event_count;
		ui_info.x_event_count = 0;
		break;
	default:
		g_assert_not_reached();
	}
	return value;
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
	g_assert_cmpint(line, <=, cpu_info.len * 2);

	if ((line % 2) == 0) {
		value = cpu_info.freq[((line - 1) / 2)];
	} else {
		i = (line - 1) / 2;
		value = cpu_info.total[i];
		/*
		 * Update label text.
		 */
		text = g_strdup_printf("CPU%d  %0.1f %%", i + 1, value);
		uber_label_set_text(UBER_LABEL(cpu_info.labels[i]), text);
		g_free(text);
	}
	return value;
}

gdouble
smon_get_net_info (UberLineGraph *graph,     /* IN */
              guint          line,      /* IN */
              gpointer       user_data) /* IN */
{
	gdouble value = UBER_LINE_GRAPH_NO_VALUE;
	switch (line) {
	case 1:
		value = net_info.total_in;
		break;
	case 2:
		value = net_info.total_out;
		break;
	default:
		g_assert_not_reached();
	}
	return value;
}

int
XNextEvent (Display *display,      /* IN */
            XEvent  *event_return) /* OUT */
{
	static gsize initialized = FALSE;
	static int (*Real_XNextEvent) (Display*disp, XEvent*evt);
	gpointer lib;
	int ret;
	
	if (G_UNLIKELY(g_once_init_enter(&initialized))) {
		if (!(lib = dlopen("libX11.so.6", RTLD_LAZY))) {
			g_error("Could not load libX11.so.6");
		}
		if (!(Real_XNextEvent = dlsym(lib, "XNextEvent"))) {
			g_error("Could not find XNextEvent in libX11.so.6");
		}
		g_once_init_leave(&initialized, TRUE);
	}

	ret = Real_XNextEvent(display, event_return);
	ui_info.x_event_count++;
	return ret;
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
	GError *error = NULL;
	gulong total_in = 0;
	gulong total_out = 0;
	gulong bytes_in;
	gulong bytes_out;
	gulong dummy;
	gchar *buf = NULL;
	gchar iface[32] = { 0 };
	gchar *line;
	gsize len;
	gint l = 0;
	gint i;

	if (!g_file_get_contents("/proc/net/dev", &buf, &len, &error)) {
		g_printerr("%s", error->message);
		g_error_free(error);
		return;
	}

	line = buf;
	for (i = 0; i < len; i++) {
		if (buf[i] == ':') {
			buf[i] = ' ';
		} else if (buf[i] == '\n') {
			buf[i] = '\0';
			if (++l > 2) { // ignore first two lines
				if (sscanf(line, "%31s %lu %lu %lu %lu %lu %lu %lu %lu %lu",
				           iface, &bytes_in,
				           &dummy, &dummy, &dummy, &dummy,
				           &dummy, &dummy, &dummy,
				           &bytes_out) != 10) {
					g_warning("Skipping invalid line: %s", line);
				} else if (g_strcmp0(iface, "lo") != 0) {
					total_in += bytes_in;
					total_out += bytes_out;
				}
				line = NULL;
			}
			line = &buf[++i];
		}
	}

	if ((net_info.last_total_in != 0.) && (net_info.last_total_out != 0.)) {
		net_info.total_in = (total_in - net_info.last_total_in);
		net_info.total_out = (total_out - net_info.last_total_out);
	}

	net_info.last_total_in = total_in;
	net_info.last_total_out = total_out;
	g_free(buf);
}
