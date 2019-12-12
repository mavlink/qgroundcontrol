/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2006> Nokia Corporation (contact <stefan.kost@nokia.com>)
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

#ifndef __GST_AVI_DEMUX_H__
#define __GST_AVI_DEMUX_H__

#include <gst/gst.h>

#include "avi-ids.h"
#include "gst/riff/riff-ids.h"
#include "gst/riff/riff-read.h"
#include <gst/base/gstadapter.h>
#include <gst/base/gstflowcombiner.h>

G_BEGIN_DECLS

#define GST_TYPE_AVI_DEMUX \
  (gst_avi_demux_get_type ())
#define GST_AVI_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_AVI_DEMUX, GstAviDemux))
#define GST_AVI_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_AVI_DEMUX, GstAviDemuxClass))
#define GST_IS_AVI_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_AVI_DEMUX))
#define GST_IS_AVI_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_AVI_DEMUX))

#define GST_AVI_DEMUX_MAX_STREAMS       16

#define CHUNKID_TO_STREAMNR(chunkid) \
  ((((chunkid) & 0xff) - '0') * 10 + \
   (((chunkid) >> 8) & 0xff) - '0')


/* new index entries 24 bytes */
typedef struct {
  guint32        flags;
  guint32        size;    /* bytes of the data */
  guint64        offset;  /* data offset in file */
  guint64        total;   /* total bytes before */
} GstAviIndexEntry;

typedef struct {
  /* index of this streamcontext */
  guint          num;

  /* pad*/
  GstPad        *pad;
  gboolean       exposed;

  /* stream info and headers */
  gst_riff_strh *strh;
  union {
    gst_riff_strf_vids *vids;
    gst_riff_strf_auds *auds;
    gst_riff_strf_iavs *iavs;
    gpointer     data;
  } strf;
  GstBuffer     *extradata, *initdata;
  GstBuffer     *rgb8_palette;
  gchar         *name;

  /* the start/step/stop entries */
  guint          start_entry;
  guint          step_entry;
  guint          stop_entry;

  /* current index entry */
  guint          current_entry;
  /* position (byte, frame, time) for current_entry */
  guint          current_total;
  GstClockTime   current_timestamp;
  GstClockTime   current_ts_end;
  guint64        current_offset;
  guint64        current_offset_end;

  gboolean       discont;

  /* stream length */
  guint64        total_bytes;
  guint32        total_blocks;
  guint          n_keyframes;
  /* stream length according to index */
  GstClockTime   idx_duration;
  /* stream length according to header */
  GstClockTime   hdr_duration;
  /* stream length based on header/index */
  GstClockTime   duration;

  /* VBR indicator */
  gboolean       is_vbr;

  /* openDML support (for files >4GB) */
  gboolean       superindex;
  guint64       *indexes;

  /* new indexes */
  GstAviIndexEntry *index;     /* array with index entries */
  guint             idx_n;     /* number of entries */
  guint             idx_max;   /* max allocated size of entries */

  GstTagList	*taglist;

  gint           index_id;
  gboolean is_raw;
  gsize alignment;
} GstAviStream;

typedef enum {
  GST_AVI_DEMUX_START,
  GST_AVI_DEMUX_HEADER,
  GST_AVI_DEMUX_MOVI,
  GST_AVI_DEMUX_SEEK,
} GstAviDemuxState;

typedef enum {
  GST_AVI_DEMUX_HEADER_TAG_LIST,
  GST_AVI_DEMUX_HEADER_AVIH,
  GST_AVI_DEMUX_HEADER_ELEMENTS,
  GST_AVI_DEMUX_HEADER_INFO,
  GST_AVI_DEMUX_HEADER_JUNK,
  GST_AVI_DEMUX_HEADER_DATA
} GstAviDemuxHeaderState;

typedef struct _GstAviDemux {
  GstElement     parent;

  /* pads */
  GstPad        *sinkpad;

  /* AVI decoding state */
  GstAviDemuxState state;
  GstAviDemuxHeaderState header_state;
  guint64        offset;
  gboolean       abort_buffering;

  /* when we loaded the indexes */
  gboolean       have_index;
  /* index offset in the file */
  guint64        index_offset;

  /* streams */
  GstAviStream   stream[GST_AVI_DEMUX_MAX_STREAMS];
  guint          num_streams;
  guint          num_v_streams;
  guint          num_a_streams;
  guint          num_t_streams;   /* subtitle text streams */
  guint          num_sp_streams;  /* subpicture streams */

  guint          main_stream; /* used for seeking */

  GstFlowCombiner *flowcombiner;

  gboolean       have_group_id;
  guint          group_id;

  /* for streaming mode */
  gboolean       streaming;
  gboolean       have_eos;
  GstAdapter    *adapter;
  guint          todrop;

  /* some stream info for length */
  gst_riff_avih *avih;
  GstClockTime   duration;

  /* segment in TIME */
  GstSegment     segment;
  guint32        segment_seqnum;

  /* pending tags/events */
  GstEvent      *seg_event;
  GstTagList	*globaltags;
  gboolean	 got_tags;

#if 0
  /* gst index support */
  GstIndex      *element_index;
  gint           index_id;
#endif

  gboolean       seekable;

  guint64        first_movi_offset;
  guint64        idx1_offset; /* offset in file of list/chunk after movi */
  GstEvent      *seek_event;

  gboolean       building_index;
  guint          odml_stream;
  guint          odml_subidx;
  guint64       *odml_subidxs;

  guint64        seek_kf_offset; /* offset of the keyframe to which we want to seek */
} GstAviDemux;

typedef struct _GstAviDemuxClass {
  GstElementClass parent_class;
} GstAviDemuxClass;

GType           gst_avi_demux_get_type          (void);

G_END_DECLS

#endif /* __GST_AVI_DEMUX_H__ */
