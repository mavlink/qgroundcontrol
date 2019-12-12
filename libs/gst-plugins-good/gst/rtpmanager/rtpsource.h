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

#ifndef __RTP_SOURCE_H__
#define __RTP_SOURCE_H__

#include <gst/gst.h>
#include <gst/rtp/rtp.h>
#include <gst/net/gstnetaddressmeta.h>
#include <gio/gio.h>

#include "rtpstats.h"

/* the default number of consecutive RTP packets we need to receive before the
 * source is considered valid */
#define RTP_NO_PROBATION        0
#define RTP_DEFAULT_PROBATION   2

#define RTP_SEQ_MOD          (1 << 16)

typedef struct _RTPSource RTPSource;
typedef struct _RTPSourceClass RTPSourceClass;

#define RTP_TYPE_SOURCE             (rtp_source_get_type())
#define RTP_SOURCE(src)             (G_TYPE_CHECK_INSTANCE_CAST((src),RTP_TYPE_SOURCE,RTPSource))
#define RTP_SOURCE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),RTP_TYPE_SOURCE,RTPSourceClass))
#define RTP_IS_SOURCE(src)          (G_TYPE_CHECK_INSTANCE_TYPE((src),RTP_TYPE_SOURCE))
#define RTP_IS_SOURCE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),RTP_TYPE_SOURCE))
#define RTP_SOURCE_CAST(src)        ((RTPSource *)(src))

/**
 * RTP_SOURCE_IS_ACTIVE:
 * @src: an #RTPSource
 *
 * Check if @src is active. A source is active when it has been validated
 * and has not yet received a BYE packet.
 */
#define RTP_SOURCE_IS_ACTIVE(src)  (src->validated && !src->marked_bye)

/**
 * RTP_SOURCE_IS_SENDER:
 * @src: an #RTPSource
 *
 * Check if @src is a sender.
 */
#define RTP_SOURCE_IS_SENDER(src)  (src->is_sender)
/**
 * RTP_SOURCE_IS_MARKED_BYE:
 * @src: an #RTPSource
 *
 * Check if @src is a marked as BYE.
 */
#define RTP_SOURCE_IS_MARKED_BYE(src)  (src->marked_bye)


/**
 * RTPSourcePushRTP:
 * @src: an #RTPSource
 * @data: the RTP buffer or buffer list ready for processing
 * @user_data: user data specified when registering
 *
 * This callback will be called when @src has @buffer ready for further
 * processing.
 *
 * Returns: a #GstFlowReturn.
 */
typedef GstFlowReturn (*RTPSourcePushRTP) (RTPSource *src, gpointer data,
	gpointer user_data);

/**
 * RTPSourceClockRate:
 * @src: an #RTPSource
 * @payload: a payload type
 * @user_data: user data specified when registering
 *
 * This callback will be called when @src needs the clock-rate of the
 * @payload.
 *
 * Returns: a clock-rate for @payload.
 */
typedef gint (*RTPSourceClockRate) (RTPSource *src, guint8 payload, gpointer user_data);

/**
 * RTPSourceCallbacks:
 * @push_rtp: a packet becomes available for handling
 * @clock_rate: a clock-rate is requested
 * @get_time: the current clock time is requested
 *
 * Callbacks performed by #RTPSource when actions need to be performed.
 */
typedef struct {
  RTPSourcePushRTP     push_rtp;
  RTPSourceClockRate   clock_rate;
} RTPSourceCallbacks;

/**
 * RTPConflictingAddress:
 * @address: #GSocketAddress which conflicted
 * @last_conflict_time: time when the last conflict was seen
 *
 * This structure is used to account for addresses that have conflicted to find
 * loops.
 */
typedef struct {
  GSocketAddress *address;
  GstClockTime time;
} RTPConflictingAddress;

/**
 * RTPSource:
 *
 * A source in the #RTPSession
 *
 * @conflicting_addresses: GList of conflicting addresses
 */
struct _RTPSource {
  GObject       object;

  /*< private >*/
  guint32       ssrc;

  guint16       generation;
  GHashTable    *reported_in_sr_of;     /* set of SSRCs */

  guint         probation;
  guint         curr_probation;
  gboolean      validated;
  gboolean      internal;
  gboolean      is_csrc;
  gboolean      is_sender;
  gboolean      closing;

  GstStructure  *sdes;

  gboolean      marked_bye;
  gchar        *bye_reason;
  gboolean      sent_bye;

  GSocketAddress *rtp_from;
  GSocketAddress *rtcp_from;

  gint          payload;
  GstCaps      *caps;
  gint          clock_rate;
  gint32        seqnum_offset;

  GstClockTime  bye_time;
  GstClockTime  last_activity;
  GstClockTime  last_rtp_activity;

  GstClockTime  last_rtime;
  GstClockTime  last_rtptime;

  /* for bitrate estimation */
  guint64       bitrate;
  GstClockTime  prev_rtime;
  guint64       bytes_sent;
  guint64       bytes_received;

  GQueue       *packets;
  RTPPacketRateCtx packet_rate_ctx;
  guint32       max_dropout_time;
  guint32       max_misorder_time;

  RTPSourceCallbacks callbacks;
  gpointer           user_data;

  RTPSourceStats stats;
  RTPReceiverReport last_rr;

  GList         *conflicting_addresses;

  GQueue        *retained_feedback;

  gboolean     send_pli;
  gboolean     send_fir;
  guint8       current_send_fir_seqnum;
  gint         last_fir_count;
  GstClockTime last_keyframe_request;

  gboolean     send_nack;
  GArray      *nacks;
  GArray      *nack_deadlines;

  gboolean      pt_set;
  guint8        pt;

  gboolean      disable_rtcp;
};

struct _RTPSourceClass {
  GObjectClass   parent_class;
};

GType rtp_source_get_type (void);

/* managing lifetime of sources */
RTPSource*      rtp_source_new                 (guint32 ssrc);
void            rtp_source_set_callbacks       (RTPSource *src, RTPSourceCallbacks *cb, gpointer data);

/* properties */
guint32         rtp_source_get_ssrc            (RTPSource *src);

void            rtp_source_set_as_csrc         (RTPSource *src);
gboolean        rtp_source_is_as_csrc          (RTPSource *src);

gboolean        rtp_source_is_active           (RTPSource *src);
gboolean        rtp_source_is_validated        (RTPSource *src);
gboolean        rtp_source_is_sender           (RTPSource *src);

void            rtp_source_mark_bye            (RTPSource *src, const gchar *reason);
gboolean        rtp_source_is_marked_bye       (RTPSource *src);
gchar *         rtp_source_get_bye_reason      (RTPSource *src);

void            rtp_source_update_caps         (RTPSource *src, GstCaps *caps);

/* SDES info */
const GstStructure *
                rtp_source_get_sdes_struct     (RTPSource * src);
gboolean        rtp_source_set_sdes_struct     (RTPSource * src, GstStructure *sdes);

/* handling network address */
void            rtp_source_set_rtp_from        (RTPSource *src, GSocketAddress *address);
void            rtp_source_set_rtcp_from       (RTPSource *src, GSocketAddress *address);

/* handling RTP */
GstFlowReturn   rtp_source_process_rtp         (RTPSource *src, RTPPacketInfo *pinfo);

GstFlowReturn   rtp_source_send_rtp            (RTPSource *src, RTPPacketInfo *pinfo);

/* RTCP messages */
void            rtp_source_process_sr          (RTPSource *src, GstClockTime time, guint64 ntptime,
                                                guint32 rtptime, guint32 packet_count, guint32 octet_count);
void            rtp_source_process_rb          (RTPSource *src, guint64 ntpnstime, guint8 fractionlost,
                                                gint32 packetslost, guint32 exthighestseq, guint32 jitter,
                                                guint32 lsr, guint32 dlsr);

gboolean        rtp_source_get_new_sr          (RTPSource *src, guint64 ntpnstime, GstClockTime running_time,
                                                guint64 *ntptime, guint32 *rtptime, guint32 *packet_count,
						guint32 *octet_count);
gboolean        rtp_source_get_new_rb          (RTPSource *src, GstClockTime time, guint8 *fractionlost,
                                                gint32 *packetslost, guint32 *exthighestseq, guint32 *jitter,
                                                guint32 *lsr, guint32 *dlsr);

gboolean        rtp_source_get_last_sr         (RTPSource *src, GstClockTime *time, guint64 *ntptime,
                                                guint32 *rtptime, guint32 *packet_count,
						guint32 *octet_count);
gboolean        rtp_source_get_last_rb         (RTPSource *src, guint8 *fractionlost, gint32 *packetslost,
                                                guint32 *exthighestseq, guint32 *jitter,
                                                guint32 *lsr, guint32 *dlsr, guint32 *round_trip);

void            rtp_source_reset               (RTPSource * src);

gboolean        rtp_source_find_conflicting_address (RTPSource * src,
                                                GSocketAddress *address,
                                                GstClockTime time);

void            rtp_source_add_conflicting_address (RTPSource * src,
                                                GSocketAddress *address,
                                                GstClockTime time);

gboolean        find_conflicting_address       (GList * conflicting_address,
                                                GSocketAddress * address,
                                                GstClockTime time);

GList *         add_conflicting_address        (GList * conflicting_addresses,
                                                GSocketAddress * address,
                                                GstClockTime time);
GList *         timeout_conflicting_addresses  (GList * conflicting_addresses,
                                                GstClockTime current_time);

void            rtp_conflicting_address_free   (RTPConflictingAddress * addr);

void            rtp_source_timeout             (RTPSource * src,
                                                GstClockTime current_time,
                                                GstClockTime running_time,
                                                GstClockTime feedback_retention_window);

void            rtp_source_retain_rtcp_packet  (RTPSource * src,
                                                GstRTCPPacket *pkt,
                                                GstClockTime running_time);
gboolean        rtp_source_has_retained        (RTPSource * src,
                                                GCompareFunc func,
                                                gconstpointer data);

void            rtp_source_register_nack       (RTPSource * src,
                                                guint16 seqnum,
                                                GstClockTime deadline);
guint16 *       rtp_source_get_nacks           (RTPSource * src, guint *n_nacks);
GstClockTime *  rtp_source_get_nack_deadlines  (RTPSource * src, guint *n_nacks);
void            rtp_source_clear_nacks         (RTPSource * src, guint n_nacks);

#endif /* __RTP_SOURCE_H__ */
