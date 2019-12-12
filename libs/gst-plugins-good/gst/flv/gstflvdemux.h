/* GStreamer
 * Copyright (C) <2007> Julien Moutte <julien@moutte.net>
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

#ifndef __FLV_DEMUX_H__
#define __FLV_DEMUX_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/base/gstflowcombiner.h>
#include "gstindex.h"

G_BEGIN_DECLS
#define GST_TYPE_FLV_DEMUX \
  (gst_flv_demux_get_type())
#define GST_FLV_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_FLV_DEMUX,GstFlvDemux))
#define GST_FLV_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_FLV_DEMUX,GstFlvDemuxClass))
#define GST_IS_FLV_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_FLV_DEMUX))
#define GST_IS_FLV_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_FLV_DEMUX))
typedef struct _GstFlvDemux GstFlvDemux;
typedef struct _GstFlvDemuxClass GstFlvDemuxClass;

typedef enum
{
  FLV_STATE_HEADER,
  FLV_STATE_TAG_TYPE,
  FLV_STATE_TAG_VIDEO,
  FLV_STATE_TAG_AUDIO,
  FLV_STATE_TAG_SCRIPT,
  FLV_STATE_SEEK,
  FLV_STATE_DONE,
  FLV_STATE_SKIP,
  FLV_STATE_NONE
} GstFlvDemuxState;

struct _GstFlvDemux
{
  GstElement element;

  GstPad *sinkpad;

  GstPad *audio_pad;
  GstPad *video_pad;

  gboolean have_group_id;
  guint group_id;

  /* <private> */
  
  GstIndex *index;
  gint index_id;
  gboolean own_index;
  
  GArray * times;
  GArray * filepositions;

  GstAdapter *adapter;

  GstFlowCombiner *flowcombiner;

  GstSegment segment;

  GstEvent *new_seg_event;

  GstTagList *taglist;
  GstTagList *audio_tags;
  GstTagList *video_tags;

  GstFlvDemuxState state;

  guint64 offset;
  guint64 cur_tag_offset;
  GstClockTime duration;
  guint64 tag_size;
  guint64 tag_data_size;

  /* Audio infos */
  guint16 rate;
  guint16 channels;
  guint16 width;
  guint16 audio_codec_tag;
  guint64 audio_offset;
  gboolean audio_need_discont;
  gboolean audio_need_segment;
  gboolean audio_linked;
  GstBuffer * audio_codec_data;
  GstClockTime audio_start;
  guint32 last_audio_pts;
  GstClockTime audio_time_offset;

  /* Video infos */
  guint32 w;
  guint32 h;
  guint32 par_x;
  guint32 par_y;
  guint16 video_codec_tag;
  guint64 video_offset;
  gboolean video_need_discont;
  gboolean video_need_segment;
  gboolean video_linked;
  gboolean got_par;
  GstBuffer * video_codec_data;
  GstClockTime video_start;
  guint32 last_video_dts;
  GstClockTime video_time_offset;
  gdouble framerate;

  gboolean random_access;
  gboolean need_header;
  gboolean has_audio;
  gboolean has_video;
  gboolean strict;
  gboolean flushing;

  gboolean no_more_pads;

#ifndef GST_DISABLE_DEBUG
  gboolean no_audio_warned;
  gboolean no_video_warned;
#endif

  gboolean seeking;
  gboolean building_index;
  gboolean indexed; /* TRUE if index is completely built */
  gboolean upstream_seekable; /* TRUE if upstream is seekable */
  gint64 file_size;
  GstEvent *seek_event;
  gint64 seek_time;

  GstClockTime index_max_time;
  gint64 index_max_pos;

  /* reverse playback */
  GstClockTime video_first_ts;
  GstClockTime audio_first_ts;
  gboolean video_done;
  gboolean audio_done;
  gint64 from_offset;
  gint64 to_offset;
};

struct _GstFlvDemuxClass
{
  GstElementClass parent_class;
};

GType gst_flv_demux_get_type (void);

G_END_DECLS
#endif /* __FLV_DEMUX_H__ */
