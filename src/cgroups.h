#ifndef PROCMAN_CGROUP_H_20111103
#define PROCMAN_CGROUP_H_20111103

#include <glib.h>

#include "procman.h"

void
get_process_cgroup_info (ProcInfo *info);

gboolean
cgroups_enabled (void);

#endif /* PROCMAN_CGROUP_H_20111103 */
