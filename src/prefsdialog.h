#ifndef _GSM_PREFS_DIALOG_H_
#define _GSM_PREFS_DIALOG_H_

#include "application.h"

void create_preferences_dialog (GsmApplication *app);
void on_bits_unit_button_toggled (GtkToggleButton *togglebutton,
                                  gpointer         bits_total_button);

#endif /* _GSM_PREFS_DIALOG_H_ */
