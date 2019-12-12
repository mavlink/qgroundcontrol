/* GStreamer Split Demuxer bin that recombines files created by
 * the splitmuxsink element.
 *
 * Copyright (C) <2014> Jan Schmidt <jan@centricular.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstsplitmuxsrc.h"

GST_DEBUG_CATEGORY_STATIC (splitmux_part_debug);
#define GST_CAT_DEFAULT splitmux_part_debug

#define SPLITMUX_PART_LOCK(p) g_mutex_lock(&(p)->lock)
#define SPLITMUX_PART_UNLOCK(p) g_mutex_unlock(&(p)->lock)
#define SPLITMUX_PART_WAIT(p) g_cond_wait (&(p)->inactive_cond, &(p)->lock)
#define SPLITMUX_PART_BROADCAST(p) g_cond_broadcast (&(p)->inactive_cond)

#define SPLITMUX_PART_TYPE_LOCK(p) g_mutex_lock(&(p)->type_lock)
#define SPLITMUX_PART_TYPE_UNLOCK(p) g_mutex_unlock(&(p)->type_lock)

typedef struct _GstSplitMuxPartPad
{
  GstPad parent;

  /* Reader we belong to */
  GstSplitMuxPartReader *reader;
  /* Output splitmuxsrc source pad */
  GstPad *target;

  GstDataQueue *queue;

  gboolean is_eos;
  gboolean flushing;
  gboolean seen_buffer;

  gboolean is_sparse;
  GstClockTime max_ts;
  GstSegment segment;

  GstSegment orig_segment;
  GstClockTime initial_ts_offset;
} GstSplitMuxPartPad;

typedef struct _GstSplitMuxPartPadClass
{
  GstPadClass parent;
} GstSplitMuxPartPadClass;

static GType gst_splitmux_part_pad_get_type (void);
#define SPLITMUX_TYPE_PART_PAD gst_splitmux_part_pad_get_type()
#define SPLITMUX_PART_PAD_CAST(p) ((GstSplitMuxPartPad *)(p))

static void splitmux_part_pad_constructed (GObject * pad);
static void splitmux_part_pad_finalize (GObject * pad);
static void handle_buffer_measuring (GstSplitMuxPartReader * reader,
    GstSplitMuxPartPad * part_pad, GstBuffer * buf);

static gboolean splitmux_data_queue_is_full_cb (GstDataQueue * queue,
    guint visible, guint bytes, guint64 time, gpointer checkdata);
static void type_found (GstElement * typefind, guint probability,
    GstCaps * caps, GstSplitMuxPartReader * reader);
static void check_if_pads_collected (GstSplitMuxPartReader * reader);

static void
gst_splitmux_part_reader_finish_measuring_streams (GstSplitMuxPartReader *
    reader);

/* Called with reader lock held */
static gboolean
have_empty_queue (GstSplitMuxPartReader * reader)
{
  GList *cur;

  for (cur = g_list_first (reader->pads); cur != NULL; cur = g_list_next (cur)) {
    GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (cur->data);
    if (part_pad->is_eos) {
      GST_LOG_OBJECT (part_pad, "Pad is EOS");
      return TRUE;
    }
    if (gst_data_queue_is_empty (part_pad->queue)) {
      GST_LOG_OBJECT (part_pad, "Queue is empty");
      return TRUE;
    }
  }

  return FALSE;
}

/* Called with reader lock held */
static gboolean
block_until_can_push (GstSplitMuxPartReader * reader)
{
  while (reader->running) {
    if (reader->flushing)
      goto out;
    if (reader->active && have_empty_queue (reader))
      goto out;

    GST_LOG_OBJECT (reader,
        "Waiting for activation or empty queue on reader %s", reader->path);
    SPLITMUX_PART_WAIT (reader);
  }

  GST_LOG_OBJECT (reader, "Done waiting on reader %s active %d flushing %d",
      reader->path, reader->active, reader->flushing);
out:
  return reader->active && !reader->flushing;
}

static void
handle_buffer_measuring (GstSplitMuxPartReader * reader,
    GstSplitMuxPartPad * part_pad, GstBuffer * buf)
{
  GstClockTimeDiff ts = GST_CLOCK_STIME_NONE;
  GstClockTimeDiff offset;

  if (reader->prep_state == PART_STATE_PREPARING_COLLECT_STREAMS &&
      !part_pad->seen_buffer) {
    /* If this is the first buffer on the pad in the collect_streams state,
     * then calculate initial offset based on running time of this segment */
    part_pad->initial_ts_offset =
        part_pad->orig_segment.start + part_pad->orig_segment.base -
        part_pad->orig_segment.time;
    GST_DEBUG_OBJECT (reader,
        "Initial TS offset for pad %" GST_PTR_FORMAT " now %" GST_TIME_FORMAT,
        part_pad, GST_TIME_ARGS (part_pad->initial_ts_offset));
  }
  part_pad->seen_buffer = TRUE;

  /* Adjust buffer timestamps */
  offset = reader->start_offset + part_pad->segment.base;
  offset -= part_pad->initial_ts_offset;

  /* Update the stored max duration on the pad,
   * always preferring making DTS contiguous
   * where possible */
  if (GST_BUFFER_DTS_IS_VALID (buf))
    ts = GST_BUFFER_DTS (buf) + offset;
  else if (GST_BUFFER_PTS_IS_VALID (buf))
    ts = GST_BUFFER_PTS (buf) + offset;

  GST_DEBUG_OBJECT (reader, "Pad %" GST_PTR_FORMAT
      " incoming PTS %" GST_TIME_FORMAT
      " DTS %" GST_TIME_FORMAT " offset by %" GST_STIME_FORMAT
      " to %" GST_STIME_FORMAT, part_pad,
      GST_TIME_ARGS (GST_BUFFER_DTS (buf)),
      GST_TIME_ARGS (GST_BUFFER_PTS (buf)),
      GST_STIME_ARGS (offset), GST_STIME_ARGS (ts));

  if (GST_CLOCK_STIME_IS_VALID (ts)) {
    if (GST_BUFFER_DURATION_IS_VALID (buf))
      ts += GST_BUFFER_DURATION (buf);

    if (GST_CLOCK_STIME_IS_VALID (ts)
        && ts > (GstClockTimeDiff) part_pad->max_ts) {
      part_pad->max_ts = ts;
      GST_LOG_OBJECT (reader,
          "pad %" GST_PTR_FORMAT " max TS now %" GST_TIME_FORMAT, part_pad,
          GST_TIME_ARGS (part_pad->max_ts));
    }
  }
  /* Is it time to move to measuring state yet? */
  check_if_pads_collected (reader);
}

static gboolean
splitmux_data_queue_is_full_cb (GstDataQueue * queue,
    guint visible, guint bytes, guint64 time, gpointer checkdata)
{
  /* Arbitrary safety limit. If we hit it, playback is likely to stall */
  if (time > 20 * GST_SECOND)
    return TRUE;
  return FALSE;
}

static void
splitmux_part_free_queue_item (GstDataQueueItem * item)
{
  gst_mini_object_unref (item->object);
  g_slice_free (GstDataQueueItem, item);
}

static GstFlowReturn
splitmux_part_pad_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (pad);
  GstSplitMuxPartReader *reader = part_pad->reader;
  GstDataQueueItem *item;
  GstClockTimeDiff offset;

  GST_LOG_OBJECT (reader, "Pad %" GST_PTR_FORMAT " %" GST_PTR_FORMAT, pad, buf);
  SPLITMUX_PART_LOCK (reader);

  if (reader->prep_state == PART_STATE_PREPARING_COLLECT_STREAMS ||
      reader->prep_state == PART_STATE_PREPARING_MEASURE_STREAMS) {
    handle_buffer_measuring (reader, part_pad, buf);
    gst_buffer_unref (buf);
    SPLITMUX_PART_UNLOCK (reader);
    return GST_FLOW_OK;
  }

  if (!block_until_can_push (reader)) {
    /* Flushing */
    SPLITMUX_PART_UNLOCK (reader);
    gst_buffer_unref (buf);
    return GST_FLOW_FLUSHING;
  }

  /* Adjust buffer timestamps */
  offset = reader->start_offset + part_pad->segment.base;
  offset -= part_pad->initial_ts_offset;

  if (GST_BUFFER_PTS_IS_VALID (buf))
    GST_BUFFER_PTS (buf) += offset;
  if (GST_BUFFER_DTS_IS_VALID (buf))
    GST_BUFFER_DTS (buf) += offset;

  /* We are active, and one queue is empty, place this buffer in
   * the dataqueue */
  GST_LOG_OBJECT (reader, "Enqueueing buffer %" GST_PTR_FORMAT, buf);
  item = g_slice_new (GstDataQueueItem);
  item->destroy = (GDestroyNotify) splitmux_part_free_queue_item;
  item->object = GST_MINI_OBJECT (buf);
  item->size = gst_buffer_get_size (buf);
  item->duration = GST_BUFFER_DURATION (buf);
  if (item->duration == GST_CLOCK_TIME_NONE)
    item->duration = 0;
  item->visible = TRUE;

  gst_object_ref (part_pad);

  SPLITMUX_PART_UNLOCK (reader);

  if (!gst_data_queue_push (part_pad->queue, item)) {
    splitmux_part_free_queue_item (item);
    gst_object_unref (part_pad);
    return GST_FLOW_FLUSHING;
  }

  gst_object_unref (part_pad);
  return GST_FLOW_OK;
}

/* Called with splitmux part lock held */
static gboolean
splitmux_part_is_eos_locked (GstSplitMuxPartReader * part)
{
  GList *cur;
  for (cur = g_list_first (part->pads); cur != NULL; cur = g_list_next (cur)) {
    GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (cur->data);
    if (!part_pad->is_eos)
      return FALSE;
  }

  return TRUE;
}

static gboolean
splitmux_part_is_prerolled_locked (GstSplitMuxPartReader * part)
{
  GList *cur;
  GST_LOG_OBJECT (part, "Checking for preroll");
  for (cur = g_list_first (part->pads); cur != NULL; cur = g_list_next (cur)) {
    GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (cur->data);
    if (!part_pad->seen_buffer) {
      GST_LOG_OBJECT (part, "Part pad %" GST_PTR_FORMAT " is not prerolled",
          part_pad);
      return FALSE;
    }
  }
  GST_LOG_OBJECT (part, "Part is prerolled");
  return TRUE;
}


gboolean
gst_splitmux_part_is_eos (GstSplitMuxPartReader * reader)
{
  gboolean res;

  SPLITMUX_PART_LOCK (reader);
  res = splitmux_part_is_eos_locked (reader);
  SPLITMUX_PART_UNLOCK (reader);

  return res;
}

/* Called with splitmux part lock held */
static gboolean
splitmux_is_flushing (GstSplitMuxPartReader * reader)
{
  GList *cur;
  for (cur = g_list_first (reader->pads); cur != NULL; cur = g_list_next (cur)) {
    GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (cur->data);
    if (part_pad->flushing)
      return TRUE;
  }

  return FALSE;
}

static gboolean
splitmux_part_pad_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (pad);
  GstSplitMuxPartReader *reader = part_pad->reader;
  gboolean ret = TRUE;
  SplitMuxSrcPad *target;
  GstDataQueueItem *item;

  SPLITMUX_PART_LOCK (reader);

  target = gst_object_ref (part_pad->target);

  GST_LOG_OBJECT (reader, "Pad %" GST_PTR_FORMAT " event %" GST_PTR_FORMAT, pad,
      event);

  if (part_pad->flushing && GST_EVENT_TYPE (event) != GST_EVENT_FLUSH_STOP)
    goto drop_event;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_STREAM_START:{
      GstStreamFlags flags;
      gst_event_parse_stream_flags (event, &flags);
      part_pad->is_sparse = (flags & GST_STREAM_FLAG_SPARSE);
      break;
    }
    case GST_EVENT_SEGMENT:{
      GstSegment *seg = &part_pad->segment;

      GST_LOG_OBJECT (pad, "Received segment %" GST_PTR_FORMAT, event);

      gst_event_copy_segment (event, seg);
      gst_event_copy_segment (event, &part_pad->orig_segment);

      if (seg->format != GST_FORMAT_TIME)
        goto wrong_segment;

      /* Adjust segment */
      /* Adjust start/stop so the overall file is 0 + start_offset based */
      if (seg->stop != -1) {
        seg->stop -= seg->start;
        seg->stop += seg->time + reader->start_offset;
      }
      seg->start = seg->time + reader->start_offset;
      seg->time += reader->start_offset;
      seg->position += reader->start_offset;

      GST_LOG_OBJECT (pad, "Adjusted segment now %" GST_PTR_FORMAT, event);

      /* Replace event */
      gst_event_unref (event);
      event = gst_event_new_segment (seg);

      if (reader->prep_state != PART_STATE_PREPARING_COLLECT_STREAMS
          && reader->prep_state != PART_STATE_PREPARING_MEASURE_STREAMS)
        break;                  /* Only do further stuff with segments during initial measuring */

      /* Take the first segment from the first part */
      if (target->segment.format == GST_FORMAT_UNDEFINED) {
        gst_segment_copy_into (seg, &target->segment);
        GST_DEBUG_OBJECT (reader,
            "Target pad segment now %" GST_SEGMENT_FORMAT, &target->segment);
      }

      if (seg->stop != -1 && target->segment.stop != -1) {
        GstClockTime stop = seg->base + seg->stop;
        if (stop > target->segment.stop) {
          target->segment.stop = stop;
          GST_DEBUG_OBJECT (reader,
              "Adjusting segment stop by %" GST_TIME_FORMAT
              " output now %" GST_SEGMENT_FORMAT,
              GST_TIME_ARGS (reader->start_offset), &target->segment);
        }
      }
      GST_LOG_OBJECT (pad, "Forwarding segment %" GST_PTR_FORMAT, event);
      break;
    }
    case GST_EVENT_EOS:{

      GST_DEBUG_OBJECT (part_pad,
          "State %u EOS event. MaxTS seen %" GST_TIME_FORMAT,
          reader->prep_state, GST_TIME_ARGS (part_pad->max_ts));

      if (reader->prep_state == PART_STATE_PREPARING_COLLECT_STREAMS ||
          reader->prep_state == PART_STATE_PREPARING_MEASURE_STREAMS) {
        /* Mark this pad as EOS */
        part_pad->is_eos = TRUE;
        if (splitmux_part_is_eos_locked (reader)) {
          /* Finished measuring things, set state and tell the state change func
           * so it can seek back to the start */
          GST_LOG_OBJECT (reader,
              "EOS while measuring streams. Resetting for ready");
          reader->prep_state = PART_STATE_PREPARING_RESET_FOR_READY;

          gst_element_call_async (GST_ELEMENT_CAST (reader),
              (GstElementCallAsyncFunc)
              gst_splitmux_part_reader_finish_measuring_streams, NULL, NULL);
        }
        goto drop_event;
      }
      break;
    }
    case GST_EVENT_FLUSH_START:
      reader->flushing = TRUE;
      part_pad->flushing = TRUE;
      GST_LOG_OBJECT (reader, "Pad %" GST_PTR_FORMAT " flushing dataqueue",
          part_pad);
      gst_data_queue_set_flushing (part_pad->queue, TRUE);
      SPLITMUX_PART_BROADCAST (reader);
      break;
    case GST_EVENT_FLUSH_STOP:{
      gst_data_queue_set_flushing (part_pad->queue, FALSE);
      gst_data_queue_flush (part_pad->queue);
      part_pad->seen_buffer = FALSE;
      part_pad->flushing = FALSE;
      part_pad->is_eos = FALSE;

      reader->flushing = splitmux_is_flushing (reader);
      GST_LOG_OBJECT (reader,
          "%s pad %" GST_PTR_FORMAT " flush_stop. Overall flushing=%d",
          reader->path, pad, reader->flushing);
      SPLITMUX_PART_BROADCAST (reader);
      break;
    }
    default:
      break;
  }

  /* Don't send events downstream while preparing */
  if (reader->prep_state != PART_STATE_READY)
    goto drop_event;

  /* Don't pass flush events - those are done by the parent */
  if (GST_EVENT_TYPE (event) == GST_EVENT_FLUSH_START ||
      GST_EVENT_TYPE (event) == GST_EVENT_FLUSH_STOP)
    goto drop_event;

  if (!block_until_can_push (reader))
    goto drop_event;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_GAP:{
      /* FIXME: Drop initial gap (if any) in each segment, not all GAPs */
      goto drop_event;
    }
    default:
      break;
  }

  /* We are active, and one queue is empty, place this buffer in
   * the dataqueue */
  gst_object_ref (part_pad->queue);
  SPLITMUX_PART_UNLOCK (reader);

  GST_LOG_OBJECT (reader, "Enqueueing event %" GST_PTR_FORMAT, event);
  item = g_slice_new (GstDataQueueItem);
  item->destroy = (GDestroyNotify) splitmux_part_free_queue_item;
  item->object = GST_MINI_OBJECT (event);
  item->size = 0;
  item->duration = 0;
  if (item->duration == GST_CLOCK_TIME_NONE)
    item->duration = 0;
  item->visible = FALSE;

  if (!gst_data_queue_push (part_pad->queue, item)) {
    splitmux_part_free_queue_item (item);
    ret = FALSE;
  }

  gst_object_unref (part_pad->queue);
  gst_object_unref (target);

  return ret;
wrong_segment:
  gst_event_unref (event);
  gst_object_unref (target);
  SPLITMUX_PART_UNLOCK (reader);
  GST_ELEMENT_ERROR (reader, STREAM, FAILED, (NULL),
      ("Received non-time segment - reader %s pad %" GST_PTR_FORMAT,
          reader->path, pad));
  return FALSE;
drop_event:
  GST_LOG_OBJECT (pad, "Dropping event %" GST_PTR_FORMAT
      " from %" GST_PTR_FORMAT " on %" GST_PTR_FORMAT, event, pad, target);
  gst_event_unref (event);
  gst_object_unref (target);
  SPLITMUX_PART_UNLOCK (reader);
  return TRUE;
}

static gboolean
splitmux_part_pad_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (pad);
  GstSplitMuxPartReader *reader = part_pad->reader;
  GstPad *target;
  gboolean ret = FALSE;
  gboolean active;

  SPLITMUX_PART_LOCK (reader);
  target = gst_object_ref (part_pad->target);
  active = reader->active;
  SPLITMUX_PART_UNLOCK (reader);

  if (active) {
    GST_LOG_OBJECT (pad, "Forwarding query %" GST_PTR_FORMAT
        " from %" GST_PTR_FORMAT " on %" GST_PTR_FORMAT, query, pad, target);

    ret = gst_pad_query (target, query);
  }

  gst_object_unref (target);

  return ret;
}

G_DEFINE_TYPE (GstSplitMuxPartPad, gst_splitmux_part_pad, GST_TYPE_PAD);

static void
splitmux_part_pad_constructed (GObject * pad)
{
  gst_pad_set_chain_function (GST_PAD (pad),
      GST_DEBUG_FUNCPTR (splitmux_part_pad_chain));
  gst_pad_set_event_function (GST_PAD (pad),
      GST_DEBUG_FUNCPTR (splitmux_part_pad_event));
  gst_pad_set_query_function (GST_PAD (pad),
      GST_DEBUG_FUNCPTR (splitmux_part_pad_query));

  G_OBJECT_CLASS (gst_splitmux_part_pad_parent_class)->constructed (pad);
}

static void
gst_splitmux_part_pad_class_init (GstSplitMuxPartPadClass * klass)
{
  GObjectClass *gobject_klass = (GObjectClass *) (klass);

  gobject_klass->constructed = splitmux_part_pad_constructed;
  gobject_klass->finalize = splitmux_part_pad_finalize;
}

static void
gst_splitmux_part_pad_init (GstSplitMuxPartPad * pad)
{
  pad->queue = gst_data_queue_new (splitmux_data_queue_is_full_cb,
      NULL, NULL, pad);
  gst_segment_init (&pad->segment, GST_FORMAT_UNDEFINED);
  gst_segment_init (&pad->orig_segment, GST_FORMAT_UNDEFINED);
}

static void
splitmux_part_pad_finalize (GObject * obj)
{
  GstSplitMuxPartPad *pad = (GstSplitMuxPartPad *) (obj);

  GST_DEBUG_OBJECT (obj, "finalize");
  gst_data_queue_set_flushing (pad->queue, TRUE);
  gst_data_queue_flush (pad->queue);
  gst_object_unref (GST_OBJECT_CAST (pad->queue));
  pad->queue = NULL;

  G_OBJECT_CLASS (gst_splitmux_part_pad_parent_class)->finalize (obj);
}

static void
new_decoded_pad_added_cb (GstElement * element, GstPad * pad,
    GstSplitMuxPartReader * part);
static void no_more_pads (GstElement * element, GstSplitMuxPartReader * reader);
static GstStateChangeReturn
gst_splitmux_part_reader_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_splitmux_part_reader_send_event (GstElement * element,
    GstEvent * event);
static void gst_splitmux_part_reader_set_flushing_locked (GstSplitMuxPartReader
    * part, gboolean flushing);
static void bus_handler (GstBin * bin, GstMessage * msg);
static void splitmux_part_reader_dispose (GObject * object);
static void splitmux_part_reader_finalize (GObject * object);
static void splitmux_part_reader_reset (GstSplitMuxPartReader * reader);

#define gst_splitmux_part_reader_parent_class parent_class
G_DEFINE_TYPE (GstSplitMuxPartReader, gst_splitmux_part_reader,
    GST_TYPE_PIPELINE);

static void
gst_splitmux_part_reader_class_init (GstSplitMuxPartReaderClass * klass)
{
  GObjectClass *gobject_klass = (GObjectClass *) (klass);
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBinClass *gstbin_class = (GstBinClass *) klass;

  GST_DEBUG_CATEGORY_INIT (splitmux_part_debug, "splitmuxpartreader", 0,
      "Split File Demuxing Source helper");

  gobject_klass->dispose = splitmux_part_reader_dispose;
  gobject_klass->finalize = splitmux_part_reader_finalize;

  gstelement_class->change_state = gst_splitmux_part_reader_change_state;
  gstelement_class->send_event = gst_splitmux_part_reader_send_event;

  gstbin_class->handle_message = bus_handler;
}

static void
gst_splitmux_part_reader_init (GstSplitMuxPartReader * reader)
{
  GstElement *typefind;

  reader->active = FALSE;
  reader->duration = GST_CLOCK_TIME_NONE;

  g_cond_init (&reader->inactive_cond);
  g_mutex_init (&reader->lock);
  g_mutex_init (&reader->type_lock);

  /* FIXME: Create elements on a state change */
  reader->src = gst_element_factory_make ("filesrc", NULL);
  if (reader->src == NULL) {
    GST_ERROR_OBJECT (reader, "Failed to create filesrc element");
    return;
  }
  gst_bin_add (GST_BIN_CAST (reader), reader->src);

  typefind = gst_element_factory_make ("typefind", NULL);
  if (!typefind) {
    GST_ERROR_OBJECT (reader,
        "Failed to create typefind element - check your installation");
    return;
  }

  gst_bin_add (GST_BIN_CAST (reader), typefind);
  reader->typefind = typefind;

  if (!gst_element_link_pads (reader->src, NULL, typefind, "sink")) {
    GST_ERROR_OBJECT (reader,
        "Failed to link typefind element - check your installation");
    return;
  }

  g_signal_connect (reader->typefind, "have-type", G_CALLBACK (type_found),
      reader);
}

static void
splitmux_part_reader_dispose (GObject * object)
{
  GstSplitMuxPartReader *reader = (GstSplitMuxPartReader *) object;

  splitmux_part_reader_reset (reader);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
splitmux_part_reader_finalize (GObject * object)
{
  GstSplitMuxPartReader *reader = (GstSplitMuxPartReader *) object;

  g_cond_clear (&reader->inactive_cond);
  g_mutex_clear (&reader->lock);
  g_mutex_clear (&reader->type_lock);

  g_free (reader->path);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
do_async_start (GstSplitMuxPartReader * reader)
{
  GstMessage *message;

  GST_STATE_LOCK (reader);
  reader->async_pending = TRUE;

  message = gst_message_new_async_start (GST_OBJECT_CAST (reader));
  GST_BIN_CLASS (parent_class)->handle_message (GST_BIN_CAST (reader), message);
  GST_STATE_UNLOCK (reader);
}

static void
do_async_done (GstSplitMuxPartReader * reader)
{
  GstMessage *message;

  GST_STATE_LOCK (reader);
  if (reader->async_pending) {
    message =
        gst_message_new_async_done (GST_OBJECT_CAST (reader),
        GST_CLOCK_TIME_NONE);
    GST_BIN_CLASS (parent_class)->handle_message (GST_BIN_CAST (reader),
        message);

    reader->async_pending = FALSE;
  }
  GST_STATE_UNLOCK (reader);
}

static void
splitmux_part_reader_reset (GstSplitMuxPartReader * reader)
{
  GList *cur;

  SPLITMUX_PART_LOCK (reader);
  for (cur = g_list_first (reader->pads); cur != NULL; cur = g_list_next (cur)) {
    GstPad *pad = GST_PAD_CAST (cur->data);
    gst_pad_set_active (GST_PAD_CAST (pad), FALSE);
    gst_object_unref (GST_OBJECT_CAST (pad));
  }

  g_list_free (reader->pads);
  reader->pads = NULL;
  SPLITMUX_PART_UNLOCK (reader);
}

static GstSplitMuxPartPad *
gst_splitmux_part_reader_new_proxy_pad (GstSplitMuxPartReader * reader,
    GstPad * target)
{
  GstSplitMuxPartPad *pad = g_object_new (SPLITMUX_TYPE_PART_PAD,
      "name", GST_PAD_NAME (target),
      "direction", GST_PAD_SINK,
      NULL);
  pad->target = target;
  pad->reader = reader;

  gst_pad_set_active (GST_PAD_CAST (pad), TRUE);

  return pad;
}

static void
new_decoded_pad_added_cb (GstElement * element, GstPad * pad,
    GstSplitMuxPartReader * reader)
{
  GstPad *out_pad = NULL;
  GstSplitMuxPartPad *proxy_pad;
  GstCaps *caps;
  GstPadLinkReturn link_ret;

  caps = gst_pad_get_current_caps (pad);

  GST_DEBUG_OBJECT (reader, "file %s new decoded pad %" GST_PTR_FORMAT
      " caps %" GST_PTR_FORMAT, reader->path, pad, caps);

  gst_caps_unref (caps);

  /* Look up or create the output pad */
  if (reader->get_pad_cb)
    out_pad = reader->get_pad_cb (reader, pad, reader->cb_data);
  if (out_pad == NULL) {
    GST_DEBUG_OBJECT (reader,
        "No output pad for %" GST_PTR_FORMAT ". Ignoring", pad);
    return;
  }

  /* Create our proxy pad to interact with this new pad */
  proxy_pad = gst_splitmux_part_reader_new_proxy_pad (reader, out_pad);
  GST_DEBUG_OBJECT (reader,
      "created proxy pad %" GST_PTR_FORMAT " for target %" GST_PTR_FORMAT,
      proxy_pad, out_pad);

  link_ret = gst_pad_link (pad, GST_PAD (proxy_pad));
  if (link_ret != GST_PAD_LINK_OK) {
    gst_object_unref (proxy_pad);
    GST_ELEMENT_ERROR (reader, STREAM, FAILED, (NULL),
        ("Failed to link proxy pad for stream part %s pad %" GST_PTR_FORMAT
            " ret %d", reader->path, pad, link_ret));
    return;
  }
  GST_DEBUG_OBJECT (reader,
      "new decoded pad %" GST_PTR_FORMAT " linked to %" GST_PTR_FORMAT,
      pad, proxy_pad);

  SPLITMUX_PART_LOCK (reader);
  reader->pads = g_list_prepend (reader->pads, proxy_pad);
  SPLITMUX_PART_UNLOCK (reader);
}

static gboolean
gst_splitmux_part_reader_send_event (GstElement * element, GstEvent * event)
{
  GstSplitMuxPartReader *reader = (GstSplitMuxPartReader *) element;
  gboolean ret = FALSE;
  GstPad *pad = NULL;

  /* Send event to the first source pad we found */
  SPLITMUX_PART_LOCK (reader);
  if (reader->pads) {
    GstPad *proxy_pad = GST_PAD_CAST (reader->pads->data);
    pad = gst_pad_get_peer (proxy_pad);
  }
  SPLITMUX_PART_UNLOCK (reader);

  if (pad) {
    ret = gst_pad_send_event (pad, event);
    gst_object_unref (pad);
  } else {
    gst_event_unref (event);
  }

  return ret;
}

/* Called with lock held. Seeks to an 'internal' time from 0 to length of this piece */
static void
gst_splitmux_part_reader_seek_to_time_locked (GstSplitMuxPartReader * reader,
    GstClockTime time)
{
  SPLITMUX_PART_UNLOCK (reader);
  GST_DEBUG_OBJECT (reader, "Seeking to time %" GST_TIME_FORMAT,
      GST_TIME_ARGS (time));
  gst_element_seek (GST_ELEMENT_CAST (reader), 1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET, time,
      GST_SEEK_TYPE_END, 0);

  SPLITMUX_PART_LOCK (reader);

  /* Wait for flush to finish, so old data is gone */
  while (reader->flushing) {
    GST_LOG_OBJECT (reader, "%s Waiting for flush to finish", reader->path);
    SPLITMUX_PART_WAIT (reader);
  }
}

/* Map the passed segment to 'internal' time from 0 to length of this piece and seek. Lock cannot be held */
static gboolean
gst_splitmux_part_reader_seek_to_segment (GstSplitMuxPartReader * reader,
    GstSegment * target_seg, GstSeekFlags extra_flags)
{
  GstSeekFlags flags;
  GstClockTime start = 0, stop = GST_CLOCK_TIME_NONE;

  flags = target_seg->flags | GST_SEEK_FLAG_FLUSH | extra_flags;

  SPLITMUX_PART_LOCK (reader);
  if (target_seg->start >= reader->start_offset)
    start = target_seg->start - reader->start_offset;
  /* If the segment stop is within this part, don't play to the end */
  if (target_seg->stop != -1 &&
      target_seg->stop < reader->start_offset + reader->duration)
    stop = target_seg->stop - reader->start_offset;

  SPLITMUX_PART_UNLOCK (reader);

  GST_DEBUG_OBJECT (reader,
      "Seeking rate %f format %d flags 0x%x start %" GST_TIME_FORMAT " stop %"
      GST_TIME_FORMAT, target_seg->rate, target_seg->format, flags,
      GST_TIME_ARGS (start), GST_TIME_ARGS (stop));

  return gst_element_seek (GST_ELEMENT_CAST (reader), target_seg->rate,
      target_seg->format, flags, GST_SEEK_TYPE_SET, start, GST_SEEK_TYPE_SET,
      stop);
}

/* Called with lock held */
static void
gst_splitmux_part_reader_measure_streams (GstSplitMuxPartReader * reader)
{
  SPLITMUX_PART_LOCK (reader);
  /* Trigger a flushing seek to near the end of the file and run each stream
   * to EOS in order to find the smallest end timestamp to start the next
   * file from
   */
  if (GST_CLOCK_TIME_IS_VALID (reader->duration)
      && reader->duration > GST_SECOND) {
    GstClockTime seek_ts = reader->duration - (0.5 * GST_SECOND);
    gst_splitmux_part_reader_seek_to_time_locked (reader, seek_ts);
  }
  SPLITMUX_PART_UNLOCK (reader);
}

static void
gst_splitmux_part_reader_finish_measuring_streams (GstSplitMuxPartReader *
    reader)
{
  SPLITMUX_PART_LOCK (reader);
  if (reader->prep_state == PART_STATE_PREPARING_RESET_FOR_READY) {
    /* Fire the prepared signal and go to READY state */
    GST_DEBUG_OBJECT (reader,
        "Stream measuring complete. File %s is now ready", reader->path);
    reader->prep_state = PART_STATE_READY;
    SPLITMUX_PART_UNLOCK (reader);
    do_async_done (reader);
  } else {
    SPLITMUX_PART_UNLOCK (reader);
  }
}

static GstElement *
find_demuxer (GstCaps * caps)
{
  GList *factories =
      gst_element_factory_list_get_elements (GST_ELEMENT_FACTORY_TYPE_DEMUXER,
      GST_RANK_MARGINAL);
  GList *compat_elements;
  GstElement *e = NULL;

  if (factories == NULL)
    return NULL;

  compat_elements =
      gst_element_factory_list_filter (factories, caps, GST_PAD_SINK, TRUE);

  if (compat_elements) {
    /* Just take the first (highest ranked) option */
    GstElementFactory *factory =
        GST_ELEMENT_FACTORY_CAST (compat_elements->data);
    e = gst_element_factory_create (factory, NULL);
    gst_plugin_feature_list_free (compat_elements);
  }

  if (factories)
    gst_plugin_feature_list_free (factories);

  return e;
}

static void
type_found (GstElement * typefind, guint probability,
    GstCaps * caps, GstSplitMuxPartReader * reader)
{
  GstElement *demux;

  GST_INFO_OBJECT (reader, "Got type %" GST_PTR_FORMAT, caps);

  /* typefind found a type. Look for the demuxer to handle it */
  demux = reader->demux = find_demuxer (caps);
  if (reader->demux == NULL) {
    GST_ERROR_OBJECT (reader, "Failed to create demuxer element");
    return;
  }

  /* Connect to demux signals */
  g_signal_connect (demux,
      "pad-added", G_CALLBACK (new_decoded_pad_added_cb), reader);
  g_signal_connect (demux, "no-more-pads", G_CALLBACK (no_more_pads), reader);

  gst_element_set_locked_state (demux, TRUE);
  gst_bin_add (GST_BIN_CAST (reader), demux);
  gst_element_link_pads (reader->typefind, "src", demux, NULL);
  gst_element_set_state (reader->demux, GST_STATE_TARGET (reader));
  gst_element_set_locked_state (demux, FALSE);
}

static void
check_if_pads_collected (GstSplitMuxPartReader * reader)
{
  if (reader->prep_state == PART_STATE_PREPARING_COLLECT_STREAMS) {
    /* Check we have all pads and each pad has seen a buffer */
    if (reader->no_more_pads && splitmux_part_is_prerolled_locked (reader)) {
      GST_DEBUG_OBJECT (reader,
          "no more pads - file %s. Measuring stream length", reader->path);
      reader->prep_state = PART_STATE_PREPARING_MEASURE_STREAMS;
      gst_element_call_async (GST_ELEMENT_CAST (reader),
          (GstElementCallAsyncFunc) gst_splitmux_part_reader_measure_streams,
          NULL, NULL);
    }
  }
}

static void
no_more_pads (GstElement * element, GstSplitMuxPartReader * reader)
{
  GstClockTime duration = GST_CLOCK_TIME_NONE;
  GList *cur;
  /* Query the minimum duration of any pad in this piece and store it.
   * FIXME: Only consider audio and video */
  SPLITMUX_PART_LOCK (reader);
  for (cur = g_list_first (reader->pads); cur != NULL; cur = g_list_next (cur)) {
    GstPad *target = GST_PAD_CAST (cur->data);
    if (target) {
      gint64 cur_duration;
      if (gst_pad_peer_query_duration (target, GST_FORMAT_TIME, &cur_duration)) {
        GST_INFO_OBJECT (reader,
            "file %s pad %" GST_PTR_FORMAT " duration %" GST_TIME_FORMAT,
            reader->path, target, GST_TIME_ARGS (cur_duration));
        if (cur_duration < duration)
          duration = cur_duration;
      }
    }
  }
  GST_INFO_OBJECT (reader, "file %s duration %" GST_TIME_FORMAT,
      reader->path, GST_TIME_ARGS (duration));
  reader->duration = (GstClockTime) duration;

  reader->no_more_pads = TRUE;

  check_if_pads_collected (reader);
  SPLITMUX_PART_UNLOCK (reader);
}

gboolean
gst_splitmux_part_reader_src_query (GstSplitMuxPartReader * part,
    GstPad * src_pad, GstQuery * query)
{
  GstPad *target = NULL;
  gboolean ret;
  GList *cur;

  SPLITMUX_PART_LOCK (part);
  /* Find the pad corresponding to the visible output target pad */
  for (cur = g_list_first (part->pads); cur != NULL; cur = g_list_next (cur)) {
    GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (cur->data);
    if (part_pad->target == src_pad) {
      target = gst_object_ref (GST_OBJECT_CAST (part_pad));
      break;
    }
  }
  SPLITMUX_PART_UNLOCK (part);

  if (target == NULL)
    return FALSE;

  ret = gst_pad_peer_query (target, query);

  if (ret == FALSE)
    goto out;

  /* Post-massaging of queries */
  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:{
      GstFormat fmt;
      gint64 position;

      gst_query_parse_position (query, &fmt, &position);
      if (fmt != GST_FORMAT_TIME)
        return FALSE;
      SPLITMUX_PART_LOCK (part);
      position += part->start_offset;
      GST_LOG_OBJECT (part, "Position %" GST_TIME_FORMAT,
          GST_TIME_ARGS (position));
      SPLITMUX_PART_UNLOCK (part);

      gst_query_set_position (query, fmt, position);
      break;
    }
    default:
      break;
  }

out:
  gst_object_unref (target);
  return ret;
}

static GstStateChangeReturn
gst_splitmux_part_reader_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstSplitMuxPartReader *reader = (GstSplitMuxPartReader *) element;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:{
      break;
    }
    case GST_STATE_CHANGE_READY_TO_PAUSED:{
      SPLITMUX_PART_LOCK (reader);
      g_object_set (reader->src, "location", reader->path, NULL);
      reader->prep_state = PART_STATE_PREPARING_COLLECT_STREAMS;
      gst_splitmux_part_reader_set_flushing_locked (reader, FALSE);
      reader->running = TRUE;
      SPLITMUX_PART_UNLOCK (reader);

      /* we go to PAUSED asynchronously once all streams have been collected
       * and seeks to measure the stream lengths are done */
      do_async_start (reader);
      break;
    }
    case GST_STATE_CHANGE_READY_TO_NULL:
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      SPLITMUX_PART_LOCK (reader);
      gst_splitmux_part_reader_set_flushing_locked (reader, TRUE);
      reader->running = FALSE;
      SPLITMUX_PART_BROADCAST (reader);
      SPLITMUX_PART_UNLOCK (reader);
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      SPLITMUX_PART_LOCK (reader);
      reader->active = FALSE;
      gst_splitmux_part_reader_set_flushing_locked (reader, TRUE);
      SPLITMUX_PART_BROADCAST (reader);
      SPLITMUX_PART_UNLOCK (reader);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    do_async_done (reader);
    goto beach;
  }

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      ret = GST_STATE_CHANGE_ASYNC;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      do_async_done (reader);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      SPLITMUX_PART_LOCK (reader);
      gst_splitmux_part_reader_set_flushing_locked (reader, FALSE);
      reader->active = TRUE;
      SPLITMUX_PART_BROADCAST (reader);
      SPLITMUX_PART_UNLOCK (reader);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      reader->prep_state = PART_STATE_NULL;
      splitmux_part_reader_reset (reader);
      break;
    default:
      break;
  }

beach:
  return ret;
}

gboolean
gst_splitmux_part_reader_prepare (GstSplitMuxPartReader * part)
{
  GstStateChangeReturn ret;

  ret = gst_element_set_state (GST_ELEMENT_CAST (part), GST_STATE_PAUSED);

  if (ret == GST_STATE_CHANGE_FAILURE)
    return FALSE;

  return TRUE;
}

void
gst_splitmux_part_reader_unprepare (GstSplitMuxPartReader * part)
{
  gst_element_set_state (GST_ELEMENT_CAST (part), GST_STATE_NULL);
}

void
gst_splitmux_part_reader_set_location (GstSplitMuxPartReader * reader,
    const gchar * path)
{
  reader->path = g_strdup (path);
}

gboolean
gst_splitmux_part_reader_activate (GstSplitMuxPartReader * reader,
    GstSegment * seg, GstSeekFlags extra_flags)
{
  GST_DEBUG_OBJECT (reader, "Activating part reader");

  if (!gst_splitmux_part_reader_seek_to_segment (reader, seg, extra_flags)) {
    GST_ERROR_OBJECT (reader, "Failed to seek part to %" GST_SEGMENT_FORMAT,
        seg);
    return FALSE;
  }
  if (gst_element_set_state (GST_ELEMENT_CAST (reader),
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    GST_ERROR_OBJECT (reader, "Failed to set state to PLAYING");
    return FALSE;
  }
  return TRUE;
}

gboolean
gst_splitmux_part_reader_is_active (GstSplitMuxPartReader * part)
{
  gboolean ret;

  SPLITMUX_PART_LOCK (part);
  ret = part->active;
  SPLITMUX_PART_UNLOCK (part);

  return ret;
}

void
gst_splitmux_part_reader_deactivate (GstSplitMuxPartReader * reader)
{
  GST_DEBUG_OBJECT (reader, "Deactivating reader");
  gst_element_set_state (GST_ELEMENT_CAST (reader), GST_STATE_PAUSED);
}

void
gst_splitmux_part_reader_set_flushing_locked (GstSplitMuxPartReader * reader,
    gboolean flushing)
{
  GList *cur;

  GST_LOG_OBJECT (reader, "%s dataqueues",
      flushing ? "Flushing" : "Done flushing");
  for (cur = g_list_first (reader->pads); cur != NULL; cur = g_list_next (cur)) {
    GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (cur->data);
    gst_data_queue_set_flushing (part_pad->queue, flushing);
    if (flushing)
      gst_data_queue_flush (part_pad->queue);
  }
};

void
gst_splitmux_part_reader_set_callbacks (GstSplitMuxPartReader * reader,
    gpointer cb_data, GstSplitMuxPartReaderPadCb get_pad_cb)
{
  reader->cb_data = cb_data;
  reader->get_pad_cb = get_pad_cb;
}

GstClockTime
gst_splitmux_part_reader_get_end_offset (GstSplitMuxPartReader * reader)
{
  GList *cur;
  GstClockTime ret = GST_CLOCK_TIME_NONE;

  SPLITMUX_PART_LOCK (reader);
  for (cur = g_list_first (reader->pads); cur != NULL; cur = g_list_next (cur)) {
    GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (cur->data);
    if (!part_pad->is_sparse && part_pad->max_ts < ret)
      ret = part_pad->max_ts;
  }

  SPLITMUX_PART_UNLOCK (reader);

  return ret;
}

void
gst_splitmux_part_reader_set_start_offset (GstSplitMuxPartReader * reader,
    GstClockTime offset)
{
  SPLITMUX_PART_LOCK (reader);
  reader->start_offset = offset;
  GST_INFO_OBJECT (reader, "TS offset now %" GST_TIME_FORMAT,
      GST_TIME_ARGS (offset));
  SPLITMUX_PART_UNLOCK (reader);
}

GstClockTime
gst_splitmux_part_reader_get_start_offset (GstSplitMuxPartReader * reader)
{
  GstClockTime ret = GST_CLOCK_TIME_NONE;

  SPLITMUX_PART_LOCK (reader);
  ret = reader->start_offset;
  SPLITMUX_PART_UNLOCK (reader);

  return ret;
}

GstClockTime
gst_splitmux_part_reader_get_duration (GstSplitMuxPartReader * reader)
{
  GstClockTime dur;

  SPLITMUX_PART_LOCK (reader);
  dur = reader->duration;
  SPLITMUX_PART_UNLOCK (reader);

  return dur;
}

GstPad *
gst_splitmux_part_reader_lookup_pad (GstSplitMuxPartReader * reader,
    GstPad * target)
{
  GstPad *result = NULL;
  GList *cur;

  SPLITMUX_PART_LOCK (reader);
  for (cur = g_list_first (reader->pads); cur != NULL; cur = g_list_next (cur)) {
    GstSplitMuxPartPad *part_pad = SPLITMUX_PART_PAD_CAST (cur->data);
    if (part_pad->target == target) {
      result = (GstPad *) gst_object_ref (part_pad);
      break;
    }
  }
  SPLITMUX_PART_UNLOCK (reader);

  return result;
}

GstFlowReturn
gst_splitmux_part_reader_pop (GstSplitMuxPartReader * reader, GstPad * pad,
    GstDataQueueItem ** item)
{
  GstSplitMuxPartPad *part_pad = (GstSplitMuxPartPad *) (pad);
  GstDataQueue *q;
  GstFlowReturn ret;

  /* Get one item from the appropriate dataqueue */
  SPLITMUX_PART_LOCK (reader);
  if (reader->prep_state == PART_STATE_FAILED) {
    SPLITMUX_PART_UNLOCK (reader);
    return GST_FLOW_ERROR;
  }

  q = gst_object_ref (part_pad->queue);

  /* Have to drop the lock around pop, so we can be woken up for flush */
  SPLITMUX_PART_UNLOCK (reader);
  if (!gst_data_queue_pop (q, item) || (*item == NULL)) {
    ret = GST_FLOW_FLUSHING;
    goto out;
  }

  SPLITMUX_PART_LOCK (reader);

  SPLITMUX_PART_BROADCAST (reader);
  if (GST_IS_EVENT ((*item)->object)) {
    GstEvent *e = (GstEvent *) ((*item)->object);
    /* Mark this pad as EOS */
    if (GST_EVENT_TYPE (e) == GST_EVENT_EOS)
      part_pad->is_eos = TRUE;
  }

  SPLITMUX_PART_UNLOCK (reader);

  ret = GST_FLOW_OK;
out:
  gst_object_unref (q);
  return ret;
}

static void
bus_handler (GstBin * bin, GstMessage * message)
{
  GstSplitMuxPartReader *reader = (GstSplitMuxPartReader *) bin;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      /* Make sure to set the state to failed and wake up the listener
       * on error */
      SPLITMUX_PART_LOCK (reader);
      GST_ERROR_OBJECT (reader, "Got error message from child %" GST_PTR_FORMAT
          " marking this reader as failed", GST_MESSAGE_SRC (message));
      reader->prep_state = PART_STATE_FAILED;
      SPLITMUX_PART_BROADCAST (reader);
      SPLITMUX_PART_UNLOCK (reader);
      do_async_done (reader);
      break;
    default:
      break;
  }

  GST_BIN_CLASS (parent_class)->handle_message (bin, message);
}
