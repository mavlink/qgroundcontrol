/* GStreamer
 * Copyright (C) <2007> Wim Taymans <wim.taymans@gmail.com>
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

#ifndef __GST_RTP_BIN_H__
#define __GST_RTP_BIN_H__

#include <gst/gst.h>

#include "rtpsession.h"
#include "gstrtpsession.h"
#include "rtpjitterbuffer.h"

#define GST_TYPE_RTP_BIN \
  (gst_rtp_bin_get_type())
#define GST_RTP_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_BIN,GstRtpBin))
#define GST_RTP_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_BIN,GstRtpBinClass))
#define GST_IS_RTP_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_BIN))
#define GST_IS_RTP_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_BIN))

typedef enum
{
  GST_RTP_BIN_RTCP_SYNC_ALWAYS,
  GST_RTP_BIN_RTCP_SYNC_INITIAL,
  GST_RTP_BIN_RTCP_SYNC_RTP
} GstRTCPSync;

typedef struct _GstRtpBin GstRtpBin;
typedef struct _GstRtpBinClass GstRtpBinClass;
typedef struct _GstRtpBinPrivate GstRtpBinPrivate;

struct _GstRtpBin {
  GstBin         bin;

  /*< private >*/
  /* default latency for sessions */
  guint           latency_ms;
  guint64         latency_ns;
  gboolean        drop_on_latency;
  gboolean        do_lost;
  gboolean        ignore_pt;
  gboolean        ntp_sync;
  gint            rtcp_sync;
  guint           rtcp_sync_interval;
  RTPJitterBufferMode buffer_mode;
  gboolean        buffering;
  gboolean        use_pipeline_clock;
  GstRtpNtpTimeSource ntp_time_source;
  gboolean        send_sync_event;
  GstClockTime    buffer_start;
  gboolean        do_retransmission;
  GstRTPProfile   rtp_profile;
  gboolean        rtcp_sync_send_time;
  gint            max_rtcp_rtp_time_diff;
  guint32         max_dropout_time;
  guint32         max_misorder_time;
  gboolean        rfc7273_sync;
  guint           max_streams;
  guint64         max_ts_offset_adjustment;
  gint64          max_ts_offset;
  gboolean        max_ts_offset_is_set;

  /* a list of session */
  GSList         *sessions;

  /* a list of clients, these are streams with the same CNAME */
  GSList         *clients;

  /* the default SDES items for sessions */
  GstStructure   *sdes;

  /*< private >*/
  GstRtpBinPrivate *priv;
};

struct _GstRtpBinClass {
  GstBinClass  parent_class;

  /* get the caps for pt */
  GstCaps*    (*request_pt_map)       (GstRtpBin *rtpbin, guint session, guint pt);

  void        (*payload_type_change)  (GstRtpBin *rtpbin, guint session, guint pt);

  void        (*new_jitterbuffer)     (GstRtpBin *rtpbin, GstElement *jitterbuffer, guint session, guint32 ssrc);

  void        (*new_storage)     (GstRtpBin *rtpbin, GstElement *jitterbuffer, guint session);

  /* action signals */
  void        (*clear_pt_map)         (GstRtpBin *rtpbin);
  void        (*reset_sync)           (GstRtpBin *rtpbin);
  GstElement* (*get_session)          (GstRtpBin *rtpbin, guint session);
  RTPSession* (*get_internal_session) (GstRtpBin *rtpbin, guint session);
  GstElement* (*get_storage)          (GstRtpBin *rtpbin, guint session);
  GObject*    (*get_internal_storage) (GstRtpBin *rtpbin, guint session);

  /* session manager signals */
  void     (*on_new_ssrc)       (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_ssrc_collision) (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_ssrc_validated) (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_ssrc_active)    (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_ssrc_sdes)      (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_bye_ssrc)       (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_bye_timeout)    (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_timeout)        (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_sender_timeout) (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_npt_stop)       (GstRtpBin *rtpbin, guint session, guint32 ssrc);

  GstElement* (*request_rtp_encoder)  (GstRtpBin *rtpbin, guint session);
  GstElement* (*request_rtp_decoder)  (GstRtpBin *rtpbin, guint session);
  GstElement* (*request_rtcp_encoder) (GstRtpBin *rtpbin, guint session);
  GstElement* (*request_rtcp_decoder) (GstRtpBin *rtpbin, guint session);

  GstElement* (*request_aux_sender)   (GstRtpBin *rtpbin, guint session);
  GstElement* (*request_aux_receiver) (GstRtpBin *rtpbin, guint session);

  GstElement* (*request_fec_encoder)  (GstRtpBin *rtpbin, guint session);
  GstElement* (*request_fec_decoder)  (GstRtpBin *rtpbin, guint session);

  GstElement* (*request_jitterbuffer) (GstRtpBin *rtpbin, guint session);

  void     (*on_new_sender_ssrc)      (GstRtpBin *rtpbin, guint session, guint32 ssrc);
  void     (*on_sender_ssrc_active)   (GstRtpBin *rtpbin, guint session, guint32 ssrc);
};

GType gst_rtp_bin_get_type (void);

#endif /* __GST_RTP_BIN_H__ */
