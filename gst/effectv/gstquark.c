/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * EffecTV:
 * Copyright (C) 2001-2002 FUKUCHI Kentarou
 *
 * QuarkTV - motion disolver.
 *
 *  EffecTV is free software. This library is free software;
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
 * SECTION:element-quarktv
 * @title: quarktv
 *
 * QuarkTV disolves moving objects. It picks up pixels from
 * the last frames randomly.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! quarktv ! videoconvert ! autovideosink
 * ]| This pipeline shows the effect of quarktv on a test stream.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "gstquark.h"
#include "gsteffectv.h"

/* number of frames of time-buffer. It should be as a configurable parameter */
/* This number also must be 2^n just for the speed. */
#define PLANES 16

enum
{
  PROP_0,
  PROP_PLANES
};

#define gst_quarktv_parent_class parent_class
G_DEFINE_TYPE (GstQuarkTV, gst_quarktv, GST_TYPE_VIDEO_FILTER);

static void gst_quarktv_planetable_clear (GstQuarkTV * filter);

static GstStaticPadTemplate gst_quarktv_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ xRGB, xBGR, BGRx, RGBx }"))
    );

static GstStaticPadTemplate gst_quarktv_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ xRGB, xBGR, BGRx, RGBx }"))
    );

static gboolean
gst_quarktv_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstQuarkTV *filter = GST_QUARKTV (vfilter);
  gint width, height;

  width = GST_VIDEO_INFO_WIDTH (in_info);
  height = GST_VIDEO_INFO_HEIGHT (in_info);

  gst_quarktv_planetable_clear (filter);
  filter->area = width * height;

  return TRUE;
}

static GstFlowReturn
gst_quarktv_transform_frame (GstVideoFilter * vfilter, GstVideoFrame * in_frame,
    GstVideoFrame * out_frame)
{
  GstQuarkTV *filter = GST_QUARKTV (vfilter);
  gint area;
  guint32 *src, *dest;
  GstClockTime timestamp;
  GstBuffer **planetable;
  gint planes, current_plane;

  timestamp = GST_BUFFER_TIMESTAMP (in_frame->buffer);
  timestamp =
      gst_segment_to_stream_time (&GST_BASE_TRANSFORM (vfilter)->segment,
      GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (filter, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (timestamp))
    gst_object_sync_values (GST_OBJECT (filter), timestamp);

  if (G_UNLIKELY (filter->planetable == NULL))
    return GST_FLOW_FLUSHING;

  src = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  dest = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

  GST_OBJECT_LOCK (filter);
  area = filter->area;
  planetable = filter->planetable;
  planes = filter->planes;
  current_plane = filter->current_plane;

  if (planetable[current_plane])
    gst_buffer_unref (planetable[current_plane]);
  planetable[current_plane] = gst_buffer_ref (in_frame->buffer);

  /* For each pixel */
  while (--area) {
    GstBuffer *rand;

    /* pick a random buffer */
    rand = planetable[(current_plane + (fastrand () >> 24)) % planes];

    /* Copy the pixel from the random buffer to dest, FIXME, slow */
    if (rand)
      gst_buffer_extract (rand, area * 4, &dest[area], 4);
    else
      dest[area] = src[area];
  }

  filter->current_plane--;
  if (filter->current_plane < 0)
    filter->current_plane = planes - 1;
  GST_OBJECT_UNLOCK (filter);

  return GST_FLOW_OK;
}

static void
gst_quarktv_planetable_clear (GstQuarkTV * filter)
{
  gint i;

  if (filter->planetable == NULL)
    return;

  for (i = 0; i < filter->planes; i++) {
    if (GST_IS_BUFFER (filter->planetable[i])) {
      gst_buffer_unref (filter->planetable[i]);
    }
    filter->planetable[i] = NULL;
  }
}

static gboolean
gst_quarktv_start (GstBaseTransform * trans)
{
  GstQuarkTV *filter = GST_QUARKTV (trans);

  if (filter->planetable) {
    gst_quarktv_planetable_clear (filter);
    g_free (filter->planetable);
  }
  filter->planetable =
      (GstBuffer **) g_malloc0 (filter->planes * sizeof (GstBuffer *));

  return TRUE;
}

static void
gst_quarktv_finalize (GObject * object)
{
  GstQuarkTV *filter = GST_QUARKTV (object);

  if (filter->planetable) {
    gst_quarktv_planetable_clear (filter);
    g_free (filter->planetable);
    filter->planetable = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_quarktv_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstQuarkTV *filter = GST_QUARKTV (object);

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_PLANES:
    {
      gint new_n_planes = g_value_get_int (value);
      GstBuffer **new_planetable;
      gint i;

      /* If the number of planes changed, copy across any existing planes */
      if (new_n_planes != filter->planes) {
        new_planetable =
            (GstBuffer **) g_malloc0 (new_n_planes * sizeof (GstBuffer *));

        if (filter->planetable) {
          for (i = 0; (i < new_n_planes) && (i < filter->planes); i++) {
            new_planetable[i] = filter->planetable[i];
          }
          for (; i < filter->planes; i++) {
            if (filter->planetable[i])
              gst_buffer_unref (filter->planetable[i]);
          }
          g_free (filter->planetable);
        }

        filter->planetable = new_planetable;
        filter->planes = new_n_planes;
        filter->current_plane = filter->planes - 1;
      }
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static void
gst_quarktv_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstQuarkTV *filter = GST_QUARKTV (object);

  switch (prop_id) {
    case PROP_PLANES:
      g_value_set_int (value, filter->planes);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_quarktv_class_init (GstQuarkTVClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_quarktv_set_property;
  gobject_class->get_property = gst_quarktv_get_property;

  gobject_class->finalize = gst_quarktv_finalize;

  g_object_class_install_property (gobject_class, PROP_PLANES,
      g_param_spec_int ("planes", "Planes",
          "Number of planes", 1, 64, PLANES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  gst_element_class_set_static_metadata (gstelement_class, "QuarkTV effect",
      "Filter/Effect/Video",
      "Motion dissolver", "FUKUCHI, Kentarou <fukuchi@users.sourceforge.net>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_quarktv_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_quarktv_src_template);

  trans_class->start = GST_DEBUG_FUNCPTR (gst_quarktv_start);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_quarktv_set_info);
  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_quarktv_transform_frame);
}

static void
gst_quarktv_init (GstQuarkTV * filter)
{
  filter->planes = PLANES;
  filter->current_plane = filter->planes - 1;
}
