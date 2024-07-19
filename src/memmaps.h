#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define GSM_TYPE_MEMMAPS_VIEW (gsm_memmaps_view_get_type ())

G_DECLARE_FINAL_TYPE (GsmMemMapsView, gsm_memmaps_view, GSM, MEMMAPS_VIEW, AdwWindow)

GsmMemMapsView *
gsm_memmaps_view_new (ProcInfo *info);

G_END_DECLS
