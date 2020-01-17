/* GStreamer
 *
 * Copyright 2007 Nokia Corporation
 * Copyright 2007 Collabora Ltd,
 *  @author: Philippe Kalaf <philippe.kalaf@collabora.co.uk>
 *
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
 *               <2007> Edward Hervey <bilboed@bilboed.com>
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

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>
#include "gstrtph263depay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtph263depay_debug);
#define GST_CAT_DEFAULT (rtph263depay_debug)

#define GST_RFC2190A_HEADER_LEN 4
#define GST_RFC2190B_HEADER_LEN 8
#define GST_RFC2190C_HEADER_LEN 12

static GstStaticPadTemplate gst_rtp_h263_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h263, "
        "variant = (string) \"itu\", " "h263version = (string) \"h263\"")
    );

static GstStaticPadTemplate gst_rtp_h263_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_H263_STRING ", "
        "clock-rate = (int) 90000; "
        /* optional SDP attribute:
         * "a-framesize = (string) \"1234-1234\", "
         */
        "application/x-rtp, "
        "media = (string) \"video\", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H263\""
        /* optional SDP attribute:
         * "a-framesize = (string) \"1234-1234\", "
         */
    )
    );

#define gst_rtp_h263_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpH263Depay, gst_rtp_h263_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static void gst_rtp_h263_depay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_h263_depay_change_state (GstElement *
    element, GstStateChange transition);

static GstBuffer *gst_rtp_h263_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
gboolean gst_rtp_h263_depay_setcaps (GstRTPBaseDepayload * filter,
    GstCaps * caps);

static void
gst_rtp_h263_depay_class_init (GstRtpH263DepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtph263depay_debug, "rtph263depay", 0,
      "H263 Video RTP Depayloader");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gobject_class->finalize = gst_rtp_h263_depay_finalize;

  gstelement_class->change_state = gst_rtp_h263_depay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h263_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h263_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP H263 depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts H263 video from RTP packets (RFC 2190)",
      "Philippe Kalaf <philippe.kalaf@collabora.co.uk>, "
      "Edward Hervey <bilboed@bilboed.com>");

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_h263_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_h263_depay_setcaps;
}

static void
gst_rtp_h263_depay_init (GstRtpH263Depay * rtph263depay)
{
  rtph263depay->adapter = gst_adapter_new ();

  rtph263depay->offset = 0;
  rtph263depay->leftover = 0;
}

static void
gst_rtp_h263_depay_finalize (GObject * object)
{
  GstRtpH263Depay *rtph263depay;

  rtph263depay = GST_RTP_H263_DEPAY (object);

  g_object_unref (rtph263depay->adapter);
  rtph263depay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_rtp_h263_parse_framesize (GstRTPBaseDepayload * filter,
    const gchar * media_attr, GstCaps * srccaps)
{
  gchar *dimension, *endptr;
  gint width, height;
  GstStructure *d;

  width = g_ascii_strtoull (media_attr, &endptr, 10);
  if (width <= 0) {
    GST_ERROR_OBJECT (filter,
        "Framesize media attribute width out of valid range");
    return FALSE;
  } else if (*endptr != '-') {
    GST_ERROR_OBJECT (filter,
        "Framesize media attribute has invalid dimension separator");
    return FALSE;
  }

  dimension = endptr + 1;
  height = g_ascii_strtoull (dimension, &endptr, 10);
  if (height <= 0) {
    GST_ERROR_OBJECT (filter,
        "Framesize media attribute height out of valid range");
    return FALSE;
  } else if (*endptr != '\0') {
    GST_ERROR_OBJECT (filter,
        "Framesize media attribute unexpectedly has trailing characters");
    return FALSE;
  }

  d = gst_caps_get_structure (srccaps, 0);
  gst_structure_set (d, "width", G_TYPE_INT, width, "height", G_TYPE_INT,
      height, NULL);

  return TRUE;
}

gboolean
gst_rtp_h263_depay_setcaps (GstRTPBaseDepayload * filter, GstCaps * caps)
{
  GstCaps *srccaps;
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  gint clock_rate;
  const gchar *framesize;

  srccaps = gst_caps_new_simple ("video/x-h263",
      "variant", G_TYPE_STRING, "itu",
      "h263version", G_TYPE_STRING, "h263", NULL);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;         /* default */
  filter->clock_rate = clock_rate;

  framesize = gst_structure_get_string (structure, "a-framesize");
  if (framesize != NULL) {
    if (!gst_rtp_h263_parse_framesize (filter, framesize, srccaps)) {
      return FALSE;
    }
  }

  gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (filter), srccaps);
  gst_caps_unref (srccaps);

  return TRUE;
}

static GstBuffer *
gst_rtp_h263_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpH263Depay *rtph263depay;
  GstBuffer *outbuf;
  gint payload_len;
  guint8 *payload;
  guint header_len;
  guint SBIT, EBIT;
  gboolean F, P, M;
  gboolean I;

  rtph263depay = GST_RTP_H263_DEPAY (depayload);

  /* flush remaining data on discont */
  if (GST_BUFFER_IS_DISCONT (rtp->buffer)) {
    GST_LOG_OBJECT (depayload, "Discont buffer, flushing adapter");
    gst_adapter_clear (rtph263depay->adapter);
    rtph263depay->offset = 0;
    rtph263depay->leftover = 0;
    rtph263depay->start = FALSE;
  }

  payload_len = gst_rtp_buffer_get_payload_len (rtp);
  payload = gst_rtp_buffer_get_payload (rtp);

  M = gst_rtp_buffer_get_marker (rtp);

  if (payload_len < 1)
    goto too_small;

  /* Let's see what mode we are using */
  F = (payload[0] & 0x80) == 0x80;
  P = (payload[0] & 0x40) == 0x40;

  /* Bit shifting */
  SBIT = (payload[0] & 0x38) >> 3;
  EBIT = (payload[0] & 0x07);

  /* Figure out header length and I-flag */
  if (F == 0) {
    /* F == 0 and P == 0 or 1
     * mode A */
    header_len = GST_RFC2190A_HEADER_LEN;
    GST_LOG ("Mode A");

    /* 0                   1                   2                   3
     * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |F|P|SBIT |EBIT | SRC |I|U|S|A|R      |DBQ| TRB |    TR         |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    if (payload_len <= header_len)
      goto too_small;
    I = (payload[1] & 0x10) == 0x10;
  } else {
    if (P == 0) {
      /* F == 1 and P == 0
       * mode B */
      header_len = GST_RFC2190B_HEADER_LEN;
      GST_LOG ("Mode B");

      /* 0                   1                   2                   3
       * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |F|P|SBIT |EBIT | SRC | QUANT   |  GOBN   |   MBA           |R  |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |I|U|S|A| HMV1        | VMV1        | HMV2        | VMV2        |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       */
      if (payload_len <= header_len)
        goto too_small;
      I = (payload[4] & 0x80) == 0x80;
    } else {
      /* F == 1 and P == 1
       * mode C */
      header_len = GST_RFC2190C_HEADER_LEN;
      GST_LOG ("Mode C");

      /* 0                   1                   2                   3
       * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |F|P|SBIT |EBIT | SRC | QUANT   |  GOBN   |   MBA           |R  |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |I|U|S|A| HMV1        | VMV1        | HMV2        | VMV2        |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * | RR                                  |DBQ| TRB |    TR         |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       */
      if (payload_len <= header_len)
        goto too_small;
      I = (payload[4] & 0x80) == 0x80;
    }
  }

  GST_LOG ("F/P/M/I : %d/%d/%d/%d", F, P, M, I);
  GST_LOG ("SBIT : %d , EBIT : %d", SBIT, EBIT);
  GST_LOG ("payload_len : %d, header_len : %d , leftover : 0x%x",
      payload_len, header_len, rtph263depay->leftover);

  /* skip header */
  payload += header_len;
  payload_len -= header_len;

  if (!rtph263depay->start) {
    /* only mode A should be used when there is a picture start code, but
     * buggy payloaders may send mode B/C in start of frame */
    if (payload_len > 4 && (GST_READ_UINT32_BE (payload) >> 10 == 0x20)) {
      GST_DEBUG ("Mode %c with PSC => frame start", "ABC"[F + P]);
      rtph263depay->start = TRUE;
      if ((! !(payload[4] & 0x02)) != I) {
        GST_DEBUG ("Wrong Picture Coding Type Flag in rtp header");
        I = !I;
      }
      rtph263depay->psc_I = I;
    } else {
      GST_DEBUG ("no frame start yet, skipping payload");
      goto skip;
    }
  }

  /* only trust I info from Mode A starting packet
   * from buggy payloaders or hw */
  I = rtph263depay->psc_I;

  if (SBIT) {
    /* take the leftover and merge it at the beginning, FIXME make the buffer
     * data writable. */
    GST_LOG ("payload[0] : 0x%x", payload[0]);
    payload[0] &= 0xFF >> SBIT;
    GST_LOG ("payload[0] : 0x%x", payload[0]);
    payload[0] |= rtph263depay->leftover;
    GST_LOG ("payload[0] : 0x%x", payload[0]);
    rtph263depay->leftover = 0;
    rtph263depay->offset = 0;
  }

  if (!EBIT) {
    GstBuffer *tmp;

    /* Take the entire buffer */
    tmp = gst_rtp_buffer_get_payload_subbuffer (rtp, header_len, payload_len);
    gst_adapter_push (rtph263depay->adapter, tmp);
  } else {
    GstBuffer *tmp;

    /* Take the entire buffer except for the last byte */
    tmp = gst_rtp_buffer_get_payload_subbuffer (rtp, header_len,
        payload_len - 1);
    gst_adapter_push (rtph263depay->adapter, tmp);

    /* Put the last byte into the leftover */
    GST_DEBUG ("payload[payload_len - 1] : 0x%x", payload[payload_len - 1]);
    GST_DEBUG ("mask : 0x%x", 0xFF << EBIT);
    rtph263depay->leftover = (payload[payload_len - 1] >> EBIT) << EBIT;
    rtph263depay->offset = 1;
    GST_DEBUG ("leftover : 0x%x", rtph263depay->leftover);
  }

skip:
  if (M) {
    if (rtph263depay->start) {
      /* frame is completed */
      guint avail;

      if (rtph263depay->offset) {
        /* push in the leftover */
        GstBuffer *buf = gst_buffer_new_and_alloc (1);

        GST_DEBUG ("Pushing leftover in adapter");
        gst_buffer_fill (buf, 0, &rtph263depay->leftover, 1);
        gst_adapter_push (rtph263depay->adapter, buf);
      }

      avail = gst_adapter_available (rtph263depay->adapter);
      outbuf = gst_adapter_take_buffer (rtph263depay->adapter, avail);

      if (I)
        GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

      GST_DEBUG ("Pushing out a buffer of %d bytes", avail);

      gst_rtp_drop_non_video_meta (rtph263depay, outbuf);

      gst_rtp_base_depayload_push (depayload, outbuf);
      rtph263depay->offset = 0;
      rtph263depay->leftover = 0;
      rtph263depay->start = FALSE;
    } else {
      rtph263depay->start = TRUE;
    }
  }

  return NULL;

too_small:
  {
    GST_ELEMENT_WARNING (rtph263depay, STREAM, DECODE,
        ("Packet payload was too small"), (NULL));
    return NULL;
  }
}

static GstStateChangeReturn
gst_rtp_h263_depay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpH263Depay *rtph263depay;
  GstStateChangeReturn ret;

  rtph263depay = GST_RTP_H263_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_adapter_clear (rtph263depay->adapter);
      rtph263depay->start = TRUE;
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
gst_rtp_h263_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtph263depay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_H263_DEPAY);
}
