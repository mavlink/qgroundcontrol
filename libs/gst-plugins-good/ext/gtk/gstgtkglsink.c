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
 * SECTION:element-gtkglsink
 * @title: gtkglsink
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gl/gstglfuncs.h>

#include "gstgtkglsink.h"
#include "gtkgstglwidget.h"

GST_DEBUG_CATEGORY (gst_debug_gtk_gl_sink);
#define GST_CAT_DEFAULT gst_debug_gtk_gl_sink

static gboolean gst_gtk_gl_sink_start (GstBaseSink * bsink);
static gboolean gst_gtk_gl_sink_stop (GstBaseSink * bsink);
static gboolean gst_gtk_gl_sink_query (GstBaseSink * bsink, GstQuery * query);
static gboolean gst_gtk_gl_sink_propose_allocation (GstBaseSink * bsink,
    GstQuery * query);
static GstCaps *gst_gtk_gl_sink_get_caps (GstBaseSink * bsink,
    GstCaps * filter);

static void gst_gtk_gl_sink_finalize (GObject * object);

static GstStaticPadTemplate gst_gtk_gl_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_GL_MEMORY, "RGBA") "; "
        GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_GL_MEMORY ", "
            GST_CAPS_FEATURE_META_GST_VIDEO_OVERLAY_COMPOSITION, "RGBA")));

#define gst_gtk_gl_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstGtkGLSink, gst_gtk_gl_sink,
    GST_TYPE_GTK_BASE_SINK, GST_DEBUG_CATEGORY_INIT (gst_debug_gtk_gl_sink,
        "gtkglsink", 0, "Gtk GL Video Sink"));

static void
gst_gtk_gl_sink_class_init (GstGtkGLSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstGtkBaseSinkClass *gstgtkbasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstgtkbasesink_class = (GstGtkBaseSinkClass *) klass;

  gobject_class->finalize = gst_gtk_gl_sink_finalize;

  gstbasesink_class->query = gst_gtk_gl_sink_query;
  gstbasesink_class->propose_allocation = gst_gtk_gl_sink_propose_allocation;
  gstbasesink_class->start = gst_gtk_gl_sink_start;
  gstbasesink_class->stop = gst_gtk_gl_sink_stop;
  gstbasesink_class->get_caps = gst_gtk_gl_sink_get_caps;

  gstgtkbasesink_class->create_widget = gtk_gst_gl_widget_new;
  gstgtkbasesink_class->window_title = "Gtk+ GL renderer";

  gst_element_class_set_metadata (gstelement_class, "Gtk GL Video Sink",
      "Sink/Video", "A video sink that renders to a GtkWidget using OpenGL",
      "Matthew Waters <matthew@centricular.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_gtk_gl_sink_template);
}

static void
gst_gtk_gl_sink_init (GstGtkGLSink * gtk_sink)
{
}

static gboolean
gst_gtk_gl_sink_query (GstBaseSink * bsink, GstQuery * query)
{
  GstGtkGLSink *gtk_sink = GST_GTK_GL_SINK (bsink);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:
    {
      if (gst_gl_handle_context_query ((GstElement *) gtk_sink, query,
              gtk_sink->display, gtk_sink->context, gtk_sink->gtk_context))
        return TRUE;
      break;
    }
    default:
      res = GST_BASE_SINK_CLASS (parent_class)->query (bsink, query);
      break;
  }

  return res;
}

static void
_size_changed_cb (GtkWidget * widget, GdkRectangle * rectangle,
    GstGtkGLSink * gtk_sink)
{
  gint scale_factor, width, height;
  gboolean reconfigure;

  scale_factor = gtk_widget_get_scale_factor (widget);
  width = scale_factor * gtk_widget_get_allocated_width (widget);
  height = scale_factor * gtk_widget_get_allocated_height (widget);

  GST_OBJECT_LOCK (gtk_sink);
  reconfigure =
      (width != gtk_sink->display_width || height != gtk_sink->display_height);
  gtk_sink->display_width = width;
  gtk_sink->display_height = height;
  GST_OBJECT_UNLOCK (gtk_sink);

  if (reconfigure) {
    GST_DEBUG_OBJECT (gtk_sink, "Sending reconfigure event on sinkpad.");
    gst_pad_push_event (GST_BASE_SINK (gtk_sink)->sinkpad,
        gst_event_new_reconfigure ());
  }
}

static void
destroy_cb (GtkWidget * widget, GstGtkGLSink * gtk_sink)
{
  if (gtk_sink->size_allocate_sig_handler) {
    g_signal_handler_disconnect (widget, gtk_sink->size_allocate_sig_handler);
    gtk_sink->size_allocate_sig_handler = 0;
  }

  if (gtk_sink->widget_destroy_sig_handler) {
    g_signal_handler_disconnect (widget, gtk_sink->widget_destroy_sig_handler);
    gtk_sink->widget_destroy_sig_handler = 0;
  }
}

static gboolean
gst_gtk_gl_sink_start (GstBaseSink * bsink)
{
  GstGtkBaseSink *base_sink = GST_GTK_BASE_SINK (bsink);
  GstGtkGLSink *gtk_sink = GST_GTK_GL_SINK (bsink);
  GtkGstGLWidget *gst_widget;

  if (!GST_BASE_SINK_CLASS (parent_class)->start (bsink))
    return FALSE;

  /* After this point, gtk_sink->widget will always be set */
  gst_widget = GTK_GST_GL_WIDGET (base_sink->widget);

  /* Track the allocation size */
  gtk_sink->size_allocate_sig_handler =
      g_signal_connect (gst_widget, "size-allocate",
      G_CALLBACK (_size_changed_cb), gtk_sink);

  gtk_sink->widget_destroy_sig_handler =
      g_signal_connect (gst_widget, "destroy", G_CALLBACK (destroy_cb),
      gtk_sink);

  _size_changed_cb (GTK_WIDGET (gst_widget), NULL, gtk_sink);

  if (!gtk_gst_gl_widget_init_winsys (gst_widget)) {
    GST_ELEMENT_ERROR (bsink, RESOURCE, NOT_FOUND, ("%s",
            "Failed to initialize OpenGL with Gtk"), (NULL));
    return FALSE;
  }

  gtk_sink->display = gtk_gst_gl_widget_get_display (gst_widget);
  gtk_sink->context = gtk_gst_gl_widget_get_context (gst_widget);
  gtk_sink->gtk_context = gtk_gst_gl_widget_get_gtk_context (gst_widget);

  if (!gtk_sink->display || !gtk_sink->context || !gtk_sink->gtk_context) {
    GST_ELEMENT_ERROR (bsink, RESOURCE, NOT_FOUND, ("%s",
            "Failed to retrieve OpenGL context from Gtk"), (NULL));
    return FALSE;
  }

  gst_gl_element_propagate_display_context (GST_ELEMENT (bsink),
      gtk_sink->display);

  return TRUE;
}

static gboolean
gst_gtk_gl_sink_stop (GstBaseSink * bsink)
{
  GstGtkGLSink *gtk_sink = GST_GTK_GL_SINK (bsink);

  if (gtk_sink->display) {
    gst_object_unref (gtk_sink->display);
    gtk_sink->display = NULL;
  }

  if (gtk_sink->context) {
    gst_object_unref (gtk_sink->context);
    gtk_sink->context = NULL;
  }

  if (gtk_sink->gtk_context) {
    gst_object_unref (gtk_sink->gtk_context);
    gtk_sink->gtk_context = NULL;
  }

  return GST_BASE_SINK_CLASS (parent_class)->stop (bsink);
}

static gboolean
gst_gtk_gl_sink_propose_allocation (GstBaseSink * bsink, GstQuery * query)
{
  GstGtkGLSink *gtk_sink = GST_GTK_GL_SINK (bsink);
  GstBufferPool *pool = NULL;
  GstStructure *config;
  GstCaps *caps;
  GstVideoInfo info;
  guint size;
  gboolean need_pool;
  GstStructure *allocation_meta = NULL;
  gint display_width, display_height;

  if (!gtk_sink->display || !gtk_sink->context)
    return FALSE;

  gst_query_parse_allocation (query, &caps, &need_pool);

  if (caps == NULL)
    goto no_caps;

  if (!gst_video_info_from_caps (&info, caps))
    goto invalid_caps;

  /* the normal size of a frame */
  size = info.size;

  if (need_pool) {
    GST_DEBUG_OBJECT (gtk_sink, "create new pool");
    pool = gst_gl_buffer_pool_new (gtk_sink->context);

    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_set_params (config, caps, size, 0, 0);
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_GL_SYNC_META);

    if (!gst_buffer_pool_set_config (pool, config))
      goto config_failed;
  }

  /* we need at least 2 buffer because we hold on to the last one */
  gst_query_add_allocation_pool (query, pool, size, 2, 0);
  if (pool)
    gst_object_unref (pool);

  GST_OBJECT_LOCK (gtk_sink);
  display_width = gtk_sink->display_width;
  display_height = gtk_sink->display_height;
  GST_OBJECT_UNLOCK (gtk_sink);

  if (display_width != 0 && display_height != 0) {
    GST_DEBUG_OBJECT (gtk_sink, "sending alloc query with size %dx%d",
        display_width, display_height);
    allocation_meta = gst_structure_new ("GstVideoOverlayCompositionMeta",
        "width", G_TYPE_UINT, display_width,
        "height", G_TYPE_UINT, display_height, NULL);
  }

  gst_query_add_allocation_meta (query,
      GST_VIDEO_OVERLAY_COMPOSITION_META_API_TYPE, allocation_meta);

  if (allocation_meta)
    gst_structure_free (allocation_meta);

  /* we also support various metadata */
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, 0);

  if (gtk_sink->context->gl_vtable->FenceSync)
    gst_query_add_allocation_meta (query, GST_GL_SYNC_META_API_TYPE, 0);

  return TRUE;

  /* ERRORS */
no_caps:
  {
    GST_DEBUG_OBJECT (bsink, "no caps specified");
    return FALSE;
  }
invalid_caps:
  {
    GST_DEBUG_OBJECT (bsink, "invalid caps specified");
    return FALSE;
  }
config_failed:
  {
    GST_DEBUG_OBJECT (bsink, "failed setting config");
    return FALSE;
  }
}

static GstCaps *
gst_gtk_gl_sink_get_caps (GstBaseSink * bsink, GstCaps * filter)
{
  GstCaps *tmp = NULL;
  GstCaps *result = NULL;

  tmp = gst_pad_get_pad_template_caps (GST_BASE_SINK_PAD (bsink));

  if (filter) {
    GST_DEBUG_OBJECT (bsink, "intersecting with filter caps %" GST_PTR_FORMAT,
        filter);

    result = gst_caps_intersect_full (filter, tmp, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (tmp);
  } else {
    result = tmp;
  }

  result = gst_gl_overlay_compositor_add_caps (result);

  GST_DEBUG_OBJECT (bsink, "returning caps: %" GST_PTR_FORMAT, result);

  return result;
}

static void
gst_gtk_gl_sink_finalize (GObject * object)
{
  GstGtkGLSink *gtk_sink = GST_GTK_GL_SINK (object);
  GstGtkBaseSink *base_sink = GST_GTK_BASE_SINK (object);

  if (gtk_sink->size_allocate_sig_handler) {
    g_signal_handler_disconnect (base_sink->widget,
        gtk_sink->size_allocate_sig_handler);
    gtk_sink->size_allocate_sig_handler = 0;
  }

  if (gtk_sink->widget_destroy_sig_handler) {
    g_signal_handler_disconnect (base_sink->widget,
        gtk_sink->widget_destroy_sig_handler);
    gtk_sink->widget_destroy_sig_handler = 0;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}
