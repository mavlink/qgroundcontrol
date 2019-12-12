/* GStreamer RTP Manager
 *
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

#include <gst/rtp/gstrtpbuffer.h>

#include "rtptimerqueue.h"

struct _RtpTimerQueue
{
  GObject parent;

  GQueue timers;
  GHashTable *hashtable;
};

G_DEFINE_TYPE (RtpTimerQueue, rtp_timer_queue, G_TYPE_OBJECT);

/* some timer private helpers */

static RtpTimer *
rtp_timer_new (void)
{
  return g_slice_new0 (RtpTimer);
}

static inline void
rtp_timer_set_next (RtpTimer * timer, RtpTimer * next)
{
  GList *list = (GList *) timer;
  list->next = (GList *) next;
}

static inline void
rtp_timer_set_prev (RtpTimer * timer, RtpTimer * prev)
{
  GList *list = (GList *) timer;
  list->prev = (GList *) prev;
}

static inline gboolean
rtp_timer_is_later (RtpTimer * timer, RtpTimer * next)
{
  if (next == NULL)
    return FALSE;

  if (GST_CLOCK_TIME_IS_VALID (next->timeout)) {
    if (!GST_CLOCK_TIME_IS_VALID (timer->timeout))
      return FALSE;

    if (timer->timeout > next->timeout)
      return TRUE;
  }

  if (timer->timeout == next->timeout &&
      gst_rtp_buffer_compare_seqnum (timer->seqnum, next->seqnum) < 0)
    return TRUE;

  return FALSE;
}

static inline gboolean
rtp_timer_is_sooner (RtpTimer * timer, RtpTimer * prev)
{
  if (prev == NULL)
    return FALSE;

  if (GST_CLOCK_TIME_IS_VALID (prev->timeout)) {
    if (!GST_CLOCK_TIME_IS_VALID (timer->timeout))
      return TRUE;

    if (timer->timeout < prev->timeout)
      return TRUE;
  }

  if (timer->timeout == prev->timeout &&
      gst_rtp_buffer_compare_seqnum (timer->seqnum, prev->seqnum) > 0)
    return TRUE;

  return FALSE;
}

static inline gboolean
rtp_timer_is_closer_to_head (RtpTimer * timer, RtpTimer * head)
{
  RtpTimer *prev = rtp_timer_get_prev (timer);
  GstClockTimeDiff prev_delta = 0;
  GstClockTimeDiff head_delta = 0;

  if (prev == NULL)
    return FALSE;

  if (rtp_timer_is_sooner (timer, head))
    return TRUE;

  if (rtp_timer_is_later (timer, prev))
    return FALSE;

  if (prev->timeout == head->timeout) {
    gint prev_gap, head_gap;

    prev_gap = gst_rtp_buffer_compare_seqnum (timer->seqnum, prev->seqnum);
    head_gap = gst_rtp_buffer_compare_seqnum (head->seqnum, timer->seqnum);

    if (head_gap < prev_gap)
      return TRUE;
  }

  if (GST_CLOCK_TIME_IS_VALID (timer->timeout) &&
      GST_CLOCK_TIME_IS_VALID (head->timeout)) {
    prev_delta = GST_CLOCK_DIFF (timer->timeout, prev->timeout);
    head_delta = GST_CLOCK_DIFF (head->timeout, timer->timeout);

    if (head_delta < prev_delta)
      return TRUE;
  }

  return FALSE;
}

static inline gboolean
rtp_timer_is_closer_to_tail (RtpTimer * timer, RtpTimer * tail)
{
  RtpTimer *next = rtp_timer_get_next (timer);
  GstClockTimeDiff tail_delta = 0;
  GstClockTimeDiff next_delta = 0;

  if (next == NULL)
    return FALSE;

  if (rtp_timer_is_later (timer, tail))
    return TRUE;

  if (rtp_timer_is_sooner (timer, next))
    return FALSE;

  if (tail->timeout == next->timeout) {
    gint tail_gap, next_gap;

    tail_gap = gst_rtp_buffer_compare_seqnum (timer->seqnum, tail->seqnum);
    next_gap = gst_rtp_buffer_compare_seqnum (next->seqnum, timer->seqnum);

    if (tail_gap < next_gap)
      return TRUE;
  }

  if (GST_CLOCK_TIME_IS_VALID (timer->timeout) &&
      GST_CLOCK_TIME_IS_VALID (next->timeout)) {
    tail_delta = GST_CLOCK_DIFF (timer->timeout, tail->timeout);
    next_delta = GST_CLOCK_DIFF (next->timeout, timer->timeout);

    if (tail_delta < next_delta)
      return TRUE;
  }

  return FALSE;
}

static inline RtpTimer *
rtp_timer_queue_get_tail (RtpTimerQueue * queue)
{
  return (RtpTimer *) queue->timers.tail;
}

static inline void
rtp_timer_queue_set_tail (RtpTimerQueue * queue, RtpTimer * timer)
{
  queue->timers.tail = (GList *) timer;
  g_assert (queue->timers.tail->next == NULL);
}

static inline RtpTimer *
rtp_timer_queue_get_head (RtpTimerQueue * queue)
{
  return (RtpTimer *) queue->timers.head;
}

static inline void
rtp_timer_queue_set_head (RtpTimerQueue * queue, RtpTimer * timer)
{
  queue->timers.head = (GList *) timer;
  g_assert (queue->timers.head->prev == NULL);
}

static void
rtp_timer_queue_insert_before (RtpTimerQueue * queue, RtpTimer * sibling,
    RtpTimer * timer)
{
  if (sibling == rtp_timer_queue_get_head (queue)) {
    rtp_timer_queue_set_head (queue, timer);
  } else {
    rtp_timer_set_prev (timer, rtp_timer_get_prev (sibling));
    rtp_timer_set_next (rtp_timer_get_prev (sibling), timer);
  }

  rtp_timer_set_next (timer, sibling);
  rtp_timer_set_prev (sibling, timer);

  queue->timers.length++;
}

static void
rtp_timer_queue_insert_after (RtpTimerQueue * queue, RtpTimer * sibling,
    RtpTimer * timer)
{
  if (sibling == rtp_timer_queue_get_tail (queue)) {
    rtp_timer_queue_set_tail (queue, timer);
  } else {
    rtp_timer_set_next (timer, rtp_timer_get_next (sibling));
    rtp_timer_set_prev (rtp_timer_get_next (sibling), timer);
  }

  rtp_timer_set_prev (timer, sibling);
  rtp_timer_set_next (sibling, timer);

  queue->timers.length++;
}

static void
rtp_timer_queue_insert_tail (RtpTimerQueue * queue, RtpTimer * timer)
{
  RtpTimer *it = rtp_timer_queue_get_tail (queue);

  while (it) {
    if (!GST_CLOCK_TIME_IS_VALID (it->timeout))
      break;

    if (timer->timeout > it->timeout)
      break;

    if (timer->timeout == it->timeout &&
        gst_rtp_buffer_compare_seqnum (timer->seqnum, it->seqnum) < 0)
      break;

    it = rtp_timer_get_prev (it);
  }

  /* the queue is empty, or this is the earliest timeout */
  if (it == NULL)
    g_queue_push_head_link (&queue->timers, (GList *) timer);
  else
    rtp_timer_queue_insert_after (queue, it, timer);
}

static void
rtp_timer_queue_insert_head (RtpTimerQueue * queue, RtpTimer * timer)
{
  RtpTimer *it = rtp_timer_queue_get_head (queue);

  while (it) {
    if (GST_CLOCK_TIME_IS_VALID (it->timeout)) {
      if (!GST_CLOCK_TIME_IS_VALID (timer->timeout))
        break;

      if (timer->timeout < it->timeout)
        break;
    }

    if (timer->timeout == it->timeout &&
        gst_rtp_buffer_compare_seqnum (timer->seqnum, it->seqnum) > 0)
      break;

    it = rtp_timer_get_next (it);
  }

  /* the queue is empty, or this is the oldest */
  if (it == NULL)
    g_queue_push_tail_link (&queue->timers, (GList *) timer);
  else
    rtp_timer_queue_insert_before (queue, it, timer);
}

static void
rtp_timer_queue_init (RtpTimerQueue * queue)
{
  queue->hashtable = g_hash_table_new (NULL, NULL);
}

static void
rtp_timer_queue_finalize (GObject * object)
{
  RtpTimerQueue *queue = RTP_TIMER_QUEUE (object);
  RtpTimer *timer;

  while ((timer = rtp_timer_queue_pop_until (queue, GST_CLOCK_TIME_NONE)))
    rtp_timer_free (timer);
  g_hash_table_unref (queue->hashtable);
  g_assert (queue->timers.length == 0);
}

static void
rtp_timer_queue_class_init (RtpTimerQueueClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = rtp_timer_queue_finalize;
}

/**
 * rtp_timer_free:
 *
 * Free a #RtpTimer structure. This should be used after a timer has been
 * poped or unscheduled. The timer must be queued.
 */
void
rtp_timer_free (RtpTimer * timer)
{
  g_return_if_fail (timer);
  g_return_if_fail (timer->queued == FALSE);
  g_return_if_fail (timer->list.next == NULL);
  g_return_if_fail (timer->list.prev == NULL);

  g_slice_free (RtpTimer, timer);
}

/**
 * rtp_timer_dup:
 * @timer: a #RtpTimer
 *
 * This allow cloning a #RtpTimer structure.
 *
 * Returns: a copy of @timer
 */
RtpTimer *
rtp_timer_dup (const RtpTimer * timer)
{
  RtpTimer *copy = g_slice_new (RtpTimer);
  memcpy (copy, timer, sizeof (RtpTimer));
  memset (&copy->list, 0, sizeof (GList));
  copy->queued = FALSE;
  return copy;
}

/**
 * rtp_timer_queue_find:
 * @queue: the #RtpTimerQueue object
 * @seqnum: the sequence number of the #RtpTimer
 *
 * Lookup for a timer with @seqnum. Only one timer per seqnum exist withing
 * the #RtpTimerQueue. This operation is o(1).
 *
 * Rerturn: the matching #RtpTimer or %NULL
 */
RtpTimer *
rtp_timer_queue_find (RtpTimerQueue * queue, guint seqnum)
{
  return g_hash_table_lookup (queue->hashtable, GINT_TO_POINTER (seqnum));
}

/**
 * rtp_timer_queue_peek_earliest:
 * @queue: the #RtpTimerQueue object
 *
 * Rerturns: the #RtpTimer with earliest timeout value
 */
RtpTimer *
rtp_timer_queue_peek_earliest (RtpTimerQueue * queue)
{
  return rtp_timer_queue_get_head (queue);
}


/**
 * rtp_timer_queue_new:
 *
 * Returns: a freshly allocated #RtpTimerQueue
 */
RtpTimerQueue *
rtp_timer_queue_new (void)
{
  return g_object_new (RTP_TYPE_TIMER_QUEUE, NULL);
}

/**
 * rtp_timer_queue_insert:
 * @queue: the #RtpTimerQueue object
 * @timer: the #RtpTimer to insert
 *
 * Insert a timer into the queue. Earliest timer are at the head and then
 * timer are sorted by seqnum (smaller seqnum first). This function is o(n)
 * but it is expected that most timers added are schedule later, in which case
 * the insertion will be faster.
 *
 * Returns: %FLASE is a timer with the same seqnum already existed
 */
gboolean
rtp_timer_queue_insert (RtpTimerQueue * queue, RtpTimer * timer)
{
  g_return_val_if_fail (timer->queued == FALSE, FALSE);

  if (rtp_timer_queue_find (queue, timer->seqnum))
    return FALSE;

  if (timer->timeout == -1)
    rtp_timer_queue_insert_head (queue, timer);
  else
    rtp_timer_queue_insert_tail (queue, timer);

  g_hash_table_insert (queue->hashtable,
      GINT_TO_POINTER (timer->seqnum), timer);
  timer->queued = TRUE;

  return TRUE;
}

/**
 * rtp_timer_queue_reschedule:
 * @queue: the #RtpTimerQueue object
 * @timer: the #RtpTimer to reschedule
 *
 * This function moves @timer inside the queue to put it back to it's new
 * location. This function is o(n) but it is assumed that nearby modification
 * of the timeout will occure.
 *
 * Returns: %TRUE if the timer was moved
 */
gboolean
rtp_timer_queue_reschedule (RtpTimerQueue * queue, RtpTimer * timer)
{
  RtpTimer *it = timer;

  g_return_val_if_fail (timer->queued == TRUE, FALSE);

  if (rtp_timer_is_closer_to_head (timer, rtp_timer_queue_get_head (queue))) {
    g_queue_unlink (&queue->timers, (GList *) timer);
    rtp_timer_queue_insert_head (queue, timer);
    return TRUE;
  }

  while (rtp_timer_is_sooner (timer, rtp_timer_get_prev (it)))
    it = rtp_timer_get_prev (it);

  if (it != timer) {
    g_queue_unlink (&queue->timers, (GList *) timer);
    rtp_timer_queue_insert_before (queue, it, timer);
    return TRUE;
  }

  if (rtp_timer_is_closer_to_tail (timer, rtp_timer_queue_get_tail (queue))) {
    g_queue_unlink (&queue->timers, (GList *) timer);
    rtp_timer_queue_insert_tail (queue, timer);
    return TRUE;
  }

  while (rtp_timer_is_later (timer, rtp_timer_get_next (it)))
    it = rtp_timer_get_next (it);

  if (it != timer) {
    g_queue_unlink (&queue->timers, (GList *) timer);
    rtp_timer_queue_insert_after (queue, it, timer);
    return TRUE;
  }

  return FALSE;
}

/**
 * rtp_timer_queue_unschedule:
 * @queue: the #RtpTimerQueue
 * @timer: the #RtpTimer to unschedule
 *
 * This removes a timer from the queue. The timer structure can be reused,
 * or freed using rtp_timer_free(). This function is o(1).
 */
void
rtp_timer_queue_unschedule (RtpTimerQueue * queue, RtpTimer * timer)
{
  g_return_if_fail (timer->queued == TRUE);

  g_queue_unlink (&queue->timers, (GList *) timer);
  g_hash_table_remove (queue->hashtable, GINT_TO_POINTER (timer->seqnum));
  timer->queued = FALSE;
}

/**
 * rtp_timer_queue_pop_until:
 * @queue: the #RtpTimerQueue
 * @timeout: Time at witch timers expired
 *
 * Unschdedule and return the earliest packet that has a timeout smaller or
 * equal to @timeout. The returns #RtpTimer must be freed with
 * rtp_timer_free(). This function is o(1).
 *
 * Returns: an expired timer according to @timeout, or %NULL.
 */
RtpTimer *
rtp_timer_queue_pop_until (RtpTimerQueue * queue, GstClockTime timeout)
{
  RtpTimer *timer;

  timer = (RtpTimer *) g_queue_peek_head_link (&queue->timers);
  if (!timer)
    return NULL;

  if (!GST_CLOCK_TIME_IS_VALID (timer->timeout) || timer->timeout <= timeout) {
    rtp_timer_queue_unschedule (queue, timer);
    return timer;
  }

  return NULL;
}

/**
 * rtp_timer_queue_remove_until:
 * @queue: the #RtpTimerQueue
 * @timeout: Time at witch timers expired
 *
 * Unschedule and free all timers that has a timeout smaller or equal to
 * @timeout.
 */
void
rtp_timer_queue_remove_until (RtpTimerQueue * queue, GstClockTime timeout)
{
  RtpTimer *timer;

  while ((timer = rtp_timer_queue_pop_until (queue, timeout))) {
    GST_LOG ("Removing expired timer #%d, %" GST_TIME_FORMAT " < %"
        GST_TIME_FORMAT, timer->seqnum, GST_TIME_ARGS (timer->timeout),
        GST_TIME_ARGS (timeout));
    rtp_timer_free (timer);
  }
}

/**
 * rtp_timer_queue_remove_all:
 * @queue: the #RtpTimerQueue
 *
 * Unschedule and free all timers from the queue.
 */
void
rtp_timer_queue_remove_all (RtpTimerQueue * queue)
{
  rtp_timer_queue_remove_until (queue, GST_CLOCK_TIME_NONE);
}

/**
 * rtp_timer_queue_set_timer:
 * @queue: the #RtpTimerQueue
 * @type: the #RtpTimerType
 * @senum: the timer seqnum
 * @num: the number of seqnum in the range (partially supported)
 * @timeout: the timer timeout
 * @delay: the additional delay (will be added to @timeout)
 * @duration: the duration of the event related to the timer
 * @offset: offset that can be used to convert the timeout to timestamp
 *
 * If there exist a timer with this seqnum it will be updated other a new
 * timer is created and inserted into the queue. This function is o(n) except
 * that it's optimized for later timer insertion.
 */
void
rtp_timer_queue_set_timer (RtpTimerQueue * queue, RtpTimerType type,
    guint16 seqnum, guint num, GstClockTime timeout, GstClockTime delay,
    GstClockTime duration, GstClockTimeDiff offset)
{
  RtpTimer *timer;

  timer = rtp_timer_queue_find (queue, seqnum);
  if (!timer)
    timer = rtp_timer_new ();

  /* for new timers or on seqnum change reset the RTX data */
  if (!timer->queued || timer->seqnum != seqnum) {
    if (type == RTP_TIMER_EXPECTED) {
      timer->rtx_base = timeout;
      timer->rtx_delay = delay;
      timer->rtx_retry = 0;
    }

    timer->rtx_last = GST_CLOCK_TIME_NONE;
    timer->num_rtx_retry = 0;
    timer->num_rtx_received = 0;
  }

  timer->type = type;
  timer->seqnum = seqnum;
  timer->num = num;

  if (timeout == -1)
    timer->timeout = -1;
  else
    timer->timeout = timeout + delay + offset;

  timer->offset = offset;
  timer->duration = duration;

  if (timer->queued)
    rtp_timer_queue_reschedule (queue, timer);
  else
    rtp_timer_queue_insert (queue, timer);
}

/**
 * rtp_timer_queue_set_expected:
 * @queue: the #RtpTimerQueue
 * @senum: the timer seqnum
 * @timeout: the timer timeout
 * @delay: the additional delay (will be added to @timeout)
 * @duration: the duration of the event related to the timer
 *
 * Specialized version of rtp_timer_queue_set_timer() that creates or updates a
 * timer with type %RTP_TIMER_EXPECTED. Expected timers do not carry
 * a timestamp, hence have no offset.
 */
void
rtp_timer_queue_set_expected (RtpTimerQueue * queue, guint16 seqnum,
    GstClockTime timeout, GstClockTime delay, GstClockTime duration)
{
  rtp_timer_queue_set_timer (queue, RTP_TIMER_EXPECTED, seqnum, 0, timeout,
      delay, duration, 0);
}

/**
 * rtp_timer_queue_set_lost:
 * @queue: the #RtpTimerQueue
 * @senum: the timer seqnum
 * @num: the number of seqnum in the range (partially supported)
 * @timeout: the timer timeout
 * @duration: the duration of the event related to the timer
 * @offset: offset that can be used to convert the timeout to timestamp
 *
 * Specialized version of rtp_timer_queue_set_timer() that creates or updates a
 * timer with type %RTP_TIMER_LOST.
 */
void
rtp_timer_queue_set_lost (RtpTimerQueue * queue, guint16 seqnum,
    guint num, GstClockTime timeout, GstClockTime duration,
    GstClockTimeDiff offset)
{
  rtp_timer_queue_set_timer (queue, RTP_TIMER_LOST, seqnum, num, timeout, 0,
      duration, offset);
}

/**
 * rtp_timer_queue_set_eos:
 * @queue: the #RtpTimerQueue
 * @timeout: the timer timeout
 * @offset: offset that can be used to convert the timeout to timestamp
 *
 * Specialized version of rtp_timer_queue_set_timer() that creates or updates a
 * timer with type %RTP_TIMER_EOS. There is only one such a timer and it has
 * the special seqnum value -1 (FIXME this is not an invalid seqnum,).
 */
void
rtp_timer_queue_set_eos (RtpTimerQueue * queue, GstClockTime timeout,
    GstClockTimeDiff offset)
{
  rtp_timer_queue_set_timer (queue, RTP_TIMER_EOS, -1, 0, timeout, 0, 0,
      offset);
}

/**
 * rtp_timer_queue_set_deadline:
 * @queue: the #RtpTimerQueue
 * @senum: the timer seqnum
 * @timeout: the timer timeout
 * @offset: offset that can be used to convert the timeout to timestamp
 *
 * Specialized version of rtp_timer_queue_set_timer() that creates or updates a
 * timer with type %RTP_TIMER_DEADLINE. There should be only one such a timer,
 * its seqnum matches the first packet to be output.
 */
void
rtp_timer_queue_set_deadline (RtpTimerQueue * queue, guint16 seqnum,
    GstClockTime timeout, GstClockTimeDiff offset)
{
  rtp_timer_queue_set_timer (queue, RTP_TIMER_DEADLINE, seqnum, 0, timeout, 0,
      0, offset);
}

/**
 * rtp_timer_queue_update_timer:
 * @queue: the #RtpTimerQueue
 * @senum: the timer seqnum
 * @timeout: the timer timeout
 * @delay: the additional delay (will be added to @timeout)
 * @offset: offset that can be used to convert the timeout to timestamp
 * @reset: if the RTX statistics should be reset
 *
 * A utility to update an already queued timer.
 */
void
rtp_timer_queue_update_timer (RtpTimerQueue * queue, RtpTimer * timer,
    guint16 seqnum, GstClockTime timeout, GstClockTime delay,
    GstClockTimeDiff offset, gboolean reset)
{
  g_return_if_fail (timer != NULL);

  if (reset) {
    GST_DEBUG ("reset rtx delay %" GST_TIME_FORMAT "->%" GST_TIME_FORMAT,
        GST_TIME_ARGS (timer->rtx_delay), GST_TIME_ARGS (delay));
    timer->rtx_base = timeout;
    timer->rtx_delay = delay;
    timer->rtx_retry = 0;
  }

  if (timer->seqnum != seqnum) {
    timer->num_rtx_retry = 0;
    timer->num_rtx_received = 0;

    if (timer->queued) {
      g_hash_table_remove (queue->hashtable, GINT_TO_POINTER (timer->seqnum));
      g_hash_table_insert (queue->hashtable, GINT_TO_POINTER (seqnum), timer);
    }
  }

  if (timeout == -1)
    timer->timeout = -1;
  else
    timer->timeout = timeout + delay + offset;

  timer->seqnum = seqnum;
  timer->offset = offset;

  if (timer->queued)
    rtp_timer_queue_reschedule (queue, timer);
  else
    rtp_timer_queue_insert (queue, timer);
}

/**
 * rtp_timer_queue_length:
 * @queue: the #RtpTimerQueue
 *
 * Returns: the number of timers in the #RtpTimerQueue
 */
guint
rtp_timer_queue_length (RtpTimerQueue * queue)
{
  return queue->timers.length;
}
