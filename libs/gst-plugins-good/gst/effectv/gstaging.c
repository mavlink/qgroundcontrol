/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2002 FUKUCHI Kentarou
 *
 * AgingTV - film-aging effect.
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
 * SECTION:element-agingtv
 * @title: agingtv
 *
 * AgingTV ages a video stream in realtime, changes the colors and adds
 * scratches and dust.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! agingtv ! videoconvert ! autovideosink
 * ]| This pipeline shows the effect of agingtv on a test stream.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>

#include "gstaging.h"
#include "gsteffectv.h"

static const gint dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static const gint dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };

enum
{
  PROP_0 = 0,
  PROP_SCRATCH_LINES,
  PROP_COLOR_AGING,
  PROP_PITS,
  PROP_DUSTS
};

#define DEFAULT_SCRATCH_LINES 7
#define DEFAULT_COLOR_AGING TRUE
#define DEFAULT_PITS TRUE
#define DEFAULT_DUSTS TRUE

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAPS_STR GST_VIDEO_CAPS_MAKE ("{ BGRx, RGBx }")
#else
#define CAPS_STR GST_VIDEO_CAPS_MAKE ("{ xRGB, xBGR }")
#endif

static GstStaticPadTemplate gst_agingtv_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static GstStaticPadTemplate gst_agingtv_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

G_DEFINE_TYPE (GstAgingTV, gst_agingtv, GST_TYPE_VIDEO_FILTER);

static void
coloraging (guint32 * src, guint32 * dest, gint video_area, gint * c)
{
  guint32 a, b;
  gint i;
  gint c_tmp = *c;

  c_tmp -= (gint) (fastrand ()) >> 28;
  if (c_tmp < 0)
    c_tmp = 0;
  if (c_tmp > 0x18)
    c_tmp = 0x18;

  for (i = 0; i < video_area; i++) {
    a = *src++;
    b = (a & 0xfcfcfc) >> 2;
    *dest++ =
        a - b + (c_tmp | (c_tmp << 8) | (c_tmp << 16)) +
        ((fastrand () >> 8) & 0x101010);
  }
  *c = c_tmp;
}


static void
scratching (scratch * scratches, gint scratch_lines, guint32 * dest, gint width,
    gint height)
{
  gint i, y, y1, y2;
  guint32 *p, a, b;
  scratch *scratch;

  for (i = 0; i < scratch_lines; i++) {
    scratch = &scratches[i];

    if (scratch->life) {
      scratch->x = scratch->x + scratch->dx;

      if (scratch->x < 0 || scratch->x > width * 256) {
        scratch->life = 0;
        break;
      }
      p = dest + (scratch->x >> 8);
      if (scratch->init) {
        y1 = scratch->init;
        scratch->init = 0;
      } else {
        y1 = 0;
      }
      scratch->life--;
      if (scratch->life) {
        y2 = height;
      } else {
        y2 = fastrand () % height;
      }
      for (y = y1; y < y2; y++) {
        a = *p & 0xfefeff;
        a += 0x202020;
        b = a & 0x1010100;
        *p = a | (b - (b >> 8));
        p += width;
      }
    } else {
      if ((fastrand () & 0xf0000000) == 0) {
        scratch->life = 2 + (fastrand () >> 27);
        scratch->x = fastrand () % (width * 256);
        scratch->dx = ((int) fastrand ()) >> 23;
        scratch->init = (fastrand () % (height - 1)) + 1;
      }
    }
  }
}

static void
dusts (guint32 * dest, gint width, gint height, gint * dust_interval,
    gint area_scale)
{
  gint i, j;
  gint dnum;
  gint d, len;
  guint x, y;

  if (*dust_interval == 0) {
    if ((fastrand () & 0xf0000000) == 0) {
      *dust_interval = fastrand () >> 29;
    }
    return;
  }
  dnum = area_scale * 4 + (fastrand () >> 27);

  for (i = 0; i < dnum; i++) {
    x = fastrand () % width;
    y = fastrand () % height;
    d = fastrand () >> 29;
    len = fastrand () % area_scale + 5;
    for (j = 0; j < len; j++) {
      dest[y * width + x] = 0x101010;
      y += dy[d];
      x += dx[d];

      if (y >= height || x >= width)
        break;

      d = (d + fastrand () % 3 - 1) & 7;
    }
  }
  *dust_interval = *dust_interval - 1;
}

static void
pits (guint32 * dest, gint width, gint height, gint area_scale,
    gint * pits_interval)
{
  gint i, j;
  gint pnum, size, pnumscale;
  guint x, y;

  pnumscale = area_scale * 2;
  if (*pits_interval) {
    pnum = pnumscale + (fastrand () % pnumscale);

    *pits_interval = *pits_interval - 1;
  } else {
    pnum = fastrand () % pnumscale;

    if ((fastrand () & 0xf8000000) == 0) {
      *pits_interval = (fastrand () >> 28) + 20;
    }
  }
  for (i = 0; i < pnum; i++) {
    x = fastrand () % (width - 1);
    y = fastrand () % (height - 1);

    size = fastrand () >> 28;

    for (j = 0; j < size; j++) {
      x = x + fastrand () % 3 - 1;
      y = y + fastrand () % 3 - 1;

      if (y >= height || x >= width)
        break;

      dest[y * width + x] = 0xc0c0c0;
    }
  }
}

static void
gst_agingtv_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAgingTV *agingtv = GST_AGINGTV (object);

  GST_OBJECT_LOCK (agingtv);
  switch (prop_id) {
    case PROP_SCRATCH_LINES:
      g_value_set_uint (value, agingtv->scratch_lines);
      break;
    case PROP_COLOR_AGING:
      g_value_set_boolean (value, agingtv->color_aging);
      break;
    case PROP_PITS:
      g_value_set_boolean (value, agingtv->pits);
      break;
    case PROP_DUSTS:
      g_value_set_boolean (value, agingtv->dusts);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
  GST_OBJECT_UNLOCK (agingtv);
}

static void
gst_agingtv_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAgingTV *agingtv = GST_AGINGTV (object);

  switch (prop_id) {
    case PROP_SCRATCH_LINES:
      agingtv->scratch_lines = g_value_get_uint (value);
      break;
    case PROP_COLOR_AGING:
      agingtv->color_aging = g_value_get_boolean (value);
      break;
    case PROP_PITS:
      agingtv->pits = g_value_get_boolean (value);
      break;
    case PROP_DUSTS:
      agingtv->dusts = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static gboolean
gst_agingtv_start (GstBaseTransform * trans)
{
  GstAgingTV *agingtv = GST_AGINGTV (trans);

  agingtv->coloraging_state = 0x18;
  agingtv->dust_interval = 0;
  agingtv->pits_interval = 0;

  memset (agingtv->scratches, 0, sizeof (agingtv->scratches));

  return TRUE;
}

static GstFlowReturn
gst_agingtv_transform_frame (GstVideoFilter * filter, GstVideoFrame * in_frame,
    GstVideoFrame * out_frame)
{
  GstAgingTV *agingtv = GST_AGINGTV (filter);
  gint area_scale;
  GstClockTime timestamp, stream_time;
  gint width, height, stride, video_size;
  guint32 *src, *dest;

  timestamp = GST_BUFFER_TIMESTAMP (in_frame->buffer);
  stream_time =
      gst_segment_to_stream_time (&GST_BASE_TRANSFORM (filter)->segment,
      GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (agingtv, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (agingtv), stream_time);

  width = GST_VIDEO_FRAME_WIDTH (in_frame);
  height = GST_VIDEO_FRAME_HEIGHT (in_frame);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);
  video_size = stride * height / 4;

  src = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  dest = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

  area_scale = width * height / 64 / 480;
  if (area_scale <= 0)
    area_scale = 1;

  if (agingtv->color_aging)
    coloraging (src, dest, video_size, &agingtv->coloraging_state);
  else
    memcpy (dest, src, stride * height);

  scratching (agingtv->scratches, agingtv->scratch_lines, dest, width, height);
  if (agingtv->pits)
    pits (dest, width, height, area_scale, &agingtv->pits_interval);
  if (area_scale > 1 && agingtv->dusts)
    dusts (dest, width, height, &agingtv->dust_interval, area_scale);

  return GST_FLOW_OK;
}

static void
gst_agingtv_class_init (GstAgingTVClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_agingtv_set_property;
  gobject_class->get_property = gst_agingtv_get_property;

  g_object_class_install_property (gobject_class, PROP_SCRATCH_LINES,
      g_param_spec_uint ("scratch-lines", "Scratch Lines",
          "Number of scratch lines", 0, SCRATCH_MAX, DEFAULT_SCRATCH_LINES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_COLOR_AGING,
      g_param_spec_boolean ("color-aging", "Color Aging",
          "Color Aging", DEFAULT_COLOR_AGING,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_PITS,
      g_param_spec_boolean ("pits", "Pits",
          "Pits", DEFAULT_PITS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_DUSTS,
      g_param_spec_boolean ("dusts", "Dusts",
          "Dusts", DEFAULT_DUSTS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  gst_element_class_set_static_metadata (gstelement_class, "AgingTV effect",
      "Filter/Effect/Video",
      "AgingTV adds age to video input using scratches and dust",
      "Sam Lantinga <slouken@devolution.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_agingtv_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_agingtv_src_template);

  trans_class->start = GST_DEBUG_FUNCPTR (gst_agingtv_start);

  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_agingtv_transform_frame);
}

static void
gst_agingtv_init (GstAgingTV * agingtv)
{
  agingtv->scratch_lines = DEFAULT_SCRATCH_LINES;
  agingtv->color_aging = DEFAULT_COLOR_AGING;
  agingtv->pits = DEFAULT_PITS;
  agingtv->dusts = DEFAULT_DUSTS;
}
