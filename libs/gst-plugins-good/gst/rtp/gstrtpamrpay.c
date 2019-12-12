/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
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
 * SECTION:element-rtpamrpay
 * @title: rtpamrpay
 * @see_also: rtpamrdepay
 *
 * Payload AMR audio into RTP packets according to RFC 3267.
 * For detailed information see: http://www.rfc-editor.org/rfc/rfc3267.txt
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 -v audiotestsrc ! amrnbenc ! rtpamrpay ! udpsink
 * ]| This example pipeline will encode and payload an AMR stream. Refer to
 * the rtpamrdepay example to depayload and decode the RTP stream.
 *
 */

/* references:
 *
 * RFC 3267 - Real-Time Transport Protocol (RTP) Payload Format and File
 *    Storage Format for the Adaptive Multi-Rate (AMR) and Adaptive
 *    Multi-Rate Wideband (AMR-WB) Audio Codecs.
 *
 * ETSI TS 126 201 V6.0.0 (2004-12) - Digital cellular telecommunications system (Phase 2+);
 *                 Universal Mobile Telecommunications System (UMTS);
 *                          AMR speech codec, wideband;
 *                                 Frame structure
 *                    (3GPP TS 26.201 version 6.0.0 Release 6)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>

#include "gstrtpamrpay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpamrpay_debug);
#define GST_CAT_DEFAULT (rtpamrpay_debug)

static GstStaticPadTemplate gst_rtp_amr_pay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/AMR, channels=(int)1, rate=(int)8000; "
        "audio/AMR-WB, channels=(int)1, rate=(int)16000")
    );

static GstStaticPadTemplate gst_rtp_amr_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 8000, "
        "encoding-name = (string) \"AMR\", "
        "encoding-params = (string) \"1\", "
        "octet-align = (string) \"1\", "
        "crc = (string) \"0\", "
        "robust-sorting = (string) \"0\", "
        "interleaving = (string) \"0\", "
        "mode-set = (int) [ 0, 7 ], "
        "mode-change-period = (int) [ 1, MAX ], "
        "mode-change-neighbor = (string) { \"0\", \"1\" }, "
        "maxptime = (int) [ 20, MAX ], " "ptime = (int) [ 20, MAX ];"
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 16000, "
        "encoding-name = (string) \"AMR-WB\", "
        "encoding-params = (string) \"1\", "
        "octet-align = (string) \"1\", "
        "crc = (string) \"0\", "
        "robust-sorting = (string) \"0\", "
        "interleaving = (string) \"0\", "
        "mode-set = (int) [ 0, 7 ], "
        "mode-change-period = (int) [ 1, MAX ], "
        "mode-change-neighbor = (string) { \"0\", \"1\" }, "
        "maxptime = (int) [ 20, MAX ], " "ptime = (int) [ 20, MAX ]")
    );

static gboolean gst_rtp_amr_pay_setcaps (GstRTPBasePayload * basepayload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_amr_pay_handle_buffer (GstRTPBasePayload * pad,
    GstBuffer * buffer);

static GstStateChangeReturn
gst_rtp_amr_pay_change_state (GstElement * element, GstStateChange transition);

#define gst_rtp_amr_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpAMRPay, gst_rtp_amr_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_amr_pay_class_init (GstRtpAMRPayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gstelement_class->change_state = gst_rtp_amr_pay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_amr_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_amr_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class, "RTP AMR payloader",
      "Codec/Payloader/Network/RTP",
      "Payload-encode AMR or AMR-WB audio into RTP packets (RFC 3267)",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasepayload_class->set_caps = gst_rtp_amr_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_amr_pay_handle_buffer;

  GST_DEBUG_CATEGORY_INIT (rtpamrpay_debug, "rtpamrpay", 0,
      "AMR/AMR-WB RTP Payloader");
}

static void
gst_rtp_amr_pay_init (GstRtpAMRPay * rtpamrpay)
{
}

static void
gst_rtp_amr_pay_reset (GstRtpAMRPay * pay)
{
  pay->next_rtp_time = 0;
  pay->first_ts = GST_CLOCK_TIME_NONE;
  pay->first_rtp_time = 0;
}

static gboolean
gst_rtp_amr_pay_setcaps (GstRTPBasePayload * basepayload, GstCaps * caps)
{
  GstRtpAMRPay *rtpamrpay;
  gboolean res;
  const GstStructure *s;
  const gchar *str;

  rtpamrpay = GST_RTP_AMR_PAY (basepayload);

  /* figure out the mode Narrow or Wideband */
  s = gst_caps_get_structure (caps, 0);
  if ((str = gst_structure_get_name (s))) {
    if (strcmp (str, "audio/AMR") == 0)
      rtpamrpay->mode = GST_RTP_AMR_P_MODE_NB;
    else if (strcmp (str, "audio/AMR-WB") == 0)
      rtpamrpay->mode = GST_RTP_AMR_P_MODE_WB;
    else
      goto wrong_type;
  } else
    goto wrong_type;

  if (rtpamrpay->mode == GST_RTP_AMR_P_MODE_NB)
    gst_rtp_base_payload_set_options (basepayload, "audio", TRUE, "AMR", 8000);
  else
    gst_rtp_base_payload_set_options (basepayload, "audio", TRUE, "AMR-WB",
        16000);

  res = gst_rtp_base_payload_set_outcaps (basepayload,
      "encoding-params", G_TYPE_STRING, "1", "octet-align", G_TYPE_STRING, "1",
      /* don't set the defaults
       *
       * "crc", G_TYPE_STRING, "0",
       * "robust-sorting", G_TYPE_STRING, "0",
       * "interleaving", G_TYPE_STRING, "0",
       */
      NULL);

  return res;

  /* ERRORS */
wrong_type:
  {
    GST_ERROR_OBJECT (rtpamrpay, "unsupported media type '%s'",
        GST_STR_NULL (str));
    return FALSE;
  }
}

static void
gst_rtp_amr_pay_recalc_rtp_time (GstRtpAMRPay * rtpamrpay,
    GstClockTime timestamp)
{
  /* re-sync rtp time */
  if (GST_CLOCK_TIME_IS_VALID (rtpamrpay->first_ts) &&
      GST_CLOCK_TIME_IS_VALID (timestamp) && timestamp >= rtpamrpay->first_ts) {
    GstClockTime diff;
    guint32 rtpdiff;

    /* interpolate to reproduce gap from start, rather than intermediate
     * intervals to avoid roundup accumulation errors */
    diff = timestamp - rtpamrpay->first_ts;
    rtpdiff = ((diff / GST_MSECOND) * 8) <<
        (rtpamrpay->mode == GST_RTP_AMR_P_MODE_WB);
    rtpamrpay->next_rtp_time = rtpamrpay->first_rtp_time + rtpdiff;
    GST_DEBUG_OBJECT (rtpamrpay,
        "elapsed time %" GST_TIME_FORMAT ", rtp %" G_GUINT32_FORMAT ", "
        "new offset %" G_GUINT32_FORMAT, GST_TIME_ARGS (diff), rtpdiff,
        rtpamrpay->next_rtp_time);
  }
}

/* -1 is invalid */
static const gint nb_frame_size[16] = {
  12, 13, 15, 17, 19, 20, 26, 31,
  5, -1, -1, -1, -1, -1, -1, 0
};

static const gint wb_frame_size[16] = {
  17, 23, 32, 36, 40, 46, 50, 58,
  60, 5, -1, -1, -1, -1, -1, 0
};

static GstFlowReturn
gst_rtp_amr_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpAMRPay *rtpamrpay;
  const gint *frame_size;
  GstFlowReturn ret;
  guint payload_len;
  GstMapInfo map;
  GstBuffer *outbuf;
  guint8 *payload, *ptr, *payload_amr;
  GstClockTime timestamp, duration;
  guint packet_len, mtu;
  gint i, num_packets, num_nonempty_packets;
  gint amr_len;
  gboolean sid = FALSE;
  GstRTPBuffer rtp = { NULL };

  rtpamrpay = GST_RTP_AMR_PAY (basepayload);
  mtu = GST_RTP_BASE_PAYLOAD_MTU (rtpamrpay);

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  timestamp = GST_BUFFER_PTS (buffer);
  duration = GST_BUFFER_DURATION (buffer);

  /* setup frame size pointer */
  if (rtpamrpay->mode == GST_RTP_AMR_P_MODE_NB)
    frame_size = nb_frame_size;
  else
    frame_size = wb_frame_size;

  GST_DEBUG_OBJECT (basepayload, "got %" G_GSIZE_FORMAT " bytes", map.size);

  /* FIXME, only
   * octet aligned, no interleaving, single channel, no CRC,
   * no robust-sorting. To fix this you need to implement the downstream
   * negotiation function. */

  /* first count number of packets and total amr frame size */
  amr_len = num_packets = num_nonempty_packets = 0;
  for (i = 0; i < map.size; i++) {
    guint8 FT;
    gint fr_size;

    FT = (map.data[i] & 0x78) >> 3;

    fr_size = frame_size[FT];
    GST_DEBUG_OBJECT (basepayload, "frame type %d, frame size %d", FT, fr_size);
    /* FIXME, we don't handle this yet.. */
    if (fr_size <= 0)
      goto wrong_size;

    if (fr_size == 5)
      sid = TRUE;

    amr_len += fr_size;
    num_nonempty_packets++;
    num_packets++;
    i += fr_size;
  }
  if (amr_len > map.size)
    goto incomplete_frame;

  /* we need one extra byte for the CMR, the ToC is in the input
   * data */
  payload_len = map.size + 1;

  /* get packet len to check against MTU */
  packet_len = gst_rtp_buffer_calc_packet_len (payload_len, 0, 0);
  if (packet_len > mtu)
    goto too_big;

  /* now alloc output buffer */
  outbuf = gst_rtp_buffer_new_allocate (payload_len, 0, 0);

  gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);

  /* copy timestamp */
  GST_BUFFER_PTS (outbuf) = timestamp;

  if (duration != GST_CLOCK_TIME_NONE)
    GST_BUFFER_DURATION (outbuf) = duration;
  else {
    GST_BUFFER_DURATION (outbuf) = num_packets * 20 * GST_MSECOND;
  }

  if (GST_BUFFER_IS_DISCONT (buffer)) {
    GST_DEBUG_OBJECT (basepayload, "discont, setting marker bit");
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
    gst_rtp_buffer_set_marker (&rtp, TRUE);
    gst_rtp_amr_pay_recalc_rtp_time (rtpamrpay, timestamp);
  }

  if (G_UNLIKELY (sid)) {
    gst_rtp_amr_pay_recalc_rtp_time (rtpamrpay, timestamp);
  }

  /* perfect rtptime */
  if (G_UNLIKELY (!GST_CLOCK_TIME_IS_VALID (rtpamrpay->first_ts))) {
    rtpamrpay->first_ts = timestamp;
    rtpamrpay->first_rtp_time = rtpamrpay->next_rtp_time;
  }
  GST_BUFFER_OFFSET (outbuf) = rtpamrpay->next_rtp_time;
  rtpamrpay->next_rtp_time +=
      (num_packets * 160) << (rtpamrpay->mode == GST_RTP_AMR_P_MODE_WB);

  /* get payload, this is now writable */
  payload = gst_rtp_buffer_get_payload (&rtp);

  /*   0 1 2 3 4 5 6 7
   *  +-+-+-+-+-+-+-+-+
   *  |  CMR  |R|R|R|R|
   *  +-+-+-+-+-+-+-+-+
   */
  payload[0] = 0xF0;            /* CMR, no specific mode requested */

  /* this is where we copy the AMR data, after num_packets FTs and the
   * CMR. */
  payload_amr = payload + num_packets + 1;

  /* copy data in payload, first we copy all the FTs then all
   * the AMR data. The last FT has to have the F flag cleared. */
  ptr = map.data;
  for (i = 1; i <= num_packets; i++) {
    guint8 FT;
    gint fr_size;

    /*   0 1 2 3 4 5 6 7
     *  +-+-+-+-+-+-+-+-+
     *  |F|  FT   |Q|P|P| more FT...
     *  +-+-+-+-+-+-+-+-+
     */
    FT = (*ptr & 0x78) >> 3;

    fr_size = frame_size[FT];

    if (i == num_packets)
      /* last packet, clear F flag */
      payload[i] = *ptr & 0x7f;
    else
      /* set F flag */
      payload[i] = *ptr | 0x80;

    memcpy (payload_amr, &ptr[1], fr_size);

    /* all sizes are > 0 since we checked for that above */
    ptr += fr_size + 1;
    payload_amr += fr_size;
  }

  gst_buffer_unmap (buffer, &map);
  gst_rtp_buffer_unmap (&rtp);

  gst_rtp_copy_audio_meta (rtpamrpay, outbuf, buffer);

  gst_buffer_unref (buffer);

  ret = gst_rtp_base_payload_push (basepayload, outbuf);

  return ret;

  /* ERRORS */
wrong_size:
  {
    GST_ELEMENT_ERROR (basepayload, STREAM, FORMAT,
        (NULL), ("received AMR frame with size <= 0"));
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);

    return GST_FLOW_ERROR;
  }
incomplete_frame:
  {
    GST_ELEMENT_ERROR (basepayload, STREAM, FORMAT,
        (NULL), ("received incomplete AMR frames"));
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);

    return GST_FLOW_ERROR;
  }
too_big:
  {
    GST_ELEMENT_ERROR (basepayload, STREAM, FORMAT,
        (NULL), ("received too many AMR frames for MTU"));
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);

    return GST_FLOW_ERROR;
  }
}

static GstStateChangeReturn
gst_rtp_amr_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  /* handle upwards state changes here */
  switch (transition) {
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  /* handle downwards state changes */
  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_amr_pay_reset (GST_RTP_AMR_PAY (element));
      break;
    default:
      break;
  }

  return ret;
}

gboolean
gst_rtp_amr_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpamrpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_AMR_PAY);
}
