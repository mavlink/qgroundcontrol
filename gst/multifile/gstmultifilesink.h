/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2006 Wim Taymans <wim@fluendo.com>
 *                    2006 David A. Schleef <ds@schleef.org>
 *
 * gstmultifilesink.h:
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

#ifndef __GST_MULTIFILESINK_H__
#define __GST_MULTIFILESINK_H__

#include <gst/gst.h>
#include <gst/base/base.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

G_BEGIN_DECLS

#define GST_TYPE_MULTI_FILE_SINK \
  (gst_multi_file_sink_get_type())
#define GST_MULTI_FILE_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MULTI_FILE_SINK,GstMultiFileSink))
#define GST_MULTI_FILE_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MULTI_FILE_SINK,GstMultiFileSinkClass))
#define GST_IS_MULTI_FILE_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MULTI_FILE_SINK))
#define GST_IS_MULTI_FILE_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MULTI_FILE_SINK))

typedef struct _GstMultiFileSink GstMultiFileSink;
typedef struct _GstMultiFileSinkClass GstMultiFileSinkClass;

/**
 * GstMultiFileSinkNext:
 * @GST_MULTI_FILE_SINK_NEXT_BUFFER: New file for each buffer
 * @GST_MULTI_FILE_SINK_NEXT_DISCONT: New file after each discontinuity
 * @GST_MULTI_FILE_SINK_NEXT_KEY_FRAME: New file at each key frame
 *  (Useful for MPEG-TS segmenting)
 * @GST_MULTI_FILE_SINK_NEXT_KEY_UNIT_EVENT: New file after a force key unit
 *  event
 * @GST_MULTI_FILE_SINK_NEXT_MAX_SIZE: New file when the configured maximum file
 *  size would be exceeded with the next buffer or buffer list
 * @GST_MULTI_FILE_SINK_NEXT_MAX_DURATION: New file when the configured maximum duration
 *  would be exceeded with the next buffer or buffer list
 *
 * File splitting modes.
 */
typedef enum {
  GST_MULTI_FILE_SINK_NEXT_BUFFER,
  GST_MULTI_FILE_SINK_NEXT_DISCONT,
  GST_MULTI_FILE_SINK_NEXT_KEY_FRAME,
  GST_MULTI_FILE_SINK_NEXT_KEY_UNIT_EVENT,
  GST_MULTI_FILE_SINK_NEXT_MAX_SIZE,
  GST_MULTI_FILE_SINK_NEXT_MAX_DURATION
} GstMultiFileSinkNext;

struct _GstMultiFileSink
{
  GstBaseSink parent;

  gchar *filename;
  gint index;
  gboolean post_messages;
  GstMultiFileSinkNext next_file;
  FILE *file;

  guint max_files;
  GQueue old_files;        /* keep track of old files for max_files handling */

  gint64 next_segment;

  int n_streamheaders;
  GstBuffer **streamheaders;
  guint force_key_unit_count;

  guint64 cur_file_size;
  guint64 max_file_size;

  GstClockTime file_pts;
  GstClockTime max_file_duration;

  gboolean aggregate_gops;
  GstAdapter *gop_adapter;  /* to aggregate GOPs */
  GList *potential_next_gop;	/* To detect false-positives */
};

struct _GstMultiFileSinkClass
{
  GstBaseSinkClass parent_class;
};

GType gst_multi_file_sink_get_type (void);

G_END_DECLS

#endif /* __GST_MULTIFILESINK_H__ */
