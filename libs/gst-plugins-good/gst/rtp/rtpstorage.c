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

#include <gst/rtp/gstrtpbuffer.h>

#include "rtpstorage.h"
#include "rtpstoragestream.h"

#define GST_CAT_DEFAULT (gst_rtp_storage_debug)

enum
{
  SIGNAL_PACKET_RECOVERED,
  LAST_SIGNAL,
};

static guint rtp_storage_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (RtpStorage, rtp_storage, G_TYPE_OBJECT);

#define STORAGE_LOCK(s)   g_mutex_lock   (&(s)->streams_lock)
#define STORAGE_UNLOCK(s) g_mutex_unlock (&(s)->streams_lock)
#define DEFAULT_SIZE_TIME (0)

static void
rtp_storage_init (RtpStorage * self)
{
  self->size_time = DEFAULT_SIZE_TIME;
  self->streams = g_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) rtp_storage_stream_free);
  g_mutex_init (&self->streams_lock);
}

static void
rtp_storage_dispose (GObject * obj)
{
  RtpStorage *self = RTP_STORAGE (obj);
  STORAGE_LOCK (self);
  g_hash_table_unref (self->streams);
  self->streams = NULL;
  STORAGE_UNLOCK (self);
  g_mutex_clear (&self->streams_lock);
  G_OBJECT_CLASS (rtp_storage_parent_class)->dispose (obj);
}

static void
rtp_storage_class_init (RtpStorageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  rtp_storage_signals[SIGNAL_PACKET_RECOVERED] =
      g_signal_new ("packet-recovered", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_BUFFER);

  gobject_class->dispose = rtp_storage_dispose;
}

GstBufferList *
rtp_storage_get_packets_for_recovery (RtpStorage * self, gint fec_pt,
    guint32 ssrc, guint16 lost_seq)
{
  GstBufferList *ret = NULL;
  RtpStorageStream *stream;

  if (0 == self->size_time) {
    GST_WARNING_OBJECT (self, "Received request for recovery RTP packets"
        " around lost_seqnum=%u fec_pt=%u for ssrc=%08x, but size is 0",
        lost_seq, fec_pt, ssrc);
    return NULL;
  }

  STORAGE_LOCK (self);
  stream = g_hash_table_lookup (self->streams, GUINT_TO_POINTER (ssrc));
  STORAGE_UNLOCK (self);

  if (NULL == stream) {
    GST_ERROR_OBJECT (self, "Can't find ssrc = 0x08%x", ssrc);
  } else {
    STREAM_LOCK (stream);
    if (stream->queue.length > 0) {
      GST_LOG_OBJECT (self, "Looking for recovery packets for fec_pt=%u around"
          " lost_seq=%u for ssrc=%08x", fec_pt, lost_seq, ssrc);
      ret =
          rtp_storage_stream_get_packets_for_recovery (stream, fec_pt,
          lost_seq);
    } else {
      GST_DEBUG_OBJECT (self, "Empty RTP storage for ssrc=%08x", ssrc);
    }
    STREAM_UNLOCK (stream);
  }

  return ret;
}

GstBuffer *
rtp_storage_get_redundant_packet (RtpStorage * self, guint32 ssrc,
    guint16 lost_seq)
{
  GstBuffer *ret = NULL;
  RtpStorageStream *stream;

  if (0 == self->size_time) {
    GST_WARNING_OBJECT (self, "Received request for redundant RTP packet with"
        " seq=%u for ssrc=%08x, but size is 0", lost_seq, ssrc);
    return NULL;
  }

  STORAGE_LOCK (self);
  stream = g_hash_table_lookup (self->streams, GUINT_TO_POINTER (ssrc));
  STORAGE_UNLOCK (self);

  if (NULL == stream) {
    GST_ERROR_OBJECT (self, "Can't find ssrc = 0x%x", ssrc);
  } else {
    STREAM_LOCK (stream);
    if (stream->queue.length > 0) {
      ret = rtp_storage_stream_get_redundant_packet (stream, lost_seq);
    } else {
      GST_DEBUG_OBJECT (self, "Empty RTP storage for ssrc=%08x", ssrc);
    }
    STREAM_UNLOCK (stream);
  }

  return ret;
}

static void
rtp_storage_do_put_recovered_packet (RtpStorage * self,
    GstBuffer * buffer, guint8 pt, guint32 ssrc, guint16 seq)
{
  RtpStorageStream *stream;

  STORAGE_LOCK (self);
  stream = g_hash_table_lookup (self->streams, GUINT_TO_POINTER (ssrc));
  STORAGE_UNLOCK (self);

  g_assert (stream);

  GST_LOG_OBJECT (self,
      "Storing recovered RTP packet with ssrc=%08x pt=%u seq=%u %"
      GST_PTR_FORMAT, ssrc, pt, seq, buffer);

  STREAM_LOCK (stream);
  rtp_storage_stream_add_item (stream, buffer, pt, seq);
  STREAM_UNLOCK (stream);
}

void
rtp_storage_put_recovered_packet (RtpStorage * self,
    GstBuffer * buffer, guint8 pt, guint32 ssrc, guint16 seq)
{
  rtp_storage_do_put_recovered_packet (self, buffer, pt, ssrc, seq);
  g_signal_emit (self, rtp_storage_signals[SIGNAL_PACKET_RECOVERED], 0, buffer);
}

gboolean
rtp_storage_append_buffer (RtpStorage * self, GstBuffer * buf)
{
  GstRTPBuffer rtpbuf = GST_RTP_BUFFER_INIT;
  RtpStorageStream *stream;
  guint32 ssrc;
  guint8 pt;
  guint16 seq;

  if (0 == self->size_time)
    return TRUE;

  /* We are about to save it in the queue, it so it is better take a ref before
   * mapping the buffer */
  gst_buffer_ref (buf);

  if (!gst_rtp_buffer_map (buf, GST_MAP_READ |
          GST_RTP_BUFFER_MAP_FLAG_SKIP_PADDING, &rtpbuf)) {
    gst_buffer_unref (buf);
    return TRUE;
  }

  ssrc = gst_rtp_buffer_get_ssrc (&rtpbuf);
  pt = gst_rtp_buffer_get_payload_type (&rtpbuf);
  seq = gst_rtp_buffer_get_seq (&rtpbuf);

  STORAGE_LOCK (self);

  stream = g_hash_table_lookup (self->streams, GUINT_TO_POINTER (ssrc));
  if (NULL == stream) {
    GST_DEBUG_OBJECT (self,
        "New media stream (ssrc=0x%08x, pt=%u) detected", ssrc, pt);
    stream = rtp_storage_stream_new (ssrc);
    g_hash_table_insert (self->streams, GUINT_TO_POINTER (ssrc), stream);
  }

  STORAGE_UNLOCK (self);

  GST_LOG_OBJECT (self,
      "Storing RTP packet with ssrc=%08x pt=%u seq=%u %" GST_PTR_FORMAT,
      ssrc, pt, seq, buf);

  STREAM_LOCK (stream);

  /* Saving the buffer, now the storage owns it */
  rtp_storage_stream_resize_and_add_item (stream, self->size_time, buf, pt,
      seq);

  STREAM_UNLOCK (stream);

  gst_rtp_buffer_unmap (&rtpbuf);

  if (GST_BUFFER_FLAG_IS_SET (buf, GST_RTP_BUFFER_FLAG_REDUNDANT)) {
    gst_buffer_unref (buf);
    return FALSE;
  }

  return TRUE;
}

void
rtp_storage_clear (RtpStorage * self)
{
  STORAGE_LOCK (self);
  g_hash_table_remove_all (self->streams);
  STORAGE_UNLOCK (self);
}

void
rtp_storage_set_size (RtpStorage * self, GstClockTime size)
{
  self->size_time = size;
  if (0 == self->size_time)
    rtp_storage_clear (self);
}

GstClockTime
rtp_storage_get_size (RtpStorage * self)
{
  return self->size_time;
}

RtpStorage *
rtp_storage_new (void)
{
  return g_object_new (RTP_TYPE_STORAGE, NULL);
}
