#include <config.h>
#include "cgroups.h"
#include "join.h"
#include "procinfo.h"

#include <filesystem>
#include <map>
#include <unordered_map>
#include <string>
#include <string_view>
#include <utility>

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
  std::string name;
  std::string cat;
};

CGroupLineData
make_parsed_cgroup_line (const std::string&line)
{
  const std::string::size_type cat_start = line.find (':');
  if (cat_start == std::string::npos)
    {
      return CGroupLineData();
    }
  
  const std::string::size_type name_start = line.find (':', cat_start + 1);
  if (name_start == std::string::npos)
    {
      return CGroupLineData();
    }

  auto cat = line.substr (cat_start + 1, name_start - cat_start - 1);
  auto name = line.substr (name_start + 1);

  // strip the name= prefix
  if (cat.find ("name=") == 0)
    cat.erase (0, 5);

  if (name.empty() or name == "/")
    {
      return CGroupLineData();
    }
  return { name, cat };
}

const CGroupLineData &
parse_cgroup_line (const std::string&line)
{
  static std::unordered_map<std::string, CGroupLineData> line_cache;

  const auto [it, inserted] = line_cache.try_emplace (line, "", "");

  if (inserted)
    {
      it->second = make_parsed_cgroup_line(it->first);
    }

  return it->second;
}

std::string
make_process_cgroup_name(const std::string& cgroup_file_text)
{
  // name -> [cat...], sorted by name;
  std::map<std::string, std::vector<std::string> > names;

  std::string::size_type last = 0, eol;

  // for each line in the file
  while ((eol = cgroup_file_text.find ('\n', last)) != std::string::npos)
    {
      auto line = cgroup_file_text.substr (last, eol - last);
      last = eol + 1;

      const auto& line_data = parse_cgroup_line (line);
      if (!line_data.name.empty ())
        names[line_data.name].push_back (line_data.cat);
    }


  // name (cat1, cat2), ...
  // sorted by name, categories
  std::vector<std::string> groups;

  for (auto&i : names)
    {
      std::sort (begin (i.second), end (i.second));
      std::string cats = procman::join<std::string> (i.second, ", "sv);
      groups.push_back (i.first + " (" + cats + ')');
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
