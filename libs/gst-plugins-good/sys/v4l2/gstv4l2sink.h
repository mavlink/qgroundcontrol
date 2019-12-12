/* GStreamer
 *
 * Copyright (C) 2009 Texas Instruments, Inc - http://www.ti.com/
 *
 * Description: V4L2 sink element
 *  Created on: Jul 2, 2009
 *      Author: Rob Clark <rob@ti.com>
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

#ifndef __GSTV4L2SINK_H__
#define __GSTV4L2SINK_H__

#include <gst/video/gstvideosink.h>
#include <gstv4l2object.h>
#include <gstv4l2bufferpool.h>

G_BEGIN_DECLS

#define GST_TYPE_V4L2SINK \
  (gst_v4l2sink_get_type())
#define GST_V4L2SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_V4L2SINK, GstV4l2Sink))
#define GST_V4L2SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_V4L2SINK, GstV4l2SinkClass))
#define GST_IS_V4L2SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_V4L2SINK))
#define GST_IS_V4L2SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_V4L2SINK))

typedef struct _GstV4l2Sink GstV4l2Sink;
typedef struct _GstV4l2SinkClass GstV4l2SinkClass;


struct _GstV4l2Sink {
  GstVideoSink videosink;

  /*< private >*/
  GstV4l2Object * v4l2object;

  gint video_width, video_height;      /* original (unscaled) video w/h */

  /*
   * field to store requested overlay and crop top/left/width/height props:
   * note, could maybe be combined with 'vwin' field in GstV4l2Object?
   */
  struct v4l2_rect overlay, crop;

  /*
   * bitmask to track which overlay and crop fields user has requested by
   * setting properties:
   */
  guint8 overlay_fields_set, crop_fields_set;
};

struct _GstV4l2SinkClass {
  GstVideoSinkClass parent_class;

  GList *v4l2_class_devices;
};

GType gst_v4l2sink_get_type(void);

G_END_DECLS


#endif /* __GSTV4L2SINK_H__ */
