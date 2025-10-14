/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSM_TYPE_COLUMN_VIEW_PERSISTER (gsm_column_view_persister_get_type ())
G_DECLARE_FINAL_TYPE (GsmColumnViewPersister, gsm_column_view_persister, GSM, COLUMN_VIEW_PERSISTER, GObject)

G_END_DECLS
