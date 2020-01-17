/* GStreamer plugin for forward error correction
 * Copyright (C) 2017 Pexip
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Mikhail Fludkov <misha@pexip.com>
 */

/**
 * SECTION:element-rtpreddec
 * @short_description: RTP Redundant Audio Data (RED) decoder
 * @title: rtpreddec
 *
 * Decode Redundant Audio Data (RED) as per RFC 2198.
 *
 * This element is mostly provided for chrome webrtc compatibility:
 * chrome will wrap ulpfec-protected streams in RED packets, and such
 * streams need to be unwrapped by this element before being passed on
 * to #GstRtpUlpFecDec.
 *
 * The #GstRtpRedDec:pt property should be set to the expected payload
 * types of the RED packets.
 *
 * When using #GstRtpBin, this element should be inserted through the
 * #GstRtpBin::request-aux-receiver signal.
 *
 * ## Example pipeline
 *
 * |[
 * gst-launch-1.0 udpsrc port=8888 caps="application/x-rtp, payload=96, clock-rate=90000" ! rtpreddec pt=122 ! rtpstorage size-time=220000000 ! rtpssrcdemux ! application/x-rtp, payload=96, clock-rate=90000, media=video, encoding-name=H264 ! rtpjitterbuffer do-lost=1 latency=200 !  rtpulpfecdec pt=122 ! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink
 * ]| This example will receive a stream with RED and ULP FEC and try to reconstruct the packets.
 *
 * See also: #GstRtpRedEnc, #GstWebRTCBin, #GstRtpBin
 * Since: 1.14
 */

#include <gst/rtp/gstrtpbuffer.h>

#include "rtpredcommon.h"
#include "gstrtpreddec.h"
#include "rtpulpfeccommon.h"

#define RTP_HISTORY_MAX_SIZE (16)

typedef struct
{
  guint32 timestamp;
  guint16 seq;
} RTPHistItem;

#define RTP_HIST_ITEM_TIMESTAMP(p) ((RTPHistItem *)p)->timestamp
#define RTP_HIST_ITEM_SEQ(p) ((RTPHistItem *)p)->seq

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp"));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp"));

#define UNDEF_PT                -1
#define MIN_PT                  UNDEF_PT
#define MAX_PT                  127
#define DEFAULT_PT              UNDEF_PT

GST_DEBUG_CATEGORY_STATIC (gst_rtp_red_dec_debug);
#define GST_CAT_DEFAULT gst_rtp_red_dec_debug

G_DEFINE_TYPE (GstRtpRedDec, gst_rtp_red_dec, GST_TYPE_ELEMENT);

enum
{
  PROP_0,
  PROP_PT,
  PROP_RECEIVED
};

static RTPHistItem *
rtp_hist_item_alloc (void)
{
  return g_slice_new (RTPHistItem);
}

static void
rtp_hist_item_free (gpointer item)
{
  g_slice_free (RTPHistItem, item);
}

static gint
gst_rtp_red_history_find_less_or_equal (gconstpointer item,
    gconstpointer timestamp)
{
  guint32 t = GPOINTER_TO_UINT (timestamp);
  gint32 diff = t - RTP_HIST_ITEM_TIMESTAMP (item);
  return diff < 0;
}

static gint
gst_rtp_red_history_find_less (gconstpointer item, gconstpointer timestamp)
{
  guint32 t = GPOINTER_TO_UINT (timestamp);
  gint32 diff = t - RTP_HIST_ITEM_TIMESTAMP (item);
  return diff <= 0;
}

static void
gst_rtp_red_history_update (GstRtpRedDec * self, GstRTPBuffer * rtp)
{
  RTPHistItem *item;
  GList *link, *sibling;

  /* If we have not reached MAX number of elements in the history,
   * allocate a new link and a new item,
   * otherwise reuse the tail (the oldest data) without any reallocations
   */
  if (self->rtp_history->length < RTP_HISTORY_MAX_SIZE) {
    item = rtp_hist_item_alloc ();
    link = g_list_alloc ();
    link->data = item;
  } else {
    link = g_queue_pop_tail_link (self->rtp_history);
    item = link->data;
  }

  item->timestamp = gst_rtp_buffer_get_timestamp (rtp);
  item->seq = gst_rtp_buffer_get_seq (rtp);

  /* Looking for a place to insert new link.
   * The queue has newest to oldest rtp timestamps, so in 99% cases
   * it is inserted before the head of the queue */
  sibling = g_list_find_custom (self->rtp_history->head,
      GUINT_TO_POINTER (item->timestamp),
      gst_rtp_red_history_find_less_or_equal);
  g_queue_push_nth_link (self->rtp_history,
      g_list_position (self->rtp_history->head, sibling), link);
}

static gboolean
rtp_red_buffer_is_valid (GstRtpRedDec * self, GstRTPBuffer * red_rtp,
    gsize * dst_first_red_payload_offset)
{
  guint8 *payload = gst_rtp_buffer_get_payload (red_rtp);
  gsize payload_len = gst_rtp_buffer_get_payload_len (red_rtp);
  gsize red_hdrs_offset = 0;
  guint red_hdrs_checked = 0;
  guint redundant_payload_len = 0;

  while (TRUE) {
    gpointer red_hdr = payload + red_hdrs_offset;
    gsize red_hdr_len;
    gboolean is_redundant;

    ++red_hdrs_checked;

    /* Can we address the first byte where F bit is located ? */
    if (red_hdrs_offset + 1 > payload_len)
      goto red_buffer_invalid;

    is_redundant = rtp_red_block_is_redundant (red_hdr);

    /* Is it the last block? */
    if (is_redundant) {
      red_hdr_len = rtp_red_block_header_get_length (TRUE);

      /* Can we address all the other bytes in RED block header? */
      if (red_hdrs_offset + red_hdr_len > payload_len)
        goto red_buffer_invalid;

      redundant_payload_len += rtp_red_block_get_payload_length (red_hdr);
      red_hdrs_offset += red_hdr_len;
    } else {
      red_hdr_len = rtp_red_block_header_get_length (FALSE);
      red_hdrs_offset += red_hdr_len;
      break;
    }
  }

  /* Do we have enough data to create redundant packets & main packet. Keep in
   * mind that redundant_payload_len contains the length of redundant packets only.
   */
  if (red_hdrs_offset + redundant_payload_len >= payload_len)
    goto red_buffer_invalid;

  *dst_first_red_payload_offset = red_hdrs_offset;

  GST_LOG_OBJECT (self, "RED packet has %u blocks", red_hdrs_checked);
  return TRUE;

red_buffer_invalid:
  GST_WARNING_OBJECT (self, "Received invalid RED packet "
      "ssrc=0x%08x pt=%u tstamp=%u seq=%u size=%u, "
      "checked %u blocks",
      gst_rtp_buffer_get_ssrc (red_rtp),
      gst_rtp_buffer_get_payload_type (red_rtp),
      gst_rtp_buffer_get_timestamp (red_rtp),
      gst_rtp_buffer_get_seq (red_rtp),
      gst_rtp_buffer_get_packet_len (red_rtp), red_hdrs_checked);
  return FALSE;
}

static gboolean
gst_red_history_lost_seq_num_for_timestamp (GstRtpRedDec * self,
    guint32 timestamp, guint16 * dst_seq_num)
{
  GList *older_sibling = g_list_find_custom (self->rtp_history->head,
      GUINT_TO_POINTER (timestamp),
      gst_rtp_red_history_find_less);
  RTPHistItem *older;
  RTPHistItem *newer;
  guint32 timestamp_diff;
  gint seq_diff, lost_packet_idx;

  if (NULL == older_sibling) {
    if (self->rtp_history->length == RTP_HISTORY_MAX_SIZE)
      GST_WARNING_OBJECT (self, "History is too short. "
          "Oldest rtp timestamp %u, looking for %u, size %u",
          RTP_HIST_ITEM_TIMESTAMP (self->rtp_history->tail->data),
          timestamp, self->rtp_history->length);
    return FALSE;
  }

  if (NULL == older_sibling->prev) {
    GST_WARNING_OBJECT (self, "RED block timestamp offset probably wrong. "
        "Latest rtp timestamp %u, looking for %u, size %u",
        RTP_HIST_ITEM_TIMESTAMP (self->rtp_history->head->data),
        timestamp, self->rtp_history->length);
    return FALSE;
  }

  older = older_sibling->data;
  newer = older_sibling->prev->data;
  /* We know for sure @older has lower timestamp than we are looking for,
   * if @newer has the same timestamp, there is no packet loss and we
   * don't need to use redundant data */
  if (newer->timestamp == timestamp)
    return FALSE;

  seq_diff = gst_rtp_buffer_compare_seqnum (older->seq, newer->seq);
  if (seq_diff <= 1) {
    if (seq_diff == 1)
      GST_WARNING_OBJECT (self, "RED block timestamp offset is wrong: "
          "#%u,%u #%u,%u looking for %u",
          older->seq, older->timestamp,
          newer->seq, newer->timestamp, timestamp);
    else
      GST_WARNING_OBJECT (self, "RTP timestamps increasing while "
          "sequence numbers decreasing: #%u,%u #%u,%u",
          older->seq, older->timestamp, newer->seq, newer->timestamp);
    return FALSE;
  }

  timestamp_diff = newer->timestamp - older->timestamp;
  for (lost_packet_idx = 1; lost_packet_idx < seq_diff; ++lost_packet_idx) {
    guint32 lost_timestamp = older->timestamp +
        lost_packet_idx * timestamp_diff / seq_diff;
    if (lost_timestamp == timestamp) {
      *dst_seq_num = older->seq + lost_packet_idx;
      return TRUE;
    }
  }

  GST_WARNING_OBJECT (self, "Can't find RED block timestamp "
      "#%u,%u #%u,%u looking for %u",
      older->seq, older->timestamp, newer->seq, newer->timestamp, timestamp);
  return FALSE;
}

static GstBuffer *
gst_rtp_red_create_packet (GstRtpRedDec * self, GstRTPBuffer * red_rtp,
    gboolean marker, guint8 pt, guint16 seq_num, guint32 timestamp,
    gsize red_payload_subbuffer_start, gsize red_payload_subbuffer_len)
{
  guint csrc_count = gst_rtp_buffer_get_csrc_count (red_rtp);
  GstBuffer *ret = gst_rtp_buffer_new_allocate (0, 0, csrc_count);
  GstRTPBuffer ret_rtp = GST_RTP_BUFFER_INIT;
  guint i;
  if (!gst_rtp_buffer_map (ret, GST_MAP_WRITE, &ret_rtp))
    g_assert_not_reached ();

  gst_rtp_buffer_set_marker (&ret_rtp, marker);
  gst_rtp_buffer_set_payload_type (&ret_rtp, pt);
  gst_rtp_buffer_set_seq (&ret_rtp, seq_num);
  gst_rtp_buffer_set_timestamp (&ret_rtp, timestamp);
  gst_rtp_buffer_set_ssrc (&ret_rtp, gst_rtp_buffer_get_ssrc (red_rtp));
  for (i = 0; i < csrc_count; ++i)
    gst_rtp_buffer_set_csrc (&ret_rtp, i, gst_rtp_buffer_get_csrc (red_rtp, i));
  gst_rtp_buffer_unmap (&ret_rtp);

  ret = gst_buffer_append (ret,
      gst_rtp_buffer_get_payload_subbuffer (red_rtp,
          red_payload_subbuffer_start, red_payload_subbuffer_len));

  /* Timestamps, meta, flags from the RED packet should go to main block packet */
  gst_buffer_copy_into (ret, red_rtp->buffer, GST_BUFFER_COPY_METADATA, 0, -1);
  return ret;
}

static GstBuffer *
gst_rtp_red_create_from_redundant_block (GstRtpRedDec * self,
    GstRTPBuffer * red_rtp, gsize * red_hdr_offset, gsize * red_payload_offset)
{
  guint8 *payload = gst_rtp_buffer_get_payload (red_rtp);
  guint8 *red_hdr = payload + *red_hdr_offset;
  guint32 lost_timestamp = gst_rtp_buffer_get_timestamp (red_rtp) -
      rtp_red_block_get_timestamp_offset (red_hdr);

  GstBuffer *ret = NULL;
  guint16 lost_seq = 0;
  if (gst_red_history_lost_seq_num_for_timestamp (self, lost_timestamp,
          &lost_seq)) {
    GST_LOG_OBJECT (self, "Recovering from RED packet pt=%u ts=%u seq=%u"
        " len=%u present", rtp_red_block_get_payload_type (red_hdr),
        lost_timestamp, lost_seq, rtp_red_block_get_payload_length (red_hdr));
    ret =
        gst_rtp_red_create_packet (self, red_rtp, FALSE,
        rtp_red_block_get_payload_type (red_hdr), lost_seq, lost_timestamp,
        *red_payload_offset, rtp_red_block_get_payload_length (red_hdr));
    GST_BUFFER_FLAG_SET (ret, GST_RTP_BUFFER_FLAG_REDUNDANT);
  } else {
    GST_LOG_OBJECT (self, "Ignore RED packet pt=%u ts=%u len=%u because already"
        " present", rtp_red_block_get_payload_type (red_hdr), lost_timestamp,
        rtp_red_block_get_payload_length (red_hdr));
  }

  *red_hdr_offset += rtp_red_block_header_get_length (TRUE);
  *red_payload_offset += rtp_red_block_get_payload_length (red_hdr);
  return ret;
}

static GstBuffer *
gst_rtp_red_create_from_main_block (GstRtpRedDec * self,
    GstRTPBuffer * red_rtp, gsize red_hdr_offset, gsize * red_payload_offset)
{
  guint8 *payload = gst_rtp_buffer_get_payload (red_rtp);
  GstBuffer *ret = gst_rtp_red_create_packet (self, red_rtp,
      gst_rtp_buffer_get_marker (red_rtp),
      rtp_red_block_get_payload_type (payload + red_hdr_offset),
      gst_rtp_buffer_get_seq (red_rtp),
      gst_rtp_buffer_get_timestamp (red_rtp),
      *red_payload_offset, -1);
  *red_payload_offset = gst_rtp_buffer_get_payload_len (red_rtp);
  GST_LOG_OBJECT (self, "Extracting main payload from RED pt=%u seq=%u ts=%u"
      " marker=%u", rtp_red_block_get_payload_type (payload + red_hdr_offset),
      gst_rtp_buffer_get_seq (red_rtp), gst_rtp_buffer_get_timestamp (red_rtp),
      gst_rtp_buffer_get_marker (red_rtp));

  return ret;
}

static GstBuffer *
gst_rtp_red_create_from_block (GstRtpRedDec * self, GstRTPBuffer * red_rtp,
    gsize * red_hdr_offset, gsize * red_payload_offset)
{
  guint8 *payload = gst_rtp_buffer_get_payload (red_rtp);

  if (rtp_red_block_is_redundant (payload + (*red_hdr_offset)))
    return gst_rtp_red_create_from_redundant_block (self, red_rtp,
        red_hdr_offset, red_payload_offset);

  return gst_rtp_red_create_from_main_block (self, red_rtp, *red_hdr_offset,
      red_payload_offset);
}

static GstFlowReturn
gst_rtp_red_process (GstRtpRedDec * self, GstRTPBuffer * red_rtp,
    gsize first_red_payload_offset)
{
  gsize red_hdr_offset = 0;
  gsize red_payload_offset = first_red_payload_offset;
  gsize payload_len = gst_rtp_buffer_get_payload_len (red_rtp);
  GstFlowReturn ret = GST_FLOW_OK;

  do {
    GstBuffer *buf =
        gst_rtp_red_create_from_block (self, red_rtp, &red_hdr_offset,
        &red_payload_offset);
    if (buf)
      ret = gst_pad_push (self->srcpad, buf);
  } while (GST_FLOW_OK == ret && red_payload_offset < payload_len);

  return ret;
}

static GstFlowReturn
gst_rtp_red_dec_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstRtpRedDec *self = GST_RTP_RED_DEC (parent);
  GstRTPBuffer irtp = GST_RTP_BUFFER_INIT;
  GstFlowReturn ret = GST_FLOW_OK;
  gsize first_red_payload_offset = 0;

  if (self->pt == UNDEF_PT)
    return gst_pad_push (self->srcpad, buffer);

  if (!gst_rtp_buffer_map (buffer, GST_MAP_READ, &irtp))
    return gst_pad_push (self->srcpad, buffer);

  gst_rtp_red_history_update (self, &irtp);

  if (self->pt != gst_rtp_buffer_get_payload_type (&irtp)) {
    GST_LOG_RTP_PACKET (self, "rtp header (incoming)", &irtp);

    gst_rtp_buffer_unmap (&irtp);
    return gst_pad_push (self->srcpad, buffer);
  }

  self->num_received++;

  if (rtp_red_buffer_is_valid (self, &irtp, &first_red_payload_offset)) {
    GST_DEBUG_RTP_PACKET (self, "rtp header (red)", &irtp);
    ret = gst_rtp_red_process (self, &irtp, first_red_payload_offset);
  }

  gst_rtp_buffer_unmap (&irtp);
  gst_buffer_unref (buffer);
  return ret;
}

static void
gst_rtp_red_dec_dispose (GObject * obj)
{
  GstRtpRedDec *self = GST_RTP_RED_DEC (obj);

  g_queue_free_full (self->rtp_history, rtp_hist_item_free);

  G_OBJECT_CLASS (gst_rtp_red_dec_parent_class)->dispose (obj);
}

static void
gst_rtp_red_dec_init (GstRtpRedDec * self)
{
  GstPadTemplate *pad_template;

  pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (self), "src");
  self->srcpad = gst_pad_new_from_template (pad_template, "src");
  gst_element_add_pad (GST_ELEMENT_CAST (self), self->srcpad);

  pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (self), "sink");
  self->sinkpad = gst_pad_new_from_template (pad_template, "sink");
  gst_pad_set_chain_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_red_dec_chain));
  GST_PAD_SET_PROXY_CAPS (self->sinkpad);
  GST_PAD_SET_PROXY_ALLOCATION (self->sinkpad);
  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

  self->pt = DEFAULT_PT;
  self->num_received = 0;
  self->rtp_history = g_queue_new ();
}


static void
gst_rtp_red_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpRedDec *self = GST_RTP_RED_DEC (object);

  switch (prop_id) {
    case PROP_PT:
      self->pt = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_red_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpRedDec *self = GST_RTP_RED_DEC (object);
  switch (prop_id) {
    case PROP_PT:
      g_value_set_int (value, self->pt);
      break;
    case PROP_RECEIVED:
      g_value_set_uint (value, self->num_received);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_red_dec_class_init (GstRtpRedDecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;

  gobject_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));

  gst_element_class_set_metadata (element_class,
      "Redundant Audio Data (RED) Decoder",
      "Codec/Depayloader/Network/RTP",
      "Decode Redundant Audio Data (RED)",
      "Hani Mustafa <hani@pexip.com>, Mikhail Fludkov <misha@pexip.com>");

  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_rtp_red_dec_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_rtp_red_dec_get_property);
  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_rtp_red_dec_dispose);

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PT,
      g_param_spec_int ("pt", "payload type",
          "Payload type FEC packets",
          MIN_PT, MAX_PT, DEFAULT_PT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_RECEIVED,
      g_param_spec_uint ("received", "Received",
          "Count of received packets",
          0, G_MAXUINT32, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (gst_rtp_red_dec_debug, "rtpreddec", 0,
      "RTP RED Decoder");
}
