/* GStreamer RTP Manager
 *
 * Copyright 2007 Collabora Ltd,
 * Copyright 2007 Nokia Corporation
 *   @author: Philippe Kalaf <philippe.kalaf@collabora.co.uk>.
 * Copyright 2007 Wim Taymans <wim.taymans@gmail.com>
 * Copyright 2015 Kurento (http://kurento.org/)
 *   @author: Miguel París <mparisdiaz@gmail.com>
 * Copyright 2016 Pexip AS
 *   @author: Havard Graff <havard@pexip.com>
 *   @author: Stian Selnes <stian@pexip.com>
 * Copyright (C) 2019 Net Insight AB
 *     Author: Nicolas Dufresne <nicolas.dufresne@collabora.com>
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

#include <gst/gst.h>

#ifndef __RTP_TIMER_QUEUE_H__
#define __RTP_TIMER_QUEUE_H__

#define RTP_TYPE_TIMER_QUEUE rtp_timer_queue_get_type()
G_DECLARE_FINAL_TYPE (RtpTimerQueue, rtp_timer_queue, RTP_TIMER, QUEUE, GObject);

/**
 * RtpTimerType:
 * @RTP_TIMER_EXPECTED: This is used to track when to emit retranmission
 *                      requests. They may be converted into %RTP_TIMER_LOST
 *                      timer if the number of retry has been exhausted.
 * @RTP_TIMER_LOST: This is used to track when a packet is considered lost.
 * @RTP_TIMER_DEADLINE: This is used to track when the jitterbuffer should
 *                      start pushing buffers.
 * @RTP_TIMER_EOS:      This is used to track when end of stream is reached.
 */
typedef enum
{
  RTP_TIMER_EXPECTED,
  RTP_TIMER_LOST,
  RTP_TIMER_DEADLINE,
  RTP_TIMER_EOS
} RtpTimerType;

typedef struct
{
  GList list;
  gboolean queued;

  guint16 seqnum;
  guint num;
  RtpTimerType type;
  GstClockTime timeout;
  GstClockTimeDiff offset;
  GstClockTime duration;
  GstClockTime rtx_base;
  GstClockTime rtx_delay;
  GstClockTime rtx_retry;
  GstClockTime rtx_last;
  guint num_rtx_retry;
  guint num_rtx_received;
} RtpTimer;

void         rtp_timer_free (RtpTimer * timer);
RtpTimer *   rtp_timer_dup (const RtpTimer * timer);

static inline RtpTimer * rtp_timer_get_next (RtpTimer * timer)
{
  GList *list = (GList *) timer;
  return (RtpTimer *) list->next;
}

static inline RtpTimer * rtp_timer_get_prev (RtpTimer * timer)
{
  GList *list = (GList *) timer;
  return (RtpTimer *) list->prev;
}

RtpTimerQueue * rtp_timer_queue_new (void);

RtpTimer *      rtp_timer_queue_find (RtpTimerQueue * queue, guint seqnum);

RtpTimer *      rtp_timer_queue_peek_earliest (RtpTimerQueue * queue);

gboolean        rtp_timer_queue_insert (RtpTimerQueue * queue, RtpTimer * timer);

gboolean        rtp_timer_queue_reschedule (RtpTimerQueue * queue, RtpTimer * timer);

void            rtp_timer_queue_unschedule (RtpTimerQueue * queue, RtpTimer * timer);

RtpTimer *      rtp_timer_queue_pop_until (RtpTimerQueue * queue, GstClockTime timeout);

void            rtp_timer_queue_remove_until (RtpTimerQueue * queue, GstClockTime timeout);

void            rtp_timer_queue_remove_all (RtpTimerQueue * queue);

void            rtp_timer_queue_set_timer (RtpTimerQueue * queue, RtpTimerType type,
                                           guint16 seqnum, guint num, GstClockTime timeout,
                                           GstClockTime delay, GstClockTime duration,
                                           GstClockTimeDiff offset);
void            rtp_timer_queue_set_expected (RtpTimerQueue * queue, guint16 seqnum,
                                              GstClockTime timeout, GstClockTime delay,
                                              GstClockTime duration);
void            rtp_timer_queue_set_lost (RtpTimerQueue * queue, guint16 seqnum,
                                          guint num, GstClockTime timeout,
                                          GstClockTime duration, GstClockTimeDiff offset);
void            rtp_timer_queue_set_eos (RtpTimerQueue * queue, GstClockTime timeout,
                                         GstClockTimeDiff offset);
void            rtp_timer_queue_set_deadline (RtpTimerQueue * queue, guint16 seqnum,
                                              GstClockTime timeout, GstClockTimeDiff offset);
void            rtp_timer_queue_update_timer (RtpTimerQueue * queue, RtpTimer * timer, guint16 seqnum,
                                              GstClockTime timeout, GstClockTime delay,
                                              GstClockTimeDiff offset, gboolean reset);
guint           rtp_timer_queue_length       (RtpTimerQueue * queue);

#endif
