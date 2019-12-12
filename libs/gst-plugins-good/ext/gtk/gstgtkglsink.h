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

#ifndef __GST_GTK_GL_SINK_H__
#define __GST_GTK_GL_SINK_H__

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>
#include <gst/gl/gl.h>

#include "gstgtkbasesink.h"


#define GST_TYPE_GTK_GL_SINK            (gst_gtk_gl_sink_get_type())
#define GST_GTK_GL_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GTK_GL_SINK,GstGtkGLSink))
#define GST_GTK_GL_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GTK_GL_SINK,GstGtkGLSinkClass))
#define GST_IS_GTK_GL_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GTK_GL_SINK))
#define GST_IS_GTK_GL_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GTK_GL_SINK))
#define GST_GTK_GL_SINK_CAST(obj)       ((GstGtkGLSink*)(obj))

G_BEGIN_DECLS

typedef struct _GstGtkGLSink GstGtkGLSink;
typedef struct _GstGtkGLSinkClass GstGtkGLSinkClass;

GType gst_gtk_gl_sink_get_type (void);

/**
 * GstGtkGLSink:
 *
 * Opaque #GstGtkGLSink object
 */
struct _GstGtkGLSink
{
  /* <private> */
  GstGtkBaseSink        parent;

  GstGLDisplay         *display;
  GstGLContext         *context;
  GstGLContext         *gtk_context;

  GstGLUpload          *upload;
  GstBuffer            *uploaded_buffer;

  /* read/write with object lock */
  gint                  display_width;
  gint                  display_height;

  gulong                size_allocate_sig_handler;
  gulong                widget_destroy_sig_handler;
};

/**
 * GstGtkGLSinkClass:
 *
 * The #GstGtkGLSinkClass struct only contains private data
 */
struct _GstGtkGLSinkClass
{
  /* <private> */
  GstGtkBaseSinkClass object_class;
};

G_END_DECLS

#endif /* __GST_GTK_GL_SINK_H__ */
