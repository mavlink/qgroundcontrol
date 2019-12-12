/* GStreamer
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * StreakTV - afterimage effector.
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

#ifndef __GST_STREAK_H__
#define __GST_STREAK_H__

#include <gst/gst.h>

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_STREAKTV \
  (gst_streaktv_get_type())
#define GST_STREAKTV(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_STREAKTV,GstStreakTV))
#define GST_STREAKTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_STREAKTV,GstStreakTVClass))
#define GST_IS_STREAKTV(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_STREAKTV))
#define GST_IS_STREAKTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_STREAKTV))

typedef struct _GstStreakTV GstStreakTV;
typedef struct _GstStreakTVClass GstStreakTVClass;

#define PLANES 32

struct _GstStreakTV
{
  GstVideoFilter element;

  /* < private > */
  gboolean feedback;

  guint32 *planebuffer;
  guint32 *planetable[PLANES];
  gint plane;
};

struct _GstStreakTVClass
{
  GstVideoFilterClass parent_class;
};

GType gst_streaktv_get_type (void);

G_END_DECLS

#endif /* __GST_STREAK_H__ */
