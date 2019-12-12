/* GStreamer
 *
 * Copyright (C) <2014> Stian Selnes <stian@pexip.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-rtph261depay
 * @title: rtph261depay
 * @see_also: rtph261pay
 *
 * Extract encoded H.261 video frames from RTP packets according to RFC 4587.
 * For detailed information see: https://www.rfc-editor.org/rfc/rfc4587.txt
 *
 * The depayloader takes an RTP packet and extracts its H.261 stream. It
 * aggregates the extracted stream until a complete frame is received before
 * it pushes it downstream.
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 udpsrc caps='application/x-rtp, payload=31' ! rtph261depay ! avdec_h261 ! autovideosink
 * ]| This example pipeline will depayload and decode an RTP H.261 video stream.
 * Refer to the rtph261pay example to create the RTP stream.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>
#include "gstrtph261depay.h"
#include "gstrtph261pay.h"      /* GstRtpH261PayHeader */
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtph261depay_debug);
#define GST_CAT_DEFAULT (rtph261depay_debug)

static const guint8 NO_LEFTOVER = 0xFF;

static GstStaticPadTemplate gst_rtp_h261_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h261")
    );

static GstStaticPadTemplate gst_rtp_h261_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_H261_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H261\"; "
        "application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H261\"")
    );

G_DEFINE_TYPE (GstRtpH261Depay, gst_rtp_h261_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);
#define parent_class gst_rtp_h261_depay_parent_class

static GstBuffer *
gst_rtp_h261_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpH261Depay *depay;
  GstBuffer *outbuf = NULL;
  gint payload_len;
  guint8 *payload;
  const guint header_len = GST_RTP_H261_PAYLOAD_HEADER_LEN;
  gboolean marker;
  GstRtpH261PayHeader *header;

  depay = GST_RTP_H261_DEPAY (depayload);

  if (GST_BUFFER_IS_DISCONT (rtp->buffer)) {
    GST_DEBUG_OBJECT (depay, "Discont buffer, flushing adapter");
    gst_adapter_clear (depay->adapter);
    depay->leftover = NO_LEFTOVER;
    depay->start = FALSE;
  }

  payload_len = gst_rtp_buffer_get_payload_len (rtp);
  payload = gst_rtp_buffer_get_payload (rtp);

  marker = gst_rtp_buffer_get_marker (rtp);

  if (payload_len < header_len + 1) {
    /* Must have at least one byte payload */
    GST_WARNING_OBJECT (depay, "Dropping packet with invalid payload length");
    return NULL;
  }

  header = (GstRtpH261PayHeader *) payload;

  GST_DEBUG_OBJECT (depay,
      "payload_len: %d, header_len: %d, sbit: %d, ebit: %d, marker %d",
      payload_len, header_len, header->sbit, header->ebit, marker);

  payload += header_len;
  payload_len -= header_len;

  if (!depay->start) {
    /* Check for picture start code */
    guint32 bits = GST_READ_UINT32_BE (payload) << header->sbit;
    if (payload_len > 4 && bits >> 12 == 0x10) {
      GST_DEBUG_OBJECT (depay, "Found picture start code");
      depay->start = TRUE;
    } else {
      GST_DEBUG_OBJECT (depay, "No picture start code yet, skipping payload");
      goto skip;
    }
  }

  if (header->sbit != 0) {
    /* Take the leftover from previous packet and merge it at the beginning */
    payload[0] &= 0xFF >> header->sbit;
    if (depay->leftover != NO_LEFTOVER) {
      /* Happens if sbit is set for first packet in frame. Then previous byte
       * has already been flushed. */
      payload[0] |= depay->leftover;
    }
    depay->leftover = NO_LEFTOVER;
  }

  if (header->ebit == 0) {
    /* H.261 stream ends on byte boundary, take entire packet */
    gst_adapter_push (depay->adapter,
        gst_rtp_buffer_get_payload_subbuffer (rtp, header_len, payload_len));
  } else {
    /* Take the entire buffer except for the last byte, which will be kept to
     * merge with next packet */
    gst_adapter_push (depay->adapter,
        gst_rtp_buffer_get_payload_subbuffer (rtp, header_len,
            payload_len - 1));
    depay->leftover = payload[payload_len - 1] & (0xFF << header->ebit);
  }

skip:
  if (marker) {
    if (depay->start) {
      guint avail;

      if (depay->leftover != NO_LEFTOVER) {
        GstBuffer *buf = gst_buffer_new_and_alloc (1);
        gst_buffer_memset (buf, 0, depay->leftover, 1);
        gst_adapter_push (depay->adapter, buf);
        depay->leftover = NO_LEFTOVER;
      }

      avail = gst_adapter_available (depay->adapter);
      outbuf = gst_adapter_take_buffer (depay->adapter, avail);
      gst_rtp_drop_non_video_meta (depay, outbuf);

      /* Note that the I flag does not mean intra frame, but that the entire
       * stream is intra coded. */
      if (header->i)
        GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);
      else
        GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

      GST_DEBUG_OBJECT (depay, "Pushing out a buffer of %u bytes", avail);
      depay->start = FALSE;
    } else {
      depay->start = TRUE;
    }
  }

  return outbuf;
}

static gboolean
gst_rtp_h261_depay_setcaps (GstRTPBaseDepayload * filter, GstCaps * caps)
{
  GstCaps *srccaps;

  srccaps = gst_caps_new_empty_simple ("video/x-h261");
  gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (filter), srccaps);
  gst_caps_unref (srccaps);

  return TRUE;
}

static GstStateChangeReturn
gst_rtp_h261_depay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpH261Depay *depay;
  GstStateChangeReturn ret;

  depay = GST_RTP_H261_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_adapter_clear (depay->adapter);
      depay->start = FALSE;
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

static void
gst_rtp_h261_depay_dispose (GObject * object)
{
  GstRtpH261Depay *depay;

  depay = GST_RTP_H261_DEPAY (object);

  if (depay->adapter) {
    gst_object_unref (depay->adapter);
    depay->adapter = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_rtp_h261_depay_class_init (GstRtpH261DepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);
  gstrtpbasedepayload_class = GST_RTP_BASE_DEPAYLOAD_CLASS (klass);

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h261_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h261_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP H261 depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts H261 video from RTP packets (RFC 4587)",
      "Stian Selnes <stian@pexip.com>");

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_h261_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_h261_depay_setcaps;

  gobject_class->dispose = gst_rtp_h261_depay_dispose;

  gstelement_class->change_state = gst_rtp_h261_depay_change_state;

  GST_DEBUG_CATEGORY_INIT (rtph261depay_debug, "rtph261depay", 0,
      "H261 Video RTP Depayloader");
}

static void
gst_rtp_h261_depay_init (GstRtpH261Depay * depay)
{
  depay->adapter = gst_adapter_new ();
  depay->leftover = NO_LEFTOVER;
}

gboolean
gst_rtp_h261_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtph261depay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_H261_DEPAY);
}
