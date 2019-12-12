/* GStreamer
 * Copyright (C) <2005> Edgard Lima <edgard.lima@gmail.com>
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

#include "gstrtpspeexpay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpspeexpay_debug);
#define GST_CAT_DEFAULT (rtpspeexpay_debug)

static GstStaticPadTemplate gst_rtp_speex_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-speex, "
        "rate = (int) [ 6000, 48000 ], " "channels = (int) 1")
    );

static GstStaticPadTemplate gst_rtp_speex_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate =  (int) [ 6000, 48000 ], "
        "encoding-name = (string) \"SPEEX\", "
        "encoding-params = (string) \"1\"")
    );

static GstStateChangeReturn gst_rtp_speex_pay_change_state (GstElement *
    element, GstStateChange transition);

static gboolean gst_rtp_speex_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstCaps *gst_rtp_speex_pay_getcaps (GstRTPBasePayload * payload,
    GstPad * pad, GstCaps * filter);
static GstFlowReturn gst_rtp_speex_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buffer);

#define gst_rtp_speex_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpSPEEXPay, gst_rtp_speex_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_speex_pay_class_init (GstRtpSPEEXPayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gstelement_class->change_state = gst_rtp_speex_pay_change_state;

  gstrtpbasepayload_class->set_caps = gst_rtp_speex_pay_setcaps;
  gstrtpbasepayload_class->get_caps = gst_rtp_speex_pay_getcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_speex_pay_handle_buffer;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_speex_pay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_speex_pay_src_template);
  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Speex payloader", "Codec/Payloader/Network/RTP",
      "Payload-encodes Speex audio into a RTP packet",
      "Edgard Lima <edgard.lima@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (rtpspeexpay_debug, "rtpspeexpay", 0,
      "Speex RTP Payloader");
}

static void
gst_rtp_speex_pay_init (GstRtpSPEEXPay * rtpspeexpay)
{
  GST_RTP_BASE_PAYLOAD (rtpspeexpay)->clock_rate = 8000;
  GST_RTP_BASE_PAYLOAD_PT (rtpspeexpay) = 110;  /* Create String */
}

static gboolean
gst_rtp_speex_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  /* don't configure yet, we wait for the ident packet */
  return TRUE;
}


static GstCaps *
gst_rtp_speex_pay_getcaps (GstRTPBasePayload * payload, GstPad * pad,
    GstCaps * filter)
{
  GstCaps *otherpadcaps;
  GstCaps *caps;

  otherpadcaps = gst_pad_get_allowed_caps (payload->srcpad);
  caps = gst_pad_get_pad_template_caps (pad);

  if (otherpadcaps) {
    if (!gst_caps_is_empty (otherpadcaps)) {
      GstStructure *ps;
      GstStructure *s;
      gint clock_rate;

      ps = gst_caps_get_structure (otherpadcaps, 0);
      caps = gst_caps_make_writable (caps);
      s = gst_caps_get_structure (caps, 0);

      if (gst_structure_get_int (ps, "clock-rate", &clock_rate)) {
        gst_structure_fixate_field_nearest_int (s, "rate", clock_rate);
      }
    }
    gst_caps_unref (otherpadcaps);
  }

  if (filter) {
    GstCaps *tcaps = caps;

    caps = gst_caps_intersect_full (filter, tcaps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (tcaps);
  }

  return caps;
}

static gboolean
gst_rtp_speex_pay_parse_ident (GstRtpSPEEXPay * rtpspeexpay,
    const guint8 * data, guint size)
{
  guint32 version, header_size, rate, mode, nb_channels;
  GstRTPBasePayload *payload;
  gchar *cstr;
  gboolean res;

  /* we need the header string (8), the version string (20), the version
   * and the header length. */
  if (size < 36)
    goto too_small;

  if (!g_str_has_prefix ((const gchar *) data, "Speex   "))
    goto wrong_header;

  /* skip header and version string */
  data += 28;

  version = GST_READ_UINT32_LE (data);
  if (version != 1)
    goto wrong_version;

  data += 4;
  /* ensure sizes */
  header_size = GST_READ_UINT32_LE (data);
  if (header_size < 80)
    goto header_too_small;

  if (size < header_size)
    goto payload_too_small;

  data += 4;
  rate = GST_READ_UINT32_LE (data);
  data += 4;
  mode = GST_READ_UINT32_LE (data);
  data += 8;
  nb_channels = GST_READ_UINT32_LE (data);

  GST_DEBUG_OBJECT (rtpspeexpay, "rate %d, mode %d, nb_channels %d",
      rate, mode, nb_channels);

  payload = GST_RTP_BASE_PAYLOAD (rtpspeexpay);

  gst_rtp_base_payload_set_options (payload, "audio", FALSE, "SPEEX", rate);
  cstr = g_strdup_printf ("%d", nb_channels);
  res = gst_rtp_base_payload_set_outcaps (payload, "encoding-params",
      G_TYPE_STRING, cstr, NULL);
  g_free (cstr);

  return res;

  /* ERRORS */
too_small:
  {
    GST_DEBUG_OBJECT (rtpspeexpay,
        "ident packet too small, need at least 32 bytes");
    return FALSE;
  }
wrong_header:
  {
    GST_DEBUG_OBJECT (rtpspeexpay,
        "ident packet does not start with \"Speex   \"");
    return FALSE;
  }
wrong_version:
  {
    GST_DEBUG_OBJECT (rtpspeexpay, "can only handle version 1, have version %d",
        version);
    return FALSE;
  }
header_too_small:
  {
    GST_DEBUG_OBJECT (rtpspeexpay,
        "header size too small, need at least 80 bytes, " "got only %d",
        header_size);
    return FALSE;
  }
payload_too_small:
  {
    GST_DEBUG_OBJECT (rtpspeexpay,
        "payload too small, need at least %d bytes, got only %d", header_size,
        size);
    return FALSE;
  }
}

static GstFlowReturn
gst_rtp_speex_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpSPEEXPay *rtpspeexpay;
  GstMapInfo map;
  GstBuffer *outbuf;
  GstClockTime timestamp, duration;
  GstFlowReturn ret;

  rtpspeexpay = GST_RTP_SPEEX_PAY (basepayload);

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  switch (rtpspeexpay->packet) {
    case 0:
      /* ident packet. We need to parse the headers to construct the RTP
       * properties. */
      if (!gst_rtp_speex_pay_parse_ident (rtpspeexpay, map.data, map.size)) {
        gst_buffer_unmap (buffer, &map);
        goto parse_error;
      }

      ret = GST_FLOW_OK;
      gst_buffer_unmap (buffer, &map);
      goto done;
    case 1:
      /* comment packet, we ignore it */
      ret = GST_FLOW_OK;
      gst_buffer_unmap (buffer, &map);
      goto done;
    default:
      /* other packets go in the payload */
      break;
  }
  gst_buffer_unmap (buffer, &map);

  if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_GAP)) {
    ret = GST_FLOW_OK;
    goto done;
  }

  timestamp = GST_BUFFER_PTS (buffer);
  duration = GST_BUFFER_DURATION (buffer);

  /* FIXME, only one SPEEX frame per RTP packet for now */

  outbuf = gst_rtp_buffer_new_allocate (0, 0, 0);
  /* FIXME, assert for now */
  g_assert (gst_buffer_get_size (buffer) <=
      GST_RTP_BASE_PAYLOAD_MTU (rtpspeexpay));

  /* copy timestamp and duration */
  GST_BUFFER_PTS (outbuf) = timestamp;
  GST_BUFFER_DURATION (outbuf) = duration;

  gst_rtp_copy_audio_meta (basepayload, outbuf, buffer);
  outbuf = gst_buffer_append (outbuf, buffer);
  buffer = NULL;

  ret = gst_rtp_base_payload_push (basepayload, outbuf);

done:
  if (buffer)
    gst_buffer_unref (buffer);

  rtpspeexpay->packet++;

  return ret;

  /* ERRORS */
parse_error:
  {
    GST_ELEMENT_ERROR (rtpspeexpay, STREAM, DECODE, (NULL),
        ("Error parsing first identification packet."));
    gst_buffer_unref (buffer);
    return GST_FLOW_ERROR;
  }
}

static GstStateChangeReturn
gst_rtp_speex_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpSPEEXPay *rtpspeexpay;
  GstStateChangeReturn ret;

  rtpspeexpay = GST_RTP_SPEEX_PAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      rtpspeexpay->packet = 0;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_speex_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpspeexpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_SPEEX_PAY);
}
