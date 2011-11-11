#include <config.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "procman.h"
#include "util.h"

gboolean
cgroups_enabled(void)
{
    static gboolean initialized = FALSE;
    static gboolean has_cgroups;

    if (!initialized) {
        initialized = TRUE;
        has_cgroups = g_file_test("/proc/cgroups", G_FILE_TEST_EXISTS);
    }

    return has_cgroups;
}

void
append_cgroup_name(char *line, gchar **current_cgroup_name)
{
    gchar *controller, *path, *tmp;
    int paren_offset, off, tmp_size;

    controller = g_strstr_len(line, -1, ":") + 1;
    path = g_strstr_len(controller, -1, ":") + 1;
    *(path - 1) = '\0';

    //printf("append_cgroup_name, controller: %s path: %s\n", controller, path);

    if (std::strcmp(path, "/") == 0)
        return;

    if (g_strstr_len(controller, -1, "name=systemd"))
        return;

    if (*current_cgroup_name == NULL) {
        *current_cgroup_name = g_strdup_printf("%s (%s)", path, controller);
        return;
    }

    if ((tmp = g_strstr_len(*current_cgroup_name, -1, path))) {
        tmp_size = strlen(*current_cgroup_name) + strlen(controller) + 1;
        paren_offset = g_strstr_len(tmp, -1, ")") - tmp;
        *(*current_cgroup_name + paren_offset) = '\0';
        tmp = (gchar *)g_strnfill(tmp_size, '\0');
        off = g_strlcat(tmp, *current_cgroup_name, tmp_size);
        *(tmp + off) = '/';
        off++;
        off += g_strlcat(tmp + off, controller, tmp_size);
        *(tmp + off) = ')';
        off++;
        off += g_strlcat(tmp + off, *current_cgroup_name + paren_offset + 1, tmp_size);
        *(*current_cgroup_name + paren_offset) = ')';
    } else  {
        tmp = g_strdup_printf("%s, %s (%s)", *current_cgroup_name, path, controller);
    }

    g_free(*current_cgroup_name);
    *current_cgroup_name = tmp;
}

void
get_process_cgroup_info(ProcInfo *info)
{
    gchar *path;
    GFile *file;
    GFileInputStream *input_stream;
    GDataInputStream *data_stream;
    char *line;
    gchar *cgroup_name = NULL;

    if (!cgroups_enabled())
        return;

    /* read out of /proc/pid/cgroup */
    path = g_strdup_printf("/proc/%d/cgroup", info->pid);
    file = g_file_new_for_path(path);
    if(!(input_stream = g_file_read(file, NULL, NULL))) {
        goto out;
    }
    data_stream = g_data_input_stream_new(G_INPUT_STREAM(input_stream));

    while ((line = g_data_input_stream_read_line(data_stream, NULL, NULL, NULL))) {
        append_cgroup_name(line, &cgroup_name);
        g_free(line);
    }
    if (cgroup_name) {
        if (info->cgroup_name) {
            g_free(info->cgroup_name);
        }
        info->cgroup_name = cgroup_name;
    }
    g_object_unref(data_stream);
    g_object_unref(input_stream);
out:
    g_object_unref(file);
    g_free(path);
}
