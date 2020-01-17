/*
 * GStreamer
 * Copyright (C) 2008 Rov Juvano <rovjuvano@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_SCALETEMPO_H__
#define __GST_SCALETEMPO_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_SCALETEMPO            (gst_scaletempo_get_type())
#define GST_SCALETEMPO(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SCALETEMPO, GstScaletempo))
#define GST_SCALETEMPO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  GST_TYPE_SCALETEMPO, GstScaletempoClass))
#define GST_IS_SCALETEMPO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SCALETEMPO))
#define GST_IS_SCALETEMPO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  GST_TYPE_SCALETEMPO))

typedef struct _GstScaletempo GstScaletempo;
typedef struct _GstScaletempoClass GstScaletempoClass;
typedef struct _GstScaletempoPrivate GstScaletempoPrivate;

struct _GstScaletempo
{
  GstBaseTransform element;

  gdouble scale;
  gboolean reverse;

  /* parameters */
  guint ms_stride;
  gdouble percent_overlap;
  guint ms_search;

  /* caps */
  GstAudioFormat format;
  guint samples_per_frame;      /* AKA number of channels */
  guint bytes_per_sample;
  guint bytes_per_frame;
  guint sample_rate;

  /* stride */
  gdouble frames_stride_scaled;
  gdouble frames_stride_error;
  guint bytes_stride;
  gdouble bytes_stride_scaled;
  guint bytes_queue_max;
  guint bytes_queued;
  guint bytes_to_slide;
  gint8 *buf_queue;

  /* overlap */
  guint samples_overlap;
  guint samples_standing;
  guint bytes_overlap;
  guint bytes_standing;
  gpointer buf_overlap;
  gpointer table_blend;
  void (*output_overlap) (GstScaletempo * scaletempo, gpointer out_buf, guint bytes_off);

  /* best overlap */
  guint frames_search;
  gpointer buf_pre_corr;
  gpointer table_window;
  guint (*best_overlap_offset) (GstScaletempo * scaletempo);

  /* gstreamer */
  GstSegment in_segment, out_segment;
  GstClockTime latency;

  /* threads */
  gboolean reinit_buffers;
};

struct _GstScaletempoClass
{
  GstBaseTransformClass parent_class;
};

GType gst_scaletempo_get_type (void);

G_END_DECLS
#endif /* __GST_SCALETEMPO_H__ */
