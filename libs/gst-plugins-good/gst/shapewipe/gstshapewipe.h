/* GStreamer
 * Copyright (C) 2009,2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#ifndef __GST_SHAPE_WIPE_H__
#define __GST_SHAPE_WIPE_H__

#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_SHAPE_WIPE \
  (gst_shape_wipe_get_type())
#define GST_SHAPE_WIPE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SHAPE_WIPE,GstShapeWipe))
#define GST_SHAPE_WIPE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SHAPE_WIPE,GstShapeWipeClass))
#define GST_SHAPE_WIPE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_SHAPE_WIPE,GstShapeWipeClass))
#define GST_IS_SHAPE_WIPE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SHAPE_WIPE))
#define GST_IS_SHAPE_WIPE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SHAPE_WIPE))

typedef struct _GstShapeWipe GstShapeWipe;
typedef struct _GstShapeWipeClass GstShapeWipeClass;

struct _GstShapeWipe
{
  GstElement parent;

  /* private */
  GstPad *video_sinkpad;
  GstPad *mask_sinkpad;

  GstPad *srcpad;

  GstSegment segment;

  GstBuffer *mask;
  gfloat mask_position;
  gfloat mask_border;
  GMutex mask_mutex;
  GCond mask_cond;
  gint mask_bpp;

  GstVideoInfo vinfo;
  GstVideoInfo minfo;

  gboolean shutdown;

  gdouble proportion;
  GstClockTime earliest_time;
  GstClockTime frame_duration;
};

struct _GstShapeWipeClass
{
  GstElementClass parent_class;
};

GType gst_shape_wipe_get_type (void);

G_END_DECLS

#endif /* __GST_SHAPE_WIPE_H__ */
