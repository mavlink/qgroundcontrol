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

/**
 * SECTION:element-rtpL24pay
 * @title: rtpL24pay
 * @see_also: rtpL24depay
 *
 * Payload raw 24-bit audio into RTP packets according to RFC 3190, section 4.
 * For detailed information see: http://www.rfc-editor.org/rfc/rfc3190.txt
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 -v audiotestsrc ! audioconvert ! rtpL24pay ! udpsink
 * ]| This example pipeline will payload raw audio. Refer to
 * the rtpL24depay example to depayload and play the RTP stream.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include <gst/audio/audio.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpL24pay.h"
#include "gstrtpchannels.h"

GST_DEBUG_CATEGORY_STATIC (rtpL24pay_debug);
#define GST_CAT_DEFAULT (rtpL24pay_debug)

static GstStaticPadTemplate gst_rtp_L24_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) S24BE, "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]")
    );

static GstStaticPadTemplate gst_rtp_L24_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) [ 96, 127 ], "
        "clock-rate = (int) [ 1, MAX ], "
        "encoding-name = (string) \"L24\", " "channels = (int) [ 1, MAX ];")
    );

static gboolean gst_rtp_L24_pay_setcaps (GstRTPBasePayload * basepayload,
    GstCaps * caps);
static GstCaps *gst_rtp_L24_pay_getcaps (GstRTPBasePayload * rtppayload,
    GstPad * pad, GstCaps * filter);
static GstFlowReturn
gst_rtp_L24_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer);

#define gst_rtp_L24_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpL24Pay, gst_rtp_L24_pay, GST_TYPE_RTP_BASE_AUDIO_PAYLOAD);

static void
gst_rtp_L24_pay_class_init (GstRtpL24PayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gstrtpbasepayload_class->set_caps = gst_rtp_L24_pay_setcaps;
  gstrtpbasepayload_class->get_caps = gst_rtp_L24_pay_getcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_L24_pay_handle_buffer;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_L24_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_L24_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP audio payloader", "Codec/Payloader/Network/RTP",
      "Payload-encode Raw 24-bit audio into RTP packets (RFC 3190)",
      "Wim Taymans <wim.taymans@gmail.com>,"
      "David Holroyd <dave@badgers-in-foil.co.uk>");

  GST_DEBUG_CATEGORY_INIT (rtpL24pay_debug, "rtpL24pay", 0,
      "L24 RTP Payloader");
}

static void
gst_rtp_L24_pay_init (GstRtpL24Pay * rtpL24pay)
{
  GstRTPBaseAudioPayload *rtpbaseaudiopayload;

  rtpbaseaudiopayload = GST_RTP_BASE_AUDIO_PAYLOAD (rtpL24pay);

  /* tell rtpbaseaudiopayload that this is a sample based codec */
  gst_rtp_base_audio_payload_set_sample_based (rtpbaseaudiopayload);
}

static gboolean
gst_rtp_L24_pay_setcaps (GstRTPBasePayload * basepayload, GstCaps * caps)
{
  GstRtpL24Pay *rtpL24pay;
  gboolean res;
  gchar *params;
  GstAudioInfo *info;
  const GstRTPChannelOrder *order;
  GstRTPBaseAudioPayload *rtpbaseaudiopayload;

  rtpbaseaudiopayload = GST_RTP_BASE_AUDIO_PAYLOAD (basepayload);
  rtpL24pay = GST_RTP_L24_PAY (basepayload);

  info = &rtpL24pay->info;
  gst_audio_info_init (info);
  if (!gst_audio_info_from_caps (info, caps))
    goto invalid_caps;

  order = gst_rtp_channels_get_by_pos (info->channels, info->position);
  rtpL24pay->order = order;

  gst_rtp_base_payload_set_options (basepayload, "audio", TRUE, "L24",
      info->rate);
  params = g_strdup_printf ("%d", info->channels);

  if (!order && info->channels > 2) {
    GST_ELEMENT_WARNING (rtpL24pay, STREAM, DECODE,
        (NULL), ("Unknown channel order for %d channels", info->channels));
  }

  if (order && order->name) {
    res = gst_rtp_base_payload_set_outcaps (basepayload,
        "encoding-params", G_TYPE_STRING, params, "channels", G_TYPE_INT,
        info->channels, "channel-order", G_TYPE_STRING, order->name, NULL);
  } else {
    res = gst_rtp_base_payload_set_outcaps (basepayload,
        "encoding-params", G_TYPE_STRING, params, "channels", G_TYPE_INT,
        info->channels, NULL);
  }

  g_free (params);

  /* octet-per-sample is 3 * channels for L24 */
  gst_rtp_base_audio_payload_set_sample_options (rtpbaseaudiopayload,
      3 * info->channels);

  return res;

  /* ERRORS */
invalid_caps:
  {
    GST_DEBUG_OBJECT (rtpL24pay, "invalid caps");
    return FALSE;
  }
}

static GstCaps *
gst_rtp_L24_pay_getcaps (GstRTPBasePayload * rtppayload, GstPad * pad,
    GstCaps * filter)
{
  GstCaps *otherpadcaps;
  GstCaps *caps;

  caps = gst_pad_get_pad_template_caps (pad);

  otherpadcaps = gst_pad_get_allowed_caps (rtppayload->srcpad);
  if (otherpadcaps) {
    if (!gst_caps_is_empty (otherpadcaps)) {
      GstStructure *structure;
      gint channels;
      gint rate;

      structure = gst_caps_get_structure (otherpadcaps, 0);
      caps = gst_caps_make_writable (caps);

      if (gst_structure_get_int (structure, "channels", &channels)) {
        gst_caps_set_simple (caps, "channels", G_TYPE_INT, channels, NULL);
      }

      if (gst_structure_get_int (structure, "clock-rate", &rate)) {
        gst_caps_set_simple (caps, "rate", G_TYPE_INT, rate, NULL);
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

static GstFlowReturn
gst_rtp_L24_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpL24Pay *rtpL24pay;

  rtpL24pay = GST_RTP_L24_PAY (basepayload);
  buffer = gst_buffer_make_writable (buffer);

  if (rtpL24pay->order &&
      !gst_audio_buffer_reorder_channels (buffer, rtpL24pay->info.finfo->format,
          rtpL24pay->info.channels, rtpL24pay->info.position,
          rtpL24pay->order->pos)) {
    return GST_FLOW_ERROR;
  }

  return GST_RTP_BASE_PAYLOAD_CLASS (parent_class)->handle_buffer (basepayload,
      buffer);
}

gboolean
gst_rtp_L24_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpL24pay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_L24_PAY);
}
