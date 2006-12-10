#ifndef H_GNOME_SYSTEM_MONITOR_DISKS_1123719137
#define H_GNOME_SYSTEM_MONITOR_DISKS_1123719137

#include "procman.h"

G_BEGIN_DECLS

GtkWidget *
create_disk_view(ProcData *procdata);

int
cb_update_disks(gpointer procdata);

G_END_DECLS

#endif /* H_GNOME_SYSTEM_MONITOR_DISKLIST_1123719137 */
