/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2011 Debarshi Ray <rishi@gnu.org>
 *
 * matroska-parse.h: matroska file/stream parseer definition
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

#ifndef __GST_MATROSKA_PARSE_H__
#define __GST_MATROSKA_PARSE_H__

#include <gst/gst.h>

#include "ebml-read.h"
#include "matroska-ids.h"
#include "matroska-read-common.h"

G_BEGIN_DECLS

#define GST_TYPE_MATROSKA_PARSE \
  (gst_matroska_parse_get_type ())
#define GST_MATROSKA_PARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_MATROSKA_PARSE, GstMatroskaParse))
#define GST_MATROSKA_PARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_MATROSKA_PARSE, GstMatroskaParseClass))
#define GST_IS_MATROSKA_PARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_MATROSKA_PARSE))
#define GST_IS_MATROSKA_PARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_MATROSKA_PARSE))

typedef struct _GstMatroskaParse {
  GstElement              parent;

  /* < private > */

  GstMatroskaReadCommon    common;

  /* pads */
  GstPad                  *srcpad;
  GstClock                *clock;
  guint                    num_v_streams;
  guint                    num_a_streams;
  guint                    num_t_streams;

  GstBuffer *streamheader;
  gboolean pushed_headers;
  GstClockTime last_timestamp;

  /* state */
  //gboolean                 streaming;
  guint64                  seek_block;
  gboolean                 seek_first;

  /* did we parse cues/tracks/segmentinfo already? */
  gboolean                 tracks_parsed;
  GList                   *seek_parsed;

  /* keeping track of playback position */
  gboolean                 segment_running;
  GstClockTime             last_stop_end;

  GstEvent                *close_segment;
  GstEvent                *new_segment;

  /* some state saving */
  GstClockTime             cluster_time;
  guint64                  cluster_offset;
  guint64                  first_cluster_offset;
  guint64                  next_cluster_offset;

  /* index stuff */
  gboolean                 seekable;
  gboolean                 building_index;
  guint64                  index_offset;
  GstEvent                *seek_event;
  gboolean                 need_newsegment;

  /* reverse playback */
  GArray                  *seek_index;
  gint                     seek_entry;
} GstMatroskaParse;

typedef struct _GstMatroskaParseClass {
  GstElementClass parent;
} GstMatroskaParseClass;

gboolean gst_matroska_parse_plugin_init (GstPlugin *plugin);

G_END_DECLS

#endif /* __GST_MATROSKA_PARSE_H__ */
