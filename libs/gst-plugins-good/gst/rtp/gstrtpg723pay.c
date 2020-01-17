/* GStreamer
 * Copyright (C) <2007> Nokia Corporation
 * Copyright (C) <2007> Collabora Ltd
 *  @author: Olivier Crete <olivier.crete@collabora.co.uk>
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
#include <config.h>
#endif

#include <string.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/base/gstadapter.h>
#include <gst/audio/audio.h>

#include "gstrtpg723pay.h"
#include "gstrtputils.h"

#define G723_FRAME_DURATION (30 * GST_MSECOND)

static gboolean gst_rtp_g723_pay_set_caps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_g723_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buf);

static GstStaticPadTemplate gst_rtp_g723_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/G723, "     /* according to RFC 3551 */
        "channels = (int) 1, " "rate = (int) 8000")
    );

static GstStaticPadTemplate gst_rtp_g723_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_G723_STRING ", "
        "clock-rate = (int) 8000, "
        "encoding-name = (string) \"G723\"; "
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 8000, " "encoding-name = (string) \"G723\"")
    );

static void gst_rtp_g723_pay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_g723_pay_change_state (GstElement * element,
    GstStateChange transition);

#define gst_rtp_g723_pay_parent_class parent_class
G_DEFINE_TYPE (GstRTPG723Pay, gst_rtp_g723_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_g723_pay_class_init (GstRTPG723PayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *payload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  payload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->finalize = gst_rtp_g723_pay_finalize;

  gstelement_class->change_state = gst_rtp_g723_pay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g723_pay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g723_pay_src_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP G.723 payloader", "Codec/Payloader/Network/RTP",
      "Packetize G.723 audio into RTP packets",
      "Wim Taymans <wim.taymans@gmail.com>");

  payload_class->set_caps = gst_rtp_g723_pay_set_caps;
  payload_class->handle_buffer = gst_rtp_g723_pay_handle_buffer;
}

static void
gst_rtp_g723_pay_init (GstRTPG723Pay * pay)
{
  GstRTPBasePayload *payload = GST_RTP_BASE_PAYLOAD (pay);

  pay->adapter = gst_adapter_new ();

  payload->pt = GST_RTP_PAYLOAD_G723;
}

static void
gst_rtp_g723_pay_finalize (GObject * object)
{
  GstRTPG723Pay *pay;

  pay = GST_RTP_G723_PAY (object);

  g_object_unref (pay->adapter);
  pay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static gboolean
gst_rtp_g723_pay_set_caps (GstRTPBasePayload * payload, GstCaps * caps)
{
  gboolean res;

  gst_rtp_base_payload_set_options (payload, "audio",
      payload->pt != GST_RTP_PAYLOAD_G723, "G723", 8000);
  res = gst_rtp_base_payload_set_outcaps (payload, NULL);

  return res;
}

static GstFlowReturn
gst_rtp_g723_pay_flush (GstRTPG723Pay * pay)
{
  GstBuffer *outbuf, *payload_buf;
  GstFlowReturn ret;
  guint avail;
  GstRTPBuffer rtp = { NULL };

  avail = gst_adapter_available (pay->adapter);

  outbuf = gst_rtp_buffer_new_allocate (0, 0, 0);

  gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);

  GST_BUFFER_PTS (outbuf) = pay->timestamp;
  GST_BUFFER_DURATION (outbuf) = pay->duration;

  /* copy G723 data as payload */
  payload_buf = gst_adapter_take_buffer_fast (pay->adapter, avail);

  pay->timestamp = GST_CLOCK_TIME_NONE;
  pay->duration = 0;

  /* set discont and marker */
  if (pay->discont) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
    gst_rtp_buffer_set_marker (&rtp, TRUE);
    pay->discont = FALSE;
  }
  gst_rtp_buffer_unmap (&rtp);
  gst_rtp_copy_audio_meta (pay, outbuf, payload_buf);

  outbuf = gst_buffer_append (outbuf, payload_buf);

  ret = gst_rtp_base_payload_push (GST_RTP_BASE_PAYLOAD (pay), outbuf);

  return ret;
}

/* 00    high-rate speech (6.3 kb/s)            24
 * 01    low-rate speech  (5.3 kb/s)            20
 * 10    SID frame                               4
 * 11    reserved                                0  */
static const guint size_tab[4] = {
  24, 20, 4, 0
};

static GstFlowReturn
gst_rtp_g723_pay_handle_buffer (GstRTPBasePayload * payload, GstBuffer * buf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstMapInfo map;
  guint8 HDR;
  GstRTPG723Pay *pay;
  GstClockTime packet_dur, timestamp;
  guint payload_len, packet_len;

  pay = GST_RTP_G723_PAY (payload);

  gst_buffer_map (buf, &map, GST_MAP_READ);
  timestamp = GST_BUFFER_PTS (buf);

  if (GST_BUFFER_IS_DISCONT (buf)) {
    /* flush everything on discont */
    gst_adapter_clear (pay->adapter);
    pay->timestamp = GST_CLOCK_TIME_NONE;
    pay->duration = 0;
    pay->discont = TRUE;
  }

  /* should be one of these sizes */
  if (map.size != 4 && map.size != 20 && map.size != 24)
    goto invalid_size;

  /* check size by looking at the header bits */
  HDR = map.data[0] & 0x3;
  if (size_tab[HDR] != map.size)
    goto wrong_size;

  /* calculate packet size and duration */
  payload_len = gst_adapter_available (pay->adapter) + map.size;
  packet_dur = pay->duration + G723_FRAME_DURATION;
  packet_len = gst_rtp_buffer_calc_packet_len (payload_len, 0, 0);

  if (gst_rtp_base_payload_is_filled (payload, packet_len, packet_dur)) {
    /* size or duration would overflow the packet, flush the queued data */
    ret = gst_rtp_g723_pay_flush (pay);
  }

  /* update timestamp, we keep the timestamp for the first packet in the adapter
   * but are able to calculate it from next packets. */
  if (timestamp != GST_CLOCK_TIME_NONE && pay->timestamp == GST_CLOCK_TIME_NONE) {
    if (timestamp > pay->duration)
      pay->timestamp = timestamp - pay->duration;
    else
      pay->timestamp = 0;
  }
  gst_buffer_unmap (buf, &map);

  /* add packet to the queue */
  gst_adapter_push (pay->adapter, buf);
  pay->duration = packet_dur;

  /* check if we can flush now */
  if (pay->duration >= payload->min_ptime) {
    ret = gst_rtp_g723_pay_flush (pay);
  }

  return ret;

  /* WARNINGS */
invalid_size:
  {
    GST_ELEMENT_WARNING (pay, STREAM, WRONG_TYPE,
        ("Invalid input buffer size"),
        ("Input size should be 4, 20 or 24, got %" G_GSIZE_FORMAT, map.size));
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }
wrong_size:
  {
    GST_ELEMENT_WARNING (pay, STREAM, WRONG_TYPE,
        ("Wrong input buffer size"),
        ("Expected input buffer size %u but got %" G_GSIZE_FORMAT,
            size_tab[HDR], map.size));
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }
}

static GstStateChangeReturn
gst_rtp_g723_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstRTPG723Pay *pay;

  pay = GST_RTP_G723_PAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_adapter_clear (pay->adapter);
      pay->timestamp = GST_CLOCK_TIME_NONE;
      pay->duration = 0;
      pay->discont = TRUE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_adapter_clear (pay->adapter);
      break;
    default:
      break;
  }

  return ret;
}

/*Plugin init functions*/
gboolean
gst_rtp_g723_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpg723pay", GST_RANK_SECONDARY,
      gst_rtp_g723_pay_get_type ());
}
