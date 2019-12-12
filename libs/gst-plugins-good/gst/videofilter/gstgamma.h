/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 * Copyright (C) 2003 Arwed v. Merkatz <v.merkatz@gmx.net>
 * Copyright (C) 2006 Mark Nauwelaerts <manauw@skynet.be>
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


#ifndef __GST_VIDEO_GAMMA_H__
#define __GST_VIDEO_GAMMA_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_GAMMA \
  (gst_gamma_get_type())
#define GST_GAMMA(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GAMMA,GstGamma))
#define GST_GAMMA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GAMMA,GstGammaClass))
#define GST_IS_GAMMA(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GAMMA))
#define GST_IS_GAMMA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GAMMA))

typedef struct _GstGamma GstGamma;
typedef struct _GstGammaClass GstGammaClass;

/**
 * GstGamma:
 *
 * Opaque data structure.
 */
struct _GstGamma
{
  GstVideoFilter videofilter;

  /* < private > */
  /* properties */
  gdouble gamma;

  /* tables */
  guint8 gamma_table[256];

  void (*process) (GstGamma *gamma, GstVideoFrame *frame);
};

struct _GstGammaClass
{
  GstVideoFilterClass parent_class;
};

GType gst_gamma_get_type(void);

G_END_DECLS

#endif /* __GST_VIDEO_GAMMA_H__ */
