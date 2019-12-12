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

/**
 * SECTION:element-rtpsession
 * @title: rtpsession
 * @see_also: rtpjitterbuffer, rtpbin, rtpptdemux, rtpssrcdemux
 *
 * The RTP session manager models participants with unique SSRC in an RTP
 * session. This session can be used to send and receive RTP and RTCP packets.
 * Based on what REQUEST pads are requested from the session manager, specific
 * functionality can be activated.
 *
 * The session manager currently implements RFC 3550 including:
 *
 *   * RTP packet validation based on consecutive sequence numbers.
 *
 *   * Maintenance of the SSRC participant database.
 *
 *   * Keeping per participant statistics based on received RTCP packets.
 *
 *   * Scheduling of RR/SR RTCP packets.
 *
 *   * Support for multiple sender SSRC.
 *
 * The rtpsession will not demux packets based on SSRC or payload type, nor will
 * it correct for packet reordering and jitter. Use #GstRtpSsrcDemux,
 * #GstRtpPtDemux and GstRtpJitterBuffer in addition to #GstRtpSession to
 * perform these tasks. It is usually a good idea to use #GstRtpBin, which
 * combines all these features in one element.
 *
 * To use #GstRtpSession as an RTP receiver, request a recv_rtp_sink pad, which will
 * automatically create recv_rtp_src pad. Data received on the recv_rtp_sink pad
 * will be processed in the session and after being validated forwarded on the
 * recv_rtp_src pad.
 *
 * To also use #GstRtpSession as an RTCP receiver, request a recv_rtcp_sink pad,
 * which will automatically create a sync_src pad. Packets received on the RTCP
 * pad will be used by the session manager to update the stats and database of
 * the other participants. SR packets will be forwarded on the sync_src pad
 * so that they can be used to perform inter-stream synchronisation when needed.
 *
 * If you want the session manager to generate and send RTCP packets, request
 * the send_rtcp_src pad. Packet pushed on this pad contain SR/RR RTCP reports
 * that should be sent to all participants in the session.
 *
 * To use #GstRtpSession as a sender, request a send_rtp_sink pad, which will
 * automatically create a send_rtp_src pad. The session manager will
 * forward the packets on the send_rtp_src pad after updating its internal state.
 *
 * The session manager needs the clock-rate of the payload types it is handling
 * and will signal the #GstRtpSession::request-pt-map signal when it needs such a
 * mapping. One can clear the cached values with the #GstRtpSession::clear-pt-map
 * signal.
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 udpsrc port=5000 caps="application/x-rtp, ..." ! .recv_rtp_sink rtpsession .recv_rtp_src ! rtptheoradepay ! theoradec ! xvimagesink
 * ]| Receive theora RTP packets from port 5000 and send them to the depayloader,
 * decoder and display. Note that the application/x-rtp caps on udpsrc should be
 * configured based on some negotiation process such as RTSP for this pipeline
 * to work correctly.
 * |[
 * gst-launch-1.0 udpsrc port=5000 caps="application/x-rtp, ..." ! .recv_rtp_sink rtpsession name=session \
 *        .recv_rtp_src ! rtptheoradepay ! theoradec ! xvimagesink \
 *     udpsrc port=5001 caps="application/x-rtcp" ! session.recv_rtcp_sink
 * ]| Receive theora RTP packets from port 5000 and send them to the depayloader,
 * decoder and display. Receive RTCP packets from port 5001 and process them in
 * the session manager.
 * Note that the application/x-rtp caps on udpsrc should be
 * configured based on some negotiation process such as RTSP for this pipeline
 * to work correctly.
 * |[
 * gst-launch-1.0 videotestsrc ! theoraenc ! rtptheorapay ! .send_rtp_sink rtpsession .send_rtp_src ! udpsink port=5000
 * ]| Send theora RTP packets through the session manager and out on UDP port
 * 5000.
 * |[
 * gst-launch-1.0 videotestsrc ! theoraenc ! rtptheorapay ! .send_rtp_sink rtpsession name=session .send_rtp_src \
 *     ! udpsink port=5000  session.send_rtcp_src ! udpsink port=5001
 * ]| Send theora RTP packets through the session manager and out on UDP port
 * 5000. Send RTCP packets on port 5001. Note that this pipeline will not preroll
 * correctly because the second udpsink will not preroll correctly (no RTCP
 * packets are sent in the PAUSED state). Applications should manually set and
 * keep (see gst_element_set_locked_state()) the RTCP udpsink to the PLAYING state.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/rtp/gstrtpbuffer.h>

#include <gst/glib-compat-private.h>

#include "gstrtpsession.h"
#include "rtpsession.h"

GST_DEBUG_CATEGORY_STATIC (gst_rtp_session_debug);
#define GST_CAT_DEFAULT gst_rtp_session_debug

GType
gst_rtp_ntp_time_source_get_type (void)
{
  static GType type = 0;
  static const GEnumValue values[] = {
    {GST_RTP_NTP_TIME_SOURCE_NTP, "NTP time based on realtime clock", "ntp"},
    {GST_RTP_NTP_TIME_SOURCE_UNIX, "UNIX time based on realtime clock", "unix"},
    {GST_RTP_NTP_TIME_SOURCE_RUNNING_TIME,
          "Running time based on pipeline clock",
        "running-time"},
    {GST_RTP_NTP_TIME_SOURCE_CLOCK_TIME, "Pipeline clock time", "clock-time"},
    {0, NULL, NULL},
  };

  if (!type) {
    type = g_enum_register_static ("GstRtpNtpTimeSource", values);
  }
  return type;
}

/* sink pads */
static GstStaticPadTemplate rtpsession_recv_rtp_sink_template =
GST_STATIC_PAD_TEMPLATE ("recv_rtp_sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate rtpsession_recv_rtcp_sink_template =
GST_STATIC_PAD_TEMPLATE ("recv_rtcp_sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

static GstStaticPadTemplate rtpsession_send_rtp_sink_template =
GST_STATIC_PAD_TEMPLATE ("send_rtp_sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtp")
    );

/* src pads */
static GstStaticPadTemplate rtpsession_recv_rtp_src_template =
GST_STATIC_PAD_TEMPLATE ("recv_rtp_src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate rtpsession_sync_src_template =
GST_STATIC_PAD_TEMPLATE ("sync_src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

static GstStaticPadTemplate rtpsession_send_rtp_src_template =
GST_STATIC_PAD_TEMPLATE ("send_rtp_src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate rtpsession_send_rtcp_src_template =
GST_STATIC_PAD_TEMPLATE ("send_rtcp_src",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

/* signals and args */
enum
{
  SIGNAL_REQUEST_PT_MAP,
  SIGNAL_CLEAR_PT_MAP,

  SIGNAL_ON_NEW_SSRC,
  SIGNAL_ON_SSRC_COLLISION,
  SIGNAL_ON_SSRC_VALIDATED,
  SIGNAL_ON_SSRC_ACTIVE,
  SIGNAL_ON_SSRC_SDES,
  SIGNAL_ON_BYE_SSRC,
  SIGNAL_ON_BYE_TIMEOUT,
  SIGNAL_ON_TIMEOUT,
  SIGNAL_ON_SENDER_TIMEOUT,
  SIGNAL_ON_NEW_SENDER_SSRC,
  SIGNAL_ON_SENDER_SSRC_ACTIVE,
  LAST_SIGNAL
};

#define DEFAULT_BANDWIDTH            0
#define DEFAULT_RTCP_FRACTION        RTP_STATS_RTCP_FRACTION
#define DEFAULT_RTCP_RR_BANDWIDTH    -1
#define DEFAULT_RTCP_RS_BANDWIDTH    -1
#define DEFAULT_SDES                 NULL
#define DEFAULT_NUM_SOURCES          0
#define DEFAULT_NUM_ACTIVE_SOURCES   0
#define DEFAULT_USE_PIPELINE_CLOCK   FALSE
#define DEFAULT_RTCP_MIN_INTERVAL    (RTP_STATS_MIN_INTERVAL * GST_SECOND)
#define DEFAULT_PROBATION            RTP_DEFAULT_PROBATION
#define DEFAULT_MAX_DROPOUT_TIME     60000
#define DEFAULT_MAX_MISORDER_TIME    2000
#define DEFAULT_RTP_PROFILE          GST_RTP_PROFILE_AVP
#define DEFAULT_NTP_TIME_SOURCE      GST_RTP_NTP_TIME_SOURCE_NTP
#define DEFAULT_RTCP_SYNC_SEND_TIME  TRUE

enum
{
  PROP_0,
  PROP_BANDWIDTH,
  PROP_RTCP_FRACTION,
  PROP_RTCP_RR_BANDWIDTH,
  PROP_RTCP_RS_BANDWIDTH,
  PROP_SDES,
  PROP_NUM_SOURCES,
  PROP_NUM_ACTIVE_SOURCES,
  PROP_INTERNAL_SESSION,
  PROP_USE_PIPELINE_CLOCK,
  PROP_RTCP_MIN_INTERVAL,
  PROP_PROBATION,
  PROP_MAX_DROPOUT_TIME,
  PROP_MAX_MISORDER_TIME,
  PROP_STATS,
  PROP_RTP_PROFILE,
  PROP_NTP_TIME_SOURCE,
  PROP_RTCP_SYNC_SEND_TIME
};

#define GST_RTP_SESSION_LOCK(sess)   g_mutex_lock (&(sess)->priv->lock)
#define GST_RTP_SESSION_UNLOCK(sess) g_mutex_unlock (&(sess)->priv->lock)

#define GST_RTP_SESSION_WAIT(sess)   g_cond_wait (&(sess)->priv->cond, &(sess)->priv->lock)
#define GST_RTP_SESSION_SIGNAL(sess) g_cond_signal (&(sess)->priv->cond)

struct _GstRtpSessionPrivate
{
  GMutex lock;
  GCond cond;
  GstClock *sysclock;

  RTPSession *session;

  /* thread for sending out RTCP */
  GstClockID id;
  gboolean stop_thread;
  GThread *thread;
  gboolean thread_stopped;
  gboolean wait_send;

  /* caps mapping */
  GHashTable *ptmap;

  GstClockTime send_latency;

  gboolean use_pipeline_clock;
  GstRtpNtpTimeSource ntp_time_source;
  gboolean rtcp_sync_send_time;

  guint recv_rtx_req_count;
  guint sent_rtx_req_count;

  /*
   * This is the list of processed packets in the receive path when upstream
   * pushed a buffer list.
   */
  GstBufferList *processed_list;
};

/* callbacks to handle actions from the session manager */
static GstFlowReturn gst_rtp_session_process_rtp (RTPSession * sess,
    RTPSource * src, GstBuffer * buffer, gpointer user_data);
static GstFlowReturn gst_rtp_session_send_rtp (RTPSession * sess,
    RTPSource * src, gpointer data, gpointer user_data);
static GstFlowReturn gst_rtp_session_send_rtcp (RTPSession * sess,
    RTPSource * src, GstBuffer * buffer, gboolean eos, gpointer user_data);
static GstFlowReturn gst_rtp_session_sync_rtcp (RTPSession * sess,
    GstBuffer * buffer, gpointer user_data);
static gint gst_rtp_session_clock_rate (RTPSession * sess, guint8 payload,
    gpointer user_data);
static void gst_rtp_session_reconsider (RTPSession * sess, gpointer user_data);
static void gst_rtp_session_request_key_unit (RTPSession * sess, guint32 ssrc,
    gboolean all_headers, gpointer user_data);
static GstClockTime gst_rtp_session_request_time (RTPSession * session,
    gpointer user_data);
static void gst_rtp_session_notify_nack (RTPSession * sess,
    guint16 seqnum, guint16 blp, guint32 ssrc, gpointer user_data);
static void gst_rtp_session_reconfigure (RTPSession * sess, gpointer user_data);
static void gst_rtp_session_notify_early_rtcp (RTPSession * sess,
    gpointer user_data);
static GstFlowReturn gst_rtp_session_chain_recv_rtp (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);
static GstFlowReturn gst_rtp_session_chain_recv_rtp_list (GstPad * pad,
    GstObject * parent, GstBufferList * list);
static GstFlowReturn gst_rtp_session_chain_recv_rtcp (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);
static GstFlowReturn gst_rtp_session_chain_send_rtp (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);
static GstFlowReturn gst_rtp_session_chain_send_rtp_list (GstPad * pad,
    GstObject * parent, GstBufferList * list);

static RTPSessionCallbacks callbacks = {
  gst_rtp_session_process_rtp,
  gst_rtp_session_send_rtp,
  gst_rtp_session_sync_rtcp,
  gst_rtp_session_send_rtcp,
  gst_rtp_session_clock_rate,
  gst_rtp_session_reconsider,
  gst_rtp_session_request_key_unit,
  gst_rtp_session_request_time,
  gst_rtp_session_notify_nack,
  gst_rtp_session_reconfigure,
  gst_rtp_session_notify_early_rtcp
};

/* GObject vmethods */
static void gst_rtp_session_finalize (GObject * object);
static void gst_rtp_session_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_session_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/* GstElement vmethods */
static GstStateChangeReturn gst_rtp_session_change_state (GstElement * element,
    GstStateChange transition);
static GstPad *gst_rtp_session_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static void gst_rtp_session_release_pad (GstElement * element, GstPad * pad);

static gboolean gst_rtp_session_sink_setcaps (GstPad * pad,
    GstRtpSession * rtpsession, GstCaps * caps);
static gboolean gst_rtp_session_setcaps_send_rtp (GstPad * pad,
    GstRtpSession * rtpsession, GstCaps * caps);

static void gst_rtp_session_clear_pt_map (GstRtpSession * rtpsession);

static GstStructure *gst_rtp_session_create_stats (GstRtpSession * rtpsession);

static guint gst_rtp_session_signals[LAST_SIGNAL] = { 0 };

static void
on_new_ssrc (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_NEW_SSRC], 0,
      src->ssrc);
}

static void
on_ssrc_collision (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  GstPad *send_rtp_sink;

  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_SSRC_COLLISION], 0,
      src->ssrc);

  GST_RTP_SESSION_LOCK (sess);
  if ((send_rtp_sink = sess->send_rtp_sink))
    gst_object_ref (send_rtp_sink);
  GST_RTP_SESSION_UNLOCK (sess);

  if (send_rtp_sink) {
    GstStructure *structure;
    GstEvent *event;
    RTPSource *internal_src;
    guint32 suggested_ssrc;

    structure = gst_structure_new ("GstRTPCollision", "ssrc", G_TYPE_UINT,
        (guint) src->ssrc, NULL);

    /* if there is no source using the suggested ssrc, most probably because
     * this ssrc has just collided, suggest upstream to use it */
    suggested_ssrc = rtp_session_suggest_ssrc (session, NULL);
    internal_src = rtp_session_get_source_by_ssrc (session, suggested_ssrc);
    if (!internal_src)
      gst_structure_set (structure, "suggested-ssrc", G_TYPE_UINT,
          (guint) suggested_ssrc, NULL);
    else
      g_object_unref (internal_src);

    event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, structure);
    gst_pad_push_event (send_rtp_sink, event);
    gst_object_unref (send_rtp_sink);
  }
}

static void
on_ssrc_validated (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_SSRC_VALIDATED], 0,
      src->ssrc);
}

static void
on_ssrc_active (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_SSRC_ACTIVE], 0,
      src->ssrc);
}

static void
on_ssrc_sdes (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  GstStructure *s;
  GstMessage *m;

  /* convert the new SDES info into a message */
  RTP_SESSION_LOCK (session);
  g_object_get (src, "sdes", &s, NULL);
  RTP_SESSION_UNLOCK (session);

  m = gst_message_new_custom (GST_MESSAGE_ELEMENT, GST_OBJECT (sess), s);
  gst_element_post_message (GST_ELEMENT_CAST (sess), m);

  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_SSRC_SDES], 0,
      src->ssrc);
}

static void
on_bye_ssrc (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_BYE_SSRC], 0,
      src->ssrc);
}

static void
on_bye_timeout (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_BYE_TIMEOUT], 0,
      src->ssrc);
}

static void
on_timeout (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_TIMEOUT], 0,
      src->ssrc);
}

static void
on_sender_timeout (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_SENDER_TIMEOUT], 0,
      src->ssrc);
}

static void
on_new_sender_ssrc (RTPSession * session, RTPSource * src, GstRtpSession * sess)
{
  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_NEW_SENDER_SSRC], 0,
      src->ssrc);
}

static void
on_sender_ssrc_active (RTPSession * session, RTPSource * src,
    GstRtpSession * sess)
{
  g_signal_emit (sess, gst_rtp_session_signals[SIGNAL_ON_SENDER_SSRC_ACTIVE], 0,
      src->ssrc);
}

static void
on_notify_stats (RTPSession * session, GParamSpec * spec,
    GstRtpSession * rtpsession)
{
  g_object_notify (G_OBJECT (rtpsession), "stats");
}

#define gst_rtp_session_parent_class parent_class
G_DEFINE_TYPE_WITH_PRIVATE (GstRtpSession, gst_rtp_session, GST_TYPE_ELEMENT);

static void
gst_rtp_session_class_init (GstRtpSessionClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->finalize = gst_rtp_session_finalize;
  gobject_class->set_property = gst_rtp_session_set_property;
  gobject_class->get_property = gst_rtp_session_get_property;

  /**
   * GstRtpSession::request-pt-map:
   * @sess: the object which received the signal
   * @pt: the pt
   *
   * Request the payload type as #GstCaps for @pt.
   */
  gst_rtp_session_signals[SIGNAL_REQUEST_PT_MAP] =
      g_signal_new ("request-pt-map", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass, request_pt_map),
      NULL, NULL, NULL, GST_TYPE_CAPS, 1, G_TYPE_UINT);
  /**
   * GstRtpSession::clear-pt-map:
   * @sess: the object which received the signal
   *
   * Clear the cached pt-maps requested with #GstRtpSession::request-pt-map.
   */
  gst_rtp_session_signals[SIGNAL_CLEAR_PT_MAP] =
      g_signal_new ("clear-pt-map", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstRtpSessionClass, clear_pt_map),
      NULL, NULL, NULL, G_TYPE_NONE, 0, G_TYPE_NONE);

  /**
   * GstRtpSession::on-new-ssrc:
   * @sess: the object which received the signal
   * @ssrc: the SSRC
   *
   * Notify of a new SSRC that entered @session.
   */
  gst_rtp_session_signals[SIGNAL_ON_NEW_SSRC] =
      g_signal_new ("on-new-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass, on_new_ssrc),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
  /**
   * GstRtpSession::on-ssrc_collision:
   * @sess: the object which received the signal
   * @ssrc: the SSRC
   *
   * Notify when we have an SSRC collision
   */
  gst_rtp_session_signals[SIGNAL_ON_SSRC_COLLISION] =
      g_signal_new ("on-ssrc-collision", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass,
          on_ssrc_collision), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
  /**
   * GstRtpSession::on-ssrc_validated:
   * @sess: the object which received the signal
   * @ssrc: the SSRC
   *
   * Notify of a new SSRC that became validated.
   */
  gst_rtp_session_signals[SIGNAL_ON_SSRC_VALIDATED] =
      g_signal_new ("on-ssrc-validated", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass,
          on_ssrc_validated), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
  /**
   * GstRtpSession::on-ssrc-active:
   * @sess: the object which received the signal
   * @ssrc: the SSRC
   *
   * Notify of a SSRC that is active, i.e., sending RTCP.
   */
  gst_rtp_session_signals[SIGNAL_ON_SSRC_ACTIVE] =
      g_signal_new ("on-ssrc-active", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass,
          on_ssrc_active), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
  /**
   * GstRtpSession::on-ssrc-sdes:
   * @session: the object which received the signal
   * @src: the SSRC
   *
   * Notify that a new SDES was received for SSRC.
   */
  gst_rtp_session_signals[SIGNAL_ON_SSRC_SDES] =
      g_signal_new ("on-ssrc-sdes", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass, on_ssrc_sdes),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);

  /**
   * GstRtpSession::on-bye-ssrc:
   * @sess: the object which received the signal
   * @ssrc: the SSRC
   *
   * Notify of an SSRC that became inactive because of a BYE packet.
   */
  gst_rtp_session_signals[SIGNAL_ON_BYE_SSRC] =
      g_signal_new ("on-bye-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass, on_bye_ssrc),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
  /**
   * GstRtpSession::on-bye-timeout:
   * @sess: the object which received the signal
   * @ssrc: the SSRC
   *
   * Notify of an SSRC that has timed out because of BYE
   */
  gst_rtp_session_signals[SIGNAL_ON_BYE_TIMEOUT] =
      g_signal_new ("on-bye-timeout", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass, on_bye_timeout),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
  /**
   * GstRtpSession::on-timeout:
   * @sess: the object which received the signal
   * @ssrc: the SSRC
   *
   * Notify of an SSRC that has timed out
   */
  gst_rtp_session_signals[SIGNAL_ON_TIMEOUT] =
      g_signal_new ("on-timeout", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass, on_timeout),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
  /**
   * GstRtpSession::on-sender-timeout:
   * @sess: the object which received the signal
   * @ssrc: the SSRC
   *
   * Notify of a sender SSRC that has timed out and became a receiver
   */
  gst_rtp_session_signals[SIGNAL_ON_SENDER_TIMEOUT] =
      g_signal_new ("on-sender-timeout", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass,
          on_sender_timeout), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);

  /**
   * GstRtpSession::on-new-sender-ssrc:
   * @sess: the object which received the signal
   * @ssrc: the sender SSRC
   *
   * Notify of a new sender SSRC that entered @session.
   *
   * Since: 1.8
   */
  gst_rtp_session_signals[SIGNAL_ON_NEW_SENDER_SSRC] =
      g_signal_new ("on-new-sender-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass, on_new_ssrc),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);

  /**
   * GstRtpSession::on-sender-ssrc-active:
   * @sess: the object which received the signal
   * @ssrc: the sender SSRC
   *
   * Notify of a sender SSRC that is active, i.e., sending RTCP.
   *
   * Since: 1.8
   */
  gst_rtp_session_signals[SIGNAL_ON_SENDER_SSRC_ACTIVE] =
      g_signal_new ("on-sender-ssrc-active", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpSessionClass,
          on_ssrc_active), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);

  g_object_class_install_property (gobject_class, PROP_BANDWIDTH,
      g_param_spec_double ("bandwidth", "Bandwidth",
          "The bandwidth of the session in bytes per second (0 for auto-discover)",
          0.0, G_MAXDOUBLE, DEFAULT_BANDWIDTH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_FRACTION,
      g_param_spec_double ("rtcp-fraction", "RTCP Fraction",
          "The RTCP bandwidth of the session in bytes per second "
          "(or as a real fraction of the RTP bandwidth if < 1.0)",
          0.0, G_MAXDOUBLE, DEFAULT_RTCP_FRACTION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_RR_BANDWIDTH,
      g_param_spec_int ("rtcp-rr-bandwidth", "RTCP RR bandwidth",
          "The RTCP bandwidth used for receivers in bytes per second (-1 = default)",
          -1, G_MAXINT, DEFAULT_RTCP_RR_BANDWIDTH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_RS_BANDWIDTH,
      g_param_spec_int ("rtcp-rs-bandwidth", "RTCP RS bandwidth",
          "The RTCP bandwidth used for senders in bytes per second (-1 = default)",
          -1, G_MAXINT, DEFAULT_RTCP_RS_BANDWIDTH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SDES,
      g_param_spec_boxed ("sdes", "SDES",
          "The SDES items of this session",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
          | GST_PARAM_DOC_SHOW_DEFAULT));

  g_object_class_install_property (gobject_class, PROP_NUM_SOURCES,
      g_param_spec_uint ("num-sources", "Num Sources",
          "The number of sources in the session", 0, G_MAXUINT,
          DEFAULT_NUM_SOURCES, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NUM_ACTIVE_SOURCES,
      g_param_spec_uint ("num-active-sources", "Num Active Sources",
          "The number of active sources in the session", 0, G_MAXUINT,
          DEFAULT_NUM_ACTIVE_SOURCES,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INTERNAL_SESSION,
      g_param_spec_object ("internal-session", "Internal Session",
          "The internal RTPSession object", RTP_TYPE_SESSION,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_USE_PIPELINE_CLOCK,
      g_param_spec_boolean ("use-pipeline-clock", "Use pipeline clock",
          "Use the pipeline running-time to set the NTP time in the RTCP SR messages "
          "(DEPRECATED: Use ntp-time-source property)",
          DEFAULT_USE_PIPELINE_CLOCK,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));

  g_object_class_install_property (gobject_class, PROP_RTCP_MIN_INTERVAL,
      g_param_spec_uint64 ("rtcp-min-interval", "Minimum RTCP interval",
          "Minimum interval between Regular RTCP packet (in ns)",
          0, G_MAXUINT64, DEFAULT_RTCP_MIN_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROBATION,
      g_param_spec_uint ("probation", "Number of probations",
          "Consecutive packet sequence numbers to accept the source",
          0, G_MAXUINT, DEFAULT_PROBATION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_DROPOUT_TIME,
      g_param_spec_uint ("max-dropout-time", "Max dropout time",
          "The maximum time (milliseconds) of missing packets tolerated.",
          0, G_MAXUINT, DEFAULT_MAX_DROPOUT_TIME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_MISORDER_TIME,
      g_param_spec_uint ("max-misorder-time", "Max misorder time",
          "The maximum time (milliseconds) of misordered packets tolerated.",
          0, G_MAXUINT, DEFAULT_MAX_MISORDER_TIME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtpSession::stats:
   *
   * Various session statistics. This property returns a GstStructure
   * with name application/x-rtp-session-stats with the following fields:
   *
   *  "recv-rtx-req-count  G_TYPE_UINT   The number of retransmission event
   *      received from downstream (in receiver mode) (Since 1.16)
   *  "sent-rtx-req-count" G_TYPE_UINT   The number of retransmission event
   *      sent downstream (in sender mode) (Since 1.16)
   *  "rtx-count"          G_TYPE_UINT   DEPRECATED Since 1.16, same as
   *      "recv-rtx-req-count".
   *  "rtx-drop-count"     G_TYPE_UINT   The number of retransmission events
   *      dropped (due to bandwidth constraints)
   *  "sent-nack-count"    G_TYPE_UINT   Number of NACKs sent
   *  "recv-nack-count"    G_TYPE_UINT   Number of NACKs received
   *  "source-stats"       G_TYPE_BOXED  GValueArray of #RTPSource::stats for all
   *      RTP sources (Since 1.8)
   *
   * Since: 1.4
   */
  g_object_class_install_property (gobject_class, PROP_STATS,
      g_param_spec_boxed ("stats", "Statistics",
          "Various statistics", GST_TYPE_STRUCTURE,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTP_PROFILE,
      g_param_spec_enum ("rtp-profile", "RTP Profile",
          "RTP profile to use", GST_TYPE_RTP_PROFILE, DEFAULT_RTP_PROFILE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NTP_TIME_SOURCE,
      g_param_spec_enum ("ntp-time-source", "NTP Time Source",
          "NTP time source for RTCP packets",
          gst_rtp_ntp_time_source_get_type (), DEFAULT_NTP_TIME_SOURCE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_SYNC_SEND_TIME,
      g_param_spec_boolean ("rtcp-sync-send-time", "RTCP Sync Send Time",
          "Use send time or capture time for RTCP sync "
          "(TRUE = send time, FALSE = capture time)",
          DEFAULT_RTCP_SYNC_SEND_TIME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_session_change_state);
  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_rtp_session_request_new_pad);
  gstelement_class->release_pad =
      GST_DEBUG_FUNCPTR (gst_rtp_session_release_pad);

  klass->clear_pt_map = GST_DEBUG_FUNCPTR (gst_rtp_session_clear_pt_map);

  /* sink pads */
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpsession_recv_rtp_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpsession_recv_rtcp_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpsession_send_rtp_sink_template);

  /* src pads */
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpsession_recv_rtp_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpsession_sync_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpsession_send_rtp_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpsession_send_rtcp_src_template);

  gst_element_class_set_static_metadata (gstelement_class, "RTP Session",
      "Filter/Network/RTP",
      "Implement an RTP session", "Wim Taymans <wim.taymans@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (gst_rtp_session_debug,
      "rtpsession", 0, "RTP Session");

  GST_DEBUG_REGISTER_FUNCPTR (gst_rtp_session_chain_recv_rtp);
  GST_DEBUG_REGISTER_FUNCPTR (gst_rtp_session_chain_recv_rtp_list);
  GST_DEBUG_REGISTER_FUNCPTR (gst_rtp_session_chain_recv_rtcp);
  GST_DEBUG_REGISTER_FUNCPTR (gst_rtp_session_chain_send_rtp);
  GST_DEBUG_REGISTER_FUNCPTR (gst_rtp_session_chain_send_rtp_list);

}

static void
gst_rtp_session_init (GstRtpSession * rtpsession)
{
  rtpsession->priv = gst_rtp_session_get_instance_private (rtpsession);
  g_mutex_init (&rtpsession->priv->lock);
  g_cond_init (&rtpsession->priv->cond);
  rtpsession->priv->sysclock = gst_system_clock_obtain ();
  rtpsession->priv->session = rtp_session_new ();
  rtpsession->priv->use_pipeline_clock = DEFAULT_USE_PIPELINE_CLOCK;
  rtpsession->priv->rtcp_sync_send_time = DEFAULT_RTCP_SYNC_SEND_TIME;

  /* configure callbacks */
  rtp_session_set_callbacks (rtpsession->priv->session, &callbacks, rtpsession);
  /* configure signals */
  g_signal_connect (rtpsession->priv->session, "on-new-ssrc",
      (GCallback) on_new_ssrc, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-ssrc-collision",
      (GCallback) on_ssrc_collision, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-ssrc-validated",
      (GCallback) on_ssrc_validated, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-ssrc-active",
      (GCallback) on_ssrc_active, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-ssrc-sdes",
      (GCallback) on_ssrc_sdes, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-bye-ssrc",
      (GCallback) on_bye_ssrc, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-bye-timeout",
      (GCallback) on_bye_timeout, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-timeout",
      (GCallback) on_timeout, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-sender-timeout",
      (GCallback) on_sender_timeout, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-new-sender-ssrc",
      (GCallback) on_new_sender_ssrc, rtpsession);
  g_signal_connect (rtpsession->priv->session, "on-sender-ssrc-active",
      (GCallback) on_sender_ssrc_active, rtpsession);
  g_signal_connect (rtpsession->priv->session, "notify::stats",
      (GCallback) on_notify_stats, rtpsession);
  rtpsession->priv->ptmap = g_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) gst_caps_unref);

  rtpsession->recv_rtcp_segment_seqnum = GST_SEQNUM_INVALID;

  gst_segment_init (&rtpsession->recv_rtp_seg, GST_FORMAT_UNDEFINED);
  gst_segment_init (&rtpsession->send_rtp_seg, GST_FORMAT_UNDEFINED);

  rtpsession->priv->thread_stopped = TRUE;

  rtpsession->priv->recv_rtx_req_count = 0;
  rtpsession->priv->sent_rtx_req_count = 0;

  rtpsession->priv->ntp_time_source = DEFAULT_NTP_TIME_SOURCE;
}

static void
gst_rtp_session_finalize (GObject * object)
{
  GstRtpSession *rtpsession;

  rtpsession = GST_RTP_SESSION (object);

  g_hash_table_destroy (rtpsession->priv->ptmap);
  g_mutex_clear (&rtpsession->priv->lock);
  g_cond_clear (&rtpsession->priv->cond);
  g_object_unref (rtpsession->priv->sysclock);
  g_object_unref (rtpsession->priv->session);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_session_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpSession *rtpsession;
  GstRtpSessionPrivate *priv;

  rtpsession = GST_RTP_SESSION (object);
  priv = rtpsession->priv;

  switch (prop_id) {
    case PROP_BANDWIDTH:
      g_object_set_property (G_OBJECT (priv->session), "bandwidth", value);
      break;
    case PROP_RTCP_FRACTION:
      g_object_set_property (G_OBJECT (priv->session), "rtcp-fraction", value);
      break;
    case PROP_RTCP_RR_BANDWIDTH:
      g_object_set_property (G_OBJECT (priv->session), "rtcp-rr-bandwidth",
          value);
      break;
    case PROP_RTCP_RS_BANDWIDTH:
      g_object_set_property (G_OBJECT (priv->session), "rtcp-rs-bandwidth",
          value);
      break;
    case PROP_SDES:
      rtp_session_set_sdes_struct (priv->session, g_value_get_boxed (value));
      break;
    case PROP_USE_PIPELINE_CLOCK:
      priv->use_pipeline_clock = g_value_get_boolean (value);
      break;
    case PROP_RTCP_MIN_INTERVAL:
      g_object_set_property (G_OBJECT (priv->session), "rtcp-min-interval",
          value);
      break;
    case PROP_PROBATION:
      g_object_set_property (G_OBJECT (priv->session), "probation", value);
      break;
    case PROP_MAX_DROPOUT_TIME:
      g_object_set_property (G_OBJECT (priv->session), "max-dropout-time",
          value);
      break;
    case PROP_MAX_MISORDER_TIME:
      g_object_set_property (G_OBJECT (priv->session), "max-misorder-time",
          value);
      break;
    case PROP_RTP_PROFILE:
      g_object_set_property (G_OBJECT (priv->session), "rtp-profile", value);
      break;
    case PROP_NTP_TIME_SOURCE:
      priv->ntp_time_source = g_value_get_enum (value);
      break;
    case PROP_RTCP_SYNC_SEND_TIME:
      priv->rtcp_sync_send_time = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_session_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpSession *rtpsession;
  GstRtpSessionPrivate *priv;

  rtpsession = GST_RTP_SESSION (object);
  priv = rtpsession->priv;

  switch (prop_id) {
    case PROP_BANDWIDTH:
      g_object_get_property (G_OBJECT (priv->session), "bandwidth", value);
      break;
    case PROP_RTCP_FRACTION:
      g_object_get_property (G_OBJECT (priv->session), "rtcp-fraction", value);
      break;
    case PROP_RTCP_RR_BANDWIDTH:
      g_object_get_property (G_OBJECT (priv->session), "rtcp-rr-bandwidth",
          value);
      break;
    case PROP_RTCP_RS_BANDWIDTH:
      g_object_get_property (G_OBJECT (priv->session), "rtcp-rs-bandwidth",
          value);
      break;
    case PROP_SDES:
      g_value_take_boxed (value, rtp_session_get_sdes_struct (priv->session));
      break;
    case PROP_NUM_SOURCES:
      g_value_set_uint (value, rtp_session_get_num_sources (priv->session));
      break;
    case PROP_NUM_ACTIVE_SOURCES:
      g_value_set_uint (value,
          rtp_session_get_num_active_sources (priv->session));
      break;
    case PROP_INTERNAL_SESSION:
      g_value_set_object (value, priv->session);
      break;
    case PROP_USE_PIPELINE_CLOCK:
      g_value_set_boolean (value, priv->use_pipeline_clock);
      break;
    case PROP_RTCP_MIN_INTERVAL:
      g_object_get_property (G_OBJECT (priv->session), "rtcp-min-interval",
          value);
      break;
    case PROP_PROBATION:
      g_object_get_property (G_OBJECT (priv->session), "probation", value);
      break;
    case PROP_MAX_DROPOUT_TIME:
      g_object_get_property (G_OBJECT (priv->session), "max-dropout-time",
          value);
      break;
    case PROP_MAX_MISORDER_TIME:
      g_object_get_property (G_OBJECT (priv->session), "max-misorder-time",
          value);
      break;
    case PROP_STATS:
      g_value_take_boxed (value, gst_rtp_session_create_stats (rtpsession));
      break;
    case PROP_RTP_PROFILE:
      g_object_get_property (G_OBJECT (priv->session), "rtp-profile", value);
      break;
    case PROP_NTP_TIME_SOURCE:
      g_value_set_enum (value, priv->ntp_time_source);
      break;
    case PROP_RTCP_SYNC_SEND_TIME:
      g_value_set_boolean (value, priv->rtcp_sync_send_time);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStructure *
gst_rtp_session_create_stats (GstRtpSession * rtpsession)
{
  GstStructure *s;

  g_object_get (rtpsession->priv->session, "stats", &s, NULL);
  gst_structure_set (s, "rtx-count", G_TYPE_UINT,
      rtpsession->priv->recv_rtx_req_count, "recv-rtx-req-count", G_TYPE_UINT,
      rtpsession->priv->recv_rtx_req_count, "sent-rtx-req-count", G_TYPE_UINT,
      rtpsession->priv->sent_rtx_req_count, NULL);

  return s;
}

static void
get_current_times (GstRtpSession * rtpsession, GstClockTime * running_time,
    guint64 * ntpnstime)
{
  guint64 ntpns = -1;
  GstClock *clock;
  GstClockTime base_time, rt, clock_time;

  GST_OBJECT_LOCK (rtpsession);
  if ((clock = GST_ELEMENT_CLOCK (rtpsession))) {
    base_time = GST_ELEMENT_CAST (rtpsession)->base_time;
    gst_object_ref (clock);
    GST_OBJECT_UNLOCK (rtpsession);

    /* get current clock time and convert to running time */
    clock_time = gst_clock_get_time (clock);
    rt = clock_time - base_time;

    if (rtpsession->priv->use_pipeline_clock) {
      ntpns = rt;
      /* add constant to convert from 1970 based time to 1900 based time */
      ntpns += (2208988800LL * GST_SECOND);
    } else {
      switch (rtpsession->priv->ntp_time_source) {
        case GST_RTP_NTP_TIME_SOURCE_NTP:
        case GST_RTP_NTP_TIME_SOURCE_UNIX:{
          /* get current NTP time */
          ntpns = g_get_real_time () * GST_USECOND;

          /* add constant to convert from 1970 based time to 1900 based time */
          if (rtpsession->priv->ntp_time_source == GST_RTP_NTP_TIME_SOURCE_NTP)
            ntpns += (2208988800LL * GST_SECOND);
          break;
        }
        case GST_RTP_NTP_TIME_SOURCE_RUNNING_TIME:
          ntpns = rt;
          break;
        case GST_RTP_NTP_TIME_SOURCE_CLOCK_TIME:
          ntpns = clock_time;
          break;
        default:
          ntpns = -1;
          g_assert_not_reached ();
          break;
      }
    }

    gst_object_unref (clock);
  } else {
    GST_OBJECT_UNLOCK (rtpsession);
    rt = -1;
    ntpns = -1;
  }
  if (running_time)
    *running_time = rt;
  if (ntpnstime)
    *ntpnstime = ntpns;
}

/* must be called with GST_RTP_SESSION_LOCK */
static void
signal_waiting_rtcp_thread_unlocked (GstRtpSession * rtpsession)
{
  if (rtpsession->priv->wait_send) {
    GST_LOG_OBJECT (rtpsession, "signal RTCP thread");
    rtpsession->priv->wait_send = FALSE;
    GST_RTP_SESSION_SIGNAL (rtpsession);
  }
}

static void
rtcp_thread (GstRtpSession * rtpsession)
{
  GstClockID id;
  GstClockTime current_time;
  GstClockTime next_timeout;
  guint64 ntpnstime;
  GstClockTime running_time;
  RTPSession *session;
  GstClock *sysclock;

  GST_DEBUG_OBJECT (rtpsession, "entering RTCP thread");

  GST_RTP_SESSION_LOCK (rtpsession);

  while (rtpsession->priv->wait_send) {
    GST_LOG_OBJECT (rtpsession, "waiting for getting started");
    GST_RTP_SESSION_WAIT (rtpsession);
    GST_LOG_OBJECT (rtpsession, "signaled...");
  }

  sysclock = rtpsession->priv->sysclock;
  current_time = gst_clock_get_time (sysclock);

  session = rtpsession->priv->session;

  GST_DEBUG_OBJECT (rtpsession, "starting at %" GST_TIME_FORMAT,
      GST_TIME_ARGS (current_time));
  session->start_time = current_time;

  while (!rtpsession->priv->stop_thread) {
    GstClockReturn res;

    /* get initial estimate */
    next_timeout = rtp_session_next_timeout (session, current_time);

    GST_DEBUG_OBJECT (rtpsession, "next check time %" GST_TIME_FORMAT,
        GST_TIME_ARGS (next_timeout));

    /* leave if no more timeouts, the session ended */
    if (next_timeout == GST_CLOCK_TIME_NONE)
      break;

    id = rtpsession->priv->id =
        gst_clock_new_single_shot_id (sysclock, next_timeout);
    GST_RTP_SESSION_UNLOCK (rtpsession);

    res = gst_clock_id_wait (id, NULL);

    GST_RTP_SESSION_LOCK (rtpsession);
    gst_clock_id_unref (id);
    rtpsession->priv->id = NULL;

    if (rtpsession->priv->stop_thread)
      break;

    /* update current time */
    current_time = gst_clock_get_time (sysclock);

    /* get current NTP time */
    get_current_times (rtpsession, &running_time, &ntpnstime);

    /* we get unlocked because we need to perform reconsideration, don't perform
     * the timeout but get a new reporting estimate. */
    GST_DEBUG_OBJECT (rtpsession, "unlocked %d, current %" GST_TIME_FORMAT,
        res, GST_TIME_ARGS (current_time));

    /* perform actions, we ignore result. Release lock because it might push. */
    GST_RTP_SESSION_UNLOCK (rtpsession);
    rtp_session_on_timeout (session, current_time, ntpnstime, running_time);
    GST_RTP_SESSION_LOCK (rtpsession);
  }
  /* mark the thread as stopped now */
  rtpsession->priv->thread_stopped = TRUE;
  GST_RTP_SESSION_UNLOCK (rtpsession);

  GST_DEBUG_OBJECT (rtpsession, "leaving RTCP thread");
}

static gboolean
start_rtcp_thread (GstRtpSession * rtpsession)
{
  GError *error = NULL;
  gboolean res;

  GST_DEBUG_OBJECT (rtpsession, "starting RTCP thread");

  GST_RTP_SESSION_LOCK (rtpsession);
  rtpsession->priv->stop_thread = FALSE;
  if (rtpsession->priv->thread_stopped) {
    /* if the thread stopped, and we still have a handle to the thread, join it
     * now. We can safely join with the lock held, the thread will not take it
     * anymore. */
    if (rtpsession->priv->thread)
      g_thread_join (rtpsession->priv->thread);
    /* only create a new thread if the old one was stopped. Otherwise we can
     * just reuse the currently running one. */
    rtpsession->priv->thread = g_thread_try_new ("rtpsession-rtcp-thread",
        (GThreadFunc) rtcp_thread, rtpsession, &error);
    rtpsession->priv->thread_stopped = FALSE;
  }
  GST_RTP_SESSION_UNLOCK (rtpsession);

  if (error != NULL) {
    res = FALSE;
    GST_DEBUG_OBJECT (rtpsession, "failed to start thread, %s", error->message);
    g_error_free (error);
  } else {
    res = TRUE;
  }
  return res;
}

static void
stop_rtcp_thread (GstRtpSession * rtpsession)
{
  GST_DEBUG_OBJECT (rtpsession, "stopping RTCP thread");

  GST_RTP_SESSION_LOCK (rtpsession);
  rtpsession->priv->stop_thread = TRUE;
  signal_waiting_rtcp_thread_unlocked (rtpsession);
  if (rtpsession->priv->id)
    gst_clock_id_unschedule (rtpsession->priv->id);
  GST_RTP_SESSION_UNLOCK (rtpsession);
}

static void
join_rtcp_thread (GstRtpSession * rtpsession)
{
  GST_RTP_SESSION_LOCK (rtpsession);
  /* don't try to join when we have no thread */
  if (rtpsession->priv->thread != NULL) {
    GST_DEBUG_OBJECT (rtpsession, "joining RTCP thread");
    GST_RTP_SESSION_UNLOCK (rtpsession);

    g_thread_join (rtpsession->priv->thread);

    GST_RTP_SESSION_LOCK (rtpsession);
    /* after the join, take the lock and clear the thread structure. The caller
     * is supposed to not concurrently call start and join. */
    rtpsession->priv->thread = NULL;
  }
  GST_RTP_SESSION_UNLOCK (rtpsession);
}

static GstStateChangeReturn
gst_rtp_session_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn res;
  GstRtpSession *rtpsession;

  rtpsession = GST_RTP_SESSION (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_RTP_SESSION_LOCK (rtpsession);
      rtpsession->priv->wait_send = TRUE;
      GST_RTP_SESSION_UNLOCK (rtpsession);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      /* no need to join yet, we might want to continue later. Also, the
       * dataflow could block downstream so that a join could just block
       * forever. */
      stop_rtcp_thread (rtpsession);
      break;
    default:
      break;
  }

  res = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      if (!start_rtcp_thread (rtpsession))
        goto failed_thread;
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      /* downstream is now releasing the dataflow and we can join. */
      join_rtcp_thread (rtpsession);
      rtp_session_reset (rtpsession->priv->session);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return res;

  /* ERRORS */
failed_thread:
  {
    return GST_STATE_CHANGE_FAILURE;
  }
}

static gboolean
return_true (gpointer key, gpointer value, gpointer user_data)
{
  return TRUE;
}

static void
gst_rtp_session_clear_pt_map (GstRtpSession * rtpsession)
{
  GST_RTP_SESSION_LOCK (rtpsession);
  g_hash_table_foreach_remove (rtpsession->priv->ptmap, return_true, NULL);
  GST_RTP_SESSION_UNLOCK (rtpsession);
}

/* called when the session manager has an RTP packet ready to be pushed */
static GstFlowReturn
gst_rtp_session_process_rtp (RTPSession * sess, RTPSource * src,
    GstBuffer * buffer, gpointer user_data)
{
  GstFlowReturn result;
  GstRtpSession *rtpsession;
  GstPad *rtp_src;

  rtpsession = GST_RTP_SESSION (user_data);

  GST_RTP_SESSION_LOCK (rtpsession);
  if ((rtp_src = rtpsession->recv_rtp_src))
    gst_object_ref (rtp_src);
  GST_RTP_SESSION_UNLOCK (rtpsession);

  if (rtp_src) {
    if (rtpsession->priv->processed_list) {
      GST_LOG_OBJECT (rtpsession, "queueing received RTP packet");
      gst_buffer_list_add (rtpsession->priv->processed_list, buffer);
      result = GST_FLOW_OK;
    } else {
      GST_LOG_OBJECT (rtpsession, "pushing received RTP packet");
      result = gst_pad_push (rtp_src, buffer);
    }
    gst_object_unref (rtp_src);
  } else {
    GST_DEBUG_OBJECT (rtpsession, "dropping received RTP packet");
    gst_buffer_unref (buffer);
    result = GST_FLOW_OK;
  }
  return result;
}

/* called when the session manager has an RTP packet ready for further
 * sending */
static GstFlowReturn
gst_rtp_session_send_rtp (RTPSession * sess, RTPSource * src,
    gpointer data, gpointer user_data)
{
  GstFlowReturn result;
  GstRtpSession *rtpsession;
  GstPad *rtp_src;

  rtpsession = GST_RTP_SESSION (user_data);

  GST_RTP_SESSION_LOCK (rtpsession);
  if ((rtp_src = rtpsession->send_rtp_src))
    gst_object_ref (rtp_src);
  signal_waiting_rtcp_thread_unlocked (rtpsession);
  GST_RTP_SESSION_UNLOCK (rtpsession);

  if (rtp_src) {
    if (GST_IS_BUFFER (data)) {
      GST_LOG_OBJECT (rtpsession, "sending RTP packet");
      result = gst_pad_push (rtp_src, GST_BUFFER_CAST (data));
    } else {
      GST_LOG_OBJECT (rtpsession, "sending RTP list");
      result = gst_pad_push_list (rtp_src, GST_BUFFER_LIST_CAST (data));
    }
    gst_object_unref (rtp_src);
  } else {
    gst_mini_object_unref (GST_MINI_OBJECT_CAST (data));
    result = GST_FLOW_OK;
  }
  return result;
}

static void
do_rtcp_events (GstRtpSession * rtpsession, GstPad * srcpad)
{
  GstCaps *caps;
  GstSegment seg;
  GstEvent *event;
  gchar *stream_id;
  gboolean have_group_id;
  guint group_id;

  stream_id =
      g_strdup_printf ("%08x%08x%08x%08x", g_random_int (), g_random_int (),
      g_random_int (), g_random_int ());

  GST_RTP_SESSION_LOCK (rtpsession);
  if (rtpsession->recv_rtp_sink) {
    event =
        gst_pad_get_sticky_event (rtpsession->recv_rtp_sink,
        GST_EVENT_STREAM_START, 0);
    if (event) {
      if (gst_event_parse_group_id (event, &group_id))
        have_group_id = TRUE;
      else
        have_group_id = FALSE;
      gst_event_unref (event);
    } else {
      have_group_id = TRUE;
      group_id = gst_util_group_id_next ();
    }
  } else {
    have_group_id = TRUE;
    group_id = gst_util_group_id_next ();
  }
  GST_RTP_SESSION_UNLOCK (rtpsession);

  event = gst_event_new_stream_start (stream_id);
  rtpsession->recv_rtcp_segment_seqnum = gst_event_get_seqnum (event);
  gst_event_set_seqnum (event, rtpsession->recv_rtcp_segment_seqnum);
  if (have_group_id)
    gst_event_set_group_id (event, group_id);
  gst_pad_push_event (srcpad, event);
  g_free (stream_id);

  caps = gst_caps_new_empty_simple ("application/x-rtcp");
  gst_pad_set_caps (srcpad, caps);
  gst_caps_unref (caps);

  gst_segment_init (&seg, GST_FORMAT_TIME);
  event = gst_event_new_segment (&seg);
  gst_event_set_seqnum (event, rtpsession->recv_rtcp_segment_seqnum);
  gst_pad_push_event (srcpad, event);
}

/* called when the session manager has an RTCP packet ready for further
 * sending. The eos flag is set when an EOS event should be sent downstream as
 * well. */
static GstFlowReturn
gst_rtp_session_send_rtcp (RTPSession * sess, RTPSource * src,
    GstBuffer * buffer, gboolean all_sources_bye, gpointer user_data)
{
  GstFlowReturn result;
  GstRtpSession *rtpsession;
  GstPad *rtcp_src;

  rtpsession = GST_RTP_SESSION (user_data);

  GST_RTP_SESSION_LOCK (rtpsession);
  if (rtpsession->priv->stop_thread)
    goto stopping;

  if ((rtcp_src = rtpsession->send_rtcp_src)) {
    gst_object_ref (rtcp_src);
    GST_RTP_SESSION_UNLOCK (rtpsession);

    /* set rtcp caps on output pad */
    if (!gst_pad_has_current_caps (rtcp_src))
      do_rtcp_events (rtpsession, rtcp_src);

    GST_LOG_OBJECT (rtpsession, "sending RTCP");
    result = gst_pad_push (rtcp_src, buffer);

    /* Forward send an EOS on the RTCP sink if we received an EOS on the
     * send_rtp_sink. We don't need to check the recv_rtp_sink since in this
     * case the EOS event would already have been sent */
    if (all_sources_bye && rtpsession->send_rtp_sink &&
        GST_PAD_IS_EOS (rtpsession->send_rtp_sink)) {
      GstEvent *event;

      GST_LOG_OBJECT (rtpsession, "sending EOS");

      event = gst_event_new_eos ();
      gst_event_set_seqnum (event, rtpsession->recv_rtcp_segment_seqnum);
      gst_pad_push_event (rtcp_src, event);
    }
    gst_object_unref (rtcp_src);
  } else {
    GST_RTP_SESSION_UNLOCK (rtpsession);

    GST_DEBUG_OBJECT (rtpsession, "not sending RTCP, no output pad");
    gst_buffer_unref (buffer);
    result = GST_FLOW_OK;
  }
  return result;

  /* ERRORS */
stopping:
  {
    GST_DEBUG_OBJECT (rtpsession, "we are stopping");
    gst_buffer_unref (buffer);
    GST_RTP_SESSION_UNLOCK (rtpsession);
    return GST_FLOW_OK;
  }
}

/* called when the session manager has an SR RTCP packet ready for handling
 * inter stream synchronisation */
static GstFlowReturn
gst_rtp_session_sync_rtcp (RTPSession * sess,
    GstBuffer * buffer, gpointer user_data)
{
  GstFlowReturn result;
  GstRtpSession *rtpsession;
  GstPad *sync_src;

  rtpsession = GST_RTP_SESSION (user_data);

  GST_RTP_SESSION_LOCK (rtpsession);
  if (rtpsession->priv->stop_thread)
    goto stopping;

  if ((sync_src = rtpsession->sync_src)) {
    gst_object_ref (sync_src);
    GST_RTP_SESSION_UNLOCK (rtpsession);

    /* set rtcp caps on output pad, this happens
     * when we receive RTCP muxed with RTP according
     * to RFC5761. Otherwise we would have forwarded
     * the events from the recv_rtcp_sink pad already
     */
    if (!gst_pad_has_current_caps (sync_src))
      do_rtcp_events (rtpsession, sync_src);

    GST_LOG_OBJECT (rtpsession, "sending Sync RTCP");
    result = gst_pad_push (sync_src, buffer);
    gst_object_unref (sync_src);
  } else {
    GST_RTP_SESSION_UNLOCK (rtpsession);

    GST_DEBUG_OBJECT (rtpsession, "not sending Sync RTCP, no output pad");
    gst_buffer_unref (buffer);
    result = GST_FLOW_OK;
  }
  return result;

  /* ERRORS */
stopping:
  {
    GST_DEBUG_OBJECT (rtpsession, "we are stopping");
    gst_buffer_unref (buffer);
    GST_RTP_SESSION_UNLOCK (rtpsession);
    return GST_FLOW_OK;
  }
}

static void
gst_rtp_session_cache_caps (GstRtpSession * rtpsession, GstCaps * caps)
{
  GstRtpSessionPrivate *priv;
  const GstStructure *s;
  gint payload;

  priv = rtpsession->priv;

  GST_DEBUG_OBJECT (rtpsession, "parsing caps");

  s = gst_caps_get_structure (caps, 0);
  if (!gst_structure_get_int (s, "payload", &payload))
    return;

  if (g_hash_table_lookup (priv->ptmap, GINT_TO_POINTER (payload)))
    return;

  g_hash_table_insert (priv->ptmap, GINT_TO_POINTER (payload),
      gst_caps_ref (caps));
}

static GstCaps *
gst_rtp_session_get_caps_for_pt (GstRtpSession * rtpsession, guint payload)
{
  GstCaps *caps = NULL;
  GValue args[2] = { {0}, {0} };
  GValue ret = { 0 };

  GST_RTP_SESSION_LOCK (rtpsession);
  caps = g_hash_table_lookup (rtpsession->priv->ptmap,
      GINT_TO_POINTER (payload));
  if (caps) {
    gst_caps_ref (caps);
    goto done;
  }

  /* not found in the cache, try to get it with a signal */
  g_value_init (&args[0], GST_TYPE_ELEMENT);
  g_value_set_object (&args[0], rtpsession);
  g_value_init (&args[1], G_TYPE_UINT);
  g_value_set_uint (&args[1], payload);

  g_value_init (&ret, GST_TYPE_CAPS);
  g_value_set_boxed (&ret, NULL);

  GST_RTP_SESSION_UNLOCK (rtpsession);

  g_signal_emitv (args, gst_rtp_session_signals[SIGNAL_REQUEST_PT_MAP], 0,
      &ret);

  GST_RTP_SESSION_LOCK (rtpsession);

  g_value_unset (&args[0]);
  g_value_unset (&args[1]);
  caps = (GstCaps *) g_value_dup_boxed (&ret);
  g_value_unset (&ret);
  if (!caps)
    goto no_caps;

  gst_rtp_session_cache_caps (rtpsession, caps);

done:
  GST_RTP_SESSION_UNLOCK (rtpsession);

  return caps;

no_caps:
  {
    GST_DEBUG_OBJECT (rtpsession, "could not get caps");
    goto done;
  }
}

/* called when the session manager needs the clock rate */
static gint
gst_rtp_session_clock_rate (RTPSession * sess, guint8 payload,
    gpointer user_data)
{
  gint result = -1;
  GstRtpSession *rtpsession;
  GstCaps *caps;
  const GstStructure *s;

  rtpsession = GST_RTP_SESSION_CAST (user_data);

  caps = gst_rtp_session_get_caps_for_pt (rtpsession, payload);

  if (!caps)
    goto done;

  s = gst_caps_get_structure (caps, 0);
  if (!gst_structure_get_int (s, "clock-rate", &result))
    goto no_clock_rate;

  gst_caps_unref (caps);

  GST_DEBUG_OBJECT (rtpsession, "parsed clock-rate %d", result);

done:

  return result;

  /* ERRORS */
no_clock_rate:
  {
    gst_caps_unref (caps);
    GST_DEBUG_OBJECT (rtpsession, "No clock-rate in caps!");
    goto done;
  }
}

/* called when the session manager asks us to reconsider the timeout */
static void
gst_rtp_session_reconsider (RTPSession * sess, gpointer user_data)
{
  GstRtpSession *rtpsession;

  rtpsession = GST_RTP_SESSION_CAST (user_data);

  GST_RTP_SESSION_LOCK (rtpsession);
  GST_DEBUG_OBJECT (rtpsession, "unlock timer for reconsideration");
  if (rtpsession->priv->id)
    gst_clock_id_unschedule (rtpsession->priv->id);
  GST_RTP_SESSION_UNLOCK (rtpsession);
}

static gboolean
gst_rtp_session_event_recv_rtp_sink (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRtpSession *rtpsession;
  gboolean ret = FALSE;

  rtpsession = GST_RTP_SESSION (parent);

  GST_DEBUG_OBJECT (rtpsession, "received event %s",
      GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      /* process */
      gst_event_parse_caps (event, &caps);
      gst_rtp_session_sink_setcaps (pad, rtpsession, caps);
      ret = gst_pad_push_event (rtpsession->recv_rtp_src, event);
      break;
    }
    case GST_EVENT_FLUSH_STOP:
      gst_segment_init (&rtpsession->recv_rtp_seg, GST_FORMAT_UNDEFINED);
      rtpsession->recv_rtcp_segment_seqnum = GST_SEQNUM_INVALID;
      ret = gst_pad_push_event (rtpsession->recv_rtp_src, event);
      break;
    case GST_EVENT_SEGMENT:
    {
      GstSegment *segment, in_segment;

      segment = &rtpsession->recv_rtp_seg;

      /* the newsegment event is needed to convert the RTP timestamp to
       * running_time, which is needed to generate a mapping from RTP to NTP
       * timestamps in SR reports */
      gst_event_copy_segment (event, &in_segment);
      GST_DEBUG_OBJECT (rtpsession, "received segment %" GST_SEGMENT_FORMAT,
          &in_segment);

      /* accept upstream */
      gst_segment_copy_into (&in_segment, segment);

      /* push event forward */
      ret = gst_pad_push_event (rtpsession->recv_rtp_src, event);
      break;
    }
    case GST_EVENT_EOS:
    {
      GstPad *rtcp_src;

      ret =
          gst_pad_push_event (rtpsession->recv_rtp_src, gst_event_ref (event));

      GST_RTP_SESSION_LOCK (rtpsession);
      if ((rtcp_src = rtpsession->send_rtcp_src))
        gst_object_ref (rtcp_src);
      GST_RTP_SESSION_UNLOCK (rtpsession);

      gst_event_unref (event);

      if (rtcp_src) {
        event = gst_event_new_eos ();
        if (rtpsession->recv_rtcp_segment_seqnum != GST_SEQNUM_INVALID)
          gst_event_set_seqnum (event, rtpsession->recv_rtcp_segment_seqnum);
        ret = gst_pad_push_event (rtcp_src, event);
        gst_object_unref (rtcp_src);
      } else {
        ret = TRUE;
      }
      break;
    }
    default:
      ret = gst_pad_push_event (rtpsession->recv_rtp_src, event);
      break;
  }

  return ret;

}

static gboolean
gst_rtp_session_request_remote_key_unit (GstRtpSession * rtpsession,
    guint32 ssrc, guint payload, gboolean all_headers, gint count)
{
  GstCaps *caps;

  caps = gst_rtp_session_get_caps_for_pt (rtpsession, payload);

  if (caps) {
    const GstStructure *s = gst_caps_get_structure (caps, 0);
    gboolean pli;
    gboolean fir;

    pli = gst_structure_has_field (s, "rtcp-fb-nack-pli");
    fir = gst_structure_has_field (s, "rtcp-fb-ccm-fir") && all_headers;

    /* Google Talk uses FIR for repair, so send it even if we just want a
     * regular PLI */
    if (!pli &&
        gst_structure_has_field (s, "rtcp-fb-x-gstreamer-fir-as-repair"))
      fir = TRUE;

    gst_caps_unref (caps);

    if (pli || fir)
      return rtp_session_request_key_unit (rtpsession->priv->session, ssrc,
          fir, count);
  }

  return FALSE;
}

static gboolean
gst_rtp_session_event_recv_rtp_src (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRtpSession *rtpsession;
  gboolean forward = TRUE;
  gboolean ret = TRUE;
  const GstStructure *s;
  guint32 ssrc;
  guint pt;

  rtpsession = GST_RTP_SESSION (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:
      s = gst_event_get_structure (event);
      if (gst_structure_has_name (s, "GstForceKeyUnit") &&
          gst_structure_get_uint (s, "ssrc", &ssrc) &&
          gst_structure_get_uint (s, "payload", &pt)) {
        gboolean all_headers = FALSE;
        gint count = -1;

        gst_structure_get_boolean (s, "all-headers", &all_headers);
        if (gst_structure_get_int (s, "count", &count) && count < 0)
          count += G_MAXINT;    /* Make sure count is positive if present */
        if (gst_rtp_session_request_remote_key_unit (rtpsession, ssrc, pt,
                all_headers, count))
          forward = FALSE;
      } else if (gst_structure_has_name (s, "GstRTPRetransmissionRequest")) {
        guint seqnum, delay, deadline, max_delay, avg_rtt;

        GST_RTP_SESSION_LOCK (rtpsession);
        rtpsession->priv->recv_rtx_req_count++;
        GST_RTP_SESSION_UNLOCK (rtpsession);

        if (!gst_structure_get_uint (s, "ssrc", &ssrc))
          ssrc = -1;
        if (!gst_structure_get_uint (s, "seqnum", &seqnum))
          seqnum = -1;
        if (!gst_structure_get_uint (s, "delay", &delay))
          delay = 0;
        if (!gst_structure_get_uint (s, "deadline", &deadline))
          deadline = 100;
        if (!gst_structure_get_uint (s, "avg-rtt", &avg_rtt))
          avg_rtt = 40;

        /* remaining time to receive the packet */
        max_delay = deadline;
        if (max_delay > delay)
          max_delay -= delay;
        /* estimated RTT */
        if (max_delay > avg_rtt)
          max_delay -= avg_rtt;
        else
          max_delay = 0;

        if (rtp_session_request_nack (rtpsession->priv->session, ssrc, seqnum,
                max_delay * GST_MSECOND))
          forward = FALSE;
      }
      break;
    default:
      break;
  }

  if (forward) {
    GstPad *recv_rtp_sink;

    GST_RTP_SESSION_LOCK (rtpsession);
    if ((recv_rtp_sink = rtpsession->recv_rtp_sink))
      gst_object_ref (recv_rtp_sink);
    GST_RTP_SESSION_UNLOCK (rtpsession);

    if (recv_rtp_sink) {
      ret = gst_pad_push_event (recv_rtp_sink, event);
      gst_object_unref (recv_rtp_sink);
    } else
      gst_event_unref (event);
  } else {
    gst_event_unref (event);
  }

  return ret;
}


static GstIterator *
gst_rtp_session_iterate_internal_links (GstPad * pad, GstObject * parent)
{
  GstRtpSession *rtpsession;
  GstPad *otherpad = NULL;
  GstIterator *it = NULL;

  rtpsession = GST_RTP_SESSION (parent);

  GST_RTP_SESSION_LOCK (rtpsession);
  if (pad == rtpsession->recv_rtp_src) {
    otherpad = gst_object_ref (rtpsession->recv_rtp_sink);
  } else if (pad == rtpsession->recv_rtp_sink) {
    otherpad = gst_object_ref (rtpsession->recv_rtp_src);
  } else if (pad == rtpsession->send_rtp_src) {
    otherpad = gst_object_ref (rtpsession->send_rtp_sink);
  } else if (pad == rtpsession->send_rtp_sink) {
    otherpad = gst_object_ref (rtpsession->send_rtp_src);
  }
  GST_RTP_SESSION_UNLOCK (rtpsession);

  if (otherpad) {
    GValue val = { 0, };

    g_value_init (&val, GST_TYPE_PAD);
    g_value_set_object (&val, otherpad);
    it = gst_iterator_new_single (GST_TYPE_PAD, &val);
    g_value_unset (&val);
    gst_object_unref (otherpad);
  } else {
    it = gst_iterator_new_single (GST_TYPE_PAD, NULL);
  }

  return it;
}

static gboolean
gst_rtp_session_sink_setcaps (GstPad * pad, GstRtpSession * rtpsession,
    GstCaps * caps)
{
  GST_RTP_SESSION_LOCK (rtpsession);
  gst_rtp_session_cache_caps (rtpsession, caps);
  GST_RTP_SESSION_UNLOCK (rtpsession);

  return TRUE;
}

/* receive a packet from a sender, send it to the RTP session manager and
 * forward the packet on the rtp_src pad
 */
static GstFlowReturn
gst_rtp_session_chain_recv_rtp (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{
  GstRtpSession *rtpsession;
  GstRtpSessionPrivate *priv;
  GstFlowReturn ret;
  GstClockTime current_time, running_time;
  GstClockTime timestamp;
  guint64 ntpnstime;

  rtpsession = GST_RTP_SESSION (parent);
  priv = rtpsession->priv;

  GST_LOG_OBJECT (rtpsession, "received RTP packet");

  GST_RTP_SESSION_LOCK (rtpsession);
  signal_waiting_rtcp_thread_unlocked (rtpsession);
  GST_RTP_SESSION_UNLOCK (rtpsession);

  /* get NTP time when this packet was captured, this depends on the timestamp. */
  timestamp = GST_BUFFER_PTS (buffer);
  if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
    /* convert to running time using the segment values */
    running_time =
        gst_segment_to_running_time (&rtpsession->recv_rtp_seg, GST_FORMAT_TIME,
        timestamp);
    ntpnstime = GST_CLOCK_TIME_NONE;
  } else {
    get_current_times (rtpsession, &running_time, &ntpnstime);
  }
  current_time = gst_clock_get_time (priv->sysclock);

  ret = rtp_session_process_rtp (priv->session, buffer, current_time,
      running_time, ntpnstime);
  if (ret != GST_FLOW_OK)
    goto push_error;

done:

  return ret;

  /* ERRORS */
push_error:
  {
    GST_DEBUG_OBJECT (rtpsession, "process returned %s",
        gst_flow_get_name (ret));
    goto done;
  }
}

static gboolean
process_received_buffer_in_list (GstBuffer ** buffer, guint idx, gpointer data)
{
  gint ret;

  ret = gst_rtp_session_chain_recv_rtp (NULL, data, *buffer);
  if (ret != GST_FLOW_OK)
    GST_ERROR ("Processing individual buffer in a list failed");

  /*
   * The buffer has been processed, remove it from the original list, if it was
   * a valid RTP buffer it has been added to the "processed" list in
   * gst_rtp_session_process_rtp().
   */
  *buffer = NULL;
  return TRUE;
}

static GstFlowReturn
gst_rtp_session_chain_recv_rtp_list (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstRtpSession *rtpsession = GST_RTP_SESSION (parent);
  GstBufferList *processed_list;

  processed_list = gst_buffer_list_new ();

  /* Set some private data to detect that a buffer list is being pushed. */
  rtpsession->priv->processed_list = processed_list;

  /*
   * Individually process the buffers from the incoming buffer list as the
   * incoming RTP packets in the list can be mixed in all sorts of ways:
   *    - different frames,
   *    - different sources,
   *    - different types (RTP or RTCP)
   */
  gst_buffer_list_foreach (list,
      (GstBufferListFunc) process_received_buffer_in_list, parent);

  gst_buffer_list_unref (list);

  /* Clean up private data in case the next push does not use a buffer list. */
  rtpsession->priv->processed_list = NULL;

  if (gst_buffer_list_length (processed_list) == 0 || !rtpsession->recv_rtp_src) {
    gst_buffer_list_unref (processed_list);
    return GST_FLOW_OK;
  }

  GST_LOG_OBJECT (rtpsession, "pushing received RTP list");
  return gst_pad_push_list (rtpsession->recv_rtp_src, processed_list);
}

static gboolean
gst_rtp_session_event_recv_rtcp_sink (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRtpSession *rtpsession;
  gboolean ret = FALSE;

  rtpsession = GST_RTP_SESSION (parent);

  GST_DEBUG_OBJECT (rtpsession, "received event %s",
      GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
      /* Make sure that the sync_src pad has caps before the segment event.
       * Otherwise we might get a segment event before caps from the receive
       * RTCP pad, and then later when receiving RTCP packets will set caps.
       * This will results in a sticky event misordering warning
       */
      if (!gst_pad_has_current_caps (rtpsession->sync_src)) {
        GstCaps *caps = gst_caps_new_empty_simple ("application/x-rtcp");
        gst_pad_set_caps (rtpsession->sync_src, caps);
        gst_caps_unref (caps);
      }
      /* fall through */
    default:
      ret = gst_pad_push_event (rtpsession->sync_src, event);
      break;
  }

  return ret;
}

/* Receive an RTCP packet from a sender, send it to the RTP session manager and
 * forward the SR packets to the sync_src pad.
 */
static GstFlowReturn
gst_rtp_session_chain_recv_rtcp (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{
  GstRtpSession *rtpsession;
  GstRtpSessionPrivate *priv;
  GstClockTime current_time;
  GstClockTime running_time;
  guint64 ntpnstime;

  rtpsession = GST_RTP_SESSION (parent);
  priv = rtpsession->priv;

  GST_LOG_OBJECT (rtpsession, "received RTCP packet");

  GST_RTP_SESSION_LOCK (rtpsession);
  signal_waiting_rtcp_thread_unlocked (rtpsession);
  GST_RTP_SESSION_UNLOCK (rtpsession);

  current_time = gst_clock_get_time (priv->sysclock);
  get_current_times (rtpsession, &running_time, &ntpnstime);

  rtp_session_process_rtcp (priv->session, buffer, current_time, running_time,
      ntpnstime);

  return GST_FLOW_OK;           /* always return OK */
}

static gboolean
gst_rtp_session_query_send_rtcp_src (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstRtpSession *rtpsession;
  gboolean ret = FALSE;

  rtpsession = GST_RTP_SESSION (parent);

  GST_DEBUG_OBJECT (rtpsession, "received QUERY %s",
      GST_QUERY_TYPE_NAME (query));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:
      ret = TRUE;
      /* use the defaults for the latency query. */
      gst_query_set_latency (query, FALSE, 0, -1);
      break;
    default:
      /* other queries simply fail for now */
      break;
  }

  return ret;
}

static gboolean
gst_rtp_session_event_send_rtcp_src (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRtpSession *rtpsession;
  gboolean ret = TRUE;

  rtpsession = GST_RTP_SESSION (parent);
  GST_DEBUG_OBJECT (rtpsession, "received EVENT %s",
      GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
    case GST_EVENT_LATENCY:
      gst_event_unref (event);
      ret = TRUE;
      break;
    default:
      /* other events simply fail for now */
      gst_event_unref (event);
      ret = FALSE;
      break;
  }

  return ret;
}


static gboolean
gst_rtp_session_event_send_rtp_sink (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRtpSession *rtpsession;
  gboolean ret = FALSE;

  rtpsession = GST_RTP_SESSION (parent);

  GST_DEBUG_OBJECT (rtpsession, "received EVENT %s",
      GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      /* process */
      gst_event_parse_caps (event, &caps);
      gst_rtp_session_setcaps_send_rtp (pad, rtpsession, caps);
      ret = gst_pad_push_event (rtpsession->send_rtp_src, event);
      break;
    }
    case GST_EVENT_FLUSH_STOP:
      gst_segment_init (&rtpsession->send_rtp_seg, GST_FORMAT_UNDEFINED);
      ret = gst_pad_push_event (rtpsession->send_rtp_src, event);
      break;
    case GST_EVENT_SEGMENT:{
      GstSegment *segment, in_segment;

      segment = &rtpsession->send_rtp_seg;

      /* the newsegment event is needed to convert the RTP timestamp to
       * running_time, which is needed to generate a mapping from RTP to NTP
       * timestamps in SR reports */
      gst_event_copy_segment (event, &in_segment);
      GST_DEBUG_OBJECT (rtpsession, "received segment %" GST_SEGMENT_FORMAT,
          &in_segment);

      /* accept upstream */
      gst_segment_copy_into (&in_segment, segment);

      /* push event forward */
      ret = gst_pad_push_event (rtpsession->send_rtp_src, event);
      break;
    }
    case GST_EVENT_EOS:{
      GstClockTime current_time;

      /* push downstream FIXME, we are not supposed to leave the session just
       * because we stop sending. */
      ret = gst_pad_push_event (rtpsession->send_rtp_src, event);
      current_time = gst_clock_get_time (rtpsession->priv->sysclock);

      GST_DEBUG_OBJECT (rtpsession, "scheduling BYE message");
      rtp_session_mark_all_bye (rtpsession->priv->session, "End Of Stream");
      rtp_session_schedule_bye (rtpsession->priv->session, current_time);
      break;
    }
    default:{
      GstPad *send_rtp_src;

      GST_RTP_SESSION_LOCK (rtpsession);
      if ((send_rtp_src = rtpsession->send_rtp_src))
        gst_object_ref (send_rtp_src);
      GST_RTP_SESSION_UNLOCK (rtpsession);

      if (send_rtp_src) {
        ret = gst_pad_push_event (send_rtp_src, event);
        gst_object_unref (send_rtp_src);
      } else
        gst_event_unref (event);

      break;
    }
  }

  return ret;
}

static gboolean
gst_rtp_session_event_send_rtp_src (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRtpSession *rtpsession;
  gboolean ret = FALSE;

  rtpsession = GST_RTP_SESSION (parent);

  GST_DEBUG_OBJECT (rtpsession, "received EVENT %s",
      GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_LATENCY:
      /* save the latency, we need this to know when an RTP packet will be
       * rendered by the sink */
      gst_event_parse_latency (event, &rtpsession->priv->send_latency);

      ret = gst_pad_event_default (pad, parent, event);
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

static GstCaps *
gst_rtp_session_getcaps_send_rtp (GstPad * pad, GstRtpSession * rtpsession,
    GstCaps * filter)
{
  GstRtpSessionPrivate *priv;
  GstCaps *result;
  GstStructure *s1, *s2;
  guint ssrc;
  gboolean is_random;

  priv = rtpsession->priv;

  ssrc = rtp_session_suggest_ssrc (priv->session, &is_random);

  /* we can basically accept anything but we prefer to receive packets with our
   * internal SSRC so that we don't have to patch it. Create a structure with
   * the SSRC and another one without.
   * Only do this if the session actually decided on an ssrc already,
   * otherwise we give upstream the opportunity to select an ssrc itself */
  if (!is_random) {
    s1 = gst_structure_new ("application/x-rtp", "ssrc", G_TYPE_UINT, ssrc,
        NULL);
    s2 = gst_structure_new_empty ("application/x-rtp");

    result = gst_caps_new_full (s1, s2, NULL);
  } else {
    result = gst_caps_new_empty_simple ("application/x-rtp");
  }

  if (filter) {
    GstCaps *caps = result;

    result = gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
  }

  GST_DEBUG_OBJECT (rtpsession, "getting caps %" GST_PTR_FORMAT, result);

  return result;
}

static gboolean
gst_rtp_session_query_send_rtp (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  gboolean res = FALSE;
  GstRtpSession *rtpsession;

  rtpsession = GST_RTP_SESSION (parent);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);
      caps = gst_rtp_session_getcaps_send_rtp (pad, rtpsession, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

static gboolean
gst_rtp_session_setcaps_send_rtp (GstPad * pad, GstRtpSession * rtpsession,
    GstCaps * caps)
{
  GstRtpSessionPrivate *priv;

  priv = rtpsession->priv;

  rtp_session_update_send_caps (priv->session, caps);

  return TRUE;
}

/* Receive an RTP packet or a list of packets to be sent to the receivers,
 * send to RTP session manager and forward to send_rtp_src.
 */
static GstFlowReturn
gst_rtp_session_chain_send_rtp_common (GstRtpSession * rtpsession,
    gpointer data, gboolean is_list)
{
  GstRtpSessionPrivate *priv;
  GstFlowReturn ret;
  GstClockTime timestamp, running_time;
  GstClockTime current_time;

  priv = rtpsession->priv;

  GST_LOG_OBJECT (rtpsession, "received RTP %s", is_list ? "list" : "packet");

  /* get NTP time when this packet was captured, this depends on the timestamp. */
  if (is_list) {
    GstBuffer *buffer = NULL;

    /* All buffers in a list have the same timestamp.
     * So, just take it from the first buffer. */
    buffer = gst_buffer_list_get (GST_BUFFER_LIST_CAST (data), 0);
    if (buffer)
      timestamp = GST_BUFFER_PTS (buffer);
    else
      timestamp = -1;
  } else {
    timestamp = GST_BUFFER_PTS (GST_BUFFER_CAST (data));
  }

  if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
    /* convert to running time using the segment start value. */
    running_time =
        gst_segment_to_running_time (&rtpsession->send_rtp_seg, GST_FORMAT_TIME,
        timestamp);
    if (priv->rtcp_sync_send_time)
      running_time += priv->send_latency;
  } else {
    /* no timestamp. */
    running_time = -1;
  }

  current_time = gst_clock_get_time (priv->sysclock);
  ret = rtp_session_send_rtp (priv->session, data, is_list, current_time,
      running_time);
  if (ret != GST_FLOW_OK)
    goto push_error;

done:

  return ret;

  /* ERRORS */
push_error:
  {
    GST_DEBUG_OBJECT (rtpsession, "process returned %s",
        gst_flow_get_name (ret));
    goto done;
  }
}

static GstFlowReturn
gst_rtp_session_chain_send_rtp (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{
  GstRtpSession *rtpsession = GST_RTP_SESSION (parent);

  return gst_rtp_session_chain_send_rtp_common (rtpsession, buffer, FALSE);
}

static GstFlowReturn
gst_rtp_session_chain_send_rtp_list (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstRtpSession *rtpsession = GST_RTP_SESSION (parent);

  return gst_rtp_session_chain_send_rtp_common (rtpsession, list, TRUE);
}

/* Create sinkpad to receive RTP packets from senders. This will also create a
 * srcpad for the RTP packets.
 */
static GstPad *
create_recv_rtp_sink (GstRtpSession * rtpsession)
{
  GST_DEBUG_OBJECT (rtpsession, "creating RTP sink pad");

  rtpsession->recv_rtp_sink =
      gst_pad_new_from_static_template (&rtpsession_recv_rtp_sink_template,
      "recv_rtp_sink");
  gst_pad_set_chain_function (rtpsession->recv_rtp_sink,
      gst_rtp_session_chain_recv_rtp);
  gst_pad_set_chain_list_function (rtpsession->recv_rtp_sink,
      gst_rtp_session_chain_recv_rtp_list);
  gst_pad_set_event_function (rtpsession->recv_rtp_sink,
      gst_rtp_session_event_recv_rtp_sink);
  gst_pad_set_iterate_internal_links_function (rtpsession->recv_rtp_sink,
      gst_rtp_session_iterate_internal_links);
  GST_PAD_SET_PROXY_ALLOCATION (rtpsession->recv_rtp_sink);
  gst_pad_set_active (rtpsession->recv_rtp_sink, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->recv_rtp_sink);

  GST_DEBUG_OBJECT (rtpsession, "creating RTP src pad");
  rtpsession->recv_rtp_src =
      gst_pad_new_from_static_template (&rtpsession_recv_rtp_src_template,
      "recv_rtp_src");
  gst_pad_set_event_function (rtpsession->recv_rtp_src,
      gst_rtp_session_event_recv_rtp_src);
  gst_pad_set_iterate_internal_links_function (rtpsession->recv_rtp_src,
      gst_rtp_session_iterate_internal_links);
  gst_pad_use_fixed_caps (rtpsession->recv_rtp_src);
  gst_pad_set_active (rtpsession->recv_rtp_src, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpsession), rtpsession->recv_rtp_src);

  return rtpsession->recv_rtp_sink;
}

/* Remove sinkpad to receive RTP packets from senders. This will also remove
 * the srcpad for the RTP packets.
 */
static void
remove_recv_rtp_sink (GstRtpSession * rtpsession)
{
  GST_DEBUG_OBJECT (rtpsession, "removing RTP sink pad");

  /* deactivate from source to sink */
  gst_pad_set_active (rtpsession->recv_rtp_src, FALSE);
  gst_pad_set_active (rtpsession->recv_rtp_sink, FALSE);

  /* remove pads */
  gst_element_remove_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->recv_rtp_sink);
  rtpsession->recv_rtp_sink = NULL;

  GST_DEBUG_OBJECT (rtpsession, "removing RTP src pad");
  gst_element_remove_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->recv_rtp_src);
  rtpsession->recv_rtp_src = NULL;
}

/* Create a sinkpad to receive RTCP messages from senders, this will also create a
 * sync_src pad for the SR packets.
 */
static GstPad *
create_recv_rtcp_sink (GstRtpSession * rtpsession)
{
  GST_DEBUG_OBJECT (rtpsession, "creating RTCP sink pad");

  rtpsession->recv_rtcp_sink =
      gst_pad_new_from_static_template (&rtpsession_recv_rtcp_sink_template,
      "recv_rtcp_sink");
  gst_pad_set_chain_function (rtpsession->recv_rtcp_sink,
      gst_rtp_session_chain_recv_rtcp);
  gst_pad_set_event_function (rtpsession->recv_rtcp_sink,
      gst_rtp_session_event_recv_rtcp_sink);
  gst_pad_set_iterate_internal_links_function (rtpsession->recv_rtcp_sink,
      gst_rtp_session_iterate_internal_links);
  gst_pad_set_active (rtpsession->recv_rtcp_sink, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->recv_rtcp_sink);

  GST_DEBUG_OBJECT (rtpsession, "creating sync src pad");
  rtpsession->sync_src =
      gst_pad_new_from_static_template (&rtpsession_sync_src_template,
      "sync_src");
  gst_pad_set_iterate_internal_links_function (rtpsession->sync_src,
      gst_rtp_session_iterate_internal_links);
  gst_pad_use_fixed_caps (rtpsession->sync_src);
  gst_pad_set_active (rtpsession->sync_src, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpsession), rtpsession->sync_src);

  return rtpsession->recv_rtcp_sink;
}

static void
remove_recv_rtcp_sink (GstRtpSession * rtpsession)
{
  GST_DEBUG_OBJECT (rtpsession, "removing RTCP sink pad");

  gst_pad_set_active (rtpsession->sync_src, FALSE);
  gst_pad_set_active (rtpsession->recv_rtcp_sink, FALSE);

  gst_element_remove_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->recv_rtcp_sink);
  rtpsession->recv_rtcp_sink = NULL;

  GST_DEBUG_OBJECT (rtpsession, "removing sync src pad");
  gst_element_remove_pad (GST_ELEMENT_CAST (rtpsession), rtpsession->sync_src);
  rtpsession->sync_src = NULL;
}

/* Create a sinkpad to receive RTP packets for receivers. This will also create a
 * send_rtp_src pad.
 */
static GstPad *
create_send_rtp_sink (GstRtpSession * rtpsession)
{
  GST_DEBUG_OBJECT (rtpsession, "creating pad");

  rtpsession->send_rtp_sink =
      gst_pad_new_from_static_template (&rtpsession_send_rtp_sink_template,
      "send_rtp_sink");
  gst_pad_set_chain_function (rtpsession->send_rtp_sink,
      gst_rtp_session_chain_send_rtp);
  gst_pad_set_chain_list_function (rtpsession->send_rtp_sink,
      gst_rtp_session_chain_send_rtp_list);
  gst_pad_set_query_function (rtpsession->send_rtp_sink,
      gst_rtp_session_query_send_rtp);
  gst_pad_set_event_function (rtpsession->send_rtp_sink,
      gst_rtp_session_event_send_rtp_sink);
  gst_pad_set_iterate_internal_links_function (rtpsession->send_rtp_sink,
      gst_rtp_session_iterate_internal_links);
  GST_PAD_SET_PROXY_CAPS (rtpsession->send_rtp_sink);
  GST_PAD_SET_PROXY_ALLOCATION (rtpsession->send_rtp_sink);
  gst_pad_set_active (rtpsession->send_rtp_sink, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->send_rtp_sink);

  rtpsession->send_rtp_src =
      gst_pad_new_from_static_template (&rtpsession_send_rtp_src_template,
      "send_rtp_src");
  gst_pad_set_iterate_internal_links_function (rtpsession->send_rtp_src,
      gst_rtp_session_iterate_internal_links);
  gst_pad_set_event_function (rtpsession->send_rtp_src,
      gst_rtp_session_event_send_rtp_src);
  GST_PAD_SET_PROXY_CAPS (rtpsession->send_rtp_src);
  gst_pad_set_active (rtpsession->send_rtp_src, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpsession), rtpsession->send_rtp_src);

  return rtpsession->send_rtp_sink;
}

static void
remove_send_rtp_sink (GstRtpSession * rtpsession)
{
  GST_DEBUG_OBJECT (rtpsession, "removing pad");

  gst_pad_set_active (rtpsession->send_rtp_src, FALSE);
  gst_pad_set_active (rtpsession->send_rtp_sink, FALSE);

  gst_element_remove_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->send_rtp_sink);
  rtpsession->send_rtp_sink = NULL;

  gst_element_remove_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->send_rtp_src);
  rtpsession->send_rtp_src = NULL;
}

/* Create a srcpad with the RTCP packets to send out.
 * This pad will be driven by the RTP session manager when it wants to send out
 * RTCP packets.
 */
static GstPad *
create_send_rtcp_src (GstRtpSession * rtpsession)
{
  GST_DEBUG_OBJECT (rtpsession, "creating pad");

  rtpsession->send_rtcp_src =
      gst_pad_new_from_static_template (&rtpsession_send_rtcp_src_template,
      "send_rtcp_src");
  gst_pad_use_fixed_caps (rtpsession->send_rtcp_src);
  gst_pad_set_active (rtpsession->send_rtcp_src, TRUE);
  gst_pad_set_iterate_internal_links_function (rtpsession->send_rtcp_src,
      gst_rtp_session_iterate_internal_links);
  gst_pad_set_query_function (rtpsession->send_rtcp_src,
      gst_rtp_session_query_send_rtcp_src);
  gst_pad_set_event_function (rtpsession->send_rtcp_src,
      gst_rtp_session_event_send_rtcp_src);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->send_rtcp_src);

  return rtpsession->send_rtcp_src;
}

static void
remove_send_rtcp_src (GstRtpSession * rtpsession)
{
  GST_DEBUG_OBJECT (rtpsession, "removing pad");

  gst_pad_set_active (rtpsession->send_rtcp_src, FALSE);

  gst_element_remove_pad (GST_ELEMENT_CAST (rtpsession),
      rtpsession->send_rtcp_src);
  rtpsession->send_rtcp_src = NULL;
}

static GstPad *
gst_rtp_session_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstRtpSession *rtpsession;
  GstElementClass *klass;
  GstPad *result;

  g_return_val_if_fail (templ != NULL, NULL);
  g_return_val_if_fail (GST_IS_RTP_SESSION (element), NULL);

  rtpsession = GST_RTP_SESSION (element);
  klass = GST_ELEMENT_GET_CLASS (element);

  GST_DEBUG_OBJECT (element, "requesting pad %s", GST_STR_NULL (name));

  GST_RTP_SESSION_LOCK (rtpsession);

  /* figure out the template */
  if (templ == gst_element_class_get_pad_template (klass, "recv_rtp_sink")) {
    if (rtpsession->recv_rtp_sink != NULL)
      goto exists;

    result = create_recv_rtp_sink (rtpsession);
  } else if (templ == gst_element_class_get_pad_template (klass,
          "recv_rtcp_sink")) {
    if (rtpsession->recv_rtcp_sink != NULL)
      goto exists;

    result = create_recv_rtcp_sink (rtpsession);
  } else if (templ == gst_element_class_get_pad_template (klass,
          "send_rtp_sink")) {
    if (rtpsession->send_rtp_sink != NULL)
      goto exists;

    result = create_send_rtp_sink (rtpsession);
  } else if (templ == gst_element_class_get_pad_template (klass,
          "send_rtcp_src")) {
    if (rtpsession->send_rtcp_src != NULL)
      goto exists;

    result = create_send_rtcp_src (rtpsession);
  } else
    goto wrong_template;

  GST_RTP_SESSION_UNLOCK (rtpsession);

  return result;

  /* ERRORS */
wrong_template:
  {
    GST_RTP_SESSION_UNLOCK (rtpsession);
    g_warning ("rtpsession: this is not our template");
    return NULL;
  }
exists:
  {
    GST_RTP_SESSION_UNLOCK (rtpsession);
    g_warning ("rtpsession: pad already requested");
    return NULL;
  }
}

static void
gst_rtp_session_release_pad (GstElement * element, GstPad * pad)
{
  GstRtpSession *rtpsession;

  g_return_if_fail (GST_IS_RTP_SESSION (element));
  g_return_if_fail (GST_IS_PAD (pad));

  rtpsession = GST_RTP_SESSION (element);

  GST_DEBUG_OBJECT (element, "releasing pad %s:%s", GST_DEBUG_PAD_NAME (pad));

  GST_RTP_SESSION_LOCK (rtpsession);

  if (rtpsession->recv_rtp_sink == pad) {
    remove_recv_rtp_sink (rtpsession);
  } else if (rtpsession->recv_rtcp_sink == pad) {
    remove_recv_rtcp_sink (rtpsession);
  } else if (rtpsession->send_rtp_sink == pad) {
    remove_send_rtp_sink (rtpsession);
  } else if (rtpsession->send_rtcp_src == pad) {
    remove_send_rtcp_src (rtpsession);
  } else
    goto wrong_pad;

  GST_RTP_SESSION_UNLOCK (rtpsession);

  return;

  /* ERRORS */
wrong_pad:
  {
    GST_RTP_SESSION_UNLOCK (rtpsession);
    g_warning ("rtpsession: asked to release an unknown pad");
    return;
  }
}

static void
gst_rtp_session_request_key_unit (RTPSession * sess,
    guint32 ssrc, gboolean all_headers, gpointer user_data)
{
  GstRtpSession *rtpsession = GST_RTP_SESSION (user_data);
  GstEvent *event;
  GstPad *send_rtp_sink;

  GST_RTP_SESSION_LOCK (rtpsession);
  if ((send_rtp_sink = rtpsession->send_rtp_sink))
    gst_object_ref (send_rtp_sink);
  GST_RTP_SESSION_UNLOCK (rtpsession);

  if (send_rtp_sink) {
    event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
        gst_structure_new ("GstForceKeyUnit", "ssrc", G_TYPE_UINT, ssrc,
            "all-headers", G_TYPE_BOOLEAN, all_headers, NULL));
    gst_pad_push_event (send_rtp_sink, event);
    gst_object_unref (send_rtp_sink);
  }
}

static GstClockTime
gst_rtp_session_request_time (RTPSession * session, gpointer user_data)
{
  GstRtpSession *rtpsession = GST_RTP_SESSION (user_data);

  return gst_clock_get_time (rtpsession->priv->sysclock);
}

static void
gst_rtp_session_notify_nack (RTPSession * sess, guint16 seqnum,
    guint16 blp, guint32 ssrc, gpointer user_data)
{
  GstRtpSession *rtpsession = GST_RTP_SESSION (user_data);
  GstEvent *event;
  GstPad *send_rtp_sink;

  GST_RTP_SESSION_LOCK (rtpsession);
  if ((send_rtp_sink = rtpsession->send_rtp_sink))
    gst_object_ref (send_rtp_sink);
  GST_RTP_SESSION_UNLOCK (rtpsession);

  if (send_rtp_sink) {
    while (TRUE) {
      event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
          gst_structure_new ("GstRTPRetransmissionRequest",
              "seqnum", G_TYPE_UINT, (guint) seqnum,
              "ssrc", G_TYPE_UINT, (guint) ssrc, NULL));
      gst_pad_push_event (send_rtp_sink, event);

      GST_RTP_SESSION_LOCK (rtpsession);
      rtpsession->priv->sent_rtx_req_count++;
      GST_RTP_SESSION_UNLOCK (rtpsession);

      if (blp == 0)
        break;

      seqnum++;
      while ((blp & 1) == 0) {
        seqnum++;
        blp >>= 1;
      }
      blp >>= 1;
    }
    gst_object_unref (send_rtp_sink);
  }
}

static void
gst_rtp_session_reconfigure (RTPSession * sess, gpointer user_data)
{
  GstRtpSession *rtpsession = GST_RTP_SESSION (user_data);
  GstPad *send_rtp_sink;

  GST_RTP_SESSION_LOCK (rtpsession);
  if ((send_rtp_sink = rtpsession->send_rtp_sink))
    gst_object_ref (send_rtp_sink);
  GST_RTP_SESSION_UNLOCK (rtpsession);

  if (send_rtp_sink) {
    gst_pad_push_event (send_rtp_sink, gst_event_new_reconfigure ());
    gst_object_unref (send_rtp_sink);
  }
}

static void
gst_rtp_session_notify_early_rtcp (RTPSession * sess, gpointer user_data)
{
  GstRtpSession *rtpsession = GST_RTP_SESSION (user_data);

  GST_DEBUG_OBJECT (rtpsession, "Notified of early RTCP");
  /* with an early RTCP request, we might have to start the RTCP thread */
  GST_RTP_SESSION_LOCK (rtpsession);
  signal_waiting_rtcp_thread_unlocked (rtpsession);
  GST_RTP_SESSION_UNLOCK (rtpsession);
}
