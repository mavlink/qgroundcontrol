/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
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
 * SECTION:element-rtpamrdepay
 * @title: rtpamrdepay
 * @see_also: rtpamrpay
 *
 * Extract AMR audio from RTP packets according to RFC 3267.
 * For detailed information see: http://www.rfc-editor.org/rfc/rfc3267.txt
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 udpsrc caps='application/x-rtp, media=(string)audio, clock-rate=(int)8000, encoding-name=(string)AMR, encoding-params=(string)1, octet-align=(string)1, payload=(int)96' ! rtpamrdepay ! amrnbdec ! pulsesink
 * ]| This example pipeline will depayload and decode an RTP AMR stream. Refer to
 * the rtpamrpay example to create the RTP stream.
 *
 */

/*
 * RFC 3267 - Real-Time Transport Protocol (RTP) Payload Format and File
 * Storage Format for the Adaptive Multi-Rate (AMR) and Adaptive Multi-Rate
 * Wideband (AMR-WB) Audio Codecs.
 *
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>

#include <stdlib.h>
#include <string.h>
#include "gstrtpamrdepay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpamrdepay_debug);
#define GST_CAT_DEFAULT (rtpamrdepay_debug)

/* RtpAMRDepay signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0
};

/* input is an RTP packet
 *
 * params see RFC 3267, section 8.1
 */
static GstStaticPadTemplate gst_rtp_amr_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) 8000, " "encoding-name = (string) \"AMR\", "
        /* This is the default, so the peer doesn't have to specify it
         * "encoding-params = (string) \"1\", " */
        /* NOTE that all values must be strings in orde to be able to do SDP <->
         * GstCaps mapping. */
        "octet-align = (string) \"1\";"
        /* following options are not needed for a decoder
         *
         "crc = (string) { \"0\", \"1\" }, "
         "robust-sorting = (string) \"0\", "
         "interleaving = (string) \"0\";"
         "mode-set = (int) [ 0, 7 ], "
         "mode-change-period = (int) [ 1, MAX ], "
         "mode-change-neighbor = (boolean) { TRUE, FALSE }, "
         "maxptime = (int) [ 20, MAX ], "
         "ptime = (int) [ 20, MAX ]"
         */
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) 16000, " "encoding-name = (string) \"AMR-WB\", "
        /* This is the default, so the peer doesn't have to specify it
         * "encoding-params = (string) \"1\", " */
        /* NOTE that all values must be strings in orde to be able to do SDP <->
         * GstCaps mapping. */
        "octet-align = (string) \"1\";"
        /* following options are not needed for a decoder
         *
         "crc = (string) { \"0\", \"1\" }, "
         "robust-sorting = (string) \"0\", "
         "interleaving = (string) \"0\""
         "mode-set = (int) [ 0, 7 ], "
         "mode-change-period = (int) [ 1, MAX ], "
         "mode-change-neighbor = (boolean) { TRUE, FALSE }, "
         "maxptime = (int) [ 20, MAX ], "
         "ptime = (int) [ 20, MAX ]"
         */
    )
    );

static GstStaticPadTemplate gst_rtp_amr_depay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/AMR, " "channels = (int) 1," "rate = (int) 8000;"
        "audio/AMR-WB, " "channels = (int) 1," "rate = (int) 16000")
    );

static gboolean gst_rtp_amr_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_amr_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);

#define gst_rtp_amr_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpAMRDepay, gst_rtp_amr_depay, GST_TYPE_RTP_BASE_DEPAYLOAD);

static void
gst_rtp_amr_depay_class_init (GstRtpAMRDepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_amr_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_amr_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP AMR depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts AMR or AMR-WB audio from RTP packets (RFC 3267)",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_amr_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_amr_depay_setcaps;

  GST_DEBUG_CATEGORY_INIT (rtpamrdepay_debug, "rtpamrdepay", 0,
      "AMR/AMR-WB RTP Depayloader");
}

static void
gst_rtp_amr_depay_init (GstRtpAMRDepay * rtpamrdepay)
{
  GstRTPBaseDepayload *depayload;

  depayload = GST_RTP_BASE_DEPAYLOAD (rtpamrdepay);

  gst_pad_use_fixed_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload));
}

static gboolean
gst_rtp_amr_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstCaps *srccaps;
  GstRtpAMRDepay *rtpamrdepay;
  const gchar *params;
  const gchar *str, *type;
  gint clock_rate, need_clock_rate;
  gboolean res;

  rtpamrdepay = GST_RTP_AMR_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  /* figure out the mode first and set the clock rates */
  if ((str = gst_structure_get_string (structure, "encoding-name"))) {
    if (strcmp (str, "AMR") == 0) {
      rtpamrdepay->mode = GST_RTP_AMR_DP_MODE_NB;
      need_clock_rate = 8000;
      type = "audio/AMR";
    } else if (strcmp (str, "AMR-WB") == 0) {
      rtpamrdepay->mode = GST_RTP_AMR_DP_MODE_WB;
      need_clock_rate = 16000;
      type = "audio/AMR-WB";
    } else
      goto invalid_mode;
  } else
    goto invalid_mode;

  if (!(str = gst_structure_get_string (structure, "octet-align")))
    rtpamrdepay->octet_align = FALSE;
  else
    rtpamrdepay->octet_align = (atoi (str) == 1);

  if (!(str = gst_structure_get_string (structure, "crc")))
    rtpamrdepay->crc = FALSE;
  else
    rtpamrdepay->crc = (atoi (str) == 1);

  if (rtpamrdepay->crc) {
    /* crc mode implies octet aligned mode */
    rtpamrdepay->octet_align = TRUE;
  }

  if (!(str = gst_structure_get_string (structure, "robust-sorting")))
    rtpamrdepay->robust_sorting = FALSE;
  else
    rtpamrdepay->robust_sorting = (atoi (str) == 1);

  if (rtpamrdepay->robust_sorting) {
    /* robust_sorting mode implies octet aligned mode */
    rtpamrdepay->octet_align = TRUE;
  }

  if (!(str = gst_structure_get_string (structure, "interleaving")))
    rtpamrdepay->interleaving = FALSE;
  else
    rtpamrdepay->interleaving = (atoi (str) == 1);

  if (rtpamrdepay->interleaving) {
    /* interleaving mode implies octet aligned mode */
    rtpamrdepay->octet_align = TRUE;
  }

  if (!(params = gst_structure_get_string (structure, "encoding-params")))
    rtpamrdepay->channels = 1;
  else {
    rtpamrdepay->channels = atoi (params);
  }

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = need_clock_rate;
  depayload->clock_rate = clock_rate;

  /* we require 1 channel, 8000 Hz, octet aligned, no CRC,
   * no robust sorting, no interleaving for now */
  if (rtpamrdepay->channels != 1)
    return FALSE;
  if (clock_rate != need_clock_rate)
    return FALSE;
  if (rtpamrdepay->octet_align != TRUE)
    return FALSE;
  if (rtpamrdepay->robust_sorting != FALSE)
    return FALSE;
  if (rtpamrdepay->interleaving != FALSE)
    return FALSE;

  srccaps = gst_caps_new_simple (type,
      "channels", G_TYPE_INT, rtpamrdepay->channels,
      "rate", G_TYPE_INT, clock_rate, NULL);
  res = gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), srccaps);
  gst_caps_unref (srccaps);

  return res;

  /* ERRORS */
invalid_mode:
  {
    GST_ERROR_OBJECT (rtpamrdepay, "invalid encoding-name");
    return FALSE;
  }
}

/* -1 is invalid */
static const gint nb_frame_size[16] = {
  12, 13, 15, 17, 19, 20, 26, 31,
  5, -1, -1, -1, -1, -1, -1, 0
};

static const gint wb_frame_size[16] = {
  17, 23, 32, 36, 40, 46, 50, 58,
  60, 5, -1, -1, -1, -1, -1, 0
};

static GstBuffer *
gst_rtp_amr_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpAMRDepay *rtpamrdepay;
  const gint *frame_size;
  GstBuffer *outbuf = NULL;
  gint payload_len;
  GstMapInfo map;

  rtpamrdepay = GST_RTP_AMR_DEPAY (depayload);

  /* setup frame size pointer */
  if (rtpamrdepay->mode == GST_RTP_AMR_DP_MODE_NB)
    frame_size = nb_frame_size;
  else
    frame_size = wb_frame_size;

  /* when we get here, 1 channel, 8000/16000 Hz, octet aligned, no CRC,
   * no robust sorting, no interleaving data is to be depayloaded */
  {
    guint8 *payload, *p, *dp;
    gint i, num_packets, num_nonempty_packets;
    gint amr_len;
    gint ILL, ILP;

    payload_len = gst_rtp_buffer_get_payload_len (rtp);

    /* need at least 2 bytes for the header */
    if (payload_len < 2)
      goto too_small;

    payload = gst_rtp_buffer_get_payload (rtp);

    /* depay CMR. The CMR is used by the sender to request
     * a new encoding mode.
     *
     *  0 1 2 3 4 5 6 7
     * +-+-+-+-+-+-+-+-+
     * | CMR   |R|R|R|R|
     * +-+-+-+-+-+-+-+-+
     */
    /* CMR = (payload[0] & 0xf0) >> 4; */

    /* strip CMR header now, pack FT and the data for the decoder */
    payload_len -= 1;
    payload += 1;

    GST_DEBUG_OBJECT (rtpamrdepay, "payload len %d", payload_len);

    if (rtpamrdepay->interleaving) {
      ILL = (payload[0] & 0xf0) >> 4;
      ILP = (payload[0] & 0x0f);

      payload_len -= 1;
      payload += 1;

      if (ILP > ILL)
        goto wrong_interleaving;
    }

    /*
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
     * +-+-+-+-+-+-+-+-+..
     * |F|  FT   |Q|P|P| more FT..
     * +-+-+-+-+-+-+-+-+..
     */
    /* count number of packets by counting the FTs. Also
     * count number of amr data bytes and number of non-empty
     * packets (this is also the number of CRCs if present). */
    amr_len = 0;
    num_nonempty_packets = 0;
    num_packets = 0;
    for (i = 0; i < payload_len; i++) {
      gint fr_size;
      guint8 FT;

      FT = (payload[i] & 0x78) >> 3;

      fr_size = frame_size[FT];
      GST_DEBUG_OBJECT (rtpamrdepay, "frame size %d", fr_size);
      if (fr_size == -1)
        goto wrong_framesize;

      if (fr_size > 0) {
        amr_len += fr_size;
        num_nonempty_packets++;
      }
      num_packets++;

      if ((payload[i] & 0x80) == 0)
        break;
    }

    if (rtpamrdepay->crc) {
      /* data len + CRC len + header bytes should be smaller than payload_len */
      if (num_packets + num_nonempty_packets + amr_len > payload_len)
        goto wrong_length_1;
    } else {
      /* data len + header bytes should be smaller than payload_len */
      if (num_packets + amr_len > payload_len)
        goto wrong_length_2;
    }

    outbuf = gst_buffer_new_and_alloc (payload_len);

    /* point to destination */
    gst_buffer_map (outbuf, &map, GST_MAP_WRITE);

    /* point to first data packet */
    p = map.data;
    dp = payload + num_packets;
    if (rtpamrdepay->crc) {
      /* skip CRC if present */
      dp += num_nonempty_packets;
    }

    for (i = 0; i < num_packets; i++) {
      gint fr_size;

      /* copy FT, clear F bit */
      *p++ = payload[i] & 0x7f;

      fr_size = frame_size[(payload[i] & 0x78) >> 3];
      if (fr_size > 0) {
        /* copy data packet, FIXME, calc CRC here. */
        memcpy (p, dp, fr_size);

        p += fr_size;
        dp += fr_size;
      }
    }
    gst_buffer_unmap (outbuf, &map);

    /* we can set the duration because each packet is 20 milliseconds */
    GST_BUFFER_DURATION (outbuf) = num_packets * 20 * GST_MSECOND;

    if (gst_rtp_buffer_get_marker (rtp)) {
      /* marker bit marks a buffer after a talkspurt. */
      GST_DEBUG_OBJECT (depayload, "marker bit was set");
      GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_RESYNC);
    }

    GST_DEBUG_OBJECT (depayload, "pushing buffer of size %" G_GSIZE_FORMAT,
        gst_buffer_get_size (outbuf));

    gst_rtp_copy_audio_meta (rtpamrdepay, outbuf, rtp->buffer);
  }

  return outbuf;

  /* ERRORS */
too_small:
  {
    GST_ELEMENT_WARNING (rtpamrdepay, STREAM, DECODE,
        (NULL), ("AMR RTP payload too small (%d)", payload_len));
    goto bad_packet;
  }
wrong_interleaving:
  {
    GST_ELEMENT_WARNING (rtpamrdepay, STREAM, DECODE,
        (NULL), ("AMR RTP wrong interleaving"));
    goto bad_packet;
  }
wrong_framesize:
  {
    GST_ELEMENT_WARNING (rtpamrdepay, STREAM, DECODE,
        (NULL), ("AMR RTP frame size == -1"));
    goto bad_packet;
  }
wrong_length_1:
  {
    GST_ELEMENT_WARNING (rtpamrdepay, STREAM, DECODE,
        (NULL), ("AMR RTP wrong length 1"));
    goto bad_packet;
  }
wrong_length_2:
  {
    GST_ELEMENT_WARNING (rtpamrdepay, STREAM, DECODE,
        (NULL), ("AMR RTP wrong length 2"));
    goto bad_packet;
  }
bad_packet:
  {
    /* no fatal error */
    return NULL;
  }
}

gboolean
gst_rtp_amr_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpamrdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_AMR_DEPAY);
}
