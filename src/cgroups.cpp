#include "cgroups.h"

#include <algorithm>
#include <filesystem>
#include <map>
#include <unordered_map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtkmm.h>

#include "join.h"
#include "procinfo.h"

using namespace std::string_view_literals;

bool
cgroups_enabled ()
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

std::string
make_process_cgroup_name(const std::string_view cgroup_file_text)
{
  // name -> [cat...], sorted by name;
  std::map<std::string_view, std::vector<std::string_view> > names;

  std::string_view::size_type last = 0, eol;

  // for each line in the file
  while ((eol = cgroup_file_text.find ('\n', last)) != std::string_view::npos)
    {
      auto line = cgroup_file_text.substr (last, eol - last);
      last = eol + 1;

      const auto line_data = parse_cgroup_line (line);
      if (!line_data.name.empty ())
        names[line_data.name].push_back (line_data.cat);
    }


  // name (cat1, cat2), ...
  // sorted by name, categories
  std::vector<std::string> groups;

  for (auto&i : names)
    {
      std::sort (begin (i.second), end (i.second));
      groups.push_back (std::string(i.first) + " (" + procman::join<std::string> (i.second, ", "sv) + ')');
    }

  return procman::join<std::string> (groups, ", "sv);
}

} // namespace

std::string_view
get_process_cgroup_name(std::string cgroup_file_text)
{
  static std::unordered_map<std::string, std::string> file_cache;

  const auto [it, inserted] = file_cache.try_emplace (std::move(cgroup_file_text), "");

  if (inserted)
    {
      it->second = make_process_cgroup_name(it->first);
    }

  return it->second;
}

namespace
{

std::string_view
get_process_cgroup_string (pid_t pid)
{
  auto path = "/proc/" + std::to_string (pid) + "/cgroup";
  std::string text;

  try {
      text = Glib::file_get_contents (path);
    } catch (...) {
      return ""sv;
    }
    return get_process_cgroup_name(std::move(text));
}

} // namespace

void
get_process_cgroup_info (ProcInfo&info)
{
  if (not cgroups_enabled ())
    return;

  info.cgroup_name = std::string(get_process_cgroup_string (info.pid));
}
