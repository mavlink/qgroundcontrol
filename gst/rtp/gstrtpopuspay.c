/*
 * Opus Payloader Gst Element
 *
 *   @author: Danilo Cesar Lemes de Paula <danilo.cesar@collabora.co.uk>
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
#include <gst/audio/audio.h>

#include "gstrtpopuspay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpopuspay_debug);
#define GST_CAT_DEFAULT (rtpopuspay_debug)


static GstStaticPadTemplate gst_rtp_opus_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS
    ("audio/x-opus, channels = (int) [1, 2], channel-mapping-family = (int) 0")
    );

static GstStaticPadTemplate gst_rtp_opus_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 48000, "
        "encoding-params = (string) \"2\", "
        "encoding-name = (string) { \"OPUS\", \"X-GST-OPUS-DRAFT-SPITTKA-00\" }")
    );

static gboolean gst_rtp_opus_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstCaps *gst_rtp_opus_pay_getcaps (GstRTPBasePayload * payload,
    GstPad * pad, GstCaps * filter);
static GstFlowReturn gst_rtp_opus_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buffer);

G_DEFINE_TYPE (GstRtpOPUSPay, gst_rtp_opus_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_opus_pay_class_init (GstRtpOPUSPayClass * klass)
{
  GstRTPBasePayloadClass *gstbasertppayload_class;
  GstElementClass *element_class;

  gstbasertppayload_class = (GstRTPBasePayloadClass *) klass;
  element_class = GST_ELEMENT_CLASS (klass);

  gstbasertppayload_class->set_caps = gst_rtp_opus_pay_setcaps;
  gstbasertppayload_class->get_caps = gst_rtp_opus_pay_getcaps;
  gstbasertppayload_class->handle_buffer = gst_rtp_opus_pay_handle_buffer;

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_opus_pay_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_opus_pay_sink_template);

  gst_element_class_set_static_metadata (element_class,
      "RTP Opus payloader",
      "Codec/Payloader/Network/RTP",
      "Puts Opus audio in RTP packets",
      "Danilo Cesar Lemes de Paula <danilo.cesar@collabora.co.uk>");

  GST_DEBUG_CATEGORY_INIT (rtpopuspay_debug, "rtpopuspay", 0,
      "Opus RTP Payloader");
}

static void
gst_rtp_opus_pay_init (GstRtpOPUSPay * rtpopuspay)
{
}

static gboolean
gst_rtp_opus_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  gboolean res;
  GstCaps *src_caps;
  GstStructure *s;
  const char *encoding_name = "OPUS";
  gint channels, rate;
  const char *sprop_stereo = NULL;
  char *sprop_maxcapturerate = NULL;

  src_caps = gst_pad_get_allowed_caps (GST_RTP_BASE_PAYLOAD_SRCPAD (payload));
  if (src_caps) {
    GstStructure *s;
    const GValue *value;

    s = gst_caps_get_structure (src_caps, 0);

    if (gst_structure_has_field (s, "encoding-name")) {
      GValue default_value = G_VALUE_INIT;

      g_value_init (&default_value, G_TYPE_STRING);
      g_value_set_static_string (&default_value, encoding_name);

      value = gst_structure_get_value (s, "encoding-name");
      if (!gst_value_can_intersect (&default_value, value))
        encoding_name = "X-GST-OPUS-DRAFT-SPITTKA-00";
    }
    gst_caps_unref (src_caps);
  }

  s = gst_caps_get_structure (caps, 0);
  if (gst_structure_get_int (s, "channels", &channels)) {
    if (channels > 2) {
      GST_ERROR_OBJECT (payload,
          "More than 2 channels with channel-mapping-family=0 is invalid");
      return FALSE;
    } else if (channels == 2) {
      sprop_stereo = "1";
    } else {
      sprop_stereo = "0";
    }
  }

  if (gst_structure_get_int (s, "rate", &rate)) {
    sprop_maxcapturerate = g_strdup_printf ("%d", rate);
  }

  gst_rtp_base_payload_set_options (payload, "audio", FALSE,
      encoding_name, 48000);

  if (sprop_maxcapturerate && sprop_stereo) {
    res =
        gst_rtp_base_payload_set_outcaps (payload, "sprop-maxcapturerate",
        G_TYPE_STRING, sprop_maxcapturerate, "sprop-stereo", G_TYPE_STRING,
        sprop_stereo, NULL);
  } else if (sprop_maxcapturerate) {
    res =
        gst_rtp_base_payload_set_outcaps (payload, "sprop-maxcapturerate",
        G_TYPE_STRING, sprop_maxcapturerate, NULL);
  } else if (sprop_stereo) {
    res =
        gst_rtp_base_payload_set_outcaps (payload, "sprop-stereo",
        G_TYPE_STRING, sprop_stereo, NULL);
  } else {
    res = gst_rtp_base_payload_set_outcaps (payload, NULL);
  }

  g_free (sprop_maxcapturerate);

  return res;
}

static GstFlowReturn
gst_rtp_opus_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstBuffer *outbuf;
  GstClockTime pts, dts, duration;

  pts = GST_BUFFER_PTS (buffer);
  dts = GST_BUFFER_DTS (buffer);
  duration = GST_BUFFER_DURATION (buffer);

  outbuf = gst_rtp_base_payload_allocate_output_buffer (basepayload, 0, 0, 0);

  gst_rtp_copy_audio_meta (basepayload, outbuf, buffer);

  outbuf = gst_buffer_append (outbuf, buffer);

  GST_BUFFER_PTS (outbuf) = pts;
  GST_BUFFER_DTS (outbuf) = dts;
  GST_BUFFER_DURATION (outbuf) = duration;

  /* Push out */
  return gst_rtp_base_payload_push (basepayload, outbuf);
}

static GstCaps *
gst_rtp_opus_pay_getcaps (GstRTPBasePayload * payload,
    GstPad * pad, GstCaps * filter)
{
  GstCaps *caps, *peercaps, *tcaps;
  GstStructure *s;
  const gchar *stereo;

  if (pad == GST_RTP_BASE_PAYLOAD_SRCPAD (payload))
    return
        GST_RTP_BASE_PAYLOAD_CLASS (gst_rtp_opus_pay_parent_class)->get_caps
        (payload, pad, filter);

  tcaps = gst_pad_get_pad_template_caps (GST_RTP_BASE_PAYLOAD_SRCPAD (payload));
  peercaps = gst_pad_peer_query_caps (GST_RTP_BASE_PAYLOAD_SRCPAD (payload),
      tcaps);
  gst_caps_unref (tcaps);
  if (!peercaps)
    return
        GST_RTP_BASE_PAYLOAD_CLASS (gst_rtp_opus_pay_parent_class)->get_caps
        (payload, pad, filter);

  if (gst_caps_is_empty (peercaps))
    return peercaps;

  caps = gst_pad_get_pad_template_caps (GST_RTP_BASE_PAYLOAD_SINKPAD (payload));

  s = gst_caps_get_structure (peercaps, 0);
  stereo = gst_structure_get_string (s, "stereo");
  if (stereo != NULL) {
    caps = gst_caps_make_writable (caps);

    if (!strcmp (stereo, "1")) {
      GstCaps *caps2 = gst_caps_copy (caps);

      gst_caps_set_simple (caps, "channels", G_TYPE_INT, 2, NULL);
      gst_caps_set_simple (caps2, "channels", G_TYPE_INT, 1, NULL);
      caps = gst_caps_merge (caps, caps2);
    } else if (!strcmp (stereo, "0")) {
      GstCaps *caps2 = gst_caps_copy (caps);

      gst_caps_set_simple (caps, "channels", G_TYPE_INT, 1, NULL);
      gst_caps_set_simple (caps2, "channels", G_TYPE_INT, 2, NULL);
      caps = gst_caps_merge (caps, caps2);
    }
  }
  gst_caps_unref (peercaps);

  if (filter) {
    GstCaps *tmp = gst_caps_intersect_full (caps, filter,
        GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    caps = tmp;
  }

  GST_DEBUG_OBJECT (payload, "Returning caps: %" GST_PTR_FORMAT, caps);
  return caps;
}

gboolean
gst_rtp_opus_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpopuspay",
      GST_RANK_PRIMARY, GST_TYPE_RTP_OPUS_PAY);
}
