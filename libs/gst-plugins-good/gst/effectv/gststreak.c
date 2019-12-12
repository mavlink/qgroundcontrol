/* GStreamer
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * StreakTV - afterimage effector.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * This combines the StreakTV and BaltanTV effects, which are
 * very similar. BaltanTV is used if the feedback property is set
 * to TRUE, otherwise StreakTV is used.
 *
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
 * SECTION:element-streaktv
 * @title: streaktv
 *
 * StreakTV makes after images of moving objects.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! streaktv ! videoconvert ! autovideosink
 * ]| This pipeline shows the effect of streaktv on a test stream.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "gststreak.h"
#include "gsteffectv.h"

#define DEFAULT_FEEDBACK FALSE

enum
{
  PROP_0,
  PROP_FEEDBACK
};

#define gst_streaktv_parent_class parent_class
G_DEFINE_TYPE (GstStreakTV, gst_streaktv, GST_TYPE_VIDEO_FILTER);

static GstStaticPadTemplate gst_streaktv_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ BGRx, RGBx, xBGR, xRGB }"))
    );

static GstStaticPadTemplate gst_streaktv_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ BGRx, RGBx, xBGR, xRGB }"))
    );


static GstFlowReturn
gst_streaktv_transform_frame (GstVideoFilter * vfilter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
  GstStreakTV *filter = GST_STREAKTV (vfilter);
  guint32 *src, *dest;
  gint i, cf;
  gint video_area, width, height;
  guint32 **planetable = filter->planetable;
  gint plane = filter->plane;
  guint stride_mask, stride_shift, stride;

  src = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  dest = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

  width = GST_VIDEO_FRAME_WIDTH (in_frame);
  height = GST_VIDEO_FRAME_HEIGHT (in_frame);

  video_area = width * height;

  GST_OBJECT_LOCK (filter);
  if (filter->feedback) {
    stride_mask = 0xfcfcfcfc;
    stride = 8;
    stride_shift = 2;
  } else {
    stride_mask = 0xf8f8f8f8;
    stride = 4;
    stride_shift = 3;
  }

  for (i = 0; i < video_area; i++) {
    planetable[plane][i] = (src[i] & stride_mask) >> stride_shift;
  }

  cf = plane & (stride - 1);
  if (filter->feedback) {
    for (i = 0; i < video_area; i++) {
      dest[i] = planetable[cf][i]
          + planetable[cf + stride][i]
          + planetable[cf + stride * 2][i]
          + planetable[cf + stride * 3][i];
      planetable[plane][i] = (dest[i] & stride_mask) >> stride_shift;
    }
  } else {
    for (i = 0; i < video_area; i++) {
      dest[i] = planetable[cf][i]
          + planetable[cf + stride][i]
          + planetable[cf + stride * 2][i]
          + planetable[cf + stride * 3][i]
          + planetable[cf + stride * 4][i]
          + planetable[cf + stride * 5][i]
          + planetable[cf + stride * 6][i]
          + planetable[cf + stride * 7][i];
    }
  }

  plane++;
  filter->plane = plane & (PLANES - 1);
  GST_OBJECT_UNLOCK (filter);

  return GST_FLOW_OK;
}

static gboolean
gst_streaktv_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstStreakTV *filter = GST_STREAKTV (vfilter);
  gint i, width, height;

  width = GST_VIDEO_INFO_WIDTH (in_info);
  height = GST_VIDEO_INFO_HEIGHT (in_info);

  g_free (filter->planebuffer);

  filter->planebuffer = g_new0 (guint32, width * height * 4 * PLANES);

  for (i = 0; i < PLANES; i++)
    filter->planetable[i] = &filter->planebuffer[width * height * i];

  return TRUE;
}

static gboolean
gst_streaktv_start (GstBaseTransform * trans)
{
  GstStreakTV *filter = GST_STREAKTV (trans);

  filter->plane = 0;

  return TRUE;
}

static void
gst_streaktv_finalize (GObject * object)
{
  GstStreakTV *filter = GST_STREAKTV (object);

  if (filter->planebuffer) {
    g_free (filter->planebuffer);
    filter->planebuffer = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_streaktv_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstStreakTV *filter = GST_STREAKTV (object);

  switch (prop_id) {
    case PROP_FEEDBACK:
      if (G_UNLIKELY (GST_STATE (filter) >= GST_STATE_PAUSED)) {
        g_warning ("Changing the \"feedback\" property only allowed "
            "in state < PLAYING");
        return;
      }

      filter->feedback = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_streaktv_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstStreakTV *filter = GST_STREAKTV (object);

  switch (prop_id) {
    case PROP_FEEDBACK:
      g_value_set_boolean (value, filter->feedback);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_streaktv_class_init (GstStreakTVClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_streaktv_set_property;
  gobject_class->get_property = gst_streaktv_get_property;

  gobject_class->finalize = gst_streaktv_finalize;

  g_object_class_install_property (gobject_class, PROP_FEEDBACK,
      g_param_spec_boolean ("feedback", "Feedback",
          "Feedback", DEFAULT_FEEDBACK,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class, "StreakTV effect",
      "Filter/Effect/Video",
      "StreakTV makes after images of moving objects",
      "FUKUCHI, Kentarou <fukuchi@users.sourceforge.net>, "
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_streaktv_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_streaktv_src_template);

  trans_class->start = GST_DEBUG_FUNCPTR (gst_streaktv_start);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_streaktv_set_info);
  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_streaktv_transform_frame);
}

static void
gst_streaktv_init (GstStreakTV * filter)
{
  filter->feedback = DEFAULT_FEEDBACK;
}
