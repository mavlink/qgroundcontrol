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

/**
 * SECTION:element-gtkgstsink
 * @title: gtkgstsink
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gtkgstwidget.h"
#include "gstgtksink.h"

GST_DEBUG_CATEGORY (gst_debug_gtk_sink);
#define GST_CAT_DEFAULT gst_debug_gtk_sink

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define FORMATS "{ BGRx, BGRA }"
#else
#define FORMATS "{ xRGB, ARGB }"
#endif

static GstStaticPadTemplate gst_gtk_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (FORMATS))
    );

#define gst_gtk_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstGtkSink, gst_gtk_sink, GST_TYPE_GTK_BASE_SINK,
    GST_DEBUG_CATEGORY_INIT (gst_debug_gtk_sink, "gtksink", 0,
        "Gtk Video Sink"));

static void
gst_gtk_sink_class_init (GstGtkSinkClass * klass)
{
  GstElementClass *gstelement_class;
  GstGtkBaseSinkClass *base_class;

  gstelement_class = (GstElementClass *) klass;
  base_class = (GstGtkBaseSinkClass *) klass;

  base_class->create_widget = gtk_gst_widget_new;
  base_class->window_title = "Gtk+ Cairo renderer";

  gst_element_class_set_metadata (gstelement_class, "Gtk Video Sink",
      "Sink/Video", "A video sink that renders to a GtkWidget",
      "Matthew Waters <matthew@centricular.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_gtk_sink_template);
}

static void
gst_gtk_sink_init (GstGtkSink * gtk_sink)
{
}
