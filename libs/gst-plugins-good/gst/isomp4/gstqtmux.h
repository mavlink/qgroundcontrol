/* Quicktime muxer plugin for GStreamer
 * Copyright (C) 2008-2010 Thiago Santos <thiagoss@embedded.ufcg.edu.br>
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
/*
 * Unless otherwise indicated, Source Code is licensed under MIT license.
 * See further explanation attached in License Statement (distributed in the file
 * LICENSE).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __GST_QT_MUX_H__
#define __GST_QT_MUX_H__

#include <gst/gst.h>
#include <gst/base/gstcollectpads.h>

#include "fourcc.h"
#include "atoms.h"
#include "atomsrecovery.h"
#include "gstqtmuxmap.h"

G_BEGIN_DECLS

#define GST_TYPE_QT_MUX (gst_qt_mux_get_type())
#define GST_QT_MUX(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_QT_MUX, GstQTMux))
#define GST_QT_MUX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_QT_MUX, GstQTMux))
#define GST_IS_QT_MUX(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_QT_MUX))
#define GST_IS_QT_MUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_QT_MUX))
#define GST_QT_MUX_CAST(obj) ((GstQTMux*)(obj))


typedef struct _GstQTMux GstQTMux;
typedef struct _GstQTMuxClass GstQTMuxClass;
typedef struct _GstQTPad GstQTPad;

/*
 * GstQTPadPrepareBufferFunc
 *
 * Receives a buffer (takes ref) and returns a new buffer that should
 * replace the passed one.
 *
 * Useful for when the pad/datatype needs some manipulation before
 * being muxed. (Originally added for image/x-jpc support, for which buffers
 * need to be wrapped into a isom box)
 */
typedef GstBuffer * (*GstQTPadPrepareBufferFunc) (GstQTPad * pad,
    GstBuffer * buf, GstQTMux * qtmux);

typedef gboolean (*GstQTPadSetCapsFunc) (GstQTPad * pad, GstCaps * caps);
typedef GstBuffer * (*GstQTPadCreateEmptyBufferFunc) (GstQTPad * pad, gint64 duration);

#define QTMUX_NO_OF_TS   10

struct _GstQTPad
{
  GstCollectData collect;       /* we extend the CollectData */

  /* fourcc id of stream */
  guint32 fourcc;
  /* whether using format that have out of order buffers */
  gboolean is_out_of_order;
  /* if not 0, track with constant sized samples, e.g. raw audio */
  guint sample_size;
  /* make sync table entry */
  gboolean sync;
  /* if it is a sparse stream
   * (meaning we can't use PTS differences to compute duration) */
  gboolean sparse;
  /* bitrates */
  guint32 avg_bitrate, max_bitrate;
  /* expected sample duration */
  guint expected_sample_duration_n;
  guint expected_sample_duration_d;

  /* for avg bitrate calculation */
  guint64 total_bytes;
  guint64 total_duration;

  GstBuffer *last_buf;
  /* dts of last_buf */
  GstClockTime last_dts;
  guint64 sample_offset;

  /* This is compensate for CTTS */
  GstClockTime dts_adjustment;

  /* store the first timestamp for comparing with other streams and
   * know if there are late streams */
  /* subjected to dts adjustment */
  GstClockTime first_ts;
  GstClockTime first_dts;

  /* all the atom and chunk book-keeping is delegated here
   * unowned/uncounted reference, parent MOOV owns */
  AtomTRAK *trak;
  AtomTRAK *tc_trak;
  SampleTableEntry *trak_ste;
  /* fragmented support */
  /* meta data book-keeping delegated here */
  AtomTRAF *traf;
  /* fragment buffers */
  ATOM_ARRAY (GstBuffer *) fragment_buffers;
  /* running fragment duration */
  gint64 fragment_duration;
  /* optional fragment index book-keeping */
  AtomTFRA *tfra;

  /* Set when tags are received, cleared when written to moov */
  gboolean tags_changed;

  GstTagList *tags;

  /* if nothing is set, it won't be called */
  GstQTPadPrepareBufferFunc prepare_buf_func;
  GstQTPadSetCapsFunc set_caps;
  GstQTPadCreateEmptyBufferFunc create_empty_buffer;

  /* SMPTE timecode */
  GstVideoTimeCode *first_tc;
  GstClockTime first_pts;
  guint64 tc_pos;

  /* for keeping track in pre-fill mode */
  GArray *samples;
  guint first_cc_sample_size;
  /* current sample */
  GstAdapter *raw_audio_adapter;
  guint64 raw_audio_adapter_offset;
  GstClockTime raw_audio_adapter_pts;
  GstFlowReturn flow_status;
};

typedef enum _GstQTMuxState
{
  GST_QT_MUX_STATE_NONE,
  GST_QT_MUX_STATE_STARTED,
  GST_QT_MUX_STATE_DATA,
  GST_QT_MUX_STATE_EOS
} GstQTMuxState;

typedef enum _GstQtMuxMode {
    GST_QT_MUX_MODE_MOOV_AT_END,
    GST_QT_MUX_MODE_FRAGMENTED,
    GST_QT_MUX_MODE_FRAGMENTED_STREAMABLE,
    GST_QT_MUX_MODE_FAST_START,
    GST_QT_MUX_MODE_ROBUST_RECORDING,
    GST_QT_MUX_MODE_ROBUST_RECORDING_PREFILL,
} GstQtMuxMode;

struct _GstQTMux
{
  GstElement element;

  GstPad *srcpad;
  GstCollectPads *collect;
  GSList *sinkpads;

  /* state */
  GstQTMuxState state;

  /* Mux mode, inferred from property
   * set in gst_qt_mux_start_file() */
  GstQtMuxMode mux_mode;

  /* size of header (prefix, atoms (ftyp, possibly moov, mdat header)) */
  guint64 header_size;
  /* accumulated size of raw media data (not including mdat header) */
  guint64 mdat_size;
  /* position of the moov (for fragmented mode) or reserved moov atom
   * area (for robust-muxing mode) */
  guint64 moov_pos;
  /* position of mdat atom header (for later updating of size) in
   * moov-at-end, fragmented and robust-muxing modes */
  guint64 mdat_pos;

  /* keep track of the largest chunk to fine-tune brands */
  GstClockTime longest_chunk;

  /* Earliest timestamp across all pads/traks
   * (unadjusted incoming PTS) */
  GstClockTime first_ts;
  /* Last DTS across all pads (= duration) */
  GstClockTime last_dts;

  /* Last pad we used for writing the current chunk */
  GstQTPad *current_pad;
  guint64 current_chunk_size;
  GstClockTime current_chunk_duration;
  guint64 current_chunk_offset;

  /* atom helper objects */
  AtomsContext *context;
  AtomFTYP *ftyp;
  AtomMOOV *moov;
  GSList *extra_atoms; /* list of extra top-level atoms (e.g. UUID for xmp)
                        * Stored as AtomInfo structs */

  /* Set when tags are received, cleared when written to moov */
  gboolean tags_changed;

  /* fragmented file index */
  AtomMFRA *mfra;

  /* fast start */
  FILE *fast_start_file;

  /* moov recovery */
  FILE *moov_recov_file;

  /* fragment sequence */
  guint32 fragment_sequence;

  /* properties */
  guint32 timescale;
  guint32 trak_timescale;
  AtomsTreeFlavor flavor;
  gboolean fast_start;
  gboolean guess_pts;
#ifndef GST_REMOVE_DEPRECATED
  gint dts_method;
#endif
  gchar *fast_start_file_path;
  gchar *moov_recov_file_path;
  guint32 fragment_duration;
  /* Whether or not to work in 'streamable' mode and not
   * seek to rewrite headers - only valid for fragmented
   * mode. */
  gboolean streamable;

  /* Requested target maximum duration */
  GstClockTime reserved_max_duration;
  /* Estimate of remaining reserved header space (in ns of recording) */
  GstClockTime reserved_duration_remaining;
  /* Multiplier for conversion from reserved_max_duration to bytes */
  guint reserved_bytes_per_sec_per_trak;

  guint64 interleave_bytes;
  GstClockTime interleave_time;
  gboolean interleave_bytes_set, interleave_time_set;

  GstClockTime max_raw_audio_drift;

  /* Reserved minimum MOOV size in bytes
   * This is converted from reserved_max_duration
   * using the bytes/trak/sec estimate */
  guint32 reserved_moov_size;
  /* Basic size of the moov (static headers + tags) */
  guint32 base_moov_size;
  /* Size of the most recently generated moov header */
  guint32 last_moov_size;
  /* True if the first moov in the ping-pong buffers
   * is the active one. See gst_qt_mux_robust_recording_rewrite_moov() */
  gboolean reserved_moov_first_active;

  /* Tracking of periodic MOOV updates */
  GstClockTime last_moov_update;
  GstClockTime reserved_moov_update_period;
  GstClockTime muxed_since_last_update;

  gboolean reserved_prefill;

  GstClockTime start_gap_threshold;

  /* for request pad naming */
  guint video_pads, audio_pads, subtitle_pads, caption_pads;
};

struct _GstQTMuxClass
{
  GstElementClass parent_class;

  GstQTMuxFormat format;
};

/* type register helper struct */
typedef struct _GstQTMuxClassParams
{
  GstQTMuxFormatProp *prop;
  GstCaps *src_caps;
  GstCaps *video_sink_caps;
  GstCaps *audio_sink_caps;
  GstCaps *subtitle_sink_caps;
  GstCaps *caption_sink_caps;
} GstQTMuxClassParams;

#define GST_QT_MUX_PARAMS_QDATA g_quark_from_static_string("qt-mux-params")

GType gst_qt_mux_get_type (void);
gboolean gst_qt_mux_register (GstPlugin * plugin);

/* FIXME: ideally classification tag should be added and
 * registered in gstreamer core gsttaglist
 *
 * this tag is a string in the format: entityfourcc://table_num/content
 * FIXME Shouldn't we add a field for 'language'?
 */
#define GST_TAG_3GP_CLASSIFICATION "classification"

G_END_DECLS

#endif /* __GST_QT_MUX_H__ */
