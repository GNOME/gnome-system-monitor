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
    gchar *controller, *path, *tmp, *path_plus_space;
    int paren_offset, off, tmp_size;

    controller = g_strstr_len(line, -1, ":") + 1;
    if (!controller)
        return;

    path = g_strstr_len(controller, -1, ":") + 1;
    if (!path)
        return;

    *(path - 1) = '\0';
    g_strdelimit(controller, ",", '/');

    if ((std::strcmp(path, "/") == 0) || (std::strncmp(controller, "name=", 5) == 0))
        return;

    if (*current_cgroup_name == NULL) {
        *current_cgroup_name = g_strdup_printf("%s (%s)", path, controller);
        return;
    }

    /* add a space to the end of the path string */
    path_plus_space = g_strdup_printf("%s ", path);

    if ((tmp = g_strstr_len(*current_cgroup_name, -1, path_plus_space))) {
        tmp_size = strlen(*current_cgroup_name) + strlen(controller) + 1;
        paren_offset = g_strstr_len(tmp + strlen(path), -1, ")") - *current_cgroup_name;
        *(*current_cgroup_name + paren_offset) = '\0';
        tmp = (gchar *)g_strnfill(tmp_size, '\0');
        off = g_strlcat(tmp, *current_cgroup_name, tmp_size);
        *(tmp + off) = '/';
        off++;
        off += g_strlcat(tmp + off, controller, tmp_size);
        *(tmp + off) = ')';
        off++;
        g_strlcat(tmp + off, *current_cgroup_name + paren_offset + 1, tmp_size);
    } else
        tmp = g_strdup_printf("%s, %s(%s)", *current_cgroup_name, path_plus_space, controller);

    g_free(path_plus_space);
    g_free(*current_cgroup_name);
    *current_cgroup_name = tmp;
}

int
check_cgroup_changed(gchar *line, gchar *current_cgroup_set)
{
    /* check if line is contained in current_cgroup_set */
    gchar *controller, *path, *tmp, *found, *close_paren, *open_paren;
    int ret = 0;

    controller = g_strstr_len(line, -1, ":") + 1;
    if (!controller)
        return 1;

    path = g_strstr_len(controller, -1, ":") + 1;
    if (!path)
        return 1;

    *(path - 1) = '\0';

    if (std::strncmp(controller, "name=", 5) == 0)
        goto out;

    /* if there are multiple controllers just report string has changed */
    if (g_strstr_len(controller, -1, ",")) {
        ret = 1;
        goto out;
    }

    if (!current_cgroup_set) {
        if (std::strcmp(path, "/") != 0)
            ret = 1;
        goto out;
    }

    /* special case for root cgroup */
    tmp = current_cgroup_set;
    if (std::strcmp(path, "/") == 0) {
        while ((found = g_strstr_len(tmp, -1, controller))) {
            close_paren = g_strstr_len(found, -1, ")");
            open_paren = g_strstr_len(found, -1, "(");
            if (close_paren) {
                if (!open_paren || (close_paren < open_paren)) {
                    ret = 1;
                    goto out;
                }
            }
            tmp = found + strlen(controller);
        }
        goto out;
    }

    tmp = current_cgroup_set;
    while ((found = g_strstr_len(tmp, -1, path))) {
            found = found + strlen(path);
            close_paren = g_strstr_len(found, -1, ")");
            if (*found == ' ') {
                if (g_strstr_len(found + 1, close_paren - found, controller))
                    goto out;
            }
            tmp = close_paren + 1;
    }
    ret = 1;
out:
    *(path - 1) = ':';
    return ret;
}

void
get_process_cgroup_info(ProcInfo *info)
{
    gchar *path;
    gchar *cgroup_name = NULL;
    int cgroups_changed = 0;
    gchar *text;
    char **lines;
    int i;

    if (!cgroups_enabled())
        return;

    /* read out of /proc/pid/cgroup */
    path = g_strdup_printf("/proc/%d/cgroup", info->pid);
    if (!path)
        return;
    if(!g_file_get_contents(path, &text, NULL, NULL))
        goto out;
    lines = g_strsplit(text, "\n", -1);
    g_free(text);
    if (!lines)
        goto out;

    for (i = 0; lines[i] != NULL; i++) {
        if (lines[i][0] == '\0')
            continue;
        if (check_cgroup_changed(lines[i], info->cgroup_name)) {
            cgroups_changed = 1;
            break;
        }
    }

    if (cgroups_changed) {
        for (i = 0; lines[i] != NULL; i++) {
            if (lines[i][0] == '\0')
                continue;
            append_cgroup_name(lines[i], &cgroup_name);
        }
        if (info->cgroup_name)
            g_free(info->cgroup_name);
        if (!cgroup_name)
            info->cgroup_name = g_strdup("");
        else
            info->cgroup_name = cgroup_name;
    }

    g_strfreev(lines);
out:
    g_free(path);
}
