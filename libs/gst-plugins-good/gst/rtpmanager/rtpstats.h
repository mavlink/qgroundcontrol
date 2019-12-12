/* GStreamer
 * Copyright (C) <2007> Wim Taymans <wim.taymans@gmail.com>
 * Copyright (C)  2015 Kurento (http://kurento.org/)
 *   @author: Miguel París <mparisdiaz@gmail.com>
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

#ifndef __RTP_STATS_H__
#define __RTP_STATS_H__

#include <gst/gst.h>
#include <gst/net/gstnetaddressmeta.h>
#include <gst/rtp/rtp.h>
#include <gio/gio.h>

/* UDP/IP is assumed for bandwidth calculation */
#define UDP_IP_HEADER_OVERHEAD 28

/**
 * RTPSenderReport:
 *
 * A sender report structure.
 */
typedef struct {
  gboolean is_valid;
  guint64 ntptime;
  guint32 rtptime;
  guint32 packet_count;
  guint32 octet_count;
  GstClockTime time;
} RTPSenderReport;

/**
 * RTPReceiverReport:
 *
 * A receiver report structure.
 */
typedef struct {
  gboolean is_valid;
  guint32 ssrc; /* who the report is from */
  guint8  fractionlost;
  guint32 packetslost;
  guint32 exthighestseq;
  guint32 jitter;
  guint32 lsr;
  guint32 dlsr;
  guint32 round_trip;
} RTPReceiverReport;

/**
 * RTPPacketInfo:
 * @send: if this is a packet for sending
 * @rtp: if this info is about an RTP packet
 * @is_list: if this is a bufferlist
 * @data: a #GstBuffer or #GstBufferList
 * @address: address of the sender of the packet
 * @current_time: current time according to the system clock
 * @running_time: time of a packet as buffer running_time
 * @ntpnstime: time of a packet NTP time in nanoseconds
 * @header_len: number of overhead bytes per packet
 * @bytes: bytes of the packet including lowlevel overhead
 * @payload_len: bytes of the RTP payload
 * @seqnum: the seqnum of the packet
 * @pt: the payload type of the packet
 * @rtptime: the RTP time of the packet
 *
 * Structure holding information about the packet.
 */
typedef struct {
  gboolean      send;
  gboolean      rtp;
  gboolean      is_list;
  gpointer      data;
  GSocketAddress *address;
  GstClockTime  current_time;
  GstClockTime  running_time;
  guint64       ntpnstime;
  guint         header_len;
  guint         bytes;
  guint         packets;
  guint         payload_len;
  guint32       ssrc;
  guint16       seqnum;
  guint8        pt;
  guint32       rtptime;
  guint32       csrc_count;
  guint32       csrcs[16];
} RTPPacketInfo;

/**
 * RTPSourceStats:
 * @packets_received: number of received packets in total
 * @prev_received: number of packets received in previous reporting
 *                       interval
 * @octets_received: number of payload bytes received
 * @bytes_received: number of total bytes received including headers and lower
 *                 protocol level overhead
 * @max_seqnr: highest sequence number received
 * @transit: previous transit time used for calculating @jitter
 * @jitter: current jitter (in clock rate units scaled by 16 for precision)
 * @prev_rtptime: previous time when an RTP packet was received
 * @prev_rtcptime: previous time when an RTCP packet was received
 * @last_rtptime: time when last RTP packet received
 * @last_rtcptime: time when last RTCP packet received
 * @curr_rr: index of current @rr block
 * @rr: previous and current receiver report block
 * @curr_sr: index of current @sr block
 * @sr: previous and current sender report block
 *
 * Stats about a source.
 */
typedef struct {
  guint64      packets_received;
  guint64      octets_received;
  guint64      bytes_received;

  guint32      prev_expected;
  guint32      prev_received;

  guint16      max_seq;
  guint64      cycles;
  guint32      base_seq;
  guint32      bad_seq;
  guint32      transit;
  guint32      jitter;

  guint64      packets_sent;
  guint64      octets_sent;

  guint        sent_pli_count;
  guint        recv_pli_count;
  guint        sent_fir_count;
  guint        recv_fir_count;
  guint        sent_nack_count;
  guint        recv_nack_count;

  /* when we received stuff */
  GstClockTime prev_rtptime;
  GstClockTime prev_rtcptime;
  GstClockTime last_rtptime;
  GstClockTime last_rtcptime;

  /* sender and receiver reports */
  gint              curr_rr;
  RTPReceiverReport rr[2];
  gint              curr_sr;
  RTPSenderReport   sr[2];
} RTPSourceStats;

#define RTP_STATS_BANDWIDTH           64000
#define RTP_STATS_RTCP_FRACTION       0.05
/*
 * Minimum average time between RTCP packets from this site (in
 * seconds).  This time prevents the reports from `clumping' when
 * sessions are small and the law of large numbers isn't helping
 * to smooth out the traffic.  It also keeps the report interval
 * from becoming ridiculously small during transient outages like
 * a network partition.
 */
#define RTP_STATS_MIN_INTERVAL      5.0
/*
 * Fraction of the RTCP bandwidth to be shared among active
 * senders.  (This fraction was chosen so that in a typical
 * session with one or two active senders, the computed report
 * time would be roughly equal to the minimum report time so that
 * we don't unnecessarily slow down receiver reports.) The
 * receiver fraction must be 1 - the sender fraction.
 */
#define RTP_STATS_SENDER_FRACTION       (0.25)
#define RTP_STATS_RECEIVER_FRACTION     (1.0 - RTP_STATS_SENDER_FRACTION)

/*
 * When receiving a BYE from a source, remove the source from the database
 * after this timeout.
 */
#define RTP_STATS_BYE_TIMEOUT           (2 * GST_SECOND)

/*
 * The default and minimum values of the maximum number of missing packets we tolerate.
 * These are packets with asequence number bigger than the last seen packet.
 */
#define RTP_DEF_DROPOUT      3000
#define RTP_MIN_DROPOUT      30

/*
 * The default and minimum values of the maximum number of misordered packets we tolerate.
 * These are packets with a sequence number smaller than the last seen packet.
 */
#define RTP_DEF_MISORDER     100
#define RTP_MIN_MISORDER     10

/**
 * RTPPacketRateCtx:
 *
 * Context to calculate the pseudo-average packet rate.
 */
typedef struct {
  gboolean probed;
  gint32 clock_rate;
  guint16 last_seqnum;
  guint64 last_ts;
  guint32 avg_packet_rate;
} RTPPacketRateCtx;

void gst_rtp_packet_rate_ctx_reset (RTPPacketRateCtx * ctx, gint32 clock_rate);
guint32 gst_rtp_packet_rate_ctx_update (RTPPacketRateCtx *ctx, guint16 seqnum, guint32 ts);
guint32 gst_rtp_packet_rate_ctx_get (RTPPacketRateCtx *ctx);
guint32 gst_rtp_packet_rate_ctx_get_max_dropout (RTPPacketRateCtx *ctx, gint32 time_ms);
guint32 gst_rtp_packet_rate_ctx_get_max_misorder (RTPPacketRateCtx *ctx, gint32 time_ms);

/**
 * RTPSessionStats:
 *
 * Stats kept for a session and used to produce RTCP packet timeouts.
 */
typedef struct {
  guint         bandwidth;
  guint         rtcp_bandwidth;
  gdouble       sender_fraction;
  gdouble       receiver_fraction;
  gdouble       min_interval;
  GstClockTime  bye_timeout;
  guint         internal_sources;
  guint         sender_sources;
  guint         internal_sender_sources;
  guint         active_sources;
  guint         avg_rtcp_packet_size;
  guint         bye_members;
  guint         nacks_dropped;
  guint         nacks_sent;
  guint         nacks_received;
} RTPSessionStats;

void           rtp_stats_init_defaults              (RTPSessionStats *stats);

void           rtp_stats_set_bandwidths             (RTPSessionStats *stats,
                                                     guint rtp_bw,
                                                     gdouble rtcp_bw,
                                                     guint rs, guint rr);

GstClockTime   rtp_stats_calculate_rtcp_interval    (RTPSessionStats *stats, gboolean sender, GstRTPProfile profile, gboolean ptp, gboolean first);
GstClockTime   rtp_stats_add_rtcp_jitter            (RTPSessionStats *stats, GstClockTime interval);
GstClockTime   rtp_stats_calculate_bye_interval     (RTPSessionStats *stats);
gint64         rtp_stats_get_packets_lost           (const RTPSourceStats *stats);

void           rtp_stats_set_min_interval           (RTPSessionStats *stats,
                                                     gdouble min_interval);


gboolean __g_socket_address_equal (GSocketAddress *a, GSocketAddress *b);
gchar * __g_socket_address_to_string (GSocketAddress * addr);

#endif /* __RTP_STATS_H__ */
