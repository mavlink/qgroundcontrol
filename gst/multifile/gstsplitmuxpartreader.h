/* GStreamer Split Muxed File Source - Part reader
 * Copyright (C) 2014 Jan Schmidt <jan@centricular.com>
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
#ifndef __GST_SPLITMUX_PART_READER_H__
#define __GST_SPLITMUX_PART_READER_H__

#include <gst/gst.h>
#include <gst/base/gstdataqueue.h>

G_BEGIN_DECLS

#define GST_TYPE_SPLITMUX_PART_READER \
  (gst_splitmux_part_reader_get_type())
#define GST_SPLITMUX_PART_READER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SPLITMUX_PART_READER,GstSplitMuxSrc))
#define GST_SPLITMUX_PART_READER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SPLITMUX_PART_READER,GstSplitMuxSrcClass))
#define GST_IS_SPLITMUX_PART_READER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SPLITMUX_PART_READER))
#define GST_IS_SPLITMUX_PART_READER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SPLITMUX_PART_READER))

typedef struct _GstSplitMuxPartReader GstSplitMuxPartReader;
typedef struct _GstSplitMuxPartReaderClass GstSplitMuxPartReaderClass;
typedef struct _SplitMuxSrcPad SplitMuxSrcPad;
typedef struct _SplitMuxSrcPadClass SplitMuxSrcPadClass;

typedef enum
{
  PART_STATE_NULL,
  PART_STATE_PREPARING_COLLECT_STREAMS,
  PART_STATE_PREPARING_MEASURE_STREAMS,
  PART_STATE_PREPARING_RESET_FOR_READY,
  PART_STATE_READY,
  PART_STATE_FAILED,
} GstSplitMuxPartState;

typedef GstPad *(*GstSplitMuxPartReaderPadCb)(GstSplitMuxPartReader *reader, GstPad *src_pad, gpointer cb_data);

struct _GstSplitMuxPartReader
{
  GstPipeline parent;

  GstSplitMuxPartState prep_state;

  gchar *path;

  GstElement *src;
  GstElement *typefind;
  GstElement *demux;

  gboolean async_pending;
  gboolean active;
  gboolean running;
  gboolean prepared;
  gboolean flushing;
  gboolean no_more_pads;

  GstClockTime duration;
  GstClockTime start_offset;

  GList *pads;

  GCond inactive_cond;
  GMutex lock;
  GMutex type_lock;

  GstSplitMuxPartReaderPadCb get_pad_cb;
  gpointer cb_data;
};

struct _GstSplitMuxPartReaderClass
{
  GstPipelineClass parent_class;

  void (*prepared)  (GstSplitMuxPartReader *reader);
  void (*end_of_part) (GstSplitMuxPartReader *reader);
};

GType gst_splitmux_part_reader_get_type (void);

void gst_splitmux_part_reader_set_callbacks (GstSplitMuxPartReader *reader,
    gpointer cb_data, GstSplitMuxPartReaderPadCb get_pad_cb);
gboolean gst_splitmux_part_reader_prepare (GstSplitMuxPartReader *part);
void gst_splitmux_part_reader_unprepare (GstSplitMuxPartReader *part);
void gst_splitmux_part_reader_set_location (GstSplitMuxPartReader *reader,
    const gchar *path);
gboolean gst_splitmux_part_is_eos (GstSplitMuxPartReader *reader);

gboolean gst_splitmux_part_reader_activate (GstSplitMuxPartReader *part, GstSegment *seg, GstSeekFlags extra_flags);
void gst_splitmux_part_reader_deactivate (GstSplitMuxPartReader *part);
gboolean gst_splitmux_part_reader_is_active (GstSplitMuxPartReader *part);

gboolean gst_splitmux_part_reader_src_query (GstSplitMuxPartReader *part, GstPad *src_pad, GstQuery * query);
void gst_splitmux_part_reader_set_start_offset (GstSplitMuxPartReader *part, GstClockTime offset);
GstClockTime gst_splitmux_part_reader_get_start_offset (GstSplitMuxPartReader *part);
GstClockTime gst_splitmux_part_reader_get_end_offset (GstSplitMuxPartReader *part);
GstClockTime gst_splitmux_part_reader_get_duration (GstSplitMuxPartReader * reader);

GstPad *gst_splitmux_part_reader_lookup_pad (GstSplitMuxPartReader *reader, GstPad *target);
GstFlowReturn gst_splitmux_part_reader_pop (GstSplitMuxPartReader *reader, GstPad *part_pad, GstDataQueueItem ** item);

G_END_DECLS

#endif
