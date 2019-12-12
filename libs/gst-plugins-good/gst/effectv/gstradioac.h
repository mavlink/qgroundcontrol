/* GStreamer
 * Cradioacyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * EffecTV - Realtime Digital Video Effector
 * Cradioacyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * RadioacTV - motion-enlightment effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * EffecTV is free software. This library is free software;
 * you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your radioaction) any later version.
 *
 * This library is distributed in the hradioace that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a cradioacy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_RADIOAC_H__
#define __GST_RADIOAC_H__

#include <gst/gst.h>

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_RADIOACTV \
  (gst_radioactv_get_type())
#define GST_RADIOACTV(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RADIOACTV,GstRadioacTV))
#define GST_RADIOACTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RADIOACTV,GstRadioacTVClass))
#define GST_IS_RADIOACTV(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RADIOACTV))
#define GST_IS_RADIOACTV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RADIOACTV))

typedef struct _GstRadioacTV GstRadioacTV;
typedef struct _GstRadioacTVClass GstRadioacTVClass;

struct _GstRadioacTV
{
  GstVideoFilter element;

  /* < private > */
  gint mode;
  gint color;
  guint interval;
  gboolean trigger;

  gint snaptime;

  guint32 *snapframe;
  guint8 *blurzoombuf;
  guint8 *diff;
  gint16 *background;
  gint *blurzoomx;
  gint *blurzoomy;

  gint buf_width_blocks;
  gint buf_width;
  gint buf_height;
  gint buf_area;
  gint buf_margin_right;
  gint buf_margin_left;
};

struct _GstRadioacTVClass
{
  GstVideoFilterClass parent_class;
};

GType gst_radioactv_get_type (void);

G_END_DECLS

#endif /* __GST_RADIOAC_H__ */
