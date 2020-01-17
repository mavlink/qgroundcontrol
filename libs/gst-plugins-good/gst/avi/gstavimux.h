/* AVI muxer plugin for GStreamer
 * Copyright (C) 2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
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


#ifndef __GST_AVI_MUX_H__
#define __GST_AVI_MUX_H__


#include <gst/gst.h>
#include <gst/base/gstcollectpads.h>
#include <gst/riff/riff-ids.h>
#include "avi-ids.h"

G_BEGIN_DECLS

#define GST_TYPE_AVI_MUX \
  (gst_avi_mux_get_type())
#define GST_AVI_MUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AVI_MUX,GstAviMux))
#define GST_AVI_MUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AVI_MUX,GstAviMuxClass))
#define GST_IS_AVI_MUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AVI_MUX))
#define GST_IS_AVI_MUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AVI_MUX))

#define GST_AVI_INDEX_OF_INDEXES     0
#define GST_AVI_INDEX_OF_CHUNKS      1

/* this allows indexing up to 64GB avi file */
#define GST_AVI_SUPERINDEX_COUNT    32

/* max size */
#define GST_AVI_MAX_SIZE    0x40000000

typedef struct _gst_avi_superindex_entry {
  guint64 offset;
  guint32 size;
  guint32 duration;
} gst_avi_superindex_entry;

typedef struct _gst_riff_strh_full {
  gst_riff_strh  parent;
  /* rcFrame, RECT structure (struct of 4 shorts) */
  gint16  left;
  gint16  top;
  gint16  right;
  gint16  bottom;
} gst_riff_strh_full;

typedef struct _GstAviPad GstAviPad;
typedef struct _GstAviMux GstAviMux;
typedef struct _GstAviMuxClass GstAviMuxClass;

typedef GstFlowReturn (*GstAviPadHook) (GstAviMux * avi, GstAviPad * avipad,
                                        GstBuffer * buffer);

struct _GstAviPad {
  /* do not extend, link to it */
  /* is NULL if original sink request pad has been removed */
  GstCollectData *collect;

  /* type */
  gboolean is_video;
  gboolean connected;

  /* chunk tag */
  gchar *tag;

  /* stream header */
  gst_riff_strh hdr;

  /* odml super indexes */
  gst_avi_superindex_entry idx[GST_AVI_SUPERINDEX_COUNT];
  gint idx_index;
  gchar *idx_tag;

  /* stream specific hook */
  GstAviPadHook hook;
};

typedef struct _GstAviVideoPad {
  GstAviPad parent;

  /* stream format */
  gst_riff_strf_vids vids;
  /* extra data */
  GstBuffer *vids_codec_data;
  /* ODML video properties */
  gst_riff_vprp vprp;

  GstBuffer *prepend_buffer;

} GstAviVideoPad;

typedef struct _GstAviAudioPad {
  GstAviPad parent;

  /* stream format */
  gst_riff_strf_auds auds;
  /* audio info for bps calculation */
  guint32 audio_size;
  guint64 audio_time;
  /* max audio chunk size for vbr */
  guint32 max_audio_chunk;

  /* counts the number of samples to put in indx chunk
   * useful for raw audio where usually there are more than
   * 1 sample in each GstBuffer */
  gint samples;

  /* extra data */
  GstBuffer *auds_codec_data;
} GstAviAudioPad;

typedef struct _GstAviCollectData {
  /* extend the CollectData */
  GstCollectData collect;

  GstAviPad      *avipad;
} GstAviCollectData;

struct _GstAviMux {
  GstElement element;

  /* pads */
  GstPad              *srcpad;
  /* sinkpads, video first */
  GSList              *sinkpads;
  /* video restricted to 1 pad */
  guint               video_pads, audio_pads;
  GstCollectPads     *collect;

  /* the AVI header */
  /* still some single stream video data in mux struct */
  gst_riff_avih avi_hdr;
  /* total number of (video) frames */
  guint32 total_frames;
  /* amount of total data (bytes) */
  guint64 total_data;
  /* amount of data (bytes) in the AVI/AVIX block;
   * actually the movi list, so counted from and including the movi tag */
  guint32 data_size, datax_size;
  /* num (video) frames in the AVI/AVIX block */
  guint32 num_frames, numx_frames;
  /* size of hdrl list, including tag as usual */

  /* total size of extra codec data */
  guint32 codec_data_size;
  /* state info */
  gboolean write_header;
  gboolean restart;

  /* tags */
  GstTagList *tags_snap;

  /* information about the AVI index ('idx') */
  gst_riff_index_entry *idx;
  gint idx_index, idx_count;
  /* offset of *chunk* (relative to a base offset); entered in the index */
  guint32 idx_offset;
  /* size of idx1 chunk (including! chunk header and size bytes) */
  guint32 idx_size;

  /* are we a big file already? */
  gboolean is_bigfile;
  guint64 avix_start;

  /* whether to use "large AVI files" or just stick to small indexed files */
  gboolean enable_large_avi;
};

struct _GstAviMuxClass {
  GstElementClass parent_class;
};

GType gst_avi_mux_get_type(void);

G_END_DECLS


#endif /* __GST_AVI_MUX_H__ */
