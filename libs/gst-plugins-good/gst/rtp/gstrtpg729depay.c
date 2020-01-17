/* GStreamer
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
#include "gstrtpg729depay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpg729depay_debug);
#define GST_CAT_DEFAULT (rtpg729depay_debug)


/* references:
 *
 * RFC 3551 (4.5.6)
 */

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
 */
static GstStaticPadTemplate gst_rtp_g729_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) 8000, "
        "encoding-name = (string) \"G729\"; "
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_G729_STRING ", "
        "clock-rate = (int) 8000")
    );

static GstStaticPadTemplate gst_rtp_g729_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/G729, " "channels = (int) 1," "rate = (int) 8000")
    );

static gboolean gst_rtp_g729_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_g729_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);

#define gst_rtp_g729_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpG729Depay, gst_rtp_g729_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static void
gst_rtp_g729_depay_class_init (GstRtpG729DepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpg729depay_debug, "rtpg729depay", 0,
      "G.729 RTP Depayloader");

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g729_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g729_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP G.729 depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts G.729 audio from RTP packets (RFC 3551)",
      "Laurent Glayal <spglegle@yahoo.fr>");

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_g729_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_g729_depay_setcaps;
}

static void
gst_rtp_g729_depay_init (GstRtpG729Depay * rtpg729depay)
{
  GstRTPBaseDepayload *depayload;

  depayload = GST_RTP_BASE_DEPAYLOAD (rtpg729depay);

  gst_pad_use_fixed_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload));
}

static gboolean
gst_rtp_g729_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstCaps *srccaps;
  GstRtpG729Depay *rtpg729depay;
  const gchar *params;
  gint clock_rate, channels;
  gboolean ret;

  rtpg729depay = GST_RTP_G729_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  if (!(params = gst_structure_get_string (structure, "encoding-params")))
    channels = 1;
  else {
    channels = atoi (params);
  }

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 8000;

  if (channels != 1)
    goto wrong_channels;

  if (clock_rate != 8000)
    goto wrong_clock_rate;

  depayload->clock_rate = clock_rate;

  srccaps = gst_caps_new_simple ("audio/G729",
      "channels", G_TYPE_INT, channels, "rate", G_TYPE_INT, clock_rate, NULL);
  ret = gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), srccaps);
  gst_caps_unref (srccaps);

  return ret;

  /* ERRORS */
wrong_channels:
  {
    GST_DEBUG_OBJECT (rtpg729depay, "expected 1 channel, got %d", channels);
    return FALSE;
  }
wrong_clock_rate:
  {
    GST_DEBUG_OBJECT (rtpg729depay, "expected 8000 clock-rate, got %d",
        clock_rate);
    return FALSE;
  }
}

static GstBuffer *
gst_rtp_g729_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpG729Depay *rtpg729depay;
  GstBuffer *outbuf = NULL;
  gint payload_len;
  gboolean marker;

  rtpg729depay = GST_RTP_G729_DEPAY (depayload);

  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  /* At least 2 bytes (CNG from G729 Annex B) */
  if (payload_len < 2) {
    GST_ELEMENT_WARNING (rtpg729depay, STREAM, DECODE,
        (NULL), ("G729 RTP payload too small (%d)", payload_len));
    goto bad_packet;
  }

  GST_LOG_OBJECT (rtpg729depay, "payload len %d", payload_len);

  if ((payload_len % 10) == 2) {
    GST_LOG_OBJECT (rtpg729depay, "G729 payload contains CNG frame");
  }

  outbuf = gst_rtp_buffer_get_payload_buffer (rtp);
  marker = gst_rtp_buffer_get_marker (rtp);

  if (marker) {
    /* marker bit starts talkspurt */
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_RESYNC);
  }

  gst_rtp_drop_non_audio_meta (depayload, outbuf);

  GST_LOG_OBJECT (depayload, "pushing buffer of size %" G_GSIZE_FORMAT,
      gst_buffer_get_size (outbuf));

  return outbuf;

  /* ERRORS */
bad_packet:
  {
    /* no fatal error */
    return NULL;
  }
}

gboolean
gst_rtp_g729_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpg729depay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_G729_DEPAY);
}
