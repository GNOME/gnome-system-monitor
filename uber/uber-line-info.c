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

#include "uber-line-info.h"

enum
{
    PROP_0,

    PROP_COLOR,
    PROP_WIDTH,
    PROP_DATA,
    PROP_LABEL
};

/**
 * uber_line_info_init_ring:
 * @ring: A #GRing.
 *
 * Initialize the #GRing to default values (UBER_LINE_INFO_NO_VALUE).
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
uber_line_info_init_ring (GRing *ring) /* IN */
{
	gdouble val = UBER_LINE_INFO_NO_VALUE;
	gint i;

	g_return_if_fail(ring != NULL);

	for (i = 0; i < ring->len; i++) {
		g_ring_append_val(ring, val);
	}
}

G_DEFINE_TYPE (UberLineInfo, uber_line_info, G_TYPE_OBJECT);

struct _UberLineInfoPrivate
{
	GRing     *raw_data;
	GdkRGBA    color;
	gdouble    width;
	GtkWidget *label;
	guint      label_id;
};

static void
uber_line_info_init (UberLineInfo *uber_line_info)
{
    /*
	 * Keep pointer to private data.
	 */
	UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(uber_line_info);
    priv->width = 1.0;
    priv->label = NULL;
    gdk_rgba_parse(&(priv->color), "#777777");
    priv->raw_data = NULL;
    //priv->dash_offset = 0;
    //priv->dashes = NULL;
    //priv->num_dashes = 0;
  
}

static void
uber_line_info_finalize (GObject *object)
{
  /* TODO: Add deinitalization code here */
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(object);
    g_ring_unref(priv->raw_data);
    G_OBJECT_CLASS (uber_line_info_parent_class)->finalize (object);
}

const gdouble  
uber_line_info_get_width      (UberLineInfo     *line)
{
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);  
    return priv->width;
}

void           
uber_line_info_set_width      (UberLineInfo     *line,
                               const gdouble    width)
{
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);  
    priv->width = width;
}

const GRing*   
uber_line_info_get_data       (UberLineInfo     *line)
{
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);  
    return priv->raw_data;
}

void           
uber_line_info_set_data       (UberLineInfo     *line,
                               GRing            *data)
{
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);  
    if (priv->raw_data)
        g_ring_unref(priv->raw_data);
    priv->raw_data = data;
}

GdkRGBA* uber_line_info_get_color      (UberLineInfo     *line)
{
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);  
    return &(priv->color);
}

void  uber_line_info_set_color      (UberLineInfo     *line,
                                     GdkRGBA          *color)
{
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);  
    priv->color = *color;
}
                                
GtkWidget* uber_line_info_get_label      (UberLineInfo     *line)
{
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);  
    return GTK_WIDGET(priv->label);
}

void  uber_line_info_set_label      (UberLineInfo     *line,
                                     GtkWidget        *label)
{
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);  
    priv->label = label;
}

static void
uber_line_info_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail (UBER_IS_LINE_INFO (object));
    UberLineInfo *line = UBER_LINE_INFO(object);
    switch (prop_id)
    {
        case PROP_COLOR:
        //uber_line_info_set_color(object, g_value_get_boxed(value));
        break;
        case PROP_WIDTH:
        uber_line_info_set_width(line, g_value_get_double(value));
        break;
        case PROP_DATA:
        uber_line_info_set_data(line, g_value_get_boxed(value));
        break;
        case PROP_LABEL:
        uber_line_info_set_label(line, g_value_get_boxed(value));
        break;
        default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
  }
}

static void
uber_line_info_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail (UBER_IS_LINE_INFO (object));

    UberLineInfo *line = UBER_LINE_INFO(object);
    switch (prop_id)
    {
        case PROP_COLOR:
        //g_value_set_boxed(value, uber_line_info_get_color(object));
        break;
        case PROP_WIDTH:
        g_value_set_double(value, uber_line_info_get_width(line));
        break;
        case PROP_DATA:
        g_value_set_boxed(value, uber_line_info_get_data(line));
        break;
        case PROP_LABEL:
        g_value_set_boxed(value, uber_line_info_get_label(line));
        break;
        default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
uber_line_info_class_init (UberLineInfoClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);
  
    object_class->finalize = uber_line_info_finalize;
    object_class->set_property = uber_line_info_set_property;
    object_class->get_property = uber_line_info_get_property;
    g_type_class_add_private(object_class, sizeof(UberLineInfoPrivate));

    g_object_class_install_property (object_class,
                                   PROP_COLOR,
                                   g_param_spec_boxed ("color",
                                                       "color",
                                                       "color",
                                                        GDK_TYPE_RGBA,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));


    g_object_class_install_property (object_class,
                                   PROP_WIDTH,
                                   g_param_spec_double ("width",
                                                        "width",
                                                        "width",
                                                        0.1,
                                                        30.0,
                                                        1.0,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));
    g_object_class_install_property (object_class,
                                   PROP_DATA,
                                   g_param_spec_boxed ("data",
                                                       "data",
                                                       "data",
                                                        G_TYPE_RING,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));
                                                        
    g_object_class_install_property (object_class,
                                   PROP_LABEL,
                                   g_param_spec_boxed ("label",
                                                       "label",
                                                       "color",
                                                        GTK_TYPE_LABEL,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));
}

void
uber_line_info_add_value      (UberLineInfo     *line, gdouble value)
{
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);
    g_ring_append_val(priv->raw_data, value);
}

/**
 * uber_line_info_new:
 *
 * Creates a new instance of #UberLineInfo.
 *
 * Returns: the newly created instance of #UberLineInfo with the specified data length.
 * Side effects: None.
 */
UberLineInfo*
uber_line_info_new (guint stride, GdkRGBA *color, gdouble width)
{
	UberLineInfo *line;

	line = g_object_new(UBER_TYPE_LINE_INFO, NULL);
    UberLineInfoPrivate *priv = UBER_LINE_INFO_GET_PRIVATE(line);
    
    priv->raw_data = g_ring_sized_new(sizeof(gdouble), stride, NULL);
	uber_line_info_init_ring(priv->raw_data);
    priv->color = *color;
    priv->width = width;
    
	return UBER_LINE_INFO(line);
}

