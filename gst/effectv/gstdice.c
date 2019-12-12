/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * dice.c: a 'dicing' effect
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Library Public License.
 *
 * I suppose this looks similar to PuzzleTV, but it's not. The screen is
 * divided into small squares, each of which is rotated either 0, 90, 180 or
 * 270 degrees.  The amount of rotation for each square is chosen at random.
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
 * SECTION:element-dicetv
 * @title: dicetv
 *
 * DiceTV 'dices' the screen up into many small squares, each defaulting
 * to a size of 16 pixels by 16 pixels.. Each square is rotated randomly
 * in one of four directions: up (no change), down (180 degrees, or
 * upside down), right (90 degrees clockwise), or left (90 degrees
 * counterclockwise). The direction of each square normally remains
 * consistent between each frame.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! dicetv ! videoconvert ! autovideosink
 * ]| This pipeline shows the effect of dicetv on a test stream.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gstdice.h"
#include "gsteffectv.h"

#define DEFAULT_CUBE_BITS   4
#define MAX_CUBE_BITS       5
#define MIN_CUBE_BITS       0

typedef enum _dice_dir
{
  DICE_UP = 0,
  DICE_RIGHT = 1,
  DICE_DOWN = 2,
  DICE_LEFT = 3
} DiceDir;

#define gst_dicetv_parent_class parent_class
G_DEFINE_TYPE (GstDiceTV, gst_dicetv, GST_TYPE_VIDEO_FILTER);

static void gst_dicetv_create_map (GstDiceTV * filter, GstVideoInfo * info);

static GstStaticPadTemplate gst_dicetv_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ RGBx, xRGB, BGRx, xBGR }"))
    );

static GstStaticPadTemplate gst_dicetv_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ RGBx, xRGB, BGRx, xBGR }"))
    );

enum
{
  PROP_0,
  PROP_CUBE_BITS
};

static gboolean
gst_dicetv_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstDiceTV *filter = GST_DICETV (vfilter);

  g_free (filter->dicemap);
  filter->dicemap =
      (guint8 *) g_malloc (GST_VIDEO_INFO_WIDTH (in_info) *
      GST_VIDEO_INFO_WIDTH (in_info));
  gst_dicetv_create_map (filter, in_info);

  return TRUE;
}

static GstFlowReturn
gst_dicetv_transform_frame (GstVideoFilter * vfilter, GstVideoFrame * in_frame,
    GstVideoFrame * out_frame)
{
  GstDiceTV *filter = GST_DICETV (vfilter);
  guint32 *src, *dest;
  gint i, map_x, map_y, map_i, base, dx, dy, di;
  gint video_stride, g_cube_bits, g_cube_size;
  gint g_map_height, g_map_width;
  GstClockTime timestamp, stream_time;
  const guint8 *dicemap;

  timestamp = GST_BUFFER_TIMESTAMP (in_frame->buffer);
  stream_time =
      gst_segment_to_stream_time (&GST_BASE_TRANSFORM (vfilter)->segment,
      GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (filter, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (filter), stream_time);

  src = (guint32 *) GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  dest = (guint32 *) GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);
  video_stride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);

  GST_OBJECT_LOCK (filter);
  g_cube_bits = filter->g_cube_bits;
  g_cube_size = filter->g_cube_size;
  g_map_height = filter->g_map_height;
  g_map_width = filter->g_map_width;

  dicemap = filter->dicemap;
  video_stride /= 4;

  map_i = 0;
  for (map_y = 0; map_y < g_map_height; map_y++) {
    for (map_x = 0; map_x < g_map_width; map_x++) {
      base = (map_y << g_cube_bits) * video_stride + (map_x << g_cube_bits);

      switch (dicemap[map_i]) {
        case DICE_UP:
          for (dy = 0; dy < g_cube_size; dy++) {
            i = base + dy * video_stride;
            for (dx = 0; dx < g_cube_size; dx++) {
              dest[i] = src[i];
              i++;
            }
          }
          break;
        case DICE_LEFT:
          for (dy = 0; dy < g_cube_size; dy++) {
            i = base + dy * video_stride;

            for (dx = 0; dx < g_cube_size; dx++) {
              di = base + (dx * video_stride) + (g_cube_size - dy - 1);
              dest[di] = src[i];
              i++;
            }
          }
          break;
        case DICE_DOWN:
          for (dy = 0; dy < g_cube_size; dy++) {
            di = base + dy * video_stride;
            i = base + (g_cube_size - dy - 1) * video_stride + g_cube_size;
            for (dx = 0; dx < g_cube_size; dx++) {
              i--;
              dest[di] = src[i];
              di++;
            }
          }
          break;
        case DICE_RIGHT:
          for (dy = 0; dy < g_cube_size; dy++) {
            i = base + (dy * video_stride);
            for (dx = 0; dx < g_cube_size; dx++) {
              di = base + dy + (g_cube_size - dx - 1) * video_stride;
              dest[di] = src[i];
              i++;
            }
          }
          break;
        default:
          g_assert_not_reached ();
          break;
      }
      map_i++;
    }
  }
  GST_OBJECT_UNLOCK (filter);

  return GST_FLOW_OK;
}

static void
gst_dicetv_create_map (GstDiceTV * filter, GstVideoInfo * info)
{
  gint x, y, i;
  gint width, height;

  width = GST_VIDEO_INFO_WIDTH (info);
  height = GST_VIDEO_INFO_HEIGHT (info);

  if (width <= 0 || height <= 0)
    return;

  filter->g_map_height = height >> filter->g_cube_bits;
  filter->g_map_width = width >> filter->g_cube_bits;
  filter->g_cube_size = 1 << filter->g_cube_bits;

  i = 0;

  for (y = 0; y < filter->g_map_height; y++) {
    for (x = 0; x < filter->g_map_width; x++) {
      // dicemap[i] = ((i + y) & 0x3); /* Up, Down, Left or Right */
      filter->dicemap[i] = (fastrand () >> 24) & 0x03;
      i++;
    }
  }
}

static void
gst_dicetv_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstDiceTV *filter = GST_DICETV (object);

  switch (prop_id) {
    case PROP_CUBE_BITS:
      GST_OBJECT_LOCK (filter);
      filter->g_cube_bits = g_value_get_int (value);
      gst_dicetv_create_map (filter, &GST_VIDEO_FILTER (filter)->in_info);
      GST_OBJECT_UNLOCK (filter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dicetv_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstDiceTV *filter = GST_DICETV (object);

  switch (prop_id) {
    case PROP_CUBE_BITS:
      g_value_set_int (value, filter->g_cube_bits);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dicetv_finalize (GObject * object)
{
  GstDiceTV *filter = GST_DICETV (object);

  g_free (filter->dicemap);
  filter->dicemap = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_dicetv_class_init (GstDiceTVClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_dicetv_set_property;
  gobject_class->get_property = gst_dicetv_get_property;
  gobject_class->finalize = gst_dicetv_finalize;

  g_object_class_install_property (gobject_class, PROP_CUBE_BITS,
      g_param_spec_int ("square-bits", "Square Bits", "The size of the Squares",
          MIN_CUBE_BITS, MAX_CUBE_BITS, DEFAULT_CUBE_BITS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  gst_element_class_set_static_metadata (gstelement_class, "DiceTV effect",
      "Filter/Effect/Video",
      "'Dices' the screen up into many small squares",
      "Wim Taymans <wim.taymans@gmail.be>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_dicetv_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_dicetv_src_template);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_dicetv_set_info);
  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_dicetv_transform_frame);
}

static void
gst_dicetv_init (GstDiceTV * filter)
{
  filter->dicemap = NULL;
  filter->g_cube_bits = DEFAULT_CUBE_BITS;
  filter->g_cube_size = 0;
  filter->g_map_height = 0;
  filter->g_map_width = 0;
}
