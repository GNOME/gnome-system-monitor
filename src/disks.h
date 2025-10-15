/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define GSM_TYPE_DISKS_VIEW (gsm_disks_view_get_type ())
G_DECLARE_FINAL_TYPE (GsmDisksView, gsm_disks_view, GSM, DISKS_VIEW, AdwBin)

GListModel    *gsm_disks_view_get_columns     (GsmDisksView *self);

G_END_DECLS
