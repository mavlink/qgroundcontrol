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

#ifndef __GST_RTP_STORAGE_ITEM_H__
#define __GST_RTP_STORAGE_ITEM_H__

#include <gst/rtp/gstrtpbuffer.h>

GST_DEBUG_CATEGORY_EXTERN (gst_rtp_storage_debug);

typedef struct {
  GstBuffer *buffer;
  guint16 seq;
  guint8 pt;
} RtpStorageItem;

typedef struct {
  GQueue queue;
  GMutex stream_lock;
  guint32 ssrc;
  GstClockTime max_arrival_time;
} RtpStorageStream;

#define STREAM_LOCK(s)   g_mutex_lock   (&(s)->stream_lock)
#define STREAM_UNLOCK(s) g_mutex_unlock (&(s)->stream_lock)

RtpStorageStream * rtp_storage_stream_new                      (guint32 ssrc);
void               rtp_storage_stream_free                     (RtpStorageStream * stream);
void               rtp_storage_stream_resize_and_add_item      (RtpStorageStream * stream,
                                                                GstClockTime size_time,
                                                                GstBuffer *buffer,
                                                                guint8 pt,
                                                                guint16 seq);
void               rtp_storage_stream_add_item                 (RtpStorageStream * stream,
                                                                GstBuffer *buffer,
                                                                guint8 pt,
                                                                guint16 seq);
GstBufferList    * rtp_storage_stream_get_packets_for_recovery (RtpStorageStream *stream,
                                                                guint8 pt_fec,
                                                                guint16 lost_seq);
GstBuffer        * rtp_storage_stream_get_redundant_packet     (RtpStorageStream *stream,
                                                                guint16 lost_seq);

#endif /* __GST_RTP_STORAGE_ITEM_H__ */

