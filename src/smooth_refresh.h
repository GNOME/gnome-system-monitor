#ifndef _PROCMAN_SMOOTH_REFRESH
#define _PROCMAN_SMOOTH_REFRESH

#include <glib.h>

G_BEGIN_DECLS


typedef struct _SmoothRefresh SmoothRefresh;


/*
  smooth_refresh_new

  @config_interval : pointer to config_interval so we can observe
		     config_interval changes.

  @return : initialized SmoothRefresh
 */

SmoothRefresh*
smooth_refresh_new(const guint * config_interval) G_GNUC_INTERNAL;



/*
  smooth_refresh_reset

  Resets state and re-read config_interval

 */

void
smooth_refresh_reset(SmoothRefresh *sm) G_GNUC_INTERNAL;



/*
  smooth_refresh_destroy

  @sm : SmoothRefresh to destroy

 */

void
smooth_refresh_destroy(SmoothRefresh *sm) G_GNUC_INTERNAL;



/*
  smooth_refresh_get

  Computes the new refresh_interval so that CPU usage is lower than
  SMOOTH_REFRESH_PCPU.

  @new_interval : where the new refresh_interval is stored.

  @return : TRUE is refresh_interval has changed. The new refresh_interval
  is stored in @new_interval. Else FALSE;
 */

gboolean
smooth_refresh_get(SmoothRefresh *sm, guint *new_interval) G_GNUC_INTERNAL;


G_END_DECLS

#endif /* _PROCMAN_SMOOTH_REFRESH */
