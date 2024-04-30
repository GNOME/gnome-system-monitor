#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSM_TYPE_DISKS_VIEW (gsm_disks_view_get_type ())

G_DECLARE_FINAL_TYPE (GsmDisksView, gsm_disks_view, GSM, DISKS_VIEW, GtkWidget)

GsmDisksView *
gsm_disks_view_new (void);

guint
gsm_disks_view_get_timeout (GsmDisksView *self);

GtkColumnView *
gsm_disks_view_get_column_view (GsmDisksView *self);

void
disks_update (GsmDisksView *self);

void
disks_freeze (GsmDisksView *self);

void
disks_thaw (GsmDisksView *self);

void
disks_reset_timeout (GsmDisksView *self);

G_END_DECLS
