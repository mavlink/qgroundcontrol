/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *
 * EffecTV:
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * revTV based on Rutt-Etra Video Synthesizer 1974?

 * (c)2002 Ed Tannenbaum
 *
 * This effect acts like a waveform monitor on each line.
 * It was originally done by deflecting the electron beam on a monitor using
 * additional electromagnets on the yoke of a b/w CRT. 
 * Here it is emulated digitally.

 * Experimaental tapes were made with this system by Bill and 
 * Louise Etra and Woody and Steina Vasulka

 * The line spacing can be controlled using the 1 and 2 Keys.
 * The gain is controlled using the 3 and 4 keys.
 * The update rate is controlled using the 0 and - keys.
 
 * EffecTV is free software. This library is free software;
 * you can redistribute it and/or
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
 * SECTION:element-revtv
 * @title: revtv
 *
 * RevTV acts like a video waveform monitor for each line of video
 * processed. This creates a pseudo 3D effect based on the brightness
 * of the video along each line.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! revtv ! videoconvert ! autovideosink
 * ]| This pipeline shows the effect of revtv on a test stream.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "gstrev.h"

#define THE_COLOR 0xffffffff

enum
{
  PROP_0,
  PROP_DELAY,
  PROP_LINESPACE,
  PROP_GAIN
};

#define gst_revtv_parent_class parent_class
G_DEFINE_TYPE (GstRevTV, gst_revtv, GST_TYPE_VIDEO_FILTER);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAPS_STR GST_VIDEO_CAPS_MAKE ("{ BGRx, RGBx }")
#else
#define CAPS_STR GST_VIDEO_CAPS_MAKE ("{ xBGR, xRGB }")
#endif

static GstStaticPadTemplate gst_revtv_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static GstStaticPadTemplate gst_revtv_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static GstFlowReturn
gst_revtv_transform_frame (GstVideoFilter * vfilter, GstVideoFrame * in_frame,
    GstVideoFrame * out_frame)
{
  GstRevTV *filter = GST_REVTV (vfilter);
  guint32 *src, *dest;
  gint width, height, sstride, dstride;
  guint32 *nsrc;
  gint y, x, R, G, B, yval;
  gint linespace, vscale;
  GstClockTime timestamp, stream_time;

  timestamp = GST_BUFFER_TIMESTAMP (in_frame->buffer);
  stream_time =
      gst_segment_to_stream_time (&GST_BASE_TRANSFORM (vfilter)->segment,
      GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (filter, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (filter), stream_time);

  src = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  sstride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);
  dest = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);
  dstride = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0);

  width = GST_VIDEO_FRAME_WIDTH (in_frame);
  height = GST_VIDEO_FRAME_HEIGHT (in_frame);

  /* Clear everything to black */
  memset (dest, 0, dstride * height);

  GST_OBJECT_LOCK (filter);
  linespace = filter->linespace;
  vscale = filter->vscale;

  /* draw the offset lines */
  for (y = 0; y < height; y += linespace) {
    for (x = 0; x <= width; x++) {
      nsrc = src + (y * sstride / 4) + x;

      /* Calc Y Value for curpix */
      R = ((*nsrc) & 0xff0000) >> (16 - 1);
      G = ((*nsrc) & 0xff00) >> (8 - 2);
      B = (*nsrc) & 0xff;

      yval = y - ((short) (R + G + B) / vscale);

      if (yval > 0) {
        dest[x + (yval * dstride / 4)] = THE_COLOR;
      }
    }
  }
  GST_OBJECT_UNLOCK (filter);

  return GST_FLOW_OK;
}

static void
gst_revtv_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstRevTV *filter = GST_REVTV (object);

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_DELAY:
      filter->vgrabtime = g_value_get_int (value);
      break;
    case PROP_LINESPACE:
      filter->linespace = g_value_get_int (value);
      break;
    case PROP_GAIN:
      filter->vscale = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static void
gst_revtv_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstRevTV *filter = GST_REVTV (object);

  switch (prop_id) {
    case PROP_DELAY:
      g_value_set_int (value, filter->vgrabtime);
      break;
    case PROP_LINESPACE:
      g_value_set_int (value, filter->linespace);
      break;
    case PROP_GAIN:
      g_value_set_int (value, filter->vscale);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_revtv_class_init (GstRevTVClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_revtv_set_property;
  gobject_class->get_property = gst_revtv_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DELAY,
      g_param_spec_int ("delay", "Delay", "Delay in frames between updates",
          1, 100, 1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_LINESPACE,
      g_param_spec_int ("linespace", "Linespace", "Control line spacing", 1,
          100, 6,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_GAIN,
      g_param_spec_int ("gain", "Gain", "Control gain", 1, 200, 50,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  gst_element_class_set_static_metadata (gstelement_class, "RevTV effect",
      "Filter/Effect/Video",
      "A video waveform monitor for each line of video processed",
      "Wim Taymans <wim.taymans@gmail.be>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_revtv_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_revtv_src_template);

  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_revtv_transform_frame);
}

static void
gst_revtv_init (GstRevTV * restv)
{
  restv->vgrabtime = 1;
  restv->vgrab = 0;
  restv->linespace = 6;
  restv->vscale = 50;
}
