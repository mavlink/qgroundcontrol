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

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>

#include <stdlib.h>
#include <string.h>
#include "gstrtpqcelpdepay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpqcelpdepay_debug);
#define GST_CAT_DEFAULT (rtpqcelpdepay_debug)

/* references:
 *
 * RFC 2658 - RTP Payload Format for PureVoice(tm) Audio
 */
#define FRAME_DURATION (20 * GST_MSECOND)

/* RtpQCELPDepay signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0
};

static GstStaticPadTemplate gst_rtp_qcelp_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) 8000, "
        "encoding-name = (string) \"QCELP\"; "
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_QCELP_STRING ", "
        "clock-rate = (int) 8000")
    );

static GstStaticPadTemplate gst_rtp_qcelp_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/qcelp, " "channels = (int) 1," "rate = (int) 8000")
    );

static void gst_rtp_qcelp_depay_finalize (GObject * object);

static gboolean gst_rtp_qcelp_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_qcelp_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);

#define gst_rtp_qcelp_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpQCELPDepay, gst_rtp_qcelp_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static void
gst_rtp_qcelp_depay_class_init (GstRtpQCELPDepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gobject_class->finalize = gst_rtp_qcelp_depay_finalize;

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_qcelp_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_qcelp_depay_setcaps;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_qcelp_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_qcelp_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP QCELP depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts QCELP (PureVoice) audio from RTP packets (RFC 2658)",
      "Wim Taymans <wim.taymans@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (rtpqcelpdepay_debug, "rtpqcelpdepay", 0,
      "QCELP RTP Depayloader");
}

static void
gst_rtp_qcelp_depay_init (GstRtpQCELPDepay * rtpqcelpdepay)
{
}

static void
gst_rtp_qcelp_depay_finalize (GObject * object)
{
  GstRtpQCELPDepay *depay;

  depay = GST_RTP_QCELP_DEPAY (object);

  if (depay->packets != NULL) {
    g_ptr_array_foreach (depay->packets, (GFunc) gst_buffer_unref, NULL);
    g_ptr_array_free (depay->packets, TRUE);
    depay->packets = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static gboolean
gst_rtp_qcelp_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstCaps *srccaps;
  gboolean res;

  srccaps = gst_caps_new_simple ("audio/qcelp",
      "channels", G_TYPE_INT, 1, "rate", G_TYPE_INT, 8000, NULL);
  res = gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), srccaps);
  gst_caps_unref (srccaps);

  return res;
}

static const gint frame_size[16] = {
  1, 4, 8, 17, 35, -8, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 0
};

/* get the frame length, 0 is invalid, negative values are invalid but can be
 * recovered from. */
static gint
get_frame_len (GstRtpQCELPDepay * depay, guint8 frame_type)
{
  if (frame_type >= G_N_ELEMENTS (frame_size))
    return 0;

  return frame_size[frame_type];
}

static guint
count_packets (GstRtpQCELPDepay * depay, guint8 * data, guint size)
{
  guint count = 0;

  while (size > 0) {
    gint frame_len;

    frame_len = get_frame_len (depay, data[0]);

    /* 0 is invalid and we throw away the remainder of the frames */
    if (frame_len == 0)
      break;

    if (frame_len < 0)
      frame_len = -frame_len;

    if (frame_len > size)
      break;

    size -= frame_len;
    data += frame_len;
    count++;
  }
  return count;
}

static void
flush_packets (GstRtpQCELPDepay * depay)
{
  guint i, size;

  GST_DEBUG_OBJECT (depay, "flushing packets");

  size = depay->packets->len;

  for (i = 0; i < size; i++) {
    GstBuffer *outbuf;

    outbuf = g_ptr_array_index (depay->packets, i);
    g_ptr_array_index (depay->packets, i) = NULL;

    gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (depay), outbuf);
  }

  /* and reset interleaving state */
  depay->interleaved = FALSE;
  depay->bundling = 0;
}

static void
add_packet (GstRtpQCELPDepay * depay, guint LLL, guint NNN, guint index,
    GstBuffer * outbuf)
{
  guint idx;
  GstBuffer *old;

  /* figure out the position in the array, note that index is never 0 because we
   * push those packets immediately. */
  idx = NNN + ((LLL + 1) * (index - 1));

  GST_DEBUG_OBJECT (depay, "adding packet at index %u", idx);
  /* free old buffer (should not happen) */
  old = g_ptr_array_index (depay->packets, idx);
  if (old)
    gst_buffer_unref (old);

  /* store new buffer */
  g_ptr_array_index (depay->packets, idx) = outbuf;
}

static GstBuffer *
create_erasure_buffer (GstRtpQCELPDepay * depay)
{
  GstBuffer *outbuf;
  GstMapInfo map;

  outbuf = gst_buffer_new_and_alloc (1);
  gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
  map.data[0] = 14;
  gst_buffer_unmap (outbuf, &map);

  return outbuf;
}

static GstBuffer *
gst_rtp_qcelp_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp)
{
  GstRtpQCELPDepay *depay;
  GstBuffer *outbuf;
  GstClockTime timestamp;
  guint payload_len, offset, index;
  guint8 *payload;
  guint LLL, NNN;

  depay = GST_RTP_QCELP_DEPAY (depayload);

  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  if (payload_len < 2)
    goto too_small;

  timestamp = GST_BUFFER_PTS (rtp->buffer);

  payload = gst_rtp_buffer_get_payload (rtp);

  /*  0 1 2 3 4 5 6 7
   * +-+-+-+-+-+-+-+-+
   * |RR | LLL | NNN |
   * +-+-+-+-+-+-+-+-+
   */
  /* RR = payload[0] >> 6; */
  LLL = (payload[0] & 0x38) >> 3;
  NNN = (payload[0] & 0x07);

  payload_len--;
  payload++;

  GST_DEBUG_OBJECT (depay, "LLL %u, NNN %u", LLL, NNN);

  if (LLL > 5)
    goto invalid_lll;

  if (NNN > LLL)
    goto invalid_nnn;

  if (LLL != 0) {
    /* we are interleaved */
    if (!depay->interleaved) {
      guint size;

      GST_DEBUG_OBJECT (depay, "starting interleaving group");
      /* bundling is not allowed to change in one interleave group */
      depay->bundling = count_packets (depay, payload, payload_len);
      GST_DEBUG_OBJECT (depay, "got bundling of %u", depay->bundling);
      /* we have one bundle where NNN goes from 0 to L, we don't store the index
       * 0 frames, so L+1 packets. Each packet has 'bundling - 1' packets */
      size = (depay->bundling - 1) * (LLL + 1);
      /* create the array to hold the packets */
      if (depay->packets == NULL)
        depay->packets = g_ptr_array_sized_new (size);
      GST_DEBUG_OBJECT (depay, "created packet array of size %u", size);
      g_ptr_array_set_size (depay->packets, size);
      /* we were previously not interleaved, figure out how much space we
       * need to deinterleave */
      depay->interleaved = TRUE;
    }
  } else {
    /* we are not interleaved */
    if (depay->interleaved) {
      GST_DEBUG_OBJECT (depay, "stopping interleaving");
      /* flush packets if we were previously interleaved */
      flush_packets (depay);
    }
    depay->bundling = 0;
  }

  index = 0;
  offset = 1;

  while (payload_len > 0) {
    gint frame_len;
    gboolean do_erasure;

    frame_len = get_frame_len (depay, payload[0]);
    GST_DEBUG_OBJECT (depay, "got frame len %d", frame_len);

    if (frame_len == 0)
      goto invalid_frame;

    if (frame_len < 0) {
      /* need to add an erasure frame but we can recover */
      frame_len = -frame_len;
      do_erasure = TRUE;
    } else {
      do_erasure = FALSE;
    }

    if (frame_len > payload_len)
      goto invalid_frame;

    if (do_erasure) {
      /* create erasure frame */
      outbuf = create_erasure_buffer (depay);
    } else {
      /* each frame goes into its buffer */
      outbuf = gst_rtp_buffer_get_payload_subbuffer (rtp, offset, frame_len);
    }

    GST_BUFFER_PTS (outbuf) = timestamp;
    GST_BUFFER_DURATION (outbuf) = FRAME_DURATION;

    gst_rtp_drop_non_audio_meta (depayload, outbuf);

    if (!depay->interleaved || index == 0) {
      /* not interleaved or first frame in packet, just push */
      gst_rtp_base_depayload_push (depayload, outbuf);

      if (timestamp != -1)
        timestamp += FRAME_DURATION;
    } else {
      /* put in interleave buffer */
      add_packet (depay, LLL, NNN, index, outbuf);

      if (timestamp != -1)
        timestamp += (FRAME_DURATION * (LLL + 1));
    }

    payload_len -= frame_len;
    payload += frame_len;
    offset += frame_len;
    index++;

    /* discard excess packets */
    if (depay->bundling > 0 && depay->bundling <= index)
      break;
  }
  while (index < depay->bundling) {
    GST_DEBUG_OBJECT (depay, "filling with erasure buffer");
    /* fill remainder with erasure packets */
    outbuf = create_erasure_buffer (depay);
    add_packet (depay, LLL, NNN, index, outbuf);
    index++;
  }
  if (depay->interleaved && LLL == NNN) {
    GST_DEBUG_OBJECT (depay, "interleave group ended, flushing");
    /* we have the complete interleave group, flush */
    flush_packets (depay);
  }

  return NULL;

  /* ERRORS */
too_small:
  {
    GST_ELEMENT_WARNING (depay, STREAM, DECODE,
        (NULL), ("QCELP RTP payload too small (%d)", payload_len));
    return NULL;
  }
invalid_lll:
  {
    GST_ELEMENT_WARNING (depay, STREAM, DECODE,
        (NULL), ("QCELP RTP invalid LLL received (%d)", LLL));
    return NULL;
  }
invalid_nnn:
  {
    GST_ELEMENT_WARNING (depay, STREAM, DECODE,
        (NULL), ("QCELP RTP invalid NNN received (%d)", NNN));
    return NULL;
  }
invalid_frame:
  {
    GST_ELEMENT_WARNING (depay, STREAM, DECODE,
        (NULL), ("QCELP RTP invalid frame received"));
    return NULL;
  }
}

gboolean
gst_rtp_qcelp_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpqcelpdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_QCELP_DEPAY);
}
