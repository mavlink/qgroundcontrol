/* GStreamer GdkPixbuf-based image decoder
 * Copyright (C) 1999-2001 Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2003 David A. Schleef <ds@schleef.org>
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

#ifndef __GST_GDK_PIXBUF_DEC_H__
#define __GST_GDK_PIXBUF_DEC_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define GST_TYPE_GDK_PIXBUF_DEC			\
  (gst_gdk_pixbuf_dec_get_type())
#define GST_GDK_PIXBUF_DEC(obj)						\
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GDK_PIXBUF_DEC,GstGdkPixbufDec))
#define GST_GDK_PIXBUF_DEC_CLASS(klass)					\
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GDK_PIXBUF_DEC,GstGdkPixbufDecClass))
#define GST_IS_GDK_PIXBUF_DEC(obj)					\
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GDK_PIXBUF_DEC))
#define GST_IS_GDK_PIXBUF_DEC_CLASS(klass)				\
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GDK_PIXBUF_DEC))

typedef struct _GstGdkPixbufDec      GstGdkPixbufDec;
typedef struct _GstGdkPixbufDecClass GstGdkPixbufDecClass;

struct _GstGdkPixbufDec
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  GstClockTime      last_timestamp;
  GdkPixbufLoader  *pixbuf_loader;

  gint in_fps_n, in_fps_d;

  GstVideoInfo   info;
  GstBufferPool *pool;
  GList         *pending_events;
  gboolean       packetized;
};

struct _GstGdkPixbufDecClass
{
  GstElementClass parent_class;
};

GType gst_gdk_pixbuf_dec_get_type (void);

G_END_DECLS

#endif /* __GST_GDK_PIXBUF_DEC_H__ */
