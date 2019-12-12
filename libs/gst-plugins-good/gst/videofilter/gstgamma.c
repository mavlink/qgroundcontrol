/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 * Copyright (C) 2003 Arwed v. Merkatz <v.merkatz@gmx.net>
 * Copyright (C) 2006 Mark Nauwelaerts <manauw@skynet.be>
 * Copyright (C) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

/*
 * This file was (probably) generated from
 * gstvideotemplate.c,v 1.12 2004/01/07 21:07:12 ds Exp 
 * and
 * make_filter,v 1.6 2004/01/07 21:33:01 ds Exp 
 */

/**
 * SECTION:element-gamma
 * @title: gamma
 *
 * Performs gamma correction on a video stream.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 videotestsrc ! gamma gamma=2.0 ! videoconvert ! ximagesink
 * ]| This pipeline will make the image "brighter".
 * |[
 * gst-launch-1.0 videotestsrc ! gamma gamma=0.5 ! videoconvert ! ximagesink
 * ]| This pipeline will make the image "darker".
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstgamma.h"
#include <string.h>
#include <math.h>

#include <gst/video/video.h>

GST_DEBUG_CATEGORY_STATIC (gamma_debug);
#define GST_CAT_DEFAULT gamma_debug

/* GstGamma properties */
enum
{
  PROP_0,
  PROP_GAMMA
      /* FILL ME */
};

#define DEFAULT_PROP_GAMMA  1

static GstStaticPadTemplate gst_gamma_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ AYUV, "
            "ARGB, BGRA, ABGR, RGBA, Y444, "
            "xRGB, RGBx, xBGR, BGRx, RGB, BGR, Y42B, NV12, "
            "NV21, YUY2, UYVY, YVYU, I420, YV12, IYUV, Y41B }"))
    );

static GstStaticPadTemplate gst_gamma_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ AYUV, "
            "ARGB, BGRA, ABGR, RGBA, Y444, "
            "xRGB, RGBx, xBGR, BGRx, RGB, BGR, Y42B, NV12, "
            "NV21, YUY2, UYVY, YVYU, I420, YV12, IYUV, Y41B }"))
    );

static void gst_gamma_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_gamma_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_gamma_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
static GstFlowReturn gst_gamma_transform_frame_ip (GstVideoFilter * vfilter,
    GstVideoFrame * frame);
static void gst_gamma_before_transform (GstBaseTransform * transform,
    GstBuffer * buf);

static void gst_gamma_calculate_tables (GstGamma * gamma);

G_DEFINE_TYPE (GstGamma, gst_gamma, GST_TYPE_VIDEO_FILTER);

static void
gst_gamma_class_init (GstGammaClass * g_class)
{
  GObjectClass *gobject_class = (GObjectClass *) g_class;
  GstElementClass *gstelement_class = (GstElementClass *) g_class;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) g_class;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) g_class;

  GST_DEBUG_CATEGORY_INIT (gamma_debug, "gamma", 0, "gamma");

  gobject_class->set_property = gst_gamma_set_property;
  gobject_class->get_property = gst_gamma_get_property;

  g_object_class_install_property (gobject_class, PROP_GAMMA,
      g_param_spec_double ("gamma", "Gamma", "gamma",
          0.01, 10, DEFAULT_PROP_GAMMA,
          GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  gst_element_class_set_static_metadata (gstelement_class,
      "Video gamma correction", "Filter/Effect/Video",
      "Adjusts gamma on a video stream", "Arwed v. Merkatz <v.merkatz@gmx.net");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_gamma_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_gamma_src_template);

  trans_class->before_transform =
      GST_DEBUG_FUNCPTR (gst_gamma_before_transform);
  trans_class->transform_ip_on_passthrough = FALSE;

  vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_gamma_set_info);
  vfilter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR (gst_gamma_transform_frame_ip);
}

static void
gst_gamma_init (GstGamma * gamma)
{
  /* properties */
  gamma->gamma = DEFAULT_PROP_GAMMA;
  gst_gamma_calculate_tables (gamma);
}

static void
gst_gamma_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstGamma *gamma = GST_GAMMA (object);

  switch (prop_id) {
    case PROP_GAMMA:{
      gdouble val = g_value_get_double (value);

      GST_DEBUG_OBJECT (gamma, "Changing gamma from %lf to %lf", gamma->gamma,
          val);
      GST_OBJECT_LOCK (gamma);
      gamma->gamma = val;
      GST_OBJECT_UNLOCK (gamma);
      gst_gamma_calculate_tables (gamma);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_gamma_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstGamma *gamma = GST_GAMMA (object);

  switch (prop_id) {
    case PROP_GAMMA:
      g_value_set_double (value, gamma->gamma);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_gamma_calculate_tables (GstGamma * gamma)
{
  gint n;
  gdouble val;
  gdouble exp;
  gboolean passthrough = FALSE;

  GST_OBJECT_LOCK (gamma);
  if (gamma->gamma == 1.0) {
    passthrough = TRUE;
  } else {
    exp = 1.0 / gamma->gamma;
    for (n = 0; n < 256; n++) {
      val = n / 255.0;
      val = pow (val, exp);
      val = 255.0 * val;
      gamma->gamma_table[n] = (guint8) floor (val + 0.5);
    }
  }
  GST_OBJECT_UNLOCK (gamma);

  gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (gamma), passthrough);
}

static void
gst_gamma_planar_yuv_ip (GstGamma * gamma, GstVideoFrame * frame)
{
  gint i, j, height;
  gint width, stride, row_wrap;
  const guint8 *table = gamma->gamma_table;
  guint8 *data;

  data = GST_VIDEO_FRAME_COMP_DATA (frame, 0);
  stride = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0);
  width = GST_VIDEO_FRAME_COMP_WIDTH (frame, 0);
  height = GST_VIDEO_FRAME_COMP_HEIGHT (frame, 0);
  row_wrap = stride - width;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      *data = table[*data];
      data++;
    }
    data += row_wrap;
  }
}

static void
gst_gamma_packed_yuv_ip (GstGamma * gamma, GstVideoFrame * frame)
{
  gint i, j, height;
  gint width, stride, row_wrap;
  gint pixel_stride;
  const guint8 *table = gamma->gamma_table;
  guint8 *data;

  data = GST_VIDEO_FRAME_COMP_DATA (frame, 0);
  stride = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0);
  width = GST_VIDEO_FRAME_COMP_WIDTH (frame, 0);
  height = GST_VIDEO_FRAME_COMP_HEIGHT (frame, 0);
  pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE (frame, 0);
  row_wrap = stride - pixel_stride * width;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      *data = table[*data];
      data += pixel_stride;
    }
    data += row_wrap;
  }
}

static const int cog_ycbcr_to_rgb_matrix_8bit_sdtv[] = {
  298, 0, 409, -57068,
  298, -100, -208, 34707,
  298, 516, 0, -70870,
};

static const gint cog_rgb_to_ycbcr_matrix_8bit_sdtv[] = {
  66, 129, 25, 4096,
  -38, -74, 112, 32768,
  112, -94, -18, 32768,
};

#define APPLY_MATRIX(m,o,v1,v2,v3) ((m[o*4] * v1 + m[o*4+1] * v2 + m[o*4+2] * v3 + m[o*4+3]) >> 8)

static void
gst_gamma_packed_rgb_ip (GstGamma * gamma, GstVideoFrame * frame)
{
  gint i, j, height;
  gint width, stride, row_wrap;
  gint pixel_stride;
  const guint8 *table = gamma->gamma_table;
  gint offsets[3];
  gint r, g, b;
  gint y, u, v;
  guint8 *data;

  data = GST_VIDEO_FRAME_PLANE_DATA (frame, 0);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE (frame, 0);
  width = GST_VIDEO_FRAME_COMP_WIDTH (frame, 0);
  height = GST_VIDEO_FRAME_COMP_HEIGHT (frame, 0);

  offsets[0] = GST_VIDEO_FRAME_COMP_OFFSET (frame, 0);
  offsets[1] = GST_VIDEO_FRAME_COMP_OFFSET (frame, 1);
  offsets[2] = GST_VIDEO_FRAME_COMP_OFFSET (frame, 2);

  pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE (frame, 0);
  row_wrap = stride - pixel_stride * width;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      r = data[offsets[0]];
      g = data[offsets[1]];
      b = data[offsets[2]];

      y = APPLY_MATRIX (cog_rgb_to_ycbcr_matrix_8bit_sdtv, 0, r, g, b);
      u = APPLY_MATRIX (cog_rgb_to_ycbcr_matrix_8bit_sdtv, 1, r, g, b);
      v = APPLY_MATRIX (cog_rgb_to_ycbcr_matrix_8bit_sdtv, 2, r, g, b);

      y = table[CLAMP (y, 0, 255)];
      r = APPLY_MATRIX (cog_ycbcr_to_rgb_matrix_8bit_sdtv, 0, y, u, v);
      g = APPLY_MATRIX (cog_ycbcr_to_rgb_matrix_8bit_sdtv, 1, y, u, v);
      b = APPLY_MATRIX (cog_ycbcr_to_rgb_matrix_8bit_sdtv, 2, y, u, v);

      data[offsets[0]] = CLAMP (r, 0, 255);
      data[offsets[1]] = CLAMP (g, 0, 255);
      data[offsets[2]] = CLAMP (b, 0, 255);
      data += pixel_stride;
    }
    data += row_wrap;
  }
}

static gboolean
gst_gamma_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstGamma *gamma = GST_GAMMA (vfilter);

  GST_DEBUG_OBJECT (gamma,
      "setting caps: in %" GST_PTR_FORMAT " out %" GST_PTR_FORMAT, incaps,
      outcaps);

  switch (GST_VIDEO_INFO_FORMAT (in_info)) {
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
    case GST_VIDEO_FORMAT_Y41B:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y444:
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
      gamma->process = gst_gamma_planar_yuv_ip;
      break;
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
    case GST_VIDEO_FORMAT_AYUV:
    case GST_VIDEO_FORMAT_YVYU:
      gamma->process = gst_gamma_packed_yuv_ip;
      break;
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
      gamma->process = gst_gamma_packed_rgb_ip;
      break;
    default:
      goto invalid_caps;
      break;
  }
  return TRUE;

  /* ERRORS */
invalid_caps:
  {
    GST_ERROR_OBJECT (gamma, "Invalid caps: %" GST_PTR_FORMAT, incaps);
    return FALSE;
  }
}

static void
gst_gamma_before_transform (GstBaseTransform * base, GstBuffer * outbuf)
{
  GstGamma *gamma = GST_GAMMA (base);
  GstClockTime timestamp, stream_time;

  timestamp = GST_BUFFER_TIMESTAMP (outbuf);
  stream_time =
      gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (gamma, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (gamma), stream_time);
}

static GstFlowReturn
gst_gamma_transform_frame_ip (GstVideoFilter * vfilter, GstVideoFrame * frame)
{
  GstGamma *gamma = GST_GAMMA (vfilter);

  if (!gamma->process)
    goto not_negotiated;

  GST_OBJECT_LOCK (gamma);
  gamma->process (gamma, frame);
  GST_OBJECT_UNLOCK (gamma);

  return GST_FLOW_OK;

  /* ERRORS */
not_negotiated:
  {
    GST_ERROR_OBJECT (gamma, "Not negotiated yet");
    return GST_FLOW_NOT_NEGOTIATED;
  }
}
