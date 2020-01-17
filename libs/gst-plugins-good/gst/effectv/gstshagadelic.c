/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *
 * EffecTV:
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * Inspired by Adrian Likin's script for the GIMP.
 * EffecTV is free software.  This library is free software;
 * you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
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
 * SECTION:element-shagadelictv
 * @title: shagadelictv
 *
 * Oh behave, ShagedelicTV makes images shagadelic!
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! shagadelictv ! videoconvert ! autovideosink
 * ]| This pipeline shows the effect of shagadelictv on a test stream.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "gstshagadelic.h"
#include "gsteffectv.h"

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

#define gst_shagadelictv_parent_class parent_class
G_DEFINE_TYPE (GstShagadelicTV, gst_shagadelictv, GST_TYPE_VIDEO_FILTER);

static void gst_shagadelic_initialize (GstShagadelicTV * filter,
    GstVideoInfo * in_info);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAPS_STR GST_VIDEO_CAPS_MAKE ("BGRx")
#else
#define CAPS_STR GST_VIDEO_CAPS_MAKE ("xRGB")
#endif

static GstStaticPadTemplate gst_shagadelictv_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static GstStaticPadTemplate gst_shagadelictv_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static gboolean
gst_shagadelictv_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstShagadelicTV *filter = GST_SHAGADELICTV (vfilter);
  gint width, height, area;

  width = GST_VIDEO_INFO_WIDTH (in_info);
  height = GST_VIDEO_INFO_HEIGHT (in_info);

  area = width * height;

  g_free (filter->ripple);
  g_free (filter->spiral);
  filter->ripple = (guint8 *) g_malloc (area * 4);
  filter->spiral = (guint8 *) g_malloc (area);

  gst_shagadelic_initialize (filter, in_info);

  return TRUE;
}

static void
gst_shagadelic_initialize (GstShagadelicTV * filter, GstVideoInfo * info)
{
  int i, x, y;
#ifdef PS2
  float xx, yy;
#else
  double xx, yy;
#endif
  gint width, height;

  width = GST_VIDEO_INFO_WIDTH (info);
  height = GST_VIDEO_INFO_HEIGHT (info);

  i = 0;
  for (y = 0; y < height * 2; y++) {
    yy = y - height;
    yy *= yy;

    for (x = 0; x < width * 2; x++) {
      xx = x - width;
#ifdef PS2
      filter->ripple[i++] = ((unsigned int) (sqrtf (xx * xx + yy) * 8)) & 255;
#else
      filter->ripple[i++] = ((unsigned int) (sqrt (xx * xx + yy) * 8)) & 255;
#endif
    }
  }

  i = 0;
  for (y = 0; y < height; y++) {
    yy = y - height / 2;

    for (x = 0; x < width; x++) {
      xx = x - width / 2;
#ifdef PS2
      filter->spiral[i++] = ((unsigned int)
          ((atan2f (xx,
                      yy) / ((float) M_PI) * 256 * 9) + (sqrtf (xx * xx +
                      yy * yy) * 5))) & 255;
#else
      filter->spiral[i++] = ((unsigned int)
          ((atan2 (xx, yy) / M_PI * 256 * 9) + (sqrt (xx * xx +
                      yy * yy) * 5))) & 255;
#endif
/* Here is another Swinger!
 * ((atan2(xx, yy)/M_PI*256) + (sqrt(xx*xx+yy*yy)*10))&255;
 */
    }
  }
  filter->rx = fastrand () % width;
  filter->ry = fastrand () % height;
  filter->bx = fastrand () % width;
  filter->by = fastrand () % height;
  filter->rvx = -2;
  filter->rvy = -2;
  filter->bvx = 2;
  filter->bvy = 2;
  filter->phase = 0;
}

static GstFlowReturn
gst_shagadelictv_transform_frame (GstVideoFilter * vfilter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
  GstShagadelicTV *filter = GST_SHAGADELICTV (vfilter);
  guint32 *src, *dest;
  gint x, y;
  guint32 v;
  guint8 r, g, b;
  gint width, height;

  src = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  dest = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

  width = GST_VIDEO_FRAME_WIDTH (in_frame);
  height = GST_VIDEO_FRAME_HEIGHT (in_frame);

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      v = *src++ | 0x1010100;
      v = (v - 0x707060) & 0x1010100;
      v -= v >> 8;
/* Try another Babe! 
 * v = *src++;
 * *dest++ = v & ((r<<16)|(g<<8)|b);
 */
      r = ((gint8) (filter->ripple[(filter->ry + y) * width * 2 + filter->rx +
                  x] + filter->phase * 2)) >> 7;
      g = ((gint8) (filter->spiral[y * width + x] + filter->phase * 3)) >> 7;
      b = ((gint8) (filter->ripple[(filter->by + y) * width * 2 + filter->bx +
                  x] - filter->phase)) >> 7;
      *dest++ = v & ((r << 16) | (g << 8) | b);
    }
  }

  filter->phase -= 8;
  if ((filter->rx + filter->rvx) < 0 || (filter->rx + filter->rvx) >= width)
    filter->rvx = -filter->rvx;
  if ((filter->ry + filter->rvy) < 0 || (filter->ry + filter->rvy) >= height)
    filter->rvy = -filter->rvy;
  if ((filter->bx + filter->bvx) < 0 || (filter->bx + filter->bvx) >= width)
    filter->bvx = -filter->bvx;
  if ((filter->by + filter->bvy) < 0 || (filter->by + filter->bvy) >= height)
    filter->bvy = -filter->bvy;
  filter->rx += filter->rvx;
  filter->ry += filter->rvy;
  filter->bx += filter->bvx;
  filter->by += filter->bvy;

  return GST_FLOW_OK;
}

static void
gst_shagadelictv_finalize (GObject * object)
{
  GstShagadelicTV *filter = GST_SHAGADELICTV (object);

  g_free (filter->ripple);
  filter->ripple = NULL;

  g_free (filter->spiral);
  filter->spiral = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_shagadelictv_class_init (GstShagadelicTVClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->finalize = gst_shagadelictv_finalize;

  gst_element_class_set_static_metadata (gstelement_class, "ShagadelicTV",
      "Filter/Effect/Video",
      "Oh behave, ShagedelicTV makes images shagadelic!",
      "Wim Taymans <wim.taymans@chello.be>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_shagadelictv_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_shagadelictv_src_template);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_shagadelictv_set_info);
  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_shagadelictv_transform_frame);
}

static void
gst_shagadelictv_init (GstShagadelicTV * filter)
{
  filter->ripple = NULL;
  filter->spiral = NULL;
}
