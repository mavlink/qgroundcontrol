/* GStreamer
 * Copyright (C) <2005,2006> Wim Taymans <wim at fluendo dot com>
 *               <2006> Lutz Mueller <lutz at topfrose dot de>
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
/*
 * Unless otherwise indicated, Source Code is licensed under MIT license.
 * See further explanation attached in License Statement (distributed in the file
 * LICENSE).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/**
 * SECTION:element-rtspsrc
 * @title: rtspsrc
 *
 * Makes a connection to an RTSP server and read the data.
 * rtspsrc strictly follows RFC 2326 and therefore does not (yet) support
 * RealMedia/Quicktime/Microsoft extensions.
 *
 * RTSP supports transport over TCP or UDP in unicast or multicast mode. By
 * default rtspsrc will negotiate a connection in the following order:
 * UDP unicast/UDP multicast/TCP. The order cannot be changed but the allowed
 * protocols can be controlled with the #GstRTSPSrc:protocols property.
 *
 * rtspsrc currently understands SDP as the format of the session description.
 * For each stream listed in the SDP a new rtp_stream\%d pad will be created
 * with caps derived from the SDP media description. This is a caps of mime type
 * "application/x-rtp" that can be connected to any available RTP depayloader
 * element.
 *
 * rtspsrc will internally instantiate an RTP session manager element
 * that will handle the RTCP messages to and from the server, jitter removal,
 * packet reordering along with providing a clock for the pipeline.
 * This feature is implemented using the gstrtpbin element.
 *
 * rtspsrc acts like a live source and will therefore only generate data in the
 * PLAYING state.
 *
 * If a RTP session times out then the rtspsrc will generate an element message
 * named "GstRTSPSrcTimeout". Currently this is only supported for timeouts
 * triggered by RTCP.
 *
 * The message's structure contains three fields:
 *
 *   GstRTSPSrcTimeoutCause `cause`: the cause of the timeout.
 *
 *   #gint `stream-number`: an internal identifier of the stream that timed out.
 *
 *   #guint `ssrc`: the SSRC of the stream that timed out.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 rtspsrc location=rtsp://some.server/url ! fakesink
 * ]| Establish a connection to an RTSP server and send the raw RTP packets to a
 * fakesink.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <gst/net/gstnet.h>
#include <gst/sdp/gstsdpmessage.h>
#include <gst/sdp/gstmikey.h>
#include <gst/rtp/rtp.h>

#include "gst/gst-i18n-plugin.h"

#include "gstrtspsrc.h"

GST_DEBUG_CATEGORY_STATIC (rtspsrc_debug);
#define GST_CAT_DEFAULT (rtspsrc_debug)

static GstStaticPadTemplate rtptemplate = GST_STATIC_PAD_TEMPLATE ("stream_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtp; application/x-rdt"));

/* templates used internally */
static GstStaticPadTemplate anysrctemplate =
GST_STATIC_PAD_TEMPLATE ("internalsrc_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate anysinktemplate =
GST_STATIC_PAD_TEMPLATE ("internalsink_%u",
    GST_PAD_SINK,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

enum
{
  SIGNAL_HANDLE_REQUEST,
  SIGNAL_ON_SDP,
  SIGNAL_SELECT_STREAM,
  SIGNAL_NEW_MANAGER,
  SIGNAL_REQUEST_RTCP_KEY,
  SIGNAL_ACCEPT_CERTIFICATE,
  SIGNAL_BEFORE_SEND,
  SIGNAL_PUSH_BACKCHANNEL_BUFFER,
  SIGNAL_GET_PARAMETER,
  SIGNAL_GET_PARAMETERS,
  SIGNAL_SET_PARAMETER,
  LAST_SIGNAL
};

enum _GstRtspSrcRtcpSyncMode
{
  RTCP_SYNC_ALWAYS,
  RTCP_SYNC_INITIAL,
  RTCP_SYNC_RTP
};

enum _GstRtspSrcBufferMode
{
  BUFFER_MODE_NONE,
  BUFFER_MODE_SLAVE,
  BUFFER_MODE_BUFFER,
  BUFFER_MODE_AUTO,
  BUFFER_MODE_SYNCED
};

#define GST_TYPE_RTSP_SRC_BUFFER_MODE (gst_rtsp_src_buffer_mode_get_type())
static GType
gst_rtsp_src_buffer_mode_get_type (void)
{
  static GType buffer_mode_type = 0;
  static const GEnumValue buffer_modes[] = {
    {BUFFER_MODE_NONE, "Only use RTP timestamps", "none"},
    {BUFFER_MODE_SLAVE, "Slave receiver to sender clock", "slave"},
    {BUFFER_MODE_BUFFER, "Do low/high watermark buffering", "buffer"},
    {BUFFER_MODE_AUTO, "Choose mode depending on stream live", "auto"},
    {BUFFER_MODE_SYNCED, "Synchronized sender and receiver clocks", "synced"},
    {0, NULL, NULL},
  };

  if (!buffer_mode_type) {
    buffer_mode_type =
        g_enum_register_static ("GstRTSPSrcBufferMode", buffer_modes);
  }
  return buffer_mode_type;
}

enum _GstRtspSrcNtpTimeSource
{
  NTP_TIME_SOURCE_NTP,
  NTP_TIME_SOURCE_UNIX,
  NTP_TIME_SOURCE_RUNNING_TIME,
  NTP_TIME_SOURCE_CLOCK_TIME
};

#define DEBUG_RTSP(__self,msg) gst_rtspsrc_print_rtsp_message (__self, msg)
#define DEBUG_SDP(__self,msg) gst_rtspsrc_print_sdp_message (__self, msg)

#define GST_TYPE_RTSP_SRC_NTP_TIME_SOURCE (gst_rtsp_src_ntp_time_source_get_type())
static GType
gst_rtsp_src_ntp_time_source_get_type (void)
{
  static GType ntp_time_source_type = 0;
  static const GEnumValue ntp_time_source_values[] = {
    {NTP_TIME_SOURCE_NTP, "NTP time based on realtime clock", "ntp"},
    {NTP_TIME_SOURCE_UNIX, "UNIX time based on realtime clock", "unix"},
    {NTP_TIME_SOURCE_RUNNING_TIME,
          "Running time based on pipeline clock",
        "running-time"},
    {NTP_TIME_SOURCE_CLOCK_TIME, "Pipeline clock time", "clock-time"},
    {0, NULL, NULL},
  };

  if (!ntp_time_source_type) {
    ntp_time_source_type =
        g_enum_register_static ("GstRTSPSrcNtpTimeSource",
        ntp_time_source_values);
  }
  return ntp_time_source_type;
}

enum _GstRtspBackchannel
{
  BACKCHANNEL_NONE,
  BACKCHANNEL_ONVIF
};

#define GST_TYPE_RTSP_BACKCHANNEL (gst_rtsp_backchannel_get_type())
static GType
gst_rtsp_backchannel_get_type (void)
{
  static GType backchannel_type = 0;
  static const GEnumValue backchannel_values[] = {
    {BACKCHANNEL_NONE, "No backchannel", "none"},
    {BACKCHANNEL_ONVIF, "ONVIF audio backchannel", "onvif"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (backchannel_type == 0)) {
    backchannel_type =
        g_enum_register_static ("GstRTSPBackchannel", backchannel_values);
  }
  return backchannel_type;
}

#define BACKCHANNEL_ONVIF_HDR_REQUIRE_VAL "www.onvif.org/ver20/backchannel"

#define DEFAULT_LOCATION         NULL
#define DEFAULT_PROTOCOLS        GST_RTSP_LOWER_TRANS_UDP | GST_RTSP_LOWER_TRANS_UDP_MCAST | GST_RTSP_LOWER_TRANS_TCP
#define DEFAULT_DEBUG            FALSE
#define DEFAULT_RETRY            20
#define DEFAULT_TIMEOUT          5000000
#define DEFAULT_UDP_BUFFER_SIZE  0x80000
#define DEFAULT_TCP_TIMEOUT      20000000
#define DEFAULT_LATENCY_MS       2000
#define DEFAULT_DROP_ON_LATENCY  FALSE
#define DEFAULT_CONNECTION_SPEED 0
#define DEFAULT_NAT_METHOD       GST_RTSP_NAT_DUMMY
#define DEFAULT_DO_RTCP          TRUE
#define DEFAULT_DO_RTSP_KEEP_ALIVE       TRUE
#define DEFAULT_PROXY            NULL
#define DEFAULT_RTP_BLOCKSIZE    0
#define DEFAULT_USER_ID          NULL
#define DEFAULT_USER_PW          NULL
#define DEFAULT_BUFFER_MODE      BUFFER_MODE_AUTO
#define DEFAULT_PORT_RANGE       NULL
#define DEFAULT_SHORT_HEADER     FALSE
#define DEFAULT_PROBATION        2
#define DEFAULT_UDP_RECONNECT    TRUE
#define DEFAULT_MULTICAST_IFACE  NULL
#define DEFAULT_NTP_SYNC         FALSE
#define DEFAULT_USE_PIPELINE_CLOCK       FALSE
#define DEFAULT_TLS_VALIDATION_FLAGS     G_TLS_CERTIFICATE_VALIDATE_ALL
#define DEFAULT_TLS_DATABASE     NULL
#define DEFAULT_TLS_INTERACTION     NULL
#define DEFAULT_DO_RETRANSMISSION        TRUE
#define DEFAULT_NTP_TIME_SOURCE  NTP_TIME_SOURCE_NTP
#define DEFAULT_USER_AGENT       "GStreamer/" PACKAGE_VERSION
#define DEFAULT_MAX_RTCP_RTP_TIME_DIFF 1000
#define DEFAULT_RFC7273_SYNC         FALSE
#define DEFAULT_MAX_TS_OFFSET_ADJUSTMENT   G_GUINT64_CONSTANT(0)
#define DEFAULT_MAX_TS_OFFSET   G_GINT64_CONSTANT(3000000000)
#define DEFAULT_VERSION         GST_RTSP_VERSION_1_0
#define DEFAULT_BACKCHANNEL  GST_RTSP_BACKCHANNEL_NONE
#define DEFAULT_TEARDOWN_TIMEOUT  (100 * GST_MSECOND)
#define DEFAULT_ONVIF_MODE FALSE
#define DEFAULT_ONVIF_RATE_CONTROL TRUE
#define DEFAULT_IS_LIVE TRUE

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_PROTOCOLS,
  PROP_DEBUG,
  PROP_RETRY,
  PROP_TIMEOUT,
  PROP_TCP_TIMEOUT,
  PROP_LATENCY,
  PROP_DROP_ON_LATENCY,
  PROP_CONNECTION_SPEED,
  PROP_NAT_METHOD,
  PROP_DO_RTCP,
  PROP_DO_RTSP_KEEP_ALIVE,
  PROP_PROXY,
  PROP_PROXY_ID,
  PROP_PROXY_PW,
  PROP_RTP_BLOCKSIZE,
  PROP_USER_ID,
  PROP_USER_PW,
  PROP_BUFFER_MODE,
  PROP_PORT_RANGE,
  PROP_UDP_BUFFER_SIZE,
  PROP_SHORT_HEADER,
  PROP_PROBATION,
  PROP_UDP_RECONNECT,
  PROP_MULTICAST_IFACE,
  PROP_NTP_SYNC,
  PROP_USE_PIPELINE_CLOCK,
  PROP_SDES,
  PROP_TLS_VALIDATION_FLAGS,
  PROP_TLS_DATABASE,
  PROP_TLS_INTERACTION,
  PROP_DO_RETRANSMISSION,
  PROP_NTP_TIME_SOURCE,
  PROP_USER_AGENT,
  PROP_MAX_RTCP_RTP_TIME_DIFF,
  PROP_RFC7273_SYNC,
  PROP_MAX_TS_OFFSET_ADJUSTMENT,
  PROP_MAX_TS_OFFSET,
  PROP_DEFAULT_VERSION,
  PROP_BACKCHANNEL,
  PROP_TEARDOWN_TIMEOUT,
  PROP_ONVIF_MODE,
  PROP_ONVIF_RATE_CONTROL,
  PROP_IS_LIVE
};

#define GST_TYPE_RTSP_NAT_METHOD (gst_rtsp_nat_method_get_type())
static GType
gst_rtsp_nat_method_get_type (void)
{
  static GType rtsp_nat_method_type = 0;
  static const GEnumValue rtsp_nat_method[] = {
    {GST_RTSP_NAT_NONE, "None", "none"},
    {GST_RTSP_NAT_DUMMY, "Send Dummy packets", "dummy"},
    {0, NULL, NULL},
  };

  if (!rtsp_nat_method_type) {
    rtsp_nat_method_type =
        g_enum_register_static ("GstRTSPNatMethod", rtsp_nat_method);
  }
  return rtsp_nat_method_type;
}

#define RTSP_SRC_RESPONSE_ERROR(src, response_msg, err_cat, err_code, error_message) \
  do { \
    GST_ELEMENT_ERROR_WITH_DETAILS((src), err_cat, err_code, ("%s", error_message), \
        ("%s (%d)", (response_msg)->type_data.response.reason, (response_msg)->type_data.response.code), \
        ("rtsp-status-code", G_TYPE_UINT, (response_msg)->type_data.response.code, \
         "rtsp-status-reason", G_TYPE_STRING, GST_STR_NULL((response_msg)->type_data.response.reason), NULL)); \
  } while (0)

typedef struct _ParameterRequest
{
  gint cmd;
  gchar *content_type;
  GString *body;
  GstPromise *promise;
} ParameterRequest;

static void gst_rtspsrc_finalize (GObject * object);

static void gst_rtspsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtspsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstClock *gst_rtspsrc_provide_clock (GstElement * element);

static void gst_rtspsrc_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

static gboolean gst_rtspsrc_set_proxy (GstRTSPSrc * rtsp, const gchar * proxy);
static void gst_rtspsrc_set_tcp_timeout (GstRTSPSrc * rtspsrc, guint64 timeout);

static GstStateChangeReturn gst_rtspsrc_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_rtspsrc_send_event (GstElement * element, GstEvent * event);
static void gst_rtspsrc_handle_message (GstBin * bin, GstMessage * message);

static gboolean gst_rtspsrc_setup_auth (GstRTSPSrc * src,
    GstRTSPMessage * response);

static gboolean gst_rtspsrc_loop_send_cmd (GstRTSPSrc * src, gint cmd,
    gint mask);
static GstRTSPResult gst_rtspsrc_send_cb (GstRTSPExtension * ext,
    GstRTSPMessage * request, GstRTSPMessage * response, GstRTSPSrc * src);

static GstRTSPResult gst_rtspsrc_open (GstRTSPSrc * src, gboolean async);
static GstRTSPResult gst_rtspsrc_play (GstRTSPSrc * src, GstSegment * segment,
    gboolean async, const gchar * seek_style);
static GstRTSPResult gst_rtspsrc_pause (GstRTSPSrc * src, gboolean async);
static GstRTSPResult gst_rtspsrc_close (GstRTSPSrc * src, gboolean async,
    gboolean only_close);

static gboolean gst_rtspsrc_uri_set_uri (GstURIHandler * handler,
    const gchar * uri, GError ** error);
static gchar *gst_rtspsrc_uri_get_uri (GstURIHandler * handler);

static gboolean gst_rtspsrc_activate_streams (GstRTSPSrc * src);
static gboolean gst_rtspsrc_loop (GstRTSPSrc * src);
static gboolean gst_rtspsrc_stream_push_event (GstRTSPSrc * src,
    GstRTSPStream * stream, GstEvent * event);
static gboolean gst_rtspsrc_push_event (GstRTSPSrc * src, GstEvent * event);
static void gst_rtspsrc_connection_flush (GstRTSPSrc * src, gboolean flush);
static GstRTSPResult gst_rtsp_conninfo_close (GstRTSPSrc * src,
    GstRTSPConnInfo * info, gboolean free);
static void
gst_rtspsrc_print_rtsp_message (GstRTSPSrc * src, const GstRTSPMessage * msg);
static void
gst_rtspsrc_print_sdp_message (GstRTSPSrc * src, const GstSDPMessage * msg);

static GstRTSPResult
gst_rtspsrc_get_parameter (GstRTSPSrc * src, ParameterRequest * req);

static GstRTSPResult
gst_rtspsrc_set_parameter (GstRTSPSrc * src, ParameterRequest * req);

static gboolean get_parameter (GstRTSPSrc * src, const gchar * parameter,
    const gchar * content_type, GstPromise * promise);

static gboolean get_parameters (GstRTSPSrc * src, gchar ** parameters,
    const gchar * content_type, GstPromise * promise);

static gboolean set_parameter (GstRTSPSrc * src, const gchar * name,
    const gchar * value, const gchar * content_type, GstPromise * promise);

static GstFlowReturn gst_rtspsrc_push_backchannel_buffer (GstRTSPSrc * src,
    guint id, GstSample * sample);

typedef struct
{
  guint8 pt;
  GstCaps *caps;
} PtMapItem;

/* commands we send to out loop to notify it of events */
#define CMD_OPEN            (1 << 0)
#define CMD_PLAY            (1 << 1)
#define CMD_PAUSE           (1 << 2)
#define CMD_CLOSE           (1 << 3)
#define CMD_WAIT            (1 << 4)
#define CMD_RECONNECT       (1 << 5)
#define CMD_LOOP            (1 << 6)
#define CMD_GET_PARAMETER   (1 << 7)
#define CMD_SET_PARAMETER   (1 << 8)

/* mask for all commands */
#define CMD_ALL         ((CMD_SET_PARAMETER << 1) - 1)

#define GST_ELEMENT_PROGRESS(el, type, code, text)      \
G_STMT_START {                                          \
  gchar *__txt = _gst_element_error_printf text;        \
  gst_element_post_message (GST_ELEMENT_CAST (el),      \
      gst_message_new_progress (GST_OBJECT_CAST (el),   \
          GST_PROGRESS_TYPE_ ##type, code, __txt));     \
  g_free (__txt);                                       \
} G_STMT_END

static guint gst_rtspsrc_signals[LAST_SIGNAL] = { 0 };

#define gst_rtspsrc_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstRTSPSrc, gst_rtspsrc, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER, gst_rtspsrc_uri_handler_init));

#ifndef GST_DISABLE_GST_DEBUG
static inline const char *
cmd_to_string (guint cmd)
{
  switch (cmd) {
    case CMD_OPEN:
      return "OPEN";
    case CMD_PLAY:
      return "PLAY";
    case CMD_PAUSE:
      return "PAUSE";
    case CMD_CLOSE:
      return "CLOSE";
    case CMD_WAIT:
      return "WAIT";
    case CMD_RECONNECT:
      return "RECONNECT";
    case CMD_LOOP:
      return "LOOP";
    case CMD_GET_PARAMETER:
      return "GET_PARAMETER";
    case CMD_SET_PARAMETER:
      return "SET_PARAMETER";
  }

  return "unknown";
}
#endif

static gboolean
default_select_stream (GstRTSPSrc * src, guint id, GstCaps * caps)
{
  GST_DEBUG_OBJECT (src, "default handler");
  return TRUE;
}

static gboolean
select_stream_accum (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer data)
{
  gboolean myboolean;

  myboolean = g_value_get_boolean (handler_return);
  GST_DEBUG ("accum %d", myboolean);
  g_value_set_boolean (return_accu, myboolean);

  /* stop emission if FALSE */
  return myboolean;
}

static gboolean
default_before_send (GstRTSPSrc * src, GstRTSPMessage * msg)
{
  GST_DEBUG_OBJECT (src, "default handler");
  return TRUE;
}

static gboolean
before_send_accum (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer data)
{
  gboolean myboolean;

  myboolean = g_value_get_boolean (handler_return);
  g_value_set_boolean (return_accu, myboolean);

  /* prevent send if FALSE */
  return myboolean;
}

static void
gst_rtspsrc_class_init (GstRTSPSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBinClass *gstbin_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbin_class = (GstBinClass *) klass;

  GST_DEBUG_CATEGORY_INIT (rtspsrc_debug, "rtspsrc", 0, "RTSP src");

  gobject_class->set_property = gst_rtspsrc_set_property;
  gobject_class->get_property = gst_rtspsrc_get_property;

  gobject_class->finalize = gst_rtspsrc_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "RTSP Location",
          "Location of the RTSP url to read",
          DEFAULT_LOCATION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROTOCOLS,
      g_param_spec_flags ("protocols", "Protocols",
          "Allowed lower transport protocols", GST_TYPE_RTSP_LOWER_TRANS,
          DEFAULT_PROTOCOLS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEBUG,
      g_param_spec_boolean ("debug", "Debug",
          "Dump request and response messages to stdout"
          "(DEPRECATED: Printed all RTSP message to gstreamer log as 'log' level)",
          DEFAULT_DEBUG,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));

  g_object_class_install_property (gobject_class, PROP_RETRY,
      g_param_spec_uint ("retry", "Retry",
          "Max number of retries when allocating RTP ports.",
          0, G_MAXUINT16, DEFAULT_RETRY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TIMEOUT,
      g_param_spec_uint64 ("timeout", "Timeout",
          "Retry TCP transport after UDP timeout microseconds (0 = disabled)",
          0, G_MAXUINT64, DEFAULT_TIMEOUT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TCP_TIMEOUT,
      g_param_spec_uint64 ("tcp-timeout", "TCP Timeout",
          "Fail after timeout microseconds on TCP connections (0 = disabled)",
          0, G_MAXUINT64, DEFAULT_TCP_TIMEOUT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LATENCY,
      g_param_spec_uint ("latency", "Buffer latency in ms",
          "Amount of ms to buffer", 0, G_MAXUINT, DEFAULT_LATENCY_MS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DROP_ON_LATENCY,
      g_param_spec_boolean ("drop-on-latency",
          "Drop buffers when maximum latency is reached",
          "Tells the jitterbuffer to never exceed the given latency in size",
          DEFAULT_DROP_ON_LATENCY, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CONNECTION_SPEED,
      g_param_spec_uint64 ("connection-speed", "Connection Speed",
          "Network connection speed in kbps (0 = unknown)",
          0, G_MAXUINT64 / 1000, DEFAULT_CONNECTION_SPEED,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NAT_METHOD,
      g_param_spec_enum ("nat-method", "NAT Method",
          "Method to use for traversing firewalls and NAT",
          GST_TYPE_RTSP_NAT_METHOD, DEFAULT_NAT_METHOD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:do-rtcp:
   *
   * Enable RTCP support. Some old server don't like RTCP and then this property
   * needs to be set to FALSE.
   */
  g_object_class_install_property (gobject_class, PROP_DO_RTCP,
      g_param_spec_boolean ("do-rtcp", "Do RTCP",
          "Send RTCP packets, disable for old incompatible server.",
          DEFAULT_DO_RTCP, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:do-rtsp-keep-alive:
   *
   * Enable RTSP keep alive support. Some old server don't like RTSP
   * keep alive and then this property needs to be set to FALSE.
   */
  g_object_class_install_property (gobject_class, PROP_DO_RTSP_KEEP_ALIVE,
      g_param_spec_boolean ("do-rtsp-keep-alive", "Do RTSP Keep Alive",
          "Send RTSP keep alive packets, disable for old incompatible server.",
          DEFAULT_DO_RTSP_KEEP_ALIVE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:proxy:
   *
   * Set the proxy parameters. This has to be a string of the format
   * [http://][user:passwd@]host[:port].
   */
  g_object_class_install_property (gobject_class, PROP_PROXY,
      g_param_spec_string ("proxy", "Proxy",
          "Proxy settings for HTTP tunneling. Format: [http://][user:passwd@]host[:port]",
          DEFAULT_PROXY, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRTSPSrc:proxy-id:
   *
   * Sets the proxy URI user id for authentication. If the URI set via the
   * "proxy" property contains a user-id already, that will take precedence.
   *
   * Since: 1.2
   */
  g_object_class_install_property (gobject_class, PROP_PROXY_ID,
      g_param_spec_string ("proxy-id", "proxy-id",
          "HTTP proxy URI user id for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRTSPSrc:proxy-pw:
   *
   * Sets the proxy URI password for authentication. If the URI set via the
   * "proxy" property contains a password already, that will take precedence.
   *
   * Since: 1.2
   */
  g_object_class_install_property (gobject_class, PROP_PROXY_PW,
      g_param_spec_string ("proxy-pw", "proxy-pw",
          "HTTP proxy URI user password for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:rtp-blocksize:
   *
   * RTP package size to suggest to server.
   */
  g_object_class_install_property (gobject_class, PROP_RTP_BLOCKSIZE,
      g_param_spec_uint ("rtp-blocksize", "RTP Blocksize",
          "RTP package size to suggest to server (0 = disabled)",
          0, 65536, DEFAULT_RTP_BLOCKSIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_USER_ID,
      g_param_spec_string ("user-id", "user-id",
          "RTSP location URI user id for authentication", DEFAULT_USER_ID,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_USER_PW,
      g_param_spec_string ("user-pw", "user-pw",
          "RTSP location URI user password for authentication", DEFAULT_USER_PW,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:buffer-mode:
   *
   * Control the buffering and timestamping mode used by the jitterbuffer.
   */
  g_object_class_install_property (gobject_class, PROP_BUFFER_MODE,
      g_param_spec_enum ("buffer-mode", "Buffer Mode",
          "Control the buffering algorithm in use",
          GST_TYPE_RTSP_SRC_BUFFER_MODE, DEFAULT_BUFFER_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:port-range:
   *
   * Configure the client port numbers that can be used to receive RTP and
   * RTCP.
   */
  g_object_class_install_property (gobject_class, PROP_PORT_RANGE,
      g_param_spec_string ("port-range", "Port range",
          "Client port range that can be used to receive RTP and RTCP data, "
          "eg. 3000-3005 (NULL = no restrictions)", DEFAULT_PORT_RANGE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:udp-buffer-size:
   *
   * Size of the kernel UDP receive buffer in bytes.
   */
  g_object_class_install_property (gobject_class, PROP_UDP_BUFFER_SIZE,
      g_param_spec_int ("udp-buffer-size", "UDP Buffer Size",
          "Size of the kernel UDP receive buffer in bytes, 0=default",
          0, G_MAXINT, DEFAULT_UDP_BUFFER_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:short-header:
   *
   * Only send the basic RTSP headers for broken encoders.
   */
  g_object_class_install_property (gobject_class, PROP_SHORT_HEADER,
      g_param_spec_boolean ("short-header", "Short Header",
          "Only send the basic RTSP headers for broken encoders",
          DEFAULT_SHORT_HEADER, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROBATION,
      g_param_spec_uint ("probation", "Number of probations",
          "Consecutive packet sequence numbers to accept the source",
          0, G_MAXUINT, DEFAULT_PROBATION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_UDP_RECONNECT,
      g_param_spec_boolean ("udp-reconnect", "Reconnect to the server",
          "Reconnect to the server if RTSP connection is closed when doing UDP",
          DEFAULT_UDP_RECONNECT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MULTICAST_IFACE,
      g_param_spec_string ("multicast-iface", "Multicast Interface",
          "The network interface on which to join the multicast group",
          DEFAULT_MULTICAST_IFACE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NTP_SYNC,
      g_param_spec_boolean ("ntp-sync", "Sync on NTP clock",
          "Synchronize received streams to the NTP clock", DEFAULT_NTP_SYNC,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_USE_PIPELINE_CLOCK,
      g_param_spec_boolean ("use-pipeline-clock", "Use pipeline clock",
          "Use the pipeline running-time to set the NTP time in the RTCP SR messages"
          "(DEPRECATED: Use ntp-time-source property)",
          DEFAULT_USE_PIPELINE_CLOCK,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));

  g_object_class_install_property (gobject_class, PROP_SDES,
      g_param_spec_boxed ("sdes", "SDES",
          "The SDES items of this session",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc::tls-validation-flags:
   *
   * TLS certificate validation flags used to validate server
   * certificate.
   *
   * Since: 1.2.1
   */
  g_object_class_install_property (gobject_class, PROP_TLS_VALIDATION_FLAGS,
      g_param_spec_flags ("tls-validation-flags", "TLS validation flags",
          "TLS certificate validation flags used to validate the server certificate",
          G_TYPE_TLS_CERTIFICATE_FLAGS, DEFAULT_TLS_VALIDATION_FLAGS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc::tls-database:
   *
   * TLS database with anchor certificate authorities used to validate
   * the server certificate.
   *
   * Since: 1.4
   */
  g_object_class_install_property (gobject_class, PROP_TLS_DATABASE,
      g_param_spec_object ("tls-database", "TLS database",
          "TLS database with anchor certificate authorities used to validate the server certificate",
          G_TYPE_TLS_DATABASE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc::tls-interaction:
   *
   * A #GTlsInteraction object to be used when the connection or certificate
   * database need to interact with the user. This will be used to prompt the
   * user for passwords where necessary.
   *
   * Since: 1.6
   */
  g_object_class_install_property (gobject_class, PROP_TLS_INTERACTION,
      g_param_spec_object ("tls-interaction", "TLS interaction",
          "A GTlsInteraction object to prompt the user for password or certificate",
          G_TYPE_TLS_INTERACTION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc::do-retransmission:
   *
   * Attempt to ask the server to retransmit lost packets according to RFC4588.
   *
   * Note: currently only works with SSRC-multiplexed retransmission streams
   *
   * Since: 1.6
   */
  g_object_class_install_property (gobject_class, PROP_DO_RETRANSMISSION,
      g_param_spec_boolean ("do-retransmission", "Retransmission",
          "Ask the server to retransmit lost packets",
          DEFAULT_DO_RETRANSMISSION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc::ntp-time-source:
   *
   * allows to select the time source that should be used
   * for the NTP time in RTCP packets
   *
   * Since: 1.6
   */
  g_object_class_install_property (gobject_class, PROP_NTP_TIME_SOURCE,
      g_param_spec_enum ("ntp-time-source", "NTP Time Source",
          "NTP time source for RTCP packets",
          GST_TYPE_RTSP_SRC_NTP_TIME_SOURCE, DEFAULT_NTP_TIME_SOURCE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc::user-agent:
   *
   * The string to set in the User-Agent header.
   *
   * Since: 1.6
   */
  g_object_class_install_property (gobject_class, PROP_USER_AGENT,
      g_param_spec_string ("user-agent", "User Agent",
          "The User-Agent string to send to the server",
          DEFAULT_USER_AGENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_RTCP_RTP_TIME_DIFF,
      g_param_spec_int ("max-rtcp-rtp-time-diff", "Max RTCP RTP Time Diff",
          "Maximum amount of time in ms that the RTP time in RTCP SRs "
          "is allowed to be ahead (-1 disabled)", -1, G_MAXINT,
          DEFAULT_MAX_RTCP_RTP_TIME_DIFF,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RFC7273_SYNC,
      g_param_spec_boolean ("rfc7273-sync", "Sync on RFC7273 clock",
          "Synchronize received streams to the RFC7273 clock "
          "(requires clock and offset to be provided)", DEFAULT_RFC7273_SYNC,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:default-rtsp-version:
   *
   * The preferred RTSP version to use while negotiating the version with the server.
   *
   * Since: 1.14
   */
  g_object_class_install_property (gobject_class, PROP_DEFAULT_VERSION,
      g_param_spec_enum ("default-rtsp-version",
          "The RTSP version to try first",
          "The RTSP version that should be tried first when negotiating version.",
          GST_TYPE_RTSP_VERSION, DEFAULT_VERSION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:max-ts-offset-adjustment:
   *
   * Syncing time stamps to NTP time adds a time offset. This parameter
   * specifies the maximum number of nanoseconds per frame that this time offset
   * may be adjusted with. This is used to avoid sudden large changes to time
   * stamps.
   */
  g_object_class_install_property (gobject_class, PROP_MAX_TS_OFFSET_ADJUSTMENT,
      g_param_spec_uint64 ("max-ts-offset-adjustment",
          "Max Timestamp Offset Adjustment",
          "The maximum number of nanoseconds per frame that time stamp offsets "
          "may be adjusted (0 = no limit).", 0, G_MAXUINT64,
          DEFAULT_MAX_TS_OFFSET_ADJUSTMENT, G_PARAM_READWRITE |
          G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:max-ts-offset:
   *
   * Used to set an upper limit of how large a time offset may be. This
   * is used to protect against unrealistic values as a result of either
   * client,server or clock issues.
   */
  g_object_class_install_property (gobject_class, PROP_MAX_TS_OFFSET,
      g_param_spec_int64 ("max-ts-offset", "Max TS Offset",
          "The maximum absolute value of the time offset in (nanoseconds). "
          "Note, if the ntp-sync parameter is set the default value is "
          "changed to 0 (no limit)", 0, G_MAXINT64, DEFAULT_MAX_TS_OFFSET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc:backchannel
   *
   * Select a type of backchannel to setup with the RTSP server.
   * Default value is "none". Allowed values are "none" and "onvif".
   *
   * Since: 1.14
   */
  g_object_class_install_property (gobject_class, PROP_BACKCHANNEL,
      g_param_spec_enum ("backchannel", "Backchannel type",
          "The type of backchannel to setup. Default is 'none'.",
          GST_TYPE_RTSP_BACKCHANNEL, BACKCHANNEL_NONE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtspSrc:teardown-timeout
   *
   * When transitioning PAUSED-READY, allow up to timeout (in nanoseconds)
   * delay in order to send teardown (0 = disabled)
   *
   * Since: 1.14
   */
  g_object_class_install_property (gobject_class, PROP_TEARDOWN_TIMEOUT,
      g_param_spec_uint64 ("teardown-timeout", "Teardown Timeout",
          "When transitioning PAUSED-READY, allow up to timeout (in nanoseconds) "
          "delay in order to send teardown (0 = disabled)",
          0, G_MAXUINT64, DEFAULT_TEARDOWN_TIMEOUT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtspSrc:onvif-mode
   *
   * Act as an ONVIF client. When set to %TRUE:
   *
   * - seeks will be interpreted as nanoseconds since prime epoch (1900-01-01)
   *
   * - #GstRtspSrc:onvif-rate-control can be used to request that the server sends
   *   data as fast as it can
   *
   * - TCP is picked as the transport protocol
   *
   * - Trickmode flags in seek events are transformed into the appropriate ONVIF
   *   request headers
   *
   * Since: 1.18
   */
  g_object_class_install_property (gobject_class, PROP_ONVIF_MODE,
      g_param_spec_boolean ("onvif-mode", "Onvif Mode",
          "Act as an ONVIF client",
          DEFAULT_ONVIF_MODE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtspSrc:onvif-rate-control
   *
   * When in onvif-mode, whether to set Rate-Control to yes or no. When set
   * to %FALSE, the server will deliver data as fast as the client can consume
   * it.
   *
   * Since: 1.18
   */
  g_object_class_install_property (gobject_class, PROP_ONVIF_RATE_CONTROL,
      g_param_spec_boolean ("onvif-rate-control", "Onvif Rate Control",
          "When in onvif-mode, whether to set Rate-Control to yes or no",
          DEFAULT_ONVIF_RATE_CONTROL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRtspSrc:is-live
   *
   * Whether to act as a live source. This is useful in combination with
   * #GstRtspSrc:onvif-rate-control set to %FALSE and usage of the TCP
   * protocol. In that situation, data delivery rate can be entirely
   * controlled from the client side, enabling features such as frame
   * stepping and instantaneous rate changes.
   *
   * Since: 1.18
   */
  g_object_class_install_property (gobject_class, PROP_IS_LIVE,
      g_param_spec_boolean ("is-live", "Is live",
          "Whether to act as a live source",
          DEFAULT_IS_LIVE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstRTSPSrc::handle-request:
   * @rtspsrc: a #GstRTSPSrc
   * @request: a #GstRTSPMessage
   * @response: a #GstRTSPMessage
   *
   * Handle a server request in @request and prepare @response.
   *
   * This signal is called from the streaming thread, you should therefore not
   * do any state changes on @rtspsrc because this might deadlock. If you want
   * to modify the state as a result of this signal, post a
   * #GST_MESSAGE_REQUEST_STATE message on the bus or signal the main thread
   * in some other way.
   *
   * Since: 1.2
   */
  gst_rtspsrc_signals[SIGNAL_HANDLE_REQUEST] =
      g_signal_new ("handle-request", G_TYPE_FROM_CLASS (klass), 0,
      0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

  /**
   * GstRTSPSrc::on-sdp:
   * @rtspsrc: a #GstRTSPSrc
   * @sdp: a #GstSDPMessage
   *
   * Emitted when the client has retrieved the SDP and before it configures the
   * streams in the SDP. @sdp can be inspected and modified.
   *
   * This signal is called from the streaming thread, you should therefore not
   * do any state changes on @rtspsrc because this might deadlock. If you want
   * to modify the state as a result of this signal, post a
   * #GST_MESSAGE_REQUEST_STATE message on the bus or signal the main thread
   * in some other way.
   *
   * Since: 1.2
   */
  gst_rtspsrc_signals[SIGNAL_ON_SDP] =
      g_signal_new ("on-sdp", G_TYPE_FROM_CLASS (klass), 0,
      0, NULL, NULL, NULL, G_TYPE_NONE, 1,
      GST_TYPE_SDP_MESSAGE | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * GstRTSPSrc::select-stream:
   * @rtspsrc: a #GstRTSPSrc
   * @num: the stream number
   * @caps: the stream caps
   *
   * Emitted before the client decides to configure the stream @num with
   * @caps.
   *
   * Returns: %TRUE when the stream should be selected, %FALSE when the stream
   * is to be ignored.
   *
   * Since: 1.2
   */
  gst_rtspsrc_signals[SIGNAL_SELECT_STREAM] =
      g_signal_new_class_handler ("select-stream", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_CLEANUP,
      (GCallback) default_select_stream, select_stream_accum, NULL, NULL,
      G_TYPE_BOOLEAN, 2, G_TYPE_UINT, GST_TYPE_CAPS);
  /**
   * GstRTSPSrc::new-manager:
   * @rtspsrc: a #GstRTSPSrc
   * @manager: a #GstElement
   *
   * Emitted after a new manager (like rtpbin) was created and the default
   * properties were configured.
   *
   * Since: 1.4
   */
  gst_rtspsrc_signals[SIGNAL_NEW_MANAGER] =
      g_signal_new_class_handler ("new-manager", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_CLEANUP, 0, NULL, NULL, NULL,
      G_TYPE_NONE, 1, GST_TYPE_ELEMENT);

  /**
   * GstRTSPSrc::request-rtcp-key:
   * @rtspsrc: a #GstRTSPSrc
   * @num: the stream number
   *
   * Signal emitted to get the crypto parameters relevant to the RTCP
   * stream. User should provide the key and the RTCP encryption ciphers
   * and authentication, and return them wrapped in a GstCaps.
   *
   * Since: 1.4
   */
  gst_rtspsrc_signals[SIGNAL_REQUEST_RTCP_KEY] =
      g_signal_new ("request-rtcp-key", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, GST_TYPE_CAPS, 1, G_TYPE_UINT);

  /**
   * GstRTSPSrc::accept-certificate:
   * @rtspsrc: a #GstRTSPSrc
   * @peer_cert: the peer's #GTlsCertificate
   * @errors: the problems with @peer_cert
   * @user_data: user data set when the signal handler was connected.
   *
   * This will directly map to #GTlsConnection 's "accept-certificate"
   * signal and be performed after the default checks of #GstRTSPConnection
   * (checking against the #GTlsDatabase with the given #GTlsCertificateFlags)
   * have failed. If no #GTlsDatabase is set on this connection, only this
   * signal will be emitted.
   *
   * Since: 1.14
   */
  gst_rtspsrc_signals[SIGNAL_ACCEPT_CERTIFICATE] =
      g_signal_new ("accept-certificate", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, g_signal_accumulator_true_handled, NULL, NULL,
      G_TYPE_BOOLEAN, 3, G_TYPE_TLS_CONNECTION, G_TYPE_TLS_CERTIFICATE,
      G_TYPE_TLS_CERTIFICATE_FLAGS);

  /*
   * GstRTSPSrc::before-send
   * @rtspsrc: a #GstRTSPSrc
   * @num: the stream number
   *
   * Emitted before each RTSP request is sent, in order to allow
   * the application to modify send parameters or to skip the message entirely.
   * This can be used, for example, to work with ONVIF Profile G servers,
   * which need a different/additional range, rate-control, and intra/x
   * parameters.
   *
   * Returns: %TRUE when the command should be sent, %FALSE when the
   * command should be dropped.
   *
   * Since: 1.14
   */
  gst_rtspsrc_signals[SIGNAL_BEFORE_SEND] =
      g_signal_new_class_handler ("before-send", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_CLEANUP,
      (GCallback) default_before_send, before_send_accum, NULL, NULL,
      G_TYPE_BOOLEAN, 1, GST_TYPE_RTSP_MESSAGE | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * GstRTSPSrc::push-backchannel-buffer:
   * @rtspsrc: a #GstRTSPSrc
   * @buffer: RTP buffer to send back
   *
   *
   */
  gst_rtspsrc_signals[SIGNAL_PUSH_BACKCHANNEL_BUFFER] =
      g_signal_new ("push-backchannel-buffer", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRTSPSrcClass,
          push_backchannel_buffer), NULL, NULL, NULL,
      GST_TYPE_FLOW_RETURN, 2, G_TYPE_UINT, GST_TYPE_BUFFER);

  /**
   * GstRTSPSrc::get-parameter:
   * @rtspsrc: a #GstRTSPSrc
   * @parameter: the parameter name
   * @parameter: the content type
   * @parameter: a pointer to #GstPromise
   *
   * Handle the GET_PARAMETER signal.
   *
   * Returns: %TRUE when the command could be issued, %FALSE otherwise
   *
   */
  gst_rtspsrc_signals[SIGNAL_GET_PARAMETER] =
      g_signal_new ("get-parameter", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRTSPSrcClass,
          get_parameter), NULL, NULL, NULL,
      G_TYPE_BOOLEAN, 3, G_TYPE_STRING, G_TYPE_STRING, GST_TYPE_PROMISE);

  /**
   * GstRTSPSrc::get-parameters:
   * @rtspsrc: a #GstRTSPSrc
   * @parameter: a NULL-terminated array of parameters
   * @parameter: the content type
   * @parameter: a pointer to #GstPromise
   *
   * Handle the GET_PARAMETERS signal.
   *
   * Returns: %TRUE when the command could be issued, %FALSE otherwise
   *
   */
  gst_rtspsrc_signals[SIGNAL_GET_PARAMETERS] =
      g_signal_new ("get-parameters", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRTSPSrcClass,
          get_parameters), NULL, NULL, NULL,
      G_TYPE_BOOLEAN, 3, G_TYPE_STRV, G_TYPE_STRING, GST_TYPE_PROMISE);

  /**
   * GstRTSPSrc::set-parameter:
   * @rtspsrc: a #GstRTSPSrc
   * @parameter: the parameter name
   * @parameter: the parameter value
   * @parameter: the content type
   * @parameter: a pointer to #GstPromise
   *
   * Handle the SET_PARAMETER signal.
   *
   * Returns: %TRUE when the command could be issued, %FALSE otherwise
   *
   */
  gst_rtspsrc_signals[SIGNAL_SET_PARAMETER] =
      g_signal_new ("set-parameter", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstRTSPSrcClass,
          set_parameter), NULL, NULL, NULL, G_TYPE_BOOLEAN, 4, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_STRING, GST_TYPE_PROMISE);

  gstelement_class->send_event = gst_rtspsrc_send_event;
  gstelement_class->provide_clock = gst_rtspsrc_provide_clock;
  gstelement_class->change_state = gst_rtspsrc_change_state;

  gst_element_class_add_static_pad_template (gstelement_class, &rtptemplate);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTSP packet receiver", "Source/Network",
      "Receive data over the network via RTSP (RFC 2326)",
      "Wim Taymans <wim@fluendo.com>, "
      "Thijs Vermeir <thijs.vermeir@barco.com>, "
      "Lutz Mueller <lutz@topfrose.de>");

  gstbin_class->handle_message = gst_rtspsrc_handle_message;

  klass->push_backchannel_buffer = gst_rtspsrc_push_backchannel_buffer;
  klass->get_parameter = GST_DEBUG_FUNCPTR (get_parameter);
  klass->get_parameters = GST_DEBUG_FUNCPTR (get_parameters);
  klass->set_parameter = GST_DEBUG_FUNCPTR (set_parameter);

  gst_rtsp_ext_list_init ();
}

static gboolean
validate_set_get_parameter_name (const gchar * parameter_name)
{
  gchar *ptr = (gchar *) parameter_name;

  while (*ptr) {
    /* Don't allow '\r', '\n', \'t', ' ' etc in the parameter name */
    if (g_ascii_isspace (*ptr) || g_ascii_iscntrl (*ptr)) {
      GST_DEBUG ("invalid parameter name '%s'", parameter_name);
      return FALSE;
    }
    ptr++;
  }
  return TRUE;
}

static gboolean
validate_set_get_parameters (gchar ** parameter_names)
{
  while (*parameter_names) {
    if (!validate_set_get_parameter_name (*parameter_names)) {
      return FALSE;
    }
    parameter_names++;
  }
  return TRUE;
}

static gboolean
get_parameter (GstRTSPSrc * src, const gchar * parameter,
    const gchar * content_type, GstPromise * promise)
{
  gchar *parameters[] = { (gchar *) parameter, NULL };

  GST_LOG_OBJECT (src, "get_parameter: %s", GST_STR_NULL (parameter));

  if (parameter == NULL || parameter[0] == '\0' || promise == NULL) {
    GST_DEBUG ("invalid input");
    return FALSE;
  }

  return get_parameters (src, parameters, content_type, promise);
}

static gboolean
get_parameters (GstRTSPSrc * src, gchar ** parameters,
    const gchar * content_type, GstPromise * promise)
{
  ParameterRequest *req;

  GST_LOG_OBJECT (src, "get_parameters: %d", g_strv_length (parameters));

  if (parameters == NULL || promise == NULL) {
    GST_DEBUG ("invalid input");
    return FALSE;
  }

  if (src->state == GST_RTSP_STATE_INVALID) {
    GST_DEBUG ("invalid state");
    return FALSE;
  }

  if (!validate_set_get_parameters (parameters)) {
    return FALSE;
  }

  req = g_new0 (ParameterRequest, 1);
  req->promise = gst_promise_ref (promise);
  req->cmd = CMD_GET_PARAMETER;
  /* Set the request body according to RFC 2326 or RFC 7826 */
  req->body = g_string_new (NULL);
  while (*parameters) {
    g_string_append_printf (req->body, "%s:\r\n", *parameters);
    parameters++;
  }
  if (content_type)
    req->content_type = g_strdup (content_type);

  GST_OBJECT_LOCK (src);
  g_queue_push_tail (&src->set_get_param_q, req);
  GST_OBJECT_UNLOCK (src);

  gst_rtspsrc_loop_send_cmd (src, CMD_GET_PARAMETER, CMD_LOOP);

  return TRUE;
}

static gboolean
set_parameter (GstRTSPSrc * src, const gchar * name, const gchar * value,
    const gchar * content_type, GstPromise * promise)
{
  ParameterRequest *req;

  GST_LOG_OBJECT (src, "set_parameter: %s: %s", GST_STR_NULL (name),
      GST_STR_NULL (value));

  if (name == NULL || name[0] == '\0' || value == NULL || promise == NULL) {
    GST_DEBUG ("invalid input");
    return FALSE;
  }

  if (src->state == GST_RTSP_STATE_INVALID) {
    GST_DEBUG ("invalid state");
    return FALSE;
  }

  if (!validate_set_get_parameter_name (name)) {
    return FALSE;
  }

  req = g_new0 (ParameterRequest, 1);
  req->cmd = CMD_SET_PARAMETER;
  req->promise = gst_promise_ref (promise);
  req->body = g_string_new (NULL);
  /* Set the request body according to RFC 2326 or RFC 7826 */
  g_string_append_printf (req->body, "%s: %s\r\n", name, value);
  if (content_type)
    req->content_type = g_strdup (content_type);

  GST_OBJECT_LOCK (src);
  g_queue_push_tail (&src->set_get_param_q, req);
  GST_OBJECT_UNLOCK (src);

  gst_rtspsrc_loop_send_cmd (src, CMD_SET_PARAMETER, CMD_LOOP);

  return TRUE;
}

static void
gst_rtspsrc_init (GstRTSPSrc * src)
{
  src->conninfo.location = g_strdup (DEFAULT_LOCATION);
  src->protocols = DEFAULT_PROTOCOLS;
  src->debug = DEFAULT_DEBUG;
  src->retry = DEFAULT_RETRY;
  src->udp_timeout = DEFAULT_TIMEOUT;
  gst_rtspsrc_set_tcp_timeout (src, DEFAULT_TCP_TIMEOUT);
  src->latency = DEFAULT_LATENCY_MS;
  src->drop_on_latency = DEFAULT_DROP_ON_LATENCY;
  src->connection_speed = DEFAULT_CONNECTION_SPEED;
  src->nat_method = DEFAULT_NAT_METHOD;
  src->do_rtcp = DEFAULT_DO_RTCP;
  src->do_rtsp_keep_alive = DEFAULT_DO_RTSP_KEEP_ALIVE;
  gst_rtspsrc_set_proxy (src, DEFAULT_PROXY);
  src->rtp_blocksize = DEFAULT_RTP_BLOCKSIZE;
  src->user_id = g_strdup (DEFAULT_USER_ID);
  src->user_pw = g_strdup (DEFAULT_USER_PW);
  src->buffer_mode = DEFAULT_BUFFER_MODE;
  src->client_port_range.min = 0;
  src->client_port_range.max = 0;
  src->udp_buffer_size = DEFAULT_UDP_BUFFER_SIZE;
  src->short_header = DEFAULT_SHORT_HEADER;
  src->probation = DEFAULT_PROBATION;
  src->udp_reconnect = DEFAULT_UDP_RECONNECT;
  src->multi_iface = g_strdup (DEFAULT_MULTICAST_IFACE);
  src->ntp_sync = DEFAULT_NTP_SYNC;
  src->use_pipeline_clock = DEFAULT_USE_PIPELINE_CLOCK;
  src->sdes = NULL;
  src->tls_validation_flags = DEFAULT_TLS_VALIDATION_FLAGS;
  src->tls_database = DEFAULT_TLS_DATABASE;
  src->tls_interaction = DEFAULT_TLS_INTERACTION;
  src->do_retransmission = DEFAULT_DO_RETRANSMISSION;
  src->ntp_time_source = DEFAULT_NTP_TIME_SOURCE;
  src->user_agent = g_strdup (DEFAULT_USER_AGENT);
  src->max_rtcp_rtp_time_diff = DEFAULT_MAX_RTCP_RTP_TIME_DIFF;
  src->rfc7273_sync = DEFAULT_RFC7273_SYNC;
  src->max_ts_offset_adjustment = DEFAULT_MAX_TS_OFFSET_ADJUSTMENT;
  src->max_ts_offset = DEFAULT_MAX_TS_OFFSET;
  src->max_ts_offset_is_set = FALSE;
  src->default_version = DEFAULT_VERSION;
  src->version = GST_RTSP_VERSION_INVALID;
  src->teardown_timeout = DEFAULT_TEARDOWN_TIMEOUT;
  src->onvif_mode = DEFAULT_ONVIF_MODE;
  src->onvif_rate_control = DEFAULT_ONVIF_RATE_CONTROL;
  src->is_live = DEFAULT_IS_LIVE;

  /* get a list of all extensions */
  src->extensions = gst_rtsp_ext_list_get ();

  /* connect to send signal */
  gst_rtsp_ext_list_connect (src->extensions, "send",
      (GCallback) gst_rtspsrc_send_cb, src);

  /* protects the streaming thread in interleaved mode or the polling
   * thread in UDP mode. */
  g_rec_mutex_init (&src->stream_rec_lock);

  /* protects our state changes from multiple invocations */
  g_rec_mutex_init (&src->state_rec_lock);

  g_queue_init (&src->set_get_param_q);

  src->state = GST_RTSP_STATE_INVALID;

  g_mutex_init (&src->conninfo.send_lock);
  g_mutex_init (&src->conninfo.recv_lock);
  g_cond_init (&src->cmd_cond);

  GST_OBJECT_FLAG_SET (src, GST_ELEMENT_FLAG_SOURCE);
  gst_bin_set_suppressed_flags (GST_BIN (src),
      GST_ELEMENT_FLAG_SOURCE | GST_ELEMENT_FLAG_SINK);
}

static void
free_param_data (ParameterRequest * req)
{
  gst_promise_unref (req->promise);
  if (req->body)
    g_string_free (req->body, TRUE);
  g_free (req->content_type);
  g_free (req);
}

static void
free_param_queue (gpointer data)
{
  ParameterRequest *req = data;

  gst_promise_expire (req->promise);
  free_param_data (req);
}

static void
gst_rtspsrc_finalize (GObject * object)
{
  GstRTSPSrc *rtspsrc;

  rtspsrc = GST_RTSPSRC (object);

  gst_rtsp_ext_list_free (rtspsrc->extensions);
  g_free (rtspsrc->conninfo.location);
  gst_rtsp_url_free (rtspsrc->conninfo.url);
  g_free (rtspsrc->conninfo.url_str);
  g_free (rtspsrc->user_id);
  g_free (rtspsrc->user_pw);
  g_free (rtspsrc->multi_iface);
  g_free (rtspsrc->user_agent);

  if (rtspsrc->sdp) {
    gst_sdp_message_free (rtspsrc->sdp);
    rtspsrc->sdp = NULL;
  }
  if (rtspsrc->provided_clock)
    gst_object_unref (rtspsrc->provided_clock);

  if (rtspsrc->sdes)
    gst_structure_free (rtspsrc->sdes);

  if (rtspsrc->tls_database)
    g_object_unref (rtspsrc->tls_database);

  if (rtspsrc->tls_interaction)
    g_object_unref (rtspsrc->tls_interaction);

  /* free locks */
  g_rec_mutex_clear (&rtspsrc->stream_rec_lock);
  g_rec_mutex_clear (&rtspsrc->state_rec_lock);

  g_mutex_clear (&rtspsrc->conninfo.send_lock);
  g_mutex_clear (&rtspsrc->conninfo.recv_lock);
  g_cond_clear (&rtspsrc->cmd_cond);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstClock *
gst_rtspsrc_provide_clock (GstElement * element)
{
  GstRTSPSrc *src = GST_RTSPSRC (element);
  GstClock *clock;

  if ((clock = src->provided_clock) != NULL)
    return gst_object_ref (clock);

  return GST_ELEMENT_CLASS (parent_class)->provide_clock (element);
}

/* a proxy string of the format [user:passwd@]host[:port] */
static gboolean
gst_rtspsrc_set_proxy (GstRTSPSrc * rtsp, const gchar * proxy)
{
  gchar *p, *at, *col;

  g_free (rtsp->proxy_user);
  rtsp->proxy_user = NULL;
  g_free (rtsp->proxy_passwd);
  rtsp->proxy_passwd = NULL;
  g_free (rtsp->proxy_host);
  rtsp->proxy_host = NULL;
  rtsp->proxy_port = 0;

  p = (gchar *) proxy;

  if (p == NULL)
    return TRUE;

  /* we allow http:// in front but ignore it */
  if (g_str_has_prefix (p, "http://"))
    p += 7;

  at = strchr (p, '@');
  if (at) {
    /* look for user:passwd */
    col = strchr (proxy, ':');
    if (col == NULL || col > at)
      return FALSE;

    rtsp->proxy_user = g_strndup (p, col - p);
    col++;
    rtsp->proxy_passwd = g_strndup (col, at - col);

    /* move to host */
    p = at + 1;
  } else {
    if (rtsp->prop_proxy_id != NULL && *rtsp->prop_proxy_id != '\0')
      rtsp->proxy_user = g_strdup (rtsp->prop_proxy_id);
    if (rtsp->prop_proxy_pw != NULL && *rtsp->prop_proxy_pw != '\0')
      rtsp->proxy_passwd = g_strdup (rtsp->prop_proxy_pw);
    if (rtsp->proxy_user != NULL || rtsp->proxy_passwd != NULL) {
      GST_LOG_OBJECT (rtsp, "set proxy user/pw from properties: %s:%s",
          GST_STR_NULL (rtsp->proxy_user), GST_STR_NULL (rtsp->proxy_passwd));
    }
  }
  col = strchr (p, ':');

  if (col) {
    /* everything before the colon is the hostname */
    rtsp->proxy_host = g_strndup (p, col - p);
    p = col + 1;
    rtsp->proxy_port = strtoul (p, (char **) &p, 10);
  } else {
    rtsp->proxy_host = g_strdup (p);
    rtsp->proxy_port = 8080;
  }
  return TRUE;
}

static void
gst_rtspsrc_set_tcp_timeout (GstRTSPSrc * rtspsrc, guint64 timeout)
{
  rtspsrc->tcp_timeout.tv_sec = timeout / G_USEC_PER_SEC;
  rtspsrc->tcp_timeout.tv_usec = timeout % G_USEC_PER_SEC;

  if (timeout != 0)
    rtspsrc->ptcp_timeout = &rtspsrc->tcp_timeout;
  else
    rtspsrc->ptcp_timeout = NULL;
}

static void
gst_rtspsrc_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstRTSPSrc *rtspsrc;

  rtspsrc = GST_RTSPSRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      gst_rtspsrc_uri_set_uri (GST_URI_HANDLER (rtspsrc),
          g_value_get_string (value), NULL);
      break;
    case PROP_PROTOCOLS:
      rtspsrc->protocols = g_value_get_flags (value);
      break;
    case PROP_DEBUG:
      rtspsrc->debug = g_value_get_boolean (value);
      break;
    case PROP_RETRY:
      rtspsrc->retry = g_value_get_uint (value);
      break;
    case PROP_TIMEOUT:
      rtspsrc->udp_timeout = g_value_get_uint64 (value);
      break;
    case PROP_TCP_TIMEOUT:
      gst_rtspsrc_set_tcp_timeout (rtspsrc, g_value_get_uint64 (value));
      break;
    case PROP_LATENCY:
      rtspsrc->latency = g_value_get_uint (value);
      break;
    case PROP_DROP_ON_LATENCY:
      rtspsrc->drop_on_latency = g_value_get_boolean (value);
      break;
    case PROP_CONNECTION_SPEED:
      rtspsrc->connection_speed = g_value_get_uint64 (value);
      break;
    case PROP_NAT_METHOD:
      rtspsrc->nat_method = g_value_get_enum (value);
      break;
    case PROP_DO_RTCP:
      rtspsrc->do_rtcp = g_value_get_boolean (value);
      break;
    case PROP_DO_RTSP_KEEP_ALIVE:
      rtspsrc->do_rtsp_keep_alive = g_value_get_boolean (value);
      break;
    case PROP_PROXY:
      gst_rtspsrc_set_proxy (rtspsrc, g_value_get_string (value));
      break;
    case PROP_PROXY_ID:
      g_free (rtspsrc->prop_proxy_id);
      rtspsrc->prop_proxy_id = g_value_dup_string (value);
      break;
    case PROP_PROXY_PW:
      g_free (rtspsrc->prop_proxy_pw);
      rtspsrc->prop_proxy_pw = g_value_dup_string (value);
      break;
    case PROP_RTP_BLOCKSIZE:
      rtspsrc->rtp_blocksize = g_value_get_uint (value);
      break;
    case PROP_USER_ID:
      g_free (rtspsrc->user_id);
      rtspsrc->user_id = g_value_dup_string (value);
      break;
    case PROP_USER_PW:
      g_free (rtspsrc->user_pw);
      rtspsrc->user_pw = g_value_dup_string (value);
      break;
    case PROP_BUFFER_MODE:
      rtspsrc->buffer_mode = g_value_get_enum (value);
      break;
    case PROP_PORT_RANGE:
    {
      const gchar *str;

      str = g_value_get_string (value);
      if (str == NULL || sscanf (str, "%u-%u", &rtspsrc->client_port_range.min,
              &rtspsrc->client_port_range.max) != 2) {
        rtspsrc->client_port_range.min = 0;
        rtspsrc->client_port_range.max = 0;
      }
      break;
    }
    case PROP_UDP_BUFFER_SIZE:
      rtspsrc->udp_buffer_size = g_value_get_int (value);
      break;
    case PROP_SHORT_HEADER:
      rtspsrc->short_header = g_value_get_boolean (value);
      break;
    case PROP_PROBATION:
      rtspsrc->probation = g_value_get_uint (value);
      break;
    case PROP_UDP_RECONNECT:
      rtspsrc->udp_reconnect = g_value_get_boolean (value);
      break;
    case PROP_MULTICAST_IFACE:
      g_free (rtspsrc->multi_iface);

      if (g_value_get_string (value) == NULL)
        rtspsrc->multi_iface = g_strdup (DEFAULT_MULTICAST_IFACE);
      else
        rtspsrc->multi_iface = g_value_dup_string (value);
      break;
    case PROP_NTP_SYNC:
      rtspsrc->ntp_sync = g_value_get_boolean (value);
      /* The default value of max_ts_offset depends on ntp_sync. If user
       * hasn't set it then change default value */
      if (!rtspsrc->max_ts_offset_is_set) {
        if (rtspsrc->ntp_sync) {
          rtspsrc->max_ts_offset = 0;
        } else {
          rtspsrc->max_ts_offset = DEFAULT_MAX_TS_OFFSET;
        }
      }
      break;
    case PROP_USE_PIPELINE_CLOCK:
      rtspsrc->use_pipeline_clock = g_value_get_boolean (value);
      break;
    case PROP_SDES:
      rtspsrc->sdes = g_value_dup_boxed (value);
      break;
    case PROP_TLS_VALIDATION_FLAGS:
      rtspsrc->tls_validation_flags = g_value_get_flags (value);
      break;
    case PROP_TLS_DATABASE:
      g_clear_object (&rtspsrc->tls_database);
      rtspsrc->tls_database = g_value_dup_object (value);
      break;
    case PROP_TLS_INTERACTION:
      g_clear_object (&rtspsrc->tls_interaction);
      rtspsrc->tls_interaction = g_value_dup_object (value);
      break;
    case PROP_DO_RETRANSMISSION:
      rtspsrc->do_retransmission = g_value_get_boolean (value);
      break;
    case PROP_NTP_TIME_SOURCE:
      rtspsrc->ntp_time_source = g_value_get_enum (value);
      break;
    case PROP_USER_AGENT:
      g_free (rtspsrc->user_agent);
      rtspsrc->user_agent = g_value_dup_string (value);
      break;
    case PROP_MAX_RTCP_RTP_TIME_DIFF:
      rtspsrc->max_rtcp_rtp_time_diff = g_value_get_int (value);
      break;
    case PROP_RFC7273_SYNC:
      rtspsrc->rfc7273_sync = g_value_get_boolean (value);
      break;
    case PROP_MAX_TS_OFFSET_ADJUSTMENT:
      rtspsrc->max_ts_offset_adjustment = g_value_get_uint64 (value);
      break;
    case PROP_MAX_TS_OFFSET:
      rtspsrc->max_ts_offset = g_value_get_int64 (value);
      rtspsrc->max_ts_offset_is_set = TRUE;
      break;
    case PROP_DEFAULT_VERSION:
      rtspsrc->default_version = g_value_get_enum (value);
      break;
    case PROP_BACKCHANNEL:
      rtspsrc->backchannel = g_value_get_enum (value);
      break;
    case PROP_TEARDOWN_TIMEOUT:
      rtspsrc->teardown_timeout = g_value_get_uint64 (value);
      break;
    case PROP_ONVIF_MODE:
      rtspsrc->onvif_mode = g_value_get_boolean (value);
      break;
    case PROP_ONVIF_RATE_CONTROL:
      rtspsrc->onvif_rate_control = g_value_get_boolean (value);
      break;
    case PROP_IS_LIVE:
      rtspsrc->is_live = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtspsrc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstRTSPSrc *rtspsrc;

  rtspsrc = GST_RTSPSRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_set_string (value, rtspsrc->conninfo.location);
      break;
    case PROP_PROTOCOLS:
      g_value_set_flags (value, rtspsrc->protocols);
      break;
    case PROP_DEBUG:
      g_value_set_boolean (value, rtspsrc->debug);
      break;
    case PROP_RETRY:
      g_value_set_uint (value, rtspsrc->retry);
      break;
    case PROP_TIMEOUT:
      g_value_set_uint64 (value, rtspsrc->udp_timeout);
      break;
    case PROP_TCP_TIMEOUT:
    {
      guint64 timeout;

      timeout = ((guint64) rtspsrc->tcp_timeout.tv_sec) * G_USEC_PER_SEC +
          rtspsrc->tcp_timeout.tv_usec;
      g_value_set_uint64 (value, timeout);
      break;
    }
    case PROP_LATENCY:
      g_value_set_uint (value, rtspsrc->latency);
      break;
    case PROP_DROP_ON_LATENCY:
      g_value_set_boolean (value, rtspsrc->drop_on_latency);
      break;
    case PROP_CONNECTION_SPEED:
      g_value_set_uint64 (value, rtspsrc->connection_speed);
      break;
    case PROP_NAT_METHOD:
      g_value_set_enum (value, rtspsrc->nat_method);
      break;
    case PROP_DO_RTCP:
      g_value_set_boolean (value, rtspsrc->do_rtcp);
      break;
    case PROP_DO_RTSP_KEEP_ALIVE:
      g_value_set_boolean (value, rtspsrc->do_rtsp_keep_alive);
      break;
    case PROP_PROXY:
    {
      gchar *str;

      if (rtspsrc->proxy_host) {
        str =
            g_strdup_printf ("%s:%d", rtspsrc->proxy_host, rtspsrc->proxy_port);
      } else {
        str = NULL;
      }
      g_value_take_string (value, str);
      break;
    }
    case PROP_PROXY_ID:
      g_value_set_string (value, rtspsrc->prop_proxy_id);
      break;
    case PROP_PROXY_PW:
      g_value_set_string (value, rtspsrc->prop_proxy_pw);
      break;
    case PROP_RTP_BLOCKSIZE:
      g_value_set_uint (value, rtspsrc->rtp_blocksize);
      break;
    case PROP_USER_ID:
      g_value_set_string (value, rtspsrc->user_id);
      break;
    case PROP_USER_PW:
      g_value_set_string (value, rtspsrc->user_pw);
      break;
    case PROP_BUFFER_MODE:
      g_value_set_enum (value, rtspsrc->buffer_mode);
      break;
    case PROP_PORT_RANGE:
    {
      gchar *str;

      if (rtspsrc->client_port_range.min != 0) {
        str = g_strdup_printf ("%u-%u", rtspsrc->client_port_range.min,
            rtspsrc->client_port_range.max);
      } else {
        str = NULL;
      }
      g_value_take_string (value, str);
      break;
    }
    case PROP_UDP_BUFFER_SIZE:
      g_value_set_int (value, rtspsrc->udp_buffer_size);
      break;
    case PROP_SHORT_HEADER:
      g_value_set_boolean (value, rtspsrc->short_header);
      break;
    case PROP_PROBATION:
      g_value_set_uint (value, rtspsrc->probation);
      break;
    case PROP_UDP_RECONNECT:
      g_value_set_boolean (value, rtspsrc->udp_reconnect);
      break;
    case PROP_MULTICAST_IFACE:
      g_value_set_string (value, rtspsrc->multi_iface);
      break;
    case PROP_NTP_SYNC:
      g_value_set_boolean (value, rtspsrc->ntp_sync);
      break;
    case PROP_USE_PIPELINE_CLOCK:
      g_value_set_boolean (value, rtspsrc->use_pipeline_clock);
      break;
    case PROP_SDES:
      g_value_set_boxed (value, rtspsrc->sdes);
      break;
    case PROP_TLS_VALIDATION_FLAGS:
      g_value_set_flags (value, rtspsrc->tls_validation_flags);
      break;
    case PROP_TLS_DATABASE:
      g_value_set_object (value, rtspsrc->tls_database);
      break;
    case PROP_TLS_INTERACTION:
      g_value_set_object (value, rtspsrc->tls_interaction);
      break;
    case PROP_DO_RETRANSMISSION:
      g_value_set_boolean (value, rtspsrc->do_retransmission);
      break;
    case PROP_NTP_TIME_SOURCE:
      g_value_set_enum (value, rtspsrc->ntp_time_source);
      break;
    case PROP_USER_AGENT:
      g_value_set_string (value, rtspsrc->user_agent);
      break;
    case PROP_MAX_RTCP_RTP_TIME_DIFF:
      g_value_set_int (value, rtspsrc->max_rtcp_rtp_time_diff);
      break;
    case PROP_RFC7273_SYNC:
      g_value_set_boolean (value, rtspsrc->rfc7273_sync);
      break;
    case PROP_MAX_TS_OFFSET_ADJUSTMENT:
      g_value_set_uint64 (value, rtspsrc->max_ts_offset_adjustment);
      break;
    case PROP_MAX_TS_OFFSET:
      g_value_set_int64 (value, rtspsrc->max_ts_offset);
      break;
    case PROP_DEFAULT_VERSION:
      g_value_set_enum (value, rtspsrc->default_version);
      break;
    case PROP_BACKCHANNEL:
      g_value_set_enum (value, rtspsrc->backchannel);
      break;
    case PROP_TEARDOWN_TIMEOUT:
      g_value_set_uint64 (value, rtspsrc->teardown_timeout);
      break;
    case PROP_ONVIF_MODE:
      g_value_set_boolean (value, rtspsrc->onvif_mode);
      break;
    case PROP_ONVIF_RATE_CONTROL:
      g_value_set_boolean (value, rtspsrc->onvif_rate_control);
      break;
    case PROP_IS_LIVE:
      g_value_set_boolean (value, rtspsrc->is_live);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gint
find_stream_by_id (GstRTSPStream * stream, gint * id)
{
  if (stream->id == *id)
    return 0;

  return -1;
}

static gint
find_stream_by_channel (GstRTSPStream * stream, gint * channel)
{
  /* ignore unconfigured channels here (e.g., those that
   * were explicitly skipped during SETUP) */
  if ((stream->channelpad[0] != NULL) &&
      (stream->channel[0] == *channel || stream->channel[1] == *channel))
    return 0;

  return -1;
}

static gint
find_stream_by_udpsrc (GstRTSPStream * stream, gconstpointer a)
{
  GstElement *src = (GstElement *) a;

  if (stream->udpsrc[0] == src)
    return 0;
  if (stream->udpsrc[1] == src)
    return 0;

  return -1;
}

static gint
find_stream_by_setup (GstRTSPStream * stream, gconstpointer a)
{
  if (stream->conninfo.location) {
    /* check qualified setup_url */
    if (!strcmp (stream->conninfo.location, (gchar *) a))
      return 0;
  }
  if (stream->control_url) {
    /* check original control_url */
    if (!strcmp (stream->control_url, (gchar *) a))
      return 0;

    /* check if qualified setup_url ends with string */
    if (g_str_has_suffix (stream->control_url, (gchar *) a))
      return 0;
  }

  return -1;
}

static GstRTSPStream *
find_stream (GstRTSPSrc * src, gconstpointer data, gconstpointer func)
{
  GList *lstream;

  /* find and get stream */
  if ((lstream = g_list_find_custom (src->streams, data, (GCompareFunc) func)))
    return (GstRTSPStream *) lstream->data;

  return NULL;
}

static const GstSDPBandwidth *
gst_rtspsrc_get_bandwidth (GstRTSPSrc * src, const GstSDPMessage * sdp,
    const GstSDPMedia * media, const gchar * type)
{
  guint i, len;

  /* first look in the media specific section */
  len = gst_sdp_media_bandwidths_len (media);
  for (i = 0; i < len; i++) {
    const GstSDPBandwidth *bw = gst_sdp_media_get_bandwidth (media, i);

    if (strcmp (bw->bwtype, type) == 0)
      return bw;
  }
  /* then look in the message specific section */
  len = gst_sdp_message_bandwidths_len (sdp);
  for (i = 0; i < len; i++) {
    const GstSDPBandwidth *bw = gst_sdp_message_get_bandwidth (sdp, i);

    if (strcmp (bw->bwtype, type) == 0)
      return bw;
  }
  return NULL;
}

static void
gst_rtspsrc_collect_bandwidth (GstRTSPSrc * src, const GstSDPMessage * sdp,
    const GstSDPMedia * media, GstRTSPStream * stream)
{
  const GstSDPBandwidth *bw;

  if ((bw = gst_rtspsrc_get_bandwidth (src, sdp, media, GST_SDP_BWTYPE_AS)))
    stream->as_bandwidth = bw->bandwidth;
  else
    stream->as_bandwidth = -1;

  if ((bw = gst_rtspsrc_get_bandwidth (src, sdp, media, GST_SDP_BWTYPE_RR)))
    stream->rr_bandwidth = bw->bandwidth;
  else
    stream->rr_bandwidth = -1;

  if ((bw = gst_rtspsrc_get_bandwidth (src, sdp, media, GST_SDP_BWTYPE_RS)))
    stream->rs_bandwidth = bw->bandwidth;
  else
    stream->rs_bandwidth = -1;
}

static void
gst_rtspsrc_do_stream_connection (GstRTSPSrc * src, GstRTSPStream * stream,
    const GstSDPConnection * conn)
{
  if (conn->nettype == NULL || strcmp (conn->nettype, "IN") != 0)
    return;

  if (conn->addrtype == NULL)
    return;

  /* check for IPV6 */
  if (strcmp (conn->addrtype, "IP4") == 0)
    stream->is_ipv6 = FALSE;
  else if (strcmp (conn->addrtype, "IP6") == 0)
    stream->is_ipv6 = TRUE;
  else
    return;

  /* save address */
  g_free (stream->destination);
  stream->destination = g_strdup (conn->address);

  /* check for multicast */
  stream->is_multicast =
      gst_sdp_address_is_multicast (conn->nettype, conn->addrtype,
      conn->address);
  stream->ttl = conn->ttl;
}

/* Go over the connections for a stream.
 * - If we are dealing with IPV6, we will setup IPV6 sockets for sending and
 *   receiving.
 * - If we are dealing with a localhost address, we disable multicast
 */
static void
gst_rtspsrc_collect_connections (GstRTSPSrc * src, const GstSDPMessage * sdp,
    const GstSDPMedia * media, GstRTSPStream * stream)
{
  const GstSDPConnection *conn;
  guint i, len;

  /* first look in the media specific section */
  len = gst_sdp_media_connections_len (media);
  for (i = 0; i < len; i++) {
    conn = gst_sdp_media_get_connection (media, i);

    gst_rtspsrc_do_stream_connection (src, stream, conn);
  }
  /* then look in the message specific section */
  if ((conn = gst_sdp_message_get_connection (sdp))) {
    gst_rtspsrc_do_stream_connection (src, stream, conn);
  }
}

static gchar *
make_stream_id (GstRTSPStream * stream, const GstSDPMedia * media)
{
  gchar *stream_id =
      g_strdup_printf ("%s:%d:%d:%s:%d", media->media, media->port,
      media->num_ports, media->proto, stream->default_pt);

  g_strcanon (stream_id, G_CSET_a_2_z G_CSET_A_2_Z G_CSET_DIGITS, ':');

  return stream_id;
}

/*   m=<media> <UDP port> RTP/AVP <payload>
 */
static void
gst_rtspsrc_collect_payloads (GstRTSPSrc * src, const GstSDPMessage * sdp,
    const GstSDPMedia * media, GstRTSPStream * stream)
{
  guint i, len;
  const gchar *proto;
  GstCaps *global_caps;

  /* get proto */
  proto = gst_sdp_media_get_proto (media);
  if (proto == NULL)
    goto no_proto;

  if (g_str_equal (proto, "RTP/AVP"))
    stream->profile = GST_RTSP_PROFILE_AVP;
  else if (g_str_equal (proto, "RTP/SAVP"))
    stream->profile = GST_RTSP_PROFILE_SAVP;
  else if (g_str_equal (proto, "RTP/AVPF"))
    stream->profile = GST_RTSP_PROFILE_AVPF;
  else if (g_str_equal (proto, "RTP/SAVPF"))
    stream->profile = GST_RTSP_PROFILE_SAVPF;
  else
    goto unknown_proto;

  if (gst_sdp_media_get_attribute_val (media, "sendonly") != NULL &&
      /* We want to setup caps for streams configured as backchannel */
      !stream->is_backchannel && src->backchannel != BACKCHANNEL_NONE)
    goto sendonly_media;

  /* Parse global SDP attributes once */
  global_caps = gst_caps_new_empty_simple ("application/x-unknown");
  GST_DEBUG ("mapping sdp session level attributes to caps");
  gst_sdp_message_attributes_to_caps (sdp, global_caps);
  GST_DEBUG ("mapping sdp media level attributes to caps");
  gst_sdp_media_attributes_to_caps (media, global_caps);

  /* Keep a copy of the SDP key management */
  gst_sdp_media_parse_keymgmt (media, &stream->mikey);
  if (stream->mikey == NULL)
    gst_sdp_message_parse_keymgmt (sdp, &stream->mikey);

  len = gst_sdp_media_formats_len (media);
  for (i = 0; i < len; i++) {
    gint pt;
    GstCaps *caps, *outcaps;
    GstStructure *s;
    const gchar *enc;
    PtMapItem item;

    pt = atoi (gst_sdp_media_get_format (media, i));

    GST_DEBUG_OBJECT (src, " looking at %d pt: %d", i, pt);

    /* convert caps */
    caps = gst_sdp_media_get_caps_from_media (media, pt);
    if (caps == NULL) {
      GST_WARNING_OBJECT (src, " skipping pt %d without caps", pt);
      continue;
    }

    /* do some tweaks */
    s = gst_caps_get_structure (caps, 0);
    if ((enc = gst_structure_get_string (s, "encoding-name"))) {
      stream->is_real = (strstr (enc, "-REAL") != NULL);
      if (strcmp (enc, "X-ASF-PF") == 0)
        stream->container = TRUE;
    }

    /* Merge in global caps */
    /* Intersect will merge in missing fields to the current caps */
    outcaps = gst_caps_intersect (caps, global_caps);
    gst_caps_unref (caps);

    /* the first pt will be the default */
    if (stream->ptmap->len == 0)
      stream->default_pt = pt;

    item.pt = pt;
    item.caps = outcaps;

    g_array_append_val (stream->ptmap, item);
  }

  stream->stream_id = make_stream_id (stream, media);

  gst_caps_unref (global_caps);
  return;

no_proto:
  {
    GST_ERROR_OBJECT (src, "can't find proto in media");
    return;
  }
unknown_proto:
  {
    GST_ERROR_OBJECT (src, "unknown proto in media: '%s'", proto);
    return;
  }
sendonly_media:
  {
    GST_DEBUG_OBJECT (src, "sendonly media ignored, no backchannel");
    return;
  }
}

static const gchar *
get_aggregate_control (GstRTSPSrc * src)
{
  const gchar *base;

  if (src->control)
    base = src->control;
  else if (src->content_base)
    base = src->content_base;
  else if (src->conninfo.url_str)
    base = src->conninfo.url_str;
  else
    base = "/";

  return base;
}

static void
clear_ptmap_item (PtMapItem * item)
{
  if (item->caps)
    gst_caps_unref (item->caps);
}

static GstRTSPStream *
gst_rtspsrc_create_stream (GstRTSPSrc * src, GstSDPMessage * sdp, gint idx,
    gint n_streams)
{
  GstRTSPStream *stream;
  const gchar *control_url;
  const GstSDPMedia *media;

  /* get media, should not return NULL */
  media = gst_sdp_message_get_media (sdp, idx);
  if (media == NULL)
    return NULL;

  stream = g_new0 (GstRTSPStream, 1);
  stream->parent = src;
  /* we mark the pad as not linked, we will mark it as OK when we add the pad to
   * the element. */
  stream->last_ret = GST_FLOW_NOT_LINKED;
  stream->added = FALSE;
  stream->setup = FALSE;
  stream->skipped = FALSE;
  stream->id = idx;
  stream->eos = FALSE;
  stream->discont = TRUE;
  stream->seqbase = -1;
  stream->timebase = -1;
  stream->send_ssrc = g_random_int ();
  stream->profile = GST_RTSP_PROFILE_AVP;
  stream->ptmap = g_array_new (FALSE, FALSE, sizeof (PtMapItem));
  stream->mikey = NULL;
  stream->stream_id = NULL;
  stream->is_backchannel = FALSE;
  g_mutex_init (&stream->conninfo.send_lock);
  g_mutex_init (&stream->conninfo.recv_lock);
  g_array_set_clear_func (stream->ptmap, (GDestroyNotify) clear_ptmap_item);

  /* stream is sendonly and onvif backchannel is requested */
  if (gst_sdp_media_get_attribute_val (media, "sendonly") != NULL &&
      src->backchannel != BACKCHANNEL_NONE)
    stream->is_backchannel = TRUE;

  /* collect bandwidth information for this steam. FIXME, configure in the RTP
   * session manager to scale RTCP. */
  gst_rtspsrc_collect_bandwidth (src, sdp, media, stream);

  /* collect connection info */
  gst_rtspsrc_collect_connections (src, sdp, media, stream);

  /* make the payload type map */
  gst_rtspsrc_collect_payloads (src, sdp, media, stream);

  /* collect port number */
  stream->port = gst_sdp_media_get_port (media);

  /* get control url to construct the setup url. The setup url is used to
   * configure the transport of the stream and is used to identity the stream in
   * the RTP-Info header field returned from PLAY. */
  control_url = gst_sdp_media_get_attribute_val (media, "control");
  if (control_url == NULL)
    control_url = gst_sdp_message_get_attribute_val_n (sdp, "control", 0);

  GST_DEBUG_OBJECT (src, "stream %d, (%p)", stream->id, stream);
  GST_DEBUG_OBJECT (src, " port: %d", stream->port);
  GST_DEBUG_OBJECT (src, " container: %d", stream->container);
  GST_DEBUG_OBJECT (src, " control: %s", GST_STR_NULL (control_url));

  /* RFC 2326, C.3: missing control_url permitted in case of a single stream */
  if (control_url == NULL && n_streams == 1) {
    control_url = "";
  }

  if (control_url != NULL) {
    stream->control_url = g_strdup (control_url);
    /* Build a fully qualified url using the content_base if any or by prefixing
     * the original request.
     * If the control_url starts with a '/' or a non rtsp: protocol we will most
     * likely build a URL that the server will fail to understand, this is ok,
     * we will fail then. */
    if (g_str_has_prefix (control_url, "rtsp://"))
      stream->conninfo.location = g_strdup (control_url);
    else {
      const gchar *base;
      gboolean has_slash;

      if (g_strcmp0 (control_url, "*") == 0)
        control_url = "";

      base = get_aggregate_control (src);

      /* check if the base ends or control starts with / */
      has_slash = g_str_has_prefix (control_url, "/");
      has_slash = has_slash || g_str_has_suffix (base, "/");

      /* concatenate the two strings, insert / when not present */
      stream->conninfo.location =
          g_strdup_printf ("%s%s%s", base, has_slash ? "" : "/", control_url);
    }
  }
  GST_DEBUG_OBJECT (src, " setup: %s",
      GST_STR_NULL (stream->conninfo.location));

  /* we keep track of all streams */
  src->streams = g_list_append (src->streams, stream);

  return stream;

  /* ERRORS */
}

static void
gst_rtspsrc_stream_free (GstRTSPSrc * src, GstRTSPStream * stream)
{
  gint i;

  GST_DEBUG_OBJECT (src, "free stream %p", stream);

  g_array_free (stream->ptmap, TRUE);

  g_free (stream->destination);
  g_free (stream->control_url);
  g_free (stream->conninfo.location);
  g_free (stream->stream_id);

  for (i = 0; i < 2; i++) {
    if (stream->udpsrc[i]) {
      gst_element_set_state (stream->udpsrc[i], GST_STATE_NULL);
      if (gst_object_has_as_parent (GST_OBJECT (stream->udpsrc[i]),
              GST_OBJECT (src)))
        gst_bin_remove (GST_BIN_CAST (src), stream->udpsrc[i]);
      gst_object_unref (stream->udpsrc[i]);
    }
    if (stream->channelpad[i])
      gst_object_unref (stream->channelpad[i]);

    if (stream->udpsink[i]) {
      gst_element_set_state (stream->udpsink[i], GST_STATE_NULL);
      if (gst_object_has_as_parent (GST_OBJECT (stream->udpsink[i]),
              GST_OBJECT (src)))
        gst_bin_remove (GST_BIN_CAST (src), stream->udpsink[i]);
      gst_object_unref (stream->udpsink[i]);
    }
  }
  if (stream->rtpsrc) {
    gst_element_set_state (stream->rtpsrc, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (src), stream->rtpsrc);
    gst_object_unref (stream->rtpsrc);
  }
  if (stream->srcpad) {
    gst_pad_set_active (stream->srcpad, FALSE);
    if (stream->added)
      gst_element_remove_pad (GST_ELEMENT_CAST (src), stream->srcpad);
  }
  if (stream->srtpenc)
    gst_object_unref (stream->srtpenc);
  if (stream->srtpdec)
    gst_object_unref (stream->srtpdec);
  if (stream->srtcpparams)
    gst_caps_unref (stream->srtcpparams);
  if (stream->mikey)
    gst_mikey_message_unref (stream->mikey);
  if (stream->rtcppad)
    gst_object_unref (stream->rtcppad);
  if (stream->session)
    g_object_unref (stream->session);
  if (stream->rtx_pt_map)
    gst_structure_free (stream->rtx_pt_map);

  g_mutex_clear (&stream->conninfo.send_lock);
  g_mutex_clear (&stream->conninfo.recv_lock);

  g_free (stream);
}

static void
gst_rtspsrc_cleanup (GstRTSPSrc * src)
{
  GList *walk;

  GST_DEBUG_OBJECT (src, "cleanup");

  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;

    gst_rtspsrc_stream_free (src, stream);
  }
  g_list_free (src->streams);
  src->streams = NULL;
  if (src->manager) {
    if (src->manager_sig_id) {
      g_signal_handler_disconnect (src->manager, src->manager_sig_id);
      src->manager_sig_id = 0;
    }
    gst_element_set_state (src->manager, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (src), src->manager);
    src->manager = NULL;
  }
  if (src->props)
    gst_structure_free (src->props);
  src->props = NULL;

  g_free (src->content_base);
  src->content_base = NULL;

  g_free (src->control);
  src->control = NULL;

  if (src->range)
    gst_rtsp_range_free (src->range);
  src->range = NULL;

  /* don't clear the SDP when it was used in the url */
  if (src->sdp && !src->from_sdp) {
    gst_sdp_message_free (src->sdp);
    src->sdp = NULL;
  }

  src->need_segment = FALSE;
  src->clip_out_segment = FALSE;

  if (src->provided_clock) {
    gst_object_unref (src->provided_clock);
    src->provided_clock = NULL;
  }

  /* free parameter requests queue */
  if (!g_queue_is_empty (&src->set_get_param_q))
    g_queue_free_full (&src->set_get_param_q, free_param_queue);

}

static gboolean
gst_rtspsrc_alloc_udp_ports (GstRTSPStream * stream,
    gint * rtpport, gint * rtcpport)
{
  GstRTSPSrc *src;
  GstStateChangeReturn ret;
  GstElement *udpsrc0, *udpsrc1;
  gint tmp_rtp, tmp_rtcp;
  guint count;
  const gchar *host;

  src = stream->parent;

  udpsrc0 = NULL;
  udpsrc1 = NULL;
  count = 0;

  /* Start at next port */
  tmp_rtp = src->next_port_num;

  if (stream->is_ipv6)
    host = "udp://[::0]";
  else
    host = "udp://0.0.0.0";

  /* try to allocate 2 UDP ports, the RTP port should be an even
   * number and the RTCP port should be the next (uneven) port */
again:

  if (tmp_rtp != 0 && src->client_port_range.max > 0 &&
      tmp_rtp >= src->client_port_range.max)
    goto no_ports;

  udpsrc0 = gst_element_make_from_uri (GST_URI_SRC, host, NULL, NULL);
  if (udpsrc0 == NULL)
    goto no_udp_protocol;
  g_object_set (G_OBJECT (udpsrc0), "port", tmp_rtp, "reuse", FALSE, NULL);

  if (src->udp_buffer_size != 0)
    g_object_set (G_OBJECT (udpsrc0), "buffer-size", src->udp_buffer_size,
        NULL);

  ret = gst_element_set_state (udpsrc0, GST_STATE_READY);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    if (tmp_rtp != 0) {
      GST_DEBUG_OBJECT (src, "Unable to make udpsrc from RTP port %d", tmp_rtp);

      tmp_rtp += 2;
      if (++count > src->retry)
        goto no_ports;

      GST_DEBUG_OBJECT (src, "free RTP udpsrc");
      gst_element_set_state (udpsrc0, GST_STATE_NULL);
      gst_object_unref (udpsrc0);
      udpsrc0 = NULL;

      GST_DEBUG_OBJECT (src, "retry %d", count);
      goto again;
    }
    goto no_udp_protocol;
  }

  g_object_get (G_OBJECT (udpsrc0), "port", &tmp_rtp, NULL);
  GST_DEBUG_OBJECT (src, "got RTP port %d", tmp_rtp);

  /* check if port is even */
  if ((tmp_rtp & 0x01) != 0) {
    /* port not even, close and allocate another */
    if (++count > src->retry)
      goto no_ports;

    GST_DEBUG_OBJECT (src, "RTP port not even");

    GST_DEBUG_OBJECT (src, "free RTP udpsrc");
    gst_element_set_state (udpsrc0, GST_STATE_NULL);
    gst_object_unref (udpsrc0);
    udpsrc0 = NULL;

    GST_DEBUG_OBJECT (src, "retry %d", count);
    tmp_rtp++;
    goto again;
  }

  /* allocate port+1 for RTCP now */
  udpsrc1 = gst_element_make_from_uri (GST_URI_SRC, host, NULL, NULL);
  if (udpsrc1 == NULL)
    goto no_udp_rtcp_protocol;

  /* set port */
  tmp_rtcp = tmp_rtp + 1;
  if (src->client_port_range.max > 0 && tmp_rtcp > src->client_port_range.max)
    goto no_ports;

  g_object_set (G_OBJECT (udpsrc1), "port", tmp_rtcp, "reuse", FALSE, NULL);

  GST_DEBUG_OBJECT (src, "starting RTCP on port %d", tmp_rtcp);
  ret = gst_element_set_state (udpsrc1, GST_STATE_READY);
  /* tmp_rtcp port is busy already : retry to make rtp/rtcp pair */
  if (ret == GST_STATE_CHANGE_FAILURE) {
    GST_DEBUG_OBJECT (src, "Unable to make udpsrc from RTCP port %d", tmp_rtcp);

    if (++count > src->retry)
      goto no_ports;

    GST_DEBUG_OBJECT (src, "free RTP udpsrc");
    gst_element_set_state (udpsrc0, GST_STATE_NULL);
    gst_object_unref (udpsrc0);
    udpsrc0 = NULL;

    GST_DEBUG_OBJECT (src, "free RTCP udpsrc");
    gst_element_set_state (udpsrc1, GST_STATE_NULL);
    gst_object_unref (udpsrc1);
    udpsrc1 = NULL;

    tmp_rtp += 2;
    GST_DEBUG_OBJECT (src, "retry %d", count);
    goto again;
  }

  /* all fine, do port check */
  g_object_get (G_OBJECT (udpsrc0), "port", rtpport, NULL);
  g_object_get (G_OBJECT (udpsrc1), "port", rtcpport, NULL);

  /* this should not happen... */
  if (*rtpport != tmp_rtp || *rtcpport != tmp_rtcp)
    goto port_error;

  /* we keep these elements, we configure all in configure_transport when the
   * server told us to really use the UDP ports. */
  stream->udpsrc[0] = gst_object_ref_sink (udpsrc0);
  stream->udpsrc[1] = gst_object_ref_sink (udpsrc1);
  gst_element_set_locked_state (stream->udpsrc[0], TRUE);
  gst_element_set_locked_state (stream->udpsrc[1], TRUE);

  /* keep track of next available port number when we have a range
   * configured */
  if (src->next_port_num != 0)
    src->next_port_num = tmp_rtcp + 1;

  return TRUE;

  /* ERRORS */
no_udp_protocol:
  {
    GST_DEBUG_OBJECT (src, "could not get UDP source");
    goto cleanup;
  }
no_ports:
  {
    GST_DEBUG_OBJECT (src, "could not allocate UDP port pair after %d retries",
        count);
    goto cleanup;
  }
no_udp_rtcp_protocol:
  {
    GST_DEBUG_OBJECT (src, "could not get UDP source for RTCP");
    goto cleanup;
  }
port_error:
  {
    GST_DEBUG_OBJECT (src, "ports don't match rtp: %d<->%d, rtcp: %d<->%d",
        tmp_rtp, *rtpport, tmp_rtcp, *rtcpport);
    goto cleanup;
  }
cleanup:
  {
    if (udpsrc0) {
      gst_element_set_state (udpsrc0, GST_STATE_NULL);
      gst_object_unref (udpsrc0);
    }
    if (udpsrc1) {
      gst_element_set_state (udpsrc1, GST_STATE_NULL);
      gst_object_unref (udpsrc1);
    }
    return FALSE;
  }
}

static void
gst_rtspsrc_set_state (GstRTSPSrc * src, GstState state)
{
  GList *walk;

  if (src->manager)
    gst_element_set_state (GST_ELEMENT_CAST (src->manager), state);

  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    gint i;

    for (i = 0; i < 2; i++) {
      if (stream->udpsrc[i])
        gst_element_set_state (stream->udpsrc[i], state);
    }
  }
}

static void
gst_rtspsrc_flush (GstRTSPSrc * src, gboolean flush, gboolean playing,
    guint32 seqnum)
{
  GstEvent *event;
  gint cmd;
  GstState state;

  if (flush) {
    event = gst_event_new_flush_start ();
    gst_event_set_seqnum (event, seqnum);
    GST_DEBUG_OBJECT (src, "start flush");
    cmd = CMD_WAIT;
    state = GST_STATE_PAUSED;
  } else {
    event = gst_event_new_flush_stop (TRUE);
    gst_event_set_seqnum (event, seqnum);
    GST_DEBUG_OBJECT (src, "stop flush; playing %d", playing);
    cmd = CMD_LOOP;
    if (playing)
      state = GST_STATE_PLAYING;
    else
      state = GST_STATE_PAUSED;
  }
  gst_rtspsrc_push_event (src, event);
  gst_rtspsrc_loop_send_cmd (src, cmd, CMD_LOOP);
  gst_rtspsrc_set_state (src, state);
}

static GstRTSPResult
gst_rtspsrc_connection_send (GstRTSPSrc * src, GstRTSPConnInfo * conninfo,
    GstRTSPMessage * message, GTimeVal * timeout)
{
  GstRTSPResult ret;

  if (conninfo->connection) {
    g_mutex_lock (&conninfo->send_lock);
    ret = gst_rtsp_connection_send (conninfo->connection, message, timeout);
    g_mutex_unlock (&conninfo->send_lock);
  } else {
    ret = GST_RTSP_ERROR;
  }

  return ret;
}

static GstRTSPResult
gst_rtspsrc_connection_receive (GstRTSPSrc * src, GstRTSPConnInfo * conninfo,
    GstRTSPMessage * message, GTimeVal * timeout)
{
  GstRTSPResult ret;

  if (conninfo->connection) {
    g_mutex_lock (&conninfo->recv_lock);
    ret = gst_rtsp_connection_receive (conninfo->connection, message, timeout);
    g_mutex_unlock (&conninfo->recv_lock);
  } else {
    ret = GST_RTSP_ERROR;
  }

  return ret;
}

static void
gst_rtspsrc_get_position (GstRTSPSrc * src)
{
  GstQuery *query;
  GList *walk;

  query = gst_query_new_position (GST_FORMAT_TIME);
  /*  should be known somewhere down the stream (e.g. jitterbuffer) */
  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    GstFormat fmt;
    gint64 pos;

    if (stream->srcpad) {
      if (gst_pad_query (stream->srcpad, query)) {
        gst_query_parse_position (query, &fmt, &pos);
        GST_DEBUG_OBJECT (src, "retaining position %" GST_TIME_FORMAT,
            GST_TIME_ARGS (pos));
        src->last_pos = pos;
        goto out;
      }
    }
  }

  src->last_pos = 0;

out:

  gst_query_unref (query);
}

static gboolean
gst_rtspsrc_perform_seek (GstRTSPSrc * src, GstEvent * event)
{
  gdouble rate;
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType cur_type = GST_SEEK_TYPE_NONE, stop_type;
  gint64 cur, stop;
  gboolean flush, server_side_trickmode;
  gboolean update;
  gboolean playing;
  GstSegment seeksegment = { 0, };
  GList *walk;
  const gchar *seek_style = NULL;

  GST_DEBUG_OBJECT (src, "doing seek with event %" GST_PTR_FORMAT, event);

  gst_event_parse_seek (event, &rate, &format, &flags,
      &cur_type, &cur, &stop_type, &stop);

  /* we need TIME format */
  if (format != src->segment.format)
    goto no_format;

  /* Check if we are not at all seekable */
  if (src->seekable == -1.0)
    goto not_seekable;

  /* Additional seeking-to-beginning-only check */
  if (src->seekable == 0.0 && cur != 0)
    goto not_seekable;

  if (flags & GST_SEEK_FLAG_SEGMENT)
    goto invalid_segment_flag;

  /* get flush flag */
  flush = flags & GST_SEEK_FLAG_FLUSH;
  server_side_trickmode = flags & GST_SEEK_FLAG_TRICKMODE;

  gst_event_parse_seek_trickmode_interval (event, &src->trickmode_interval);

  /* now we need to make sure the streaming thread is stopped. We do this by
   * either sending a FLUSH_START event downstream which will cause the
   * streaming thread to stop with a WRONG_STATE.
   * For a non-flushing seek we simply pause the task, which will happen as soon
   * as it completes one iteration (and thus might block when the sink is
   * blocking in preroll). */
  if (flush) {
    GST_DEBUG_OBJECT (src, "starting flush");
    gst_rtspsrc_flush (src, TRUE, FALSE, gst_event_get_seqnum (event));
  } else {
    if (src->task) {
      gst_task_pause (src->task);
    }
  }

  /* we should now be able to grab the streaming thread because we stopped it
   * with the above flush/pause code */
  GST_RTSP_STREAM_LOCK (src);

  GST_DEBUG_OBJECT (src, "stopped streaming");

  /* stop flushing the rtsp connection so we can send PAUSE/PLAY below */
  gst_rtspsrc_connection_flush (src, FALSE);

  /* copy segment, we need this because we still need the old
   * segment when we close the current segment. */
  memcpy (&seeksegment, &src->segment, sizeof (GstSegment));

  /* configure the seek parameters in the seeksegment. We will then have the
   * right values in the segment to perform the seek */
  GST_DEBUG_OBJECT (src, "configuring seek");
  seeksegment.duration = GST_CLOCK_TIME_NONE;
  gst_segment_do_seek (&seeksegment, rate, format, flags,
      cur_type, cur, stop_type, stop, &update);

  /* figure out the last position we need to play. If it's configured (stop !=
   * -1), use that, else we play until the total duration of the file */
  if ((stop = seeksegment.stop) == -1)
    stop = seeksegment.duration;

  /* if we were playing, pause first */
  playing = (src->state == GST_RTSP_STATE_PLAYING);
  if (playing) {
    /* obtain current position in case seek fails */
    gst_rtspsrc_get_position (src);
    gst_rtspsrc_pause (src, FALSE);
  }
  src->server_side_trickmode = server_side_trickmode;

  src->state = GST_RTSP_STATE_SEEKING;

  /* PLAY will add the range header now. */
  src->need_range = TRUE;

  /* prepare for streaming again */
  if (flush) {
    /* if we started flush, we stop now */
    GST_DEBUG_OBJECT (src, "stopping flush");
    gst_rtspsrc_flush (src, FALSE, playing, gst_event_get_seqnum (event));
  }

  /* now we did the seek and can activate the new segment values */
  memcpy (&src->segment, &seeksegment, sizeof (GstSegment));

  /* if we're doing a segment seek, post a SEGMENT_START message */
  if (src->segment.flags & GST_SEEK_FLAG_SEGMENT) {
    gst_element_post_message (GST_ELEMENT_CAST (src),
        gst_message_new_segment_start (GST_OBJECT_CAST (src),
            src->segment.format, src->segment.position));
  }

  /* now create the newsegment */
  GST_DEBUG_OBJECT (src, "Creating newsegment from %" G_GINT64_FORMAT
      " to %" G_GINT64_FORMAT, src->segment.position, stop);

  /* mark discont */
  GST_DEBUG_OBJECT (src, "mark DISCONT, we did a seek to another position");
  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    stream->discont = TRUE;
  }

  /* and continue playing if needed. If we are not acting as a live source,
   * then only the RTSP PLAYING state, set earlier, matters. */
  GST_OBJECT_LOCK (src);
  if (src->is_live) {
    playing = (GST_STATE_PENDING (src) == GST_STATE_VOID_PENDING
        && GST_STATE (src) == GST_STATE_PLAYING)
        || (GST_STATE_PENDING (src) == GST_STATE_PLAYING);
  }
  GST_OBJECT_UNLOCK (src);

  if (src->version >= GST_RTSP_VERSION_2_0) {
    if (flags & GST_SEEK_FLAG_ACCURATE)
      seek_style = "RAP";
    else if (flags & GST_SEEK_FLAG_KEY_UNIT)
      seek_style = "CoRAP";
    else if (flags & GST_SEEK_FLAG_KEY_UNIT
        && flags & GST_SEEK_FLAG_SNAP_BEFORE)
      seek_style = "First-Prior";
    else if (flags & GST_SEEK_FLAG_KEY_UNIT && flags & GST_SEEK_FLAG_SNAP_AFTER)
      seek_style = "Next";
  }

  /* If an accurate seek was requested, we want to clip the segment we
   * output in ONVIF mode to the requested bounds */
  src->clip_out_segment = ! !(flags & GST_SEEK_FLAG_ACCURATE);

  if (playing)
    gst_rtspsrc_play (src, &seeksegment, FALSE, seek_style);

  GST_RTSP_STREAM_UNLOCK (src);

  return TRUE;

  /* ERRORS */
no_format:
  {
    GST_DEBUG_OBJECT (src, "unsupported format given, seek aborted.");
    return FALSE;
  }
not_seekable:
  {
    GST_DEBUG_OBJECT (src, "stream is not seekable");
    return FALSE;
  }
invalid_segment_flag:
  {
    GST_WARNING_OBJECT (src, "Segment seeks not supported");
    return FALSE;
  }
}

static gboolean
gst_rtspsrc_handle_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRTSPSrc *src;
  gboolean res = TRUE;
  gboolean forward;

  src = GST_RTSPSRC_CAST (parent);

  GST_DEBUG_OBJECT (src, "pad %s:%s received event %s",
      GST_DEBUG_PAD_NAME (pad), GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      res = gst_rtspsrc_perform_seek (src, event);
      forward = FALSE;
      break;
    case GST_EVENT_QOS:
    case GST_EVENT_NAVIGATION:
    case GST_EVENT_LATENCY:
    default:
      forward = TRUE;
      break;
  }
  if (forward) {
    GstPad *target;

    if ((target = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad)))) {
      res = gst_pad_send_event (target, event);
      gst_object_unref (target);
    } else {
      gst_event_unref (event);
    }
  } else {
    gst_event_unref (event);
  }

  return res;
}

static gboolean
gst_rtspsrc_handle_src_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRTSPStream *stream;

  stream = gst_pad_get_element_private (pad);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_STREAM_START:{
      const gchar *upstream_id;
      gchar *stream_id;

      gst_event_parse_stream_start (event, &upstream_id);
      stream_id = g_strdup_printf ("%s/%s", upstream_id, stream->stream_id);

      gst_event_unref (event);
      event = gst_event_new_stream_start (stream_id);
      g_free (stream_id);
      break;
    }
    default:
      break;
  }

  return gst_pad_push_event (stream->srcpad, event);
}

/* this is the final event function we receive on the internal source pad when
 * we deal with TCP connections */
static gboolean
gst_rtspsrc_handle_internal_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res;

  GST_DEBUG_OBJECT (pad, "received event %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
    case GST_EVENT_QOS:
    case GST_EVENT_NAVIGATION:
    case GST_EVENT_LATENCY:
    default:
      gst_event_unref (event);
      res = TRUE;
      break;
  }
  return res;
}

/* this is the final query function we receive on the internal source pad when
 * we deal with TCP connections */
static gboolean
gst_rtspsrc_handle_internal_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstRTSPSrc *src;
  gboolean res = TRUE;

  src = GST_RTSPSRC_CAST (gst_pad_get_element_private (pad));

  GST_DEBUG_OBJECT (src, "pad %s:%s received query %s",
      GST_DEBUG_PAD_NAME (pad), GST_QUERY_TYPE_NAME (query));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:
    {
      /* no idea */
      break;
    }
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      switch (format) {
        case GST_FORMAT_TIME:
          gst_query_set_duration (query, format, src->segment.duration);
          break;
        default:
          res = FALSE;
          break;
      }
      break;
    }
    case GST_QUERY_LATENCY:
    {
      /* we are live with a min latency of 0 and unlimited max latency, this
       * result will be updated by the session manager if there is any. */
      gst_query_set_latency (query, src->is_live, 0, -1);
      break;
    }
    default:
      break;
  }

  return res;
}

/* this query is executed on the ghost source pad exposed on rtspsrc. */
static gboolean
gst_rtspsrc_handle_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstRTSPSrc *src;
  gboolean res = FALSE;

  src = GST_RTSPSRC_CAST (parent);

  GST_DEBUG_OBJECT (src, "pad %s:%s received query %s",
      GST_DEBUG_PAD_NAME (pad), GST_QUERY_TYPE_NAME (query));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      switch (format) {
        case GST_FORMAT_TIME:
          gst_query_set_duration (query, format, src->segment.duration);
          res = TRUE;
          break;
        default:
          break;
      }
      break;
    }
    case GST_QUERY_SEEKING:
    {
      GstFormat format;

      gst_query_parse_seeking (query, &format, NULL, NULL, NULL);
      if (format == GST_FORMAT_TIME) {
        gboolean seekable =
            src->cur_protocols != GST_RTSP_LOWER_TRANS_UDP_MCAST;
        GstClockTime start = 0, duration = src->segment.duration;

        /* seeking without duration is unlikely */
        seekable = seekable && src->seekable >= 0.0 && src->segment.duration &&
            GST_CLOCK_TIME_IS_VALID (src->segment.duration);

        if (seekable) {
          if (src->seekable > 0.0) {
            start = src->last_pos - src->seekable * GST_SECOND;
          } else {
            /* src->seekable == 0 means that we can only seek to 0 */
            start = 0;
            duration = 0;
          }
        }

        GST_LOG_OBJECT (src, "seekable : %d", seekable);

        gst_query_set_seeking (query, GST_FORMAT_TIME, seekable, start,
            duration);
        res = TRUE;
      }
      break;
    }
    case GST_QUERY_URI:
    {
      gchar *uri;

      uri = gst_rtspsrc_uri_get_uri (GST_URI_HANDLER (src));
      if (uri != NULL) {
        gst_query_set_uri (query, uri);
        g_free (uri);
        res = TRUE;
      }
      break;
    }
    default:
    {
      GstPad *target = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad));

      /* forward the query to the proxy target pad */
      if (target) {
        res = gst_pad_query (target, query);
        gst_object_unref (target);
      }
      break;
    }
  }

  return res;
}

/* callback for RTCP messages to be sent to the server when operating in TCP
 * mode. */
static GstFlowReturn
gst_rtspsrc_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstRTSPSrc *src;
  GstRTSPStream *stream;
  GstFlowReturn res = GST_FLOW_OK;
  GstRTSPResult ret;
  GstRTSPMessage message = { 0 };
  GstRTSPConnInfo *conninfo;

  stream = (GstRTSPStream *) gst_pad_get_element_private (pad);
  src = stream->parent;

  gst_rtsp_message_init_data (&message, stream->channel[1]);

  /* lend the body data to the message */
  gst_rtsp_message_set_body_buffer (&message, buffer);

  if (stream->conninfo.connection)
    conninfo = &stream->conninfo;
  else
    conninfo = &src->conninfo;

  GST_DEBUG_OBJECT (src, "sending %u bytes RTCP",
      (guint) gst_buffer_get_size (buffer));
  ret = gst_rtspsrc_connection_send (src, conninfo, &message, NULL);
  GST_DEBUG_OBJECT (src, "sent RTCP, %d", ret);

  gst_rtsp_message_unset (&message);

  gst_buffer_unref (buffer);

  return res;
}

static GstFlowReturn
gst_rtspsrc_push_backchannel_buffer (GstRTSPSrc * src, guint id,
    GstSample * sample)
{
  GstFlowReturn res = GST_FLOW_OK;
  GstRTSPStream *stream;

  if (!src->conninfo.connected || src->state != GST_RTSP_STATE_PLAYING)
    goto out;

  stream = find_stream (src, &id, (gpointer) find_stream_by_id);
  if (stream == NULL) {
    GST_ERROR_OBJECT (src, "no stream with id %u", id);
    goto out;
  }

  if (src->interleaved) {
    GstBuffer *buffer;
    GstRTSPResult ret;
    GstRTSPMessage message = { 0 };
    GstRTSPConnInfo *conninfo;

    buffer = gst_sample_get_buffer (sample);

    gst_rtsp_message_init_data (&message, stream->channel[0]);

    /* lend the body data to the message */
    gst_rtsp_message_set_body_buffer (&message, buffer);

    if (stream->conninfo.connection)
      conninfo = &stream->conninfo;
    else
      conninfo = &src->conninfo;

    GST_DEBUG_OBJECT (src, "sending %u bytes backchannel RTP",
        (guint) gst_buffer_get_size (buffer));
    ret = gst_rtspsrc_connection_send (src, conninfo, &message, NULL);
    GST_DEBUG_OBJECT (src, "sent backchannel RTP, %d", ret);

    gst_rtsp_message_unset (&message);

    res = GST_FLOW_OK;
  } else {
    g_signal_emit_by_name (stream->rtpsrc, "push-sample", sample, &res);
    GST_DEBUG_OBJECT (src, "sent backchannel RTP sample %p: %s", sample,
        gst_flow_get_name (res));
  }

out:
  gst_sample_unref (sample);

  return res;
}

static GstPadProbeReturn
pad_blocked (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstRTSPSrc *src = user_data;

  GST_DEBUG_OBJECT (src, "pad %s:%s blocked, activating streams",
      GST_DEBUG_PAD_NAME (pad));

  /* activate the streams */
  GST_OBJECT_LOCK (src);
  if (!src->need_activate)
    goto was_ok;

  src->need_activate = FALSE;
  GST_OBJECT_UNLOCK (src);

  gst_rtspsrc_activate_streams (src);

  return GST_PAD_PROBE_OK;

was_ok:
  {
    GST_OBJECT_UNLOCK (src);
    return GST_PAD_PROBE_OK;
  }
}

static GstPadProbeReturn
udpsrc_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  guint32 *segment_seqnum = user_data;

  switch (GST_EVENT_TYPE (info->data)) {
    case GST_EVENT_SEGMENT:
      if (!gst_event_is_writable (info->data))
        info->data = gst_event_make_writable (info->data);

      *segment_seqnum = gst_event_get_seqnum (info->data);
    default:
      break;
  }

  return GST_PAD_PROBE_OK;
}

static gboolean
copy_sticky_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  GstPad *gpad = GST_PAD_CAST (user_data);

  GST_DEBUG_OBJECT (gpad, "store sticky event %" GST_PTR_FORMAT, *event);
  gst_pad_store_sticky_event (gpad, *event);

  return TRUE;
}

static gboolean
add_backchannel_fakesink (GstRTSPSrc * src, GstRTSPStream * stream,
    GstPad * srcpad)
{
  GstPad *sinkpad;
  GstElement *fakesink;

  fakesink = gst_element_factory_make ("fakesink", NULL);
  if (fakesink == NULL) {
    GST_ERROR_OBJECT (src, "no fakesink");
    return FALSE;
  }

  sinkpad = gst_element_get_static_pad (fakesink, "sink");

  GST_DEBUG_OBJECT (src, "backchannel stream %p, hooking fakesink", stream);

  gst_bin_add (GST_BIN_CAST (src), fakesink);
  if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
    GST_WARNING_OBJECT (src, "could not link to fakesink");
    return FALSE;
  }

  gst_object_unref (sinkpad);

  gst_element_sync_state_with_parent (fakesink);
  return TRUE;
}

/* this callback is called when the session manager generated a new src pad with
 * payloaded RTP packets. We simply ghost the pad here. */
static void
new_manager_pad (GstElement * manager, GstPad * pad, GstRTSPSrc * src)
{
  gchar *name;
  GstPadTemplate *template;
  gint id, ssrc, pt;
  GList *ostreams;
  GstRTSPStream *stream;
  gboolean all_added;
  GstPad *internal_src;

  GST_DEBUG_OBJECT (src, "got new manager pad %" GST_PTR_FORMAT, pad);

  GST_RTSP_STATE_LOCK (src);
  /* find stream */
  name = gst_object_get_name (GST_OBJECT_CAST (pad));
  if (sscanf (name, "recv_rtp_src_%u_%u_%u", &id, &ssrc, &pt) != 3)
    goto unknown_stream;

  GST_DEBUG_OBJECT (src, "stream: %u, SSRC %08x, PT %d", id, ssrc, pt);

  stream = find_stream (src, &id, (gpointer) find_stream_by_id);
  if (stream == NULL)
    goto unknown_stream;

  /* save SSRC */
  stream->ssrc = ssrc;

  /* we'll add it later see below */
  stream->added = TRUE;

  /* check if we added all streams */
  all_added = TRUE;
  for (ostreams = src->streams; ostreams; ostreams = g_list_next (ostreams)) {
    GstRTSPStream *ostream = (GstRTSPStream *) ostreams->data;

    GST_DEBUG_OBJECT (src, "stream %p, container %d, added %d, setup %d",
        ostream, ostream->container, ostream->added, ostream->setup);

    /* if we find a stream for which we did a setup that is not added, we
     * need to wait some more */
    if (ostream->setup && !ostream->added) {
      all_added = FALSE;
      break;
    }
  }
  GST_RTSP_STATE_UNLOCK (src);

  /* create a new pad we will use to stream to */
  template = gst_static_pad_template_get (&rtptemplate);
  stream->srcpad = gst_ghost_pad_new_from_template (name, pad, template);
  gst_object_unref (template);
  g_free (name);

  /* We intercept and modify the stream start event */
  internal_src =
      GST_PAD (gst_proxy_pad_get_internal (GST_PROXY_PAD (stream->srcpad)));
  gst_pad_set_element_private (internal_src, stream);
  gst_pad_set_event_function (internal_src, gst_rtspsrc_handle_src_sink_event);
  gst_object_unref (internal_src);

  gst_pad_set_event_function (stream->srcpad, gst_rtspsrc_handle_src_event);
  gst_pad_set_query_function (stream->srcpad, gst_rtspsrc_handle_src_query);
  gst_pad_set_active (stream->srcpad, TRUE);
  gst_pad_sticky_events_foreach (pad, copy_sticky_events, stream->srcpad);

  /* don't add the srcpad if this is a sendonly stream */
  if (stream->is_backchannel)
    add_backchannel_fakesink (src, stream, stream->srcpad);
  else
    gst_element_add_pad (GST_ELEMENT_CAST (src), stream->srcpad);

  if (all_added) {
    GST_DEBUG_OBJECT (src, "We added all streams");
    /* when we get here, all stream are added and we can fire the no-more-pads
     * signal. */
    gst_element_no_more_pads (GST_ELEMENT_CAST (src));
  }

  return;

  /* ERRORS */
unknown_stream:
  {
    GST_DEBUG_OBJECT (src, "ignoring unknown stream");
    GST_RTSP_STATE_UNLOCK (src);
    g_free (name);
    return;
  }
}

static GstCaps *
stream_get_caps_for_pt (GstRTSPStream * stream, guint pt)
{
  guint i, len;

  len = stream->ptmap->len;
  for (i = 0; i < len; i++) {
    PtMapItem *item = &g_array_index (stream->ptmap, PtMapItem, i);
    if (item->pt == pt)
      return item->caps;
  }
  return NULL;
}

static GstCaps *
request_pt_map (GstElement * manager, guint session, guint pt, GstRTSPSrc * src)
{
  GstRTSPStream *stream;
  GstCaps *caps;

  GST_DEBUG_OBJECT (src, "getting pt map for pt %d in session %d", pt, session);

  GST_RTSP_STATE_LOCK (src);
  stream = find_stream (src, &session, (gpointer) find_stream_by_id);
  if (!stream)
    goto unknown_stream;

  if ((caps = stream_get_caps_for_pt (stream, pt)))
    gst_caps_ref (caps);
  GST_RTSP_STATE_UNLOCK (src);

  return caps;

unknown_stream:
  {
    GST_DEBUG_OBJECT (src, "unknown stream %d", session);
    GST_RTSP_STATE_UNLOCK (src);
    return NULL;
  }
}

static void
gst_rtspsrc_do_stream_eos (GstRTSPSrc * src, GstRTSPStream * stream)
{
  GST_DEBUG_OBJECT (src, "setting stream for session %u to EOS", stream->id);

  if (stream->eos)
    goto was_eos;

  stream->eos = TRUE;
  gst_rtspsrc_stream_push_event (src, stream, gst_event_new_eos ());
  return;

  /* ERRORS */
was_eos:
  {
    GST_DEBUG_OBJECT (src, "stream for session %u was already EOS", stream->id);
    return;
  }
}

static void
on_bye_ssrc (GObject * session, GObject * source, GstRTSPStream * stream)
{
  GstRTSPSrc *src = stream->parent;
  guint ssrc;

  g_object_get (source, "ssrc", &ssrc, NULL);

  GST_DEBUG_OBJECT (src, "source %08x, stream %08x, session %u received BYE",
      ssrc, stream->ssrc, stream->id);

  if (ssrc == stream->ssrc)
    gst_rtspsrc_do_stream_eos (src, stream);
}

static void
on_timeout_common (GObject * session, GObject * source, GstRTSPStream * stream)
{
  GstRTSPSrc *src = stream->parent;
  guint ssrc;

  g_object_get (source, "ssrc", &ssrc, NULL);

  GST_WARNING_OBJECT (src, "source %08x, stream %08x in session %u timed out",
      ssrc, stream->ssrc, stream->id);

  if (ssrc == stream->ssrc)
    gst_rtspsrc_do_stream_eos (src, stream);
}

static void
on_timeout (GObject * session, GObject * source, GstRTSPStream * stream)
{
  GstRTSPSrc *src = stream->parent;

  /* timeout, post element message */
  gst_element_post_message (GST_ELEMENT_CAST (src),
      gst_message_new_element (GST_OBJECT_CAST (src),
          gst_structure_new ("GstRTSPSrcTimeout",
              "cause", G_TYPE_ENUM, GST_RTSP_SRC_TIMEOUT_CAUSE_RTCP,
              "stream-number", G_TYPE_INT, stream->id, "ssrc", G_TYPE_UINT,
              stream->ssrc, NULL)));

  /* In non-live mode, timeouts can occur if we are PAUSED, this doesn't mean
   * the stream is EOS, it may simply be blocked */
  if (src->is_live || !src->interleaved)
    on_timeout_common (session, source, stream);
}

static void
on_npt_stop (GstElement * rtpbin, guint session, guint ssrc, GstRTSPSrc * src)
{
  GstRTSPStream *stream;

  GST_DEBUG_OBJECT (src, "source in session %u reached NPT stop", session);

  /* get stream for session */
  stream = find_stream (src, &session, (gpointer) find_stream_by_id);
  if (stream) {
    gst_rtspsrc_do_stream_eos (src, stream);
  }
}

static void
on_ssrc_active (GObject * session, GObject * source, GstRTSPStream * stream)
{
  GST_DEBUG_OBJECT (stream->parent, "source in session %u is active",
      stream->id);
}

static void
set_manager_buffer_mode (GstRTSPSrc * src)
{
  GObjectClass *klass;

  if (src->manager == NULL)
    return;

  klass = G_OBJECT_GET_CLASS (G_OBJECT (src->manager));

  if (!g_object_class_find_property (klass, "buffer-mode"))
    return;

  if (src->buffer_mode != BUFFER_MODE_AUTO) {
    g_object_set (src->manager, "buffer-mode", src->buffer_mode, NULL);

    return;
  }

  GST_DEBUG_OBJECT (src,
      "auto buffering mode, have clock %" GST_PTR_FORMAT, src->provided_clock);

  if (src->provided_clock) {
    GstClock *clock = gst_element_get_clock (GST_ELEMENT_CAST (src));

    if (clock == src->provided_clock) {
      GST_DEBUG_OBJECT (src, "selected synced");
      g_object_set (src->manager, "buffer-mode", BUFFER_MODE_SYNCED, NULL);

      if (clock)
        gst_object_unref (clock);

      return;
    }

    /* Otherwise fall-through and use another buffer mode */
    if (clock)
      gst_object_unref (clock);
  }

  GST_DEBUG_OBJECT (src, "auto buffering mode");
  if (src->use_buffering) {
    GST_DEBUG_OBJECT (src, "selected buffer");
    g_object_set (src->manager, "buffer-mode", BUFFER_MODE_BUFFER, NULL);
  } else {
    GST_DEBUG_OBJECT (src, "selected slave");
    g_object_set (src->manager, "buffer-mode", BUFFER_MODE_SLAVE, NULL);
  }
}

static GstCaps *
request_key (GstElement * srtpdec, guint ssrc, GstRTSPStream * stream)
{
  guint i;
  GstCaps *caps;
  GstMIKEYMessage *msg = stream->mikey;

  GST_DEBUG ("request key SSRC %u", ssrc);

  caps = gst_caps_ref (stream_get_caps_for_pt (stream, stream->default_pt));
  caps = gst_caps_make_writable (caps);

  /* parse crypto sessions and look for the SSRC rollover counter */
  msg = stream->mikey;
  for (i = 0; msg && i < gst_mikey_message_get_n_cs (msg); i++) {
    const GstMIKEYMapSRTP *map = gst_mikey_message_get_cs_srtp (msg, i);

    if (ssrc == map->ssrc) {
      gst_caps_set_simple (caps, "roc", G_TYPE_UINT, map->roc, NULL);
      break;
    }
  }

  return caps;
}

static GstElement *
request_rtp_decoder (GstElement * rtpbin, guint session, GstRTSPStream * stream)
{
  GST_DEBUG ("decoder session %u, stream %p, %d", session, stream, stream->id);
  if (stream->id != session)
    return NULL;

  if (stream->profile != GST_RTSP_PROFILE_SAVP &&
      stream->profile != GST_RTSP_PROFILE_SAVPF)
    return NULL;

  if (stream->srtpdec == NULL) {
    gchar *name;

    name = g_strdup_printf ("srtpdec_%u", session);
    stream->srtpdec = gst_element_factory_make ("srtpdec", name);
    g_free (name);

    if (stream->srtpdec == NULL) {
      GST_ELEMENT_ERROR (stream->parent, CORE, MISSING_PLUGIN, (NULL),
          ("no srtpdec element present!"));
      return NULL;
    }
    g_signal_connect (stream->srtpdec, "request-key",
        (GCallback) request_key, stream);
  }
  return gst_object_ref (stream->srtpdec);
}

static GstElement *
request_rtcp_encoder (GstElement * rtpbin, guint session,
    GstRTSPStream * stream)
{
  gchar *name;
  GstPad *pad;

  GST_DEBUG ("decoder session %u, stream %p, %d", session, stream, stream->id);
  if (stream->id != session)
    return NULL;

  if (stream->profile != GST_RTSP_PROFILE_SAVP &&
      stream->profile != GST_RTSP_PROFILE_SAVPF)
    return NULL;

  if (stream->srtpenc == NULL) {
    GstStructure *s;

    name = g_strdup_printf ("srtpenc_%u", session);
    stream->srtpenc = gst_element_factory_make ("srtpenc", name);
    g_free (name);

    if (stream->srtpenc == NULL) {
      GST_ELEMENT_ERROR (stream->parent, CORE, MISSING_PLUGIN, (NULL),
          ("no srtpenc element present!"));
      return NULL;
    }

    /* get RTCP crypto parameters from caps */
    s = gst_caps_get_structure (stream->srtcpparams, 0);
    if (s) {
      GstBuffer *buf;
      const gchar *str;
      GType ciphertype, authtype;
      GValue rtcp_cipher = G_VALUE_INIT, rtcp_auth = G_VALUE_INIT;

      ciphertype = g_type_from_name ("GstSrtpCipherType");
      authtype = g_type_from_name ("GstSrtpAuthType");
      g_value_init (&rtcp_cipher, ciphertype);
      g_value_init (&rtcp_auth, authtype);

      str = gst_structure_get_string (s, "srtcp-cipher");
      gst_value_deserialize (&rtcp_cipher, str);
      str = gst_structure_get_string (s, "srtcp-auth");
      gst_value_deserialize (&rtcp_auth, str);
      gst_structure_get (s, "srtp-key", GST_TYPE_BUFFER, &buf, NULL);

      g_object_set_property (G_OBJECT (stream->srtpenc), "rtp-cipher",
          &rtcp_cipher);
      g_object_set_property (G_OBJECT (stream->srtpenc), "rtp-auth",
          &rtcp_auth);
      g_object_set_property (G_OBJECT (stream->srtpenc), "rtcp-cipher",
          &rtcp_cipher);
      g_object_set_property (G_OBJECT (stream->srtpenc), "rtcp-auth",
          &rtcp_auth);
      g_object_set (stream->srtpenc, "key", buf, NULL);

      g_value_unset (&rtcp_cipher);
      g_value_unset (&rtcp_auth);
      gst_buffer_unref (buf);
    }
  }
  name = g_strdup_printf ("rtcp_sink_%d", session);
  pad = gst_element_get_request_pad (stream->srtpenc, name);
  g_free (name);
  gst_object_unref (pad);

  return gst_object_ref (stream->srtpenc);
}

static GstElement *
request_aux_receiver (GstElement * rtpbin, guint sessid, GstRTSPSrc * src)
{
  GstElement *rtx, *bin;
  GstPad *pad;
  gchar *name;
  GstRTSPStream *stream;

  stream = find_stream (src, &sessid, (gpointer) find_stream_by_id);
  if (!stream) {
    GST_WARNING_OBJECT (src, "Stream %u not found", sessid);
    return NULL;
  }

  GST_INFO_OBJECT (src, "creating retransmision receiver for session %u "
      "with map %" GST_PTR_FORMAT, sessid, stream->rtx_pt_map);
  bin = gst_bin_new (NULL);
  rtx = gst_element_factory_make ("rtprtxreceive", NULL);
  g_object_set (rtx, "payload-type-map", stream->rtx_pt_map, NULL);
  gst_bin_add (GST_BIN (bin), rtx);

  pad = gst_element_get_static_pad (rtx, "src");
  name = g_strdup_printf ("src_%u", sessid);
  gst_element_add_pad (bin, gst_ghost_pad_new (name, pad));
  g_free (name);
  gst_object_unref (pad);

  pad = gst_element_get_static_pad (rtx, "sink");
  name = g_strdup_printf ("sink_%u", sessid);
  gst_element_add_pad (bin, gst_ghost_pad_new (name, pad));
  g_free (name);
  gst_object_unref (pad);

  return bin;
}

static void
add_retransmission (GstRTSPSrc * src, GstRTSPTransport * transport)
{
  GList *walk;
  guint signal_id;
  gboolean do_retransmission = FALSE;

  if (transport->trans != GST_RTSP_TRANS_RTP)
    return;
  if (transport->profile != GST_RTSP_PROFILE_AVPF &&
      transport->profile != GST_RTSP_PROFILE_SAVPF)
    return;

  signal_id = g_signal_lookup ("request-aux-receiver",
      G_OBJECT_TYPE (src->manager));
  /* there's already something connected */
  if (g_signal_handler_find (src->manager, G_SIGNAL_MATCH_ID, signal_id, 0,
          NULL, NULL, NULL) != 0) {
    GST_DEBUG_OBJECT (src, "Not adding RTX AUX element as "
        "\"request-aux-receiver\" signal is "
        "already used by the application");
    return;
  }

  /* build the retransmission payload type map */
  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    gboolean do_retransmission_stream = FALSE;
    int i;

    if (stream->rtx_pt_map)
      gst_structure_free (stream->rtx_pt_map);
    stream->rtx_pt_map = gst_structure_new_empty ("application/x-rtp-pt-map");

    for (i = 0; i < stream->ptmap->len; i++) {
      PtMapItem *item = &g_array_index (stream->ptmap, PtMapItem, i);
      GstStructure *s = gst_caps_get_structure (item->caps, 0);
      const gchar *encoding;

      /* we only care about RTX streams */
      if ((encoding = gst_structure_get_string (s, "encoding-name"))
          && g_strcmp0 (encoding, "RTX") == 0) {
        const gchar *stream_pt_s;
        gint rtx_pt;

        if (gst_structure_get_int (s, "payload", &rtx_pt)
            && (stream_pt_s = gst_structure_get_string (s, "apt"))) {

          if (rtx_pt != 0) {
            gst_structure_set (stream->rtx_pt_map, stream_pt_s, G_TYPE_UINT,
                rtx_pt, NULL);
            do_retransmission_stream = TRUE;
          }
        }
      }
    }

    if (do_retransmission_stream) {
      GST_DEBUG_OBJECT (src, "built retransmission payload map for stream "
          "id %i: %" GST_PTR_FORMAT, stream->id, stream->rtx_pt_map);
      do_retransmission = TRUE;
    } else {
      GST_DEBUG_OBJECT (src, "no retransmission payload map for stream "
          "id %i", stream->id);
      gst_structure_free (stream->rtx_pt_map);
      stream->rtx_pt_map = NULL;
    }
  }

  if (do_retransmission) {
    GST_DEBUG_OBJECT (src, "Enabling retransmissions");

    g_object_set (src->manager, "do-retransmission", TRUE, NULL);

    /* enable RFC4588 retransmission handling by setting rtprtxreceive
     * as the "aux" element of rtpbin */
    g_signal_connect (src->manager, "request-aux-receiver",
        (GCallback) request_aux_receiver, src);
  } else {
    GST_DEBUG_OBJECT (src,
        "Not enabling retransmissions as no stream had a retransmission payload map");
  }
}

/* try to get and configure a manager */
static gboolean
gst_rtspsrc_stream_configure_manager (GstRTSPSrc * src, GstRTSPStream * stream,
    GstRTSPTransport * transport)
{
  const gchar *manager;
  gchar *name;
  GstStateChangeReturn ret;

  if (!src->is_live)
    goto use_no_manager;

  /* find a manager */
  if (gst_rtsp_transport_get_manager (transport->trans, &manager, 0) < 0)
    goto no_manager;

  if (manager) {
    GST_DEBUG_OBJECT (src, "using manager %s", manager);

    /* configure the manager */
    if (src->manager == NULL) {
      GObjectClass *klass;

      if (!(src->manager = gst_element_factory_make (manager, "manager"))) {
        /* fallback */
        if (gst_rtsp_transport_get_manager (transport->trans, &manager, 1) < 0)
          goto no_manager;

        if (!manager)
          goto use_no_manager;

        if (!(src->manager = gst_element_factory_make (manager, "manager")))
          goto manager_failed;
      }

      /* we manage this element */
      gst_element_set_locked_state (src->manager, TRUE);
      gst_bin_add (GST_BIN_CAST (src), src->manager);

      ret = gst_element_set_state (src->manager, GST_STATE_PAUSED);
      if (ret == GST_STATE_CHANGE_FAILURE)
        goto start_manager_failure;

      g_object_set (src->manager, "latency", src->latency, NULL);

      klass = G_OBJECT_GET_CLASS (G_OBJECT (src->manager));

      if (g_object_class_find_property (klass, "ntp-sync")) {
        g_object_set (src->manager, "ntp-sync", src->ntp_sync, NULL);
      }

      if (g_object_class_find_property (klass, "rfc7273-sync")) {
        g_object_set (src->manager, "rfc7273-sync", src->rfc7273_sync, NULL);
      }

      if (src->use_pipeline_clock) {
        if (g_object_class_find_property (klass, "use-pipeline-clock")) {
          g_object_set (src->manager, "use-pipeline-clock", TRUE, NULL);
        }
      } else {
        if (g_object_class_find_property (klass, "ntp-time-source")) {
          g_object_set (src->manager, "ntp-time-source", src->ntp_time_source,
              NULL);
        }
      }

      if (src->sdes && g_object_class_find_property (klass, "sdes")) {
        g_object_set (src->manager, "sdes", src->sdes, NULL);
      }

      if (g_object_class_find_property (klass, "drop-on-latency")) {
        g_object_set (src->manager, "drop-on-latency", src->drop_on_latency,
            NULL);
      }

      if (g_object_class_find_property (klass, "max-rtcp-rtp-time-diff")) {
        g_object_set (src->manager, "max-rtcp-rtp-time-diff",
            src->max_rtcp_rtp_time_diff, NULL);
      }

      if (g_object_class_find_property (klass, "max-ts-offset-adjustment")) {
        g_object_set (src->manager, "max-ts-offset-adjustment",
            src->max_ts_offset_adjustment, NULL);
      }

      if (g_object_class_find_property (klass, "max-ts-offset")) {
        gint64 max_ts_offset;

        /* setting max-ts-offset in the manager has side effects so only do it
         * if the value differs */
        g_object_get (src->manager, "max-ts-offset", &max_ts_offset, NULL);
        if (max_ts_offset != src->max_ts_offset) {
          g_object_set (src->manager, "max-ts-offset", src->max_ts_offset,
              NULL);
        }
      }

      /* buffer mode pauses are handled by adding offsets to buffer times,
       * but some depayloaders may have a hard time syncing output times
       * with such input times, e.g. container ones, most notably ASF */
      /* TODO alternatives are having an event that indicates these shifts,
       * or having rtsp extensions provide suggestion on buffer mode */
      /* valid duration implies not likely live pipeline,
       * so slaving in jitterbuffer does not make much sense
       * (and might mess things up due to bursts) */
      if (GST_CLOCK_TIME_IS_VALID (src->segment.duration) &&
          src->segment.duration && stream->container) {
        src->use_buffering = TRUE;
      } else {
        src->use_buffering = FALSE;
      }

      set_manager_buffer_mode (src);

      /* connect to signals */
      GST_DEBUG_OBJECT (src, "connect to signals on session manager, stream %p",
          stream);
      src->manager_sig_id =
          g_signal_connect (src->manager, "pad-added",
          (GCallback) new_manager_pad, src);
      src->manager_ptmap_id =
          g_signal_connect (src->manager, "request-pt-map",
          (GCallback) request_pt_map, src);

      g_signal_connect (src->manager, "on-npt-stop", (GCallback) on_npt_stop,
          src);

      g_signal_emit (src, gst_rtspsrc_signals[SIGNAL_NEW_MANAGER], 0,
          src->manager);

      if (src->do_retransmission)
        add_retransmission (src, transport);
    }
    g_signal_connect (src->manager, "request-rtp-decoder",
        (GCallback) request_rtp_decoder, stream);
    g_signal_connect (src->manager, "request-rtcp-decoder",
        (GCallback) request_rtp_decoder, stream);
    g_signal_connect (src->manager, "request-rtcp-encoder",
        (GCallback) request_rtcp_encoder, stream);

    /* we stream directly to the manager, get some pads. Each RTSP stream goes
     * into a separate RTP session. */
    name = g_strdup_printf ("recv_rtp_sink_%u", stream->id);
    stream->channelpad[0] = gst_element_get_request_pad (src->manager, name);
    g_free (name);
    name = g_strdup_printf ("recv_rtcp_sink_%u", stream->id);
    stream->channelpad[1] = gst_element_get_request_pad (src->manager, name);
    g_free (name);

    /* now configure the bandwidth in the manager */
    if (g_signal_lookup ("get-internal-session",
            G_OBJECT_TYPE (src->manager)) != 0) {
      GObject *rtpsession;

      g_signal_emit_by_name (src->manager, "get-internal-session", stream->id,
          &rtpsession);
      if (rtpsession) {
        GstRTPProfile rtp_profile;

        GST_INFO_OBJECT (src, "configure bandwidth in session %p", rtpsession);

        stream->session = rtpsession;

        if (stream->as_bandwidth != -1) {
          GST_INFO_OBJECT (src, "setting AS: %f",
              (gdouble) (stream->as_bandwidth * 1000));
          g_object_set (rtpsession, "bandwidth",
              (gdouble) (stream->as_bandwidth * 1000), NULL);
        }
        if (stream->rr_bandwidth != -1) {
          GST_INFO_OBJECT (src, "setting RR: %u", stream->rr_bandwidth);
          g_object_set (rtpsession, "rtcp-rr-bandwidth", stream->rr_bandwidth,
              NULL);
        }
        if (stream->rs_bandwidth != -1) {
          GST_INFO_OBJECT (src, "setting RS: %u", stream->rs_bandwidth);
          g_object_set (rtpsession, "rtcp-rs-bandwidth", stream->rs_bandwidth,
              NULL);
        }

        switch (stream->profile) {
          case GST_RTSP_PROFILE_AVPF:
            rtp_profile = GST_RTP_PROFILE_AVPF;
            break;
          case GST_RTSP_PROFILE_SAVP:
            rtp_profile = GST_RTP_PROFILE_SAVP;
            break;
          case GST_RTSP_PROFILE_SAVPF:
            rtp_profile = GST_RTP_PROFILE_SAVPF;
            break;
          case GST_RTSP_PROFILE_AVP:
          default:
            rtp_profile = GST_RTP_PROFILE_AVP;
            break;
        }

        g_object_set (rtpsession, "rtp-profile", rtp_profile, NULL);

        g_object_set (rtpsession, "probation", src->probation, NULL);

        g_object_set (rtpsession, "internal-ssrc", stream->send_ssrc, NULL);

        g_signal_connect (rtpsession, "on-bye-ssrc", (GCallback) on_bye_ssrc,
            stream);
        g_signal_connect (rtpsession, "on-bye-timeout",
            (GCallback) on_timeout_common, stream);
        g_signal_connect (rtpsession, "on-timeout", (GCallback) on_timeout,
            stream);
        g_signal_connect (rtpsession, "on-ssrc-active",
            (GCallback) on_ssrc_active, stream);
      }
    }
  }

use_no_manager:
  return TRUE;

  /* ERRORS */
no_manager:
  {
    GST_DEBUG_OBJECT (src, "cannot get a session manager");
    return FALSE;
  }
manager_failed:
  {
    GST_DEBUG_OBJECT (src, "no session manager element %s found", manager);
    return FALSE;
  }
start_manager_failure:
  {
    GST_DEBUG_OBJECT (src, "could not start session manager");
    return FALSE;
  }
}

/* free the UDP sources allocated when negotiating a transport.
 * This function is called when the server negotiated to a transport where the
 * UDP sources are not needed anymore, such as TCP or multicast. */
static void
gst_rtspsrc_stream_free_udp (GstRTSPStream * stream)
{
  gint i;

  for (i = 0; i < 2; i++) {
    if (stream->udpsrc[i]) {
      GST_DEBUG ("free UDP source %d for stream %p", i, stream);
      gst_element_set_state (stream->udpsrc[i], GST_STATE_NULL);
      gst_object_unref (stream->udpsrc[i]);
      stream->udpsrc[i] = NULL;
    }
  }
}

/* for TCP, create pads to send and receive data to and from the manager and to
 * intercept various events and queries
 */
static gboolean
gst_rtspsrc_stream_configure_tcp (GstRTSPSrc * src, GstRTSPStream * stream,
    GstRTSPTransport * transport, GstPad ** outpad)
{
  gchar *name;
  GstPadTemplate *template;
  GstPad *pad0, *pad1;

  /* configure for interleaved delivery, nothing needs to be done
   * here, the loop function will call the chain functions of the
   * session manager. */
  stream->channel[0] = transport->interleaved.min;
  stream->channel[1] = transport->interleaved.max;
  GST_DEBUG_OBJECT (src, "stream %p on channels %d-%d", stream,
      stream->channel[0], stream->channel[1]);

  /* we can remove the allocated UDP ports now */
  gst_rtspsrc_stream_free_udp (stream);

  /* no session manager, send data to srcpad directly */
  if (!stream->channelpad[0]) {
    GST_DEBUG_OBJECT (src, "no manager, creating pad");

    /* create a new pad we will use to stream to */
    name = g_strdup_printf ("stream_%u", stream->id);
    template = gst_static_pad_template_get (&rtptemplate);
    stream->channelpad[0] = gst_pad_new_from_template (template, name);
    gst_object_unref (template);
    g_free (name);

    /* set caps and activate */
    gst_pad_use_fixed_caps (stream->channelpad[0]);
    gst_pad_set_active (stream->channelpad[0], TRUE);

    *outpad = gst_object_ref (stream->channelpad[0]);
  } else {
    GST_DEBUG_OBJECT (src, "using manager source pad");

    template = gst_static_pad_template_get (&anysrctemplate);

    /* allocate pads for sending the channel data into the manager */
    pad0 = gst_pad_new_from_template (template, "internalsrc_0");
    gst_pad_link_full (pad0, stream->channelpad[0], GST_PAD_LINK_CHECK_NOTHING);
    gst_object_unref (stream->channelpad[0]);
    stream->channelpad[0] = pad0;
    gst_pad_set_event_function (pad0, gst_rtspsrc_handle_internal_src_event);
    gst_pad_set_query_function (pad0, gst_rtspsrc_handle_internal_src_query);
    gst_pad_set_element_private (pad0, src);
    gst_pad_set_active (pad0, TRUE);

    if (stream->channelpad[1]) {
      /* if we have a sinkpad for the other channel, create a pad and link to the
       * manager. */
      pad1 = gst_pad_new_from_template (template, "internalsrc_1");
      gst_pad_set_event_function (pad1, gst_rtspsrc_handle_internal_src_event);
      gst_pad_link_full (pad1, stream->channelpad[1],
          GST_PAD_LINK_CHECK_NOTHING);
      gst_object_unref (stream->channelpad[1]);
      stream->channelpad[1] = pad1;
      gst_pad_set_active (pad1, TRUE);
    }
    gst_object_unref (template);
  }
  /* setup RTCP transport back to the server if we have to. */
  if (src->manager && src->do_rtcp) {
    GstPad *pad;

    template = gst_static_pad_template_get (&anysinktemplate);

    stream->rtcppad = gst_pad_new_from_template (template, "internalsink_0");
    gst_pad_set_chain_function (stream->rtcppad, gst_rtspsrc_sink_chain);
    gst_pad_set_element_private (stream->rtcppad, stream);
    gst_pad_set_active (stream->rtcppad, TRUE);

    /* get session RTCP pad */
    name = g_strdup_printf ("send_rtcp_src_%u", stream->id);
    pad = gst_element_get_request_pad (src->manager, name);
    g_free (name);

    /* and link */
    if (pad) {
      gst_pad_link_full (pad, stream->rtcppad, GST_PAD_LINK_CHECK_NOTHING);
      gst_object_unref (pad);
    }

    gst_object_unref (template);
  }
  return TRUE;
}

static void
gst_rtspsrc_get_transport_info (GstRTSPSrc * src, GstRTSPStream * stream,
    GstRTSPTransport * transport, const gchar ** destination, gint * min,
    gint * max, guint * ttl)
{
  if (transport->lower_transport == GST_RTSP_LOWER_TRANS_UDP_MCAST) {
    if (destination) {
      if (!(*destination = transport->destination))
        *destination = stream->destination;
    }
    if (min && max) {
      /* transport first */
      *min = transport->port.min;
      *max = transport->port.max;
      if (*min == -1 && *max == -1) {
        /* then try from SDP */
        if (stream->port != 0) {
          *min = stream->port;
          *max = stream->port + 1;
        }
      }
    }

    if (ttl) {
      if (!(*ttl = transport->ttl))
        *ttl = stream->ttl;
    }
  } else {
    if (destination) {
      /* first take the source, then the endpoint to figure out where to send
       * the RTCP. */
      if (!(*destination = transport->source)) {
        if (src->conninfo.connection)
          *destination = gst_rtsp_connection_get_ip (src->conninfo.connection);
        else if (stream->conninfo.connection)
          *destination =
              gst_rtsp_connection_get_ip (stream->conninfo.connection);
      }
    }
    if (min && max) {
      /* for unicast we only expect the ports here */
      *min = transport->server_port.min;
      *max = transport->server_port.max;
    }
  }
}

/* For multicast create UDP sources and join the multicast group. */
static gboolean
gst_rtspsrc_stream_configure_mcast (GstRTSPSrc * src, GstRTSPStream * stream,
    GstRTSPTransport * transport, GstPad ** outpad)
{
  gchar *uri;
  const gchar *destination;
  gint min, max;

  GST_DEBUG_OBJECT (src, "creating UDP sources for multicast");

  /* we can remove the allocated UDP ports now */
  gst_rtspsrc_stream_free_udp (stream);

  gst_rtspsrc_get_transport_info (src, stream, transport, &destination, &min,
      &max, NULL);

  /* we need a destination now */
  if (destination == NULL)
    goto no_destination;

  /* we really need ports now or we won't be able to receive anything at all */
  if (min == -1 && max == -1)
    goto no_ports;

  GST_DEBUG_OBJECT (src, "have destination '%s' and ports (%d)-(%d)",
      destination, min, max);

  /* creating UDP source for RTP */
  if (min != -1) {
    uri = g_strdup_printf ("udp://%s:%d", destination, min);
    stream->udpsrc[0] =
        gst_element_make_from_uri (GST_URI_SRC, uri, NULL, NULL);
    g_free (uri);
    if (stream->udpsrc[0] == NULL)
      goto no_element;

    /* take ownership */
    gst_object_ref_sink (stream->udpsrc[0]);

    if (src->udp_buffer_size != 0)
      g_object_set (G_OBJECT (stream->udpsrc[0]), "buffer-size",
          src->udp_buffer_size, NULL);

    if (src->multi_iface != NULL)
      g_object_set (G_OBJECT (stream->udpsrc[0]), "multicast-iface",
          src->multi_iface, NULL);

    /* change state */
    gst_element_set_locked_state (stream->udpsrc[0], TRUE);
    gst_element_set_state (stream->udpsrc[0], GST_STATE_READY);
  }

  /* creating another UDP source for RTCP */
  if (max != -1) {
    GstCaps *caps;

    uri = g_strdup_printf ("udp://%s:%d", destination, max);
    stream->udpsrc[1] =
        gst_element_make_from_uri (GST_URI_SRC, uri, NULL, NULL);
    g_free (uri);
    if (stream->udpsrc[1] == NULL)
      goto no_element;

    if (stream->profile == GST_RTSP_PROFILE_SAVP ||
        stream->profile == GST_RTSP_PROFILE_SAVPF)
      caps = gst_caps_new_empty_simple ("application/x-srtcp");
    else
      caps = gst_caps_new_empty_simple ("application/x-rtcp");
    g_object_set (stream->udpsrc[1], "caps", caps, NULL);
    gst_caps_unref (caps);

    /* take ownership */
    gst_object_ref_sink (stream->udpsrc[1]);

    if (src->multi_iface != NULL)
      g_object_set (G_OBJECT (stream->udpsrc[1]), "multicast-iface",
          src->multi_iface, NULL);

    gst_element_set_state (stream->udpsrc[1], GST_STATE_READY);
  }
  return TRUE;

  /* ERRORS */
no_element:
  {
    GST_DEBUG_OBJECT (src, "no UDP source element found");
    return FALSE;
  }
no_destination:
  {
    GST_DEBUG_OBJECT (src, "no destination found");
    return FALSE;
  }
no_ports:
  {
    GST_DEBUG_OBJECT (src, "no ports found");
    return FALSE;
  }
}

/* configure the remainder of the UDP ports */
static gboolean
gst_rtspsrc_stream_configure_udp (GstRTSPSrc * src, GstRTSPStream * stream,
    GstRTSPTransport * transport, GstPad ** outpad)
{
  /* we manage the UDP elements now. For unicast, the UDP sources where
   * allocated in the stream when we suggested a transport. */
  if (stream->udpsrc[0]) {
    GstCaps *caps;

    gst_element_set_locked_state (stream->udpsrc[0], TRUE);
    gst_bin_add (GST_BIN_CAST (src), stream->udpsrc[0]);

    GST_DEBUG_OBJECT (src, "setting up UDP source");

    /* configure a timeout on the UDP port. When the timeout message is
     * posted, we assume UDP transport is not possible. We reconnect using TCP
     * if we can. */
    g_object_set (G_OBJECT (stream->udpsrc[0]), "timeout",
        src->udp_timeout * 1000, NULL);

    if ((caps = stream_get_caps_for_pt (stream, stream->default_pt)))
      g_object_set (stream->udpsrc[0], "caps", caps, NULL);

    /* get output pad of the UDP source. */
    *outpad = gst_element_get_static_pad (stream->udpsrc[0], "src");

    /* save it so we can unblock */
    stream->blockedpad = *outpad;

    /* configure pad block on the pad. As soon as there is dataflow on the
     * UDP source, we know that UDP is not blocked by a firewall and we can
     * configure all the streams to let the application autoplug decoders. */
    stream->blockid =
        gst_pad_add_probe (stream->blockedpad,
        GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_BUFFER |
        GST_PAD_PROBE_TYPE_BUFFER_LIST, pad_blocked, src, NULL);

    gst_pad_add_probe (stream->blockedpad,
        GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, udpsrc_probe_cb,
        &(stream->segment_seqnum[0]), NULL);

    if (stream->channelpad[0]) {
      GST_DEBUG_OBJECT (src, "connecting UDP source 0 to manager");
      /* configure for UDP delivery, we need to connect the UDP pads to
       * the session plugin. */
      gst_pad_link_full (*outpad, stream->channelpad[0],
          GST_PAD_LINK_CHECK_NOTHING);
      gst_object_unref (*outpad);
      *outpad = NULL;
      /* we connected to pad-added signal to get pads from the manager */
    } else {
      GST_DEBUG_OBJECT (src, "using UDP src pad as output");
    }
  }

  /* RTCP port */
  if (stream->udpsrc[1]) {
    GstCaps *caps;

    gst_element_set_locked_state (stream->udpsrc[1], TRUE);
    gst_bin_add (GST_BIN_CAST (src), stream->udpsrc[1]);

    if (stream->profile == GST_RTSP_PROFILE_SAVP ||
        stream->profile == GST_RTSP_PROFILE_SAVPF)
      caps = gst_caps_new_empty_simple ("application/x-srtcp");
    else
      caps = gst_caps_new_empty_simple ("application/x-rtcp");
    g_object_set (stream->udpsrc[1], "caps", caps, NULL);
    gst_caps_unref (caps);

    if (stream->channelpad[1]) {
      GstPad *pad;

      GST_DEBUG_OBJECT (src, "connecting UDP source 1 to manager");

      pad = gst_element_get_static_pad (stream->udpsrc[1], "src");
      gst_pad_add_probe (pad,
          GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, udpsrc_probe_cb,
          &(stream->segment_seqnum[1]), NULL);
      gst_pad_link_full (pad, stream->channelpad[1],
          GST_PAD_LINK_CHECK_NOTHING);
      gst_object_unref (pad);
    } else {
      /* leave unlinked */
    }
  }
  return TRUE;
}

/* configure the UDP sink back to the server for status reports */
static gboolean
gst_rtspsrc_stream_configure_udp_sinks (GstRTSPSrc * src,
    GstRTSPStream * stream, GstRTSPTransport * transport)
{
  GstPad *pad;
  gint rtp_port, rtcp_port;
  gboolean do_rtp, do_rtcp;
  const gchar *destination;
  gchar *uri, *name;
  guint ttl = 0;
  GSocket *socket;

  /* get transport info */
  gst_rtspsrc_get_transport_info (src, stream, transport, &destination,
      &rtp_port, &rtcp_port, &ttl);

  /* see what we need to do */
  do_rtp = (rtp_port != -1);
  /* it's possible that the server does not want us to send RTCP in which case
   * the port is -1 */
  do_rtcp = (rtcp_port != -1 && src->manager != NULL && src->do_rtcp);

  /* we need a destination when we have RTP or RTCP ports */
  if (destination == NULL && (do_rtp || do_rtcp))
    goto no_destination;

  /* try to construct the fakesrc to the RTP port of the server to open up any
   * NAT firewalls or, if backchannel, construct an appsrc */
  if (do_rtp) {
    GST_DEBUG_OBJECT (src, "configure RTP UDP sink for %s:%d", destination,
        rtp_port);

    uri = g_strdup_printf ("udp://%s:%d", destination, rtp_port);
    stream->udpsink[0] =
        gst_element_make_from_uri (GST_URI_SINK, uri, NULL, NULL);
    g_free (uri);
    if (stream->udpsink[0] == NULL)
      goto no_sink_element;

    /* don't join multicast group, we will have the source socket do that */
    /* no sync or async state changes needed */
    g_object_set (G_OBJECT (stream->udpsink[0]), "auto-multicast", FALSE,
        "loop", FALSE, "sync", FALSE, "async", FALSE, NULL);
    if (ttl > 0)
      g_object_set (G_OBJECT (stream->udpsink[0]), "ttl", ttl, NULL);

    if (stream->udpsrc[0]) {
      /* configure socket, we give it the same UDP socket as the udpsrc for RTP
       * so that NAT firewalls will open a hole for us */
      g_object_get (G_OBJECT (stream->udpsrc[0]), "used-socket", &socket, NULL);
      if (!socket)
        goto no_socket;

      GST_DEBUG_OBJECT (src, "RTP UDP src has sock %p", socket);
      /* configure socket and make sure udpsink does not close it when shutting
       * down, it belongs to udpsrc after all. */
      g_object_set (G_OBJECT (stream->udpsink[0]), "socket", socket,
          "close-socket", FALSE, NULL);
      g_object_unref (socket);
    }

    if (stream->is_backchannel) {
      /* appsrc is for the app to shovel data using push-backchannel-buffer */
      stream->rtpsrc = gst_element_factory_make ("appsrc", NULL);
      if (stream->rtpsrc == NULL)
        goto no_appsrc_element;

      /* interal use only, don't emit signals */
      g_object_set (G_OBJECT (stream->rtpsrc), "emit-signals", TRUE,
          "is-live", TRUE, NULL);
    } else {
      /* the source for the dummy packets to open up NAT */
      stream->rtpsrc = gst_element_factory_make ("fakesrc", NULL);
      if (stream->rtpsrc == NULL)
        goto no_fakesrc_element;

      /* random data in 5 buffers, a size of 200 bytes should be fine */
      g_object_set (G_OBJECT (stream->rtpsrc), "filltype", 3, "num-buffers", 5,
          "sizetype", 2, "sizemax", 200, "silent", TRUE, NULL);
    }

    /* keep everything locked */
    gst_element_set_locked_state (stream->udpsink[0], TRUE);
    gst_element_set_locked_state (stream->rtpsrc, TRUE);

    gst_object_ref (stream->udpsink[0]);
    gst_bin_add (GST_BIN_CAST (src), stream->udpsink[0]);
    gst_object_ref (stream->rtpsrc);
    gst_bin_add (GST_BIN_CAST (src), stream->rtpsrc);

    gst_element_link_pads_full (stream->rtpsrc, "src", stream->udpsink[0],
        "sink", GST_PAD_LINK_CHECK_NOTHING);
  }
  if (do_rtcp) {
    GST_DEBUG_OBJECT (src, "configure RTCP UDP sink for %s:%d", destination,
        rtcp_port);

    uri = g_strdup_printf ("udp://%s:%d", destination, rtcp_port);
    stream->udpsink[1] =
        gst_element_make_from_uri (GST_URI_SINK, uri, NULL, NULL);
    g_free (uri);
    if (stream->udpsink[1] == NULL)
      goto no_sink_element;

    /* don't join multicast group, we will have the source socket do that */
    /* no sync or async state changes needed */
    g_object_set (G_OBJECT (stream->udpsink[1]), "auto-multicast", FALSE,
        "loop", FALSE, "sync", FALSE, "async", FALSE, NULL);
    if (ttl > 0)
      g_object_set (G_OBJECT (stream->udpsink[0]), "ttl", ttl, NULL);

    if (stream->udpsrc[1]) {
      /* configure socket, we give it the same UDP socket as the udpsrc for RTCP
       * because some servers check the port number of where it sends RTCP to identify
       * the RTCP packets it receives */
      g_object_get (G_OBJECT (stream->udpsrc[1]), "used-socket", &socket, NULL);
      if (!socket)
        goto no_socket;

      GST_DEBUG_OBJECT (src, "RTCP UDP src has sock %p", socket);
      /* configure socket and make sure udpsink does not close it when shutting
       * down, it belongs to udpsrc after all. */
      g_object_set (G_OBJECT (stream->udpsink[1]), "socket", socket,
          "close-socket", FALSE, NULL);
      g_object_unref (socket);
    }

    /* we keep this playing always */
    gst_element_set_locked_state (stream->udpsink[1], TRUE);
    gst_element_set_state (stream->udpsink[1], GST_STATE_PLAYING);

    gst_object_ref (stream->udpsink[1]);
    gst_bin_add (GST_BIN_CAST (src), stream->udpsink[1]);

    stream->rtcppad = gst_element_get_static_pad (stream->udpsink[1], "sink");

    /* get session RTCP pad */
    name = g_strdup_printf ("send_rtcp_src_%u", stream->id);
    pad = gst_element_get_request_pad (src->manager, name);
    g_free (name);

    /* and link */
    if (pad) {
      gst_pad_link_full (pad, stream->rtcppad, GST_PAD_LINK_CHECK_NOTHING);
      gst_object_unref (pad);
    }
  }

  return TRUE;

  /* ERRORS */
no_destination:
  {
    GST_ERROR_OBJECT (src, "no destination address specified");
    return FALSE;
  }
no_sink_element:
  {
    GST_ERROR_OBJECT (src, "no UDP sink element found");
    return FALSE;
  }
no_appsrc_element:
  {
    GST_ERROR_OBJECT (src, "no appsrc element found");
    return FALSE;
  }
no_fakesrc_element:
  {
    GST_ERROR_OBJECT (src, "no fakesrc element found");
    return FALSE;
  }
no_socket:
  {
    GST_ERROR_OBJECT (src, "failed to create socket");
    return FALSE;
  }
}

/* sets up all elements needed for streaming over the specified transport.
 * Does not yet expose the element pads, this will be done when there is actuall
 * dataflow detected, which might never happen when UDP is blocked in a
 * firewall, for example.
 */
static gboolean
gst_rtspsrc_stream_configure_transport (GstRTSPStream * stream,
    GstRTSPTransport * transport)
{
  GstRTSPSrc *src;
  GstPad *outpad = NULL;
  GstPadTemplate *template;
  gchar *name;
  const gchar *media_type;
  guint i, len;

  src = stream->parent;

  GST_DEBUG_OBJECT (src, "configuring transport for stream %p", stream);

  /* get the proper media type for this stream now */
  if (gst_rtsp_transport_get_media_type (transport, &media_type) < 0)
    goto unknown_transport;
  if (!media_type)
    goto unknown_transport;

  /* configure the final media type */
  GST_DEBUG_OBJECT (src, "setting media type to %s", media_type);

  len = stream->ptmap->len;
  for (i = 0; i < len; i++) {
    GstStructure *s;
    PtMapItem *item = &g_array_index (stream->ptmap, PtMapItem, i);

    if (item->caps == NULL)
      continue;

    s = gst_caps_get_structure (item->caps, 0);
    gst_structure_set_name (s, media_type);
    /* set ssrc if known */
    if (transport->ssrc)
      gst_structure_set (s, "ssrc", G_TYPE_UINT, transport->ssrc, NULL);
  }

  /* try to get and configure a manager, channelpad[0-1] will be configured with
   * the pads for the manager, or NULL when no manager is needed. */
  if (!gst_rtspsrc_stream_configure_manager (src, stream, transport))
    goto no_manager;

  switch (transport->lower_transport) {
    case GST_RTSP_LOWER_TRANS_TCP:
      if (!gst_rtspsrc_stream_configure_tcp (src, stream, transport, &outpad))
        goto transport_failed;
      break;
    case GST_RTSP_LOWER_TRANS_UDP_MCAST:
      if (!gst_rtspsrc_stream_configure_mcast (src, stream, transport, &outpad))
        goto transport_failed;
      /* fallthrough, the rest is the same for UDP and MCAST */
    case GST_RTSP_LOWER_TRANS_UDP:
      if (!gst_rtspsrc_stream_configure_udp (src, stream, transport, &outpad))
        goto transport_failed;
      /* configure udpsinks back to the server for RTCP messages, for the
       * dummy RTP messages to open NAT, and for the backchannel */
      if (!gst_rtspsrc_stream_configure_udp_sinks (src, stream, transport))
        goto transport_failed;
      break;
    default:
      goto unknown_transport;
  }

  /* using backchannel and no manager, hence no srcpad for this stream */
  if (outpad && stream->is_backchannel) {
    add_backchannel_fakesink (src, stream, outpad);
    gst_object_unref (outpad);
  } else if (outpad) {
    GST_DEBUG_OBJECT (src, "creating ghostpad for stream %p", stream);

    gst_pad_use_fixed_caps (outpad);

    /* create ghostpad, don't add just yet, this will be done when we activate
     * the stream. */
    name = g_strdup_printf ("stream_%u", stream->id);
    template = gst_static_pad_template_get (&rtptemplate);
    stream->srcpad = gst_ghost_pad_new_from_template (name, outpad, template);
    gst_pad_set_event_function (stream->srcpad, gst_rtspsrc_handle_src_event);
    gst_pad_set_query_function (stream->srcpad, gst_rtspsrc_handle_src_query);
    gst_object_unref (template);
    g_free (name);

    gst_object_unref (outpad);
  }
  /* mark pad as ok */
  stream->last_ret = GST_FLOW_OK;

  return TRUE;

  /* ERRORS */
transport_failed:
  {
    GST_WARNING_OBJECT (src, "failed to configure transport");
    return FALSE;
  }
unknown_transport:
  {
    GST_WARNING_OBJECT (src, "unknown transport");
    return FALSE;
  }
no_manager:
  {
    GST_WARNING_OBJECT (src, "cannot get a session manager");
    return FALSE;
  }
}

/* send a couple of dummy random packets on the receiver RTP port to the server,
 * this should make a firewall think we initiated the data transfer and
 * hopefully allow packets to go from the sender port to our RTP receiver port */
static gboolean
gst_rtspsrc_send_dummy_packets (GstRTSPSrc * src)
{
  GList *walk;

  if (src->nat_method != GST_RTSP_NAT_DUMMY)
    return TRUE;

  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;

    if (!stream->rtpsrc || !stream->udpsink[0])
      continue;

    if (stream->is_backchannel)
      GST_DEBUG_OBJECT (src, "starting backchannel stream %p", stream);
    else
      GST_DEBUG_OBJECT (src, "sending dummy packet to stream %p", stream);

    gst_element_set_state (stream->udpsink[0], GST_STATE_NULL);
    gst_element_set_state (stream->rtpsrc, GST_STATE_NULL);
    gst_element_set_state (stream->udpsink[0], GST_STATE_PLAYING);
    gst_element_set_state (stream->rtpsrc, GST_STATE_PLAYING);
  }
  return TRUE;
}

/* Adds the source pads of all configured streams to the element.
 * This code is performed when we detected dataflow.
 *
 * We detect dataflow from either the _loop function or with pad probes on the
 * udp sources.
 */
static gboolean
gst_rtspsrc_activate_streams (GstRTSPSrc * src)
{
  GList *walk;

  GST_DEBUG_OBJECT (src, "activating streams");

  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;

    if (stream->udpsrc[0]) {
      /* remove timeout, we are streaming now and timeouts will be handled by
       * the session manager and jitter buffer */
      g_object_set (G_OBJECT (stream->udpsrc[0]), "timeout", (guint64) 0, NULL);
    }
    if (stream->srcpad) {
      GST_DEBUG_OBJECT (src, "activating stream pad %p", stream);
      gst_pad_set_active (stream->srcpad, TRUE);

      /* if we don't have a session manager, set the caps now. If we have a
       * session, we will get a notification of the pad and the caps. */
      if (!src->manager) {
        GstCaps *caps;

        caps = stream_get_caps_for_pt (stream, stream->default_pt);
        GST_DEBUG_OBJECT (src, "setting pad caps for stream %p", stream);
        gst_pad_set_caps (stream->srcpad, caps);
      }
      /* add the pad */
      if (!stream->added) {
        GST_DEBUG_OBJECT (src, "adding stream pad %p", stream);
        if (stream->is_backchannel)
          add_backchannel_fakesink (src, stream, stream->srcpad);
        else
          gst_element_add_pad (GST_ELEMENT_CAST (src), stream->srcpad);
        stream->added = TRUE;
      }
    }
  }

  /* unblock all pads */
  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;

    if (stream->blockid) {
      GST_DEBUG_OBJECT (src, "unblocking stream pad %p", stream);
      gst_pad_remove_probe (stream->blockedpad, stream->blockid);
      stream->blockid = 0;
    }
  }

  return TRUE;
}

static void
gst_rtspsrc_configure_caps (GstRTSPSrc * src, GstSegment * segment,
    gboolean reset_manager)
{
  GList *walk;
  guint64 start, stop;
  gdouble play_speed, play_scale;

  GST_DEBUG_OBJECT (src, "configuring stream caps");

  start = segment->rate > 0.0 ? segment->start : segment->stop;
  stop = segment->rate > 0.0 ? segment->stop : segment->start;
  play_speed = segment->rate;
  play_scale = segment->applied_rate;

  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    guint j, len;

    if (!stream->setup)
      continue;

    len = stream->ptmap->len;
    for (j = 0; j < len; j++) {
      GstCaps *caps;
      PtMapItem *item = &g_array_index (stream->ptmap, PtMapItem, j);

      if (item->caps == NULL)
        continue;

      caps = gst_caps_make_writable (item->caps);
      /* update caps */
      if (stream->timebase != -1)
        gst_caps_set_simple (caps, "clock-base", G_TYPE_UINT,
            (guint) stream->timebase, NULL);
      if (stream->seqbase != -1)
        gst_caps_set_simple (caps, "seqnum-base", G_TYPE_UINT,
            (guint) stream->seqbase, NULL);
      gst_caps_set_simple (caps, "npt-start", G_TYPE_UINT64, start, NULL);
      if (stop != -1)
        gst_caps_set_simple (caps, "npt-stop", G_TYPE_UINT64, stop, NULL);
      gst_caps_set_simple (caps, "play-speed", G_TYPE_DOUBLE, play_speed, NULL);
      gst_caps_set_simple (caps, "play-scale", G_TYPE_DOUBLE, play_scale, NULL);
      gst_caps_set_simple (caps, "onvif-mode", G_TYPE_BOOLEAN, src->onvif_mode,
          NULL);

      item->caps = caps;
      GST_DEBUG_OBJECT (src, "stream %p, pt %d, caps %" GST_PTR_FORMAT, stream,
          item->pt, caps);

      if (item->pt == stream->default_pt) {
        if (stream->udpsrc[0])
          g_object_set (stream->udpsrc[0], "caps", caps, NULL);
        stream->need_caps = TRUE;
      }
    }
  }
  if (reset_manager && src->manager) {
    GST_DEBUG_OBJECT (src, "clear session");
    g_signal_emit_by_name (src->manager, "clear-pt-map", NULL);
  }
}

static GstFlowReturn
gst_rtspsrc_combine_flows (GstRTSPSrc * src, GstRTSPStream * stream,
    GstFlowReturn ret)
{
  GList *streams;

  /* store the value */
  stream->last_ret = ret;

  /* if it's success we can return the value right away */
  if (ret == GST_FLOW_OK)
    goto done;

  /* any other error that is not-linked can be returned right
   * away */
  if (ret != GST_FLOW_NOT_LINKED)
    goto done;

  /* only return NOT_LINKED if all other pads returned NOT_LINKED */
  for (streams = src->streams; streams; streams = g_list_next (streams)) {
    GstRTSPStream *ostream = (GstRTSPStream *) streams->data;

    ret = ostream->last_ret;
    /* some other return value (must be SUCCESS but we can return
     * other values as well) */
    if (ret != GST_FLOW_NOT_LINKED)
      goto done;
  }
  /* if we get here, all other pads were unlinked and we return
   * NOT_LINKED then */
done:
  return ret;
}

static gboolean
gst_rtspsrc_stream_push_event (GstRTSPSrc * src, GstRTSPStream * stream,
    GstEvent * event)
{
  gboolean res = TRUE;

  /* only streams that have a connection to the outside world */
  if (!stream->setup)
    goto done;

  if (stream->udpsrc[0]) {
    GstEvent *sent_event;

    if (GST_EVENT_TYPE (event) == GST_EVENT_EOS) {
      sent_event = gst_event_new_eos ();
      gst_event_set_seqnum (sent_event, stream->segment_seqnum[0]);
    } else {
      sent_event = gst_event_ref (event);
    }

    res = gst_element_send_event (stream->udpsrc[0], sent_event);
  } else if (stream->channelpad[0]) {
    gst_event_ref (event);
    if (GST_PAD_IS_SRC (stream->channelpad[0]))
      res = gst_pad_push_event (stream->channelpad[0], event);
    else
      res = gst_pad_send_event (stream->channelpad[0], event);
  }

  if (stream->udpsrc[1]) {
    GstEvent *sent_event;

    if (GST_EVENT_TYPE (event) == GST_EVENT_EOS) {
      sent_event = gst_event_new_eos ();
      if (stream->segment_seqnum[1] != GST_SEQNUM_INVALID) {
        gst_event_set_seqnum (sent_event, stream->segment_seqnum[1]);
      }
    } else {
      sent_event = gst_event_ref (event);
    }

    res &= gst_element_send_event (stream->udpsrc[1], sent_event);
  } else if (stream->channelpad[1]) {
    gst_event_ref (event);
    if (GST_PAD_IS_SRC (stream->channelpad[1]))
      res &= gst_pad_push_event (stream->channelpad[1], event);
    else
      res &= gst_pad_send_event (stream->channelpad[1], event);
  }

done:
  gst_event_unref (event);

  return res;
}

static gboolean
gst_rtspsrc_push_event (GstRTSPSrc * src, GstEvent * event)
{
  GList *streams;
  gboolean res = TRUE;

  for (streams = src->streams; streams; streams = g_list_next (streams)) {
    GstRTSPStream *ostream = (GstRTSPStream *) streams->data;

    gst_event_ref (event);
    res &= gst_rtspsrc_stream_push_event (src, ostream, event);
  }
  gst_event_unref (event);

  return res;
}

static gboolean
accept_certificate_cb (GTlsConnection * conn, GTlsCertificate * peer_cert,
    GTlsCertificateFlags errors, gpointer user_data)
{
  GstRTSPSrc *src = user_data;
  gboolean accept = FALSE;

  g_signal_emit (src, gst_rtspsrc_signals[SIGNAL_ACCEPT_CERTIFICATE], 0, conn,
      peer_cert, errors, &accept);

  return accept;
}

static GstRTSPResult
gst_rtsp_conninfo_connect (GstRTSPSrc * src, GstRTSPConnInfo * info,
    gboolean async)
{
  GstRTSPResult res;
  GstRTSPMessage response;
  gboolean retry = FALSE;
  memset (&response, 0, sizeof (response));
  gst_rtsp_message_init (&response);
  do {
    if (info->connection == NULL) {
      if (info->url == NULL) {
        GST_DEBUG_OBJECT (src, "parsing uri (%s)...", info->location);
        if ((res = gst_rtsp_url_parse (info->location, &info->url)) < 0)
          goto parse_error;
      }
      /* create connection */
      GST_DEBUG_OBJECT (src, "creating connection (%s)...", info->location);
      if ((res = gst_rtsp_connection_create (info->url, &info->connection)) < 0)
        goto could_not_create;

      if (retry) {
        gst_rtspsrc_setup_auth (src, &response);
      }

      g_free (info->url_str);
      info->url_str = gst_rtsp_url_get_request_uri (info->url);

      GST_DEBUG_OBJECT (src, "sanitized uri %s", info->url_str);

      if (info->url->transports & GST_RTSP_LOWER_TRANS_TLS) {
        if (!gst_rtsp_connection_set_tls_validation_flags (info->connection,
                src->tls_validation_flags))
          GST_WARNING_OBJECT (src, "Unable to set TLS validation flags");

        if (src->tls_database)
          gst_rtsp_connection_set_tls_database (info->connection,
              src->tls_database);

        if (src->tls_interaction)
          gst_rtsp_connection_set_tls_interaction (info->connection,
              src->tls_interaction);
        gst_rtsp_connection_set_accept_certificate_func (info->connection,
            accept_certificate_cb, src, NULL);
      }

      if (info->url->transports & GST_RTSP_LOWER_TRANS_HTTP)
        gst_rtsp_connection_set_tunneled (info->connection, TRUE);

      if (src->proxy_host) {
        GST_DEBUG_OBJECT (src, "setting proxy %s:%d", src->proxy_host,
            src->proxy_port);
        gst_rtsp_connection_set_proxy (info->connection, src->proxy_host,
            src->proxy_port);
      }
    }

    if (!info->connected) {
      /* connect */
      if (async)
        GST_ELEMENT_PROGRESS (src, CONTINUE, "connect",
            ("Connecting to %s", info->location));
      GST_DEBUG_OBJECT (src, "connecting (%s)...", info->location);
      res = gst_rtsp_connection_connect_with_response (info->connection,
          src->ptcp_timeout, &response);

      if (response.type == GST_RTSP_MESSAGE_HTTP_RESPONSE &&
          response.type_data.response.code == GST_RTSP_STS_UNAUTHORIZED) {
        gst_rtsp_conninfo_close (src, info, TRUE);
        if (!retry)
          retry = TRUE;
        else
          retry = FALSE;        // we should not retry more than once
      } else {
        retry = FALSE;
      }

      if (res == GST_RTSP_OK)
        info->connected = TRUE;
      else if (!retry)
        goto could_not_connect;
    }
  } while (!info->connected && retry);

  gst_rtsp_message_unset (&response);
  return GST_RTSP_OK;

  /* ERRORS */
parse_error:
  {
    GST_ERROR_OBJECT (src, "No valid RTSP URL was provided");
    gst_rtsp_message_unset (&response);
    return res;
  }
could_not_create:
  {
    gchar *str = gst_rtsp_strresult (res);
    GST_ERROR_OBJECT (src, "Could not create connection. (%s)", str);
    g_free (str);
    gst_rtsp_message_unset (&response);
    return res;
  }
could_not_connect:
  {
    gchar *str = gst_rtsp_strresult (res);
    GST_ERROR_OBJECT (src, "Could not connect to server. (%s)", str);
    g_free (str);
    gst_rtsp_message_unset (&response);
    return res;
  }
}

static GstRTSPResult
gst_rtsp_conninfo_close (GstRTSPSrc * src, GstRTSPConnInfo * info,
    gboolean free)
{
  GST_RTSP_STATE_LOCK (src);
  if (info->connected) {
    GST_DEBUG_OBJECT (src, "closing connection...");
    gst_rtsp_connection_close (info->connection);
    info->connected = FALSE;
  }
  if (free && info->connection) {
    /* free connection */
    GST_DEBUG_OBJECT (src, "freeing connection...");
    gst_rtsp_connection_free (info->connection);
    info->connection = NULL;
    info->flushing = FALSE;
  }
  GST_RTSP_STATE_UNLOCK (src);
  return GST_RTSP_OK;
}

static GstRTSPResult
gst_rtsp_conninfo_reconnect (GstRTSPSrc * src, GstRTSPConnInfo * info,
    gboolean async)
{
  GstRTSPResult res;

  GST_DEBUG_OBJECT (src, "reconnecting connection...");
  gst_rtsp_conninfo_close (src, info, FALSE);
  res = gst_rtsp_conninfo_connect (src, info, async);

  return res;
}

static void
gst_rtspsrc_connection_flush (GstRTSPSrc * src, gboolean flush)
{
  GList *walk;

  GST_DEBUG_OBJECT (src, "set flushing %d", flush);
  GST_RTSP_STATE_LOCK (src);
  if (src->conninfo.connection && src->conninfo.flushing != flush) {
    GST_DEBUG_OBJECT (src, "connection flush");
    gst_rtsp_connection_flush (src->conninfo.connection, flush);
    src->conninfo.flushing = flush;
  }
  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    if (stream->conninfo.connection && stream->conninfo.flushing != flush) {
      GST_DEBUG_OBJECT (src, "stream %p flush", stream);
      gst_rtsp_connection_flush (stream->conninfo.connection, flush);
      stream->conninfo.flushing = flush;
    }
  }
  GST_RTSP_STATE_UNLOCK (src);
}

static GstRTSPResult
gst_rtspsrc_init_request (GstRTSPSrc * src, GstRTSPMessage * msg,
    GstRTSPMethod method, const gchar * uri)
{
  GstRTSPResult res;

  res = gst_rtsp_message_init_request (msg, method, uri);
  if (res < 0)
    return res;

  /* set user-agent */
  if (src->user_agent)
    gst_rtsp_message_add_header (msg, GST_RTSP_HDR_USER_AGENT, src->user_agent);

  return res;
}

/* FIXME, handle server request, reply with OK, for now */
static GstRTSPResult
gst_rtspsrc_handle_request (GstRTSPSrc * src, GstRTSPConnInfo * conninfo,
    GstRTSPMessage * request)
{
  GstRTSPMessage response = { 0 };
  GstRTSPResult res;

  GST_DEBUG_OBJECT (src, "got server request message");

  DEBUG_RTSP (src, request);

  res = gst_rtsp_ext_list_receive_request (src->extensions, request);

  if (res == GST_RTSP_ENOTIMPL) {
    /* default implementation, send OK */
    GST_DEBUG_OBJECT (src, "prepare OK reply");
    res =
        gst_rtsp_message_init_response (&response, GST_RTSP_STS_OK, "OK",
        request);
    if (res < 0)
      goto send_error;

    /* let app parse and reply */
    g_signal_emit (src, gst_rtspsrc_signals[SIGNAL_HANDLE_REQUEST],
        0, request, &response);

    DEBUG_RTSP (src, &response);

    res = gst_rtspsrc_connection_send (src, conninfo, &response, NULL);
    if (res < 0)
      goto send_error;

    gst_rtsp_message_unset (&response);
  } else if (res == GST_RTSP_EEOF)
    return res;

  return GST_RTSP_OK;

  /* ERRORS */
send_error:
  {
    gst_rtsp_message_unset (&response);
    return res;
  }
}

/* send server keep-alive */
static GstRTSPResult
gst_rtspsrc_send_keep_alive (GstRTSPSrc * src)
{
  GstRTSPMessage request = { 0 };
  GstRTSPResult res;
  GstRTSPMethod method;
  const gchar *control;

  if (src->do_rtsp_keep_alive == FALSE) {
    GST_DEBUG_OBJECT (src, "do-rtsp-keep-alive is FALSE, not sending.");
    gst_rtsp_connection_reset_timeout (src->conninfo.connection);
    return GST_RTSP_OK;
  }

  GST_DEBUG_OBJECT (src, "creating server keep-alive");

  /* find a method to use for keep-alive */
  if (src->methods & GST_RTSP_GET_PARAMETER)
    method = GST_RTSP_GET_PARAMETER;
  else
    method = GST_RTSP_OPTIONS;

  control = get_aggregate_control (src);
  if (control == NULL)
    goto no_control;

  res = gst_rtspsrc_init_request (src, &request, method, control);
  if (res < 0)
    goto send_error;

  request.type_data.request.version = src->version;

  res = gst_rtspsrc_connection_send (src, &src->conninfo, &request, NULL);
  if (res < 0)
    goto send_error;

  gst_rtsp_connection_reset_timeout (src->conninfo.connection);
  gst_rtsp_message_unset (&request);

  return GST_RTSP_OK;

  /* ERRORS */
no_control:
  {
    GST_WARNING_OBJECT (src, "no control url to send keepalive");
    return GST_RTSP_OK;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    gst_rtsp_message_unset (&request);
    GST_ELEMENT_WARNING (src, RESOURCE, WRITE, (NULL),
        ("Could not send keep-alive. (%s)", str));
    g_free (str);
    return res;
  }
}

static GstFlowReturn
gst_rtspsrc_handle_data (GstRTSPSrc * src, GstRTSPMessage * message)
{
  GstFlowReturn ret = GST_FLOW_OK;
  gint channel;
  GstRTSPStream *stream;
  GstPad *outpad = NULL;
  guint8 *data;
  guint size;
  GstBuffer *buf;
  gboolean is_rtcp;

  channel = message->type_data.data.channel;

  stream = find_stream (src, &channel, (gpointer) find_stream_by_channel);
  if (!stream)
    goto unknown_stream;

  if (channel == stream->channel[0]) {
    outpad = stream->channelpad[0];
    is_rtcp = FALSE;
  } else if (channel == stream->channel[1]) {
    outpad = stream->channelpad[1];
    is_rtcp = TRUE;
  } else {
    is_rtcp = FALSE;
  }

  /* take a look at the body to figure out what we have */
  gst_rtsp_message_get_body (message, &data, &size);
  if (size < 2)
    goto invalid_length;

  /* channels are not correct on some servers, do extra check */
  if (data[1] >= 200 && data[1] <= 204) {
    /* hmm RTCP message switch to the RTCP pad of the same stream. */
    outpad = stream->channelpad[1];
    is_rtcp = TRUE;
  }

  /* we have no clue what this is, just ignore then. */
  if (outpad == NULL)
    goto unknown_stream;

  /* take the message body for further processing */
  gst_rtsp_message_steal_body (message, &data, &size);

  /* strip the trailing \0 */
  size -= 1;

  buf = gst_buffer_new ();
  gst_buffer_append_memory (buf,
      gst_memory_new_wrapped (0, data, size, 0, size, data, g_free));

  /* don't need message anymore */
  gst_rtsp_message_unset (message);

  GST_DEBUG_OBJECT (src, "pushing data of size %d on channel %d", size,
      channel);

  if (src->need_activate) {
    gchar *stream_id;
    GstEvent *event;
    GChecksum *cs;
    gchar *uri;
    GList *streams;
    guint group_id = gst_util_group_id_next ();

    /* generate an SHA256 sum of the URI */
    cs = g_checksum_new (G_CHECKSUM_SHA256);
    uri = src->conninfo.location;
    g_checksum_update (cs, (const guchar *) uri, strlen (uri));

    for (streams = src->streams; streams; streams = g_list_next (streams)) {
      GstRTSPStream *ostream = (GstRTSPStream *) streams->data;
      GstCaps *caps;

      /* Activate in advance so that the stream-start event is registered */
      if (stream->srcpad) {
        gst_pad_set_active (stream->srcpad, TRUE);
      }

      stream_id =
          g_strdup_printf ("%s/%d", g_checksum_get_string (cs), ostream->id);
      event = gst_event_new_stream_start (stream_id);
      gst_event_set_group_id (event, group_id);

      g_free (stream_id);
      gst_rtspsrc_stream_push_event (src, ostream, event);

      if ((caps = stream_get_caps_for_pt (ostream, ostream->default_pt))) {
        /* only streams that have a connection to the outside world */
        if (ostream->setup) {
          if (ostream->udpsrc[0]) {
            gst_element_send_event (ostream->udpsrc[0],
                gst_event_new_caps (caps));
          } else if (ostream->channelpad[0]) {
            if (GST_PAD_IS_SRC (ostream->channelpad[0]))
              gst_pad_push_event (ostream->channelpad[0],
                  gst_event_new_caps (caps));
            else
              gst_pad_send_event (ostream->channelpad[0],
                  gst_event_new_caps (caps));
          }
          ostream->need_caps = FALSE;

          if (ostream->profile == GST_RTSP_PROFILE_SAVP ||
              ostream->profile == GST_RTSP_PROFILE_SAVPF)
            caps = gst_caps_new_empty_simple ("application/x-srtcp");
          else
            caps = gst_caps_new_empty_simple ("application/x-rtcp");

          if (ostream->udpsrc[1]) {
            gst_element_send_event (ostream->udpsrc[1],
                gst_event_new_caps (caps));
          } else if (ostream->channelpad[1]) {
            if (GST_PAD_IS_SRC (ostream->channelpad[1]))
              gst_pad_push_event (ostream->channelpad[1],
                  gst_event_new_caps (caps));
            else
              gst_pad_send_event (ostream->channelpad[1],
                  gst_event_new_caps (caps));
          }

          gst_caps_unref (caps);
        }
      }
    }
    g_checksum_free (cs);

    gst_rtspsrc_activate_streams (src);
    src->need_activate = FALSE;
    src->need_segment = TRUE;
  }

  if (src->base_time == -1) {
    /* Take current running_time. This timestamp will be put on
     * the first buffer of each stream because we are a live source and so we
     * timestamp with the running_time. When we are dealing with TCP, we also
     * only timestamp the first buffer (using the DISCONT flag) because a server
     * typically bursts data, for which we don't want to compensate by speeding
     * up the media. The other timestamps will be interpollated from this one
     * using the RTP timestamps. */
    GST_OBJECT_LOCK (src);
    if (GST_ELEMENT_CLOCK (src)) {
      GstClockTime now;
      GstClockTime base_time;

      now = gst_clock_get_time (GST_ELEMENT_CLOCK (src));
      base_time = GST_ELEMENT_CAST (src)->base_time;

      src->base_time = now - base_time;

      GST_DEBUG_OBJECT (src, "first buffer at time %" GST_TIME_FORMAT ", base %"
          GST_TIME_FORMAT, GST_TIME_ARGS (now), GST_TIME_ARGS (base_time));
    }
    GST_OBJECT_UNLOCK (src);
  }

  /* If needed send a new segment, don't forget we are live and buffer are
   * timestamped with running time */
  if (src->need_segment) {
    src->need_segment = FALSE;
    if (src->onvif_mode) {
      gst_rtspsrc_push_event (src, gst_event_new_segment (&src->out_segment));
    } else {
      GstSegment segment;

      gst_segment_init (&segment, GST_FORMAT_TIME);
      gst_rtspsrc_push_event (src, gst_event_new_segment (&segment));
    }
  }

  if (stream->need_caps) {
    GstCaps *caps;

    if ((caps = stream_get_caps_for_pt (stream, stream->default_pt))) {
      /* only streams that have a connection to the outside world */
      if (stream->setup) {
        /* Only need to update the TCP caps here, UDP is already handled */
        if (stream->channelpad[0]) {
          if (GST_PAD_IS_SRC (stream->channelpad[0]))
            gst_pad_push_event (stream->channelpad[0],
                gst_event_new_caps (caps));
          else
            gst_pad_send_event (stream->channelpad[0],
                gst_event_new_caps (caps));
        }
        stream->need_caps = FALSE;
      }
    }

    stream->need_caps = FALSE;
  }

  if (stream->discont && !is_rtcp) {
    /* mark first RTP buffer as discont */
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
    stream->discont = FALSE;
    /* first buffer gets the timestamp, other buffers are not timestamped and
     * their presentation time will be interpollated from the rtp timestamps. */
    GST_DEBUG_OBJECT (src, "setting timestamp %" GST_TIME_FORMAT,
        GST_TIME_ARGS (src->base_time));

    GST_BUFFER_TIMESTAMP (buf) = src->base_time;
  }

  /* chain to the peer pad */
  if (GST_PAD_IS_SINK (outpad))
    ret = gst_pad_chain (outpad, buf);
  else
    ret = gst_pad_push (outpad, buf);

  if (!is_rtcp) {
    /* combine all stream flows for the data transport */
    ret = gst_rtspsrc_combine_flows (src, stream, ret);
  }
  return ret;

  /* ERRORS */
unknown_stream:
  {
    GST_DEBUG_OBJECT (src, "unknown stream on channel %d, ignored", channel);
    gst_rtsp_message_unset (message);
    return GST_FLOW_OK;
  }
invalid_length:
  {
    GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
        ("Short message received, ignoring."));
    gst_rtsp_message_unset (message);
    return GST_FLOW_OK;
  }
}

static GstFlowReturn
gst_rtspsrc_loop_interleaved (GstRTSPSrc * src)
{
  GstRTSPMessage message = { 0 };
  GstRTSPResult res;
  GstFlowReturn ret = GST_FLOW_OK;
  GTimeVal tv_timeout;

  while (TRUE) {
    /* get the next timeout interval */
    gst_rtsp_connection_next_timeout (src->conninfo.connection, &tv_timeout);

    GST_DEBUG_OBJECT (src, "doing receive with timeout %ld seconds, %ld usec",
        tv_timeout.tv_sec, tv_timeout.tv_usec);

    gst_rtsp_message_unset (&message);

    /* protect the connection with the connection lock so that we can see when
     * we are finished doing server communication */
    res =
        gst_rtspsrc_connection_receive (src, &src->conninfo,
        &message, src->ptcp_timeout);

    switch (res) {
      case GST_RTSP_OK:
        GST_DEBUG_OBJECT (src, "we received a server message");
        break;
      case GST_RTSP_EINTR:
        /* we got interrupted this means we need to stop */
        goto interrupt;
      case GST_RTSP_ETIMEOUT:
        /* no reply, send keep alive */
        GST_DEBUG_OBJECT (src, "timeout, sending keep-alive");
        if ((res = gst_rtspsrc_send_keep_alive (src)) == GST_RTSP_EINTR)
          goto interrupt;
        continue;
      case GST_RTSP_EEOF:
        /* go EOS when the server closed the connection */
        goto server_eof;
      default:
        goto receive_error;
    }

    switch (message.type) {
      case GST_RTSP_MESSAGE_REQUEST:
        /* server sends us a request message, handle it */
        res = gst_rtspsrc_handle_request (src, &src->conninfo, &message);
        if (res == GST_RTSP_EEOF)
          goto server_eof;
        else if (res < 0)
          goto handle_request_failed;
        break;
      case GST_RTSP_MESSAGE_RESPONSE:
        /* we ignore response messages */
        GST_DEBUG_OBJECT (src, "ignoring response message");
        DEBUG_RTSP (src, &message);
        break;
      case GST_RTSP_MESSAGE_DATA:
        GST_DEBUG_OBJECT (src, "got data message");
        ret = gst_rtspsrc_handle_data (src, &message);
        if (ret != GST_FLOW_OK)
          goto handle_data_failed;
        break;
      default:
        GST_WARNING_OBJECT (src, "ignoring unknown message type %d",
            message.type);
        break;
    }
  }
  g_assert_not_reached ();

  /* ERRORS */
server_eof:
  {
    GST_DEBUG_OBJECT (src, "we got an eof from the server");
    GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
        ("The server closed the connection."));
    src->conninfo.connected = FALSE;
    gst_rtsp_message_unset (&message);
    return GST_FLOW_EOS;
  }
interrupt:
  {
    gst_rtsp_message_unset (&message);
    GST_DEBUG_OBJECT (src, "got interrupted");
    return GST_FLOW_FLUSHING;
  }
receive_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("Could not receive message. (%s)", str));
    g_free (str);

    gst_rtsp_message_unset (&message);
    return GST_FLOW_ERROR;
  }
handle_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
        ("Could not handle server message. (%s)", str));
    g_free (str);
    gst_rtsp_message_unset (&message);
    return GST_FLOW_ERROR;
  }
handle_data_failed:
  {
    GST_DEBUG_OBJECT (src, "could no handle data message");
    return ret;
  }
}

static GstFlowReturn
gst_rtspsrc_loop_udp (GstRTSPSrc * src)
{
  GstRTSPResult res;
  GstRTSPMessage message = { 0 };
  gint retry = 0;

  while (TRUE) {
    GTimeVal tv_timeout;

    /* get the next timeout interval */
    gst_rtsp_connection_next_timeout (src->conninfo.connection, &tv_timeout);

    GST_DEBUG_OBJECT (src, "doing receive with timeout %d seconds",
        (gint) tv_timeout.tv_sec);

    gst_rtsp_message_unset (&message);

    /* we should continue reading the TCP socket because the server might
     * send us requests. When the session timeout expires, we need to send a
     * keep-alive request to keep the session open. */
    res = gst_rtspsrc_connection_receive (src, &src->conninfo,
        &message, &tv_timeout);

    switch (res) {
      case GST_RTSP_OK:
        GST_DEBUG_OBJECT (src, "we received a server message");
        break;
      case GST_RTSP_EINTR:
        /* we got interrupted, see what we have to do */
        goto interrupt;
      case GST_RTSP_ETIMEOUT:
        /* send keep-alive, ignore the result, a warning will be posted. */
        GST_DEBUG_OBJECT (src, "timeout, sending keep-alive");
        if ((res = gst_rtspsrc_send_keep_alive (src)) == GST_RTSP_EINTR)
          goto interrupt;
        continue;
      case GST_RTSP_EEOF:
        /* server closed the connection. not very fatal for UDP, reconnect and
         * see what happens. */
        GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
            ("The server closed the connection."));
        if (src->udp_reconnect) {
          if ((res =
                  gst_rtsp_conninfo_reconnect (src, &src->conninfo, FALSE)) < 0)
            goto connect_error;
        } else {
          goto server_eof;
        }
        continue;
      case GST_RTSP_ENET:
        GST_DEBUG_OBJECT (src, "An ethernet problem occurred.");
      default:
        GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
            ("Unhandled return value %d.", res));
        goto receive_error;
    }

    switch (message.type) {
      case GST_RTSP_MESSAGE_REQUEST:
        /* server sends us a request message, handle it */
        res = gst_rtspsrc_handle_request (src, &src->conninfo, &message);
        if (res == GST_RTSP_EEOF)
          goto server_eof;
        else if (res < 0)
          goto handle_request_failed;
        break;
      case GST_RTSP_MESSAGE_RESPONSE:
        /* we ignore response and data messages */
        GST_DEBUG_OBJECT (src, "ignoring response message");
        DEBUG_RTSP (src, &message);
        if (message.type_data.response.code == GST_RTSP_STS_UNAUTHORIZED) {
          GST_DEBUG_OBJECT (src, "but is Unauthorized response ...");
          if (gst_rtspsrc_setup_auth (src, &message) && !(retry++)) {
            GST_DEBUG_OBJECT (src, "so retrying keep-alive");
            if ((res = gst_rtspsrc_send_keep_alive (src)) == GST_RTSP_EINTR)
              goto interrupt;
          }
        } else {
          retry = 0;
        }
        break;
      case GST_RTSP_MESSAGE_DATA:
        /* we ignore response and data messages */
        GST_DEBUG_OBJECT (src, "ignoring data message");
        break;
      default:
        GST_WARNING_OBJECT (src, "ignoring unknown message type %d",
            message.type);
        break;
    }
  }
  g_assert_not_reached ();

  /* we get here when the connection got interrupted */
interrupt:
  {
    gst_rtsp_message_unset (&message);
    GST_DEBUG_OBJECT (src, "got interrupted");
    return GST_FLOW_FLUSHING;
  }
connect_error:
  {
    gchar *str = gst_rtsp_strresult (res);
    GstFlowReturn ret;

    src->conninfo.connected = FALSE;
    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ_WRITE, (NULL),
          ("Could not connect to server. (%s)", str));
      g_free (str);
      ret = GST_FLOW_ERROR;
    } else {
      ret = GST_FLOW_FLUSHING;
    }
    return ret;
  }
receive_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("Could not receive message. (%s)", str));
    g_free (str);
    return GST_FLOW_ERROR;
  }
handle_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);
    GstFlowReturn ret;

    gst_rtsp_message_unset (&message);
    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
          ("Could not handle server message. (%s)", str));
      g_free (str);
      ret = GST_FLOW_ERROR;
    } else {
      ret = GST_FLOW_FLUSHING;
    }
    return ret;
  }
server_eof:
  {
    GST_DEBUG_OBJECT (src, "we got an eof from the server");
    GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
        ("The server closed the connection."));
    src->conninfo.connected = FALSE;
    gst_rtsp_message_unset (&message);
    return GST_FLOW_EOS;
  }
}

static GstRTSPResult
gst_rtspsrc_reconnect (GstRTSPSrc * src, gboolean async)
{
  GstRTSPResult res = GST_RTSP_OK;
  gboolean restart;

  GST_DEBUG_OBJECT (src, "doing reconnect");

  GST_OBJECT_LOCK (src);
  /* only restart when the pads were not yet activated, else we were
   * streaming over UDP */
  restart = src->need_activate;
  GST_OBJECT_UNLOCK (src);

  /* no need to restart, we're done */
  if (!restart)
    goto done;

  /* we can try only TCP now */
  src->cur_protocols = GST_RTSP_LOWER_TRANS_TCP;

  /* close and cleanup our state */
  if ((res = gst_rtspsrc_close (src, async, FALSE)) < 0)
    goto done;

  /* see if we have TCP left to try. Also don't try TCP when we were configured
   * with an SDP. */
  if (!(src->protocols & GST_RTSP_LOWER_TRANS_TCP) || src->from_sdp)
    goto no_protocols;

  /* We post a warning message now to inform the user
   * that nothing happened. It's most likely a firewall thing. */
  GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
      ("Could not receive any UDP packets for %.4f seconds, maybe your "
          "firewall is blocking it. Retrying using a tcp connection.",
          gst_guint64_to_gdouble (src->udp_timeout) / 1000000.0));

  /* open new connection using tcp */
  if (gst_rtspsrc_open (src, async) < 0)
    goto open_failed;

  /* start playback */
  if (gst_rtspsrc_play (src, &src->segment, async, NULL) < 0)
    goto play_failed;

done:
  return res;

  /* ERRORS */
no_protocols:
  {
    src->cur_protocols = 0;
    /* no transport possible, post an error and stop */
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("Could not receive any UDP packets for %.4f seconds, maybe your "
            "firewall is blocking it. No other protocols to try.",
            gst_guint64_to_gdouble (src->udp_timeout) / 1000000.0));
    return GST_RTSP_ERROR;
  }
open_failed:
  {
    GST_DEBUG_OBJECT (src, "open failed");
    return GST_RTSP_OK;
  }
play_failed:
  {
    GST_DEBUG_OBJECT (src, "play failed");
    return GST_RTSP_OK;
  }
}

static void
gst_rtspsrc_loop_start_cmd (GstRTSPSrc * src, gint cmd)
{
  switch (cmd) {
    case CMD_OPEN:
      GST_ELEMENT_PROGRESS (src, START, "open", ("Opening Stream"));
      break;
    case CMD_PLAY:
      GST_ELEMENT_PROGRESS (src, START, "request", ("Sending PLAY request"));
      break;
    case CMD_PAUSE:
      GST_ELEMENT_PROGRESS (src, START, "request", ("Sending PAUSE request"));
      break;
    case CMD_GET_PARAMETER:
      GST_ELEMENT_PROGRESS (src, START, "request",
          ("Sending GET_PARAMETER request"));
      break;
    case CMD_SET_PARAMETER:
      GST_ELEMENT_PROGRESS (src, START, "request",
          ("Sending SET_PARAMETER request"));
      break;
    case CMD_CLOSE:
      GST_ELEMENT_PROGRESS (src, START, "close", ("Closing Stream"));
      break;
    default:
      break;
  }
}

static void
gst_rtspsrc_loop_complete_cmd (GstRTSPSrc * src, gint cmd)
{
  switch (cmd) {
    case CMD_OPEN:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "open", ("Opened Stream"));
      break;
    case CMD_PLAY:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "request", ("Sent PLAY request"));
      break;
    case CMD_PAUSE:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "request", ("Sent PAUSE request"));
      break;
    case CMD_GET_PARAMETER:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "request",
          ("Sent GET_PARAMETER request"));
      break;
    case CMD_SET_PARAMETER:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "request",
          ("Sent SET_PARAMETER request"));
      break;
    case CMD_CLOSE:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "close", ("Closed Stream"));
      break;
    default:
      break;
  }
}

static void
gst_rtspsrc_loop_cancel_cmd (GstRTSPSrc * src, gint cmd)
{
  switch (cmd) {
    case CMD_OPEN:
      GST_ELEMENT_PROGRESS (src, CANCELED, "open", ("Open canceled"));
      break;
    case CMD_PLAY:
      GST_ELEMENT_PROGRESS (src, CANCELED, "request", ("PLAY canceled"));
      break;
    case CMD_PAUSE:
      GST_ELEMENT_PROGRESS (src, CANCELED, "request", ("PAUSE canceled"));
      break;
    case CMD_GET_PARAMETER:
      GST_ELEMENT_PROGRESS (src, CANCELED, "request",
          ("GET_PARAMETER canceled"));
      break;
    case CMD_SET_PARAMETER:
      GST_ELEMENT_PROGRESS (src, CANCELED, "request",
          ("SET_PARAMETER canceled"));
      break;
    case CMD_CLOSE:
      GST_ELEMENT_PROGRESS (src, CANCELED, "close", ("Close canceled"));
      break;
    default:
      break;
  }
}

static void
gst_rtspsrc_loop_error_cmd (GstRTSPSrc * src, gint cmd)
{
  switch (cmd) {
    case CMD_OPEN:
      GST_ELEMENT_PROGRESS (src, ERROR, "open", ("Open failed"));
      break;
    case CMD_PLAY:
      GST_ELEMENT_PROGRESS (src, ERROR, "request", ("PLAY failed"));
      break;
    case CMD_PAUSE:
      GST_ELEMENT_PROGRESS (src, ERROR, "request", ("PAUSE failed"));
      break;
    case CMD_GET_PARAMETER:
      GST_ELEMENT_PROGRESS (src, ERROR, "request", ("GET_PARAMETER failed"));
      break;
    case CMD_SET_PARAMETER:
      GST_ELEMENT_PROGRESS (src, ERROR, "request", ("SET_PARAMETER failed"));
      break;
    case CMD_CLOSE:
      GST_ELEMENT_PROGRESS (src, ERROR, "close", ("Close failed"));
      break;
    default:
      break;
  }
}

static void
gst_rtspsrc_loop_end_cmd (GstRTSPSrc * src, gint cmd, GstRTSPResult ret)
{
  if (ret == GST_RTSP_OK)
    gst_rtspsrc_loop_complete_cmd (src, cmd);
  else if (ret == GST_RTSP_EINTR)
    gst_rtspsrc_loop_cancel_cmd (src, cmd);
  else
    gst_rtspsrc_loop_error_cmd (src, cmd);
}

static gboolean
gst_rtspsrc_loop_send_cmd (GstRTSPSrc * src, gint cmd, gint mask)
{
  gint old;
  gboolean flushed = FALSE;

  /* start new request */
  gst_rtspsrc_loop_start_cmd (src, cmd);

  GST_DEBUG_OBJECT (src, "sending cmd %s", cmd_to_string (cmd));

  GST_OBJECT_LOCK (src);
  old = src->pending_cmd;

  if (old == CMD_RECONNECT) {
    GST_DEBUG_OBJECT (src, "ignore, we were reconnecting");
    cmd = CMD_RECONNECT;
  } else if (old == CMD_CLOSE) {
    /* our CMD_CLOSE might have interrutped CMD_LOOP. gst_rtspsrc_loop
     * will send a CMD_WAIT which would cancel our pending CMD_CLOSE (if
     * still pending). We just avoid it here by making sure CMD_CLOSE is
     * still the pending command. */
    GST_DEBUG_OBJECT (src, "ignore, we were closing");
    cmd = CMD_CLOSE;
  } else if (old == CMD_SET_PARAMETER) {
    GST_DEBUG_OBJECT (src, "ignore, we have a pending %s", cmd_to_string (old));
    cmd = CMD_SET_PARAMETER;
  } else if (old == CMD_GET_PARAMETER) {
    GST_DEBUG_OBJECT (src, "ignore, we have a pending %s", cmd_to_string (old));
    cmd = CMD_GET_PARAMETER;
  } else if (old != CMD_WAIT) {
    src->pending_cmd = CMD_WAIT;
    GST_OBJECT_UNLOCK (src);
    /* cancel previous request */
    GST_DEBUG_OBJECT (src, "cancel previous request %s", cmd_to_string (old));
    gst_rtspsrc_loop_cancel_cmd (src, old);
    GST_OBJECT_LOCK (src);
  }
  src->pending_cmd = cmd;
  /* interrupt if allowed */
  if (src->busy_cmd & mask) {
    GST_DEBUG_OBJECT (src, "connection flush busy %s",
        cmd_to_string (src->busy_cmd));
    gst_rtspsrc_connection_flush (src, TRUE);
    flushed = TRUE;
  } else {
    GST_DEBUG_OBJECT (src, "not interrupting busy cmd %s",
        cmd_to_string (src->busy_cmd));
  }
  if (src->task)
    gst_task_start (src->task);
  GST_OBJECT_UNLOCK (src);

  return flushed;
}

static gboolean
gst_rtspsrc_loop_send_cmd_and_wait (GstRTSPSrc * src, gint cmd, gint mask,
    GstClockTime timeout)
{
  gboolean flushed = gst_rtspsrc_loop_send_cmd (src, cmd, mask);

  if (timeout > 0) {
    gint64 end_time = g_get_monotonic_time () + (timeout / 1000);
    GST_OBJECT_LOCK (src);
    while (src->pending_cmd == cmd || src->busy_cmd == cmd) {
      if (!g_cond_wait_until (&src->cmd_cond, GST_OBJECT_GET_LOCK (src),
              end_time)) {
        GST_WARNING_OBJECT (src,
            "Timed out waiting for TEARDOWN to be processed.");
        break;                  /* timeout passed */
      }
    }
    GST_OBJECT_UNLOCK (src);
  }
  return flushed;
}

static gboolean
gst_rtspsrc_loop (GstRTSPSrc * src)
{
  GstFlowReturn ret;

  if (!src->conninfo.connection || !src->conninfo.connected)
    goto no_connection;

  if (src->interleaved)
    ret = gst_rtspsrc_loop_interleaved (src);
  else
    ret = gst_rtspsrc_loop_udp (src);

  if (ret != GST_FLOW_OK)
    goto pause;

  return TRUE;

  /* ERRORS */
no_connection:
  {
    GST_WARNING_OBJECT (src, "we are not connected");
    ret = GST_FLOW_FLUSHING;
    goto pause;
  }
pause:
  {
    const gchar *reason = gst_flow_get_name (ret);

    GST_DEBUG_OBJECT (src, "pausing task, reason %s", reason);
    src->running = FALSE;
    if (ret == GST_FLOW_EOS) {
      /* perform EOS logic */
      if (src->segment.flags & GST_SEEK_FLAG_SEGMENT) {
        gst_element_post_message (GST_ELEMENT_CAST (src),
            gst_message_new_segment_done (GST_OBJECT_CAST (src),
                src->segment.format, src->segment.position));
        gst_rtspsrc_push_event (src,
            gst_event_new_segment_done (src->segment.format,
                src->segment.position));
      } else {
        gst_rtspsrc_push_event (src, gst_event_new_eos ());
      }
    } else if (ret == GST_FLOW_NOT_LINKED || ret < GST_FLOW_EOS) {
      /* for fatal errors we post an error message, post the error before the
       * EOS so the app knows about the error first. */
      GST_ELEMENT_FLOW_ERROR (src, ret);
      gst_rtspsrc_push_event (src, gst_event_new_eos ());
    }
    gst_rtspsrc_loop_send_cmd (src, CMD_WAIT, CMD_LOOP);
    return FALSE;
  }
}

#ifndef GST_DISABLE_GST_DEBUG
static const gchar *
gst_rtsp_auth_method_to_string (GstRTSPAuthMethod method)
{
  gint index = 0;

  while (method != 0) {
    index++;
    method >>= 1;
  }
  switch (index) {
    case 0:
      return "None";
    case 1:
      return "Basic";
    case 2:
      return "Digest";
  }

  return "Unknown";
}
#endif

/* Parse a WWW-Authenticate Response header and determine the
 * available authentication methods
 *
 * This code should also cope with the fact that each WWW-Authenticate
 * header can contain multiple challenge methods + tokens
 *
 * At the moment, for Basic auth, we just do a minimal check and don't
 * even parse out the realm */
static void
gst_rtspsrc_parse_auth_hdr (GstRTSPMessage * response,
    GstRTSPAuthMethod * methods, GstRTSPConnection * conn, gboolean * stale)
{
  GstRTSPAuthCredential **credentials, **credential;

  g_return_if_fail (response != NULL);
  g_return_if_fail (methods != NULL);
  g_return_if_fail (stale != NULL);

  credentials =
      gst_rtsp_message_parse_auth_credentials (response,
      GST_RTSP_HDR_WWW_AUTHENTICATE);
  if (!credentials)
    return;

  credential = credentials;
  while (*credential) {
    if ((*credential)->scheme == GST_RTSP_AUTH_BASIC) {
      *methods |= GST_RTSP_AUTH_BASIC;
    } else if ((*credential)->scheme == GST_RTSP_AUTH_DIGEST) {
      GstRTSPAuthParam **param = (*credential)->params;

      *methods |= GST_RTSP_AUTH_DIGEST;

      gst_rtsp_connection_clear_auth_params (conn);
      *stale = FALSE;

      while (*param) {
        if (strcmp ((*param)->name, "stale") == 0
            && g_ascii_strcasecmp ((*param)->value, "TRUE") == 0)
          *stale = TRUE;
        gst_rtsp_connection_set_auth_param (conn, (*param)->name,
            (*param)->value);
        param++;
      }
    }

    credential++;
  }

  gst_rtsp_auth_credentials_free (credentials);
}

/**
 * gst_rtspsrc_setup_auth:
 * @src: the rtsp source
 *
 * Configure a username and password and auth method on the
 * connection object based on a response we received from the
 * peer.
 *
 * Currently, this requires that a username and password were supplied
 * in the uri. In the future, they may be requested on demand by sending
 * a message up the bus.
 *
 * Returns: TRUE if authentication information could be set up correctly.
 */
static gboolean
gst_rtspsrc_setup_auth (GstRTSPSrc * src, GstRTSPMessage * response)
{
  gchar *user = NULL;
  gchar *pass = NULL;
  GstRTSPAuthMethod avail_methods = GST_RTSP_AUTH_NONE;
  GstRTSPAuthMethod method;
  GstRTSPResult auth_result;
  GstRTSPUrl *url;
  GstRTSPConnection *conn;
  gboolean stale = FALSE;

  conn = src->conninfo.connection;

  /* Identify the available auth methods and see if any are supported */
  gst_rtspsrc_parse_auth_hdr (response, &avail_methods, conn, &stale);

  if (avail_methods == GST_RTSP_AUTH_NONE)
    goto no_auth_available;

  /* For digest auth, if the response indicates that the session
   * data are stale, we just update them in the connection object and
   * return TRUE to retry the request */
  if (stale)
    src->tried_url_auth = FALSE;

  url = gst_rtsp_connection_get_url (conn);

  /* Do we have username and password available? */
  if (url != NULL && !src->tried_url_auth && url->user != NULL
      && url->passwd != NULL) {
    user = url->user;
    pass = url->passwd;
    src->tried_url_auth = TRUE;
    GST_DEBUG_OBJECT (src,
        "Attempting authentication using credentials from the URL");
  } else {
    user = src->user_id;
    pass = src->user_pw;
    GST_DEBUG_OBJECT (src,
        "Attempting authentication using credentials from the properties");
  }

  /* FIXME: If the url didn't contain username and password or we tried them
   * already, request a username and passwd from the application via some kind
   * of credentials request message */

  /* If we don't have a username and passwd at this point, bail out. */
  if (user == NULL || pass == NULL)
    goto no_user_pass;

  /* Try to configure for each available authentication method, strongest to
   * weakest */
  for (method = GST_RTSP_AUTH_MAX; method != GST_RTSP_AUTH_NONE; method >>= 1) {
    /* Check if this method is available on the server */
    if ((method & avail_methods) == 0)
      continue;

    /* Pass the credentials to the connection to try on the next request */
    auth_result = gst_rtsp_connection_set_auth (conn, method, user, pass);
    /* INVAL indicates an invalid username/passwd were supplied, so we'll just
     * ignore it and end up retrying later */
    if (auth_result == GST_RTSP_OK || auth_result == GST_RTSP_EINVAL) {
      GST_DEBUG_OBJECT (src, "Attempting %s authentication",
          gst_rtsp_auth_method_to_string (method));
      break;
    }
  }

  if (method == GST_RTSP_AUTH_NONE)
    goto no_auth_available;

  return TRUE;

no_auth_available:
  {
    /* Output an error indicating that we couldn't connect because there were
     * no supported authentication protocols */
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
        ("No supported authentication protocol was found"));
    return FALSE;
  }
no_user_pass:
  {
    /* We don't fire an error message, we just return FALSE and let the
     * normal NOT_AUTHORIZED error be propagated */
    return FALSE;
  }
}

static GstRTSPResult
gst_rtsp_src_receive_response (GstRTSPSrc * src, GstRTSPConnInfo * conninfo,
    GstRTSPMessage * response, GstRTSPStatusCode * code)
{
  GstRTSPStatusCode thecode;
  gchar *content_base = NULL;
  GstRTSPResult res = gst_rtspsrc_connection_receive (src, conninfo,
      response, src->ptcp_timeout);

  if (res < 0)
    goto receive_error;

  DEBUG_RTSP (src, response);

  switch (response->type) {
    case GST_RTSP_MESSAGE_REQUEST:
      res = gst_rtspsrc_handle_request (src, conninfo, response);
      if (res == GST_RTSP_EEOF)
        goto server_eof;
      else if (res < 0)
        goto handle_request_failed;

      /* Not a response, receive next message */
      return gst_rtsp_src_receive_response (src, conninfo, response, code);
    case GST_RTSP_MESSAGE_RESPONSE:
      /* ok, a response is good */
      GST_DEBUG_OBJECT (src, "received response message");
      break;
    case GST_RTSP_MESSAGE_DATA:
      /* get next response */
      GST_DEBUG_OBJECT (src, "handle data response message");
      gst_rtspsrc_handle_data (src, response);

      /* Not a response, receive next message */
      return gst_rtsp_src_receive_response (src, conninfo, response, code);
    default:
      GST_WARNING_OBJECT (src, "ignoring unknown message type %d",
          response->type);

      /* Not a response, receive next message */
      return gst_rtsp_src_receive_response (src, conninfo, response, code);
  }

  thecode = response->type_data.response.code;

  GST_DEBUG_OBJECT (src, "got response message %d", thecode);

  /* if the caller wanted the result code, we store it. */
  if (code)
    *code = thecode;

  /* If the request didn't succeed, bail out before doing any more */
  if (thecode != GST_RTSP_STS_OK)
    return GST_RTSP_OK;

  /* store new content base if any */
  gst_rtsp_message_get_header (response, GST_RTSP_HDR_CONTENT_BASE,
      &content_base, 0);
  if (content_base) {
    g_free (src->content_base);
    src->content_base = g_strdup (content_base);
  }

  return GST_RTSP_OK;

  /* ERRORS */
receive_error:
  {
    switch (res) {
      case GST_RTSP_EEOF:
        return GST_RTSP_EEOF;
      default:
      {
        gchar *str = gst_rtsp_strresult (res);

        if (res != GST_RTSP_EINTR) {
          GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
              ("Could not receive message. (%s)", str));
        } else {
          GST_WARNING_OBJECT (src, "receive interrupted");
        }
        g_free (str);
        break;
      }
    }
    return res;
  }
handle_request_failed:
  {
    /* ERROR was posted */
    gst_rtsp_message_unset (response);
    return res;
  }
server_eof:
  {
    GST_DEBUG_OBJECT (src, "we got an eof from the server");
    GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
        ("The server closed the connection."));
    gst_rtsp_message_unset (response);
    return res;
  }
}


static GstRTSPResult
gst_rtspsrc_try_send (GstRTSPSrc * src, GstRTSPConnInfo * conninfo,
    GstRTSPMessage * request, GstRTSPMessage * response,
    GstRTSPStatusCode * code)
{
  GstRTSPResult res;
  gint try = 0;
  gboolean allow_send = TRUE;

again:
  if (!src->short_header)
    gst_rtsp_ext_list_before_send (src->extensions, request);

  g_signal_emit (src, gst_rtspsrc_signals[SIGNAL_BEFORE_SEND], 0,
      request, &allow_send);
  if (!allow_send) {
    GST_DEBUG_OBJECT (src, "skipping message, disabled by signal");
    return GST_RTSP_OK;
  }

  GST_DEBUG_OBJECT (src, "sending message");

  DEBUG_RTSP (src, request);

  res = gst_rtspsrc_connection_send (src, conninfo, request, src->ptcp_timeout);
  if (res < 0)
    goto send_error;

  gst_rtsp_connection_reset_timeout (conninfo->connection);
  if (!response)
    return res;

  res = gst_rtsp_src_receive_response (src, conninfo, response, code);
  if (res == GST_RTSP_EEOF) {
    GST_WARNING_OBJECT (src, "server closed connection");
    /* only try once after reconnect, then fallthrough and error out */
    if ((try == 0) && !src->interleaved && src->udp_reconnect) {
      try++;
      /* if reconnect succeeds, try again */
      if ((res = gst_rtsp_conninfo_reconnect (src, &src->conninfo, FALSE)) == 0)
        goto again;
    }
  }
  gst_rtsp_ext_list_after_send (src->extensions, request, response);

  return res;

send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
          ("Could not send message. (%s)", str));
    } else {
      GST_WARNING_OBJECT (src, "send interrupted");
    }
    g_free (str);
    return res;
  }
}

/**
 * gst_rtspsrc_send:
 * @src: the rtsp source
 * @conninfo: the connection information to send on
 * @request: must point to a valid request
 * @response: must point to an empty #GstRTSPMessage
 * @code: an optional code result
 * @versions: List of versions to try, setting it back onto the @request message
 *            if not set, `src->version` will be used as RTSP version.
 *
 * send @request and retrieve the response in @response. optionally @code can be
 * non-NULL in which case it will contain the status code of the response.
 *
 * If This function returns #GST_RTSP_OK, @response will contain a valid response
 * message that should be cleaned with gst_rtsp_message_unset() after usage.
 *
 * If @code is NULL, this function will return #GST_RTSP_ERROR (with an invalid
 * @response message) if the response code was not 200 (OK).
 *
 * If the attempt results in an authentication failure, then this will attempt
 * to retrieve authentication credentials via gst_rtspsrc_setup_auth and retry
 * the request.
 *
 * Returns: #GST_RTSP_OK if the processing was successful.
 */
static GstRTSPResult
gst_rtspsrc_send (GstRTSPSrc * src, GstRTSPConnInfo * conninfo,
    GstRTSPMessage * request, GstRTSPMessage * response,
    GstRTSPStatusCode * code, GstRTSPVersion * versions)
{
  GstRTSPStatusCode int_code = GST_RTSP_STS_OK;
  GstRTSPResult res = GST_RTSP_ERROR;
  gint count;
  gboolean retry;
  GstRTSPMethod method = GST_RTSP_INVALID;
  gint version_retry = 0;

  count = 0;
  do {
    retry = FALSE;

    /* make sure we don't loop forever */
    if (count++ > 8)
      break;

    /* save method so we can disable it when the server complains */
    method = request->type_data.request.method;

    if (!versions)
      request->type_data.request.version = src->version;

    if ((res =
            gst_rtspsrc_try_send (src, conninfo, request, response,
                &int_code)) < 0)
      goto error;

    switch (int_code) {
      case GST_RTSP_STS_UNAUTHORIZED:
      case GST_RTSP_STS_NOT_FOUND:
        if (gst_rtspsrc_setup_auth (src, response)) {
          /* Try the request/response again after configuring the auth info
           * and loop again */
          retry = TRUE;
        }
        break;
      case GST_RTSP_STS_RTSP_VERSION_NOT_SUPPORTED:
        GST_INFO_OBJECT (src, "Version %s not supported by the server",
            versions ? gst_rtsp_version_as_text (versions[version_retry]) :
            "unknown");
        if (versions && versions[version_retry] != GST_RTSP_VERSION_INVALID) {
          GST_INFO_OBJECT (src, "Unsupported version %s => trying %s",
              gst_rtsp_version_as_text (request->type_data.request.version),
              gst_rtsp_version_as_text (versions[version_retry]));
          request->type_data.request.version = versions[version_retry];
          retry = TRUE;
          version_retry++;
          break;
        }
        /* fallthrough */
      default:
        break;
    }
  } while (retry == TRUE);

  /* If the user requested the code, let them handle errors, otherwise
   * post an error below */
  if (code != NULL)
    *code = int_code;
  else if (int_code != GST_RTSP_STS_OK)
    goto error_response;

  return res;

  /* ERRORS */
error:
  {
    GST_DEBUG_OBJECT (src, "got error %d", res);
    return res;
  }
error_response:
  {
    res = GST_RTSP_ERROR;

    switch (response->type_data.response.code) {
      case GST_RTSP_STS_NOT_FOUND:
        RTSP_SRC_RESPONSE_ERROR (src, response, RESOURCE, NOT_FOUND,
            "Not found");
        break;
      case GST_RTSP_STS_UNAUTHORIZED:
        RTSP_SRC_RESPONSE_ERROR (src, response, RESOURCE, NOT_AUTHORIZED,
            "Unauthorized");
        break;
      case GST_RTSP_STS_MOVED_PERMANENTLY:
      case GST_RTSP_STS_MOVE_TEMPORARILY:
      {
        gchar *new_location;
        GstRTSPLowerTrans transports;

        GST_DEBUG_OBJECT (src, "got redirection");
        /* if we don't have a Location Header, we must error */
        if (gst_rtsp_message_get_header (response, GST_RTSP_HDR_LOCATION,
                &new_location, 0) < 0)
          break;

        /* When we receive a redirect result, we go back to the INIT state after
         * parsing the new URI. The caller should do the needed steps to issue
         * a new setup when it detects this state change. */
        GST_DEBUG_OBJECT (src, "redirection to %s", new_location);

        /* save current transports */
        if (src->conninfo.url)
          transports = src->conninfo.url->transports;
        else
          transports = GST_RTSP_LOWER_TRANS_UNKNOWN;

        gst_rtspsrc_uri_set_uri (GST_URI_HANDLER (src), new_location, NULL);

        /* set old transports */
        if (src->conninfo.url && transports != GST_RTSP_LOWER_TRANS_UNKNOWN)
          src->conninfo.url->transports = transports;

        src->need_redirect = TRUE;
        res = GST_RTSP_OK;
        break;
      }
      case GST_RTSP_STS_NOT_ACCEPTABLE:
      case GST_RTSP_STS_NOT_IMPLEMENTED:
      case GST_RTSP_STS_METHOD_NOT_ALLOWED:
        GST_WARNING_OBJECT (src, "got NOT IMPLEMENTED, disable method %s",
            gst_rtsp_method_as_text (method));
        src->methods &= ~method;
        res = GST_RTSP_OK;
        break;
      default:
        RTSP_SRC_RESPONSE_ERROR (src, response, RESOURCE, READ,
            "Unhandled error");
        break;
    }
    /* if we return ERROR we should unset the response ourselves */
    if (res == GST_RTSP_ERROR)
      gst_rtsp_message_unset (response);

    return res;
  }
}

static GstRTSPResult
gst_rtspsrc_send_cb (GstRTSPExtension * ext, GstRTSPMessage * request,
    GstRTSPMessage * response, GstRTSPSrc * src)
{
  return gst_rtspsrc_send (src, &src->conninfo, request, response, NULL, NULL);
}


/* parse the response and collect all the supported methods. We need this
 * information so that we don't try to send an unsupported request to the
 * server.
 */
static gboolean
gst_rtspsrc_parse_methods (GstRTSPSrc * src, GstRTSPMessage * response)
{
  GstRTSPHeaderField field;
  gchar *respoptions;
  gint indx = 0;

  /* reset supported methods */
  src->methods = 0;

  /* Try Allow Header first */
  field = GST_RTSP_HDR_ALLOW;
  while (TRUE) {
    respoptions = NULL;
    gst_rtsp_message_get_header (response, field, &respoptions, indx);
    if (!respoptions)
      break;

    src->methods |= gst_rtsp_options_from_text (respoptions);

    indx++;
  }

  indx = 0;
  field = GST_RTSP_HDR_PUBLIC;
  while (TRUE) {
    respoptions = NULL;
    gst_rtsp_message_get_header (response, field, &respoptions, indx);
    if (!respoptions)
      break;

    src->methods |= gst_rtsp_options_from_text (respoptions);

    indx++;
  }

  if (src->methods == 0) {
    /* neither Allow nor Public are required, assume the server supports
     * at least DESCRIBE, SETUP, we always assume it supports PLAY as
     * well. */
    GST_DEBUG_OBJECT (src, "could not get OPTIONS");
    src->methods = GST_RTSP_DESCRIBE | GST_RTSP_SETUP;
  }
  /* always assume PLAY, FIXME, extensions should be able to override
   * this */
  src->methods |= GST_RTSP_PLAY;
  /* also assume it will support Range */
  src->seekable = G_MAXFLOAT;

  /* we need describe and setup */
  if (!(src->methods & GST_RTSP_DESCRIBE))
    goto no_describe;
  if (!(src->methods & GST_RTSP_SETUP))
    goto no_setup;

  return TRUE;

  /* ERRORS */
no_describe:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
        ("Server does not support DESCRIBE."));
    return FALSE;
  }
no_setup:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
        ("Server does not support SETUP."));
    return FALSE;
  }
}

/* masks to be kept in sync with the hardcoded protocol order of preference
 * in code below */
static const guint protocol_masks[] = {
  GST_RTSP_LOWER_TRANS_UDP,
  GST_RTSP_LOWER_TRANS_UDP_MCAST,
  GST_RTSP_LOWER_TRANS_TCP,
  0
};

static GstRTSPResult
gst_rtspsrc_create_transports_string (GstRTSPSrc * src,
    GstRTSPLowerTrans protocols, GstRTSPProfile profile, gchar ** transports)
{
  GstRTSPResult res;
  GString *result;
  gboolean add_udp_str;

  *transports = NULL;

  res =
      gst_rtsp_ext_list_get_transports (src->extensions, protocols, transports);

  if (res < 0)
    goto failed;

  GST_DEBUG_OBJECT (src, "got transports %s", GST_STR_NULL (*transports));

  /* extension listed transports, use those */
  if (*transports != NULL)
    return GST_RTSP_OK;

  /* it's the default */
  add_udp_str = FALSE;

  /* the default RTSP transports */
  result = g_string_new ("RTP");

  switch (profile) {
    case GST_RTSP_PROFILE_AVP:
      g_string_append (result, "/AVP");
      break;
    case GST_RTSP_PROFILE_SAVP:
      g_string_append (result, "/SAVP");
      break;
    case GST_RTSP_PROFILE_AVPF:
      g_string_append (result, "/AVPF");
      break;
    case GST_RTSP_PROFILE_SAVPF:
      g_string_append (result, "/SAVPF");
      break;
    default:
      break;
  }

  if (protocols & GST_RTSP_LOWER_TRANS_UDP) {
    GST_DEBUG_OBJECT (src, "adding UDP unicast");
    if (add_udp_str)
      g_string_append (result, "/UDP");
    g_string_append (result, ";unicast;client_port=%%u1-%%u2");
  } else if (protocols & GST_RTSP_LOWER_TRANS_UDP_MCAST) {
    GST_DEBUG_OBJECT (src, "adding UDP multicast");
    /* we don't have to allocate any UDP ports yet, if the selected transport
     * turns out to be multicast we can create them and join the multicast
     * group indicated in the transport reply */
    if (add_udp_str)
      g_string_append (result, "/UDP");
    g_string_append (result, ";multicast");
    if (src->next_port_num != 0) {
      if (src->client_port_range.max > 0 &&
          src->next_port_num >= src->client_port_range.max)
        goto no_ports;

      g_string_append_printf (result, ";client_port=%d-%d",
          src->next_port_num, src->next_port_num + 1);
    }
  } else if (protocols & GST_RTSP_LOWER_TRANS_TCP) {
    GST_DEBUG_OBJECT (src, "adding TCP");

    g_string_append (result, "/TCP;unicast;interleaved=%%i1-%%i2");
  }
  *transports = g_string_free (result, FALSE);

  GST_DEBUG_OBJECT (src, "prepared transports %s", GST_STR_NULL (*transports));

  return GST_RTSP_OK;

  /* ERRORS */
failed:
  {
    GST_ERROR ("extension gave error %d", res);
    return res;
  }
no_ports:
  {
    GST_ERROR ("no more ports available");
    return GST_RTSP_ERROR;
  }
}

static GstRTSPResult
gst_rtspsrc_prepare_transports (GstRTSPStream * stream, gchar ** transports,
    gint orig_rtpport, gint orig_rtcpport)
{
  GstRTSPSrc *src;
  gint nr_udp, nr_int;
  gchar *next, *p;
  gint rtpport = 0, rtcpport = 0;
  GString *str;

  src = stream->parent;

  /* find number of placeholders first */
  if (strstr (*transports, "%%i2"))
    nr_int = 2;
  else if (strstr (*transports, "%%i1"))
    nr_int = 1;
  else
    nr_int = 0;

  if (strstr (*transports, "%%u2"))
    nr_udp = 2;
  else if (strstr (*transports, "%%u1"))
    nr_udp = 1;
  else
    nr_udp = 0;

  if (nr_udp == 0 && nr_int == 0)
    goto done;

  if (nr_udp > 0) {
    if (!orig_rtpport || !orig_rtcpport) {
      if (!gst_rtspsrc_alloc_udp_ports (stream, &rtpport, &rtcpport))
        goto failed;
    } else {
      rtpport = orig_rtpport;
      rtcpport = orig_rtcpport;
    }
  }

  str = g_string_new ("");
  p = *transports;
  while ((next = strstr (p, "%%"))) {
    g_string_append_len (str, p, next - p);
    if (next[2] == 'u') {
      if (next[3] == '1')
        g_string_append_printf (str, "%d", rtpport);
      else if (next[3] == '2')
        g_string_append_printf (str, "%d", rtcpport);
    }
    if (next[2] == 'i') {
      if (next[3] == '1')
        g_string_append_printf (str, "%d", src->free_channel);
      else if (next[3] == '2')
        g_string_append_printf (str, "%d", src->free_channel + 1);

    }

    p = next + 4;
  }
  if (src->version >= GST_RTSP_VERSION_2_0)
    src->free_channel += 2;

  /* append final part */
  g_string_append (str, p);

  g_free (*transports);
  *transports = g_string_free (str, FALSE);

done:
  return GST_RTSP_OK;

  /* ERRORS */
failed:
  {
    GST_ERROR ("failed to allocate udp ports");
    return GST_RTSP_ERROR;
  }
}

static GstCaps *
signal_get_srtcp_params (GstRTSPSrc * src, GstRTSPStream * stream)
{
  GstCaps *caps = NULL;

  g_signal_emit (src, gst_rtspsrc_signals[SIGNAL_REQUEST_RTCP_KEY], 0,
      stream->id, &caps);

  if (caps != NULL)
    GST_DEBUG_OBJECT (src, "SRTP parameters received");

  return caps;
}

static GstCaps *
default_srtcp_params (void)
{
  guint i;
  GstCaps *caps;
  GstBuffer *buf;
  guint8 *key_data;
#define KEY_SIZE 30
  guint data_size = GST_ROUND_UP_4 (KEY_SIZE);

  /* create a random key */
  key_data = g_malloc (data_size);
  for (i = 0; i < data_size; i += 4)
    GST_WRITE_UINT32_BE (key_data + i, g_random_int ());

  buf = gst_buffer_new_wrapped (key_data, KEY_SIZE);

  caps = gst_caps_new_simple ("application/x-srtcp",
      "srtp-key", GST_TYPE_BUFFER, buf,
      "srtp-cipher", G_TYPE_STRING, "aes-128-icm",
      "srtp-auth", G_TYPE_STRING, "hmac-sha1-80",
      "srtcp-cipher", G_TYPE_STRING, "aes-128-icm",
      "srtcp-auth", G_TYPE_STRING, "hmac-sha1-80", NULL);

  gst_buffer_unref (buf);

  return caps;
}

static gchar *
gst_rtspsrc_stream_make_keymgmt (GstRTSPSrc * src, GstRTSPStream * stream)
{
  gchar *base64, *result = NULL;
  GstMIKEYMessage *mikey_msg;

  stream->srtcpparams = signal_get_srtcp_params (src, stream);
  if (stream->srtcpparams == NULL)
    stream->srtcpparams = default_srtcp_params ();

  mikey_msg = gst_mikey_message_new_from_caps (stream->srtcpparams);
  if (mikey_msg) {
    /* add policy '0' for our SSRC */
    gst_mikey_message_add_cs_srtp (mikey_msg, 0, stream->send_ssrc, 0);

    base64 = gst_mikey_message_base64_encode (mikey_msg);
    gst_mikey_message_unref (mikey_msg);

    if (base64) {
      result = gst_sdp_make_keymgmt (stream->conninfo.location, base64);
      g_free (base64);
    }
  }

  return result;
}

static GstRTSPResult
gst_rtsp_src_setup_stream_from_response (GstRTSPSrc * src,
    GstRTSPStream * stream, GstRTSPMessage * response,
    GstRTSPLowerTrans * protocols, gint retry, gint * rtpport, gint * rtcpport)
{
  gchar *resptrans = NULL;
  GstRTSPTransport transport = { 0 };

  gst_rtsp_message_get_header (response, GST_RTSP_HDR_TRANSPORT, &resptrans, 0);
  if (!resptrans) {
    gst_rtspsrc_stream_free_udp (stream);
    goto no_transport;
  }

  /* parse transport, go to next stream on parse error */
  if (gst_rtsp_transport_parse (resptrans, &transport) != GST_RTSP_OK) {
    GST_WARNING_OBJECT (src, "failed to parse transport %s", resptrans);
    return GST_RTSP_ELAST;
  }

  /* update allowed transports for other streams. once the transport of
   * one stream has been determined, we make sure that all other streams
   * are configured in the same way */
  switch (transport.lower_transport) {
    case GST_RTSP_LOWER_TRANS_TCP:
      GST_DEBUG_OBJECT (src, "stream %p as TCP interleaved", stream);
      if (protocols)
        *protocols = GST_RTSP_LOWER_TRANS_TCP;
      src->interleaved = TRUE;
      if (src->version < GST_RTSP_VERSION_2_0) {
        /* update free channels */
        src->free_channel = MAX (transport.interleaved.min, src->free_channel);
        src->free_channel = MAX (transport.interleaved.max, src->free_channel);
        src->free_channel++;
      }
      break;
    case GST_RTSP_LOWER_TRANS_UDP_MCAST:
      /* only allow multicast for other streams */
      GST_DEBUG_OBJECT (src, "stream %p as UDP multicast", stream);
      if (protocols)
        *protocols = GST_RTSP_LOWER_TRANS_UDP_MCAST;
      /* if the server selected our ports, increment our counters so that
       * we select a new port later */
      if (src->next_port_num == transport.port.min &&
          src->next_port_num + 1 == transport.port.max) {
        src->next_port_num += 2;
      }
      break;
    case GST_RTSP_LOWER_TRANS_UDP:
      /* only allow unicast for other streams */
      GST_DEBUG_OBJECT (src, "stream %p as UDP unicast", stream);
      if (protocols)
        *protocols = GST_RTSP_LOWER_TRANS_UDP;
      break;
    default:
      GST_DEBUG_OBJECT (src, "stream %p unknown transport %d", stream,
          transport.lower_transport);
      break;
  }

  if (!src->interleaved || !retry) {
    /* now configure the stream with the selected transport */
    if (!gst_rtspsrc_stream_configure_transport (stream, &transport)) {
      GST_DEBUG_OBJECT (src,
          "could not configure stream %p transport, skipping stream", stream);
      goto done;
    } else if (stream->udpsrc[0] && stream->udpsrc[1] && rtpport && rtcpport) {
      /* retain the first allocated UDP port pair */
      g_object_get (G_OBJECT (stream->udpsrc[0]), "port", rtpport, NULL);
      g_object_get (G_OBJECT (stream->udpsrc[1]), "port", rtcpport, NULL);
    }
  }
  /* we need to activate at least one stream when we detect activity */
  src->need_activate = TRUE;

  /* stream is setup now */
  stream->setup = TRUE;
  stream->waiting_setup_response = FALSE;

  if (src->version >= GST_RTSP_VERSION_2_0) {
    gchar *prop, *media_properties;
    gchar **props;
    gint i;

    if (gst_rtsp_message_get_header (response, GST_RTSP_HDR_MEDIA_PROPERTIES,
            &media_properties, 0) != GST_RTSP_OK) {
      GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
          ("Error: No MEDIA_PROPERTY header in a SETUP request in RTSP 2.0"
              " - this header is mandatory."));

      gst_rtsp_message_unset (response);
      return GST_RTSP_ERROR;
    }

    props = g_strsplit (media_properties, ",", -2);
    for (i = 0; props[i]; i++) {
      prop = props[i];

      while (*prop == ' ')
        prop++;

      if (strstr (prop, "Random-Access")) {
        gchar **random_seekable_val = g_strsplit (prop, "=", 2);

        if (!random_seekable_val[1])
          src->seekable = G_MAXFLOAT;
        else
          src->seekable = g_ascii_strtod (random_seekable_val[1], NULL);

        g_strfreev (random_seekable_val);
      } else if (!g_strcmp0 (prop, "No-Seeking")) {
        src->seekable = -1.0;
      } else if (!g_strcmp0 (prop, "Beginning-Only")) {
        src->seekable = 0.0;
      }
    }

    g_strfreev (props);
  }

done:
  /* clean up our transport struct */
  gst_rtsp_transport_init (&transport);
  /* clean up used RTSP messages */
  gst_rtsp_message_unset (response);

  return GST_RTSP_OK;

no_transport:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("Server did not select transport."));

    gst_rtsp_message_unset (response);
    return GST_RTSP_ERROR;
  }
}

static GstRTSPResult
gst_rtspsrc_setup_streams_end (GstRTSPSrc * src, gboolean async)
{
  GList *tmp;
  GstRTSPConnInfo *conninfo;

  g_assert (src->version >= GST_RTSP_VERSION_2_0);

  conninfo = &src->conninfo;
  for (tmp = src->streams; tmp; tmp = tmp->next) {
    GstRTSPStream *stream = (GstRTSPStream *) tmp->data;
    GstRTSPMessage response = { 0, };

    if (!stream->waiting_setup_response)
      continue;

    if (!src->conninfo.connection)
      conninfo = &((GstRTSPStream *) tmp->data)->conninfo;

    gst_rtsp_src_receive_response (src, conninfo, &response, NULL);

    gst_rtsp_src_setup_stream_from_response (src, stream,
        &response, NULL, 0, NULL, NULL);
  }

  return GST_RTSP_OK;
}

/* Perform the SETUP request for all the streams.
 *
 * We ask the server for a specific transport, which initially includes all the
 * ones we can support (UDP/TCP/MULTICAST). For the UDP transport we allocate
 * two local UDP ports that we send to the server.
 *
 * Once the server replied with a transport, we configure the other streams
 * with the same transport.
 *
 * In case setup request are not pipelined, this function will also configure the
 * stream for the selected transport, * which basically means creating the pipeline.
 * Otherwise, the first stream is setup right away from the reply and a
 * CMD_FINALIZE_SETUP command is set for the stream pipelines to happen on the
 * remaining streams from the RTSP thread.
 */
static GstRTSPResult
gst_rtspsrc_setup_streams_start (GstRTSPSrc * src, gboolean async)
{
  GList *walk;
  GstRTSPResult res = GST_RTSP_ERROR;
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPStream *stream = NULL;
  GstRTSPLowerTrans protocols;
  GstRTSPStatusCode code;
  gboolean unsupported_real = FALSE;
  gint rtpport, rtcpport;
  GstRTSPUrl *url;
  gchar *hval;
  gchar *pipelined_request_id = NULL;

  if (src->conninfo.connection) {
    url = gst_rtsp_connection_get_url (src->conninfo.connection);
    /* we initially allow all configured lower transports. based on the URL
     * transports and the replies from the server we narrow them down. */
    protocols = url->transports & src->cur_protocols;
  } else {
    url = NULL;
    protocols = src->cur_protocols;
  }

  /* In ONVIF mode, we only want to try TCP transport */
  if (src->onvif_mode && (protocols & GST_RTSP_LOWER_TRANS_TCP))
    protocols = GST_RTSP_LOWER_TRANS_TCP;

  if (protocols == 0)
    goto no_protocols;

  /* reset some state */
  src->free_channel = 0;
  src->interleaved = FALSE;
  src->need_activate = FALSE;
  /* keep track of next port number, 0 is random */
  src->next_port_num = src->client_port_range.min;
  rtpport = rtcpport = 0;

  if (G_UNLIKELY (src->streams == NULL))
    goto no_streams;

  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPConnInfo *conninfo;
    gchar *transports;
    gint retry = 0;
    guint mask = 0;
    gboolean selected;
    GstCaps *caps;

    stream = (GstRTSPStream *) walk->data;

    caps = stream_get_caps_for_pt (stream, stream->default_pt);
    if (caps == NULL) {
      GST_WARNING_OBJECT (src, "skipping stream %p, no caps", stream);
      continue;
    }

    if (stream->skipped) {
      GST_DEBUG_OBJECT (src, "skipping stream %p", stream);
      continue;
    }

    /* see if we need to configure this stream */
    if (!gst_rtsp_ext_list_configure_stream (src->extensions, caps)) {
      GST_DEBUG_OBJECT (src, "skipping stream %p, disabled by extension",
          stream);
      continue;
    }

    g_signal_emit (src, gst_rtspsrc_signals[SIGNAL_SELECT_STREAM], 0,
        stream->id, caps, &selected);
    if (!selected) {
      GST_DEBUG_OBJECT (src, "skipping stream %p, disabled by signal", stream);
      continue;
    }

    /* merge/overwrite global caps */
    if (caps) {
      guint j, num;
      GstStructure *s;

      s = gst_caps_get_structure (caps, 0);

      num = gst_structure_n_fields (src->props);
      for (j = 0; j < num; j++) {
        const gchar *name;
        const GValue *val;

        name = gst_structure_nth_field_name (src->props, j);
        val = gst_structure_get_value (src->props, name);
        gst_structure_set_value (s, name, val);

        GST_DEBUG_OBJECT (src, "copied %s", name);
      }
    }

    /* skip setup if we have no URL for it */
    if (stream->conninfo.location == NULL) {
      GST_WARNING_OBJECT (src, "skipping stream %p, no setup", stream);
      continue;
    }

    if (src->conninfo.connection == NULL) {
      if (!gst_rtsp_conninfo_connect (src, &stream->conninfo, async)) {
        GST_WARNING_OBJECT (src, "skipping stream %p, failed to connect",
            stream);
        continue;
      }
      conninfo = &stream->conninfo;
    } else {
      conninfo = &src->conninfo;
    }
    GST_DEBUG_OBJECT (src, "doing setup of stream %p with %s", stream,
        stream->conninfo.location);

    /* if we have a multicast connection, only suggest multicast from now on */
    if (stream->is_multicast)
      protocols &= GST_RTSP_LOWER_TRANS_UDP_MCAST;

  next_protocol:
    /* first selectable protocol */
    while (protocol_masks[mask] && !(protocols & protocol_masks[mask]))
      mask++;
    if (!protocol_masks[mask])
      goto no_protocols;

  retry:
    GST_DEBUG_OBJECT (src, "protocols = 0x%x, protocol mask = 0x%x", protocols,
        protocol_masks[mask]);
    /* create a string with first transport in line */
    transports = NULL;
    res = gst_rtspsrc_create_transports_string (src,
        protocols & protocol_masks[mask], stream->profile, &transports);
    if (res < 0 || transports == NULL)
      goto setup_transport_failed;

    if (strlen (transports) == 0) {
      g_free (transports);
      GST_DEBUG_OBJECT (src, "no transports found");
      mask++;
      goto next_protocol;
    }

    GST_DEBUG_OBJECT (src, "replace ports in %s", GST_STR_NULL (transports));

    /* replace placeholders with real values, this function will optionally
     * allocate UDP ports and other info needed to execute the setup request */
    res = gst_rtspsrc_prepare_transports (stream, &transports,
        retry > 0 ? rtpport : 0, retry > 0 ? rtcpport : 0);
    if (res < 0) {
      g_free (transports);
      goto setup_transport_failed;
    }

    GST_DEBUG_OBJECT (src, "transport is now %s", GST_STR_NULL (transports));
    /* create SETUP request */
    res =
        gst_rtspsrc_init_request (src, &request, GST_RTSP_SETUP,
        stream->conninfo.location);
    if (res < 0) {
      g_free (transports);
      goto create_request_failed;
    }

    if (src->version >= GST_RTSP_VERSION_2_0) {
      if (!pipelined_request_id)
        pipelined_request_id = g_strdup_printf ("%d",
            g_random_int_range (0, G_MAXINT32));

      gst_rtsp_message_add_header (&request, GST_RTSP_HDR_PIPELINED_REQUESTS,
          pipelined_request_id);
      gst_rtsp_message_add_header (&request, GST_RTSP_HDR_ACCEPT_RANGES,
          "npt, clock, smpte, clock");
    }

    /* select transport */
    gst_rtsp_message_take_header (&request, GST_RTSP_HDR_TRANSPORT, transports);

    if (stream->is_backchannel && src->backchannel == BACKCHANNEL_ONVIF)
      gst_rtsp_message_add_header (&request, GST_RTSP_HDR_REQUIRE,
          BACKCHANNEL_ONVIF_HDR_REQUIRE_VAL);

    /* set up keys */
    if (stream->profile == GST_RTSP_PROFILE_SAVP ||
        stream->profile == GST_RTSP_PROFILE_SAVPF) {
      hval = gst_rtspsrc_stream_make_keymgmt (src, stream);
      gst_rtsp_message_take_header (&request, GST_RTSP_HDR_KEYMGMT, hval);
    }

    /* if the user wants a non default RTP packet size we add the blocksize
     * parameter */
    if (src->rtp_blocksize > 0) {
      hval = g_strdup_printf ("%d", src->rtp_blocksize);
      gst_rtsp_message_take_header (&request, GST_RTSP_HDR_BLOCKSIZE, hval);
    }

    if (async)
      GST_ELEMENT_PROGRESS (src, CONTINUE, "request", ("SETUP stream %d",
              stream->id));

    /* handle the code ourselves */
    res =
        gst_rtspsrc_send (src, conninfo, &request,
        pipelined_request_id ? NULL : &response, &code, NULL);
    if (res < 0)
      goto send_error;

    switch (code) {
      case GST_RTSP_STS_OK:
        break;
      case GST_RTSP_STS_UNSUPPORTED_TRANSPORT:
        gst_rtsp_message_unset (&request);
        gst_rtsp_message_unset (&response);
        /* cleanup of leftover transport */
        gst_rtspsrc_stream_free_udp (stream);
        /* MS WMServer RTSP MUST use same UDP pair in all SETUP requests;
         * we might be in this case */
        if (stream->container && rtpport && rtcpport && !retry) {
          GST_DEBUG_OBJECT (src, "retrying with original port pair %u-%u",
              rtpport, rtcpport);
          retry++;
          goto retry;
        }
        /* this transport did not go down well, but we may have others to try
         * that we did not send yet, try those and only give up then
         * but not without checking for lost cause/extension so we can
         * post a nicer/more useful error message later */
        if (!unsupported_real)
          unsupported_real = stream->is_real;
        /* select next available protocol, give up on this stream if none */
        mask++;
        while (protocol_masks[mask] && !(protocols & protocol_masks[mask]))
          mask++;
        if (!protocol_masks[mask] || unsupported_real)
          continue;
        else
          goto retry;
      default:
        /* cleanup of leftover transport and move to the next stream */
        gst_rtspsrc_stream_free_udp (stream);
        goto response_error;
    }


    if (!pipelined_request_id) {
      /* parse response transport */
      res = gst_rtsp_src_setup_stream_from_response (src, stream,
          &response, &protocols, retry, &rtpport, &rtcpport);
      switch (res) {
        case GST_RTSP_ERROR:
          goto cleanup_error;
        case GST_RTSP_ELAST:
          goto retry;
        default:
          break;
      }
    } else {
      stream->waiting_setup_response = TRUE;
      /* we need to activate at least one stream when we detect activity */
      src->need_activate = TRUE;
    }

    {
      GList *skip = walk;

      while (TRUE) {
        GstRTSPStream *sskip;

        skip = g_list_next (skip);
        if (skip == NULL)
          break;

        sskip = (GstRTSPStream *) skip->data;

        /* skip all streams with the same control url */
        if (g_str_equal (stream->conninfo.location, sskip->conninfo.location)) {
          GST_DEBUG_OBJECT (src, "found stream %p with same control %s",
              sskip, sskip->conninfo.location);
          sskip->skipped = TRUE;
        }
      }
    }
    gst_rtsp_message_unset (&request);
  }

  if (pipelined_request_id) {
    gst_rtspsrc_setup_streams_end (src, TRUE);
  }

  /* store the transport protocol that was configured */
  src->cur_protocols = protocols;

  gst_rtsp_ext_list_stream_select (src->extensions, url);

  if (pipelined_request_id)
    g_free (pipelined_request_id);

  /* if there is nothing to activate, error out */
  if (!src->need_activate)
    goto nothing_to_activate;

  return res;

  /* ERRORS */
no_protocols:
  {
    /* no transport possible, post an error and stop */
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("Could not connect to server, no protocols left"));
    return GST_RTSP_ERROR;
  }
no_streams:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("SDP contains no streams"));
    return GST_RTSP_ERROR;
  }
create_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request. (%s)", str));
    g_free (str);
    goto cleanup_error;
  }
setup_transport_failed:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("Could not setup transport."));
    res = GST_RTSP_ERROR;
    goto cleanup_error;
  }
response_error:
  {
    const gchar *str = gst_rtsp_status_as_text (code);

    GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
        ("Error (%d): %s", code, GST_STR_NULL (str)));
    res = GST_RTSP_ERROR;
    goto cleanup_error;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
          ("Could not send message. (%s)", str));
    } else {
      GST_WARNING_OBJECT (src, "send interrupted");
    }
    g_free (str);
    goto cleanup_error;
  }
nothing_to_activate:
  {
    /* none of the available error codes is really right .. */
    if (unsupported_real) {
      GST_ELEMENT_ERROR (src, STREAM, CODEC_NOT_FOUND,
          (_("No supported stream was found. You might need to install a "
                  "GStreamer RTSP extension plugin for Real media streams.")),
          (NULL));
    } else {
      GST_ELEMENT_ERROR (src, STREAM, CODEC_NOT_FOUND,
          (_("No supported stream was found. You might need to allow "
                  "more transport protocols or may otherwise be missing "
                  "the right GStreamer RTSP extension plugin.")), (NULL));
    }
    return GST_RTSP_ERROR;
  }
cleanup_error:
  {
    if (pipelined_request_id)
      g_free (pipelined_request_id);
    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);
    return res;
  }
}

static gboolean
gst_rtspsrc_parse_range (GstRTSPSrc * src, const gchar * range,
    GstSegment * segment, gboolean update_duration)
{
  GstClockTime begin_seconds, end_seconds;
  gint64 seconds;
  GstRTSPTimeRange *therange;

  if (src->range)
    gst_rtsp_range_free (src->range);

  if (gst_rtsp_range_parse (range, &therange) == GST_RTSP_OK) {
    GST_DEBUG_OBJECT (src, "parsed range %s", range);
    src->range = therange;
  } else {
    GST_DEBUG_OBJECT (src, "failed to parse range %s", range);
    src->range = NULL;
    gst_segment_init (segment, GST_FORMAT_TIME);
    return FALSE;
  }

  gst_rtsp_range_get_times (therange, &begin_seconds, &end_seconds);

  GST_DEBUG_OBJECT (src, "range: type %d, min %f - type %d,  max %f ",
      therange->min.type, therange->min.seconds, therange->max.type,
      therange->max.seconds);

  if (therange->min.type == GST_RTSP_TIME_NOW)
    seconds = 0;
  else if (therange->min.type == GST_RTSP_TIME_END)
    seconds = 0;
  else
    seconds = begin_seconds;

  GST_DEBUG_OBJECT (src, "range: min %" GST_TIME_FORMAT,
      GST_TIME_ARGS (seconds));

  /* we need to start playback without clipping from the position reported by
   * the server */
  if (segment->rate > 0.0)
    segment->start = seconds;
  else
    segment->stop = seconds;

  segment->position = seconds;

  if (therange->max.type == GST_RTSP_TIME_NOW)
    seconds = -1;
  else if (therange->max.type == GST_RTSP_TIME_END)
    seconds = -1;
  else
    seconds = end_seconds;

  GST_DEBUG_OBJECT (src, "range: max %" GST_TIME_FORMAT,
      GST_TIME_ARGS (seconds));

  /* live (WMS) server might send overflowed large max as its idea of infinity,
   * compensate to prevent problems later on */
  if (seconds != -1 && seconds < 0) {
    seconds = -1;
    GST_DEBUG_OBJECT (src, "insane range, set to NONE");
  }

  /* live (WMS) might send min == max, which is not worth recording */
  if (segment->duration == -1 && seconds == begin_seconds)
    seconds = -1;

  /* don't change duration with unknown value, we might have a valid value
   * there that we want to keep. Also, the total duration of the stream
   * can only be determined from the response to a DESCRIBE request, not
   * from a PLAY request where we might have requested a custom range, so
   * don't update duration in that case */
  if (update_duration && seconds != -1) {
    segment->duration = seconds;
  }

  if (segment->rate > 0.0)
    segment->stop = seconds;
  else
    segment->start = seconds;

  return TRUE;
}

/* Parse clock profived by the server with following syntax:
 *
 * "GstNetTimeProvider <wrapped-clock> <server-IP:port> <clock-time>"
 */
static gboolean
gst_rtspsrc_parse_gst_clock (GstRTSPSrc * src, const gchar * gstclock)
{
  gboolean res = FALSE;

  if (g_str_has_prefix (gstclock, "GstNetTimeProvider ")) {
    gchar **fields = NULL, **parts = NULL;
    gchar *remote_ip, *str;
    gint port;
    GstClockTime base_time;
    GstClock *netclock;

    fields = g_strsplit (gstclock, " ", 0);

    /* wrapped clock, not very interesting for now */
    if (fields[1] == NULL)
      goto cleanup;

    /* remote IP address and port */
    if ((str = fields[2]) == NULL)
      goto cleanup;

    parts = g_strsplit (str, ":", 0);

    if ((remote_ip = parts[0]) == NULL)
      goto cleanup;

    if ((str = parts[1]) == NULL)
      goto cleanup;

    port = atoi (str);
    if (port == 0)
      goto cleanup;

    /* base-time */
    if ((str = fields[3]) == NULL)
      goto cleanup;

    base_time = g_ascii_strtoull (str, NULL, 10);

    netclock =
        gst_net_client_clock_new ((gchar *) "GstRTSPClock", remote_ip, port,
        base_time);

    if (src->provided_clock)
      gst_object_unref (src->provided_clock);
    src->provided_clock = netclock;

    gst_element_post_message (GST_ELEMENT_CAST (src),
        gst_message_new_clock_provide (GST_OBJECT_CAST (src),
            src->provided_clock, TRUE));

    res = TRUE;
  cleanup:
    g_strfreev (fields);
    g_strfreev (parts);
  }
  return res;
}

/* must be called with the RTSP state lock */
static GstRTSPResult
gst_rtspsrc_open_from_sdp (GstRTSPSrc * src, GstSDPMessage * sdp,
    gboolean async)
{
  GstRTSPResult res;
  gint i, n_streams;

  /* prepare global stream caps properties */
  if (src->props)
    gst_structure_remove_all_fields (src->props);
  else
    src->props = gst_structure_new_empty ("RTSPProperties");

  DEBUG_SDP (src, sdp);

  gst_rtsp_ext_list_parse_sdp (src->extensions, sdp, src->props);

  /* let the app inspect and change the SDP */
  g_signal_emit (src, gst_rtspsrc_signals[SIGNAL_ON_SDP], 0, sdp);

  gst_segment_init (&src->segment, GST_FORMAT_TIME);

  /* parse range for duration reporting. */
  {
    const gchar *range;

    for (i = 0;; i++) {
      range = gst_sdp_message_get_attribute_val_n (sdp, "range", i);
      if (range == NULL)
        break;

      /* keep track of the range and configure it in the segment */
      if (gst_rtspsrc_parse_range (src, range, &src->segment, TRUE))
        break;
    }
  }
  /* parse clock information. This is GStreamer specific, a server can tell the
   * client what clock it is using and wrap that in a network clock. The
   * advantage of that is that we can slave to it. */
  {
    const gchar *gstclock;

    for (i = 0;; i++) {
      gstclock = gst_sdp_message_get_attribute_val_n (sdp, "x-gst-clock", i);
      if (gstclock == NULL)
        break;

      /* parse the clock and expose it in the provide_clock method */
      if (gst_rtspsrc_parse_gst_clock (src, gstclock))
        break;
    }
  }
  /* try to find a global control attribute. Note that a '*' means that we should
   * do aggregate control with the current url (so we don't do anything and
   * leave the current connection as is) */
  {
    const gchar *control;

    for (i = 0;; i++) {
      control = gst_sdp_message_get_attribute_val_n (sdp, "control", i);
      if (control == NULL)
        break;

      /* only take fully qualified urls */
      if (g_str_has_prefix (control, "rtsp://"))
        break;
    }
    if (control) {
      g_free (src->conninfo.location);
      src->conninfo.location = g_strdup (control);
      /* make a connection for this, if there was a connection already, nothing
       * happens. */
      if (gst_rtsp_conninfo_connect (src, &src->conninfo, async) < 0) {
        GST_ERROR_OBJECT (src, "could not connect");
      }
    }
    /* we need to keep the control url separate from the connection url because
     * the rules for constructing the media control url need it */
    g_free (src->control);
    src->control = g_strdup (control);
  }

  /* create streams */
  n_streams = gst_sdp_message_medias_len (sdp);
  for (i = 0; i < n_streams; i++) {
    gst_rtspsrc_create_stream (src, sdp, i, n_streams);
  }

  src->state = GST_RTSP_STATE_INIT;

  /* setup streams */
  if ((res = gst_rtspsrc_setup_streams_start (src, async)) < 0)
    goto setup_failed;

  /* reset our state */
  src->need_range = TRUE;
  src->server_side_trickmode = FALSE;
  src->trickmode_interval = 0;

  src->state = GST_RTSP_STATE_READY;

  return res;

  /* ERRORS */
setup_failed:
  {
    GST_ERROR_OBJECT (src, "setup failed");
    gst_rtspsrc_cleanup (src);
    return res;
  }
}

static GstRTSPResult
gst_rtspsrc_retrieve_sdp (GstRTSPSrc * src, GstSDPMessage ** sdp,
    gboolean async)
{
  GstRTSPResult res;
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  guint8 *data;
  guint size;
  gchar *respcont = NULL;
  GstRTSPVersion versions[] =
      { GST_RTSP_VERSION_2_0, GST_RTSP_VERSION_INVALID };

  src->version = src->default_version;
  if (src->default_version == GST_RTSP_VERSION_2_0) {
    versions[0] = GST_RTSP_VERSION_1_0;
  }

restart:
  src->need_redirect = FALSE;

  /* can't continue without a valid url */
  if (G_UNLIKELY (src->conninfo.url == NULL)) {
    res = GST_RTSP_EINVAL;
    goto no_url;
  }
  src->tried_url_auth = FALSE;

  if ((res = gst_rtsp_conninfo_connect (src, &src->conninfo, async)) < 0)
    goto connect_failed;

  /* create OPTIONS */
  GST_DEBUG_OBJECT (src, "create options... (%s)", async ? "async" : "sync");
  res =
      gst_rtspsrc_init_request (src, &request, GST_RTSP_OPTIONS,
      src->conninfo.url_str);
  if (res < 0)
    goto create_request_failed;

  /* send OPTIONS */
  request.type_data.request.version = src->version;
  GST_DEBUG_OBJECT (src, "send options...");

  if (async)
    GST_ELEMENT_PROGRESS (src, CONTINUE, "open", ("Retrieving server options"));

  if ((res =
          gst_rtspsrc_send (src, &src->conninfo, &request, &response,
              NULL, versions)) < 0) {
    goto send_error;
  }

  src->version = request.type_data.request.version;
  GST_INFO_OBJECT (src, "Now using version: %s",
      gst_rtsp_version_as_text (src->version));

  /* parse OPTIONS */
  if (!gst_rtspsrc_parse_methods (src, &response))
    goto methods_error;

  /* create DESCRIBE */
  GST_DEBUG_OBJECT (src, "create describe...");
  res =
      gst_rtspsrc_init_request (src, &request, GST_RTSP_DESCRIBE,
      src->conninfo.url_str);
  if (res < 0)
    goto create_request_failed;

  /* we only accept SDP for now */
  gst_rtsp_message_add_header (&request, GST_RTSP_HDR_ACCEPT,
      "application/sdp");

  if (src->backchannel == BACKCHANNEL_ONVIF)
    gst_rtsp_message_add_header (&request, GST_RTSP_HDR_REQUIRE,
        BACKCHANNEL_ONVIF_HDR_REQUIRE_VAL);
  /* TODO: Handle the case when backchannel is unsupported and goto restart */

  /* send DESCRIBE */
  GST_DEBUG_OBJECT (src, "send describe...");

  if (async)
    GST_ELEMENT_PROGRESS (src, CONTINUE, "open", ("Retrieving media info"));

  if ((res =
          gst_rtspsrc_send (src, &src->conninfo, &request, &response,
              NULL, NULL)) < 0)
    goto send_error;

  /* we only perform redirect for describe and play, currently */
  if (src->need_redirect) {
    /* close connection, we don't have to send a TEARDOWN yet, ignore the
     * result. */
    gst_rtsp_conninfo_close (src, &src->conninfo, TRUE);

    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);

    /* and now retry */
    goto restart;
  }

  /* it could be that the DESCRIBE method was not implemented */
  if (!(src->methods & GST_RTSP_DESCRIBE))
    goto no_describe;

  /* check if reply is SDP */
  gst_rtsp_message_get_header (&response, GST_RTSP_HDR_CONTENT_TYPE, &respcont,
      0);
  /* could not be set but since the request returned OK, we assume it
   * was SDP, else check it. */
  if (respcont) {
    const gchar *props = strchr (respcont, ';');

    if (props) {
      gchar *mimetype = g_strndup (respcont, props - respcont);

      mimetype = g_strstrip (mimetype);
      if (g_ascii_strcasecmp (mimetype, "application/sdp") != 0) {
        g_free (mimetype);
        goto wrong_content_type;
      }

      /* TODO: Check for charset property and do conversions of all messages if
       * needed. Some servers actually send that property */

      g_free (mimetype);
    } else if (g_ascii_strcasecmp (respcont, "application/sdp") != 0) {
      goto wrong_content_type;
    }
  }

  /* get message body and parse as SDP */
  gst_rtsp_message_get_body (&response, &data, &size);
  if (data == NULL || size == 0)
    goto no_describe;

  GST_DEBUG_OBJECT (src, "parse SDP...");
  gst_sdp_message_new (sdp);
  gst_sdp_message_parse_buffer (data, size, *sdp);

  /* clean up any messages */
  gst_rtsp_message_unset (&request);
  gst_rtsp_message_unset (&response);

  return res;

  /* ERRORS */
no_url:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (NULL),
        ("No valid RTSP URL was provided"));
    goto cleanup_error;
  }
connect_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ_WRITE, (NULL),
          ("Failed to connect. (%s)", str));
    } else {
      GST_WARNING_OBJECT (src, "connect interrupted");
    }
    g_free (str);
    goto cleanup_error;
  }
create_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request. (%s)", str));
    g_free (str);
    goto cleanup_error;
  }
send_error:
  {
    /* Don't post a message - the rtsp_send method will have
     * taken care of it because we passed NULL for the response code */
    goto cleanup_error;
  }
methods_error:
  {
    /* error was posted */
    res = GST_RTSP_ERROR;
    goto cleanup_error;
  }
wrong_content_type:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("Server does not support SDP, got %s.", respcont));
    res = GST_RTSP_ERROR;
    goto cleanup_error;
  }
no_describe:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("Server can not provide an SDP."));
    res = GST_RTSP_ERROR;
    goto cleanup_error;
  }
cleanup_error:
  {
    if (src->conninfo.connection) {
      GST_DEBUG_OBJECT (src, "free connection");
      gst_rtsp_conninfo_close (src, &src->conninfo, TRUE);
    }
    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);
    return res;
  }
}

static GstRTSPResult
gst_rtspsrc_open (GstRTSPSrc * src, gboolean async)
{
  GstRTSPResult ret;

  src->methods =
      GST_RTSP_SETUP | GST_RTSP_PLAY | GST_RTSP_PAUSE | GST_RTSP_TEARDOWN;

  if (src->sdp == NULL) {
    if ((ret = gst_rtspsrc_retrieve_sdp (src, &src->sdp, async)) < 0)
      goto no_sdp;
  }

  if ((ret = gst_rtspsrc_open_from_sdp (src, src->sdp, async)) < 0)
    goto open_failed;

  if (src->initial_seek) {
    if (!gst_rtspsrc_perform_seek (src, src->initial_seek))
      goto initial_seek_failed;
    gst_event_replace (&src->initial_seek, NULL);
  }

done:
  if (async)
    gst_rtspsrc_loop_end_cmd (src, CMD_OPEN, ret);

  return ret;

  /* ERRORS */
no_sdp:
  {
    GST_WARNING_OBJECT (src, "can't get sdp");
    src->open_error = TRUE;
    goto done;
  }
open_failed:
  {
    GST_WARNING_OBJECT (src, "can't setup streaming from sdp");
    src->open_error = TRUE;
    goto done;
  }
initial_seek_failed:
  {
    GST_WARNING_OBJECT (src, "Failed to perform initial seek");
    ret = GST_RTSP_ERROR;
    src->open_error = TRUE;
    goto done;
  }
}

static GstRTSPResult
gst_rtspsrc_close (GstRTSPSrc * src, gboolean async, gboolean only_close)
{
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPResult res = GST_RTSP_OK;
  GList *walk;
  const gchar *control;

  GST_DEBUG_OBJECT (src, "TEARDOWN...");

  gst_rtspsrc_set_state (src, GST_STATE_READY);

  if (src->state < GST_RTSP_STATE_READY) {
    GST_DEBUG_OBJECT (src, "not ready, doing cleanup");
    goto close;
  }

  if (only_close)
    goto close;

  /* construct a control url */
  control = get_aggregate_control (src);

  if (!(src->methods & (GST_RTSP_PLAY | GST_RTSP_TEARDOWN)))
    goto not_supported;

  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    const gchar *setup_url;
    GstRTSPConnInfo *info;

    /* try aggregate control first but do non-aggregate control otherwise */
    if (control)
      setup_url = control;
    else if ((setup_url = stream->conninfo.location) == NULL)
      continue;

    if (src->conninfo.connection) {
      info = &src->conninfo;
    } else if (stream->conninfo.connection) {
      info = &stream->conninfo;
    } else {
      continue;
    }
    if (!info->connected)
      goto next;

    /* do TEARDOWN */
    res =
        gst_rtspsrc_init_request (src, &request, GST_RTSP_TEARDOWN, setup_url);
    GST_LOG_OBJECT (src, "Teardown on %s", setup_url);
    if (res < 0)
      goto create_request_failed;

    if (stream->is_backchannel && src->backchannel == BACKCHANNEL_ONVIF)
      gst_rtsp_message_add_header (&request, GST_RTSP_HDR_REQUIRE,
          BACKCHANNEL_ONVIF_HDR_REQUIRE_VAL);

    if (async)
      GST_ELEMENT_PROGRESS (src, CONTINUE, "close", ("Closing stream"));

    if ((res =
            gst_rtspsrc_send (src, info, &request, &response, NULL, NULL)) < 0)
      goto send_error;

    /* FIXME, parse result? */
    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);

  next:
    /* early exit when we did aggregate control */
    if (control)
      break;
  }

close:
  /* close connections */
  GST_DEBUG_OBJECT (src, "closing connection...");
  gst_rtsp_conninfo_close (src, &src->conninfo, TRUE);
  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    gst_rtsp_conninfo_close (src, &stream->conninfo, TRUE);
  }

  /* cleanup */
  gst_rtspsrc_cleanup (src);

  src->state = GST_RTSP_STATE_INVALID;

  if (async)
    gst_rtspsrc_loop_end_cmd (src, CMD_CLOSE, res);

  return res;

  /* ERRORS */
create_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request. (%s)", str));
    g_free (str);
    goto close;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    gst_rtsp_message_unset (&request);
    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
          ("Could not send message. (%s)", str));
    } else {
      GST_WARNING_OBJECT (src, "TEARDOWN interrupted");
    }
    g_free (str);
    goto close;
  }
not_supported:
  {
    GST_DEBUG_OBJECT (src,
        "TEARDOWN and PLAY not supported, can't do TEARDOWN");
    goto close;
  }
}

/* RTP-Info is of the format:
 *
 * url=<URL>;[seq=<seqbase>;rtptime=<timebase>] [, url=...]
 *
 * rtptime corresponds to the timestamp for the NPT time given in the header
 * seqbase corresponds to the next sequence number we received. This number
 * indicates the first seqnum after the seek and should be used to discard
 * packets that are from before the seek.
 */
static gboolean
gst_rtspsrc_parse_rtpinfo (GstRTSPSrc * src, gchar * rtpinfo)
{
  gchar **infos;
  gint i, j;

  GST_DEBUG_OBJECT (src, "parsing RTP-Info %s", rtpinfo);

  infos = g_strsplit (rtpinfo, ",", 0);
  for (i = 0; infos[i]; i++) {
    gchar **fields;
    GstRTSPStream *stream;
    gint32 seqbase;
    gint64 timebase;

    GST_DEBUG_OBJECT (src, "parsing info %s", infos[i]);

    /* init values, types of seqbase and timebase are bigger than needed so we
     * can store -1 as uninitialized values */
    stream = NULL;
    seqbase = -1;
    timebase = -1;

    /* parse url, find stream for url.
     * parse seq and rtptime. The seq number should be configured in the rtp
     * depayloader or session manager to detect gaps. Same for the rtptime, it
     * should be used to create an initial time newsegment. */
    fields = g_strsplit (infos[i], ";", 0);
    for (j = 0; fields[j]; j++) {
      GST_DEBUG_OBJECT (src, "parsing field %s", fields[j]);
      /* remove leading whitespace */
      fields[j] = g_strchug (fields[j]);
      if (g_str_has_prefix (fields[j], "url=")) {
        /* get the url and the stream */
        stream =
            find_stream (src, (fields[j] + 4), (gpointer) find_stream_by_setup);
      } else if (g_str_has_prefix (fields[j], "seq=")) {
        seqbase = atoi (fields[j] + 4);
      } else if (g_str_has_prefix (fields[j], "rtptime=")) {
        timebase = g_ascii_strtoll (fields[j] + 8, NULL, 10);
      }
    }
    g_strfreev (fields);
    /* now we need to store the values for the caps of the stream */
    if (stream != NULL) {
      GST_DEBUG_OBJECT (src,
          "found stream %p, setting: seqbase %d, timebase %" G_GINT64_FORMAT,
          stream, seqbase, timebase);

      /* we have a stream, configure detected params */
      stream->seqbase = seqbase;
      stream->timebase = timebase;
    }
  }
  g_strfreev (infos);

  return TRUE;
}

static void
gst_rtspsrc_handle_rtcp_interval (GstRTSPSrc * src, gchar * rtcp)
{
  guint64 interval;
  GList *walk;

  interval = strtoul (rtcp, NULL, 10);
  GST_DEBUG_OBJECT (src, "rtcp interval: %" G_GUINT64_FORMAT " ms", interval);

  if (!interval)
    return;

  interval *= GST_MSECOND;

  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;

    /* already (optionally) retrieved this when configuring manager */
    if (stream->session) {
      GObject *rtpsession = stream->session;

      GST_DEBUG_OBJECT (src, "configure rtcp interval in session %p",
          rtpsession);
      g_object_set (rtpsession, "rtcp-min-interval", interval, NULL);
    }
  }

  /* now it happens that (Xenon) server sending this may also provide bogus
   * RTCP SR sync data (i.e. with quite some jitter), so never mind those
   * and just use RTP-Info to sync */
  if (src->manager) {
    GObjectClass *klass;

    klass = G_OBJECT_GET_CLASS (G_OBJECT (src->manager));
    if (g_object_class_find_property (klass, "rtcp-sync")) {
      GST_DEBUG_OBJECT (src, "configuring rtp sync method");
      g_object_set (src->manager, "rtcp-sync", RTCP_SYNC_RTP, NULL);
    }
  }
}

static gdouble
gst_rtspsrc_get_float (const gchar * dstr)
{
  gchar s[G_ASCII_DTOSTR_BUF_SIZE] = { 0, };

  /* canonicalise floating point string so we can handle float strings
   * in the form "24.930" or "24,930" irrespective of the current locale */
  g_strlcpy (s, dstr, sizeof (s));
  g_strdelimit (s, ",", '.');
  return g_ascii_strtod (s, NULL);
}

static gchar *
gen_range_header (GstRTSPSrc * src, GstSegment * segment)
{
  GstRTSPTimeRange range = { 0, };
  gdouble begin_seconds, end_seconds;

  if (segment->rate > 0) {
    begin_seconds = (gdouble) segment->start / GST_SECOND;
    end_seconds = (gdouble) segment->stop / GST_SECOND;
  } else {
    begin_seconds = (gdouble) segment->stop / GST_SECOND;
    end_seconds = (gdouble) segment->start / GST_SECOND;
  }

  if (src->onvif_mode) {
    GDateTime *prime_epoch, *datetime;

    range.unit = GST_RTSP_RANGE_CLOCK;

    prime_epoch = g_date_time_new_utc (1900, 1, 1, 0, 0, 0);

    datetime = g_date_time_add_seconds (prime_epoch, begin_seconds);

    range.min.type = GST_RTSP_TIME_UTC;
    range.min2.year = g_date_time_get_year (datetime);
    range.min2.month = g_date_time_get_month (datetime);
    range.min2.day = g_date_time_get_day_of_month (datetime);
    range.min.seconds =
        g_date_time_get_seconds (datetime) +
        g_date_time_get_minute (datetime) * 60 +
        g_date_time_get_hour (datetime) * 60 * 60;

    g_date_time_unref (datetime);

    datetime = g_date_time_add_seconds (prime_epoch, end_seconds);

    range.max.type = GST_RTSP_TIME_UTC;
    range.max2.year = g_date_time_get_year (datetime);
    range.max2.month = g_date_time_get_month (datetime);
    range.max2.day = g_date_time_get_day_of_month (datetime);
    range.max.seconds =
        g_date_time_get_seconds (datetime) +
        g_date_time_get_minute (datetime) * 60 +
        g_date_time_get_hour (datetime) * 60 * 60;

    g_date_time_unref (datetime);
    g_date_time_unref (prime_epoch);
  } else {
    range.unit = GST_RTSP_RANGE_NPT;
    range.min.type = GST_RTSP_TIME_SECONDS;
    range.min.seconds = begin_seconds;
    range.max.type = GST_RTSP_TIME_SECONDS;
    range.max.seconds = end_seconds;
  }

  /* Don't set end bounds when not required to */
  if (!GST_CLOCK_TIME_IS_VALID (segment->stop)) {
    if (segment->rate > 0)
      range.max.type = GST_RTSP_TIME_END;
    else
      range.min.type = GST_RTSP_TIME_END;
  }

  return gst_rtsp_range_to_string (&range);
}

static void
clear_rtp_base (GstRTSPSrc * src, GstRTSPStream * stream)
{
  guint i, len;

  stream->timebase = -1;
  stream->seqbase = -1;

  len = stream->ptmap->len;
  for (i = 0; i < len; i++) {
    PtMapItem *item = &g_array_index (stream->ptmap, PtMapItem, i);
    GstStructure *s;

    if (item->caps == NULL)
      continue;

    item->caps = gst_caps_make_writable (item->caps);
    s = gst_caps_get_structure (item->caps, 0);
    gst_structure_remove_fields (s, "clock-base", "seqnum-base", NULL);
    if (item->pt == stream->default_pt && stream->udpsrc[0])
      g_object_set (stream->udpsrc[0], "caps", item->caps, NULL);
  }
  stream->need_caps = TRUE;
}

static GstRTSPResult
gst_rtspsrc_ensure_open (GstRTSPSrc * src, gboolean async)
{
  GstRTSPResult res = GST_RTSP_OK;

  if (src->state < GST_RTSP_STATE_READY) {
    res = GST_RTSP_ERROR;
    if (src->open_error) {
      GST_DEBUG_OBJECT (src, "the stream was in error");
      goto done;
    }
    if (async)
      gst_rtspsrc_loop_start_cmd (src, CMD_OPEN);

    if ((res = gst_rtspsrc_open (src, async)) < 0) {
      GST_DEBUG_OBJECT (src, "failed to open stream");
      goto done;
    }
  }

done:
  return res;
}

static GstRTSPResult
gst_rtspsrc_play (GstRTSPSrc * src, GstSegment * segment, gboolean async,
    const gchar * seek_style)
{
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPResult res = GST_RTSP_OK;
  GList *walk;
  gchar *hval;
  gint hval_idx;
  const gchar *control;
  GstSegment requested;

  GST_DEBUG_OBJECT (src, "PLAY...");

restart:
  if ((res = gst_rtspsrc_ensure_open (src, async)) < 0)
    goto open_failed;

  if (!(src->methods & GST_RTSP_PLAY))
    goto not_supported;

  if (src->state == GST_RTSP_STATE_PLAYING)
    goto was_playing;

  if (!src->conninfo.connection || !src->conninfo.connected)
    goto done;

  requested = *segment;

  /* send some dummy packets before we activate the receive in the
   * udp sources */
  gst_rtspsrc_send_dummy_packets (src);

  /* require new SR packets */
  if (src->manager)
    g_signal_emit_by_name (src->manager, "reset-sync", NULL);

  /* construct a control url */
  control = get_aggregate_control (src);

  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    const gchar *setup_url;
    GstRTSPConnInfo *conninfo;

    /* try aggregate control first but do non-aggregate control otherwise */
    if (control)
      setup_url = control;
    else if ((setup_url = stream->conninfo.location) == NULL)
      continue;

    if (src->conninfo.connection) {
      conninfo = &src->conninfo;
    } else if (stream->conninfo.connection) {
      conninfo = &stream->conninfo;
    } else {
      continue;
    }

    /* do play */
    res = gst_rtspsrc_init_request (src, &request, GST_RTSP_PLAY, setup_url);
    if (res < 0)
      goto create_request_failed;

    if (src->need_range && src->seekable >= 0.0) {
      hval = gen_range_header (src, segment);

      gst_rtsp_message_take_header (&request, GST_RTSP_HDR_RANGE, hval);

      /* store the newsegment event so it can be sent from the streaming thread. */
      src->need_segment = TRUE;
    }

    if (segment->rate != 1.0) {
      gchar scale_val[G_ASCII_DTOSTR_BUF_SIZE];
      gchar speed_val[G_ASCII_DTOSTR_BUF_SIZE];

      if (src->server_side_trickmode) {
        g_ascii_dtostr (scale_val, sizeof (scale_val), segment->rate);
        gst_rtsp_message_add_header (&request, GST_RTSP_HDR_SCALE, scale_val);
      } else if (segment->rate < 0.0) {
        g_ascii_dtostr (scale_val, sizeof (scale_val), -1.0);
        gst_rtsp_message_add_header (&request, GST_RTSP_HDR_SCALE, scale_val);

        if (ABS (segment->rate) != 1.0) {
          g_ascii_dtostr (speed_val, sizeof (speed_val), ABS (segment->rate));
          gst_rtsp_message_add_header (&request, GST_RTSP_HDR_SPEED, speed_val);
        }
      } else {
        g_ascii_dtostr (speed_val, sizeof (speed_val), segment->rate);
        gst_rtsp_message_add_header (&request, GST_RTSP_HDR_SPEED, speed_val);
      }
    }

    if (src->onvif_mode) {
      if (segment->flags & GST_SEEK_FLAG_TRICKMODE_KEY_UNITS) {
        gchar *hval;

        if (src->trickmode_interval)
          hval =
              g_strdup_printf ("intra/%" G_GUINT64_FORMAT,
              src->trickmode_interval / GST_MSECOND);
        else
          hval = g_strdup ("intra");

        gst_rtsp_message_add_header (&request, GST_RTSP_HDR_FRAMES, hval);

        g_free (hval);
      } else if (segment->flags & GST_SEEK_FLAG_TRICKMODE_FORWARD_PREDICTED) {
        gst_rtsp_message_add_header (&request, GST_RTSP_HDR_FRAMES,
            "predicted");
      }
    }

    if (seek_style)
      gst_rtsp_message_add_header (&request, GST_RTSP_HDR_SEEK_STYLE,
          seek_style);

    /* when we have an ONVIF audio backchannel, the PLAY request must have the
     * Require: header when doing either aggregate or non-aggregate control */
    if (src->backchannel == BACKCHANNEL_ONVIF &&
        (control || stream->is_backchannel))
      gst_rtsp_message_add_header (&request, GST_RTSP_HDR_REQUIRE,
          BACKCHANNEL_ONVIF_HDR_REQUIRE_VAL);

    if (src->onvif_mode) {
      if (src->onvif_rate_control)
        gst_rtsp_message_add_header (&request, GST_RTSP_HDR_RATE_CONTROL,
            "yes");
      else
        gst_rtsp_message_add_header (&request, GST_RTSP_HDR_RATE_CONTROL, "no");
    }

    if (async)
      GST_ELEMENT_PROGRESS (src, CONTINUE, "request", ("Sending PLAY request"));

    if ((res =
            gst_rtspsrc_send (src, conninfo, &request, &response, NULL, NULL))
        < 0)
      goto send_error;

    if (src->need_redirect) {
      GST_DEBUG_OBJECT (src,
          "redirect: tearing down and restarting with new url");
      /* teardown and restart with new url */
      gst_rtspsrc_close (src, TRUE, FALSE);
      /* reset protocols to force re-negotiation with redirected url */
      src->cur_protocols = src->protocols;
      gst_rtsp_message_unset (&request);
      gst_rtsp_message_unset (&response);
      goto restart;
    }

    /* seek may have silently failed as it is not supported */
    if (!(src->methods & GST_RTSP_PLAY)) {
      GST_DEBUG_OBJECT (src, "PLAY Range not supported; re-enable PLAY");

      if (src->version >= GST_RTSP_VERSION_2_0 && src->seekable >= 0.0) {
        GST_WARNING_OBJECT (src, "Server declared stream as seekable but"
            " playing with range failed... Ignoring information.");
      }
      /* obviously it is supported as we made it here */
      src->methods |= GST_RTSP_PLAY;
      src->seekable = -1.0;
      /* but there is nothing to parse in the response,
       * so convey we have no idea and not to expect anything particular */
      clear_rtp_base (src, stream);
      if (control) {
        GList *run;

        /* need to do for all streams */
        for (run = src->streams; run; run = g_list_next (run))
          clear_rtp_base (src, (GstRTSPStream *) run->data);
      }
      /* NOTE the above also disables npt based eos detection */
      /* and below forces position to 0,
       * which is visible feedback we lost the plot */
      segment->start = segment->position = src->last_pos;
    }

    gst_rtsp_message_unset (&request);

    /* parse RTP npt field. This is the current position in the stream (Normal
     * Play Time) and should be put in the NEWSEGMENT position field. */
    if (gst_rtsp_message_get_header (&response, GST_RTSP_HDR_RANGE, &hval,
            0) == GST_RTSP_OK)
      gst_rtspsrc_parse_range (src, hval, segment, FALSE);

    /* assume 1.0 rate now, overwrite when the SCALE or SPEED headers are present. */
    segment->rate = 1.0;

    /* parse Speed header. This is the intended playback rate of the stream
     * and should be put in the NEWSEGMENT rate field. */
    if (gst_rtsp_message_get_header (&response, GST_RTSP_HDR_SPEED, &hval,
            0) == GST_RTSP_OK) {
      segment->rate = gst_rtspsrc_get_float (hval);
    } else if (gst_rtsp_message_get_header (&response, GST_RTSP_HDR_SCALE,
            &hval, 0) == GST_RTSP_OK) {
      segment->rate = gst_rtspsrc_get_float (hval);
    }

    /* parse the RTP-Info header field (if ANY) to get the base seqnum and timestamp
     * for the RTP packets. If this is not present, we assume all starts from 0...
     * This is info for the RTP session manager that we pass to it in caps. */
    hval_idx = 0;
    while (gst_rtsp_message_get_header (&response, GST_RTSP_HDR_RTP_INFO,
            &hval, hval_idx++) == GST_RTSP_OK)
      gst_rtspsrc_parse_rtpinfo (src, hval);

    /* some servers indicate RTCP parameters in PLAY response,
     * rather than properly in SDP */
    if (gst_rtsp_message_get_header (&response, GST_RTSP_HDR_RTCP_INTERVAL,
            &hval, 0) == GST_RTSP_OK)
      gst_rtspsrc_handle_rtcp_interval (src, hval);

    gst_rtsp_message_unset (&response);

    /* early exit when we did aggregate control */
    if (control)
      break;
  }

  memcpy (&src->out_segment, segment, sizeof (GstSegment));

  if (src->clip_out_segment) {
    /* Only clip the output segment when the server has answered with valid
     * values, we cannot know otherwise whether the requested bounds were
     * available */
    if (GST_CLOCK_TIME_IS_VALID (src->segment.start) &&
        GST_CLOCK_TIME_IS_VALID (requested.start))
      src->out_segment.start = MAX (src->out_segment.start, requested.start);
    if (GST_CLOCK_TIME_IS_VALID (src->segment.stop) &&
        GST_CLOCK_TIME_IS_VALID (requested.stop))
      src->out_segment.stop = MIN (src->out_segment.stop, requested.stop);
  }

  /* configure the caps of the streams after we parsed all headers. Only reset
   * the manager object when we set a new Range header (we did a seek) */
  gst_rtspsrc_configure_caps (src, segment, src->need_range);

  /* set to PLAYING after we have configured the caps, otherwise we
   * might end up calling request_key (with SRTP) while caps are still
   * being configured. */
  gst_rtspsrc_set_state (src, GST_STATE_PLAYING);

  /* set again when needed */
  src->need_range = FALSE;

  src->running = TRUE;
  src->base_time = -1;
  src->state = GST_RTSP_STATE_PLAYING;

  /* mark discont */
  GST_DEBUG_OBJECT (src, "mark DISCONT, we did a seek to another position");
  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    stream->discont = TRUE;
  }

done:
  if (async)
    gst_rtspsrc_loop_end_cmd (src, CMD_PLAY, res);

  return res;

  /* ERRORS */
open_failed:
  {
    GST_WARNING_OBJECT (src, "failed to open stream");
    goto done;
  }
not_supported:
  {
    GST_WARNING_OBJECT (src, "PLAY is not supported");
    goto done;
  }
was_playing:
  {
    GST_WARNING_OBJECT (src, "we were already PLAYING");
    goto done;
  }
create_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request. (%s)", str));
    g_free (str);
    goto done;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    gst_rtsp_message_unset (&request);
    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
          ("Could not send message. (%s)", str));
    } else {
      GST_WARNING_OBJECT (src, "PLAY interrupted");
    }
    g_free (str);
    goto done;
  }
}

static GstRTSPResult
gst_rtspsrc_pause (GstRTSPSrc * src, gboolean async)
{
  GstRTSPResult res = GST_RTSP_OK;
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GList *walk;
  const gchar *control;

  GST_DEBUG_OBJECT (src, "PAUSE...");

  if ((res = gst_rtspsrc_ensure_open (src, async)) < 0)
    goto open_failed;

  if (!(src->methods & GST_RTSP_PAUSE))
    goto not_supported;

  if (src->state == GST_RTSP_STATE_READY)
    goto was_paused;

  if (!src->conninfo.connection || !src->conninfo.connected)
    goto no_connection;

  /* construct a control url */
  control = get_aggregate_control (src);

  /* loop over the streams. We might exit the loop early when we could do an
   * aggregate control */
  for (walk = src->streams; walk; walk = g_list_next (walk)) {
    GstRTSPStream *stream = (GstRTSPStream *) walk->data;
    GstRTSPConnInfo *conninfo;
    const gchar *setup_url;

    /* try aggregate control first but do non-aggregate control otherwise */
    if (control)
      setup_url = control;
    else if ((setup_url = stream->conninfo.location) == NULL)
      continue;

    if (src->conninfo.connection) {
      conninfo = &src->conninfo;
    } else if (stream->conninfo.connection) {
      conninfo = &stream->conninfo;
    } else {
      continue;
    }

    if (async)
      GST_ELEMENT_PROGRESS (src, CONTINUE, "request",
          ("Sending PAUSE request"));

    if ((res =
            gst_rtspsrc_init_request (src, &request, GST_RTSP_PAUSE,
                setup_url)) < 0)
      goto create_request_failed;

    /* when we have an ONVIF audio backchannel, the PAUSE request must have the
     * Require: header when doing either aggregate or non-aggregate control */
    if (src->backchannel == BACKCHANNEL_ONVIF &&
        (control || stream->is_backchannel))
      gst_rtsp_message_add_header (&request, GST_RTSP_HDR_REQUIRE,
          BACKCHANNEL_ONVIF_HDR_REQUIRE_VAL);

    if ((res =
            gst_rtspsrc_send (src, conninfo, &request, &response, NULL,
                NULL)) < 0)
      goto send_error;

    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);

    /* exit early when we did aggregate control */
    if (control)
      break;
  }

  /* change element states now */
  gst_rtspsrc_set_state (src, GST_STATE_PAUSED);

no_connection:
  src->state = GST_RTSP_STATE_READY;

done:
  if (async)
    gst_rtspsrc_loop_end_cmd (src, CMD_PAUSE, res);

  return res;

  /* ERRORS */
open_failed:
  {
    GST_DEBUG_OBJECT (src, "failed to open stream");
    goto done;
  }
not_supported:
  {
    GST_DEBUG_OBJECT (src, "PAUSE is not supported");
    goto done;
  }
was_paused:
  {
    GST_DEBUG_OBJECT (src, "we were already PAUSED");
    goto done;
  }
create_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request. (%s)", str));
    g_free (str);
    goto done;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    gst_rtsp_message_unset (&request);
    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
          ("Could not send message. (%s)", str));
    } else {
      GST_WARNING_OBJECT (src, "PAUSE interrupted");
    }
    g_free (str);
    goto done;
  }
}

static void
gst_rtspsrc_handle_message (GstBin * bin, GstMessage * message)
{
  GstRTSPSrc *rtspsrc;

  rtspsrc = GST_RTSPSRC (bin);

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      gst_message_unref (message);
      break;
    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure *s = gst_message_get_structure (message);

      if (gst_structure_has_name (s, "GstUDPSrcTimeout")) {
        gboolean ignore_timeout;

        GST_DEBUG_OBJECT (bin, "timeout on UDP port");

        GST_OBJECT_LOCK (rtspsrc);
        ignore_timeout = rtspsrc->ignore_timeout;
        rtspsrc->ignore_timeout = TRUE;
        GST_OBJECT_UNLOCK (rtspsrc);

        /* we only act on the first udp timeout message, others are irrelevant
         * and can be ignored. */
        if (!ignore_timeout)
          gst_rtspsrc_loop_send_cmd (rtspsrc, CMD_RECONNECT, CMD_LOOP);
        /* eat and free */
        gst_message_unref (message);
        return;
      }
      GST_BIN_CLASS (parent_class)->handle_message (bin, message);
      break;
    }
    case GST_MESSAGE_ERROR:
    {
      GstObject *udpsrc;
      GstRTSPStream *stream;
      GstFlowReturn ret;

      udpsrc = GST_MESSAGE_SRC (message);

      GST_DEBUG_OBJECT (rtspsrc, "got error from %s",
          GST_ELEMENT_NAME (udpsrc));

      stream = find_stream (rtspsrc, udpsrc, (gpointer) find_stream_by_udpsrc);
      if (!stream)
        goto forward;

      /* we ignore the RTCP udpsrc */
      if (stream->udpsrc[1] == GST_ELEMENT_CAST (udpsrc))
        goto done;

      /* if we get error messages from the udp sources, that's not a problem as
       * long as not all of them error out. We also don't really know what the
       * problem is, the message does not give enough detail... */
      ret = gst_rtspsrc_combine_flows (rtspsrc, stream, GST_FLOW_NOT_LINKED);
      GST_DEBUG_OBJECT (rtspsrc, "combined flows: %s", gst_flow_get_name (ret));
      if (ret != GST_FLOW_OK)
        goto forward;

    done:
      gst_message_unref (message);
      break;

    forward:
      /* fatal but not our message, forward */
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

/* the thread where everything happens */
static void
gst_rtspsrc_thread (GstRTSPSrc * src)
{
  gint cmd;
  ParameterRequest *req = NULL;

  GST_OBJECT_LOCK (src);
  cmd = src->pending_cmd;
  if (cmd == CMD_RECONNECT || cmd == CMD_PLAY || cmd == CMD_PAUSE
      || cmd == CMD_LOOP || cmd == CMD_OPEN || cmd == CMD_GET_PARAMETER
      || cmd == CMD_SET_PARAMETER) {
    if (g_queue_is_empty (&src->set_get_param_q)) {
      src->pending_cmd = CMD_LOOP;
    } else {
      ParameterRequest *next_req;
      req = g_queue_pop_head (&src->set_get_param_q);
      next_req = g_queue_peek_head (&src->set_get_param_q);
      src->pending_cmd = next_req ? next_req->cmd : CMD_LOOP;
    }
  } else
    src->pending_cmd = CMD_WAIT;
  GST_DEBUG_OBJECT (src, "got command %s", cmd_to_string (cmd));

  /* we got the message command, so ensure communication is possible again */
  gst_rtspsrc_connection_flush (src, FALSE);

  src->busy_cmd = cmd;
  GST_OBJECT_UNLOCK (src);

  switch (cmd) {
    case CMD_OPEN:
      gst_rtspsrc_open (src, TRUE);
      break;
    case CMD_PLAY:
      gst_rtspsrc_play (src, &src->segment, TRUE, NULL);
      break;
    case CMD_PAUSE:
      gst_rtspsrc_pause (src, TRUE);
      break;
    case CMD_CLOSE:
      gst_rtspsrc_close (src, TRUE, FALSE);
      break;
    case CMD_GET_PARAMETER:
      gst_rtspsrc_get_parameter (src, req);
      break;
    case CMD_SET_PARAMETER:
      gst_rtspsrc_set_parameter (src, req);
      break;
    case CMD_LOOP:
      gst_rtspsrc_loop (src);
      break;
    case CMD_RECONNECT:
      gst_rtspsrc_reconnect (src, FALSE);
      break;
    default:
      break;
  }

  GST_OBJECT_LOCK (src);
  /* No more cmds, wake any waiters */
  g_cond_broadcast (&src->cmd_cond);
  /* and go back to sleep */
  if (src->pending_cmd == CMD_WAIT) {
    if (src->task)
      gst_task_pause (src->task);
  }
  /* reset waiting */
  src->busy_cmd = CMD_WAIT;
  GST_OBJECT_UNLOCK (src);
}

static gboolean
gst_rtspsrc_start (GstRTSPSrc * src)
{
  GST_DEBUG_OBJECT (src, "starting");

  GST_OBJECT_LOCK (src);

  src->pending_cmd = CMD_WAIT;

  if (src->task == NULL) {
    src->task = gst_task_new ((GstTaskFunction) gst_rtspsrc_thread, src, NULL);
    if (src->task == NULL)
      goto task_error;

    gst_task_set_lock (src->task, GST_RTSP_STREAM_GET_LOCK (src));
  }
  GST_OBJECT_UNLOCK (src);

  return TRUE;

  /* ERRORS */
task_error:
  {
    GST_OBJECT_UNLOCK (src);
    GST_ERROR_OBJECT (src, "failed to create task");
    return FALSE;
  }
}

static gboolean
gst_rtspsrc_stop (GstRTSPSrc * src)
{
  GstTask *task;

  GST_DEBUG_OBJECT (src, "stopping");

  /* also cancels pending task */
  gst_rtspsrc_loop_send_cmd (src, CMD_WAIT, CMD_ALL);

  GST_OBJECT_LOCK (src);
  if ((task = src->task)) {
    src->task = NULL;
    GST_OBJECT_UNLOCK (src);

    gst_task_stop (task);

    /* make sure it is not running */
    GST_RTSP_STREAM_LOCK (src);
    GST_RTSP_STREAM_UNLOCK (src);

    /* now wait for the task to finish */
    gst_task_join (task);

    /* and free the task */
    gst_object_unref (GST_OBJECT (task));

    GST_OBJECT_LOCK (src);
  }
  GST_OBJECT_UNLOCK (src);

  /* ensure synchronously all is closed and clean */
  gst_rtspsrc_close (src, FALSE, TRUE);

  return TRUE;
}

static GstStateChangeReturn
gst_rtspsrc_change_state (GstElement * element, GstStateChange transition)
{
  GstRTSPSrc *rtspsrc;
  GstStateChangeReturn ret;

  rtspsrc = GST_RTSPSRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!gst_rtspsrc_start (rtspsrc))
        goto start_failed;
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      /* init some state */
      rtspsrc->cur_protocols = rtspsrc->protocols;
      /* first attempt, don't ignore timeouts */
      rtspsrc->ignore_timeout = FALSE;
      rtspsrc->open_error = FALSE;
      if (rtspsrc->is_live)
        gst_rtspsrc_loop_send_cmd (rtspsrc, CMD_OPEN, 0);
      else
        gst_rtspsrc_loop_send_cmd (rtspsrc, CMD_PLAY, 0);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      set_manager_buffer_mode (rtspsrc);
      /* fall-through */
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      if (rtspsrc->is_live) {
        /* unblock the tcp tasks and make the loop waiting */
        if (gst_rtspsrc_loop_send_cmd (rtspsrc, CMD_WAIT, CMD_LOOP)) {
          /* make sure it is waiting before we send PAUSE or PLAY below */
          GST_RTSP_STREAM_LOCK (rtspsrc);
          GST_RTSP_STREAM_UNLOCK (rtspsrc);
        }
      }
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto done;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      ret = GST_STATE_CHANGE_SUCCESS;
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (rtspsrc->is_live)
        ret = GST_STATE_CHANGE_NO_PREROLL;
      else
        ret = GST_STATE_CHANGE_SUCCESS;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      if (rtspsrc->is_live)
        gst_rtspsrc_loop_send_cmd (rtspsrc, CMD_PLAY, 0);
      ret = GST_STATE_CHANGE_SUCCESS;
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      if (rtspsrc->is_live) {
        /* send pause request and keep the idle task around */
        gst_rtspsrc_loop_send_cmd (rtspsrc, CMD_PAUSE, CMD_LOOP);
      }
      ret = GST_STATE_CHANGE_SUCCESS;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtspsrc_loop_send_cmd_and_wait (rtspsrc, CMD_CLOSE, CMD_ALL,
          rtspsrc->teardown_timeout);
      ret = GST_STATE_CHANGE_SUCCESS;
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_rtspsrc_stop (rtspsrc);
      ret = GST_STATE_CHANGE_SUCCESS;
      break;
    default:
      /* Otherwise it's success, we don't want to return spurious
       * NO_PREROLL or ASYNC from internal elements as we care for
       * state changes ourselves here
       *
       * This is to catch PAUSED->PAUSED and PLAYING->PLAYING transitions.
       */
      if (GST_STATE_TRANSITION_NEXT (transition) == GST_STATE_PAUSED)
        ret = GST_STATE_CHANGE_NO_PREROLL;
      else
        ret = GST_STATE_CHANGE_SUCCESS;
      break;
  }

done:
  return ret;

start_failed:
  {
    GST_DEBUG_OBJECT (rtspsrc, "start failed");
    return GST_STATE_CHANGE_FAILURE;
  }
}

static gboolean
gst_rtspsrc_send_event (GstElement * element, GstEvent * event)
{
  gboolean res;
  GstRTSPSrc *rtspsrc;

  rtspsrc = GST_RTSPSRC (element);

  if (GST_EVENT_TYPE (event) == GST_EVENT_SEEK) {
    if (rtspsrc->state >= GST_RTSP_STATE_READY) {
      res = gst_rtspsrc_perform_seek (rtspsrc, event);
      gst_event_unref (event);
    } else {
      /* Store for later use */
      res = TRUE;
      rtspsrc->initial_seek = event;
    }
  } else if (GST_EVENT_IS_DOWNSTREAM (event)) {
    res = gst_rtspsrc_push_event (rtspsrc, event);
  } else {
    res = GST_ELEMENT_CLASS (parent_class)->send_event (element, event);
  }

  return res;
}


/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
gst_rtspsrc_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_rtspsrc_uri_get_protocols (GType type)
{
  static const gchar *protocols[] =
      { "rtsp", "rtspu", "rtspt", "rtsph", "rtsp-sdp",
    "rtsps", "rtspsu", "rtspst", "rtspsh", NULL
  };

  return protocols;
}

static gchar *
gst_rtspsrc_uri_get_uri (GstURIHandler * handler)
{
  GstRTSPSrc *src = GST_RTSPSRC (handler);

  /* FIXME: make thread-safe */
  return g_strdup (src->conninfo.location);
}

static gboolean
gst_rtspsrc_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  GstRTSPSrc *src;
  GstRTSPResult res;
  GstSDPResult sres;
  GstRTSPUrl *newurl = NULL;
  GstSDPMessage *sdp = NULL;

  src = GST_RTSPSRC (handler);

  /* same URI, we're fine */
  if (src->conninfo.location && uri && !strcmp (uri, src->conninfo.location))
    goto was_ok;

  if (g_str_has_prefix (uri, "rtsp-sdp://")) {
    sres = gst_sdp_message_new (&sdp);
    if (sres < 0)
      goto sdp_failed;

    GST_DEBUG_OBJECT (src, "parsing SDP message");
    sres = gst_sdp_message_parse_uri (uri, sdp);
    if (sres < 0)
      goto invalid_sdp;
  } else {
    /* try to parse */
    GST_DEBUG_OBJECT (src, "parsing URI");
    if ((res = gst_rtsp_url_parse (uri, &newurl)) < 0)
      goto parse_error;
  }

  /* if worked, free previous and store new url object along with the original
   * location. */
  GST_DEBUG_OBJECT (src, "configuring URI");
  g_free (src->conninfo.location);
  src->conninfo.location = g_strdup (uri);
  gst_rtsp_url_free (src->conninfo.url);
  src->conninfo.url = newurl;
  g_free (src->conninfo.url_str);
  if (newurl)
    src->conninfo.url_str = gst_rtsp_url_get_request_uri (src->conninfo.url);
  else
    src->conninfo.url_str = NULL;

  if (src->sdp)
    gst_sdp_message_free (src->sdp);
  src->sdp = sdp;
  src->from_sdp = sdp != NULL;

  GST_DEBUG_OBJECT (src, "set uri: %s", GST_STR_NULL (uri));
  GST_DEBUG_OBJECT (src, "request uri is: %s",
      GST_STR_NULL (src->conninfo.url_str));

  return TRUE;

  /* Special cases */
was_ok:
  {
    GST_DEBUG_OBJECT (src, "URI was ok: '%s'", GST_STR_NULL (uri));
    return TRUE;
  }
sdp_failed:
  {
    GST_ERROR_OBJECT (src, "Could not create new SDP (%d)", sres);
    g_set_error_literal (error, GST_URI_ERROR, GST_URI_ERROR_BAD_URI,
        "Could not create SDP");
    return FALSE;
  }
invalid_sdp:
  {
    GST_ERROR_OBJECT (src, "Not a valid SDP (%d) '%s'", sres,
        GST_STR_NULL (uri));
    gst_sdp_message_free (sdp);
    g_set_error_literal (error, GST_URI_ERROR, GST_URI_ERROR_BAD_URI,
        "Invalid SDP");
    return FALSE;
  }
parse_error:
  {
    GST_ERROR_OBJECT (src, "Not a valid RTSP url '%s' (%d)",
        GST_STR_NULL (uri), res);
    g_set_error_literal (error, GST_URI_ERROR, GST_URI_ERROR_BAD_URI,
        "Invalid RTSP URI");
    return FALSE;
  }
}

static void
gst_rtspsrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_rtspsrc_uri_get_type;
  iface->get_protocols = gst_rtspsrc_uri_get_protocols;
  iface->get_uri = gst_rtspsrc_uri_get_uri;
  iface->set_uri = gst_rtspsrc_uri_set_uri;
}


/* send GET_PARAMETER */
static GstRTSPResult
gst_rtspsrc_get_parameter (GstRTSPSrc * src, ParameterRequest * req)
{
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPResult res;
  GstRTSPStatusCode code = GST_RTSP_STS_OK;
  const gchar *control;
  gchar *recv_body = NULL;
  guint recv_body_len;

  GST_DEBUG_OBJECT (src, "creating server get_parameter");

  if ((res = gst_rtspsrc_ensure_open (src, FALSE)) < 0)
    goto open_failed;

  control = get_aggregate_control (src);
  if (control == NULL)
    goto no_control;

  if (!(src->methods & GST_RTSP_GET_PARAMETER))
    goto not_supported;

  gst_rtspsrc_connection_flush (src, FALSE);

  res = gst_rtsp_message_init_request (&request, GST_RTSP_GET_PARAMETER,
      control);
  if (res < 0)
    goto create_request_failed;

  res = gst_rtsp_message_add_header (&request, GST_RTSP_HDR_CONTENT_TYPE,
      req->content_type == NULL ? "text/parameters" : req->content_type);
  if (res < 0)
    goto add_content_hdr_failed;

  if (req->body && req->body->len) {
    res =
        gst_rtsp_message_set_body (&request, (guint8 *) req->body->str,
        req->body->len);
    if (res < 0)
      goto set_body_failed;
  }

  if ((res = gst_rtspsrc_send (src, &src->conninfo,
              &request, &response, &code, NULL)) < 0)
    goto send_error;

  res = gst_rtsp_message_get_body (&response, (guint8 **) & recv_body,
      &recv_body_len);
  if (res < 0)
    goto get_body_failed;

done:
  {
    gst_promise_reply (req->promise,
        gst_structure_new ("get-parameter-reply",
            "rtsp-result", G_TYPE_INT, res,
            "rtsp-code", G_TYPE_INT, code,
            "rtsp-reason", G_TYPE_STRING, gst_rtsp_status_as_text (code),
            "body", G_TYPE_STRING, GST_STR_NULL (recv_body), NULL));
    free_param_data (req);


    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);

    return res;
  }

  /* ERRORS */
open_failed:
  {
    GST_DEBUG_OBJECT (src, "failed to open stream");
    goto done;
  }
no_control:
  {
    GST_DEBUG_OBJECT (src, "no control url to send GET_PARAMETER");
    res = GST_RTSP_ERROR;
    goto done;
  }
not_supported:
  {
    GST_DEBUG_OBJECT (src, "GET_PARAMETER is not supported");
    res = GST_RTSP_ERROR;
    goto done;
  }
create_request_failed:
  {
    GST_DEBUG_OBJECT (src, "could not create GET_PARAMETER request");
    goto done;
  }
add_content_hdr_failed:
  {
    GST_DEBUG_OBJECT (src, "could not add content header");
    goto done;
  }
set_body_failed:
  {
    GST_DEBUG_OBJECT (src, "could not set body");
    goto done;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_WARNING (src, RESOURCE, WRITE, (NULL),
        ("Could not send get-parameter. (%s)", str));
    g_free (str);
    goto done;
  }
get_body_failed:
  {
    GST_DEBUG_OBJECT (src, "could not get body");
    goto done;
  }
}

/* send SET_PARAMETER */
static GstRTSPResult
gst_rtspsrc_set_parameter (GstRTSPSrc * src, ParameterRequest * req)
{
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPResult res = GST_RTSP_OK;
  GstRTSPStatusCode code = GST_RTSP_STS_OK;
  const gchar *control;

  GST_DEBUG_OBJECT (src, "creating server set_parameter");

  if ((res = gst_rtspsrc_ensure_open (src, FALSE)) < 0)
    goto open_failed;

  control = get_aggregate_control (src);
  if (control == NULL)
    goto no_control;

  if (!(src->methods & GST_RTSP_SET_PARAMETER))
    goto not_supported;

  gst_rtspsrc_connection_flush (src, FALSE);

  res =
      gst_rtsp_message_init_request (&request, GST_RTSP_SET_PARAMETER, control);
  if (res < 0)
    goto send_error;

  res = gst_rtsp_message_add_header (&request, GST_RTSP_HDR_CONTENT_TYPE,
      req->content_type == NULL ? "text/parameters" : req->content_type);
  if (res < 0)
    goto add_content_hdr_failed;

  if (req->body && req->body->len) {
    res =
        gst_rtsp_message_set_body (&request, (guint8 *) req->body->str,
        req->body->len);

    if (res < 0)
      goto set_body_failed;
  }

  if ((res = gst_rtspsrc_send (src, &src->conninfo,
              &request, &response, &code, NULL)) < 0)
    goto send_error;

done:
  {
    gst_promise_reply (req->promise, gst_structure_new ("set-parameter-reply",
            "rtsp-result", G_TYPE_INT, res,
            "rtsp-code", G_TYPE_INT, code,
            "rtsp-reason", G_TYPE_STRING, gst_rtsp_status_as_text (code),
            NULL));
    free_param_data (req);

    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);

    return res;
  }

  /* ERRORS */
open_failed:
  {
    GST_DEBUG_OBJECT (src, "failed to open stream");
    goto done;
  }
no_control:
  {
    GST_DEBUG_OBJECT (src, "no control url to send SET_PARAMETER");
    res = GST_RTSP_ERROR;
    goto done;
  }
not_supported:
  {
    GST_DEBUG_OBJECT (src, "SET_PARAMETER is not supported");
    res = GST_RTSP_ERROR;
    goto done;
  }
add_content_hdr_failed:
  {
    GST_DEBUG_OBJECT (src, "could not add content header");
    goto done;
  }
set_body_failed:
  {
    GST_DEBUG_OBJECT (src, "could not set body");
    goto done;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_WARNING (src, RESOURCE, WRITE, (NULL),
        ("Could not send set-parameter. (%s)", str));
    g_free (str);
    goto done;
  }
}

typedef struct _RTSPKeyValue
{
  GstRTSPHeaderField field;
  gchar *value;
  gchar *custom_key;            /* custom header string (field is INVALID then) */
} RTSPKeyValue;

static void
key_value_foreach (GArray * array, GFunc func, gpointer user_data)
{
  guint i;

  g_return_if_fail (array != NULL);

  for (i = 0; i < array->len; i++) {
    (*func) (&g_array_index (array, RTSPKeyValue, i), user_data);
  }
}

static void
dump_key_value (gpointer data, gpointer user_data G_GNUC_UNUSED)
{
  RTSPKeyValue *key_value = (RTSPKeyValue *) data;
  GstRTSPSrc *src = GST_RTSPSRC (user_data);
  const gchar *key_string;

  if (key_value->custom_key != NULL)
    key_string = key_value->custom_key;
  else
    key_string = gst_rtsp_header_as_text (key_value->field);

  GST_LOG_OBJECT (src, "   key: '%s', value: '%s'", key_string,
      key_value->value);
}

static void
gst_rtspsrc_print_rtsp_message (GstRTSPSrc * src, const GstRTSPMessage * msg)
{
  guint8 *data;
  guint size;
  GString *body_string = NULL;

  g_return_if_fail (src != NULL);
  g_return_if_fail (msg != NULL);

  if (gst_debug_category_get_threshold (GST_CAT_DEFAULT) < GST_LEVEL_LOG)
    return;

  GST_LOG_OBJECT (src, "--------------------------------------------");
  switch (msg->type) {
    case GST_RTSP_MESSAGE_REQUEST:
      GST_LOG_OBJECT (src, "RTSP request message %p", msg);
      GST_LOG_OBJECT (src, " request line:");
      GST_LOG_OBJECT (src, "   method: '%s'",
          gst_rtsp_method_as_text (msg->type_data.request.method));
      GST_LOG_OBJECT (src, "   uri:    '%s'", msg->type_data.request.uri);
      GST_LOG_OBJECT (src, "   version: '%s'",
          gst_rtsp_version_as_text (msg->type_data.request.version));
      GST_LOG_OBJECT (src, " headers:");
      key_value_foreach (msg->hdr_fields, dump_key_value, src);
      GST_LOG_OBJECT (src, " body:");
      gst_rtsp_message_get_body (msg, &data, &size);
      if (size > 0) {
        body_string = g_string_new_len ((const gchar *) data, size);
        GST_LOG_OBJECT (src, " %s(%d)", body_string->str, size);
        g_string_free (body_string, TRUE);
        body_string = NULL;
      }
      break;
    case GST_RTSP_MESSAGE_RESPONSE:
      GST_LOG_OBJECT (src, "RTSP response message %p", msg);
      GST_LOG_OBJECT (src, " status line:");
      GST_LOG_OBJECT (src, "   code:   '%d'", msg->type_data.response.code);
      GST_LOG_OBJECT (src, "   reason: '%s'", msg->type_data.response.reason);
      GST_LOG_OBJECT (src, "   version: '%s",
          gst_rtsp_version_as_text (msg->type_data.response.version));
      GST_LOG_OBJECT (src, " headers:");
      key_value_foreach (msg->hdr_fields, dump_key_value, src);
      gst_rtsp_message_get_body (msg, &data, &size);
      GST_LOG_OBJECT (src, " body: length %d", size);
      if (size > 0) {
        body_string = g_string_new_len ((const gchar *) data, size);
        GST_LOG_OBJECT (src, " %s(%d)", body_string->str, size);
        g_string_free (body_string, TRUE);
        body_string = NULL;
      }
      break;
    case GST_RTSP_MESSAGE_HTTP_REQUEST:
      GST_LOG_OBJECT (src, "HTTP request message %p", msg);
      GST_LOG_OBJECT (src, " request line:");
      GST_LOG_OBJECT (src, "   method:  '%s'",
          gst_rtsp_method_as_text (msg->type_data.request.method));
      GST_LOG_OBJECT (src, "   uri:     '%s'", msg->type_data.request.uri);
      GST_LOG_OBJECT (src, "   version: '%s'",
          gst_rtsp_version_as_text (msg->type_data.request.version));
      GST_LOG_OBJECT (src, " headers:");
      key_value_foreach (msg->hdr_fields, dump_key_value, src);
      GST_LOG_OBJECT (src, " body:");
      gst_rtsp_message_get_body (msg, &data, &size);
      if (size > 0) {
        body_string = g_string_new_len ((const gchar *) data, size);
        GST_LOG_OBJECT (src, " %s(%d)", body_string->str, size);
        g_string_free (body_string, TRUE);
        body_string = NULL;
      }
      break;
    case GST_RTSP_MESSAGE_HTTP_RESPONSE:
      GST_LOG_OBJECT (src, "HTTP response message %p", msg);
      GST_LOG_OBJECT (src, " status line:");
      GST_LOG_OBJECT (src, "   code:    '%d'", msg->type_data.response.code);
      GST_LOG_OBJECT (src, "   reason:  '%s'", msg->type_data.response.reason);
      GST_LOG_OBJECT (src, "   version: '%s'",
          gst_rtsp_version_as_text (msg->type_data.response.version));
      GST_LOG_OBJECT (src, " headers:");
      key_value_foreach (msg->hdr_fields, dump_key_value, src);
      gst_rtsp_message_get_body (msg, &data, &size);
      GST_LOG_OBJECT (src, " body: length %d", size);
      if (size > 0) {
        body_string = g_string_new_len ((const gchar *) data, size);
        GST_LOG_OBJECT (src, " %s(%d)", body_string->str, size);
        g_string_free (body_string, TRUE);
        body_string = NULL;
      }
      break;
    case GST_RTSP_MESSAGE_DATA:
      GST_LOG_OBJECT (src, "RTSP data message %p", msg);
      GST_LOG_OBJECT (src, " channel: '%d'", msg->type_data.data.channel);
      GST_LOG_OBJECT (src, " size:    '%d'", msg->body_size);
      gst_rtsp_message_get_body (msg, &data, &size);
      if (size > 0) {
        body_string = g_string_new_len ((const gchar *) data, size);
        GST_LOG_OBJECT (src, " %s(%d)", body_string->str, size);
        g_string_free (body_string, TRUE);
        body_string = NULL;
      }
      break;
    default:
      GST_LOG_OBJECT (src, "unsupported message type %d", msg->type);
      break;
  }
  GST_LOG_OBJECT (src, "--------------------------------------------");
}

static void
gst_rtspsrc_print_sdp_media (GstRTSPSrc * src, GstSDPMedia * media)
{
  GST_LOG_OBJECT (src, "   media:       '%s'", GST_STR_NULL (media->media));
  GST_LOG_OBJECT (src, "   port:        '%u'", media->port);
  GST_LOG_OBJECT (src, "   num_ports:   '%u'", media->num_ports);
  GST_LOG_OBJECT (src, "   proto:       '%s'", GST_STR_NULL (media->proto));
  if (media->fmts && media->fmts->len > 0) {
    guint i;

    GST_LOG_OBJECT (src, "   formats:");
    for (i = 0; i < media->fmts->len; i++) {
      GST_LOG_OBJECT (src, "    format  '%s'", g_array_index (media->fmts,
              gchar *, i));
    }
  }
  GST_LOG_OBJECT (src, "   information: '%s'",
      GST_STR_NULL (media->information));
  if (media->connections && media->connections->len > 0) {
    guint i;

    GST_LOG_OBJECT (src, "   connections:");
    for (i = 0; i < media->connections->len; i++) {
      GstSDPConnection *conn =
          &g_array_index (media->connections, GstSDPConnection, i);

      GST_LOG_OBJECT (src, "    nettype:      '%s'",
          GST_STR_NULL (conn->nettype));
      GST_LOG_OBJECT (src, "    addrtype:     '%s'",
          GST_STR_NULL (conn->addrtype));
      GST_LOG_OBJECT (src, "    address:      '%s'",
          GST_STR_NULL (conn->address));
      GST_LOG_OBJECT (src, "    ttl:          '%u'", conn->ttl);
      GST_LOG_OBJECT (src, "    addr_number:  '%u'", conn->addr_number);
    }
  }
  if (media->bandwidths && media->bandwidths->len > 0) {
    guint i;

    GST_LOG_OBJECT (src, "   bandwidths:");
    for (i = 0; i < media->bandwidths->len; i++) {
      GstSDPBandwidth *bw =
          &g_array_index (media->bandwidths, GstSDPBandwidth, i);

      GST_LOG_OBJECT (src, "    type:         '%s'", GST_STR_NULL (bw->bwtype));
      GST_LOG_OBJECT (src, "    bandwidth:    '%u'", bw->bandwidth);
    }
  }
  GST_LOG_OBJECT (src, "   key:");
  GST_LOG_OBJECT (src, "    type:       '%s'", GST_STR_NULL (media->key.type));
  GST_LOG_OBJECT (src, "    data:       '%s'", GST_STR_NULL (media->key.data));
  if (media->attributes && media->attributes->len > 0) {
    guint i;

    GST_LOG_OBJECT (src, "   attributes:");
    for (i = 0; i < media->attributes->len; i++) {
      GstSDPAttribute *attr =
          &g_array_index (media->attributes, GstSDPAttribute, i);

      GST_LOG_OBJECT (src, "    attribute '%s' : '%s'", attr->key, attr->value);
    }
  }
}

void
gst_rtspsrc_print_sdp_message (GstRTSPSrc * src, const GstSDPMessage * msg)
{
  g_return_if_fail (src != NULL);
  g_return_if_fail (msg != NULL);

  if (gst_debug_category_get_threshold (GST_CAT_DEFAULT) < GST_LEVEL_LOG)
    return;

  GST_LOG_OBJECT (src, "--------------------------------------------");
  GST_LOG_OBJECT (src, "sdp packet %p:", msg);
  GST_LOG_OBJECT (src, " version:       '%s'", GST_STR_NULL (msg->version));
  GST_LOG_OBJECT (src, " origin:");
  GST_LOG_OBJECT (src, "  username:     '%s'",
      GST_STR_NULL (msg->origin.username));
  GST_LOG_OBJECT (src, "  sess_id:      '%s'",
      GST_STR_NULL (msg->origin.sess_id));
  GST_LOG_OBJECT (src, "  sess_version: '%s'",
      GST_STR_NULL (msg->origin.sess_version));
  GST_LOG_OBJECT (src, "  nettype:      '%s'",
      GST_STR_NULL (msg->origin.nettype));
  GST_LOG_OBJECT (src, "  addrtype:     '%s'",
      GST_STR_NULL (msg->origin.addrtype));
  GST_LOG_OBJECT (src, "  addr:         '%s'", GST_STR_NULL (msg->origin.addr));
  GST_LOG_OBJECT (src, " session_name:  '%s'",
      GST_STR_NULL (msg->session_name));
  GST_LOG_OBJECT (src, " information:   '%s'", GST_STR_NULL (msg->information));
  GST_LOG_OBJECT (src, " uri:           '%s'", GST_STR_NULL (msg->uri));

  if (msg->emails && msg->emails->len > 0) {
    guint i;

    GST_LOG_OBJECT (src, " emails:");
    for (i = 0; i < msg->emails->len; i++) {
      GST_LOG_OBJECT (src, "  email '%s'", g_array_index (msg->emails, gchar *,
              i));
    }
  }
  if (msg->phones && msg->phones->len > 0) {
    guint i;

    GST_LOG_OBJECT (src, " phones:");
    for (i = 0; i < msg->phones->len; i++) {
      GST_LOG_OBJECT (src, "  phone '%s'", g_array_index (msg->phones, gchar *,
              i));
    }
  }
  GST_LOG_OBJECT (src, " connection:");
  GST_LOG_OBJECT (src, "  nettype:      '%s'",
      GST_STR_NULL (msg->connection.nettype));
  GST_LOG_OBJECT (src, "  addrtype:     '%s'",
      GST_STR_NULL (msg->connection.addrtype));
  GST_LOG_OBJECT (src, "  address:      '%s'",
      GST_STR_NULL (msg->connection.address));
  GST_LOG_OBJECT (src, "  ttl:          '%u'", msg->connection.ttl);
  GST_LOG_OBJECT (src, "  addr_number:  '%u'", msg->connection.addr_number);
  if (msg->bandwidths && msg->bandwidths->len > 0) {
    guint i;

    GST_LOG_OBJECT (src, " bandwidths:");
    for (i = 0; i < msg->bandwidths->len; i++) {
      GstSDPBandwidth *bw =
          &g_array_index (msg->bandwidths, GstSDPBandwidth, i);

      GST_LOG_OBJECT (src, "  type:         '%s'", GST_STR_NULL (bw->bwtype));
      GST_LOG_OBJECT (src, "  bandwidth:    '%u'", bw->bandwidth);
    }
  }
  GST_LOG_OBJECT (src, " key:");
  GST_LOG_OBJECT (src, "  type:         '%s'", GST_STR_NULL (msg->key.type));
  GST_LOG_OBJECT (src, "  data:         '%s'", GST_STR_NULL (msg->key.data));
  if (msg->attributes && msg->attributes->len > 0) {
    guint i;

    GST_LOG_OBJECT (src, " attributes:");
    for (i = 0; i < msg->attributes->len; i++) {
      GstSDPAttribute *attr =
          &g_array_index (msg->attributes, GstSDPAttribute, i);

      GST_LOG_OBJECT (src, "  attribute '%s' : '%s'", attr->key, attr->value);
    }
  }
  if (msg->medias && msg->medias->len > 0) {
    guint i;

    GST_LOG_OBJECT (src, " medias:");
    for (i = 0; i < msg->medias->len; i++) {
      GST_LOG_OBJECT (src, "  media %u:", i);
      gst_rtspsrc_print_sdp_media (src, &g_array_index (msg->medias,
              GstSDPMedia, i));
    }
  }
  GST_LOG_OBJECT (src, "--------------------------------------------");
}
