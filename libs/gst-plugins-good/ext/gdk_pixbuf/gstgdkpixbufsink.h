/* GStreamer GdkPixbuf sink
 * Copyright (C) 2006-2008 Tim-Philipp Müller <tim centricular net>
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

#ifndef GST_GDK_PIXBUF_SINK_H
#define GST_GDK_PIXBUF_SINK_H

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#define GST_TYPE_GDK_PIXBUF_SINK            (gst_gdk_pixbuf_sink_get_type())
#define GST_GDK_PIXBUF_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GDK_PIXBUF_SINK,GstGdkPixbufSink))
#define GST_GDK_PIXBUF_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GDK_PIXBUF_SINK,GstGdkPixbufSinkClass))
#define GST_IS_GDK_PIXBUF_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GDK_PIXBUF_SINK))
#define GST_IS_GDK_PIXBUF_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GDK_PIXBUF_SINK))

typedef struct _GstGdkPixbufSink       GstGdkPixbufSink;
typedef struct _GstGdkPixbufSinkClass  GstGdkPixbufSinkClass;

/**
 * GstGdkPixbufSink:
 *
 * Opaque element structure.
 */
struct _GstGdkPixbufSink
{
  GstVideoSink  basesink;

  /*< private >*/

  /* current caps */
  GstVideoInfo info;
  gint         width;
  gint         height;
  gint         par_n;
  gint         par_d;
  gboolean     has_alpha;

  /* properties */
  gboolean     post_messages;
  GdkPixbuf  * last_pixbuf;
};

/**
 * GstGdkPixbufSinkClass:
 *
 * Opaque element class structure.
 */
struct _GstGdkPixbufSinkClass 
{
  GstVideoSinkClass  basesinkclass;
};

GType   gst_gdk_pixbuf_sink_get_type (void);

#endif /* GST_GDK_PIXBUF_SINK_H */

