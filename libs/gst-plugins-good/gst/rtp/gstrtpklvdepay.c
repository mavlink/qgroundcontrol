/* GStreamer RTP KLV Depayloader
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
 * SECTION:element-rtpklvdepay
 * @title: rtpklvdepay
 * @see_also: rtpklvpay
 *
 * Extract KLV metadata from RTP packets according to RFC 6597.
 * For detailed information see: http://tools.ietf.org/html/rfc6597
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 udpsrc caps='application/x-rtp, media=(string)application, clock-rate=(int)90000, encoding-name=(string)SMPTE336M' ! rtpklvdepay ! fakesink dump=true
 * ]| This example pipeline will depayload an RTP KLV stream and display
 * a hexdump of the KLV data on stdout.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstrtpklvdepay.h"

#include <string.h>

GST_DEBUG_CATEGORY_STATIC (klvdepay_debug);
#define GST_CAT_DEFAULT (klvdepay_debug)

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("meta/x-klv, parsed = (bool) true"));

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) application, clock-rate = (int) [1, MAX], "
        "encoding-name = (string) SMPTE336M")
    );

#define gst_rtp_klv_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpKlvDepay, gst_rtp_klv_depay, GST_TYPE_RTP_BASE_DEPAYLOAD);

static void gst_rtp_klv_depay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_klv_depay_change_state (GstElement *
    element, GstStateChange transition);
static gboolean gst_rtp_klv_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_klv_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
static gboolean gst_rtp_klv_depay_handle_event (GstRTPBaseDepayload * depay,
    GstEvent * ev);

static void gst_rtp_klv_depay_reset (GstRtpKlvDepay * klvdepay);

static void
gst_rtp_klv_depay_class_init (GstRtpKlvDepayClass * klass)
{
  GstElementClass *element_class = (GstElementClass *) klass;
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstRTPBaseDepayloadClass *rtpbasedepayload_class;

  GST_DEBUG_CATEGORY_INIT (klvdepay_debug, "klvdepay", 0,
      "RTP KLV Depayloader");

  gobject_class->finalize = gst_rtp_klv_depay_finalize;

  element_class->change_state = gst_rtp_klv_depay_change_state;

  gst_element_class_add_static_pad_template (element_class, &src_template);
  gst_element_class_add_static_pad_template (element_class, &sink_template);

  gst_element_class_set_static_metadata (element_class,
      "RTP KLV Depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts KLV (SMPTE ST 336) metadata from RTP packets",
      "Tim-Philipp Müller <tim@centricular.com>");

  rtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  rtpbasedepayload_class->set_caps = gst_rtp_klv_depay_setcaps;
  rtpbasedepayload_class->process_rtp_packet = gst_rtp_klv_depay_process;
  rtpbasedepayload_class->handle_event = gst_rtp_klv_depay_handle_event;
}

static void
gst_rtp_klv_depay_init (GstRtpKlvDepay * klvdepay)
{
  klvdepay->adapter = gst_adapter_new ();
}

static void
gst_rtp_klv_depay_finalize (GObject * object)
{
  GstRtpKlvDepay *klvdepay;

  klvdepay = GST_RTP_KLV_DEPAY (object);

  gst_rtp_klv_depay_reset (klvdepay);
  g_object_unref (klvdepay->adapter);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_klv_depay_reset (GstRtpKlvDepay * klvdepay)
{
  GST_DEBUG_OBJECT (klvdepay, "resetting");
  gst_adapter_clear (klvdepay->adapter);
  klvdepay->resync = TRUE;
  klvdepay->last_rtp_ts = -1;
}

static gboolean
gst_rtp_klv_depay_handle_event (GstRTPBaseDepayload * depay, GstEvent * ev)
{
  switch (GST_EVENT_TYPE (ev)) {
    case GST_EVENT_STREAM_START:{
      GstStreamFlags flags;

      ev = gst_event_make_writable (ev);
      gst_event_parse_stream_flags (ev, &flags);
      gst_event_set_stream_flags (ev, flags | GST_STREAM_FLAG_SPARSE);
      break;
    }
    default:
      break;
  }

  return GST_RTP_BASE_DEPAYLOAD_CLASS (parent_class)->handle_event (depay, ev);
}

static gboolean
gst_rtp_klv_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *s;
  GstCaps *src_caps;
  gboolean res;
  gint clock_rate;

  s = gst_caps_get_structure (caps, 0);

  if (!gst_structure_get_int (s, "clock-rate", &clock_rate))
    return FALSE;

  depayload->clock_rate = clock_rate;

  src_caps = gst_static_pad_template_get_caps (&src_template);
  res = gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), src_caps);
  gst_caps_unref (src_caps);

  return res;
}

static gboolean
klv_get_vlen (const guint8 * data, guint data_len, guint64 * v_len,
    gsize * len_size)
{
  guint8 first_byte, len_len;
  guint64 len;

  g_assert (data_len > 0);

  first_byte = *data++;

  if ((first_byte & 0x80) == 0) {
    *v_len = first_byte & 0x7f;
    *len_size = 1;
    return TRUE;
  }

  len_len = first_byte & 0x7f;

  if (len_len == 0 || len_len > 8)
    return FALSE;

  if ((1 + len_len) > data_len)
    return FALSE;

  *len_size = 1 + len_len;

  len = 0;
  while (len_len > 0) {
    len = len << 8 | *data++;
    --len_len;
  }

  *v_len = len;

  return TRUE;
}

static GstBuffer *
gst_rtp_klv_depay_process_data (GstRtpKlvDepay * klvdepay)
{
  gsize avail, data_len, len_size;
  GstBuffer *outbuf;
  guint8 data[1 + 8];
  guint64 v_len;

  avail = gst_adapter_available (klvdepay->adapter);

  GST_TRACE_OBJECT (klvdepay, "%" G_GSIZE_FORMAT " bytes in adapter", avail);

  if (avail == 0)
    return NULL;

  /* need at least 16 bytes of UL key plus 1 byte of length */
  if (avail < 16 + 1)
    goto bad_klv_packet;

  /* check if the declared KLV unit size matches actual bytes available */
  data_len = MIN (avail - 16, 1 + 8);
  gst_adapter_copy (klvdepay->adapter, data, 16, data_len);
  if (!klv_get_vlen (data, data_len, &v_len, &len_size))
    goto bad_klv_packet;

  GST_LOG_OBJECT (klvdepay, "want %" G_GUINT64_FORMAT " bytes, "
      "have %" G_GSIZE_FORMAT " bytes", 16 + len_size + v_len, avail);

  if (avail < 16 + len_size + v_len)
    goto incomplete_klv_packet;

  /* something is wrong, this shouldn't ever happen */
  if (avail > 16 + len_size + v_len)
    goto bad_klv_packet;

  outbuf = gst_adapter_take_buffer (klvdepay->adapter, avail);

  /* Mark buffers as key unit to signal this is the start of a KLV unit
   * (for now all buffers will be flagged like this, since all buffers are
   * self-contained KLV units, but in future that might change) */
  outbuf = gst_buffer_make_writable (outbuf);
  GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

  return outbuf;

/* ERRORS */
bad_klv_packet:
  {
    GST_WARNING_OBJECT (klvdepay, "bad KLV packet, dropping");
    gst_rtp_klv_depay_reset (klvdepay);
    return NULL;
  }
incomplete_klv_packet:
  {
    GST_DEBUG_OBJECT (klvdepay, "partial KLV packet: have %u bytes, want %u",
        (guint) avail, (guint) (16 + len_size + v_len));
    return NULL;
  }
}

/* We're trying to be pragmatic here, not quite as strict as the spec wants
 * us to be with regard to marker bits and resyncing after packet loss */
static GstBuffer *
gst_rtp_klv_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpKlvDepay *klvdepay = GST_RTP_KLV_DEPAY (depayload);
  GstBuffer *payload, *outbuf = NULL;
  gboolean marker, start = FALSE, maybe_start;
  guint32 rtp_ts;
  guint16 seq;
  guint payload_len;

  /* Ignore DISCONT on first buffer and on buffers following a discont */
  if (GST_BUFFER_IS_DISCONT (rtp->buffer) && klvdepay->last_rtp_ts != -1) {
    GST_WARNING_OBJECT (klvdepay, "DISCONT, need to resync");
    gst_rtp_klv_depay_reset (klvdepay);
  }

  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  /* marker bit signals last fragment of a KLV unit */
  marker = gst_rtp_buffer_get_marker (rtp);

  seq = gst_rtp_buffer_get_seq (rtp);

  /* packet directly after one with marker bit set => start */
  start = klvdepay->last_marker_seq != -1
      && gst_rtp_buffer_compare_seqnum (klvdepay->last_marker_seq, seq) == 1;

  /* deduce start of new KLV unit in case sender doesn't set marker bits
   * (it's not like the spec is ambiguous about that, but what can you do) */
  rtp_ts = gst_rtp_buffer_get_timestamp (rtp);

  maybe_start = klvdepay->last_rtp_ts == -1 || klvdepay->last_rtp_ts != rtp_ts;

  klvdepay->last_rtp_ts = rtp_ts;

  /* fallback to detect self-contained single KLV unit (usual case) */
  if ((!start || !marker || maybe_start) && payload_len > 16) {
    const guint8 *data;
    guint64 v_len;
    gsize len_size;

    data = gst_rtp_buffer_get_payload (rtp);
    if (GST_READ_UINT32_BE (data) == 0x060e2b34 &&
        klv_get_vlen (data + 16, payload_len - 16, &v_len, &len_size)) {
      if (16 + len_size + v_len == payload_len) {
        GST_LOG_OBJECT (klvdepay, "Looks like a self-contained KLV unit");
        marker = TRUE;
        start = TRUE;
      } else if (16 + len_size + v_len > payload_len) {
        GST_LOG_OBJECT (klvdepay,
            "Looks like the start of a fragmented KLV unit");
        start = TRUE;
      }
    }
  }

  /* If this is the first packet and looks like a start, clear resync flag */
  if (klvdepay->resync && klvdepay->last_marker_seq == -1 && start)
    klvdepay->resync = FALSE;

  if (marker)
    klvdepay->last_marker_seq = seq;

  GST_LOG_OBJECT (klvdepay, "payload of %u bytes, marker=%d, start=%d",
      payload_len, marker, start);

  if (klvdepay->resync && !start) {
    GST_DEBUG_OBJECT (klvdepay, "Dropping buffer, waiting to resync");

    if (marker)
      klvdepay->resync = FALSE;

    goto done;
  }

  if (start && !marker)
    outbuf = gst_rtp_klv_depay_process_data (klvdepay);

  payload = gst_rtp_buffer_get_payload_buffer (rtp);
  gst_adapter_push (klvdepay->adapter, payload);

  if (marker)
    outbuf = gst_rtp_klv_depay_process_data (klvdepay);

done:

  return outbuf;
}

static GstStateChangeReturn
gst_rtp_klv_depay_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpKlvDepay *klvdepay;
  GstStateChangeReturn ret;

  klvdepay = GST_RTP_KLV_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_klv_depay_reset (klvdepay);
      klvdepay->last_marker_seq = -1;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_klv_depay_reset (klvdepay);
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_klv_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpklvdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_KLV_DEPAY);
}
