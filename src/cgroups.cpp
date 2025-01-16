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

const CGroupLineData &
parse_cgroup_line (const std::string&line)
{
  static std::unordered_map<std::string, CGroupLineData> line_cache;

  auto it = line_cache.try_emplace (line, "", "");

  if (it.second)     // inserted new
    {
      std::string::size_type cat_start, name_start;

      if ((cat_start = line.find (':')) != std::string::npos
          && (name_start = line.find (':', cat_start + 1)) != std::string::npos)
        {
          // printf("%s %lu %lu\n", line.c_str(), cat_start, name_start);
          auto cat = line.substr (cat_start + 1, name_start - cat_start - 1);
          auto name = line.substr (name_start + 1);

          // strip the name= prefix
          if (cat.find ("name=") == 0)
            cat.erase (0, 5);

          if (!name.empty () && name != "/")
            {
              it.first->second = { name, cat };
            }
        }
    }

  return it.first->second;
}


const std::string&
get_process_cgroup_string (pid_t pid)
{
  static std::unordered_map<std::string, std::string> file_cache{ { "", "" } };

  /* read out of /proc/pid/cgroup */
  auto path = "/proc/" + std::to_string (pid) + "/cgroup";
  std::string text;

  try {
      text = Glib::file_get_contents (path);
    } catch (...) {
      return file_cache[""];
    }

  auto it = file_cache.try_emplace (std::move(text), "");

  if (it.second)     // inserted new

  // name -> [cat...], sorted by name;
    {
      std::map<std::string, std::vector<std::string> > names;

      std::string::size_type last = 0, eol;

      // for each line in the file
      while ((eol = text.find ('\n', last)) != std::string::npos)
        {
          auto line = text.substr (last, eol - last);
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

      it.first->second = procman::join<std::string> (groups, ", "sv);
    }

  return it.first->second;
}

} // namespace

void
get_process_cgroup_info (ProcInfo&info)
{
  if (not cgroups_enabled ())
    return;

  const auto&cgroup_string = get_process_cgroup_string (info.pid);

  info.cgroup_name = cgroup_string;
}
