#include "proclist.h"

#include "procinfo.h"

ProcInfo*
ProcList::find (pid_t pid)
{
  auto it = data.find (pid);

  return (it == data.end () ? nullptr : &it->second);
}
