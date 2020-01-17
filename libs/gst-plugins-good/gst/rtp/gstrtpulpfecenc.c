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
 * SECTION:element-rtpulpfecenc
 * @short_description: Generic RTP Forward Error Correction (FEC) encoder
 * @title: rtpulpfecenc
 *
 * Generic Forward Error Correction (FEC) encoder using Uneven Level
 * Protection (ULP) as described in RFC 5109.
 *
 * It differs from the RFC in one important way, it multiplexes the
 * FEC packets in the same sequence number as media packets. This is to be
 * compatible with libwebrtc as using in Google Chrome and with Microsoft
 * Lync / Skype for Business.
 *
 * Be warned that after using this element, it is no longer possible to know if
 * there is a gap in the media stream based on the sequence numbers as the FEC
 * packets become interleaved with the media packets.
 *
 * This element will insert protection packets in any RTP stream, which
 * can then be used on the receiving side to recover lost packets.
 *
 * This element rewrites packets' seqnums, which means that when combined
 * with retransmission elements such as #GstRtpRtxSend, it *must* be
 * placed upstream of those, otherwise retransmission requests will request
 * incorrect seqnums.
 *
 * A payload type for the protection packets *must* be specified, different
 * from the payload type of the protected packets, with the GstRtpUlpFecEnc:pt
 * property.
 *
 * The marker bit of RTP packets is used to determine sets of packets to
 * protect as a unit, in order to modulate the level of protection, this
 * behaviour can be disabled with GstRtpUlpFecEnc:multipacket, but should
 * be left enabled for video streams.
 *
 * The level of protection can be configured with two properties,
 * #GstRtpUlpFecEnc:percentage and #GstRtpUlpFecEnc:percentage-important,
 * the element will determine which percentage to use for a given set of
 * packets based on the presence of the #GST_BUFFER_FLAG_NON_DROPPABLE
 * flag, upstream payloaders are expected to set this flag on "important"
 * packets such as those making up a keyframe.
 *
 * The percentage is expressed not in terms of bytes, but in terms of
 * packets, this for implementation convenience. The drawback with this
 * approach is that when using a percentage different from 100 %, and a
 * low bitrate, entire frames may be contained in a single packet, leading
 * to some packets not being protected, thus lowering the overall recovery
 * rate on the receiving side.
 *
 * When using #GstRtpBin, this element should be inserted through the
 * #GstRtpBin::request-fec-encoder signal.
 *
 * ## Example pipeline
 *
 * |[
 * gst-launch-1.0 videotestsrc ! x264enc ! video/x-h264, profile=baseline ! rtph264pay pt=96 ! rtpulpfecenc percentage=100 pt=122 ! udpsink port=8888
 * ]| This example will receive a stream with FEC and try to reconstruct the packets.
 *
 * Example programs are available at
 * <https://gitlab.freedesktop.org/gstreamer/gstreamer-rs/blob/master/examples/src/bin/rtpfecserver.rs>
 * and
 * <https://gitlab.freedesktop.org/gstreamer/gstreamer-rs/blob/master/examples/src/bin/rtpfecclient.rs>
 *
 * See also: #GstRtpUlpFecDec, #GstRtpBin
 * Since: 1.14
 */

#include <gst/rtp/gstrtp-enumtypes.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <string.h>

#include "rtpulpfeccommon.h"
#include "gstrtpulpfecenc.h"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp"));

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp"));

#define UNDEF_PT                255

#define DEFAULT_PT              UNDEF_PT
#define DEFAULT_PCT             0
#define DEFAULT_PCT_IMPORTANT   0
#define DEFAULT_MULTIPACKET     TRUE

#define PACKETS_BUF_MAX_LENGTH  (RTP_ULPFEC_PROTECTED_PACKETS_MAX(TRUE))

GST_DEBUG_CATEGORY (gst_rtp_ulpfec_enc_debug);
#define GST_CAT_DEFAULT (gst_rtp_ulpfec_enc_debug)

G_DEFINE_TYPE (GstRtpUlpFecEnc, gst_rtp_ulpfec_enc, GST_TYPE_ELEMENT);

enum
{
  PROP_0,
  PROP_PT,
  PROP_MULTIPACKET,
  PROP_PROTECTED,
  PROP_PERCENTAGE,
  PROP_PERCENTAGE_IMPORTANT,
};

#define RTP_FEC_MAP_INFO_NTH(ctx, data) (&g_array_index (\
    ((GstRtpUlpFecEncStreamCtx *)ctx)->info_arr, \
    RtpUlpFecMapInfo, \
    GPOINTER_TO_UINT(data)))

static void
gst_rtp_ulpfec_enc_stream_ctx_start (GstRtpUlpFecEncStreamCtx * ctx,
    GQueue * packets, guint fec_packets)
{
  GList *it = packets->tail;
  guint i;

  g_array_set_size (ctx->info_arr, packets->length);

  for (i = 0; i < packets->length; ++i) {
    GstBuffer *buffer = it->data;
    RtpUlpFecMapInfo *info = RTP_FEC_MAP_INFO_NTH (ctx, i);

    if (!rtp_ulpfec_map_info_map (gst_buffer_ref (buffer), info))
      g_assert_not_reached ();

    GST_LOG_RTP_PACKET (ctx->parent, "rtp header (incoming)", &info->rtp);

    it = g_list_previous (it);
  }

  ctx->fec_packets = fec_packets;
  ctx->fec_packet_idx = 0;
}

static void
gst_rtp_ulpfec_enc_stream_ctx_stop (GstRtpUlpFecEncStreamCtx * ctx)
{
  g_array_set_size (ctx->info_arr, 0);
  g_array_set_size (ctx->scratch_buf, 0);

  ctx->fec_packets = 0;
  ctx->fec_packet_idx = 0;
}

static void
    gst_rtp_ulpfec_enc_stream_ctx_get_protection_parameters
    (GstRtpUlpFecEncStreamCtx * ctx, guint16 * dst_seq_base, guint64 * dst_mask,
    guint * dst_start, guint * dst_end)
{
  guint media_packets = ctx->info_arr->len;
  guint start = ctx->fec_packet_idx * media_packets / ctx->fec_packets;
  guint end =
      ((ctx->fec_packet_idx + 1) * media_packets + ctx->fec_packets -
      1) / ctx->fec_packets - 1;
  guint len = end - start + 1;
  guint64 mask = 0;
  guint16 seq_base = 0;
  guint i;

  len = MIN (len, RTP_ULPFEC_PROTECTED_PACKETS_MAX (TRUE));
  end = start + len - 1;

  for (i = start; i <= end; ++i) {
    RtpUlpFecMapInfo *info = RTP_FEC_MAP_INFO_NTH (ctx, i);
    guint16 seq = gst_rtp_buffer_get_seq (&info->rtp);

    if (mask) {
      gint diff = gst_rtp_buffer_compare_seqnum (seq_base, seq);
      if (diff < 0) {
        seq_base = seq;
        mask = mask >> (-diff);
      }
      mask |= rtp_ulpfec_packet_mask_from_seqnum (seq, seq_base, TRUE);
    } else {
      seq_base = seq;
      mask = rtp_ulpfec_packet_mask_from_seqnum (seq, seq_base, TRUE);
    }
  }

  *dst_start = start;
  *dst_end = end;
  *dst_mask = mask;
  *dst_seq_base = seq_base;
}

static GstBuffer *
gst_rtp_ulpfec_enc_stream_ctx_protect (GstRtpUlpFecEncStreamCtx * ctx,
    guint8 pt, guint16 seq, guint32 timestamp, guint32 ssrc)
{
  guint end = 0;
  guint start = 0;
  guint64 fec_mask = 0;
  guint16 seq_base = 0;
  GstBuffer *ret;
  guint64 tmp_mask;
  gboolean fec_mask_long;
  guint i;

  if (ctx->fec_packet_idx >= ctx->fec_packets)
    return NULL;

  g_array_set_size (ctx->scratch_buf, 0);
  gst_rtp_ulpfec_enc_stream_ctx_get_protection_parameters (ctx, &seq_base,
      &fec_mask, &start, &end);

  tmp_mask = fec_mask;
  fec_mask_long = rtp_ulpfec_mask_is_long (fec_mask);
  for (i = start; i <= end; ++i) {
    RtpUlpFecMapInfo *info = RTP_FEC_MAP_INFO_NTH (ctx, i);
    guint64 packet_mask =
        rtp_ulpfec_packet_mask_from_seqnum (gst_rtp_buffer_get_seq (&info->rtp),
        seq_base,
        TRUE);

    if (tmp_mask & packet_mask) {
      tmp_mask ^= packet_mask;
      rtp_buffer_to_ulpfec_bitstring (&info->rtp, ctx->scratch_buf, FALSE,
          fec_mask_long);
    }
  }

  g_assert (tmp_mask == 0);
  ret =
      rtp_ulpfec_bitstring_to_fec_rtp_buffer (ctx->scratch_buf, seq_base,
      fec_mask_long, fec_mask, FALSE, pt, seq, timestamp, ssrc);
  ++ctx->fec_packet_idx;
  return ret;
}

static void
gst_rtp_ulpfec_enc_stream_ctx_report_budget (GstRtpUlpFecEncStreamCtx * ctx)
{
  GST_TRACE_OBJECT (ctx->parent, "budget = %f budget_important = %f",
      ctx->budget, ctx->budget_important);
}

static void
gst_rtp_ulpfec_enc_stream_ctx_increment_budget (GstRtpUlpFecEncStreamCtx * ctx,
    GstBuffer * buffer)
{
  if (ctx->percentage == 0 && ctx->percentage_important == 0) {
    if (ctx->budget > 0) {
      ctx->budget = 0;
      ctx->budget_important = 0;
    }
    if (ctx->budget < 0)
      ctx->budget += ctx->budget_inc;

    return;
  }
  ctx->budget += ctx->budget_inc;

  if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_NON_DROPPABLE)) {
    ctx->budget_important += ctx->budget_inc_important;
  }

  gst_rtp_ulpfec_enc_stream_ctx_report_budget (ctx);
}

static void
gst_rtp_ulpfec_enc_stream_ctx_decrement_budget (GstRtpUlpFecEncStreamCtx * ctx,
    guint fec_packets_num)
{
  if (ctx->budget_important >= 1.)
    ctx->budget_important -= fec_packets_num;
  ctx->budget -= fec_packets_num;

  gst_rtp_ulpfec_enc_stream_ctx_report_budget (ctx);
}

static guint
gst_rtp_ulpfec_enc_stream_ctx_get_fec_packets_num (GstRtpUlpFecEncStreamCtx *
    ctx)
{
  g_assert_cmpfloat (ctx->budget_important, >=, 0.);

  if (ctx->budget_important >= 1.)
    return ctx->budget_important;
  return ctx->budget > 0. ? (guint) ctx->budget : 0;
}

static void
gst_rtp_ulpfec_enc_stream_ctx_free_packets_buf (GstRtpUlpFecEncStreamCtx * ctx)
{
  while (ctx->packets_buf.length)
    gst_buffer_unref (g_queue_pop_tail (&ctx->packets_buf));
}

static void
gst_rtp_ulpfec_enc_stream_ctx_prepend_to_fec_buffer (GstRtpUlpFecEncStreamCtx *
    ctx, GstRTPBuffer * rtp, guint buf_max_size)
{
  GList *new_head;
  if (ctx->packets_buf.length == buf_max_size) {
    new_head = g_queue_pop_tail_link (&ctx->packets_buf);
  } else {
    new_head = g_list_alloc ();
  }

  gst_buffer_replace ((GstBuffer **) & new_head->data, rtp->buffer);
  g_queue_push_head_link (&ctx->packets_buf, new_head);

  g_assert_cmpint (ctx->packets_buf.length, <=, buf_max_size);
}

static GstFlowReturn
gst_rtp_ulpfec_enc_stream_ctx_push_fec_packets (GstRtpUlpFecEncStreamCtx * ctx,
    guint8 pt, guint16 seq, guint32 timestamp, guint32 ssrc)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint fec_packets_num =
      gst_rtp_ulpfec_enc_stream_ctx_get_fec_packets_num (ctx);

  if (fec_packets_num) {
    guint fec_packets_pushed = 0;
    GstBuffer *latest_packet = ctx->packets_buf.head->data;
    GstBuffer *fec = NULL;

    gst_rtp_ulpfec_enc_stream_ctx_start (ctx, &ctx->packets_buf,
        fec_packets_num);

    while (NULL != (fec =
            gst_rtp_ulpfec_enc_stream_ctx_protect (ctx, pt,
                seq + fec_packets_pushed, timestamp, ssrc))) {
      gst_buffer_copy_into (fec, latest_packet, GST_BUFFER_COPY_TIMESTAMPS, 0,
          -1);

      ret = gst_pad_push (ctx->srcpad, fec);
      if (GST_FLOW_OK == ret)
        ++fec_packets_pushed;
      else
        break;
    }

    gst_rtp_ulpfec_enc_stream_ctx_stop (ctx);

    g_assert_cmpint (fec_packets_pushed, <=, fec_packets_num);

    ctx->num_packets_protected += ctx->packets_buf.length;
    ctx->num_packets_fec += fec_packets_pushed;
    ctx->seqnum_offset += fec_packets_pushed;
    ctx->seqnum += fec_packets_pushed;
  }

  gst_rtp_ulpfec_enc_stream_ctx_decrement_budget (ctx, fec_packets_num);
  return ret;
}

static void
gst_rtp_ulpfec_enc_stream_ctx_cache_packet (GstRtpUlpFecEncStreamCtx * ctx,
    GstRTPBuffer * rtp, gboolean * dst_empty_packet_buffer,
    gboolean * dst_push_fec)
{
  if (ctx->multipacket) {
    gst_rtp_ulpfec_enc_stream_ctx_prepend_to_fec_buffer (ctx, rtp,
        PACKETS_BUF_MAX_LENGTH);
    gst_rtp_ulpfec_enc_stream_ctx_increment_budget (ctx, rtp->buffer);

    *dst_empty_packet_buffer = gst_rtp_buffer_get_marker (rtp);
    *dst_push_fec = *dst_empty_packet_buffer;
  } else {
    gboolean push_fec;

    gst_rtp_ulpfec_enc_stream_ctx_prepend_to_fec_buffer (ctx, rtp, 1);

    push_fec = ctx->fec_nth == 0 ? FALSE :
        0 == (ctx->num_packets_received % ctx->fec_nth);

    ctx->budget = push_fec ? 1 : 0;
    ctx->budget_important = 0;

    *dst_push_fec = push_fec;
    *dst_empty_packet_buffer = FALSE;
  }
}

static void
gst_rtp_ulpfec_enc_stream_ctx_configure (GstRtpUlpFecEncStreamCtx * ctx,
    guint pt, guint percentage, guint percentage_important,
    gboolean multipacket)
{
  ctx->pt = pt;
  ctx->percentage = percentage;
  ctx->percentage_important = percentage_important;
  ctx->multipacket = multipacket;

  ctx->fec_nth = percentage ? 100 / percentage : 0;
  if (percentage) {
    ctx->budget_inc = percentage / 100.;
    ctx->budget_inc_important = percentage > percentage_important ?
        ctx->budget_inc : percentage_important / 100.;
  }
/*
   else {
    ctx->budget_inc = 0.0;
  }
*/
  ctx->budget_inc_important = percentage > percentage_important ?
      ctx->budget_inc : percentage_important / 100.;
}

static GstRtpUlpFecEncStreamCtx *
gst_rtp_ulpfec_enc_stream_ctx_new (guint ssrc,
    GstElement * parent, GstPad * srcpad,
    guint pt, guint percentage, guint percentage_important,
    gboolean multipacket)
{
  GstRtpUlpFecEncStreamCtx *ctx = g_new0 (GstRtpUlpFecEncStreamCtx, 1);

  ctx->ssrc = ssrc;
  ctx->parent = parent;
  ctx->srcpad = srcpad;

  ctx->seqnum = g_random_int_range (0, G_MAXUINT16 / 2);

  ctx->info_arr = g_array_new (FALSE, TRUE, sizeof (RtpUlpFecMapInfo));
  g_array_set_clear_func (ctx->info_arr,
      (GDestroyNotify) rtp_ulpfec_map_info_unmap);
  ctx->parent = parent;
  ctx->scratch_buf = g_array_new (FALSE, TRUE, sizeof (guint8));
  gst_rtp_ulpfec_enc_stream_ctx_configure (ctx, pt,
      percentage, percentage_important, multipacket);

  return ctx;
}

static void
gst_rtp_ulpfec_enc_stream_ctx_free (GstRtpUlpFecEncStreamCtx * ctx)
{
  if (ctx->num_packets_received) {
    GST_INFO_OBJECT (ctx->parent, "Actual FEC overhead is %4.2f%% (%u/%u)\n",
        ctx->num_packets_fec * (double) 100. / ctx->num_packets_received,
        ctx->num_packets_fec, ctx->num_packets_received);
  }
  gst_rtp_ulpfec_enc_stream_ctx_free_packets_buf (ctx);

  g_assert (0 == ctx->info_arr->len);
  g_array_free (ctx->info_arr, TRUE);
  g_array_free (ctx->scratch_buf, TRUE);
  g_slice_free1 (sizeof (GstRtpUlpFecEncStreamCtx), ctx);
}

static GstFlowReturn
gst_rtp_ulpfec_enc_stream_ctx_process (GstRtpUlpFecEncStreamCtx * ctx,
    GstBuffer * buffer)
{
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstFlowReturn ret;
  gboolean push_fec = FALSE;
  gboolean empty_packet_buffer = FALSE;

  ctx->num_packets_received++;

  if (ctx->seqnum_offset > 0) {
    buffer = gst_buffer_make_writable (buffer);
    if (!gst_rtp_buffer_map (buffer,
            GST_MAP_READWRITE | GST_RTP_BUFFER_MAP_FLAG_SKIP_PADDING, &rtp))
      g_assert_not_reached ();
    gst_rtp_buffer_set_seq (&rtp,
        gst_rtp_buffer_get_seq (&rtp) + ctx->seqnum_offset);
  } else {
    if (!gst_rtp_buffer_map (buffer,
            GST_MAP_READ | GST_RTP_BUFFER_MAP_FLAG_SKIP_PADDING, &rtp))
      g_assert_not_reached ();
  }

  gst_rtp_ulpfec_enc_stream_ctx_cache_packet (ctx, &rtp, &empty_packet_buffer,
      &push_fec);

  if (push_fec) {
    guint32 fec_timestamp = gst_rtp_buffer_get_timestamp (&rtp);
    guint32 fec_ssrc = gst_rtp_buffer_get_ssrc (&rtp);
    guint16 fec_seq = gst_rtp_buffer_get_seq (&rtp) + 1;

    gst_rtp_buffer_unmap (&rtp);

    ret = gst_pad_push (ctx->srcpad, buffer);
    if (GST_FLOW_OK == ret)
      ret =
          gst_rtp_ulpfec_enc_stream_ctx_push_fec_packets (ctx, ctx->pt, fec_seq,
          fec_timestamp, fec_ssrc);
  } else {
    gst_rtp_buffer_unmap (&rtp);
    ret = gst_pad_push (ctx->srcpad, buffer);
  }

  if (empty_packet_buffer)
    gst_rtp_ulpfec_enc_stream_ctx_free_packets_buf (ctx);

  return ret;
}

static GstRtpUlpFecEncStreamCtx *
gst_rtp_ulpfec_enc_aquire_ctx (GstRtpUlpFecEnc * fec, guint ssrc)
{
  GstRtpUlpFecEncStreamCtx *ctx;

  GST_OBJECT_LOCK (fec);
  ctx = g_hash_table_lookup (fec->ssrc_to_ctx, GUINT_TO_POINTER (ssrc));
  if (ctx == NULL) {
    ctx =
        gst_rtp_ulpfec_enc_stream_ctx_new (ssrc, GST_ELEMENT_CAST (fec),
        fec->srcpad, fec->pt, fec->percentage,
        fec->percentage_important, fec->multipacket);
    g_hash_table_insert (fec->ssrc_to_ctx, GUINT_TO_POINTER (ssrc), ctx);
  }
  GST_OBJECT_UNLOCK (fec);

  return ctx;
}

static GstFlowReturn
gst_rtp_ulpfec_enc_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstRtpUlpFecEnc *fec = GST_RTP_ULPFEC_ENC (parent);
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstFlowReturn ret;
  guint ssrc = 0;
  GstRtpUlpFecEncStreamCtx *ctx;

  if (fec->pt == UNDEF_PT)
    return gst_pad_push (fec->srcpad, buffer);

  /* FIXME: avoid this additional mapping of the buffer to get the
     ssrc! */
  if (!gst_rtp_buffer_map (buffer,
          GST_MAP_READ | GST_RTP_BUFFER_MAP_FLAG_SKIP_PADDING, &rtp)) {
    g_assert_not_reached ();
  }
  ssrc = gst_rtp_buffer_get_ssrc (&rtp);
  gst_rtp_buffer_unmap (&rtp);

  ctx = gst_rtp_ulpfec_enc_aquire_ctx (fec, ssrc);

  ret = gst_rtp_ulpfec_enc_stream_ctx_process (ctx, buffer);

  /* FIXME: does not work for multiple ssrcs */
  fec->num_packets_protected = ctx->num_packets_protected;

  return ret;
}

static void
gst_rtp_ulpfec_enc_configure_ctx (gpointer key, gpointer value,
    gpointer user_data)
{
  GstRtpUlpFecEnc *fec = user_data;
  GstRtpUlpFecEncStreamCtx *ctx = value;

  gst_rtp_ulpfec_enc_stream_ctx_configure (ctx, fec->pt,
      fec->percentage, fec->percentage_important, fec->multipacket);
}

static void
gst_rtp_ulpfec_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpUlpFecEnc *fec = GST_RTP_ULPFEC_ENC (object);

  switch (prop_id) {
    case PROP_PT:
      fec->pt = g_value_get_uint (value);
      break;
    case PROP_MULTIPACKET:
      fec->multipacket = g_value_get_boolean (value);
      break;
    case PROP_PERCENTAGE:
      fec->percentage = g_value_get_uint (value);
      break;
    case PROP_PERCENTAGE_IMPORTANT:
      fec->percentage_important = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  GST_OBJECT_LOCK (fec);
  g_hash_table_foreach (fec->ssrc_to_ctx, gst_rtp_ulpfec_enc_configure_ctx,
      fec);
  GST_OBJECT_UNLOCK (fec);
}

static void
gst_rtp_ulpfec_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpUlpFecEnc *fec = GST_RTP_ULPFEC_ENC (object);
  switch (prop_id) {
    case PROP_PT:
      g_value_set_uint (value, fec->pt);
      break;
    case PROP_PROTECTED:
      g_value_set_uint (value, fec->num_packets_protected);
      break;
    case PROP_PERCENTAGE:
      g_value_set_uint (value, fec->percentage);
      break;
    case PROP_PERCENTAGE_IMPORTANT:
      g_value_set_uint (value, fec->percentage_important);
      break;
    case PROP_MULTIPACKET:
      g_value_set_boolean (value, fec->multipacket);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_ulpfec_enc_dispose (GObject * obj)
{
  GstRtpUlpFecEnc *fec = GST_RTP_ULPFEC_ENC (obj);

  g_hash_table_destroy (fec->ssrc_to_ctx);

  G_OBJECT_CLASS (gst_rtp_ulpfec_enc_parent_class)->dispose (obj);
}

static void
gst_rtp_ulpfec_enc_init (GstRtpUlpFecEnc * fec)
{
  fec->srcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  gst_element_add_pad (GST_ELEMENT (fec), fec->srcpad);

  fec->sinkpad = gst_pad_new_from_static_template (&sinktemplate, "sink");
  GST_PAD_SET_PROXY_CAPS (fec->sinkpad);
  GST_PAD_SET_PROXY_ALLOCATION (fec->sinkpad);
  gst_pad_set_chain_function (fec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_ulpfec_enc_chain));
  gst_element_add_pad (GST_ELEMENT (fec), fec->sinkpad);

  fec->ssrc_to_ctx = g_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) gst_rtp_ulpfec_enc_stream_ctx_free);
}

static void
gst_rtp_ulpfec_enc_class_init (GstRtpUlpFecEncClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_rtp_ulpfec_enc_debug, "rtpulpfecenc", 0,
      "FEC encoder element");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sinktemplate));

  gst_element_class_set_static_metadata (element_class,
      "RTP FEC Encoder",
      "Codec/Payloader/Network/RTP",
      "Encodes RTP FEC (RFC5109)", "Mikhail Fludkov <misha@pexip.com>");

  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_rtp_ulpfec_enc_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_rtp_ulpfec_enc_get_property);
  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_rtp_ulpfec_enc_dispose);

  g_object_class_install_property (gobject_class, PROP_PT,
      g_param_spec_uint ("pt", "payload type",
          "The payload type of FEC packets", 0, 255, DEFAULT_PT,
          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MULTIPACKET,
      g_param_spec_boolean ("multipacket", "Multipacket",
          "Apply FEC on multiple packets", DEFAULT_MULTIPACKET,
          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PERCENTAGE,
      g_param_spec_uint ("percentage", "Percentage",
          "FEC overhead percentage for the whole stream", 0, 100, DEFAULT_PCT,
          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PERCENTAGE_IMPORTANT,
      g_param_spec_uint ("percentage-important", "Percentage important",
          "FEC overhead percentage for important packets",
          0, 100, DEFAULT_PCT_IMPORTANT,
          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROTECTED,
      g_param_spec_uint ("protected", "Protected",
          "Count of protected packets", 0, G_MAXUINT32, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
