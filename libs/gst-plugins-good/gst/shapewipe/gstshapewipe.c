/* GStreamer
 * Copyright (C) 2009,2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
 * SECTION:element-shapewipe
 * @title: shapewipe
 *
 * The shapewipe element provides custom transitions on video streams
 * based on a grayscale bitmap. The state of the transition can be
 * controlled by the position property and an optional blended border
 * can be added by the border property.
 *
 * Transition bitmaps can be downloaded from the
 * [Cinelerra transition](http://cinelerra.org/transitions.php) page.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! video/x-raw,format=AYUV,width=640,height=480 ! shapewipe position=0.5 name=shape ! videomixer name=mixer ! videoconvert ! autovideosink     filesrc location=mask.png ! typefind ! decodebin ! videoconvert ! videoscale ! queue ! shape.mask_sink    videotestsrc pattern=snow ! video/x-raw,format=AYUV,width=640,height=480 ! queue ! mixer.
 * ]| This pipeline adds the transition from mask.png with position 0.5 to an SMPTE test screen and snow.
 *
 */


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include <gst/gst.h>

#include "gstshapewipe.h"

static void gst_shape_wipe_finalize (GObject * object);
static void gst_shape_wipe_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_shape_wipe_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void gst_shape_wipe_reset (GstShapeWipe * self);
static void gst_shape_wipe_update_qos (GstShapeWipe * self, gdouble proportion,
    GstClockTimeDiff diff, GstClockTime time);
static void gst_shape_wipe_reset_qos (GstShapeWipe * self);
static void gst_shape_wipe_read_qos (GstShapeWipe * self, gdouble * proportion,
    GstClockTime * time);

static GstStateChangeReturn gst_shape_wipe_change_state (GstElement * element,
    GstStateChange transition);

static GstFlowReturn gst_shape_wipe_video_sink_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);
static gboolean gst_shape_wipe_video_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_shape_wipe_video_sink_setcaps (GstShapeWipe * self,
    GstCaps * caps);
static GstCaps *gst_shape_wipe_video_sink_getcaps (GstShapeWipe * self,
    GstPad * pad, GstCaps * filter);
static gboolean gst_shape_wipe_video_sink_query (GstPad * pad,
    GstObject * parent, GstQuery * query);
static GstFlowReturn gst_shape_wipe_mask_sink_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);
static gboolean gst_shape_wipe_mask_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_shape_wipe_mask_sink_setcaps (GstShapeWipe * self,
    GstCaps * caps);
static GstCaps *gst_shape_wipe_mask_sink_getcaps (GstShapeWipe * self,
    GstPad * pad, GstCaps * filter);
static gboolean gst_shape_wipe_mask_sink_query (GstPad * pad,
    GstObject * parent, GstQuery * query);
static gboolean gst_shape_wipe_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstCaps *gst_shape_wipe_src_getcaps (GstPad * pad, GstCaps * filter);
static gboolean gst_shape_wipe_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query);

enum
{
  PROP_0,
  PROP_POSITION,
  PROP_BORDER
};

#define DEFAULT_POSITION 0.0
#define DEFAULT_BORDER 0.0

static GstStaticPadTemplate video_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("video_sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ AYUV, ARGB, BGRA, ABGR, RGBA }")));

static GstStaticPadTemplate mask_sink_pad_template =
    GST_STATIC_PAD_TEMPLATE ("mask_sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) GRAY8, "
        "width = " GST_VIDEO_SIZE_RANGE ", "
        "height = " GST_VIDEO_SIZE_RANGE ", " "framerate = 0/1 ; "
        "video/x-raw, " "format = (string) " GST_VIDEO_NE (GRAY16) ", "
        "width = " GST_VIDEO_SIZE_RANGE ", "
        "height = " GST_VIDEO_SIZE_RANGE ", " "framerate = 0/1"));

static GstStaticPadTemplate src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ AYUV, ARGB, BGRA, ABGR, RGBA }")));

GST_DEBUG_CATEGORY_STATIC (gst_shape_wipe_debug);
#define GST_CAT_DEFAULT gst_shape_wipe_debug

#define gst_shape_wipe_parent_class parent_class
G_DEFINE_TYPE (GstShapeWipe, gst_shape_wipe, GST_TYPE_ELEMENT);

static void
gst_shape_wipe_class_init (GstShapeWipeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->finalize = gst_shape_wipe_finalize;
  gobject_class->set_property = gst_shape_wipe_set_property;
  gobject_class->get_property = gst_shape_wipe_get_property;

  g_object_class_install_property (gobject_class, PROP_POSITION,
      g_param_spec_float ("position", "Position", "Position of the mask",
          0.0, 1.0, DEFAULT_POSITION,
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_BORDER,
      g_param_spec_float ("border", "Border", "Border of the mask",
          0.0, 1.0, DEFAULT_BORDER,
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_shape_wipe_change_state);

  gst_element_class_set_static_metadata (gstelement_class,
      "Shape Wipe transition filter",
      "Filter/Editor/Video",
      "Adds a shape wipe transition to a video stream",
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &video_sink_pad_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &mask_sink_pad_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &src_pad_template);
}

static void
gst_shape_wipe_init (GstShapeWipe * self)
{
  self->video_sinkpad =
      gst_pad_new_from_static_template (&video_sink_pad_template, "video_sink");
  gst_pad_set_chain_function (self->video_sinkpad,
      GST_DEBUG_FUNCPTR (gst_shape_wipe_video_sink_chain));
  gst_pad_set_event_function (self->video_sinkpad,
      GST_DEBUG_FUNCPTR (gst_shape_wipe_video_sink_event));
  gst_pad_set_query_function (self->video_sinkpad,
      GST_DEBUG_FUNCPTR (gst_shape_wipe_video_sink_query));
  gst_element_add_pad (GST_ELEMENT (self), self->video_sinkpad);

  self->mask_sinkpad =
      gst_pad_new_from_static_template (&mask_sink_pad_template, "mask_sink");
  gst_pad_set_chain_function (self->mask_sinkpad,
      GST_DEBUG_FUNCPTR (gst_shape_wipe_mask_sink_chain));
  gst_pad_set_event_function (self->mask_sinkpad,
      GST_DEBUG_FUNCPTR (gst_shape_wipe_mask_sink_event));
  gst_pad_set_query_function (self->mask_sinkpad,
      GST_DEBUG_FUNCPTR (gst_shape_wipe_mask_sink_query));
  gst_element_add_pad (GST_ELEMENT (self), self->mask_sinkpad);

  self->srcpad = gst_pad_new_from_static_template (&src_pad_template, "src");
  gst_pad_set_event_function (self->srcpad,
      GST_DEBUG_FUNCPTR (gst_shape_wipe_src_event));
  gst_pad_set_query_function (self->srcpad,
      GST_DEBUG_FUNCPTR (gst_shape_wipe_src_query));
  gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

  g_mutex_init (&self->mask_mutex);
  g_cond_init (&self->mask_cond);

  gst_shape_wipe_reset (self);
}

static void
gst_shape_wipe_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (object);

  switch (prop_id) {
    case PROP_POSITION:
      g_value_set_float (value, self->mask_position);
      break;
    case PROP_BORDER:
      g_value_set_float (value, self->mask_border);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_shape_wipe_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (object);

  switch (prop_id) {
    case PROP_POSITION:{
      gfloat f = g_value_get_float (value);

      GST_LOG_OBJECT (self, "Setting mask position: %f", f);
      self->mask_position = f;
      break;
    }
    case PROP_BORDER:{
      gfloat f = g_value_get_float (value);

      GST_LOG_OBJECT (self, "Setting mask border: %f", f);
      self->mask_border = f;
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_shape_wipe_finalize (GObject * object)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (object);

  gst_shape_wipe_reset (self);

  g_cond_clear (&self->mask_cond);
  g_mutex_clear (&self->mask_mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_shape_wipe_reset (GstShapeWipe * self)
{
  GST_DEBUG_OBJECT (self, "Resetting internal state");

  if (self->mask)
    gst_buffer_unref (self->mask);
  self->mask = NULL;

  g_mutex_lock (&self->mask_mutex);
  g_cond_signal (&self->mask_cond);
  g_mutex_unlock (&self->mask_mutex);

  gst_video_info_init (&self->vinfo);
  gst_video_info_init (&self->minfo);
  self->mask_bpp = 0;

  gst_segment_init (&self->segment, GST_FORMAT_TIME);

  gst_shape_wipe_reset_qos (self);
  self->frame_duration = 0;
}

static gboolean
gst_shape_wipe_video_sink_setcaps (GstShapeWipe * self, GstCaps * caps)
{
  gboolean ret = TRUE;
  GstVideoInfo info;

  GST_DEBUG_OBJECT (self, "Setting caps: %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&info, caps))
    goto invalid_caps;

  if ((self->vinfo.width != info.width || self->vinfo.height != info.height) &&
      self->vinfo.width > 0 && self->vinfo.height > 0) {
    g_mutex_lock (&self->mask_mutex);
    if (self->mask)
      gst_buffer_unref (self->mask);
    self->mask = NULL;
    g_mutex_unlock (&self->mask_mutex);
  }


  if (info.fps_n != 0)
    self->frame_duration =
        gst_util_uint64_scale (GST_SECOND, info.fps_d, info.fps_n);
  else
    self->frame_duration = 0;

  self->vinfo = info;

  ret = gst_pad_set_caps (self->srcpad, caps);

  return ret;

  /* ERRORS */
invalid_caps:
  {
    GST_ERROR_OBJECT (self, "Invalid caps");
    return FALSE;
  }
}

static GstCaps *
gst_shape_wipe_video_sink_getcaps (GstShapeWipe * self, GstPad * pad,
    GstCaps * filter)
{
  GstCaps *templ, *ret, *tmp;

  ret = gst_pad_get_current_caps (pad);
  if (ret != NULL)
    return ret;

  templ = gst_pad_get_pad_template_caps (pad);
  tmp = gst_pad_peer_query_caps (self->srcpad, NULL);
  if (tmp) {
    ret = gst_caps_intersect (tmp, templ);
    gst_caps_unref (templ);
    gst_caps_unref (tmp);
  } else {
    ret = templ;
  }

  GST_LOG_OBJECT (pad, "srcpad accepted caps: %" GST_PTR_FORMAT, ret);

  if (gst_caps_is_empty (ret))
    goto done;

  tmp = gst_pad_peer_query_caps (pad, NULL);

  GST_LOG_OBJECT (pad, "peerpad accepted caps: %" GST_PTR_FORMAT, tmp);
  if (tmp) {
    GstCaps *intersection;

    intersection = gst_caps_intersect (tmp, ret);
    gst_caps_unref (tmp);
    gst_caps_unref (ret);
    ret = intersection;
  }

  GST_LOG_OBJECT (pad, "intersection: %" GST_PTR_FORMAT, tmp);

  if (gst_caps_is_empty (ret))
    goto done;

  if (self->vinfo.height && self->vinfo.width) {
    guint i, n;

    ret = gst_caps_make_writable (ret);
    n = gst_caps_get_size (ret);
    for (i = 0; i < n; i++) {
      GstStructure *s = gst_caps_get_structure (ret, i);

      gst_structure_set (s, "width", G_TYPE_INT, self->vinfo.width, "height",
          G_TYPE_INT, self->vinfo.height, NULL);
    }
  }

  tmp = gst_pad_peer_query_caps (self->mask_sinkpad, NULL);

  GST_LOG_OBJECT (pad, "mask accepted caps: %" GST_PTR_FORMAT, tmp);
  if (tmp) {
    GstCaps *intersection, *tmp2;
    guint i, n;

    tmp2 = gst_pad_get_pad_template_caps (self->mask_sinkpad);
    intersection = gst_caps_intersect (tmp, tmp2);
    gst_caps_unref (tmp);
    gst_caps_unref (tmp2);
    tmp = intersection;

    tmp = gst_caps_make_writable (tmp);
    n = gst_caps_get_size (tmp);

    for (i = 0; i < n; i++) {
      GstStructure *s = gst_caps_get_structure (tmp, i);

      gst_structure_remove_fields (s, "format", "framerate", NULL);
      gst_structure_set_name (s, "video/x-raw");
    }

    intersection = gst_caps_intersect (tmp, ret);
    gst_caps_unref (tmp);
    gst_caps_unref (ret);
    ret = intersection;
  }
done:
  GST_LOG_OBJECT (pad, "Returning caps: %" GST_PTR_FORMAT, ret);

  return ret;
}

static gboolean
gst_shape_wipe_mask_sink_setcaps (GstShapeWipe * self, GstCaps * caps)
{
  gboolean ret = TRUE;
  gint width, height, bpp;
  GstVideoInfo info;

  GST_DEBUG_OBJECT (self, "Setting caps: %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&info, caps)) {
    ret = FALSE;
    goto done;
  }

  width = GST_VIDEO_INFO_WIDTH (&info);
  height = GST_VIDEO_INFO_HEIGHT (&info);
  bpp = GST_VIDEO_INFO_COMP_DEPTH (&info, 0);

  if ((self->vinfo.width != width || self->vinfo.height != height) &&
      self->vinfo.width > 0 && self->vinfo.height > 0) {
    GST_ERROR_OBJECT (self, "Mask caps must have the same width/height "
        "as the video caps");
    ret = FALSE;
    goto done;
  }

  self->mask_bpp = bpp;
  self->minfo = info;

done:
  return ret;
}

static GstCaps *
gst_shape_wipe_mask_sink_getcaps (GstShapeWipe * self, GstPad * pad,
    GstCaps * filter)
{
  GstCaps *ret, *tmp, *tcaps;
  guint i, n;

  ret = gst_pad_get_current_caps (pad);
  if (ret != NULL)
    return ret;

  tcaps = gst_pad_get_pad_template_caps (self->video_sinkpad);
  tmp = gst_pad_peer_query_caps (self->video_sinkpad, NULL);
  if (tmp) {
    ret = gst_caps_intersect (tmp, tcaps);
    gst_caps_unref (tcaps);
    gst_caps_unref (tmp);
  } else {
    ret = tcaps;
  }

  GST_LOG_OBJECT (pad, "video sink accepted caps: %" GST_PTR_FORMAT, ret);

  if (gst_caps_is_empty (ret))
    goto done;

  tmp = gst_pad_peer_query_caps (self->srcpad, NULL);
  GST_LOG_OBJECT (pad, "srcpad accepted caps: %" GST_PTR_FORMAT, ret);

  if (tmp) {
    GstCaps *intersection;

    intersection = gst_caps_intersect (ret, tmp);
    gst_caps_unref (ret);
    gst_caps_unref (tmp);
    ret = intersection;
  }

  GST_LOG_OBJECT (pad, "intersection: %" GST_PTR_FORMAT, ret);

  if (gst_caps_is_empty (ret))
    goto done;

  ret = gst_caps_make_writable (ret);
  n = gst_caps_get_size (ret);
  tmp = gst_caps_new_empty ();
  for (i = 0; i < n; i++) {
    GstStructure *s = gst_caps_get_structure (ret, i);
    GstStructure *t;

    gst_structure_set_name (s, "video/x-raw");
    gst_structure_remove_fields (s, "format", "framerate", NULL);

    if (self->vinfo.width && self->vinfo.height)
      gst_structure_set (s, "width", G_TYPE_INT, self->vinfo.width, "height",
          G_TYPE_INT, self->vinfo.height, NULL);

    gst_structure_set (s, "framerate", GST_TYPE_FRACTION, 0, 1, NULL);

    t = gst_structure_copy (s);

    gst_structure_set (s, "format", G_TYPE_STRING, GST_VIDEO_NE (GRAY16), NULL);
    gst_structure_set (t, "format", G_TYPE_STRING, "GRAY8", NULL);

    gst_caps_append_structure (tmp, t);
  }
  gst_caps_append (ret, tmp);

  tmp = gst_pad_peer_query_caps (pad, NULL);
  GST_LOG_OBJECT (pad, "peer accepted caps: %" GST_PTR_FORMAT, tmp);

  if (tmp) {
    GstCaps *intersection;

    intersection = gst_caps_intersect (tmp, ret);
    gst_caps_unref (tmp);
    gst_caps_unref (ret);
    ret = intersection;
  }

done:
  GST_LOG_OBJECT (pad, "Returning caps: %" GST_PTR_FORMAT, ret);

  return ret;
}

static GstCaps *
gst_shape_wipe_src_getcaps (GstPad * pad, GstCaps * filter)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (gst_pad_get_parent (pad));
  GstCaps *templ, *ret, *tmp;

  ret = gst_pad_get_current_caps (pad);
  if (ret != NULL)
    return ret;

  ret = gst_pad_get_current_caps (self->video_sinkpad);
  if (ret != NULL)
    return ret;

  templ = gst_pad_get_pad_template_caps (self->video_sinkpad);
  tmp = gst_pad_peer_query_caps (self->video_sinkpad, NULL);
  if (tmp) {
    ret = gst_caps_intersect (tmp, templ);
    gst_caps_unref (templ);
    gst_caps_unref (tmp);
  } else {
    ret = templ;
  }

  GST_LOG_OBJECT (pad, "video sink accepted caps: %" GST_PTR_FORMAT, ret);

  if (gst_caps_is_empty (ret))
    goto done;

  tmp = gst_pad_peer_query_caps (pad, NULL);
  GST_LOG_OBJECT (pad, "peer accepted caps: %" GST_PTR_FORMAT, ret);
  if (tmp) {
    GstCaps *intersection;

    intersection = gst_caps_intersect (tmp, ret);
    gst_caps_unref (tmp);
    gst_caps_unref (ret);
    ret = intersection;
  }

  GST_LOG_OBJECT (pad, "intersection: %" GST_PTR_FORMAT, ret);

  if (gst_caps_is_empty (ret))
    goto done;

  if (self->vinfo.height && self->vinfo.width) {
    guint i, n;

    ret = gst_caps_make_writable (ret);
    n = gst_caps_get_size (ret);
    for (i = 0; i < n; i++) {
      GstStructure *s = gst_caps_get_structure (ret, i);

      gst_structure_set (s, "width", G_TYPE_INT, self->vinfo.width, "height",
          G_TYPE_INT, self->vinfo.height, NULL);
    }
  }

  tmp = gst_pad_peer_query_caps (self->mask_sinkpad, NULL);
  GST_LOG_OBJECT (pad, "mask sink accepted caps: %" GST_PTR_FORMAT, ret);
  if (tmp) {
    GstCaps *intersection, *tmp2;
    guint i, n;

    tmp2 = gst_pad_get_pad_template_caps (self->mask_sinkpad);
    intersection = gst_caps_intersect (tmp, tmp2);
    gst_caps_unref (tmp);
    gst_caps_unref (tmp2);

    tmp = gst_caps_make_writable (intersection);
    n = gst_caps_get_size (tmp);

    for (i = 0; i < n; i++) {
      GstStructure *s = gst_caps_get_structure (tmp, i);

      gst_structure_remove_fields (s, "format", "framerate", NULL);
      gst_structure_set_name (s, "video/x-raw");
    }

    intersection = gst_caps_intersect (tmp, ret);
    gst_caps_unref (tmp);
    gst_caps_unref (ret);
    ret = intersection;
  }

done:

  gst_object_unref (self);

  GST_LOG_OBJECT (pad, "Returning caps: %" GST_PTR_FORMAT, ret);

  return ret;
}

static gboolean
gst_shape_wipe_video_sink_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstShapeWipe *self = (GstShapeWipe *) parent;
  gboolean ret;

  GST_LOG_OBJECT (pad, "Handling query of type '%s'",
      gst_query_type_get_name (GST_QUERY_TYPE (query)));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);
      caps = gst_shape_wipe_video_sink_getcaps (self, pad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      ret = TRUE;
      break;
    }
    default:
      ret = gst_pad_query_default (pad, parent, query);
      break;
  }

  return ret;
}

static gboolean
gst_shape_wipe_src_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (parent);
  gboolean ret;

  GST_LOG_OBJECT (pad, "Handling query of type '%s'",
      gst_query_type_get_name (GST_QUERY_TYPE (query)));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);
      caps = gst_shape_wipe_src_getcaps (pad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      ret = TRUE;
      break;
    }
    default:
      ret = gst_pad_peer_query (self->video_sinkpad, query);
      break;
  }

  return ret;
}

static void
gst_shape_wipe_update_qos (GstShapeWipe * self, gdouble proportion,
    GstClockTimeDiff diff, GstClockTime timestamp)
{
  GST_OBJECT_LOCK (self);
  self->proportion = proportion;
  if (G_LIKELY (timestamp != GST_CLOCK_TIME_NONE)) {
    if (G_UNLIKELY (diff > 0))
      self->earliest_time = timestamp + 2 * diff + self->frame_duration;
    else
      self->earliest_time = timestamp + diff;
  } else {
    self->earliest_time = GST_CLOCK_TIME_NONE;
  }
  GST_OBJECT_UNLOCK (self);
}

static void
gst_shape_wipe_reset_qos (GstShapeWipe * self)
{
  gst_shape_wipe_update_qos (self, 0.5, 0, GST_CLOCK_TIME_NONE);
}

static void
gst_shape_wipe_read_qos (GstShapeWipe * self, gdouble * proportion,
    GstClockTime * time)
{
  GST_OBJECT_LOCK (self);
  *proportion = self->proportion;
  *time = self->earliest_time;
  GST_OBJECT_UNLOCK (self);
}

/* Perform qos calculations before processing the next frame. Returns TRUE if
 * the frame should be processed, FALSE if the frame can be dropped entirely */
static gboolean
gst_shape_wipe_do_qos (GstShapeWipe * self, GstClockTime timestamp)
{
  GstClockTime qostime, earliest_time;
  gdouble proportion;

  /* no timestamp, can't do QoS => process frame */
  if (G_UNLIKELY (!GST_CLOCK_TIME_IS_VALID (timestamp))) {
    GST_LOG_OBJECT (self, "invalid timestamp, can't do QoS, process frame");
    return TRUE;
  }

  /* get latest QoS observation values */
  gst_shape_wipe_read_qos (self, &proportion, &earliest_time);

  /* skip qos if we have no observation (yet) => process frame */
  if (G_UNLIKELY (!GST_CLOCK_TIME_IS_VALID (earliest_time))) {
    GST_LOG_OBJECT (self, "no observation yet, process frame");
    return TRUE;
  }

  /* qos is done on running time */
  qostime = gst_segment_to_running_time (&self->segment, GST_FORMAT_TIME,
      timestamp);

  /* see how our next timestamp relates to the latest qos timestamp */
  GST_LOG_OBJECT (self, "qostime %" GST_TIME_FORMAT ", earliest %"
      GST_TIME_FORMAT, GST_TIME_ARGS (qostime), GST_TIME_ARGS (earliest_time));

  if (qostime != GST_CLOCK_TIME_NONE && qostime <= earliest_time) {
    GST_DEBUG_OBJECT (self, "we are late, drop frame");
    return FALSE;
  }

  GST_LOG_OBJECT (self, "process frame");
  return TRUE;
}

#define CREATE_ARGB_FUNCTIONS(depth, name, shift, a, r, g, b) \
static void \
gst_shape_wipe_blend_##name##_##depth (GstShapeWipe * self, GstVideoFrame * inframe, \
    GstVideoFrame * maskframe, GstVideoFrame * outframe) \
{ \
  const guint##depth *mask = (const guint##depth *) GST_VIDEO_FRAME_PLANE_DATA (maskframe, 0); \
  const guint8 *input = (const guint8 *) GST_VIDEO_FRAME_PLANE_DATA (inframe, 0); \
  guint8 *output = (guint8 *) GST_VIDEO_FRAME_PLANE_DATA (outframe, 0); \
  guint i, j; \
  gint width = GST_VIDEO_FRAME_WIDTH (inframe); \
  gint height = GST_VIDEO_FRAME_HEIGHT (inframe); \
  guint mask_increment = ((depth == 16) ? GST_ROUND_UP_2 (width) : \
                           GST_ROUND_UP_4 (width)) - width; \
  gfloat position = self->mask_position; \
  gfloat low = position - (self->mask_border / 2.0f); \
  gfloat high = position + (self->mask_border / 2.0f); \
  guint32 low_i, high_i, round_i; \
  \
  if (low < 0.0f) { \
    high = 0.0f; \
    low = 0.0f; \
  } \
  \
  if (high > 1.0f) { \
    low = 1.0f; \
    high = 1.0f; \
  } \
  \
  low_i = low * 65536; \
  high_i = high * 65536; \
  round_i = (high_i - low_i) >> 1; \
  \
  for (i = 0; i < height; i++) { \
    for (j = 0; j < width; j++) { \
      guint32 in = *mask << shift; \
      \
      if (in < low_i) { \
        output[a] = 0x00;       /* A */ \
        output[r] = input[r];   /* R */ \
        output[g] = input[g];   /* G */ \
        output[b] = input[b];   /* B */ \
      } else if (in >= high_i) { \
        output[a] = input[a];   /* A */ \
        output[r] = input[r];   /* R */ \
        output[g] = input[g];   /* G */ \
        output[b] = input[b];   /* B */ \
      } else { \
        guint32 val; \
        /* Note: This will never overflow or be larger than 255! */ \
        val = (((in - low_i) << 16) + round_i) / (high_i - low_i); \
        val = (val * input[a] + 32768) >> 16; \
        \
        output[a] = val;        /* A */ \
        output[r] = input[r];   /* R */ \
        output[g] = input[g];   /* G */ \
        output[b] = input[b];   /* B */ \
      } \
      \
      mask++; \
      input += 4; \
      output += 4; \
    } \
    mask += mask_increment; \
  } \
}

CREATE_ARGB_FUNCTIONS (16, argb, 0, 0, 1, 2, 3);
CREATE_ARGB_FUNCTIONS (8, argb, 8, 0, 1, 2, 3);

CREATE_ARGB_FUNCTIONS (16, bgra, 0, 3, 2, 1, 0);
CREATE_ARGB_FUNCTIONS (8, bgra, 8, 3, 2, 1, 0);

static GstFlowReturn
gst_shape_wipe_video_sink_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (parent);
  GstFlowReturn ret = GST_FLOW_OK;
  GstBuffer *mask = NULL, *outbuf = NULL;
  GstClockTime timestamp;
  GstVideoFrame inframe, outframe, maskframe;

  if (G_UNLIKELY (GST_VIDEO_INFO_FORMAT (&self->vinfo) ==
          GST_VIDEO_FORMAT_UNKNOWN))
    goto not_negotiated;

  timestamp = GST_BUFFER_TIMESTAMP (buffer);
  timestamp =
      gst_segment_to_stream_time (&self->segment, GST_FORMAT_TIME, timestamp);

  if (GST_CLOCK_TIME_IS_VALID (timestamp))
    gst_object_sync_values (GST_OBJECT (self), timestamp);

  GST_LOG_OBJECT (self,
      "Blending buffer with timestamp %" GST_TIME_FORMAT " at position %f",
      GST_TIME_ARGS (timestamp), self->mask_position);

  g_mutex_lock (&self->mask_mutex);
  if (self->shutdown)
    goto shutdown;

  if (!self->mask)
    g_cond_wait (&self->mask_cond, &self->mask_mutex);

  if (self->mask == NULL || self->shutdown) {
    goto shutdown;
  } else {
    mask = gst_buffer_ref (self->mask);
  }
  g_mutex_unlock (&self->mask_mutex);

  if (!gst_shape_wipe_do_qos (self, GST_BUFFER_TIMESTAMP (buffer)))
    goto qos;

  /* Will blend inplace if buffer is writable */
  outbuf = gst_buffer_make_writable (buffer);
  gst_video_frame_map (&outframe, &self->vinfo, outbuf, GST_MAP_READWRITE);
  gst_video_frame_map (&inframe, &self->vinfo, outbuf, GST_MAP_READ);

  gst_video_frame_map (&maskframe, &self->minfo, mask, GST_MAP_READ);

  switch (GST_VIDEO_INFO_FORMAT (&self->vinfo)) {
    case GST_VIDEO_FORMAT_AYUV:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_ABGR:
      if (self->mask_bpp == 16)
        gst_shape_wipe_blend_argb_16 (self, &inframe, &maskframe, &outframe);
      else
        gst_shape_wipe_blend_argb_8 (self, &inframe, &maskframe, &outframe);
      break;
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_RGBA:
      if (self->mask_bpp == 16)
        gst_shape_wipe_blend_bgra_16 (self, &inframe, &maskframe, &outframe);
      else
        gst_shape_wipe_blend_bgra_8 (self, &inframe, &maskframe, &outframe);
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  gst_video_frame_unmap (&outframe);
  gst_video_frame_unmap (&inframe);

  gst_video_frame_unmap (&maskframe);

  gst_buffer_unref (mask);

  ret = gst_pad_push (self->srcpad, outbuf);
  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto push_failed;

  return ret;

  /* Errors */
not_negotiated:
  {
    GST_ERROR_OBJECT (self, "No valid caps yet");
    gst_buffer_unref (buffer);
    return GST_FLOW_NOT_NEGOTIATED;
  }
shutdown:
  {
    GST_DEBUG_OBJECT (self, "Shutting down");
    gst_buffer_unref (buffer);
    return GST_FLOW_FLUSHING;
  }
qos:
  {
    GST_DEBUG_OBJECT (self, "Dropping buffer because of QoS");
    gst_buffer_unref (buffer);
    gst_buffer_unref (mask);
    return GST_FLOW_OK;
  }
push_failed:
  {
    if (ret != GST_FLOW_FLUSHING)
      GST_ERROR_OBJECT (self, "Pushing buffer downstream failed: %s",
          gst_flow_get_name (ret));
    return ret;
  }
}

static GstFlowReturn
gst_shape_wipe_mask_sink_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (parent);
  GstFlowReturn ret = GST_FLOW_OK;

  g_mutex_lock (&self->mask_mutex);
  GST_DEBUG_OBJECT (self, "Setting new mask buffer: %" GST_PTR_FORMAT, buffer);

  gst_buffer_replace (&self->mask, buffer);
  g_cond_signal (&self->mask_cond);
  g_mutex_unlock (&self->mask_mutex);

  gst_buffer_unref (buffer);

  return ret;
}

static GstStateChangeReturn
gst_shape_wipe_change_state (GstElement * element, GstStateChange transition)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      self->shutdown = FALSE;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      /* Unblock video sink chain function */
      g_mutex_lock (&self->mask_mutex);
      self->shutdown = TRUE;
      g_cond_signal (&self->mask_cond);
      g_mutex_unlock (&self->mask_mutex);
      break;
    default:
      break;
  }

  if (GST_ELEMENT_CLASS (parent_class)->change_state)
    ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_shape_wipe_reset (self);
      break;
    default:
      break;
  }

  return ret;
}

static gboolean
gst_shape_wipe_video_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (parent);
  gboolean ret;

  GST_LOG_OBJECT (pad, "Got %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      ret = gst_shape_wipe_video_sink_setcaps (self, caps);
      gst_event_unref (event);
      break;
    }
    case GST_EVENT_SEGMENT:
    {
      GstSegment seg;

      gst_event_copy_segment (event, &seg);
      if (seg.format == GST_FORMAT_TIME) {
        GST_DEBUG_OBJECT (pad,
            "Got SEGMENT event in GST_FORMAT_TIME %" GST_PTR_FORMAT, &seg);
        self->segment = seg;
      } else {
        gst_segment_init (&self->segment, GST_FORMAT_TIME);
      }
    }
      /* fall through */
    case GST_EVENT_FLUSH_STOP:
      gst_shape_wipe_reset_qos (self);
      /* fall through */
    default:
      ret = gst_pad_push_event (self->srcpad, event);
      break;
  }

  return ret;
}

static gboolean
gst_shape_wipe_mask_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (parent);

  GST_LOG_OBJECT (pad, "Got %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      gst_shape_wipe_mask_sink_setcaps (self, caps);
      break;
    }
    case GST_EVENT_FLUSH_STOP:
      g_mutex_lock (&self->mask_mutex);
      gst_buffer_replace (&self->mask, NULL);
      g_mutex_unlock (&self->mask_mutex);
      break;
    default:
      break;
  }

  /* Dropping all events here */
  gst_event_unref (event);

  return TRUE;
}

static gboolean
gst_shape_wipe_mask_sink_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (parent);
  gboolean ret;

  GST_LOG_OBJECT (pad, "Handling query of type '%s'",
      gst_query_type_get_name (GST_QUERY_TYPE (query)));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);
      caps = gst_shape_wipe_mask_sink_getcaps (self, pad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      ret = TRUE;
      break;
    }
    default:
      ret = gst_pad_query_default (pad, parent, query);
      break;
  }

  return ret;
}


static gboolean
gst_shape_wipe_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstShapeWipe *self = GST_SHAPE_WIPE (parent);
  gboolean ret;

  GST_LOG_OBJECT (pad, "Got %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_QOS:{
      GstQOSType type;
      GstClockTimeDiff diff;
      GstClockTime timestamp;
      gdouble proportion;

      gst_event_parse_qos (event, &type, &proportion, &diff, &timestamp);

      gst_shape_wipe_update_qos (self, proportion, diff, timestamp);
    }
      /* fall through */
    default:
      ret = gst_pad_push_event (self->video_sinkpad, event);
      break;
  }

  return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_shape_wipe_debug, "shapewipe", 0,
      "shapewipe element");

  if (!gst_element_register (plugin, "shapewipe", GST_RANK_NONE,
          GST_TYPE_SHAPE_WIPE))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    shapewipe,
    "Shape Wipe transition filter",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
