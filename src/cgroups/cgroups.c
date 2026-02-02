/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "cgroups.h"


gboolean
gsm_cgroups_is_enabled (void)
{
  enum {
    UNINIT = 0,
    ENABLED,
    NOT_ENABLED,
  };
  static size_t status = UNINIT;

  if (g_once_init_enter (&status)) {
    gboolean has_cgroups = g_access ("/proc/cgroups", F_OK) >= 0;

    g_once_init_leave (&status, has_cgroups ? ENABLED : NOT_ENABLED);
  }

  return status == ENABLED;
}


static inline gboolean
str_non_empty (const char *str, ssize_t len)
{
  const char *first_non_space = str;

  if (first_non_space == NULL || len == 0) {
    return FALSE;
  }

  while (*first_non_space != '\0' &&
         g_ascii_isspace (*first_non_space) &&
         (len < 0 || first_non_space - str < len)) {
    first_non_space++;
  }

  return *first_non_space != '\0';
}


typedef struct _BorrowedString BorrowedString;
struct _BorrowedString {
  /* This IS NOT null terminated! */
  const char *ptr;
  size_t len;
};


static int
borrowed_cmp (gconstpointer a, gconstpointer b)
{
  return strncmp (((BorrowedString *) a)->ptr,
                  ((BorrowedString *) b)->ptr,
                  ((BorrowedString *) a)->len);
}


static inline gboolean
parse_cgroup_line (const char     *line,
                   size_t          line_len,
                   BorrowedString *path,
                   BorrowedString *controller)
{
  /*
   * cgroups(7):
   * For each cgroup hierarchy of which the process is a member,
   * there is one entry containing three colon-separated fields:
   *   hierarchy-ID:controller-list:cgroup-path
   * For example:
   *   5:cpuacct,cpu,cpuset:/daemons
   */
  const char *path_prefix = "name=";
  const char *path_start = NULL;
  const char *con_start = NULL;
  const char *ptr = line;
  size_t remaining_len = line_len;

  /* Jump past the hierarchy id */
  con_start = g_strstr_len (ptr, remaining_len, ":");
  if (con_start == NULL) {
    return FALSE;
  }

  ptr = controller->ptr = con_start + 1;
  remaining_len = line_len - (ptr - line);

  /* Jump past the controller list */
  path_start = g_strstr_len (ptr, remaining_len, ":");
  if (path_start == NULL) {
    return FALSE;
  }

  /* What remains is the path */
  ptr = path->ptr = path_start + 1;
  remaining_len = path->len = line_len - (ptr - line);

  if (!str_non_empty (path->ptr, remaining_len) || strncmp (path->ptr, "/", remaining_len) == 0) {
    return FALSE;
  }

  controller->len = path_start - con_start - 1;
  if (g_str_has_prefix (controller->ptr, path_prefix)) {
    controller->ptr += strlen (path_prefix);
    controller->len -= strlen (path_prefix);
  }

  return TRUE;
}


typedef struct _CGroupPath CGroupPath;
struct _CGroupPath {
  BorrowedString name;
  GArray *controllers;
};


static int
path_cmp (gconstpointer a, gconstpointer b)
{
  return strncmp (((CGroupPath *) a)->name.ptr,
                  ((CGroupPath *) b)->name.ptr,
                  ((CGroupPath *) a)->name.len);
}


static void
clear_path (gpointer data)
{
  CGroupPath *path = (CGroupPath *) data;

  if (path->controllers) {
    g_array_free (path->controllers, TRUE);
  }
}


static inline void
add_controller (CGroupPath *path, const char *name, size_t len)
{
  BorrowedString controller = {
    .ptr = name,
    .len = len,
  };

  if (!str_non_empty (name, len)) {
    return;
  }

  if (!path->controllers) {
    path->controllers = g_array_new (TRUE, TRUE, sizeof (BorrowedString));
  }

  g_array_append_val (path->controllers, controller);
}


static char *
extract_name (const char *file_text)
{
  g_autoptr (GString) groups = g_string_sized_new (150);
  g_autoptr (GArray) paths = g_array_new (TRUE, TRUE, sizeof (CGroupPath));
  const char *ptr = file_text;
  const char *next_newline = NULL;
  size_t line_len;
  gboolean final_line = FALSE;

  g_array_set_clear_func (paths, clear_path);

  if (!str_non_empty (file_text, -1)) {
    goto out;
  }

  do {
    BorrowedString path;
    BorrowedString controller;
    guint index;

    next_newline = strstr (ptr, "\n");
    if (next_newline) {
      line_len = next_newline - ptr;
    } else {
      line_len = strlen (ptr);
      final_line = TRUE;
    }

    if (line_len < 1) {
      goto next;
    }

    if (!parse_cgroup_line (ptr, line_len, &path, &controller)) {
      goto next;
    }

    if (g_array_binary_search (paths, &path, borrowed_cmp, &index)) {
      add_controller (&g_array_index (paths, CGroupPath, index),
                      controller.ptr,
                      controller.len);
    } else {
      CGroupPath path_info = {
        .name = path,
        .controllers = NULL,
      };
      add_controller (&path_info, controller.ptr, controller.len);
      g_array_append_val (paths, path_info);
      g_array_sort (paths, path_cmp);
    }

next:
    ptr = next_newline + 1;
  } while (!final_line);

  for (size_t i = 0; i < paths->len; i++) {
    CGroupPath *path = &g_array_index (paths, CGroupPath, i);

    if (groups->len > 0) {
      g_string_append (groups, ", ");
    }
    g_string_append_len (groups, path->name.ptr, path->name.len);

    if (!path->controllers) {
      continue;
    }

    g_array_sort (path->controllers, borrowed_cmp);

    g_string_append (groups, " (");
    for (size_t j = 0; j < path->controllers->len; j++) {
      BorrowedString *controller =
        &g_array_index (path->controllers, BorrowedString, j);

      if (j != 0) {
        g_string_append (groups, ", ");
      }

      g_string_append_len (groups, controller->ptr, controller->len);
    }
    g_string_append_c (groups, ')');
  }

out:
  return g_string_free_and_steal (g_steal_pointer (&groups));
}


static const char *
extract_name_cached (const char *file_text)
{
  static GHashTable *cache = NULL;
  g_autofree char *calculated = NULL;
  const char *name = NULL;
  guint hash = g_str_hash (file_text);

  if (g_once_init_enter_pointer (&cache)) {
    GHashTable *table =
      g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

    g_once_init_leave_pointer (&cache, g_steal_pointer (&table));
  }

  name = (const char *) g_hash_table_lookup (cache, GUINT_TO_POINTER (hash));
  if (name) {
    return name;
  }

  calculated = extract_name (file_text);
  name = calculated;
  g_hash_table_insert (cache,
                       GUINT_TO_POINTER (hash),
                       g_steal_pointer (&calculated));

  return name;
}


const char *
gsm_cgroups_get_name (pid_t pid)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *path = NULL;
  g_autofree char *text = NULL;

  if (!gsm_cgroups_is_enabled ()) {
    return NULL;
  }

  path = g_strdup_printf ("/proc/%i/cgroup", pid);

  g_file_get_contents (path, &text, NULL, &error);
  if (error) {
    return NULL;
  }

  return extract_name_cached (text);
}
