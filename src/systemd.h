#ifndef PROCMAN_SYSTEMD_H_144CA8D6_BF03_11E4_B291_000C298F6617
#define PROCMAN_SYSTEMD_H_144CA8D6_BF03_11E4_B291_000C298F6617

#include "procinfo.h"

namespace procman
{
bool systemd_logind_running ();
void get_process_systemd_info (ProcInfo *info);
}

#endif
