/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 *
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001 FUKUCHI Kentarou
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

#ifndef __GST_WARP_H__
#define __GST_WARP_H__

#include <gst/gst.h>

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_WARPTV \
  (gst_warptv_get_type())
#define GST_WARPTV(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_WARPTV,GstWarpTV))
#define GST_WARPTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_WARPTV,GstWarpTVClass))
#define GST_IS_WARPTV(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_WARPTV))
#define GST_IS_WARPTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_WARPTV))

typedef struct _GstWarpTV GstWarpTV;
typedef struct _GstWarpTVClass GstWarpTVClass;

struct _GstWarpTV
{
  GstVideoFilter videofilter;

  /* < private > */
  gint32 *disttable;
  gint32 ctable[1024];
  gint tval;
};

struct _GstWarpTVClass
{
  GstVideoFilterClass parent_class;
};

GType gst_warptv_get_type (void);

G_END_DECLS

#endif /* __GST_WARP_H__ */
