/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_QTDEMUX_H__
#define __GST_QTDEMUX_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/base/gstflowcombiner.h>
#include "gstisoff.h"

G_BEGIN_DECLS

#define GST_TYPE_QTDEMUX \
  (gst_qtdemux_get_type())
#define GST_QTDEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_QTDEMUX,GstQTDemux))
#define GST_QTDEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_QTDEMUX,GstQTDemuxClass))
#define GST_IS_QTDEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_QTDEMUX))
#define GST_IS_QTDEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_QTDEMUX))

#define GST_QTDEMUX_CAST(obj) ((GstQTDemux *)(obj))

/* qtdemux produces these for atoms it cannot parse */
#define GST_QT_DEMUX_PRIVATE_TAG "private-qt-tag"
#define GST_QT_DEMUX_CLASSIFICATION_TAG "classification"

typedef struct _GstQTDemux GstQTDemux;
typedef struct _GstQTDemuxClass GstQTDemuxClass;
typedef struct _QtDemuxStream QtDemuxStream;

enum QtDemuxState
{
  QTDEMUX_STATE_INITIAL,        /* Initial state (haven't got the header yet) */
  QTDEMUX_STATE_HEADER,         /* Parsing the header */
  QTDEMUX_STATE_MOVIE,          /* Parsing/Playing the media data */
  QTDEMUX_STATE_BUFFER_MDAT     /* Buffering the mdat atom */
};

struct _GstQTDemux {
  GstElement element;

  /* Global state */
  enum QtDemuxState state;

  /* static sink pad */
  GstPad *sinkpad;

  /* TRUE if pull-based */
  gboolean pullbased;

  gchar *redirect_location;

  /* Protect pad exposing from flush event */
  GMutex expose_lock;

  /* list of QtDemuxStream */
  GPtrArray *active_streams;
  GPtrArray *old_streams;

  gint     n_video_streams;
  gint     n_audio_streams;
  gint     n_sub_streams;

  GstFlowCombiner *flowcombiner;

  /* Incoming stream group-id to set on downstream STREAM_START events.
   * If upstream doesn't contain one, a global one will be generated */
  gboolean have_group_id;
  guint group_id;

  guint  major_brand;
  GstBuffer *comp_brands;

  /* [moov] header.
   * FIXME : This is discarded just after it's created. Just move it
   * to a temporary variable ? */
  GNode *moov_node;

  /* FIXME : This is never freed. It is only assigned once. memleak ? */
  GNode *moov_node_compressed;

  /* Set to TRUE when the [moov] header has been fully parsed */
  gboolean got_moov;

  /* Global timescale for the incoming stream. Use the QTTIME macros
   * to convert values to/from GstClockTime */
  guint32 timescale;

  /* Global duration (in global timescale). Use QTTIME macros to get GstClockTime */
  guint64 duration;

  /* Total size of header atoms. Used to calculate fallback overall bitrate */
  guint header_size;

  GstTagList *tag_list;

  /* configured playback region */
  GstSegment segment;

  /* State for key_units trickmode */
  GstClockTime trickmode_interval;

  /* PUSH-BASED only: If the initial segment event, or a segment consequence of
   * a seek or incoming TIME segment from upstream needs to be pushed. This
   * variable is used instead of pushing the event directly because at that
   * point we may not have yet emitted the srcpads. */
  gboolean need_segment;

  guint32 segment_seqnum;

  /* flag to indicate that we're working with a smoothstreaming fragment
   * Mss doesn't have 'moov' or any information about the streams format,
   * requiring qtdemux to expose and create the streams */
  gboolean mss_mode;

  /* Set to TRUE if the incoming stream is either a MSS stream or
   * a Fragmented MP4 (containing the [mvex] atom in the header) */
  gboolean fragmented;

  /* PULL-BASED only : If TRUE there is a pending seek */
  gboolean fragmented_seek_pending;

  /* PULL-BASED : offset of first [moof] or of fragment to seek to
   * PUSH-BASED : offset of latest [moof] */
  guint64 moof_offset;

  /* MSS streams have a single media that is unspecified at the atoms, so
   * upstream provides it at the caps */
  GstCaps *media_caps;

  /* Set to TRUE when all streams have been exposed */
  gboolean exposed;

  gint64 chapters_track_id;

  /* protection support */
  GPtrArray *protection_system_ids; /* Holds identifiers of all content protection systems for all tracks */
  GQueue protection_event_queue; /* holds copy of upstream protection events */
  guint64 cenc_aux_info_offset;
  guint8 *cenc_aux_info_sizes;
  guint32 cenc_aux_sample_count;
  gchar *preferred_protection_system_id;

  /* Whether the parent bin is streams-aware, meaning we can
   * add/remove streams at any point in time */
  gboolean streams_aware;

  /*
   * ALL VARIABLES BELOW ARE ONLY USED IN PUSH-BASED MODE
   */
  GstAdapter *adapter;
  guint neededbytes;
  guint todrop;
  /* Used to store data if [mdat] is before the headers */
  GstBuffer *mdatbuffer;
  /* Amount of bytes left to read in the current [mdat] */
  guint64 mdatleft, mdatsize;

  /* When restoring the mdat to the adapter, this buffer stores any
   * trailing data that was after the last atom parsed as it has to be
   * restored later along with the correct offset. Used in fragmented
   * scenario where mdat/moof are one after the other in any order.
   *
   * Check https://bugzilla.gnome.org/show_bug.cgi?id=710623 */
  GstBuffer *restoredata_buffer;
  guint64 restoredata_offset;

  /* The current offset in bytes from upstream.
   * Note: While it makes complete sense when we are PULL-BASED (pulling
   * in BYTES from upstream) and PUSH-BASED with a BYTE SEGMENT (receiving
   * buffers with actual offsets), it is undefined in PUSH-BASED with a
   * TIME SEGMENT */
  guint64 offset;

  /* offset of the mdat atom */
  guint64 mdatoffset;
  /* Offset of the first mdat */
  guint64 first_mdat;
  /* offset of last [moov] seen */
  guint64 last_moov_offset;

  /* If TRUE, qtdemux received upstream newsegment in TIME format
   * which likely means that upstream is driving the pipeline (such as
   * adaptive demuxers or dlna sources) */
  gboolean upstream_format_is_time;

  /* Seqnum of the seek event sent upstream.  Will be used to
   * detect incoming FLUSH events corresponding to that */
  guint32 offset_seek_seqnum;

  /* UPSTREAM BYTE: Requested upstream byte seek offset.
   * Currently it is only used to check if an incoming BYTE SEGMENT
   * corresponds to a seek event that was sent upstream */
  gint64 seek_offset;

  /* UPSTREAM BYTE: Requested start/stop TIME values from
   * downstream.
   * Used to set on the downstream segment once the corresponding upstream
   * BYTE SEEK has succeeded */
  gint64 push_seek_start;
  gint64 push_seek_stop;

#if 0
  /* gst index support */
  GstIndex *element_index;
  gint index_id;
#endif

  /* Whether upstream is seekable in BYTES */
  gboolean upstream_seekable;
  /* UPSTREAM BYTE: Size of upstream content.
   * Note : This is only computed once ! If upstream grows in the meantime
   * it will not be updated */
  gint64 upstream_size;

  /* UPSTREAM TIME : Contains the PTS (if any) of the
   * buffer that contains a [moof] header. Will be used to establish
   * the actual PTS of the samples contained within that fragment. */
  guint64 fragment_start;
  /* UPSTREAM TIME : The offset in bytes of the [moof]
   * header start.
   * Note : This is not computed from the GST_BUFFER_OFFSET field */
  guint64 fragment_start_offset;

  /* These two fields are used to perform an implicit seek when a fragmented
   * file whose first tfdt is not zero. This way if the first fragment starts
   * at 1 hour, the user does not have to wait 1 hour or perform a manual seek
   * for the image to move and the sound to play.
   *
   * This implicit seek is only done if the first parsed fragment has a non-zero
   * decode base time and a seek has not been received previously, hence these
   * fields. */
  gboolean received_seek;
  gboolean first_moof_already_parsed;
};

struct _GstQTDemuxClass {
  GstElementClass parent_class;
};

GType gst_qtdemux_get_type (void);

G_END_DECLS

#endif /* __GST_QTDEMUX_H__ */
