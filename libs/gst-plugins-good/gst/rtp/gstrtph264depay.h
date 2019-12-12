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

#ifndef __GST_RTP_H264_DEPAY_H__
#define __GST_RTP_H264_DEPAY_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasedepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_H264_DEPAY \
  (gst_rtp_h264_depay_get_type())
#define GST_RTP_H264_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_H264_DEPAY,GstRtpH264Depay))
#define GST_RTP_H264_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_H264_DEPAY,GstRtpH264DepayClass))
#define GST_IS_RTP_H264_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_H264_DEPAY))
#define GST_IS_RTP_H264_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_H264_DEPAY))

typedef struct _GstRtpH264Depay GstRtpH264Depay;
typedef struct _GstRtpH264DepayClass GstRtpH264DepayClass;

struct _GstRtpH264Depay
{
  GstRTPBaseDepayload depayload;

  gboolean    byte_stream;

  GstBuffer  *codec_data;
  GstAdapter *adapter;
  gboolean    wait_start;

  /* nal merging */
  gboolean    merge;
  GstAdapter *picture_adapter;
  gboolean    picture_start;
  GstClockTime last_ts;
  gboolean    last_keyframe;

  /* Work around broken payloaders wrt. FU-A & FU-B */
  guint8 current_fu_type;
  GstClockTime fu_timestamp;
  gboolean fu_marker;

  /* misc */
  GPtrArray *sps;
  GPtrArray *pps;
  gboolean new_codec_data;

  /* downstream allocator */
  GstAllocator *allocator;
  GstAllocationParams params;
};

struct _GstRtpH264DepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

GType gst_rtp_h264_depay_get_type (void);

gboolean gst_rtp_h264_depay_plugin_init (GstPlugin * plugin);

gboolean gst_rtp_h264_add_sps_pps (GstElement * rtph264, GPtrArray * sps,
    GPtrArray * pps, GstBuffer * nal);

G_END_DECLS

#endif /* __GST_RTP_H264_DEPAY_H__ */
