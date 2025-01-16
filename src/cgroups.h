#ifndef _GSM_CGROUPS_H_
#define _GSM_CGROUPS_H_

#include <string>
#include <string_view>

#include "procinfo.h"

bool cgroups_enabled ();
std::string_view get_process_cgroup_name(std::string cgroup_file_text);
void get_process_cgroup_info (ProcInfo&info);

#endif /* _GSM_CGROUPS_H_ */
