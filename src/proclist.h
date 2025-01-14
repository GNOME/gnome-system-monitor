#pragma once

#include <map>
#include <mutex>
#include <tuple>

#include "procinfo.h"

class ProcList
{
  // TODO: use a set instead
  // sorted by pid. The map has a nice property : it is sorted
  // by pid so this helps a lot when looking for the parent node
  // as ppid is nearly always < pid.
  typedef std::map<pid_t, ProcInfo> List;
  List data;
  std::mutex data_lock;
  public:
  std::map<pid_t, unsigned long> cpu_times;
  typedef List::iterator Iterator;
  Iterator
  begin ()
  {
    return std::begin (data);
  }
  Iterator
  end ()
  {
    return std::end (data);
  }
  Iterator
  erase (Iterator it)
  {
    std::lock_guard<std::mutex> lg (data_lock);

    return data.erase (it);
  }
  ProcInfo*
  add (pid_t pid)
  {
    return &data.emplace (std::piecewise_construct, std::forward_as_tuple (pid), std::forward_as_tuple (pid)).first->second;
  }
  void
  clear ()
  {
    return data.clear ();
  }

  ProcInfo * find (pid_t pid);
};
