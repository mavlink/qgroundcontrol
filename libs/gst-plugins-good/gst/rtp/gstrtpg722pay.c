/* GStreamer
 * Copyright (C) <2010> Wim Taymans <wim.taymans@gmail.com>
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

#include <gst/audio/audio.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpg722pay.h"
#include "gstrtpchannels.h"

GST_DEBUG_CATEGORY_STATIC (rtpg722pay_debug);
#define GST_CAT_DEFAULT (rtpg722pay_debug)

static GstStaticPadTemplate gst_rtp_g722_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/G722, " "rate = (int) 16000, " "channels = (int) 1")
    );

static GstStaticPadTemplate gst_rtp_g722_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "encoding-name = (string) \"G722\", "
        "payload = (int) " GST_RTP_PAYLOAD_G722_STRING ", "
        "encoding-params = (string) 1, "
        "clock-rate = (int) 8000; "
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "encoding-name = (string) \"G722\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "encoding-params = (string) 1, " "clock-rate = (int) 8000")
    );

static gboolean gst_rtp_g722_pay_setcaps (GstRTPBasePayload * basepayload,
    GstCaps * caps);
static GstCaps *gst_rtp_g722_pay_getcaps (GstRTPBasePayload * rtppayload,
    GstPad * pad, GstCaps * filter);

#define gst_rtp_g722_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpG722Pay, gst_rtp_g722_pay,
    GST_TYPE_RTP_BASE_AUDIO_PAYLOAD);

static void
gst_rtp_g722_pay_class_init (GstRtpG722PayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpg722pay_debug, "rtpg722pay", 0,
      "G722 RTP Payloader");

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g722_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g722_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP audio payloader", "Codec/Payloader/Network/RTP",
      "Payload-encode Raw audio into RTP packets (RFC 3551)",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasepayload_class->set_caps = gst_rtp_g722_pay_setcaps;
  gstrtpbasepayload_class->get_caps = gst_rtp_g722_pay_getcaps;
}

static void
gst_rtp_g722_pay_init (GstRtpG722Pay * rtpg722pay)
{
  GstRTPBaseAudioPayload *rtpbaseaudiopayload;

  rtpbaseaudiopayload = GST_RTP_BASE_AUDIO_PAYLOAD (rtpg722pay);

  GST_RTP_BASE_PAYLOAD (rtpg722pay)->pt = GST_RTP_PAYLOAD_G722;

  /* tell rtpbaseaudiopayload that this is a sample based codec */
  gst_rtp_base_audio_payload_set_sample_based (rtpbaseaudiopayload);
}

static gboolean
gst_rtp_g722_pay_setcaps (GstRTPBasePayload * basepayload, GstCaps * caps)
{
  GstRtpG722Pay *rtpg722pay;
  GstStructure *structure;
  gint rate, channels, clock_rate;
  gboolean res;
  gchar *params;
#if 0
  GstAudioChannelPosition *pos;
  const GstRTPChannelOrder *order;
#endif
  GstRTPBaseAudioPayload *rtpbaseaudiopayload;

  rtpbaseaudiopayload = GST_RTP_BASE_AUDIO_PAYLOAD (basepayload);
  rtpg722pay = GST_RTP_G722_PAY (basepayload);

  structure = gst_caps_get_structure (caps, 0);

  /* first parse input caps */
  if (!gst_structure_get_int (structure, "rate", &rate))
    goto no_rate;

  if (!gst_structure_get_int (structure, "channels", &channels))
    goto no_channels;

  /* FIXME: Do something with the channel positions */
#if 0
  /* get the channel order */
  pos = gst_audio_get_channel_positions (structure);
  if (pos)
    order = gst_rtp_channels_get_by_pos (channels, pos);
  else
    order = NULL;
#endif

  /* Clock rate is always 8000 Hz for G722 according to
   * RFC 3551 although the sampling rate is 16000 Hz */
  clock_rate = 8000;

  gst_rtp_base_payload_set_options (basepayload, "audio",
      basepayload->pt != GST_RTP_PAYLOAD_G722, "G722", clock_rate);
  params = g_strdup_printf ("%d", channels);

#if 0
  if (!order && channels > 2) {
    GST_ELEMENT_WARNING (rtpg722pay, STREAM, DECODE,
        (NULL), ("Unknown channel order for %d channels", channels));
  }

  if (order && order->name) {
    res = gst_rtp_base_payload_set_outcaps (basepayload,
        "encoding-params", G_TYPE_STRING, params, "channels", G_TYPE_INT,
        channels, "channel-order", G_TYPE_STRING, order->name, NULL);
  } else {
#endif
    res = gst_rtp_base_payload_set_outcaps (basepayload,
        "encoding-params", G_TYPE_STRING, params, "channels", G_TYPE_INT,
        channels, NULL);
#if 0
  }
#endif

  g_free (params);
#if 0
  g_free (pos);
#endif

  rtpg722pay->rate = rate;
  rtpg722pay->channels = channels;

  /* bits-per-sample is 4 * channels for G722, but as the RTP clock runs at
   * half speed (8 instead of 16 khz), pretend it's 8 bits per sample
   * channels. */
  gst_rtp_base_audio_payload_set_samplebits_options (rtpbaseaudiopayload,
      8 * rtpg722pay->channels);

  return res;

  /* ERRORS */
no_rate:
  {
    GST_DEBUG_OBJECT (rtpg722pay, "no rate given");
    return FALSE;
  }
no_channels:
  {
    GST_DEBUG_OBJECT (rtpg722pay, "no channels given");
    return FALSE;
  }
}

static GstCaps *
gst_rtp_g722_pay_getcaps (GstRTPBasePayload * rtppayload, GstPad * pad,
    GstCaps * filter)
{
  GstCaps *otherpadcaps;
  GstCaps *caps;

  otherpadcaps = gst_pad_get_allowed_caps (rtppayload->srcpad);
  caps = gst_pad_get_pad_template_caps (pad);

  if (otherpadcaps) {
    if (!gst_caps_is_empty (otherpadcaps)) {
      caps = gst_caps_make_writable (caps);
      gst_caps_set_simple (caps, "channels", G_TYPE_INT, 1, NULL);
      gst_caps_set_simple (caps, "rate", G_TYPE_INT, 16000, NULL);
    }
    gst_caps_unref (otherpadcaps);
  }

  if (filter) {
    GstCaps *tmp;

    GST_DEBUG_OBJECT (rtppayload, "Intersect %" GST_PTR_FORMAT " and filter %"
        GST_PTR_FORMAT, caps, filter);
    tmp = gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    caps = tmp;
  }

  return caps;
}

gboolean
gst_rtp_g722_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpg722pay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_G722_PAY);
}
