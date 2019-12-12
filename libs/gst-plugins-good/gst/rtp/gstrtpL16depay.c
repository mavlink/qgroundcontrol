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
 * SECTION:element-rtpL16depay
 * @title: rtpL16depay
 * @see_also: rtpL16pay
 *
 * Extract raw audio from RTP packets according to RFC 3551.
 * For detailed information see: http://www.rfc-editor.org/rfc/rfc3551.txt
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 udpsrc caps='application/x-rtp, media=(string)audio, clock-rate=(int)44100, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1, payload=(int)96' ! rtpL16depay ! pulsesink
 * ]| This example pipeline will depayload an RTP raw audio stream. Refer to
 * the rtpL16pay example to create the RTP stream.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include <gst/audio/audio.h>

#include "gstrtpL16depay.h"
#include "gstrtpchannels.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpL16depay_debug);
#define GST_CAT_DEFAULT (rtpL16depay_debug)

static GstStaticPadTemplate gst_rtp_L16_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) S16BE, "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]")
    );

static GstStaticPadTemplate gst_rtp_L16_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", " "clock-rate = (int) [ 1, MAX ], "
        /* "channels = (int) [1, MAX]"  */
        /* "emphasis = (string) ANY" */
        /* "channel-order = (string) ANY" */
        "encoding-name = (string) \"L16\";"
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) { " GST_RTP_PAYLOAD_L16_STEREO_STRING ", "
        GST_RTP_PAYLOAD_L16_MONO_STRING " }," "clock-rate = (int) [ 1, MAX ]"
        /* "channels = (int) [1, MAX]" */
        /* "emphasis = (string) ANY" */
        /* "channel-order = (string) ANY" */
    )
    );

#define gst_rtp_L16_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpL16Depay, gst_rtp_L16_depay, GST_TYPE_RTP_BASE_DEPAYLOAD);

static gboolean gst_rtp_L16_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_L16_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);

static void
gst_rtp_L16_depay_class_init (GstRtpL16DepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gstrtpbasedepayload_class->set_caps = gst_rtp_L16_depay_setcaps;
  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_L16_depay_process;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_L16_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_L16_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP audio depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts raw audio from RTP packets",
      "Zeeshan Ali <zak147@yahoo.com>," "Wim Taymans <wim.taymans@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (rtpL16depay_debug, "rtpL16depay", 0,
      "Raw Audio RTP Depayloader");
}

static void
gst_rtp_L16_depay_init (GstRtpL16Depay * rtpL16depay)
{
}

static gint
gst_rtp_L16_depay_parse_int (GstStructure * structure, const gchar * field,
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
gst_rtp_L16_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstRtpL16Depay *rtpL16depay;
  gint clock_rate, payload;
  gint channels;
  GstCaps *srccaps;
  gboolean res;
  const gchar *channel_order;
  const GstRTPChannelOrder *order;
  GstAudioInfo *info;

  rtpL16depay = GST_RTP_L16_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  payload = 96;
  gst_structure_get_int (structure, "payload", &payload);
  switch (payload) {
    case GST_RTP_PAYLOAD_L16_STEREO:
      channels = 2;
      clock_rate = 44100;
      break;
    case GST_RTP_PAYLOAD_L16_MONO:
      channels = 1;
      clock_rate = 44100;
      break;
    default:
      /* no fixed mapping, we need clock-rate */
      channels = 0;
      clock_rate = 0;
      break;
  }

  /* caps can overwrite defaults */
  clock_rate =
      gst_rtp_L16_depay_parse_int (structure, "clock-rate", clock_rate);
  if (clock_rate == 0)
    goto no_clockrate;

  channels =
      gst_rtp_L16_depay_parse_int (structure, "encoding-params", channels);
  if (channels == 0) {
    channels = gst_rtp_L16_depay_parse_int (structure, "channels", channels);
    if (channels == 0) {
      /* channels defaults to 1 otherwise */
      channels = 1;
    }
  }

  depayload->clock_rate = clock_rate;

  info = &rtpL16depay->info;
  gst_audio_info_init (info);
  info->finfo = gst_audio_format_get_info (GST_AUDIO_FORMAT_S16BE);
  info->rate = clock_rate;
  info->channels = channels;
  info->bpf = (info->finfo->width / 8) * channels;

  /* add channel positions */
  channel_order = gst_structure_get_string (structure, "channel-order");

  order = gst_rtp_channels_get_by_order (channels, channel_order);
  rtpL16depay->order = order;
  if (order) {
    memcpy (info->position, order->pos,
        sizeof (GstAudioChannelPosition) * channels);
    gst_audio_channel_positions_to_valid_order (info->position, info->channels);
  } else {
    GST_ELEMENT_WARNING (rtpL16depay, STREAM, DECODE,
        (NULL), ("Unknown channel order '%s' for %d channels",
            GST_STR_NULL (channel_order), channels));
    /* create default NONE layout */
    gst_rtp_channels_create_default (channels, info->position);
  }

  srccaps = gst_audio_info_to_caps (info);
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
gst_rtp_L16_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpL16Depay *rtpL16depay;
  GstBuffer *outbuf;
  gint payload_len;
  gboolean marker;
  GstAudioInfo *info;

  rtpL16depay = GST_RTP_L16_DEPAY (depayload);

  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  if (payload_len <= 0)
    goto empty_packet;

  GST_DEBUG_OBJECT (rtpL16depay, "got payload of %d bytes", payload_len);

  outbuf = gst_rtp_buffer_get_payload_buffer (rtp);
  marker = gst_rtp_buffer_get_marker (rtp);

  if (marker) {
    /* mark talk spurt with RESYNC */
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_RESYNC);
  }

  outbuf = gst_buffer_make_writable (outbuf);
  info = &rtpL16depay->info;

  if (payload_len % info->bpf != 0)
    goto wrong_payload_size;

  if (rtpL16depay->order &&
      !gst_audio_buffer_reorder_channels (outbuf,
          info->finfo->format, info->channels,
          info->position, rtpL16depay->order->pos)) {
    goto reorder_failed;
  }

  gst_rtp_drop_non_audio_meta (rtpL16depay, outbuf);

  return outbuf;

  /* ERRORS */
empty_packet:
  {
    GST_ELEMENT_WARNING (rtpL16depay, STREAM, DECODE,
        ("Empty Payload."), (NULL));
    return NULL;
  }
wrong_payload_size:
  {
    GST_ELEMENT_WARNING (rtpL16depay, STREAM, DECODE,
        ("Wrong Payload Size."), (NULL));
    return NULL;
  }
reorder_failed:
  {
    GST_ELEMENT_ERROR (rtpL16depay, STREAM, DECODE,
        ("Channel reordering failed."), (NULL));
    return NULL;
  }
}

gboolean
gst_rtp_L16_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpL16depay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_L16_DEPAY);
}
