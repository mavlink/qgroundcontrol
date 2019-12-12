/* GStreamer monoscope visualisation element
 * Copyright (C) <2002> Richard Boulton <richard@tartarus.org>
 * Copyright (C) <2006> Tim-Philipp Müller <tim centricular net>
 * Copyright (C) <2006> Wim Taymans <wim at fluendo dot com>
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

#ifndef __GST_MONOSCOPE__
#define __GST_MONOSCOPE__

G_BEGIN_DECLS

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

#define GST_TYPE_MONOSCOPE            (gst_monoscope_get_type())
#define GST_MONOSCOPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MONOSCOPE,GstMonoscope))
#define GST_MONOSCOPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MONOSCOPE,GstMonoscopeClass))
#define GST_IS_MONOSCOPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MONOSCOPE))
#define GST_IS_MONOSCOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MONOSCOPE))

typedef struct _GstMonoscope GstMonoscope;
typedef struct _GstMonoscopeClass GstMonoscopeClass;

struct _GstMonoscope
{
  GstElement element;

  /* pads */
  GstPad      *sinkpad;
  GstPad      *srcpad;

  GstAdapter  *adapter;

  guint64      next_ts;             /* expected timestamp of the next frame */
  guint64      frame_duration;      /* video frame duration    */
  gint         rate;                /* sample rate             */
  guint        bps;                 /* bytes per sample        */
  guint        spf;                 /* samples per video frame */
  GstBufferPool *pool;

  GstSegment   segment;
  gboolean     segment_pending;

  /* QoS stuff *//* with LOCK */
  gdouble      proportion;
  GstClockTime earliest_time;

  /* video state */
  gint         fps_num;
  gint         fps_denom;
  gint         width;
  gint         height;
  guint        outsize;

  /* visualisation state */
  struct monoscope_state *visstate;
};

struct _GstMonoscopeClass
{
  GstElementClass parent_class;
};

GType gst_monoscope_get_type (void);

G_END_DECLS

#endif /* __GST_MONOSCOPE__ */

