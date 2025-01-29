/*
 * Gnome system monitor colour pickers
 * Copyright (C) 2007 Karl Lattimer <karl@qdh.org.uk>
 * Copyright (C) 2022 Ondřej Míchal <harrymichal@seznam.cz>
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

#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <librsvg/rsvg.h>

#include "gsm_color_button.h"

typedef struct
{
  GtkColorDialog *cc_dialog;     /* Color dialog */

  gchar *title;                 /* Title for the color selection window */

  GdkRGBA color;
  gdouble fraction;             /* Only used by GSMCP_TYPE_PIE */
  guint type;
  cairo_surface_t *image_buffer;
  cairo_surface_t *mask_buffer;
  gdouble highlight;
  GCancellable *cancellable;
} GsmColorButtonPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GsmColorButton, gsm_color_button, GTK_TYPE_WIDGET)

/* Properties */
enum
{
  PROP_0,
  PROP_PERCENTAGE,
  PROP_TITLE,
  PROP_COLOR,
  PROP_TYPE,
};

/* Signals */
enum
{
  COLOR_SET,
  LAST_SIGNAL
};

#define GSMCP_MIN_WIDTH 15
#define GSMCP_MIN_HEIGHT 15

static guint color_button_signals[LAST_SIGNAL] = {
  0
};

static cairo_surface_t *
fill_image_buffer_from_resource (cairo_t    *cr,
                                 const char *path)
{
  GBytes *bytes;
  const guint8 *data;
  gsize len;
  GError *error = NULL;
  RsvgHandle *handle;
  RsvgRectangle viewport = {
    0, 0, 32, 32
  };
  cairo_surface_t *tmp_surface;
  cairo_t *tmp_cr;

  bytes = g_resources_lookup_data (path, 0, NULL);
  data = g_bytes_get_data (bytes, &len);

  handle = rsvg_handle_new_from_data (data, len, &error);
  if (handle == NULL)
    {
      g_warning ("rsvg_handle_new_from_data(\"%s\") failed: %s",
                 path, (error ? error->message : "unknown error"));
      if (error)
        g_error_free (error);
      g_bytes_unref (bytes);
      return NULL;
    }

  tmp_surface = cairo_surface_create_similar (cairo_get_target (cr),
                                              CAIRO_CONTENT_COLOR_ALPHA,
                                              32, 32);
  tmp_cr = cairo_create (tmp_surface);
  if (!rsvg_handle_render_document (handle, tmp_cr, &viewport, &error))
    {
      g_warning ("rsvg_handle_render_document(\"%s\") failed: %s",
                 path, (error ? error->message : "unknown error"));
      if (error)
        g_error_free (error);
      g_bytes_unref (bytes);
      return NULL;
    }

  cairo_destroy (tmp_cr);
  g_object_unref (handle);
  g_bytes_unref (bytes);
  return tmp_surface;
}

static void
set_color_icon (GtkDragSource *drag_source,
                GdkRGBA       *color)
{
  GdkPixbuf *pixbuf;
  GdkTexture *texture;
  guint32 pixel;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 48, 32);

  pixel = ((guint32)(color->red * 0xff) << 24) |
          ((guint32)(color->green * 0xff) << 16) |
          ((guint32)(color->blue * 0xff) << 8);

  gdk_pixbuf_fill (pixbuf, pixel);

  texture = gdk_texture_new_for_pixbuf (pixbuf);

  gtk_drag_source_set_icon (drag_source, GDK_PAINTABLE (texture), -2, -2);
  g_object_unref (pixbuf);
}

static void
gsm_color_button_draw_colored_icon (cairo_t         *cr,
                                    cairo_surface_t *image_buffer,
                                    cairo_surface_t *mask_buffer,
                                    gboolean         upside_down)
{
  if (upside_down)
    {
      cairo_translate (cr, 16, 16);
      cairo_rotate (cr, G_PI);
      cairo_translate (cr, -16, -13);
    }

  cairo_mask_surface (cr, mask_buffer, 0.0, 0.0);
  cairo_stroke (cr);

  cairo_set_source_surface (cr, image_buffer, 0.0, 0.0);
  cairo_paint (cr);

  if (upside_down)
    {
      cairo_translate (cr, 16, 13);
      cairo_rotate (cr, -G_PI);
      cairo_translate (cr, -16, -16);
    }
}

static void
gsm_color_button_snapshot (GtkWidget   *widget,
                           GtkSnapshot *snapshot)
{
  GsmColorButton *color_button = GSM_COLOR_BUTTON (widget);
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);
  GdkRGBA *color = gdk_rgba_copy (&priv->color);
  GtkStateFlags state_flags = gtk_widget_get_state_flags (widget);
  gint width, height;
  gdouble radius, arc_start, arc_end;
  gdouble highlight_factor;
  gboolean sensitive = gtk_widget_get_sensitive (widget);

  width = gtk_widget_get_width (widget);
  height = gtk_widget_get_height (widget);

  graphene_rect_t bounds;

  graphene_rect_init (&bounds, 0, 0, width, height);
  cairo_t *cr = gtk_snapshot_append_cairo (snapshot, &bounds);

  if (sensitive && state_flags & GTK_STATE_FLAG_PRELIGHT)
    {
      highlight_factor = 0.125;

      color->red = MIN (1.0, color->red + highlight_factor);

      color->blue = MIN (1.0, color->blue + highlight_factor);

      color->green = MIN (1.0, color->green + highlight_factor);
    }
  else if (!sensitive)
    {
      gtk_widget_get_color (widget, color);
    }
  gdk_cairo_set_source_rgba (cr, color);
  gdk_rgba_free (color);

  switch (priv->type)
    {
      case GSMCP_TYPE_CPU:
        //gtk_widget_set_size_request (widget, GSMCP_MIN_WIDTH, GSMCP_MIN_HEIGHT);
        cairo_paint (cr);
        cairo_set_line_width (cr, 1);
        cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
        cairo_rectangle (cr, 0.5, 0.5, width - 1, height - 1);
        cairo_stroke (cr);
        cairo_set_line_width (cr, 1);
        cairo_set_source_rgba (cr, 1, 1, 1, 0.4);
        cairo_rectangle (cr, 1.5, 1.5, width - 3, height - 3);
        cairo_stroke (cr);
        break;

      case GSMCP_TYPE_PIE:
        if (width < 32)         // 32px minimum size
          gtk_widget_set_size_request (widget, 32, 32);
        if (width < height)
          radius = width / 2;
        else
          radius = height / 2;

        arc_start = -G_PI_2 + 2 * G_PI * priv->fraction;
        arc_end = -G_PI_2;

        cairo_set_line_width (cr, 1);

        // Draw external stroke and fill
        if (priv->fraction < 0.01)
          {
            cairo_arc (cr, (width / 2) + .5, (height / 2) + .5, 4.5,
                       0, 2 * G_PI);
          }
        else if (priv->fraction > 0.99)
          {
            cairo_arc (cr, (width / 2) + .5, (height / 2) + .5, radius - 2.25,
                       0, 2 * G_PI);
          }
        else
          {
            cairo_arc_negative (cr, (width / 2) + .5, (height / 2) + .5, radius - 2.25,
                                arc_start, arc_end);
            cairo_arc_negative (cr, (width / 2) + .5, (height / 2) + .5, 4.5,
                                arc_end, arc_start);
            cairo_arc_negative (cr, (width / 2) + .5, (height / 2) + .5, radius - 2.25,
                                arc_start, arc_start);
          }
        cairo_fill_preserve (cr);
        cairo_set_source_rgba (cr, 0, 0, 0, 0.7);
        cairo_stroke (cr);

        // Draw internal highlight
        cairo_set_source_rgba (cr, 1, 1, 1, 0.45);
        cairo_set_line_width (cr, 1);

        if (priv->fraction < 0.03)
          {
            cairo_arc (cr, (width / 2) + .5, (height / 2) + .5, 3.25,
                       0, 2 * G_PI);
          }
        else if (priv->fraction > 0.99)
          {
            cairo_arc (cr, (width / 2) + .5, (height / 2) + .5, radius - 3.5,
                       0, 2 * G_PI);
          }
        else
          {
            cairo_arc_negative (cr, (width / 2) + .5, (height / 2) + .5, radius - 3.5,
                                arc_start + (1 / (radius - 3.75)),
                                arc_end - (1 / (radius - 3.75)));
            cairo_arc_negative (cr, (width / 2) + .5, (height / 2) + .5, 3.25,
                                arc_end - (1 / (radius - 3.75)),
                                arc_start + (1 / (radius - 3.75)));
            cairo_arc_negative (cr, (width / 2) + .5, (height / 2) + .5, radius - 3.5,
                                arc_start + (1 / (radius - 3.75)),
                                arc_start + (1 / (radius - 3.75)));
          }
        cairo_stroke (cr);

        // Draw external shape
        cairo_set_line_width (cr, 1);
        cairo_set_source_rgba (cr, 0, 0, 0, 0.2);
        cairo_arc (cr, (width / 2) + .5, (height / 2) + .5, radius - 1.25, 0,
                   G_PI * 2);
        cairo_stroke (cr);

        break;

      case GSMCP_TYPE_NETWORK_IN:
        if (priv->image_buffer == NULL)
          priv->image_buffer =
            fill_image_buffer_from_resource (cr, "/org/gnome/gnome-system-monitor/pixmaps/arrow_overlay.svg");
        if (priv->mask_buffer == NULL)
          priv->mask_buffer =
            fill_image_buffer_from_resource (cr, "/org/gnome/gnome-system-monitor/pixmaps/arrow_mask.svg");

        gtk_widget_set_size_request (widget, 32, 32);
        gsm_color_button_draw_colored_icon (cr, priv->image_buffer, priv->mask_buffer, TRUE);

        break;

      case GSMCP_TYPE_NETWORK_OUT:
        if (priv->image_buffer == NULL)
          priv->image_buffer =
            fill_image_buffer_from_resource (cr, "/org/gnome/gnome-system-monitor/pixmaps/arrow_overlay.svg");
        if (priv->mask_buffer == NULL)
          priv->mask_buffer =
            fill_image_buffer_from_resource (cr, "/org/gnome/gnome-system-monitor/pixmaps/arrow_mask.svg");

        gtk_widget_set_size_request (widget, 32, 32);
        gsm_color_button_draw_colored_icon (cr, priv->image_buffer, priv->mask_buffer, FALSE);

        break;

      case GSMCP_TYPE_DISK_READ:
        if (priv->image_buffer == NULL)
          priv->image_buffer =
            fill_image_buffer_from_resource (cr, "/org/gnome/gnome-system-monitor/pixmaps/arrow_overlay.svg");
        if (priv->mask_buffer == NULL)
          priv->mask_buffer =
            fill_image_buffer_from_resource (cr, "/org/gnome/gnome-system-monitor/pixmaps/arrow_mask.svg");

        gtk_widget_set_size_request (widget, 32, 32);
        gsm_color_button_draw_colored_icon (cr, priv->image_buffer, priv->mask_buffer, FALSE);

        break;

      case GSMCP_TYPE_DISK_WRITE:
        if (priv->image_buffer == NULL)
          priv->image_buffer =
            fill_image_buffer_from_resource (cr, "/org/gnome/gnome-system-monitor/pixmaps/arrow_overlay.svg");
        if (priv->mask_buffer == NULL)
          priv->mask_buffer =
            fill_image_buffer_from_resource (cr, "/org/gnome/gnome-system-monitor/pixmaps/arrow_mask.svg");

        gtk_widget_set_size_request (widget, 32, 32);
        gsm_color_button_draw_colored_icon (cr, priv->image_buffer, priv->mask_buffer, TRUE);

        break;
    }

    cairo_destroy (cr);
}

static void
gsm_color_button_measure (GtkWidget*,
                          GtkOrientation orientation,
                          int,
                          int           *minimum,
                          int           *natural,
                          int*,
                          int*)
{
  if (orientation & GTK_ORIENTATION_HORIZONTAL)
    {
      *minimum = GSMCP_MIN_WIDTH;
      *natural = GSMCP_MIN_WIDTH;
    }
  if (orientation & GTK_ORIENTATION_VERTICAL)
    {
      *minimum = GSMCP_MIN_HEIGHT;
      *natural = GSMCP_MIN_HEIGHT;
    }
}

static void
gsm_color_button_state_flags_changed (GtkWidget *self,
                                      GtkStateFlags)
{
  gtk_widget_queue_draw (self);
}

static void
dialog_response (GObject      *source,
                 GAsyncResult *result,
                 gpointer      data)
{
  GtkColorDialog *dialog = GTK_COLOR_DIALOG (source);
  GsmColorButton *color_button = data;
  GdkRGBA *color;

  color = gtk_color_dialog_choose_rgba_finish (dialog, result, NULL);

  if (color)
    {
      gsm_color_button_set_color (color_button, color);
      gdk_rgba_free (color);
    }
}

static void
gsm_color_button_released (GtkGestureClick*,
                           gint,
                           gdouble,
                           gdouble,
                           GsmColorButton *color_button)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);
  GtkRoot *parent;

  parent = gtk_widget_get_root (GTK_WIDGET (color_button));

  /* if dialog already exists, make sure it's shown and raised */
  if (!priv->cc_dialog)
    {
      /* Create the dialog and connects its buttons */
      GtkColorDialog *cc_dialog;

      cc_dialog = gtk_color_dialog_new ();

      gtk_color_dialog_set_title (cc_dialog, priv->title);
      gtk_color_dialog_set_modal (cc_dialog, TRUE);

      priv->cc_dialog = cc_dialog;
    }

  g_cancellable_cancel (priv->cancellable);
  priv->cancellable = g_cancellable_new ();

  gtk_color_dialog_choose_rgba (priv->cc_dialog,
                                GTK_WINDOW (parent),
                                &priv->color,
                                priv->cancellable,
                                dialog_response,
                                color_button);
}

static gboolean
gsm_color_button_drag_data_drop (GtkDropTarget*,
                                 const GValue *value,
                                 gdouble,
                                 gdouble,
                                 GsmColorButton *color_button)
{
  if (G_VALUE_HOLDS (value, GDK_TYPE_RGBA))
    gsm_color_button_set_color (color_button, g_value_get_boxed (value));
  else
    return FALSE;

  return TRUE;
}

static void
gsm_color_button_drag_begin (GtkDragSource  *drag_source,
                             GdkDrag*,
                             GsmColorButton *color_button)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);

  set_color_icon (drag_source, &priv->color);
}

static GdkContentProvider *
gsm_color_button_prepare (GtkDragSource*,
                          gdouble,
                          gdouble,
                          GsmColorButton *color_button)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);

  return gdk_content_provider_new_typed (GDK_TYPE_RGBA, &priv->color);
}

static void
gsm_color_button_set_property (GObject      *object,
                               guint         param_id,
                               const GValue *value,
                               GParamSpec   *pspec)
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
gsm_color_button_get_property (GObject    *object,
                               guint       param_id,
                               GValue     *value,
                               GParamSpec *pspec)
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

      case PROP_COLOR:
        gsm_color_button_get_color (color_button, &color);
        g_value_set_boxed (value, &color);
        break;

      case PROP_TYPE:
        g_value_set_uint (value, gsm_color_button_get_cbtype (color_button));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
gsm_color_button_finalize (GObject *object)
{
  GsmColorButton *color_button = GSM_COLOR_BUTTON (object);
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);

  if (priv->cc_dialog != NULL)
    g_object_unref (G_OBJECT (priv->cc_dialog));
  priv->cc_dialog = NULL;

  g_free (priv->title);
  priv->title = NULL;

  cairo_surface_destroy (priv->image_buffer);
  priv->image_buffer = NULL;

  G_OBJECT_CLASS (gsm_color_button_parent_class)->finalize (object);
}

static void
gsm_color_button_class_init (GsmColorButtonClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->get_property = gsm_color_button_get_property;
  gobject_class->set_property = gsm_color_button_set_property;
  gobject_class->finalize = gsm_color_button_finalize;
  widget_class->snapshot = gsm_color_button_snapshot;
  widget_class->measure = gsm_color_button_measure;
  widget_class->state_flags_changed = gsm_color_button_state_flags_changed;

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
                                                      0, 6, 0,
                                                      G_PARAM_READWRITE));

  color_button_signals[COLOR_SET] = g_signal_new ("color-set",
                                                  G_TYPE_FROM_CLASS
                                                    (gobject_class),
                                                  G_SIGNAL_RUN_FIRST,
                                                  0, NULL, NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE, 0);
}

static void
gsm_color_button_init (GsmColorButton *color_button)
{
  GsmColorButtonPrivate *priv = gsm_color_button_get_instance_private (color_button);

  priv->color.red = 0;
  priv->color.green = 0;
  priv->color.blue = 0;
  priv->fraction = 0.5;
  priv->type = GSMCP_TYPE_CPU;
  priv->image_buffer = NULL;
  priv->title = g_strdup (_("Pick a Color"));   /* default title */

  GtkGesture *click_controller = gtk_gesture_click_new ();

  g_signal_connect (click_controller, "released",
                    G_CALLBACK (gsm_color_button_released), color_button);
  gtk_widget_add_controller (GTK_WIDGET (color_button), GTK_EVENT_CONTROLLER (click_controller));

  GtkDragSource *drag_source = gtk_drag_source_new ();

  g_signal_connect (drag_source, "drag-begin",
                    G_CALLBACK (gsm_color_button_drag_begin), color_button);
  g_signal_connect (drag_source, "prepare",
                    G_CALLBACK (gsm_color_button_prepare), color_button);
  gtk_widget_add_controller (GTK_WIDGET (color_button), GTK_EVENT_CONTROLLER (drag_source));

  GtkDropTarget *drop_target = gtk_drop_target_new (GDK_TYPE_RGBA,
                                                    GDK_ACTION_COPY);

  g_signal_connect (drop_target, "drop",
                    G_CALLBACK (gsm_color_button_drag_data_drop), color_button);
  gtk_widget_add_controller (GTK_WIDGET (color_button), GTK_EVENT_CONTROLLER (drop_target));

  gtk_widget_set_tooltip_text (GTK_WIDGET (color_button), _("Click to set graph colors"));
}

GsmColorButton *
gsm_color_button_new (const GdkRGBA *color,
                      guint          type)
{
  return g_object_new (GSM_TYPE_COLOR_BUTTON, "color", color, "type", type,
                       "visible", TRUE, NULL);
}

guint
gsm_color_button_get_cbtype (GsmColorButton *color_button)
{
  GsmColorButtonPrivate *priv;

  g_return_val_if_fail (GSM_IS_COLOR_BUTTON (color_button), 0);

  priv = gsm_color_button_get_instance_private (color_button);
  return priv->type;
}

void
gsm_color_button_set_cbtype (GsmColorButton *color_button,
                             guint           type)
{
  GsmColorButtonPrivate *priv;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));

  priv = gsm_color_button_get_instance_private (color_button);
  priv->type = type;

  gtk_widget_queue_draw (GTK_WIDGET (color_button));

  g_object_notify (G_OBJECT (color_button), "type");
}

gdouble
gsm_color_button_get_fraction (GsmColorButton *color_button)
{
  GsmColorButtonPrivate *priv;

  g_return_val_if_fail (GSM_IS_COLOR_BUTTON (color_button), 0);

  priv = gsm_color_button_get_instance_private (color_button);
  return priv->fraction;
}

void
gsm_color_button_set_fraction (GsmColorButton *color_button,
                               gdouble         fraction)
{
  GsmColorButtonPrivate *priv;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));

  priv = gsm_color_button_get_instance_private (color_button);

  priv->fraction = fraction;

  gtk_widget_queue_draw (GTK_WIDGET (color_button));

  g_object_notify (G_OBJECT (color_button), "fraction");
}

void
gsm_color_button_get_color (GsmColorButton *color_button,
                            GdkRGBA        *color)
{
  GsmColorButtonPrivate *priv;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));

  priv = gsm_color_button_get_instance_private (color_button);

  *color = priv->color;
}

void
gsm_color_button_set_color (GsmColorButton *color_button,
                            const GdkRGBA  *color)
{
  GsmColorButtonPrivate *priv;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));
  g_return_if_fail (color != NULL);

  priv = gsm_color_button_get_instance_private (color_button);

  priv->color = *color;

  gtk_widget_queue_draw (GTK_WIDGET (color_button));

  g_signal_emit (color_button, color_button_signals[COLOR_SET], 0);
  g_object_notify (G_OBJECT (color_button), "color");
}

void
gsm_color_button_set_title (GsmColorButton *color_button,
                            const gchar    *title)
{
  GsmColorButtonPrivate *priv;
  gchar *old_title;

  g_return_if_fail (GSM_IS_COLOR_BUTTON (color_button));

  priv = gsm_color_button_get_instance_private (color_button);

  old_title = priv->title;
  priv->title = g_strdup (title);
  g_free (old_title);

  if (priv->cc_dialog)
    gtk_color_dialog_set_title (priv->cc_dialog,
                                priv->title);

  g_object_notify (G_OBJECT (color_button), "title");
}

gchar *
gsm_color_button_get_title (GsmColorButton *color_button)
{
  GsmColorButtonPrivate *priv;

  g_return_val_if_fail (GSM_IS_COLOR_BUTTON (color_button), NULL);

  priv = gsm_color_button_get_instance_private (color_button);
  return priv->title;
}
