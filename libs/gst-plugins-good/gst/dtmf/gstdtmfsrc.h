/* GStreamer DTMF source
 *
 * gstdtmfsrc.h:
 *
 * Copyright (C) <2007> Collabora.
 *   Contact: Youness Alaoui <youness.alaoui@collabora.co.uk>
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

#ifndef __GST_DTMF_SRC_H__
#define __GST_DTMF_SRC_H__

#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/base/gstbasesrc.h>

G_BEGIN_DECLS
#define GST_TYPE_DTMF_SRC               (gst_dtmf_src_get_type())
#define GST_DTMF_SRC(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DTMF_SRC,GstDTMFSrc))
#define GST_DTMF_SRC_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DTMF_SRC,GstDTMFSrcClass))
#define GST_DTMF_SRC_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_DTMF_SRC, GstDTMFSrcClass))
#define GST_IS_DTMF_SRC(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DTMF_SRC))
#define GST_IS_DTMF_SRC_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DTMF_SRC))
#define GST_DTMF_SRC_CAST(obj)          ((GstDTMFSrc *)(obj))
typedef struct _GstDTMFSrc GstDTMFSrc;
typedef struct _GstDTMFSrcClass GstDTMFSrcClass;

enum _GstDTMFEventType
{
  DTMF_EVENT_TYPE_START,
  DTMF_EVENT_TYPE_STOP,
  DTMF_EVENT_TYPE_PAUSE_TASK
};

typedef enum _GstDTMFEventType GstDTMFEventType;

struct _GstDTMFSrcEvent
{
  GstDTMFEventType event_type;
  double sample;
  guint16 event_number;
  guint16 volume;
  guint32 packet_count;
};

typedef struct _GstDTMFSrcEvent GstDTMFSrcEvent;

/**
 * GstDTMFSrc:
 * @element: the parent element.
 *
 * The opaque #GstDTMFSrc data structure.
 */
struct _GstDTMFSrc
{
  /*< private >*/
  GstBaseSrc parent;
  GAsyncQueue *event_queue;
  GstDTMFSrcEvent *last_event;
  gboolean last_event_was_start;

  guint16 interval;
  GstClockTime timestamp;

  gboolean paused;
  GstClockID clockid;

  GstClockTime last_stop;

  gint sample_rate;
};


struct _GstDTMFSrcClass
{
  GstBaseSrcClass parent_class;
};

GType gst_dtmf_src_get_type (void);

gboolean gst_dtmf_src_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_DTMF_SRC_H__ */
