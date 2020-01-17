/* GStreamer
 * Copyright (C) <2007> Wim Taymans <wim.taymans@gmail.com>
 * Copyright (C) <2015> GE Intelligent Platforms Embedded Systems, Inc.
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
 * SECTION:element-rtpL8depay
 * @see_also: rtpL8pay
 *
 * Extract raw audio from RTP packets according to RFC 3551.
 * For detailed information see: http://www.rfc-editor.org/rfc/rfc3551.txt
 *
 * ## Example pipeline
 *
 * |[
 * gst-launch udpsrc caps='application/x-rtp, media=(string)audio, clock-rate=(int)44100, encoding-name=(string)L8, encoding-params=(string)1, channels=(int)1, payload=(int)96' ! rtpL8depay ! pulsesink
 * ]| This example pipeline will depayload an RTP raw audio stream. Refer to
 * the rtpL8pay example to create the RTP stream.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include <gst/audio/audio.h>

#include "gstrtpL8depay.h"
#include "gstrtpchannels.h"

GST_DEBUG_CATEGORY_STATIC (rtpL8depay_debug);
#define GST_CAT_DEFAULT (rtpL8depay_debug)

static GstStaticPadTemplate gst_rtp_L8_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) U8, "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]")
    );

static GstStaticPadTemplate gst_rtp_L8_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) audio, clock-rate = (int) [ 1, MAX ], "
        /* "channels = (int) [1, MAX]"  */
        /* "emphasis = (string) ANY" */
        /* "channel-order = (string) ANY" */
        "encoding-name = (string) L8;")
    );

#define gst_rtp_L8_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpL8Depay, gst_rtp_L8_depay, GST_TYPE_RTP_BASE_DEPAYLOAD);

static gboolean gst_rtp_L8_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_L8_depay_process (GstRTPBaseDepayload * depayload,
    GstBuffer * buf);

static void
gst_rtp_L8_depay_class_init (GstRtpL8DepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gstrtpbasedepayload_class->set_caps = gst_rtp_L8_depay_setcaps;
  gstrtpbasedepayload_class->process = gst_rtp_L8_depay_process;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_rtp_L8_depay_src_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_rtp_L8_depay_sink_template));

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP audio depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts raw audio from RTP packets",
      "Zeeshan Ali <zak147@yahoo.com>," "Wim Taymans <wim.taymans@gmail.com>, "
      "GE Intelligent Platforms Embedded Systems, Inc.");

  GST_DEBUG_CATEGORY_INIT (rtpL8depay_debug, "rtpL8depay", 0,
      "Raw Audio RTP Depayloader");
}

static void
gst_rtp_L8_depay_init (GstRtpL8Depay * rtpL8depay)
{
}

static gint
gst_rtp_L8_depay_parse_int (GstStructure * structure, const gchar * field,
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
gst_rtp_L8_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstRtpL8Depay *rtpL8depay;
  gint clock_rate;
  gint channels;
  GstCaps *srccaps;
  gboolean res;
  const gchar *channel_order;
  const GstRTPChannelOrder *order;
  GstAudioInfo *info;

  rtpL8depay = GST_RTP_L8_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  /* no fixed mapping, we need clock-rate */
  channels = 0;
  clock_rate = 0;

  /* caps can overwrite defaults */
  clock_rate = gst_rtp_L8_depay_parse_int (structure, "clock-rate", clock_rate);
  if (clock_rate == 0)
    goto no_clockrate;

  channels =
      gst_rtp_L8_depay_parse_int (structure, "encoding-params", channels);
  if (channels == 0) {
    channels = gst_rtp_L8_depay_parse_int (structure, "channels", channels);
    if (channels == 0) {
      /* channels defaults to 1 otherwise */
      channels = 1;
    }
  }

  depayload->clock_rate = clock_rate;

  info = &rtpL8depay->info;
  gst_audio_info_init (info);
  info->finfo = gst_audio_format_get_info (GST_AUDIO_FORMAT_U8);
  info->rate = clock_rate;
  info->channels = channels;
  info->bpf = (info->finfo->width / 8) * channels;

  /* add channel positions */
  channel_order = gst_structure_get_string (structure, "channel-order");

  order = gst_rtp_channels_get_by_order (channels, channel_order);
  rtpL8depay->order = order;
  if (order) {
    memcpy (info->position, order->pos,
        sizeof (GstAudioChannelPosition) * channels);
    gst_audio_channel_positions_to_valid_order (info->position, info->channels);
  } else {
    GST_ELEMENT_WARNING (rtpL8depay, STREAM, DECODE,
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
gst_rtp_L8_depay_process (GstRTPBaseDepayload * depayload, GstBuffer * buf)
{
  GstRtpL8Depay *rtpL8depay;
  GstBuffer *outbuf;
  gint payload_len;
  gboolean marker;
  GstRTPBuffer rtp = { NULL };

  rtpL8depay = GST_RTP_L8_DEPAY (depayload);

  gst_rtp_buffer_map (buf, GST_MAP_READ, &rtp);
  payload_len = gst_rtp_buffer_get_payload_len (&rtp);

  if (payload_len <= 0)
    goto empty_packet;

  GST_DEBUG_OBJECT (rtpL8depay, "got payload of %d bytes", payload_len);

  outbuf = gst_rtp_buffer_get_payload_buffer (&rtp);
  marker = gst_rtp_buffer_get_marker (&rtp);

  if (marker) {
    /* mark talk spurt with RESYNC */
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_RESYNC);
  }

  outbuf = gst_buffer_make_writable (outbuf);
  if (rtpL8depay->order &&
      !gst_audio_buffer_reorder_channels (outbuf,
          rtpL8depay->info.finfo->format, rtpL8depay->info.channels,
          rtpL8depay->info.position, rtpL8depay->order->pos)) {
    goto reorder_failed;
  }

  gst_rtp_buffer_unmap (&rtp);

  return outbuf;

  /* ERRORS */
empty_packet:
  {
    GST_ELEMENT_WARNING (rtpL8depay, STREAM, DECODE,
        ("Empty Payload."), (NULL));
    gst_rtp_buffer_unmap (&rtp);
    return NULL;
  }
reorder_failed:
  {
    GST_ELEMENT_ERROR (rtpL8depay, STREAM, DECODE,
        ("Channel reordering failed."), (NULL));
    gst_rtp_buffer_unmap (&rtp);
    return NULL;
  }
}

gboolean
gst_rtp_L8_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpL8depay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_L8_DEPAY);
}
