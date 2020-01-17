/* GStreamer alphacolor element
 * Copyright (C) 2005 Wim Taymans <wim@fluendo.com>
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

#ifndef _GST_ALPHA_COLOR_H_
#define _GST_ALPHA_COLOR_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

#define GST_TYPE_ALPHA_COLOR \
  (gst_alpha_color_get_type())
#define GST_ALPHA_COLOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ALPHA_COLOR,GstAlphaColor))
#define GST_ALPHA_COLOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ALPHA_COLOR,GstAlphaColorClass))
#define GST_IS_ALPHA_COLOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ALPHA_COLOR))
#define GST_IS_ALPHA_COLOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ALPHA_COLOR))

typedef struct _GstAlphaColor GstAlphaColor;
typedef struct _GstAlphaColorClass GstAlphaColorClass;

struct _GstAlphaColor
{
  GstVideoFilter parent;

  /*< private >*/
  void (*process) (GstVideoFrame * frame, const gint * matrix);

  const gint *matrix;
};

struct _GstAlphaColorClass
{
  GstVideoFilterClass parent_class;
};

GType gst_alpha_color_get_type (void);

#endif /* _GST_ALPHA_COLOR_H_ */
