/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include "gstvideomedian.h"

static GstStaticPadTemplate video_median_src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ I420, YV12 }"))
    );

static GstStaticPadTemplate video_median_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ I420, YV12 }"))
    );


/* Median signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

#define DEFAULT_FILTERSIZE   5
#define DEFAULT_LUM_ONLY     TRUE
enum
{
  PROP_0,
  PROP_FILTERSIZE,
  PROP_LUM_ONLY
};

#define GST_TYPE_VIDEO_MEDIAN_SIZE (gst_video_median_size_get_type())

static const GEnumValue video_median_sizes[] = {
  {GST_VIDEO_MEDIAN_SIZE_5, "Median of 5 neighbour pixels", "5"},
  {GST_VIDEO_MEDIAN_SIZE_9, "Median of 9 neighbour pixels", "9"},
  {0, NULL, NULL},
};

static GType
gst_video_median_size_get_type (void)
{
  static GType video_median_size_type = 0;

  if (!video_median_size_type) {
    video_median_size_type = g_enum_register_static ("GstVideoMedianSize",
        video_median_sizes);
  }
  return video_median_size_type;
}

#define gst_video_median_parent_class parent_class
G_DEFINE_TYPE (GstVideoMedian, gst_video_median, GST_TYPE_VIDEO_FILTER);

static GstFlowReturn gst_video_median_transform_frame (GstVideoFilter * filter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame);

static void gst_video_median_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_video_median_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void
gst_video_median_class_init (GstVideoMedianClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstVideoFilterClass *vfilter_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_video_median_set_property;
  gobject_class->get_property = gst_video_median_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_FILTERSIZE,
      g_param_spec_enum ("filtersize", "Filtersize", "The size of the filter",
          GST_TYPE_VIDEO_MEDIAN_SIZE, DEFAULT_FILTERSIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_LUM_ONLY,
      g_param_spec_boolean ("lum-only", "Lum Only", "Only apply filter on "
          "luminance", DEFAULT_LUM_ONLY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class,
      &video_median_sink_factory);
  gst_element_class_add_static_pad_template (gstelement_class,
      &video_median_src_factory);
  gst_element_class_set_static_metadata (gstelement_class, "Median effect",
      "Filter/Effect/Video", "Apply a median filter to an image",
      "Wim Taymans <wim.taymans@gmail.com>");

  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_video_median_transform_frame);
}

void
gst_video_median_init (GstVideoMedian * median)
{
  median->filtersize = DEFAULT_FILTERSIZE;
  median->lum_only = DEFAULT_LUM_ONLY;
}

#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { unsigned char temp=(a);(a)=(b);(b)=temp; }

static void
median_5 (guint8 * dest, gint dstride, const guint8 * src, gint sstride,
    gint width, gint height)
{
  unsigned char p[5];
  int i, j, k;

  /* copy the top and bottom rows into the result array */
  for (i = 0; i < width; i++) {
    dest[i] = src[i];
    dest[(height - 1) * dstride + i] = src[(height - 1) * sstride + i];
  }

  /* process the interior pixels */
  for (k = 2; k < height; k++) {
    dest += dstride;
    src += sstride;

    dest[0] = src[0];
    for (j = 2, i = 1; j < width; j++, i++) {
      p[0] = src[i - sstride];
      p[1] = src[i - 1];
      p[2] = src[i];
      p[3] = src[i + 1];
      p[4] = src[i + sstride];
      PIX_SORT (p[0], p[1]);
      PIX_SORT (p[3], p[4]);
      PIX_SORT (p[0], p[3]);
      PIX_SORT (p[1], p[4]);
      PIX_SORT (p[1], p[2]);
      PIX_SORT (p[2], p[3]);
      PIX_SORT (p[1], p[2]);
      dest[i] = p[2];
    }
    dest[i] = src[i];
  }
}

static void
median_9 (guint8 * dest, gint dstride, const guint8 * src, gint sstride,
    gint width, gint height)
{
  unsigned char p[9];
  int i, j, k;

  /*copy the top and bottom rows into the result array */
  for (i = 0; i < width; i++) {
    dest[i] = src[i];
    dest[(height - 1) * dstride + i] = src[(height - 1) * sstride + i];
  }
  /* process the interior pixels */
  for (k = 2; k < height; k++) {
    dest += dstride;
    src += sstride;

    dest[0] = src[0];
    for (j = 2, i = 1; j < width; j++, i++) {
      p[0] = src[i - sstride - 1];
      p[1] = src[i - sstride];
      p[2] = src[i - sstride + 1];
      p[3] = src[i - 1];
      p[4] = src[i];
      p[5] = src[i + 1];
      p[6] = src[i + sstride - 1];
      p[7] = src[i + sstride];
      p[8] = src[i + sstride + 1];
      PIX_SORT (p[1], p[2]);
      PIX_SORT (p[4], p[5]);
      PIX_SORT (p[7], p[8]);
      PIX_SORT (p[0], p[1]);
      PIX_SORT (p[3], p[4]);
      PIX_SORT (p[6], p[7]);
      PIX_SORT (p[1], p[2]);
      PIX_SORT (p[4], p[5]);
      PIX_SORT (p[7], p[8]);
      PIX_SORT (p[0], p[3]);
      PIX_SORT (p[5], p[8]);
      PIX_SORT (p[4], p[7]);
      PIX_SORT (p[3], p[6]);
      PIX_SORT (p[1], p[4]);
      PIX_SORT (p[2], p[5]);
      PIX_SORT (p[4], p[7]);
      PIX_SORT (p[4], p[2]);
      PIX_SORT (p[6], p[4]);
      PIX_SORT (p[4], p[2]);
      dest[i] = p[4];
    }
    dest[i] = src[i];
  }
}

static GstFlowReturn
gst_video_median_transform_frame (GstVideoFilter * filter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
  GstVideoMedian *median = GST_VIDEO_MEDIAN (filter);

  if (median->filtersize == 5) {
    median_5 (GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0),
        GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0),
        GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0),
        GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0),
        GST_VIDEO_FRAME_WIDTH (in_frame), GST_VIDEO_FRAME_HEIGHT (in_frame));

    if (median->lum_only) {
      gst_video_frame_copy_plane (out_frame, in_frame, 1);
      gst_video_frame_copy_plane (out_frame, in_frame, 2);
    } else {
      median_5 (GST_VIDEO_FRAME_PLANE_DATA (out_frame, 1),
          GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 1),
          GST_VIDEO_FRAME_PLANE_DATA (in_frame, 1),
          GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 1),
          GST_VIDEO_FRAME_WIDTH (in_frame) / 2,
          GST_VIDEO_FRAME_HEIGHT (in_frame) / 2);
      median_5 (GST_VIDEO_FRAME_PLANE_DATA (out_frame, 2),
          GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 2),
          GST_VIDEO_FRAME_PLANE_DATA (in_frame, 2),
          GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 2),
          GST_VIDEO_FRAME_WIDTH (in_frame) / 2,
          GST_VIDEO_FRAME_HEIGHT (in_frame) / 2);
    }
  } else {
    median_9 (GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0),
        GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0),
        GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0),
        GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0),
        GST_VIDEO_FRAME_WIDTH (in_frame), GST_VIDEO_FRAME_HEIGHT (in_frame));

    if (median->lum_only) {
      gst_video_frame_copy_plane (out_frame, in_frame, 1);
      gst_video_frame_copy_plane (out_frame, in_frame, 2);
    } else {
      median_9 (GST_VIDEO_FRAME_PLANE_DATA (out_frame, 1),
          GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 1),
          GST_VIDEO_FRAME_PLANE_DATA (in_frame, 1),
          GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 1),
          GST_VIDEO_FRAME_WIDTH (in_frame) / 2,
          GST_VIDEO_FRAME_HEIGHT (in_frame) / 2);
      median_9 (GST_VIDEO_FRAME_PLANE_DATA (out_frame, 2),
          GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 2),
          GST_VIDEO_FRAME_PLANE_DATA (in_frame, 2),
          GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 2),
          GST_VIDEO_FRAME_WIDTH (in_frame) / 2,
          GST_VIDEO_FRAME_HEIGHT (in_frame) / 2);
    }
  }

  return GST_FLOW_OK;
}

static void
gst_video_median_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVideoMedian *median;

  median = GST_VIDEO_MEDIAN (object);

  switch (prop_id) {
    case PROP_FILTERSIZE:
      median->filtersize = g_value_get_enum (value);
      break;
    case PROP_LUM_ONLY:
      median->lum_only = g_value_get_boolean (value);
      break;
    default:
      break;
  }
}

static void
gst_video_median_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstVideoMedian *median;

  median = GST_VIDEO_MEDIAN (object);

  switch (prop_id) {
    case PROP_FILTERSIZE:
      g_value_set_enum (value, median->filtersize);
      break;
    case PROP_LUM_ONLY:
      g_value_set_boolean (value, median->lum_only);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
