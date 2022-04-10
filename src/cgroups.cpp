#include <config.h>
#include "cgroups.h"
#include "util.h"

#include <map>
#include <unordered_map>
#include <utility>

bool
cgroups_enabled (void)
{
  static bool initialized = false;
  static bool has_cgroups;

  if (not initialized)
    {
      initialized = true;
      has_cgroups = Glib::file_test ("/proc/cgroups", Glib::FileTest::EXISTS);
    }

  return has_cgroups;
}



static const std::pair<std::string, std::string> &
parse_cgroup_line (const std::string&line)
{
  static std::unordered_map<std::string, std::pair<std::string, std::string> > line_cache;

  auto it = line_cache.insert ({ line, { "", "" } });

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


static const std::string&
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

  auto it = file_cache.insert ({ text, "" });

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

          const auto&p = parse_cgroup_line (line);
          if (!p.first.empty ())
            names[p.first].push_back (p.second);
        }


      // name (cat1, cat2), ...
      // sorted by name, categories
      std::vector<std::string> groups;

      for (auto&i : names)
        {
          std::sort (begin (i.second), end (i.second));
          std::string cats = procman::join (i.second, ", ");
          groups.push_back (i.first + " (" + cats + ')');
        }

      it.first->second = procman::join (groups, ", ");
    }

  return it.first->second;
}


void
get_process_cgroup_info (ProcInfo&info)
{
  if (not cgroups_enabled ())
    return;

  const auto&cgroup_string = get_process_cgroup_string (info.pid);

  info.cgroup_name = cgroup_string;
}
