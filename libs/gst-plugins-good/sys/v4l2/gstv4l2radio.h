/* GStreamer v4l2 radio tuner element
 * Copyright (C) 2010, 2011 Alexey Chernov <4ernov@gmail.com>
 *
 * gstv4l2radio.h - V4L2 radio tuner element
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

#ifndef __GST_V4L2RADIO_H__
#define __GST_V4L2RADIO_H__

#include "gstv4l2object.h"

G_BEGIN_DECLS

#define GST_TYPE_V4L2RADIO \
  (gst_v4l2radio_get_type())
#define GST_V4L2RADIO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_V4L2RADIO,GstV4l2Radio))
#define GST_V4L2RADIO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_V4L2RADIO,GstV4l2RadioClass))
#define GST_IS_V4L2RADIO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_V4L2RADIO))
#define GST_IS_V4L2RADIO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_V4L2RADIO))

typedef struct _GstV4l2Radio      GstV4l2Radio;
typedef struct _GstV4l2RadioClass GstV4l2RadioClass;

/**
 * GstV4l2Radio:
 *
 * Opaque video4linux2 radio tuner element
 */
struct _GstV4l2Radio
{
  GstElement element;

  /*< private >*/
  GstV4l2Object * v4l2object;
};

struct _GstV4l2RadioClass
{
  GstElementClass parent_class;

  GList *v4l2_class_devices;
};

GType gst_v4l2radio_get_type (void);

G_END_DECLS

#endif /* __GST_V4L2RADIO_H__ */
