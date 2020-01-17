/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2011 Debarshi Ray <rishi@gnu.org>
 *
 * matroska-read-common.h: shared by matroska file/stream demuxer and parser
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

#ifndef __GST_MATROSKA_READ_COMMON_H__
#define __GST_MATROSKA_READ_COMMON_H__

#include <glib.h>
#include <gst/gst.h>
#include <gst/base/gstadapter.h>

#include "matroska-ids.h"

G_BEGIN_DECLS

GST_DEBUG_CATEGORY_EXTERN(matroskareadcommon_debug);

typedef enum {
  GST_MATROSKA_READ_STATE_START,
  GST_MATROSKA_READ_STATE_SEGMENT,
  GST_MATROSKA_READ_STATE_HEADER,
  GST_MATROSKA_READ_STATE_DATA,
  GST_MATROSKA_READ_STATE_SEEK,
  GST_MATROSKA_READ_STATE_SCANNING
} GstMatroskaReadState;

typedef struct _GstMatroskaReadCommon {
#if 0
  GstIndex                *element_index;
  gint                     element_index_writer_id;
#endif

  /* pads */
  GstPad                  *sinkpad;
  GPtrArray               *src;
  guint                    num_streams;

  /* metadata */
  gchar                   *muxing_app;
  gchar                   *writing_app;
  gint64                   created;

  /* state */
  GstMatroskaReadState     state;

  /* stream type */
  gboolean                 is_webm;
  gboolean                 has_video;

  /* did we parse cues/tracks/segmentinfo already? */
  gboolean                 index_parsed;
  gboolean                 segmentinfo_parsed;
  gboolean                 attachments_parsed;
  gboolean                 chapters_parsed;
  GList                   *tags_parsed;

  /* chapters stuff */
  /* Internal toc is used to keep track of the internal UID
   * which are different from the external StringUID used
   * in the user toc */
  GstToc                  *toc;
  GstToc                  *internal_toc;
  gboolean                toc_updated;

  /* start-of-segment and length */
  guint64                  ebml_segment_start;
  guint64                  ebml_segment_length;

  /* a cue (index) table */
  GArray                  *index;

  /* timescale in the file */
  guint64                  time_scale;

  /* keeping track of playback position */
  GstSegment               segment;

  GstTagList              *global_tags;
  gboolean                 global_tags_changed;

  /* pull mode caching */
  GstBuffer *cached_buffer;
  guint8 *cached_data;
  GstMapInfo cached_map;

  /* push and pull mode */
  guint64                  offset;

  guint64                  start_resync_offset;

  /* state to restore after scanning for invalid data */
  gint                     state_to_restore;

  /* push based mode usual suspects */
  GstAdapter              *adapter;

  /* cache for track tags that forward-reference their tracks */
  GHashTable *cached_track_taglists ;

} GstMatroskaReadCommon;

GstFlowReturn gst_matroska_decode_content_encodings (GArray * encodings);
gboolean gst_matroska_decode_data (GArray * encodings, gpointer * data_out,
    gsize * size_out, GstMatroskaTrackEncodingScope scope, gboolean free);
gboolean
gst_matroska_parse_protection_meta (gpointer * data_out, gsize * size_out,
    GstStructure * info_protect, gboolean * encrypted);
gint gst_matroska_index_seek_find (GstMatroskaIndex * i1, GstClockTime * time,
    gpointer user_data);
GstMatroskaIndex * gst_matroska_read_common_do_index_seek (
    GstMatroskaReadCommon * common, GstMatroskaTrackContext * track, gint64
    seek_pos, GArray ** _index, gint * _entry_index, GstSearchMode snap_dir);
void gst_matroska_read_common_found_global_tag (GstMatroskaReadCommon * common,
    GstElement * el, GstTagList * taglist);
gint64 gst_matroska_read_common_get_length (GstMatroskaReadCommon * common);
GstMatroskaTrackContext * gst_matroska_read_common_get_seek_track (
    GstMatroskaReadCommon * common, GstMatroskaTrackContext * track);
GstFlowReturn gst_matroska_read_common_parse_index (GstMatroskaReadCommon *
    common, GstEbmlRead * ebml);
GstFlowReturn gst_matroska_read_common_parse_info (GstMatroskaReadCommon *
    common, GstElement * el, GstEbmlRead * ebml);
GstFlowReturn gst_matroska_read_common_parse_attachments (
    GstMatroskaReadCommon * common, GstElement * el, GstEbmlRead * ebml);
GstFlowReturn gst_matroska_read_common_parse_chapters (GstMatroskaReadCommon *
    common, GstEbmlRead * ebml);
GstFlowReturn gst_matroska_read_common_parse_header (GstMatroskaReadCommon *
    common, GstEbmlRead * ebml);
GstFlowReturn gst_matroska_read_common_parse_metadata (GstMatroskaReadCommon *
    common, GstElement * el, GstEbmlRead * ebml);
GstFlowReturn gst_matroska_read_common_parse_skip (GstMatroskaReadCommon *
    common, GstEbmlRead * ebml, const gchar * parent_name, guint id);
GstFlowReturn gst_matroska_read_common_peek_bytes (GstMatroskaReadCommon *
    common, guint64 offset, guint size, GstBuffer ** p_buf, guint8 ** bytes);
GstFlowReturn gst_matroska_read_common_peek_id_length_pull (GstMatroskaReadCommon *
    common, GstElement * el, guint32 * _id, guint64 * _length, guint *
    _needed);
GstFlowReturn gst_matroska_read_common_peek_id_length_push (GstMatroskaReadCommon *
    common, GstElement * el, guint32 * _id, guint64 * _length, guint *
    _needed);
gint gst_matroska_read_common_stream_from_num (GstMatroskaReadCommon * common,
    guint track_num);
GstFlowReturn gst_matroska_read_common_read_track_encodings (
    GstMatroskaReadCommon * common, GstEbmlRead * ebml,
    GstMatroskaTrackContext * context);
void gst_matroska_read_common_reset_streams (GstMatroskaReadCommon * common,
    GstClockTime time, gboolean full);
void gst_matroska_read_common_free_parsed_el (gpointer mem, gpointer user_data);
void gst_matroska_read_common_init (GstMatroskaReadCommon * ctx);
void gst_matroska_read_common_finalize (GstMatroskaReadCommon * ctx);
void gst_matroska_read_common_reset (GstElement * element,
    GstMatroskaReadCommon * ctx);
gboolean gst_matroska_read_common_tracknumber_unique (GstMatroskaReadCommon *
    common, guint64 num);

G_END_DECLS

#endif /* __GST_MATROSKA_READ_COMMON_H__ */
