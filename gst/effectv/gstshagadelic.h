/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *
 * EffecTV:
 * Copyright (C) 2001 FUKUCHI Kentarou
 *
 * Inspired by Adrian Likin's script for the GIMP.
 * EffecTV is free software.  This library is free software;
 * you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
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

#ifndef __GST_SHAGADELIC_H__
#define __GST_SHAGADELIC_H__

#include <gst/gst.h>

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_SHAGADELICTV \
  (gst_shagadelictv_get_type())
#define GST_SHAGADELICTV(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SHAGADELICTV,GstShagadelicTV))
#define GST_SHAGADELICTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SHAGADELICTV,GstShagadelicTVClass))
#define GST_IS_SHAGADELICTV(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SHAGADELICTV))
#define GST_IS_SHAGADELICTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SHAGADELICTV))

typedef struct _GstShagadelicTV GstShagadelicTV;
typedef struct _GstShagadelicTVClass GstShagadelicTVClass;

struct _GstShagadelicTV
{
  GstVideoFilter videofilter;

  /* < private > */
  guint8 *ripple;
  guint8 *spiral;
  guint8 phase;
  gint rx, ry;
  gint bx, by;
  gint rvx, rvy;
  gint bvx, bvy;
};

struct _GstShagadelicTVClass
{
  GstVideoFilterClass parent_class;
};

GType gst_shagadelictv_get_type (void);

G_END_DECLS

#endif /* __GST_SHAGADELIC_H__ */
