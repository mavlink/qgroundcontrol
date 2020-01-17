/* GStreamer
 * Copyright (C) <2006> Wim Taymans <wim.taymans@gmail.com>
 * Copyright (C) <2014> Jurgen Slowack <jurgenslowack@gmail.com>
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

#ifndef __GST_RTP_H265_PAY_H__
#define __GST_RTP_H265_PAY_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasepayload.h>
#include "gstrtph265types.h"

G_BEGIN_DECLS
#define GST_TYPE_RTP_H265_PAY \
  (gst_rtp_h265_pay_get_type())
#define GST_RTP_H265_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_H265_PAY,GstRtpH265Pay))
#define GST_RTP_H265_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_H265_PAY,GstRtpH265PayClass))
#define GST_IS_RTP_H265_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_H265_PAY))
#define GST_IS_RTP_H265_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_H265_PAY))
typedef struct _GstRtpH265Pay GstRtpH265Pay;
typedef struct _GstRtpH265PayClass GstRtpH265PayClass;

typedef enum
{
  GST_H265_ALIGNMENT_UNKNOWN,
  GST_H265_ALIGNMENT_NAL,
  GST_H265_ALIGNMENT_AU
} GstH265Alignment;

typedef enum
{
  GST_RTP_H265_AGGREGATE_NONE,
  GST_RTP_H265_AGGREGATE_ZERO_LATENCY,
  GST_RTP_H265_AGGREGATE_MAX,
} GstRTPH265AggregateMode;

struct _GstRtpH265Pay
{
  GstRTPBasePayload payload;

  GPtrArray *sps, *pps, *vps;

  GstH265StreamFormat stream_format;
  GstH265Alignment alignment;
  gint fps_num;
  gint fps_denum;
  guint nal_length_size;
  GArray *queue;

  GstAdapter *adapter;

  gint vps_sps_pps_interval;
  gboolean send_vps_sps_pps;
  GstClockTime last_vps_sps_pps;

  /* aggregate buffers with AP */
  GstBufferList *bundle;
  guint bundle_size;
  gboolean bundle_contains_vcl_or_suffix;
  GstRTPH265AggregateMode aggregate_mode;
};

struct _GstRtpH265PayClass
{
  GstRTPBasePayloadClass parent_class;
};

GType gst_rtp_h265_pay_get_type (void);

gboolean gst_rtp_h265_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_RTP_H265_PAY_H__ */
