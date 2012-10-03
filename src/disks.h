#ifndef H_GNOME_SYSTEM_MONITOR_DISKS_1123719137
#define H_GNOME_SYSTEM_MONITOR_DISKS_1123719137

#include "procman-app.h"

void
create_disk_view(ProcmanApp *app, GtkBuilder *builder);

int
cb_update_disks(gpointer procdata);

#endif /* H_GNOME_SYSTEM_MONITOR_DISKLIST_1123719137 */
