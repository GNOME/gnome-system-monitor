/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef H_GNOME_SYSTEM_MONITOR_DISKS_1123719137
#define H_GNOME_SYSTEM_MONITOR_DISKS_1123719137

#include "application.h"

void create_disk_view(GsmApplication *app, GtkBuilder *builder);

void disks_update (GsmApplication *app);
void disks_freeze (GsmApplication *app);
void disks_thaw (GsmApplication *app);
void disks_reset_timeout (GsmApplication *app);
#endif /* H_GNOME_SYSTEM_MONITOR_DISKLIST_1123719137 */
