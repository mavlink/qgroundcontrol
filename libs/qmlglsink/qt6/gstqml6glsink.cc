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
 * SECTION:gstqml6glsink
 *
 * qml6glsink provides a way to render a video stream as a Qml object inside
 * the Qml scene graph.  This is achieved by providing the incoming OpenGL
 * textures to Qt as a scene graph object.
 *
 * qml6glsink will attempt to retrieve the windowing system display connection
 * that Qt is using (#GstGLDisplay).  This may be different to any already
 * existing window system display connection already in use in the pipeline for
 * a number of reasons.  A couple of examples of this are:
 *
 * 1. Adding qml6glsink to an already running pipeline
 * 2. Not having any qml6glsink element start up before any
 *    other OpenGL-based element in the pipeline.
 *
 * If one of these scenarios occurs, then there will be multiple OpenGL contexts
 * in use in the pipeline.  This means that either the pipeline will fail to
 * start up correctly, a downstream element may reject buffers, or a complete
 * GPU->System memory->GPU transfer is performed for every buffer.
 *
 * The requirement to avoid this is that all elements share the same
 * #GstGLDisplay object and as Qt cannot currently share an existing window
 * system display connection, GStreamer must use the window system display
 * connection provided by Qt.  This window system display connection can be
 * retrieved by either a `qml6glsink` element, a `qml6gloverlay` element, or a
 * `qml6glmixer` element. The recommended usage is to have either element
 * (`qml6glsink`, or `qml6gloverlay`, or `qml6glmixer`) be the first to
 * propagate the #GstGLDisplay for the entire pipeline to use by setting either
 * element to the READY element state before any other OpenGL element in the
 * pipeline.
 *
 * In the dynamically adding `qml6glsink` (or `qml6gloverlay`, or `qml6glmixer`)
 * to a pipeline case, there are some considerations for ensuring that the
 * window system display and OpenGL contexts are compatible with Qt.  When the
 * `qml6gloverlay` (or `qml6glsink`, or `qml6glmixer`) element is added and
 * brought up to READY, it will propagate it's own #GstGLDisplay using the
 * #GstContext mechanism regardless of any existing #GstGLDisplay used by the
 * pipeline previously.  In order for the new #GstGLDisplay to be used, the
 * application must then set the provided #GstGLDisplay containing #GstContext
 * on the pipeline.  This may effectively cause each OpenGL element to replace
 * the window system display and also the OpenGL context it is using.  As such
 * this process may take a significant amount of time and resources as objects
 * are recreated in the new OpenGL context.
 *
 * All instances of `qml6glsink`, `qml6gloverlay`, and `qml6glmixer` will
 * return the exact same #GstGLDisplay object while the pipeline is running
 * regardless of whether any `qml6glsink` or `qml6gloverlay` elements are
 * added or removed from the pipeline.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstqt6elements.h"
#include "gstqml6glsink.h"
#include <QtGui/QGuiApplication>

#include <gst/gl/gstglfuncs.h>

#define GST_CAT_DEFAULT gst_debug_qml6_gl_sink
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

static void gst_qml6_gl_sink_navigation_interface_init (GstNavigationInterface * iface);
static void gst_qml6_gl_sink_finalize (GObject * object);
static void gst_qml6_gl_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * param_spec);
static void gst_qml6_gl_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * param_spec);

static gboolean gst_qml6_gl_sink_stop (GstBaseSink * bsink);

static gboolean gst_qml6_gl_sink_query (GstBaseSink * bsink, GstQuery * query);

static GstStateChangeReturn
gst_qml6_gl_sink_change_state (GstElement * element, GstStateChange transition);

static void gst_qml6_gl_sink_get_times (GstBaseSink * bsink, GstBuffer * buf,
    GstClockTime * start, GstClockTime * end);
static gboolean gst_qml6_gl_sink_set_caps (GstBaseSink * bsink, GstCaps * caps);
static GstFlowReturn gst_qml6_gl_sink_show_frame (GstVideoSink * bsink,
    GstBuffer * buf);
static gboolean gst_qml6_gl_sink_propose_allocation (GstBaseSink * bsink,
    GstQuery * query);

static GstStaticPadTemplate gst_qt_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "
    "format = (string) { RGBA, BGRA, RGB, YV12 }, "
    "width = " GST_VIDEO_SIZE_RANGE ", "
    "height = " GST_VIDEO_SIZE_RANGE ", "
    "framerate = " GST_VIDEO_FPS_RANGE ", "
    "texture-target = (string) 2D"));

#define DEFAULT_FORCE_ASPECT_RATIO  TRUE
#define DEFAULT_PAR_N               0
#define DEFAULT_PAR_D               1

enum
{
  ARG_0,
  PROP_WIDGET,
  PROP_FORCE_ASPECT_RATIO,
  PROP_PIXEL_ASPECT_RATIO,
};

enum
{
  SIGNAL_0,
  LAST_SIGNAL
};

#define gst_qml6_gl_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstQml6GLSink, gst_qml6_gl_sink,
    GST_TYPE_VIDEO_SINK, GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT,
        "qtsink", 0, "Qt Video Sink");
    G_IMPLEMENT_INTERFACE (GST_TYPE_NAVIGATION,
        gst_qml6_gl_sink_navigation_interface_init));
GST_ELEMENT_REGISTER_DEFINE_WITH_CODE (qml6glsink, "qml6glsink",
    GST_RANK_NONE, GST_TYPE_QML6_GL_SINK, qt6_element_init (plugin));

static void
gst_qml6_gl_sink_class_init (GstQml6GLSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstVideoSinkClass *gstvideosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstvideosink_class = (GstVideoSinkClass *) klass;

  gobject_class->set_property = gst_qml6_gl_sink_set_property;
  gobject_class->get_property = gst_qml6_gl_sink_get_property;

  gst_element_class_set_metadata (gstelement_class, "Qt6 Video Sink",
      "Sink/Video", "A video sink that renders to a QQuickItem for Qt6",
      "Matthew Waters <matthew@centricular.com>");

  g_object_class_install_property (gobject_class, PROP_WIDGET,
      g_param_spec_pointer ("widget", "QQuickItem",
          "The QQuickItem to place in the object hierarchy",
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_FORCE_ASPECT_RATIO,
      g_param_spec_boolean ("force-aspect-ratio",
          "Force aspect ratio",
          "When enabled, scaling will respect original aspect ratio",
          DEFAULT_FORCE_ASPECT_RATIO,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_PIXEL_ASPECT_RATIO,
      gst_param_spec_fraction ("pixel-aspect-ratio", "Pixel Aspect Ratio",
          "The pixel aspect ratio of the device", DEFAULT_PAR_N, DEFAULT_PAR_D,
          G_MAXINT, 1, 1, 1,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_add_static_pad_template (gstelement_class, &gst_qt_sink_template);

  gobject_class->finalize = gst_qml6_gl_sink_finalize;

  gstelement_class->change_state = gst_qml6_gl_sink_change_state;
  gstbasesink_class->query = gst_qml6_gl_sink_query;
  gstbasesink_class->set_caps = gst_qml6_gl_sink_set_caps;
  gstbasesink_class->get_times = gst_qml6_gl_sink_get_times;
  gstbasesink_class->propose_allocation = gst_qml6_gl_sink_propose_allocation;
  gstbasesink_class->stop = gst_qml6_gl_sink_stop;

  gstvideosink_class->show_frame = gst_qml6_gl_sink_show_frame;
}

static void
gst_qml6_gl_sink_init (GstQml6GLSink * qt_sink)
{
  qt_sink->widget = QSharedPointer<Qt6GLVideoItemInterface>();
  if (qt_sink->widget)
    qt_sink->widget->setSink (GST_ELEMENT_CAST (qt_sink));
}

static void
gst_qml6_gl_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (object);

  switch (prop_id) {
    case PROP_WIDGET: {
      Qt6GLVideoItem *qt_item = static_cast<Qt6GLVideoItem *> (g_value_get_pointer (value));
      if (qt_item) {
        qt_sink->widget = qt_item->getInterface();
        if (qt_sink->widget) {
          qt_sink->widget->setSink (GST_ELEMENT_CAST (qt_sink));
        }
      } else {
        qt_sink->widget.clear();
      }
      break;
    }
    case PROP_FORCE_ASPECT_RATIO:
      g_return_if_fail (qt_sink->widget);
      qt_sink->widget->setForceAspectRatio (g_value_get_boolean (value));
      break;
    case PROP_PIXEL_ASPECT_RATIO:
      g_return_if_fail (qt_sink->widget);
      qt_sink->widget->setDAR (gst_value_get_fraction_numerator (value),
          gst_value_get_fraction_denominator (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
_reset (GstQml6GLSink * qt_sink)
{
  if (qt_sink->display) {
    gst_object_unref (qt_sink->display);
    qt_sink->display = NULL;
  }

  if (qt_sink->context) {
    gst_object_unref (qt_sink->context);
    qt_sink->context = NULL;
  }

  if (qt_sink->qt_context) {
    gst_object_unref (qt_sink->qt_context);
    qt_sink->qt_context = NULL;
  }
}

static void
gst_qml6_gl_sink_finalize (GObject * object)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (object);

  _reset (qt_sink);

  qt_sink->widget.clear();

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_qml6_gl_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (object);

  switch (prop_id) {
    case PROP_WIDGET:
      /* This is not really safe - the app needs to be
       * sure the widget is going to be kept alive or
       * this can crash */
      if (qt_sink->widget)
        g_value_set_pointer (value, qt_sink->widget->videoItem());
      else
        g_value_set_pointer (value, NULL);
      break;
    case PROP_FORCE_ASPECT_RATIO:
      if (qt_sink->widget)
        g_value_set_boolean (value, qt_sink->widget->getForceAspectRatio ());
      else
        g_value_set_boolean (value, DEFAULT_FORCE_ASPECT_RATIO);
      break;
    case PROP_PIXEL_ASPECT_RATIO:
      if (qt_sink->widget) {
        gint num, den;
        qt_sink->widget->getDAR (&num, &den);
        gst_value_set_fraction (value, num, den);
      } else {
        gst_value_set_fraction (value, DEFAULT_PAR_N, DEFAULT_PAR_D);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_qml6_gl_sink_query (GstBaseSink * bsink, GstQuery * query)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (bsink);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:
    {
      if (gst_gl_handle_context_query ((GstElement *) qt_sink, query,
          qt_sink->display, qt_sink->context, qt_sink->qt_context))
        return TRUE;

      /* fallthrough */
    }
    default:
      res = GST_BASE_SINK_CLASS (parent_class)->query (bsink, query);
      break;
  }

  return res;
}

static gboolean
gst_qml6_gl_sink_stop (GstBaseSink * bsink)
{
  return TRUE;
}

static GstStateChangeReturn
gst_qml6_gl_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  QGuiApplication *app;

  GST_DEBUG ("changing state: %s => %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      app = static_cast<QGuiApplication *> (QCoreApplication::instance ());
      if (!app) {
        GST_ELEMENT_ERROR (element, RESOURCE, NOT_FOUND,
            ("%s", "Failed to connect to Qt"),
            ("%s", "Could not retrieve QGuiApplication instance"));
        return GST_STATE_CHANGE_FAILURE;
      }

      if (!qt_sink->widget) {
        GST_ELEMENT_ERROR (element, RESOURCE, NOT_FOUND,
            ("%s", "Required property \'widget\' not set"),
            (NULL));
        return GST_STATE_CHANGE_FAILURE;
      }

      if (!qt_sink->widget->initWinSys()) {
        GST_ELEMENT_ERROR (element, RESOURCE, NOT_FOUND,
            ("%s", "Could not initialize window system"),
            (NULL));
        return GST_STATE_CHANGE_FAILURE;
      }

      qt_sink->display = qt_sink->widget->getDisplay();
      qt_sink->context = qt_sink->widget->getContext();
      qt_sink->qt_context = qt_sink->widget->getQtContext();

      if (!qt_sink->display || !qt_sink->context || !qt_sink->qt_context) {
        GST_ELEMENT_ERROR (element, RESOURCE, NOT_FOUND,
            ("%s", "Could not retrieve window system OpenGL configuration"),
            (NULL));
        return GST_STATE_CHANGE_FAILURE;
      }

      GST_OBJECT_LOCK (qt_sink->display);
      gst_gl_display_add_context (qt_sink->display, qt_sink->context);
      GST_OBJECT_UNLOCK (qt_sink->display);

      gst_gl_element_propagate_display_context (GST_ELEMENT (qt_sink), qt_sink->display);

      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if (qt_sink->widget)
        qt_sink->widget->setBuffer(NULL);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_qml6_gl_sink_get_times (GstBaseSink * bsink, GstBuffer * buf,
    GstClockTime * start, GstClockTime * end)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (bsink);

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buf)) {
    *start = GST_BUFFER_TIMESTAMP (buf);
    if (GST_BUFFER_DURATION_IS_VALID (buf))
      *end = *start + GST_BUFFER_DURATION (buf);
    else {
      if (GST_VIDEO_INFO_FPS_N (&qt_sink->v_info) > 0) {
        *end = *start +
            gst_util_uint64_scale_int (GST_SECOND,
            GST_VIDEO_INFO_FPS_D (&qt_sink->v_info),
            GST_VIDEO_INFO_FPS_N (&qt_sink->v_info));
      }
    }
  }
}

gboolean
gst_qml6_gl_sink_set_caps (GstBaseSink * bsink, GstCaps * caps)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (bsink);

  GST_DEBUG ("set caps with %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&qt_sink->v_info, caps))
    return FALSE;

  if (!qt_sink->widget)
    return FALSE;

  return qt_sink->widget->setCaps(caps);
}

static GstFlowReturn
gst_qml6_gl_sink_show_frame (GstVideoSink * vsink, GstBuffer * buf)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (vsink);

  GST_TRACE ("rendering buffer:%p", buf);

  if (qt_sink->widget)
    qt_sink->widget->setBuffer(buf);

  return GST_FLOW_OK;
}

static gboolean
gst_qml6_gl_sink_propose_allocation (GstBaseSink * bsink, GstQuery * query)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (bsink);
  GstBufferPool *pool;
  GstStructure *config;
  GstCaps *caps;
  guint size;
  gboolean need_pool;

  if (!qt_sink->display || !qt_sink->context)
    return FALSE;

  gst_query_parse_allocation (query, &caps, &need_pool);

  if (caps == NULL)
    goto no_caps;

  /* FIXME re-using buffer pool breaks renegotiation */
  if ((pool = qt_sink->pool))
    gst_object_ref (pool);

  if (pool != NULL) {
    GstCaps *pcaps;

    /* we had a pool, check caps */
    GST_DEBUG_OBJECT (qt_sink, "check existing pool caps");
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_get_params (config, &pcaps, &size, NULL, NULL);

    if (!gst_caps_is_equal (caps, pcaps)) {
      GST_DEBUG_OBJECT (qt_sink, "pool has different caps");
      /* different caps, we can't use this pool */
      gst_object_unref (pool);
      pool = NULL;
    }
    gst_structure_free (config);
  } else {
    GstVideoInfo info;

    if (!gst_video_info_from_caps (&info, caps))
      goto invalid_caps;

    /* the normal size of a frame */
    size = info.size;
  }

  if (pool == NULL && need_pool) {
  
    GST_DEBUG_OBJECT (qt_sink, "create new pool");
    pool = gst_gl_buffer_pool_new (qt_sink->context);

    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_set_params (config, caps, size, 0, 0);
    if (!gst_buffer_pool_set_config (pool, config))
      goto config_failed;
  }

  /* we need at least 2 buffer because we hold on to the last one */
  gst_query_add_allocation_pool (query, pool, size, 2, 0);
  if (pool)
    gst_object_unref (pool);

  /* we also support various metadata */
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, 0);

  if (qt_sink->context->gl_vtable->FenceSync)
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

static void
gst_qml6_gl_sink_navigation_send_event (GstNavigation * navigation,
                                   GstEvent * event)
{
  GstQml6GLSink *qt_sink = GST_QML6_GL_SINK (navigation);
  GstPad *pad;

  pad = gst_pad_get_peer (GST_VIDEO_SINK_PAD (qt_sink));

  GST_TRACE_OBJECT (qt_sink, "navigation event %" GST_PTR_FORMAT,
      gst_event_get_structure(event));

  if (GST_IS_PAD (pad) && GST_IS_EVENT (event)) {
    if (!gst_pad_send_event (pad, gst_event_ref (event))) {
      /* If upstream didn't handle the event we'll post a message with it
       * for the application in case it wants to do something with it */
      gst_element_post_message (GST_ELEMENT_CAST (qt_sink),
                                gst_navigation_message_new_event (GST_OBJECT_CAST (qt_sink), event));
    }
    gst_event_unref (event);
    gst_object_unref (pad);
  }
}

static void gst_qml6_gl_sink_navigation_interface_init (GstNavigationInterface * iface)
{
  iface->send_event_simple = gst_qml6_gl_sink_navigation_send_event;
}
