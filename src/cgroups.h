#ifndef _GSM_CGROUPS_H_
#define _GSM_CGROUPS_H_

#include "procinfo.h"

void get_process_cgroup_info (ProcInfo&info);
bool cgroups_enabled ();

#endif /* _GSM_CGROUPS_H_ */
