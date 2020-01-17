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

#ifndef __RTP_ULPFEC_COMMON_H__
#define __RTP_ULPFEC_COMMON_H__

#include <gst/gst.h>
#include <gst/rtp/rtp.h>

G_BEGIN_DECLS

#define GST_WARNING_RTP_PACKET(obj, name, pkt) rtp_ulpfec_log_rtppacket (GST_CAT_DEFAULT, GST_LEVEL_WARNING, obj, name, pkt)
#define GST_LOG_RTP_PACKET(obj, name, pkt)     rtp_ulpfec_log_rtppacket (GST_CAT_DEFAULT, GST_LEVEL_LOG, obj, name, pkt)
#define GST_DEBUG_RTP_PACKET(obj, name, pkt)   rtp_ulpfec_log_rtppacket (GST_CAT_DEFAULT, GST_LEVEL_DEBUG, obj, name, pkt)
#define GST_INFO_RTP_PACKET(obj, name, pkt)    rtp_ulpfec_log_rtppacket (GST_CAT_DEFAULT, GST_LEVEL_INFO, obj, name, pkt)
#define GST_WARNING_FEC_PACKET(obj, pkt)       rtp_ulpfec_log_fec_packet (GST_CAT_DEFAULT, GST_LEVEL_WARNING, obj, pkt)
#define GST_DEBUG_FEC_PACKET(obj, pkt)         rtp_ulpfec_log_fec_packet (GST_CAT_DEFAULT, GST_LEVEL_DEBUG, obj, pkt)
#define GST_INFO_FEC_PACKET(obj, pkt)          rtp_ulpfec_log_fec_packet (GST_CAT_DEFAULT, GST_LEVEL_INFO, obj, pkt)

#define RTP_ULPFEC_PROTECTED_PACKETS_MAX(L)    ((L) ? 48 : 16)
#define RTP_ULPFEC_SEQ_BASE_OFFSET_MAX(L)      (RTP_ULPFEC_PROTECTED_PACKETS_MAX(L) - 1)

/**
 * RtpUlpFecMapInfo: Helper wrapper around GstRTPBuffer
 *
 * @rtp: mapped RTP buffer
 **/
typedef struct {
  // FIXME: it used to contain more fields now we are left with only GstRTPBuffer.
  //        it will be nice to use it directly
  GstRTPBuffer rtp;
} RtpUlpFecMapInfo;

/* FIXME: parse/write these properly instead of relying in packed structs */
#ifdef _MSC_VER
#pragma pack(push, 1)
#define ATTRIBUTE_PACKED
#else
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#endif

/* RFC 5109 */
/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |E|L|P|X|  CC   |M| PT recovery |            SN base            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          TS recovery                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        length recovery        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                  Figure 3: FEC Header Format
*/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
typedef struct {
  guint8 CC:4;
  guint8 X:1;
  guint8 P:1;
  guint8 L:1;
  guint8 E:1;

  guint8 pt:7;
  guint8 M:1;

  guint16 seq;
  guint32 timestamp;
  guint16 len;
} ATTRIBUTE_PACKED RtpUlpFecHeader;
#else
typedef struct {
  guint8 E:1;
  guint8 L:1;
  guint8 P:1;
  guint8 X:1;
  guint8 CC:4;

  guint8 M:1;
  guint8 pt:7;

  guint16 seq;
  guint32 timestamp;
  guint16 len;
} ATTRIBUTE_PACKED RtpUlpFecHeader;
#endif

/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       Protection Length       |             mask              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              mask cont. (present only when L = 1)             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                  Figure 4: ULP Level Header Format
*/
typedef struct
{
  guint16 protection_len;
  guint16 mask;
  guint32 mask_continued;
} ATTRIBUTE_PACKED RtpUlpFecLevelHeader;

#ifdef _MSC_VER
#pragma pack(pop)
#else
#undef ATTRIBUTE_PACKED
#endif

gboolean          rtp_ulpfec_map_info_map                  (GstBuffer *buffer, RtpUlpFecMapInfo *info);
void              rtp_ulpfec_map_info_unmap                (RtpUlpFecMapInfo *info);
void              rtp_buffer_to_ulpfec_bitstring           (GstRTPBuffer *rtp, GArray *dst_arr,
                                                            gboolean fec_buffer, gboolean fec_mask_long);
GstBuffer       * rtp_ulpfec_bitstring_to_media_rtp_buffer (GArray *arr,
                                                            gboolean fec_mask_long, guint32 ssrc, guint16 seq);
GstBuffer       * rtp_ulpfec_bitstring_to_fec_rtp_buffer   (GArray *arr, guint16 seq_base, gboolean fec_mask_long,
                                                            guint64 fec_mask, gboolean marker, guint8 pt, guint16 seq,
                                                            guint32 timestamp, guint32 ssrc);

#ifndef GST_DISABLE_GST_DEBUG
void              rtp_ulpfec_log_rtppacket                 (GstDebugCategory * cat, GstDebugLevel level,
                                                            gpointer object, const gchar *name,
                                                            GstRTPBuffer *rtp);

void              rtp_ulpfec_log_fec_packet                (GstDebugCategory * cat, GstDebugLevel level,
                                                            gpointer object, GstRTPBuffer *fecrtp);
#else
#define rtp_ulpfec_log_rtppacket(cat,level,obj,name,rtp) /* NOOP */
#define rtp_ulpfec_log_fec_packet(cat,level,obj,fecrtp)  /* NOOP */
#endif

RtpUlpFecHeader * rtp_ulpfec_buffer_get_fechdr             (GstRTPBuffer *rtp);
guint             rtp_ulpfec_get_headers_len               (gboolean fec_mask_long);
guint16           rtp_ulpfec_hdr_get_protection_len        (RtpUlpFecHeader const *fec_hdr);
guint64           rtp_ulpfec_packet_mask_from_seqnum       (guint16 seq, guint16 fec_seq_base, gboolean fec_mask_long);
guint64           rtp_ulpfec_buffer_get_mask               (GstRTPBuffer *rtp);
guint16           rtp_ulpfec_buffer_get_seq_base           (GstRTPBuffer *rtp);
gboolean          rtp_ulpfec_mask_is_long                  (guint64 mask);
gboolean          rtp_ulpfec_buffer_is_valid               (GstRTPBuffer * rtp);

G_END_DECLS

#endif
