/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_UDPSINK_H__
#define __GST_UDPSINK_H__

#include <gst/gst.h>
#include "gstmultiudpsink.h"

G_BEGIN_DECLS

#include "gstudpnetutils.h"

#define GST_TYPE_UDPSINK                (gst_udpsink_get_type())
#define GST_UDPSINK(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_UDPSINK,GstUDPSink))
#define GST_UDPSINK_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_UDPSINK,GstUDPSinkClass))
#define GST_IS_UDPSINK(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_UDPSINK))
#define GST_IS_UDPSINK_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_UDPSINK))

typedef struct _GstUDPSink GstUDPSink;
typedef struct _GstUDPSinkClass GstUDPSinkClass;

struct _GstUDPSink {
  GstMultiUDPSink parent;

  gchar *host;
  guint16 port;

  gchar *uri;
};

struct _GstUDPSinkClass {
  GstMultiUDPSinkClass parent_class;
};

GType gst_udpsink_get_type(void);

G_END_DECLS

#endif /* __GST_UDPSINK_H__ */
