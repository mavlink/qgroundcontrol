/* Generic video mixer plugin
 * Copyright (C) 2008 Wim Taymans <wim@fluendo.com>
 * Copyright (C) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
 
#ifndef __GST_VIDEO_MIXER2_H__
#define __GST_VIDEO_MIXER2_H__

#include <gst/gst.h>
#include <gst/video/video.h>

#include "blend.h"
#include <gst/base/gstcollectpads.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_MIXER2 (gst_videomixer2_get_type())
#define GST_VIDEO_MIXER2(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIDEO_MIXER2, GstVideoMixer2))
#define GST_VIDEO_MIXER2_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIDEO_MIXER2, GstVideoMixer2Class))
#define GST_IS_VIDEO_MIXER2(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIDEO_MIXER2))
#define GST_IS_VIDEO_MIXER2_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIDEO_MIXER2))

typedef struct _GstVideoMixer2 GstVideoMixer2;
typedef struct _GstVideoMixer2Class GstVideoMixer2Class;

/**
 * GstVideoMixer2Background:
 * @VIDEO_MIXER2_BACKGROUND_CHECKER: checker pattern background
 * @VIDEO_MIXER2_BACKGROUND_BLACK: solid color black background
 * @VIDEO_MIXER2_BACKGROUND_WHITE: solid color white background
 * @VIDEO_MIXER2_BACKGROUND_TRANSPARENT: background is left transparent and layers are composited using "A OVER B" composition rules. This is only applicable to AYUV and ARGB (and variants) as it preserves the alpha channel and allows for further mixing.
 *
 * The different backgrounds videomixer can blend over.
 */
typedef enum
{
  VIDEO_MIXER2_BACKGROUND_CHECKER,
  VIDEO_MIXER2_BACKGROUND_BLACK,
  VIDEO_MIXER2_BACKGROUND_WHITE,
  VIDEO_MIXER2_BACKGROUND_TRANSPARENT,
}
GstVideoMixer2Background;

/**
 * GstVideoMixer2:
 *
 * The opaque #GstVideoMixer2 structure.
 */
struct _GstVideoMixer2
{
  GstElement element;

  /* < private > */

  /* pad */
  GstPad *srcpad;

  /* Lock to prevent the state to change while blending */
  GMutex lock;

  /* Lock to prevent two src setcaps from happening at the same time  */
  GMutex setcaps_lock;

  /* Sink pads using Collect Pads 2*/
  GstCollectPads *collect;

  /* sinkpads, a GSList of GstVideoMixer2Pads */
  GSList *sinkpads;
  gint numpads;
  /* Next available sinkpad index */
  guint next_sinkpad;

  /* Output caps */
  GstVideoInfo info;

  /* current caps */
  GstCaps *current_caps;
  gboolean send_caps;

  gboolean newseg_pending;

  GstVideoMixer2Background background;

  /* Current downstream segment */
  GstSegment segment;
  GstClockTime ts_offset;
  guint64 nframes;

  /* QoS stuff */
  gdouble proportion;
  GstClockTime earliest_time;
  guint64 qos_processed, qos_dropped;

  BlendFunction blend, overlay;
  FillCheckerFunction fill_checker;
  FillColorFunction fill_color;

  gboolean send_stream_start;

  /* latency */
  gboolean live;

  GstTagList *pending_tags;
};

struct _GstVideoMixer2Class
{
  GstElementClass parent_class;
};

GType gst_videomixer2_get_type (void);

G_END_DECLS
#endif /* __GST_VIDEO_MIXER2_H__ */
