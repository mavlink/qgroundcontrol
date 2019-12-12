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

#ifndef __GST_RTP_SESSION_H__
#define __GST_RTP_SESSION_H__

#include <gst/gst.h>

#define GST_TYPE_RTP_SESSION \
  (gst_rtp_session_get_type())
#define GST_RTP_SESSION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_SESSION,GstRtpSession))
#define GST_RTP_SESSION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_SESSION,GstRtpSessionClass))
#define GST_IS_RTP_SESSION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_SESSION))
#define GST_IS_RTP_SESSION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_SESSION))
#define GST_RTP_SESSION_CAST(obj) ((GstRtpSession *)(obj))

typedef struct _GstRtpSession GstRtpSession;
typedef struct _GstRtpSessionClass GstRtpSessionClass;
typedef struct _GstRtpSessionPrivate GstRtpSessionPrivate;

struct _GstRtpSession {
  GstElement     element;

  /*< private >*/
  GstPad        *recv_rtp_sink;
  GstSegment     recv_rtp_seg;
  GstPad        *recv_rtcp_sink;
  GstPad        *send_rtp_sink;
  GstSegment     send_rtp_seg;

  GstPad        *recv_rtp_src;
  GstPad        *sync_src;
  GstPad        *send_rtp_src;
  GstPad        *send_rtcp_src;

  guint32        recv_rtcp_segment_seqnum;

  GstRtpSessionPrivate *priv;
};

struct _GstRtpSessionClass {
  GstElementClass parent_class;

  /* signals */
  GstCaps* (*request_pt_map) (GstRtpSession *sess, guint pt);
  void     (*clear_pt_map)   (GstRtpSession *sess);

  void     (*on_new_ssrc)       (GstRtpSession *sess, guint32 ssrc);
  void     (*on_ssrc_collision) (GstRtpSession *sess, guint32 ssrc);
  void     (*on_ssrc_validated) (GstRtpSession *sess, guint32 ssrc);
  void     (*on_ssrc_active)    (GstRtpSession *sess, guint32 ssrc);
  void     (*on_ssrc_sdes)      (GstRtpSession *sess, guint32 ssrc);
  void     (*on_bye_ssrc)       (GstRtpSession *sess, guint32 ssrc);
  void     (*on_bye_timeout)    (GstRtpSession *sess, guint32 ssrc);
  void     (*on_timeout)        (GstRtpSession *sess, guint32 ssrc);
  void     (*on_sender_timeout) (GstRtpSession *sess, guint32 ssrc);
  void     (*on_new_sender_ssrc)      (GstRtpSession *sess, guint32 ssrc);
  void     (*on_sender_ssrc_active)   (GstRtpSession *sess, guint32 ssrc);
};

GType gst_rtp_session_get_type (void);

typedef enum {
  GST_RTP_NTP_TIME_SOURCE_NTP,
  GST_RTP_NTP_TIME_SOURCE_UNIX,
  GST_RTP_NTP_TIME_SOURCE_RUNNING_TIME,
  GST_RTP_NTP_TIME_SOURCE_CLOCK_TIME
} GstRtpNtpTimeSource;

GType gst_rtp_ntp_time_source_get_type (void);

#endif /* __GST_RTP_SESSION_H__ */
