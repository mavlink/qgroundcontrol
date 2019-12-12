/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2005> Zeeshan Ali <zeenix@gmail.com>
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
#include <string.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>

#include "gstrtpgsmpay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpgsmpay_debug);
#define GST_CAT_DEFAULT (rtpgsmpay_debug)

static GstStaticPadTemplate gst_rtp_gsm_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-gsm, " "rate = (int) 8000, " "channels = (int) 1")
    );

static GstStaticPadTemplate gst_rtp_gsm_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_GSM_STRING ", "
        "clock-rate = (int) 8000, " "encoding-name = (string) \"GSM\"; "
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 8000, " "encoding-name = (string) \"GSM\"")
    );

static gboolean gst_rtp_gsm_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_gsm_pay_handle_buffer (GstRTPBasePayload * payload,
    GstBuffer * buffer);

#define gst_rtp_gsm_pay_parent_class parent_class
G_DEFINE_TYPE (GstRTPGSMPay, gst_rtp_gsm_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_gsm_pay_class_init (GstRTPGSMPayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpgsmpay_debug, "rtpgsmpay", 0,
      "GSM Audio RTP Payloader");

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_gsm_pay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_gsm_pay_src_template);

  gst_element_class_set_static_metadata (gstelement_class, "RTP GSM payloader",
      "Codec/Payloader/Network/RTP",
      "Payload-encodes GSM audio into a RTP packet",
      "Zeeshan Ali <zeenix@gmail.com>");

  gstrtpbasepayload_class->set_caps = gst_rtp_gsm_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_gsm_pay_handle_buffer;
}

static void
gst_rtp_gsm_pay_init (GstRTPGSMPay * rtpgsmpay)
{
  GST_RTP_BASE_PAYLOAD (rtpgsmpay)->clock_rate = 8000;
  GST_RTP_BASE_PAYLOAD_PT (rtpgsmpay) = GST_RTP_PAYLOAD_GSM;
}

static gboolean
gst_rtp_gsm_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  const char *stname;
  GstStructure *structure;
  gboolean res;

  structure = gst_caps_get_structure (caps, 0);

  stname = gst_structure_get_name (structure);

  if (strcmp ("audio/x-gsm", stname))
    goto invalid_type;

  gst_rtp_base_payload_set_options (payload, "audio",
      payload->pt != GST_RTP_PAYLOAD_GSM, "GSM", 8000);
  res = gst_rtp_base_payload_set_outcaps (payload, NULL);

  return res;

  /* ERRORS */
invalid_type:
  {
    GST_WARNING_OBJECT (payload, "invalid media type received");
    return FALSE;
  }
}

static GstFlowReturn
gst_rtp_gsm_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRTPGSMPay *rtpgsmpay;
  guint payload_len;
  GstBuffer *outbuf;
  GstClockTime timestamp, duration;
  GstFlowReturn ret;

  rtpgsmpay = GST_RTP_GSM_PAY (basepayload);

  timestamp = GST_BUFFER_PTS (buffer);
  duration = GST_BUFFER_DURATION (buffer);

  /* FIXME, only one GSM frame per RTP packet for now */
  payload_len = gst_buffer_get_size (buffer);

  /* FIXME, just error out for now */
  if (payload_len > GST_RTP_BASE_PAYLOAD_MTU (rtpgsmpay))
    goto too_big;

  outbuf = gst_rtp_buffer_new_allocate (0, 0, 0);

  /* copy timestamp and duration */
  GST_BUFFER_PTS (outbuf) = timestamp;
  GST_BUFFER_DURATION (outbuf) = duration;

  gst_rtp_copy_audio_meta (rtpgsmpay, outbuf, buffer);

  /* append payload */
  outbuf = gst_buffer_append (outbuf, buffer);

  GST_DEBUG ("gst_rtp_gsm_pay_chain: pushing buffer of size %" G_GSIZE_FORMAT,
      gst_buffer_get_size (outbuf));

  ret = gst_rtp_base_payload_push (basepayload, outbuf);

  return ret;

  /* ERRORS */
too_big:
  {
    GST_ELEMENT_ERROR (rtpgsmpay, STREAM, ENCODE, (NULL),
        ("payload_len %u > mtu %u", payload_len,
            GST_RTP_BASE_PAYLOAD_MTU (rtpgsmpay)));
    return GST_FLOW_ERROR;
  }
}

gboolean
gst_rtp_gsm_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpgsmpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_GSM_PAY);
}
