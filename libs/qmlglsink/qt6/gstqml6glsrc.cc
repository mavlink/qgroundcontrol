/*
 * GStreamer
 * Copyright (C) 2016 Freescale Semiconductor, Inc. All rights reserved.
 * Copyright (C) 2022 Matthew Waters <matthew@centricular.com>
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
 * SECTION:qml6glsrc
 *
 * Since: 1.24
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstqt6elements.h"
#include "gstqml6glsrc.h"
#include <QtGui/QGuiApplication>

#define GST_CAT_DEFAULT gst_debug_qml6_gl_src
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

#define DEFAULT_IS_LIVE TRUE

static void gst_qml6_gl_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_qml6_gl_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void gst_qml6_gl_src_finalize (GObject * object);

static gboolean gst_qml6_gl_src_setcaps (GstBaseSrc * bsrc, GstCaps * caps);
static GstCaps *gst_qml6_gl_src_get_caps (GstBaseSrc * bsrc, GstCaps * filter);
static gboolean gst_qml6_gl_src_query (GstBaseSrc * bsrc, GstQuery * query);

static gboolean gst_qml6_gl_src_decide_allocation (GstBaseSrc * bsrc,
    GstQuery * query);
static GstFlowReturn gst_qml6_gl_src_create (GstPushSrc * psrc, GstBuffer ** buffer);
static gboolean gst_qml6_gl_src_unlock(GstBaseSrc * bsrc);
static gboolean gst_qml6_gl_src_unlock_stop (GstBaseSrc * bsrc);
static GstStateChangeReturn gst_qml6_gl_src_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_qml6_gl_src_start (GstBaseSrc * basesrc);
static gboolean gst_qml6_gl_src_stop (GstBaseSrc * basesrc);

static GstStaticPadTemplate gst_qml6_gl_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "
        "format = (string) RGBA, "
        "width = " GST_VIDEO_SIZE_RANGE ", "
        "height = " GST_VIDEO_SIZE_RANGE ", "
        "framerate = " GST_VIDEO_FPS_RANGE ", "
        "texture-target = (string) 2D"));

enum
{
  ARG_0,
  PROP_WINDOW,
  PROP_DEFAULT_FBO
};

#define gst_qml6_gl_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstQml6GLSrc, gst_qml6_gl_src,
    GST_TYPE_PUSH_SRC, GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT,
        "qml6glsrc", 0, "Qt6 Qml Video Src"));
GST_ELEMENT_REGISTER_DEFINE_WITH_CODE (qml6glsrc, "qml6glsrc",
    GST_RANK_NONE, GST_TYPE_QML6_GL_SRC, qt6_element_init (plugin));

static const gfloat vertical_flip_matrix[] = {
  1.0f, 0.0f, 0.0f, 0.0f,
  0.0f, -1.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f, 1.0f,
};

static void
gst_qml6_gl_src_class_init (GstQml6GLSrcClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseSrcClass *gstbasesrc_class = (GstBaseSrcClass *) klass;
  GstPushSrcClass *gstpushsrc_class = (GstPushSrcClass *) klass;

  gobject_class->set_property = gst_qml6_gl_src_set_property;
  gobject_class->get_property = gst_qml6_gl_src_get_property;
  gobject_class->finalize = gst_qml6_gl_src_finalize;

  gst_element_class_set_metadata (gstelement_class, "Qt Video Source",
      "Source/Video", "A video src that captures a window from a QML view",
      "Multimedia Team <shmmmw@freescale.com>");

  g_object_class_install_property (gobject_class, PROP_WINDOW,
      g_param_spec_pointer ("window", "QQuickWindow",
          "The QQuickWindow to place in the object hierarchy",
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_DEFAULT_FBO,
      g_param_spec_boolean ("use-default-fbo",
          "Whether to use default FBO",
          "When set it will not create a new FBO for the QML render thread",
          FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_qml6_gl_src_template));

  gstelement_class->change_state = gst_qml6_gl_src_change_state;
  gstbasesrc_class->set_caps = gst_qml6_gl_src_setcaps;
  gstbasesrc_class->get_caps = gst_qml6_gl_src_get_caps;
  gstbasesrc_class->query = gst_qml6_gl_src_query;
  gstbasesrc_class->start = gst_qml6_gl_src_start;
  gstbasesrc_class->stop = gst_qml6_gl_src_stop;
  gstbasesrc_class->decide_allocation = gst_qml6_gl_src_decide_allocation;
  gstbasesrc_class->unlock = gst_qml6_gl_src_unlock;
  gstbasesrc_class->unlock_stop = gst_qml6_gl_src_unlock_stop;

  gstpushsrc_class->create = gst_qml6_gl_src_create;
}

static void
gst_qml6_gl_src_init (GstQml6GLSrc * src)
{
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), DEFAULT_IS_LIVE);
  src->default_fbo = FALSE;
  src->pending_image_orientation = TRUE;
}

static void
gst_qml6_gl_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (object);

  switch (prop_id) {
    case PROP_WINDOW:{
      qt_src->qwindow =
          static_cast < QQuickWindow * >(g_value_get_pointer (value));

      if (qt_src->window) {
        delete qt_src->window;
        qt_src->window = NULL;
      }

      if (qt_src->qwindow)
        qt_src->window = new Qt6GLWindow (NULL, qt_src->qwindow);

      break;
    }
    case PROP_DEFAULT_FBO:
      qt_src->default_fbo = g_value_get_boolean (value);
      if (qt_src->window)
        qt6_gl_window_use_default_fbo (qt_src->window, qt_src->default_fbo);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_qml6_gl_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (object);

  switch (prop_id) {
    case PROP_WINDOW:
      g_value_set_pointer (value, qt_src->qwindow);
      break;
    case PROP_DEFAULT_FBO:
      g_value_set_boolean (value, qt_src->default_fbo);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_qml6_gl_src_finalize (GObject * object)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (object);

  GST_DEBUG ("qmlglsrc finalize");
  if (qt_src->context)
    gst_object_unref (qt_src->context);
  qt_src->context = NULL;

  if (qt_src->qt_context)
    gst_object_unref (qt_src->qt_context);
  qt_src->qt_context = NULL;

  if (qt_src->display)
    gst_object_unref (qt_src->display);
  qt_src->display = NULL;

  if (qt_src->window)
    delete qt_src->window;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_qml6_gl_src_setcaps (GstBaseSrc * bsrc, GstCaps * caps)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (bsrc);

  GST_DEBUG ("set caps with %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&qt_src->v_info, caps))
    return FALSE;

  return TRUE;
}

static GstCaps *
gst_qml6_gl_src_get_caps (GstBaseSrc * bsrc, GstCaps * filter)
{
  GstCaps *caps = NULL, *temp = NULL;
  GstPadTemplate *pad_template;
  GstBaseSrcClass *bclass = GST_BASE_SRC_GET_CLASS (bsrc);
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (bsrc);
  guint i;
  gint width, height;

  if (qt_src->window) {
    qt_src->window->getGeometry (&width, &height);
  }

  pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_CLASS (bclass), "src");
  if (pad_template != NULL)
    caps = gst_pad_template_get_caps (pad_template);

  if (qt_src->window) {
    temp = gst_caps_copy (caps);
    guint n_caps = gst_caps_get_size (caps);

    for (i = 0; i < n_caps; i++) {
      GstStructure *s = gst_caps_get_structure (temp, i);
      gst_structure_set (s, "width", G_TYPE_INT, width, NULL);
      gst_structure_set (s, "height", G_TYPE_INT, height, NULL);
      /* because the framerate is unknown */
      gst_structure_set (s, "framerate", GST_TYPE_FRACTION, 0, 1, NULL);
      gst_structure_set (s, "pixel-aspect-ratio",
          GST_TYPE_FRACTION, 1, 1, NULL);
    }

    gst_caps_unref (caps);
    caps = temp;
  }

  if (filter) {
    GstCaps *intersection;

    intersection =
        gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    caps = intersection;
  }

  return caps;
}

static gboolean
gst_qml6_gl_src_query (GstBaseSrc * bsrc, GstQuery * query)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (bsrc);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:
    {
      if (!qt6_gl_window_is_scenegraph_initialized (qt_src->window))
        return FALSE;

      if (!qt_src->display && !qt_src->qt_context) {
        if (!qt_src->display)
          qt_src->display = qt6_gl_window_get_display (qt_src->window);
        if (!qt_src->qt_context)
          qt_src->qt_context = qt6_gl_window_get_qt_context (qt_src->window);
        if (!qt_src->context)
          qt_src->context = qt6_gl_window_get_context (qt_src->window);
      }

      if (gst_gl_handle_context_query ((GstElement *) qt_src, query,
          qt_src->display, qt_src->context, qt_src->qt_context))
        return TRUE;

      /* fallthrough */
    }
    default:
      res = GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);
      break;
  }

  return res;
}

static gboolean
_find_local_gl_context (GstQml6GLSrc * qt_src)
{
  if (gst_gl_query_local_gl_context (GST_ELEMENT (qt_src), GST_PAD_SRC,
      &qt_src->context))
    return TRUE;
  return FALSE;
}

static gboolean
gst_qml6_gl_src_decide_allocation (GstBaseSrc * bsrc, GstQuery * query)
{
  GstBufferPool *pool = NULL;
  GstStructure *config;
  GstCaps *caps;
  guint min, max, size, n, i;
  gboolean update_pool, update_allocator;
  GstAllocator *allocator;
  GstAllocationParams params;
  GstGLVideoAllocationParams *glparams;
  GstVideoInfo vinfo;
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (bsrc);

  if (gst_query_find_allocation_meta (query,
          GST_VIDEO_AFFINE_TRANSFORMATION_META_API_TYPE, NULL)) {
    qt_src->downstream_supports_affine_meta = TRUE;
  } else {
    qt_src->downstream_supports_affine_meta = FALSE;
  }

  gst_query_parse_allocation (query, &caps, NULL);
  if (!caps)
    return FALSE;

  gst_video_info_from_caps (&vinfo, caps);

  n = gst_query_get_n_allocation_pools (query);
  if (n > 0) {
    update_pool = TRUE;
    for (i = 0; i < n; i++) {
      gst_query_parse_nth_allocation_pool (query, i, &pool, &size, &min, &max);

      if (!pool || !GST_IS_GL_BUFFER_POOL (pool)) {
        if (pool)
          gst_object_unref (pool);
        pool = NULL;
      }
    }
  }

  if (!pool) {
    size = vinfo.size;
    min = max = 0;
    update_pool = FALSE;
  }

  if (!qt_src->context && !_find_local_gl_context (qt_src))
    return FALSE;

  if (!qt6_gl_window_set_context (qt_src->window, qt_src->context))
    return FALSE;

  if (!pool) {
    if (!qt_src->context || !GST_IS_GL_CONTEXT (qt_src->context))
      return FALSE;

    pool = gst_gl_buffer_pool_new (qt_src->context);
    GST_INFO_OBJECT (qt_src, "No pool, create one ourself %p", pool);
  }

  config = gst_buffer_pool_get_config (pool);

  gst_buffer_pool_config_set_params (config, caps, size, min, max);
  gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_META);
  if (gst_query_find_allocation_meta (query, GST_GL_SYNC_META_API_TYPE, NULL))
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_GL_SYNC_META);

  if (gst_query_get_n_allocation_params (query) > 0) {
    gst_query_parse_nth_allocation_param (query, 0, &allocator, &params);
    gst_buffer_pool_config_set_allocator (config, allocator, &params);
    GST_INFO_OBJECT (qt_src, "got allocator %p", allocator);
    update_allocator = TRUE;
  } else {
    allocator = NULL;
    gst_allocation_params_init (&params);
    update_allocator = FALSE;
  }

  glparams =
      gst_gl_video_allocation_params_new (qt_src->context, &params, &vinfo, 0,
      NULL, GST_GL_TEXTURE_TARGET_2D, GST_GL_RGBA);
  gst_buffer_pool_config_set_gl_allocation_params (config,
      (GstGLAllocationParams *) glparams);
  gst_gl_allocation_params_free ((GstGLAllocationParams *) glparams);

  if (!gst_buffer_pool_set_config (pool, config))
    GST_WARNING_OBJECT (qt_src, "Failed to set buffer pool config");

  if (update_allocator)
    gst_query_set_nth_allocation_param (query, 0, allocator, &params);
  else
    gst_query_add_allocation_param (query, allocator, &params);
  if (allocator)
    gst_object_unref (allocator);

  if (update_pool)
    gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
  else
    gst_query_add_allocation_pool (query, pool, size, min, max);
  gst_object_unref (pool);

  GST_INFO_OBJECT (qt_src, "successfully decide_allocation");
  return TRUE;
}

static GstFlowReturn
gst_qml6_gl_src_create (GstPushSrc * psrc, GstBuffer ** buffer)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (psrc);
  GstCaps *updated_caps = NULL;
  GstGLContext* context = qt_src->context;
  GstGLSyncMeta *sync_meta;

  *buffer = qt6_gl_window_take_buffer (qt_src->window, &updated_caps);
  GST_DEBUG_OBJECT (qt_src, "produced buffer %p", *buffer);
  if (!*buffer)
    return GST_FLOW_FLUSHING;

  if (updated_caps) {
    GST_DEBUG_OBJECT (qt_src, "new_caps %" GST_PTR_FORMAT, updated_caps);
    gst_base_src_set_caps (GST_BASE_SRC (qt_src), updated_caps);
  }
  gst_clear_caps (&updated_caps);

  sync_meta = gst_buffer_get_gl_sync_meta(*buffer);
  if (sync_meta)
      gst_gl_sync_meta_wait(sync_meta, context);

  if (!qt_src->downstream_supports_affine_meta) {
    if (qt_src->pending_image_orientation) {
      /* let downstream know the image orientation is vertical filp */
      GstTagList *image_orientation_tag =
          gst_tag_list_new (GST_TAG_IMAGE_ORIENTATION, "flip-rotate-180", NULL);

      gst_pad_push_event (GST_BASE_SRC_PAD (psrc),
          gst_event_new_tag (image_orientation_tag));

      qt_src->pending_image_orientation = FALSE;
    }
  } else {
    GstVideoAffineTransformationMeta *trans_meta;
    trans_meta = gst_buffer_add_video_affine_transformation_meta (*buffer);
    gst_video_affine_transformation_meta_apply_matrix (trans_meta,
        vertical_flip_matrix);
  }

  GST_DEBUG_OBJECT (qt_src, "buffer create done %p", *buffer);

  return GST_FLOW_OK;
}

static gboolean
gst_qml6_gl_src_unlock (GstBaseSrc * bsrc)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (bsrc);

  if (!qt_src->window)
    return TRUE;

  qt6_gl_window_unlock (qt_src->window);

  return TRUE;
}

static gboolean
gst_qml6_gl_src_unlock_stop (GstBaseSrc * bsrc)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (bsrc);

  if (!qt_src->window)
    return TRUE;

  qt6_gl_window_unlock_stop (qt_src->window);

  return TRUE;
}

static GstStateChangeReturn
gst_qml6_gl_src_change_state (GstElement * element, GstStateChange transition)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  QGuiApplication *app;

  GST_DEBUG ("changing state: %s => %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      app = static_cast < QGuiApplication * >(QCoreApplication::instance ());
      if (!app) {
        GST_ELEMENT_ERROR (element, RESOURCE, NOT_FOUND,
            ("%s", "Failed to connect to Qt"),
            ("%s", "Could not retrieve QGuiApplication instance"));
        return GST_STATE_CHANGE_FAILURE;
      }

      if (!qt_src->window) {
        GST_ELEMENT_ERROR (element, RESOURCE, NOT_FOUND,
            ("%s", "Required property \'window\' not set"), (NULL));
        return GST_STATE_CHANGE_FAILURE;
      }

      if (!qt6_gl_window_is_scenegraph_initialized (qt_src->window)) {
        GST_ELEMENT_ERROR (element, RESOURCE, NOT_FOUND,
            ("%s", "Could not initialize window system"), (NULL));
        return GST_STATE_CHANGE_FAILURE;
      }

      qt6_gl_window_use_default_fbo (qt_src->window, qt_src->default_fbo);

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
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static gboolean
gst_qml6_gl_src_start (GstBaseSrc * basesrc)
{
  GstQml6GLSrc *qt_src = GST_QML6_GL_SRC (basesrc);

  /* already has get OpenGL configuration from qt */
  if (qt_src->display && qt_src->qt_context)
    return TRUE;

  if (!qt6_gl_window_is_scenegraph_initialized (qt_src->window))
    return FALSE;

  qt_src->display = qt6_gl_window_get_display (qt_src->window);
  qt_src->qt_context = qt6_gl_window_get_qt_context (qt_src->window);
  qt_src->context = qt6_gl_window_get_context (qt_src->window);

  if (!qt_src->display || !qt_src->qt_context) {
    GST_ERROR_OBJECT (qt_src,
        "Could not retrieve window system OpenGL configuration");
    return FALSE;
  }

  GST_DEBUG_OBJECT (qt_src, "Got qt display %p and qt gl context %p",
      qt_src->display, qt_src->qt_context);
  return TRUE;
}

static gboolean
gst_qml6_gl_src_stop (GstBaseSrc * basesrc)
{
  return TRUE;
}
