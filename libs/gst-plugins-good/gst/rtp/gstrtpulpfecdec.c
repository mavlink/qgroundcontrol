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
 * SECTION:element-rtpulpfecdec
 * @short_description: Generic RTP Forward Error Correction (FEC) decoder
 * @title: rtpulpfecdec
 *
 * Generic Forward Error Correction (FEC) decoder for Uneven Level
 * Protection (ULP) as described in RFC 5109.
 *
 * It differs from the RFC in one important way, it multiplexes the
 * FEC packets in the same sequence number as media packets. This is to be
 * compatible with libwebrtc as using in Google Chrome and with Microsoft
 * Lync / Skype for Business.
 *
 * This element will work in combination with an upstream #GstRtpStorage
 * element and attempt to recover packets declared lost through custom
 * 'GstRTPPacketLost' events, usually emitted by #GstRtpJitterBuffer.
 *
 * If no storage is provided using the #GstRtpUlpFecDec:storage
 * property, it will try to get it from an element upstream.
 *
 * Additionally, the payload types of the protection packets *must* be
 * provided to this element via its #GstRtpUlpFecDec:pt property.
 *
 * When using #GstRtpBin, this element should be inserted through the
 * #GstRtpBin::request-fec-decoder signal.
 *
 * ## Example pipeline
 *
 * |[
 * gst-launch-1.0 udpsrc port=8888 caps="application/x-rtp, payload=96, clock-rate=90000" ! rtpstorage size-time=220000000 ! rtpssrcdemux ! application/x-rtp, payload=96, clock-rate=90000, media=video, encoding-name=H264 ! rtpjitterbuffer do-lost=1 latency=200 !  rtpulpfecdec pt=122 ! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink
 * ]| This example will receive a stream with FEC and try to reconstruct the packets.
 *
 * Example programs are available at
 * <https://gitlab.freedesktop.org/gstreamer/gstreamer-rs/blob/master/examples/src/bin/rtpfecserver.rs>
 * and
 * <https://gitlab.freedesktop.org/gstreamer/gstreamer-rs/blob/master/examples/src/bin/rtpfecclient.rs>
 *
 * See also: #GstRtpUlpFecEnc, #GstRtpBin, #GstRtpStorage
 * Since: 1.14
 */

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/rtp/gstrtp-enumtypes.h>

#include "rtpulpfeccommon.h"
#include "gstrtpulpfecdec.h"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp")
    );

enum
{
  PROP_0,
  PROP_PT,
  PROP_STORAGE,
  PROP_RECOVERED,
  PROP_UNRECOVERED,
  N_PROPERTIES
};

#define DEFAULT_FEC_PT 0

static GParamSpec *klass_properties[N_PROPERTIES] = { NULL, };

GST_DEBUG_CATEGORY (gst_rtp_ulpfec_dec_debug);
#define GST_CAT_DEFAULT (gst_rtp_ulpfec_dec_debug)

G_DEFINE_TYPE (GstRtpUlpFecDec, gst_rtp_ulpfec_dec, GST_TYPE_ELEMENT);

#define RTP_FEC_MAP_INFO_NTH(dec, data) (&g_array_index (\
    ((GstRtpUlpFecDec *)dec)->info_arr, \
    RtpUlpFecMapInfo, \
    GPOINTER_TO_UINT(data)))

static gint
_compare_fec_map_info (gconstpointer a, gconstpointer b, gpointer userdata)
{
  guint16 aseq =
      gst_rtp_buffer_get_seq (&RTP_FEC_MAP_INFO_NTH (userdata, a)->rtp);
  guint16 bseq =
      gst_rtp_buffer_get_seq (&RTP_FEC_MAP_INFO_NTH (userdata, b)->rtp);
  return gst_rtp_buffer_compare_seqnum (bseq, aseq);
}

static void
gst_rtp_ulpfec_dec_start (GstRtpUlpFecDec * self, GstBufferList * buflist,
    guint8 fec_pt, guint16 lost_seq)
{
  guint fec_packets = 0;
  gsize i;

  g_assert (NULL == self->info_media);
  g_assert (0 == self->info_fec->len);
  g_assert (0 == self->info_arr->len);

  g_array_set_size (self->info_arr, gst_buffer_list_length (buflist));

  for (i = 0;
      i < gst_buffer_list_length (buflist) && !self->lost_packet_from_storage;
      ++i) {
    GstBuffer *buffer = gst_buffer_list_get (buflist, i);
    RtpUlpFecMapInfo *info = RTP_FEC_MAP_INFO_NTH (self, i);

    if (!rtp_ulpfec_map_info_map (gst_buffer_ref (buffer), info))
      g_assert_not_reached ();

    if (fec_pt == gst_rtp_buffer_get_payload_type (&info->rtp)) {
      GST_DEBUG_RTP_PACKET (self, "rtp header (fec)", &info->rtp);

      ++fec_packets;
      if (rtp_ulpfec_buffer_is_valid (&info->rtp)) {
        GST_DEBUG_FEC_PACKET (self, &info->rtp);
        g_ptr_array_add (self->info_fec, GUINT_TO_POINTER (i));
      }
    } else {
      GST_LOG_RTP_PACKET (self, "rtp header (incoming)", &info->rtp);

      if (lost_seq == gst_rtp_buffer_get_seq (&info->rtp)) {
        GST_DEBUG_OBJECT (self, "Received lost packet from the storage");
        g_list_free (self->info_media);
        self->info_media = NULL;
        self->lost_packet_from_storage = TRUE;
      }
      self->info_media =
          g_list_insert_sorted_with_data (self->info_media,
          GUINT_TO_POINTER (i), _compare_fec_map_info, self);
    }
  }
  if (!self->lost_packet_from_storage) {
    self->fec_packets_received += fec_packets;
    self->fec_packets_rejected += fec_packets - self->info_fec->len;
  }
}

static void
gst_rtp_ulpfec_dec_stop (GstRtpUlpFecDec * self)
{
  g_array_set_size (self->info_arr, 0);
  g_ptr_array_set_size (self->info_fec, 0);
  g_list_free (self->info_media);
  self->info_media = NULL;
  self->lost_packet_from_storage = FALSE;
  self->lost_packet_returned = FALSE;
}

static guint64
gst_rtp_ulpfec_dec_get_media_buffers_mask (GstRtpUlpFecDec * self,
    guint16 fec_seq_base)
{
  guint64 mask = 0;
  GList *it;

  for (it = self->info_media; it; it = it->next) {
    RtpUlpFecMapInfo *info = RTP_FEC_MAP_INFO_NTH (self, it->data);
    mask |=
        rtp_ulpfec_packet_mask_from_seqnum (gst_rtp_buffer_get_seq (&info->rtp),
        fec_seq_base, TRUE);
  }
  return mask;
}

static gboolean
gst_rtp_ulpfec_dec_is_recovered_pt_valid (GstRtpUlpFecDec * self, gint media_pt,
    guint8 recovered_pt)
{
  GList *it;
  if (media_pt == recovered_pt)
    return TRUE;

  for (it = self->info_media; it; it = it->next) {
    RtpUlpFecMapInfo *info = RTP_FEC_MAP_INFO_NTH (self, it->data);
    if (gst_rtp_buffer_get_payload_type (&info->rtp) == recovered_pt)
      return TRUE;
  }
  return FALSE;
}

static GstBuffer *
gst_rtp_ulpfec_dec_recover_from_fec (GstRtpUlpFecDec * self,
    RtpUlpFecMapInfo * info_fec, guint32 ssrc, gint media_pt, guint16 seq,
    guint8 * dst_pt)
{
  guint64 fec_mask = rtp_ulpfec_buffer_get_mask (&info_fec->rtp);
  gboolean fec_mask_long = rtp_ulpfec_buffer_get_fechdr (&info_fec->rtp)->L;
  guint16 fec_seq_base = rtp_ulpfec_buffer_get_seq_base (&info_fec->rtp);
  GstBuffer *ret;
  GList *it;

  g_array_set_size (self->scratch_buf, 0);
  rtp_buffer_to_ulpfec_bitstring (&info_fec->rtp, self->scratch_buf, TRUE,
      fec_mask_long);

  for (it = self->info_media; it; it = it->next) {
    RtpUlpFecMapInfo *info = RTP_FEC_MAP_INFO_NTH (self, it->data);
    guint64 packet_mask =
        rtp_ulpfec_packet_mask_from_seqnum (gst_rtp_buffer_get_seq (&info->rtp),
        fec_seq_base, TRUE);

    if (fec_mask & packet_mask) {
      fec_mask ^= packet_mask;
      rtp_buffer_to_ulpfec_bitstring (&info->rtp, self->scratch_buf, FALSE,
          fec_mask_long);
    }
  }

  ret =
      rtp_ulpfec_bitstring_to_media_rtp_buffer (self->scratch_buf,
      fec_mask_long, ssrc, seq);
  if (ret) {
    /* We are about to put recovered packet back in self->info_media to be able
     * to reuse it later for recovery of other packets
     **/
    gint i = self->info_arr->len;
    RtpUlpFecMapInfo *info;
    guint8 recovered_pt;

    g_array_set_size (self->info_arr, self->info_arr->len + 1);
    info = RTP_FEC_MAP_INFO_NTH (self, i);

    if (!rtp_ulpfec_map_info_map (gst_buffer_ref (ret), info)) {
      GST_WARNING_OBJECT (self, "Invalid recovered packet");
      goto recovered_packet_invalid;
    }

    recovered_pt = gst_rtp_buffer_get_payload_type (&info->rtp);
    if (!gst_rtp_ulpfec_dec_is_recovered_pt_valid (self, media_pt,
            recovered_pt)) {
      GST_WARNING_OBJECT (self,
          "Recovered packet has unexpected payload type (%u)", recovered_pt);
      goto recovered_packet_invalid;
    }

    GST_DEBUG_RTP_PACKET (self, "rtp header (recovered)", &info->rtp);
    self->info_media =
        g_list_insert_sorted_with_data (self->info_media, GUINT_TO_POINTER (i),
        _compare_fec_map_info, self);
    *dst_pt = recovered_pt;
  }
  return ret;

recovered_packet_invalid:
  g_array_set_size (self->info_arr, self->info_arr->len - 1);
  gst_buffer_unref (ret);
  return NULL;
}

static GstBuffer *
gst_rtp_ulpfec_dec_recover_from_storage (GstRtpUlpFecDec * self,
    guint8 * dst_pt, guint16 * dst_seq)
{
  RtpUlpFecMapInfo *info;

  if (self->lost_packet_returned)
    return NULL;

  g_assert (g_list_length (self->info_media) == 1);

  info = RTP_FEC_MAP_INFO_NTH (self, self->info_media->data);
  *dst_seq = gst_rtp_buffer_get_seq (&info->rtp);
  *dst_pt = gst_rtp_buffer_get_payload_type (&info->rtp);
  self->lost_packet_returned = TRUE;
  GST_DEBUG_RTP_PACKET (self, "rtp header (recovered)", &info->rtp);
  return gst_buffer_ref (info->rtp.buffer);
}

/* __has_builtin only works with clang, so test compiler version for gcc */
/* Intel compiler and MSVC probably have their own things as well */
/* TODO: make sure we use builtin for clang as well */
#if defined(__GNUC__) && __GNUC__ >= 4
#define rtp_ulpfec_ctz64 __builtin_ctzll
#else
static inline gint
rtp_ulpfec_ctz64_inline (guint64 mask)
{
  gint nth_bit = 0;

  do {
    if ((mask & 1))
      return nth_bit;
    mask = mask >> 1;
  } while (++nth_bit < 64);

  return -1;                    /* should not be reached, since mask must not be 0 */
}

#define rtp_ulpfec_ctz64 rtp_ulpfec_ctz64_inline
#endif

static GstBuffer *
gst_rtp_ulpfec_dec_recover (GstRtpUlpFecDec * self, guint32 ssrc, gint media_pt,
    guint8 * dst_pt, guint16 * dst_seq)
{
  guint64 media_mask = 0;
  gint media_mask_seq_base = -1;
  gsize i;

  if (self->lost_packet_from_storage)
    return gst_rtp_ulpfec_dec_recover_from_storage (self, dst_pt, dst_seq);

  /* Looking for a FEC packet which can be used for recovery */
  for (i = 0; i < self->info_fec->len; ++i) {
    RtpUlpFecMapInfo *info = RTP_FEC_MAP_INFO_NTH (self,
        g_ptr_array_index (self->info_fec, i));
    guint16 seq_base = rtp_ulpfec_buffer_get_seq_base (&info->rtp);
    guint64 fec_mask = rtp_ulpfec_buffer_get_mask (&info->rtp);
    guint64 missing_packets_mask;

    if (media_mask_seq_base != (gint) seq_base) {
      media_mask_seq_base = seq_base;
      media_mask = gst_rtp_ulpfec_dec_get_media_buffers_mask (self, seq_base);
    }

    /* media_mask has 1s if packet exist.
     * fec_mask is the mask of protected packets
     * The statement below excludes existing packets from the protected. So
     * we are left with 1s only for missing packets which can be recovered
     * by this FEC packet. */
    missing_packets_mask = fec_mask & (~media_mask);

    /* Do we have any 1s? Checking if current FEC packet can be used for recovery */
    if (0 != missing_packets_mask) {
      guint trailing_zeros = rtp_ulpfec_ctz64 (missing_packets_mask);

      /* Is it the only 1 in the mask? Checking if we lacking single packet in
       * that case FEC packet can be used for recovery */
      if (missing_packets_mask == (G_GUINT64_CONSTANT (1) << trailing_zeros)) {
        GstBuffer *ret;

        *dst_seq =
            seq_base + (RTP_ULPFEC_SEQ_BASE_OFFSET_MAX (TRUE) - trailing_zeros);
        ret =
            gst_rtp_ulpfec_dec_recover_from_fec (self, info, ssrc, media_pt,
            *dst_seq, dst_pt);
        if (ret)
          return ret;
      }
    }
  }
  return NULL;
}

static GstFlowReturn
gst_rtp_ulpfec_dec_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstRtpUlpFecDec *self = GST_RTP_ULPFEC_DEC (parent);

  if (G_LIKELY (GST_FLOW_OK == self->chain_return_val)) {
    GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
    buf = gst_buffer_make_writable (buf);

    if (G_UNLIKELY (self->unset_discont_flag)) {
      self->unset_discont_flag = FALSE;
      GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DISCONT);
    }

    gst_rtp_buffer_map (buf, GST_MAP_WRITE, &rtp);
    gst_rtp_buffer_set_seq (&rtp, self->next_seqnum++);
    gst_rtp_buffer_unmap (&rtp);

    return gst_pad_push (self->srcpad, buf);
  }

  gst_buffer_unref (buf);
  return self->chain_return_val;
}

static gboolean
gst_rtp_ulpfec_dec_handle_packet_loss (GstRtpUlpFecDec * self, guint16 seqnum,
    GstClockTime timestamp, GstClockTime duration)
{
  gint caps_pt = self->have_caps_pt ? self->caps_pt : -1;
  gboolean ret = TRUE;
  GstBufferList *buflist =
      rtp_storage_get_packets_for_recovery (self->storage, self->fec_pt,
      self->caps_ssrc, seqnum);

  if (buflist) {
    GstBuffer *recovered_buffer = NULL;
    guint16 recovered_seq = 0;
    guint8 recovered_pt = 0;

    gst_rtp_ulpfec_dec_start (self, buflist, self->fec_pt, seqnum);

    while (NULL != (recovered_buffer =
            gst_rtp_ulpfec_dec_recover (self, self->caps_ssrc, caps_pt,
                &recovered_pt, &recovered_seq))) {
      if (seqnum == recovered_seq) {
        GstBuffer *sent_buffer;
        GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

        recovered_buffer = gst_buffer_make_writable (recovered_buffer);
        GST_BUFFER_PTS (recovered_buffer) = timestamp;
        /* GST_BUFFER_DURATION (recovered_buffer) = duration;
         * JB does not set the duration, so we will not too */

        if (!self->lost_packet_from_storage)
          rtp_storage_put_recovered_packet (self->storage,
              recovered_buffer, recovered_pt, self->caps_ssrc, recovered_seq);

        GST_DEBUG_OBJECT (self,
            "Pushing recovered packet ssrc=0x%08x seq=%u %" GST_PTR_FORMAT,
            self->caps_ssrc, seqnum, recovered_buffer);

        sent_buffer = gst_buffer_copy_deep (recovered_buffer);

        if (self->lost_packet_from_storage)
          gst_buffer_unref (recovered_buffer);

        gst_rtp_buffer_map (sent_buffer, GST_MAP_WRITE, &rtp);
        gst_rtp_buffer_set_seq (&rtp, self->next_seqnum++);
        gst_rtp_buffer_unmap (&rtp);

        ret = FALSE;
        self->unset_discont_flag = TRUE;
        self->chain_return_val = gst_pad_push (self->srcpad, sent_buffer);
        break;
      }

      if (!self->lost_packet_from_storage) {
        rtp_storage_put_recovered_packet (self->storage,
            recovered_buffer, recovered_pt, self->caps_ssrc, recovered_seq);
      } else {
        gst_buffer_unref (recovered_buffer);
      }
    }

    gst_rtp_ulpfec_dec_stop (self);
    gst_buffer_list_unref (buflist);
  }

  GST_DEBUG_OBJECT (self, "Packet lost ssrc=0x%08x seq=%u", self->caps_ssrc,
      seqnum);

  return ret;
}

static gboolean
gst_rtp_ulpfec_dec_handle_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRtpUlpFecDec *self = GST_RTP_ULPFEC_DEC (parent);
  gboolean forward = TRUE;

  GST_LOG_OBJECT (self, "Received event %" GST_PTR_FORMAT, event);

  if (GST_FLOW_OK == self->chain_return_val &&
      GST_EVENT_CUSTOM_DOWNSTREAM == GST_EVENT_TYPE (event) &&
      gst_event_has_name (event, "GstRTPPacketLost")) {
    guint seqnum;
    GstClockTime timestamp, duration;
    GstStructure *s;

    event = gst_event_make_writable (event);
    s = gst_event_writable_structure (event);

    g_assert (self->have_caps_ssrc);

    if (self->storage == NULL) {
      GstQuery *q = gst_query_new_custom (GST_QUERY_CUSTOM,
          gst_structure_new_empty ("GstRtpStorage"));

      if (gst_pad_peer_query (self->sinkpad, q)) {
        const GstStructure *s = gst_query_get_structure (q);

        if (gst_structure_has_field_typed (s, "storage", G_TYPE_OBJECT)) {
          gst_structure_get (s, "storage", G_TYPE_OBJECT, &self->storage, NULL);
        }
      }
      gst_query_unref (q);
    }

    if (self->storage == NULL) {
      GST_ELEMENT_WARNING (self, STREAM, FAILED, ("Internal storage not found"),
          ("You need to add rtpstorage element upstream from rtpulpfecdec."));
      return FALSE;
    }

    if (!gst_structure_get (s,
            "seqnum", G_TYPE_UINT, &seqnum,
            "timestamp", G_TYPE_UINT64, &timestamp,
            "duration", G_TYPE_UINT64, &duration, NULL))
      g_assert_not_reached ();

    forward =
        gst_rtp_ulpfec_dec_handle_packet_loss (self, seqnum, timestamp,
        duration);

    if (forward) {
      gst_structure_remove_field (s, "seqnum");
      gst_structure_set (s, "might-have-been-fec", G_TYPE_BOOLEAN, TRUE, NULL);
      ++self->packets_unrecovered;
    } else {
      ++self->packets_recovered;
    }

    GST_DEBUG_OBJECT (self, "Unrecovered / Recovered: %lu / %lu",
        (gulong) self->packets_unrecovered, (gulong) self->packets_recovered);
  } else if (GST_EVENT_CAPS == GST_EVENT_TYPE (event)) {
    GstCaps *caps;
    gboolean have_caps_pt = FALSE;
    gboolean have_caps_ssrc = FALSE;
    guint caps_ssrc = 0;
    gint caps_pt = 0;

    gst_event_parse_caps (event, &caps);
    have_caps_ssrc =
        gst_structure_get_uint (gst_caps_get_structure (caps, 0), "ssrc",
        &caps_ssrc);
    have_caps_pt =
        gst_structure_get_int (gst_caps_get_structure (caps, 0), "payload",
        &caps_pt);

    if (self->have_caps_ssrc != have_caps_ssrc || self->caps_ssrc != caps_ssrc)
      GST_DEBUG_OBJECT (self, "SSRC changed %u, 0x%08x -> %u, 0x%08x",
          self->have_caps_ssrc, self->caps_ssrc, have_caps_ssrc, caps_ssrc);
    if (self->have_caps_pt != have_caps_pt || self->caps_pt != caps_pt)
      GST_DEBUG_OBJECT (self, "PT changed %u, %u -> %u, %u",
          self->have_caps_pt, self->caps_pt, have_caps_pt, caps_pt);

    self->have_caps_ssrc = have_caps_ssrc;
    self->have_caps_pt = have_caps_pt;
    self->caps_ssrc = caps_ssrc;
    self->caps_pt = caps_pt;
  }

  if (forward)
    return gst_pad_push_event (self->srcpad, event);
  gst_event_unref (event);
  return TRUE;
}

static void
gst_rtp_ulpfec_dec_init (GstRtpUlpFecDec * self)
{
  self->srcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  self->sinkpad = gst_pad_new_from_static_template (&sinktemplate, "sink");
  GST_PAD_SET_PROXY_CAPS (self->sinkpad);
  GST_PAD_SET_PROXY_ALLOCATION (self->sinkpad);
  gst_pad_set_chain_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_ulpfec_dec_chain));
  gst_pad_set_event_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_ulpfec_dec_handle_sink_event));

  gst_element_add_pad (GST_ELEMENT (self), self->srcpad);
  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

  self->fec_pt = DEFAULT_FEC_PT;

  self->next_seqnum = g_random_int_range (0, G_MAXINT16);

  self->chain_return_val = GST_FLOW_OK;
  self->have_caps_ssrc = FALSE;
  self->caps_ssrc = 0;
  self->info_fec = g_ptr_array_new ();
  self->info_arr = g_array_new (FALSE, TRUE, sizeof (RtpUlpFecMapInfo));
  g_array_set_clear_func (self->info_arr,
      (GDestroyNotify) rtp_ulpfec_map_info_unmap);
  self->scratch_buf = g_array_new (FALSE, TRUE, sizeof (guint8));
}

static void
gst_rtp_ulpfec_dec_dispose (GObject * obj)
{
  GstRtpUlpFecDec *self = GST_RTP_ULPFEC_DEC (obj);

  GST_INFO_OBJECT (self,
      " ssrc=0x%08x pt=%u"
      " packets_recovered=%" G_GSIZE_FORMAT
      " packets_unrecovered=%" G_GSIZE_FORMAT,
      self->caps_ssrc, self->caps_pt,
      self->packets_recovered, self->packets_unrecovered);

  if (self->storage)
    g_object_unref (self->storage);

  g_assert (NULL == self->info_media);
  g_assert (0 == self->info_fec->len);
  g_assert (0 == self->info_arr->len);

  if (self->fec_packets_received) {
    GST_INFO_OBJECT (self,
        " fec_packets_received=%" G_GSIZE_FORMAT
        " fec_packets_rejected=%" G_GSIZE_FORMAT
        " packets_rejected=%" G_GSIZE_FORMAT,
        self->fec_packets_received,
        self->fec_packets_rejected, self->packets_rejected);
  }

  g_ptr_array_free (self->info_fec, TRUE);
  g_array_free (self->info_arr, TRUE);
  g_array_free (self->scratch_buf, TRUE);

  G_OBJECT_CLASS (gst_rtp_ulpfec_dec_parent_class)->dispose (obj);
}

static void
gst_rtp_ulpfec_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpUlpFecDec *self = GST_RTP_ULPFEC_DEC (object);

  switch (prop_id) {
    case PROP_PT:
      self->fec_pt = g_value_get_uint (value);
      break;
    case PROP_STORAGE:
      if (self->storage)
        g_object_unref (self->storage);
      self->storage = g_value_get_object (value);
      if (self->storage)
        g_object_ref (self->storage);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_ulpfec_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpUlpFecDec *self = GST_RTP_ULPFEC_DEC (object);

  switch (prop_id) {
    case PROP_PT:
      g_value_set_uint (value, self->fec_pt);
      break;
    case PROP_STORAGE:
      g_value_set_object (value, self->storage);
      break;
    case PROP_RECOVERED:
      g_value_set_uint (value, (guint) self->packets_recovered);
      break;
    case PROP_UNRECOVERED:
      g_value_set_uint (value, (guint) self->packets_unrecovered);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_ulpfec_dec_class_init (GstRtpUlpFecDecClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_rtp_ulpfec_dec_debug,
      "rtpulpfecdec", 0, "RTP FEC Decoder");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sinktemplate));

  gst_element_class_set_static_metadata (element_class,
      "RTP FEC Decoder",
      "Codec/Depayloader/Network/RTP",
      "Decodes RTP FEC (RFC5109)", "Mikhail Fludkov <misha@pexip.com>");

  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_rtp_ulpfec_dec_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_rtp_ulpfec_dec_get_property);
  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_rtp_ulpfec_dec_dispose);

  klass_properties[PROP_PT] = g_param_spec_uint ("pt", "pt",
      "FEC packets payload type", 0, 127,
      DEFAULT_FEC_PT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  klass_properties[PROP_STORAGE] =
      g_param_spec_object ("storage", "RTP storage", "RTP storage",
      G_TYPE_OBJECT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  klass_properties[PROP_RECOVERED] =
      g_param_spec_uint ("recovered", "recovered",
      "The number of recovered packets", 0, G_MAXUINT, 0,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  klass_properties[PROP_UNRECOVERED] =
      g_param_spec_uint ("unrecovered", "unrecovered",
      "The number of unrecovered packets", 0, G_MAXUINT, 0,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES,
      klass_properties);

  g_assert (rtp_ulpfec_ctz64 (G_GUINT64_CONSTANT (0x1)) == 0);
  g_assert (rtp_ulpfec_ctz64 (G_GUINT64_CONSTANT (0x8000000000000000)) == 63);
}
