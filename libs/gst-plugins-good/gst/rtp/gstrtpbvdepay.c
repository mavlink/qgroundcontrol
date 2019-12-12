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

/**
 * SECTION:element-rtpbvdepay
 * @title: rtpbvdepay
 * @see_also: rtpbvpay
 *
 * Extract BroadcomVoice audio from RTP packets according to RFC 4298.
 * For detailed information see: http://www.rfc-editor.org/rfc/rfc4298.txt
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>
#include "gstrtpbvdepay.h"
#include "gstrtputils.h"

static GstStaticPadTemplate gst_rtp_bv_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) 8000, "
        "encoding-name = (string) \"BV16\"; "
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) 16000, " "encoding-name = (string) \"BV32\"")
    );

static GstStaticPadTemplate gst_rtp_bv_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-bv, " "mode = (int) { 16, 32 }")
    );

static GstBuffer *gst_rtp_bv_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
static gboolean gst_rtp_bv_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);

#define gst_rtp_bv_depay_parent_class parent_class
G_DEFINE_TYPE (GstRTPBVDepay, gst_rtp_bv_depay, GST_TYPE_RTP_BASE_DEPAYLOAD);

static void
gst_rtp_bv_depay_class_init (GstRTPBVDepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_bv_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_bv_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP BroadcomVoice depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts BroadcomVoice audio from RTP packets (RFC 4298)",
      "Wim Taymans <wim.taymans@collabora.co.uk>");

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_bv_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_bv_depay_setcaps;
}

static void
gst_rtp_bv_depay_init (GstRTPBVDepay * rtpbvdepay)
{
  rtpbvdepay->mode = -1;
}

static gboolean
gst_rtp_bv_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstRTPBVDepay *rtpbvdepay = GST_RTP_BV_DEPAY (depayload);
  GstCaps *srccaps;
  GstStructure *structure;
  const gchar *mode_str = NULL;
  gint mode, clock_rate, expected_rate;
  gboolean ret;

  structure = gst_caps_get_structure (caps, 0);

  mode_str = gst_structure_get_string (structure, "encoding-name");
  if (!mode_str)
    goto no_mode;

  if (!strcmp (mode_str, "BV16")) {
    mode = 16;
    expected_rate = 8000;
  } else if (!strcmp (mode_str, "BV32")) {
    mode = 32;
    expected_rate = 16000;
  } else
    goto invalid_mode;

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = expected_rate;
  else if (clock_rate != expected_rate)
    goto wrong_rate;

  depayload->clock_rate = clock_rate;
  rtpbvdepay->mode = mode;

  srccaps = gst_caps_new_simple ("audio/x-bv",
      "mode", G_TYPE_INT, rtpbvdepay->mode, NULL);
  ret = gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), srccaps);

  GST_DEBUG ("set caps on source: %" GST_PTR_FORMAT " (ret=%d)", srccaps, ret);
  gst_caps_unref (srccaps);

  return ret;

  /* ERRORS */
no_mode:
  {
    GST_ERROR_OBJECT (rtpbvdepay, "did not receive an encoding-name");
    return FALSE;
  }
invalid_mode:
  {
    GST_ERROR_OBJECT (rtpbvdepay,
        "invalid encoding-name, expected BV16 or BV32, got %s", mode_str);
    return FALSE;
  }
wrong_rate:
  {
    GST_ERROR_OBJECT (rtpbvdepay, "invalid clock-rate, expected %d, got %d",
        expected_rate, clock_rate);
    return FALSE;
  }
}

static GstBuffer *
gst_rtp_bv_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstBuffer *outbuf;
  gboolean marker;

  marker = gst_rtp_buffer_get_marker (rtp);

  GST_DEBUG ("process : got %" G_GSIZE_FORMAT " bytes, mark %d ts %u seqn %d",
      gst_buffer_get_size (rtp->buffer), marker,
      gst_rtp_buffer_get_timestamp (rtp), gst_rtp_buffer_get_seq (rtp));

  outbuf = gst_rtp_buffer_get_payload_buffer (rtp);

  if (marker && outbuf) {
    /* mark start of talkspurt with RESYNC */
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_RESYNC);
  }

  if (outbuf) {
    gst_rtp_drop_non_audio_meta (depayload, outbuf);
  }

  return outbuf;
}

gboolean
gst_rtp_bv_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpbvdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_BV_DEPAY);
}
