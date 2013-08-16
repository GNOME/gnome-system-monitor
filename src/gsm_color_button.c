/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <librsvg/rsvg.h>

#include "gsm_color_button.h"

typedef struct
{
  GtkWidget *cc_dialog;		/* Color chooser dialog */

  gchar *title;			/* Title for the color selection window */
  gchar *text;          /* Only used by GSMCP_TYPE_RECTANGLE */
  
  GdkRGBA color;
  gdouble fraction;		/* Only used by GSMCP_TYPE_PIE */
  guint type;
  cairo_surface_t *image_buffer;
  gdouble highlight;
  gboolean button_down;
  gboolean in_button;
} GsmColorButtonPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GsmColorButton, gsm_color_button, GTK_TYPE_DRAWING_AREA);

/* Properties */
enum
{
  PROP_0,
  PROP_PERCENTAGE,
  PROP_TITLE,
  PROP_COLOR,
  PROP_TYPE, 
  PROP_TEXT
};

/* Signals */
enum
{
  COLOR_SET,
  LAST_SIGNAL
};

#define GSMCP_MIN_WIDTH 28
#define GSMCP_MIN_HEIGHT 28

static void gsm_color_button_class_init (GsmColorButtonClass * klass);
static void gsm_color_button_init (GsmColorButton * color_button);
static void gsm_color_button_finalize (GObject * object);
static void gsm_color_button_set_property (GObject * object, guint param_id,
					   const GValue * value,
					   GParamSpec * pspec);
static void gsm_color_button_get_property (GObject * object, guint param_id,
					   GValue * value,
					   GParamSpec * pspec);
static gboolean gsm_color_button_draw (GtkWidget *widget,
                                       cairo_t   *cr);
static void gsm_color_button_get_preferred_width (GtkWidget * widget,
                                                  gint * minimum,
                                                  gint * natural);
static void gsm_color_button_get_preferred_height (GtkWidget * widget,
                                                   gint * minimum,
                                                   gint * natural);
static gint gsm_color_button_pressed (GtkWidget * widget,
				      GdkEventButton * event);
static gint gsm_color_button_released (GtkWidget * widget,
				      GdkEventButton * event);
static gboolean gsm_color_button_enter_notify (GtkWidget * widget,
					       GdkEventCrossing * event);
static gboolean gsm_color_button_leave_notify (GtkWidget * widget,
					       GdkEventCrossing * event);
/* source side drag signals */
static void gsm_color_button_drag_begin (GtkWidget * widget,
					 GdkDragContext * context,
					 gpointer data);
static void gsm_color_button_drag_data_get (GtkWidget * widget,
					    GdkDragContext * context,
					    GtkSelectionData * selection_data,
					    guint info, guint time,
					    GsmColorButton * color_button);

/* target side drag signals */
static void gsm_color_button_drag_data_received (GtkWidget * widget,
						 GdkDragContext * context,
						 gint x,
						 gint y,
						 GtkSelectionData *
						 selection_data, guint info,
						 guint32 time,
						 GsmColorButton *
						 color_button);


static guint color_button_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry drop_types[] = { {"application/x-color", 0, 0} };

static void
gsm_color_button_class_init (GsmColorButtonClass * klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->get_property = gsm_color_button_get_property;
  gobject_class->set_property = gsm_color_button_set_property;
  gobject_class->finalize = gsm_color_button_finalize;
  widget_class->draw = gsm_color_button_draw;
  widget_class->get_preferred_width  = gsm_color_button_get_preferred_width;
  widget_class->get_preferred_height = gsm_color_button_get_preferred_height;
  widget_class->button_release_event = gsm_color_button_released;
  widget_class->button_press_event = gsm_color_button_pressed;
  widget_class->enter_notify_event = gsm_color_button_enter_notify;
  widget_class->leave_notify_event = gsm_color_button_leave_notify;

  g_object_class_install_property (gobject_class,
				   PROP_PERCENTAGE,
				   g_param_spec_double ("fraction",
							_("Fraction"),
                            // TRANSLATORS: description of the pie color picker's (mem, swap) filled percentage property
							_("Percentage full for pie color pickers"),
							0, 1, 0.5,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_TITLE,
				   g_param_spec_string ("title",
							_("Title"),
							_("The title of the color selection dialog"),
							_("Pick a Color"),
							G_PARAM_READWRITE));
                            
  g_object_class_install_property (gobject_class,
				   PROP_TEXT,
				   g_param_spec_string ("text",
							_("Text"),
							_("The text on the color picker"),
							NULL,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_COLOR,
				   g_param_spec_boxed ("color",
						       _("Current Color"),
						       _("The selected color"),
						       GDK_TYPE_RGBA,
						       G_PARAM_READWRITE));
                               
  g_object_class_install_property (gobject_class,
				   PROP_TYPE,
				   g_param_spec_uint ("type", _("Type"),
						      _("Type of color picker"),
						      0, 4, 0,
						      G_PARAM_READWRITE));

  color_button_signals[COLOR_SET] = g_signal_new ("color-set",
                                                  G_TYPE_FROM_CLASS
                                                  (gobject_class),
                                                  G_SIGNAL_RUN_FIRST,
                                                  0, NULL, NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE, 0);
}


static gboolean
gsm_color_button_draw (GtkWidget *widget, cairo_t * cr)
{
  GsmColorButton *color_button = GSM_COLOR_BUTTON (widget);
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);
  GdkRGBA *color = gdk_rgba_copy(&priv->color);
  gint width, height;
  gdouble radius, arc_start, arc_end;
  gdouble highlight_factor;
  gboolean sensitive = gtk_widget_get_sensitive (widget);
  PangoLayout* layout;
  PangoRectangle extents;
  GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (widget));
  
  if (sensitive && priv->highlight > 0) {
    highlight_factor = 0.125 * priv->highlight;

    color->red = MIN (1.0, color->red + highlight_factor);
    
    color->blue = MIN (1.0, color->blue + highlight_factor) ;
    
    color->green = MIN (1.0, color->green + highlight_factor);
  } else if (!sensitive)
    gtk_style_context_get_color (context, GTK_STATE_FLAG_INSENSITIVE, color);
  
  gdk_cairo_set_source_rgba (cr, color);
  width  = gdk_window_get_width (gtk_widget_get_window (widget));
  height = gdk_window_get_height(gtk_widget_get_window (widget));
  // colored background
  
  cairo_paint (cr);
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
  cairo_rectangle (cr, 0.5, 0.5, width - 1, height - 1);
  cairo_stroke (cr);
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba (cr, 1, 1, 1, 0.4);
  cairo_rectangle (cr, 1.5, 1.5, width - 3, height - 3);
  cairo_stroke (cr);  
  if (priv->text != NULL) {
    // label text with the usage percentage or network rate
    gchar *markup = g_strdup_printf ("<span font='sans'>%s</span>", priv->text);
    layout = pango_cairo_create_layout (cr);
    pango_layout_set_markup (layout, markup, -1);
    g_free (markup);
    pango_layout_get_pixel_extents (layout, NULL, &extents);
  } 
  switch (priv->type)
    {
    case GSMCP_TYPE_RECTANGLE:

      if (priv->text != NULL) {
        gtk_render_layout (context, cr,
                           (width - extents.width) / 2,
                           (height - extents.height) / 2,
                           layout);
      } 
      
      
      break;
    case GSMCP_TYPE_PIE:
      if (width < GSMCP_MIN_WIDTH)		// 24px minimum size
        gtk_widget_set_size_request (widget, GSMCP_MIN_WIDTH, -1);
      guint pie_padding = 2;
      guint pie_size = MIN (width, height) - 2*pie_padding;
      radius = pie_size / 2;
      
      if (priv->text != NULL) {
        gtk_render_layout (context, cr,
                           pie_size + 2 * pie_padding + (width - pie_size - 2 * pie_padding- extents.width) / 2,
                           (height - extents.height) / 2,
                           layout);
      }
      arc_start = -G_PI_2 + 2 * G_PI * priv->fraction;
      arc_end = -G_PI_2;

      cairo_set_line_width (cr, 1);
      cairo_set_operator (cr, CAIRO_OPERATOR_ATOP);

      // Draw internal highlight
      cairo_set_source_rgba (cr, 1, 1, 1, 0.45);
      cairo_set_line_width (cr, 1);

      if (priv->fraction < 0.03) {
        cairo_arc (cr, pie_padding+pie_size/2 + .5, pie_padding+pie_size/2, 3.25,
			    0, 2 * G_PI);
      } else if (priv->fraction > 0.99) {
        cairo_arc (cr, pie_padding+pie_size/2 + .5, pie_padding+pie_size/2, radius - 3.5,
			    0, 2 * G_PI);
      } else {
        cairo_arc_negative (cr, pie_padding+pie_size/2 + .5, pie_padding+pie_size/2, radius - 3.5,
		 arc_start + (1 / (radius - 3.75)), 
		 arc_end - (1 / (radius - 3.75)));
        cairo_arc_negative (cr, pie_padding+pie_size/2 + .5, pie_padding+pie_size/2, 3.25,
		   arc_end - (1 / (radius - 3.75)),
                   arc_start + (1 / (radius - 3.75)));
        cairo_arc_negative (cr, pie_padding+pie_size/2 + .5, pie_padding+pie_size/2, radius - 3.5,
		 arc_start + (1 / (radius - 3.75)), 
		 arc_start + (1 / (radius - 3.75)));
      }
      cairo_fill (cr);

      // Draw external shape
      cairo_set_line_width (cr, 1);
      cairo_set_source_rgba (cr, 0, 0, 0, 0.2);
      cairo_arc (cr, pie_padding+pie_size/2 + .5, pie_padding+pie_size/2, radius - 2.25, 0,
		 G_PI * 2);
      cairo_stroke (cr);

      break;
    }

  return FALSE;
}

static void
gsm_color_button_get_preferred_width (GtkWidget * widget,
                                      gint      * minimum,
                                      gint      * natural)
{
  if (minimum)
    *minimum = GSMCP_MIN_WIDTH;
  if (natural)
    *natural = GSMCP_MIN_WIDTH;
}

static void
gsm_color_button_get_preferred_height (GtkWidget * widget,
                                       gint      * minimum,
                                       gint      * natural)
{
  if (minimum)
    *minimum = GSMCP_MIN_HEIGHT;
  if (natural)
    *natural = GSMCP_MIN_HEIGHT;
}

static void
gsm_color_button_drag_data_received (GtkWidget * widget,
				     GdkDragContext * context,
				     gint x,
				     gint y,
				     GtkSelectionData * selection_data,
				     guint info,
				     guint32 time,
				     GsmColorButton * color_button)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);

  gint length;
  guint16 *dropped;

  length = gtk_selection_data_get_length (selection_data);

  if (length < 0)
    return;

  /* We accept drops with the wrong format, since the KDE color
   * chooser incorrectly drops application/x-color with format 8.
   */
  if (length != 8)
    {
      g_warning (_("Received invalid color data\n"));
      return;
    }


  dropped = (guint16 *) gtk_selection_data_get_data (selection_data);

  priv->color.red   = (gdouble)dropped[0] / 0xffff;
  priv->color.green = (gdouble)dropped[1] / 0xffff;
  priv->color.blue  = (gdouble)dropped[2] / 0xffff;

  gtk_widget_queue_draw (GTK_WIDGET (color_button));

  g_signal_emit (color_button, color_button_signals[COLOR_SET], 0);

  g_object_freeze_notify (G_OBJECT (color_button));
  g_object_notify (G_OBJECT (color_button), "color");
  g_object_thaw_notify (G_OBJECT (color_button));
}


static void
set_color_icon (GdkDragContext * context, GdkRGBA * color)
{
  GdkPixbuf *pixbuf;
  guint32 pixel;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 48, 32);

  pixel = ((guint32)(color->red * 0xff) << 24) |
          ((guint32)(color->green * 0xff) << 16) |
          ((guint32)(color->blue * 0xff) << 8);

  gdk_pixbuf_fill (pixbuf, pixel);

  gtk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
  g_object_unref (pixbuf);
}

static void
gsm_color_button_drag_begin (GtkWidget * widget,
			     GdkDragContext * context, gpointer data)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (GSM_COLOR_BUTTON (data));
  set_color_icon (context, &priv->color);
}

static void
gsm_color_button_drag_data_get (GtkWidget * widget,
				GdkDragContext * context,
				GtkSelectionData * selection_data,
				guint info,
				guint time, GsmColorButton * color_button)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);
  guint16 dropped[4];

  dropped[0] = priv->color.red * 0xffff;
  dropped[1] = priv->color.green * 0xffff;
  dropped[2] = priv->color.blue * 0xffff;
  dropped[3] = 65535;		// This widget doesn't care about alpha

  gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data),
			  16, (guchar *) dropped, 8);
}

static const char css_data[] =
    "* {"
    "    color: white;"
    "    text-shadow: 1px 1px black;"
    "}";

static void
gsm_color_button_init (GsmColorButton * color_button)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);
  GtkStyleContext *context;
  GtkCssProvider *css_provider;

  priv->color.red = 0;
  priv->color.green = 0;
  priv->color.blue = 0;
  priv->fraction = 0.5;
  priv->type = GSMCP_TYPE_RECTANGLE;
  priv->image_buffer = NULL;
  priv->title = g_strdup (_("Pick a Color")); 	/* default title */
  priv->text = NULL;
  priv->in_button = FALSE;
  priv->button_down = FALSE;

  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css_provider, css_data, -1, NULL);
  context = gtk_widget_get_style_context (GTK_WIDGET (color_button));
  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_drag_dest_set (GTK_WIDGET (color_button),
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_DROP, drop_types, 1, GDK_ACTION_COPY);
  gtk_drag_source_set (GTK_WIDGET (color_button),
		       GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
		       drop_types, 1, GDK_ACTION_COPY);
  g_signal_connect (color_button, "drag_begin",
		    G_CALLBACK (gsm_color_button_drag_begin), color_button);
  g_signal_connect (color_button, "drag_data_received",
		    G_CALLBACK (gsm_color_button_drag_data_received),
		    color_button);
  g_signal_connect (color_button, "drag_data_get",
		    G_CALLBACK (gsm_color_button_drag_data_get),
		    color_button);

  gtk_widget_add_events (GTK_WIDGET(color_button), GDK_ENTER_NOTIFY_MASK
			 			 | GDK_LEAVE_NOTIFY_MASK);

  gtk_widget_set_tooltip_text (GTK_WIDGET(color_button), _("Click to set graph colors"));
}

static void
gsm_color_button_finalize (GObject * object)
{
  GsmColorButton *color_button = GSM_COLOR_BUTTON (object);
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);

  if (priv->cc_dialog != NULL)
    gtk_widget_destroy (priv->cc_dialog);
  priv->cc_dialog = NULL;

  g_free (priv->title);
  priv->title = NULL;
  if (priv->text != NULL)
    g_free (priv->text);
  priv->text = NULL;
  cairo_surface_destroy (priv->image_buffer);
  priv->image_buffer = NULL;

  G_OBJECT_CLASS (gsm_color_button_parent_class)->finalize (object);
}

GtkWidget *
gsm_color_button_new (const GdkRGBA * color, guint type)
{
  return g_object_new (GSM_TYPE_COLOR_BUTTON, "color", color, "type", type,
		               "visible", TRUE, NULL);
}

static void
dialog_response (GtkWidget * widget, GtkResponseType response, gpointer data)
{
  GsmColorButton *color_button = GSM_COLOR_BUTTON (data);
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);
  GtkColorChooser *color_chooser;

  if (response == GTK_RESPONSE_OK) {
    color_chooser = GTK_COLOR_CHOOSER (priv->cc_dialog);

    gtk_color_chooser_get_rgba (color_chooser, &priv->color);

    gtk_widget_hide (priv->cc_dialog);

    gtk_widget_queue_draw (GTK_WIDGET (color_button));

    g_signal_emit (color_button, color_button_signals[COLOR_SET], 0);

    g_object_freeze_notify (G_OBJECT (color_button));
    g_object_notify (G_OBJECT (color_button), "color");
    g_object_thaw_notify (G_OBJECT (color_button));
  }
  else  /* (response == GTK_RESPONSE_CANCEL) */
    gtk_widget_hide (priv->cc_dialog);
}

static gboolean
dialog_destroy (GtkWidget * widget, gpointer data)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (GSM_COLOR_BUTTON (widget));

  priv->cc_dialog = NULL;

  return FALSE;
}

static gint
gsm_color_button_clicked (GtkWidget * widget, GdkEventButton * event)
{
  GsmColorButton *color_button = GSM_COLOR_BUTTON (widget);
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);

  /* if dialog already exists, make sure it's shown and raised */
  if (!priv->cc_dialog)
    {
      /* Create the dialog and connects its buttons */
      GtkWidget *cc_dialog;
      GtkWidget *parent;

      parent = gtk_widget_get_toplevel (GTK_WIDGET (color_button));
      if (!gtk_widget_is_toplevel (parent))
        parent = NULL;

      cc_dialog = gtk_color_chooser_dialog_new (priv->title, GTK_WINDOW (parent));

      gtk_window_set_modal (GTK_WINDOW (cc_dialog), TRUE);

      g_signal_connect (cc_dialog, "response",
                        G_CALLBACK (dialog_response), color_button);

      g_signal_connect (cc_dialog, "destroy",
			G_CALLBACK (dialog_destroy), color_button);

      priv->cc_dialog = cc_dialog;
    }

  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (priv->cc_dialog),
                              &priv->color);

  gtk_window_present (GTK_WINDOW (priv->cc_dialog));
  return 0;
}

static gint
gsm_color_button_pressed (GtkWidget * widget, GdkEventButton * event)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (GSM_COLOR_BUTTON (widget));

  if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1))
    priv->button_down = TRUE;
  return 0;
}

static gint
gsm_color_button_released (GtkWidget * widget, GdkEventButton * event)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (GSM_COLOR_BUTTON (widget));
  if (priv->button_down && priv->in_button)
    gsm_color_button_clicked (widget, event);
  priv->button_down = FALSE;
  return 0;
}


static gboolean
gsm_color_button_enter_notify (GtkWidget * widget, GdkEventCrossing * event)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (GSM_COLOR_BUTTON (widget));
  priv->highlight = 1.0;
  priv->in_button = TRUE;
  gtk_widget_queue_draw(widget);
  return FALSE;
}

static gboolean
gsm_color_button_leave_notify (GtkWidget * widget, GdkEventCrossing * event)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (GSM_COLOR_BUTTON (widget));
  priv->highlight = 0;
  priv->in_button = FALSE;
  gtk_widget_queue_draw(widget);
  return FALSE;
}

guint
gsm_color_button_get_cbtype (GsmColorButton * color_button)
{
  GsmColorButtonPrivate *priv;

  g_return_val_if_fail (GSM_IS_COLOR_BUTTON (color_button), 0);

  priv = gsm_color_button_get_instance_private (color_button);
  return priv->type;
}

void
gsm_color_button_set_cbtype (GsmColorButton * color_button, guint type)
{
  GsmColorButtonPrivate *priv;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));

  priv = gsm_color_button_get_instance_private (color_button);
  priv->type = type;

  gtk_widget_queue_draw (GTK_WIDGET (color_button));

  g_object_notify (G_OBJECT (color_button), "type");
}

gdouble
gsm_color_button_get_fraction (GsmColorButton * color_button)
{
  GsmColorButtonPrivate *priv;

  g_return_val_if_fail (GSM_IS_COLOR_BUTTON (color_button), 0);

  priv = gsm_color_button_get_instance_private (color_button);
  return priv->fraction;
}

void
gsm_color_button_set_fraction (GsmColorButton * color_button,
			       gdouble fraction)
{
  GsmColorButtonPrivate *priv;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));
 
  priv = gsm_color_button_get_instance_private (color_button);

  priv->fraction = fraction;

  gtk_widget_queue_draw (GTK_WIDGET (color_button));

  g_object_notify (G_OBJECT (color_button), "fraction");
}

void
gsm_color_button_get_color (GsmColorButton * color_button, GdkRGBA * color)
{
  GsmColorButtonPrivate *priv;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));

  priv = gsm_color_button_get_instance_private (color_button);

  color->red = priv->color.red;
  color->green = priv->color.green;
  color->blue = priv->color.blue;
  color->alpha = priv->color.alpha;
}

void
gsm_color_button_set_color (GsmColorButton * color_button,
                            const GdkRGBA * color)
{
  GsmColorButtonPrivate *priv;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));
  g_return_if_fail (color != NULL);

  priv = gsm_color_button_get_instance_private (color_button);

  priv->color.red = color->red;
  priv->color.green = color->green;
  priv->color.blue = color->blue;
  priv->color.alpha = color->alpha;

  gtk_widget_queue_draw (GTK_WIDGET (color_button));

  g_object_notify (G_OBJECT (color_button), "color");
}

void
gsm_color_button_set_title (GsmColorButton * color_button,
                            const gchar * title)
{
  GsmColorButtonPrivate *priv;
  gchar *old_title;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));

  priv = gsm_color_button_get_instance_private (color_button);

  old_title = priv->title;
  priv->title = g_strdup (title);
  g_free (old_title);

  if (priv->cc_dialog)
    gtk_window_set_title (GTK_WINDOW (priv->cc_dialog),
                          priv->title);

  g_object_notify (G_OBJECT (color_button), "title");
}

gchar *
gsm_color_button_get_title (GsmColorButton * color_button)
{
  GsmColorButtonPrivate *priv;

  g_return_val_if_fail (GSM_IS_COLOR_BUTTON (color_button), NULL);

  priv = gsm_color_button_get_instance_private (color_button);
  return priv->title;
}

void
gsm_color_button_set_text (GsmColorButton * color_button,
                            const gchar * text)
{
  GsmColorButtonPrivate *priv;
  gchar *old_text;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));

  priv = gsm_color_button_get_instance_private (color_button);

  old_text = priv->text;
  priv->text = g_strdup (text);
  if (old_text != NULL)
    g_free (old_text);

  g_object_notify (G_OBJECT (color_button), "text");
}

gchar *
gsm_color_button_get_text (GsmColorButton * color_button)
{
  GsmColorButtonPrivate *priv;

  g_return_val_if_fail (GSM_IS_COLOR_BUTTON (color_button), NULL);

  priv = gsm_color_button_get_instance_private (color_button);
  return priv->text;
}

static void
gsm_color_button_set_property (GObject * object,
			       guint param_id,
			       const GValue * value, GParamSpec * pspec)
{
  GsmColorButton *color_button = GSM_COLOR_BUTTON (object);

  switch (param_id)
    {
    case PROP_PERCENTAGE:
      gsm_color_button_set_fraction (color_button,
				     g_value_get_double (value));
      break;
    case PROP_TITLE:
      gsm_color_button_set_title (color_button, g_value_get_string (value));
      break;
    case PROP_TEXT:
      gsm_color_button_set_text (color_button, g_value_get_string (value));
      break;
    case PROP_COLOR:
      gsm_color_button_set_color (color_button, g_value_get_boxed (value));
      break;
    case PROP_TYPE:
      gsm_color_button_set_cbtype (color_button, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gsm_color_button_get_property (GObject * object,
			       guint param_id,
			       GValue * value, GParamSpec * pspec)
{
  GsmColorButton *color_button = GSM_COLOR_BUTTON (object);
  GdkRGBA color;

  switch (param_id)
    {
    case PROP_PERCENTAGE:
      g_value_set_double (value,
			  gsm_color_button_get_fraction (color_button));
      break;
    case PROP_TITLE:
      g_value_set_string (value, gsm_color_button_get_title (color_button));
      break;
    case PROP_TEXT:
      g_value_set_string (value, gsm_color_button_get_text (color_button));
      break;
    case PROP_COLOR:
      gsm_color_button_get_color (color_button, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_TYPE:
      g_value_set_uint (value, gsm_color_button_get_cbtype (color_button));
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}
