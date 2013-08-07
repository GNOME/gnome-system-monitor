/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _GSM_CGROUPS_H_
#define _GSM_CGROUPS_H_

#include <glib.h>

#include "application.h"

void
get_process_cgroup_info (ProcInfo *info);

gboolean
cgroups_enabled (void);

#endif /* _GSM_CGROUPS_H_ */
