/* GStreamer video frame cropping
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
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

#ifndef __GST_VIDEO_CROP_H__
#define __GST_VIDEO_CROP_H__

#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_CROP \
  (gst_video_crop_get_type())
#define GST_VIDEO_CROP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIDEO_CROP,GstVideoCrop))
#define GST_VIDEO_CROP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIDEO_CROP,GstVideoCropClass))
#define GST_IS_VIDEO_CROP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIDEO_CROP))
#define GST_IS_VIDEO_CROP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIDEO_CROP))

typedef enum {
  VIDEO_CROP_PIXEL_FORMAT_PACKED_SIMPLE = 0,  /* RGBx, AYUV */
  VIDEO_CROP_PIXEL_FORMAT_PACKED_COMPLEX,     /* UYVY, YVYU */
  VIDEO_CROP_PIXEL_FORMAT_PLANAR,             /* I420, YV12 */
  VIDEO_CROP_PIXEL_FORMAT_SEMI_PLANAR         /* NV12, NV21 */
} VideoCropPixelFormat;

typedef struct _GstVideoCropImageDetails GstVideoCropImageDetails;

typedef struct _GstVideoCrop GstVideoCrop;
typedef struct _GstVideoCropClass GstVideoCropClass;

struct _GstVideoCrop
{
  GstVideoFilter parent;

  /*< private >*/
  gint prop_left;
  gint prop_right;
  gint prop_top;
  gint prop_bottom;
  gboolean need_update;

  GstVideoInfo in_info;
  GstVideoInfo out_info;

  gint crop_left;
  gint crop_right;
  gint crop_top;
  gint crop_bottom;

  VideoCropPixelFormat  packing;
  gint macro_y_off;
};

struct _GstVideoCropClass
{
  GstVideoFilterClass parent_class;
};

GType gst_video_crop_get_type (void);

G_END_DECLS

#endif /* __GST_VIDEO_CROP_H__ */

