/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2005 Wim Taymans <wim@fluendo.com>
 *
 * gstossdmabuffer.h: 
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

#ifndef __GST_OSSDMABUFFER_H__
#define __GST_OSSDMABUFFER_H__

#include <gst/gst.h>

#include "gstosshelper.h"
#include <gst/audio/gstringbuffer.h>

G_BEGIN_DECLS

#define GST_TYPE_OSSDMABUFFER            (gst_ossdmabuffer_get_type())
#define GST_OSSDMABUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_OSSDMABUFFER,GstOssDMABuffer))
#define GST_OSSDMABUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_OSSDMABUFFER,GstOssDMABufferClass))
#define GST_IS_OSSDMABUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_OSSDMABUFFER))
#define GST_IS_OSSDMABUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_OSSDMABUFFER))

#define GST_OSSELEMENT_GET(obj)  GST_OSSELEMENT (obj->element)

typedef enum {
  GST_OSSDMABUFFER_OPEN         = (1 << 0),
} GstOssDMABufferFlags;

typedef struct _GstOssDMABuffer GstOssDMABuffer;
typedef struct _GstOssDMABufferClass GstOssDMABufferClass;

#define GST_OSSDMABUFFER_THREAD(buf)   (GST_OSSDMABUFFER(buf)->thread)
#define GST_OSSDMABUFFER_LOCK          GST_OBJECT_LOCK
#define GST_OSSDMABUFFER_UNLOCK        GST_OBJECT_UNLOCK
#define GST_OSSDMABUFFER_COND(buf)     (GST_OSSDMABUFFER(buf)->cond)
#define GST_OSSDMABUFFER_SIGNAL(buf)   (g_cond_signal (GST_OSSDMABUFFER_COND (buf)))
#define GST_OSSDMABUFFER_WAIT(buf)     (g_cond_wait (GST_OSSDMABUFFER_COND (buf), GST_OBJECT_GET_LOCK (buf)))

struct _GstOssDMABuffer {
  GstRingBuffer  buffer;

  GstOssElement *element;

  int            fd;
  int            caps;
  int            frag;

  GThread       *thread;
  GCond         *cond;
  gboolean       running;
};

struct _GstOssDMABufferClass {
  GstRingBufferClass parent_class;
};

GType gst_ossdmabuffer_get_type(void);

G_END_DECLS

#endif /* __GST_OSSDMABUFFER_H__ */
