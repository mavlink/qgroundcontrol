/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "gtkgstwidget.h"
#include <gst/video/video.h>

/**
 * SECTION:gtkgstwidget
 * @title: GtkGstWidget
 * @short_description: a #GtkWidget that renders GStreamer video #GstBuffers
 * @see_also: #GtkDrawingArea, #GstBuffer
 *
 * #GtkGstWidget is an #GtkWidget that renders GStreamer video buffers.
 */

G_DEFINE_TYPE (GtkGstWidget, gtk_gst_widget, GTK_TYPE_DRAWING_AREA);

static gboolean
gtk_gst_widget_draw (GtkWidget * widget, cairo_t * cr)
{
  GtkGstBaseWidget *gst_widget = (GtkGstBaseWidget *) widget;
  guint widget_width, widget_height;
  cairo_surface_t *surface;
  GstVideoFrame frame;

  widget_width = gtk_widget_get_allocated_width (widget);
  widget_height = gtk_widget_get_allocated_height (widget);

  GTK_GST_BASE_WIDGET_LOCK (gst_widget);

  /* There is not much to optimize in term of redisplay, so simply swap the
   * pending_buffer with the active buffer */
  if (gst_widget->pending_buffer) {
    if (gst_widget->buffer)
      gst_buffer_unref (gst_widget->buffer);
    gst_widget->buffer = gst_widget->pending_buffer;
    gst_widget->pending_buffer = NULL;
  }

  /* failed to map the video frame */
  if (gst_widget->negotiated && gst_widget->buffer
      && gst_video_frame_map (&frame, &gst_widget->v_info,
          gst_widget->buffer, GST_MAP_READ)) {
    gdouble scale_x = (gdouble) widget_width / gst_widget->display_width;
    gdouble scale_y = (gdouble) widget_height / gst_widget->display_height;
    GstVideoRectangle result;
    cairo_format_t format;

    gst_widget->v_info = frame.info;
    if (frame.info.finfo->format == GST_VIDEO_FORMAT_ARGB ||
        frame.info.finfo->format == GST_VIDEO_FORMAT_BGRA) {
      format = CAIRO_FORMAT_ARGB32;
    } else {
      format = CAIRO_FORMAT_RGB24;
    }

    surface = cairo_image_surface_create_for_data (frame.data[0],
        format, frame.info.width, frame.info.height, frame.info.stride[0]);

    if (gst_widget->force_aspect_ratio) {
      GstVideoRectangle src, dst;

      src.x = 0;
      src.y = 0;
      src.w = gst_widget->display_width;
      src.h = gst_widget->display_height;

      dst.x = 0;
      dst.y = 0;
      dst.w = widget_width;
      dst.h = widget_height;

      gst_video_sink_center_rect (src, dst, &result, TRUE);

      scale_x = scale_y = MIN (scale_x, scale_y);
    } else {
      result.x = 0;
      result.y = 0;
      result.w = widget_width;
      result.h = widget_height;
    }

    if (gst_widget->ignore_alpha) {
      GdkRGBA color = { 0.0, 0.0, 0.0, 1.0 };

      gdk_cairo_set_source_rgba (cr, &color);
      if (result.x > 0) {
        cairo_rectangle (cr, 0, 0, result.x, widget_height);
        cairo_fill (cr);
      }
      if (result.y > 0) {
        cairo_rectangle (cr, 0, 0, widget_width, result.y);
        cairo_fill (cr);
      }
      if (result.w < widget_width) {
        cairo_rectangle (cr, result.x + result.w, 0, widget_width - result.w,
            widget_height);
        cairo_fill (cr);
      }
      if (result.h < widget_height) {
        cairo_rectangle (cr, 0, result.y + result.h, widget_width,
            widget_height - result.h);
        cairo_fill (cr);
      }
    }

    scale_x *= (gdouble) gst_widget->display_width / (gdouble) frame.info.width;
    scale_y *=
        (gdouble) gst_widget->display_height / (gdouble) frame.info.height;

    cairo_translate (cr, result.x, result.y);
    cairo_scale (cr, scale_x, scale_y);
    cairo_rectangle (cr, 0, 0, result.w, result.h);
    cairo_set_source_surface (cr, surface, 0, 0);
    cairo_paint (cr);

    cairo_surface_destroy (surface);

    gst_video_frame_unmap (&frame);
  } else {
    GdkRGBA color;

    if (gst_widget->ignore_alpha) {
      color.red = color.blue = color.green = 0.0;
      color.alpha = 1.0;
    } else {
      gtk_style_context_get_color (gtk_widget_get_style_context (widget),
          GTK_STATE_FLAG_NORMAL, &color);
    }
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_rectangle (cr, 0, 0, widget_width, widget_height);
    cairo_fill (cr);
  }

  GTK_GST_BASE_WIDGET_UNLOCK (gst_widget);
  return FALSE;
}

static void
gtk_gst_widget_finalize (GObject * object)
{
  gtk_gst_base_widget_finalize (object);

  G_OBJECT_CLASS (gtk_gst_widget_parent_class)->finalize (object);
}

static void
gtk_gst_widget_class_init (GtkGstWidgetClass * klass)
{
  GObjectClass *gobject_klass = (GObjectClass *) klass;
  GtkWidgetClass *widget_klass = (GtkWidgetClass *) klass;

  gtk_gst_base_widget_class_init (GTK_GST_BASE_WIDGET_CLASS (klass));
  gobject_klass->finalize = gtk_gst_widget_finalize;
  widget_klass->draw = gtk_gst_widget_draw;
}

static void
gtk_gst_widget_init (GtkGstWidget * widget)
{
  gtk_gst_base_widget_init (GTK_GST_BASE_WIDGET (widget));
}

GtkWidget *
gtk_gst_widget_new (void)
{
  return (GtkWidget *) g_object_new (GTK_TYPE_GST_WIDGET, NULL);
}
