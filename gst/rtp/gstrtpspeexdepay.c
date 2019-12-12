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

#include <string.h>
#include <stdlib.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>

#include "gstrtpspeexdepay.h"
#include "gstrtputils.h"

/* RtpSPEEXDepay signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0
};

static GstStaticPadTemplate gst_rtp_speex_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate =  (int) [6000, 48000], "
        "encoding-name = (string) \"SPEEX\"")
    /*  "encoding-params = (string) \"1\"" */
    );

static GstStaticPadTemplate gst_rtp_speex_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-speex")
    );

static GstBuffer *gst_rtp_speex_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
static gboolean gst_rtp_speex_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);

G_DEFINE_TYPE (GstRtpSPEEXDepay, gst_rtp_speex_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static void
gst_rtp_speex_depay_class_init (GstRtpSPEEXDepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_speex_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_speex_depay_setcaps;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_speex_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_speex_depay_sink_template);
  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Speex depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts Speex audio from RTP packets",
      "Edgard Lima <edgard.lima@gmail.com>");
}

static void
gst_rtp_speex_depay_init (GstRtpSPEEXDepay * rtpspeexdepay)
{
}

static gint
gst_rtp_speex_depay_get_mode (gint rate)
{
  if (rate > 25000)
    return 2;
  else if (rate > 12500)
    return 1;
  else
    return 0;
}

/* len 4 bytes LE,
 * vendor string (len bytes),
 * user_len 4 (0) bytes LE
 */
static const gchar gst_rtp_speex_comment[] =
    "\045\0\0\0Depayloaded with GStreamer speexdepay\0\0\0\0";

static gboolean
gst_rtp_speex_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstRtpSPEEXDepay *rtpspeexdepay;
  gint clock_rate, nb_channels;
  GstBuffer *buf;
  GstMapInfo map;
  guint8 *data;
  const gchar *params;
  GstCaps *srccaps;
  gboolean res;

  rtpspeexdepay = GST_RTP_SPEEX_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    goto no_clockrate;
  depayload->clock_rate = clock_rate;

  if (!(params = gst_structure_get_string (structure, "encoding-params")))
    nb_channels = 1;
  else {
    nb_channels = atoi (params);
  }

  /* construct minimal header and comment packet for the decoder */
  buf = gst_buffer_new_and_alloc (80);
  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = map.data;
  memcpy (data, "Speex   ", 8);
  data += 8;
  memcpy (data, "1.1.12", 7);
  data += 20;
  GST_WRITE_UINT32_LE (data, 1);        /* version */
  data += 4;
  GST_WRITE_UINT32_LE (data, 80);       /* header_size */
  data += 4;
  GST_WRITE_UINT32_LE (data, clock_rate);       /* rate */
  data += 4;
  GST_WRITE_UINT32_LE (data, gst_rtp_speex_depay_get_mode (clock_rate));        /* mode */
  data += 4;
  GST_WRITE_UINT32_LE (data, 4);        /* mode_bitstream_version */
  data += 4;
  GST_WRITE_UINT32_LE (data, nb_channels);      /* nb_channels */
  data += 4;
  GST_WRITE_UINT32_LE (data, -1);       /* bitrate */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0xa0);     /* frame_size */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0);        /* VBR */
  data += 4;
  GST_WRITE_UINT32_LE (data, 1);        /* frames_per_packet */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0);        /* extra_headers */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0);        /* reserved1 */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0);        /* reserved2 */
  gst_buffer_unmap (buf, &map);

  srccaps = gst_caps_new_empty_simple ("audio/x-speex");
  res = gst_pad_set_caps (depayload->srcpad, srccaps);
  gst_caps_unref (srccaps);

  gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (rtpspeexdepay), buf);

  buf = gst_buffer_new_and_alloc (sizeof (gst_rtp_speex_comment));
  gst_buffer_fill (buf, 0, gst_rtp_speex_comment,
      sizeof (gst_rtp_speex_comment));

  gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (rtpspeexdepay), buf);

  return res;

  /* ERRORS */
no_clockrate:
  {
    GST_DEBUG_OBJECT (depayload, "no clock-rate specified");
    return FALSE;
  }
}

static GstBuffer *
gst_rtp_speex_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp)
{
  GstBuffer *outbuf = NULL;

  GST_DEBUG ("process : got %" G_GSIZE_FORMAT " bytes, mark %d ts %u seqn %d",
      gst_buffer_get_size (rtp->buffer),
      gst_rtp_buffer_get_marker (rtp),
      gst_rtp_buffer_get_timestamp (rtp), gst_rtp_buffer_get_seq (rtp));

  /* nothing special to be done */
  outbuf = gst_rtp_buffer_get_payload_buffer (rtp);

  if (outbuf) {
    GST_BUFFER_DURATION (outbuf) = 20 * GST_MSECOND;
    gst_rtp_drop_non_audio_meta (depayload, outbuf);
  }

  return outbuf;
}

gboolean
gst_rtp_speex_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpspeexdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_SPEEX_DEPAY);
}
