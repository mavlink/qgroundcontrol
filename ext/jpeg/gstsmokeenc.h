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


#ifndef __GST_SMOKEENC_H__
#define __GST_SMOKEENC_H__


#include <gst/gst.h>
#include "smokecodec.h"

G_BEGIN_DECLS

#define GST_TYPE_SMOKEENC \
  (gst_smokeenc_get_type())
#define GST_SMOKEENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SMOKEENC,GstSmokeEnc))
#define GST_SMOKEENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SMOKEENC,GstSmokeEncClass))
#define GST_IS_SMOKEENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SMOKEENC))
#define GST_IS_SMOKEENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SMOKEENC))

typedef struct _GstSmokeEnc GstSmokeEnc;
typedef struct _GstSmokeEncClass GstSmokeEncClass;

struct _GstSmokeEnc {
  GstElement element;

  /* pads */
  GstPad *sinkpad,*srcpad;

  /* video state */
  gint format;
  gint width;
  gint height;
  gint frame;
  gint keyframe;
  gint fps_num, fps_denom;

  SmokeCodecInfo *info;

  gint threshold;
  gint min_quality;
  gint max_quality;

  gboolean need_header;
};

struct _GstSmokeEncClass {
  GstElementClass parent_class;
};

GType gst_smokeenc_get_type(void);

G_END_DECLS

#endif /* __GST_SMOKEENC_H__ */
