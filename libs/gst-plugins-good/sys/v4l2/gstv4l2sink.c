/* GStreamer
 *
 * Copyright (C) 2009 Texas Instruments, Inc - http://www.ti.com/
 *
 * Description: V4L2 sink element
 *  Created on: Jul 2, 2009
 *      Author: Rob Clark <rob@ti.com>
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
 * SECTION:element-v4l2sink
 * @title: v4l2sink
 *
 * v4l2sink can be used to display video to v4l2 devices (screen overlays
 * provided by the graphics hardware, tv-out, etc)
 *
 * ## Example launch lines
 * |[
 * gst-launch-1.0 videotestsrc ! v4l2sink device=/dev/video1
 * ]| This pipeline displays a test pattern on /dev/video1
 * |[
 * gst-launch-1.0 -v videotestsrc ! navigationtest ! v4l2sink
 * ]| A pipeline to test navigation events.
 * While moving the mouse pointer over the test signal you will see a black box
 * following the mouse pointer. If you press the mouse button somewhere on the
 * video and release it somewhere else a green box will appear where you pressed
 * the button and a red one where you released it. (The navigationtest element
 * is part of gst-plugins-good.) You can observe here that even if the images
 * are scaled through hardware the pointer coordinates are converted back to the
 * original video frame geometry so that the box can be drawn to the correct
 * position. This also handles borders correctly, limiting coordinates to the
 * image area
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gst/video/gstvideometa.h"

#include "gstv4l2colorbalance.h"
#include "gstv4l2tuner.h"
#include "gstv4l2vidorient.h"

#include "gstv4l2sink.h"
#include "gst/gst-i18n-plugin.h"

#include <string.h>

GST_DEBUG_CATEGORY (v4l2sink_debug);
#define GST_CAT_DEFAULT v4l2sink_debug

#define DEFAULT_PROP_DEVICE   "/dev/video1"

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS,
  PROP_OVERLAY_TOP,
  PROP_OVERLAY_LEFT,
  PROP_OVERLAY_WIDTH,
  PROP_OVERLAY_HEIGHT,
  PROP_CROP_TOP,
  PROP_CROP_LEFT,
  PROP_CROP_WIDTH,
  PROP_CROP_HEIGHT,
};


GST_IMPLEMENT_V4L2_COLOR_BALANCE_METHODS (GstV4l2Sink, gst_v4l2sink);
GST_IMPLEMENT_V4L2_TUNER_METHODS (GstV4l2Sink, gst_v4l2sink);
GST_IMPLEMENT_V4L2_VIDORIENT_METHODS (GstV4l2Sink, gst_v4l2sink);

#define gst_v4l2sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstV4l2Sink, gst_v4l2sink, GST_TYPE_VIDEO_SINK,
    G_IMPLEMENT_INTERFACE (GST_TYPE_TUNER, gst_v4l2sink_tuner_interface_init);
    G_IMPLEMENT_INTERFACE (GST_TYPE_COLOR_BALANCE,
        gst_v4l2sink_color_balance_interface_init);
    G_IMPLEMENT_INTERFACE (GST_TYPE_VIDEO_ORIENTATION,
        gst_v4l2sink_video_orientation_interface_init));


static void gst_v4l2sink_finalize (GstV4l2Sink * v4l2sink);

/* GObject methods: */
static void gst_v4l2sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_v4l2sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/* GstElement methods: */
static GstStateChangeReturn gst_v4l2sink_change_state (GstElement * element,
    GstStateChange transition);

/* GstBaseSink methods: */
static gboolean gst_v4l2sink_propose_allocation (GstBaseSink * bsink,
    GstQuery * query);
static GstCaps *gst_v4l2sink_get_caps (GstBaseSink * bsink, GstCaps * filter);
static gboolean gst_v4l2sink_set_caps (GstBaseSink * bsink, GstCaps * caps);
static GstFlowReturn gst_v4l2sink_show_frame (GstVideoSink * bsink,
    GstBuffer * buf);
static gboolean gst_v4l2sink_unlock (GstBaseSink * sink);
static gboolean gst_v4l2sink_unlock_stop (GstBaseSink * sink);

static void
gst_v4l2sink_class_init (GstV4l2SinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseSinkClass *basesink_class;
  GstVideoSinkClass *videosink_class;

  gobject_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);
  basesink_class = GST_BASE_SINK_CLASS (klass);
  videosink_class = GST_VIDEO_SINK_CLASS (klass);

  gobject_class->finalize = (GObjectFinalizeFunc) gst_v4l2sink_finalize;
  gobject_class->set_property = gst_v4l2sink_set_property;
  gobject_class->get_property = gst_v4l2sink_get_property;

  element_class->change_state = gst_v4l2sink_change_state;

  gst_v4l2_object_install_properties_helper (gobject_class,
      DEFAULT_PROP_DEVICE);

  g_object_class_install_property (gobject_class, PROP_OVERLAY_TOP,
      g_param_spec_int ("overlay-top", "Overlay top",
          "The topmost (y) coordinate of the video overlay; top left corner of screen is 0,0",
          G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_LEFT,
      g_param_spec_int ("overlay-left", "Overlay left",
          "The leftmost (x) coordinate of the video overlay; top left corner of screen is 0,0",
          G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_WIDTH,
      g_param_spec_uint ("overlay-width", "Overlay width",
          "The width of the video overlay; default is equal to negotiated image width",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_HEIGHT,
      g_param_spec_uint ("overlay-height", "Overlay height",
          "The height of the video overlay; default is equal to negotiated image height",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CROP_TOP,
      g_param_spec_int ("crop-top", "Crop top",
          "The topmost (y) coordinate of the video crop; top left corner of image is 0,0",
          0x80000000, 0x7fffffff, 0, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CROP_LEFT,
      g_param_spec_int ("crop-left", "Crop left",
          "The leftmost (x) coordinate of the video crop; top left corner of image is 0,0",
          0x80000000, 0x7fffffff, 0, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CROP_WIDTH,
      g_param_spec_uint ("crop-width", "Crop width",
          "The width of the video crop; default is equal to negotiated image width",
          0, 0xffffffff, 0, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CROP_HEIGHT,
      g_param_spec_uint ("crop-height", "Crop height",
          "The height of the video crop; default is equal to negotiated image height",
          0, 0xffffffff, 0, G_PARAM_READWRITE));

  gst_element_class_set_static_metadata (element_class,
      "Video (video4linux2) Sink", "Sink/Video",
      "Displays frames on a video4linux2 device", "Rob Clark <rob@ti.com>,");

  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_v4l2_object_get_all_caps ()));

  basesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_v4l2sink_get_caps);
  basesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_v4l2sink_set_caps);
  basesink_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_v4l2sink_propose_allocation);
  basesink_class->unlock = GST_DEBUG_FUNCPTR (gst_v4l2sink_unlock);
  basesink_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_v4l2sink_unlock_stop);

  videosink_class->show_frame = GST_DEBUG_FUNCPTR (gst_v4l2sink_show_frame);

  klass->v4l2_class_devices = NULL;

  GST_DEBUG_CATEGORY_INIT (v4l2sink_debug, "v4l2sink", 0, "V4L2 sink element");

}

static void
gst_v4l2sink_init (GstV4l2Sink * v4l2sink)
{
  v4l2sink->v4l2object = gst_v4l2_object_new (GST_ELEMENT (v4l2sink),
      GST_OBJECT (GST_BASE_SINK_PAD (v4l2sink)), V4L2_BUF_TYPE_VIDEO_OUTPUT,
      DEFAULT_PROP_DEVICE, gst_v4l2_get_output, gst_v4l2_set_output, NULL);

  /* same default value for video output device as is used for
   * v4l2src/capture is no good..  so lets set a saner default
   * (which can be overridden by the one creating the v4l2sink
   * after the constructor returns)
   */
  g_object_set (v4l2sink, "device", "/dev/video1", NULL);

  v4l2sink->overlay_fields_set = 0;
  v4l2sink->crop_fields_set = 0;
}


static void
gst_v4l2sink_finalize (GstV4l2Sink * v4l2sink)
{
  gst_v4l2_object_destroy (v4l2sink->v4l2object);

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (v4l2sink));
}


/*
 * flags to indicate which overlay/crop properties the user has set (and
 * therefore which ones should override the defaults from the driver)
 */
enum
{
  RECT_TOP_SET = 0x01,
  RECT_LEFT_SET = 0x02,
  RECT_WIDTH_SET = 0x04,
  RECT_HEIGHT_SET = 0x08
};

static void
gst_v4l2sink_sync_overlay_fields (GstV4l2Sink * v4l2sink)
{
  if (!v4l2sink->overlay_fields_set)
    return;

  if (GST_V4L2_IS_OPEN (v4l2sink->v4l2object)) {

    GstV4l2Object *obj = v4l2sink->v4l2object;
    struct v4l2_format format;

    memset (&format, 0x00, sizeof (struct v4l2_format));
    if (obj->device_caps & V4L2_CAP_VIDEO_OUTPUT_OVERLAY)
      format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY;
    else
      format.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;

    if (obj->ioctl (obj->video_fd, VIDIOC_G_FMT, &format) < 0) {
      GST_WARNING_OBJECT (v4l2sink, "VIDIOC_G_FMT failed");
      return;
    }

    GST_DEBUG_OBJECT (v4l2sink,
        "setting overlay: overlay_fields_set=0x%02x, top=%d, left=%d, width=%d, height=%d",
        v4l2sink->overlay_fields_set,
        v4l2sink->overlay.top, v4l2sink->overlay.left,
        v4l2sink->overlay.width, v4l2sink->overlay.height);

    if (v4l2sink->overlay_fields_set & RECT_TOP_SET)
      format.fmt.win.w.top = v4l2sink->overlay.top;
    if (v4l2sink->overlay_fields_set & RECT_LEFT_SET)
      format.fmt.win.w.left = v4l2sink->overlay.left;
    if (v4l2sink->overlay_fields_set & RECT_WIDTH_SET)
      format.fmt.win.w.width = v4l2sink->overlay.width;
    if (v4l2sink->overlay_fields_set & RECT_HEIGHT_SET)
      format.fmt.win.w.height = v4l2sink->overlay.height;

    if (obj->ioctl (obj->video_fd, VIDIOC_S_FMT, &format) < 0) {
      GST_WARNING_OBJECT (v4l2sink, "VIDIOC_S_FMT failed");
      return;
    }

    v4l2sink->overlay_fields_set = 0;
    v4l2sink->overlay = format.fmt.win.w;
  }
}

static void
gst_v4l2sink_sync_crop_fields (GstV4l2Sink * v4l2sink)
{
  if (!v4l2sink->crop_fields_set)
    return;

  if (GST_V4L2_IS_OPEN (v4l2sink->v4l2object)) {

    GstV4l2Object *obj = v4l2sink->v4l2object;
    struct v4l2_crop crop;

    memset (&crop, 0x00, sizeof (struct v4l2_crop));
    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (obj->ioctl (obj->video_fd, VIDIOC_G_CROP, &crop) < 0) {
      GST_WARNING_OBJECT (v4l2sink, "VIDIOC_G_CROP failed");
      return;
    }

    GST_DEBUG_OBJECT (v4l2sink,
        "setting crop: crop_fields_set=0x%02x, top=%d, left=%d, width=%d, height=%d",
        v4l2sink->crop_fields_set,
        v4l2sink->crop.top, v4l2sink->crop.left,
        v4l2sink->crop.width, v4l2sink->crop.height);

    if (v4l2sink->crop_fields_set & RECT_TOP_SET)
      crop.c.top = v4l2sink->crop.top;
    if (v4l2sink->crop_fields_set & RECT_LEFT_SET)
      crop.c.left = v4l2sink->crop.left;
    if (v4l2sink->crop_fields_set & RECT_WIDTH_SET)
      crop.c.width = v4l2sink->crop.width;
    if (v4l2sink->crop_fields_set & RECT_HEIGHT_SET)
      crop.c.height = v4l2sink->crop.height;

    if (obj->ioctl (obj->video_fd, VIDIOC_S_CROP, &crop) < 0) {
      GST_WARNING_OBJECT (v4l2sink, "VIDIOC_S_CROP failed");
      return;
    }

    if (obj->ioctl (obj->video_fd, VIDIOC_G_CROP, &crop) < 0) {
      GST_WARNING_OBJECT (v4l2sink, "VIDIOC_G_CROP failed");
      return;
    }

    v4l2sink->crop_fields_set = 0;
    v4l2sink->crop = crop.c;
  }
}


static void
gst_v4l2sink_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (object);

  if (!gst_v4l2_object_set_property_helper (v4l2sink->v4l2object,
          prop_id, value, pspec)) {
    switch (prop_id) {
      case PROP_OVERLAY_TOP:
        v4l2sink->overlay.top = g_value_get_int (value);
        v4l2sink->overlay_fields_set |= RECT_TOP_SET;
        gst_v4l2sink_sync_overlay_fields (v4l2sink);
        break;
      case PROP_OVERLAY_LEFT:
        v4l2sink->overlay.left = g_value_get_int (value);
        v4l2sink->overlay_fields_set |= RECT_LEFT_SET;
        gst_v4l2sink_sync_overlay_fields (v4l2sink);
        break;
      case PROP_OVERLAY_WIDTH:
        v4l2sink->overlay.width = g_value_get_uint (value);
        v4l2sink->overlay_fields_set |= RECT_WIDTH_SET;
        gst_v4l2sink_sync_overlay_fields (v4l2sink);
        break;
      case PROP_OVERLAY_HEIGHT:
        v4l2sink->overlay.height = g_value_get_uint (value);
        v4l2sink->overlay_fields_set |= RECT_HEIGHT_SET;
        gst_v4l2sink_sync_overlay_fields (v4l2sink);
        break;
      case PROP_CROP_TOP:
        v4l2sink->crop.top = g_value_get_int (value);
        v4l2sink->crop_fields_set |= RECT_TOP_SET;
        gst_v4l2sink_sync_crop_fields (v4l2sink);
        break;
      case PROP_CROP_LEFT:
        v4l2sink->crop.left = g_value_get_int (value);
        v4l2sink->crop_fields_set |= RECT_LEFT_SET;
        gst_v4l2sink_sync_crop_fields (v4l2sink);
        break;
      case PROP_CROP_WIDTH:
        v4l2sink->crop.width = g_value_get_uint (value);
        v4l2sink->crop_fields_set |= RECT_WIDTH_SET;
        gst_v4l2sink_sync_crop_fields (v4l2sink);
        break;
      case PROP_CROP_HEIGHT:
        v4l2sink->crop.height = g_value_get_uint (value);
        v4l2sink->crop_fields_set |= RECT_HEIGHT_SET;
        gst_v4l2sink_sync_crop_fields (v4l2sink);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
  }
}


static void
gst_v4l2sink_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (object);

  if (!gst_v4l2_object_get_property_helper (v4l2sink->v4l2object,
          prop_id, value, pspec)) {
    switch (prop_id) {
      case PROP_OVERLAY_TOP:
        g_value_set_int (value, v4l2sink->overlay.top);
        break;
      case PROP_OVERLAY_LEFT:
        g_value_set_int (value, v4l2sink->overlay.left);
        break;
      case PROP_OVERLAY_WIDTH:
        g_value_set_uint (value, v4l2sink->overlay.width);
        break;
      case PROP_OVERLAY_HEIGHT:
        g_value_set_uint (value, v4l2sink->overlay.height);
        break;
      case PROP_CROP_TOP:
        g_value_set_int (value, v4l2sink->crop.top);
        break;
      case PROP_CROP_LEFT:
        g_value_set_int (value, v4l2sink->crop.left);
        break;
      case PROP_CROP_WIDTH:
        g_value_set_uint (value, v4l2sink->crop.width);
        break;
      case PROP_CROP_HEIGHT:
        g_value_set_uint (value, v4l2sink->crop.height);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
  }
}

static GstStateChangeReturn
gst_v4l2sink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (element);

  GST_DEBUG_OBJECT (v4l2sink, "%d -> %d",
      GST_STATE_TRANSITION_CURRENT (transition),
      GST_STATE_TRANSITION_NEXT (transition));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      /* open the device */
      if (!gst_v4l2_object_open (v4l2sink->v4l2object))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if (!gst_v4l2_object_stop (v4l2sink->v4l2object))
        return GST_STATE_CHANGE_FAILURE;
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      /* we need to call stop here too */
      if (!gst_v4l2_object_stop (v4l2sink->v4l2object))
        return GST_STATE_CHANGE_FAILURE;
      /* close the device */
      if (!gst_v4l2_object_close (v4l2sink->v4l2object))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  return ret;
}


static GstCaps *
gst_v4l2sink_get_caps (GstBaseSink * bsink, GstCaps * filter)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (bsink);

  if (!GST_V4L2_IS_OPEN (v4l2sink->v4l2object)) {
    /* FIXME: copy? */
    GST_DEBUG_OBJECT (v4l2sink, "device is not open");
    return gst_pad_get_pad_template_caps (GST_BASE_SINK_PAD (v4l2sink));
  }


  return gst_v4l2_object_get_caps (v4l2sink->v4l2object, filter);
}

static gboolean
gst_v4l2sink_set_caps (GstBaseSink * bsink, GstCaps * caps)
{
  GstV4l2Error error = GST_V4L2_ERROR_INIT;
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (bsink);
  GstV4l2Object *obj = v4l2sink->v4l2object;

  GST_DEBUG_OBJECT (v4l2sink, "caps: %" GST_PTR_FORMAT, caps);

  if (!GST_V4L2_IS_OPEN (obj)) {
    GST_DEBUG_OBJECT (v4l2sink, "device is not open");
    return FALSE;
  }

  /* make sure the caps changed before doing anything */
  if (gst_v4l2_object_caps_equal (obj, caps))
    return TRUE;

  if (!gst_v4l2_object_stop (obj))
    goto stop_failed;

  if (!gst_v4l2_object_set_format (obj, caps, &error))
    goto invalid_format;

  gst_v4l2sink_sync_overlay_fields (v4l2sink);
  gst_v4l2sink_sync_crop_fields (v4l2sink);

  GST_INFO_OBJECT (v4l2sink, "outputting buffers via mode %u", obj->mode);

  v4l2sink->video_width = GST_V4L2_WIDTH (obj);
  v4l2sink->video_height = GST_V4L2_HEIGHT (obj);

  /* TODO: videosink width/height should be scaled according to
   * pixel-aspect-ratio
   */
  GST_VIDEO_SINK_WIDTH (v4l2sink) = v4l2sink->video_width;
  GST_VIDEO_SINK_HEIGHT (v4l2sink) = v4l2sink->video_height;

  return TRUE;

  /* ERRORS */
stop_failed:
  {
    GST_DEBUG_OBJECT (v4l2sink, "failed to stop streaming");
    return FALSE;
  }
invalid_format:
  {
    /* error already posted */
    gst_v4l2_error (v4l2sink, &error);
    GST_DEBUG_OBJECT (v4l2sink, "can't set format");
    return FALSE;
  }
}

static gboolean
gst_v4l2sink_propose_allocation (GstBaseSink * bsink, GstQuery * query)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (bsink);
  gboolean last_sample_enabled;

  if (!gst_v4l2_object_propose_allocation (v4l2sink->v4l2object, query))
    return FALSE;

  g_object_get (bsink, "enable-last-sample", &last_sample_enabled, NULL);

  if (last_sample_enabled && gst_query_get_n_allocation_pools (query) > 0) {
    GstBufferPool *pool;
    guint size, min, max;

    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);

    /* we need 1 more, otherwise we'll run out of buffers at preroll */
    min++;
    if (max < min)
      max = min;

    gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
    if (pool)
      gst_object_unref (pool);
  }

  return TRUE;
}

/* called after A/V sync to render frame */
static GstFlowReturn
gst_v4l2sink_show_frame (GstVideoSink * vsink, GstBuffer * buf)
{
  GstFlowReturn ret;
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (vsink);
  GstV4l2Object *obj = v4l2sink->v4l2object;
  GstBufferPool *bpool = GST_BUFFER_POOL (obj->pool);

  GST_DEBUG_OBJECT (v4l2sink, "render buffer: %p", buf);

  if (G_UNLIKELY (obj->pool == NULL))
    goto not_negotiated;

  if (G_UNLIKELY (!gst_buffer_pool_is_active (bpool))) {
    GstStructure *config;

    /* this pool was not activated, configure and activate */
    GST_DEBUG_OBJECT (v4l2sink, "activating pool");

    config = gst_buffer_pool_get_config (bpool);
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
    gst_buffer_pool_set_config (bpool, config);

    if (!gst_buffer_pool_set_active (bpool, TRUE))
      goto activate_failed;
  }

  gst_buffer_ref (buf);
again:
  ret = gst_v4l2_buffer_pool_process (GST_V4L2_BUFFER_POOL_CAST (obj->pool),
      &buf);
  if (ret == GST_FLOW_FLUSHING) {
    ret = gst_base_sink_wait_preroll (GST_BASE_SINK (vsink));
    if (ret == GST_FLOW_OK)
      goto again;
  }
  gst_buffer_unref (buf);

  return ret;

  /* ERRORS */
not_negotiated:
  {
    GST_ERROR_OBJECT (v4l2sink, "not negotiated");
    return GST_FLOW_NOT_NEGOTIATED;
  }
activate_failed:
  {
    GST_ELEMENT_ERROR (v4l2sink, RESOURCE, SETTINGS,
        (_("Failed to allocated required memory.")),
        ("Buffer pool activation failed"));
    return GST_FLOW_ERROR;
  }
}

static gboolean
gst_v4l2sink_unlock (GstBaseSink * sink)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (sink);
  return gst_v4l2_object_unlock (v4l2sink->v4l2object);
}

static gboolean
gst_v4l2sink_unlock_stop (GstBaseSink * sink)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (sink);
  return gst_v4l2_object_unlock_stop (v4l2sink->v4l2object);
}
