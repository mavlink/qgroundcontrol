/*
 * GStreamer
 * Copyright (C) 2023 Matthew Waters <matthew@centricular.com>
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
 * SECTION:gstqml6gmixer
 *
 * `qml6glmixer` provides a way to render an almost-arbitrary QML scene within
 * GStreamer pipeline using the same OpenGL context that GStreamer uses
 * internally.  This avoids attempting to share multiple OpenGL contexts
 * avoiding increased synchronisation points and attempting to share an OpenGL
 * context at runtime which some drivers do not like.  The Intel driver on
 * Windows is a notable example of the last point.
 *
 * `qml6glmixer` will attempt to retrieve the windowing system display connection
 * that Qt is using (#GstGLDisplay).  This may be different to any already
 * existing window system display connection already in use in the pipeline for
 * a number of reasons.  A couple of examples of this are:
 *
 * 1. Adding `qml6glmixer` to an already running pipeline
 * 2. Not having any `qml6glmixer` (or `qml6glsink`, or `qml6gloverlay`) element
 *    start up before any other OpenGL-based element in the pipeline.
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
 * retrieved by either a `qml6glsink` element, a `qml6gloverlay` element or a
 * `qmlglmixer element. The recommended usage is to have either elements
 * (`qml6glsink` or `qml6gloverlay` or `qml6glmixer) be the first to propagate
 * the #GstGLDisplay for the entire pipeline to use by setting either element
 * to the READY element state before any other OpenGL element in the pipeline.
 *
 * In the dynamically adding `qml6glmixer` (or `qml6glsink`, or `qml6gloverlay`)
 * to a pipeline case, there are some considerations for ensuring that the
 * window system display and OpenGL contexts are compatible with Qt.  When the
 * `qml6glmixer` (or `qml6glsink`, or `qml6gloverlay`) element is added and
 * brought up to READY, it will propagate it's own #GstGLDisplay using the
 * #GstContext mechanism regardless of any existing #GstGLDisplay used by the
 * pipeline previously.  In order for the new #GstGLDisplay to be used, the
 * application must then set the provided #GstGLDisplay containing #GstContext
 * on the pipeline.  This may effectively cause each OpenGL element to replace
 * the window system display and also the OpenGL context it is using.  As such
 * this process may take a significant amount of time and resources as objects
 * are recreated in the new OpenGL context.
 *
 * All instances of `qml6glmixer`, `qml6glsink`, and `qml6gloverlay` will return
 * the exact same #GstGLDisplay object while the pipeline is running regardless
 * of whether any `qml6glmixer`, `qml6glsink`, or `qml6gloverlay` elements are
 * added or removed from the pipeline.
 *
 * The Qml scene will run at configured output framerate.  The timestamps on the
 * output buffers are used to drive the animation time.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstqt6elements.h"
#include "gstqml6glmixer.h"
#include "qt6glrenderer.h"
#include "gstqt6glutility.h"

#include <QtGui/QGuiApplication>

#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>

#define GST_CAT_DEFAULT gst_debug_qml6_gl_mixer
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

enum
{
  PROP_PAD_0,
  PROP_PAD_WIDGET,
};

struct _GstQml6GLMixerPad
{
  GstGLMixerPad parent;

  QSharedPointer<Qt6GLVideoItemInterface> widget;
};

G_DEFINE_FINAL_TYPE (GstQml6GLMixerPad, gst_qml6_gl_mixer_pad, GST_TYPE_GL_MIXER_PAD);

static gboolean
gst_qml6_gl_mixer_pad_prepare_frame (GstVideoAggregatorPad *vagg_pad, GstVideoAggregator * vagg,
    GstBuffer *buffer, GstVideoFrame * prepared_frame)
{
  GstQml6GLMixerPad *pad = GST_QML6_GL_MIXER_PAD (vagg_pad);

  if (!GST_VIDEO_AGGREGATOR_PAD_CLASS (gst_qml6_gl_mixer_pad_parent_class)->prepare_frame (vagg_pad, vagg, buffer, prepared_frame))
    return FALSE;

  if (pad->widget) {
    GstMemory *mem;
    GstGLMemory *gl_mem;
    GstCaps *in_caps;
    GstGLContext *context;

    in_caps = gst_video_info_to_caps (&vagg_pad->info);
    gst_caps_set_features_simple (in_caps, gst_caps_features_from_string (GST_CAPS_FEATURE_MEMORY_GL_MEMORY));
    pad->widget->setCaps (in_caps);
    gst_clear_caps (&in_caps);

    mem = gst_buffer_peek_memory (buffer, 0);
    if (!gst_is_gl_memory (mem)) {
      GST_ELEMENT_ERROR (vagg_pad, RESOURCE, NOT_FOUND,
          (NULL), ("Input memory must be a GstGLMemory"));
      return GST_FLOW_ERROR;
    }
    gl_mem = (GstGLMemory *) mem;
    context = gst_gl_base_mixer_get_gl_context (GST_GL_BASE_MIXER (vagg));
    if (!gst_gl_context_can_share (gl_mem->mem.context, context)) {
      GST_WARNING_OBJECT (vagg_pad, "Cannot use the current input texture "
          "(input buffer GL context %" GST_PTR_FORMAT " cannot share "
          "resources with the configured OpenGL context %" GST_PTR_FORMAT ")",
          gl_mem->mem.context, context);
    } else {
      pad->widget->setBuffer (buffer);
    }
  }

  return TRUE;
}

static void
gst_qml6_gl_mixer_pad_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQml6GLMixerPad *qml6_gl_mixer_pad = GST_QML6_GL_MIXER_PAD (object);

  switch (prop_id) {
    case PROP_PAD_WIDGET: {
      Qt6GLVideoItem *qt_item = static_cast<Qt6GLVideoItem *> (g_value_get_pointer (value));
      if (qt_item)
        qml6_gl_mixer_pad->widget = qt_item->getInterface();
      else
        qml6_gl_mixer_pad->widget.clear();
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_qml6_gl_mixer_pad_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstQml6GLMixerPad *qml6_gl_mixer_pad = GST_QML6_GL_MIXER_PAD (object);

  switch (prop_id) {
    case PROP_PAD_WIDGET:
      /* This is not really safe - the app needs to be
       * sure the widget is going to be kept alive or
       * this can crash */
      if (qml6_gl_mixer_pad->widget)
        g_value_set_pointer (value, qml6_gl_mixer_pad->widget->videoItem());
      else
        g_value_set_pointer (value, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_qml6_gl_mixer_pad_finalize (GObject * object)
{
  GstQml6GLMixerPad *pad = GST_QML6_GL_MIXER_PAD (object);

  pad->widget.clear();

  G_OBJECT_CLASS (gst_qml6_gl_mixer_pad_parent_class)->finalize (object);
}

static void
gst_qml6_gl_mixer_pad_class_init (GstQml6GLMixerPadClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstVideoAggregatorPadClass *vagg_pad_class = (GstVideoAggregatorPadClass *) klass;

  gobject_class->set_property = gst_qml6_gl_mixer_pad_set_property;
  gobject_class->get_property = gst_qml6_gl_mixer_pad_get_property;
  gobject_class->finalize = gst_qml6_gl_mixer_pad_finalize;

  g_object_class_install_property (gobject_class, PROP_PAD_WIDGET,
      g_param_spec_pointer ("widget", "QQuickItem",
          "The QQuickItem to place the input video in the object hierarchy",
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  vagg_pad_class->prepare_frame = gst_qml6_gl_mixer_pad_prepare_frame;
}

static void
gst_qml6_gl_mixer_pad_init (GstQml6GLMixerPad * pad)
{
  pad->widget = QSharedPointer<Qt6GLVideoItemInterface>();
}

static void gst_qml6_gl_mixer_finalize (GObject * object);
static void gst_qml6_gl_mixer_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * param_spec);
static void gst_qml6_gl_mixer_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * param_spec);

static gboolean gst_qml6_gl_mixer_process_buffers (GstGLMixer * btrans,
    GstBuffer * outbuf);

static gboolean gst_qml6_gl_mixer_gl_start (GstGLBaseMixer * bmixer);
static void gst_qml6_gl_mixer_gl_stop (GstGLBaseMixer * bmixer);

static GstFlowReturn gst_qml6_gl_mixer_create_output_buffer (GstVideoAggregator * vagg, GstBuffer ** outbuf);

static gboolean gst_qml6_gl_mixer_negotiated_src_caps (GstAggregator * aggregator, GstCaps * out_caps);

static GstStateChangeReturn gst_qml6_gl_mixer_change_state (GstElement * element,
    GstStateChange transition);

enum
{
  PROP_0,
  PROP_QML_SCENE,
  PROP_ROOT_ITEM,
};

enum
{
  SIGNAL_0,
  SIGNAL_QML_SCENE_INITIALIZED,
  SIGNAL_QML_SCENE_DESTROYED,
  LAST_SIGNAL
};

static guint gst_qml6_gl_mixer_signals[LAST_SIGNAL] = { 0 };

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_GL_MEMORY,
            "RGBA"))
    );

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_GL_MEMORY,
            "{ RGBA, BGRA, YV12 }"))
    );

struct _GstQml6GLMixer {
  GstGLMixer parent;

  gchar *qml_scene;

  GstQt6QuickRenderer *renderer;
  GstBuffer *outbuf;
};

#define gst_qml6_gl_mixer_parent_class parent_class
G_DEFINE_FINAL_TYPE_WITH_CODE (GstQml6GLMixer, gst_qml6_gl_mixer,
    GST_TYPE_GL_MIXER, GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT,
        "qml6glmixer", 0, "Qt6 Video Mixer"));
GST_ELEMENT_REGISTER_DEFINE_WITH_CODE (qml6glmixer, "qml6glmixer",
    GST_RANK_NONE, GST_TYPE_QML6_GL_MIXER, qt6_element_init (plugin));

static void
gst_qml6_gl_mixer_class_init (GstQml6GLMixerClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstAggregatorClass *agg_class;
  GstVideoAggregatorClass *vagg_class;
  GstGLBaseMixerClass *glbasemixer_class;
  GstGLMixerClass *glmixer_class;
  GstElementClass *element_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  glbasemixer_class = (GstGLBaseMixerClass *) klass;
  glmixer_class = (GstGLMixerClass *) klass;
  vagg_class = (GstVideoAggregatorClass *) klass;
  agg_class = (GstAggregatorClass *) klass;
  element_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_qml6_gl_mixer_set_property;
  gobject_class->get_property = gst_qml6_gl_mixer_get_property;
  gobject_class->finalize = gst_qml6_gl_mixer_finalize;

  gst_element_class_set_metadata (gstelement_class, "Qt6 Video Mixer",
      "Video/QML/Mixer", "A mixer that renders a QML scene",
      "Matthew Waters <matthew@centricular.com>");

  g_object_class_install_property (gobject_class, PROP_QML_SCENE,
      g_param_spec_string ("qml-scene", "QML Scene",
          "The contents of the QML scene", NULL,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_ROOT_ITEM,
      g_param_spec_pointer ("root-item", "QQuickItem",
          "The root QQuickItem from the qml-scene used to render",
          (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

  /**
   * GstQmlGLMixer::qml-scene-initialized
   * @element: the #GstQmlGLMixer
   * @user_data: user provided data
   */
  gst_qml6_gl_mixer_signals[SIGNAL_QML_SCENE_INITIALIZED] =
      g_signal_new ("qml-scene-initialized", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  /**
   * GstQmlGLMixer::qml-scene-destroyed
   * @element: the #GstQmlGLMixer
   * @user_data: user provided data
   */
  gst_qml6_gl_mixer_signals[SIGNAL_QML_SCENE_DESTROYED] =
      g_signal_new ("qml-scene-destroyed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  glbasemixer_class->gl_start = gst_qml6_gl_mixer_gl_start;
  glbasemixer_class->gl_stop = gst_qml6_gl_mixer_gl_stop;

  glmixer_class->process_buffers = gst_qml6_gl_mixer_process_buffers;

  vagg_class->create_output_buffer = gst_qml6_gl_mixer_create_output_buffer;

  agg_class->negotiated_src_caps = gst_qml6_gl_mixer_negotiated_src_caps;

  element_class->change_state = gst_qml6_gl_mixer_change_state;

  gst_element_class_add_static_pad_template_with_gtype (element_class,
      &src_factory, GST_TYPE_AGGREGATOR_PAD);
  gst_element_class_add_static_pad_template_with_gtype (element_class,
      &sink_factory, GST_TYPE_QML6_GL_MIXER_PAD);
}

static void
gst_qml6_gl_mixer_init (GstQml6GLMixer * qml6_gl_mixer)
{
  qml6_gl_mixer->qml_scene = NULL;
}

static void
gst_qml6_gl_mixer_finalize (GObject * object)
{
  GstQml6GLMixer *qml6_gl_mixer = GST_QML6_GL_MIXER (object);

  g_free (qml6_gl_mixer->qml_scene);
  qml6_gl_mixer->qml_scene = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_qml6_gl_mixer_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQml6GLMixer *qml6_gl_mixer = GST_QML6_GL_MIXER (object);

  switch (prop_id) {
    case PROP_QML_SCENE:
      g_free (qml6_gl_mixer->qml_scene);
      qml6_gl_mixer->qml_scene = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_qml6_gl_mixer_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstQml6GLMixer *qml6_gl_mixer = GST_QML6_GL_MIXER (object);

  switch (prop_id) {
    case PROP_QML_SCENE:
      g_value_set_string (value, qml6_gl_mixer->qml_scene);
      break;
    case PROP_ROOT_ITEM:
      GST_OBJECT_LOCK (qml6_gl_mixer);
      if (qml6_gl_mixer->renderer) {
        QQuickItem *root = qml6_gl_mixer->renderer->rootItem();
        if (root)
          g_value_set_pointer (value, root);
        else
          g_value_set_pointer (value, NULL);
      } else {
        g_value_set_pointer (value, NULL);
      }
      GST_OBJECT_UNLOCK (qml6_gl_mixer);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_qml6_gl_mixer_negotiated_src_caps (GstAggregator * aggregator, GstCaps * out_caps)
{
  GstQml6GLMixer *qml6_gl_mixer = GST_QML6_GL_MIXER (aggregator);
  GstVideoInfo out_info;

  if (!gst_video_info_from_caps (&out_info, out_caps))
    return FALSE;

  qml6_gl_mixer->renderer->setSize (GST_VIDEO_INFO_WIDTH (&out_info),
      GST_VIDEO_INFO_HEIGHT (&out_info));
  
  return GST_AGGREGATOR_CLASS (parent_class)->negotiated_src_caps (aggregator, out_caps);
}

static gboolean
gst_qml6_gl_mixer_gl_start (GstGLBaseMixer * bmixer)
{
  GstQml6GLMixer *qml6_gl_mixer = GST_QML6_GL_MIXER (bmixer);

  QQuickItem *root;
  GError *error = NULL;

  GST_TRACE_OBJECT (bmixer, "using scene:\n%s", qml6_gl_mixer->qml_scene);

  if (!qml6_gl_mixer->qml_scene || g_strcmp0 (qml6_gl_mixer->qml_scene, "") == 0) {
    GST_ELEMENT_ERROR (bmixer, RESOURCE, NOT_FOUND, ("qml-scene property not set"), (NULL));
    return FALSE;
  }

  if (!GST_GL_BASE_MIXER_CLASS (parent_class)->gl_start (bmixer))
    return FALSE;

  GST_OBJECT_LOCK (bmixer);
  qml6_gl_mixer->renderer = new GstQt6QuickRenderer;
  if (!qml6_gl_mixer->renderer->init (bmixer->context, &error)) {
    GST_ELEMENT_ERROR (GST_ELEMENT (bmixer), RESOURCE, NOT_FOUND,
        ("%s", error->message), (NULL));
    delete qml6_gl_mixer->renderer;
    qml6_gl_mixer->renderer = NULL;
    GST_OBJECT_UNLOCK (bmixer);
    return FALSE;
  }

  /* FIXME: Qml may do async loading and we need to propagate qml errors in that case as well */
  if (!qml6_gl_mixer->renderer->setQmlScene (qml6_gl_mixer->qml_scene, &error)) {
    GST_ELEMENT_ERROR (GST_ELEMENT (bmixer), RESOURCE, NOT_FOUND,
        ("%s", error->message), (NULL));
    goto fail_renderer;
  }

  root = qml6_gl_mixer->renderer->rootItem();
  if (!root) {
    GST_ELEMENT_ERROR (GST_ELEMENT (bmixer), RESOURCE, NOT_FOUND,
        ("Qml scene does not have a root item"), (NULL));
    goto fail_renderer;
  }
  GST_OBJECT_UNLOCK (bmixer);

  g_object_notify (G_OBJECT (qml6_gl_mixer), "root-item");
  g_signal_emit (qml6_gl_mixer, gst_qml6_gl_mixer_signals[SIGNAL_QML_SCENE_INITIALIZED], 0);

  return TRUE;

fail_renderer:
  {
    qml6_gl_mixer->renderer->cleanup();
    delete qml6_gl_mixer->renderer;
    qml6_gl_mixer->renderer = NULL;
    GST_OBJECT_UNLOCK (bmixer);
    return FALSE;
  }
}

static void
gst_qml6_gl_mixer_gl_stop (GstGLBaseMixer * bmixer)
{
  GstQml6GLMixer *qml6_gl_mixer = GST_QML6_GL_MIXER (bmixer);
  GstQt6QuickRenderer *renderer = NULL;

  /* notify before actually destroying anything */
  GST_OBJECT_LOCK (qml6_gl_mixer);
  if (qml6_gl_mixer->renderer)
    renderer = qml6_gl_mixer->renderer;
  qml6_gl_mixer->renderer = NULL;
  GST_OBJECT_UNLOCK (qml6_gl_mixer);

  g_signal_emit (qml6_gl_mixer, gst_qml6_gl_mixer_signals[SIGNAL_QML_SCENE_DESTROYED], 0);
  g_object_notify (G_OBJECT (qml6_gl_mixer), "root-item");

  /* TODO: clear all pad buffers in the items?
  if (qml6_gl_mixer->widget)
    qml6_gl_mixer->widget->setBuffer (NULL);
*/
  if (renderer) {
    renderer->cleanup();
    delete renderer;
  }

  GST_GL_BASE_MIXER_CLASS (parent_class)->gl_stop (bmixer);
}

static GstFlowReturn
gst_qml6_gl_mixer_create_output_buffer (GstVideoAggregator * vagg, GstBuffer ** outbuf)
{
  *outbuf = gst_buffer_new();

  return GST_FLOW_OK;
}

static gboolean
qml6_gl_mixer_gl_callback (GstGLContext *context, GstQml6GLMixer * qml6_gl_mixer)
{
  GstVideoAggregator *vagg = GST_VIDEO_AGGREGATOR (qml6_gl_mixer);
  GstGLMemory *out_mem;

  /* XXX: is this the correct ts to drive the animation */
  out_mem = qml6_gl_mixer->renderer->generateOutput (GST_BUFFER_PTS (qml6_gl_mixer->outbuf));
  if (!out_mem) {
    GST_ERROR_OBJECT (qml6_gl_mixer, "Failed to generate output");
    return FALSE;
  }

  gst_buffer_append_memory (qml6_gl_mixer->outbuf, (GstMemory *) out_mem);
  gst_buffer_add_video_meta (qml6_gl_mixer->outbuf, (GstVideoFrameFlags) 0,
      GST_VIDEO_INFO_FORMAT (&vagg->info),
      GST_VIDEO_INFO_WIDTH (&vagg->info),
      GST_VIDEO_INFO_HEIGHT (&vagg->info));

  return TRUE;
}

static gboolean
gst_qml6_gl_mixer_process_buffers (GstGLMixer * mix,
    GstBuffer * outbuf)
{
  GstQml6GLMixer *qml6_gl_mixer = GST_QML6_GL_MIXER (mix);
  GstGLBaseMixer *bmix = GST_GL_BASE_MIXER (mix);
  GstGLContext *context = gst_gl_base_mixer_get_gl_context (bmix);

  qml6_gl_mixer->outbuf = outbuf;
  gst_gl_context_thread_add (context,
    (GstGLContextThreadFunc) qml6_gl_mixer_gl_callback, qml6_gl_mixer);
  qml6_gl_mixer->outbuf = NULL;

  gst_clear_object (&context);

  return TRUE;
}

static GstStateChangeReturn
gst_qml6_gl_mixer_change_state (GstElement * element,
    GstStateChange transition)
{
  GstQml6GLMixer *qml6_gl_mixer = GST_QML6_GL_MIXER (element);
  GstGLBaseMixer *bmixer = GST_GL_BASE_MIXER (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  GST_DEBUG_OBJECT (element, "changing state: %s => %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY: {
      QGuiApplication *app;
      GstGLDisplay *display = NULL;

      app = static_cast<QGuiApplication *> (QCoreApplication::instance ());
      if (!app) {
        GST_ELEMENT_ERROR (element, RESOURCE, NOT_FOUND,
            ("%s", "Failed to connect to Qt"),
            ("%s", "Could not retrieve QGuiApplication instance"));
        return GST_STATE_CHANGE_FAILURE;
      }

      display = gst_qml6_get_gl_display (FALSE);

      if (display != bmixer->display)
        /* always propagate. The application may need to choose between window
         * system display connections */
        gst_gl_element_propagate_display_context (GST_ELEMENT (qml6_gl_mixer), display);
      gst_object_unref (display);
      break;
    }
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    default:
      break;
  }

  return ret;

}
