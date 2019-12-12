/* GStreamer
 * Copyright (C) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#ifndef __GST_IMAGE_FREEZE_H__
#define __GST_IMAGE_FREEZE_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_IMAGE_FREEZE \
  (gst_image_freeze_get_type())
#define GST_IMAGE_FREEZE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_IMAGE_FREEZE,GstImageFreeze))
#define GST_IMAGE_FREEZE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_IMAGE_FREEZE,GstImageFreezeClass))
#define GST_IMAGE_FREEZE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_IMAGE_FREEZE,GstImageFreezeClass))
#define GST_IS_IMAGE_FREEZE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_IMAGE_FREEZE))
#define GST_IS_IMAGE_FREEZE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_IMAGE_FREEZE))

typedef struct _GstImageFreeze GstImageFreeze;
typedef struct _GstImageFreezeClass GstImageFreezeClass;

struct _GstImageFreeze
{
  GstElement parent;

  /* < private > */
  GstPad *sinkpad;
  GstPad *srcpad;

  GMutex lock;
  GstBuffer *buffer;
  gint fps_n, fps_d;

  GstSegment segment;
  gboolean need_segment;
  guint seqnum;

  gint num_buffers;
  gint num_buffers_left;

  guint64 offset;

  /* TRUE if currently doing a flushing seek, protected
   * by srcpad's stream lock */
  gint seeking;
};

struct _GstImageFreezeClass
{
  GstElementClass parent_class;
};

GType gst_image_freeze_get_type (void);

G_END_DECLS

#endif /* __GST_IMAGE_FREEZE_H__ */
