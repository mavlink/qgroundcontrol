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

#include <string.h>
#include <stdlib.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>

#include "gstrtpceltdepay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpceltdepay_debug);
#define GST_CAT_DEFAULT (rtpceltdepay_debug)

/* RtpCELTDepay signals and args */

#define DEFAULT_FRAMESIZE       480
#define DEFAULT_CHANNELS        1
#define DEFAULT_CLOCKRATE       32000

enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0
};

static GstStaticPadTemplate gst_rtp_celt_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate =  (int) [32000, 48000], "
        "encoding-name = (string) \"CELT\"")
    );

static GstStaticPadTemplate gst_rtp_celt_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-celt")
    );

static GstBuffer *gst_rtp_celt_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
static gboolean gst_rtp_celt_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);

#define gst_rtp_celt_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpCELTDepay, gst_rtp_celt_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static void
gst_rtp_celt_depay_class_init (GstRtpCELTDepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpceltdepay_debug, "rtpceltdepay", 0,
      "CELT RTP Depayloader");

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_celt_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_celt_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP CELT depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts CELT audio from RTP packets",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_celt_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_celt_depay_setcaps;
}

static void
gst_rtp_celt_depay_init (GstRtpCELTDepay * rtpceltdepay)
{
}

/* len 4 bytes LE,
 * vendor string (len bytes),
 * user_len 4 (0) bytes LE
 */
static const gchar gst_rtp_celt_comment[] =
    "\045\0\0\0Depayloaded with GStreamer celtdepay\0\0\0\0";

static gboolean
gst_rtp_celt_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstRtpCELTDepay *rtpceltdepay;
  gint clock_rate, nb_channels = 0, frame_size = 0;
  GstBuffer *buf;
  GstMapInfo map;
  guint8 *ptr;
  const gchar *params;
  GstCaps *srccaps;
  gboolean res;

  rtpceltdepay = GST_RTP_CELT_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    goto no_clockrate;
  depayload->clock_rate = clock_rate;

  if ((params = gst_structure_get_string (structure, "encoding-params")))
    nb_channels = atoi (params);
  if (!nb_channels)
    nb_channels = DEFAULT_CHANNELS;

  if ((params = gst_structure_get_string (structure, "frame-size")))
    frame_size = atoi (params);
  if (!frame_size)
    frame_size = DEFAULT_FRAMESIZE;
  rtpceltdepay->frame_size = frame_size;

  GST_DEBUG_OBJECT (depayload, "clock-rate=%d channels=%d frame-size=%d",
      clock_rate, nb_channels, frame_size);

  /* construct minimal header and comment packet for the decoder */
  buf = gst_buffer_new_and_alloc (60);
  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  ptr = map.data;
  memcpy (ptr, "CELT    ", 8);
  ptr += 8;
  memcpy (ptr, "1.1.12", 7);
  ptr += 20;
  GST_WRITE_UINT32_LE (ptr, 0x80000006);        /* version */
  ptr += 4;
  GST_WRITE_UINT32_LE (ptr, 56);        /* header_size */
  ptr += 4;
  GST_WRITE_UINT32_LE (ptr, clock_rate);        /* rate */
  ptr += 4;
  GST_WRITE_UINT32_LE (ptr, nb_channels);       /* channels */
  ptr += 4;
  GST_WRITE_UINT32_LE (ptr, frame_size);        /* frame-size */
  ptr += 4;
  GST_WRITE_UINT32_LE (ptr, -1);        /* overlap */
  ptr += 4;
  GST_WRITE_UINT32_LE (ptr, -1);        /* bytes_per_packet */
  ptr += 4;
  GST_WRITE_UINT32_LE (ptr, 0); /* extra headers */
  gst_buffer_unmap (buf, &map);

  srccaps = gst_caps_new_empty_simple ("audio/x-celt");
  res = gst_pad_set_caps (depayload->srcpad, srccaps);
  gst_caps_unref (srccaps);

  gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (rtpceltdepay), buf);

  buf = gst_buffer_new_and_alloc (sizeof (gst_rtp_celt_comment));
  gst_buffer_fill (buf, 0, gst_rtp_celt_comment, sizeof (gst_rtp_celt_comment));

  gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (rtpceltdepay), buf);

  return res;

  /* ERRORS */
no_clockrate:
  {
    GST_ERROR_OBJECT (depayload, "no clock-rate specified");
    return FALSE;
  }
}

static GstBuffer *
gst_rtp_celt_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstBuffer *outbuf = NULL;
  guint8 *payload;
  guint offset, pos, payload_len, total_size, size;
  guint8 s;
  gint clock_rate = 0, frame_size = 0;
  GstClockTime framesize_ns = 0, timestamp;
  guint n = 0;
  GstRtpCELTDepay *rtpceltdepay;

  rtpceltdepay = GST_RTP_CELT_DEPAY (depayload);
  clock_rate = depayload->clock_rate;
  frame_size = rtpceltdepay->frame_size;
  framesize_ns = gst_util_uint64_scale_int (frame_size, GST_SECOND, clock_rate);

  timestamp = GST_BUFFER_PTS (rtp->buffer);

  GST_LOG_OBJECT (depayload,
      "got %" G_GSIZE_FORMAT " bytes, mark %d ts %u seqn %d",
      gst_buffer_get_size (rtp->buffer), gst_rtp_buffer_get_marker (rtp),
      gst_rtp_buffer_get_timestamp (rtp), gst_rtp_buffer_get_seq (rtp));

  GST_LOG_OBJECT (depayload, "got clock-rate=%d, frame_size=%d, "
      "_ns=%" GST_TIME_FORMAT ", timestamp=%" GST_TIME_FORMAT, clock_rate,
      frame_size, GST_TIME_ARGS (framesize_ns), GST_TIME_ARGS (timestamp));

  payload = gst_rtp_buffer_get_payload (rtp);
  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  /* first count how many bytes are consumed by the size headers and make offset
   * point to the first data byte */
  total_size = 0;
  offset = 0;
  while (total_size < payload_len) {
    do {
      s = payload[offset++];
      total_size += s + 1;
    } while (s == 0xff);
  }

  /* offset is now pointing to the payload */
  total_size = 0;
  pos = 0;
  while (total_size < payload_len) {
    n++;
    size = 0;
    do {
      s = payload[pos++];
      size += s;
      total_size += s + 1;
    } while (s == 0xff);

    outbuf = gst_rtp_buffer_get_payload_subbuffer (rtp, offset, size);
    offset += size;

    if (frame_size != -1 && clock_rate != -1) {
      GST_BUFFER_PTS (outbuf) = timestamp + framesize_ns * n;
      GST_BUFFER_DURATION (outbuf) = framesize_ns;
    }
    GST_LOG_OBJECT (depayload, "push timestamp=%"
        GST_TIME_FORMAT ", duration=%" GST_TIME_FORMAT,
        GST_TIME_ARGS (GST_BUFFER_PTS (outbuf)),
        GST_TIME_ARGS (GST_BUFFER_DURATION (outbuf)));

    gst_rtp_drop_non_audio_meta (depayload, outbuf);

    gst_rtp_base_depayload_push (depayload, outbuf);
  }

  return NULL;
}

gboolean
gst_rtp_celt_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpceltdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_CELT_DEPAY);
}
