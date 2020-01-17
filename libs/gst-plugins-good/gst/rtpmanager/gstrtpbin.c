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
 * SECTION:element-rtpbin
 * @title: rtpbin
 * @see_also: rtpjitterbuffer, rtpsession, rtpptdemux, rtpssrcdemux
 *
 * RTP bin combines the functions of #GstRtpSession, #GstRtpSsrcDemux,
 * #GstRtpJitterBuffer and #GstRtpPtDemux in one element. It allows for multiple
 * RTP sessions that will be synchronized together using RTCP SR packets.
 *
 * #GstRtpBin is configured with a number of request pads that define the
 * functionality that is activated, similar to the #GstRtpSession element.
 *
 * To use #GstRtpBin as an RTP receiver, request a recv_rtp_sink_\%u pad. The session
 * number must be specified in the pad name.
 * Data received on the recv_rtp_sink_\%u pad will be processed in the #GstRtpSession
 * manager and after being validated forwarded on #GstRtpSsrcDemux element. Each
 * RTP stream is demuxed based on the SSRC and send to a #GstRtpJitterBuffer. After
 * the packets are released from the jitterbuffer, they will be forwarded to a
 * #GstRtpPtDemux element. The #GstRtpPtDemux element will demux the packets based
 * on the payload type and will create a unique pad recv_rtp_src_\%u_\%u_\%u on
 * rtpbin with the session number, SSRC and payload type respectively as the pad
 * name.
 *
 * To also use #GstRtpBin as an RTCP receiver, request a recv_rtcp_sink_\%u pad. The
 * session number must be specified in the pad name.
 *
 * If you want the session manager to generate and send RTCP packets, request
 * the send_rtcp_src_\%u pad with the session number in the pad name. Packet pushed
 * on this pad contain SR/RR RTCP reports that should be sent to all participants
 * in the session.
 *
 * To use #GstRtpBin as a sender, request a send_rtp_sink_\%u pad, which will
 * automatically create a send_rtp_src_\%u pad. If the session number is not provided,
 * the pad from the lowest available session will be returned. The session manager will modify the
 * SSRC in the RTP packets to its own SSRC and will forward the packets on the
 * send_rtp_src_\%u pad after updating its internal state.
 *
 * The session manager needs the clock-rate of the payload types it is handling
 * and will signal the #GstRtpSession::request-pt-map signal when it needs such a
 * mapping. One can clear the cached values with the #GstRtpSession::clear-pt-map
 * signal.
 *
 * Access to the internal statistics of rtpbin is provided with the
 * get-internal-session property. This action signal gives access to the
 * RTPSession object which further provides action signals to retrieve the
 * internal source and other sources.
 *
 * #GstRtpBin also has signals (#GstRtpBin::request-rtp-encoder,
 * #GstRtpBin::request-rtp-decoder, #GstRtpBin::request-rtcp-encoder and
 * #GstRtpBin::request-rtp-decoder) to dynamically request for RTP and RTCP encoders
 * and decoders in order to support SRTP. The encoders must provide the pads
 * rtp_sink_\%u and rtp_src_\%u for RTP and rtcp_sink_\%u and rtcp_src_\%u for
 * RTCP. The session number will be used in the pad name. The decoders must provide
 * rtp_sink and rtp_src for RTP and rtcp_sink and rtcp_src for RTCP. The decoders will
 * be placed before the #GstRtpSession element, thus they must support SSRC demuxing
 * internally.
 *
 * #GstRtpBin has signals (#GstRtpBin::request-aux-sender and
 * #GstRtpBin::request-aux-receiver to dynamically request an element that can be
 * used to create or merge additional RTP streams. AUX elements are needed to
 * implement FEC or retransmission (such as RFC 4588). An AUX sender must have one
 * sink_\%u pad that matches the sessionid in the signal and it should have 1 or
 * more src_\%u pads. For each src_%\u pad, a session will be made (if needed)
 * and the pad will be linked to the session send_rtp_sink pad. Each session will
 * then expose its source pad as send_rtp_src_\%u on #GstRtpBin.
 * An AUX receiver has 1 src_\%u pad that much match the sessionid in the signal
 * and 1 or more sink_\%u pads. A session will be made for each sink_\%u pad
 * when the corresponding recv_rtp_sink_\%u pad is requested on #GstRtpBin.
 * The #GstRtpBin::request-jitterbuffer signal can be used to provide a custom
 * element to perform arrival time smoothing, reordering and optionally packet
 * loss detection and retransmission requests.
 *
 * ## Example pipelines
 *
 * |[
 * gst-launch-1.0 udpsrc port=5000 caps="application/x-rtp, ..." ! .recv_rtp_sink_0 \
 *     rtpbin ! rtptheoradepay ! theoradec ! xvimagesink
 * ]| Receive RTP data from port 5000 and send to the session 0 in rtpbin.
 * |[
 * gst-launch-1.0 rtpbin name=rtpbin \
 *         v4l2src ! videoconvert ! ffenc_h263 ! rtph263ppay ! rtpbin.send_rtp_sink_0 \
 *                   rtpbin.send_rtp_src_0 ! udpsink port=5000                            \
 *                   rtpbin.send_rtcp_src_0 ! udpsink port=5001 sync=false async=false    \
 *                   udpsrc port=5005 ! rtpbin.recv_rtcp_sink_0                           \
 *         audiotestsrc ! amrnbenc ! rtpamrpay ! rtpbin.send_rtp_sink_1                   \
 *                   rtpbin.send_rtp_src_1 ! udpsink port=5002                            \
 *                   rtpbin.send_rtcp_src_1 ! udpsink port=5003 sync=false async=false    \
 *                   udpsrc port=5007 ! rtpbin.recv_rtcp_sink_1
 * ]| Encode and payload H263 video captured from a v4l2src. Encode and payload AMR
 * audio generated from audiotestsrc. The video is sent to session 0 in rtpbin
 * and the audio is sent to session 1. Video packets are sent on UDP port 5000
 * and audio packets on port 5002. The video RTCP packets for session 0 are sent
 * on port 5001 and the audio RTCP packets for session 0 are sent on port 5003.
 * RTCP packets for session 0 are received on port 5005 and RTCP for session 1
 * is received on port 5007. Since RTCP packets from the sender should be sent
 * as soon as possible and do not participate in preroll, sync=false and
 * async=false is configured on udpsink
 * |[
 * gst-launch-1.0 -v rtpbin name=rtpbin                                          \
 *     udpsrc caps="application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H263-1998" \
 *             port=5000 ! rtpbin.recv_rtp_sink_0                                \
 *         rtpbin. ! rtph263pdepay ! ffdec_h263 ! xvimagesink                    \
 *      udpsrc port=5001 ! rtpbin.recv_rtcp_sink_0                               \
 *      rtpbin.send_rtcp_src_0 ! udpsink port=5005 sync=false async=false        \
 *     udpsrc caps="application/x-rtp,media=(string)audio,clock-rate=(int)8000,encoding-name=(string)AMR,encoding-params=(string)1,octet-align=(string)1" \
 *             port=5002 ! rtpbin.recv_rtp_sink_1                                \
 *         rtpbin. ! rtpamrdepay ! amrnbdec ! alsasink                           \
 *      udpsrc port=5003 ! rtpbin.recv_rtcp_sink_1                               \
 *      rtpbin.send_rtcp_src_1 ! udpsink port=5007 sync=false async=false
 * ]| Receive H263 on port 5000, send it through rtpbin in session 0, depayload,
 * decode and display the video.
 * Receive AMR on port 5002, send it through rtpbin in session 1, depayload,
 * decode and play the audio.
 * Receive server RTCP packets for session 0 on port 5001 and RTCP packets for
 * session 1 on port 5003. These packets will be used for session management and
 * synchronisation.
 * Send RTCP reports for session 0 on port 5005 and RTCP reports for session 1
 * on port 5007.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/rtp/gstrtcpbuffer.h>

#include "gstrtpbin.h"
#include "rtpsession.h"
#include "gstrtpsession.h"
#include "gstrtpjitterbuffer.h"

#include <gst/glib-compat-private.h>

GST_DEBUG_CATEGORY_STATIC (gst_rtp_bin_debug);
#define GST_CAT_DEFAULT gst_rtp_bin_debug

/* sink pads */
static GstStaticPadTemplate rtpbin_recv_rtp_sink_template =
    GST_STATIC_PAD_TEMPLATE ("recv_rtp_sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtp;application/x-srtp")
    );

static GstStaticPadTemplate rtpbin_recv_rtcp_sink_template =
    GST_STATIC_PAD_TEMPLATE ("recv_rtcp_sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtcp;application/x-srtcp")
    );

static GstStaticPadTemplate rtpbin_send_rtp_sink_template =
GST_STATIC_PAD_TEMPLATE ("send_rtp_sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtp")
    );

/* src pads */
static GstStaticPadTemplate rtpbin_recv_rtp_src_template =
GST_STATIC_PAD_TEMPLATE ("recv_rtp_src_%u_%u_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate rtpbin_send_rtcp_src_template =
    GST_STATIC_PAD_TEMPLATE ("send_rtcp_src_%u",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtcp;application/x-srtcp")
    );

static GstStaticPadTemplate rtpbin_send_rtp_src_template =
    GST_STATIC_PAD_TEMPLATE ("send_rtp_src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtp;application/x-srtp")
    );

#define GST_RTP_BIN_LOCK(bin)   g_mutex_lock (&(bin)->priv->bin_lock)
#define GST_RTP_BIN_UNLOCK(bin) g_mutex_unlock (&(bin)->priv->bin_lock)

/* lock to protect dynamic callbacks, like pad-added and new ssrc. */
#define GST_RTP_BIN_DYN_LOCK(bin)    g_mutex_lock (&(bin)->priv->dyn_lock)
#define GST_RTP_BIN_DYN_UNLOCK(bin)  g_mutex_unlock (&(bin)->priv->dyn_lock)

/* lock for shutdown */
#define GST_RTP_BIN_SHUTDOWN_LOCK(bin,label)     \
G_STMT_START {                                   \
  if (g_atomic_int_get (&bin->priv->shutdown))   \
    goto label;                                  \
  GST_RTP_BIN_DYN_LOCK (bin);                    \
  if (g_atomic_int_get (&bin->priv->shutdown)) { \
    GST_RTP_BIN_DYN_UNLOCK (bin);                \
    goto label;                                  \
  }                                              \
} G_STMT_END

/* unlock for shutdown */
#define GST_RTP_BIN_SHUTDOWN_UNLOCK(bin)         \
  GST_RTP_BIN_DYN_UNLOCK (bin);                  \

/* Minimum time offset to apply. This compensates for rounding errors in NTP to
 * RTP timestamp conversions */
#define MIN_TS_OFFSET (4 * GST_MSECOND)

struct _GstRtpBinPrivate
{
  GMutex bin_lock;

  /* lock protecting dynamic adding/removing */
  GMutex dyn_lock;

  /* if we are shutting down or not */
  gint shutdown;

  gboolean autoremove;

  /* NTP time in ns of last SR sync used */
  guint64 last_ntpnstime;

  /* list of extra elements */
  GList *elements;
};

/* signals and args */
enum
{
  SIGNAL_REQUEST_PT_MAP,
  SIGNAL_PAYLOAD_TYPE_CHANGE,
  SIGNAL_CLEAR_PT_MAP,
  SIGNAL_RESET_SYNC,
  SIGNAL_GET_SESSION,
  SIGNAL_GET_INTERNAL_SESSION,
  SIGNAL_GET_STORAGE,
  SIGNAL_GET_INTERNAL_STORAGE,

  SIGNAL_ON_NEW_SSRC,
  SIGNAL_ON_SSRC_COLLISION,
  SIGNAL_ON_SSRC_VALIDATED,
  SIGNAL_ON_SSRC_ACTIVE,
  SIGNAL_ON_SSRC_SDES,
  SIGNAL_ON_BYE_SSRC,
  SIGNAL_ON_BYE_TIMEOUT,
  SIGNAL_ON_TIMEOUT,
  SIGNAL_ON_SENDER_TIMEOUT,
  SIGNAL_ON_NPT_STOP,

  SIGNAL_REQUEST_RTP_ENCODER,
  SIGNAL_REQUEST_RTP_DECODER,
  SIGNAL_REQUEST_RTCP_ENCODER,
  SIGNAL_REQUEST_RTCP_DECODER,

  SIGNAL_REQUEST_FEC_DECODER,
  SIGNAL_REQUEST_FEC_ENCODER,

  SIGNAL_REQUEST_JITTERBUFFER,

  SIGNAL_NEW_JITTERBUFFER,
  SIGNAL_NEW_STORAGE,

  SIGNAL_REQUEST_AUX_SENDER,
  SIGNAL_REQUEST_AUX_RECEIVER,

  SIGNAL_ON_NEW_SENDER_SSRC,
  SIGNAL_ON_SENDER_SSRC_ACTIVE,

  SIGNAL_ON_BUNDLED_SSRC,

  LAST_SIGNAL
};

#define DEFAULT_LATENCY_MS           200
#define DEFAULT_DROP_ON_LATENCY      FALSE
#define DEFAULT_SDES                 NULL
#define DEFAULT_DO_LOST              FALSE
#define DEFAULT_IGNORE_PT            FALSE
#define DEFAULT_NTP_SYNC             FALSE
#define DEFAULT_AUTOREMOVE           FALSE
#define DEFAULT_BUFFER_MODE          RTP_JITTER_BUFFER_MODE_SLAVE
#define DEFAULT_USE_PIPELINE_CLOCK   FALSE
#define DEFAULT_RTCP_SYNC            GST_RTP_BIN_RTCP_SYNC_ALWAYS
#define DEFAULT_RTCP_SYNC_INTERVAL   0
#define DEFAULT_DO_SYNC_EVENT        FALSE
#define DEFAULT_DO_RETRANSMISSION    FALSE
#define DEFAULT_RTP_PROFILE          GST_RTP_PROFILE_AVP
#define DEFAULT_NTP_TIME_SOURCE      GST_RTP_NTP_TIME_SOURCE_NTP
#define DEFAULT_RTCP_SYNC_SEND_TIME  TRUE
#define DEFAULT_MAX_RTCP_RTP_TIME_DIFF 1000
#define DEFAULT_MAX_DROPOUT_TIME     60000
#define DEFAULT_MAX_MISORDER_TIME    2000
#define DEFAULT_RFC7273_SYNC         FALSE
#define DEFAULT_MAX_STREAMS          G_MAXUINT
#define DEFAULT_MAX_TS_OFFSET_ADJUSTMENT G_GUINT64_CONSTANT(0)
#define DEFAULT_MAX_TS_OFFSET        G_GINT64_CONSTANT(3000000000)

enum
{
  PROP_0,
  PROP_LATENCY,
  PROP_DROP_ON_LATENCY,
  PROP_SDES,
  PROP_DO_LOST,
  PROP_IGNORE_PT,
  PROP_NTP_SYNC,
  PROP_RTCP_SYNC,
  PROP_RTCP_SYNC_INTERVAL,
  PROP_AUTOREMOVE,
  PROP_BUFFER_MODE,
  PROP_USE_PIPELINE_CLOCK,
  PROP_DO_SYNC_EVENT,
  PROP_DO_RETRANSMISSION,
  PROP_RTP_PROFILE,
  PROP_NTP_TIME_SOURCE,
  PROP_RTCP_SYNC_SEND_TIME,
  PROP_MAX_RTCP_RTP_TIME_DIFF,
  PROP_MAX_DROPOUT_TIME,
  PROP_MAX_MISORDER_TIME,
  PROP_RFC7273_SYNC,
  PROP_MAX_STREAMS,
  PROP_MAX_TS_OFFSET_ADJUSTMENT,
  PROP_MAX_TS_OFFSET,
};

#define GST_RTP_BIN_RTCP_SYNC_TYPE (gst_rtp_bin_rtcp_sync_get_type())
static GType
gst_rtp_bin_rtcp_sync_get_type (void)
{
  static GType rtcp_sync_type = 0;
  static const GEnumValue rtcp_sync_types[] = {
    {GST_RTP_BIN_RTCP_SYNC_ALWAYS, "always", "always"},
    {GST_RTP_BIN_RTCP_SYNC_INITIAL, "initial", "initial"},
    {GST_RTP_BIN_RTCP_SYNC_RTP, "rtp-info", "rtp-info"},
    {0, NULL, NULL},
  };

  if (!rtcp_sync_type) {
    rtcp_sync_type = g_enum_register_static ("GstRTCPSync", rtcp_sync_types);
  }
  return rtcp_sync_type;
}

/* helper objects */
typedef struct _GstRtpBinSession GstRtpBinSession;
typedef struct _GstRtpBinStream GstRtpBinStream;
typedef struct _GstRtpBinClient GstRtpBinClient;

static guint gst_rtp_bin_signals[LAST_SIGNAL] = { 0 };

static GstCaps *pt_map_requested (GstElement * element, guint pt,
    GstRtpBinSession * session);
static void payload_type_change (GstElement * element, guint pt,
    GstRtpBinSession * session);
static void remove_recv_rtp (GstRtpBin * rtpbin, GstRtpBinSession * session);
static void remove_recv_rtcp (GstRtpBin * rtpbin, GstRtpBinSession * session);
static void remove_send_rtp (GstRtpBin * rtpbin, GstRtpBinSession * session);
static void remove_rtcp (GstRtpBin * rtpbin, GstRtpBinSession * session);
static void free_client (GstRtpBinClient * client, GstRtpBin * bin);
static void free_stream (GstRtpBinStream * stream, GstRtpBin * bin);
static GstRtpBinSession *create_session (GstRtpBin * rtpbin, gint id);
static GstPad *complete_session_sink (GstRtpBin * rtpbin,
    GstRtpBinSession * session);
static void
complete_session_receiver (GstRtpBin * rtpbin, GstRtpBinSession * session,
    guint sessid);
static GstPad *complete_session_rtcp (GstRtpBin * rtpbin,
    GstRtpBinSession * session, guint sessid);
static GstElement *session_request_element (GstRtpBinSession * session,
    guint signal);

/* Manages the RTP stream for one SSRC.
 *
 * We pipe the stream (coming from the SSRC demuxer) into a jitterbuffer.
 * If we see an SDES RTCP packet that links multiple SSRCs together based on a
 * common CNAME, we create a GstRtpBinClient structure to group the SSRCs
 * together (see below).
 */
struct _GstRtpBinStream
{
  /* the SSRC of this stream */
  guint32 ssrc;

  /* parent bin */
  GstRtpBin *bin;

  /* the session this SSRC belongs to */
  GstRtpBinSession *session;

  /* the jitterbuffer of the SSRC */
  GstElement *buffer;
  gulong buffer_handlesync_sig;
  gulong buffer_ptreq_sig;
  gulong buffer_ntpstop_sig;
  gint percent;

  /* the PT demuxer of the SSRC */
  GstElement *demux;
  gulong demux_newpad_sig;
  gulong demux_padremoved_sig;
  gulong demux_ptreq_sig;
  gulong demux_ptchange_sig;

  /* if we have calculated a valid rt_delta for this stream */
  gboolean have_sync;
  /* mapping to local RTP and NTP time */
  gint64 rt_delta;
  gint64 rtp_delta;
  /* base rtptime in gst time */
  gint64 clock_base;
};

#define GST_RTP_SESSION_LOCK(sess)   g_mutex_lock (&(sess)->lock)
#define GST_RTP_SESSION_UNLOCK(sess) g_mutex_unlock (&(sess)->lock)

/* Manages the receiving end of the packets.
 *
 * There is one such structure for each RTP session (audio/video/...).
 * We get the RTP/RTCP packets and stuff them into the session manager. From
 * there they are pushed into an SSRC demuxer that splits the stream based on
 * SSRC. Each of the SSRC streams go into their own jitterbuffer (managed with
 * the GstRtpBinStream above).
 *
 * Before the SSRC demuxer, a storage element may be inserted for the purpose
 * of Forward Error Correction.
 */
struct _GstRtpBinSession
{
  /* session id */
  gint id;
  /* the parent bin */
  GstRtpBin *bin;
  /* the session element */
  GstElement *session;
  /* the SSRC demuxer */
  GstElement *demux;
  gulong demux_newpad_sig;
  gulong demux_padremoved_sig;

  /* Fec support */
  GstElement *storage;

  GMutex lock;

  /* list of GstRtpBinStream */
  GSList *streams;

  /* list of elements */
  GSList *elements;

  /* mapping of payload type to caps */
  GHashTable *ptmap;

  /* the pads of the session */
  GstPad *recv_rtp_sink;
  GstPad *recv_rtp_sink_ghost;
  GstPad *recv_rtp_src;
  GstPad *recv_rtcp_sink;
  GstPad *recv_rtcp_sink_ghost;
  GstPad *sync_src;
  GstPad *send_rtp_sink;
  GstPad *send_rtp_sink_ghost;
  GstPad *send_rtp_src_ghost;
  GstPad *send_rtcp_src;
  GstPad *send_rtcp_src_ghost;
};

/* Manages the RTP streams that come from one client and should therefore be
 * synchronized.
 */
struct _GstRtpBinClient
{
  /* the common CNAME for the streams */
  gchar *cname;
  guint cname_len;

  /* the streams */
  guint nstreams;
  GSList *streams;
};

/* find a session with the given id. Must be called with RTP_BIN_LOCK */
static GstRtpBinSession *
find_session_by_id (GstRtpBin * rtpbin, gint id)
{
  GSList *walk;

  for (walk = rtpbin->sessions; walk; walk = g_slist_next (walk)) {
    GstRtpBinSession *sess = (GstRtpBinSession *) walk->data;

    if (sess->id == id)
      return sess;
  }
  return NULL;
}

/* find a session with the given request pad. Must be called with RTP_BIN_LOCK */
static GstRtpBinSession *
find_session_by_pad (GstRtpBin * rtpbin, GstPad * pad)
{
  GSList *walk;

  for (walk = rtpbin->sessions; walk; walk = g_slist_next (walk)) {
    GstRtpBinSession *sess = (GstRtpBinSession *) walk->data;

    if ((sess->recv_rtp_sink_ghost == pad) ||
        (sess->recv_rtcp_sink_ghost == pad) ||
        (sess->send_rtp_sink_ghost == pad)
        || (sess->send_rtcp_src_ghost == pad))
      return sess;
  }
  return NULL;
}

static void
on_new_ssrc (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_NEW_SSRC], 0,
      sess->id, ssrc);
}

static void
on_ssrc_collision (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_SSRC_COLLISION], 0,
      sess->id, ssrc);
}

static void
on_ssrc_validated (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_SSRC_VALIDATED], 0,
      sess->id, ssrc);
}

static void
on_ssrc_active (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_SSRC_ACTIVE], 0,
      sess->id, ssrc);
}

static void
on_ssrc_sdes (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_SSRC_SDES], 0,
      sess->id, ssrc);
}

static void
on_bye_ssrc (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_BYE_SSRC], 0,
      sess->id, ssrc);
}

static void
on_bye_timeout (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_BYE_TIMEOUT], 0,
      sess->id, ssrc);

  if (sess->bin->priv->autoremove)
    g_signal_emit_by_name (sess->demux, "clear-ssrc", ssrc, NULL);
}

static void
on_timeout (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_TIMEOUT], 0,
      sess->id, ssrc);

  if (sess->bin->priv->autoremove)
    g_signal_emit_by_name (sess->demux, "clear-ssrc", ssrc, NULL);
}

static void
on_sender_timeout (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_SENDER_TIMEOUT], 0,
      sess->id, ssrc);
}

static void
on_npt_stop (GstElement * jbuf, GstRtpBinStream * stream)
{
  g_signal_emit (stream->bin, gst_rtp_bin_signals[SIGNAL_ON_NPT_STOP], 0,
      stream->session->id, stream->ssrc);
}

static void
on_new_sender_ssrc (GstElement * session, guint32 ssrc, GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_NEW_SENDER_SSRC], 0,
      sess->id, ssrc);
}

static void
on_sender_ssrc_active (GstElement * session, guint32 ssrc,
    GstRtpBinSession * sess)
{
  g_signal_emit (sess->bin, gst_rtp_bin_signals[SIGNAL_ON_SENDER_SSRC_ACTIVE],
      0, sess->id, ssrc);
}

/* must be called with the SESSION lock */
static GstRtpBinStream *
find_stream_by_ssrc (GstRtpBinSession * session, guint32 ssrc)
{
  GSList *walk;

  for (walk = session->streams; walk; walk = g_slist_next (walk)) {
    GstRtpBinStream *stream = (GstRtpBinStream *) walk->data;

    if (stream->ssrc == ssrc)
      return stream;
  }
  return NULL;
}

static void
ssrc_demux_pad_removed (GstElement * element, guint ssrc, GstPad * pad,
    GstRtpBinSession * session)
{
  GstRtpBinStream *stream = NULL;
  GstRtpBin *rtpbin;

  rtpbin = session->bin;

  GST_RTP_BIN_LOCK (rtpbin);

  GST_RTP_SESSION_LOCK (session);
  if ((stream = find_stream_by_ssrc (session, ssrc)))
    session->streams = g_slist_remove (session->streams, stream);
  GST_RTP_SESSION_UNLOCK (session);

  if (stream)
    free_stream (stream, rtpbin);

  GST_RTP_BIN_UNLOCK (rtpbin);
}

/* create a session with the given id.  Must be called with RTP_BIN_LOCK */
static GstRtpBinSession *
create_session (GstRtpBin * rtpbin, gint id)
{
  GstRtpBinSession *sess;
  GstElement *session, *demux;
  GstElement *storage = NULL;
  GstState target;

  if (!(session = gst_element_factory_make ("rtpsession", NULL)))
    goto no_session;

  if (!(demux = gst_element_factory_make ("rtpssrcdemux", NULL)))
    goto no_demux;

  if (!(storage = gst_element_factory_make ("rtpstorage", NULL)))
    goto no_storage;

  /* need to sink the storage or otherwise signal handlers from bindings will
   * take ownership of it and we don't own it anymore */
  gst_object_ref_sink (storage);
  g_signal_emit (rtpbin, gst_rtp_bin_signals[SIGNAL_NEW_STORAGE], 0, storage,
      id);

  sess = g_new0 (GstRtpBinSession, 1);
  g_mutex_init (&sess->lock);
  sess->id = id;
  sess->bin = rtpbin;
  sess->session = session;
  sess->demux = demux;
  sess->storage = storage;

  sess->ptmap = g_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) gst_caps_unref);
  rtpbin->sessions = g_slist_prepend (rtpbin->sessions, sess);

  /* configure SDES items */
  GST_OBJECT_LOCK (rtpbin);
  g_object_set (session, "sdes", rtpbin->sdes, "rtp-profile",
      rtpbin->rtp_profile, "rtcp-sync-send-time", rtpbin->rtcp_sync_send_time,
      NULL);
  if (rtpbin->use_pipeline_clock)
    g_object_set (session, "use-pipeline-clock", rtpbin->use_pipeline_clock,
        NULL);
  else
    g_object_set (session, "ntp-time-source", rtpbin->ntp_time_source, NULL);

  g_object_set (session, "max-dropout-time", rtpbin->max_dropout_time,
      "max-misorder-time", rtpbin->max_misorder_time, NULL);
  GST_OBJECT_UNLOCK (rtpbin);

  /* provide clock_rate to the session manager when needed */
  g_signal_connect (session, "request-pt-map",
      (GCallback) pt_map_requested, sess);

  g_signal_connect (sess->session, "on-new-ssrc",
      (GCallback) on_new_ssrc, sess);
  g_signal_connect (sess->session, "on-ssrc-collision",
      (GCallback) on_ssrc_collision, sess);
  g_signal_connect (sess->session, "on-ssrc-validated",
      (GCallback) on_ssrc_validated, sess);
  g_signal_connect (sess->session, "on-ssrc-active",
      (GCallback) on_ssrc_active, sess);
  g_signal_connect (sess->session, "on-ssrc-sdes",
      (GCallback) on_ssrc_sdes, sess);
  g_signal_connect (sess->session, "on-bye-ssrc",
      (GCallback) on_bye_ssrc, sess);
  g_signal_connect (sess->session, "on-bye-timeout",
      (GCallback) on_bye_timeout, sess);
  g_signal_connect (sess->session, "on-timeout", (GCallback) on_timeout, sess);
  g_signal_connect (sess->session, "on-sender-timeout",
      (GCallback) on_sender_timeout, sess);
  g_signal_connect (sess->session, "on-new-sender-ssrc",
      (GCallback) on_new_sender_ssrc, sess);
  g_signal_connect (sess->session, "on-sender-ssrc-active",
      (GCallback) on_sender_ssrc_active, sess);

  gst_bin_add (GST_BIN_CAST (rtpbin), session);
  gst_bin_add (GST_BIN_CAST (rtpbin), demux);
  gst_bin_add (GST_BIN_CAST (rtpbin), storage);

  /* unref the storage again, the bin has a reference now and
   * we don't need it anymore */
  gst_object_unref (storage);

  GST_OBJECT_LOCK (rtpbin);
  target = GST_STATE_TARGET (rtpbin);
  GST_OBJECT_UNLOCK (rtpbin);

  /* change state only to what's needed */
  gst_element_set_state (demux, target);
  gst_element_set_state (session, target);
  gst_element_set_state (storage, target);

  return sess;

  /* ERRORS */
no_session:
  {
    g_warning ("rtpbin: could not create rtpsession element");
    return NULL;
  }
no_demux:
  {
    gst_object_unref (session);
    g_warning ("rtpbin: could not create rtpssrcdemux element");
    return NULL;
  }
no_storage:
  {
    gst_object_unref (session);
    gst_object_unref (demux);
    g_warning ("rtpbin: could not create rtpstorage element");
    return NULL;
  }
}

static gboolean
bin_manage_element (GstRtpBin * bin, GstElement * element)
{
  GstRtpBinPrivate *priv = bin->priv;

  if (g_list_find (priv->elements, element)) {
    GST_DEBUG_OBJECT (bin, "requested element %p already in bin", element);
  } else {
    GST_DEBUG_OBJECT (bin, "adding requested element %p", element);

    if (g_object_is_floating (element))
      element = gst_object_ref_sink (element);

    if (!gst_bin_add (GST_BIN_CAST (bin), element))
      goto add_failed;
    if (!gst_element_sync_state_with_parent (element))
      GST_WARNING_OBJECT (bin, "unable to sync element state with rtpbin");
  }
  /* we add the element multiple times, each we need an equal number of
   * removes to really remove the element from the bin */
  priv->elements = g_list_prepend (priv->elements, element);

  return TRUE;

  /* ERRORS */
add_failed:
  {
    GST_WARNING_OBJECT (bin, "unable to add element");
    gst_object_unref (element);
    return FALSE;
  }
}

static void
remove_bin_element (GstElement * element, GstRtpBin * bin)
{
  GstRtpBinPrivate *priv = bin->priv;
  GList *find;

  find = g_list_find (priv->elements, element);
  if (find) {
    priv->elements = g_list_delete_link (priv->elements, find);

    if (!g_list_find (priv->elements, element)) {
      gst_element_set_locked_state (element, TRUE);
      gst_bin_remove (GST_BIN_CAST (bin), element);
      gst_element_set_state (element, GST_STATE_NULL);
    }

    gst_object_unref (element);
  }
}

/* called with RTP_BIN_LOCK */
static void
free_session (GstRtpBinSession * sess, GstRtpBin * bin)
{
  GST_DEBUG_OBJECT (bin, "freeing session %p", sess);

  gst_element_set_locked_state (sess->demux, TRUE);
  gst_element_set_locked_state (sess->session, TRUE);
  gst_element_set_locked_state (sess->storage, TRUE);

  gst_element_set_state (sess->demux, GST_STATE_NULL);
  gst_element_set_state (sess->session, GST_STATE_NULL);
  gst_element_set_state (sess->storage, GST_STATE_NULL);

  remove_recv_rtp (bin, sess);
  remove_recv_rtcp (bin, sess);
  remove_send_rtp (bin, sess);
  remove_rtcp (bin, sess);

  gst_bin_remove (GST_BIN_CAST (bin), sess->session);
  gst_bin_remove (GST_BIN_CAST (bin), sess->demux);
  gst_bin_remove (GST_BIN_CAST (bin), sess->storage);

  g_slist_foreach (sess->elements, (GFunc) remove_bin_element, bin);
  g_slist_free (sess->elements);

  g_slist_foreach (sess->streams, (GFunc) free_stream, bin);
  g_slist_free (sess->streams);

  g_mutex_clear (&sess->lock);
  g_hash_table_destroy (sess->ptmap);

  g_free (sess);
}

/* get the payload type caps for the specific payload @pt in @session */
static GstCaps *
get_pt_map (GstRtpBinSession * session, guint pt)
{
  GstCaps *caps = NULL;
  GstRtpBin *bin;
  GValue ret = { 0 };
  GValue args[3] = { {0}, {0}, {0} };

  GST_DEBUG ("searching pt %u in cache", pt);

  GST_RTP_SESSION_LOCK (session);

  /* first look in the cache */
  caps = g_hash_table_lookup (session->ptmap, GINT_TO_POINTER (pt));
  if (caps) {
    gst_caps_ref (caps);
    goto done;
  }

  bin = session->bin;

  GST_DEBUG ("emitting signal for pt %u in session %u", pt, session->id);

  /* not in cache, send signal to request caps */
  g_value_init (&args[0], GST_TYPE_ELEMENT);
  g_value_set_object (&args[0], bin);
  g_value_init (&args[1], G_TYPE_UINT);
  g_value_set_uint (&args[1], session->id);
  g_value_init (&args[2], G_TYPE_UINT);
  g_value_set_uint (&args[2], pt);

  g_value_init (&ret, GST_TYPE_CAPS);
  g_value_set_boxed (&ret, NULL);

  GST_RTP_SESSION_UNLOCK (session);

  g_signal_emitv (args, gst_rtp_bin_signals[SIGNAL_REQUEST_PT_MAP], 0, &ret);

  GST_RTP_SESSION_LOCK (session);

  g_value_unset (&args[0]);
  g_value_unset (&args[1]);
  g_value_unset (&args[2]);

  /* look in the cache again because we let the lock go */
  caps = g_hash_table_lookup (session->ptmap, GINT_TO_POINTER (pt));
  if (caps) {
    gst_caps_ref (caps);
    g_value_unset (&ret);
    goto done;
  }

  caps = (GstCaps *) g_value_dup_boxed (&ret);
  g_value_unset (&ret);
  if (!caps)
    goto no_caps;

  GST_DEBUG ("caching pt %u as %" GST_PTR_FORMAT, pt, caps);

  /* store in cache, take additional ref */
  g_hash_table_insert (session->ptmap, GINT_TO_POINTER (pt),
      gst_caps_ref (caps));

done:
  GST_RTP_SESSION_UNLOCK (session);

  return caps;

  /* ERRORS */
no_caps:
  {
    GST_RTP_SESSION_UNLOCK (session);
    GST_DEBUG ("no pt map could be obtained");
    return NULL;
  }
}

static gboolean
return_true (gpointer key, gpointer value, gpointer user_data)
{
  return TRUE;
}

static void
gst_rtp_bin_reset_sync (GstRtpBin * rtpbin)
{
  GSList *clients, *streams;

  GST_DEBUG_OBJECT (rtpbin, "Reset sync on all clients");

  GST_RTP_BIN_LOCK (rtpbin);
  for (clients = rtpbin->clients; clients; clients = g_slist_next (clients)) {
    GstRtpBinClient *client = (GstRtpBinClient *) clients->data;

    /* reset sync on all streams for this client */
    for (streams = client->streams; streams; streams = g_slist_next (streams)) {
      GstRtpBinStream *stream = (GstRtpBinStream *) streams->data;

      /* make use require a new SR packet for this stream before we attempt new
       * lip-sync */
      stream->have_sync = FALSE;
      stream->rt_delta = 0;
      stream->rtp_delta = 0;
      stream->clock_base = -100 * GST_SECOND;
    }
  }
  GST_RTP_BIN_UNLOCK (rtpbin);
}

static void
gst_rtp_bin_clear_pt_map (GstRtpBin * bin)
{
  GSList *sessions, *streams;

  GST_RTP_BIN_LOCK (bin);
  GST_DEBUG_OBJECT (bin, "clearing pt map");
  for (sessions = bin->sessions; sessions; sessions = g_slist_next (sessions)) {
    GstRtpBinSession *session = (GstRtpBinSession *) sessions->data;

    GST_DEBUG_OBJECT (bin, "clearing session %p", session);
    g_signal_emit_by_name (session->session, "clear-pt-map", NULL);

    GST_RTP_SESSION_LOCK (session);
    g_hash_table_foreach_remove (session->ptmap, return_true, NULL);

    for (streams = session->streams; streams; streams = g_slist_next (streams)) {
      GstRtpBinStream *stream = (GstRtpBinStream *) streams->data;

      GST_DEBUG_OBJECT (bin, "clearing stream %p", stream);
      if (g_signal_lookup ("clear-pt-map", G_OBJECT_TYPE (stream->buffer)) != 0)
        g_signal_emit_by_name (stream->buffer, "clear-pt-map", NULL);
      if (stream->demux)
        g_signal_emit_by_name (stream->demux, "clear-pt-map", NULL);
    }
    GST_RTP_SESSION_UNLOCK (session);
  }
  GST_RTP_BIN_UNLOCK (bin);

  /* reset sync too */
  gst_rtp_bin_reset_sync (bin);
}

static GstElement *
gst_rtp_bin_get_session (GstRtpBin * bin, guint session_id)
{
  GstRtpBinSession *session;
  GstElement *ret = NULL;

  GST_RTP_BIN_LOCK (bin);
  GST_DEBUG_OBJECT (bin, "retrieving GstRtpSession, index: %u", session_id);
  session = find_session_by_id (bin, (gint) session_id);
  if (session) {
    ret = gst_object_ref (session->session);
  }
  GST_RTP_BIN_UNLOCK (bin);

  return ret;
}

static RTPSession *
gst_rtp_bin_get_internal_session (GstRtpBin * bin, guint session_id)
{
  RTPSession *internal_session = NULL;
  GstRtpBinSession *session;

  GST_RTP_BIN_LOCK (bin);
  GST_DEBUG_OBJECT (bin, "retrieving internal RTPSession object, index: %u",
      session_id);
  session = find_session_by_id (bin, (gint) session_id);
  if (session) {
    g_object_get (session->session, "internal-session", &internal_session,
        NULL);
  }
  GST_RTP_BIN_UNLOCK (bin);

  return internal_session;
}

static GstElement *
gst_rtp_bin_get_storage (GstRtpBin * bin, guint session_id)
{
  GstRtpBinSession *session;
  GstElement *res = NULL;

  GST_RTP_BIN_LOCK (bin);
  GST_DEBUG_OBJECT (bin, "retrieving internal storage object, index: %u",
      session_id);
  session = find_session_by_id (bin, (gint) session_id);
  if (session && session->storage) {
    res = gst_object_ref (session->storage);
  }
  GST_RTP_BIN_UNLOCK (bin);

  return res;
}

static GObject *
gst_rtp_bin_get_internal_storage (GstRtpBin * bin, guint session_id)
{
  GObject *internal_storage = NULL;
  GstRtpBinSession *session;

  GST_RTP_BIN_LOCK (bin);
  GST_DEBUG_OBJECT (bin, "retrieving internal storage object, index: %u",
      session_id);
  session = find_session_by_id (bin, (gint) session_id);
  if (session && session->storage) {
    g_object_get (session->storage, "internal-storage", &internal_storage,
        NULL);
  }
  GST_RTP_BIN_UNLOCK (bin);

  return internal_storage;
}

static GstElement *
gst_rtp_bin_request_encoder (GstRtpBin * bin, guint session_id)
{
  GST_DEBUG_OBJECT (bin, "return NULL encoder");
  return NULL;
}

static GstElement *
gst_rtp_bin_request_decoder (GstRtpBin * bin, guint session_id)
{
  GST_DEBUG_OBJECT (bin, "return NULL decoder");
  return NULL;
}

static GstElement *
gst_rtp_bin_request_jitterbuffer (GstRtpBin * bin, guint session_id)
{
  return gst_element_factory_make ("rtpjitterbuffer", NULL);
}

static void
gst_rtp_bin_propagate_property_to_jitterbuffer (GstRtpBin * bin,
    const gchar * name, const GValue * value)
{
  GSList *sessions, *streams;

  GST_RTP_BIN_LOCK (bin);
  for (sessions = bin->sessions; sessions; sessions = g_slist_next (sessions)) {
    GstRtpBinSession *session = (GstRtpBinSession *) sessions->data;

    GST_RTP_SESSION_LOCK (session);
    for (streams = session->streams; streams; streams = g_slist_next (streams)) {
      GstRtpBinStream *stream = (GstRtpBinStream *) streams->data;
      GObjectClass *jb_class;

      jb_class = G_OBJECT_GET_CLASS (G_OBJECT (stream->buffer));
      if (g_object_class_find_property (jb_class, name))
        g_object_set_property (G_OBJECT (stream->buffer), name, value);
      else
        GST_WARNING_OBJECT (bin,
            "Stream jitterbuffer does not expose property %s", name);
    }
    GST_RTP_SESSION_UNLOCK (session);
  }
  GST_RTP_BIN_UNLOCK (bin);
}

static void
gst_rtp_bin_propagate_property_to_session (GstRtpBin * bin,
    const gchar * name, const GValue * value)
{
  GSList *sessions;

  GST_RTP_BIN_LOCK (bin);
  for (sessions = bin->sessions; sessions; sessions = g_slist_next (sessions)) {
    GstRtpBinSession *sess = (GstRtpBinSession *) sessions->data;

    g_object_set_property (G_OBJECT (sess->session), name, value);
  }
  GST_RTP_BIN_UNLOCK (bin);
}

/* get a client with the given SDES name. Must be called with RTP_BIN_LOCK */
static GstRtpBinClient *
get_client (GstRtpBin * bin, guint8 len, guint8 * data, gboolean * created)
{
  GstRtpBinClient *result = NULL;
  GSList *walk;

  for (walk = bin->clients; walk; walk = g_slist_next (walk)) {
    GstRtpBinClient *client = (GstRtpBinClient *) walk->data;

    if (len != client->cname_len)
      continue;

    if (!strncmp ((gchar *) data, client->cname, client->cname_len)) {
      GST_DEBUG_OBJECT (bin, "found existing client %p with CNAME %s", client,
          client->cname);
      result = client;
      break;
    }
  }

  /* nothing found, create one */
  if (result == NULL) {
    result = g_new0 (GstRtpBinClient, 1);
    result->cname = g_strndup ((gchar *) data, len);
    result->cname_len = len;
    bin->clients = g_slist_prepend (bin->clients, result);
    GST_DEBUG_OBJECT (bin, "created new client %p with CNAME %s", result,
        result->cname);
  }
  return result;
}

static void
free_client (GstRtpBinClient * client, GstRtpBin * bin)
{
  GST_DEBUG_OBJECT (bin, "freeing client %p", client);
  g_slist_free (client->streams);
  g_free (client->cname);
  g_free (client);
}

static void
get_current_times (GstRtpBin * bin, GstClockTime * running_time,
    guint64 * ntpnstime)
{
  guint64 ntpns = -1;
  GstClock *clock;
  GstClockTime base_time, rt, clock_time;

  GST_OBJECT_LOCK (bin);
  if ((clock = GST_ELEMENT_CLOCK (bin))) {
    base_time = GST_ELEMENT_CAST (bin)->base_time;
    gst_object_ref (clock);
    GST_OBJECT_UNLOCK (bin);

    /* get current clock time and convert to running time */
    clock_time = gst_clock_get_time (clock);
    rt = clock_time - base_time;

    if (bin->use_pipeline_clock) {
      ntpns = rt;
      /* add constant to convert from 1970 based time to 1900 based time */
      ntpns += (2208988800LL * GST_SECOND);
    } else {
      switch (bin->ntp_time_source) {
        case GST_RTP_NTP_TIME_SOURCE_NTP:
        case GST_RTP_NTP_TIME_SOURCE_UNIX:{
          /* get current NTP time */
          ntpns = g_get_real_time () * GST_USECOND;

          /* add constant to convert from 1970 based time to 1900 based time */
          if (bin->ntp_time_source == GST_RTP_NTP_TIME_SOURCE_NTP)
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
          ntpns = -1;           /* Fix uninited compiler warning */
          g_assert_not_reached ();
          break;
      }
    }

    gst_object_unref (clock);
  } else {
    GST_OBJECT_UNLOCK (bin);
    rt = -1;
    ntpns = -1;
  }
  if (running_time)
    *running_time = rt;
  if (ntpnstime)
    *ntpnstime = ntpns;
}

static void
stream_set_ts_offset (GstRtpBin * bin, GstRtpBinStream * stream,
    gint64 ts_offset, gint64 max_ts_offset, gint64 min_ts_offset,
    gboolean allow_positive_ts_offset)
{
  gint64 prev_ts_offset;
  GObjectClass *jb_class;

  jb_class = G_OBJECT_GET_CLASS (G_OBJECT (stream->buffer));

  if (!g_object_class_find_property (jb_class, "ts-offset")) {
    GST_LOG_OBJECT (bin,
        "stream's jitterbuffer does not expose ts-offset property");
    return;
  }

  g_object_get (stream->buffer, "ts-offset", &prev_ts_offset, NULL);

  /* delta changed, see how much */
  if (prev_ts_offset != ts_offset) {
    gint64 diff;

    diff = prev_ts_offset - ts_offset;

    GST_DEBUG_OBJECT (bin,
        "ts-offset %" G_GINT64_FORMAT ", prev %" G_GINT64_FORMAT
        ", diff: %" G_GINT64_FORMAT, ts_offset, prev_ts_offset, diff);

    /* ignore minor offsets */
    if (ABS (diff) < min_ts_offset) {
      GST_DEBUG_OBJECT (bin, "offset too small, ignoring");
      return;
    }

    /* sanity check offset */
    if (max_ts_offset > 0) {
      if (ts_offset > 0 && !allow_positive_ts_offset) {
        GST_DEBUG_OBJECT (bin,
            "offset is positive (clocks are out of sync), ignoring");
        return;
      }
      if (ABS (ts_offset) > max_ts_offset) {
        GST_DEBUG_OBJECT (bin, "offset too large, ignoring");
        return;
      }
    }

    g_object_set (stream->buffer, "ts-offset", ts_offset, NULL);
  }
  GST_DEBUG_OBJECT (bin, "stream SSRC %08x, delta %" G_GINT64_FORMAT,
      stream->ssrc, ts_offset);
}

static void
gst_rtp_bin_send_sync_event (GstRtpBinStream * stream)
{
  if (stream->bin->send_sync_event) {
    GstEvent *event;
    GstPad *srcpad;

    GST_DEBUG_OBJECT (stream->bin,
        "sending GstRTCPSRReceived event downstream");

    event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
        gst_structure_new_empty ("GstRTCPSRReceived"));

    srcpad = gst_element_get_static_pad (stream->buffer, "src");
    gst_pad_push_event (srcpad, event);
    gst_object_unref (srcpad);
  }
}

/* associate a stream to the given CNAME. This will make sure all streams for
 * that CNAME are synchronized together.
 * Must be called with GST_RTP_BIN_LOCK */
static void
gst_rtp_bin_associate (GstRtpBin * bin, GstRtpBinStream * stream, guint8 len,
    guint8 * data, guint64 ntptime, guint64 last_extrtptime,
    guint64 base_rtptime, guint64 base_time, guint clock_rate,
    gint64 rtp_clock_base)
{
  GstRtpBinClient *client;
  gboolean created;
  GSList *walk;
  GstClockTime running_time, running_time_rtp;
  guint64 ntpnstime;

  /* first find or create the CNAME */
  client = get_client (bin, len, data, &created);

  /* find stream in the client */
  for (walk = client->streams; walk; walk = g_slist_next (walk)) {
    GstRtpBinStream *ostream = (GstRtpBinStream *) walk->data;

    if (ostream == stream)
      break;
  }
  /* not found, add it to the list */
  if (walk == NULL) {
    GST_DEBUG_OBJECT (bin,
        "new association of SSRC %08x with client %p with CNAME %s",
        stream->ssrc, client, client->cname);
    client->streams = g_slist_prepend (client->streams, stream);
    client->nstreams++;
  } else {
    GST_DEBUG_OBJECT (bin,
        "found association of SSRC %08x with client %p with CNAME %s",
        stream->ssrc, client, client->cname);
  }

  if (!GST_CLOCK_TIME_IS_VALID (last_extrtptime)) {
    GST_DEBUG_OBJECT (bin, "invalidated sync data");
    if (bin->rtcp_sync == GST_RTP_BIN_RTCP_SYNC_RTP) {
      /* we don't need that data, so carry on,
       * but make some values look saner */
      last_extrtptime = base_rtptime;
    } else {
      /* nothing we can do with this data in this case */
      GST_DEBUG_OBJECT (bin, "bailing out");
      return;
    }
  }

  /* Take the extended rtptime we found in the SR packet and map it to the
   * local rtptime. The local rtp time is used to construct timestamps on the
   * buffers so we will calculate what running_time corresponds to the RTP
   * timestamp in the SR packet. */
  running_time_rtp = last_extrtptime - base_rtptime;

  GST_DEBUG_OBJECT (bin,
      "base %" G_GUINT64_FORMAT ", extrtptime %" G_GUINT64_FORMAT
      ", local RTP %" G_GUINT64_FORMAT ", clock-rate %d, "
      "clock-base %" G_GINT64_FORMAT, base_rtptime,
      last_extrtptime, running_time_rtp, clock_rate, rtp_clock_base);

  /* calculate local RTP time in gstreamer timestamp, we essentially perform the
   * same conversion that a jitterbuffer would use to convert an rtp timestamp
   * into a corresponding gstreamer timestamp. Note that the base_time also
   * contains the drift between sender and receiver. */
  running_time =
      gst_util_uint64_scale_int (running_time_rtp, GST_SECOND, clock_rate);
  running_time += base_time;

  /* convert ntptime to nanoseconds */
  ntpnstime = gst_util_uint64_scale (ntptime, GST_SECOND,
      (G_GINT64_CONSTANT (1) << 32));

  stream->have_sync = TRUE;

  GST_DEBUG_OBJECT (bin,
      "SR RTP running time %" G_GUINT64_FORMAT ", SR NTP %" G_GUINT64_FORMAT,
      running_time, ntpnstime);

  /* recalc inter stream playout offset, but only if there is more than one
   * stream or we're doing NTP sync. */
  if (bin->ntp_sync) {
    gint64 ntpdiff, rtdiff;
    guint64 local_ntpnstime;
    GstClockTime local_running_time;

    /* For NTP sync we need to first get a snapshot of running_time and NTP
     * time. We know at what running_time we play a certain RTP time, we also
     * calculated when we would play the RTP time in the SR packet. Now we need
     * to know how the running_time and the NTP time relate to each other. */
    get_current_times (bin, &local_running_time, &local_ntpnstime);

    /* see how far away the NTP time is. This is the difference between the
     * current NTP time and the NTP time in the last SR packet. */
    ntpdiff = local_ntpnstime - ntpnstime;
    /* see how far away the running_time is. This is the difference between the
     * current running_time and the running_time of the RTP timestamp in the
     * last SR packet. */
    rtdiff = local_running_time - running_time;

    GST_DEBUG_OBJECT (bin,
        "local NTP time %" G_GUINT64_FORMAT ", SR NTP time %" G_GUINT64_FORMAT,
        local_ntpnstime, ntpnstime);
    GST_DEBUG_OBJECT (bin,
        "local running time %" G_GUINT64_FORMAT ", SR RTP running time %"
        G_GUINT64_FORMAT, local_running_time, running_time);
    GST_DEBUG_OBJECT (bin,
        "NTP diff %" G_GINT64_FORMAT ", RT diff %" G_GINT64_FORMAT, ntpdiff,
        rtdiff);

    /* combine to get the final diff to apply to the running_time */
    stream->rt_delta = rtdiff - ntpdiff;

    stream_set_ts_offset (bin, stream, stream->rt_delta, bin->max_ts_offset,
        0, FALSE);
  } else {
    gint64 min, rtp_min, clock_base = stream->clock_base;
    gboolean all_sync, use_rtp;
    gboolean rtcp_sync = g_atomic_int_get (&bin->rtcp_sync);

    /* calculate delta between server and receiver. ntpnstime is created by
     * converting the ntptime in the last SR packet to a gstreamer timestamp. This
     * delta expresses the difference to our timeline and the server timeline. The
     * difference in itself doesn't mean much but we can combine the delta of
     * multiple streams to create a stream specific offset. */
    stream->rt_delta = ntpnstime - running_time;

    /* calculate the min of all deltas, ignoring streams that did not yet have a
     * valid rt_delta because we did not yet receive an SR packet for those
     * streams.
     * We calculate the minimum because we would like to only apply positive
     * offsets to streams, delaying their playback instead of trying to speed up
     * other streams (which might be impossible when we have to create negative
     * latencies).
     * The stream that has the smallest diff is selected as the reference stream,
     * all other streams will have a positive offset to this difference. */

    /* some alternative setting allow ignoring RTCP as much as possible,
     * for servers generating bogus ntp timeline */
    min = rtp_min = G_MAXINT64;
    use_rtp = FALSE;
    if (rtcp_sync == GST_RTP_BIN_RTCP_SYNC_RTP) {
      guint64 ext_base;

      use_rtp = TRUE;
      /* signed version for convenience */
      clock_base = base_rtptime;
      /* deal with possible wrap-around */
      ext_base = base_rtptime;
      rtp_clock_base = gst_rtp_buffer_ext_timestamp (&ext_base, rtp_clock_base);
      /* sanity check; base rtp and provided clock_base should be close */
      if (rtp_clock_base >= clock_base) {
        if (rtp_clock_base - clock_base < 10 * clock_rate) {
          rtp_clock_base = base_time +
              gst_util_uint64_scale_int (rtp_clock_base - clock_base,
              GST_SECOND, clock_rate);
        } else {
          use_rtp = FALSE;
        }
      } else {
        if (clock_base - rtp_clock_base < 10 * clock_rate) {
          rtp_clock_base = base_time -
              gst_util_uint64_scale_int (clock_base - rtp_clock_base,
              GST_SECOND, clock_rate);
        } else {
          use_rtp = FALSE;
        }
      }
      /* warn and bail for clarity out if no sane values */
      if (!use_rtp) {
        GST_WARNING_OBJECT (bin, "unable to sync to provided rtptime");
        return;
      }
      /* store to track changes */
      clock_base = rtp_clock_base;
      /* generate a fake as before,
       * now equating rtptime obtained from RTP-Info,
       * where the large time represent the otherwise irrelevant npt/ntp time */
      stream->rtp_delta = (GST_SECOND << 28) - rtp_clock_base;
    } else {
      clock_base = rtp_clock_base;
    }

    all_sync = TRUE;
    for (walk = client->streams; walk; walk = g_slist_next (walk)) {
      GstRtpBinStream *ostream = (GstRtpBinStream *) walk->data;

      if (!ostream->have_sync) {
        all_sync = FALSE;
        continue;
      }

      /* change in current stream's base from previously init'ed value
       * leads to reset of all stream's base */
      if (stream != ostream && stream->clock_base >= 0 &&
          (stream->clock_base != clock_base)) {
        GST_DEBUG_OBJECT (bin, "reset upon clock base change");
        ostream->clock_base = -100 * GST_SECOND;
        ostream->rtp_delta = 0;
      }

      if (ostream->rt_delta < min)
        min = ostream->rt_delta;
      if (ostream->rtp_delta < rtp_min)
        rtp_min = ostream->rtp_delta;
    }

    /* arrange to re-sync for each stream upon significant change,
     * e.g. post-seek */
    all_sync = all_sync && (stream->clock_base == clock_base);
    stream->clock_base = clock_base;

    /* may need init performed above later on, but nothing more to do now */
    if (client->nstreams <= 1)
      return;

    GST_DEBUG_OBJECT (bin, "client %p min delta %" G_GINT64_FORMAT
        " all sync %d", client, min, all_sync);
    GST_DEBUG_OBJECT (bin, "rtcp sync mode %d, use_rtp %d", rtcp_sync, use_rtp);

    switch (rtcp_sync) {
      case GST_RTP_BIN_RTCP_SYNC_RTP:
        if (!use_rtp)
          break;
        GST_DEBUG_OBJECT (bin, "using rtp generated reports; "
            "client %p min rtp delta %" G_GINT64_FORMAT, client, rtp_min);
        /* fall-through */
      case GST_RTP_BIN_RTCP_SYNC_INITIAL:
        /* if all have been synced already, do not bother further */
        if (all_sync) {
          GST_DEBUG_OBJECT (bin, "all streams already synced; done");
          return;
        }
        break;
      default:
        break;
    }

    /* bail out if we adjusted recently enough */
    if (all_sync && (ntpnstime - bin->priv->last_ntpnstime) <
        bin->rtcp_sync_interval * GST_MSECOND) {
      GST_DEBUG_OBJECT (bin, "discarding RTCP sender packet for sync; "
          "previous sender info too recent "
          "(previous NTP %" G_GUINT64_FORMAT ")", bin->priv->last_ntpnstime);
      return;
    }
    bin->priv->last_ntpnstime = ntpnstime;

    /* calculate offsets for each stream */
    for (walk = client->streams; walk; walk = g_slist_next (walk)) {
      GstRtpBinStream *ostream = (GstRtpBinStream *) walk->data;
      gint64 ts_offset;

      /* ignore streams for which we didn't receive an SR packet yet, we
       * can't synchronize them yet. We can however sync other streams just
       * fine. */
      if (!ostream->have_sync)
        continue;

      /* calculate offset to our reference stream, this should always give a
       * positive number. */
      if (use_rtp)
        ts_offset = ostream->rtp_delta - rtp_min;
      else
        ts_offset = ostream->rt_delta - min;

      stream_set_ts_offset (bin, ostream, ts_offset, bin->max_ts_offset,
          MIN_TS_OFFSET, TRUE);
    }
  }
  gst_rtp_bin_send_sync_event (stream);

  return;
}

#define GST_RTCP_BUFFER_FOR_PACKETS(b,buffer,packet) \
  for ((b) = gst_rtcp_buffer_get_first_packet ((buffer), (packet)); (b); \
          (b) = gst_rtcp_packet_move_to_next ((packet)))

#define GST_RTCP_SDES_FOR_ITEMS(b,packet) \
  for ((b) = gst_rtcp_packet_sdes_first_item ((packet)); (b); \
          (b) = gst_rtcp_packet_sdes_next_item ((packet)))

#define GST_RTCP_SDES_FOR_ENTRIES(b,packet) \
  for ((b) = gst_rtcp_packet_sdes_first_entry ((packet)); (b); \
          (b) = gst_rtcp_packet_sdes_next_entry ((packet)))

static void
gst_rtp_bin_handle_sync (GstElement * jitterbuffer, GstStructure * s,
    GstRtpBinStream * stream)
{
  GstRtpBin *bin;
  GstRTCPPacket packet;
  guint32 ssrc;
  guint64 ntptime;
  gboolean have_sr, have_sdes;
  gboolean more;
  guint64 base_rtptime;
  guint64 base_time;
  guint clock_rate;
  guint64 clock_base;
  guint64 extrtptime;
  GstBuffer *buffer;
  GstRTCPBuffer rtcp = { NULL, };

  bin = stream->bin;

  GST_DEBUG_OBJECT (bin, "sync handler called");

  /* get the last relation between the rtp timestamps and the gstreamer
   * timestamps. We get this info directly from the jitterbuffer which
   * constructs gstreamer timestamps from rtp timestamps and so it know exactly
   * what the current situation is. */
  base_rtptime =
      g_value_get_uint64 (gst_structure_get_value (s, "base-rtptime"));
  base_time = g_value_get_uint64 (gst_structure_get_value (s, "base-time"));
  clock_rate = g_value_get_uint (gst_structure_get_value (s, "clock-rate"));
  clock_base = g_value_get_uint64 (gst_structure_get_value (s, "clock-base"));
  extrtptime =
      g_value_get_uint64 (gst_structure_get_value (s, "sr-ext-rtptime"));
  buffer = gst_value_get_buffer (gst_structure_get_value (s, "sr-buffer"));

  have_sr = FALSE;
  have_sdes = FALSE;

  gst_rtcp_buffer_map (buffer, GST_MAP_READ, &rtcp);

  GST_RTCP_BUFFER_FOR_PACKETS (more, &rtcp, &packet) {
    /* first packet must be SR or RR or else the validate would have failed */
    switch (gst_rtcp_packet_get_type (&packet)) {
      case GST_RTCP_TYPE_SR:
        /* only parse first. There is only supposed to be one SR in the packet
         * but we will deal with malformed packets gracefully */
        if (have_sr)
          break;
        /* get NTP and RTP times */
        gst_rtcp_packet_sr_get_sender_info (&packet, &ssrc, &ntptime, NULL,
            NULL, NULL);

        GST_DEBUG_OBJECT (bin, "received sync packet from SSRC %08x", ssrc);
        /* ignore SR that is not ours */
        if (ssrc != stream->ssrc)
          continue;

        have_sr = TRUE;
        break;
      case GST_RTCP_TYPE_SDES:
      {
        gboolean more_items, more_entries;

        /* only deal with first SDES, there is only supposed to be one SDES in
         * the RTCP packet but we deal with bad packets gracefully. Also bail
         * out if we have not seen an SR item yet. */
        if (have_sdes || !have_sr)
          break;

        GST_RTCP_SDES_FOR_ITEMS (more_items, &packet) {
          /* skip items that are not about the SSRC of the sender */
          if (gst_rtcp_packet_sdes_get_ssrc (&packet) != ssrc)
            continue;

          /* find the CNAME entry */
          GST_RTCP_SDES_FOR_ENTRIES (more_entries, &packet) {
            GstRTCPSDESType type;
            guint8 len;
            guint8 *data;

            gst_rtcp_packet_sdes_get_entry (&packet, &type, &len, &data);

            if (type == GST_RTCP_SDES_CNAME) {
              GST_RTP_BIN_LOCK (bin);
              /* associate the stream to CNAME */
              gst_rtp_bin_associate (bin, stream, len, data,
                  ntptime, extrtptime, base_rtptime, base_time, clock_rate,
                  clock_base);
              GST_RTP_BIN_UNLOCK (bin);
            }
          }
        }
        have_sdes = TRUE;
        break;
      }
      default:
        /* we can ignore these packets */
        break;
    }
  }
  gst_rtcp_buffer_unmap (&rtcp);
}

/* create a new stream with @ssrc in @session. Must be called with
 * RTP_SESSION_LOCK. */
static GstRtpBinStream *
create_stream (GstRtpBinSession * session, guint32 ssrc)
{
  GstElement *buffer, *demux = NULL;
  GstRtpBinStream *stream;
  GstRtpBin *rtpbin;
  GstState target;
  GObjectClass *jb_class;

  rtpbin = session->bin;

  if (g_slist_length (session->streams) >= rtpbin->max_streams)
    goto max_streams;

  if (!(buffer =
          session_request_element (session, SIGNAL_REQUEST_JITTERBUFFER)))
    goto no_jitterbuffer;

  if (!rtpbin->ignore_pt) {
    if (!(demux = gst_element_factory_make ("rtpptdemux", NULL)))
      goto no_demux;
  }

  stream = g_new0 (GstRtpBinStream, 1);
  stream->ssrc = ssrc;
  stream->bin = rtpbin;
  stream->session = session;
  stream->buffer = gst_object_ref (buffer);
  stream->demux = demux;

  stream->have_sync = FALSE;
  stream->rt_delta = 0;
  stream->rtp_delta = 0;
  stream->percent = 100;
  stream->clock_base = -100 * GST_SECOND;
  session->streams = g_slist_prepend (session->streams, stream);

  jb_class = G_OBJECT_GET_CLASS (G_OBJECT (buffer));

  if (g_signal_lookup ("request-pt-map", G_OBJECT_TYPE (buffer)) != 0) {
    /* provide clock_rate to the jitterbuffer when needed */
    stream->buffer_ptreq_sig = g_signal_connect (buffer, "request-pt-map",
        (GCallback) pt_map_requested, session);
  }
  if (g_signal_lookup ("on-npt-stop", G_OBJECT_TYPE (buffer)) != 0) {
    stream->buffer_ntpstop_sig = g_signal_connect (buffer, "on-npt-stop",
        (GCallback) on_npt_stop, stream);
  }

  g_object_set_data (G_OBJECT (buffer), "GstRTPBin.session", session);
  g_object_set_data (G_OBJECT (buffer), "GstRTPBin.stream", stream);

  /* configure latency and packet lost */
  g_object_set (buffer, "latency", rtpbin->latency_ms, NULL);

  if (g_object_class_find_property (jb_class, "drop-on-latency"))
    g_object_set (buffer, "drop-on-latency", rtpbin->drop_on_latency, NULL);
  if (g_object_class_find_property (jb_class, "do-lost"))
    g_object_set (buffer, "do-lost", rtpbin->do_lost, NULL);
  if (g_object_class_find_property (jb_class, "mode"))
    g_object_set (buffer, "mode", rtpbin->buffer_mode, NULL);
  if (g_object_class_find_property (jb_class, "do-retransmission"))
    g_object_set (buffer, "do-retransmission", rtpbin->do_retransmission, NULL);
  if (g_object_class_find_property (jb_class, "max-rtcp-rtp-time-diff"))
    g_object_set (buffer, "max-rtcp-rtp-time-diff",
        rtpbin->max_rtcp_rtp_time_diff, NULL);
  if (g_object_class_find_property (jb_class, "max-dropout-time"))
    g_object_set (buffer, "max-dropout-time", rtpbin->max_dropout_time, NULL);
  if (g_object_class_find_property (jb_class, "max-misorder-time"))
    g_object_set (buffer, "max-dropout-time", rtpbin->max_misorder_time, NULL);
  if (g_object_class_find_property (jb_class, "rfc7273-sync"))
    g_object_set (buffer, "rfc7273-sync", rtpbin->rfc7273_sync, NULL);
  if (g_object_class_find_property (jb_class, "max-ts-offset-adjustment"))
    g_object_set (buffer, "max-ts-offset-adjustment",
        rtpbin->max_ts_offset_adjustment, NULL);

  /* need to sink the jitterbufer or otherwise signal handlers from bindings will
   * take ownership of it and we don't own it anymore */
  gst_object_ref_sink (buffer);
  g_signal_emit (rtpbin, gst_rtp_bin_signals[SIGNAL_NEW_JITTERBUFFER], 0,
      buffer, session->id, ssrc);

  if (!rtpbin->ignore_pt)
    gst_bin_add (GST_BIN_CAST (rtpbin), demux);

  /* unref the jitterbuffer again, the bin has a reference now and
   * we don't need it anymore */
  gst_object_unref (buffer);

  /* link stuff */
  if (demux)
    gst_element_link_pads_full (buffer, "src", demux, "sink",
        GST_PAD_LINK_CHECK_NOTHING);

  if (rtpbin->buffering) {
    guint64 last_out;

    if (g_signal_lookup ("set-active", G_OBJECT_TYPE (buffer)) != 0) {
      GST_INFO_OBJECT (rtpbin,
          "bin is buffering, set jitterbuffer as not active");
      g_signal_emit_by_name (buffer, "set-active", FALSE, (gint64) 0,
          &last_out);
    }
  }


  GST_OBJECT_LOCK (rtpbin);
  target = GST_STATE_TARGET (rtpbin);
  GST_OBJECT_UNLOCK (rtpbin);

  /* from sink to source */
  if (demux)
    gst_element_set_state (demux, target);

  gst_element_set_state (buffer, target);

  return stream;

  /* ERRORS */
max_streams:
  {
    GST_WARNING_OBJECT (rtpbin, "stream exceeds maximum (%d)",
        rtpbin->max_streams);
    return NULL;
  }
no_jitterbuffer:
  {
    g_warning ("rtpbin: could not create rtpjitterbuffer element");
    return NULL;
  }
no_demux:
  {
    gst_object_unref (buffer);
    g_warning ("rtpbin: could not create rtpptdemux element");
    return NULL;
  }
}

/* called with RTP_BIN_LOCK */
static void
free_stream (GstRtpBinStream * stream, GstRtpBin * bin)
{
  GSList *clients, *next_client;

  GST_DEBUG_OBJECT (bin, "freeing stream %p", stream);

  if (stream->demux) {
    g_signal_handler_disconnect (stream->demux, stream->demux_newpad_sig);
    g_signal_handler_disconnect (stream->demux, stream->demux_ptreq_sig);
    g_signal_handler_disconnect (stream->demux, stream->demux_ptchange_sig);
  }

  if (stream->buffer_handlesync_sig)
    g_signal_handler_disconnect (stream->buffer, stream->buffer_handlesync_sig);
  if (stream->buffer_ptreq_sig)
    g_signal_handler_disconnect (stream->buffer, stream->buffer_ptreq_sig);
  if (stream->buffer_ntpstop_sig)
    g_signal_handler_disconnect (stream->buffer, stream->buffer_ntpstop_sig);

  gst_object_unref (stream->buffer);

  if (stream->demux)
    gst_element_set_locked_state (stream->demux, TRUE);

  if (stream->demux)
    gst_element_set_state (stream->demux, GST_STATE_NULL);

  /* now remove this signal, we need this while going to NULL because it to
   * do some cleanups */
  if (stream->demux)
    g_signal_handler_disconnect (stream->demux, stream->demux_padremoved_sig);

  if (stream->demux)
    gst_bin_remove (GST_BIN_CAST (bin), stream->demux);

  for (clients = bin->clients; clients; clients = next_client) {
    GstRtpBinClient *client = (GstRtpBinClient *) clients->data;
    GSList *streams, *next_stream;

    next_client = g_slist_next (clients);

    for (streams = client->streams; streams; streams = next_stream) {
      GstRtpBinStream *ostream = (GstRtpBinStream *) streams->data;

      next_stream = g_slist_next (streams);

      if (ostream == stream) {
        client->streams = g_slist_delete_link (client->streams, streams);
        /* If this was the last stream belonging to this client,
         * clean up the client. */
        if (--client->nstreams == 0) {
          bin->clients = g_slist_delete_link (bin->clients, clients);
          free_client (client, bin);
          break;
        }
      }
    }
  }
  g_free (stream);
}

/* GObject vmethods */
static void gst_rtp_bin_dispose (GObject * object);
static void gst_rtp_bin_finalize (GObject * object);
static void gst_rtp_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/* GstElement vmethods */
static GstStateChangeReturn gst_rtp_bin_change_state (GstElement * element,
    GstStateChange transition);
static GstPad *gst_rtp_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static void gst_rtp_bin_release_pad (GstElement * element, GstPad * pad);
static void gst_rtp_bin_handle_message (GstBin * bin, GstMessage * message);

#define gst_rtp_bin_parent_class parent_class
G_DEFINE_TYPE_WITH_PRIVATE (GstRtpBin, gst_rtp_bin, GST_TYPE_BIN);

static gboolean
_gst_element_accumulator (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer dummy)
{
  GstElement *element;

  element = g_value_get_object (handler_return);
  GST_DEBUG ("got element %" GST_PTR_FORMAT, element);

  if (!(ihint->run_type & G_SIGNAL_RUN_CLEANUP))
    g_value_set_object (return_accu, element);

  /* stop emission if we have an element */
  return (element == NULL);
}

static gboolean
_gst_caps_accumulator (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer dummy)
{
  GstCaps *caps;

  caps = g_value_get_boxed (handler_return);
  GST_DEBUG ("got caps %" GST_PTR_FORMAT, caps);

  if (!(ihint->run_type & G_SIGNAL_RUN_CLEANUP))
    g_value_set_boxed (return_accu, caps);

  /* stop emission if we have a caps */
  return (caps == NULL);
}

static void
gst_rtp_bin_class_init (GstRtpBinClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBinClass *gstbin_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbin_class = (GstBinClass *) klass;

  gobject_class->dispose = gst_rtp_bin_dispose;
  gobject_class->finalize = gst_rtp_bin_finalize;
  gobject_class->set_property = gst_rtp_bin_set_property;
  gobject_class->get_property = gst_rtp_bin_get_property;

  g_object_class_install_property (gobject_class, PROP_LATENCY,
      g_param_spec_uint ("latency", "Buffer latency in ms",
          "Default amount of ms to buffer in the jitterbuffers", 0,
          G_MAXUINT, DEFAULT_LATENCY_MS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DROP_ON_LATENCY,
      g_param_spec_boolean ("drop-on-latency",
          "Drop buffers when maximum latency is reached",
          "Tells the jitterbuffer to never exceed the given latency in size",
          DEFAULT_DROP_ON_LATENCY, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtpBin::request-pt-map:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @pt: the pt
   *
   * Request the payload type as #GstCaps for @pt in @session.
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_PT_MAP] =
      g_signal_new ("request-pt-map", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, request_pt_map),
      _gst_caps_accumulator, NULL, NULL, GST_TYPE_CAPS, 2, G_TYPE_UINT,
      G_TYPE_UINT);

    /**
   * GstRtpBin::payload-type-change:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @pt: the pt
   *
   * Signal that the current payload type changed to @pt in @session.
   */
  gst_rtp_bin_signals[SIGNAL_PAYLOAD_TYPE_CHANGE] =
      g_signal_new ("payload-type-change", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, payload_type_change),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  /**
   * GstRtpBin::clear-pt-map:
   * @rtpbin: the object which received the signal
   *
   * Clear all previously cached pt-mapping obtained with
   * #GstRtpBin::request-pt-map.
   */
  gst_rtp_bin_signals[SIGNAL_CLEAR_PT_MAP] =
      g_signal_new ("clear-pt-map", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRtpBinClass,
          clear_pt_map), NULL, NULL, NULL, G_TYPE_NONE, 0, G_TYPE_NONE);

  /**
   * GstRtpBin::reset-sync:
   * @rtpbin: the object which received the signal
   *
   * Reset all currently configured lip-sync parameters and require new SR
   * packets for all streams before lip-sync is attempted again.
   */
  gst_rtp_bin_signals[SIGNAL_RESET_SYNC] =
      g_signal_new ("reset-sync", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRtpBinClass,
          reset_sync), NULL, NULL, NULL, G_TYPE_NONE, 0, G_TYPE_NONE);

  /**
   * GstRtpBin::get-session:
   * @rtpbin: the object which received the signal
   * @id: the session id
   *
   * Request the related GstRtpSession as #GstElement related with session @id.
   *
   * Since: 1.8
   */
  gst_rtp_bin_signals[SIGNAL_GET_SESSION] =
      g_signal_new ("get-session", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRtpBinClass,
          get_session), NULL, NULL, NULL, GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::get-internal-session:
   * @rtpbin: the object which received the signal
   * @id: the session id
   *
   * Request the internal RTPSession object as #GObject in session @id.
   */
  gst_rtp_bin_signals[SIGNAL_GET_INTERNAL_SESSION] =
      g_signal_new ("get-internal-session", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRtpBinClass,
          get_internal_session), NULL, NULL, NULL, RTP_TYPE_SESSION, 1,
      G_TYPE_UINT);

  /**
   * GstRtpBin::get-internal-storage:
   * @rtpbin: the object which received the signal
   * @id: the session id
   *
   * Request the internal RTPStorage object as #GObject in session @id. This
   * is the internal storage used by the RTPStorage element, which is used to
   * keep a backlog of received RTP packets for the session @id.
   *
   * Since: 1.14
   */
  gst_rtp_bin_signals[SIGNAL_GET_INTERNAL_STORAGE] =
      g_signal_new ("get-internal-storage", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRtpBinClass,
          get_internal_storage), NULL, NULL, NULL, G_TYPE_OBJECT, 1,
      G_TYPE_UINT);

  /**
   * GstRtpBin::get-storage:
   * @rtpbin: the object which received the signal
   * @id: the session id
   *
   * Request the RTPStorage element as #GObject in session @id. This element
   * is used to keep a backlog of received RTP packets for the session @id.
   *
   * Since: 1.16
   */
  gst_rtp_bin_signals[SIGNAL_GET_STORAGE] =
      g_signal_new ("get-storage", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRtpBinClass,
          get_storage), NULL, NULL, NULL, GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::on-new-ssrc:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify of a new SSRC that entered @session.
   */
  gst_rtp_bin_signals[SIGNAL_ON_NEW_SSRC] =
      g_signal_new ("on-new-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_new_ssrc),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  /**
   * GstRtpBin::on-ssrc-collision:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify when we have an SSRC collision
   */
  gst_rtp_bin_signals[SIGNAL_ON_SSRC_COLLISION] =
      g_signal_new ("on-ssrc-collision", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_ssrc_collision),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  /**
   * GstRtpBin::on-ssrc-validated:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify of a new SSRC that became validated.
   */
  gst_rtp_bin_signals[SIGNAL_ON_SSRC_VALIDATED] =
      g_signal_new ("on-ssrc-validated", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_ssrc_validated),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  /**
   * GstRtpBin::on-ssrc-active:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify of a SSRC that is active, i.e., sending RTCP.
   */
  gst_rtp_bin_signals[SIGNAL_ON_SSRC_ACTIVE] =
      g_signal_new ("on-ssrc-active", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_ssrc_active),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  /**
   * GstRtpBin::on-ssrc-sdes:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify of a SSRC that is active, i.e., sending RTCP.
   */
  gst_rtp_bin_signals[SIGNAL_ON_SSRC_SDES] =
      g_signal_new ("on-ssrc-sdes", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_ssrc_sdes),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  /**
   * GstRtpBin::on-bye-ssrc:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify of an SSRC that became inactive because of a BYE packet.
   */
  gst_rtp_bin_signals[SIGNAL_ON_BYE_SSRC] =
      g_signal_new ("on-bye-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_bye_ssrc),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  /**
   * GstRtpBin::on-bye-timeout:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify of an SSRC that has timed out because of BYE
   */
  gst_rtp_bin_signals[SIGNAL_ON_BYE_TIMEOUT] =
      g_signal_new ("on-bye-timeout", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_bye_timeout),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  /**
   * GstRtpBin::on-timeout:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify of an SSRC that has timed out
   */
  gst_rtp_bin_signals[SIGNAL_ON_TIMEOUT] =
      g_signal_new ("on-timeout", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_timeout),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  /**
   * GstRtpBin::on-sender-timeout:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify of a sender SSRC that has timed out and became a receiver
   */
  gst_rtp_bin_signals[SIGNAL_ON_SENDER_TIMEOUT] =
      g_signal_new ("on-sender-timeout", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_sender_timeout),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  /**
   * GstRtpBin::on-npt-stop:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify that SSRC sender has sent data up to the configured NPT stop time.
   */
  gst_rtp_bin_signals[SIGNAL_ON_NPT_STOP] =
      g_signal_new ("on-npt-stop", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_npt_stop),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  /**
   * GstRtpBin::request-rtp-encoder:
   * @rtpbin: the object which received the signal
   * @session: the session
   *
   * Request an RTP encoder element for the given @session. The encoder
   * element will be added to the bin if not previously added.
   *
   * If no handler is connected, no encoder will be used.
   *
   * Since: 1.4
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_RTP_ENCODER] =
      g_signal_new ("request-rtp-encoder", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          request_rtp_encoder), _gst_element_accumulator, NULL, NULL,
      GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::request-rtp-decoder:
   * @rtpbin: the object which received the signal
   * @session: the session
   *
   * Request an RTP decoder element for the given @session. The decoder
   * element will be added to the bin if not previously added.
   *
   * If no handler is connected, no encoder will be used.
   *
   * Since: 1.4
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_RTP_DECODER] =
      g_signal_new ("request-rtp-decoder", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          request_rtp_decoder), _gst_element_accumulator, NULL,
      NULL, GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::request-rtcp-encoder:
   * @rtpbin: the object which received the signal
   * @session: the session
   *
   * Request an RTCP encoder element for the given @session. The encoder
   * element will be added to the bin if not previously added.
   *
   * If no handler is connected, no encoder will be used.
   *
   * Since: 1.4
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_RTCP_ENCODER] =
      g_signal_new ("request-rtcp-encoder", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          request_rtcp_encoder), _gst_element_accumulator, NULL, NULL,
      GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::request-rtcp-decoder:
   * @rtpbin: the object which received the signal
   * @session: the session
   *
   * Request an RTCP decoder element for the given @session. The decoder
   * element will be added to the bin if not previously added.
   *
   * If no handler is connected, no encoder will be used.
   *
   * Since: 1.4
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_RTCP_DECODER] =
      g_signal_new ("request-rtcp-decoder", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          request_rtcp_decoder), _gst_element_accumulator, NULL, NULL,
      GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::request-jitterbuffer:
   * @rtpbin: the object which received the signal
   * @session: the session
   *
   * Request a jitterbuffer element for the given @session.
   *
   * If no handler is connected, the default jitterbuffer will be used.
   *
   * Note: The provided element is expected to conform to the API exposed
   * by the standard #GstRtpJitterBuffer. Runtime checks will be made to
   * determine whether it exposes properties and signals before attempting
   * to set, call or connect to them, and some functionalities of #GstRtpBin
   * may not be available when that is not the case.
   *
   * This should be considered experimental API, as the standard jitterbuffer
   * API is susceptible to change, provided elements will have to update their
   * custom jitterbuffer's API to match the API of #GstRtpJitterBuffer if and
   * when it changes.
   *
   * Since: 1.18
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_JITTERBUFFER] =
      g_signal_new ("request-jitterbuffer", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          request_jitterbuffer), _gst_element_accumulator, NULL,
      g_cclosure_marshal_generic, GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::new-jitterbuffer:
   * @rtpbin: the object which received the signal
   * @jitterbuffer: the new jitterbuffer
   * @session: the session
   * @ssrc: the SSRC
   *
   * Notify that a new @jitterbuffer was created for @session and @ssrc.
   * This signal can, for example, be used to configure @jitterbuffer.
   *
   * Since: 1.4
   */
  gst_rtp_bin_signals[SIGNAL_NEW_JITTERBUFFER] =
      g_signal_new ("new-jitterbuffer", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          new_jitterbuffer), NULL, NULL, NULL,
      G_TYPE_NONE, 3, GST_TYPE_ELEMENT, G_TYPE_UINT, G_TYPE_UINT);

  /**
   * GstRtpBin::new-storage:
   * @rtpbin: the object which received the signal
   * @storage: the new storage
   * @session: the session
   *
   * Notify that a new @storage was created for @session.
   * This signal can, for example, be used to configure @storage.
   *
   * Since: 1.14
   */
  gst_rtp_bin_signals[SIGNAL_NEW_STORAGE] =
      g_signal_new ("new-storage", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          new_storage), NULL, NULL, NULL,
      G_TYPE_NONE, 2, GST_TYPE_ELEMENT, G_TYPE_UINT);

  /**
   * GstRtpBin::request-aux-sender:
   * @rtpbin: the object which received the signal
   * @session: the session
   *
   * Request an AUX sender element for the given @session. The AUX
   * element will be added to the bin.
   *
   * If no handler is connected, no AUX element will be used.
   *
   * Since: 1.4
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_AUX_SENDER] =
      g_signal_new ("request-aux-sender", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          request_aux_sender), _gst_element_accumulator, NULL, NULL,
      GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::request-aux-receiver:
   * @rtpbin: the object which received the signal
   * @session: the session
   *
   * Request an AUX receiver element for the given @session. The AUX
   * element will be added to the bin.
   *
   * If no handler is connected, no AUX element will be used.
   *
   * Since: 1.4
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_AUX_RECEIVER] =
      g_signal_new ("request-aux-receiver", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          request_aux_receiver), _gst_element_accumulator, NULL, NULL,
      GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::request-fec-decoder:
   * @rtpbin: the object which received the signal
   * @session: the session index
   *
   * Request a FEC decoder element for the given @session. The element
   * will be added to the bin after the pt demuxer.
   *
   * If no handler is connected, no FEC decoder will be used.
   *
   * Since: 1.14
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_FEC_DECODER] =
      g_signal_new ("request-fec-decoder", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          request_fec_decoder), _gst_element_accumulator, NULL, NULL,
      GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::request-fec-encoder:
   * @rtpbin: the object which received the signal
   * @session: the session index
   *
   * Request a FEC encoder element for the given @session. The element
   * will be added to the bin after the RTPSession.
   *
   * If no handler is connected, no FEC encoder will be used.
   *
   * Since: 1.14
   */
  gst_rtp_bin_signals[SIGNAL_REQUEST_FEC_ENCODER] =
      g_signal_new ("request-fec-encoder", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          request_fec_encoder), _gst_element_accumulator, NULL, NULL,
      GST_TYPE_ELEMENT, 1, G_TYPE_UINT);

  /**
   * GstRtpBin::on-new-sender-ssrc:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the sender SSRC
   *
   * Notify of a new sender SSRC that entered @session.
   *
   * Since: 1.8
   */
  gst_rtp_bin_signals[SIGNAL_ON_NEW_SENDER_SSRC] =
      g_signal_new ("on-new-sender-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass, on_new_sender_ssrc),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  /**
   * GstRtpBin::on-sender-ssrc-active:
   * @rtpbin: the object which received the signal
   * @session: the session
   * @ssrc: the sender SSRC
   *
   * Notify of a sender SSRC that is active, i.e., sending RTCP.
   *
   * Since: 1.8
   */
  gst_rtp_bin_signals[SIGNAL_ON_SENDER_SSRC_ACTIVE] =
      g_signal_new ("on-sender-ssrc-active", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpBinClass,
          on_sender_ssrc_active), NULL, NULL, NULL,
      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  g_object_class_install_property (gobject_class, PROP_SDES,
      g_param_spec_boxed ("sdes", "SDES",
          "The SDES items of this session",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
          | GST_PARAM_DOC_SHOW_DEFAULT));

  g_object_class_install_property (gobject_class, PROP_DO_LOST,
      g_param_spec_boolean ("do-lost", "Do Lost",
          "Send an event downstream when a packet is lost", DEFAULT_DO_LOST,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_AUTOREMOVE,
      g_param_spec_boolean ("autoremove", "Auto Remove",
          "Automatically remove timed out sources", DEFAULT_AUTOREMOVE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_IGNORE_PT,
      g_param_spec_boolean ("ignore-pt", "Ignore PT",
          "Do not demultiplex based on PT values", DEFAULT_IGNORE_PT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_USE_PIPELINE_CLOCK,
      g_param_spec_boolean ("use-pipeline-clock", "Use pipeline clock",
          "Use the pipeline running-time to set the NTP time in the RTCP SR messages "
          "(DEPRECATED: Use ntp-time-source property)",
          DEFAULT_USE_PIPELINE_CLOCK,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));
  /**
   * GstRtpBin:buffer-mode:
   *
   * Control the buffering and timestamping mode used by the jitterbuffer.
   */
  g_object_class_install_property (gobject_class, PROP_BUFFER_MODE,
      g_param_spec_enum ("buffer-mode", "Buffer Mode",
          "Control the buffering algorithm in use", RTP_TYPE_JITTER_BUFFER_MODE,
          DEFAULT_BUFFER_MODE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRtpBin:ntp-sync:
   *
   * Set the NTP time from the sender reports as the running-time on the
   * buffers. When both the sender and receiver have sychronized
   * running-time, i.e. when the clock and base-time is shared
   * between the receivers and the and the senders, this option can be
   * used to synchronize receivers on multiple machines.
   */
  g_object_class_install_property (gobject_class, PROP_NTP_SYNC,
      g_param_spec_boolean ("ntp-sync", "Sync on NTP clock",
          "Synchronize received streams to the NTP clock", DEFAULT_NTP_SYNC,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtpBin:rtcp-sync:
   *
   * If not synchronizing (directly) to the NTP clock, determines how to sync
   * the various streams.
   */
  g_object_class_install_property (gobject_class, PROP_RTCP_SYNC,
      g_param_spec_enum ("rtcp-sync", "RTCP Sync",
          "Use of RTCP SR in synchronization", GST_RTP_BIN_RTCP_SYNC_TYPE,
          DEFAULT_RTCP_SYNC, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtpBin:rtcp-sync-interval:
   *
   * Determines how often to sync streams using RTCP data.
   */
  g_object_class_install_property (gobject_class, PROP_RTCP_SYNC_INTERVAL,
      g_param_spec_uint ("rtcp-sync-interval", "RTCP Sync Interval",
          "RTCP SR interval synchronization (ms) (0 = always)",
          0, G_MAXUINT, DEFAULT_RTCP_SYNC_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DO_SYNC_EVENT,
      g_param_spec_boolean ("do-sync-event", "Do Sync Event",
          "Send event downstream when a stream is synchronized to the sender",
          DEFAULT_DO_SYNC_EVENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtpBin:do-retransmission:
   *
   * Enables RTP retransmission on all streams. To control retransmission on
   * a per-SSRC basis, connect to the #GstRtpBin::new-jitterbuffer signal and
   * set the #GstRtpJitterBuffer:do-retransmission property on the
   * #GstRtpJitterBuffer object instead.
   */
  g_object_class_install_property (gobject_class, PROP_DO_RETRANSMISSION,
      g_param_spec_boolean ("do-retransmission", "Do retransmission",
          "Enable retransmission on all streams",
          DEFAULT_DO_RETRANSMISSION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtpBin:rtp-profile:
   *
   * Sets the default RTP profile of newly created RTP sessions. The
   * profile can be changed afterwards on a per-session basis.
   */
  g_object_class_install_property (gobject_class, PROP_RTP_PROFILE,
      g_param_spec_enum ("rtp-profile", "RTP Profile",
          "Default RTP profile of newly created sessions",
          GST_TYPE_RTP_PROFILE, DEFAULT_RTP_PROFILE,
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

  g_object_class_install_property (gobject_class, PROP_MAX_RTCP_RTP_TIME_DIFF,
      g_param_spec_int ("max-rtcp-rtp-time-diff", "Max RTCP RTP Time Diff",
          "Maximum amount of time in ms that the RTP time in RTCP SRs "
          "is allowed to be ahead (-1 disabled)", -1, G_MAXINT,
          DEFAULT_MAX_RTCP_RTP_TIME_DIFF,
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

  g_object_class_install_property (gobject_class, PROP_RFC7273_SYNC,
      g_param_spec_boolean ("rfc7273-sync", "Sync on RFC7273 clock",
          "Synchronize received streams to the RFC7273 clock "
          "(requires clock and offset to be provided)", DEFAULT_RFC7273_SYNC,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_STREAMS,
      g_param_spec_uint ("max-streams", "Max Streams",
          "The maximum number of streams to create for one session",
          0, G_MAXUINT, DEFAULT_MAX_STREAMS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtpBin:max-ts-offset-adjustment:
   *
   * Syncing time stamps to NTP time adds a time offset. This parameter
   * specifies the maximum number of nanoseconds per frame that this time offset
   * may be adjusted with. This is used to avoid sudden large changes to time
   * stamps.
   *
   * Since: 1.14
   */
  g_object_class_install_property (gobject_class, PROP_MAX_TS_OFFSET_ADJUSTMENT,
      g_param_spec_uint64 ("max-ts-offset-adjustment",
          "Max Timestamp Offset Adjustment",
          "The maximum number of nanoseconds per frame that time stamp offsets "
          "may be adjusted (0 = no limit).", 0, G_MAXUINT64,
          DEFAULT_MAX_TS_OFFSET_ADJUSTMENT, G_PARAM_READWRITE |
          G_PARAM_STATIC_STRINGS));

  /**
   * GstRtpBin:max-ts-offset:
   *
   * Used to set an upper limit of how large a time offset may be. This
   * is used to protect against unrealistic values as a result of either
   * client,server or clock issues.
   *
   * Since: 1.14
   */
  g_object_class_install_property (gobject_class, PROP_MAX_TS_OFFSET,
      g_param_spec_int64 ("max-ts-offset", "Max TS Offset",
          "The maximum absolute value of the time offset in (nanoseconds). "
          "Note, if the ntp-sync parameter is set the default value is "
          "changed to 0 (no limit)", 0, G_MAXINT64, DEFAULT_MAX_TS_OFFSET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_rtp_bin_change_state);
  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_rtp_bin_request_new_pad);
  gstelement_class->release_pad = GST_DEBUG_FUNCPTR (gst_rtp_bin_release_pad);

  /* sink pads */
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpbin_recv_rtp_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpbin_recv_rtcp_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpbin_send_rtp_sink_template);

  /* src pads */
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpbin_recv_rtp_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpbin_send_rtcp_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &rtpbin_send_rtp_src_template);

  gst_element_class_set_static_metadata (gstelement_class, "RTP Bin",
      "Filter/Network/RTP",
      "Real-Time Transport Protocol bin",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstbin_class->handle_message = GST_DEBUG_FUNCPTR (gst_rtp_bin_handle_message);

  klass->clear_pt_map = GST_DEBUG_FUNCPTR (gst_rtp_bin_clear_pt_map);
  klass->reset_sync = GST_DEBUG_FUNCPTR (gst_rtp_bin_reset_sync);
  klass->get_session = GST_DEBUG_FUNCPTR (gst_rtp_bin_get_session);
  klass->get_internal_session =
      GST_DEBUG_FUNCPTR (gst_rtp_bin_get_internal_session);
  klass->get_storage = GST_DEBUG_FUNCPTR (gst_rtp_bin_get_storage);
  klass->get_internal_storage =
      GST_DEBUG_FUNCPTR (gst_rtp_bin_get_internal_storage);
  klass->request_rtp_encoder = GST_DEBUG_FUNCPTR (gst_rtp_bin_request_encoder);
  klass->request_rtp_decoder = GST_DEBUG_FUNCPTR (gst_rtp_bin_request_decoder);
  klass->request_rtcp_encoder = GST_DEBUG_FUNCPTR (gst_rtp_bin_request_encoder);
  klass->request_rtcp_decoder = GST_DEBUG_FUNCPTR (gst_rtp_bin_request_decoder);
  klass->request_jitterbuffer =
      GST_DEBUG_FUNCPTR (gst_rtp_bin_request_jitterbuffer);

  GST_DEBUG_CATEGORY_INIT (gst_rtp_bin_debug, "rtpbin", 0, "RTP bin");
}

static void
gst_rtp_bin_init (GstRtpBin * rtpbin)
{
  gchar *cname;

  rtpbin->priv = gst_rtp_bin_get_instance_private (rtpbin);
  g_mutex_init (&rtpbin->priv->bin_lock);
  g_mutex_init (&rtpbin->priv->dyn_lock);

  rtpbin->latency_ms = DEFAULT_LATENCY_MS;
  rtpbin->latency_ns = DEFAULT_LATENCY_MS * GST_MSECOND;
  rtpbin->drop_on_latency = DEFAULT_DROP_ON_LATENCY;
  rtpbin->do_lost = DEFAULT_DO_LOST;
  rtpbin->ignore_pt = DEFAULT_IGNORE_PT;
  rtpbin->ntp_sync = DEFAULT_NTP_SYNC;
  rtpbin->rtcp_sync = DEFAULT_RTCP_SYNC;
  rtpbin->rtcp_sync_interval = DEFAULT_RTCP_SYNC_INTERVAL;
  rtpbin->priv->autoremove = DEFAULT_AUTOREMOVE;
  rtpbin->buffer_mode = DEFAULT_BUFFER_MODE;
  rtpbin->use_pipeline_clock = DEFAULT_USE_PIPELINE_CLOCK;
  rtpbin->send_sync_event = DEFAULT_DO_SYNC_EVENT;
  rtpbin->do_retransmission = DEFAULT_DO_RETRANSMISSION;
  rtpbin->rtp_profile = DEFAULT_RTP_PROFILE;
  rtpbin->ntp_time_source = DEFAULT_NTP_TIME_SOURCE;
  rtpbin->rtcp_sync_send_time = DEFAULT_RTCP_SYNC_SEND_TIME;
  rtpbin->max_rtcp_rtp_time_diff = DEFAULT_MAX_RTCP_RTP_TIME_DIFF;
  rtpbin->max_dropout_time = DEFAULT_MAX_DROPOUT_TIME;
  rtpbin->max_misorder_time = DEFAULT_MAX_MISORDER_TIME;
  rtpbin->rfc7273_sync = DEFAULT_RFC7273_SYNC;
  rtpbin->max_streams = DEFAULT_MAX_STREAMS;
  rtpbin->max_ts_offset_adjustment = DEFAULT_MAX_TS_OFFSET_ADJUSTMENT;
  rtpbin->max_ts_offset = DEFAULT_MAX_TS_OFFSET;
  rtpbin->max_ts_offset_is_set = FALSE;

  /* some default SDES entries */
  cname = g_strdup_printf ("user%u@host-%x", g_random_int (), g_random_int ());
  rtpbin->sdes = gst_structure_new ("application/x-rtp-source-sdes",
      "cname", G_TYPE_STRING, cname, "tool", G_TYPE_STRING, "GStreamer", NULL);
  g_free (cname);
}

static void
gst_rtp_bin_dispose (GObject * object)
{
  GstRtpBin *rtpbin;

  rtpbin = GST_RTP_BIN (object);

  GST_RTP_BIN_LOCK (rtpbin);
  GST_DEBUG_OBJECT (object, "freeing sessions");
  g_slist_foreach (rtpbin->sessions, (GFunc) free_session, rtpbin);
  g_slist_free (rtpbin->sessions);
  rtpbin->sessions = NULL;
  GST_RTP_BIN_UNLOCK (rtpbin);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_rtp_bin_finalize (GObject * object)
{
  GstRtpBin *rtpbin;

  rtpbin = GST_RTP_BIN (object);

  if (rtpbin->sdes)
    gst_structure_free (rtpbin->sdes);

  g_mutex_clear (&rtpbin->priv->bin_lock);
  g_mutex_clear (&rtpbin->priv->dyn_lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gst_rtp_bin_set_sdes_struct (GstRtpBin * bin, const GstStructure * sdes)
{
  GSList *item;

  if (sdes == NULL)
    return;

  GST_RTP_BIN_LOCK (bin);

  GST_OBJECT_LOCK (bin);
  if (bin->sdes)
    gst_structure_free (bin->sdes);
  bin->sdes = gst_structure_copy (sdes);
  GST_OBJECT_UNLOCK (bin);

  /* store in all sessions */
  for (item = bin->sessions; item; item = g_slist_next (item)) {
    GstRtpBinSession *session = item->data;
    g_object_set (session->session, "sdes", sdes, NULL);
  }

  GST_RTP_BIN_UNLOCK (bin);
}

static GstStructure *
gst_rtp_bin_get_sdes_struct (GstRtpBin * bin)
{
  GstStructure *result;

  GST_OBJECT_LOCK (bin);
  result = gst_structure_copy (bin->sdes);
  GST_OBJECT_UNLOCK (bin);

  return result;
}

static void
gst_rtp_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpBin *rtpbin;

  rtpbin = GST_RTP_BIN (object);

  switch (prop_id) {
    case PROP_LATENCY:
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->latency_ms = g_value_get_uint (value);
      rtpbin->latency_ns = rtpbin->latency_ms * GST_MSECOND;
      GST_RTP_BIN_UNLOCK (rtpbin);
      /* propagate the property down to the jitterbuffer */
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin, "latency", value);
      break;
    case PROP_DROP_ON_LATENCY:
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->drop_on_latency = g_value_get_boolean (value);
      GST_RTP_BIN_UNLOCK (rtpbin);
      /* propagate the property down to the jitterbuffer */
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin,
          "drop-on-latency", value);
      break;
    case PROP_SDES:
      gst_rtp_bin_set_sdes_struct (rtpbin, g_value_get_boxed (value));
      break;
    case PROP_DO_LOST:
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->do_lost = g_value_get_boolean (value);
      GST_RTP_BIN_UNLOCK (rtpbin);
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin, "do-lost", value);
      break;
    case PROP_NTP_SYNC:
      rtpbin->ntp_sync = g_value_get_boolean (value);
      /* The default value of max_ts_offset depends on ntp_sync. If user
       * hasn't set it then change default value */
      if (!rtpbin->max_ts_offset_is_set) {
        if (rtpbin->ntp_sync) {
          rtpbin->max_ts_offset = 0;
        } else {
          rtpbin->max_ts_offset = DEFAULT_MAX_TS_OFFSET;
        }
      }
      break;
    case PROP_RTCP_SYNC:
      g_atomic_int_set (&rtpbin->rtcp_sync, g_value_get_enum (value));
      break;
    case PROP_RTCP_SYNC_INTERVAL:
      rtpbin->rtcp_sync_interval = g_value_get_uint (value);
      break;
    case PROP_IGNORE_PT:
      rtpbin->ignore_pt = g_value_get_boolean (value);
      break;
    case PROP_AUTOREMOVE:
      rtpbin->priv->autoremove = g_value_get_boolean (value);
      break;
    case PROP_USE_PIPELINE_CLOCK:
    {
      GSList *sessions;
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->use_pipeline_clock = g_value_get_boolean (value);
      for (sessions = rtpbin->sessions; sessions;
          sessions = g_slist_next (sessions)) {
        GstRtpBinSession *session = (GstRtpBinSession *) sessions->data;

        g_object_set (G_OBJECT (session->session),
            "use-pipeline-clock", rtpbin->use_pipeline_clock, NULL);
      }
      GST_RTP_BIN_UNLOCK (rtpbin);
    }
      break;
    case PROP_DO_SYNC_EVENT:
      rtpbin->send_sync_event = g_value_get_boolean (value);
      break;
    case PROP_BUFFER_MODE:
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->buffer_mode = g_value_get_enum (value);
      GST_RTP_BIN_UNLOCK (rtpbin);
      /* propagate the property down to the jitterbuffer */
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin, "mode", value);
      break;
    case PROP_DO_RETRANSMISSION:
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->do_retransmission = g_value_get_boolean (value);
      GST_RTP_BIN_UNLOCK (rtpbin);
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin,
          "do-retransmission", value);
      break;
    case PROP_RTP_PROFILE:
      rtpbin->rtp_profile = g_value_get_enum (value);
      break;
    case PROP_NTP_TIME_SOURCE:{
      GSList *sessions;
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->ntp_time_source = g_value_get_enum (value);
      for (sessions = rtpbin->sessions; sessions;
          sessions = g_slist_next (sessions)) {
        GstRtpBinSession *session = (GstRtpBinSession *) sessions->data;

        g_object_set (G_OBJECT (session->session),
            "ntp-time-source", rtpbin->ntp_time_source, NULL);
      }
      GST_RTP_BIN_UNLOCK (rtpbin);
      break;
    }
    case PROP_RTCP_SYNC_SEND_TIME:{
      GSList *sessions;
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->rtcp_sync_send_time = g_value_get_boolean (value);
      for (sessions = rtpbin->sessions; sessions;
          sessions = g_slist_next (sessions)) {
        GstRtpBinSession *session = (GstRtpBinSession *) sessions->data;

        g_object_set (G_OBJECT (session->session),
            "rtcp-sync-send-time", rtpbin->rtcp_sync_send_time, NULL);
      }
      GST_RTP_BIN_UNLOCK (rtpbin);
      break;
    }
    case PROP_MAX_RTCP_RTP_TIME_DIFF:
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->max_rtcp_rtp_time_diff = g_value_get_int (value);
      GST_RTP_BIN_UNLOCK (rtpbin);
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin,
          "max-rtcp-rtp-time-diff", value);
      break;
    case PROP_MAX_DROPOUT_TIME:
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->max_dropout_time = g_value_get_uint (value);
      GST_RTP_BIN_UNLOCK (rtpbin);
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin,
          "max-dropout-time", value);
      gst_rtp_bin_propagate_property_to_session (rtpbin, "max-dropout-time",
          value);
      break;
    case PROP_MAX_MISORDER_TIME:
      GST_RTP_BIN_LOCK (rtpbin);
      rtpbin->max_misorder_time = g_value_get_uint (value);
      GST_RTP_BIN_UNLOCK (rtpbin);
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin,
          "max-misorder-time", value);
      gst_rtp_bin_propagate_property_to_session (rtpbin, "max-misorder-time",
          value);
      break;
    case PROP_RFC7273_SYNC:
      rtpbin->rfc7273_sync = g_value_get_boolean (value);
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin,
          "rfc7273-sync", value);
      break;
    case PROP_MAX_STREAMS:
      rtpbin->max_streams = g_value_get_uint (value);
      break;
    case PROP_MAX_TS_OFFSET_ADJUSTMENT:
      rtpbin->max_ts_offset_adjustment = g_value_get_uint64 (value);
      gst_rtp_bin_propagate_property_to_jitterbuffer (rtpbin,
          "max-ts-offset-adjustment", value);
      break;
    case PROP_MAX_TS_OFFSET:
      rtpbin->max_ts_offset = g_value_get_int64 (value);
      rtpbin->max_ts_offset_is_set = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpBin *rtpbin;

  rtpbin = GST_RTP_BIN (object);

  switch (prop_id) {
    case PROP_LATENCY:
      GST_RTP_BIN_LOCK (rtpbin);
      g_value_set_uint (value, rtpbin->latency_ms);
      GST_RTP_BIN_UNLOCK (rtpbin);
      break;
    case PROP_DROP_ON_LATENCY:
      GST_RTP_BIN_LOCK (rtpbin);
      g_value_set_boolean (value, rtpbin->drop_on_latency);
      GST_RTP_BIN_UNLOCK (rtpbin);
      break;
    case PROP_SDES:
      g_value_take_boxed (value, gst_rtp_bin_get_sdes_struct (rtpbin));
      break;
    case PROP_DO_LOST:
      GST_RTP_BIN_LOCK (rtpbin);
      g_value_set_boolean (value, rtpbin->do_lost);
      GST_RTP_BIN_UNLOCK (rtpbin);
      break;
    case PROP_IGNORE_PT:
      g_value_set_boolean (value, rtpbin->ignore_pt);
      break;
    case PROP_NTP_SYNC:
      g_value_set_boolean (value, rtpbin->ntp_sync);
      break;
    case PROP_RTCP_SYNC:
      g_value_set_enum (value, g_atomic_int_get (&rtpbin->rtcp_sync));
      break;
    case PROP_RTCP_SYNC_INTERVAL:
      g_value_set_uint (value, rtpbin->rtcp_sync_interval);
      break;
    case PROP_AUTOREMOVE:
      g_value_set_boolean (value, rtpbin->priv->autoremove);
      break;
    case PROP_BUFFER_MODE:
      g_value_set_enum (value, rtpbin->buffer_mode);
      break;
    case PROP_USE_PIPELINE_CLOCK:
      g_value_set_boolean (value, rtpbin->use_pipeline_clock);
      break;
    case PROP_DO_SYNC_EVENT:
      g_value_set_boolean (value, rtpbin->send_sync_event);
      break;
    case PROP_DO_RETRANSMISSION:
      GST_RTP_BIN_LOCK (rtpbin);
      g_value_set_boolean (value, rtpbin->do_retransmission);
      GST_RTP_BIN_UNLOCK (rtpbin);
      break;
    case PROP_RTP_PROFILE:
      g_value_set_enum (value, rtpbin->rtp_profile);
      break;
    case PROP_NTP_TIME_SOURCE:
      g_value_set_enum (value, rtpbin->ntp_time_source);
      break;
    case PROP_RTCP_SYNC_SEND_TIME:
      g_value_set_boolean (value, rtpbin->rtcp_sync_send_time);
      break;
    case PROP_MAX_RTCP_RTP_TIME_DIFF:
      GST_RTP_BIN_LOCK (rtpbin);
      g_value_set_int (value, rtpbin->max_rtcp_rtp_time_diff);
      GST_RTP_BIN_UNLOCK (rtpbin);
      break;
    case PROP_MAX_DROPOUT_TIME:
      g_value_set_uint (value, rtpbin->max_dropout_time);
      break;
    case PROP_MAX_MISORDER_TIME:
      g_value_set_uint (value, rtpbin->max_misorder_time);
      break;
    case PROP_RFC7273_SYNC:
      g_value_set_boolean (value, rtpbin->rfc7273_sync);
      break;
    case PROP_MAX_STREAMS:
      g_value_set_uint (value, rtpbin->max_streams);
      break;
    case PROP_MAX_TS_OFFSET_ADJUSTMENT:
      g_value_set_uint64 (value, rtpbin->max_ts_offset_adjustment);
      break;
    case PROP_MAX_TS_OFFSET:
      g_value_set_int64 (value, rtpbin->max_ts_offset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_bin_handle_message (GstBin * bin, GstMessage * message)
{
  GstRtpBin *rtpbin;

  rtpbin = GST_RTP_BIN (bin);

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure *s = gst_message_get_structure (message);

      /* we change the structure name and add the session ID to it */
      if (gst_structure_has_name (s, "application/x-rtp-source-sdes")) {
        GstRtpBinSession *sess;

        /* find the session we set it as object data */
        sess = g_object_get_data (G_OBJECT (GST_MESSAGE_SRC (message)),
            "GstRTPBin.session");

        if (G_LIKELY (sess)) {
          message = gst_message_make_writable (message);
          s = gst_message_get_structure (message);
          gst_structure_set ((GstStructure *) s, "session", G_TYPE_UINT,
              sess->id, NULL);
        }
      }
      GST_BIN_CLASS (parent_class)->handle_message (bin, message);
      break;
    }
    case GST_MESSAGE_BUFFERING:
    {
      gint percent;
      gint min_percent = 100;
      GSList *sessions, *streams;
      GstRtpBinStream *stream;
      gboolean change = FALSE, active = FALSE;
      GstClockTime min_out_time;
      GstBufferingMode mode;
      gint avg_in, avg_out;
      gint64 buffering_left;

      gst_message_parse_buffering (message, &percent);
      gst_message_parse_buffering_stats (message, &mode, &avg_in, &avg_out,
          &buffering_left);

      stream =
          g_object_get_data (G_OBJECT (GST_MESSAGE_SRC (message)),
          "GstRTPBin.stream");

      GST_DEBUG_OBJECT (bin, "got percent %d from stream %p", percent, stream);

      /* get the stream */
      if (G_LIKELY (stream)) {
        GST_RTP_BIN_LOCK (rtpbin);
        /* fill in the percent */
        stream->percent = percent;

        /* calculate the min value for all streams */
        for (sessions = rtpbin->sessions; sessions;
            sessions = g_slist_next (sessions)) {
          GstRtpBinSession *session = (GstRtpBinSession *) sessions->data;

          GST_RTP_SESSION_LOCK (session);
          if (session->streams) {
            for (streams = session->streams; streams;
                streams = g_slist_next (streams)) {
              GstRtpBinStream *stream = (GstRtpBinStream *) streams->data;

              GST_DEBUG_OBJECT (bin, "stream %p percent %d", stream,
                  stream->percent);

              /* find min percent */
              if (min_percent > stream->percent)
                min_percent = stream->percent;
            }
          } else {
            GST_INFO_OBJECT (bin,
                "session has no streams, setting min_percent to 0");
            min_percent = 0;
          }
          GST_RTP_SESSION_UNLOCK (session);
        }
        GST_DEBUG_OBJECT (bin, "min percent %d", min_percent);

        if (rtpbin->buffering) {
          if (min_percent == 100) {
            rtpbin->buffering = FALSE;
            active = TRUE;
            change = TRUE;
          }
        } else {
          if (min_percent < 100) {
            /* pause the streams */
            rtpbin->buffering = TRUE;
            active = FALSE;
            change = TRUE;
          }
        }
        GST_RTP_BIN_UNLOCK (rtpbin);

        gst_message_unref (message);

        /* make a new buffering message with the min value */
        message =
            gst_message_new_buffering (GST_OBJECT_CAST (bin), min_percent);
        gst_message_set_buffering_stats (message, mode, avg_in, avg_out,
            buffering_left);

        if (G_UNLIKELY (change)) {
          GstClock *clock;
          guint64 running_time = 0;
          guint64 offset = 0;

          /* figure out the running time when we have a clock */
          if (G_LIKELY ((clock =
                      gst_element_get_clock (GST_ELEMENT_CAST (bin))))) {
            guint64 now, base_time;

            now = gst_clock_get_time (clock);
            base_time = gst_element_get_base_time (GST_ELEMENT_CAST (bin));
            running_time = now - base_time;
            gst_object_unref (clock);
          }
          GST_DEBUG_OBJECT (bin,
              "running time now %" GST_TIME_FORMAT,
              GST_TIME_ARGS (running_time));

          GST_RTP_BIN_LOCK (rtpbin);

          /* when we reactivate, calculate the offsets so that all streams have
           * an output time that is at least as big as the running_time */
          offset = 0;
          if (active) {
            if (running_time > rtpbin->buffer_start) {
              offset = running_time - rtpbin->buffer_start;
              if (offset >= rtpbin->latency_ns)
                offset -= rtpbin->latency_ns;
              else
                offset = 0;
            }
          }

          /* pause all streams */
          min_out_time = -1;
          for (sessions = rtpbin->sessions; sessions;
              sessions = g_slist_next (sessions)) {
            GstRtpBinSession *session = (GstRtpBinSession *) sessions->data;

            GST_RTP_SESSION_LOCK (session);
            for (streams = session->streams; streams;
                streams = g_slist_next (streams)) {
              GstRtpBinStream *stream = (GstRtpBinStream *) streams->data;
              GstElement *element = stream->buffer;
              guint64 last_out;

              if (g_signal_lookup ("set-active", G_OBJECT_TYPE (element)) != 0) {
                g_signal_emit_by_name (element, "set-active", active, offset,
                    &last_out);
              }

              if (!active) {
                g_object_get (element, "percent", &stream->percent, NULL);

                if (last_out == -1)
                  last_out = 0;
                if (min_out_time == -1 || last_out < min_out_time)
                  min_out_time = last_out;
              }

              GST_DEBUG_OBJECT (bin,
                  "setting %p to %d, offset %" GST_TIME_FORMAT ", last %"
                  GST_TIME_FORMAT ", percent %d", element, active,
                  GST_TIME_ARGS (offset), GST_TIME_ARGS (last_out),
                  stream->percent);
            }
            GST_RTP_SESSION_UNLOCK (session);
          }
          GST_DEBUG_OBJECT (bin,
              "min out time %" GST_TIME_FORMAT, GST_TIME_ARGS (min_out_time));

          /* the buffer_start is the min out time of all paused jitterbuffers */
          if (!active)
            rtpbin->buffer_start = min_out_time;

          GST_RTP_BIN_UNLOCK (rtpbin);
        }
      }
      GST_BIN_CLASS (parent_class)->handle_message (bin, message);
      break;
    }
    default:
    {
      GST_BIN_CLASS (parent_class)->handle_message (bin, message);
      break;
    }
  }
}

static GstStateChangeReturn
gst_rtp_bin_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn res;
  GstRtpBin *rtpbin;
  GstRtpBinPrivate *priv;

  rtpbin = GST_RTP_BIN (element);
  priv = rtpbin->priv;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      priv->last_ntpnstime = 0;
      GST_LOG_OBJECT (rtpbin, "clearing shutdown flag");
      g_atomic_int_set (&priv->shutdown, 0);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_LOG_OBJECT (rtpbin, "setting shutdown flag");
      g_atomic_int_set (&priv->shutdown, 1);
      /* wait for all callbacks to end by taking the lock. No new callbacks will
       * be able to happen as we set the shutdown flag. */
      GST_RTP_BIN_DYN_LOCK (rtpbin);
      GST_LOG_OBJECT (rtpbin, "dynamic lock taken, we can continue shutdown");
      GST_RTP_BIN_DYN_UNLOCK (rtpbin);
      break;
    default:
      break;
  }

  res = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return res;
}

static GstElement *
session_request_element (GstRtpBinSession * session, guint signal)
{
  GstElement *element = NULL;
  GstRtpBin *bin = session->bin;

  g_signal_emit (bin, gst_rtp_bin_signals[signal], 0, session->id, &element);

  if (element) {
    if (!bin_manage_element (bin, element))
      goto manage_failed;
    session->elements = g_slist_prepend (session->elements, element);
  }
  return element;

  /* ERRORS */
manage_failed:
  {
    GST_WARNING_OBJECT (bin, "unable to manage element");
    gst_object_unref (element);
    return NULL;
  }
}

static gboolean
copy_sticky_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  GstPad *gpad = GST_PAD_CAST (user_data);

  GST_DEBUG_OBJECT (gpad, "store sticky event %" GST_PTR_FORMAT, *event);
  gst_pad_store_sticky_event (gpad, *event);

  return TRUE;
}

static void
expose_recv_src_pad (GstRtpBin * rtpbin, GstPad * pad, GstRtpBinStream * stream,
    guint8 pt)
{
  GstElementClass *klass;
  GstPadTemplate *templ;
  gchar *padname;
  GstPad *gpad;

  gst_object_ref (pad);

  if (stream->session->storage) {
    GstElement *fec_decoder =
        session_request_element (stream->session, SIGNAL_REQUEST_FEC_DECODER);

    if (fec_decoder) {
      GstPad *sinkpad, *srcpad;
      GstPadLinkReturn ret;

      sinkpad = gst_element_get_static_pad (fec_decoder, "sink");

      if (!sinkpad)
        goto fec_decoder_sink_failed;

      ret = gst_pad_link (pad, sinkpad);
      gst_object_unref (sinkpad);

      if (ret != GST_PAD_LINK_OK)
        goto fec_decoder_link_failed;

      srcpad = gst_element_get_static_pad (fec_decoder, "src");

      if (!srcpad)
        goto fec_decoder_src_failed;

      gst_pad_sticky_events_foreach (pad, copy_sticky_events, srcpad);
      gst_object_unref (pad);
      pad = srcpad;
    }
  }

  GST_RTP_BIN_SHUTDOWN_LOCK (rtpbin, shutdown);

  /* ghost the pad to the parent */
  klass = GST_ELEMENT_GET_CLASS (rtpbin);
  templ = gst_element_class_get_pad_template (klass, "recv_rtp_src_%u_%u_%u");
  padname = g_strdup_printf ("recv_rtp_src_%u_%u_%u",
      stream->session->id, stream->ssrc, pt);
  gpad = gst_ghost_pad_new_from_template (padname, pad, templ);
  g_free (padname);
  g_object_set_data (G_OBJECT (pad), "GstRTPBin.ghostpad", gpad);

  gst_pad_set_active (gpad, TRUE);
  GST_RTP_BIN_SHUTDOWN_UNLOCK (rtpbin);

  gst_pad_sticky_events_foreach (pad, copy_sticky_events, gpad);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpbin), gpad);

done:
  gst_object_unref (pad);

  return;

shutdown:
  {
    GST_DEBUG ("ignoring, we are shutting down");
    goto done;
  }
fec_decoder_sink_failed:
  {
    g_warning ("rtpbin: failed to get fec encoder sink pad for session %u",
        stream->session->id);
    goto done;
  }
fec_decoder_src_failed:
  {
    g_warning ("rtpbin: failed to get fec encoder src pad for session %u",
        stream->session->id);
    goto done;
  }
fec_decoder_link_failed:
  {
    g_warning ("rtpbin: failed to link fec decoder for session %u",
        stream->session->id);
    goto done;
  }
}

/* a new pad (SSRC) was created in @session. This signal is emitted from the
 * payload demuxer. */
static void
new_payload_found (GstElement * element, guint pt, GstPad * pad,
    GstRtpBinStream * stream)
{
  GstRtpBin *rtpbin;

  rtpbin = stream->bin;

  GST_DEBUG_OBJECT (rtpbin, "new payload pad %u", pt);

  expose_recv_src_pad (rtpbin, pad, stream, pt);
}

static void
payload_pad_removed (GstElement * element, GstPad * pad,
    GstRtpBinStream * stream)
{
  GstRtpBin *rtpbin;
  GstPad *gpad;

  rtpbin = stream->bin;

  GST_DEBUG ("payload pad removed");

  GST_RTP_BIN_DYN_LOCK (rtpbin);
  if ((gpad = g_object_get_data (G_OBJECT (pad), "GstRTPBin.ghostpad"))) {
    g_object_set_data (G_OBJECT (pad), "GstRTPBin.ghostpad", NULL);

    gst_pad_set_active (gpad, FALSE);
    gst_element_remove_pad (GST_ELEMENT_CAST (rtpbin), gpad);
  }
  GST_RTP_BIN_DYN_UNLOCK (rtpbin);
}

static GstCaps *
pt_map_requested (GstElement * element, guint pt, GstRtpBinSession * session)
{
  GstRtpBin *rtpbin;
  GstCaps *caps;

  rtpbin = session->bin;

  GST_DEBUG_OBJECT (rtpbin, "payload map requested for pt %u in session %u", pt,
      session->id);

  caps = get_pt_map (session, pt);
  if (!caps)
    goto no_caps;

  return caps;

  /* ERRORS */
no_caps:
  {
    GST_DEBUG_OBJECT (rtpbin, "could not get caps");
    return NULL;
  }
}

static GstCaps *
ptdemux_pt_map_requested (GstElement * element, guint pt,
    GstRtpBinSession * session)
{
  GstCaps *ret = pt_map_requested (element, pt, session);

  if (ret && gst_caps_get_size (ret) == 1) {
    const GstStructure *s = gst_caps_get_structure (ret, 0);
    gboolean is_fec;

    if (gst_structure_get_boolean (s, "is-fec", &is_fec) && is_fec) {
      GValue v = G_VALUE_INIT;
      GValue v2 = G_VALUE_INIT;

      GST_INFO_OBJECT (session->bin, "Will ignore FEC pt %u in session %u", pt,
          session->id);
      g_value_init (&v, GST_TYPE_ARRAY);
      g_value_init (&v2, G_TYPE_INT);
      g_object_get_property (G_OBJECT (element), "ignored-payload-types", &v);
      g_value_set_int (&v2, pt);
      gst_value_array_append_value (&v, &v2);
      g_value_unset (&v2);
      g_object_set_property (G_OBJECT (element), "ignored-payload-types", &v);
      g_value_unset (&v);
    }
  }

  return ret;
}

static void
payload_type_change (GstElement * element, guint pt, GstRtpBinSession * session)
{
  GST_DEBUG_OBJECT (session->bin,
      "emitting signal for pt type changed to %u in session %u", pt,
      session->id);

  g_signal_emit (session->bin, gst_rtp_bin_signals[SIGNAL_PAYLOAD_TYPE_CHANGE],
      0, session->id, pt);
}

/* emitted when caps changed for the session */
static void
caps_changed (GstPad * pad, GParamSpec * pspec, GstRtpBinSession * session)
{
  GstRtpBin *bin;
  GstCaps *caps;
  gint payload;
  const GstStructure *s;

  bin = session->bin;

  g_object_get (pad, "caps", &caps, NULL);

  if (caps == NULL)
    return;

  GST_DEBUG_OBJECT (bin, "got caps %" GST_PTR_FORMAT, caps);

  s = gst_caps_get_structure (caps, 0);

  /* get payload, finish when it's not there */
  if (!gst_structure_get_int (s, "payload", &payload)) {
    gst_caps_unref (caps);
    return;
  }

  GST_RTP_SESSION_LOCK (session);
  GST_DEBUG_OBJECT (bin, "insert caps for payload %d", payload);
  g_hash_table_insert (session->ptmap, GINT_TO_POINTER (payload), caps);
  GST_RTP_SESSION_UNLOCK (session);
}

/* a new pad (SSRC) was created in @session */
static void
new_ssrc_pad_found (GstElement * element, guint ssrc, GstPad * pad,
    GstRtpBinSession * session)
{
  GstRtpBin *rtpbin;
  GstRtpBinStream *stream;
  GstPad *sinkpad, *srcpad;
  gchar *padname;

  rtpbin = session->bin;

  GST_DEBUG_OBJECT (rtpbin, "new SSRC pad %08x, %s:%s", ssrc,
      GST_DEBUG_PAD_NAME (pad));

  GST_RTP_BIN_SHUTDOWN_LOCK (rtpbin, shutdown);

  GST_RTP_SESSION_LOCK (session);

  /* create new stream */
  stream = create_stream (session, ssrc);
  if (!stream)
    goto no_stream;

  /* get pad and link */
  GST_DEBUG_OBJECT (rtpbin, "linking jitterbuffer RTP");
  padname = g_strdup_printf ("src_%u", ssrc);
  srcpad = gst_element_get_static_pad (element, padname);
  g_free (padname);
  sinkpad = gst_element_get_static_pad (stream->buffer, "sink");
  gst_pad_link_full (srcpad, sinkpad, GST_PAD_LINK_CHECK_NOTHING);
  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  sinkpad = gst_element_get_request_pad (stream->buffer, "sink_rtcp");
  if (sinkpad) {
    GST_DEBUG_OBJECT (rtpbin, "linking jitterbuffer RTCP");
    padname = g_strdup_printf ("rtcp_src_%u", ssrc);
    srcpad = gst_element_get_static_pad (element, padname);
    g_free (padname);
    gst_pad_link_full (srcpad, sinkpad, GST_PAD_LINK_CHECK_NOTHING);
    gst_object_unref (sinkpad);
    gst_object_unref (srcpad);
  }

  if (g_signal_lookup ("handle-sync", G_OBJECT_TYPE (stream->buffer)) != 0) {
    /* connect to the RTCP sync signal from the jitterbuffer */
    GST_DEBUG_OBJECT (rtpbin, "connecting sync signal");
    stream->buffer_handlesync_sig = g_signal_connect (stream->buffer,
        "handle-sync", (GCallback) gst_rtp_bin_handle_sync, stream);
  }

  if (stream->demux) {
    /* connect to the new-pad signal of the payload demuxer, this will expose the
     * new pad by ghosting it. */
    stream->demux_newpad_sig = g_signal_connect (stream->demux,
        "new-payload-type", (GCallback) new_payload_found, stream);
    stream->demux_padremoved_sig = g_signal_connect (stream->demux,
        "pad-removed", (GCallback) payload_pad_removed, stream);

    /* connect to the request-pt-map signal. This signal will be emitted by the
     * demuxer so that it can apply a proper caps on the buffers for the
     * depayloaders. */
    stream->demux_ptreq_sig = g_signal_connect (stream->demux,
        "request-pt-map", (GCallback) ptdemux_pt_map_requested, session);
    /* connect to the  signal so it can be forwarded. */
    stream->demux_ptchange_sig = g_signal_connect (stream->demux,
        "payload-type-change", (GCallback) payload_type_change, session);

    GST_RTP_SESSION_UNLOCK (session);
    GST_RTP_BIN_SHUTDOWN_UNLOCK (rtpbin);
  } else {
    /* add rtpjitterbuffer src pad to pads */
    GstPad *pad;

    pad = gst_element_get_static_pad (stream->buffer, "src");

    GST_RTP_SESSION_UNLOCK (session);
    GST_RTP_BIN_SHUTDOWN_UNLOCK (rtpbin);

    expose_recv_src_pad (rtpbin, pad, stream, 255);

    gst_object_unref (pad);
  }

  return;

  /* ERRORS */
shutdown:
  {
    GST_DEBUG_OBJECT (rtpbin, "we are shutting down");
    return;
  }
no_stream:
  {
    GST_RTP_SESSION_UNLOCK (session);
    GST_RTP_BIN_SHUTDOWN_UNLOCK (rtpbin);
    GST_DEBUG_OBJECT (rtpbin, "could not create stream");
    return;
  }
}

static GstPad *
complete_session_sink (GstRtpBin * rtpbin, GstRtpBinSession * session)
{
  guint sessid = session->id;
  GstPad *recv_rtp_sink;
  GstElement *decoder;

  g_assert (!session->recv_rtp_sink);

  /* get recv_rtp pad and store */
  session->recv_rtp_sink =
      gst_element_get_request_pad (session->session, "recv_rtp_sink");
  if (session->recv_rtp_sink == NULL)
    goto pad_failed;

  g_signal_connect (session->recv_rtp_sink, "notify::caps",
      (GCallback) caps_changed, session);

  GST_DEBUG_OBJECT (rtpbin, "requesting RTP decoder");
  decoder = session_request_element (session, SIGNAL_REQUEST_RTP_DECODER);
  if (decoder) {
    GstPad *decsrc, *decsink;
    GstPadLinkReturn ret;

    GST_DEBUG_OBJECT (rtpbin, "linking RTP decoder");
    decsink = gst_element_get_static_pad (decoder, "rtp_sink");
    if (decsink == NULL)
      goto dec_sink_failed;

    recv_rtp_sink = decsink;

    decsrc = gst_element_get_static_pad (decoder, "rtp_src");
    if (decsrc == NULL)
      goto dec_src_failed;

    ret = gst_pad_link (decsrc, session->recv_rtp_sink);

    gst_object_unref (decsrc);

    if (ret != GST_PAD_LINK_OK)
      goto dec_link_failed;

  } else {
    GST_DEBUG_OBJECT (rtpbin, "no RTP decoder given");
    recv_rtp_sink = gst_object_ref (session->recv_rtp_sink);
  }

  return recv_rtp_sink;

  /* ERRORS */
pad_failed:
  {
    g_warning ("rtpbin: failed to get session recv_rtp_sink pad");
    return NULL;
  }
dec_sink_failed:
  {
    g_warning ("rtpbin: failed to get decoder sink pad for session %u", sessid);
    return NULL;
  }
dec_src_failed:
  {
    g_warning ("rtpbin: failed to get decoder src pad for session %u", sessid);
    gst_object_unref (recv_rtp_sink);
    return NULL;
  }
dec_link_failed:
  {
    g_warning ("rtpbin: failed to link rtp decoder for session %u", sessid);
    gst_object_unref (recv_rtp_sink);
    return NULL;
  }
}

static void
complete_session_receiver (GstRtpBin * rtpbin, GstRtpBinSession * session,
    guint sessid)
{
  GstElement *aux;
  GstPad *recv_rtp_src;

  g_assert (!session->recv_rtp_src);

  session->recv_rtp_src =
      gst_element_get_static_pad (session->session, "recv_rtp_src");
  if (session->recv_rtp_src == NULL)
    goto pad_failed;

  /* find out if we need AUX elements */
  aux = session_request_element (session, SIGNAL_REQUEST_AUX_RECEIVER);
  if (aux) {
    gchar *pname;
    GstPad *auxsink;
    GstPadLinkReturn ret;

    GST_DEBUG_OBJECT (rtpbin, "linking AUX receiver");

    pname = g_strdup_printf ("sink_%u", sessid);
    auxsink = gst_element_get_static_pad (aux, pname);
    g_free (pname);
    if (auxsink == NULL)
      goto aux_sink_failed;

    ret = gst_pad_link (session->recv_rtp_src, auxsink);
    gst_object_unref (auxsink);
    if (ret != GST_PAD_LINK_OK)
      goto aux_link_failed;

    /* this can be NULL when this AUX element is not to be linked any further */
    pname = g_strdup_printf ("src_%u", sessid);
    recv_rtp_src = gst_element_get_static_pad (aux, pname);
    g_free (pname);
  } else {
    recv_rtp_src = gst_object_ref (session->recv_rtp_src);
  }

  /* Add a storage element if needed */
  if (recv_rtp_src && session->storage) {
    GstPadLinkReturn ret;
    GstPad *sinkpad = gst_element_get_static_pad (session->storage, "sink");

    ret = gst_pad_link (recv_rtp_src, sinkpad);

    gst_object_unref (sinkpad);
    gst_object_unref (recv_rtp_src);

    if (ret != GST_PAD_LINK_OK)
      goto storage_link_failed;

    recv_rtp_src = gst_element_get_static_pad (session->storage, "src");
  }

  if (recv_rtp_src) {
    GstPad *sinkdpad;

    GST_DEBUG_OBJECT (rtpbin, "getting demuxer RTP sink pad");
    sinkdpad = gst_element_get_static_pad (session->demux, "sink");
    GST_DEBUG_OBJECT (rtpbin, "linking demuxer RTP sink pad");
    gst_pad_link_full (recv_rtp_src, sinkdpad, GST_PAD_LINK_CHECK_NOTHING);
    gst_object_unref (sinkdpad);
    gst_object_unref (recv_rtp_src);

    /* connect to the new-ssrc-pad signal of the SSRC demuxer */
    session->demux_newpad_sig = g_signal_connect (session->demux,
        "new-ssrc-pad", (GCallback) new_ssrc_pad_found, session);
    session->demux_padremoved_sig = g_signal_connect (session->demux,
        "removed-ssrc-pad", (GCallback) ssrc_demux_pad_removed, session);
  }

  return;

pad_failed:
  {
    g_warning ("rtpbin: failed to get session recv_rtp_src pad");
    return;
  }
aux_sink_failed:
  {
    g_warning ("rtpbin: failed to get AUX sink pad for session %u", sessid);
    return;
  }
aux_link_failed:
  {
    g_warning ("rtpbin: failed to link AUX pad to session %u", sessid);
    return;
  }
storage_link_failed:
  {
    g_warning ("rtpbin: failed to link storage");
    return;
  }
}

/* Create a pad for receiving RTP for the session in @name. Must be called with
 * RTP_BIN_LOCK.
 */
static GstPad *
create_recv_rtp (GstRtpBin * rtpbin, GstPadTemplate * templ, const gchar * name)
{
  guint sessid;
  GstRtpBinSession *session;
  GstPad *recv_rtp_sink;

  /* first get the session number */
  if (name == NULL || sscanf (name, "recv_rtp_sink_%u", &sessid) != 1)
    goto no_name;

  GST_DEBUG_OBJECT (rtpbin, "finding session %u", sessid);

  /* get or create session */
  session = find_session_by_id (rtpbin, sessid);
  if (!session) {
    GST_DEBUG_OBJECT (rtpbin, "creating session %u", sessid);
    /* create session now */
    session = create_session (rtpbin, sessid);
    if (session == NULL)
      goto create_error;
  }

  /* check if pad was requested */
  if (session->recv_rtp_sink_ghost != NULL)
    return session->recv_rtp_sink_ghost;

  /* setup the session sink pad */
  recv_rtp_sink = complete_session_sink (rtpbin, session);
  if (!recv_rtp_sink)
    goto session_sink_failed;

  GST_DEBUG_OBJECT (rtpbin, "ghosting session sink pad");
  session->recv_rtp_sink_ghost =
      gst_ghost_pad_new_from_template (name, recv_rtp_sink, templ);
  gst_object_unref (recv_rtp_sink);
  gst_pad_set_active (session->recv_rtp_sink_ghost, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpbin), session->recv_rtp_sink_ghost);

  complete_session_receiver (rtpbin, session, sessid);

  return session->recv_rtp_sink_ghost;

  /* ERRORS */
no_name:
  {
    g_warning ("rtpbin: invalid name given");
    return NULL;
  }
create_error:
  {
    /* create_session already warned */
    return NULL;
  }
session_sink_failed:
  {
    /* warning already done */
    return NULL;
  }
}

static void
remove_recv_rtp (GstRtpBin * rtpbin, GstRtpBinSession * session)
{
  if (session->demux_newpad_sig) {
    g_signal_handler_disconnect (session->demux, session->demux_newpad_sig);
    session->demux_newpad_sig = 0;
  }
  if (session->demux_padremoved_sig) {
    g_signal_handler_disconnect (session->demux, session->demux_padremoved_sig);
    session->demux_padremoved_sig = 0;
  }
  if (session->recv_rtp_src) {
    gst_object_unref (session->recv_rtp_src);
    session->recv_rtp_src = NULL;
  }
  if (session->recv_rtp_sink) {
    gst_element_release_request_pad (session->session, session->recv_rtp_sink);
    gst_object_unref (session->recv_rtp_sink);
    session->recv_rtp_sink = NULL;
  }
  if (session->recv_rtp_sink_ghost) {
    gst_pad_set_active (session->recv_rtp_sink_ghost, FALSE);
    gst_element_remove_pad (GST_ELEMENT_CAST (rtpbin),
        session->recv_rtp_sink_ghost);
    session->recv_rtp_sink_ghost = NULL;
  }
}

static GstPad *
complete_session_rtcp (GstRtpBin * rtpbin, GstRtpBinSession * session,
    guint sessid)
{
  GstElement *decoder;
  GstPad *sinkdpad;
  GstPad *decsink = NULL;

  /* get recv_rtp pad and store */
  GST_DEBUG_OBJECT (rtpbin, "getting RTCP sink pad");
  session->recv_rtcp_sink =
      gst_element_get_request_pad (session->session, "recv_rtcp_sink");
  if (session->recv_rtcp_sink == NULL)
    goto pad_failed;

  GST_DEBUG_OBJECT (rtpbin, "getting RTCP decoder");
  decoder = session_request_element (session, SIGNAL_REQUEST_RTCP_DECODER);
  if (decoder) {
    GstPad *decsrc;
    GstPadLinkReturn ret;

    GST_DEBUG_OBJECT (rtpbin, "linking RTCP decoder");
    decsink = gst_element_get_static_pad (decoder, "rtcp_sink");
    decsrc = gst_element_get_static_pad (decoder, "rtcp_src");

    if (decsink == NULL)
      goto dec_sink_failed;

    if (decsrc == NULL)
      goto dec_src_failed;

    ret = gst_pad_link (decsrc, session->recv_rtcp_sink);

    gst_object_unref (decsrc);

    if (ret != GST_PAD_LINK_OK)
      goto dec_link_failed;
  } else {
    GST_DEBUG_OBJECT (rtpbin, "no RTCP decoder given");
    decsink = gst_object_ref (session->recv_rtcp_sink);
  }

  /* get srcpad, link to SSRCDemux */
  GST_DEBUG_OBJECT (rtpbin, "getting sync src pad");
  session->sync_src = gst_element_get_static_pad (session->session, "sync_src");
  if (session->sync_src == NULL)
    goto src_pad_failed;

  GST_DEBUG_OBJECT (rtpbin, "getting demuxer RTCP sink pad");
  sinkdpad = gst_element_get_static_pad (session->demux, "rtcp_sink");
  gst_pad_link_full (session->sync_src, sinkdpad, GST_PAD_LINK_CHECK_NOTHING);
  gst_object_unref (sinkdpad);

  return decsink;

pad_failed:
  {
    g_warning ("rtpbin: failed to get session rtcp_sink pad");
    return NULL;
  }
dec_sink_failed:
  {
    g_warning ("rtpbin: failed to get decoder sink pad for session %u", sessid);
    return NULL;
  }
dec_src_failed:
  {
    g_warning ("rtpbin: failed to get decoder src pad for session %u", sessid);
    goto cleanup;
  }
dec_link_failed:
  {
    g_warning ("rtpbin: failed to link rtcp decoder for session %u", sessid);
    goto cleanup;
  }
src_pad_failed:
  {
    g_warning ("rtpbin: failed to get session sync_src pad");
  }

cleanup:
  gst_object_unref (decsink);
  return NULL;
}

/* Create a pad for receiving RTCP for the session in @name. Must be called with
 * RTP_BIN_LOCK.
 */
static GstPad *
create_recv_rtcp (GstRtpBin * rtpbin, GstPadTemplate * templ,
    const gchar * name)
{
  guint sessid;
  GstRtpBinSession *session;
  GstPad *decsink = NULL;

  /* first get the session number */
  if (name == NULL || sscanf (name, "recv_rtcp_sink_%u", &sessid) != 1)
    goto no_name;

  GST_DEBUG_OBJECT (rtpbin, "finding session %u", sessid);

  /* get or create the session */
  session = find_session_by_id (rtpbin, sessid);
  if (!session) {
    GST_DEBUG_OBJECT (rtpbin, "creating session %u", sessid);
    /* create session now */
    session = create_session (rtpbin, sessid);
    if (session == NULL)
      goto create_error;
  }

  /* check if pad was requested */
  if (session->recv_rtcp_sink_ghost != NULL)
    return session->recv_rtcp_sink_ghost;

  decsink = complete_session_rtcp (rtpbin, session, sessid);
  if (!decsink)
    goto create_error;

  session->recv_rtcp_sink_ghost =
      gst_ghost_pad_new_from_template (name, decsink, templ);
  gst_object_unref (decsink);
  gst_pad_set_active (session->recv_rtcp_sink_ghost, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpbin),
      session->recv_rtcp_sink_ghost);

  return session->recv_rtcp_sink_ghost;

  /* ERRORS */
no_name:
  {
    g_warning ("rtpbin: invalid name given");
    return NULL;
  }
create_error:
  {
    /* create_session already warned */
    return NULL;
  }
}

static void
remove_recv_rtcp (GstRtpBin * rtpbin, GstRtpBinSession * session)
{
  if (session->recv_rtcp_sink_ghost) {
    gst_pad_set_active (session->recv_rtcp_sink_ghost, FALSE);
    gst_element_remove_pad (GST_ELEMENT_CAST (rtpbin),
        session->recv_rtcp_sink_ghost);
    session->recv_rtcp_sink_ghost = NULL;
  }
  if (session->sync_src) {
    /* releasing the request pad should also unref the sync pad */
    gst_object_unref (session->sync_src);
    session->sync_src = NULL;
  }
  if (session->recv_rtcp_sink) {
    gst_element_release_request_pad (session->session, session->recv_rtcp_sink);
    gst_object_unref (session->recv_rtcp_sink);
    session->recv_rtcp_sink = NULL;
  }
}

static gboolean
complete_session_src (GstRtpBin * rtpbin, GstRtpBinSession * session)
{
  gchar *gname;
  guint sessid = session->id;
  GstPad *send_rtp_src;
  GstElement *encoder;
  GstElementClass *klass;
  GstPadTemplate *templ;
  gboolean ret = FALSE;

  /* get srcpad */
  send_rtp_src = gst_element_get_static_pad (session->session, "send_rtp_src");

  if (send_rtp_src == NULL)
    goto no_srcpad;

  GST_DEBUG_OBJECT (rtpbin, "getting RTP encoder");
  encoder = session_request_element (session, SIGNAL_REQUEST_RTP_ENCODER);
  if (encoder) {
    gchar *ename;
    GstPad *encsrc, *encsink;
    GstPadLinkReturn ret;

    GST_DEBUG_OBJECT (rtpbin, "linking RTP encoder");
    ename = g_strdup_printf ("rtp_src_%u", sessid);
    encsrc = gst_element_get_static_pad (encoder, ename);
    g_free (ename);

    if (encsrc == NULL)
      goto enc_src_failed;

    ename = g_strdup_printf ("rtp_sink_%u", sessid);
    encsink = gst_element_get_static_pad (encoder, ename);
    g_free (ename);
    if (encsink == NULL)
      goto enc_sink_failed;

    ret = gst_pad_link (send_rtp_src, encsink);
    gst_object_unref (encsink);
    gst_object_unref (send_rtp_src);

    send_rtp_src = encsrc;

    if (ret != GST_PAD_LINK_OK)
      goto enc_link_failed;
  } else {
    GST_DEBUG_OBJECT (rtpbin, "no RTP encoder given");
  }

  /* ghost the new source pad */
  klass = GST_ELEMENT_GET_CLASS (rtpbin);
  gname = g_strdup_printf ("send_rtp_src_%u", sessid);
  templ = gst_element_class_get_pad_template (klass, "send_rtp_src_%u");
  session->send_rtp_src_ghost =
      gst_ghost_pad_new_from_template (gname, send_rtp_src, templ);
  gst_pad_set_active (session->send_rtp_src_ghost, TRUE);
  gst_pad_sticky_events_foreach (send_rtp_src, copy_sticky_events,
      session->send_rtp_src_ghost);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpbin), session->send_rtp_src_ghost);
  g_free (gname);

  ret = TRUE;

done:
  if (send_rtp_src)
    gst_object_unref (send_rtp_src);

  return ret;

  /* ERRORS */
no_srcpad:
  {
    g_warning ("rtpbin: failed to get rtp source pad for session %u", sessid);
    goto done;
  }
enc_src_failed:
  {
    g_warning ("rtpbin: failed to get %" GST_PTR_FORMAT
        " src pad for session %u", encoder, sessid);
    goto done;
  }
enc_sink_failed:
  {
    g_warning ("rtpbin: failed to get %" GST_PTR_FORMAT
        " sink pad for session %u", encoder, sessid);
    goto done;
  }
enc_link_failed:
  {
    g_warning ("rtpbin: failed to link %" GST_PTR_FORMAT " for session %u",
        encoder, sessid);
    goto done;
  }
}

static gboolean
setup_aux_sender_fold (const GValue * item, GValue * result, gpointer user_data)
{
  GstPad *pad;
  gchar *name;
  guint sessid;
  GstRtpBinSession *session = user_data, *newsess;
  GstRtpBin *rtpbin = session->bin;
  GstPadLinkReturn ret;

  pad = g_value_get_object (item);
  name = gst_pad_get_name (pad);

  if (name == NULL || sscanf (name, "src_%u", &sessid) != 1)
    goto no_name;

  g_free (name);

  newsess = find_session_by_id (rtpbin, sessid);
  if (newsess == NULL) {
    /* create new session */
    newsess = create_session (rtpbin, sessid);
    if (newsess == NULL)
      goto create_error;
  } else if (newsess->send_rtp_sink != NULL)
    goto existing_session;

  /* get send_rtp pad and store */
  newsess->send_rtp_sink =
      gst_element_get_request_pad (newsess->session, "send_rtp_sink");
  if (newsess->send_rtp_sink == NULL)
    goto pad_failed;

  ret = gst_pad_link (pad, newsess->send_rtp_sink);
  if (ret != GST_PAD_LINK_OK)
    goto aux_link_failed;

  if (!complete_session_src (rtpbin, newsess))
    goto session_src_failed;

  return TRUE;

  /* ERRORS */
no_name:
  {
    GST_WARNING ("ignoring invalid pad name %s", GST_STR_NULL (name));
    g_free (name);
    return TRUE;
  }
create_error:
  {
    /* create_session already warned */
    return FALSE;
  }
existing_session:
  {
    GST_DEBUG_OBJECT (rtpbin,
        "skipping src_%i setup, since it is already configured.", sessid);
    return TRUE;
  }
pad_failed:
  {
    g_warning ("rtpbin: failed to get session pad for session %u", sessid);
    return FALSE;
  }
aux_link_failed:
  {
    g_warning ("rtpbin: failed to link AUX for session %u", sessid);
    return FALSE;
  }
session_src_failed:
  {
    g_warning ("rtpbin: failed to complete AUX for session %u", sessid);
    return FALSE;
  }
}

static gboolean
setup_aux_sender (GstRtpBin * rtpbin, GstRtpBinSession * session,
    GstElement * aux)
{
  GstIterator *it;
  GValue result = { 0, };
  GstIteratorResult res;

  it = gst_element_iterate_src_pads (aux);
  res = gst_iterator_fold (it, setup_aux_sender_fold, &result, session);
  gst_iterator_free (it);

  return res == GST_ITERATOR_DONE;
}

/* Create a pad for sending RTP for the session in @name. Must be called with
 * RTP_BIN_LOCK.
 */
static GstPad *
create_send_rtp (GstRtpBin * rtpbin, GstPadTemplate * templ, const gchar * name)
{
  gchar *pname;
  guint sessid;
  GstPad *send_rtp_sink;
  GstElement *aux;
  GstElement *encoder;
  GstElement *prev = NULL;
  GstRtpBinSession *session;

  /* first get the session number */
  if (name == NULL || sscanf (name, "send_rtp_sink_%u", &sessid) != 1)
    goto no_name;

  /* get or create session */
  session = find_session_by_id (rtpbin, sessid);
  if (!session) {
    /* create session now */
    session = create_session (rtpbin, sessid);
    if (session == NULL)
      goto create_error;
  }

  /* check if pad was requested */
  if (session->send_rtp_sink_ghost != NULL)
    return session->send_rtp_sink_ghost;

  /* check if we are already using this session as a sender */
  if (session->send_rtp_sink != NULL)
    goto existing_session;

  encoder = session_request_element (session, SIGNAL_REQUEST_FEC_ENCODER);

  if (encoder) {
    GST_DEBUG_OBJECT (rtpbin, "Linking FEC encoder");

    send_rtp_sink = gst_element_get_static_pad (encoder, "sink");

    if (!send_rtp_sink)
      goto enc_sink_failed;

    prev = encoder;
  }

  GST_DEBUG_OBJECT (rtpbin, "getting RTP AUX sender");
  aux = session_request_element (session, SIGNAL_REQUEST_AUX_SENDER);
  if (aux) {
    GstPad *sinkpad;
    GST_DEBUG_OBJECT (rtpbin, "linking AUX sender");
    if (!setup_aux_sender (rtpbin, session, aux))
      goto aux_session_failed;

    pname = g_strdup_printf ("sink_%u", sessid);
    sinkpad = gst_element_get_static_pad (aux, pname);
    g_free (pname);

    if (sinkpad == NULL)
      goto aux_sink_failed;

    if (!prev) {
      send_rtp_sink = sinkpad;
    } else {
      GstPad *srcpad = gst_element_get_static_pad (prev, "src");
      GstPadLinkReturn ret;

      ret = gst_pad_link (srcpad, sinkpad);
      gst_object_unref (srcpad);
      if (ret != GST_PAD_LINK_OK) {
        goto aux_link_failed;
      }
    }
    prev = aux;
  } else {
    /* get send_rtp pad and store */
    session->send_rtp_sink =
        gst_element_get_request_pad (session->session, "send_rtp_sink");
    if (session->send_rtp_sink == NULL)
      goto pad_failed;

    if (!complete_session_src (rtpbin, session))
      goto session_src_failed;

    if (!prev) {
      send_rtp_sink = gst_object_ref (session->send_rtp_sink);
    } else {
      GstPad *srcpad = gst_element_get_static_pad (prev, "src");
      GstPadLinkReturn ret;

      ret = gst_pad_link (srcpad, session->send_rtp_sink);
      gst_object_unref (srcpad);
      if (ret != GST_PAD_LINK_OK)
        goto session_link_failed;
    }
  }

  session->send_rtp_sink_ghost =
      gst_ghost_pad_new_from_template (name, send_rtp_sink, templ);
  gst_object_unref (send_rtp_sink);
  gst_pad_set_active (session->send_rtp_sink_ghost, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpbin), session->send_rtp_sink_ghost);

  return session->send_rtp_sink_ghost;

  /* ERRORS */
no_name:
  {
    g_warning ("rtpbin: invalid name given");
    return NULL;
  }
create_error:
  {
    /* create_session already warned */
    return NULL;
  }
existing_session:
  {
    g_warning ("rtpbin: session %u is already in use", sessid);
    return NULL;
  }
aux_session_failed:
  {
    g_warning ("rtpbin: failed to get AUX sink pad for session %u", sessid);
    return NULL;
  }
aux_sink_failed:
  {
    g_warning ("rtpbin: failed to get AUX sink pad for session %u", sessid);
    return NULL;
  }
aux_link_failed:
  {
    g_warning ("rtpbin: failed to link %" GST_PTR_FORMAT " for session %u",
        aux, sessid);
    return NULL;
  }
pad_failed:
  {
    g_warning ("rtpbin: failed to get session pad for session %u", sessid);
    return NULL;
  }
session_src_failed:
  {
    g_warning ("rtpbin: failed to setup source pads for session %u", sessid);
    return NULL;
  }
session_link_failed:
  {
    g_warning ("rtpbin: failed to link %" GST_PTR_FORMAT " for session %u",
        session, sessid);
    return NULL;
  }
enc_sink_failed:
  {
    g_warning ("rtpbin: failed to get %" GST_PTR_FORMAT
        " sink pad for session %u", encoder, sessid);
    return NULL;
  }
}

static void
remove_send_rtp (GstRtpBin * rtpbin, GstRtpBinSession * session)
{
  if (session->send_rtp_src_ghost) {
    gst_pad_set_active (session->send_rtp_src_ghost, FALSE);
    gst_element_remove_pad (GST_ELEMENT_CAST (rtpbin),
        session->send_rtp_src_ghost);
    session->send_rtp_src_ghost = NULL;
  }
  if (session->send_rtp_sink) {
    gst_element_release_request_pad (GST_ELEMENT_CAST (session->session),
        session->send_rtp_sink);
    gst_object_unref (session->send_rtp_sink);
    session->send_rtp_sink = NULL;
  }
  if (session->send_rtp_sink_ghost) {
    gst_pad_set_active (session->send_rtp_sink_ghost, FALSE);
    gst_element_remove_pad (GST_ELEMENT_CAST (rtpbin),
        session->send_rtp_sink_ghost);
    session->send_rtp_sink_ghost = NULL;
  }
}

/* Create a pad for sending RTCP for the session in @name. Must be called with
 * RTP_BIN_LOCK.
 */
static GstPad *
create_send_rtcp (GstRtpBin * rtpbin, GstPadTemplate * templ,
    const gchar * name)
{
  guint sessid;
  GstPad *encsrc;
  GstElement *encoder;
  GstRtpBinSession *session;

  /* first get the session number */
  if (name == NULL || sscanf (name, "send_rtcp_src_%u", &sessid) != 1)
    goto no_name;

  /* get or create session */
  session = find_session_by_id (rtpbin, sessid);
  if (!session) {
    GST_DEBUG_OBJECT (rtpbin, "creating session %u", sessid);
    /* create session now */
    session = create_session (rtpbin, sessid);
    if (session == NULL)
      goto create_error;
  }

  /* check if pad was requested */
  if (session->send_rtcp_src_ghost != NULL)
    return session->send_rtcp_src_ghost;

  /* get rtcp_src pad and store */
  session->send_rtcp_src =
      gst_element_get_request_pad (session->session, "send_rtcp_src");
  if (session->send_rtcp_src == NULL)
    goto pad_failed;

  GST_DEBUG_OBJECT (rtpbin, "getting RTCP encoder");
  encoder = session_request_element (session, SIGNAL_REQUEST_RTCP_ENCODER);
  if (encoder) {
    gchar *ename;
    GstPad *encsink;
    GstPadLinkReturn ret;

    GST_DEBUG_OBJECT (rtpbin, "linking RTCP encoder");

    ename = g_strdup_printf ("rtcp_src_%u", sessid);
    encsrc = gst_element_get_static_pad (encoder, ename);
    g_free (ename);
    if (encsrc == NULL)
      goto enc_src_failed;

    ename = g_strdup_printf ("rtcp_sink_%u", sessid);
    encsink = gst_element_get_static_pad (encoder, ename);
    g_free (ename);
    if (encsink == NULL)
      goto enc_sink_failed;

    ret = gst_pad_link (session->send_rtcp_src, encsink);
    gst_object_unref (encsink);

    if (ret != GST_PAD_LINK_OK)
      goto enc_link_failed;
  } else {
    GST_DEBUG_OBJECT (rtpbin, "no RTCP encoder given");
    encsrc = gst_object_ref (session->send_rtcp_src);
  }

  session->send_rtcp_src_ghost =
      gst_ghost_pad_new_from_template (name, encsrc, templ);
  gst_object_unref (encsrc);
  gst_pad_set_active (session->send_rtcp_src_ghost, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (rtpbin), session->send_rtcp_src_ghost);

  return session->send_rtcp_src_ghost;

  /* ERRORS */
no_name:
  {
    g_warning ("rtpbin: invalid name given");
    return NULL;
  }
create_error:
  {
    /* create_session already warned */
    return NULL;
  }
pad_failed:
  {
    g_warning ("rtpbin: failed to get rtcp pad for session %u", sessid);
    return NULL;
  }
enc_src_failed:
  {
    g_warning ("rtpbin: failed to get encoder src pad for session %u", sessid);
    return NULL;
  }
enc_sink_failed:
  {
    g_warning ("rtpbin: failed to get encoder sink pad for session %u", sessid);
    gst_object_unref (encsrc);
    return NULL;
  }
enc_link_failed:
  {
    g_warning ("rtpbin: failed to link rtcp encoder for session %u", sessid);
    gst_object_unref (encsrc);
    return NULL;
  }
}

static void
remove_rtcp (GstRtpBin * rtpbin, GstRtpBinSession * session)
{
  if (session->send_rtcp_src_ghost) {
    gst_pad_set_active (session->send_rtcp_src_ghost, FALSE);
    gst_element_remove_pad (GST_ELEMENT_CAST (rtpbin),
        session->send_rtcp_src_ghost);
    session->send_rtcp_src_ghost = NULL;
  }
  if (session->send_rtcp_src) {
    gst_element_release_request_pad (session->session, session->send_rtcp_src);
    gst_object_unref (session->send_rtcp_src);
    session->send_rtcp_src = NULL;
  }
}

/* If the requested name is NULL we should create a name with
 * the session number assuming we want the lowest possible session
 * with a free pad like the template */
static gchar *
gst_rtp_bin_get_free_pad_name (GstElement * element, GstPadTemplate * templ)
{
  gboolean name_found = FALSE;
  gint session = 0;
  GstIterator *pad_it = NULL;
  gchar *pad_name = NULL;
  GValue data = { 0, };

  GST_DEBUG_OBJECT (element, "find a free pad name for template");
  while (!name_found) {
    gboolean done = FALSE;

    g_free (pad_name);
    pad_name = g_strdup_printf (templ->name_template, session++);
    pad_it = gst_element_iterate_pads (GST_ELEMENT (element));
    name_found = TRUE;
    while (!done) {
      switch (gst_iterator_next (pad_it, &data)) {
        case GST_ITERATOR_OK:
        {
          GstPad *pad;
          gchar *name;

          pad = g_value_get_object (&data);
          name = gst_pad_get_name (pad);

          if (strcmp (name, pad_name) == 0) {
            done = TRUE;
            name_found = FALSE;
          }
          g_free (name);
          g_value_reset (&data);
          break;
        }
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_RESYNC:
          /* restart iteration */
          done = TRUE;
          name_found = FALSE;
          session = 0;
          break;
        case GST_ITERATOR_DONE:
          done = TRUE;
          break;
      }
    }
    g_value_unset (&data);
    gst_iterator_free (pad_it);
  }

  GST_DEBUG_OBJECT (element, "free pad name found: '%s'", pad_name);
  return pad_name;
}

/*
 */
static GstPad *
gst_rtp_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstRtpBin *rtpbin;
  GstElementClass *klass;
  GstPad *result;

  gchar *pad_name = NULL;

  g_return_val_if_fail (templ != NULL, NULL);
  g_return_val_if_fail (GST_IS_RTP_BIN (element), NULL);

  rtpbin = GST_RTP_BIN (element);
  klass = GST_ELEMENT_GET_CLASS (element);

  GST_RTP_BIN_LOCK (rtpbin);

  if (name == NULL) {
    /* use a free pad name */
    pad_name = gst_rtp_bin_get_free_pad_name (element, templ);
  } else {
    /* use the provided name */
    pad_name = g_strdup (name);
  }

  GST_DEBUG_OBJECT (rtpbin, "Trying to request a pad with name %s", pad_name);

  /* figure out the template */
  if (templ == gst_element_class_get_pad_template (klass, "recv_rtp_sink_%u")) {
    result = create_recv_rtp (rtpbin, templ, pad_name);
  } else if (templ == gst_element_class_get_pad_template (klass,
          "recv_rtcp_sink_%u")) {
    result = create_recv_rtcp (rtpbin, templ, pad_name);
  } else if (templ == gst_element_class_get_pad_template (klass,
          "send_rtp_sink_%u")) {
    result = create_send_rtp (rtpbin, templ, pad_name);
  } else if (templ == gst_element_class_get_pad_template (klass,
          "send_rtcp_src_%u")) {
    result = create_send_rtcp (rtpbin, templ, pad_name);
  } else
    goto wrong_template;

  g_free (pad_name);
  GST_RTP_BIN_UNLOCK (rtpbin);

  return result;

  /* ERRORS */
wrong_template:
  {
    g_free (pad_name);
    GST_RTP_BIN_UNLOCK (rtpbin);
    g_warning ("rtpbin: this is not our template");
    return NULL;
  }
}

static void
gst_rtp_bin_release_pad (GstElement * element, GstPad * pad)
{
  GstRtpBinSession *session;
  GstRtpBin *rtpbin;

  g_return_if_fail (GST_IS_GHOST_PAD (pad));
  g_return_if_fail (GST_IS_RTP_BIN (element));

  rtpbin = GST_RTP_BIN (element);

  GST_RTP_BIN_LOCK (rtpbin);
  GST_DEBUG_OBJECT (rtpbin, "Trying to release pad %s:%s",
      GST_DEBUG_PAD_NAME (pad));

  if (!(session = find_session_by_pad (rtpbin, pad)))
    goto unknown_pad;

  if (session->recv_rtp_sink_ghost == pad) {
    remove_recv_rtp (rtpbin, session);
  } else if (session->recv_rtcp_sink_ghost == pad) {
    remove_recv_rtcp (rtpbin, session);
  } else if (session->send_rtp_sink_ghost == pad) {
    remove_send_rtp (rtpbin, session);
  } else if (session->send_rtcp_src_ghost == pad) {
    remove_rtcp (rtpbin, session);
  }

  /* no more request pads, free the complete session */
  if (session->recv_rtp_sink_ghost == NULL
      && session->recv_rtcp_sink_ghost == NULL
      && session->send_rtp_sink_ghost == NULL
      && session->send_rtcp_src_ghost == NULL) {
    GST_DEBUG_OBJECT (rtpbin, "no more pads for session %p", session);
    rtpbin->sessions = g_slist_remove (rtpbin->sessions, session);
    free_session (session, rtpbin);
  }
  GST_RTP_BIN_UNLOCK (rtpbin);

  return;

  /* ERROR */
unknown_pad:
  {
    GST_RTP_BIN_UNLOCK (rtpbin);
    g_warning ("rtpbin: %s:%s is not one of our request pads",
        GST_DEBUG_PAD_NAME (pad));
    return;
  }
}
