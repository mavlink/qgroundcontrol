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

#include <string.h>
#include "rtpulpfeccommon.h"

#define MIN_RTP_HEADER_LEN 12

typedef struct
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  unsigned int csrc_count:4;    /* CSRC count */
  unsigned int extension:1;     /* header extension flag */
  unsigned int padding:1;       /* padding flag */
  unsigned int version:2;       /* protocol version */
  unsigned int payload_type:7;  /* payload type */
  unsigned int marker:1;        /* marker bit */
#elif G_BYTE_ORDER == G_BIG_ENDIAN
  unsigned int version:2;       /* protocol version */
  unsigned int padding:1;       /* padding flag */
  unsigned int extension:1;     /* header extension flag */
  unsigned int csrc_count:4;    /* CSRC count */
  unsigned int marker:1;        /* marker bit */
  unsigned int payload_type:7;  /* payload type */
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
  unsigned int seq:16;          /* sequence number */
  unsigned int timestamp:32;    /* timestamp */
  unsigned int ssrc:32;         /* synchronization source */
  guint8 csrclist[4];           /* optional CSRC list, 32 bits each */
} RtpHeader;

static gsize
fec_level_hdr_get_size (gboolean l_bit)
{
  return sizeof (RtpUlpFecLevelHeader) - (l_bit ? 0 : 4);
}

static guint64
fec_level_hdr_get_mask (RtpUlpFecLevelHeader const *fec_lvl_hdr, gboolean l_bit)
{
  return ((guint64) g_ntohs (fec_lvl_hdr->mask) << 32) |
      (l_bit ? g_ntohl (fec_lvl_hdr->mask_continued) : 0);
}

static void
fec_level_hdr_set_mask (RtpUlpFecLevelHeader * fec_lvl_hdr, gboolean l_bit,
    guint64 mask)
{
  fec_lvl_hdr->mask = g_htons (mask >> 32);
  if (l_bit)
    fec_lvl_hdr->mask_continued = g_htonl (mask);
}

static guint16
fec_level_hdr_get_protection_len (RtpUlpFecLevelHeader * fec_lvl_hdr)
{
  return g_ntohs (fec_lvl_hdr->protection_len);
}

static void
fec_level_hdr_set_protection_len (RtpUlpFecLevelHeader * fec_lvl_hdr,
    guint16 len)
{
  fec_lvl_hdr->protection_len = g_htons (len);
}

static RtpUlpFecLevelHeader *
fec_hdr_get_level_hdr (RtpUlpFecHeader const *fec_hdr)
{
  return (RtpUlpFecLevelHeader *) (fec_hdr + 1);
}

static guint64
fec_hdr_get_mask (RtpUlpFecHeader const *fec_hdr)
{
  return fec_level_hdr_get_mask (fec_hdr_get_level_hdr (fec_hdr), fec_hdr->L);
}

static guint16
fec_hdr_get_seq_base (RtpUlpFecHeader const *fec_hdr, gboolean is_ulpfec,
    guint16 fec_seq)
{
  guint16 seq = g_ntohs (fec_hdr->seq);
  if (is_ulpfec)
    return seq;
  return fec_seq - seq;
}

static guint16
fec_hdr_get_packets_len_recovery (RtpUlpFecHeader const *fec_hdr)
{
  return g_htons (fec_hdr->len);
}

static guint32
fec_hdr_get_timestamp_recovery (RtpUlpFecHeader const *fec_hdr)
{
  return g_ntohl (fec_hdr->timestamp);
}

static void
_xor_mem (guint8 * restrict dst, const guint8 * restrict src, gsize length)
{
  guint i;

  for (i = 0; i < (length / sizeof (guint64)); ++i) {
    *((guint64 *) dst) ^= *((const guint64 *) src);
    dst += sizeof (guint64);
    src += sizeof (guint64);
  }
  for (i = 0; i < (length % sizeof (guint64)); ++i)
    dst[i] ^= src[i];
}

guint16
rtp_ulpfec_hdr_get_protection_len (RtpUlpFecHeader const *fec_hdr)
{
  return fec_level_hdr_get_protection_len (fec_hdr_get_level_hdr (fec_hdr));
}

RtpUlpFecHeader *
rtp_ulpfec_buffer_get_fechdr (GstRTPBuffer * rtp)
{
  return (RtpUlpFecHeader *) gst_rtp_buffer_get_payload (rtp);
}

guint64
rtp_ulpfec_buffer_get_mask (GstRTPBuffer * rtp)
{
  return fec_hdr_get_mask (rtp_ulpfec_buffer_get_fechdr (rtp));
}

guint16
rtp_ulpfec_buffer_get_seq_base (GstRTPBuffer * rtp)
{
  return g_ntohs (rtp_ulpfec_buffer_get_fechdr (rtp)->seq);
}

guint
rtp_ulpfec_get_headers_len (gboolean fec_mask_long)
{
  return sizeof (RtpUlpFecHeader) + fec_level_hdr_get_size (fec_mask_long);
}

#define ONE_64BIT G_GUINT64_CONSTANT(1)

guint64
rtp_ulpfec_packet_mask_from_seqnum (guint16 seq,
    guint16 fec_seq_base, gboolean fec_mask_long)
{
  gint seq_delta = gst_rtp_buffer_compare_seqnum (fec_seq_base, seq);
  if (seq_delta >= 0
      && seq_delta <= RTP_ULPFEC_SEQ_BASE_OFFSET_MAX (fec_mask_long)) {
    return ONE_64BIT << (RTP_ULPFEC_SEQ_BASE_OFFSET_MAX (TRUE) - seq_delta);
  }
  return 0;
}

gboolean
rtp_ulpfec_mask_is_long (guint64 mask)
{
  return (mask & 0xffffffff) ? TRUE : FALSE;
}

gboolean
rtp_ulpfec_buffer_is_valid (GstRTPBuffer * rtp)
{
  guint payload_len = gst_rtp_buffer_get_payload_len (rtp);
  RtpUlpFecHeader *fec_hdr;
  guint fec_hdrs_len;
  guint fec_packet_len;

  if (payload_len < sizeof (RtpUlpFecHeader))
    goto toosmall;

  fec_hdr = rtp_ulpfec_buffer_get_fechdr (rtp);
  if (fec_hdr->E)
    goto invalidcontent;

  fec_hdrs_len = rtp_ulpfec_get_headers_len (fec_hdr->L);
  if (payload_len < fec_hdrs_len)
    goto toosmall;

  fec_packet_len = fec_hdrs_len + rtp_ulpfec_hdr_get_protection_len (fec_hdr);
  if (fec_packet_len != payload_len)
    goto lengthmismatch;

  return TRUE;
toosmall:
  GST_WARNING ("FEC packet too small");
  return FALSE;

lengthmismatch:
  GST_WARNING ("invalid FEC packet (declared length %u, real length %u)",
      fec_packet_len, payload_len);
  return FALSE;

invalidcontent:
  GST_WARNING ("FEC Header contains invalid fields: %u", fec_hdr->E);
  return FALSE;
}


void
rtp_buffer_to_ulpfec_bitstring (GstRTPBuffer * rtp, GArray * dst_arr,
    gboolean fec_buffer, gboolean fec_mask_long)
{
  if (G_UNLIKELY (fec_buffer)) {
    guint payload_len = gst_rtp_buffer_get_payload_len (rtp);
    g_array_set_size (dst_arr, MAX (payload_len, dst_arr->len));
    memcpy (dst_arr->data, gst_rtp_buffer_get_payload (rtp), payload_len);
  } else {
    const guint8 *src = rtp->data[0];
    guint len = gst_rtp_buffer_get_packet_len (rtp) - MIN_RTP_HEADER_LEN;
    guint dst_offset = rtp_ulpfec_get_headers_len (fec_mask_long);
    guint src_offset = MIN_RTP_HEADER_LEN;
    guint8 *dst;

    g_array_set_size (dst_arr, MAX (dst_offset + len, dst_arr->len));
    dst = (guint8 *) dst_arr->data;

    *((guint64 *) dst) ^= *((const guint64 *) src);
    ((RtpUlpFecHeader *) dst)->len ^= g_htons (len);
    _xor_mem (dst + dst_offset, src + src_offset, len);
  }
}

GstBuffer *
rtp_ulpfec_bitstring_to_media_rtp_buffer (GArray * arr,
    gboolean fec_mask_long, guint32 ssrc, guint16 seq)
{
  guint fec_hdrs_len = rtp_ulpfec_get_headers_len (fec_mask_long);
  guint payload_len =
      fec_hdr_get_packets_len_recovery ((RtpUlpFecHeader *) arr->data);
  GstMapInfo ret_info = GST_MAP_INFO_INIT;
  GstMemory *ret_mem;
  GstBuffer *ret;

  if (payload_len > arr->len - fec_hdrs_len)
    return NULL;                // Not enough data

  ret_mem = gst_allocator_alloc (NULL, MIN_RTP_HEADER_LEN + payload_len, NULL);
  gst_memory_map (ret_mem, &ret_info, GST_MAP_READWRITE);

  /* Filling 12 bytes of RTP header */
  *((guint64 *) ret_info.data) = *((guint64 *) arr->data);
  ((RtpHeader *) ret_info.data)->version = 2;
  ((RtpHeader *) ret_info.data)->seq = g_htons (seq);
  ((RtpHeader *) ret_info.data)->ssrc = g_htonl (ssrc);
  /* Filling payload */
  memcpy (ret_info.data + MIN_RTP_HEADER_LEN,
      arr->data + fec_hdrs_len, payload_len);

  gst_memory_unmap (ret_mem, &ret_info);
  ret = gst_buffer_new ();
  gst_buffer_append_memory (ret, ret_mem);
  return ret;
}

GstBuffer *
rtp_ulpfec_bitstring_to_fec_rtp_buffer (GArray * arr,
    guint16 seq_base, gboolean fec_mask_long, guint64 fec_mask,
    gboolean marker, guint8 pt, guint16 seq, guint32 timestamp, guint32 ssrc)
{
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstBuffer *ret;

  /* Filling FEC headers */
  {
    RtpUlpFecHeader *hdr = (RtpUlpFecHeader *) arr->data;
    RtpUlpFecLevelHeader *lvlhdr;
    hdr->E = 0;
    hdr->L = fec_mask_long;
    hdr->seq = g_htons (seq_base);

    lvlhdr = fec_hdr_get_level_hdr (hdr);
    fec_level_hdr_set_protection_len (lvlhdr,
        arr->len - rtp_ulpfec_get_headers_len (fec_mask_long));
    fec_level_hdr_set_mask (lvlhdr, fec_mask_long, fec_mask);
  }

  /* Filling RTP header, copying payload */
  ret = gst_rtp_buffer_new_allocate (arr->len, 0, 0);
  if (!gst_rtp_buffer_map (ret, GST_MAP_READWRITE, &rtp))
    g_assert_not_reached ();

  gst_rtp_buffer_set_marker (&rtp, marker);
  gst_rtp_buffer_set_payload_type (&rtp, pt);
  gst_rtp_buffer_set_seq (&rtp, seq);
  gst_rtp_buffer_set_timestamp (&rtp, timestamp);
  gst_rtp_buffer_set_ssrc (&rtp, ssrc);

  memcpy (gst_rtp_buffer_get_payload (&rtp), arr->data, arr->len);

  gst_rtp_buffer_unmap (&rtp);

  return ret;
}

/**
 * rtp_ulpfec_map_info_map:
 * @buffer: (transfer: full) #GstBuffer
 * @info: #RtpUlpFecMapInfo
 *
 * Maps the contents of @buffer into @info. If @buffer made of many #GstMemory
 * objects, merges them together to create a new buffer made of single
 * continious #GstMemory.
 *
 * Returns: %TRUE if @buffer could be mapped
 **/
gboolean
rtp_ulpfec_map_info_map (GstBuffer * buffer, RtpUlpFecMapInfo * info)
{
  /* We need to make sure we are working with continious memory chunk.
   * If not merge all memories together */
  if (gst_buffer_n_memory (buffer) > 1) {
    GstBuffer *new_buffer = gst_buffer_new ();
    GstMemory *mem = gst_buffer_get_all_memory (buffer);
    gst_buffer_append_memory (new_buffer, mem);

    /* We supposed to own the old buffer, but we don't use it here, so unref */
    gst_buffer_unref (buffer);
    buffer = new_buffer;
  }

  if (!gst_rtp_buffer_map (buffer,
          GST_MAP_READ | GST_RTP_BUFFER_MAP_FLAG_SKIP_PADDING, &info->rtp)) {
    /* info->rtp.buffer = NULL is an indication for rtp_ulpfec_map_info_unmap()
     * that mapping has failed */
    g_assert (NULL == info->rtp.buffer);
    gst_buffer_unref (buffer);
    return FALSE;
  }
  return TRUE;
}

/**
 * rtp_ulpfec_map_info_unmap:
 * @info: #RtpUlpFecMapInfo
 *
 * Unmap @info previously mapped with rtp_ulpfec_map_info_map() and unrefs the
 * buffer. For convenience can even be called even if rtp_ulpfec_map_info_map
 * returned FALSE
 **/
void
rtp_ulpfec_map_info_unmap (RtpUlpFecMapInfo * info)
{
  GstBuffer *buffer = info->rtp.buffer;

  if (buffer) {
    gst_rtp_buffer_unmap (&info->rtp);
    gst_buffer_unref (buffer);
  }
}

#ifndef GST_DISABLE_GST_DEBUG
void
rtp_ulpfec_log_rtppacket (GstDebugCategory * cat, GstDebugLevel level,
    gpointer object, const gchar * name, GstRTPBuffer * rtp)
{
  guint seq;
  guint ssrc;
  guint timestamp;
  guint pt;

  if (level > gst_debug_category_get_threshold (cat))
    return;

  seq = gst_rtp_buffer_get_seq (rtp);
  ssrc = gst_rtp_buffer_get_ssrc (rtp);
  timestamp = gst_rtp_buffer_get_timestamp (rtp);
  pt = gst_rtp_buffer_get_payload_type (rtp);

  GST_CAT_LEVEL_LOG (cat, level, object,
      "%-22s: [%c%c%c%c] ssrc=0x%08x pt=%u tstamp=%u seq=%u size=%u(%u,%u)",
      name,
      gst_rtp_buffer_get_marker (rtp) ? 'M' : ' ',
      gst_rtp_buffer_get_extension (rtp) ? 'X' : ' ',
      gst_rtp_buffer_get_padding (rtp) ? 'P' : ' ',
      gst_rtp_buffer_get_csrc_count (rtp) > 0 ? 'C' : ' ',
      ssrc, pt, timestamp, seq,
      gst_rtp_buffer_get_packet_len (rtp),
      gst_rtp_buffer_get_packet_len (rtp) - MIN_RTP_HEADER_LEN,
      gst_rtp_buffer_get_payload_len (rtp));
}
#endif /* GST_DISABLE_GST_DEBUG */

#ifndef GST_DISABLE_GST_DEBUG
void
rtp_ulpfec_log_fec_packet (GstDebugCategory * cat, GstDebugLevel level,
    gpointer object, GstRTPBuffer * fecrtp)
{
  RtpUlpFecHeader *fec_hdr;
  RtpUlpFecLevelHeader *fec_level_hdr;

  if (level > gst_debug_category_get_threshold (cat))
    return;

  fec_hdr = gst_rtp_buffer_get_payload (fecrtp);
  GST_CAT_LEVEL_LOG (cat, level, object,
      "%-22s: [%c%c%c%c%c%c] pt=%u tstamp=%u seq=%u recovery_len=%u",
      "fec header",
      fec_hdr->E ? 'E' : ' ',
      fec_hdr->L ? 'L' : ' ',
      fec_hdr->P ? 'P' : ' ',
      fec_hdr->X ? 'X' : ' ',
      fec_hdr->CC ? 'C' : ' ',
      fec_hdr->M ? 'M' : ' ',
      fec_hdr->pt,
      fec_hdr_get_timestamp_recovery (fec_hdr),
      fec_hdr_get_seq_base (fec_hdr, TRUE,
          gst_rtp_buffer_get_seq (fecrtp)),
      fec_hdr_get_packets_len_recovery (fec_hdr));

  fec_level_hdr = fec_hdr_get_level_hdr (fec_hdr);
  GST_CAT_LEVEL_LOG (cat, level, object,
      "%-22s: protection_len=%u mask=0x%012" G_GINT64_MODIFIER "x",
      "fec level header",
      g_ntohs (fec_level_hdr->protection_len),
      fec_level_hdr_get_mask (fec_level_hdr, fec_hdr->L));
}
#endif /* GST_DISABLE_GST_DEBUG */
