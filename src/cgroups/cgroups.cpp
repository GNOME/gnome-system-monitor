/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <algorithm>
#include <filesystem>
#include <map>
#include <unordered_map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glib.h>

#include "cgroups.h"

using namespace std::string_view_literals;


gboolean
gsm_cgroups_is_enabled ()
{
  static const bool has_cgroups = std::filesystem::exists ("/proc/cgroups");
  return has_cgroups;
}


namespace
{

struct CGroupLineData
{
  std::string_view name;
  std::string_view cat;
};

CGroupLineData
make_parsed_cgroup_line (const std::string_view line)
{
  const std::string_view::size_type cat_start = line.find (':');
  if (cat_start == std::string_view::npos)
    {
      return CGroupLineData();
    }

  const std::string_view::size_type name_start = line.find (':', cat_start + 1);
  if (name_start == std::string_view::npos)
    {
      return CGroupLineData();
    }

  const auto name = line.substr (name_start + 1);
  if (name.empty () or name == "/"sv)
    {
      return CGroupLineData();
    }

  auto cat = line.substr (cat_start + 1, name_start - cat_start - 1);

  constexpr auto name_prefix = "name="sv;
  if (cat.starts_with (name_prefix))
    {
      cat.remove_prefix (name_prefix.size());
    }

  return { name, cat };
}

CGroupLineData
parse_cgroup_line (const std::string_view line)
{
  static std::unordered_map<std::string_view, CGroupLineData> line_cache;

  const auto [it, inserted] = line_cache.try_emplace (line, ""sv, ""sv);

  if (inserted)
    {
      it->second = make_parsed_cgroup_line(it->first);
    }

  return it->second;
}


static inline char *
make_process_cgroup_name (const std::string_view cgroup_file_text)
{
  g_autoptr (GString) groups = g_string_new (NULL);
  // name -> [cat...], sorted by name;
  std::map<std::string_view, std::vector<std::string_view> > names;

  std::string_view::size_type last = 0, eol;

  // for each line in the file
  while ((eol = cgroup_file_text.find ('\n', last)) != std::string_view::npos)
    {
      auto line = cgroup_file_text.substr (last, eol - last);
      last = eol + 1;

      const auto line_data = parse_cgroup_line (line);
      if (!line_data.name.empty ()) {
        if (!line_data.cat.empty ()) {
          names[line_data.name].push_back (line_data.cat);
        } else {
          names.try_emplace (line_data.name, std::vector<std::string_view> ());
        }
      }
    }


  // name (cat1, cat2), ...
  // sorted by name, categories

  for (auto&i : names) {
    std::sort (begin (i.second), end (i.second));

    if (groups->len > 0) {
      g_string_append (groups, ", ");
    }

    g_string_append_len (groups, i.first.data (), i.first.size ());

    if (i.second.empty ()) {
      continue;
    }

    g_string_append (groups, " (");
    for (size_t j = 0; j < i.second.size (); j++) {
      if (j != 0) {
        g_string_append (groups, ", ");
      }

      g_string_append_len (groups, i.second[j].data (), i.second[j].size ());
    }
    g_string_append_c (groups, ')');
  }

  return g_string_free_and_steal (g_steal_pointer (&groups));
}

} // namespace


const char *
gsm_cgroups_extract_name (const char *file_text)
{
  static std::unordered_map<std::string, std::string> file_cache;

  const auto [it, inserted] = file_cache.try_emplace (std::string (file_text), "");

  if (inserted) {
    g_autofree char *name = make_process_cgroup_name (it->first);

    it->second = name ? std::string (name) : "";
  }

  return it->second.c_str ();
}


static inline const char *
get_process_cgroup_string (pid_t pid)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *path = g_strdup_printf ("/proc/%i/cgroup", pid);
  g_autofree char *text = NULL;

  g_file_get_contents (path, &text, NULL, &error);
  if (error) {
    return NULL;
  }

  return gsm_cgroups_extract_name (text);
}


char *
gsm_cgroups_get_name (pid_t pid)
{
  if (!gsm_cgroups_is_enabled ()) {
    return NULL;
  }

  return g_strdup (get_process_cgroup_string (pid));
}
