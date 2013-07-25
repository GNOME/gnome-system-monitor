/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef H_GNOME_SYSTEM_MONITOR_DISKS_1123719137
#define H_GNOME_SYSTEM_MONITOR_DISKS_1123719137

#include "procman-app.h"

void create_disk_view(ProcmanApp *app, GtkBuilder *builder);

void disks_update (ProcmanApp *app);
void disks_freeze (ProcmanApp *app);
void disks_thaw (ProcmanApp *app);
void disks_reset_timeout (ProcmanApp *app);
#endif /* H_GNOME_SYSTEM_MONITOR_DISKLIST_1123719137 */
