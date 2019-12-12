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

#ifndef __GST_VIDEO_FLIP_H__
#define __GST_VIDEO_FLIP_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

/**
 * GstVideoFlipMethod:
 * @GST_VIDEO_FLIP_METHOD_IDENTITY: Identity (no rotation)
 * @GST_VIDEO_FLIP_METHOD_90R: Rotate clockwise 90 degrees
 * @GST_VIDEO_FLIP_METHOD_180: Rotate 180 degrees
 * @GST_VIDEO_FLIP_METHOD_90L: Rotate counter-clockwise 90 degrees
 * @GST_VIDEO_FLIP_METHOD_HORIZ: Flip horizontally
 * @GST_VIDEO_FLIP_METHOD_VERT: Flip vertically
 * @GST_VIDEO_FLIP_METHOD_TRANS: Flip across upper left/lower right diagonal
 * @GST_VIDEO_FLIP_METHOD_OTHER: Flip across upper right/lower left diagonal
 * @GST_VIDEO_FLIP_METHOD_AUTO: Select flip method based on image-orientation tag
 *
 * The different flip methods.
 */
typedef enum {
  GST_VIDEO_FLIP_METHOD_IDENTITY,
  GST_VIDEO_FLIP_METHOD_90R,
  GST_VIDEO_FLIP_METHOD_180,
  GST_VIDEO_FLIP_METHOD_90L,
  GST_VIDEO_FLIP_METHOD_HORIZ,
  GST_VIDEO_FLIP_METHOD_VERT,
  GST_VIDEO_FLIP_METHOD_TRANS,
  GST_VIDEO_FLIP_METHOD_OTHER,
  GST_VIDEO_FLIP_METHOD_AUTO
} GstVideoFlipMethod;

#define GST_TYPE_VIDEO_FLIP \
  (gst_video_flip_get_type())
#define GST_VIDEO_FLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIDEO_FLIP,GstVideoFlip))
#define GST_VIDEO_FLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIDEO_FLIP,GstVideoFlipClass))
#define GST_IS_VIDEO_FLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIDEO_FLIP))
#define GST_IS_VIDEO_FLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIDEO_FLIP))

typedef struct _GstVideoFlip GstVideoFlip;
typedef struct _GstVideoFlipClass GstVideoFlipClass;

/**
 * GstVideoFlip:
 *
 * Opaque datastructure.
 */
struct _GstVideoFlip {
  GstVideoFilter videofilter;

  /* < private > */
  GstVideoOrientationMethod method;
  GstVideoOrientationMethod tag_method;
  GstVideoOrientationMethod active_method;
  void (*process) (GstVideoFlip *videoflip, GstVideoFrame *dest, const GstVideoFrame *src);
};

struct _GstVideoFlipClass {
  GstVideoFilterClass parent_class;
};

GType gst_video_flip_get_type (void);

G_END_DECLS

#endif /* __GST_VIDEO_FLIP_H__ */
