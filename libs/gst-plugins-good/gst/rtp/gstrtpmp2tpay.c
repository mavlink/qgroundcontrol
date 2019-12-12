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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpmp2tpay.h"
#include "gstrtputils.h"

static GstStaticPadTemplate gst_rtp_mp2t_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpegts,"
        "packetsize=(int)188," "systemstream=(boolean)true")
    );

static GstStaticPadTemplate gst_rtp_mp2t_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_MP2T_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"MP2T\" ; "
        "application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"MP2T\"")
    );

static gboolean gst_rtp_mp2t_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_mp2t_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buffer);
static GstFlowReturn gst_rtp_mp2t_pay_flush (GstRTPMP2TPay * rtpmp2tpay);
static void gst_rtp_mp2t_pay_finalize (GObject * object);

#define gst_rtp_mp2t_pay_parent_class parent_class
G_DEFINE_TYPE (GstRTPMP2TPay, gst_rtp_mp2t_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_mp2t_pay_class_init (GstRTPMP2TPayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->finalize = gst_rtp_mp2t_pay_finalize;

  gstrtpbasepayload_class->set_caps = gst_rtp_mp2t_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_mp2t_pay_handle_buffer;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mp2t_pay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mp2t_pay_src_template);
  gst_element_class_set_static_metadata (gstelement_class,
      "RTP MPEG2 Transport Stream payloader", "Codec/Payloader/Network/RTP",
      "Payload-encodes MPEG2 TS into RTP packets (RFC 2250)",
      "Wim Taymans <wim.taymans@gmail.com>");
}

static void
gst_rtp_mp2t_pay_init (GstRTPMP2TPay * rtpmp2tpay)
{
  GST_RTP_BASE_PAYLOAD (rtpmp2tpay)->clock_rate = 90000;
  GST_RTP_BASE_PAYLOAD_PT (rtpmp2tpay) = GST_RTP_PAYLOAD_MP2T;

  rtpmp2tpay->adapter = gst_adapter_new ();
}

static void
gst_rtp_mp2t_pay_finalize (GObject * object)
{
  GstRTPMP2TPay *rtpmp2tpay;

  rtpmp2tpay = GST_RTP_MP2T_PAY (object);

  g_object_unref (rtpmp2tpay->adapter);
  rtpmp2tpay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_rtp_mp2t_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  gboolean res;

  gst_rtp_base_payload_set_options (payload, "video",
      payload->pt != GST_RTP_PAYLOAD_MP2T, "MP2T", 90000);
  res = gst_rtp_base_payload_set_outcaps (payload, NULL);

  return res;
}

static GstFlowReturn
gst_rtp_mp2t_pay_flush (GstRTPMP2TPay * rtpmp2tpay)
{
  guint avail, mtu;
  GstFlowReturn ret = GST_FLOW_OK;
  GstBuffer *outbuf;

  avail = gst_adapter_available (rtpmp2tpay->adapter);

  mtu = GST_RTP_BASE_PAYLOAD_MTU (rtpmp2tpay);

  while (avail > 0 && (ret == GST_FLOW_OK)) {
    guint towrite;
    guint payload_len;
    guint packet_len;
    GstBuffer *paybuf;

    /* this will be the total length of the packet */
    packet_len = gst_rtp_buffer_calc_packet_len (avail, 0, 0);

    /* fill one MTU or all available bytes */
    towrite = MIN (packet_len, mtu);

    /* this is the payload length */
    payload_len = gst_rtp_buffer_calc_payload_len (towrite, 0, 0);
    payload_len -= payload_len % 188;

    /* need whole packets */
    if (!payload_len)
      break;

    /* create buffer to hold the payload */
    outbuf = gst_rtp_buffer_new_allocate (0, 0, 0);

    /* get payload */
    paybuf = gst_adapter_take_buffer_fast (rtpmp2tpay->adapter, payload_len);
    gst_rtp_copy_meta (GST_ELEMENT_CAST (rtpmp2tpay), outbuf, paybuf, 0);
    outbuf = gst_buffer_append (outbuf, paybuf);
    avail -= payload_len;

    GST_BUFFER_PTS (outbuf) = rtpmp2tpay->first_ts;
    GST_BUFFER_DURATION (outbuf) = rtpmp2tpay->duration;

    GST_DEBUG_OBJECT (rtpmp2tpay, "pushing buffer of size %u",
        (guint) gst_buffer_get_size (outbuf));

    ret = gst_rtp_base_payload_push (GST_RTP_BASE_PAYLOAD (rtpmp2tpay), outbuf);
  }

  return ret;
}

static GstFlowReturn
gst_rtp_mp2t_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRTPMP2TPay *rtpmp2tpay;
  guint size, avail, packet_len;
  GstClockTime timestamp, duration;
  GstFlowReturn ret;

  rtpmp2tpay = GST_RTP_MP2T_PAY (basepayload);

  size = gst_buffer_get_size (buffer);
  timestamp = GST_BUFFER_PTS (buffer);
  duration = GST_BUFFER_DURATION (buffer);

again:
  ret = GST_FLOW_OK;
  avail = gst_adapter_available (rtpmp2tpay->adapter);

  /* Initialize new RTP payload */
  if (avail == 0) {
    rtpmp2tpay->first_ts = timestamp;
    rtpmp2tpay->duration = duration;
  }

  /* get packet length of previous data and this new data */
  packet_len = gst_rtp_buffer_calc_packet_len (avail + size, 0, 0);

  /* if this buffer is going to overflow the packet, flush what we have,
   * or if upstream is handing us several packets, to keep latency low */
  if (!size || gst_rtp_base_payload_is_filled (basepayload,
          packet_len, rtpmp2tpay->duration + duration)) {
    ret = gst_rtp_mp2t_pay_flush (rtpmp2tpay);
    rtpmp2tpay->first_ts = timestamp;
    rtpmp2tpay->duration = duration;

    /* keep filling the payload */
  } else {
    if (GST_CLOCK_TIME_IS_VALID (duration))
      rtpmp2tpay->duration += duration;
  }

  /* copy buffer to adapter */
  if (buffer) {
    gst_adapter_push (rtpmp2tpay->adapter, buffer);
    buffer = NULL;
  }

  if (size >= (188 * 2)) {
    size = 0;
    goto again;
  }

  return ret;

}

gboolean
gst_rtp_mp2t_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpmp2tpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_MP2T_PAY);
}
