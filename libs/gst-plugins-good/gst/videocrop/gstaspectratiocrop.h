/* GStreamer video frame cropping to aspect-ratio
 * Copyright (C) 2009 Thijs Vermeir <thijsvermeir@gmail.com>
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

#ifndef __GST_ASPECT_RATIO_CROP_H__
#define __GST_ASPECT_RATIO_CROP_H__

#include <gst/gstbin.h>

G_BEGIN_DECLS

#define GST_TYPE_ASPECT_RATIO_CROP \
  (gst_aspect_ratio_crop_get_type())
#define GST_ASPECT_RATIO_CROP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ASPECT_RATIO_CROP,GstAspectRatioCrop))
#define GST_ASPECT_RATIO_CROP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ASPECT_RATIO_CROP,GstAspectRatioCropClass))
#define GST_IS_ASPECT_RATIO_CROP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ASPECT_RATIO_CROP))
#define GST_IS_ASPECT_RATIO_CROP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ASPECT_RATIO_CROP))

typedef struct _GstAspectRatioCrop GstAspectRatioCrop;
typedef struct _GstAspectRatioCropClass GstAspectRatioCropClass;

struct _GstAspectRatioCrop
{
  GstBin parent;

  /* our videocrop element */
  GstElement *videocrop;

  GstPad *sink;

  /* target aspect ratio */
  gint ar_num; /* if < 1 then don't change ar */
  gint ar_denom;

  GstCaps *renegotiation_caps;

  GMutex crop_lock;
};

struct _GstAspectRatioCropClass
{
  GstBinClass parent_class;
};

GType gst_aspect_ratio_crop_get_type (void);

G_END_DECLS

#endif /* __GST_ASPECT_RATIO_CROP_H__ */

