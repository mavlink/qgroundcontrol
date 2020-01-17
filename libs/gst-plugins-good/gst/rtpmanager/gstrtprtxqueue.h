/* RTP muxer element for GStreamer
 *
 * gstrtpmux.h:
 *
 * Copyright (C) <2007> Nokia Corporation.
 *   Contact: Zeeshan Ali <zeeshan.ali@nokia.com>
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *               2000,2005 Wim Taymans <wim@fluendo.com>
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

#ifndef __GST_RTP_RTX_QUEUE_H__
#define __GST_RTP_RTX_QUEUE_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbuffer.h>

G_BEGIN_DECLS
#define GST_TYPE_RTP_RTX_QUEUE (gst_rtp_rtx_queue_get_type())
#define GST_RTP_RTX_QUEUE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_RTX_QUEUE, GstRTPRtxQueue))
#define GST_RTP_RTX_QUEUE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_RTX_QUEUE, GstRTPRtxQueueClass))
#define GST_RTP_RTX_QUEUE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTP_RTX_QUEUE, GstRTPRtxQueueClass))
#define GST_IS_RTP_RTX_QUEUE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_RTX_QUEUE))
#define GST_IS_RTP_RTX_QUEUE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_RTX_QUEUE))
typedef struct _GstRTPRtxQueue GstRTPRtxQueue;
typedef struct _GstRTPRtxQueueClass GstRTPRtxQueueClass;

/**
 * GstRTPRtxQueue:
 *
 * The opaque #GstRTPRtxQueue structure.
 */
struct _GstRTPRtxQueue
{
  GstElement element;

  /* pad */
  GstPad *sinkpad;
  GstPad *srcpad;

  GMutex lock;
  GQueue *queue;
  GList *pending;

  guint max_size_time;
  guint max_size_packets;

  GstSegment head_segment;
  GstSegment tail_segment;

  /* Statistics */
  guint n_requests;
  guint n_fulfilled_requests;
};

struct _GstRTPRtxQueueClass
{
  GstElementClass parent_class;
};


GType gst_rtp_rtx_queue_get_type (void);
gboolean gst_rtp_rtx_queue_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_RTP_RTX_QUEUE_H__ */
