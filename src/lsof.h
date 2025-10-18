/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define GSM_TYPE_LSOF (gsm_lsof_get_type ())
G_DECLARE_FINAL_TYPE (GsmLsof, gsm_lsof, GSM, LSOF, AdwWindow)

GsmLsof *gsm_lsof_new (GtkWindow *transient_for);

G_END_DECLS
