/* GStreamer
 * Copyright (C) 2006 Sjoerd Simons <sjoerd@luon.net>
 * Copyright (C) 2004 Wim Taymans <wim@fluendo.com>
 *
 * gstmultipartdemux.h: multipart stream demuxer
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

#ifndef __GST_MULTIPART_DEMUX__
#define __GST_MULTIPART_DEMUX__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

#include <string.h>

G_BEGIN_DECLS

#define GST_TYPE_MULTIPART_DEMUX (gst_multipart_demux_get_type())
#define GST_MULTIPART_DEMUX(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MULTIPART_DEMUX, GstMultipartDemux))
#define GST_MULTIPART_DEMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MULTIPART_DEMUX, GstMultipartDemux))
#define GST_MULTIPART_DEMUX_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_MULTIPART_DEMUX, GstMultipartDemuxClass))
#define GST_IS_MULTIPART_DEMUX(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MULTIPART_DEMUX))
#define GST_IS_MULTIPART_DEMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MULTIPART_DEMUX))

typedef struct _GstMultipartDemux GstMultipartDemux;
typedef struct _GstMultipartDemuxClass GstMultipartDemuxClass;

#define MULTIPART_NEED_MORE_DATA -1
#define MULTIPART_DATA_ERROR     -2
#define MULTIPART_DATA_EOS       -3

/* all information needed for one multipart stream */
typedef struct
{
  GstPad *pad;                  /* reference for this pad is held by element we belong to */

  gchar *mime;

  GstClockTime  last_ts;        /* last timestamp to make sure we don't send
                                 * two buffers with the same timestamp */
  GstFlowReturn last_ret;

  gboolean      discont;
}
GstMultipartPad;

/**
 * GstMultipartDemux:
 *
 * The opaque #GstMultipartDemux structure.
 */
struct _GstMultipartDemux
{
  GstElement element;

  /* pad */
  GstPad *sinkpad;

  GSList *srcpads;
  guint numpads;

  GstAdapter *adapter;

  /* Header information of the current frame */
  gboolean header_completed;
  gchar *boundary;
  guint boundary_len;
  gchar *mime_type;
  gint content_length;

  /* Index inside the current data when manually looking for the boundary */
  gint scanpos;

  gboolean singleStream;

  /* to handle stream-start */
  gboolean have_group_id;
  guint group_id;
};

struct _GstMultipartDemuxClass
{
  GstElementClass parent_class;

  GHashTable *gstnames;
};

GType gst_multipart_demux_get_type (void);

gboolean gst_multipart_demux_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_MULTIPART_DEMUX__ */

