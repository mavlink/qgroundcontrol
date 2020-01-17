/* GStreamer
 * Copyright (C) <2007> Thijs Vermeir <thijsvermeir@gmail.com>
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
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>

#include "gstrtpmpvpay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpmpvpay_debug);
#define GST_CAT_DEFAULT (rtpmpvpay_debug)

static GstStaticPadTemplate gst_rtp_mpv_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg, "
        "mpegversion = (int) 2, systemstream = (boolean) FALSE")
    );

static GstStaticPadTemplate gst_rtp_mpv_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_MPV_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"MPV\"; "
        "application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"MPV\"")
    );

static GstStateChangeReturn gst_rtp_mpv_pay_change_state (GstElement * element,
    GstStateChange transition);

static void gst_rtp_mpv_pay_finalize (GObject * object);

static GstFlowReturn gst_rtp_mpv_pay_flush (GstRTPMPVPay * rtpmpvpay);
static gboolean gst_rtp_mpv_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_mpv_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buffer);
static gboolean gst_rtp_mpv_pay_sink_event (GstRTPBasePayload * payload,
    GstEvent * event);

#define gst_rtp_mpv_pay_parent_class parent_class
G_DEFINE_TYPE (GstRTPMPVPay, gst_rtp_mpv_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_mpv_pay_class_init (GstRTPMPVPayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->finalize = gst_rtp_mpv_pay_finalize;

  gstelement_class->change_state = gst_rtp_mpv_pay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mpv_pay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mpv_pay_src_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP MPEG2 ES video payloader", "Codec/Payloader/Network/RTP",
      "Payload-encodes MPEG2 ES into RTP packets (RFC 2250)",
      "Thijs Vermeir <thijsvermeir@gmail.com>");

  gstrtpbasepayload_class->set_caps = gst_rtp_mpv_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_mpv_pay_handle_buffer;
  gstrtpbasepayload_class->sink_event = gst_rtp_mpv_pay_sink_event;

  GST_DEBUG_CATEGORY_INIT (rtpmpvpay_debug, "rtpmpvpay", 0,
      "MPEG2 ES Video RTP Payloader");
}

static void
gst_rtp_mpv_pay_init (GstRTPMPVPay * rtpmpvpay)
{
  GST_RTP_BASE_PAYLOAD (rtpmpvpay)->clock_rate = 90000;
  GST_RTP_BASE_PAYLOAD_PT (rtpmpvpay) = GST_RTP_PAYLOAD_MPV;

  rtpmpvpay->adapter = gst_adapter_new ();
}

static void
gst_rtp_mpv_pay_finalize (GObject * object)
{
  GstRTPMPVPay *rtpmpvpay;

  rtpmpvpay = GST_RTP_MPV_PAY (object);

  g_object_unref (rtpmpvpay->adapter);
  rtpmpvpay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_mpv_pay_reset (GstRTPMPVPay * pay)
{
  pay->first_ts = -1;
  pay->duration = 0;
  gst_adapter_clear (pay->adapter);
  GST_DEBUG_OBJECT (pay, "reset depayloader");
}

static gboolean
gst_rtp_mpv_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  gst_rtp_base_payload_set_options (payload, "video",
      payload->pt != GST_RTP_PAYLOAD_MPV, "MPV", 90000);
  return gst_rtp_base_payload_set_outcaps (payload, NULL);
}

static gboolean
gst_rtp_mpv_pay_sink_event (GstRTPBasePayload * payload, GstEvent * event)
{
  gboolean ret;
  GstRTPMPVPay *rtpmpvpay;

  rtpmpvpay = GST_RTP_MPV_PAY (payload);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      /* make sure we push the last packets in the adapter on EOS */
      gst_rtp_mpv_pay_flush (rtpmpvpay);
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_mpv_pay_reset (rtpmpvpay);
      break;
    default:
      break;
  }

  ret = GST_RTP_BASE_PAYLOAD_CLASS (parent_class)->sink_event (payload, event);

  return ret;
}

#define RTP_HEADER_LEN 12

static GstFlowReturn
gst_rtp_mpv_pay_flush (GstRTPMPVPay * rtpmpvpay)
{
  GstFlowReturn ret;
  guint avail;
  GstBufferList *list;
  GstBuffer *outbuf;

  guint8 *payload;

  avail = gst_adapter_available (rtpmpvpay->adapter);

  ret = GST_FLOW_OK;

  GST_DEBUG_OBJECT (rtpmpvpay, "available %u", avail);
  if (avail == 0)
    return GST_FLOW_OK;

  list =
      gst_buffer_list_new_sized (avail / (GST_RTP_BASE_PAYLOAD_MTU (rtpmpvpay) -
          RTP_HEADER_LEN) + 1);

  while (avail > 0) {
    guint towrite;
    guint packet_len;
    guint payload_len;
    GstRTPBuffer rtp = { NULL };
    GstBuffer *paybuf;

    packet_len = gst_rtp_buffer_calc_packet_len (avail + 4, 0, 0);

    towrite = MIN (packet_len, GST_RTP_BASE_PAYLOAD_MTU (rtpmpvpay));

    payload_len = gst_rtp_buffer_calc_payload_len (towrite, 0, 0);

    outbuf = gst_rtp_buffer_new_allocate (4, 0, 0);

    payload_len -= 4;

    gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);

    payload = gst_rtp_buffer_get_payload (&rtp);
    /* enable MPEG Video-specific header
     *
     *  0                   1                   2                   3
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |    MBZ  |T|         TR        | |N|S|B|E|  P  | | BFC | | FFC |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *                                  AN              FBV     FFV
     */

    /* fill in the MPEG Video-specific header 
     * data is set to 0x0 here
     */
    memset (payload, 0x0, 4);

    avail -= payload_len;

    gst_rtp_buffer_set_marker (&rtp, avail == 0);
    gst_rtp_buffer_unmap (&rtp);

    paybuf = gst_adapter_take_buffer_fast (rtpmpvpay->adapter, payload_len);
    gst_rtp_copy_video_meta (rtpmpvpay, outbuf, paybuf);
    outbuf = gst_buffer_append (outbuf, paybuf);

    GST_DEBUG_OBJECT (rtpmpvpay, "Adding buffer");

    GST_BUFFER_PTS (outbuf) = rtpmpvpay->first_ts;
    gst_buffer_list_add (list, outbuf);
  }

  ret = gst_rtp_base_payload_push_list (GST_RTP_BASE_PAYLOAD (rtpmpvpay), list);

  return ret;
}

static GstFlowReturn
gst_rtp_mpv_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRTPMPVPay *rtpmpvpay;
  guint avail, packet_len;
  GstClockTime timestamp, duration;
  GstFlowReturn ret = GST_FLOW_OK;

  rtpmpvpay = GST_RTP_MPV_PAY (basepayload);

  timestamp = GST_BUFFER_PTS (buffer);
  duration = GST_BUFFER_DURATION (buffer);

  if (GST_BUFFER_IS_DISCONT (buffer)) {
    GST_DEBUG_OBJECT (rtpmpvpay, "DISCONT");
    gst_rtp_mpv_pay_reset (rtpmpvpay);
  }

  avail = gst_adapter_available (rtpmpvpay->adapter);

  if (duration == -1)
    duration = 0;

  if (rtpmpvpay->first_ts == GST_CLOCK_TIME_NONE || avail == 0)
    rtpmpvpay->first_ts = timestamp;

  if (avail == 0) {
    rtpmpvpay->duration = duration;
  } else {
    rtpmpvpay->duration += duration;
  }

  gst_adapter_push (rtpmpvpay->adapter, buffer);
  avail = gst_adapter_available (rtpmpvpay->adapter);

  /* get packet length of previous data and this new data,
   * payload length includes a 4 byte MPEG video-specific header */
  packet_len = gst_rtp_buffer_calc_packet_len (avail, 4, 0);
  GST_LOG_OBJECT (rtpmpvpay, "available %d, rtp packet length %d", avail,
      packet_len);

  if (gst_rtp_base_payload_is_filled (basepayload,
          packet_len, rtpmpvpay->duration)) {
    ret = gst_rtp_mpv_pay_flush (rtpmpvpay);
  } else {
    rtpmpvpay->first_ts = timestamp;
  }

  return ret;
}

static GstStateChangeReturn
gst_rtp_mpv_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstRTPMPVPay *rtpmpvpay;
  GstStateChangeReturn ret;

  rtpmpvpay = GST_RTP_MPV_PAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_mpv_pay_reset (rtpmpvpay);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_mpv_pay_reset (rtpmpvpay);
      break;
    default:
      break;
  }
  return ret;
}


gboolean
gst_rtp_mpv_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpmpvpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_MPV_PAY);
}
