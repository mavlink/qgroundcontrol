/* GStreamer
 * Copyright (C) <2009> Wim Taymans <wim.taymans@gmail.com>
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
#include <gst/audio/audio.h>

#include "gstrtpceltpay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpceltpay_debug);
#define GST_CAT_DEFAULT (rtpceltpay_debug)

static GstStaticPadTemplate gst_rtp_celt_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-celt, "
        "rate = (int) [ 32000, 64000 ], "
        "channels = (int) [1, 2], " "frame-size = (int) [ 64, 512 ]")
    );

static GstStaticPadTemplate gst_rtp_celt_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate =  (int) [ 32000, 48000 ], "
        "encoding-name = (string) \"CELT\"")
    );

static void gst_rtp_celt_pay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_celt_pay_change_state (GstElement *
    element, GstStateChange transition);

static gboolean gst_rtp_celt_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstCaps *gst_rtp_celt_pay_getcaps (GstRTPBasePayload * payload,
    GstPad * pad, GstCaps * filter);
static GstFlowReturn gst_rtp_celt_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buffer);

#define gst_rtp_celt_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpCELTPay, gst_rtp_celt_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_celt_pay_class_init (GstRtpCELTPayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpceltpay_debug, "rtpceltpay", 0,
      "CELT RTP Payloader");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->finalize = gst_rtp_celt_pay_finalize;

  gstelement_class->change_state = gst_rtp_celt_pay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_celt_pay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_celt_pay_src_template);

  gst_element_class_set_static_metadata (gstelement_class, "RTP CELT payloader",
      "Codec/Payloader/Network/RTP",
      "Payload-encodes CELT audio into a RTP packet",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasepayload_class->set_caps = gst_rtp_celt_pay_setcaps;
  gstrtpbasepayload_class->get_caps = gst_rtp_celt_pay_getcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_celt_pay_handle_buffer;
}

static void
gst_rtp_celt_pay_init (GstRtpCELTPay * rtpceltpay)
{
  rtpceltpay->queue = g_queue_new ();
}

static void
gst_rtp_celt_pay_finalize (GObject * object)
{
  GstRtpCELTPay *rtpceltpay;

  rtpceltpay = GST_RTP_CELT_PAY (object);

  g_queue_free (rtpceltpay->queue);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_celt_pay_clear_queued (GstRtpCELTPay * rtpceltpay)
{
  GstBuffer *buf;

  while ((buf = g_queue_pop_head (rtpceltpay->queue)))
    gst_buffer_unref (buf);

  rtpceltpay->bytes = 0;
  rtpceltpay->sbytes = 0;
  rtpceltpay->qduration = 0;
}

static void
gst_rtp_celt_pay_add_queued (GstRtpCELTPay * rtpceltpay, GstBuffer * buffer,
    guint ssize, guint size, GstClockTime duration)
{
  g_queue_push_tail (rtpceltpay->queue, buffer);
  rtpceltpay->sbytes += ssize;
  rtpceltpay->bytes += size;
  /* only add durations when we have a valid previous duration */
  if (rtpceltpay->qduration != -1) {
    if (duration != -1)
      /* only add valid durations */
      rtpceltpay->qduration += duration;
    else
      /* if we add a buffer without valid duration, our total queued duration
       * becomes unknown */
      rtpceltpay->qduration = -1;
  }
}

static gboolean
gst_rtp_celt_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  /* don't configure yet, we wait for the ident packet */
  return TRUE;
}


static GstCaps *
gst_rtp_celt_pay_getcaps (GstRTPBasePayload * payload, GstPad * pad,
    GstCaps * filter)
{
  GstCaps *otherpadcaps;
  GstCaps *caps;
  const gchar *params;

  caps = gst_pad_get_pad_template_caps (pad);

  otherpadcaps = gst_pad_get_allowed_caps (payload->srcpad);
  if (otherpadcaps) {
    if (!gst_caps_is_empty (otherpadcaps)) {
      GstStructure *ps;
      GstStructure *s;
      gint clock_rate = 0, frame_size = 0, channels = 1;

      caps = gst_caps_make_writable (caps);

      ps = gst_caps_get_structure (otherpadcaps, 0);
      s = gst_caps_get_structure (caps, 0);

      if (gst_structure_get_int (ps, "clock-rate", &clock_rate)) {
        gst_structure_fixate_field_nearest_int (s, "rate", clock_rate);
      }

      if ((params = gst_structure_get_string (ps, "frame-size")))
        frame_size = atoi (params);
      if (frame_size)
        gst_structure_set (s, "frame-size", G_TYPE_INT, frame_size, NULL);

      if ((params = gst_structure_get_string (ps, "encoding-params"))) {
        channels = atoi (params);
        gst_structure_fixate_field_nearest_int (s, "channels", channels);
      }

      GST_DEBUG_OBJECT (payload, "clock-rate=%d frame-size=%d channels=%d",
          clock_rate, frame_size, channels);
    }
    gst_caps_unref (otherpadcaps);
  }

  if (filter) {
    GstCaps *tmp;

    GST_DEBUG_OBJECT (payload, "Intersect %" GST_PTR_FORMAT " and filter %"
        GST_PTR_FORMAT, caps, filter);
    tmp = gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    caps = tmp;
  }

  return caps;
}

static gboolean
gst_rtp_celt_pay_parse_ident (GstRtpCELTPay * rtpceltpay,
    const guint8 * data, guint size)
{
  guint32 version, header_size, rate, nb_channels, frame_size, overlap;
  guint32 bytes_per_packet;
  GstRTPBasePayload *payload;
  gchar *cstr, *fsstr;
  gboolean res;

  /* we need the header string (8), the version string (20), the version
   * and the header length. */
  if (size < 36)
    goto too_small;

  if (!g_str_has_prefix ((const gchar *) data, "CELT    "))
    goto wrong_header;

  /* skip header and version string */
  data += 28;

  version = GST_READ_UINT32_LE (data);
  GST_DEBUG_OBJECT (rtpceltpay, "version %08x", version);
#if 0
  if (version != 1)
    goto wrong_version;
#endif

  data += 4;
  /* ensure sizes */
  header_size = GST_READ_UINT32_LE (data);
  if (header_size < 56)
    goto header_too_small;

  if (size < header_size)
    goto payload_too_small;

  data += 4;
  rate = GST_READ_UINT32_LE (data);
  data += 4;
  nb_channels = GST_READ_UINT32_LE (data);
  data += 4;
  frame_size = GST_READ_UINT32_LE (data);
  data += 4;
  overlap = GST_READ_UINT32_LE (data);
  data += 4;
  bytes_per_packet = GST_READ_UINT32_LE (data);

  GST_DEBUG_OBJECT (rtpceltpay, "rate %d, nb_channels %d, frame_size %d",
      rate, nb_channels, frame_size);
  GST_DEBUG_OBJECT (rtpceltpay, "overlap %d, bytes_per_packet %d",
      overlap, bytes_per_packet);

  payload = GST_RTP_BASE_PAYLOAD (rtpceltpay);

  gst_rtp_base_payload_set_options (payload, "audio", FALSE, "CELT", rate);
  cstr = g_strdup_printf ("%d", nb_channels);
  fsstr = g_strdup_printf ("%d", frame_size);
  res = gst_rtp_base_payload_set_outcaps (payload, "encoding-params",
      G_TYPE_STRING, cstr, "frame-size", G_TYPE_STRING, fsstr, NULL);
  g_free (cstr);
  g_free (fsstr);

  return res;

  /* ERRORS */
too_small:
  {
    GST_DEBUG_OBJECT (rtpceltpay,
        "ident packet too small, need at least 32 bytes");
    return FALSE;
  }
wrong_header:
  {
    GST_DEBUG_OBJECT (rtpceltpay,
        "ident packet does not start with \"CELT    \"");
    return FALSE;
  }
#if 0
wrong_version:
  {
    GST_DEBUG_OBJECT (rtpceltpay, "can only handle version 1, have version %d",
        version);
    return FALSE;
  }
#endif
header_too_small:
  {
    GST_DEBUG_OBJECT (rtpceltpay,
        "header size too small, need at least 80 bytes, " "got only %d",
        header_size);
    return FALSE;
  }
payload_too_small:
  {
    GST_DEBUG_OBJECT (rtpceltpay,
        "payload too small, need at least %d bytes, got only %d", header_size,
        size);
    return FALSE;
  }
}

static GstFlowReturn
gst_rtp_celt_pay_flush_queued (GstRtpCELTPay * rtpceltpay)
{
  GstFlowReturn ret;
  GstBuffer *buf, *outbuf;
  guint8 *payload, *spayload;
  guint payload_len;
  GstClockTime duration;
  GstRTPBuffer rtp = { NULL, };

  payload_len = rtpceltpay->bytes + rtpceltpay->sbytes;
  duration = rtpceltpay->qduration;

  GST_DEBUG_OBJECT (rtpceltpay, "flushing out %u, duration %" GST_TIME_FORMAT,
      payload_len, GST_TIME_ARGS (rtpceltpay->qduration));

  /* get a big enough packet for the sizes + payloads */
  outbuf = gst_rtp_buffer_new_allocate (payload_len, 0, 0);

  GST_BUFFER_DURATION (outbuf) = duration;

  gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);

  /* point to the payload for size headers and data */
  spayload = gst_rtp_buffer_get_payload (&rtp);
  payload = spayload + rtpceltpay->sbytes;

  while ((buf = g_queue_pop_head (rtpceltpay->queue))) {
    guint size;

    /* copy first timestamp to output */
    if (GST_BUFFER_PTS (outbuf) == -1)
      GST_BUFFER_PTS (outbuf) = GST_BUFFER_PTS (buf);

    /* write the size to the header */
    size = gst_buffer_get_size (buf);
    while (size > 0xff) {
      *spayload++ = 0xff;
      size -= 0xff;
    }
    *spayload++ = size;

    /* copy payload */
    size = gst_buffer_get_size (buf);
    gst_buffer_extract (buf, 0, payload, size);
    payload += size;

    gst_rtp_copy_audio_meta (rtpceltpay, outbuf, buf);

    gst_buffer_unref (buf);
  }
  gst_rtp_buffer_unmap (&rtp);

  /* we consumed it all */
  rtpceltpay->bytes = 0;
  rtpceltpay->sbytes = 0;
  rtpceltpay->qduration = 0;

  ret = gst_rtp_base_payload_push (GST_RTP_BASE_PAYLOAD (rtpceltpay), outbuf);

  return ret;
}

static GstFlowReturn
gst_rtp_celt_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstFlowReturn ret;
  GstRtpCELTPay *rtpceltpay;
  gsize payload_len;
  GstMapInfo map;
  GstClockTime duration, packet_dur;
  guint i, ssize, packet_len;

  rtpceltpay = GST_RTP_CELT_PAY (basepayload);

  ret = GST_FLOW_OK;

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  switch (rtpceltpay->packet) {
    case 0:
      /* ident packet. We need to parse the headers to construct the RTP
       * properties. */
      if (!gst_rtp_celt_pay_parse_ident (rtpceltpay, map.data, map.size))
        goto parse_error;

      goto cleanup;
    case 1:
      /* comment packet, we ignore it */
      goto cleanup;
    default:
      /* other packets go in the payload */
      break;
  }
  gst_buffer_unmap (buffer, &map);

  duration = GST_BUFFER_DURATION (buffer);

  GST_LOG_OBJECT (rtpceltpay,
      "got buffer of duration %" GST_TIME_FORMAT ", size %" G_GSIZE_FORMAT,
      GST_TIME_ARGS (duration), map.size);

  /* calculate the size of the size field and the payload */
  ssize = 1;
  for (i = map.size; i > 0xff; i -= 0xff)
    ssize++;

  GST_DEBUG_OBJECT (rtpceltpay, "bytes for size %u", ssize);

  /* calculate what the new size and duration would be of the packet */
  payload_len = ssize + map.size + rtpceltpay->bytes + rtpceltpay->sbytes;
  if (rtpceltpay->qduration != -1 && duration != -1)
    packet_dur = rtpceltpay->qduration + duration;
  else
    packet_dur = 0;

  packet_len = gst_rtp_buffer_calc_packet_len (payload_len, 0, 0);

  if (gst_rtp_base_payload_is_filled (basepayload, packet_len, packet_dur)) {
    /* size or duration would overflow the packet, flush the queued data */
    ret = gst_rtp_celt_pay_flush_queued (rtpceltpay);
  }

  /* queue the packet */
  gst_rtp_celt_pay_add_queued (rtpceltpay, buffer, ssize, map.size, duration);

done:
  rtpceltpay->packet++;

  return ret;

  /* ERRORS */
cleanup:
  {
    gst_buffer_unmap (buffer, &map);
    goto done;
  }
parse_error:
  {
    GST_ELEMENT_ERROR (rtpceltpay, STREAM, DECODE, (NULL),
        ("Error parsing first identification packet."));
    gst_buffer_unmap (buffer, &map);
    return GST_FLOW_ERROR;
  }
}

static GstStateChangeReturn
gst_rtp_celt_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpCELTPay *rtpceltpay;
  GstStateChangeReturn ret;

  rtpceltpay = GST_RTP_CELT_PAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      rtpceltpay->packet = 0;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_celt_pay_clear_queued (rtpceltpay);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_celt_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpceltpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_CELT_PAY);
}
