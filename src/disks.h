#ifndef _GSM_DISKS_H_
#define _GSM_DISKS_H_

#include "application.h"

void create_disk_view (GsmApplication *app,
                       GtkBuilder     *builder);

void disks_update (GsmApplication *app);
void disks_freeze (GsmApplication *app);
void disks_thaw (GsmApplication *app);
void disks_reset_timeout (GsmApplication *app);

#endif /* _GSM_DISKS_H_ */
