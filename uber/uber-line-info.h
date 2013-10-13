/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/*
 * uber-line-info.c
 * Copyright (C) 2013 Robert Roth <robert.roth.off@gmail.com>
 * 
 * uber-line-info.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * uber-line-info.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UBER_LINE_INFO_H_
#define _UBER_LINE_INFO_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <math.h>
#include "g-ring.h"

G_BEGIN_DECLS

#define UBER_TYPE_LINE_INFO             (uber_line_info_get_type ())
#define UBER_LINE_INFO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_LINE_INFO, UberLineInfo))
#define UBER_LINE_INFO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), UBER_TYPE_LINE_INFO, UberLineInfoClass))
#define UBER_IS_LINE_INFO(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UBER_TYPE_LINE_INFO))
#define UBER_IS_LINE_INFO_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), UBER_TYPE_LINE_INFO))
#define UBER_LINE_INFO_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), UBER_TYPE_LINE_INFO, UberLineInfoClass))

#define UBER_LINE_INFO_NO_VALUE (NAN)

typedef struct _UberLineInfoClass UberLineInfoClass;
typedef struct _UberLineInfo UberLineInfo;
typedef struct _UberLineInfoPrivate UberLineInfoPrivate;

#define UBER_LINE_INFO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, UBER_TYPE_LINE_INFO, UberLineInfoPrivate))

struct _UberLineInfoClass
{
    GObjectClass parent_class;
};

struct _UberLineInfo
{
    GObject parent_instance;
};

GType uber_line_info_get_type (void) G_GNUC_CONST;

const gdouble  uber_line_info_get_width      (UberLineInfo     *line);
void           uber_line_info_set_width      (UberLineInfo     *line,
                                              const gdouble    width);
const GRing*   uber_line_info_get_data       (UberLineInfo     *line);
void           uber_line_info_set_data       (UberLineInfo     *line,
                                              GRing            *data);
GdkRGBA       *uber_line_info_get_color      (UberLineInfo     *line);
void           uber_line_info_set_color      (UberLineInfo     *line,
                                              GdkRGBA          *color);
GtkWidget     *uber_line_info_get_label      (UberLineInfo     *line);
void           uber_line_info_set_label      (UberLineInfo     *line,
                                              GtkWidget        *label);
                                    
void           uber_line_info_add_value      (UberLineInfo     *line,
                                              gdouble value);
UberLineInfo*  uber_line_info_new            (guint stride, 
                                              GdkRGBA *color, 
                                              gdouble width);
                                              
G_END_DECLS

#endif /* _UBER_LINE_INFO_H_ */

