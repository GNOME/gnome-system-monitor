#ifndef H_GNOME_SYSTEM_MONITOR_GNOMESU_H_1132171917
#define H_GNOME_SYSTEM_MONITOR_GNOMESU_H_1132171917

#include <glib.h>

gboolean
procman_gnomesu_create_root_password_dialog(const char * message);

gboolean
procman_has_gnomesu(void) G_GNUC_CONST;

#endif /* H_GNOME_SYSTEM_MONITOR_GNOMESU_H_1132171917 */
