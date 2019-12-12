/*
 * Siren Payloader Gst Element
 *
 *   @author: Youness Alaoui <kakaroto@kakaroto.homelinux.net>
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
#include "config.h"
#endif

#include "gstrtpsirenpay.h"
#include <gst/rtp/gstrtpbuffer.h>

GST_DEBUG_CATEGORY_STATIC (rtpsirenpay_debug);
#define GST_CAT_DEFAULT (rtpsirenpay_debug)

static GstStaticPadTemplate gst_rtp_siren_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-siren, " "dct-length = (int) 320")
    );

static GstStaticPadTemplate gst_rtp_siren_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 16000, "
        "encoding-name = (string) \"SIREN\", "
        "bitrate = (string) \"16000\", " "dct-length = (int) 320")
    );

static gboolean gst_rtp_siren_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);

G_DEFINE_TYPE (GstRTPSirenPay, gst_rtp_siren_pay,
    GST_TYPE_RTP_BASE_AUDIO_PAYLOAD);

static void
gst_rtp_siren_pay_class_init (GstRTPSirenPayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gstrtpbasepayload_class->set_caps = gst_rtp_siren_pay_setcaps;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_siren_pay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_siren_pay_src_template);
  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Payloader for Siren Audio", "Codec/Payloader/Network/RTP",
      "Packetize Siren audio streams into RTP packets",
      "Youness Alaoui <kakaroto@kakaroto.homelinux.net>");

  GST_DEBUG_CATEGORY_INIT (rtpsirenpay_debug, "rtpsirenpay", 0,
      "siren audio RTP payloader");
}

static void
gst_rtp_siren_pay_init (GstRTPSirenPay * rtpsirenpay)
{
  GstRTPBasePayload *rtpbasepayload;
  GstRTPBaseAudioPayload *rtpbaseaudiopayload;

  rtpbasepayload = GST_RTP_BASE_PAYLOAD (rtpsirenpay);
  rtpbaseaudiopayload = GST_RTP_BASE_AUDIO_PAYLOAD (rtpsirenpay);

  /* we don't set the payload type, it should be set by the application using
   * the pt property or the default 96 will be used */
  rtpbasepayload->clock_rate = 16000;

  /* tell rtpbaseaudiopayload that this is a frame based codec */
  gst_rtp_base_audio_payload_set_frame_based (rtpbaseaudiopayload);
}

static gboolean
gst_rtp_siren_pay_setcaps (GstRTPBasePayload * rtpbasepayload, GstCaps * caps)
{
  GstRTPSirenPay *rtpsirenpay;
  GstRTPBaseAudioPayload *rtpbaseaudiopayload;
  gint dct_length;
  GstStructure *structure;
  const char *payload_name;

  rtpsirenpay = GST_RTP_SIREN_PAY (rtpbasepayload);
  rtpbaseaudiopayload = GST_RTP_BASE_AUDIO_PAYLOAD (rtpbasepayload);

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_get_int (structure, "dct-length", &dct_length);
  if (dct_length != 320)
    goto wrong_dct;

  payload_name = gst_structure_get_name (structure);
  if (g_ascii_strcasecmp ("audio/x-siren", payload_name))
    goto wrong_caps;

  gst_rtp_base_payload_set_options (rtpbasepayload, "audio", TRUE, "SIREN",
      16000);
  /* set options for this frame based audio codec */
  gst_rtp_base_audio_payload_set_frame_options (rtpbaseaudiopayload, 20, 40);

  return gst_rtp_base_payload_set_outcaps (rtpbasepayload, NULL);

  /* ERRORS */
wrong_dct:
  {
    GST_ERROR_OBJECT (rtpsirenpay, "dct-length must be 320, received %d",
        dct_length);
    return FALSE;
  }
wrong_caps:
  {
    GST_ERROR_OBJECT (rtpsirenpay, "expected audio/x-siren, received %s",
        payload_name);
    return FALSE;
  }
}

gboolean
gst_rtp_siren_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpsirenpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_SIREN_PAY);
}
