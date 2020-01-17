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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>

#include "gstrtpmpapay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpmpapay_debug);
#define GST_CAT_DEFAULT (rtpmpapay_debug)

static GstStaticPadTemplate gst_rtp_mpa_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg, " "mpegversion = (int) 1")
    );

static GstStaticPadTemplate gst_rtp_mpa_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_MPA_STRING ", "
        "clock-rate = (int) 90000; "
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"MPA\"")
    );

static void gst_rtp_mpa_pay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_mpa_pay_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_rtp_mpa_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static gboolean gst_rtp_mpa_pay_sink_event (GstRTPBasePayload * payload,
    GstEvent * event);
static GstFlowReturn gst_rtp_mpa_pay_flush (GstRtpMPAPay * rtpmpapay);
static GstFlowReturn gst_rtp_mpa_pay_handle_buffer (GstRTPBasePayload * payload,
    GstBuffer * buffer);

#define gst_rtp_mpa_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpMPAPay, gst_rtp_mpa_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_mpa_pay_class_init (GstRtpMPAPayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpmpapay_debug, "rtpmpapay", 0,
      "MPEG Audio RTP Depayloader");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->finalize = gst_rtp_mpa_pay_finalize;

  gstelement_class->change_state = gst_rtp_mpa_pay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mpa_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mpa_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP MPEG audio payloader", "Codec/Payloader/Network/RTP",
      "Payload MPEG audio as RTP packets (RFC 2038)",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasepayload_class->set_caps = gst_rtp_mpa_pay_setcaps;
  gstrtpbasepayload_class->sink_event = gst_rtp_mpa_pay_sink_event;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_mpa_pay_handle_buffer;
}

static void
gst_rtp_mpa_pay_init (GstRtpMPAPay * rtpmpapay)
{
  rtpmpapay->adapter = gst_adapter_new ();

  GST_RTP_BASE_PAYLOAD (rtpmpapay)->pt = GST_RTP_PAYLOAD_MPA;
}

static void
gst_rtp_mpa_pay_finalize (GObject * object)
{
  GstRtpMPAPay *rtpmpapay;

  rtpmpapay = GST_RTP_MPA_PAY (object);

  g_object_unref (rtpmpapay->adapter);
  rtpmpapay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_mpa_pay_reset (GstRtpMPAPay * pay)
{
  pay->first_ts = -1;
  pay->duration = 0;
  gst_adapter_clear (pay->adapter);
  GST_DEBUG_OBJECT (pay, "reset depayloader");
}

static gboolean
gst_rtp_mpa_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  gboolean res;

  gst_rtp_base_payload_set_options (payload, "audio",
      payload->pt != GST_RTP_PAYLOAD_MPA, "MPA", 90000);
  res = gst_rtp_base_payload_set_outcaps (payload, NULL);

  return res;
}

static gboolean
gst_rtp_mpa_pay_sink_event (GstRTPBasePayload * payload, GstEvent * event)
{
  gboolean ret;
  GstRtpMPAPay *rtpmpapay;

  rtpmpapay = GST_RTP_MPA_PAY (payload);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      /* make sure we push the last packets in the adapter on EOS */
      gst_rtp_mpa_pay_flush (rtpmpapay);
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_mpa_pay_reset (rtpmpapay);
      break;
    default:
      break;
  }

  ret = GST_RTP_BASE_PAYLOAD_CLASS (parent_class)->sink_event (payload, event);

  return ret;
}

#define RTP_HEADER_LEN 12

static GstFlowReturn
gst_rtp_mpa_pay_flush (GstRtpMPAPay * rtpmpapay)
{
  guint avail;
  GstBuffer *outbuf;
  GstFlowReturn ret;
  guint16 frag_offset;
  GstBufferList *list;

  /* the data available in the adapter is either smaller
   * than the MTU or bigger. In the case it is smaller, the complete
   * adapter contents can be put in one packet. In the case the
   * adapter has more than one MTU, we need to split the MPA data
   * over multiple packets. The frag_offset in each packet header
   * needs to be updated with the position in the MPA frame. */
  avail = gst_adapter_available (rtpmpapay->adapter);

  ret = GST_FLOW_OK;

  list =
      gst_buffer_list_new_sized (avail / (GST_RTP_BASE_PAYLOAD_MTU (rtpmpapay) -
          RTP_HEADER_LEN) + 1);

  frag_offset = 0;
  while (avail > 0) {
    guint towrite;
    guint8 *payload;
    guint payload_len;
    guint packet_len;
    GstRTPBuffer rtp = { NULL };
    GstBuffer *paybuf;

    /* this will be the total length of the packet */
    packet_len = gst_rtp_buffer_calc_packet_len (4 + avail, 0, 0);

    /* fill one MTU or all available bytes */
    towrite = MIN (packet_len, GST_RTP_BASE_PAYLOAD_MTU (rtpmpapay));

    /* this is the payload length */
    payload_len = gst_rtp_buffer_calc_payload_len (towrite, 0, 0);

    /* create buffer to hold the payload */
    outbuf = gst_rtp_buffer_new_allocate (4, 0, 0);

    gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);

    payload_len -= 4;

    gst_rtp_buffer_set_payload_type (&rtp, GST_RTP_PAYLOAD_MPA);

    /*
     *  0                   1                   2                   3
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |             MBZ               |          Frag_offset          |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    payload = gst_rtp_buffer_get_payload (&rtp);
    payload[0] = 0;
    payload[1] = 0;
    payload[2] = frag_offset >> 8;
    payload[3] = frag_offset & 0xff;

    avail -= payload_len;
    frag_offset += payload_len;

    if (avail == 0)
      gst_rtp_buffer_set_marker (&rtp, TRUE);

    gst_rtp_buffer_unmap (&rtp);

    paybuf = gst_adapter_take_buffer_fast (rtpmpapay->adapter, payload_len);
    gst_rtp_copy_audio_meta (rtpmpapay, outbuf, paybuf);
    outbuf = gst_buffer_append (outbuf, paybuf);

    GST_BUFFER_PTS (outbuf) = rtpmpapay->first_ts;
    GST_BUFFER_DURATION (outbuf) = rtpmpapay->duration;
    gst_buffer_list_add (list, outbuf);
  }

  ret = gst_rtp_base_payload_push_list (GST_RTP_BASE_PAYLOAD (rtpmpapay), list);

  return ret;
}

static GstFlowReturn
gst_rtp_mpa_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpMPAPay *rtpmpapay;
  GstFlowReturn ret;
  guint size, avail;
  guint packet_len;
  GstClockTime duration, timestamp;

  rtpmpapay = GST_RTP_MPA_PAY (basepayload);

  size = gst_buffer_get_size (buffer);
  duration = GST_BUFFER_DURATION (buffer);
  timestamp = GST_BUFFER_PTS (buffer);

  if (GST_BUFFER_IS_DISCONT (buffer)) {
    GST_DEBUG_OBJECT (rtpmpapay, "DISCONT");
    gst_rtp_mpa_pay_reset (rtpmpapay);
  }

  avail = gst_adapter_available (rtpmpapay->adapter);

  /* get packet length of previous data and this new data,
   * payload length includes a 4 byte header */
  packet_len = gst_rtp_buffer_calc_packet_len (4 + avail + size, 0, 0);

  /* if this buffer is going to overflow the packet, flush what we
   * have. */
  if (gst_rtp_base_payload_is_filled (basepayload,
          packet_len, rtpmpapay->duration + duration)) {
    ret = gst_rtp_mpa_pay_flush (rtpmpapay);
    avail = 0;
  } else {
    ret = GST_FLOW_OK;
  }

  if (avail == 0) {
    GST_DEBUG_OBJECT (rtpmpapay,
        "first packet, save timestamp %" GST_TIME_FORMAT,
        GST_TIME_ARGS (timestamp));
    rtpmpapay->first_ts = timestamp;
    rtpmpapay->duration = 0;
  }

  gst_adapter_push (rtpmpapay->adapter, buffer);
  rtpmpapay->duration = duration;

  return ret;
}

static GstStateChangeReturn
gst_rtp_mpa_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpMPAPay *rtpmpapay;
  GstStateChangeReturn ret;

  rtpmpapay = GST_RTP_MPA_PAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_mpa_pay_reset (rtpmpapay);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_mpa_pay_reset (rtpmpapay);
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_mpa_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpmpapay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_MPA_PAY);
}
