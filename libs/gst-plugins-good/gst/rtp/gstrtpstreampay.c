/*
 * GStreamer
 * Copyright (C) 2013 Sebastian Dröge <sebastian@centricular.com>
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
 * SECTION:element-rtpstreampay
 * @title: rtpstreampay
 *
 * Implements stream payloading of RTP and RTCP packets for connection-oriented
 * transport protocols according to RFC4571.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc ! "audio/x-raw,rate=48000" ! vorbisenc ! rtpvorbispay config-interval=1 ! rtpstreampay ! tcpserversink port=5678
 * gst-launch-1.0 tcpclientsrc port=5678 host=127.0.0.1 do-timestamp=true ! "application/x-rtp-stream,media=audio,clock-rate=48000,encoding-name=VORBIS" ! rtpstreamdepay ! rtpvorbisdepay ! decodebin ! audioconvert ! audioresample ! autoaudiosink
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstrtpstreampay.h"

#define GST_CAT_DEFAULT gst_rtp_stream_pay_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp; application/x-rtcp; "
        "application/x-srtp; application/x-srtcp")
    );

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp-stream; application/x-rtcp-stream; "
        "application/x-srtp-stream; application/x-srtcp-stream")
    );

#define parent_class gst_rtp_stream_pay_parent_class
G_DEFINE_TYPE (GstRtpStreamPay, gst_rtp_stream_pay, GST_TYPE_ELEMENT);

static gboolean gst_rtp_stream_pay_sink_query (GstPad * pad, GstObject * parent,
    GstQuery * query);
static GstFlowReturn gst_rtp_stream_pay_sink_chain (GstPad * pad,
    GstObject * parent, GstBuffer * inbuf);
static gboolean gst_rtp_stream_pay_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

static void
gst_rtp_stream_pay_class_init (GstRtpStreamPayClass * klass)
{
  GstElementClass *gstelement_class;

  GST_DEBUG_CATEGORY_INIT (gst_rtp_stream_pay_debug, "rtpstreampay", 0,
      "RTP stream payloader");

  gstelement_class = (GstElementClass *) klass;

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Stream Payloading", "Codec/Payloader/Network",
      "Payloads RTP/RTCP packets for streaming protocols according to RFC4571",
      "Sebastian Dröge <sebastian@centricular.com>");

  gst_element_class_add_static_pad_template (gstelement_class, &src_template);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);
}

static void
gst_rtp_stream_pay_init (GstRtpStreamPay * self)
{
  self->sinkpad = gst_pad_new_from_static_template (&sink_template, "sink");
  gst_pad_set_chain_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_stream_pay_sink_chain));
  gst_pad_set_event_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_stream_pay_sink_event));
  gst_pad_set_query_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_stream_pay_sink_query));
  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

  self->srcpad = gst_pad_new_from_static_template (&src_template, "src");
  gst_pad_use_fixed_caps (self->srcpad);
  gst_element_add_pad (GST_ELEMENT (self), self->srcpad);
}

static GstCaps *
gst_rtp_stream_pay_sink_get_caps (GstRtpStreamPay * self, GstCaps * filter)
{
  GstCaps *peerfilter = NULL, *peercaps, *templ;
  GstCaps *res;
  GstStructure *structure;
  guint i, n;

  if (filter) {
    peerfilter = gst_caps_copy (filter);
    n = gst_caps_get_size (peerfilter);
    for (i = 0; i < n; i++) {
      structure = gst_caps_get_structure (peerfilter, i);

      if (gst_structure_has_name (structure, "application/x-rtp"))
        gst_structure_set_name (structure, "application/x-rtp-stream");
      else if (gst_structure_has_name (structure, "application/x-rtcp"))
        gst_structure_set_name (structure, "application/x-rtcp-stream");
      else if (gst_structure_has_name (structure, "application/x-srtp"))
        gst_structure_set_name (structure, "application/x-srtp-stream");
      else
        gst_structure_set_name (structure, "application/x-srtcp-stream");
    }
  }

  templ = gst_pad_get_pad_template_caps (self->sinkpad);
  peercaps = gst_pad_peer_query_caps (self->srcpad, peerfilter);

  if (peercaps) {
    /* Rename structure names */
    peercaps = gst_caps_make_writable (peercaps);
    n = gst_caps_get_size (peercaps);
    for (i = 0; i < n; i++) {
      structure = gst_caps_get_structure (peercaps, i);

      if (gst_structure_has_name (structure, "application/x-rtp-stream"))
        gst_structure_set_name (structure, "application/x-rtp");
      else if (gst_structure_has_name (structure, "application/x-rtcp-stream"))
        gst_structure_set_name (structure, "application/x-rtcp");
      else if (gst_structure_has_name (structure, "application/x-srtp-stream"))
        gst_structure_set_name (structure, "application/x-srtp");
      else
        gst_structure_set_name (structure, "application/x-srtcp");
    }

    res = gst_caps_intersect_full (peercaps, templ, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (peercaps);
  } else {
    res = templ;
  }

  if (filter) {
    GstCaps *intersection;

    intersection =
        gst_caps_intersect_full (filter, res, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (res);
    res = intersection;

    gst_caps_unref (peerfilter);
  }

  return res;
}

static gboolean
gst_rtp_stream_pay_sink_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstRtpStreamPay *self = GST_RTP_STREAM_PAY (parent);
  gboolean ret;

  GST_LOG_OBJECT (pad, "Handling query of type '%s'",
      gst_query_type_get_name (GST_QUERY_TYPE (query)));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *caps;

      gst_query_parse_caps (query, &caps);
      caps = gst_rtp_stream_pay_sink_get_caps (self, caps);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      ret = TRUE;
      break;
    }
    default:
      ret = gst_pad_query_default (pad, parent, query);
  }

  return ret;
}

static gboolean
gst_rtp_stream_pay_sink_set_caps (GstRtpStreamPay * self, GstCaps * caps)
{
  GstCaps *othercaps;
  GstStructure *structure;
  gboolean ret;

  othercaps = gst_caps_copy (caps);
  structure = gst_caps_get_structure (othercaps, 0);

  if (gst_structure_has_name (structure, "application/x-rtp"))
    gst_structure_set_name (structure, "application/x-rtp-stream");
  else if (gst_structure_has_name (structure, "application/x-rtcp"))
    gst_structure_set_name (structure, "application/x-rtcp-stream");
  else if (gst_structure_has_name (structure, "application/x-srtp"))
    gst_structure_set_name (structure, "application/x-srtp-stream");
  else
    gst_structure_set_name (structure, "application/x-srtcp-stream");

  ret = gst_pad_set_caps (self->srcpad, othercaps);
  gst_caps_unref (othercaps);

  return ret;
}

static gboolean
gst_rtp_stream_pay_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRtpStreamPay *self = GST_RTP_STREAM_PAY (parent);
  gboolean ret;

  GST_LOG_OBJECT (pad, "Got %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      ret = gst_rtp_stream_pay_sink_set_caps (self, caps);
      gst_event_unref (event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

static GstFlowReturn
gst_rtp_stream_pay_sink_chain (GstPad * pad, GstObject * parent,
    GstBuffer * inbuf)
{
  GstRtpStreamPay *self = GST_RTP_STREAM_PAY (parent);
  GstBuffer *outbuf;
  gsize size;
  guint8 size16[2];

  size = gst_buffer_get_size (inbuf);
  if (size > G_MAXUINT16) {
    GST_ELEMENT_ERROR (self, CORE, FAILED, (NULL),
        ("Only buffers up to %d bytes supported, got %" G_GSIZE_FORMAT,
            G_MAXUINT16, size));
    gst_buffer_unref (inbuf);
    return GST_FLOW_ERROR;
  }

  outbuf = gst_buffer_new_and_alloc (2);

  GST_WRITE_UINT16_BE (size16, size);
  gst_buffer_fill (outbuf, 0, size16, 2);

  gst_buffer_copy_into (outbuf, inbuf, GST_BUFFER_COPY_ALL, 0, -1);

  gst_buffer_unref (inbuf);

  return gst_pad_push (self->srcpad, outbuf);
}

gboolean
gst_rtp_stream_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpstreampay",
      GST_RANK_NONE, GST_TYPE_RTP_STREAM_PAY);
}
