/* GStreamer
 * Copyright (C) <2006> Philippe Khalaf <burger@speedy.org>
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

#include <stdlib.h>
#include <gst/rtp/gstrtpbuffer.h>
#include "gstrtpilbcpay.h"

GST_DEBUG_CATEGORY_STATIC (rtpilbcpay_debug);
#define GST_CAT_DEFAULT (rtpilbcpay_debug)

static GstStaticPadTemplate gst_rtp_ilbc_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-iLBC, " "mode = (int) {20, 30}")
    );

static GstStaticPadTemplate gst_rtp_ilbc_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 8000, "
        "encoding-name = (string) \"ILBC\", "
        "mode = (string) { \"20\", \"30\" }")
    );


static GstCaps *gst_rtp_ilbc_pay_sink_getcaps (GstRTPBasePayload * payload,
    GstPad * pad, GstCaps * filter);
static gboolean gst_rtp_ilbc_pay_sink_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);

#define gst_rtp_ilbc_pay_parent_class parent_class
G_DEFINE_TYPE (GstRTPILBCPay, gst_rtp_ilbc_pay,
    GST_TYPE_RTP_BASE_AUDIO_PAYLOAD);

static void
gst_rtp_ilbc_pay_class_init (GstRTPILBCPayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpilbcpay_debug, "rtpilbcpay", 0,
      "iLBC audio RTP payloader");

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_ilbc_pay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_ilbc_pay_src_template);

  gst_element_class_set_static_metadata (gstelement_class, "RTP iLBC Payloader",
      "Codec/Payloader/Network/RTP",
      "Packetize iLBC audio streams into RTP packets",
      "Philippe Kalaf <philippe.kalaf@collabora.co.uk>");

  gstrtpbasepayload_class->set_caps = gst_rtp_ilbc_pay_sink_setcaps;
  gstrtpbasepayload_class->get_caps = gst_rtp_ilbc_pay_sink_getcaps;
}

static void
gst_rtp_ilbc_pay_init (GstRTPILBCPay * rtpilbcpay)
{
  GstRTPBasePayload *rtpbasepayload;
  GstRTPBaseAudioPayload *rtpbaseaudiopayload;

  rtpbasepayload = GST_RTP_BASE_PAYLOAD (rtpilbcpay);
  rtpbaseaudiopayload = GST_RTP_BASE_AUDIO_PAYLOAD (rtpilbcpay);

  /* we don't set the payload type, it should be set by the application using
   * the pt property or the default 96 will be used */
  rtpbasepayload->clock_rate = 8000;

  rtpilbcpay->mode = -1;

  /* tell rtpbaseaudiopayload that this is a frame based codec */
  gst_rtp_base_audio_payload_set_frame_based (rtpbaseaudiopayload);
}

static gboolean
gst_rtp_ilbc_pay_sink_setcaps (GstRTPBasePayload * rtpbasepayload,
    GstCaps * caps)
{
  GstRTPILBCPay *rtpilbcpay;
  GstRTPBaseAudioPayload *rtpbaseaudiopayload;
  gboolean ret;
  gint mode;
  gchar *mode_str;
  GstStructure *structure;
  const char *payload_name;

  rtpilbcpay = GST_RTP_ILBC_PAY (rtpbasepayload);
  rtpbaseaudiopayload = GST_RTP_BASE_AUDIO_PAYLOAD (rtpbasepayload);

  structure = gst_caps_get_structure (caps, 0);

  payload_name = gst_structure_get_name (structure);
  if (g_ascii_strcasecmp ("audio/x-iLBC", payload_name))
    goto wrong_caps;

  if (!gst_structure_get_int (structure, "mode", &mode))
    goto no_mode;

  if (mode != 20 && mode != 30)
    goto wrong_mode;

  gst_rtp_base_payload_set_options (rtpbasepayload, "audio", TRUE, "ILBC",
      8000);
  /* set options for this frame based audio codec */
  gst_rtp_base_audio_payload_set_frame_options (rtpbaseaudiopayload,
      mode, mode == 30 ? 50 : 38);

  mode_str = g_strdup_printf ("%d", mode);
  ret =
      gst_rtp_base_payload_set_outcaps (rtpbasepayload, "mode", G_TYPE_STRING,
      mode_str, NULL);
  g_free (mode_str);

  if (mode != rtpilbcpay->mode && rtpilbcpay->mode != -1)
    goto mode_changed;

  rtpilbcpay->mode = mode;

  return ret;

  /* ERRORS */
wrong_caps:
  {
    GST_ERROR_OBJECT (rtpilbcpay, "expected audio/x-iLBC, received %s",
        payload_name);
    return FALSE;
  }
no_mode:
  {
    GST_ERROR_OBJECT (rtpilbcpay, "did not receive a mode");
    return FALSE;
  }
wrong_mode:
  {
    GST_ERROR_OBJECT (rtpilbcpay, "mode must be 20 or 30, received %d", mode);
    return FALSE;
  }
mode_changed:
  {
    GST_ERROR_OBJECT (rtpilbcpay, "Mode has changed from %d to %d! "
        "Mode cannot change while streaming", rtpilbcpay->mode, mode);
    return FALSE;
  }
}

/* we return the padtemplate caps with the mode field fixated to a value if we
 * can */
static GstCaps *
gst_rtp_ilbc_pay_sink_getcaps (GstRTPBasePayload * rtppayload, GstPad * pad,
    GstCaps * filter)
{
  GstCaps *otherpadcaps;
  GstCaps *caps;

  otherpadcaps = gst_pad_get_allowed_caps (rtppayload->srcpad);
  caps = gst_pad_get_pad_template_caps (pad);

  if (otherpadcaps) {
    if (!gst_caps_is_empty (otherpadcaps)) {
      GstStructure *structure;
      const gchar *mode_str;
      gint mode;

      structure = gst_caps_get_structure (otherpadcaps, 0);

      /* parse mode, if we can */
      mode_str = gst_structure_get_string (structure, "mode");
      if (mode_str) {
        mode = strtol (mode_str, NULL, 10);
        if (mode == 20 || mode == 30) {
          caps = gst_caps_make_writable (caps);
          structure = gst_caps_get_structure (caps, 0);
          gst_structure_set (structure, "mode", G_TYPE_INT, mode, NULL);
        }
      }
    }
    gst_caps_unref (otherpadcaps);
  }

  if (filter) {
    GstCaps *tmp;

    GST_DEBUG_OBJECT (rtppayload, "Intersect %" GST_PTR_FORMAT " and filter %"
        GST_PTR_FORMAT, caps, filter);
    tmp = gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    caps = tmp;
  }

  return caps;
}

gboolean
gst_rtp_ilbc_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpilbcpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_ILBC_PAY);
}
