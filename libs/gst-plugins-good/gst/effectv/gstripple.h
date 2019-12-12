/* GStreamer
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * RippleTV - Water ripple effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
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

#ifndef __GST_RIPPLE_H__
#define __GST_RIPPLE_H__

#include <gst/gst.h>

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_RIPPLETV \
  (gst_rippletv_get_type())
#define GST_RIPPLETV(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RIPPLETV,GstRippleTV))
#define GST_RIPPLETV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RIPPLETV,GstRippleTVClass))
#define GST_IS_RIPPLETV(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RIPPLETV))
#define GST_IS_RIPPLETV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RIPPLETV))

typedef struct _GstRippleTV GstRippleTV;
typedef struct _GstRippleTVClass GstRippleTVClass;

struct _GstRippleTV
{
  GstVideoFilter element;

  /* < private > */
  gint mode;

  gint16 *background;
  guint8 *diff;

  gint *map, *map1, *map2, *map3;
  gint map_h, map_w;

  gint8 *vtable;

  gboolean bg_is_set;

  gint period;
  gint rain_stat;
  guint drop_prob;
  gint drop_prob_increment;
  gint drops_per_frame_max;
  gint drops_per_frame;
  gint drop_power;
};

struct _GstRippleTVClass
{
  GstVideoFilterClass parent_class;
};

GType gst_rippletv_get_type (void);

G_END_DECLS

#endif /* __GST_RIPPLE_H__ */
