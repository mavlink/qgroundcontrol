/* GStreamer RTP KLV Payloader
 * Copyright (C) 2014-2015 Tim-Philipp Müller <tim@centricular.com>>
 * Copyright (C) 2014-2015 Centricular Ltd
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
 * SECTION:element-rtpklvpay
 * @title: rtpklvpay
 * @see_also: rtpklvdepay
 *
 * Payloads KLV metadata into RTP packets according to RFC 6597.
 * For detailed information see: http://tools.ietf.org/html/rfc6597
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 filesrc location=video-with-klv.ts ! tsdemux ! rtpklvpay ! udpsink
 * ]| This example pipeline will payload an RTP KLV stream extracted from an
 * MPEG-TS stream and send it via UDP to an RTP receiver.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstrtpklvpay.h"
#include "gstrtputils.h"

#include <string.h>

GST_DEBUG_CATEGORY_STATIC (klvpay_debug);
#define GST_CAT_DEFAULT (klvpay_debug)

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) application, clock-rate = (int) [1, MAX], "
        "encoding-name = (string) SMPTE336M")
    );

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("meta/x-klv, parsed = (bool) true"));

#define gst_rtp_klv_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpKlvPay, gst_rtp_klv_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static gboolean gst_rtp_klv_pay_setcaps (GstRTPBasePayload * pay,
    GstCaps * caps);
static GstFlowReturn gst_rtp_klv_pay_handle_buffer (GstRTPBasePayload * pay,
    GstBuffer * buf);

static void
gst_rtp_klv_pay_class_init (GstRtpKlvPayClass * klass)
{
  GstElementClass *element_class = (GstElementClass *) klass;
  GstRTPBasePayloadClass *rtpbasepay_class;

  GST_DEBUG_CATEGORY_INIT (klvpay_debug, "klvpay", 0, "RTP KLV Payloader");

  gst_element_class_add_static_pad_template (element_class, &src_template);
  gst_element_class_add_static_pad_template (element_class, &sink_template);

  gst_element_class_set_static_metadata (element_class,
      "RTP KLV Payloader", "Codec/Payloader/Network/RTP",
      "Payloads KLV (SMPTE ST 336) metadata as RTP packets",
      "Tim-Philipp Müller <tim@centricular.com>");

  rtpbasepay_class = (GstRTPBasePayloadClass *) klass;

  rtpbasepay_class->set_caps = gst_rtp_klv_pay_setcaps;
  rtpbasepay_class->handle_buffer = gst_rtp_klv_pay_handle_buffer;
}

static void
gst_rtp_klv_pay_init (GstRtpKlvPay * klvpay)
{
  /* nothing to do here yet */
}

static gboolean
gst_rtp_klv_pay_setcaps (GstRTPBasePayload * pay, GstCaps * caps)
{
  /* FIXME: allow other clock rates */
  gst_rtp_base_payload_set_options (pay, "application", TRUE, "SMPTE336M",
      90000);

  return gst_rtp_base_payload_set_outcaps (pay, NULL);
}

static GstFlowReturn
gst_rtp_klv_pay_handle_buffer (GstRTPBasePayload * basepayload, GstBuffer * buf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstBufferList *list = NULL;
  GstRtpKlvPay *pay;
  GstMapInfo map;
  GstBuffer *outbuf = NULL;
  gsize offset;
  guint mtu, rtp_header_size, max_payload_size;

  pay = GST_RTP_KLV_PAY (basepayload);
  mtu = GST_RTP_BASE_PAYLOAD_MTU (basepayload);

  rtp_header_size = gst_rtp_buffer_calc_header_len (0);
  max_payload_size = mtu - rtp_header_size;

  gst_buffer_map (buf, &map, GST_MAP_READ);

  if (map.size == 0)
    goto done;

  /* KLV coding shall use and only use a fixed 16-byte SMPTE-administered
   * Universal Label, according to SMPTE 298M as Key (Rec. ITU R-BT.1653-1) */
  if (map.size < 16 || GST_READ_UINT32_BE (map.data) != 0x060E2B34)
    goto bad_input;

  if (map.size > max_payload_size)
    list = gst_buffer_list_new ();

  GST_LOG_OBJECT (pay, "%" G_GSIZE_FORMAT " bytes of data to payload",
      map.size);

  offset = 0;
  while (offset < map.size) {
    GstBuffer *payloadbuf;
    GstRTPBuffer rtp = { NULL };
    guint payload_size;
    guint bytes_left;

    bytes_left = map.size - offset;
    payload_size = MIN (bytes_left, max_payload_size);

    outbuf = gst_rtp_buffer_new_allocate (0, 0, 0);

    if (payload_size == bytes_left) {
      GST_LOG_OBJECT (pay, "last packet of KLV unit");
      gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);
      gst_rtp_buffer_set_marker (&rtp, 1);
      gst_rtp_buffer_unmap (&rtp);
    }

    GST_LOG_OBJECT (pay, "packet with payload size %u", payload_size);

    gst_rtp_copy_meta (GST_ELEMENT_CAST (pay), outbuf, buf, 0);

    payloadbuf = gst_buffer_copy_region (buf, GST_BUFFER_COPY_MEMORY,
        offset, payload_size);

    /* join rtp header + payload memory parts */
    outbuf = gst_buffer_append (outbuf, payloadbuf);

    GST_BUFFER_PTS (outbuf) = GST_BUFFER_PTS (buf);
    GST_BUFFER_DTS (outbuf) = GST_BUFFER_DTS (buf);

    /* and add to list */
    if (list != NULL)
      gst_buffer_list_insert (list, -1, outbuf);

    offset += payload_size;
  }

done:

  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  if (list != NULL)
    ret = gst_rtp_base_payload_push_list (basepayload, list);
  else if (outbuf != NULL)
    ret = gst_rtp_base_payload_push (basepayload, outbuf);

  return ret;

/* ERRORS */
bad_input:
  {
    GST_ERROR_OBJECT (pay, "Input doesn't look like a KLV packet, ignoring");
    goto done;
  }
}

gboolean
gst_rtp_klv_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpklvpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_KLV_PAY);
}
