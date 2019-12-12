/* GStreamer
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * OpTV - Optical art meets real-time video effect.
 * Copyright (C) 2004-2005 FUKUCHI Kentaro
 *
 * EffecTV is free software. This library is free software;
 * you can redistribute it and/or
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

#ifndef __GST_OP_H__
#define __GST_OP_H__

#include <gst/gst.h>

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_OPTV \
  (gst_optv_get_type())
#define GST_OPTV(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_OPTV,GstOpTV))
#define GST_OPTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_OPTV,GstOpTVClass))
#define GST_IS_OPTV(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_OPTV))
#define GST_IS_OPTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_OPTV))

typedef struct _GstOpTV GstOpTV;
typedef struct _GstOpTVClass GstOpTVClass;

struct _GstOpTV
{
  GstVideoFilter element;

  /* < private > */
  gint mode;
  gint speed;
  guint threshold;

  gint8 *opmap[4];
  guint8 *diff;
  guint8 phase;
};

struct _GstOpTVClass
{
  GstVideoFilterClass parent_class;
};

GType gst_optv_get_type (void);

G_END_DECLS

#endif /* __GST_OP_H__ */
