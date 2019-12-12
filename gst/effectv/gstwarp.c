/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 *
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
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
 * This file was (probably) generated from gstvideotemplate.c,
 * gstvideotemplate.c,v 1.11 2004/01/07 08:56:45 ds Exp 
 */

/* From main.c of warp-1.1:
 *
 *      Simple DirectMedia Layer demo
 *      Realtime picture 'gooing'
 *      by sam lantinga slouken@devolution.com
 */

/**
 * SECTION:element-warptv
 * @title: warptv
 *
 * WarpTV does realtime goo'ing of the video input.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! warptv ! videoconvert ! autovideosink
 * ]| This pipeline shows the effect of warptv on a test stream.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>

#include "gstwarp.h"
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define gst_warptv_parent_class parent_class
G_DEFINE_TYPE (GstWarpTV, gst_warptv, GST_TYPE_VIDEO_FILTER);

static void initSinTable ();
static void initDistTable (GstWarpTV * filter, gint width, gint height);

static GstStaticPadTemplate gst_warptv_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ RGBx, xRGB, BGRx, xBGR }"))
    );

static GstStaticPadTemplate gst_warptv_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ RGBx, xRGB, BGRx, xBGR }"))
    );

static gboolean
gst_warptv_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstWarpTV *filter = GST_WARPTV (vfilter);
  gint width, height;

  width = GST_VIDEO_INFO_WIDTH (in_info);
  height = GST_VIDEO_INFO_HEIGHT (in_info);

  g_free (filter->disttable);
  filter->disttable = g_malloc (width * height * sizeof (guint32));
  initDistTable (filter, width, height);

  return TRUE;
}

static gint32 sintable[1024 + 256];

static void
initSinTable (void)
{
  gint32 *tptr, *tsinptr;
  gint i;

  tsinptr = tptr = sintable;

  for (i = 0; i < 1024; i++)
    *tptr++ = (int) (sin (i * M_PI / 512) * 32767);

  for (i = 0; i < 256; i++)
    *tptr++ = *tsinptr++;
}

static void
initDistTable (GstWarpTV * filter, gint width, gint height)
{
  gint32 halfw, halfh, *distptr;
  gint x, y;
  float m;

  halfw = width >> 1;
  halfh = height >> 1;

  distptr = filter->disttable;

  m = sqrt ((double) (halfw * halfw + halfh * halfh));

  for (y = -halfh; y < halfh; y++)
    for (x = -halfw; x < halfw; x++)
#ifdef PS2
      *distptr++ = ((int) ((sqrtf (x * x + y * y) * 511.9999) / m)) << 1;
#else
      *distptr++ = ((int) ((sqrt (x * x + y * y) * 511.9999) / m)) << 1;
#endif
}

static GstFlowReturn
gst_warptv_transform_frame (GstVideoFilter * filter, GstVideoFrame * in_frame,
    GstVideoFrame * out_frame)
{
  GstWarpTV *warptv = GST_WARPTV (filter);
  gint width, height;
  gint xw, yw, cw;
  gint32 c, i, x, y, dx, dy, maxx, maxy;
  gint32 *ctptr, *distptr;
  gint32 *ctable;
  guint32 *src, *dest;
  gint sstride, dstride;

  src = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  dest = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

  sstride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);
  dstride = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0);

  width = GST_VIDEO_FRAME_WIDTH (in_frame);
  height = GST_VIDEO_FRAME_HEIGHT (in_frame);

  GST_OBJECT_LOCK (warptv);
  xw = (gint) (sin ((warptv->tval + 100) * M_PI / 128) * 30);
  yw = (gint) (sin ((warptv->tval) * M_PI / 256) * -35);
  cw = (gint) (sin ((warptv->tval - 70) * M_PI / 64) * 50);
  xw += (gint) (sin ((warptv->tval - 10) * M_PI / 512) * 40);
  yw += (gint) (sin ((warptv->tval + 30) * M_PI / 512) * 40);

  ctptr = warptv->ctable;
  distptr = warptv->disttable;
  ctable = warptv->ctable;

  c = 0;

  for (x = 0; x < 512; x++) {
    i = (c >> 3) & 0x3FE;
    *ctptr++ = ((sintable[i] * yw) >> 15);
    *ctptr++ = ((sintable[i + 256] * xw) >> 15);
    c += cw;
  }
  maxx = width - 2;
  maxy = height - 2;

  for (y = 0; y < height - 1; y++) {
    for (x = 0; x < width; x++) {
      i = *distptr++;
      dx = ctable[i + 1] + x;
      dy = ctable[i] + y;

      if (dx < 0)
        dx = 0;
      else if (dx > maxx)
        dx = maxx;

      if (dy < 0)
        dy = 0;
      else if (dy > maxy)
        dy = maxy;

      dest[x] = src[dy * sstride / 4 + dx];
    }
    dest += dstride / 4;
  }

  warptv->tval = (warptv->tval + 1) & 511;
  GST_OBJECT_UNLOCK (warptv);

  return GST_FLOW_OK;
}

static gboolean
gst_warptv_start (GstBaseTransform * trans)
{
  GstWarpTV *warptv = GST_WARPTV (trans);

  warptv->tval = 0;

  return TRUE;
}

static void
gst_warptv_finalize (GObject * object)
{
  GstWarpTV *warptv = GST_WARPTV (object);

  g_free (warptv->disttable);
  warptv->disttable = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_warptv_class_init (GstWarpTVClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->finalize = gst_warptv_finalize;

  gst_element_class_set_static_metadata (gstelement_class, "WarpTV effect",
      "Filter/Effect/Video",
      "WarpTV does realtime goo'ing of the video input",
      "Sam Lantinga <slouken@devolution.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_warptv_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_warptv_src_template);

  trans_class->start = GST_DEBUG_FUNCPTR (gst_warptv_start);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_warptv_set_info);
  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_warptv_transform_frame);

  initSinTable ();
}

static void
gst_warptv_init (GstWarpTV * warptv)
{
  /* nothing to do */
}
