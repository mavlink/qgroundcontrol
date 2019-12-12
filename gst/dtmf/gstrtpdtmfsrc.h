/* GStreamer RTP DTMF source
 *
 * gstrtpdtmfsrc.h:
 *
 * Copyright (C) <2007> Nokia Corporation.
 *   Contact: Zeeshan Ali <zeeshan.ali@nokia.com>
 * Copyright (C) <2005> Wim Taymans <wim@fluendo.com>
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

#ifndef __GST_RTP_DTMF_SRC_H__
#define __GST_RTP_DTMF_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "gstdtmfcommon.h"

G_BEGIN_DECLS
#define GST_TYPE_RTP_DTMF_SRC		(gst_rtp_dtmf_src_get_type())
#define GST_RTP_DTMF_SRC(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_DTMF_SRC,GstRTPDTMFSrc))
#define GST_RTP_DTMF_SRC_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_DTMF_SRC,GstRTPDTMFSrcClass))
#define GST_RTP_DTMF_SRC_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTP_DTMF_SRC, GstRTPDTMFSrcClass))
#define GST_IS_RTP_DTMF_SRC(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_DTMF_SRC))
#define GST_IS_RTP_DTMF_SRC_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_DTMF_SRC))
#define GST_RTP_DTMF_SRC_CAST(obj)		((GstRTPDTMFSrc *)(obj))
typedef struct _GstRTPDTMFSrc GstRTPDTMFSrc;
typedef struct _GstRTPDTMFSrcClass GstRTPDTMFSrcClass;



enum _GstRTPDTMFEventType
{
  RTP_DTMF_EVENT_TYPE_START,
  RTP_DTMF_EVENT_TYPE_STOP,
  RTP_DTMF_EVENT_TYPE_PAUSE_TASK
};

typedef enum _GstRTPDTMFEventType GstRTPDTMFEventType;

struct _GstRTPDTMFSrcEvent
{
  GstRTPDTMFEventType event_type;
  GstRTPDTMFPayload *payload;
};

typedef struct _GstRTPDTMFSrcEvent GstRTPDTMFSrcEvent;

/**
 * GstRTPDTMFSrc:
 * @element: the parent element.
 *
 * The opaque #GstRTPDTMFSrc data structure.
 */
struct _GstRTPDTMFSrc
{
  /*< private >*/
  GstBaseSrc basesrc;

  GAsyncQueue *event_queue;
  GstClockID clockid;
  gboolean paused;
  GstRTPDTMFPayload *payload;

  GstClockTime timestamp;
  GstClockTime start_timestamp;
  gboolean first_packet;
  gboolean last_packet;
  guint32 ts_base;
  guint16 seqnum_base;
  gint16 seqnum_offset;
  guint16 seqnum;
  gint32 ts_offset;
  guint32 rtp_timestamp;
  guint pt;
  guint ssrc;
  guint current_ssrc;
  guint16 ptime;
  guint16 packet_redundancy;
  guint32 clock_rate;
  gboolean last_event_was_start;

  GstClockTime last_stop;

  gboolean dirty;
  guint16 redundancy_count;
};

struct _GstRTPDTMFSrcClass
{
  GstBaseSrcClass parent_class;
};

GType gst_rtp_dtmf_src_get_type (void);

gboolean gst_rtp_dtmf_src_plugin_init (GstPlugin * plugin);


G_END_DECLS
#endif /* __GST_RTP_DTMF_SRC_H__ */
