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

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <string.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/rtp/gstrtcpbuffer.h>

#include <gst/glib-compat-private.h>

#include "rtpsession.h"

GST_DEBUG_CATEGORY_STATIC (rtp_session_debug);
#define GST_CAT_DEFAULT rtp_session_debug

/* signals and args */
enum
{
  SIGNAL_GET_SOURCE_BY_SSRC,
  SIGNAL_ON_NEW_SSRC,
  SIGNAL_ON_SSRC_COLLISION,
  SIGNAL_ON_SSRC_VALIDATED,
  SIGNAL_ON_SSRC_ACTIVE,
  SIGNAL_ON_SSRC_SDES,
  SIGNAL_ON_BYE_SSRC,
  SIGNAL_ON_BYE_TIMEOUT,
  SIGNAL_ON_TIMEOUT,
  SIGNAL_ON_SENDER_TIMEOUT,
  SIGNAL_ON_SENDING_RTCP,
  SIGNAL_ON_APP_RTCP,
  SIGNAL_ON_FEEDBACK_RTCP,
  SIGNAL_SEND_RTCP,
  SIGNAL_SEND_RTCP_FULL,
  SIGNAL_ON_RECEIVING_RTCP,
  SIGNAL_ON_NEW_SENDER_SSRC,
  SIGNAL_ON_SENDER_SSRC_ACTIVE,
  SIGNAL_ON_SENDING_NACKS,
  LAST_SIGNAL
};

#define DEFAULT_INTERNAL_SOURCE      NULL
#define DEFAULT_BANDWIDTH            0.0
#define DEFAULT_RTCP_FRACTION        RTP_STATS_RTCP_FRACTION
#define DEFAULT_RTCP_RR_BANDWIDTH    -1
#define DEFAULT_RTCP_RS_BANDWIDTH    -1
#define DEFAULT_RTCP_MTU             1400
#define DEFAULT_SDES                 NULL
#define DEFAULT_NUM_SOURCES          0
#define DEFAULT_NUM_ACTIVE_SOURCES   0
#define DEFAULT_SOURCES              NULL
#define DEFAULT_RTCP_MIN_INTERVAL    (RTP_STATS_MIN_INTERVAL * GST_SECOND)
#define DEFAULT_RTCP_FEEDBACK_RETENTION_WINDOW (2 * GST_SECOND)
#define DEFAULT_RTCP_IMMEDIATE_FEEDBACK_THRESHOLD (3)
#define DEFAULT_PROBATION            RTP_DEFAULT_PROBATION
#define DEFAULT_MAX_DROPOUT_TIME     60000
#define DEFAULT_MAX_MISORDER_TIME    2000
#define DEFAULT_RTP_PROFILE          GST_RTP_PROFILE_AVP
#define DEFAULT_RTCP_REDUCED_SIZE    FALSE
#define DEFAULT_RTCP_DISABLE_SR_TIMESTAMP FALSE

enum
{
  PROP_0,
  PROP_INTERNAL_SSRC,
  PROP_INTERNAL_SOURCE,
  PROP_BANDWIDTH,
  PROP_RTCP_FRACTION,
  PROP_RTCP_RR_BANDWIDTH,
  PROP_RTCP_RS_BANDWIDTH,
  PROP_RTCP_MTU,
  PROP_SDES,
  PROP_NUM_SOURCES,
  PROP_NUM_ACTIVE_SOURCES,
  PROP_SOURCES,
  PROP_FAVOR_NEW,
  PROP_RTCP_MIN_INTERVAL,
  PROP_RTCP_FEEDBACK_RETENTION_WINDOW,
  PROP_RTCP_IMMEDIATE_FEEDBACK_THRESHOLD,
  PROP_PROBATION,
  PROP_MAX_DROPOUT_TIME,
  PROP_MAX_MISORDER_TIME,
  PROP_STATS,
  PROP_RTP_PROFILE,
  PROP_RTCP_REDUCED_SIZE,
  PROP_RTCP_DISABLE_SR_TIMESTAMP
};

/* update average packet size */
#define INIT_AVG(avg, val) \
   (avg) = (val);
#define UPDATE_AVG(avg, val)            \
  if ((avg) == 0)                       \
   (avg) = (val);                       \
  else                                  \
   (avg) = ((val) + (15 * (avg))) >> 4;


/* GObject vmethods */
static void rtp_session_finalize (GObject * object);
static void rtp_session_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void rtp_session_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean rtp_session_send_rtcp (RTPSession * sess,
    GstClockTime max_delay);
static gboolean rtp_session_send_rtcp_with_deadline (RTPSession * sess,
    GstClockTime deadline);

static guint rtp_session_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (RTPSession, rtp_session, G_TYPE_OBJECT);

static guint32 rtp_session_create_new_ssrc (RTPSession * sess);
static RTPSource *obtain_source (RTPSession * sess, guint32 ssrc,
    gboolean * created, RTPPacketInfo * pinfo, gboolean rtp);
static RTPSource *obtain_internal_source (RTPSession * sess,
    guint32 ssrc, gboolean * created, GstClockTime current_time);
static GstFlowReturn rtp_session_schedule_bye_locked (RTPSession * sess,
    GstClockTime current_time);
static GstClockTime calculate_rtcp_interval (RTPSession * sess,
    gboolean deterministic, gboolean first);

static gboolean
accumulate_trues (GSignalInvocationHint * ihint, GValue * return_accu,
    const GValue * handler_return, gpointer data)
{
  if (g_value_get_boolean (handler_return))
    g_value_set_boolean (return_accu, TRUE);

  return TRUE;
}

static void
rtp_session_class_init (RTPSessionClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;

  gobject_class->finalize = rtp_session_finalize;
  gobject_class->set_property = rtp_session_set_property;
  gobject_class->get_property = rtp_session_get_property;

  /**
   * RTPSession::get-source-by-ssrc:
   * @session: the object which received the signal
   * @ssrc: the SSRC of the RTPSource
   *
   * Request the #RTPSource object with SSRC @ssrc in @session.
   */
  rtp_session_signals[SIGNAL_GET_SOURCE_BY_SSRC] =
      g_signal_new ("get-source-by-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (RTPSessionClass,
          get_source_by_ssrc), NULL, NULL, NULL,
      RTP_TYPE_SOURCE, 1, G_TYPE_UINT);

  /**
   * RTPSession::on-new-ssrc:
   * @session: the object which received the signal
   * @src: the new RTPSource
   *
   * Notify of a new SSRC that entered @session.
   */
  rtp_session_signals[SIGNAL_ON_NEW_SSRC] =
      g_signal_new ("on-new-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_new_ssrc),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);
  /**
   * RTPSession::on-ssrc-collision:
   * @session: the object which received the signal
   * @src: the #RTPSource that caused a collision
   *
   * Notify when we have an SSRC collision
   */
  rtp_session_signals[SIGNAL_ON_SSRC_COLLISION] =
      g_signal_new ("on-ssrc-collision", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_ssrc_collision),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);
  /**
   * RTPSession::on-ssrc-validated:
   * @session: the object which received the signal
   * @src: the new validated RTPSource
   *
   * Notify of a new SSRC that became validated.
   */
  rtp_session_signals[SIGNAL_ON_SSRC_VALIDATED] =
      g_signal_new ("on-ssrc-validated", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_ssrc_validated),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);
  /**
   * RTPSession::on-ssrc-active:
   * @session: the object which received the signal
   * @src: the active RTPSource
   *
   * Notify of a SSRC that is active, i.e., sending RTCP.
   */
  rtp_session_signals[SIGNAL_ON_SSRC_ACTIVE] =
      g_signal_new ("on-ssrc-active", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_ssrc_active),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);
  /**
   * RTPSession::on-ssrc-sdes:
   * @session: the object which received the signal
   * @src: the RTPSource
   *
   * Notify that a new SDES was received for SSRC.
   */
  rtp_session_signals[SIGNAL_ON_SSRC_SDES] =
      g_signal_new ("on-ssrc-sdes", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_ssrc_sdes),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);
  /**
   * RTPSession::on-bye-ssrc:
   * @session: the object which received the signal
   * @src: the RTPSource that went away
   *
   * Notify of an SSRC that became inactive because of a BYE packet.
   */
  rtp_session_signals[SIGNAL_ON_BYE_SSRC] =
      g_signal_new ("on-bye-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_bye_ssrc),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);
  /**
   * RTPSession::on-bye-timeout:
   * @session: the object which received the signal
   * @src: the RTPSource that timed out
   *
   * Notify of an SSRC that has timed out because of BYE
   */
  rtp_session_signals[SIGNAL_ON_BYE_TIMEOUT] =
      g_signal_new ("on-bye-timeout", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_bye_timeout),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);
  /**
   * RTPSession::on-timeout:
   * @session: the object which received the signal
   * @src: the RTPSource that timed out
   *
   * Notify of an SSRC that has timed out
   */
  rtp_session_signals[SIGNAL_ON_TIMEOUT] =
      g_signal_new ("on-timeout", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_timeout),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);
  /**
   * RTPSession::on-sender-timeout:
   * @session: the object which received the signal
   * @src: the RTPSource that timed out
   *
   * Notify of an SSRC that was a sender but timed out and became a receiver.
   */
  rtp_session_signals[SIGNAL_ON_SENDER_TIMEOUT] =
      g_signal_new ("on-sender-timeout", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_sender_timeout),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);

  /**
   * RTPSession::on-sending-rtcp
   * @session: the object which received the signal
   * @buffer: the #GstBuffer containing the RTCP packet about to be sent
   * @early: %TRUE if the packet is early, %FALSE if it is regular
   *
   * This signal is emitted before sending an RTCP packet, it can be used
   * to add extra RTCP Packets.
   *
   * Returns: %TRUE if the RTCP buffer should NOT be suppressed, %FALSE
   * if suppressing it is acceptable
   */
  rtp_session_signals[SIGNAL_ON_SENDING_RTCP] =
      g_signal_new ("on-sending-rtcp", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_sending_rtcp),
      accumulate_trues, NULL, NULL, G_TYPE_BOOLEAN, 2,
      GST_TYPE_BUFFER | G_SIGNAL_TYPE_STATIC_SCOPE, G_TYPE_BOOLEAN);

  /**
   * RTPSession::on-app-rtcp:
   * @session: the object which received the signal
   * @subtype: The subtype of the packet
   * @ssrc: The SSRC/CSRC of the packet
   * @name: The name of the packet
   * @data: a #GstBuffer with the application-dependant data or %NULL if
   * there was no data
   *
   * Notify that a RTCP APP packet has been received
   */
  rtp_session_signals[SIGNAL_ON_APP_RTCP] =
      g_signal_new ("on-app-rtcp", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_app_rtcp),
      NULL, NULL, NULL, G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT,
      G_TYPE_STRING, GST_TYPE_BUFFER);

  /**
   * RTPSession::on-feedback-rtcp:
   * @session: the object which received the signal
   * @type: Type of RTCP packet, will be %GST_RTCP_TYPE_RTPFB or
   *  %GST_RTCP_TYPE_RTPFB
   * @fbtype: The type of RTCP FB packet, probably part of #GstRTCPFBType
   * @sender_ssrc: The SSRC of the sender
   * @media_ssrc: The SSRC of the media this refers to
   * @fci: a #GstBuffer with the FCI data from the FB packet or %NULL if
   * there was no FCI
   *
   * Notify that a RTCP feedback packet has been received
   */
  rtp_session_signals[SIGNAL_ON_FEEDBACK_RTCP] =
      g_signal_new ("on-feedback-rtcp", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_feedback_rtcp),
      NULL, NULL, NULL, G_TYPE_NONE, 5, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
      G_TYPE_UINT, GST_TYPE_BUFFER);

  /**
   * RTPSession::send-rtcp:
   * @session: the object which received the signal
   * @max_delay: The maximum delay after which the feedback will not be useful
   *  anymore
   *
   * Requests that the #RTPSession initiate a new RTCP packet as soon as
   * possible within the requested delay.
   *
   * This sets feedback to %TRUE if not already done before.
   */
  rtp_session_signals[SIGNAL_SEND_RTCP] =
      g_signal_new ("send-rtcp", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (RTPSessionClass, send_rtcp), NULL, NULL,
      NULL, G_TYPE_NONE, 1, G_TYPE_UINT64);

  /**
   * RTPSession::send-rtcp-full:
   * @session: the object which received the signal
   * @max_delay: The maximum delay after which the feedback will not be useful
   *  anymore
   *
   * Requests that the #RTPSession initiate a new RTCP packet as soon as
   * possible within the requested delay.
   *
   * This sets feedback to %TRUE if not already done before.
   *
   * Returns: TRUE if the new RTCP packet could be scheduled within the
   * requested delay, FALSE otherwise.
   *
   * Since: 1.6
   */
  rtp_session_signals[SIGNAL_SEND_RTCP_FULL] =
      g_signal_new ("send-rtcp-full", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (RTPSessionClass, send_rtcp), NULL, NULL,
      NULL, G_TYPE_BOOLEAN, 1, G_TYPE_UINT64);

  /**
   * RTPSession::on-receiving-rtcp
   * @session: the object which received the signal
   * @buffer: the #GstBuffer containing the RTCP packet that was received
   *
   * This signal is emitted when receiving an RTCP packet before it is handled
   * by the session. It can be used to extract custom information from RTCP packets.
   *
   * Since: 1.6
   */
  rtp_session_signals[SIGNAL_ON_RECEIVING_RTCP] =
      g_signal_new ("on-receiving-rtcp", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_receiving_rtcp),
      NULL, NULL, NULL, G_TYPE_NONE, 1,
      GST_TYPE_BUFFER | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * RTPSession::on-new-sender-ssrc:
   * @session: the object which received the signal
   * @src: the new sender RTPSource
   *
   * Notify of a new sender SSRC that entered @session.
   *
   * Since: 1.8
   */
  rtp_session_signals[SIGNAL_ON_NEW_SENDER_SSRC] =
      g_signal_new ("on-new-sender-ssrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_new_sender_ssrc),
      NULL, NULL, NULL, G_TYPE_NONE, 1, RTP_TYPE_SOURCE);

  /**
   * RTPSession::on-sender-ssrc-active:
   * @session: the object which received the signal
   * @src: the active sender RTPSource
   *
   * Notify of a sender SSRC that is active, i.e., sending RTCP.
   *
   * Since: 1.8
   */
  rtp_session_signals[SIGNAL_ON_SENDER_SSRC_ACTIVE] =
      g_signal_new ("on-sender-ssrc-active", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass,
          on_sender_ssrc_active), NULL, NULL, NULL,
      G_TYPE_NONE, 1, RTP_TYPE_SOURCE);

  /**
   * RTPSession::on-sending-nack
   * @session: the object which received the signal
   * @sender_ssrc: the sender ssrc
   * @media_ssrc: the media ssrc
   * @nacks: (element-type guint16): the list of seqnum to be nacked
   * @buffer: the #GstBuffer containing the RTCP packet about to be sent
   *
   * This signal is emitted before NACK packets are added into the RTCP
   * packet. This signal can be used to override the conversion of the NACK
   * seqnum array into packets. This can be used if your protocol uses
   * different type of NACK (e.g. based on RTCP APP).
   *
   * The handler should transform the seqnum from @nacks array into packets.
   * @nacks seqnum must be consumed from the start. The remaining will be
   * rescheduled for later base on bandwidth. Only one handler will be
   * signalled.
   *
   * A handler may return 0 to signal that generic NACKs should be created
   * for this set. This can be useful if the signal is used for other purpose
   * or if the other type of NACK would use more space.
   *
   * Returns: the number of NACK seqnum that was consumed from @nacks.
   *
   * Since: 1.16
   */
  rtp_session_signals[SIGNAL_ON_SENDING_NACKS] =
      g_signal_new ("on-sending-nacks", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (RTPSessionClass, on_sending_nacks),
      g_signal_accumulator_first_wins, NULL, NULL,
      G_TYPE_UINT, 4, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_ARRAY,
      GST_TYPE_BUFFER | G_SIGNAL_TYPE_STATIC_SCOPE);

  g_object_class_install_property (gobject_class, PROP_INTERNAL_SSRC,
      g_param_spec_uint ("internal-ssrc", "Internal SSRC",
          "The internal SSRC used for the session (deprecated)",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INTERNAL_SOURCE,
      g_param_spec_object ("internal-source", "Internal Source",
          "The internal source element of the session (deprecated)",
          RTP_TYPE_SOURCE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BANDWIDTH,
      g_param_spec_double ("bandwidth", "Bandwidth",
          "The bandwidth of the session in bits per second (0 for auto-discover)",
          0.0, G_MAXDOUBLE, DEFAULT_BANDWIDTH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_FRACTION,
      g_param_spec_double ("rtcp-fraction", "RTCP Fraction",
          "The fraction of the bandwidth used for RTCP in bits per second (or as a real fraction of the RTP bandwidth if < 1)",
          0.0, G_MAXDOUBLE, DEFAULT_RTCP_FRACTION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_RR_BANDWIDTH,
      g_param_spec_int ("rtcp-rr-bandwidth", "RTCP RR bandwidth",
          "The RTCP bandwidth used for receivers in bits per second (-1 = default)",
          -1, G_MAXINT, DEFAULT_RTCP_RR_BANDWIDTH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_RS_BANDWIDTH,
      g_param_spec_int ("rtcp-rs-bandwidth", "RTCP RS bandwidth",
          "The RTCP bandwidth used for senders in bits per second (-1 = default)",
          -1, G_MAXINT, DEFAULT_RTCP_RS_BANDWIDTH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_MTU,
      g_param_spec_uint ("rtcp-mtu", "RTCP MTU",
          "The maximum size of the RTCP packets",
          16, G_MAXINT16, DEFAULT_RTCP_MTU,
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
  /**
   * RTPSource::sources
   *
   * Get a GValue Array of all sources in the session.
   *
   * ## Getting the #RTPSources of a session
   * |[
   * {
   *   GValueArray *arr;
   *   GValue *val;
   *   guint i;
   *
   *   g_object_get (sess, "sources", &arr, NULL);
   *
   *   for (i = 0; i < arr->n_values; i++) {
   *     RTPSource *source;
   *
   *     val = g_value_array_get_nth (arr, i);
   *     source = g_value_get_object (val);
   *   }
   *   g_value_array_free (arr);
   * }
   * ]|
   */
  g_object_class_install_property (gobject_class, PROP_SOURCES,
      g_param_spec_boxed ("sources", "Sources",
          "An array of all known sources in the session",
          G_TYPE_VALUE_ARRAY, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FAVOR_NEW,
      g_param_spec_boolean ("favor-new", "Favor new sources",
          "Resolve SSRC conflict in favor of new sources", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_MIN_INTERVAL,
      g_param_spec_uint64 ("rtcp-min-interval", "Minimum RTCP interval",
          "Minimum interval between Regular RTCP packet (in ns)",
          0, G_MAXUINT64, DEFAULT_RTCP_MIN_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_RTCP_FEEDBACK_RETENTION_WINDOW,
      g_param_spec_uint64 ("rtcp-feedback-retention-window",
          "RTCP Feedback retention window",
          "Duration during which RTCP Feedback packets are retained (in ns)",
          0, G_MAXUINT64, DEFAULT_RTCP_FEEDBACK_RETENTION_WINDOW,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_RTCP_IMMEDIATE_FEEDBACK_THRESHOLD,
      g_param_spec_uint ("rtcp-immediate-feedback-threshold",
          "RTCP Immediate Feedback threshold",
          "The maximum number of members of a RTP session for which immediate"
          " feedback is used (DEPRECATED: has no effect and is not needed)",
          0, G_MAXUINT, DEFAULT_RTCP_IMMEDIATE_FEEDBACK_THRESHOLD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));

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
   * RTPSession::stats:
   *
   * Various session statistics. This property returns a GstStructure
   * with name application/x-rtp-session-stats with the following fields:
   *
   *  "rtx-drop-count"  G_TYPE_UINT   The number of retransmission events
   *      dropped (due to bandwidth constraints)
   *  "sent-nack-count" G_TYPE_UINT   Number of NACKs sent
   *  "recv-nack-count" G_TYPE_UINT   Number of NACKs received
   *  "source-stats"    G_TYPE_BOXED  GValueArray of #RTPSource::stats for all
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
          "RTP profile to use for this session", GST_TYPE_RTP_PROFILE,
          DEFAULT_RTP_PROFILE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RTCP_REDUCED_SIZE,
      g_param_spec_boolean ("rtcp-reduced-size", "RTCP Reduced Size",
          "Use Reduced Size RTCP for feedback packets",
          DEFAULT_RTCP_REDUCED_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * RTPSession::disable-sr-timestamp:
   *
   * Whether sender reports should be timestamped.
   *
   * Since: 1.16
   */
  g_object_class_install_property (gobject_class,
      PROP_RTCP_DISABLE_SR_TIMESTAMP,
      g_param_spec_boolean ("disable-sr-timestamp",
          "Disable Sender Report Timestamp",
          "Whether sender reports should be timestamped",
          DEFAULT_RTCP_DISABLE_SR_TIMESTAMP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  klass->get_source_by_ssrc =
      GST_DEBUG_FUNCPTR (rtp_session_get_source_by_ssrc);
  klass->send_rtcp = GST_DEBUG_FUNCPTR (rtp_session_send_rtcp);

  GST_DEBUG_CATEGORY_INIT (rtp_session_debug, "rtpsession", 0, "RTP Session");
}

static void
rtp_session_init (RTPSession * sess)
{
  gint i;
  gchar *str;

  g_mutex_init (&sess->lock);
  sess->key = g_random_int ();
  sess->mask_idx = 0;
  sess->mask = 0;

  /* TODO: We currently only use the first hash table but this is the
   * beginning of an implementation for RFC2762
   for (i = 0; i < 32; i++) {
   */
  for (i = 0; i < 1; i++) {
    sess->ssrcs[i] =
        g_hash_table_new_full (NULL, NULL, NULL,
        (GDestroyNotify) g_object_unref);
  }

  rtp_stats_init_defaults (&sess->stats);
  INIT_AVG (sess->stats.avg_rtcp_packet_size, 100);
  rtp_stats_set_min_interval (&sess->stats,
      (gdouble) DEFAULT_RTCP_MIN_INTERVAL / GST_SECOND);

  sess->recalc_bandwidth = TRUE;
  sess->bandwidth = DEFAULT_BANDWIDTH;
  sess->rtcp_bandwidth = DEFAULT_RTCP_FRACTION;
  sess->rtcp_rr_bandwidth = DEFAULT_RTCP_RR_BANDWIDTH;
  sess->rtcp_rs_bandwidth = DEFAULT_RTCP_RS_BANDWIDTH;

  /* default UDP header length */
  sess->header_len = UDP_IP_HEADER_OVERHEAD;
  sess->mtu = DEFAULT_RTCP_MTU;

  sess->probation = DEFAULT_PROBATION;
  sess->max_dropout_time = DEFAULT_MAX_DROPOUT_TIME;
  sess->max_misorder_time = DEFAULT_MAX_MISORDER_TIME;

  /* some default SDES entries */
  sess->sdes = gst_structure_new_empty ("application/x-rtp-source-sdes");

  /* we do not want to leak details like the username or hostname here */
  str = g_strdup_printf ("user%u@host-%x", g_random_int (), g_random_int ());
  gst_structure_set (sess->sdes, "cname", G_TYPE_STRING, str, NULL);
  g_free (str);

#if 0
  /* we do not want to leak the user's real name here */
  str = g_strdup_printf ("Anon%u", g_random_int ());
  gst_structure_set (sdes, "name", G_TYPE_STRING, str, NULL);
  g_free (str);
#endif

  gst_structure_set (sess->sdes, "tool", G_TYPE_STRING, "GStreamer", NULL);

  /* this is the SSRC we suggest */
  sess->suggested_ssrc = rtp_session_create_new_ssrc (sess);
  sess->internal_ssrc_set = FALSE;

  sess->first_rtcp = TRUE;
  sess->next_rtcp_check_time = GST_CLOCK_TIME_NONE;
  sess->last_rtcp_check_time = GST_CLOCK_TIME_NONE;
  sess->last_rtcp_send_time = GST_CLOCK_TIME_NONE;
  sess->last_rtcp_interval = GST_CLOCK_TIME_NONE;

  sess->next_early_rtcp_time = GST_CLOCK_TIME_NONE;
  sess->rtcp_feedback_retention_window = DEFAULT_RTCP_FEEDBACK_RETENTION_WINDOW;
  sess->rtcp_immediate_feedback_threshold =
      DEFAULT_RTCP_IMMEDIATE_FEEDBACK_THRESHOLD;
  sess->rtp_profile = DEFAULT_RTP_PROFILE;
  sess->reduced_size_rtcp = DEFAULT_RTCP_REDUCED_SIZE;
  sess->timestamp_sender_reports = !DEFAULT_RTCP_DISABLE_SR_TIMESTAMP;

  sess->is_doing_ptp = TRUE;
}

static void
rtp_session_finalize (GObject * object)
{
  RTPSession *sess;
  gint i;

  sess = RTP_SESSION_CAST (object);

  gst_structure_free (sess->sdes);

  g_list_free_full (sess->conflicting_addresses,
      (GDestroyNotify) rtp_conflicting_address_free);

  /* TODO: Change this again when implementing RFC 2762
   * for (i = 0; i < 32; i++)
   */
  for (i = 0; i < 1; i++)
    g_hash_table_destroy (sess->ssrcs[i]);

  g_mutex_clear (&sess->lock);

  G_OBJECT_CLASS (rtp_session_parent_class)->finalize (object);
}

static void
copy_source (gpointer key, RTPSource * source, GValueArray * arr)
{
  GValue value = { 0 };

  g_value_init (&value, RTP_TYPE_SOURCE);
  g_value_take_object (&value, source);
  /* copies the value */
  g_value_array_append (arr, &value);
}

static GValueArray *
rtp_session_create_sources (RTPSession * sess)
{
  GValueArray *res;
  guint size;

  RTP_SESSION_LOCK (sess);
  /* get number of elements in the table */
  size = g_hash_table_size (sess->ssrcs[sess->mask_idx]);
  /* create the result value array */
  res = g_value_array_new (size);

  /* and copy all values into the array */
  g_hash_table_foreach (sess->ssrcs[sess->mask_idx], (GHFunc) copy_source, res);
  RTP_SESSION_UNLOCK (sess);

  return res;
}

static void
create_source_stats (gpointer key, RTPSource * source, GValueArray * arr)
{
  GValue *value;
  GstStructure *s;

  g_object_get (source, "stats", &s, NULL);

  g_value_array_append (arr, NULL);
  value = g_value_array_get_nth (arr, arr->n_values - 1);
  g_value_init (value, GST_TYPE_STRUCTURE);
  g_value_take_boxed (value, s);
}

static GstStructure *
rtp_session_create_stats (RTPSession * sess)
{
  GstStructure *s;
  GValueArray *source_stats;
  GValue source_stats_v = G_VALUE_INIT;
  guint size;

  RTP_SESSION_LOCK (sess);
  s = gst_structure_new ("application/x-rtp-session-stats",
      "rtx-drop-count", G_TYPE_UINT, sess->stats.nacks_dropped,
      "sent-nack-count", G_TYPE_UINT, sess->stats.nacks_sent,
      "recv-nack-count", G_TYPE_UINT, sess->stats.nacks_received, NULL);

  size = g_hash_table_size (sess->ssrcs[sess->mask_idx]);
  source_stats = g_value_array_new (size);
  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) create_source_stats, source_stats);
  RTP_SESSION_UNLOCK (sess);

  g_value_init (&source_stats_v, G_TYPE_VALUE_ARRAY);
  g_value_take_boxed (&source_stats_v, source_stats);
  gst_structure_take_value (s, "source-stats", &source_stats_v);

  return s;
}

static void
rtp_session_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  RTPSession *sess;

  sess = RTP_SESSION (object);

  switch (prop_id) {
    case PROP_INTERNAL_SSRC:
      RTP_SESSION_LOCK (sess);
      sess->suggested_ssrc = g_value_get_uint (value);
      sess->internal_ssrc_set = TRUE;
      sess->internal_ssrc_from_caps_or_property = TRUE;
      RTP_SESSION_UNLOCK (sess);
      if (sess->callbacks.reconfigure)
        sess->callbacks.reconfigure (sess, sess->reconfigure_user_data);
      break;
    case PROP_BANDWIDTH:
      RTP_SESSION_LOCK (sess);
      sess->bandwidth = g_value_get_double (value);
      sess->recalc_bandwidth = TRUE;
      RTP_SESSION_UNLOCK (sess);
      break;
    case PROP_RTCP_FRACTION:
      RTP_SESSION_LOCK (sess);
      sess->rtcp_bandwidth = g_value_get_double (value);
      sess->recalc_bandwidth = TRUE;
      RTP_SESSION_UNLOCK (sess);
      break;
    case PROP_RTCP_RR_BANDWIDTH:
      RTP_SESSION_LOCK (sess);
      sess->rtcp_rr_bandwidth = g_value_get_int (value);
      sess->recalc_bandwidth = TRUE;
      RTP_SESSION_UNLOCK (sess);
      break;
    case PROP_RTCP_RS_BANDWIDTH:
      RTP_SESSION_LOCK (sess);
      sess->rtcp_rs_bandwidth = g_value_get_int (value);
      sess->recalc_bandwidth = TRUE;
      RTP_SESSION_UNLOCK (sess);
      break;
    case PROP_RTCP_MTU:
      sess->mtu = g_value_get_uint (value);
      break;
    case PROP_SDES:
      rtp_session_set_sdes_struct (sess, g_value_get_boxed (value));
      break;
    case PROP_FAVOR_NEW:
      sess->favor_new = g_value_get_boolean (value);
      break;
    case PROP_RTCP_MIN_INTERVAL:
      rtp_stats_set_min_interval (&sess->stats,
          (gdouble) g_value_get_uint64 (value) / GST_SECOND);
      /* trigger reconsideration */
      RTP_SESSION_LOCK (sess);
      sess->next_rtcp_check_time = 0;
      RTP_SESSION_UNLOCK (sess);
      if (sess->callbacks.reconsider)
        sess->callbacks.reconsider (sess, sess->reconsider_user_data);
      break;
    case PROP_RTCP_FEEDBACK_RETENTION_WINDOW:
      sess->rtcp_feedback_retention_window = g_value_get_uint64 (value);
      break;
    case PROP_RTCP_IMMEDIATE_FEEDBACK_THRESHOLD:
      sess->rtcp_immediate_feedback_threshold = g_value_get_uint (value);
      break;
    case PROP_PROBATION:
      sess->probation = g_value_get_uint (value);
      break;
    case PROP_MAX_DROPOUT_TIME:
      sess->max_dropout_time = g_value_get_uint (value);
      break;
    case PROP_MAX_MISORDER_TIME:
      sess->max_misorder_time = g_value_get_uint (value);
      break;
    case PROP_RTP_PROFILE:
      sess->rtp_profile = g_value_get_enum (value);
      /* trigger reconsideration */
      RTP_SESSION_LOCK (sess);
      sess->next_rtcp_check_time = 0;
      RTP_SESSION_UNLOCK (sess);
      if (sess->callbacks.reconsider)
        sess->callbacks.reconsider (sess, sess->reconsider_user_data);
      break;
    case PROP_RTCP_REDUCED_SIZE:
      sess->reduced_size_rtcp = g_value_get_boolean (value);
      break;
    case PROP_RTCP_DISABLE_SR_TIMESTAMP:
      sess->timestamp_sender_reports = !g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
rtp_session_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  RTPSession *sess;

  sess = RTP_SESSION (object);

  switch (prop_id) {
    case PROP_INTERNAL_SSRC:
      g_value_set_uint (value, rtp_session_suggest_ssrc (sess, NULL));
      break;
    case PROP_INTERNAL_SOURCE:
      /* FIXME, return a random source */
      g_value_set_object (value, NULL);
      break;
    case PROP_BANDWIDTH:
      g_value_set_double (value, sess->bandwidth);
      break;
    case PROP_RTCP_FRACTION:
      g_value_set_double (value, sess->rtcp_bandwidth);
      break;
    case PROP_RTCP_RR_BANDWIDTH:
      g_value_set_int (value, sess->rtcp_rr_bandwidth);
      break;
    case PROP_RTCP_RS_BANDWIDTH:
      g_value_set_int (value, sess->rtcp_rs_bandwidth);
      break;
    case PROP_RTCP_MTU:
      g_value_set_uint (value, sess->mtu);
      break;
    case PROP_SDES:
      g_value_take_boxed (value, rtp_session_get_sdes_struct (sess));
      break;
    case PROP_NUM_SOURCES:
      g_value_set_uint (value, rtp_session_get_num_sources (sess));
      break;
    case PROP_NUM_ACTIVE_SOURCES:
      g_value_set_uint (value, rtp_session_get_num_active_sources (sess));
      break;
    case PROP_SOURCES:
      g_value_take_boxed (value, rtp_session_create_sources (sess));
      break;
    case PROP_FAVOR_NEW:
      g_value_set_boolean (value, sess->favor_new);
      break;
    case PROP_RTCP_MIN_INTERVAL:
      g_value_set_uint64 (value, sess->stats.min_interval * GST_SECOND);
      break;
    case PROP_RTCP_FEEDBACK_RETENTION_WINDOW:
      g_value_set_uint64 (value, sess->rtcp_feedback_retention_window);
      break;
    case PROP_RTCP_IMMEDIATE_FEEDBACK_THRESHOLD:
      g_value_set_uint (value, sess->rtcp_immediate_feedback_threshold);
      break;
    case PROP_PROBATION:
      g_value_set_uint (value, sess->probation);
      break;
    case PROP_MAX_DROPOUT_TIME:
      g_value_set_uint (value, sess->max_dropout_time);
      break;
    case PROP_MAX_MISORDER_TIME:
      g_value_set_uint (value, sess->max_misorder_time);
      break;
    case PROP_STATS:
      g_value_take_boxed (value, rtp_session_create_stats (sess));
      break;
    case PROP_RTP_PROFILE:
      g_value_set_enum (value, sess->rtp_profile);
      break;
    case PROP_RTCP_REDUCED_SIZE:
      g_value_set_boolean (value, sess->reduced_size_rtcp);
      break;
    case PROP_RTCP_DISABLE_SR_TIMESTAMP:
      g_value_set_boolean (value, !sess->timestamp_sender_reports);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
on_new_ssrc (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_NEW_SSRC], 0, source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_ssrc_collision (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_SSRC_COLLISION], 0,
      source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_ssrc_validated (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_SSRC_VALIDATED], 0,
      source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_ssrc_active (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_SSRC_ACTIVE], 0, source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_ssrc_sdes (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  GST_DEBUG ("SDES changed for SSRC %08x", source->ssrc);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_SSRC_SDES], 0, source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_bye_ssrc (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_BYE_SSRC], 0, source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_bye_timeout (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_BYE_TIMEOUT], 0, source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_timeout (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_TIMEOUT], 0, source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_sender_timeout (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_SENDER_TIMEOUT], 0,
      source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_new_sender_ssrc (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_NEW_SENDER_SSRC], 0,
      source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

static void
on_sender_ssrc_active (RTPSession * sess, RTPSource * source)
{
  g_object_ref (source);
  RTP_SESSION_UNLOCK (sess);
  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_SENDER_SSRC_ACTIVE], 0,
      source);
  RTP_SESSION_LOCK (sess);
  g_object_unref (source);
}

/**
 * rtp_session_new:
 *
 * Create a new session object.
 *
 * Returns: a new #RTPSession. g_object_unref() after usage.
 */
RTPSession *
rtp_session_new (void)
{
  RTPSession *sess;

  sess = g_object_new (RTP_TYPE_SESSION, NULL);

  return sess;
}

/**
 * rtp_session_reset:
 * @sess: an #RTPSession
 *
 * Reset the sources of @sess.
 */
void
rtp_session_reset (RTPSession * sess)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  /* remove all sources */
  g_hash_table_remove_all (sess->ssrcs[sess->mask_idx]);
  sess->total_sources = 0;
  sess->stats.sender_sources = 0;
  sess->stats.internal_sender_sources = 0;
  sess->stats.internal_sources = 0;
  sess->stats.active_sources = 0;

  sess->generation = 0;
  sess->first_rtcp = TRUE;
  sess->next_rtcp_check_time = GST_CLOCK_TIME_NONE;
  sess->last_rtcp_check_time = GST_CLOCK_TIME_NONE;
  sess->last_rtcp_send_time = GST_CLOCK_TIME_NONE;
  sess->last_rtcp_interval = GST_CLOCK_TIME_NONE;
  sess->next_early_rtcp_time = GST_CLOCK_TIME_NONE;
  sess->scheduled_bye = FALSE;

  /* reset session stats */
  sess->stats.bye_members = 0;
  sess->stats.nacks_dropped = 0;
  sess->stats.nacks_sent = 0;
  sess->stats.nacks_received = 0;

  sess->is_doing_ptp = TRUE;

  g_list_free_full (sess->conflicting_addresses,
      (GDestroyNotify) rtp_conflicting_address_free);
  sess->conflicting_addresses = NULL;
}

/**
 * rtp_session_set_callbacks:
 * @sess: an #RTPSession
 * @callbacks: callbacks to configure
 * @user_data: user data passed in the callbacks
 *
 * Configure a set of callbacks to be notified of actions.
 */
void
rtp_session_set_callbacks (RTPSession * sess, RTPSessionCallbacks * callbacks,
    gpointer user_data)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  if (callbacks->process_rtp) {
    sess->callbacks.process_rtp = callbacks->process_rtp;
    sess->process_rtp_user_data = user_data;
  }
  if (callbacks->send_rtp) {
    sess->callbacks.send_rtp = callbacks->send_rtp;
    sess->send_rtp_user_data = user_data;
  }
  if (callbacks->send_rtcp) {
    sess->callbacks.send_rtcp = callbacks->send_rtcp;
    sess->send_rtcp_user_data = user_data;
  }
  if (callbacks->sync_rtcp) {
    sess->callbacks.sync_rtcp = callbacks->sync_rtcp;
    sess->sync_rtcp_user_data = user_data;
  }
  if (callbacks->clock_rate) {
    sess->callbacks.clock_rate = callbacks->clock_rate;
    sess->clock_rate_user_data = user_data;
  }
  if (callbacks->reconsider) {
    sess->callbacks.reconsider = callbacks->reconsider;
    sess->reconsider_user_data = user_data;
  }
  if (callbacks->request_key_unit) {
    sess->callbacks.request_key_unit = callbacks->request_key_unit;
    sess->request_key_unit_user_data = user_data;
  }
  if (callbacks->request_time) {
    sess->callbacks.request_time = callbacks->request_time;
    sess->request_time_user_data = user_data;
  }
  if (callbacks->notify_nack) {
    sess->callbacks.notify_nack = callbacks->notify_nack;
    sess->notify_nack_user_data = user_data;
  }
  if (callbacks->reconfigure) {
    sess->callbacks.reconfigure = callbacks->reconfigure;
    sess->reconfigure_user_data = user_data;
  }
  if (callbacks->notify_early_rtcp) {
    sess->callbacks.notify_early_rtcp = callbacks->notify_early_rtcp;
    sess->notify_early_rtcp_user_data = user_data;
  }
}

/**
 * rtp_session_set_process_rtp_callback:
 * @sess: an #RTPSession
 * @callback: callback to set
 * @user_data: user data passed in the callback
 *
 * Configure only the process_rtp callback to be notified of the process_rtp action.
 */
void
rtp_session_set_process_rtp_callback (RTPSession * sess,
    RTPSessionProcessRTP callback, gpointer user_data)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  sess->callbacks.process_rtp = callback;
  sess->process_rtp_user_data = user_data;
}

/**
 * rtp_session_set_send_rtp_callback:
 * @sess: an #RTPSession
 * @callback: callback to set
 * @user_data: user data passed in the callback
 *
 * Configure only the send_rtp callback to be notified of the send_rtp action.
 */
void
rtp_session_set_send_rtp_callback (RTPSession * sess,
    RTPSessionSendRTP callback, gpointer user_data)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  sess->callbacks.send_rtp = callback;
  sess->send_rtp_user_data = user_data;
}

/**
 * rtp_session_set_send_rtcp_callback:
 * @sess: an #RTPSession
 * @callback: callback to set
 * @user_data: user data passed in the callback
 *
 * Configure only the send_rtcp callback to be notified of the send_rtcp action.
 */
void
rtp_session_set_send_rtcp_callback (RTPSession * sess,
    RTPSessionSendRTCP callback, gpointer user_data)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  sess->callbacks.send_rtcp = callback;
  sess->send_rtcp_user_data = user_data;
}

/**
 * rtp_session_set_sync_rtcp_callback:
 * @sess: an #RTPSession
 * @callback: callback to set
 * @user_data: user data passed in the callback
 *
 * Configure only the sync_rtcp callback to be notified of the sync_rtcp action.
 */
void
rtp_session_set_sync_rtcp_callback (RTPSession * sess,
    RTPSessionSyncRTCP callback, gpointer user_data)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  sess->callbacks.sync_rtcp = callback;
  sess->sync_rtcp_user_data = user_data;
}

/**
 * rtp_session_set_clock_rate_callback:
 * @sess: an #RTPSession
 * @callback: callback to set
 * @user_data: user data passed in the callback
 *
 * Configure only the clock_rate callback to be notified of the clock_rate action.
 */
void
rtp_session_set_clock_rate_callback (RTPSession * sess,
    RTPSessionClockRate callback, gpointer user_data)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  sess->callbacks.clock_rate = callback;
  sess->clock_rate_user_data = user_data;
}

/**
 * rtp_session_set_reconsider_callback:
 * @sess: an #RTPSession
 * @callback: callback to set
 * @user_data: user data passed in the callback
 *
 * Configure only the reconsider callback to be notified of the reconsider action.
 */
void
rtp_session_set_reconsider_callback (RTPSession * sess,
    RTPSessionReconsider callback, gpointer user_data)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  sess->callbacks.reconsider = callback;
  sess->reconsider_user_data = user_data;
}

/**
 * rtp_session_set_request_time_callback:
 * @sess: an #RTPSession
 * @callback: callback to set
 * @user_data: user data passed in the callback
 *
 * Configure only the request_time callback
 */
void
rtp_session_set_request_time_callback (RTPSession * sess,
    RTPSessionRequestTime callback, gpointer user_data)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  sess->callbacks.request_time = callback;
  sess->request_time_user_data = user_data;
}

/**
 * rtp_session_set_bandwidth:
 * @sess: an #RTPSession
 * @bandwidth: the bandwidth allocated
 *
 * Set the session bandwidth in bits per second.
 */
void
rtp_session_set_bandwidth (RTPSession * sess, gdouble bandwidth)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  RTP_SESSION_LOCK (sess);
  sess->stats.bandwidth = bandwidth;
  RTP_SESSION_UNLOCK (sess);
}

/**
 * rtp_session_get_bandwidth:
 * @sess: an #RTPSession
 *
 * Get the session bandwidth.
 *
 * Returns: the session bandwidth.
 */
gdouble
rtp_session_get_bandwidth (RTPSession * sess)
{
  gdouble result;

  g_return_val_if_fail (RTP_IS_SESSION (sess), 0);

  RTP_SESSION_LOCK (sess);
  result = sess->stats.bandwidth;
  RTP_SESSION_UNLOCK (sess);

  return result;
}

/**
 * rtp_session_set_rtcp_fraction:
 * @sess: an #RTPSession
 * @bandwidth: the RTCP bandwidth
 *
 * Set the bandwidth in bits per second that should be used for RTCP
 * messages.
 */
void
rtp_session_set_rtcp_fraction (RTPSession * sess, gdouble bandwidth)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  RTP_SESSION_LOCK (sess);
  sess->stats.rtcp_bandwidth = bandwidth;
  RTP_SESSION_UNLOCK (sess);
}

/**
 * rtp_session_get_rtcp_fraction:
 * @sess: an #RTPSession
 *
 * Get the session bandwidth used for RTCP.
 *
 * Returns: The bandwidth used for RTCP messages.
 */
gdouble
rtp_session_get_rtcp_fraction (RTPSession * sess)
{
  gdouble result;

  g_return_val_if_fail (RTP_IS_SESSION (sess), 0.0);

  RTP_SESSION_LOCK (sess);
  result = sess->stats.rtcp_bandwidth;
  RTP_SESSION_UNLOCK (sess);

  return result;
}

/**
 * rtp_session_get_sdes_struct:
 * @sess: an #RTSPSession
 *
 * Get the SDES data as a #GstStructure
 *
 * Returns: a GstStructure with SDES items for @sess. This function returns a
 * copy of the SDES structure, use gst_structure_free() after usage.
 */
GstStructure *
rtp_session_get_sdes_struct (RTPSession * sess)
{
  GstStructure *result = NULL;

  g_return_val_if_fail (RTP_IS_SESSION (sess), NULL);

  RTP_SESSION_LOCK (sess);
  if (sess->sdes)
    result = gst_structure_copy (sess->sdes);
  RTP_SESSION_UNLOCK (sess);

  return result;
}

static void
source_set_sdes (const gchar * key, RTPSource * source, GstStructure * sdes)
{
  rtp_source_set_sdes_struct (source, gst_structure_copy (sdes));
}

/**
 * rtp_session_set_sdes_struct:
 * @sess: an #RTSPSession
 * @sdes: a #GstStructure
 *
 * Set the SDES data as a #GstStructure. This function makes a copy of @sdes.
 */
void
rtp_session_set_sdes_struct (RTPSession * sess, const GstStructure * sdes)
{
  g_return_if_fail (sdes);
  g_return_if_fail (RTP_IS_SESSION (sess));

  RTP_SESSION_LOCK (sess);
  if (sess->sdes)
    gst_structure_free (sess->sdes);
  sess->sdes = gst_structure_copy (sdes);

  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) source_set_sdes, sess->sdes);
  RTP_SESSION_UNLOCK (sess);
}

static GstFlowReturn
source_push_rtp (RTPSource * source, gpointer data, RTPSession * session)
{
  GstFlowReturn result = GST_FLOW_OK;

  if (source->internal) {
    GST_LOG ("source %08x pushed sender RTP packet", source->ssrc);

    RTP_SESSION_UNLOCK (session);

    if (session->callbacks.send_rtp)
      result =
          session->callbacks.send_rtp (session, source, data,
          session->send_rtp_user_data);
    else {
      gst_mini_object_unref (GST_MINI_OBJECT_CAST (data));
    }
  } else {
    GST_LOG ("source %08x pushed receiver RTP packet", source->ssrc);
    RTP_SESSION_UNLOCK (session);

    if (session->callbacks.process_rtp)
      result =
          session->callbacks.process_rtp (session, source,
          GST_BUFFER_CAST (data), session->process_rtp_user_data);
    else
      gst_buffer_unref (GST_BUFFER_CAST (data));
  }
  RTP_SESSION_LOCK (session);

  return result;
}

static gint
source_clock_rate (RTPSource * source, guint8 pt, RTPSession * session)
{
  gint result;

  RTP_SESSION_UNLOCK (session);

  if (session->callbacks.clock_rate)
    result =
        session->callbacks.clock_rate (session, pt,
        session->clock_rate_user_data);
  else
    result = -1;

  RTP_SESSION_LOCK (session);

  GST_DEBUG ("got clock-rate %d for pt %d", result, pt);

  return result;
}

static RTPSourceCallbacks callbacks = {
  (RTPSourcePushRTP) source_push_rtp,
  (RTPSourceClockRate) source_clock_rate,
};


/**
 * rtp_session_find_conflicting_address:
 * @session: The session the packet came in
 * @address: address to check for
 * @time: The time when the packet that is possibly in conflict arrived
 *
 * Checks if an address which has a conflict is already known. If it is
 * a known conflict, remember the time
 *
 * Returns: TRUE if it was a known conflict, FALSE otherwise
 */
static gboolean
rtp_session_find_conflicting_address (RTPSession * session,
    GSocketAddress * address, GstClockTime time)
{
  return find_conflicting_address (session->conflicting_addresses, address,
      time);
}

/**
 * rtp_session_add_conflicting_address:
 * @session: The session the packet came in
 * @address: address to remember
 * @time: The time when the packet that is in conflict arrived
 *
 * Adds a new conflict address
 */
static void
rtp_session_add_conflicting_address (RTPSession * sess,
    GSocketAddress * address, GstClockTime time)
{
  sess->conflicting_addresses =
      add_conflicting_address (sess->conflicting_addresses, address, time);
}

static void
rtp_session_have_conflict (RTPSession * sess, RTPSource * source,
    GSocketAddress * address, GstClockTime current_time)
{
  guint32 ssrc = rtp_source_get_ssrc (source);

  /* Its a new collision, lets change our SSRC */
  rtp_session_add_conflicting_address (sess, address, current_time);

  /* mark the source BYE */
  rtp_source_mark_bye (source, "SSRC Collision");
  /* if we were suggesting this SSRC, change to something else */
  if (sess->suggested_ssrc == ssrc) {
    sess->suggested_ssrc = rtp_session_create_new_ssrc (sess);
    sess->internal_ssrc_set = TRUE;
  }

  on_ssrc_collision (sess, source);

  rtp_session_schedule_bye_locked (sess, current_time);
}

static gboolean
check_collision (RTPSession * sess, RTPSource * source,
    RTPPacketInfo * pinfo, gboolean rtp)
{
  guint32 ssrc;

  /* If we have no pinfo address, we can't do collision checking */
  if (!pinfo->address)
    return FALSE;

  ssrc = rtp_source_get_ssrc (source);

  if (!source->internal) {
    GSocketAddress *from;

    /* This is not our local source, but lets check if two remote
     * source collide */
    if (rtp) {
      from = source->rtp_from;
    } else {
      from = source->rtcp_from;
    }

    if (from) {
      if (__g_socket_address_equal (from, pinfo->address)) {
        /* Address is the same */
        return FALSE;
      } else {
        GST_LOG ("we have a third-party collision or loop ssrc:%x", ssrc);
        if (sess->favor_new) {
          if (rtp_source_find_conflicting_address (source,
                  pinfo->address, pinfo->current_time)) {
            gchar *buf1;

            buf1 = __g_socket_address_to_string (pinfo->address);
            GST_LOG ("Known conflict on %x for %s, dropping packet", ssrc,
                buf1);
            g_free (buf1);

            return TRUE;
          } else {
            gchar *buf1, *buf2;

            /* Current address is not a known conflict, lets assume this is
             * a new source. Save old address in possible conflict list
             */
            rtp_source_add_conflicting_address (source, from,
                pinfo->current_time);

            buf1 = __g_socket_address_to_string (from);
            buf2 = __g_socket_address_to_string (pinfo->address);

            GST_DEBUG ("New conflict for ssrc %x, replacing %s with %s,"
                " saving old as known conflict", ssrc, buf1, buf2);

            if (rtp)
              rtp_source_set_rtp_from (source, pinfo->address);
            else
              rtp_source_set_rtcp_from (source, pinfo->address);

            g_free (buf1);
            g_free (buf2);

            return FALSE;
          }
        } else {
          /* Don't need to save old addresses, we ignore new sources */
          return TRUE;
        }
      }
    } else {
      /* We don't already have a from address for RTP, just set it */
      if (rtp)
        rtp_source_set_rtp_from (source, pinfo->address);
      else
        rtp_source_set_rtcp_from (source, pinfo->address);
      return FALSE;
    }

    /* FIXME: Log 3rd party collision somehow
     * Maybe should be done in upper layer, only the SDES can tell us
     * if its a collision or a loop
     */
  } else {
    /* This is sending with our ssrc, is it an address we already know */
    if (rtp_session_find_conflicting_address (sess, pinfo->address,
            pinfo->current_time)) {
      /* Its a known conflict, its probably a loop, not a collision
       * lets just drop the incoming packet
       */
      GST_DEBUG ("Our packets are being looped back to us, dropping");
    } else {
      GST_DEBUG ("Collision for SSRC %x from new incoming packet,"
          " change our sender ssrc", ssrc);

      rtp_session_have_conflict (sess, source, pinfo->address,
          pinfo->current_time);
    }
  }

  return TRUE;
}

typedef struct
{
  gboolean is_doing_ptp;
  GSocketAddress *new_addr;
} CompareAddrData;

/* check if the two given ip addr are the same (do not care about the port) */
static gboolean
ip_addr_equal (GSocketAddress * a, GSocketAddress * b)
{
  return
      g_inet_address_equal (g_inet_socket_address_get_address
      (G_INET_SOCKET_ADDRESS (a)),
      g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (b)));
}

static void
compare_rtp_source_addr (const gchar * key, RTPSource * source,
    CompareAddrData * data)
{
  /* only compare ip addr of remote sources which are also not closing */
  if (!source->internal && !source->closing && source->rtp_from) {
    /* look for the first rtp source */
    if (!data->new_addr)
      data->new_addr = source->rtp_from;
    /* compare current ip addr with the first one */
    else
      data->is_doing_ptp &= ip_addr_equal (data->new_addr, source->rtp_from);
  }
}

static void
compare_rtcp_source_addr (const gchar * key, RTPSource * source,
    CompareAddrData * data)
{
  /* only compare ip addr of remote sources which are also not closing */
  if (!source->internal && !source->closing && source->rtcp_from) {
    /* look for the first rtcp source */
    if (!data->new_addr)
      data->new_addr = source->rtcp_from;
    else
      /* compare current ip addr with the first one */
      data->is_doing_ptp &= ip_addr_equal (data->new_addr, source->rtcp_from);
  }
}

/* loop over our non-internal source to know if the session
 * is doing point-to-point */
static void
session_update_ptp (RTPSession * sess)
{
  /* to know if the session is doing point to point, the ip addr
   * of each non-internal (=remotes) source have to be compared
   * to each other.
   */
  gboolean is_doing_rtp_ptp;
  gboolean is_doing_rtcp_ptp;
  CompareAddrData data;

  /* compare the first remote source's ip addr that receive rtp packets
   * with other remote rtp source.
   * it's enough because the session just needs to know if they are all
   * equals or not
   */
  data.is_doing_ptp = TRUE;
  data.new_addr = NULL;
  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) compare_rtp_source_addr, (gpointer) & data);
  is_doing_rtp_ptp = data.is_doing_ptp;

  /* same but about rtcp */
  data.is_doing_ptp = TRUE;
  data.new_addr = NULL;
  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) compare_rtcp_source_addr, (gpointer) & data);
  is_doing_rtcp_ptp = data.is_doing_ptp;

  /* the session is doing point-to-point if all rtp remote have the same
   * ip addr and if all rtcp remote sources have the same ip addr */
  sess->is_doing_ptp = is_doing_rtp_ptp && is_doing_rtcp_ptp;

  GST_DEBUG ("doing point-to-point: %d", sess->is_doing_ptp);
}

static void
add_source (RTPSession * sess, RTPSource * src)
{
  g_hash_table_insert (sess->ssrcs[sess->mask_idx],
      GINT_TO_POINTER (src->ssrc), src);
  /* report the new source ASAP */
  src->generation = sess->generation;
  /* we have one more source now */
  sess->total_sources++;
  if (RTP_SOURCE_IS_ACTIVE (src))
    sess->stats.active_sources++;
  if (src->internal) {
    sess->stats.internal_sources++;
    if (!sess->internal_ssrc_from_caps_or_property
        && sess->suggested_ssrc != src->ssrc) {
      sess->suggested_ssrc = src->ssrc;
      sess->internal_ssrc_set = TRUE;
    }
  }

  /* update point-to-point status */
  if (!src->internal)
    session_update_ptp (sess);
}

static RTPSource *
find_source (RTPSession * sess, guint32 ssrc)
{
  return g_hash_table_lookup (sess->ssrcs[sess->mask_idx],
      GINT_TO_POINTER (ssrc));
}

/* must be called with the session lock, the returned source needs to be
 * unreffed after usage. */
static RTPSource *
obtain_source (RTPSession * sess, guint32 ssrc, gboolean * created,
    RTPPacketInfo * pinfo, gboolean rtp)
{
  RTPSource *source;

  source = find_source (sess, ssrc);
  if (source == NULL) {
    /* make new Source in probation and insert */
    source = rtp_source_new (ssrc);

    GST_DEBUG ("creating new source %08x %p", ssrc, source);

    /* for RTP packets we need to set the source in probation. Receiving RTCP
     * packets of an SSRC, on the other hand, is a strong indication that we
     * are dealing with a valid source. */
    g_object_set (source, "probation", rtp ? sess->probation : 0,
        "max-dropout-time", sess->max_dropout_time, "max-misorder-time",
        sess->max_misorder_time, NULL);

    /* store from address, if any */
    if (pinfo->address) {
      if (rtp)
        rtp_source_set_rtp_from (source, pinfo->address);
      else
        rtp_source_set_rtcp_from (source, pinfo->address);
    }

    /* configure a callback on the source */
    rtp_source_set_callbacks (source, &callbacks, sess);

    add_source (sess, source);
    *created = TRUE;
  } else {
    *created = FALSE;
    /* check for collision, this updates the address when not previously set */
    if (check_collision (sess, source, pinfo, rtp)) {
      return NULL;
    }
    /* Receiving RTCP packets of an SSRC is a strong indication that we
     * are dealing with a valid source. */
    if (!rtp)
      g_object_set (source, "probation", 0, NULL);
  }
  /* update last activity */
  source->last_activity = pinfo->current_time;
  if (rtp)
    source->last_rtp_activity = pinfo->current_time;
  g_object_ref (source);

  return source;
}

/* must be called with the session lock, the returned source needs to be
 * unreffed after usage. */
static RTPSource *
obtain_internal_source (RTPSession * sess, guint32 ssrc, gboolean * created,
    GstClockTime current_time)
{
  RTPSource *source;

  source = find_source (sess, ssrc);
  if (source == NULL) {
    /* make new internal Source and insert */
    source = rtp_source_new (ssrc);

    GST_DEBUG ("creating new internal source %08x %p", ssrc, source);

    source->validated = TRUE;
    source->internal = TRUE;
    source->probation = FALSE;
    rtp_source_set_sdes_struct (source, gst_structure_copy (sess->sdes));
    rtp_source_set_callbacks (source, &callbacks, sess);

    add_source (sess, source);
    *created = TRUE;
  } else {
    *created = FALSE;
  }
  /* update last activity */
  if (current_time != GST_CLOCK_TIME_NONE) {
    source->last_activity = current_time;
    source->last_rtp_activity = current_time;
  }
  g_object_ref (source);

  return source;
}

/**
 * rtp_session_suggest_ssrc:
 * @sess: a #RTPSession
 * @is_random: if the suggested ssrc is random
 *
 * Suggest an unused SSRC in @sess.
 *
 * Returns: a free unused SSRC
 */
guint32
rtp_session_suggest_ssrc (RTPSession * sess, gboolean * is_random)
{
  guint32 result;

  g_return_val_if_fail (RTP_IS_SESSION (sess), 0);

  RTP_SESSION_LOCK (sess);
  result = sess->suggested_ssrc;
  if (is_random)
    *is_random = !sess->internal_ssrc_set;
  RTP_SESSION_UNLOCK (sess);

  return result;
}

/**
 * rtp_session_add_source:
 * @sess: a #RTPSession
 * @src: #RTPSource to add
 *
 * Add @src to @session.
 *
 * Returns: %TRUE on success, %FALSE if a source with the same SSRC already
 * existed in the session.
 */
gboolean
rtp_session_add_source (RTPSession * sess, RTPSource * src)
{
  gboolean result = FALSE;
  RTPSource *find;

  g_return_val_if_fail (RTP_IS_SESSION (sess), FALSE);
  g_return_val_if_fail (src != NULL, FALSE);

  RTP_SESSION_LOCK (sess);
  find = find_source (sess, src->ssrc);
  if (find == NULL) {
    add_source (sess, src);
    result = TRUE;
  }
  RTP_SESSION_UNLOCK (sess);

  return result;
}

/**
 * rtp_session_get_num_sources:
 * @sess: an #RTPSession
 *
 * Get the number of sources in @sess.
 *
 * Returns: The number of sources in @sess.
 */
guint
rtp_session_get_num_sources (RTPSession * sess)
{
  guint result;

  g_return_val_if_fail (RTP_IS_SESSION (sess), FALSE);

  RTP_SESSION_LOCK (sess);
  result = sess->total_sources;
  RTP_SESSION_UNLOCK (sess);

  return result;
}

/**
 * rtp_session_get_num_active_sources:
 * @sess: an #RTPSession
 *
 * Get the number of active sources in @sess. A source is considered active when
 * it has been validated and has not yet received a BYE RTCP message.
 *
 * Returns: The number of active sources in @sess.
 */
guint
rtp_session_get_num_active_sources (RTPSession * sess)
{
  guint result;

  g_return_val_if_fail (RTP_IS_SESSION (sess), 0);

  RTP_SESSION_LOCK (sess);
  result = sess->stats.active_sources;
  RTP_SESSION_UNLOCK (sess);

  return result;
}

/**
 * rtp_session_get_source_by_ssrc:
 * @sess: an #RTPSession
 * @ssrc: an SSRC
 *
 * Find the source with @ssrc in @sess.
 *
 * Returns: a #RTPSource with SSRC @ssrc or NULL if the source was not found.
 * g_object_unref() after usage.
 */
RTPSource *
rtp_session_get_source_by_ssrc (RTPSession * sess, guint32 ssrc)
{
  RTPSource *result;

  g_return_val_if_fail (RTP_IS_SESSION (sess), NULL);

  RTP_SESSION_LOCK (sess);
  result = find_source (sess, ssrc);
  if (result != NULL)
    g_object_ref (result);
  RTP_SESSION_UNLOCK (sess);

  return result;
}

/* should be called with the SESSION lock */
static guint32
rtp_session_create_new_ssrc (RTPSession * sess)
{
  guint32 ssrc;

  while (TRUE) {
    ssrc = g_random_int ();

    /* see if it exists in the session, we're done if it doesn't */
    if (find_source (sess, ssrc) == NULL)
      break;
  }
  return ssrc;
}

static gboolean
update_packet (GstBuffer ** buffer, guint idx, RTPPacketInfo * pinfo)
{
  GstNetAddressMeta *meta;

  /* get packet size including header overhead */
  pinfo->bytes += gst_buffer_get_size (*buffer) + pinfo->header_len;
  pinfo->packets++;

  if (pinfo->rtp) {
    GstRTPBuffer rtp = { NULL };

    if (!gst_rtp_buffer_map (*buffer, GST_MAP_READ, &rtp))
      goto invalid_packet;

    pinfo->payload_len += gst_rtp_buffer_get_payload_len (&rtp);
    if (idx == 0) {
      gint i;

      /* only keep info for first buffer */
      pinfo->ssrc = gst_rtp_buffer_get_ssrc (&rtp);
      pinfo->seqnum = gst_rtp_buffer_get_seq (&rtp);
      pinfo->pt = gst_rtp_buffer_get_payload_type (&rtp);
      pinfo->rtptime = gst_rtp_buffer_get_timestamp (&rtp);
      /* copy available csrc */
      pinfo->csrc_count = gst_rtp_buffer_get_csrc_count (&rtp);
      for (i = 0; i < pinfo->csrc_count; i++)
        pinfo->csrcs[i] = gst_rtp_buffer_get_csrc (&rtp, i);
    }
    gst_rtp_buffer_unmap (&rtp);
  }

  if (idx == 0) {
    /* for netbuffer we can store the IP address to check for collisions */
    meta = gst_buffer_get_net_address_meta (*buffer);
    if (pinfo->address)
      g_object_unref (pinfo->address);
    if (meta) {
      pinfo->address = G_SOCKET_ADDRESS (g_object_ref (meta->addr));
    } else {
      pinfo->address = NULL;
    }
  }
  return TRUE;

  /* ERRORS */
invalid_packet:
  {
    GST_DEBUG ("invalid RTP packet received");
    return FALSE;
  }
}

/* update the RTPPacketInfo structure with the current time and other bits
 * about the current buffer we are handling.
 * This function is typically called when a validated packet is received.
 * This function should be called with the RTP_SESSION_LOCK
 */
static gboolean
update_packet_info (RTPSession * sess, RTPPacketInfo * pinfo,
    gboolean send, gboolean rtp, gboolean is_list, gpointer data,
    GstClockTime current_time, GstClockTime running_time, guint64 ntpnstime)
{
  gboolean res;

  pinfo->send = send;
  pinfo->rtp = rtp;
  pinfo->is_list = is_list;
  pinfo->data = data;
  pinfo->current_time = current_time;
  pinfo->running_time = running_time;
  pinfo->ntpnstime = ntpnstime;
  pinfo->header_len = sess->header_len;
  pinfo->bytes = 0;
  pinfo->payload_len = 0;
  pinfo->packets = 0;

  if (is_list) {
    GstBufferList *list = GST_BUFFER_LIST_CAST (data);
    res =
        gst_buffer_list_foreach (list, (GstBufferListFunc) update_packet,
        pinfo);
  } else {
    GstBuffer *buffer = GST_BUFFER_CAST (data);
    res = update_packet (&buffer, 0, pinfo);
  }
  return res;
}

static void
clean_packet_info (RTPPacketInfo * pinfo)
{
  if (pinfo->address)
    g_object_unref (pinfo->address);
  if (pinfo->data) {
    gst_mini_object_unref (pinfo->data);
    pinfo->data = NULL;
  }
}

static gboolean
source_update_active (RTPSession * sess, RTPSource * source,
    gboolean prevactive)
{
  gboolean active = RTP_SOURCE_IS_ACTIVE (source);
  guint32 ssrc = source->ssrc;

  if (prevactive == active)
    return FALSE;

  if (active) {
    sess->stats.active_sources++;
    GST_DEBUG ("source: %08x became active, %d active sources", ssrc,
        sess->stats.active_sources);
  } else {
    sess->stats.active_sources--;
    GST_DEBUG ("source: %08x became inactive, %d active sources", ssrc,
        sess->stats.active_sources);
  }
  return TRUE;
}

static gboolean
source_update_sender (RTPSession * sess, RTPSource * source,
    gboolean prevsender)
{
  gboolean sender = RTP_SOURCE_IS_SENDER (source);
  guint32 ssrc = source->ssrc;

  if (prevsender == sender)
    return FALSE;

  if (sender) {
    sess->stats.sender_sources++;
    if (source->internal)
      sess->stats.internal_sender_sources++;
    GST_DEBUG ("source: %08x became sender, %d sender sources", ssrc,
        sess->stats.sender_sources);
  } else {
    sess->stats.sender_sources--;
    if (source->internal)
      sess->stats.internal_sender_sources--;
    GST_DEBUG ("source: %08x became non sender, %d sender sources", ssrc,
        sess->stats.sender_sources);
  }
  return TRUE;
}

/**
 * rtp_session_process_rtp:
 * @sess: and #RTPSession
 * @buffer: an RTP buffer
 * @current_time: the current system time
 * @running_time: the running_time of @buffer
 *
 * Process an RTP buffer in the session manager. This function takes ownership
 * of @buffer.
 *
 * Returns: a #GstFlowReturn.
 */
GstFlowReturn
rtp_session_process_rtp (RTPSession * sess, GstBuffer * buffer,
    GstClockTime current_time, GstClockTime running_time, guint64 ntpnstime)
{
  GstFlowReturn result;
  guint32 ssrc;
  RTPSource *source;
  gboolean created;
  gboolean prevsender, prevactive;
  RTPPacketInfo pinfo = { 0, };
  guint64 oldrate;

  g_return_val_if_fail (RTP_IS_SESSION (sess), GST_FLOW_ERROR);
  g_return_val_if_fail (GST_IS_BUFFER (buffer), GST_FLOW_ERROR);

  RTP_SESSION_LOCK (sess);

  /* update pinfo stats */
  if (!update_packet_info (sess, &pinfo, FALSE, TRUE, FALSE, buffer,
          current_time, running_time, ntpnstime)) {
    GST_DEBUG ("invalid RTP packet received");
    RTP_SESSION_UNLOCK (sess);
    return rtp_session_process_rtcp (sess, buffer, current_time, running_time,
        ntpnstime);
  }

  ssrc = pinfo.ssrc;

  source = obtain_source (sess, ssrc, &created, &pinfo, TRUE);
  if (!source)
    goto collision;

  prevsender = RTP_SOURCE_IS_SENDER (source);
  prevactive = RTP_SOURCE_IS_ACTIVE (source);
  oldrate = source->bitrate;

  if (created)
    on_new_ssrc (sess, source);

  /* let source process the packet */
  result = rtp_source_process_rtp (source, &pinfo);

  /* source became active */
  if (source_update_active (sess, source, prevactive))
    on_ssrc_validated (sess, source);

  source_update_sender (sess, source, prevsender);

  if (oldrate != source->bitrate)
    sess->recalc_bandwidth = TRUE;


  if (source->validated) {
    gboolean created;
    gint i;

    /* for validated sources, we add the CSRCs as well */
    for (i = 0; i < pinfo.csrc_count; i++) {
      guint32 csrc;
      RTPSource *csrc_src;

      csrc = pinfo.csrcs[i];

      /* get source */
      csrc_src = obtain_source (sess, csrc, &created, &pinfo, TRUE);
      if (!csrc_src)
        continue;

      if (created) {
        GST_DEBUG ("created new CSRC: %08x", csrc);
        rtp_source_set_as_csrc (csrc_src);
        source_update_active (sess, csrc_src, FALSE);
        on_new_ssrc (sess, csrc_src);
      }
      g_object_unref (csrc_src);
    }
  }
  g_object_unref (source);

  RTP_SESSION_UNLOCK (sess);

  clean_packet_info (&pinfo);

  return result;

  /* ERRORS */
collision:
  {
    RTP_SESSION_UNLOCK (sess);
    clean_packet_info (&pinfo);
    GST_DEBUG ("ignoring packet because its collisioning");
    return GST_FLOW_OK;
  }
}

static void
rtp_session_process_rb (RTPSession * sess, RTPSource * source,
    GstRTCPPacket * packet, RTPPacketInfo * pinfo)
{
  guint count, i;

  count = gst_rtcp_packet_get_rb_count (packet);
  for (i = 0; i < count; i++) {
    guint32 ssrc, exthighestseq, jitter, lsr, dlsr;
    guint8 fractionlost;
    gint32 packetslost;
    RTPSource *src;

    gst_rtcp_packet_get_rb (packet, i, &ssrc, &fractionlost,
        &packetslost, &exthighestseq, &jitter, &lsr, &dlsr);

    GST_DEBUG ("RB %d: SSRC %08x, jitter %" G_GUINT32_FORMAT, i, ssrc, jitter);

    /* find our own source */
    src = find_source (sess, ssrc);
    if (src == NULL)
      continue;

    if (src->internal && RTP_SOURCE_IS_ACTIVE (src)) {
      /* only deal with report blocks for our session, we update the stats of
       * the sender of the RTCP message. We could also compare our stats against
       * the other sender to see if we are better or worse. */
      /* FIXME, need to keep track who the RB block is from */
      rtp_source_process_rb (source, pinfo->ntpnstime, fractionlost,
          packetslost, exthighestseq, jitter, lsr, dlsr);
    }
  }
  on_ssrc_active (sess, source);
}

/* A Sender report contains statistics about how the sender is doing. This
 * includes timing informataion such as the relation between RTP and NTP
 * timestamps and the number of packets/bytes it sent to us.
 *
 * In this report is also included a set of report blocks related to how this
 * sender is receiving data (in case we (or somebody else) is also sending stuff
 * to it). This info includes the packet loss, jitter and seqnum. It also
 * contains information to calculate the round trip time (LSR/DLSR).
 */
static void
rtp_session_process_sr (RTPSession * sess, GstRTCPPacket * packet,
    RTPPacketInfo * pinfo, gboolean * do_sync)
{
  guint32 senderssrc, rtptime, packet_count, octet_count;
  guint64 ntptime;
  RTPSource *source;
  gboolean created, prevsender;

  gst_rtcp_packet_sr_get_sender_info (packet, &senderssrc, &ntptime, &rtptime,
      &packet_count, &octet_count);

  GST_DEBUG ("got SR packet: SSRC %08x, time %" GST_TIME_FORMAT,
      senderssrc, GST_TIME_ARGS (pinfo->current_time));

  source = obtain_source (sess, senderssrc, &created, pinfo, FALSE);
  if (!source)
    return;

  /* skip non-bye packets for sources that are marked BYE */
  if (sess->scheduled_bye && RTP_SOURCE_IS_MARKED_BYE (source))
    goto out;

  /* don't try to do lip-sync for sources that sent a BYE */
  if (RTP_SOURCE_IS_MARKED_BYE (source))
    *do_sync = FALSE;
  else
    *do_sync = TRUE;

  prevsender = RTP_SOURCE_IS_SENDER (source);

  /* first update the source */
  rtp_source_process_sr (source, pinfo->current_time, ntptime, rtptime,
      packet_count, octet_count);

  source_update_sender (sess, source, prevsender);

  if (created)
    on_new_ssrc (sess, source);

  rtp_session_process_rb (sess, source, packet, pinfo);

out:
  g_object_unref (source);
}

/* A receiver report contains statistics about how a receiver is doing. It
 * includes stuff like packet loss, jitter and the seqnum it received last. It
 * also contains info to calculate the round trip time.
 *
 * We are only interested in how the sender of this report is doing wrt to us.
 */
static void
rtp_session_process_rr (RTPSession * sess, GstRTCPPacket * packet,
    RTPPacketInfo * pinfo)
{
  guint32 senderssrc;
  RTPSource *source;
  gboolean created;

  senderssrc = gst_rtcp_packet_rr_get_ssrc (packet);

  GST_DEBUG ("got RR packet: SSRC %08x", senderssrc);

  source = obtain_source (sess, senderssrc, &created, pinfo, FALSE);
  if (!source)
    return;

  /* skip non-bye packets for sources that are marked BYE */
  if (sess->scheduled_bye && RTP_SOURCE_IS_MARKED_BYE (source))
    goto out;

  if (created)
    on_new_ssrc (sess, source);

  rtp_session_process_rb (sess, source, packet, pinfo);

out:
  g_object_unref (source);
}

/* Get SDES items and store them in the SSRC */
static void
rtp_session_process_sdes (RTPSession * sess, GstRTCPPacket * packet,
    RTPPacketInfo * pinfo)
{
  guint items, i, j;
  gboolean more_items, more_entries;

  items = gst_rtcp_packet_sdes_get_item_count (packet);
  GST_DEBUG ("got SDES packet with %d items", items);

  more_items = gst_rtcp_packet_sdes_first_item (packet);
  i = 0;
  while (more_items) {
    guint32 ssrc;
    gboolean changed, created, prevactive;
    RTPSource *source;
    GstStructure *sdes;

    ssrc = gst_rtcp_packet_sdes_get_ssrc (packet);

    GST_DEBUG ("item %d, SSRC %08x", i, ssrc);

    changed = FALSE;

    /* find src, no probation when dealing with RTCP */
    source = obtain_source (sess, ssrc, &created, pinfo, FALSE);
    if (!source)
      return;

    /* skip non-bye packets for sources that are marked BYE */
    if (sess->scheduled_bye && RTP_SOURCE_IS_MARKED_BYE (source))
      goto next;

    sdes = gst_structure_new_empty ("application/x-rtp-source-sdes");

    more_entries = gst_rtcp_packet_sdes_first_entry (packet);
    j = 0;
    while (more_entries) {
      GstRTCPSDESType type;
      guint8 len;
      guint8 *data;
      gchar *name;
      gchar *value;

      gst_rtcp_packet_sdes_get_entry (packet, &type, &len, &data);

      GST_DEBUG ("entry %d, type %d, len %d, data %.*s", j, type, len, len,
          data);

      if (type == GST_RTCP_SDES_PRIV) {
        name = g_strndup ((const gchar *) &data[1], data[0]);
        len -= data[0] + 1;
        data += data[0] + 1;
      } else {
        name = g_strdup (gst_rtcp_sdes_type_to_name (type));
      }

      value = g_strndup ((const gchar *) data, len);

      if (g_utf8_validate (value, -1, NULL)) {
        gst_structure_set (sdes, name, G_TYPE_STRING, value, NULL);
      } else {
        GST_WARNING ("ignore SDES field %s with non-utf8 data %s", name, value);
      }

      g_free (name);
      g_free (value);

      more_entries = gst_rtcp_packet_sdes_next_entry (packet);
      j++;
    }

    /* takes ownership of sdes */
    changed = rtp_source_set_sdes_struct (source, sdes);

    prevactive = RTP_SOURCE_IS_ACTIVE (source);
    source->validated = TRUE;

    if (created)
      on_new_ssrc (sess, source);

    /* source became active */
    if (source_update_active (sess, source, prevactive))
      on_ssrc_validated (sess, source);

    if (changed)
      on_ssrc_sdes (sess, source);

  next:
    g_object_unref (source);

    more_items = gst_rtcp_packet_sdes_next_item (packet);
    i++;
  }
}

/* BYE is sent when a client leaves the session
 */
static void
rtp_session_process_bye (RTPSession * sess, GstRTCPPacket * packet,
    RTPPacketInfo * pinfo)
{
  guint count, i;
  gchar *reason;
  gboolean reconsider = FALSE;

  reason = gst_rtcp_packet_bye_get_reason (packet);
  GST_DEBUG ("got BYE packet (reason: %s)", GST_STR_NULL (reason));

  count = gst_rtcp_packet_bye_get_ssrc_count (packet);
  for (i = 0; i < count; i++) {
    guint32 ssrc;
    RTPSource *source;
    gboolean prevactive, prevsender;
    guint pmembers, members;

    ssrc = gst_rtcp_packet_bye_get_nth_ssrc (packet, i);
    GST_DEBUG ("SSRC: %08x", ssrc);

    /* find src and mark bye, no probation when dealing with RTCP */
    source = find_source (sess, ssrc);
    if (!source || source->internal) {
      GST_DEBUG ("Ignoring suspicious BYE packet (reason: %s)",
          !source ? "can't find source" : "has internal source SSRC");
      break;
    }

    /* store time for when we need to time out this source */
    source->bye_time = pinfo->current_time;

    prevactive = RTP_SOURCE_IS_ACTIVE (source);
    prevsender = RTP_SOURCE_IS_SENDER (source);

    /* mark the source BYE */
    rtp_source_mark_bye (source, reason);

    pmembers = sess->stats.active_sources;

    source_update_active (sess, source, prevactive);
    source_update_sender (sess, source, prevsender);

    members = sess->stats.active_sources;

    if (!sess->scheduled_bye && members < pmembers) {
      /* some members went away since the previous timeout estimate.
       * Perform reverse reconsideration but only when we are not scheduling a
       * BYE ourselves. */
      if (sess->next_rtcp_check_time != GST_CLOCK_TIME_NONE &&
          pinfo->current_time < sess->next_rtcp_check_time) {
        GstClockTime time_remaining;

        /* Scale our next RTCP check time according to the change of numbers
         * of members. But only if a) this is the first RTCP, or b) this is not
         * a feedback session, or c) this is a feedback session but we schedule
         * for every RTCP interval (aka no t-rr-interval set).
         *
         * FIXME: a) and b) are not great as we will possibly go below Tmin
         * for non-feedback profiles and in case of a) below
         * Tmin/t-rr-interval in any case.
         */
        if (sess->last_rtcp_send_time == GST_CLOCK_TIME_NONE ||
            !(sess->rtp_profile == GST_RTP_PROFILE_AVPF
                || sess->rtp_profile == GST_RTP_PROFILE_SAVPF) ||
            sess->next_rtcp_check_time - sess->last_rtcp_send_time ==
            sess->last_rtcp_interval) {
          time_remaining = sess->next_rtcp_check_time - pinfo->current_time;
          sess->next_rtcp_check_time =
              gst_util_uint64_scale (time_remaining, members, pmembers);
          sess->next_rtcp_check_time += pinfo->current_time;
        }
        sess->last_rtcp_interval =
            gst_util_uint64_scale (sess->last_rtcp_interval, members, pmembers);

        GST_DEBUG ("reverse reconsideration %" GST_TIME_FORMAT,
            GST_TIME_ARGS (sess->next_rtcp_check_time));

        /* mark pending reconsider. We only want to signal the reconsideration
         * once after we handled all the source in the bye packet */
        reconsider = TRUE;
      }
    }

    on_bye_ssrc (sess, source);
  }
  if (reconsider) {
    RTP_SESSION_UNLOCK (sess);
    /* notify app of reconsideration */
    if (sess->callbacks.reconsider)
      sess->callbacks.reconsider (sess, sess->reconsider_user_data);
    RTP_SESSION_LOCK (sess);
  }

  g_free (reason);
}

static void
rtp_session_process_app (RTPSession * sess, GstRTCPPacket * packet,
    RTPPacketInfo * pinfo)
{
  GST_DEBUG ("received APP");

  if (g_signal_has_handler_pending (sess,
          rtp_session_signals[SIGNAL_ON_APP_RTCP], 0, TRUE)) {
    GstBuffer *data_buffer = NULL;
    guint16 data_length;
    gchar name[5];

    data_length = gst_rtcp_packet_app_get_data_length (packet) * 4;
    if (data_length > 0) {
      guint8 *data = gst_rtcp_packet_app_get_data (packet);
      data_buffer = gst_buffer_copy_region (packet->rtcp->buffer,
          GST_BUFFER_COPY_MEMORY, data - packet->rtcp->map.data, data_length);
      GST_BUFFER_PTS (data_buffer) = pinfo->running_time;
    }

    memcpy (name, gst_rtcp_packet_app_get_name (packet), 4);
    name[4] = '\0';

    RTP_SESSION_UNLOCK (sess);
    g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_APP_RTCP], 0,
        gst_rtcp_packet_app_get_subtype (packet),
        gst_rtcp_packet_app_get_ssrc (packet), name, data_buffer);
    RTP_SESSION_LOCK (sess);

    if (data_buffer)
      gst_buffer_unref (data_buffer);
  }
}

static gboolean
rtp_session_request_local_key_unit (RTPSession * sess, RTPSource * src,
    guint32 media_ssrc, gboolean fir, GstClockTime current_time)
{
  guint32 round_trip = 0;

  rtp_source_get_last_rb (src, NULL, NULL, NULL, NULL, NULL, NULL, &round_trip);

  if (src->last_keyframe_request != GST_CLOCK_TIME_NONE && round_trip) {
    GstClockTime round_trip_in_ns = gst_util_uint64_scale (round_trip,
        GST_SECOND, 65536);

    /* Sanity check to avoid always ignoring PLI/FIR if we receive RTCP
     * packets with erroneous values resulting in crazy high RTT. */
    if (round_trip_in_ns > 5 * GST_SECOND)
      round_trip_in_ns = GST_SECOND / 2;

    if (current_time - src->last_keyframe_request < 2 * round_trip_in_ns) {
      GST_DEBUG ("Ignoring %s request from %X because one was send without one "
          "RTT (%" GST_TIME_FORMAT " < %" GST_TIME_FORMAT ")",
          fir ? "FIR" : "PLI", rtp_source_get_ssrc (src),
          GST_TIME_ARGS (current_time - src->last_keyframe_request),
          GST_TIME_ARGS (round_trip_in_ns));
      return FALSE;
    }
  }

  src->last_keyframe_request = current_time;

  GST_LOG ("received %s request from %X about %X %p(%p)", fir ? "FIR" : "PLI",
      rtp_source_get_ssrc (src), media_ssrc, sess->callbacks.process_rtp,
      sess->callbacks.request_key_unit);

  RTP_SESSION_UNLOCK (sess);
  sess->callbacks.request_key_unit (sess, media_ssrc, fir,
      sess->request_key_unit_user_data);
  RTP_SESSION_LOCK (sess);

  return TRUE;
}

static void
rtp_session_process_pli (RTPSession * sess, guint32 sender_ssrc,
    guint32 media_ssrc, GstClockTime current_time)
{
  RTPSource *src;

  if (!sess->callbacks.request_key_unit)
    return;

  src = find_source (sess, sender_ssrc);
  if (src == NULL) {
    /* try to find a src with media_ssrc instead */
    src = find_source (sess, media_ssrc);
    if (src == NULL)
      return;
  }

  rtp_session_request_local_key_unit (sess, src, media_ssrc, FALSE,
      current_time);
}

static void
rtp_session_process_fir (RTPSession * sess, guint32 sender_ssrc,
    guint32 media_ssrc, guint8 * fci_data, guint fci_length,
    GstClockTime current_time)
{
  RTPSource *src;
  guint32 ssrc;
  guint position = 0;
  gboolean our_request = FALSE;

  if (!sess->callbacks.request_key_unit)
    return;

  if (fci_length < 8)
    return;

  src = find_source (sess, sender_ssrc);

  /* Hack because Google fails to set the sender_ssrc correctly */
  if (!src && sender_ssrc == 1) {
    GHashTableIter iter;

    /* we can't find the source if there are multiple */
    if (sess->stats.sender_sources > sess->stats.internal_sender_sources + 1)
      return;

    g_hash_table_iter_init (&iter, sess->ssrcs[sess->mask_idx]);
    while (g_hash_table_iter_next (&iter, NULL, (gpointer *) & src)) {
      if (!src->internal && rtp_source_is_sender (src))
        break;
      src = NULL;
    }
  }
  if (!src)
    return;

  for (position = 0; position < fci_length; position += 8) {
    guint8 *data = fci_data + position;
    RTPSource *own;

    ssrc = GST_READ_UINT32_BE (data);

    own = find_source (sess, ssrc);
    if (own == NULL)
      continue;

    if (own->internal) {
      our_request = TRUE;
      break;
    }
  }
  if (!our_request)
    return;

  rtp_session_request_local_key_unit (sess, src, media_ssrc, TRUE,
      current_time);
}

static void
rtp_session_process_nack (RTPSession * sess, guint32 sender_ssrc,
    guint32 media_ssrc, guint8 * fci_data, guint fci_length,
    GstClockTime current_time)
{
  sess->stats.nacks_received++;

  if (!sess->callbacks.notify_nack)
    return;

  while (fci_length > 0) {
    guint16 seqnum, blp;

    seqnum = GST_READ_UINT16_BE (fci_data);
    blp = GST_READ_UINT16_BE (fci_data + 2);

    GST_DEBUG ("NACK #%u, blp %04x, SSRC 0x%08x", seqnum, blp, media_ssrc);

    RTP_SESSION_UNLOCK (sess);
    sess->callbacks.notify_nack (sess, seqnum, blp, media_ssrc,
        sess->notify_nack_user_data);
    RTP_SESSION_LOCK (sess);

    fci_data += 4;
    fci_length -= 4;
  }
}

static void
rtp_session_process_feedback (RTPSession * sess, GstRTCPPacket * packet,
    RTPPacketInfo * pinfo, GstClockTime current_time)
{
  GstRTCPType type;
  GstRTCPFBType fbtype;
  guint32 sender_ssrc, media_ssrc;
  guint8 *fci_data;
  guint fci_length;
  RTPSource *src;

  /* The feedback packet must include both sender SSRC and media SSRC */
  if (packet->length < 2)
    return;

  type = gst_rtcp_packet_get_type (packet);
  fbtype = gst_rtcp_packet_fb_get_type (packet);
  sender_ssrc = gst_rtcp_packet_fb_get_sender_ssrc (packet);
  media_ssrc = gst_rtcp_packet_fb_get_media_ssrc (packet);

  src = find_source (sess, media_ssrc);

  /* skip non-bye packets for sources that are marked BYE */
  if (sess->scheduled_bye && src && RTP_SOURCE_IS_MARKED_BYE (src))
    return;

  if (src)
    g_object_ref (src);

  fci_data = gst_rtcp_packet_fb_get_fci (packet);
  fci_length = gst_rtcp_packet_fb_get_fci_length (packet) * sizeof (guint32);

  GST_DEBUG ("received feedback %d:%d from %08X about %08X with FCI of "
      "length %d", type, fbtype, sender_ssrc, media_ssrc, fci_length);

  if (g_signal_has_handler_pending (sess,
          rtp_session_signals[SIGNAL_ON_FEEDBACK_RTCP], 0, TRUE)) {
    GstBuffer *fci_buffer = NULL;

    if (fci_length > 0) {
      fci_buffer = gst_buffer_copy_region (packet->rtcp->buffer,
          GST_BUFFER_COPY_MEMORY, fci_data - packet->rtcp->map.data,
          fci_length);
      GST_BUFFER_PTS (fci_buffer) = pinfo->running_time;
    }

    RTP_SESSION_UNLOCK (sess);
    g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_FEEDBACK_RTCP], 0,
        type, fbtype, sender_ssrc, media_ssrc, fci_buffer);
    RTP_SESSION_LOCK (sess);

    if (fci_buffer)
      gst_buffer_unref (fci_buffer);
  }

  if (src && sess->rtcp_feedback_retention_window != GST_CLOCK_TIME_NONE) {
    rtp_source_retain_rtcp_packet (src, packet, pinfo->running_time);
  }

  if ((src && src->internal) ||
      /* PSFB FIR puts the media ssrc inside the FCI */
      (type == GST_RTCP_TYPE_PSFB && fbtype == GST_RTCP_PSFB_TYPE_FIR)) {
    switch (type) {
      case GST_RTCP_TYPE_PSFB:
        switch (fbtype) {
          case GST_RTCP_PSFB_TYPE_PLI:
            if (src)
              src->stats.recv_pli_count++;
            rtp_session_process_pli (sess, sender_ssrc, media_ssrc,
                current_time);
            break;
          case GST_RTCP_PSFB_TYPE_FIR:
            if (src)
              src->stats.recv_fir_count++;
            rtp_session_process_fir (sess, sender_ssrc, media_ssrc, fci_data,
                fci_length, current_time);
            break;
          default:
            break;
        }
        break;
      case GST_RTCP_TYPE_RTPFB:
        switch (fbtype) {
          case GST_RTCP_RTPFB_TYPE_NACK:
            if (src)
              src->stats.recv_nack_count++;
            rtp_session_process_nack (sess, sender_ssrc, media_ssrc,
                fci_data, fci_length, current_time);
            break;
          default:
            break;
        }
      default:
        break;
    }
  }

  if (src)
    g_object_unref (src);
}

/**
 * rtp_session_process_rtcp:
 * @sess: and #RTPSession
 * @buffer: an RTCP buffer
 * @current_time: the current system time
 * @ntpnstime: the current NTP time in nanoseconds
 *
 * Process an RTCP buffer in the session manager. This function takes ownership
 * of @buffer.
 *
 * Returns: a #GstFlowReturn.
 */
GstFlowReturn
rtp_session_process_rtcp (RTPSession * sess, GstBuffer * buffer,
    GstClockTime current_time, GstClockTime running_time, guint64 ntpnstime)
{
  GstRTCPPacket packet;
  gboolean more, is_bye = FALSE, do_sync = FALSE;
  RTPPacketInfo pinfo = { 0, };
  GstFlowReturn result = GST_FLOW_OK;
  GstRTCPBuffer rtcp = { NULL, };

  g_return_val_if_fail (RTP_IS_SESSION (sess), GST_FLOW_ERROR);
  g_return_val_if_fail (GST_IS_BUFFER (buffer), GST_FLOW_ERROR);

  if (!gst_rtcp_buffer_validate_reduced (buffer))
    goto invalid_packet;

  GST_DEBUG ("received RTCP packet");

  g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_RECEIVING_RTCP], 0,
      buffer);

  RTP_SESSION_LOCK (sess);
  /* update pinfo stats */
  update_packet_info (sess, &pinfo, FALSE, FALSE, FALSE, buffer, current_time,
      running_time, ntpnstime);

  /* start processing the compound packet */
  gst_rtcp_buffer_map (buffer, GST_MAP_READ, &rtcp);
  more = gst_rtcp_buffer_get_first_packet (&rtcp, &packet);
  while (more) {
    GstRTCPType type;

    type = gst_rtcp_packet_get_type (&packet);

    switch (type) {
      case GST_RTCP_TYPE_SR:
        rtp_session_process_sr (sess, &packet, &pinfo, &do_sync);
        break;
      case GST_RTCP_TYPE_RR:
        rtp_session_process_rr (sess, &packet, &pinfo);
        break;
      case GST_RTCP_TYPE_SDES:
        rtp_session_process_sdes (sess, &packet, &pinfo);
        break;
      case GST_RTCP_TYPE_BYE:
        is_bye = TRUE;
        /* don't try to attempt lip-sync anymore for streams with a BYE */
        do_sync = FALSE;
        rtp_session_process_bye (sess, &packet, &pinfo);
        break;
      case GST_RTCP_TYPE_APP:
        rtp_session_process_app (sess, &packet, &pinfo);
        break;
      case GST_RTCP_TYPE_RTPFB:
      case GST_RTCP_TYPE_PSFB:
        rtp_session_process_feedback (sess, &packet, &pinfo, current_time);
        break;
      case GST_RTCP_TYPE_XR:
        /* FIXME: This block is added to downgrade warning level.
         * Once the parser is implemented, it should be replaced with
         * a proper process function. */
        GST_DEBUG ("got RTCP XR packet, but ignored");
        break;
      default:
        GST_WARNING ("got unknown RTCP packet type: %d", type);
        break;
    }
    more = gst_rtcp_packet_move_to_next (&packet);
  }

  gst_rtcp_buffer_unmap (&rtcp);

  /* if we are scheduling a BYE, we only want to count bye packets, else we
   * count everything */
  if (sess->scheduled_bye && is_bye) {
    sess->bye_stats.bye_members++;
    UPDATE_AVG (sess->bye_stats.avg_rtcp_packet_size, pinfo.bytes);
  }

  /* keep track of average packet size */
  UPDATE_AVG (sess->stats.avg_rtcp_packet_size, pinfo.bytes);

  GST_DEBUG ("%p, received RTCP packet, avg size %u, %u", &sess->stats,
      sess->stats.avg_rtcp_packet_size, pinfo.bytes);
  RTP_SESSION_UNLOCK (sess);

  pinfo.data = NULL;
  clean_packet_info (&pinfo);

  /* notify caller of sr packets in the callback */
  if (do_sync && sess->callbacks.sync_rtcp) {
    result = sess->callbacks.sync_rtcp (sess, buffer,
        sess->sync_rtcp_user_data);
  } else
    gst_buffer_unref (buffer);

  return result;

  /* ERRORS */
invalid_packet:
  {
    GST_DEBUG ("invalid RTCP packet received");
    gst_buffer_unref (buffer);
    return GST_FLOW_OK;
  }
}

/**
 * rtp_session_update_send_caps:
 * @sess: an #RTPSession
 * @caps: a #GstCaps
 *
 * Update the caps of the sender in the rtp session.
 */
void
rtp_session_update_send_caps (RTPSession * sess, GstCaps * caps)
{
  GstStructure *s;
  guint ssrc;

  g_return_if_fail (RTP_IS_SESSION (sess));
  g_return_if_fail (GST_IS_CAPS (caps));

  GST_LOG ("received caps %" GST_PTR_FORMAT, caps);

  s = gst_caps_get_structure (caps, 0);

  if (gst_structure_get_uint (s, "ssrc", &ssrc)) {
    RTPSource *source;
    gboolean created;

    RTP_SESSION_LOCK (sess);
    source = obtain_internal_source (sess, ssrc, &created, GST_CLOCK_TIME_NONE);
    sess->suggested_ssrc = ssrc;
    sess->internal_ssrc_set = TRUE;
    sess->internal_ssrc_from_caps_or_property = TRUE;
    if (source) {
      rtp_source_update_caps (source, caps);

      if (created)
        on_new_sender_ssrc (sess, source);

      g_object_unref (source);
    }

    if (gst_structure_get_uint (s, "rtx-ssrc", &ssrc)) {
      source =
          obtain_internal_source (sess, ssrc, &created, GST_CLOCK_TIME_NONE);
      if (source) {
        rtp_source_update_caps (source, caps);

        if (created)
          on_new_sender_ssrc (sess, source);

        g_object_unref (source);
      }
    }
    RTP_SESSION_UNLOCK (sess);
  } else {
    sess->internal_ssrc_from_caps_or_property = FALSE;
  }
}

/**
 * rtp_session_send_rtp:
 * @sess: an #RTPSession
 * @data: pointer to either an RTP buffer or a list of RTP buffers
 * @is_list: TRUE when @data is a buffer list
 * @current_time: the current system time
 * @running_time: the running time of @data
 *
 * Send the RTP data (a buffer or buffer list) in the session manager. This
 * function takes ownership of @data.
 *
 * Returns: a #GstFlowReturn.
 */
GstFlowReturn
rtp_session_send_rtp (RTPSession * sess, gpointer data, gboolean is_list,
    GstClockTime current_time, GstClockTime running_time)
{
  GstFlowReturn result;
  RTPSource *source;
  gboolean prevsender;
  guint64 oldrate;
  RTPPacketInfo pinfo = { 0, };
  gboolean created;

  g_return_val_if_fail (RTP_IS_SESSION (sess), GST_FLOW_ERROR);
  g_return_val_if_fail (is_list || GST_IS_BUFFER (data), GST_FLOW_ERROR);

  GST_LOG ("received RTP %s for sending", is_list ? "list" : "packet");

  RTP_SESSION_LOCK (sess);
  if (!update_packet_info (sess, &pinfo, TRUE, TRUE, is_list, data,
          current_time, running_time, -1))
    goto invalid_packet;

  source = obtain_internal_source (sess, pinfo.ssrc, &created, current_time);
  if (created)
    on_new_sender_ssrc (sess, source);

  if (!source->internal) {
    GSocketAddress *from;

    if (source->rtp_from)
      from = source->rtp_from;
    else
      from = source->rtcp_from;
    if (from) {
      if (rtp_session_find_conflicting_address (sess, from, current_time)) {
        /* Its a known conflict, its probably a loop, not a collision
         * lets just drop the incoming packet
         */
        GST_LOG ("Our packets are being looped back to us, ignoring collision");
      } else {
        GST_DEBUG ("Collision for SSRC %x, change our sender ssrc", pinfo.ssrc);

        rtp_session_have_conflict (sess, source, from, current_time);

        goto collision;
      }
    } else {
      GST_LOG ("Ignoring collision on sent SSRC %x because remote source"
          " doesn't have an address", pinfo.ssrc);
    }
  }

  prevsender = RTP_SOURCE_IS_SENDER (source);
  oldrate = source->bitrate;

  /* we use our own source to send */
  result = rtp_source_send_rtp (source, &pinfo);

  source_update_sender (sess, source, prevsender);

  if (oldrate != source->bitrate)
    sess->recalc_bandwidth = TRUE;
  RTP_SESSION_UNLOCK (sess);

  g_object_unref (source);
  clean_packet_info (&pinfo);

  return result;

invalid_packet:
  {
    gst_mini_object_unref (GST_MINI_OBJECT_CAST (data));
    RTP_SESSION_UNLOCK (sess);
    GST_DEBUG ("invalid RTP packet received");
    return GST_FLOW_OK;
  }
collision:
  {
    g_object_unref (source);
    gst_mini_object_unref (GST_MINI_OBJECT_CAST (data));
    RTP_SESSION_UNLOCK (sess);
    GST_WARNING ("non-internal source with same ssrc %08x, drop packet",
        pinfo.ssrc);
    return GST_FLOW_OK;
  }
}

static void
add_bitrates (gpointer key, RTPSource * source, gdouble * bandwidth)
{
  *bandwidth += source->bitrate;
}

/* must be called with session lock */
static GstClockTime
calculate_rtcp_interval (RTPSession * sess, gboolean deterministic,
    gboolean first)
{
  GstClockTime result;
  RTPSessionStats *stats;

  /* recalculate bandwidth when it changed */
  if (sess->recalc_bandwidth) {
    gdouble bandwidth;

    if (sess->bandwidth > 0)
      bandwidth = sess->bandwidth;
    else {
      /* If it is <= 0, then try to estimate the actual bandwidth */
      bandwidth = 0;

      g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
          (GHFunc) add_bitrates, &bandwidth);
    }
    if (bandwidth < RTP_STATS_BANDWIDTH)
      bandwidth = RTP_STATS_BANDWIDTH;

    rtp_stats_set_bandwidths (&sess->stats, bandwidth,
        sess->rtcp_bandwidth, sess->rtcp_rs_bandwidth, sess->rtcp_rr_bandwidth);

    sess->recalc_bandwidth = FALSE;
  }

  if (sess->scheduled_bye) {
    stats = &sess->bye_stats;
    result = rtp_stats_calculate_bye_interval (stats);
  } else {
    session_update_ptp (sess);

    stats = &sess->stats;
    result = rtp_stats_calculate_rtcp_interval (stats,
        stats->internal_sender_sources > 0, sess->rtp_profile,
        sess->is_doing_ptp, first);
  }

  GST_DEBUG ("next deterministic interval: %" GST_TIME_FORMAT ", first %d",
      GST_TIME_ARGS (result), first);

  if (!deterministic && result != GST_CLOCK_TIME_NONE)
    result = rtp_stats_add_rtcp_jitter (stats, result);

  GST_DEBUG ("next interval: %" GST_TIME_FORMAT, GST_TIME_ARGS (result));

  return result;
}

static void
source_mark_bye (const gchar * key, RTPSource * source, const gchar * reason)
{
  if (source->internal)
    rtp_source_mark_bye (source, reason);
}

/**
 * rtp_session_mark_all_bye:
 * @sess: an #RTPSession
 * @reason: a reason
 *
 * Mark all internal sources of the session as BYE with @reason.
 */
void
rtp_session_mark_all_bye (RTPSession * sess, const gchar * reason)
{
  g_return_if_fail (RTP_IS_SESSION (sess));

  RTP_SESSION_LOCK (sess);
  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) source_mark_bye, (gpointer) reason);
  RTP_SESSION_UNLOCK (sess);
}

/* Stop the current @sess and schedule a BYE message for the other members.
 * One must have the session lock to call this function
 */
static GstFlowReturn
rtp_session_schedule_bye_locked (RTPSession * sess, GstClockTime current_time)
{
  GstFlowReturn result = GST_FLOW_OK;
  GstClockTime interval;

  /* nothing to do it we already scheduled bye */
  if (sess->scheduled_bye)
    goto done;

  /* we schedule BYE now */
  sess->scheduled_bye = TRUE;
  /* at least one member wants to send a BYE */
  memcpy (&sess->bye_stats, &sess->stats, sizeof (RTPSessionStats));
  INIT_AVG (sess->bye_stats.avg_rtcp_packet_size, 100);
  sess->bye_stats.bye_members = 1;
  sess->first_rtcp = TRUE;

  /* reschedule transmission */
  sess->last_rtcp_send_time = current_time;
  sess->last_rtcp_check_time = current_time;
  interval = calculate_rtcp_interval (sess, FALSE, TRUE);

  if (interval != GST_CLOCK_TIME_NONE)
    sess->next_rtcp_check_time = current_time + interval;
  else
    sess->next_rtcp_check_time = GST_CLOCK_TIME_NONE;
  sess->last_rtcp_interval = interval;

  GST_DEBUG ("Schedule BYE for %" GST_TIME_FORMAT ", %" GST_TIME_FORMAT,
      GST_TIME_ARGS (interval), GST_TIME_ARGS (sess->next_rtcp_check_time));

  RTP_SESSION_UNLOCK (sess);
  /* notify app of reconsideration */
  if (sess->callbacks.reconsider)
    sess->callbacks.reconsider (sess, sess->reconsider_user_data);
  RTP_SESSION_LOCK (sess);
done:

  return result;
}

/**
 * rtp_session_schedule_bye:
 * @sess: an #RTPSession
 * @current_time: the current system time
 *
 * Schedule a BYE message for all sources marked as BYE in @sess.
 *
 * Returns: a #GstFlowReturn.
 */
GstFlowReturn
rtp_session_schedule_bye (RTPSession * sess, GstClockTime current_time)
{
  GstFlowReturn result;

  g_return_val_if_fail (RTP_IS_SESSION (sess), GST_FLOW_ERROR);

  RTP_SESSION_LOCK (sess);
  result = rtp_session_schedule_bye_locked (sess, current_time);
  RTP_SESSION_UNLOCK (sess);

  return result;
}

/**
 * rtp_session_next_timeout:
 * @sess: an #RTPSession
 * @current_time: the current system time
 *
 * Get the next time we should perform session maintenance tasks.
 *
 * Returns: a time when rtp_session_on_timeout() should be called with the
 * current system time.
 */
GstClockTime
rtp_session_next_timeout (RTPSession * sess, GstClockTime current_time)
{
  GstClockTime result, interval = 0;

  g_return_val_if_fail (RTP_IS_SESSION (sess), GST_CLOCK_TIME_NONE);

  RTP_SESSION_LOCK (sess);

  if (GST_CLOCK_TIME_IS_VALID (sess->next_early_rtcp_time)) {
    GST_DEBUG ("have early rtcp time");
    result = sess->next_early_rtcp_time;
    goto early_exit;
  }

  result = sess->next_rtcp_check_time;

  GST_DEBUG ("current time: %" GST_TIME_FORMAT
      ", next time: %" GST_TIME_FORMAT,
      GST_TIME_ARGS (current_time), GST_TIME_ARGS (result));

  if (result == GST_CLOCK_TIME_NONE || result < current_time) {
    GST_DEBUG ("take current time as base");
    /* our previous check time expired, start counting from the current time
     * again. */
    result = current_time;
  }

  if (sess->scheduled_bye) {
    if (sess->bye_stats.active_sources >= 50) {
      GST_DEBUG ("reconsider BYE, more than 50 sources");
      /* reconsider BYE if members >= 50 */
      interval = calculate_rtcp_interval (sess, FALSE, TRUE);
      sess->last_rtcp_interval = interval;
    }
  } else {
    if (sess->first_rtcp) {
      GST_DEBUG ("first RTCP packet");
      /* we are called for the first time */
      interval = calculate_rtcp_interval (sess, FALSE, TRUE);
      sess->last_rtcp_interval = interval;
    } else if (sess->next_rtcp_check_time < current_time) {
      GST_DEBUG ("old check time expired, getting new timeout");
      /* get a new timeout when we need to */
      interval = calculate_rtcp_interval (sess, FALSE, FALSE);
      sess->last_rtcp_interval = interval;

      if ((sess->rtp_profile == GST_RTP_PROFILE_AVPF
              || sess->rtp_profile == GST_RTP_PROFILE_SAVPF)
          && interval != GST_CLOCK_TIME_NONE) {
        /* Apply the rules from RFC 4585 section 3.5.3 */
        if (sess->stats.min_interval != 0) {
          GstClockTime T_rr_current_interval = g_random_double_range (0.5,
              1.5) * sess->stats.min_interval * GST_SECOND;

          if (T_rr_current_interval > interval) {
            GST_DEBUG ("Adjusting interval for t-rr-interval: %" GST_TIME_FORMAT
                " > %" GST_TIME_FORMAT, GST_TIME_ARGS (T_rr_current_interval),
                GST_TIME_ARGS (interval));
            interval = T_rr_current_interval;
          }
        }
      }
    }
  }

  if (interval != GST_CLOCK_TIME_NONE)
    result += interval;
  else
    result = GST_CLOCK_TIME_NONE;

  sess->next_rtcp_check_time = result;

early_exit:

  GST_DEBUG ("current time: %" GST_TIME_FORMAT
      ", next time: %" GST_TIME_FORMAT,
      GST_TIME_ARGS (current_time), GST_TIME_ARGS (result));
  RTP_SESSION_UNLOCK (sess);

  return result;
}

typedef struct
{
  RTPSource *source;
  gboolean is_bye;
  GstBuffer *buffer;
} ReportOutput;

typedef struct
{
  GstRTCPBuffer rtcpbuf;
  RTPSession *sess;
  RTPSource *source;
  guint num_to_report;
  gboolean have_fir;
  gboolean have_pli;
  gboolean have_nack;
  GstBuffer *rtcp;
  GstClockTime current_time;
  guint64 ntpnstime;
  GstClockTime running_time;
  GstClockTime interval;
  GstRTCPPacket packet;
  gboolean has_sdes;
  gboolean is_early;
  gboolean may_suppress;
  GQueue output;
  guint nacked_seqnums;
} ReportData;

static void
session_start_rtcp (RTPSession * sess, ReportData * data)
{
  GstRTCPPacket *packet = &data->packet;
  RTPSource *own = data->source;
  GstRTCPBuffer *rtcp = &data->rtcpbuf;

  data->rtcp = gst_rtcp_buffer_new (sess->mtu);
  data->has_sdes = FALSE;

  gst_rtcp_buffer_map (data->rtcp, GST_MAP_READWRITE, rtcp);

  if (data->is_early && sess->reduced_size_rtcp)
    return;

  if (RTP_SOURCE_IS_SENDER (own)) {
    guint64 ntptime;
    guint32 rtptime;
    guint32 packet_count, octet_count;

    /* we are a sender, create SR */
    GST_DEBUG ("create SR for SSRC %08x", own->ssrc);
    gst_rtcp_buffer_add_packet (rtcp, GST_RTCP_TYPE_SR, packet);

    /* get latest stats */
    rtp_source_get_new_sr (own, data->ntpnstime, data->running_time,
        &ntptime, &rtptime, &packet_count, &octet_count);
    /* store stats */
    rtp_source_process_sr (own, data->current_time, ntptime, rtptime,
        packet_count, octet_count);

    /* fill in sender report info */
    gst_rtcp_packet_sr_set_sender_info (packet, own->ssrc,
        sess->timestamp_sender_reports ? ntptime : 0,
        sess->timestamp_sender_reports ? rtptime : 0,
        packet_count, octet_count);
  } else {
    /* we are only receiver, create RR */
    GST_DEBUG ("create RR for SSRC %08x", own->ssrc);
    gst_rtcp_buffer_add_packet (rtcp, GST_RTCP_TYPE_RR, packet);
    gst_rtcp_packet_rr_set_ssrc (packet, own->ssrc);
  }
}

/* construct a Sender or Receiver Report */
static void
session_report_blocks (const gchar * key, RTPSource * source, ReportData * data)
{
  RTPSession *sess = data->sess;
  GstRTCPPacket *packet = &data->packet;
  guint8 fractionlost;
  gint32 packetslost;
  guint32 exthighestseq, jitter;
  guint32 lsr, dlsr;

  /* don't report for sources in future generations */
  if (((gint16) (source->generation - sess->generation)) > 0) {
    GST_DEBUG ("source %08x generation %u > %u", source->ssrc,
        source->generation, sess->generation);
    return;
  }

  if (g_hash_table_contains (source->reported_in_sr_of,
          GUINT_TO_POINTER (data->source->ssrc))) {
    GST_DEBUG ("source %08x already reported in this generation", source->ssrc);
    return;
  }

  if (gst_rtcp_packet_get_rb_count (packet) == GST_RTCP_MAX_RB_COUNT) {
    GST_DEBUG ("max RB count reached");
    return;
  }

  /* only report about remote sources */
  if (source->internal)
    goto reported;

  if (!RTP_SOURCE_IS_SENDER (source)) {
    GST_DEBUG ("source %08x not sender", source->ssrc);
    goto reported;
  }

  if (source->disable_rtcp) {
    GST_DEBUG ("source %08x has RTCP disabled", source->ssrc);
    goto reported;
  }

  GST_DEBUG ("create RB for SSRC %08x", source->ssrc);

  /* get new stats */
  rtp_source_get_new_rb (source, data->current_time, &fractionlost,
      &packetslost, &exthighestseq, &jitter, &lsr, &dlsr);

  /* store last generated RR packet */
  source->last_rr.is_valid = TRUE;
  source->last_rr.fractionlost = fractionlost;
  source->last_rr.packetslost = packetslost;
  source->last_rr.exthighestseq = exthighestseq;
  source->last_rr.jitter = jitter;
  source->last_rr.lsr = lsr;
  source->last_rr.dlsr = dlsr;

  /* packet is not yet filled, add report block for this source. */
  gst_rtcp_packet_add_rb (packet, source->ssrc, fractionlost, packetslost,
      exthighestseq, jitter, lsr, dlsr);

reported:
  g_hash_table_add (source->reported_in_sr_of,
      GUINT_TO_POINTER (data->source->ssrc));
}

/* construct FIR */
static void
session_add_fir (const gchar * key, RTPSource * source, ReportData * data)
{
  GstRTCPPacket *packet = &data->packet;
  guint16 len;
  guint8 *fci_data;

  if (!source->send_fir)
    return;

  len = gst_rtcp_packet_fb_get_fci_length (packet);
  if (!gst_rtcp_packet_fb_set_fci_length (packet, len + 2))
    /* exit because the packet is full, will put next request in a
     * further packet */
    return;

  fci_data = gst_rtcp_packet_fb_get_fci (packet) + (len * 4);

  GST_WRITE_UINT32_BE (fci_data, source->ssrc);
  fci_data += 4;
  fci_data[0] = source->current_send_fir_seqnum;
  fci_data[1] = fci_data[2] = fci_data[3] = 0;

  source->send_fir = FALSE;
  source->stats.sent_fir_count++;
}

static void
session_fir (RTPSession * sess, ReportData * data)
{
  GstRTCPBuffer *rtcp = &data->rtcpbuf;
  GstRTCPPacket *packet = &data->packet;

  if (!gst_rtcp_buffer_add_packet (rtcp, GST_RTCP_TYPE_PSFB, packet))
    return;

  gst_rtcp_packet_fb_set_type (packet, GST_RTCP_PSFB_TYPE_FIR);
  gst_rtcp_packet_fb_set_sender_ssrc (packet, data->source->ssrc);
  gst_rtcp_packet_fb_set_media_ssrc (packet, 0);

  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) session_add_fir, data);

  if (gst_rtcp_packet_fb_get_fci_length (packet) == 0)
    gst_rtcp_packet_remove (packet);
  else
    data->may_suppress = FALSE;
}

static gboolean
has_pli_compare_func (gconstpointer a, gconstpointer ignored)
{
  GstRTCPPacket packet;
  GstRTCPBuffer rtcp = { NULL, };
  gboolean ret = FALSE;

  gst_rtcp_buffer_map ((GstBuffer *) a, GST_MAP_READ, &rtcp);

  if (gst_rtcp_buffer_get_first_packet (&rtcp, &packet)) {
    if (gst_rtcp_packet_get_type (&packet) == GST_RTCP_TYPE_PSFB &&
        gst_rtcp_packet_fb_get_type (&packet) == GST_RTCP_PSFB_TYPE_PLI)
      ret = TRUE;
  }

  gst_rtcp_buffer_unmap (&rtcp);

  return ret;
}

/* construct PLI */
static void
session_pli (const gchar * key, RTPSource * source, ReportData * data)
{
  GstRTCPBuffer *rtcp = &data->rtcpbuf;
  GstRTCPPacket *packet = &data->packet;

  if (!source->send_pli)
    return;

  if (rtp_source_has_retained (source, has_pli_compare_func, NULL))
    return;

  if (!gst_rtcp_buffer_add_packet (rtcp, GST_RTCP_TYPE_PSFB, packet))
    /* exit because the packet is full, will put next request in a
     * further packet */
    return;

  gst_rtcp_packet_fb_set_type (packet, GST_RTCP_PSFB_TYPE_PLI);
  gst_rtcp_packet_fb_set_sender_ssrc (packet, data->source->ssrc);
  gst_rtcp_packet_fb_set_media_ssrc (packet, source->ssrc);

  source->send_pli = FALSE;
  data->may_suppress = FALSE;

  source->stats.sent_pli_count++;
}

/* construct NACK */
static void
session_nack (const gchar * key, RTPSource * source, ReportData * data)
{
  RTPSession *sess = data->sess;
  GstRTCPBuffer *rtcp = &data->rtcpbuf;
  GstRTCPPacket *packet = &data->packet;
  guint16 *nacks;
  GstClockTime *nack_deadlines;
  guint n_nacks, i = 0;
  guint nacked_seqnums = 0;
  guint16 n_fb_nacks = 0;
  guint8 *fci_data;

  if (!source->send_nack)
    return;

  nacks = rtp_source_get_nacks (source, &n_nacks);
  nack_deadlines = rtp_source_get_nack_deadlines (source, NULL);
  GST_DEBUG ("%u NACKs current time %" GST_TIME_FORMAT, n_nacks,
      GST_TIME_ARGS (data->current_time));

  /* cleanup expired nacks */
  for (i = 0; i < n_nacks; i++) {
    GST_DEBUG ("#%u deadline %" GST_TIME_FORMAT, nacks[i],
        GST_TIME_ARGS (nack_deadlines[i]));
    if (nack_deadlines[i] >= data->current_time)
      break;
  }

  if (data->is_early) {
    /* don't remove them all if this is an early RTCP packet. It may happen
     * that the NACKs are late due to high RTT, not sending NACKs at all would
     * keep the RTX RTT stats high and maintain a dropping state. */
    i = MIN (n_nacks - 1, i);
  }

  if (i) {
    GST_WARNING ("Removing %u expired NACKS", i);
    rtp_source_clear_nacks (source, i);
    n_nacks -= i;
    if (n_nacks == 0)
      return;
  }

  /* allow overriding NACK to packet conversion */
  if (g_signal_has_handler_pending (sess,
          rtp_session_signals[SIGNAL_ON_SENDING_NACKS], 0, TRUE)) {
    /* this is needed as it will actually resize the buffer */
    gst_rtcp_buffer_unmap (rtcp);

    g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_SENDING_NACKS], 0,
        data->source->ssrc, source->ssrc, source->nacks, data->rtcp,
        &nacked_seqnums);

    /* and now remap for the remaining work */
    gst_rtcp_buffer_map (data->rtcp, GST_MAP_READWRITE, rtcp);

    if (nacked_seqnums > 0)
      goto done;
  }

  if (!gst_rtcp_buffer_add_packet (rtcp, GST_RTCP_TYPE_RTPFB, packet))
    /* exit because the packet is full, will put next request in a
     * further packet */
    return;

  gst_rtcp_packet_fb_set_type (packet, GST_RTCP_RTPFB_TYPE_NACK);
  gst_rtcp_packet_fb_set_sender_ssrc (packet, data->source->ssrc);
  gst_rtcp_packet_fb_set_media_ssrc (packet, source->ssrc);

  if (!gst_rtcp_packet_fb_set_fci_length (packet, 1)) {
    gst_rtcp_packet_remove (packet);
    GST_WARNING ("no nacks fit in the packet");
    return;
  }

  fci_data = gst_rtcp_packet_fb_get_fci (packet);
  for (i = 0; i < n_nacks; i = nacked_seqnums) {
    guint16 seqnum = nacks[i];
    guint16 blp = 0;
    guint j;

    if (!gst_rtcp_packet_fb_set_fci_length (packet, n_fb_nacks + 1))
      break;

    n_fb_nacks++;
    nacked_seqnums++;

    for (j = i + 1; j < n_nacks; j++) {
      gint diff;

      diff = gst_rtp_buffer_compare_seqnum (seqnum, nacks[j]);
      GST_TRACE ("[%u][%u] %u %u diff %i", i, j, seqnum, nacks[j], diff);
      if (diff > 16)
        break;

      blp |= 1 << (diff - 1);
      nacked_seqnums++;
    }

    GST_WRITE_UINT32_BE (fci_data, seqnum << 16 | blp);
    fci_data += 4;
  }

  GST_DEBUG ("Sent %u seqnums into %u FB NACKs", nacked_seqnums, n_fb_nacks);
  source->stats.sent_nack_count += n_fb_nacks;

done:
  data->nacked_seqnums += nacked_seqnums;
  rtp_source_clear_nacks (source, nacked_seqnums);
  data->may_suppress = FALSE;
}

/* perform cleanup of sources that timed out */
static void
session_cleanup (const gchar * key, RTPSource * source, ReportData * data)
{
  gboolean remove = FALSE;
  gboolean byetimeout = FALSE;
  gboolean sendertimeout = FALSE;
  gboolean is_sender, is_active;
  RTPSession *sess = data->sess;
  GstClockTime interval, binterval;
  GstClockTime btime;

  GST_DEBUG ("look at %08x, generation %u", source->ssrc, source->generation);

  /* check for outdated collisions */
  if (source->internal) {
    GST_DEBUG ("Timing out collisions for %x", source->ssrc);
    rtp_source_timeout (source, data->current_time, data->running_time,
        sess->rtcp_feedback_retention_window);
  }

  /* nothing else to do when without RTCP */
  if (data->interval == GST_CLOCK_TIME_NONE)
    return;

  is_sender = RTP_SOURCE_IS_SENDER (source);
  is_active = RTP_SOURCE_IS_ACTIVE (source);

  /* our own rtcp interval may have been forced low by secondary configuration,
   * while sender side may still operate with higher interval,
   * so do not just take our interval to decide on timing out sender,
   * but take (if data->interval <= 5 * GST_SECOND):
   *   interval = CLAMP (sender_interval, data->interval, 5 * GST_SECOND)
   * where sender_interval is difference between last 2 received RTCP reports
   */
  if (data->interval >= 5 * GST_SECOND || source->internal) {
    binterval = data->interval;
  } else {
    GST_LOG ("prev_rtcp %" GST_TIME_FORMAT ", last_rtcp %" GST_TIME_FORMAT,
        GST_TIME_ARGS (source->stats.prev_rtcptime),
        GST_TIME_ARGS (source->stats.last_rtcptime));
    /* if not received enough yet, fallback to larger default */
    if (source->stats.last_rtcptime > source->stats.prev_rtcptime)
      binterval = source->stats.last_rtcptime - source->stats.prev_rtcptime;
    else
      binterval = 5 * GST_SECOND;
    binterval = CLAMP (binterval, data->interval, 5 * GST_SECOND);
  }
  GST_LOG ("timeout base interval %" GST_TIME_FORMAT,
      GST_TIME_ARGS (binterval));

  if (!source->internal && source->marked_bye) {
    /* if we received a BYE from the source, remove the source after some
     * time. */
    if (data->current_time > source->bye_time &&
        data->current_time - source->bye_time > sess->stats.bye_timeout) {
      GST_DEBUG ("removing BYE source %08x", source->ssrc);
      remove = TRUE;
      byetimeout = TRUE;
    }
  }

  if (source->internal && source->sent_bye) {
    GST_DEBUG ("removing internal source that has sent BYE %08x", source->ssrc);
    remove = TRUE;
  }

  /* sources that were inactive for more than 5 times the deterministic reporting
   * interval get timed out. the min timeout is 5 seconds. */
  /* mind old time that might pre-date last time going to PLAYING */
  btime = MAX (source->last_activity, sess->start_time);
  if (data->current_time > btime) {
    interval = MAX (binterval * 5, 5 * GST_SECOND);
    if (data->current_time - btime > interval) {
      GST_DEBUG ("removing timeout source %08x, last %" GST_TIME_FORMAT,
          source->ssrc, GST_TIME_ARGS (btime));
      if (source->internal) {
        /* this is an internal source that is not using our suggested ssrc.
         * since there must be another source using this ssrc, we can remove
         * this one instead of making it a receiver forever */
        if (source->ssrc != sess->suggested_ssrc) {
          rtp_source_mark_bye (source, "timed out");
          /* do not schedule bye here, since we are inside the RTCP timeout
           * processing and scheduling bye will interfere with SR/RR sending */
        }
      } else {
        remove = TRUE;
      }
    }
  }

  /* senders that did not send for a long time become a receiver, this also
   * holds for our own sources. */
  if (is_sender) {
    /* mind old time that might pre-date last time going to PLAYING */
    btime = MAX (source->last_rtp_activity, sess->start_time);
    if (data->current_time > btime) {
      interval = MAX (binterval * 2, 5 * GST_SECOND);
      if (data->current_time - btime > interval) {
        GST_DEBUG ("sender source %08x timed out and became receiver, last %"
            GST_TIME_FORMAT, source->ssrc, GST_TIME_ARGS (btime));
        sendertimeout = TRUE;
      }
    }
  }

  if (remove) {
    sess->total_sources--;
    if (is_sender) {
      sess->stats.sender_sources--;
      if (source->internal)
        sess->stats.internal_sender_sources--;
    }
    if (is_active)
      sess->stats.active_sources--;

    if (source->internal)
      sess->stats.internal_sources--;

    if (byetimeout)
      on_bye_timeout (sess, source);
    else
      on_timeout (sess, source);
  } else {
    if (sendertimeout) {
      source->is_sender = FALSE;
      sess->stats.sender_sources--;
      if (source->internal)
        sess->stats.internal_sender_sources--;

      on_sender_timeout (sess, source);
    }
    /* count how many source to report in this generation */
    if (((gint16) (source->generation - sess->generation)) <= 0)
      data->num_to_report++;
  }
  source->closing = remove;
}

static void
session_sdes (RTPSession * sess, ReportData * data)
{
  GstRTCPPacket *packet = &data->packet;
  const GstStructure *sdes;
  gint i, n_fields;
  GstRTCPBuffer *rtcp = &data->rtcpbuf;

  /* add SDES packet */
  gst_rtcp_buffer_add_packet (rtcp, GST_RTCP_TYPE_SDES, packet);

  gst_rtcp_packet_sdes_add_item (packet, data->source->ssrc);

  sdes = rtp_source_get_sdes_struct (data->source);

  /* add all fields in the structure, the order is not important. */
  n_fields = gst_structure_n_fields (sdes);
  for (i = 0; i < n_fields; ++i) {
    const gchar *field;
    const gchar *value;
    GstRTCPSDESType type;

    field = gst_structure_nth_field_name (sdes, i);
    if (field == NULL)
      continue;
    value = gst_structure_get_string (sdes, field);
    if (value == NULL)
      continue;
    type = gst_rtcp_sdes_name_to_type (field);

    /* Early packets are minimal and only include the CNAME */
    if (data->is_early && type != GST_RTCP_SDES_CNAME)
      continue;

    if (type > GST_RTCP_SDES_END && type < GST_RTCP_SDES_PRIV) {
      gst_rtcp_packet_sdes_add_entry (packet, type, strlen (value),
          (const guint8 *) value);
    } else if (type == GST_RTCP_SDES_PRIV) {
      gsize prefix_len;
      gsize value_len;
      gsize data_len;
      guint8 data[256];

      /* don't accept entries that are too big */
      prefix_len = strlen (field);
      if (prefix_len > 255)
        continue;
      value_len = strlen (value);
      if (value_len > 255)
        continue;
      data_len = 1 + prefix_len + value_len;
      if (data_len > 255)
        continue;

      data[0] = prefix_len;
      memcpy (&data[1], field, prefix_len);
      memcpy (&data[1 + prefix_len], value, value_len);

      gst_rtcp_packet_sdes_add_entry (packet, type, data_len, data);
    }
  }

  data->has_sdes = TRUE;
}

/* schedule a BYE packet */
static void
make_source_bye (RTPSession * sess, RTPSource * source, ReportData * data)
{
  GstRTCPPacket *packet = &data->packet;
  GstRTCPBuffer *rtcp = &data->rtcpbuf;

  /* add SDES */
  session_sdes (sess, data);
  /* add a BYE packet */
  gst_rtcp_buffer_add_packet (rtcp, GST_RTCP_TYPE_BYE, packet);
  gst_rtcp_packet_bye_add_ssrc (packet, source->ssrc);
  if (source->bye_reason)
    gst_rtcp_packet_bye_set_reason (packet, source->bye_reason);

  /* we have a BYE packet now */
  source->sent_bye = TRUE;
}

static gboolean
is_rtcp_time (RTPSession * sess, GstClockTime current_time, ReportData * data)
{
  GstClockTime new_send_time;
  GstClockTime interval;
  RTPSessionStats *stats;

  if (sess->scheduled_bye)
    stats = &sess->bye_stats;
  else
    stats = &sess->stats;

  if (GST_CLOCK_TIME_IS_VALID (sess->next_early_rtcp_time))
    data->is_early = TRUE;
  else
    data->is_early = FALSE;

  if (data->is_early && sess->next_early_rtcp_time <= current_time) {
    GST_DEBUG ("early feedback %" GST_TIME_FORMAT " <= now %"
        GST_TIME_FORMAT, GST_TIME_ARGS (sess->next_early_rtcp_time),
        GST_TIME_ARGS (current_time));
  } else if (sess->next_rtcp_check_time == GST_CLOCK_TIME_NONE ||
      sess->next_rtcp_check_time > current_time) {
    GST_DEBUG ("no check time yet, next %" GST_TIME_FORMAT " > now %"
        GST_TIME_FORMAT, GST_TIME_ARGS (sess->next_rtcp_check_time),
        GST_TIME_ARGS (current_time));
    return FALSE;
  }

  /* take interval and add jitter */
  interval = data->interval;
  if (interval != GST_CLOCK_TIME_NONE)
    interval = rtp_stats_add_rtcp_jitter (stats, interval);

  if (sess->last_rtcp_check_time != GST_CLOCK_TIME_NONE) {
    /* perform forward reconsideration */
    if (interval != GST_CLOCK_TIME_NONE) {
      GstClockTime elapsed;

      /* get elapsed time since we last reported */
      elapsed = current_time - sess->last_rtcp_check_time;

      GST_DEBUG ("forward reconsideration %" GST_TIME_FORMAT ", elapsed %"
          GST_TIME_FORMAT, GST_TIME_ARGS (interval), GST_TIME_ARGS (elapsed));
      new_send_time = interval + sess->last_rtcp_check_time;
    } else {
      new_send_time = sess->last_rtcp_check_time;
    }
  } else {
    /* If this is the first RTCP packet, we can reconsider anything based
     * on the last RTCP send time because there was none.
     */
    g_warn_if_fail (!data->is_early);
    data->is_early = FALSE;
    new_send_time = current_time;
  }

  if (!data->is_early) {
    /* check if reconsideration */
    if (new_send_time == GST_CLOCK_TIME_NONE || current_time < new_send_time) {
      GST_DEBUG ("reconsider RTCP for %" GST_TIME_FORMAT,
          GST_TIME_ARGS (new_send_time));
      /* store new check time */
      sess->next_rtcp_check_time = new_send_time;
      sess->last_rtcp_interval = interval;
      return FALSE;
    }

    sess->last_rtcp_interval = interval;
    if ((sess->rtp_profile == GST_RTP_PROFILE_AVPF
            || sess->rtp_profile == GST_RTP_PROFILE_SAVPF)
        && interval != GST_CLOCK_TIME_NONE) {
      /* Apply the rules from RFC 4585 section 3.5.3 */
      if (stats->min_interval != 0 && !sess->first_rtcp) {
        GstClockTime T_rr_current_interval =
            g_random_double_range (0.5, 1.5) * stats->min_interval * GST_SECOND;

        if (T_rr_current_interval > interval) {
          GST_DEBUG ("Adjusting interval for t-rr-interval: %" GST_TIME_FORMAT
              " > %" GST_TIME_FORMAT, GST_TIME_ARGS (T_rr_current_interval),
              GST_TIME_ARGS (interval));
          interval = T_rr_current_interval;
        }
      }
    }
    sess->next_rtcp_check_time = current_time + interval;
  }


  GST_DEBUG ("can send RTCP now, next %" GST_TIME_FORMAT,
      GST_TIME_ARGS (sess->next_rtcp_check_time));

  return TRUE;
}

static void
clone_ssrcs_hashtable (gchar * key, RTPSource * source, GHashTable * hash_table)
{
  g_hash_table_insert (hash_table, key, g_object_ref (source));
}

static gboolean
remove_closing_sources (const gchar * key, RTPSource * source,
    ReportData * data)
{
  if (source->closing)
    return TRUE;

  if (source->send_fir)
    data->have_fir = TRUE;
  if (source->send_pli)
    data->have_pli = TRUE;
  if (source->send_nack)
    data->have_nack = TRUE;

  return FALSE;
}

static void
generate_rtcp (const gchar * key, RTPSource * source, ReportData * data)
{
  RTPSession *sess = data->sess;
  gboolean is_bye = FALSE;
  ReportOutput *output;

  /* only generate RTCP for active internal sources */
  if (!source->internal || source->sent_bye)
    return;

  /* ignore other sources when we do the timeout after a scheduled BYE */
  if (sess->scheduled_bye && !source->marked_bye)
    return;

  /* skip if RTCP is disabled */
  if (source->disable_rtcp) {
    GST_DEBUG ("source %08x has RTCP disabled", source->ssrc);
    return;
  }

  data->source = source;

  /* open packet */
  session_start_rtcp (sess, data);

  if (source->marked_bye) {
    /* send BYE */
    make_source_bye (sess, source, data);
    is_bye = TRUE;
  } else if (!data->is_early) {
    /* loop over all known sources and add report blocks. If we are early, we
     * just make a minimal RTCP packet and skip this step */
    g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
        (GHFunc) session_report_blocks, data);
  }
  if (!data->has_sdes && (!data->is_early || !sess->reduced_size_rtcp))
    session_sdes (sess, data);

  if (data->have_fir)
    session_fir (sess, data);

  if (data->have_pli)
    g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
        (GHFunc) session_pli, data);

  if (data->have_nack)
    g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
        (GHFunc) session_nack, data);

  gst_rtcp_buffer_unmap (&data->rtcpbuf);

  output = g_slice_new (ReportOutput);
  output->source = g_object_ref (source);
  output->is_bye = is_bye;
  output->buffer = data->rtcp;
  /* queue the RTCP packet to push later */
  g_queue_push_tail (&data->output, output);
}

static void
update_generation (const gchar * key, RTPSource * source, ReportData * data)
{
  RTPSession *sess = data->sess;

  if (g_hash_table_size (source->reported_in_sr_of) >=
      sess->stats.internal_sources) {
    /* source is reported, move to next generation */
    source->generation = sess->generation + 1;
    g_hash_table_remove_all (source->reported_in_sr_of);

    GST_LOG ("reported source %x, new generation: %d", source->ssrc,
        source->generation);

    /* if we reported all sources in this generation, move to next */
    if (--data->num_to_report == 0) {
      sess->generation++;
      GST_DEBUG ("all reported, generation now %u", sess->generation);
    }
  }
}

static void
schedule_remaining_nacks (const gchar * key, RTPSource * source,
    ReportData * data)
{
  RTPSession *sess = data->sess;
  GstClockTime *nack_deadlines;
  GstClockTime deadline;
  guint n_nacks;

  if (!source->send_nack)
    return;

  /* the scheduling is entirely based on available bandwidth, just take the
   * biggest seqnum, which will have the largest deadline to request early
   * RTCP. */
  nack_deadlines = rtp_source_get_nack_deadlines (source, &n_nacks);
  deadline = nack_deadlines[n_nacks - 1];
  RTP_SESSION_UNLOCK (sess);
  rtp_session_send_rtcp_with_deadline (sess, deadline);
  RTP_SESSION_LOCK (sess);
}

static gboolean
rtp_session_are_all_sources_bye (RTPSession * sess)
{
  GHashTableIter iter;
  RTPSource *src;

  RTP_SESSION_LOCK (sess);
  g_hash_table_iter_init (&iter, sess->ssrcs[sess->mask_idx]);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) & src)) {
    if (src->internal && !src->sent_bye) {
      RTP_SESSION_UNLOCK (sess);
      return FALSE;
    }
  }
  RTP_SESSION_UNLOCK (sess);

  return TRUE;
}

/**
 * rtp_session_on_timeout:
 * @sess: an #RTPSession
 * @current_time: the current system time
 * @ntpnstime: the current NTP time in nanoseconds
 * @running_time: the current running_time of the pipeline
 *
 * Perform maintenance actions after the timeout obtained with
 * rtp_session_next_timeout() expired.
 *
 * This function will perform timeouts of receivers and senders, send a BYE
 * packet or generate RTCP packets with current session stats.
 *
 * This function can call the #RTPSessionSendRTCP callback, possibly multiple
 * times, for each packet that should be processed.
 *
 * Returns: a #GstFlowReturn.
 */
GstFlowReturn
rtp_session_on_timeout (RTPSession * sess, GstClockTime current_time,
    guint64 ntpnstime, GstClockTime running_time)
{
  GstFlowReturn result = GST_FLOW_OK;
  ReportData data = { GST_RTCP_BUFFER_INIT };
  GHashTable *table_copy;
  ReportOutput *output;
  gboolean all_empty = FALSE;

  g_return_val_if_fail (RTP_IS_SESSION (sess), GST_FLOW_ERROR);

  GST_DEBUG ("reporting at %" GST_TIME_FORMAT ", NTP time %" GST_TIME_FORMAT
      ", running-time %" GST_TIME_FORMAT, GST_TIME_ARGS (current_time),
      GST_TIME_ARGS (ntpnstime), GST_TIME_ARGS (running_time));

  data.sess = sess;
  data.current_time = current_time;
  data.ntpnstime = ntpnstime;
  data.running_time = running_time;
  data.num_to_report = 0;
  data.may_suppress = FALSE;
  data.nacked_seqnums = 0;
  g_queue_init (&data.output);

  RTP_SESSION_LOCK (sess);
  /* get a new interval, we need this for various cleanups etc */
  data.interval = calculate_rtcp_interval (sess, TRUE, sess->first_rtcp);

  GST_DEBUG ("interval %" GST_TIME_FORMAT, GST_TIME_ARGS (data.interval));

  /* we need an internal source now */
  if (sess->stats.internal_sources == 0) {
    RTPSource *source;
    gboolean created;

    source = obtain_internal_source (sess, sess->suggested_ssrc, &created,
        current_time);
    sess->internal_ssrc_set = TRUE;

    if (created)
      on_new_sender_ssrc (sess, source);

    g_object_unref (source);
  }

  sess->conflicting_addresses =
      timeout_conflicting_addresses (sess->conflicting_addresses, current_time);

  /* Make a local copy of the hashtable. We need to do this because the
   * cleanup stage below releases the session lock. */
  table_copy = g_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) g_object_unref);
  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) clone_ssrcs_hashtable, table_copy);

  /* Clean up the session, mark the source for removing, this might release the
   * session lock. */
  g_hash_table_foreach (table_copy, (GHFunc) session_cleanup, &data);
  g_hash_table_destroy (table_copy);

  /* Now remove the marked sources */
  g_hash_table_foreach_remove (sess->ssrcs[sess->mask_idx],
      (GHRFunc) remove_closing_sources, &data);

  /* update point-to-point status */
  session_update_ptp (sess);

  /* see if we need to generate SR or RR packets */
  if (!is_rtcp_time (sess, current_time, &data))
    goto done;

  /* check if all the buffers are empty after generation */
  all_empty = TRUE;

  GST_DEBUG
      ("doing RTCP generation %u for %u sources, early %d, may suppress %d",
      sess->generation, data.num_to_report, data.is_early, data.may_suppress);

  /* generate RTCP for all internal sources */
  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) generate_rtcp, &data);

  /* update the generation for all the sources that have been reported */
  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) update_generation, &data);

  /* we keep track of the last report time in order to timeout inactive
   * receivers or senders */
  if (!data.is_early) {
    GST_DEBUG ("Time since last regular RTCP: %" GST_TIME_FORMAT " - %"
        GST_TIME_FORMAT " = %" GST_TIME_FORMAT,
        GST_TIME_ARGS (data.current_time),
        GST_TIME_ARGS (sess->last_rtcp_send_time),
        GST_TIME_ARGS (data.current_time - sess->last_rtcp_send_time));
    sess->last_rtcp_send_time = data.current_time;
  }

  GST_DEBUG ("Time since last RTCP: %" GST_TIME_FORMAT " - %" GST_TIME_FORMAT
      " = %" GST_TIME_FORMAT, GST_TIME_ARGS (data.current_time),
      GST_TIME_ARGS (sess->last_rtcp_check_time),
      GST_TIME_ARGS (data.current_time - sess->last_rtcp_check_time));
  sess->last_rtcp_check_time = data.current_time;
  sess->first_rtcp = FALSE;
  sess->next_early_rtcp_time = GST_CLOCK_TIME_NONE;
  sess->scheduled_bye = FALSE;

done:
  RTP_SESSION_UNLOCK (sess);

  /* notify about updated statistics */
  g_object_notify (G_OBJECT (sess), "stats");

  /* push out the RTCP packets */
  while ((output = g_queue_pop_head (&data.output))) {
    gboolean do_not_suppress, empty_buffer;
    GstBuffer *buffer = output->buffer;
    RTPSource *source = output->source;

    /* Give the user a change to add its own packet */
    g_signal_emit (sess, rtp_session_signals[SIGNAL_ON_SENDING_RTCP], 0,
        buffer, data.is_early, &do_not_suppress);

    empty_buffer = gst_buffer_get_size (buffer) == 0;

    if (!empty_buffer)
      all_empty = FALSE;

    if (sess->callbacks.send_rtcp &&
        !empty_buffer && (do_not_suppress || !data.may_suppress)) {
      guint packet_size;

      packet_size = gst_buffer_get_size (buffer) + sess->header_len;

      UPDATE_AVG (sess->stats.avg_rtcp_packet_size, packet_size);
      GST_DEBUG ("%p, sending RTCP packet, avg size %u, %u", &sess->stats,
          sess->stats.avg_rtcp_packet_size, packet_size);
      result =
          sess->callbacks.send_rtcp (sess, source, buffer,
          rtp_session_are_all_sources_bye (sess), sess->send_rtcp_user_data);

      RTP_SESSION_LOCK (sess);
      sess->stats.nacks_sent += data.nacked_seqnums;
      on_sender_ssrc_active (sess, source);
      RTP_SESSION_UNLOCK (sess);
    } else {
      GST_DEBUG ("freeing packet callback: %p"
          " empty_buffer: %d, "
          " do_not_suppress: %d may_suppress: %d", sess->callbacks.send_rtcp,
          empty_buffer, do_not_suppress, data.may_suppress);
      if (!empty_buffer) {
        RTP_SESSION_LOCK (sess);
        sess->stats.nacks_dropped += data.nacked_seqnums;
        RTP_SESSION_UNLOCK (sess);
      }
      gst_buffer_unref (buffer);
    }
    g_object_unref (source);
    g_slice_free (ReportOutput, output);
  }

  if (all_empty)
    GST_ERROR ("generated empty RTCP messages for all the sources");

  /* schedule remaining nacks */
  RTP_SESSION_LOCK (sess);
  g_hash_table_foreach (sess->ssrcs[sess->mask_idx],
      (GHFunc) schedule_remaining_nacks, &data);
  RTP_SESSION_UNLOCK (sess);

  return result;
}

/**
 * rtp_session_request_early_rtcp:
 * @sess: an #RTPSession
 * @current_time: the current system time
 * @max_delay: maximum delay
 *
 * Request transmission of early RTCP
 *
 * Returns: %TRUE if the related RTCP can be scheduled.
 */
gboolean
rtp_session_request_early_rtcp (RTPSession * sess, GstClockTime current_time,
    GstClockTime max_delay)
{
  GstClockTime T_dither_max, T_rr, offset = 0;
  gboolean ret;
  gboolean allow_early;

  /* Implements the algorithm described in RFC 4585 section 3.5.2 */

  RTP_SESSION_LOCK (sess);

  /* We assume a feedback profile if something is requesting RTCP
   * to be sent */
  sess->rtp_profile = GST_RTP_PROFILE_AVPF;

  /* Check if already requested */
  /*  RFC 4585 section 3.5.2 step 2 */
  if (GST_CLOCK_TIME_IS_VALID (sess->next_early_rtcp_time)) {
    GST_LOG_OBJECT (sess, "already have next early rtcp time");
    ret = (current_time + max_delay > sess->next_early_rtcp_time);
    goto end;
  }

  if (!GST_CLOCK_TIME_IS_VALID (sess->next_rtcp_check_time)) {
    GST_LOG_OBJECT (sess, "no next RTCP check time");
    ret = FALSE;
    goto end;
  }

  /* RFC 4585 section 3.5.3 step 1
   * If no regular RTCP packet has been sent before, then a regular
   * RTCP packet has to be scheduled first and FB messages might be
   * included there
   */
  if (!GST_CLOCK_TIME_IS_VALID (sess->last_rtcp_send_time)) {
    GST_LOG_OBJECT (sess, "no RTCP sent yet");

    if (current_time + max_delay > sess->next_rtcp_check_time) {
      GST_LOG_OBJECT (sess,
          "next scheduled time is soon %" GST_TIME_FORMAT " + %" GST_TIME_FORMAT
          " > %" GST_TIME_FORMAT, GST_TIME_ARGS (current_time),
          GST_TIME_ARGS (max_delay),
          GST_TIME_ARGS (sess->next_rtcp_check_time));
      ret = TRUE;
    } else {
      GST_LOG_OBJECT (sess,
          "can't allow early feedback, next scheduled time is too late %"
          GST_TIME_FORMAT " + %" GST_TIME_FORMAT " < %" GST_TIME_FORMAT,
          GST_TIME_ARGS (current_time), GST_TIME_ARGS (max_delay),
          GST_TIME_ARGS (sess->next_rtcp_check_time));
      ret = FALSE;
    }
    goto end;
  }

  T_rr = sess->last_rtcp_interval;

  /*  RFC 4585 section 3.5.2 step 2b */
  /* If the total sources is <=2, then there is only us and one peer */
  /* When there is one auxiliary stream the session can still do point
   * to point.
   */
  if (sess->is_doing_ptp) {
    T_dither_max = 0;
  } else {
    /* Divide by 2 because l = 0.5 */
    T_dither_max = T_rr;
    T_dither_max /= 2;
  }

  /*  RFC 4585 section 3.5.2 step 3 */
  if (current_time + T_dither_max > sess->next_rtcp_check_time) {
    GST_LOG_OBJECT (sess,
        "don't send because of dither, next scheduled time is too soon %"
        GST_TIME_FORMAT " + %" GST_TIME_FORMAT " > %" GST_TIME_FORMAT,
        GST_TIME_ARGS (current_time), GST_TIME_ARGS (T_dither_max),
        GST_TIME_ARGS (sess->next_rtcp_check_time));
    ret = T_dither_max <= max_delay;
    goto end;
  }

  /*  RFC 4585 section 3.5.2 step 4a and
   *  RFC 4585 section 3.5.2 step 6 */
  allow_early = FALSE;
  if (sess->last_rtcp_check_time == sess->last_rtcp_send_time) {
    /* Last time we sent a full RTCP packet, we can now immediately
     * send an early one as allow_early was reset to TRUE */
    allow_early = TRUE;
  } else if (sess->last_rtcp_check_time + T_rr <= current_time + max_delay) {
    /* Last packet we sent was an early RTCP packet and more than
     * T_rr has passed since then, meaning we would have suppressed
     * a regular RTCP packet already and reset allow_early to TRUE */
    allow_early = TRUE;

    /* We have to offset a bit as T_rr has not passed yet, but will before
     * max_delay */
    if (sess->last_rtcp_check_time + T_rr > current_time)
      offset = (sess->last_rtcp_check_time + T_rr) - current_time;
  } else {
    GST_DEBUG_OBJECT (sess,
        "can't allow early RTCP yet: last regular %" GST_TIME_FORMAT ", %"
        GST_TIME_FORMAT " + %" GST_TIME_FORMAT " > %" GST_TIME_FORMAT " + %"
        GST_TIME_FORMAT, GST_TIME_ARGS (sess->last_rtcp_send_time),
        GST_TIME_ARGS (sess->last_rtcp_check_time), GST_TIME_ARGS (T_rr),
        GST_TIME_ARGS (current_time), GST_TIME_ARGS (max_delay));
  }

  if (!allow_early) {
    /* Ignore the request a scheduled packet will be in time anyway */
    if (current_time + max_delay > sess->next_rtcp_check_time) {
      GST_LOG_OBJECT (sess,
          "next scheduled time is soon %" GST_TIME_FORMAT " + %" GST_TIME_FORMAT
          " > %" GST_TIME_FORMAT, GST_TIME_ARGS (current_time),
          GST_TIME_ARGS (max_delay),
          GST_TIME_ARGS (sess->next_rtcp_check_time));
      ret = TRUE;
    } else {
      GST_LOG_OBJECT (sess,
          "can't allow early feedback and next scheduled time is too late %"
          GST_TIME_FORMAT " + %" GST_TIME_FORMAT " < %" GST_TIME_FORMAT,
          GST_TIME_ARGS (current_time), GST_TIME_ARGS (max_delay),
          GST_TIME_ARGS (sess->next_rtcp_check_time));
      ret = FALSE;
    }
    goto end;
  }

  /*  RFC 4585 section 3.5.2 step 4b */
  if (T_dither_max) {
    /* Schedule an early transmission later */
    sess->next_early_rtcp_time = g_random_double () * T_dither_max +
        current_time + offset;
  } else {
    /* If no dithering, schedule it for NOW */
    sess->next_early_rtcp_time = current_time + offset;
  }

  GST_LOG_OBJECT (sess, "next early RTCP time %" GST_TIME_FORMAT
      ", next regular RTCP time %" GST_TIME_FORMAT,
      GST_TIME_ARGS (sess->next_early_rtcp_time),
      GST_TIME_ARGS (sess->next_rtcp_check_time));
  RTP_SESSION_UNLOCK (sess);

  /* notify app of need to send packet early
   * and therefore of timeout change */
  if (sess->callbacks.reconsider)
    sess->callbacks.reconsider (sess, sess->reconsider_user_data);

  return TRUE;

end:

  RTP_SESSION_UNLOCK (sess);

  return ret;
}

static gboolean
rtp_session_send_rtcp_internal (RTPSession * sess, GstClockTime now,
    GstClockTime max_delay)
{
  /* notify the application that we intend to send early RTCP */
  if (sess->callbacks.notify_early_rtcp)
    sess->callbacks.notify_early_rtcp (sess, sess->notify_early_rtcp_user_data);

  return rtp_session_request_early_rtcp (sess, now, max_delay);
}

static gboolean
rtp_session_send_rtcp_with_deadline (RTPSession * sess, GstClockTime deadline)
{
  GstClockTime now, max_delay;

  if (!sess->callbacks.send_rtcp)
    return FALSE;

  now = sess->callbacks.request_time (sess, sess->request_time_user_data);

  if (deadline < now)
    return FALSE;

  max_delay = deadline - now;

  return rtp_session_send_rtcp_internal (sess, now, max_delay);
}

static gboolean
rtp_session_send_rtcp (RTPSession * sess, GstClockTime max_delay)
{
  GstClockTime now;

  if (!sess->callbacks.send_rtcp)
    return FALSE;

  now = sess->callbacks.request_time (sess, sess->request_time_user_data);

  return rtp_session_send_rtcp_internal (sess, now, max_delay);
}

gboolean
rtp_session_request_key_unit (RTPSession * sess, guint32 ssrc,
    gboolean fir, gint count)
{
  RTPSource *src;

  RTP_SESSION_LOCK (sess);
  src = find_source (sess, ssrc);
  if (src == NULL)
    goto no_source;

  if (fir) {
    src->send_pli = FALSE;
    src->send_fir = TRUE;

    if (count == -1 || count != src->last_fir_count)
      src->current_send_fir_seqnum++;
    src->last_fir_count = count;
  } else if (!src->send_fir) {
    src->send_pli = TRUE;
  }
  RTP_SESSION_UNLOCK (sess);

  if (!rtp_session_send_rtcp (sess, 5 * GST_SECOND)) {
    GST_DEBUG ("FIR/PLI not sent early, sending with next regular RTCP");
  }

  return TRUE;

  /* ERRORS */
no_source:
  {
    RTP_SESSION_UNLOCK (sess);
    return FALSE;
  }
}

/**
 * rtp_session_request_nack:
 * @sess: a #RTPSession
 * @ssrc: the SSRC
 * @seqnum: the missing seqnum
 * @max_delay: max delay to request NACK
 *
 * Request scheduling of a NACK feedback packet for @seqnum in @ssrc.
 *
 * Returns: %TRUE if the NACK feedback could be scheduled
 */
gboolean
rtp_session_request_nack (RTPSession * sess, guint32 ssrc, guint16 seqnum,
    GstClockTime max_delay)
{
  RTPSource *source;
  GstClockTime now;

  if (!sess->callbacks.send_rtcp)
    return FALSE;

  now = sess->callbacks.request_time (sess, sess->request_time_user_data);

  RTP_SESSION_LOCK (sess);
  source = find_source (sess, ssrc);
  if (source == NULL)
    goto no_source;

  GST_DEBUG ("request NACK for SSRC %08x, #%u, deadline %" GST_TIME_FORMAT,
      ssrc, seqnum, GST_TIME_ARGS (now + max_delay));
  rtp_source_register_nack (source, seqnum, now + max_delay);
  RTP_SESSION_UNLOCK (sess);

  if (!rtp_session_send_rtcp_internal (sess, now, max_delay)) {
    GST_DEBUG ("NACK not sent early, sending with next regular RTCP");
  }

  return TRUE;

  /* ERRORS */
no_source:
  {
    RTP_SESSION_UNLOCK (sess);
    return FALSE;
  }
}
