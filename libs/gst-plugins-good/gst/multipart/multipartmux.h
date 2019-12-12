/* GStreamer
 * Copyright (C) 2004 Wim Taymans <wim@fluendo.com>
 *
 * gstmultipartmux.h: multipart stream muxer
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

#ifndef __GST_MULTIPART_MUX__
#define __GST_MULTIPART_MUX__

#include <gst/gst.h>
#include <gst/base/gstcollectpads.h>

#include <string.h>

G_BEGIN_DECLS

#define GST_TYPE_MULTIPART_MUX (gst_multipart_mux_get_type())
#define GST_MULTIPART_MUX(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MULTIPART_MUX, GstMultipartMux))
#define GST_MULTIPART_MUX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MULTIPART_MUX, GstMultipartMux))
#define GST_MULTIPART_MUX_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_MULTIPART_MUX, GstMultipartMuxClass))
#define GST_IS_MULTIPART_MUX(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MULTIPART_MUX))
#define GST_IS_MULTIPART_MUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MULTIPART_MUX))

typedef struct _GstMultipartMux GstMultipartMux;
typedef struct _GstMultipartMuxClass GstMultipartMuxClass;

/* all information needed for one multipart stream */
typedef struct
{
  GstCollectData collect;       /* we extend the CollectData */

  GstBuffer *buffer;            /* the queued buffer for this pad */
  GstClockTime pts_timestamp;   /* its pts timestamp, converted to running_time so that we can
                                   correctly sort over multiple segments. */
  GstClockTime dts_timestamp;   /* its dts timestamp, converted to running_time so that we can
                                   correctly sort over multiple segments. */
  GstPad *pad;
}
GstMultipartPadData;

/**
 * GstMultipartMux:
 *
 * The opaque #GstMultipartMux structure.
 */
struct _GstMultipartMux
{
  GstElement element;

  /* pad */
  GstPad *srcpad;

  /* sinkpads */
  GstCollectPads *collect;

  gint numpads;

  /* offset in stream */
  guint64 offset;

  /* boundary string */
  gchar *boundary;

  gboolean negotiated;
  gboolean need_segment;
  gboolean need_stream_start;
};

struct _GstMultipartMuxClass
{
  GstElementClass parent_class;

  GHashTable *mimetypes;
};

GType gst_multipart_mux_get_type (void);

gboolean gst_multipart_mux_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_MULTIPART_MUX__ */

