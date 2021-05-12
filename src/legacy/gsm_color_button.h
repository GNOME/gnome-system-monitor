/*
 * Gnome system monitor colour pickers
 * Copyright (C) 2007 Karl Lattimer <karl@qdh.org.uk>
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with the software; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef _GSM_COLOR_BUTTON_H_
#define _GSM_COLOR_BUTTON_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* The GtkColorSelectionButton widget is a simple color picker in a button.
 * The button displays a sample of the currently selected color. When
 * the user clicks on the button, a color selection dialog pops up.
 * The color picker emits the "color_set" signal when the color is set.
 */
#define GSM_TYPE_COLOR_BUTTON            (gsm_color_button_get_type ())
#define GSM_COLOR_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSM_TYPE_COLOR_BUTTON, GsmColorButton))
#define GSM_COLOR_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSM_TYPE_COLOR_BUTTON, GsmColorButtonClass))
#define GSM_IS_COLOR_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSM_TYPE_COLOR_BUTTON))
#define GSM_IS_COLOR_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSM_TYPE_COLOR_BUTTON))
#define GSM_COLOR_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSM_TYPE_COLOR_BUTTON, GsmColorButtonClass))

typedef struct _GsmColorButton GsmColorButton;
typedef struct _GsmColorButtonClass GsmColorButtonClass;

struct _GsmColorButton
{
  GtkWidget parent_instance;
};

/* Widget types */
enum
{
  GSMCP_TYPE_CPU,
  GSMCP_TYPE_PIE,
  GSMCP_TYPE_NETWORK_IN,
  GSMCP_TYPE_NETWORK_OUT,
  GSMCP_TYPE_DISK_READ,
  GSMCP_TYPE_DISK_WRITE,
  GSMCP_TYPES
};

struct _GsmColorButtonClass
{
  GtkWidgetClass parent_class;
};

GType            gsm_color_button_get_type (void);
GsmColorButton * gsm_color_button_new (const GdkRGBA *color,
                                       guint          type);
void             gsm_color_button_set_color (GsmColorButton *color_button,
                                             const GdkRGBA  *color);
void             gsm_color_button_set_fraction (GsmColorButton *color_button,
                                                const gdouble   fraction);
void             gsm_color_button_set_cbtype (GsmColorButton *color_button,
                                              guint           type);
void             gsm_color_button_get_color (GsmColorButton *color_button,
                                             GdkRGBA        *color);
gdouble          gsm_color_button_get_fraction (GsmColorButton *color_button);
guint            gsm_color_button_get_cbtype (GsmColorButton *color_button);
void             gsm_color_button_set_title (GsmColorButton *color_button,
                                             const gchar    *title);
gchar *          gsm_color_button_get_title (GsmColorButton *color_button);

G_END_DECLS

#endif /* _GSM_COLOR_BUTTON_H_ */
