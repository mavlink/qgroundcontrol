/* GStreamer
 * Copyright (C) <2006> Wim Taymans <wim.taymans@gmail.com>
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

#ifndef __GST_RTP_H264_PAY_H__
#define __GST_RTP_H264_PAY_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_H264_PAY \
  (gst_rtp_h264_pay_get_type())
#define GST_RTP_H264_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_H264_PAY,GstRtpH264Pay))
#define GST_RTP_H264_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_H264_PAY,GstRtpH264PayClass))
#define GST_IS_RTP_H264_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_H264_PAY))
#define GST_IS_RTP_H264_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_H264_PAY))

typedef struct _GstRtpH264Pay GstRtpH264Pay;
typedef struct _GstRtpH264PayClass GstRtpH264PayClass;

typedef enum
{
  GST_H264_STREAM_FORMAT_UNKNOWN,
  GST_H264_STREAM_FORMAT_BYTESTREAM,
  GST_H264_STREAM_FORMAT_AVC
} GstH264StreamFormat;

typedef enum
{
  GST_H264_ALIGNMENT_UNKNOWN,
  GST_H264_ALIGNMENT_NAL,
  GST_H264_ALIGNMENT_AU
} GstH264Alignment;

typedef enum
{
  GST_RTP_H264_AGGREGATE_NONE,
  GST_RTP_H264_AGGREGATE_ZERO_LATENCY,
  GST_RTP_H264_AGGREGATE_MAX_STAP,
} GstRTPH264AggregateMode;

struct _GstRtpH264Pay
{
  GstRTPBasePayload payload;

  guint profile;
  GPtrArray *sps, *pps;

  GstH264StreamFormat stream_format;
  GstH264Alignment alignment;
  guint nal_length_size;
  GArray *queue;

  gchar *sprop_parameter_sets;
  gboolean update_caps;

  GstAdapter *adapter;

  gint spspps_interval;
  gboolean send_spspps;
  GstClockTime last_spspps;

  gint fps_num;
  gint fps_denum;

  /* TRUE if the next NALU processed should have the DELTA_UNIT flag */
  gboolean delta_unit;
  /* TRUE if the next NALU processed should have the DISCONT flag */
  gboolean discont;

  /* aggregate buffers with STAP-A */
  GstBufferList *bundle;
  guint bundle_size;
  gboolean bundle_contains_vcl;
  GstRTPH264AggregateMode aggregate_mode;
};

struct _GstRtpH264PayClass
{
  GstRTPBasePayloadClass parent_class;
};

GType gst_rtp_h264_pay_get_type (void);

gboolean gst_rtp_h264_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_H264_PAY_H__ */
