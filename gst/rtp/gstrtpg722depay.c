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
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include <gst/audio/audio.h>

#include "gstrtpg722depay.h"
#include "gstrtpchannels.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpg722depay_debug);
#define GST_CAT_DEFAULT (rtpg722depay_debug)

static GstStaticPadTemplate gst_rtp_g722_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/G722, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]")
    );

static GstStaticPadTemplate gst_rtp_g722_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", " "clock-rate = (int) 8000, "
        /* "channels = (int) [1, MAX]"  */
        /* "channel-order = (string) ANY" */
        "encoding-name = (string) \"G722\";"
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_G722_STRING ", "
        "clock-rate = (int) [ 1, MAX ]"
        /* "channels = (int) [1, MAX]" */
        /* "emphasis = (string) ANY" */
        /* "channel-order = (string) ANY" */
    )
    );

#define gst_rtp_g722_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpG722Depay, gst_rtp_g722_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static gboolean gst_rtp_g722_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_g722_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);

static void
gst_rtp_g722_depay_class_init (GstRtpG722DepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpg722depay_debug, "rtpg722depay", 0,
      "G722 RTP Depayloader");

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g722_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g722_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP audio depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts G722 audio from RTP packets",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasedepayload_class->set_caps = gst_rtp_g722_depay_setcaps;
  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_g722_depay_process;
}

static void
gst_rtp_g722_depay_init (GstRtpG722Depay * rtpg722depay)
{
}

static gint
gst_rtp_g722_depay_parse_int (GstStructure * structure, const gchar * field,
    gint def)
{
  const gchar *str;
  gint res;

  if ((str = gst_structure_get_string (structure, field)))
    return atoi (str);

  if (gst_structure_get_int (structure, field, &res))
    return res;

  return def;
}

static gboolean
gst_rtp_g722_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstRtpG722Depay *rtpg722depay;
  gint clock_rate, payload, samplerate;
  gint channels;
  GstCaps *srccaps;
  gboolean res;
#if 0
  const gchar *channel_order;
  const GstRTPChannelOrder *order;
#endif

  rtpg722depay = GST_RTP_G722_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  payload = 96;
  gst_structure_get_int (structure, "payload", &payload);
  switch (payload) {
    case GST_RTP_PAYLOAD_G722:
      channels = 1;
      clock_rate = 8000;
      samplerate = 16000;
      break;
    default:
      /* no fixed mapping, we need clock-rate */
      channels = 0;
      clock_rate = 0;
      samplerate = 0;
      break;
  }

  /* caps can overwrite defaults */
  clock_rate =
      gst_rtp_g722_depay_parse_int (structure, "clock-rate", clock_rate);
  if (clock_rate == 0)
    goto no_clockrate;

  if (clock_rate == 8000)
    samplerate = 16000;

  if (samplerate == 0)
    samplerate = clock_rate;

  channels =
      gst_rtp_g722_depay_parse_int (structure, "encoding-params", channels);
  if (channels == 0) {
    channels = gst_rtp_g722_depay_parse_int (structure, "channels", channels);
    if (channels == 0) {
      /* channels defaults to 1 otherwise */
      channels = 1;
    }
  }

  depayload->clock_rate = clock_rate;
  rtpg722depay->rate = samplerate;
  rtpg722depay->channels = channels;

  srccaps = gst_caps_new_simple ("audio/G722",
      "rate", G_TYPE_INT, samplerate, "channels", G_TYPE_INT, channels, NULL);

  /* FIXME: Do something with the channel order */
#if 0
  /* add channel positions */
  channel_order = gst_structure_get_string (structure, "channel-order");

  order = gst_rtp_channels_get_by_order (channels, channel_order);
  if (order) {
    gst_audio_set_channel_positions (gst_caps_get_structure (srccaps, 0),
        order->pos);
  } else {
    GstAudioChannelPosition *pos;

    GST_ELEMENT_WARNING (rtpg722depay, STREAM, DECODE,
        (NULL), ("Unknown channel order '%s' for %d channels",
            GST_STR_NULL (channel_order), channels));
    /* create default NONE layout */
    pos = gst_rtp_channels_create_default (channels);
    gst_audio_set_channel_positions (gst_caps_get_structure (srccaps, 0), pos);
    g_free (pos);
  }
#endif

  res = gst_pad_set_caps (depayload->srcpad, srccaps);
  gst_caps_unref (srccaps);

  return res;

  /* ERRORS */
no_clockrate:
  {
    GST_ERROR_OBJECT (depayload, "no clock-rate specified");
    return FALSE;
  }
}

static GstBuffer *
gst_rtp_g722_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpG722Depay *rtpg722depay;
  GstBuffer *outbuf;
  gint payload_len;
  gboolean marker;

  rtpg722depay = GST_RTP_G722_DEPAY (depayload);

  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  if (payload_len <= 0)
    goto empty_packet;

  GST_DEBUG_OBJECT (rtpg722depay, "got payload of %d bytes", payload_len);

  outbuf = gst_rtp_buffer_get_payload_buffer (rtp);
  marker = gst_rtp_buffer_get_marker (rtp);

  if (marker && outbuf) {
    /* mark talk spurt with RESYNC */
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_RESYNC);
  }

  if (outbuf) {
    gst_rtp_drop_non_audio_meta (rtpg722depay, outbuf);
  }

  return outbuf;

  /* ERRORS */
empty_packet:
  {
    GST_ELEMENT_WARNING (rtpg722depay, STREAM, DECODE,
        ("Empty Payload."), (NULL));
    return NULL;
  }
}

gboolean
gst_rtp_g722_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpg722depay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_G722_DEPAY);
}
