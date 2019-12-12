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


#ifndef __GST_SMOKEDEC_H__
#define __GST_SMOKEDEC_H__


#include <gst/gst.h>
#include "smokecodec.h"

G_BEGIN_DECLS

#define GST_TYPE_SMOKEDEC \
  (gst_smokedec_get_type())
#define GST_SMOKEDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SMOKEDEC,GstSmokeDec))
#define GST_SMOKEDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SMOKEDEC,GstSmokeDecClass))
#define GST_IS_SMOKEDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SMOKEDEC))
#define GST_IS_SMOKEDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SMOKEDEC))

typedef struct _GstSmokeDec GstSmokeDec;
typedef struct _GstSmokeDecClass GstSmokeDecClass;

struct _GstSmokeDec {
  GstElement element;

  /* pads */
  GstPad *sinkpad,*srcpad;

  /* video state */
  gint format;
  gint width;
  gint height;
  gint fps_num;
  gint fps_denom;
  GstClockTime next_time;

  SmokeCodecInfo *info;

  gint threshold;
  gint quality;
  gint smoothing;

  gboolean need_keyframe;
};

struct _GstSmokeDecClass {
  GstElementClass parent_class;
};

GType gst_smokedec_get_type(void);

G_END_DECLS

#endif /* __GST_SMOKEDEC_H__ */
