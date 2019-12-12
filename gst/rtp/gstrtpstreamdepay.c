/* GStreamer
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
 * SECTION:element-rtpstreamdepay
 * @title: rtpstreamdepay
 *
 * Implements stream depayloading of RTP and RTCP packets for connection-oriented
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

#include "gstrtpstreamdepay.h"

GST_DEBUG_CATEGORY (gst_rtp_stream_depay_debug);
#define GST_CAT_DEFAULT gst_rtp_stream_depay_debug

static GstStaticPadTemplate src_template =
    GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp; application/x-rtcp;"
        "application/x-srtp; application/x-srtcp")
    );

static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp-stream; application/x-rtcp-stream;"
        "application/x-srtp-stream; application/x-srtcp-stream")
    );

#define parent_class gst_rtp_stream_depay_parent_class
G_DEFINE_TYPE (GstRtpStreamDepay, gst_rtp_stream_depay, GST_TYPE_BASE_PARSE);

static gboolean gst_rtp_stream_depay_set_sink_caps (GstBaseParse * parse,
    GstCaps * caps);
static GstCaps *gst_rtp_stream_depay_get_sink_caps (GstBaseParse * parse,
    GstCaps * filter);
static GstFlowReturn gst_rtp_stream_depay_handle_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, gint * skipsize);

static gboolean gst_rtp_stream_depay_sink_activate (GstPad * pad,
    GstObject * parent);

static void
gst_rtp_stream_depay_class_init (GstRtpStreamDepayClass * klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstBaseParseClass *parse_class = GST_BASE_PARSE_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_rtp_stream_depay_debug, "rtpstreamdepay", 0,
      "RTP stream depayloader");

  gst_element_class_add_static_pad_template (gstelement_class, &src_template);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Stream Depayloading", "Codec/Depayloader/Network",
      "Depayloads RTP/RTCP packets for streaming protocols according to RFC4571",
      "Sebastian Dröge <sebastian@centricular.com>");

  parse_class->set_sink_caps =
      GST_DEBUG_FUNCPTR (gst_rtp_stream_depay_set_sink_caps);
  parse_class->get_sink_caps =
      GST_DEBUG_FUNCPTR (gst_rtp_stream_depay_get_sink_caps);
  parse_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_rtp_stream_depay_handle_frame);
}

static void
gst_rtp_stream_depay_init (GstRtpStreamDepay * self)
{
  gst_base_parse_set_min_frame_size (GST_BASE_PARSE (self), 2);

  /* Force activation in push mode. We need to get a caps event from upstream
   * to know the full RTP caps. */
  gst_pad_set_activate_function (GST_BASE_PARSE_SINK_PAD (self),
      gst_rtp_stream_depay_sink_activate);
}

static gboolean
gst_rtp_stream_depay_set_sink_caps (GstBaseParse * parse, GstCaps * caps)
{
  GstCaps *othercaps;
  GstStructure *structure;
  gboolean ret;

  othercaps = gst_caps_copy (caps);
  structure = gst_caps_get_structure (othercaps, 0);

  if (gst_structure_has_name (structure, "application/x-rtp-stream"))
    gst_structure_set_name (structure, "application/x-rtp");
  else if (gst_structure_has_name (structure, "application/x-rtcp-stream"))
    gst_structure_set_name (structure, "application/x-rtcp");
  else if (gst_structure_has_name (structure, "application/x-srtp-stream"))
    gst_structure_set_name (structure, "application/x-srtp");
  else
    gst_structure_set_name (structure, "application/x-srtcp");

  ret = gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (parse), othercaps);
  gst_caps_unref (othercaps);

  return ret;
}

static GstCaps *
gst_rtp_stream_depay_get_sink_caps (GstBaseParse * parse, GstCaps * filter)
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

      if (gst_structure_has_name (structure, "application/x-rtp-stream"))
        gst_structure_set_name (structure, "application/x-rtp");
      else if (gst_structure_has_name (structure, "application/x-rtcp-stream"))
        gst_structure_set_name (structure, "application/x-rtcp");
      else if (gst_structure_has_name (structure, "application/x-srtp-stream"))
        gst_structure_set_name (structure, "application/x-srtp");
      else
        gst_structure_set_name (structure, "application/x-srtcp");
    }
  }

  templ = gst_pad_get_pad_template_caps (GST_BASE_PARSE_SINK_PAD (parse));
  peercaps =
      gst_pad_peer_query_caps (GST_BASE_PARSE_SRC_PAD (parse), peerfilter);

  if (peercaps) {
    /* Rename structure names */
    peercaps = gst_caps_make_writable (peercaps);
    n = gst_caps_get_size (peercaps);
    for (i = 0; i < n; i++) {
      structure = gst_caps_get_structure (peercaps, i);

      if (gst_structure_has_name (structure, "application/x-rtp"))
        gst_structure_set_name (structure, "application/x-rtp-stream");
      else if (gst_structure_has_name (structure, "application/x-rtcp"))
        gst_structure_set_name (structure, "application/x-rtcp-stream");
      else if (gst_structure_has_name (structure, "application/x-srtp"))
        gst_structure_set_name (structure, "application/x-srtp-stream");
      else
        gst_structure_set_name (structure, "application/x-srtcp-stream");
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

static GstFlowReturn
gst_rtp_stream_depay_handle_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, gint * skipsize)
{
  gsize buf_size;
  guint16 size;

  if (gst_buffer_extract (frame->buffer, 0, &size, 2) != 2)
    return GST_FLOW_ERROR;

  size = GUINT16_FROM_BE (size);
  buf_size = gst_buffer_get_size (frame->buffer);

  /* Need more data */
  if (size + 2 > buf_size)
    return GST_FLOW_OK;

  frame->out_buffer =
      gst_buffer_copy_region (frame->buffer, GST_BUFFER_COPY_ALL, 2, size);

  return gst_base_parse_finish_frame (parse, frame, size + 2);
}

static gboolean
gst_rtp_stream_depay_sink_activate (GstPad * pad, GstObject * parent)
{
  return gst_pad_activate_mode (pad, GST_PAD_MODE_PUSH, TRUE);
}

gboolean
gst_rtp_stream_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpstreamdepay",
      GST_RANK_NONE, GST_TYPE_RTP_STREAM_DEPAY);
}
