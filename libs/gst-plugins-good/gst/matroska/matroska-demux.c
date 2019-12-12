/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2006 Tim-Philipp Müller <tim centricular net>
 * (c) 2008 Sebastian Dröge <slomo@circular-chaos.org>
 * (c) 2011 Debarshi Ray <rishi@gnu.org>
 *
 * matroska-demux.c: matroska file/stream demuxer
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

/* TODO: check CRC32 if present
 * TODO: there can be a segment after the first segment. Handle like
 *       chained oggs. Fixes #334082
 * TODO: Test samples: http://www.matroska.org/samples/matrix/index.html
 *                     http://samples.mplayerhq.hu/Matroska/
 * TODO: check if demuxing is done correct for all codecs according to spec
 * TODO: seeking with incomplete or without CUE
 */

/**
 * SECTION:element-matroskademux
 * @title: matroskademux
 *
 * matroskademux demuxes a Matroska file into the different contained streams.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v filesrc location=/path/to/mkv ! matroskademux ! vorbisdec ! audioconvert ! audioresample ! autoaudiosink
 * ]| This pipeline demuxes a Matroska file and outputs the contained Vorbis audio.
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>
#include <glib/gprintf.h>

#include <gst/base/base.h>

/* For AVI compatibility mode
   and for fourcc stuff */
#include <gst/riff/riff-read.h>
#include <gst/riff/riff-ids.h>
#include <gst/riff/riff-media.h>

#include <gst/audio/audio.h>
#include <gst/tag/tag.h>
#include <gst/pbutils/pbutils.h>
#include <gst/video/video.h>

#include "matroska-demux.h"
#include "matroska-ids.h"

GST_DEBUG_CATEGORY_STATIC (matroskademux_debug);
#define GST_CAT_DEFAULT matroskademux_debug

#define DEBUG_ELEMENT_START(demux, ebml, element) \
    GST_DEBUG_OBJECT (demux, "Parsing " element " element at offset %" \
        G_GUINT64_FORMAT, gst_ebml_read_get_pos (ebml))

#define DEBUG_ELEMENT_STOP(demux, ebml, element, ret) \
    GST_DEBUG_OBJECT (demux, "Parsing " element " element " \
        " finished with '%s'", gst_flow_get_name (ret))

enum
{
  PROP_0,
  PROP_METADATA,
  PROP_STREAMINFO,
  PROP_MAX_GAP_TIME,
  PROP_MAX_BACKTRACK_DISTANCE
};

#define DEFAULT_MAX_GAP_TIME           (2 * GST_SECOND)
#define DEFAULT_MAX_BACKTRACK_DISTANCE 30
#define INVALID_DATA_THRESHOLD         (2 * 1024 * 1024)

static GstStaticPadTemplate sink_templ = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-matroska; video/x-matroska; "
        "video/x-matroska-3d; audio/webm; video/webm")
    );

/* TODO: fill in caps! */

static GstStaticPadTemplate audio_src_templ =
GST_STATIC_PAD_TEMPLATE ("audio_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate video_src_templ =
GST_STATIC_PAD_TEMPLATE ("video_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate subtitle_src_templ =
    GST_STATIC_PAD_TEMPLATE ("subtitle_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("text/x-raw, format=pango-markup; application/x-ssa; "
        "application/x-ass;application/x-usf; subpicture/x-dvd; "
        "subpicture/x-pgs; subtitle/x-kate; " "application/x-subtitle-unknown")
    );

static GstFlowReturn gst_matroska_demux_parse_id (GstMatroskaDemux * demux,
    guint32 id, guint64 length, guint needed);

/* element functions */
static void gst_matroska_demux_loop (GstPad * pad);

static gboolean gst_matroska_demux_element_send_event (GstElement * element,
    GstEvent * event);
static gboolean gst_matroska_demux_element_query (GstElement * element,
    GstQuery * query);

/* pad functions */
static gboolean gst_matroska_demux_sink_activate (GstPad * sinkpad,
    GstObject * parent);
static gboolean gst_matroska_demux_sink_activate_mode (GstPad * sinkpad,
    GstObject * parent, GstPadMode mode, gboolean active);

static gboolean gst_matroska_demux_handle_seek_event (GstMatroskaDemux * demux,
    GstPad * pad, GstEvent * event);
static gboolean gst_matroska_demux_handle_src_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_matroska_demux_handle_src_query (GstPad * pad,
    GstObject * parent, GstQuery * query);

static gboolean gst_matroska_demux_handle_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_matroska_demux_handle_sink_query (GstPad * pad,
    GstObject * parent, GstQuery * query);
static GstFlowReturn gst_matroska_demux_chain (GstPad * pad,
    GstObject * object, GstBuffer * buffer);

static GstStateChangeReturn
gst_matroska_demux_change_state (GstElement * element,
    GstStateChange transition);
#if 0
static void
gst_matroska_demux_set_index (GstElement * element, GstIndex * index);
static GstIndex *gst_matroska_demux_get_index (GstElement * element);
#endif

/* caps functions */
static GstCaps *gst_matroska_demux_video_caps (GstMatroskaTrackVideoContext
    * videocontext, const gchar * codec_id, guint8 * data, guint size,
    gchar ** codec_name, guint32 * riff_fourcc);
static GstCaps *gst_matroska_demux_audio_caps (GstMatroskaTrackAudioContext
    * audiocontext, const gchar * codec_id, guint8 * data, guint size,
    gchar ** codec_name, guint16 * riff_audio_fmt, GstClockTime * lead_in_ts);
static GstCaps
    * gst_matroska_demux_subtitle_caps (GstMatroskaTrackSubtitleContext *
    subtitlecontext, const gchar * codec_id, gpointer data, guint size);
static const gchar *gst_matroska_track_encryption_algorithm_name (gint val);
static const gchar *gst_matroska_track_encryption_cipher_mode_name (gint val);
static const gchar *gst_matroska_track_encoding_scope_name (gint val);

/* stream methods */
static void gst_matroska_demux_reset (GstElement * element);
static gboolean perform_seek_to_offset (GstMatroskaDemux * demux,
    gdouble rate, guint64 offset, guint32 seqnum, GstSeekFlags flags);

/* gobject functions */
static void gst_matroska_demux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_matroska_demux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

GType gst_matroska_demux_get_type (void);
#define parent_class gst_matroska_demux_parent_class
G_DEFINE_TYPE (GstMatroskaDemux, gst_matroska_demux, GST_TYPE_ELEMENT);

static void
gst_matroska_demux_finalize (GObject * object)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (object);

  gst_matroska_read_common_finalize (&demux->common);
  gst_flow_combiner_free (demux->flowcombiner);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_matroska_demux_class_init (GstMatroskaDemuxClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;

  GST_DEBUG_CATEGORY_INIT (matroskademux_debug, "matroskademux", 0,
      "Matroska demuxer");

  gobject_class->finalize = gst_matroska_demux_finalize;

  gobject_class->get_property = gst_matroska_demux_get_property;
  gobject_class->set_property = gst_matroska_demux_set_property;

  g_object_class_install_property (gobject_class, PROP_MAX_GAP_TIME,
      g_param_spec_uint64 ("max-gap-time", "Maximum gap time",
          "The demuxer sends out segment events for skipping "
          "gaps longer than this (0 = disabled).", 0, G_MAXUINT64,
          DEFAULT_MAX_GAP_TIME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_BACKTRACK_DISTANCE,
      g_param_spec_uint ("max-backtrack-distance",
          "Maximum backtrack distance",
          "Maximum backtrack distance in seconds when seeking without "
          "and index in pull mode and search for a keyframe "
          "(0 = disable backtracking).",
          0, G_MAXUINT, DEFAULT_MAX_BACKTRACK_DISTANCE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_change_state);
  gstelement_class->send_event =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_element_send_event);
  gstelement_class->query =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_element_query);
#if 0
  gstelement_class->set_index =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_set_index);
  gstelement_class->get_index =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_get_index);
#endif

  gst_element_class_add_static_pad_template (gstelement_class,
      &video_src_templ);
  gst_element_class_add_static_pad_template (gstelement_class,
      &audio_src_templ);
  gst_element_class_add_static_pad_template (gstelement_class,
      &subtitle_src_templ);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_templ);

  gst_element_class_set_static_metadata (gstelement_class, "Matroska demuxer",
      "Codec/Demuxer",
      "Demuxes Matroska/WebM streams into video/audio/subtitles",
      "GStreamer maintainers <gstreamer-devel@lists.freedesktop.org>");
}

static void
gst_matroska_demux_init (GstMatroskaDemux * demux)
{
  demux->common.sinkpad = gst_pad_new_from_static_template (&sink_templ,
      "sink");
  gst_pad_set_activate_function (demux->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_sink_activate));
  gst_pad_set_activatemode_function (demux->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_sink_activate_mode));
  gst_pad_set_chain_function (demux->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_chain));
  gst_pad_set_event_function (demux->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_handle_sink_event));
  gst_pad_set_query_function (demux->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_handle_sink_query));
  gst_element_add_pad (GST_ELEMENT (demux), demux->common.sinkpad);

  /* init defaults for common read context */
  gst_matroska_read_common_init (&demux->common);

  /* property defaults */
  demux->max_gap_time = DEFAULT_MAX_GAP_TIME;
  demux->max_backtrack_distance = DEFAULT_MAX_BACKTRACK_DISTANCE;

  GST_OBJECT_FLAG_SET (demux, GST_ELEMENT_FLAG_INDEXABLE);

  demux->flowcombiner = gst_flow_combiner_new ();

  /* finish off */
  gst_matroska_demux_reset (GST_ELEMENT (demux));
}

static void
gst_matroska_demux_reset (GstElement * element)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);

  GST_DEBUG_OBJECT (demux, "Resetting state");

  gst_matroska_read_common_reset (GST_ELEMENT (demux), &demux->common);

  demux->num_a_streams = 0;
  demux->num_t_streams = 0;
  demux->num_v_streams = 0;
  demux->have_nonintraonly_v_streams = FALSE;

  demux->have_group_id = FALSE;
  demux->group_id = G_MAXUINT;

  demux->clock = NULL;
  demux->tracks_parsed = FALSE;

  if (demux->clusters) {
    g_array_free (demux->clusters, TRUE);
    demux->clusters = NULL;
  }

  g_list_foreach (demux->seek_parsed,
      (GFunc) gst_matroska_read_common_free_parsed_el, NULL);
  g_list_free (demux->seek_parsed);
  demux->seek_parsed = NULL;

  demux->last_stop_end = GST_CLOCK_TIME_NONE;
  demux->seek_block = 0;
  demux->stream_start_time = GST_CLOCK_TIME_NONE;
  demux->to_time = GST_CLOCK_TIME_NONE;
  demux->cluster_time = GST_CLOCK_TIME_NONE;
  demux->cluster_offset = 0;
  demux->cluster_prevsize = 0;
  demux->seen_cluster_prevsize = FALSE;
  demux->next_cluster_offset = 0;
  demux->stream_last_time = GST_CLOCK_TIME_NONE;
  demux->last_cluster_offset = 0;
  demux->index_offset = 0;
  demux->seekable = FALSE;
  demux->need_segment = FALSE;
  demux->segment_seqnum = 0;
  demux->requested_seek_time = GST_CLOCK_TIME_NONE;
  demux->seek_offset = -1;
  demux->audio_lead_in_ts = 0;
  demux->building_index = FALSE;
  if (demux->seek_event) {
    gst_event_unref (demux->seek_event);
    demux->seek_event = NULL;
  }

  demux->seek_index = NULL;
  demux->seek_entry = 0;

  if (demux->new_segment) {
    gst_event_unref (demux->new_segment);
    demux->new_segment = NULL;
  }

  demux->invalid_duration = FALSE;

  demux->cached_length = G_MAXUINT64;

  if (demux->deferred_seek_event)
    gst_event_unref (demux->deferred_seek_event);
  demux->deferred_seek_event = NULL;
  demux->deferred_seek_pad = NULL;

  gst_flow_combiner_clear (demux->flowcombiner);
}

static GstBuffer *
gst_matroska_decode_buffer (GstMatroskaTrackContext * context, GstBuffer * buf)
{
  GstMapInfo map;
  gpointer data;
  gsize size;
  GstBuffer *out_buf = buf;

  g_return_val_if_fail (GST_IS_BUFFER (buf), NULL);

  GST_DEBUG ("decoding buffer %p", buf);

  gst_buffer_map (out_buf, &map, GST_MAP_READ);
  data = map.data;
  size = map.size;

  g_return_val_if_fail (size > 0, buf);

  if (gst_matroska_decode_data (context->encodings, &data, &size,
          GST_MATROSKA_TRACK_ENCODING_SCOPE_FRAME, FALSE)) {
    if (data != map.data) {
      gst_buffer_unmap (out_buf, &map);
      gst_buffer_unref (out_buf);
      out_buf = gst_buffer_new_wrapped (data, size);
    } else {
      gst_buffer_unmap (out_buf, &map);
    }
  } else {
    GST_DEBUG ("decode data failed");
    gst_buffer_unmap (out_buf, &map);
    gst_buffer_unref (out_buf);
    return NULL;
  }
  /* Encrypted stream */
  if (context->protection_info) {

    GstStructure *info_protect = gst_structure_copy (context->protection_info);
    gboolean encrypted = FALSE;

    gst_buffer_map (out_buf, &map, GST_MAP_READ);
    data = map.data;
    size = map.size;

    if (gst_matroska_parse_protection_meta (&data, &size, info_protect,
            &encrypted)) {
      if (data != map.data) {
        GstBuffer *tmp_buf;

        gst_buffer_unmap (out_buf, &map);
        tmp_buf = out_buf;
        out_buf = gst_buffer_copy_region (tmp_buf, GST_BUFFER_COPY_ALL,
            gst_buffer_get_size (tmp_buf) - size, size);
        gst_buffer_unref (tmp_buf);
        if (encrypted)
          gst_buffer_add_protection_meta (out_buf, info_protect);
        else
          gst_structure_free (info_protect);
      } else {
        gst_buffer_unmap (out_buf, &map);
        gst_structure_free (info_protect);
      }
    } else {
      GST_WARNING ("Adding protection metadata failed");
      gst_buffer_unmap (out_buf, &map);
      gst_buffer_unref (out_buf);
      gst_structure_free (info_protect);
      return NULL;
    }
  }

  return out_buf;
}

static void
gst_matroska_demux_add_stream_headers_to_caps (GstMatroskaDemux * demux,
    GstBufferList * list, GstCaps * caps)
{
  GstStructure *s;
  GValue arr_val = G_VALUE_INIT;
  GValue buf_val = G_VALUE_INIT;
  gint i, num;

  g_assert (gst_caps_is_writable (caps));

  g_value_init (&arr_val, GST_TYPE_ARRAY);
  g_value_init (&buf_val, GST_TYPE_BUFFER);

  num = gst_buffer_list_length (list);
  for (i = 0; i < num; ++i) {
    g_value_set_boxed (&buf_val, gst_buffer_list_get (list, i));
    gst_value_array_append_value (&arr_val, &buf_val);
  }

  s = gst_caps_get_structure (caps, 0);
  gst_structure_take_value (s, "streamheader", &arr_val);
  g_value_unset (&buf_val);
}

static GstFlowReturn
gst_matroska_demux_parse_mastering_metadata (GstMatroskaDemux * demux,
    GstEbmlRead * ebml, GstMatroskaTrackVideoContext * video_context)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstVideoMasteringDisplayInfo minfo;
  guint32 id;
  gdouble num;
  /* Precision defined by HEVC specification */
  const guint chroma_den = 50000;
  const guint luma_den = 10000;

  gst_video_mastering_display_info_init (&minfo);

  DEBUG_ELEMENT_START (demux, ebml, "MasteringMetadata");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK)
    goto beach;

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      goto beach;

    /* all sub elements have float type */
    if ((ret = gst_ebml_read_float (ebml, &id, &num)) != GST_FLOW_OK)
      goto beach;

    /* chromaticity should be in [0, 1] range */
    if (id >= GST_MATROSKA_ID_PRIMARYRCHROMATICITYX &&
        id <= GST_MATROSKA_ID_WHITEPOINTCHROMATICITYY) {
      if (num < 0 || num > 1.0) {
        GST_WARNING_OBJECT (demux, "0x%x has invalid value %f", id, num);
        goto beach;
      }
    } else if (id == GST_MATROSKA_ID_LUMINANCEMAX ||
        id == GST_MATROSKA_ID_LUMINANCEMIN) {
      /* Note: webM spec said valid range is [0, 999.9999] but
       * 1000 cd/m^2 is generally used value on HDR. Just check guint range here.
       * See https://www.webmproject.org/docs/container/#LuminanceMax
       */
      if (num < 0 || num > (gdouble) (G_MAXUINT32 / luma_den)) {
        GST_WARNING_OBJECT (demux, "0x%x has invalid value %f", id, num);
        goto beach;
      }
    }

    switch (id) {
      case GST_MATROSKA_ID_PRIMARYRCHROMATICITYX:
        minfo.Rx_n = (guint) round (num * chroma_den);
        minfo.Rx_d = chroma_den;
        break;
      case GST_MATROSKA_ID_PRIMARYRCHROMATICITYY:
        minfo.Ry_n = (guint) round (num * chroma_den);
        minfo.Ry_d = chroma_den;
        break;
      case GST_MATROSKA_ID_PRIMARYGCHROMATICITYX:
        minfo.Gx_n = (guint) round (num * chroma_den);
        minfo.Gx_d = chroma_den;
        break;
      case GST_MATROSKA_ID_PRIMARYGCHROMATICITYY:
        minfo.Gy_n = (guint) round (num * chroma_den);
        minfo.Gy_d = chroma_den;
        break;
      case GST_MATROSKA_ID_PRIMARYBCHROMATICITYX:
        minfo.Bx_n = (guint) round (num * chroma_den);
        minfo.Bx_d = chroma_den;
        break;
      case GST_MATROSKA_ID_PRIMARYBCHROMATICITYY:
        minfo.By_n = (guint) round (num * chroma_den);
        minfo.By_d = chroma_den;
        break;
      case GST_MATROSKA_ID_WHITEPOINTCHROMATICITYX:
        minfo.Wx_n = (guint) round (num * chroma_den);
        minfo.Wx_d = chroma_den;
        break;
      case GST_MATROSKA_ID_WHITEPOINTCHROMATICITYY:
        minfo.Wy_n = (guint) round (num * chroma_den);
        minfo.Wy_d = chroma_den;
        break;
      case GST_MATROSKA_ID_LUMINANCEMAX:
        minfo.max_luma_n = (guint) round (num * luma_den);
        minfo.max_luma_d = luma_den;
        break;
      case GST_MATROSKA_ID_LUMINANCEMIN:
        minfo.min_luma_n = (guint) round (num * luma_den);
        minfo.min_luma_d = luma_den;
        break;
      default:
        GST_FIXME_OBJECT (demux,
            "Unsupported subelement 0x%x in MasteringMetadata", id);
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  video_context->mastering_display_info = minfo;
  video_context->mastering_display_info_present = TRUE;

beach:
  DEBUG_ELEMENT_STOP (demux, ebml, "MasteringMetadata", ret);

  return ret;
}

static GstFlowReturn
gst_matroska_demux_parse_colour (GstMatroskaDemux * demux, GstEbmlRead * ebml,
    GstMatroskaTrackVideoContext * video_context)
{
  GstFlowReturn ret;
  GstVideoColorimetry colorimetry;
  guint32 id;
  guint64 num;

  colorimetry.range = GST_VIDEO_COLOR_RANGE_UNKNOWN;
  colorimetry.matrix = GST_VIDEO_COLOR_MATRIX_UNKNOWN;
  colorimetry.transfer = GST_VIDEO_TRANSFER_UNKNOWN;
  colorimetry.primaries = GST_VIDEO_COLOR_PRIMARIES_UNKNOWN;

  DEBUG_ELEMENT_START (demux, ebml, "TrackVideoColour");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK)
    goto beach;

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      goto beach;

    switch (id) {
      case GST_MATROSKA_ID_VIDEOMATRIXCOEFFICIENTS:{
        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          goto beach;

        colorimetry.matrix = gst_video_color_matrix_from_iso ((guint) num);
        break;
      }

      case GST_MATROSKA_ID_VIDEORANGE:{
        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          goto beach;

        switch (num) {
          case 0:
            colorimetry.range = GST_VIDEO_COLOR_RANGE_UNKNOWN;
            break;
          case 1:
            colorimetry.range = GST_VIDEO_COLOR_RANGE_16_235;
            break;
          case 2:
            colorimetry.range = GST_VIDEO_COLOR_RANGE_0_255;
            break;
          default:
            GST_FIXME_OBJECT (demux, "Unsupported color range  %"
                G_GUINT64_FORMAT, num);
            break;
        }
        break;
      }

      case GST_MATROSKA_ID_VIDEOTRANSFERCHARACTERISTICS:{
        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          goto beach;

        colorimetry.transfer = gst_video_color_transfer_from_iso ((guint) num);
        break;
      }

      case GST_MATROSKA_ID_VIDEOPRIMARIES:{
        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          goto beach;

        colorimetry.primaries =
            gst_video_color_primaries_from_iso ((guint) num);
        break;
      }

      case GST_MATROSKA_ID_MASTERINGMETADATA:{
        if ((ret =
                gst_matroska_demux_parse_mastering_metadata (demux, ebml,
                    video_context)) != GST_FLOW_OK)
          goto beach;
        break;
      }

      case GST_MATROSKA_ID_MAXCLL:{
        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          goto beach;
        if (num >= G_MAXUINT32) {
          GST_WARNING_OBJECT (demux,
              "Too large maxCLL value %" G_GUINT64_FORMAT, num);
        } else {
          video_context->content_light_level.maxCLL_n = num;
          video_context->content_light_level.maxCLL_d = 1;
        }
        break;
      }

      case GST_MATROSKA_ID_MAXFALL:{
        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          goto beach;
        if (num >= G_MAXUINT32) {
          GST_WARNING_OBJECT (demux,
              "Too large maxFALL value %" G_GUINT64_FORMAT, num);
        } else {
          video_context->content_light_level.maxFALL_n = num;
          video_context->content_light_level.maxFALL_d = 1;
        }
        break;
      }

      default:
        GST_FIXME_OBJECT (demux, "Unsupported subelement 0x%x in Colour", id);
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  memcpy (&video_context->colorimetry, &colorimetry,
      sizeof (GstVideoColorimetry));

beach:
  DEBUG_ELEMENT_STOP (demux, ebml, "TrackVideoColour", ret);
  return ret;
}

static GstFlowReturn
gst_matroska_demux_parse_stream (GstMatroskaDemux * demux, GstEbmlRead * ebml,
    GstMatroskaTrackContext ** dest_context)
{
  GstMatroskaTrackContext *context;
  GstCaps *caps = NULL;
  GstTagList *cached_taglist;
  GstFlowReturn ret;
  guint32 id, riff_fourcc = 0;
  guint16 riff_audio_fmt = 0;
  gchar *codec = NULL;

  DEBUG_ELEMENT_START (demux, ebml, "TrackEntry");

  /* start with the master */
  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (demux, ebml, "TrackEntry", ret);
    return ret;
  }

  /* allocate generic... if we know the type, we'll g_renew()
   * with the precise type */
  context = g_new0 (GstMatroskaTrackContext, 1);
  context->index_writer_id = -1;
  context->type = 0;            /* no type yet */
  context->default_duration = 0;
  context->pos = 0;
  context->set_discont = TRUE;
  context->timecodescale = 1.0;
  context->flags =
      GST_MATROSKA_TRACK_ENABLED | GST_MATROSKA_TRACK_DEFAULT |
      GST_MATROSKA_TRACK_LACING;
  context->from_time = GST_CLOCK_TIME_NONE;
  context->from_offset = -1;
  context->to_offset = G_MAXINT64;
  context->alignment = 1;
  context->dts_only = FALSE;
  context->intra_only = FALSE;
  context->tags = gst_tag_list_new_empty ();
  g_queue_init (&context->protection_event_queue);
  context->protection_info = NULL;

  GST_DEBUG_OBJECT (demux, "Parsing a TrackEntry (%d tracks parsed so far)",
      demux->common.num_streams);

  /* try reading the trackentry headers */
  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* track number (unique stream ID) */
      case GST_MATROSKA_ID_TRACKNUMBER:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num == 0) {
          GST_ERROR_OBJECT (demux, "Invalid TrackNumber 0");
          ret = GST_FLOW_ERROR;
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackNumber: %" G_GUINT64_FORMAT, num);
        context->num = num;
        break;
      }
        /* track UID (unique identifier) */
      case GST_MATROSKA_ID_TRACKUID:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num == 0) {
          GST_ERROR_OBJECT (demux, "Invalid TrackUID 0");
          ret = GST_FLOW_ERROR;
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackUID: %" G_GUINT64_FORMAT, num);
        context->uid = num;
        break;
      }

        /* track type (video, audio, combined, subtitle, etc.) */
      case GST_MATROSKA_ID_TRACKTYPE:{
        guint64 track_type;

        if ((ret = gst_ebml_read_uint (ebml, &id, &track_type)) != GST_FLOW_OK) {
          break;
        }

        if (context->type != 0 && context->type != track_type) {
          GST_WARNING_OBJECT (demux,
              "More than one tracktype defined in a TrackEntry - skipping");
          break;
        } else if (track_type < 1 || track_type > 254) {
          GST_WARNING_OBJECT (demux, "Invalid TrackType %" G_GUINT64_FORMAT,
              track_type);
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackType: %" G_GUINT64_FORMAT, track_type);

        /* ok, so we're actually going to reallocate this thing */
        switch (track_type) {
          case GST_MATROSKA_TRACK_TYPE_VIDEO:
            gst_matroska_track_init_video_context (&context);
            break;
          case GST_MATROSKA_TRACK_TYPE_AUDIO:
            gst_matroska_track_init_audio_context (&context);
            break;
          case GST_MATROSKA_TRACK_TYPE_SUBTITLE:
            gst_matroska_track_init_subtitle_context (&context);
            break;
          case GST_MATROSKA_TRACK_TYPE_COMPLEX:
          case GST_MATROSKA_TRACK_TYPE_LOGO:
          case GST_MATROSKA_TRACK_TYPE_BUTTONS:
          case GST_MATROSKA_TRACK_TYPE_CONTROL:
          default:
            GST_WARNING_OBJECT (demux,
                "Unknown or unsupported TrackType %" G_GUINT64_FORMAT,
                track_type);
            context->type = 0;
            break;
        }
        break;
      }

        /* tracktype specific stuff for video */
      case GST_MATROSKA_ID_TRACKVIDEO:{
        GstMatroskaTrackVideoContext *videocontext;

        DEBUG_ELEMENT_START (demux, ebml, "TrackVideo");

        if (!gst_matroska_track_init_video_context (&context)) {
          GST_WARNING_OBJECT (demux,
              "TrackVideo element in non-video track - ignoring track");
          ret = GST_FLOW_ERROR;
          break;
        } else if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
          break;
        }
        videocontext = (GstMatroskaTrackVideoContext *) context;

        while (ret == GST_FLOW_OK &&
            gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
          if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
            break;

          switch (id) {
              /* Should be one level up but some broken muxers write it here. */
            case GST_MATROSKA_ID_TRACKDEFAULTDURATION:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackDefaultDuration 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackDefaultDuration: %" G_GUINT64_FORMAT, num);
              context->default_duration = num;
              break;
            }

              /* video framerate */
              /* NOTE: This one is here only for backward compatibility.
               * Use _TRACKDEFAULDURATION one level up. */
            case GST_MATROSKA_ID_VIDEOFRAMERATE:{
              gdouble num;

              if ((ret = gst_ebml_read_float (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num <= 0.0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoFPS %lf", num);
                break;
              }

              GST_DEBUG_OBJECT (demux, "TrackVideoFrameRate: %lf", num);
              if (context->default_duration == 0)
                context->default_duration =
                    gst_gdouble_to_guint64 ((gdouble) GST_SECOND * (1.0 / num));
              videocontext->default_fps = num;
              break;
            }

              /* width of the size to display the video at */
            case GST_MATROSKA_ID_VIDEODISPLAYWIDTH:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoDisplayWidth 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackVideoDisplayWidth: %" G_GUINT64_FORMAT, num);
              videocontext->display_width = num;
              break;
            }

              /* height of the size to display the video at */
            case GST_MATROSKA_ID_VIDEODISPLAYHEIGHT:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoDisplayHeight 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackVideoDisplayHeight: %" G_GUINT64_FORMAT, num);
              videocontext->display_height = num;
              break;
            }

              /* width of the video in the file */
            case GST_MATROSKA_ID_VIDEOPIXELWIDTH:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoPixelWidth 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackVideoPixelWidth: %" G_GUINT64_FORMAT, num);
              videocontext->pixel_width = num;
              break;
            }

              /* height of the video in the file */
            case GST_MATROSKA_ID_VIDEOPIXELHEIGHT:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoPixelHeight 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackVideoPixelHeight: %" G_GUINT64_FORMAT, num);
              videocontext->pixel_height = num;
              break;
            }

              /* whether the video is interlaced */
            case GST_MATROSKA_ID_VIDEOFLAGINTERLACED:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 1)
                videocontext->interlace_mode =
                    GST_MATROSKA_INTERLACE_MODE_INTERLACED;
              else if (num == 2)
                videocontext->interlace_mode =
                    GST_MATROSKA_INTERLACE_MODE_PROGRESSIVE;
              else
                videocontext->interlace_mode =
                    GST_MATROSKA_INTERLACE_MODE_UNKNOWN;

              GST_DEBUG_OBJECT (demux, "video track interlacing mode: %d",
                  videocontext->interlace_mode);
              break;
            }

              /* interlaced field order */
            case GST_MATROSKA_ID_VIDEOFIELDORDER:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (videocontext->interlace_mode !=
                  GST_MATROSKA_INTERLACE_MODE_INTERLACED) {
                GST_WARNING_OBJECT (demux,
                    "FieldOrder element when not interlaced - ignoring");
                break;
              }

              if (num == 0)
                /* turns out we're actually progressive */
                videocontext->interlace_mode =
                    GST_MATROSKA_INTERLACE_MODE_PROGRESSIVE;
              else if (num == 2)
                videocontext->field_order = GST_VIDEO_FIELD_ORDER_UNKNOWN;
              else if (num == 9)
                videocontext->field_order =
                    GST_VIDEO_FIELD_ORDER_TOP_FIELD_FIRST;
              else if (num == 14)
                videocontext->field_order =
                    GST_VIDEO_FIELD_ORDER_BOTTOM_FIELD_FIRST;
              else {
                GST_FIXME_OBJECT (demux,
                    "Unknown or unsupported FieldOrder %" G_GUINT64_FORMAT,
                    num);
                videocontext->field_order = GST_VIDEO_FIELD_ORDER_UNKNOWN;
              }

              GST_DEBUG_OBJECT (demux, "video track field order: %d",
                  videocontext->field_order);
              break;
            }

              /* aspect ratio behaviour */
            case GST_MATROSKA_ID_VIDEOASPECTRATIOTYPE:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num != GST_MATROSKA_ASPECT_RATIO_MODE_FREE &&
                  num != GST_MATROSKA_ASPECT_RATIO_MODE_KEEP &&
                  num != GST_MATROSKA_ASPECT_RATIO_MODE_FIXED) {
                GST_WARNING_OBJECT (demux,
                    "Unknown TrackVideoAspectRatioType 0x%x", (guint) num);
                break;
              }
              GST_DEBUG_OBJECT (demux,
                  "TrackVideoAspectRatioType: %" G_GUINT64_FORMAT, num);
              videocontext->asr_mode = num;
              break;
            }

              /* colourspace (only matters for raw video) fourcc */
            case GST_MATROSKA_ID_VIDEOCOLOURSPACE:{
              guint8 *data;
              guint64 datalen;

              if ((ret =
                      gst_ebml_read_binary (ebml, &id, &data,
                          &datalen)) != GST_FLOW_OK)
                break;

              if (datalen != 4) {
                g_free (data);
                GST_WARNING_OBJECT (demux,
                    "Invalid TrackVideoColourSpace length %" G_GUINT64_FORMAT,
                    datalen);
                break;
              }

              memcpy (&videocontext->fourcc, data, 4);
              GST_DEBUG_OBJECT (demux,
                  "TrackVideoColourSpace: %" GST_FOURCC_FORMAT,
                  GST_FOURCC_ARGS (videocontext->fourcc));
              g_free (data);
              break;
            }

              /* color info */
            case GST_MATROSKA_ID_VIDEOCOLOUR:{
              ret = gst_matroska_demux_parse_colour (demux, ebml, videocontext);
              break;
            }

            case GST_MATROSKA_ID_VIDEOSTEREOMODE:
            {
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              GST_DEBUG_OBJECT (demux, "StereoMode: %" G_GUINT64_FORMAT, num);

              switch (num) {
                case GST_MATROSKA_STEREO_MODE_SBS_RL:
                  videocontext->multiview_flags =
                      GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST;
                  /* fall through */
                case GST_MATROSKA_STEREO_MODE_SBS_LR:
                  videocontext->multiview_mode =
                      GST_VIDEO_MULTIVIEW_MODE_SIDE_BY_SIDE;
                  break;
                case GST_MATROSKA_STEREO_MODE_TB_RL:
                  videocontext->multiview_flags =
                      GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST;
                  /* fall through */
                case GST_MATROSKA_STEREO_MODE_TB_LR:
                  videocontext->multiview_mode =
                      GST_VIDEO_MULTIVIEW_MODE_TOP_BOTTOM;
                  break;
                case GST_MATROSKA_STEREO_MODE_CHECKER_RL:
                  videocontext->multiview_flags =
                      GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST;
                  /* fall through */
                case GST_MATROSKA_STEREO_MODE_CHECKER_LR:
                  videocontext->multiview_mode =
                      GST_VIDEO_MULTIVIEW_MODE_CHECKERBOARD;
                  break;
                case GST_MATROSKA_STEREO_MODE_FBF_RL:
                  videocontext->multiview_flags =
                      GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST;
                  /* fall through */
                case GST_MATROSKA_STEREO_MODE_FBF_LR:
                  videocontext->multiview_mode =
                      GST_VIDEO_MULTIVIEW_MODE_FRAME_BY_FRAME;
                  /* FIXME: In frame-by-frame mode, left/right frame buffers are
                   * laced within one block, and we'll need to apply FIRST_IN_BUNDLE
                   * accordingly. See http://www.matroska.org/technical/specs/index.html#StereoMode */
                  GST_FIXME_OBJECT (demux,
                      "Frame-by-frame stereoscopic mode not fully implemented");
                  break;
              }
              break;
            }

            default:
              GST_WARNING_OBJECT (demux,
                  "Unknown TrackVideo subelement 0x%x - ignoring", id);
              /* fall through */
            case GST_MATROSKA_ID_VIDEODISPLAYUNIT:
            case GST_MATROSKA_ID_VIDEOPIXELCROPBOTTOM:
            case GST_MATROSKA_ID_VIDEOPIXELCROPTOP:
            case GST_MATROSKA_ID_VIDEOPIXELCROPLEFT:
            case GST_MATROSKA_ID_VIDEOPIXELCROPRIGHT:
            case GST_MATROSKA_ID_VIDEOGAMMAVALUE:
              ret = gst_ebml_read_skip (ebml);
              break;
          }
        }

        DEBUG_ELEMENT_STOP (demux, ebml, "TrackVideo", ret);
        break;
      }

        /* tracktype specific stuff for audio */
      case GST_MATROSKA_ID_TRACKAUDIO:{
        GstMatroskaTrackAudioContext *audiocontext;

        DEBUG_ELEMENT_START (demux, ebml, "TrackAudio");

        if (!gst_matroska_track_init_audio_context (&context)) {
          GST_WARNING_OBJECT (demux,
              "TrackAudio element in non-audio track - ignoring track");
          ret = GST_FLOW_ERROR;
          break;
        }

        if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK)
          break;

        audiocontext = (GstMatroskaTrackAudioContext *) context;

        while (ret == GST_FLOW_OK &&
            gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
          if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
            break;

          switch (id) {
              /* samplerate */
            case GST_MATROSKA_ID_AUDIOSAMPLINGFREQ:{
              gdouble num;

              if ((ret = gst_ebml_read_float (ebml, &id, &num)) != GST_FLOW_OK)
                break;


              if (num <= 0.0) {
                GST_WARNING_OBJECT (demux,
                    "Invalid TrackAudioSamplingFrequency %lf", num);
                break;
              }

              GST_DEBUG_OBJECT (demux, "TrackAudioSamplingFrequency: %lf", num);
              audiocontext->samplerate = num;
              break;
            }

              /* bitdepth */
            case GST_MATROSKA_ID_AUDIOBITDEPTH:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackAudioBitDepth 0");
                break;
              }

              GST_DEBUG_OBJECT (demux, "TrackAudioBitDepth: %" G_GUINT64_FORMAT,
                  num);
              audiocontext->bitdepth = num;
              break;
            }

              /* channels */
            case GST_MATROSKA_ID_AUDIOCHANNELS:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackAudioChannels 0");
                break;
              }

              GST_DEBUG_OBJECT (demux, "TrackAudioChannels: %" G_GUINT64_FORMAT,
                  num);
              audiocontext->channels = num;
              break;
            }

            default:
              GST_WARNING_OBJECT (demux,
                  "Unknown TrackAudio subelement 0x%x - ignoring", id);
              /* fall through */
            case GST_MATROSKA_ID_AUDIOCHANNELPOSITIONS:
            case GST_MATROSKA_ID_AUDIOOUTPUTSAMPLINGFREQ:
              ret = gst_ebml_read_skip (ebml);
              break;
          }
        }

        DEBUG_ELEMENT_STOP (demux, ebml, "TrackAudio", ret);

        break;
      }

        /* codec identifier */
      case GST_MATROSKA_ID_CODECID:{
        gchar *text;

        if ((ret = gst_ebml_read_ascii (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (demux, "CodecID: %s", GST_STR_NULL (text));
        context->codec_id = text;
        break;
      }

        /* codec private data */
      case GST_MATROSKA_ID_CODECPRIVATE:{
        guint8 *data;
        guint64 size;

        if ((ret =
                gst_ebml_read_binary (ebml, &id, &data, &size)) != GST_FLOW_OK)
          break;

        context->codec_priv = data;
        context->codec_priv_size = size;

        GST_DEBUG_OBJECT (demux, "CodecPrivate of size %" G_GUINT64_FORMAT,
            size);
        break;
      }

        /* name of the codec */
      case GST_MATROSKA_ID_CODECNAME:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (demux, "CodecName: %s", GST_STR_NULL (text));
        context->codec_name = text;
        break;
      }

        /* codec delay */
      case GST_MATROSKA_ID_CODECDELAY:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        context->codec_delay = num;

        GST_DEBUG_OBJECT (demux, "CodecDelay: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (num));
        break;
      }

        /* codec delay */
      case GST_MATROSKA_ID_SEEKPREROLL:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        context->seek_preroll = num;

        GST_DEBUG_OBJECT (demux, "SeekPreroll: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (num));
        break;
      }

        /* name of this track */
      case GST_MATROSKA_ID_TRACKNAME:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        context->name = text;
        GST_DEBUG_OBJECT (demux, "TrackName: %s", GST_STR_NULL (text));
        break;
      }

        /* language (matters for audio/subtitles, mostly) */
      case GST_MATROSKA_ID_TRACKLANGUAGE:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;


        context->language = text;

        /* fre-ca => fre */
        if (strlen (context->language) >= 4 && context->language[3] == '-')
          context->language[3] = '\0';

        GST_DEBUG_OBJECT (demux, "TrackLanguage: %s",
            GST_STR_NULL (context->language));
        break;
      }

        /* whether this is actually used */
      case GST_MATROSKA_ID_TRACKFLAGENABLED:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num)
          context->flags |= GST_MATROSKA_TRACK_ENABLED;
        else
          context->flags &= ~GST_MATROSKA_TRACK_ENABLED;

        GST_DEBUG_OBJECT (demux, "TrackEnabled: %d",
            (context->flags & GST_MATROSKA_TRACK_ENABLED) ? 1 : 0);
        break;
      }

        /* whether it's the default for this track type */
      case GST_MATROSKA_ID_TRACKFLAGDEFAULT:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num)
          context->flags |= GST_MATROSKA_TRACK_DEFAULT;
        else
          context->flags &= ~GST_MATROSKA_TRACK_DEFAULT;

        GST_DEBUG_OBJECT (demux, "TrackDefault: %d",
            (context->flags & GST_MATROSKA_TRACK_DEFAULT) ? 1 : 0);
        break;
      }

        /* whether the track must be used during playback */
      case GST_MATROSKA_ID_TRACKFLAGFORCED:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num)
          context->flags |= GST_MATROSKA_TRACK_FORCED;
        else
          context->flags &= ~GST_MATROSKA_TRACK_FORCED;

        GST_DEBUG_OBJECT (demux, "TrackForced: %d",
            (context->flags & GST_MATROSKA_TRACK_FORCED) ? 1 : 0);
        break;
      }

        /* lacing (like MPEG, where blocks don't end/start on frame
         * boundaries) */
      case GST_MATROSKA_ID_TRACKFLAGLACING:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num)
          context->flags |= GST_MATROSKA_TRACK_LACING;
        else
          context->flags &= ~GST_MATROSKA_TRACK_LACING;

        GST_DEBUG_OBJECT (demux, "TrackLacing: %d",
            (context->flags & GST_MATROSKA_TRACK_LACING) ? 1 : 0);
        break;
      }

        /* default length (in time) of one data block in this track */
      case GST_MATROSKA_ID_TRACKDEFAULTDURATION:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;


        if (num == 0) {
          GST_WARNING_OBJECT (demux, "Invalid TrackDefaultDuration 0");
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackDefaultDuration: %" G_GUINT64_FORMAT,
            num);
        context->default_duration = num;
        break;
      }

      case GST_MATROSKA_ID_CONTENTENCODINGS:{
        ret = gst_matroska_read_common_read_track_encodings (&demux->common,
            ebml, context);
        break;
      }

      case GST_MATROSKA_ID_TRACKTIMECODESCALE:{
        gdouble num;

        if ((ret = gst_ebml_read_float (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num <= 0.0) {
          GST_WARNING_OBJECT (demux, "Invalid TrackTimeCodeScale %lf", num);
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackTimeCodeScale: %lf", num);
        context->timecodescale = num;
        break;
      }

      default:
        GST_WARNING ("Unknown TrackEntry subelement 0x%x - ignoring", id);
        /* pass-through */

        /* we ignore these because they're nothing useful (i.e. crap)
         * or simply not implemented yet. */
      case GST_MATROSKA_ID_TRACKMINCACHE:
      case GST_MATROSKA_ID_TRACKMAXCACHE:
      case GST_MATROSKA_ID_MAXBLOCKADDITIONID:
      case GST_MATROSKA_ID_TRACKATTACHMENTLINK:
      case GST_MATROSKA_ID_TRACKOVERLAY:
      case GST_MATROSKA_ID_TRACKTRANSLATE:
      case GST_MATROSKA_ID_TRACKOFFSET:
      case GST_MATROSKA_ID_CODECSETTINGS:
      case GST_MATROSKA_ID_CODECINFOURL:
      case GST_MATROSKA_ID_CODECDOWNLOADURL:
      case GST_MATROSKA_ID_CODECDECODEALL:
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (demux, ebml, "TrackEntry", ret);

  /* Decode codec private data if necessary */
  if (context->encodings && context->encodings->len > 0 && context->codec_priv
      && context->codec_priv_size > 0) {
    if (!gst_matroska_decode_data (context->encodings,
            &context->codec_priv, &context->codec_priv_size,
            GST_MATROSKA_TRACK_ENCODING_SCOPE_CODEC_DATA, TRUE)) {
      GST_WARNING_OBJECT (demux, "Decoding codec private data failed");
      ret = GST_FLOW_ERROR;
    }
  }

  if (context->type == 0 || context->codec_id == NULL || (ret != GST_FLOW_OK
          && ret != GST_FLOW_EOS)) {
    if (ret == GST_FLOW_OK || ret == GST_FLOW_EOS)
      GST_WARNING_OBJECT (ebml, "Unknown stream/codec in track entry header");

    gst_matroska_track_free (context);
    context = NULL;
    *dest_context = NULL;
    return ret;
  }

  /* check for a cached track taglist  */
  cached_taglist =
      (GstTagList *) g_hash_table_lookup (demux->common.cached_track_taglists,
      GUINT_TO_POINTER (context->uid));
  if (cached_taglist)
    gst_tag_list_insert (context->tags, cached_taglist, GST_TAG_MERGE_APPEND);

  /* compute caps */
  switch (context->type) {
    case GST_MATROSKA_TRACK_TYPE_VIDEO:{
      GstMatroskaTrackVideoContext *videocontext =
          (GstMatroskaTrackVideoContext *) context;

      caps = gst_matroska_demux_video_caps (videocontext,
          context->codec_id, context->codec_priv,
          context->codec_priv_size, &codec, &riff_fourcc);

      if (codec) {
        gst_tag_list_add (context->tags, GST_TAG_MERGE_REPLACE,
            GST_TAG_VIDEO_CODEC, codec, NULL);
        context->tags_changed = TRUE;
        g_free (codec);
      }
      break;
    }

    case GST_MATROSKA_TRACK_TYPE_AUDIO:{
      GstClockTime lead_in_ts = 0;
      GstMatroskaTrackAudioContext *audiocontext =
          (GstMatroskaTrackAudioContext *) context;

      caps = gst_matroska_demux_audio_caps (audiocontext,
          context->codec_id, context->codec_priv, context->codec_priv_size,
          &codec, &riff_audio_fmt, &lead_in_ts);
      if (lead_in_ts > demux->audio_lead_in_ts) {
        demux->audio_lead_in_ts = lead_in_ts;
        GST_DEBUG_OBJECT (demux, "Increased audio lead-in to %" GST_TIME_FORMAT,
            GST_TIME_ARGS (lead_in_ts));
      }

      if (codec) {
        gst_tag_list_add (context->tags, GST_TAG_MERGE_REPLACE,
            GST_TAG_AUDIO_CODEC, codec, NULL);
        context->tags_changed = TRUE;
        g_free (codec);
      }
      break;
    }

    case GST_MATROSKA_TRACK_TYPE_SUBTITLE:{
      GstMatroskaTrackSubtitleContext *subtitlecontext =
          (GstMatroskaTrackSubtitleContext *) context;

      caps = gst_matroska_demux_subtitle_caps (subtitlecontext,
          context->codec_id, context->codec_priv, context->codec_priv_size);
      break;
    }

    case GST_MATROSKA_TRACK_TYPE_COMPLEX:
    case GST_MATROSKA_TRACK_TYPE_LOGO:
    case GST_MATROSKA_TRACK_TYPE_BUTTONS:
    case GST_MATROSKA_TRACK_TYPE_CONTROL:
    default:
      /* we should already have quit by now */
      g_assert_not_reached ();
  }

  if ((context->language == NULL || *context->language == '\0') &&
      (context->type == GST_MATROSKA_TRACK_TYPE_AUDIO ||
          context->type == GST_MATROSKA_TRACK_TYPE_SUBTITLE)) {
    GST_LOG ("stream %d: language=eng (assuming default)", context->index);
    context->language = g_strdup ("eng");
  }

  if (context->language) {
    const gchar *lang;

    /* Matroska contains ISO 639-2B codes, we want ISO 639-1 */
    lang = gst_tag_get_language_code (context->language);
    gst_tag_list_add (context->tags, GST_TAG_MERGE_REPLACE,
        GST_TAG_LANGUAGE_CODE, (lang) ? lang : context->language, NULL);

    if (context->name) {
      gst_tag_list_add (context->tags, GST_TAG_MERGE_REPLACE,
          GST_TAG_TITLE, context->name, NULL);
    }
    context->tags_changed = TRUE;
  }

  if (caps == NULL) {
    GST_WARNING_OBJECT (demux, "could not determine caps for stream with "
        "codec_id='%s'", context->codec_id);
    switch (context->type) {
      case GST_MATROSKA_TRACK_TYPE_VIDEO:
        caps = gst_caps_new_empty_simple ("video/x-unknown");
        break;
      case GST_MATROSKA_TRACK_TYPE_AUDIO:
        caps = gst_caps_new_empty_simple ("audio/x-unknown");
        break;
      case GST_MATROSKA_TRACK_TYPE_SUBTITLE:
        caps = gst_caps_new_empty_simple ("application/x-subtitle-unknown");
        break;
      case GST_MATROSKA_TRACK_TYPE_COMPLEX:
      default:
        caps = gst_caps_new_empty_simple ("application/x-matroska-unknown");
        break;
    }
    gst_caps_set_simple (caps, "codec-id", G_TYPE_STRING, context->codec_id,
        NULL);

    /* add any unrecognised riff fourcc / audio format, but after codec-id */
    if (context->type == GST_MATROSKA_TRACK_TYPE_AUDIO && riff_audio_fmt != 0)
      gst_caps_set_simple (caps, "format", G_TYPE_INT, riff_audio_fmt, NULL);
    else if (context->type == GST_MATROSKA_TRACK_TYPE_VIDEO && riff_fourcc != 0) {
      gchar *fstr = g_strdup_printf ("%" GST_FOURCC_FORMAT,
          GST_FOURCC_ARGS (riff_fourcc));
      gst_caps_set_simple (caps, "fourcc", G_TYPE_STRING, fstr, NULL);
      g_free (fstr);
    }
  } else if (context->stream_headers != NULL) {
    gst_matroska_demux_add_stream_headers_to_caps (demux,
        context->stream_headers, caps);
  }

  if (context->encodings) {
    GstMatroskaTrackEncoding *enc;
    guint i;

    for (i = 0; i < context->encodings->len; i++) {
      enc = &g_array_index (context->encodings, GstMatroskaTrackEncoding, i);
      if (enc->type == GST_MATROSKA_ENCODING_ENCRYPTION /* encryption */ ) {
        GstStructure *s = gst_caps_get_structure (caps, 0);
        if (!gst_structure_has_name (s, "application/x-webm-enc")) {
          gst_structure_set (s, "original-media-type", G_TYPE_STRING,
              gst_structure_get_name (s), NULL);
          gst_structure_set (s, "encryption-algorithm", G_TYPE_STRING,
              gst_matroska_track_encryption_algorithm_name (enc->enc_algo),
              NULL);
          gst_structure_set (s, "encoding-scope", G_TYPE_STRING,
              gst_matroska_track_encoding_scope_name (enc->scope), NULL);
          gst_structure_set (s, "cipher-mode", G_TYPE_STRING,
              gst_matroska_track_encryption_cipher_mode_name
              (enc->enc_cipher_mode), NULL);
          gst_structure_set_name (s, "application/x-webm-enc");
        }
      }
    }
  }

  context->caps = caps;

  /* tadaah! */
  *dest_context = context;
  return ret;
}

static void
gst_matroska_demux_add_stream (GstMatroskaDemux * demux,
    GstMatroskaTrackContext * context)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (demux);
  gchar *padname = NULL;
  GstPadTemplate *templ = NULL;
  GstStreamFlags stream_flags;

  GstEvent *stream_start;

  gchar *stream_id;

  g_ptr_array_add (demux->common.src, context);
  context->index = demux->common.num_streams++;
  g_assert (demux->common.src->len == demux->common.num_streams);
  g_ptr_array_index (demux->common.src, demux->common.num_streams - 1) =
      context;

  /* now create the GStreamer connectivity */
  switch (context->type) {
    case GST_MATROSKA_TRACK_TYPE_VIDEO:
      padname = g_strdup_printf ("video_%u", demux->num_v_streams++);
      templ = gst_element_class_get_pad_template (klass, "video_%u");

      if (!context->intra_only)
        demux->have_nonintraonly_v_streams = TRUE;
      break;

    case GST_MATROSKA_TRACK_TYPE_AUDIO:
      padname = g_strdup_printf ("audio_%u", demux->num_a_streams++);
      templ = gst_element_class_get_pad_template (klass, "audio_%u");
      break;

    case GST_MATROSKA_TRACK_TYPE_SUBTITLE:
      padname = g_strdup_printf ("subtitle_%u", demux->num_t_streams++);
      templ = gst_element_class_get_pad_template (klass, "subtitle_%u");
      break;

    default:
      /* we should already have quit by now */
      g_assert_not_reached ();
  }

  /* the pad in here */
  context->pad = gst_pad_new_from_template (templ, padname);

  gst_pad_set_event_function (context->pad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_handle_src_event));
  gst_pad_set_query_function (context->pad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_handle_src_query));

  GST_INFO_OBJECT (demux, "Adding pad '%s' with caps %" GST_PTR_FORMAT,
      padname, context->caps);

  gst_pad_set_element_private (context->pad, context);

  gst_pad_use_fixed_caps (context->pad);
  gst_pad_set_active (context->pad, TRUE);

  stream_id =
      gst_pad_create_stream_id_printf (context->pad, GST_ELEMENT_CAST (demux),
      "%03" G_GUINT64_FORMAT ":%03" G_GUINT64_FORMAT,
      context->num, context->uid);
  stream_start =
      gst_pad_get_sticky_event (demux->common.sinkpad, GST_EVENT_STREAM_START,
      0);
  if (stream_start) {
    if (gst_event_parse_group_id (stream_start, &demux->group_id))
      demux->have_group_id = TRUE;
    else
      demux->have_group_id = FALSE;
    gst_event_unref (stream_start);
  } else if (!demux->have_group_id) {
    demux->have_group_id = TRUE;
    demux->group_id = gst_util_group_id_next ();
  }

  stream_start = gst_event_new_stream_start (stream_id);
  g_free (stream_id);
  if (demux->have_group_id)
    gst_event_set_group_id (stream_start, demux->group_id);
  stream_flags = GST_STREAM_FLAG_NONE;
  if (context->type == GST_MATROSKA_TRACK_TYPE_SUBTITLE)
    stream_flags |= GST_STREAM_FLAG_SPARSE;
  if (context->flags & GST_MATROSKA_TRACK_DEFAULT)
    stream_flags |= GST_STREAM_FLAG_SELECT;
  else if (!(context->flags & GST_MATROSKA_TRACK_ENABLED))
    stream_flags |= GST_STREAM_FLAG_UNSELECT;

  gst_event_set_stream_flags (stream_start, stream_flags);
  gst_pad_push_event (context->pad, stream_start);
  gst_pad_set_caps (context->pad, context->caps);


  if (demux->common.global_tags) {
    GstEvent *tag_event;

    gst_tag_list_add (demux->common.global_tags, GST_TAG_MERGE_REPLACE,
        GST_TAG_CONTAINER_FORMAT, "Matroska", NULL);
    GST_DEBUG_OBJECT (context->pad, "Sending global_tags %p: %" GST_PTR_FORMAT,
        demux->common.global_tags, demux->common.global_tags);

    tag_event =
        gst_event_new_tag (gst_tag_list_copy (demux->common.global_tags));

    gst_pad_push_event (context->pad, tag_event);
  }

  if (G_UNLIKELY (context->tags_changed)) {
    GST_DEBUG_OBJECT (context->pad, "Sending tags %p: %"
        GST_PTR_FORMAT, context->tags, context->tags);
    gst_pad_push_event (context->pad,
        gst_event_new_tag (gst_tag_list_copy (context->tags)));
    context->tags_changed = FALSE;
  }

  gst_element_add_pad (GST_ELEMENT (demux), context->pad);
  gst_flow_combiner_add_pad (demux->flowcombiner, context->pad);

  g_free (padname);
}

static gboolean
gst_matroska_demux_query (GstMatroskaDemux * demux, GstPad * pad,
    GstQuery * query)
{
  gboolean res = FALSE;
  GstMatroskaTrackContext *context = NULL;

  if (pad) {
    context = gst_pad_get_element_private (pad);
  }

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:
    {
      GstFormat format;

      gst_query_parse_position (query, &format, NULL);

      res = TRUE;
      if (format == GST_FORMAT_TIME) {
        GST_OBJECT_LOCK (demux);
        if (context)
          gst_query_set_position (query, GST_FORMAT_TIME,
              MAX (context->pos, demux->stream_start_time) -
              demux->stream_start_time);
        else
          gst_query_set_position (query, GST_FORMAT_TIME,
              MAX (demux->common.segment.position, demux->stream_start_time) -
              demux->stream_start_time);
        GST_OBJECT_UNLOCK (demux);
      } else if (format == GST_FORMAT_DEFAULT && context
          && context->default_duration) {
        GST_OBJECT_LOCK (demux);
        gst_query_set_position (query, GST_FORMAT_DEFAULT,
            context->pos / context->default_duration);
        GST_OBJECT_UNLOCK (demux);
      } else {
        GST_DEBUG_OBJECT (demux,
            "only position query in TIME and DEFAULT format is supported");
        res = FALSE;
      }

      break;
    }
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      res = TRUE;
      if (format == GST_FORMAT_TIME) {
        GST_OBJECT_LOCK (demux);
        gst_query_set_duration (query, GST_FORMAT_TIME,
            demux->common.segment.duration);
        GST_OBJECT_UNLOCK (demux);
      } else if (format == GST_FORMAT_DEFAULT && context
          && context->default_duration) {
        GST_OBJECT_LOCK (demux);
        gst_query_set_duration (query, GST_FORMAT_DEFAULT,
            demux->common.segment.duration / context->default_duration);
        GST_OBJECT_UNLOCK (demux);
      } else {
        GST_DEBUG_OBJECT (demux,
            "only duration query in TIME and DEFAULT format is supported");
        res = FALSE;
      }
      break;
    }

    case GST_QUERY_SEEKING:
    {
      GstFormat fmt;

      gst_query_parse_seeking (query, &fmt, NULL, NULL, NULL);
      GST_OBJECT_LOCK (demux);
      if (fmt == GST_FORMAT_TIME) {
        gboolean seekable;

        if (demux->streaming) {
          /* assuming we'll be able to get an index ... */
          seekable = demux->seekable;
        } else {
          seekable = TRUE;
        }

        gst_query_set_seeking (query, GST_FORMAT_TIME, seekable,
            0, demux->common.segment.duration);
        res = TRUE;
      }
      GST_OBJECT_UNLOCK (demux);
      break;
    }
    case GST_QUERY_SEGMENT:
    {
      GstFormat format;
      gint64 start, stop;

      format = demux->common.segment.format;

      start =
          gst_segment_to_stream_time (&demux->common.segment, format,
          demux->common.segment.start);
      if ((stop = demux->common.segment.stop) == -1)
        stop = demux->common.segment.duration;
      else
        stop =
            gst_segment_to_stream_time (&demux->common.segment, format, stop);

      gst_query_set_segment (query, demux->common.segment.rate, format, start,
          stop);
      res = TRUE;
      break;
    }
    default:
      if (pad)
        res = gst_pad_query_default (pad, (GstObject *) demux, query);
      else
        res =
            GST_ELEMENT_CLASS (parent_class)->query (GST_ELEMENT_CAST (demux),
            query);
      break;
  }

  return res;
}

static gboolean
gst_matroska_demux_element_query (GstElement * element, GstQuery * query)
{
  return gst_matroska_demux_query (GST_MATROSKA_DEMUX (element), NULL, query);
}

static gboolean
gst_matroska_demux_handle_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (parent);

  return gst_matroska_demux_query (demux, pad, query);
}

/* returns FALSE if there are no pads to deliver event to,
 * otherwise TRUE (whatever the outcome of event sending),
 * takes ownership of the passed event! */
static gboolean
gst_matroska_demux_send_event (GstMatroskaDemux * demux, GstEvent * event)
{
  gboolean ret = FALSE;
  gint i;

  g_return_val_if_fail (event != NULL, FALSE);

  GST_DEBUG_OBJECT (demux, "Sending event of type %s to all source pads",
      GST_EVENT_TYPE_NAME (event));

  g_assert (demux->common.src->len == demux->common.num_streams);
  for (i = 0; i < demux->common.src->len; i++) {
    GstMatroskaTrackContext *stream;

    stream = g_ptr_array_index (demux->common.src, i);
    gst_event_ref (event);
    gst_pad_push_event (stream->pad, event);
    ret = TRUE;
  }

  gst_event_unref (event);
  return ret;
}

static void
gst_matroska_demux_send_tags (GstMatroskaDemux * demux)
{
  gint i;

  if (G_UNLIKELY (demux->common.global_tags_changed)) {
    GstEvent *tag_event;
    gst_tag_list_add (demux->common.global_tags, GST_TAG_MERGE_REPLACE,
        GST_TAG_CONTAINER_FORMAT, "Matroska", NULL);
    GST_DEBUG_OBJECT (demux, "Sending global_tags %p : %" GST_PTR_FORMAT,
        demux->common.global_tags, demux->common.global_tags);

    tag_event =
        gst_event_new_tag (gst_tag_list_copy (demux->common.global_tags));

    for (i = 0; i < demux->common.src->len; i++) {
      GstMatroskaTrackContext *stream;

      stream = g_ptr_array_index (demux->common.src, i);
      gst_pad_push_event (stream->pad, gst_event_ref (tag_event));
    }

    gst_event_unref (tag_event);
    demux->common.global_tags_changed = FALSE;
  }

  g_assert (demux->common.src->len == demux->common.num_streams);
  for (i = 0; i < demux->common.src->len; i++) {
    GstMatroskaTrackContext *stream;

    stream = g_ptr_array_index (demux->common.src, i);

    if (G_UNLIKELY (stream->tags_changed)) {
      GST_DEBUG_OBJECT (demux, "Sending tags %p for pad %s:%s : %"
          GST_PTR_FORMAT, stream->tags,
          GST_DEBUG_PAD_NAME (stream->pad), stream->tags);
      gst_pad_push_event (stream->pad,
          gst_event_new_tag (gst_tag_list_copy (stream->tags)));
      stream->tags_changed = FALSE;
    }
  }
}

static gboolean
gst_matroska_demux_element_send_event (GstElement * element, GstEvent * event)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);
  gboolean res;

  g_return_val_if_fail (event != NULL, FALSE);

  if (GST_EVENT_TYPE (event) == GST_EVENT_SEEK) {
    /* no seeking until we are (safely) ready */
    if (demux->common.state != GST_MATROSKA_READ_STATE_DATA) {
      GST_DEBUG_OBJECT (demux,
          "not ready for seeking yet, deferring seek: %" GST_PTR_FORMAT, event);
      if (demux->deferred_seek_event)
        gst_event_unref (demux->deferred_seek_event);
      demux->deferred_seek_event = event;
      demux->deferred_seek_pad = NULL;
      return TRUE;
    }
    res = gst_matroska_demux_handle_seek_event (demux, NULL, event);
  } else {
    GST_WARNING_OBJECT (demux, "Unhandled event of type %s",
        GST_EVENT_TYPE_NAME (event));
    res = FALSE;
  }
  gst_event_unref (event);
  return res;
}

static gboolean
gst_matroska_demux_move_to_entry (GstMatroskaDemux * demux,
    GstMatroskaIndex * entry, gboolean reset, gboolean update)
{
  gint i;

  GST_OBJECT_LOCK (demux);

  if (update) {
    /* seek (relative to matroska segment) */
    /* position might be invalid; will error when streaming resumes ... */
    demux->common.offset = entry->pos + demux->common.ebml_segment_start;
    demux->next_cluster_offset = 0;

    GST_DEBUG_OBJECT (demux,
        "Seeked to offset %" G_GUINT64_FORMAT ", block %d, " "time %"
        GST_TIME_FORMAT, entry->pos + demux->common.ebml_segment_start,
        entry->block, GST_TIME_ARGS (entry->time));

    /* update the time */
    gst_matroska_read_common_reset_streams (&demux->common, entry->time, TRUE);
    gst_flow_combiner_reset (demux->flowcombiner);
    demux->common.segment.position = entry->time;
    demux->seek_block = entry->block;
    demux->seek_first = TRUE;
    demux->last_stop_end = GST_CLOCK_TIME_NONE;
  }

  for (i = 0; i < demux->common.src->len; i++) {
    GstMatroskaTrackContext *stream = g_ptr_array_index (demux->common.src, i);

    if (reset) {
      stream->to_offset = G_MAXINT64;
    } else {
      if (stream->from_offset != -1)
        stream->to_offset = stream->from_offset;
    }
    stream->from_offset = -1;
    stream->from_time = GST_CLOCK_TIME_NONE;
  }

  GST_OBJECT_UNLOCK (demux);

  return TRUE;
}

static gint
gst_matroska_cluster_compare (gint64 * i1, gint64 * i2)
{
  if (*i1 < *i2)
    return -1;
  else if (*i1 > *i2)
    return 1;
  else
    return 0;
}

/* searches for a cluster start from @pos,
 * return GST_FLOW_OK and cluster position in @pos if found */
static GstFlowReturn
gst_matroska_demux_search_cluster (GstMatroskaDemux * demux, gint64 * pos,
    gboolean forward)
{
  gint64 newpos = *pos;
  gint64 orig_offset;
  GstFlowReturn ret = GST_FLOW_OK;
  const guint chunk = 128 * 1024;
  GstBuffer *buf = NULL;
  GstMapInfo map;
  gpointer data = NULL;
  gsize size;
  guint64 length;
  guint32 id;
  guint needed;
  gint64 oldpos, oldlength;

  orig_offset = demux->common.offset;

  GST_LOG_OBJECT (demux, "searching cluster %s offset %" G_GINT64_FORMAT,
      forward ? "following" : "preceding", *pos);

  if (demux->clusters) {
    gint64 *cpos;

    cpos = gst_util_array_binary_search (demux->clusters->data,
        demux->clusters->len, sizeof (gint64),
        (GCompareDataFunc) gst_matroska_cluster_compare,
        forward ? GST_SEARCH_MODE_AFTER : GST_SEARCH_MODE_BEFORE, pos, NULL);
    /* sanity check */
    if (cpos) {
      GST_DEBUG_OBJECT (demux,
          "cluster reported at offset %" G_GINT64_FORMAT, *cpos);
      demux->common.offset = *cpos;
      ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
          GST_ELEMENT_CAST (demux), &id, &length, &needed);
      if (ret == GST_FLOW_OK && id == GST_MATROSKA_ID_CLUSTER) {
        newpos = *cpos;
        goto exit;
      }
    }
  }

  /* read in at newpos and scan for ebml cluster id */
  oldpos = oldlength = -1;
  while (1) {
    GstByteReader reader;
    gint cluster_pos;
    guint toread = chunk;

    if (!forward) {
      /* never read beyond the requested target */
      if (G_UNLIKELY (newpos < chunk)) {
        toread = newpos;
        newpos = 0;
      } else {
        newpos -= chunk;
      }
    }
    if (buf != NULL) {
      gst_buffer_unmap (buf, &map);
      gst_buffer_unref (buf);
      buf = NULL;
    }
    ret = gst_pad_pull_range (demux->common.sinkpad, newpos, toread, &buf);
    if (ret != GST_FLOW_OK)
      break;
    GST_DEBUG_OBJECT (demux,
        "read buffer size %" G_GSIZE_FORMAT " at offset %" G_GINT64_FORMAT,
        gst_buffer_get_size (buf), newpos);
    gst_buffer_map (buf, &map, GST_MAP_READ);
    data = map.data;
    size = map.size;
    if (oldpos == newpos && oldlength == map.size) {
      GST_ERROR_OBJECT (demux, "Stuck at same position");
      ret = GST_FLOW_ERROR;
      goto exit;
    } else {
      oldpos = newpos;
      oldlength = map.size;
    }

    gst_byte_reader_init (&reader, data, size);
    cluster_pos = -1;
    while (1) {
      gint found = gst_byte_reader_masked_scan_uint32 (&reader, 0xffffffff,
          GST_MATROSKA_ID_CLUSTER, 0, gst_byte_reader_get_remaining (&reader));
      if (forward) {
        cluster_pos = found;
        break;
      }
      /* need last occurrence when searching backwards */
      if (found >= 0) {
        cluster_pos = gst_byte_reader_get_pos (&reader) + found;
        gst_byte_reader_skip (&reader, found + 4);
      } else {
        break;
      }
    }

    if (cluster_pos >= 0) {
      newpos += cluster_pos;
      GST_DEBUG_OBJECT (demux,
          "found cluster ebml id at offset %" G_GINT64_FORMAT, newpos);
      /* extra checks whether we really sync'ed to a cluster:
       * - either it is the first and only cluster
       * - either there is a cluster after this one
       * - either cluster length is undefined
       */
      /* ok if first cluster (there may not a subsequent one) */
      if (newpos == demux->first_cluster_offset) {
        GST_DEBUG_OBJECT (demux, "cluster is first cluster -> OK");
        break;
      }
      demux->common.offset = newpos;
      ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
          GST_ELEMENT_CAST (demux), &id, &length, &needed);
      if (ret != GST_FLOW_OK) {
        GST_DEBUG_OBJECT (demux, "need more data -> continue");
        goto next;
      }
      g_assert (id == GST_MATROSKA_ID_CLUSTER);
      GST_DEBUG_OBJECT (demux, "cluster size %" G_GUINT64_FORMAT ", prefix %d",
          length, needed);
      /* ok if undefined length or first cluster */
      if (length == GST_EBML_SIZE_UNKNOWN || length == G_MAXUINT64) {
        GST_DEBUG_OBJECT (demux, "cluster has undefined length -> OK");
        break;
      }
      /* skip cluster */
      demux->common.offset += length + needed;
      ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
          GST_ELEMENT_CAST (demux), &id, &length, &needed);
      if (ret != GST_FLOW_OK)
        goto next;
      GST_DEBUG_OBJECT (demux, "next element is %scluster",
          id == GST_MATROSKA_ID_CLUSTER ? "" : "not ");
      if (id == GST_MATROSKA_ID_CLUSTER)
        break;
    next:
      if (forward)
        newpos += 1;
    } else {
      /* partial cluster id may have been in tail of buffer */
      newpos +=
          forward ? MAX (gst_byte_reader_get_remaining (&reader), 4) - 3 : 3;
    }
  }

  if (buf) {
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    buf = NULL;
  }

exit:
  demux->common.offset = orig_offset;
  *pos = newpos;
  return ret;
}

/* Three states to express: starts with I-frame, starts with delta, don't know */
typedef enum
{
  CLUSTER_STATUS_NONE = 0,
  CLUSTER_STATUS_STARTS_WITH_KEYFRAME,
  CLUSTER_STATUS_STARTS_WITH_DELTAUNIT,
} ClusterStatus;

typedef struct
{
  guint64 offset;
  guint64 size;
  guint64 prev_size;
  GstClockTime time;
  ClusterStatus status;
} ClusterInfo;

static const gchar *
cluster_status_get_nick (ClusterStatus status)
{
  switch (status) {
    case CLUSTER_STATUS_NONE:
      return "none";
    case CLUSTER_STATUS_STARTS_WITH_KEYFRAME:
      return "key";
    case CLUSTER_STATUS_STARTS_WITH_DELTAUNIT:
      return "delta";
  }
  return "???";
}

/* Skip ebml-coded number:
 *  1xxx.. = 1 byte
 *  01xx.. = 2 bytes
 *  001x.. = 3 bytes, etc.
 */
static gboolean
bit_reader_skip_ebml_num (GstBitReader * br)
{
  guint8 i, v = 0;

  if (!gst_bit_reader_peek_bits_uint8 (br, &v, 8))
    return FALSE;

  for (i = 0; i < 8; i++) {
    if ((v & (0x80 >> i)) != 0)
      break;
  }
  return gst_bit_reader_skip (br, (i + 1) * 8);
}

/* Don't probe more than that many bytes into the cluster for keyframe info
 * (random value, mostly for sanity checking) */
#define MAX_CLUSTER_INFO_PROBE_LENGTH 256

static gboolean
gst_matroska_demux_peek_cluster_info (GstMatroskaDemux * demux,
    ClusterInfo * cluster, guint64 offset)
{
  demux->common.offset = offset;
  demux->cluster_time = GST_CLOCK_TIME_NONE;

  cluster->offset = offset;
  cluster->size = 0;
  cluster->prev_size = 0;
  cluster->time = GST_CLOCK_TIME_NONE;
  cluster->status = CLUSTER_STATUS_NONE;

  /* parse first few elements in cluster */
  do {
    GstFlowReturn flow;
    guint64 length;
    guint32 id;
    guint needed;

    flow = gst_matroska_read_common_peek_id_length_pull (&demux->common,
        GST_ELEMENT_CAST (demux), &id, &length, &needed);

    if (flow != GST_FLOW_OK)
      break;

    GST_LOG_OBJECT (demux, "Offset %" G_GUINT64_FORMAT ", Element id 0x%x, "
        "size %" G_GUINT64_FORMAT ", needed %d", demux->common.offset, id,
        length, needed);

    /* Reached start of next cluster without finding data, stop processing */
    if (id == GST_MATROSKA_ID_CLUSTER && cluster->offset != offset)
      break;

    /* Not going to parse into these for now, stop processing */
    if (id == GST_MATROSKA_ID_ENCRYPTEDBLOCK
        || id == GST_MATROSKA_ID_BLOCKGROUP || id == GST_MATROSKA_ID_BLOCK)
      break;

    /* SimpleBlock: peek at headers to check if it's a keyframe */
    if (id == GST_MATROSKA_ID_SIMPLEBLOCK) {
      GstBitReader br;
      guint8 *d, hdr_len, v = 0;

      GST_DEBUG_OBJECT (demux, "SimpleBlock found");

      /* SimpleBlock header is max. 21 bytes */
      hdr_len = MIN (21, length);

      flow = gst_matroska_read_common_peek_bytes (&demux->common,
          demux->common.offset, hdr_len, NULL, &d);

      if (flow != GST_FLOW_OK)
        break;

      gst_bit_reader_init (&br, d, hdr_len);

      /* skip prefix: ebml id (SimpleBlock) + element length */
      if (!gst_bit_reader_skip (&br, 8 * needed))
        break;

      /* skip track number (ebml coded) */
      if (!bit_reader_skip_ebml_num (&br))
        break;

      /* skip Timecode */
      if (!gst_bit_reader_skip (&br, 16))
        break;

      /* read flags */
      if (!gst_bit_reader_get_bits_uint8 (&br, &v, 8))
        break;

      if ((v & 0x80) != 0)
        cluster->status = CLUSTER_STATUS_STARTS_WITH_KEYFRAME;
      else
        cluster->status = CLUSTER_STATUS_STARTS_WITH_DELTAUNIT;

      break;
    }

    flow = gst_matroska_demux_parse_id (demux, id, length, needed);

    if (flow != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_CLUSTER:
        if (length == G_MAXUINT64)
          cluster->size = 0;
        else
          cluster->size = length + needed;
        break;
      case GST_MATROSKA_ID_PREVSIZE:
        cluster->prev_size = demux->cluster_prevsize;
        break;
      case GST_MATROSKA_ID_CLUSTERTIMECODE:
        cluster->time = demux->cluster_time * demux->common.time_scale;
        break;
      case GST_MATROSKA_ID_SILENTTRACKS:
      case GST_EBML_ID_CRC32:
        /* ignore and continue */
        break;
      default:
        GST_WARNING_OBJECT (demux, "Unknown ebml id 0x%08x (possibly garbage), "
            "bailing out", id);
        goto out;
    }
  } while (demux->common.offset - offset < MAX_CLUSTER_INFO_PROBE_LENGTH);

out:

  GST_INFO_OBJECT (demux, "Cluster @ %" G_GUINT64_FORMAT ": "
      "time %" GST_TIME_FORMAT ", size %" G_GUINT64_FORMAT ", "
      "prev_size %" G_GUINT64_FORMAT ", %s", cluster->offset,
      GST_TIME_ARGS (cluster->time), cluster->size, cluster->prev_size,
      cluster_status_get_nick (cluster->status));

  /* return success as long as we could extract the minimum useful information */
  return cluster->time != GST_CLOCK_TIME_NONE;
}

/* returns TRUE if the cluster offset was updated */
static gboolean
gst_matroska_demux_scan_back_for_keyframe_cluster (GstMatroskaDemux * demux,
    gint64 * cluster_offset, GstClockTime * cluster_time)
{
  GstClockTime stream_start_time = demux->stream_start_time;
  guint64 first_cluster_offset = demux->first_cluster_offset;
  gint64 off = *cluster_offset;
  ClusterInfo cluster = { 0, };

  GST_INFO_OBJECT (demux, "Checking if cluster starts with keyframe");
  while (off > first_cluster_offset) {
    if (!gst_matroska_demux_peek_cluster_info (demux, &cluster, off)) {
      GST_LOG_OBJECT (demux,
          "Couldn't get info on cluster @ %" G_GUINT64_FORMAT, off);
      break;
    }

    /* Keyframe? Then we're done */
    if (cluster.status == CLUSTER_STATUS_STARTS_WITH_KEYFRAME) {
      GST_LOG_OBJECT (demux,
          "Found keyframe at start of cluster @ %" G_GUINT64_FORMAT, off);
      break;
    }

    /* We only scan back if we *know* we landed on a cluster that
     * starts with a delta frame. */
    if (cluster.status != CLUSTER_STATUS_STARTS_WITH_DELTAUNIT) {
      GST_LOG_OBJECT (demux,
          "No delta frame at start of cluster @ %" G_GUINT64_FORMAT, off);
      break;
    }

    GST_DEBUG_OBJECT (demux, "Cluster starts with delta frame, backtracking");

    /* Don't scan back more than this much in time from the cluster we
     * originally landed on. This is mostly a sanity check in case a file
     * always has keyframes in the middle of clusters and never at the
     * beginning. Without this we would always scan back to the beginning
     * of the file in that case. */
    if (cluster.time != GST_CLOCK_TIME_NONE) {
      GstClockTimeDiff distance = GST_CLOCK_DIFF (cluster.time, *cluster_time);

      if (distance < 0 || distance > demux->max_backtrack_distance * GST_SECOND) {
        GST_DEBUG_OBJECT (demux, "Haven't found cluster with keyframe within "
            "%u secs of original seek target cluster, stopping",
            demux->max_backtrack_distance);
        break;
      }
    }

    /* If we have cluster prev_size we can skip back efficiently. If not,
     * we'll just do a brute force search for a cluster identifier */
    if (cluster.prev_size > 0 && off >= cluster.prev_size) {
      off -= cluster.prev_size;
    } else {
      GstFlowReturn flow;

      GST_LOG_OBJECT (demux, "Cluster has no or invalid prev size, searching "
          "for previous cluster instead then");

      flow = gst_matroska_demux_search_cluster (demux, &off, FALSE);
      if (flow != GST_FLOW_OK) {
        GST_DEBUG_OBJECT (demux, "cluster search yielded flow %s, stopping",
            gst_flow_get_name (flow));
        break;
      }
    }

    if (off <= first_cluster_offset) {
      GST_LOG_OBJECT (demux, "Reached first cluster, stopping");
      *cluster_offset = first_cluster_offset;
      *cluster_time = stream_start_time;
      return TRUE;
    }
    GST_LOG_OBJECT (demux, "Trying prev cluster @ %" G_GUINT64_FORMAT, off);
  }

  /* If we found a cluster starting with a keyframe jump to that instead,
   * otherwise leave everything as it was before */
  if (cluster.time != GST_CLOCK_TIME_NONE
      && (cluster.offset == first_cluster_offset
          || cluster.status == CLUSTER_STATUS_STARTS_WITH_KEYFRAME)) {
    *cluster_offset = cluster.offset;
    *cluster_time = cluster.time;
    return TRUE;
  }

  return FALSE;
}

/* bisect and scan through file for cluster starting before @time,
 * returns fake index entry with corresponding info on cluster */
static GstMatroskaIndex *
gst_matroska_demux_search_pos (GstMatroskaDemux * demux, GstClockTime time)
{
  GstMatroskaIndex *entry = NULL;
  GstMatroskaReadState current_state;
  GstClockTime otime, prev_cluster_time, current_cluster_time, cluster_time;
  GstClockTime atime;
  gint64 opos, newpos, current_offset;
  gint64 prev_cluster_offset = -1, current_cluster_offset, cluster_offset;
  gint64 apos, maxpos;
  guint64 cluster_size = 0;
  GstFlowReturn ret;
  guint64 length;
  guint32 id;
  guint needed;

  /* estimate new position, resync using cluster ebml id,
   * and bisect further or scan forward to appropriate cluster */

  /* save some current global state which will be touched by our scanning */
  current_state = demux->common.state;
  g_return_val_if_fail (current_state == GST_MATROSKA_READ_STATE_DATA, NULL);

  current_cluster_offset = demux->cluster_offset;
  current_cluster_time = demux->cluster_time;
  current_offset = demux->common.offset;

  demux->common.state = GST_MATROSKA_READ_STATE_SCANNING;

  /* estimate using start and last known cluster */
  GST_OBJECT_LOCK (demux);
  apos = demux->first_cluster_offset;
  atime = demux->stream_start_time;
  opos = demux->last_cluster_offset;
  otime = demux->stream_last_time;
  GST_OBJECT_UNLOCK (demux);

  /* sanitize */
  time = MAX (time, atime);
  otime = MAX (otime, atime);
  opos = MAX (opos, apos);

  maxpos = gst_matroska_read_common_get_length (&demux->common);

  /* invariants;
   * apos <= opos
   * atime <= otime
   * apos always refer to a cluster before target time;
   * opos may or may not be after target time, but if it is once so,
   * then also in next iteration
   * */

retry:
  GST_LOG_OBJECT (demux,
      "apos: %" G_GUINT64_FORMAT ", atime: %" GST_TIME_FORMAT ", %"
      GST_TIME_FORMAT " in stream time, "
      "opos: %" G_GUINT64_FORMAT ", otime: %" GST_TIME_FORMAT ", %"
      GST_TIME_FORMAT " in stream time (start %" GST_TIME_FORMAT "), time %"
      GST_TIME_FORMAT, apos, GST_TIME_ARGS (atime),
      GST_TIME_ARGS (atime - demux->stream_start_time), opos,
      GST_TIME_ARGS (otime), GST_TIME_ARGS (otime - demux->stream_start_time),
      GST_TIME_ARGS (demux->stream_start_time), GST_TIME_ARGS (time));

  g_assert (atime <= otime);
  g_assert (apos <= opos);
  if (time == GST_CLOCK_TIME_NONE) {
    GST_DEBUG_OBJECT (demux, "searching last cluster");
    newpos = maxpos;
    if (newpos == -1) {
      GST_DEBUG_OBJECT (demux, "unknown file size; bailing out");
      goto exit;
    }
  } else if (otime <= atime) {
    newpos = apos;
  } else {
    newpos = apos +
        gst_util_uint64_scale (opos - apos, time - atime, otime - atime);
    if (maxpos != -1 && newpos > maxpos)
      newpos = maxpos;
  }

  GST_DEBUG_OBJECT (demux,
      "estimated offset for %" GST_TIME_FORMAT ": %" G_GINT64_FORMAT,
      GST_TIME_ARGS (time), newpos);

  /* search backwards */
  if (newpos > apos) {
    ret = gst_matroska_demux_search_cluster (demux, &newpos, FALSE);
    if (ret != GST_FLOW_OK)
      goto exit;
  }

  /* then start scanning and parsing for cluster time,
   * re-estimate if possible, otherwise next cluster and so on */
  /* note that each re-estimate is entered with a change in apos or opos,
   * avoiding infinite loop */
  demux->common.offset = newpos;
  demux->cluster_time = cluster_time = GST_CLOCK_TIME_NONE;
  cluster_size = 0;
  prev_cluster_time = GST_CLOCK_TIME_NONE;
  while (1) {
    /* peek and parse some elements */
    ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
        GST_ELEMENT_CAST (demux), &id, &length, &needed);
    if (ret != GST_FLOW_OK)
      goto error;
    GST_LOG_OBJECT (demux, "Offset %" G_GUINT64_FORMAT ", Element id 0x%x, "
        "size %" G_GUINT64_FORMAT ", needed %d", demux->common.offset, id,
        length, needed);
    ret = gst_matroska_demux_parse_id (demux, id, length, needed);
    if (ret != GST_FLOW_OK)
      goto error;

    if (id == GST_MATROSKA_ID_CLUSTER) {
      cluster_time = GST_CLOCK_TIME_NONE;
      if (length == G_MAXUINT64)
        cluster_size = 0;
      else
        cluster_size = length + needed;
    }
    if (demux->cluster_time != GST_CLOCK_TIME_NONE &&
        cluster_time == GST_CLOCK_TIME_NONE) {
      cluster_time = demux->cluster_time * demux->common.time_scale;
      cluster_offset = demux->cluster_offset;
      GST_DEBUG_OBJECT (demux, "found cluster at offset %" G_GINT64_FORMAT
          " with time %" GST_TIME_FORMAT, cluster_offset,
          GST_TIME_ARGS (cluster_time));
      if (time == GST_CLOCK_TIME_NONE) {
        GST_DEBUG_OBJECT (demux, "found last cluster");
        prev_cluster_time = cluster_time;
        prev_cluster_offset = cluster_offset;
        break;
      }
      if (cluster_time > time) {
        GST_DEBUG_OBJECT (demux, "overshot target");
        /* cluster overshoots */
        if (cluster_offset == demux->first_cluster_offset) {
          /* but no prev one */
          GST_DEBUG_OBJECT (demux, "but using first cluster anyway");
          prev_cluster_time = cluster_time;
          prev_cluster_offset = cluster_offset;
          break;
        }
        if (prev_cluster_time != GST_CLOCK_TIME_NONE) {
          /* prev cluster did not overshoot, so prev cluster is target */
          break;
        } else {
          /* re-estimate using this new position info */
          opos = cluster_offset;
          otime = cluster_time;
          goto retry;
        }
      } else {
        /* cluster undershoots */
        GST_DEBUG_OBJECT (demux, "undershot target");
        /* ok if close enough */
        if (GST_CLOCK_DIFF (cluster_time, time) < 5 * GST_SECOND) {
          GST_DEBUG_OBJECT (demux, "target close enough");
          prev_cluster_time = cluster_time;
          prev_cluster_offset = cluster_offset;
          break;
        }
        if (otime > time) {
          /* we are in between atime and otime => can bisect if worthwhile */
          if (prev_cluster_time != GST_CLOCK_TIME_NONE &&
              cluster_time > prev_cluster_time &&
              (GST_CLOCK_DIFF (prev_cluster_time, cluster_time) * 10 <
                  GST_CLOCK_DIFF (cluster_time, time))) {
            /* we moved at least one cluster forward,
             * and it looks like target is still far away,
             * let's estimate again */
            GST_DEBUG_OBJECT (demux, "bisecting with new apos");
            apos = cluster_offset;
            atime = cluster_time;
            goto retry;
          }
        }
        /* cluster undershoots, goto next one */
        prev_cluster_time = cluster_time;
        prev_cluster_offset = cluster_offset;
        /* skip cluster if length is defined,
         * otherwise will be skippingly parsed into */
        if (cluster_size) {
          GST_DEBUG_OBJECT (demux, "skipping to next cluster");
          demux->common.offset = cluster_offset + cluster_size;
          demux->cluster_time = GST_CLOCK_TIME_NONE;
        } else {
          GST_DEBUG_OBJECT (demux, "parsing/skipping cluster elements");
        }
      }
    }
    continue;

  error:
    if (ret == GST_FLOW_EOS) {
      if (prev_cluster_time != GST_CLOCK_TIME_NONE)
        break;
    }
    goto exit;
  }

  /* In the bisect loop above we always undershoot and then jump forward
   * cluster-by-cluster until we overshoot, so if we get here we've gone
   * over and the previous cluster is where we need to go to. */
  cluster_offset = prev_cluster_offset;
  cluster_time = prev_cluster_time;

  /* If we have video and can easily backtrack, check if we landed on a cluster
   * that starts with a keyframe - and if not backtrack until we find one that
   * does. */
  if (demux->have_nonintraonly_v_streams && demux->max_backtrack_distance > 0) {
    if (gst_matroska_demux_scan_back_for_keyframe_cluster (demux,
            &cluster_offset, &cluster_time)) {
      GST_INFO_OBJECT (demux, "Adjusted cluster to %" GST_TIME_FORMAT " @ "
          "%" G_GUINT64_FORMAT, GST_TIME_ARGS (cluster_time), cluster_offset);
    }
  }

  entry = g_new0 (GstMatroskaIndex, 1);
  entry->time = cluster_time;
  entry->pos = cluster_offset - demux->common.ebml_segment_start;
  GST_DEBUG_OBJECT (demux, "simulated index entry; time %" GST_TIME_FORMAT
      ", pos %" G_GUINT64_FORMAT, GST_TIME_ARGS (entry->time), entry->pos);

exit:

  /* restore some state */
  demux->cluster_offset = current_cluster_offset;
  demux->cluster_time = current_cluster_time;
  demux->common.offset = current_offset;
  demux->common.state = current_state;

  return entry;
}

static gboolean
gst_matroska_demux_handle_seek_event (GstMatroskaDemux * demux,
    GstPad * pad, GstEvent * event)
{
  GstMatroskaIndex *entry = NULL;
  GstMatroskaIndex scan_entry;
  GstSeekFlags flags;
  GstSeekType cur_type, stop_type;
  GstFormat format;
  gboolean flush, keyunit, before, after, accurate, snap_next;
  gdouble rate;
  gint64 cur, stop;
  GstMatroskaTrackContext *track = NULL;
  GstSegment seeksegment = { 0, };
  guint64 seekpos;
  gboolean update = TRUE;
  gboolean pad_locked = FALSE;
  guint32 seqnum;
  GstSearchMode snap_dir;

  g_return_val_if_fail (event != NULL, FALSE);

  if (pad)
    track = gst_pad_get_element_private (pad);

  GST_DEBUG_OBJECT (demux, "Have seek %" GST_PTR_FORMAT, event);

  gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur,
      &stop_type, &stop);
  seqnum = gst_event_get_seqnum (event);

  /* we can only seek on time */
  if (format != GST_FORMAT_TIME) {
    GST_DEBUG_OBJECT (demux, "Can only seek on TIME");
    return FALSE;
  }

  /* copy segment, we need this because we still need the old
   * segment when we close the current segment. */
  memcpy (&seeksegment, &demux->common.segment, sizeof (GstSegment));

  /* pull mode without index means that the actual duration is not known,
   * we might be playing a file that's still being recorded
   * so, invalidate our current duration, which is only a moving target,
   * and should not be used to clamp anything */
  if (!demux->streaming && !demux->common.index && demux->invalid_duration) {
    seeksegment.duration = GST_CLOCK_TIME_NONE;
  }

  GST_DEBUG_OBJECT (demux, "configuring seek");
  /* Subtract stream_start_time so we always seek on a segment
   * in stream time */
  if (GST_CLOCK_TIME_IS_VALID (demux->stream_start_time)) {
    seeksegment.start -= demux->stream_start_time;
    seeksegment.position -= demux->stream_start_time;
    if (GST_CLOCK_TIME_IS_VALID (seeksegment.stop))
      seeksegment.stop -= demux->stream_start_time;
    else
      seeksegment.stop = seeksegment.duration;
  }

  gst_segment_do_seek (&seeksegment, rate, format, flags,
      cur_type, cur, stop_type, stop, &update);

  /* Restore the clip timestamp offset */
  if (GST_CLOCK_TIME_IS_VALID (demux->stream_start_time)) {
    seeksegment.position += demux->stream_start_time;
    seeksegment.start += demux->stream_start_time;
    if (!GST_CLOCK_TIME_IS_VALID (seeksegment.stop))
      seeksegment.stop = seeksegment.duration;
    if (GST_CLOCK_TIME_IS_VALID (seeksegment.stop))
      seeksegment.stop += demux->stream_start_time;
  }

  /* restore segment duration (if any effect),
   * would be determined again when parsing, but anyway ... */
  seeksegment.duration = demux->common.segment.duration;

  flush = ! !(flags & GST_SEEK_FLAG_FLUSH);
  keyunit = ! !(flags & GST_SEEK_FLAG_KEY_UNIT);
  after = ! !(flags & GST_SEEK_FLAG_SNAP_AFTER);
  before = ! !(flags & GST_SEEK_FLAG_SNAP_BEFORE);
  accurate = ! !(flags & GST_SEEK_FLAG_ACCURATE);

  /* always do full update if flushing,
   * otherwise problems might arise downstream with missing keyframes etc */
  update = update || flush;

  GST_DEBUG_OBJECT (demux, "New segment %" GST_SEGMENT_FORMAT, &seeksegment);

  /* check sanity before we start flushing and all that */
  snap_next = after && !before;
  if (seeksegment.rate < 0)
    snap_dir = snap_next ? GST_SEARCH_MODE_BEFORE : GST_SEARCH_MODE_AFTER;
  else
    snap_dir = snap_next ? GST_SEARCH_MODE_AFTER : GST_SEARCH_MODE_BEFORE;

  GST_OBJECT_LOCK (demux);

  seekpos = seeksegment.position;
  if (accurate) {
    seekpos -= MIN (seeksegment.position, demux->audio_lead_in_ts);
  }

  track = gst_matroska_read_common_get_seek_track (&demux->common, track);
  if ((entry = gst_matroska_read_common_do_index_seek (&demux->common, track,
              seekpos, &demux->seek_index, &demux->seek_entry,
              snap_dir)) == NULL) {
    /* pull mode without index can scan later on */
    if (demux->streaming) {
      GST_DEBUG_OBJECT (demux, "No matching seek entry in index");
      GST_OBJECT_UNLOCK (demux);
      return FALSE;
    } else if (rate < 0.0) {
      /* FIXME: We should build an index during playback or when scanning
       * that can be used here. The reverse playback code requires seek_index
       * and seek_entry to be set!
       */
      GST_DEBUG_OBJECT (demux,
          "No matching seek entry in index, needed for reverse playback");
      GST_OBJECT_UNLOCK (demux);
      return FALSE;
    }
  }
  GST_DEBUG_OBJECT (demux, "Seek position looks sane");
  GST_OBJECT_UNLOCK (demux);

  if (!update) {
    /* only have to update some segment,
     * but also still have to honour flush and so on */
    GST_DEBUG_OBJECT (demux, "... no update");
    /* bad goto, bad ... */
    goto next;
  }

  if (demux->streaming)
    goto finish;

next:
  if (flush) {
    GstEvent *flush_event = gst_event_new_flush_start ();
    gst_event_set_seqnum (flush_event, seqnum);
    GST_DEBUG_OBJECT (demux, "Starting flush");
    gst_pad_push_event (demux->common.sinkpad, gst_event_ref (flush_event));
    gst_matroska_demux_send_event (demux, flush_event);
  } else {
    GST_DEBUG_OBJECT (demux, "Non-flushing seek, pausing task");
    gst_pad_pause_task (demux->common.sinkpad);
  }
  /* ouch */
  if (!update) {
    GST_PAD_STREAM_LOCK (demux->common.sinkpad);
    pad_locked = TRUE;
    goto exit;
  }

  /* now grab the stream lock so that streaming cannot continue, for
   * non flushing seeks when the element is in PAUSED this could block
   * forever. */
  GST_DEBUG_OBJECT (demux, "Waiting for streaming to stop");
  GST_PAD_STREAM_LOCK (demux->common.sinkpad);
  pad_locked = TRUE;

  /* pull mode without index can do some scanning */
  if (!demux->streaming && !entry) {
    GstEvent *flush_event;

    /* need to stop flushing upstream as we need it next */
    if (flush) {
      flush_event = gst_event_new_flush_stop (TRUE);
      gst_event_set_seqnum (flush_event, seqnum);
      gst_pad_push_event (demux->common.sinkpad, flush_event);
    }
    entry = gst_matroska_demux_search_pos (demux, seekpos);
    /* keep local copy */
    if (entry) {
      scan_entry = *entry;
      g_free (entry);
      entry = &scan_entry;
    } else {
      GST_DEBUG_OBJECT (demux, "Scan failed to find matching position");
      if (flush) {
        flush_event = gst_event_new_flush_stop (TRUE);
        gst_event_set_seqnum (flush_event, seqnum);
        gst_matroska_demux_send_event (demux, flush_event);
      }
      goto seek_error;
    }
  }

finish:
  if (keyunit && seeksegment.rate > 0) {
    GST_DEBUG_OBJECT (demux, "seek to key unit, adjusting segment start from %"
        GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (seeksegment.start), GST_TIME_ARGS (entry->time));
    seeksegment.start = MAX (entry->time, demux->stream_start_time);
    seeksegment.position = seeksegment.start;
    seeksegment.time = seeksegment.start - demux->stream_start_time;
  } else if (keyunit) {
    GST_DEBUG_OBJECT (demux, "seek to key unit, adjusting segment stop from %"
        GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (seeksegment.stop), GST_TIME_ARGS (entry->time));
    seeksegment.stop = MAX (entry->time, demux->stream_start_time);
    seeksegment.position = seeksegment.stop;
  }

  if (demux->streaming) {
    GST_OBJECT_LOCK (demux);
    /* track real position we should start at */
    GST_DEBUG_OBJECT (demux, "storing segment start");
    demux->requested_seek_time = seeksegment.position;
    demux->seek_offset = entry->pos + demux->common.ebml_segment_start;
    GST_OBJECT_UNLOCK (demux);
    /* need to seek to cluster start to pick up cluster time */
    /* upstream takes care of flushing and all that
     * ... and newsegment event handling takes care of the rest */
    return perform_seek_to_offset (demux, rate,
        entry->pos + demux->common.ebml_segment_start, seqnum, flags);
  }

exit:
  if (flush) {
    GstEvent *flush_event = gst_event_new_flush_stop (TRUE);
    gst_event_set_seqnum (flush_event, seqnum);
    GST_DEBUG_OBJECT (demux, "Stopping flush");
    gst_pad_push_event (demux->common.sinkpad, gst_event_ref (flush_event));
    gst_matroska_demux_send_event (demux, flush_event);
  }

  GST_OBJECT_LOCK (demux);
  /* now update the real segment info */
  GST_DEBUG_OBJECT (demux, "Committing new seek segment");
  memcpy (&demux->common.segment, &seeksegment, sizeof (GstSegment));
  GST_OBJECT_UNLOCK (demux);

  /* update some (segment) state */
  if (!gst_matroska_demux_move_to_entry (demux, entry, TRUE, update))
    goto seek_error;

  /* notify start of new segment */
  if (demux->common.segment.flags & GST_SEEK_FLAG_SEGMENT) {
    GstMessage *msg;

    msg = gst_message_new_segment_start (GST_OBJECT (demux),
        GST_FORMAT_TIME, demux->common.segment.start);
    gst_message_set_seqnum (msg, seqnum);
    gst_element_post_message (GST_ELEMENT (demux), msg);
  }

  GST_OBJECT_LOCK (demux);
  if (demux->new_segment)
    gst_event_unref (demux->new_segment);

  /* On port from 0.10, discarded !update (for segment.update) here, FIXME? */
  demux->new_segment = gst_event_new_segment (&demux->common.segment);
  gst_event_set_seqnum (demux->new_segment, seqnum);
  if (demux->common.segment.rate < 0 && demux->common.segment.stop == -1)
    demux->to_time = demux->common.segment.position;
  else
    demux->to_time = GST_CLOCK_TIME_NONE;
  demux->segment_seqnum = seqnum;
  GST_OBJECT_UNLOCK (demux);

  /* restart our task since it might have been stopped when we did the
   * flush. */
  gst_pad_start_task (demux->common.sinkpad,
      (GstTaskFunction) gst_matroska_demux_loop, demux->common.sinkpad, NULL);

  /* streaming can continue now */
  if (pad_locked) {
    GST_PAD_STREAM_UNLOCK (demux->common.sinkpad);
  }

  return TRUE;

seek_error:
  {
    if (pad_locked) {
      GST_PAD_STREAM_UNLOCK (demux->common.sinkpad);
    }
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL), ("Got a seek error"));
    return FALSE;
  }
}

/*
 * Handle whether we can perform the seek event or if we have to let the chain
 * function handle seeks to build the seek indexes first.
 */
static gboolean
gst_matroska_demux_handle_seek_push (GstMatroskaDemux * demux, GstPad * pad,
    GstEvent * event)
{
  GstSeekFlags flags;
  GstSeekType cur_type, stop_type;
  GstFormat format;
  gdouble rate;
  gint64 cur, stop;

  gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur,
      &stop_type, &stop);

  /* sanity checks */

  /* we can only seek on time */
  if (format != GST_FORMAT_TIME) {
    GST_DEBUG_OBJECT (demux, "Can only seek on TIME");
    return FALSE;
  }

  if (stop_type != GST_SEEK_TYPE_NONE && stop != GST_CLOCK_TIME_NONE) {
    GST_DEBUG_OBJECT (demux, "Seek end-time not supported in streaming mode");
    return FALSE;
  }

  if (!(flags & GST_SEEK_FLAG_FLUSH)) {
    GST_DEBUG_OBJECT (demux,
        "Non-flushing seek not supported in streaming mode");
    return FALSE;
  }

  if (flags & GST_SEEK_FLAG_SEGMENT) {
    GST_DEBUG_OBJECT (demux, "Segment seek not supported in streaming mode");
    return FALSE;
  }

  /* check for having parsed index already */
  if (!demux->common.index_parsed) {
    gboolean building_index;
    guint64 offset = 0;

    if (!demux->index_offset) {
      GST_DEBUG_OBJECT (demux, "no index (location); no seek in push mode");
      return FALSE;
    }

    GST_OBJECT_LOCK (demux);
    /* handle the seek event in the chain function */
    demux->common.state = GST_MATROSKA_READ_STATE_SEEK;
    /* no more seek can be issued until state reset to _DATA */

    /* copy the event */
    if (demux->seek_event)
      gst_event_unref (demux->seek_event);
    demux->seek_event = gst_event_ref (event);

    /* set the building_index flag so that only one thread can setup the
     * structures for index seeking. */
    building_index = demux->building_index;
    if (!building_index) {
      demux->building_index = TRUE;
      offset = demux->index_offset;
    }
    GST_OBJECT_UNLOCK (demux);

    if (!building_index) {
      /* seek to the first subindex or legacy index */
      GST_INFO_OBJECT (demux, "Seeking to Cues at %" G_GUINT64_FORMAT, offset);
      return perform_seek_to_offset (demux, rate, offset,
          gst_event_get_seqnum (event), GST_SEEK_FLAG_NONE);
    }

    /* well, we are handling it already */
    return TRUE;
  }

  /* delegate to tweaked regular seek */
  return gst_matroska_demux_handle_seek_event (demux, pad, event);
}

static gboolean
gst_matroska_demux_handle_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (parent);
  gboolean res = TRUE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      /* no seeking until we are (safely) ready */
      if (demux->common.state != GST_MATROSKA_READ_STATE_DATA) {
        GST_DEBUG_OBJECT (demux,
            "not ready for seeking yet, deferring seek event: %" GST_PTR_FORMAT,
            event);
        if (demux->deferred_seek_event)
          gst_event_unref (demux->deferred_seek_event);
        demux->deferred_seek_event = event;
        demux->deferred_seek_pad = pad;
        return TRUE;
      }

      {
        guint32 seqnum = gst_event_get_seqnum (event);
        if (seqnum == demux->segment_seqnum) {
          GST_LOG_OBJECT (pad,
              "Drop duplicated SEEK event seqnum %" G_GUINT32_FORMAT, seqnum);
          gst_event_unref (event);
          return TRUE;
        }
      }

      if (!demux->streaming)
        res = gst_matroska_demux_handle_seek_event (demux, pad, event);
      else
        res = gst_matroska_demux_handle_seek_push (demux, pad, event);
      gst_event_unref (event);
      break;

    case GST_EVENT_QOS:
    {
      GstMatroskaTrackContext *context = gst_pad_get_element_private (pad);
      if (context->type == GST_MATROSKA_TRACK_TYPE_VIDEO) {
        GstMatroskaTrackVideoContext *videocontext =
            (GstMatroskaTrackVideoContext *) context;
        gdouble proportion;
        GstClockTimeDiff diff;
        GstClockTime timestamp;

        gst_event_parse_qos (event, NULL, &proportion, &diff, &timestamp);

        GST_OBJECT_LOCK (demux);
        videocontext->earliest_time = timestamp + diff;
        GST_OBJECT_UNLOCK (demux);
      }
      res = TRUE;
      gst_event_unref (event);
      break;
    }

    case GST_EVENT_TOC_SELECT:
    {
      char *uid = NULL;
      GstTocEntry *entry = NULL;
      GstEvent *seek_event;
      gint64 start_pos;

      if (!demux->common.toc) {
        GST_DEBUG_OBJECT (demux, "no TOC to select");
        return FALSE;
      } else {
        gst_event_parse_toc_select (event, &uid);
        if (uid != NULL) {
          GST_OBJECT_LOCK (demux);
          entry = gst_toc_find_entry (demux->common.toc, uid);
          if (entry == NULL) {
            GST_OBJECT_UNLOCK (demux);
            GST_WARNING_OBJECT (demux, "no TOC entry with given UID: %s", uid);
            res = FALSE;
          } else {
            gst_toc_entry_get_start_stop_times (entry, &start_pos, NULL);
            GST_OBJECT_UNLOCK (demux);
            seek_event = gst_event_new_seek (1.0,
                GST_FORMAT_TIME,
                GST_SEEK_FLAG_FLUSH,
                GST_SEEK_TYPE_SET, start_pos, GST_SEEK_TYPE_SET, -1);
            res = gst_matroska_demux_handle_seek_event (demux, pad, seek_event);
            gst_event_unref (seek_event);
          }
          g_free (uid);
        } else {
          GST_WARNING_OBJECT (demux, "received empty TOC select event");
          res = FALSE;
        }
      }
      gst_event_unref (event);
      break;
    }

      /* events we don't need to handle */
    case GST_EVENT_NAVIGATION:
      gst_event_unref (event);
      res = FALSE;
      break;

    case GST_EVENT_LATENCY:
    default:
      res = gst_pad_push_event (demux->common.sinkpad, event);
      break;
  }

  return res;
}

static gboolean
gst_matroska_demux_handle_sink_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (parent);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_BITRATE:
    {
      if (G_UNLIKELY (demux->cached_length == G_MAXUINT64 ||
              demux->common.offset >= demux->cached_length)) {
        demux->cached_length =
            gst_matroska_read_common_get_length (&demux->common);
      }

      if (demux->cached_length < G_MAXUINT64
          && demux->common.segment.duration > 0) {
        /* TODO: better results based on ranges/index tables */
        guint bitrate =
            gst_util_uint64_scale (8 * demux->cached_length, GST_SECOND,
            demux->common.segment.duration);

        GST_LOG_OBJECT (demux, "bitrate query byte length: %" G_GUINT64_FORMAT
            " duration %" GST_TIME_FORMAT " resulting in a bitrate of %u",
            demux->cached_length,
            GST_TIME_ARGS (demux->common.segment.duration), bitrate);

        gst_query_set_bitrate (query, bitrate);
        res = TRUE;
      }
      break;
    }
    default:
      res = gst_pad_query_default (pad, (GstObject *) demux, query);
      break;
  }

  return res;
}

static GstFlowReturn
gst_matroska_demux_seek_to_previous_keyframe (GstMatroskaDemux * demux)
{
  GstFlowReturn ret = GST_FLOW_EOS;
  gboolean done = TRUE;
  gint i;

  g_return_val_if_fail (demux->seek_index, GST_FLOW_EOS);
  g_return_val_if_fail (demux->seek_entry < demux->seek_index->len,
      GST_FLOW_EOS);

  GST_DEBUG_OBJECT (demux, "locating previous keyframe");

  if (!demux->seek_entry) {
    GST_DEBUG_OBJECT (demux, "no earlier index entry");
    goto exit;
  }

  for (i = 0; i < demux->common.src->len; i++) {
    GstMatroskaTrackContext *stream = g_ptr_array_index (demux->common.src, i);

    GST_DEBUG_OBJECT (demux, "segment start %" GST_TIME_FORMAT
        ", stream %d at %" GST_TIME_FORMAT,
        GST_TIME_ARGS (demux->common.segment.start), stream->index,
        GST_TIME_ARGS (stream->from_time));
    if (GST_CLOCK_TIME_IS_VALID (stream->from_time)) {
      if (stream->from_time > demux->common.segment.start) {
        GST_DEBUG_OBJECT (demux, "stream %d not finished yet", stream->index);
        done = FALSE;
      }
    } else {
      /* nothing pushed for this stream;
       * likely seek entry did not start at keyframe, so all was skipped.
       * So we need an earlier entry */
      done = FALSE;
    }
  }

  if (!done) {
    GstMatroskaIndex *entry;

    entry = &g_array_index (demux->seek_index, GstMatroskaIndex,
        --demux->seek_entry);
    if (!gst_matroska_demux_move_to_entry (demux, entry, FALSE, TRUE))
      goto exit;

    ret = GST_FLOW_OK;
  }

exit:
  return ret;
}

static GstFlowReturn
gst_matroska_demux_parse_tracks (GstMatroskaDemux * demux, GstEbmlRead * ebml)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 id;

  DEBUG_ELEMENT_START (demux, ebml, "Tracks");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (demux, ebml, "Tracks", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* one track within the "all-tracks" header */
      case GST_MATROSKA_ID_TRACKENTRY:{
        GstMatroskaTrackContext *track;
        ret = gst_matroska_demux_parse_stream (demux, ebml, &track);
        if (track != NULL) {
          if (gst_matroska_read_common_tracknumber_unique (&demux->common,
                  track->num)) {
            gst_matroska_demux_add_stream (demux, track);
          } else {
            GST_ERROR_OBJECT (demux,
                "TrackNumber %" G_GUINT64_FORMAT " is not unique", track->num);
            ret = GST_FLOW_ERROR;
            gst_matroska_track_free (track);
            track = NULL;
          }
        }
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&demux->common, ebml,
            "Track", id);
        break;
    }
  }
  DEBUG_ELEMENT_STOP (demux, ebml, "Tracks", ret);

  demux->tracks_parsed = TRUE;
  GST_DEBUG_OBJECT (demux, "signaling no more pads");
  gst_element_no_more_pads (GST_ELEMENT (demux));

  return ret;
}

static GstFlowReturn
gst_matroska_demux_update_tracks (GstMatroskaDemux * demux, GstEbmlRead * ebml)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint num_tracks_found = 0;
  guint32 id;

  GST_INFO_OBJECT (demux, "Reparsing Tracks element");

  DEBUG_ELEMENT_START (demux, ebml, "Tracks");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (demux, ebml, "Tracks", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* one track within the "all-tracks" header */
      case GST_MATROSKA_ID_TRACKENTRY:{
        GstMatroskaTrackContext *new_track;
        gint old_track_index;
        GstMatroskaTrackContext *old_track;
        ret = gst_matroska_demux_parse_stream (demux, ebml, &new_track);
        if (new_track == NULL)
          break;
        num_tracks_found++;

        if (gst_matroska_read_common_tracknumber_unique (&demux->common,
                new_track->num)) {
          GST_ERROR_OBJECT (demux,
              "Unexpected new TrackNumber: %" G_GUINT64_FORMAT, new_track->num);
          goto track_mismatch_error;
        }

        old_track_index =
            gst_matroska_read_common_stream_from_num (&demux->common,
            new_track->num);
        g_assert (old_track_index != -1);
        old_track = g_ptr_array_index (demux->common.src, old_track_index);

        if (old_track->type != new_track->type) {
          GST_ERROR_OBJECT (demux,
              "Mismatch reparsing track %" G_GUINT64_FORMAT
              " on track type. Expected %d, found %d", new_track->num,
              old_track->type, new_track->type);
          goto track_mismatch_error;
        }

        if (g_strcmp0 (old_track->codec_id, new_track->codec_id) != 0) {
          GST_ERROR_OBJECT (demux,
              "Mismatch reparsing track %" G_GUINT64_FORMAT
              " on codec id. Expected '%s', found '%s'", new_track->num,
              old_track->codec_id, new_track->codec_id);
          goto track_mismatch_error;
        }

        /* The new track matches the old track. No problems on our side.
         * Let's make it replace the old track. */
        new_track->pad = old_track->pad;
        new_track->index = old_track->index;
        new_track->pos = old_track->pos;
        g_ptr_array_index (demux->common.src, old_track_index) = new_track;
        gst_pad_set_element_private (new_track->pad, new_track);

        if (!gst_caps_is_equal (old_track->caps, new_track->caps)) {
          gst_pad_set_caps (new_track->pad, new_track->caps);
        }
        gst_caps_replace (&old_track->caps, NULL);

        if (!gst_tag_list_is_equal (old_track->tags, new_track->tags)) {
          GST_DEBUG_OBJECT (old_track->pad, "Sending tags %p: %"
              GST_PTR_FORMAT, new_track->tags, new_track->tags);
          gst_pad_push_event (new_track->pad,
              gst_event_new_tag (gst_tag_list_copy (new_track->tags)));
        }

        gst_matroska_track_free (old_track);
        break;

      track_mismatch_error:
        gst_matroska_track_free (new_track);
        new_track = NULL;
        ret = GST_FLOW_ERROR;
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&demux->common, ebml,
            "Track", id);
        break;
    }
  }
  DEBUG_ELEMENT_STOP (demux, ebml, "Tracks", ret);

  if (ret != GST_FLOW_ERROR && demux->common.num_streams != num_tracks_found) {
    GST_ERROR_OBJECT (demux,
        "Mismatch on the number of tracks. Expected %du tracks, found %du",
        demux->common.num_streams, num_tracks_found);
    ret = GST_FLOW_ERROR;
  }

  return ret;
}

/*
 * Read signed/unsigned "EBML" numbers.
 * Return: number of bytes processed.
 */

static gint
gst_matroska_ebmlnum_uint (guint8 * data, guint size, guint64 * num)
{
  gint len_mask = 0x80, read = 1, n = 1, num_ffs = 0;
  guint64 total;

  if (size <= 0) {
    return -1;
  }

  total = data[0];
  while (read <= 8 && !(total & len_mask)) {
    read++;
    len_mask >>= 1;
  }
  if (read > 8)
    return -1;

  if ((total &= (len_mask - 1)) == len_mask - 1)
    num_ffs++;
  if (size < read)
    return -1;
  while (n < read) {
    if (data[n] == 0xff)
      num_ffs++;
    total = (total << 8) | data[n];
    n++;
  }

  if (read == num_ffs && total != 0)
    *num = G_MAXUINT64;
  else
    *num = total;

  return read;
}

static gint
gst_matroska_ebmlnum_sint (guint8 * data, guint size, gint64 * num)
{
  guint64 unum;
  gint res;

  /* read as unsigned number first */
  if ((res = gst_matroska_ebmlnum_uint (data, size, &unum)) < 0)
    return -1;

  /* make signed */
  if (unum == G_MAXUINT64)
    *num = G_MAXINT64;
  else
    *num = unum - ((1 << ((7 * res) - 1)) - 1);

  return res;
}

/*
 * Mostly used for subtitles. We add void filler data for each
 * lagging stream to make sure we don't deadlock.
 */

static void
gst_matroska_demux_sync_streams (GstMatroskaDemux * demux)
{
  GstClockTime gap_threshold;
  gint stream_nr;

  GST_OBJECT_LOCK (demux);

  GST_LOG_OBJECT (demux, "Sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (demux->common.segment.position));

  g_assert (demux->common.num_streams == demux->common.src->len);
  for (stream_nr = 0; stream_nr < demux->common.src->len; stream_nr++) {
    GstMatroskaTrackContext *context;

    context = g_ptr_array_index (demux->common.src, stream_nr);

    GST_LOG_OBJECT (demux,
        "Checking for resync on stream %d (%" GST_TIME_FORMAT ")", stream_nr,
        GST_TIME_ARGS (context->pos));

    /* Only send gap events on non-subtitle streams if lagging way behind.
     * The 0.5 second threshold for subtitle streams is also quite random. */
    if (context->type == GST_MATROSKA_TRACK_TYPE_SUBTITLE)
      gap_threshold = GST_SECOND / 2;
    else
      gap_threshold = 3 * GST_SECOND;

    /* Lag need only be considered if we have advanced into requested segment */
    if (GST_CLOCK_TIME_IS_VALID (context->pos) &&
        GST_CLOCK_TIME_IS_VALID (demux->common.segment.position) &&
        demux->common.segment.position > demux->common.segment.start &&
        context->pos + gap_threshold < demux->common.segment.position) {

      GstEvent *event;
      guint64 start = context->pos;
      guint64 stop = demux->common.segment.position - gap_threshold;

      GST_DEBUG_OBJECT (demux,
          "Synchronizing stream %d with other by advancing time from %"
          GST_TIME_FORMAT " to %" GST_TIME_FORMAT, stream_nr,
          GST_TIME_ARGS (start), GST_TIME_ARGS (stop));

      context->pos = stop;

      event = gst_event_new_gap (start, stop - start);
      GST_OBJECT_UNLOCK (demux);
      gst_pad_push_event (context->pad, event);
      GST_OBJECT_LOCK (demux);
    }
  }

  GST_OBJECT_UNLOCK (demux);
}

static GstFlowReturn
gst_matroska_demux_push_stream_headers (GstMatroskaDemux * demux,
    GstMatroskaTrackContext * stream)
{
  GstFlowReturn ret = GST_FLOW_OK;
  gint i, num;

  num = gst_buffer_list_length (stream->stream_headers);
  for (i = 0; i < num; ++i) {
    GstBuffer *buf;

    buf = gst_buffer_list_get (stream->stream_headers, i);
    buf = gst_buffer_copy (buf);

    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_HEADER);

    if (stream->set_discont) {
      GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
      stream->set_discont = FALSE;
    } else {
      GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DISCONT);
    }

    /* push out all headers in one go and use last flow return */
    ret = gst_pad_push (stream->pad, buf);
  }

  /* don't need these any  longer */
  gst_buffer_list_unref (stream->stream_headers);
  stream->stream_headers = NULL;

  /* combine flows */
  ret = gst_flow_combiner_update_flow (demux->flowcombiner, ret);

  return ret;
}

static void
gst_matroska_demux_push_dvd_clut_change_event (GstMatroskaDemux * demux,
    GstMatroskaTrackContext * stream)
{
  gchar *buf, *start;

  g_assert (!strcmp (stream->codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_VOBSUB));

  if (!stream->codec_priv)
    return;

  /* ideally, VobSub private data should be parsed and stored more convenient
   * elsewhere, but for now, only interested in a small part */

  /* make sure we have terminating 0 */
  buf = g_strndup (stream->codec_priv, stream->codec_priv_size);

  /* just locate and parse palette part */
  start = strstr (buf, "palette:");
  if (start) {
    gint i;
    guint32 clut[16];
    guint32 col;
    guint8 r, g, b, y, u, v;

    start += 8;
    while (g_ascii_isspace (*start))
      start++;
    for (i = 0; i < 16; i++) {
      if (sscanf (start, "%06x", &col) != 1)
        break;
      start += 6;
      while ((*start == ',') || g_ascii_isspace (*start))
        start++;
      /* sigh, need to convert this from vobsub pseudo-RGB to YUV */
      r = (col >> 16) & 0xff;
      g = (col >> 8) & 0xff;
      b = col & 0xff;
      y = CLAMP ((0.1494 * r + 0.6061 * g + 0.2445 * b) * 219 / 255 + 16, 0,
          255);
      u = CLAMP (0.6066 * r - 0.4322 * g - 0.1744 * b + 128, 0, 255);
      v = CLAMP (-0.08435 * r - 0.3422 * g + 0.4266 * b + 128, 0, 255);
      clut[i] = (y << 16) | (u << 8) | v;
    }

    /* got them all without problems; build and send event */
    if (i == 16) {
      GstStructure *s;

      s = gst_structure_new ("application/x-gst-dvd", "event", G_TYPE_STRING,
          "dvd-spu-clut-change", "clut00", G_TYPE_INT, clut[0], "clut01",
          G_TYPE_INT, clut[1], "clut02", G_TYPE_INT, clut[2], "clut03",
          G_TYPE_INT, clut[3], "clut04", G_TYPE_INT, clut[4], "clut05",
          G_TYPE_INT, clut[5], "clut06", G_TYPE_INT, clut[6], "clut07",
          G_TYPE_INT, clut[7], "clut08", G_TYPE_INT, clut[8], "clut09",
          G_TYPE_INT, clut[9], "clut10", G_TYPE_INT, clut[10], "clut11",
          G_TYPE_INT, clut[11], "clut12", G_TYPE_INT, clut[12], "clut13",
          G_TYPE_INT, clut[13], "clut14", G_TYPE_INT, clut[14], "clut15",
          G_TYPE_INT, clut[15], NULL);

      gst_pad_push_event (stream->pad,
          gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM_STICKY, s));
    }
  }
  g_free (buf);
}

static void
gst_matroska_demux_push_codec_data_all (GstMatroskaDemux * demux)
{
  gint stream_nr;

  g_assert (demux->common.num_streams == demux->common.src->len);
  for (stream_nr = 0; stream_nr < demux->common.src->len; stream_nr++) {
    GstMatroskaTrackContext *stream;

    stream = g_ptr_array_index (demux->common.src, stream_nr);

    if (stream->send_stream_headers) {
      if (stream->stream_headers != NULL) {
        gst_matroska_demux_push_stream_headers (demux, stream);
      } else {
        /* FIXME: perhaps we can just disable and skip this stream then */
        GST_ELEMENT_ERROR (demux, STREAM, DECODE, (NULL),
            ("Failed to extract stream headers from codec private data"));
      }
      stream->send_stream_headers = FALSE;
    }

    if (stream->send_dvd_event) {
      gst_matroska_demux_push_dvd_clut_change_event (demux, stream);
      /* FIXME: should we send this event again after (flushing) seek ? */
      stream->send_dvd_event = FALSE;
    }
  }

}

static GstFlowReturn
gst_matroska_demux_add_mpeg_seq_header (GstElement * element,
    GstMatroskaTrackContext * stream, GstBuffer ** buf)
{
  guint8 *seq_header;
  guint seq_header_len;
  guint32 header, tmp;

  if (stream->codec_state) {
    seq_header = stream->codec_state;
    seq_header_len = stream->codec_state_size;
  } else if (stream->codec_priv) {
    seq_header = stream->codec_priv;
    seq_header_len = stream->codec_priv_size;
  } else {
    return GST_FLOW_OK;
  }

  /* Sequence header only needed for keyframes */
  if (GST_BUFFER_FLAG_IS_SET (*buf, GST_BUFFER_FLAG_DELTA_UNIT))
    return GST_FLOW_OK;

  if (gst_buffer_get_size (*buf) < 4)
    return GST_FLOW_OK;

  gst_buffer_extract (*buf, 0, &tmp, sizeof (guint32));
  header = GUINT32_FROM_BE (tmp);

  /* Sequence start code, if not found prepend */
  if (header != 0x000001b3) {
    GstBuffer *newbuf;

    GST_DEBUG_OBJECT (element, "Prepending MPEG sequence header");

    newbuf = gst_buffer_new_wrapped (g_memdup (seq_header, seq_header_len),
        seq_header_len);

    gst_buffer_copy_into (newbuf, *buf, GST_BUFFER_COPY_TIMESTAMPS |
        GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_MEMORY, 0,
        gst_buffer_get_size (*buf));

    gst_buffer_unref (*buf);
    *buf = newbuf;
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_matroska_demux_add_wvpk_header (GstElement * element,
    GstMatroskaTrackContext * stream, GstBuffer ** buf)
{
  GstMatroskaTrackAudioContext *audiocontext =
      (GstMatroskaTrackAudioContext *) stream;
  GstBuffer *newbuf = NULL;
  GstMapInfo map, outmap;
  guint8 *buf_data, *data;
  Wavpack4Header wvh;

  wvh.ck_id[0] = 'w';
  wvh.ck_id[1] = 'v';
  wvh.ck_id[2] = 'p';
  wvh.ck_id[3] = 'k';

  wvh.version = GST_READ_UINT16_LE (stream->codec_priv);
  wvh.track_no = 0;
  wvh.index_no = 0;
  wvh.total_samples = -1;
  wvh.block_index = audiocontext->wvpk_block_index;

  if (audiocontext->channels <= 2) {
    guint32 block_samples, tmp;
    gsize size = gst_buffer_get_size (*buf);

    gst_buffer_extract (*buf, 0, &tmp, sizeof (guint32));
    block_samples = GUINT32_FROM_LE (tmp);
    /* we need to reconstruct the header of the wavpack block */

    /* -20 because ck_size is the size of the wavpack block -8
     * and lace_size is the size of the wavpack block + 12
     * (the three guint32 of the header that already are in the buffer) */
    wvh.ck_size = size + sizeof (Wavpack4Header) - 20;

    /* block_samples, flags and crc are already in the buffer */
    newbuf = gst_buffer_new_allocate (NULL, sizeof (Wavpack4Header) - 12, NULL);

    gst_buffer_map (newbuf, &outmap, GST_MAP_WRITE);
    data = outmap.data;
    data[0] = 'w';
    data[1] = 'v';
    data[2] = 'p';
    data[3] = 'k';
    GST_WRITE_UINT32_LE (data + 4, wvh.ck_size);
    GST_WRITE_UINT16_LE (data + 8, wvh.version);
    GST_WRITE_UINT8 (data + 10, wvh.track_no);
    GST_WRITE_UINT8 (data + 11, wvh.index_no);
    GST_WRITE_UINT32_LE (data + 12, wvh.total_samples);
    GST_WRITE_UINT32_LE (data + 16, wvh.block_index);
    gst_buffer_unmap (newbuf, &outmap);

    /* Append data from buf: */
    gst_buffer_copy_into (newbuf, *buf, GST_BUFFER_COPY_TIMESTAMPS |
        GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_MEMORY, 0, size);

    gst_buffer_unref (*buf);
    *buf = newbuf;
    audiocontext->wvpk_block_index += block_samples;
  } else {
    guint8 *outdata = NULL;
    guint outpos = 0;
    gsize buf_size, size, out_size = 0;
    guint32 block_samples, flags, crc, blocksize;

    gst_buffer_map (*buf, &map, GST_MAP_READ);
    buf_data = map.data;
    buf_size = map.size;

    if (buf_size < 4) {
      GST_ERROR_OBJECT (element, "Too small wavpack buffer");
      gst_buffer_unmap (*buf, &map);
      return GST_FLOW_ERROR;
    }

    data = buf_data;
    size = buf_size;

    block_samples = GST_READ_UINT32_LE (data);
    data += 4;
    size -= 4;

    while (size > 12) {
      flags = GST_READ_UINT32_LE (data);
      data += 4;
      size -= 4;
      crc = GST_READ_UINT32_LE (data);
      data += 4;
      size -= 4;
      blocksize = GST_READ_UINT32_LE (data);
      data += 4;
      size -= 4;

      if (blocksize == 0 || size < blocksize)
        break;

      g_assert ((newbuf == NULL) == (outdata == NULL));

      if (newbuf == NULL) {
        out_size = sizeof (Wavpack4Header) + blocksize;
        newbuf = gst_buffer_new_allocate (NULL, out_size, NULL);

        gst_buffer_copy_into (newbuf, *buf,
            GST_BUFFER_COPY_TIMESTAMPS | GST_BUFFER_COPY_FLAGS, 0, -1);

        outpos = 0;
        gst_buffer_map (newbuf, &outmap, GST_MAP_WRITE);
        outdata = outmap.data;
      } else {
        gst_buffer_unmap (newbuf, &outmap);
        out_size += sizeof (Wavpack4Header) + blocksize;
        gst_buffer_set_size (newbuf, out_size);
        gst_buffer_map (newbuf, &outmap, GST_MAP_WRITE);
        outdata = outmap.data;
      }

      outdata[outpos] = 'w';
      outdata[outpos + 1] = 'v';
      outdata[outpos + 2] = 'p';
      outdata[outpos + 3] = 'k';
      outpos += 4;

      GST_WRITE_UINT32_LE (outdata + outpos,
          blocksize + sizeof (Wavpack4Header) - 8);
      GST_WRITE_UINT16_LE (outdata + outpos + 4, wvh.version);
      GST_WRITE_UINT8 (outdata + outpos + 6, wvh.track_no);
      GST_WRITE_UINT8 (outdata + outpos + 7, wvh.index_no);
      GST_WRITE_UINT32_LE (outdata + outpos + 8, wvh.total_samples);
      GST_WRITE_UINT32_LE (outdata + outpos + 12, wvh.block_index);
      GST_WRITE_UINT32_LE (outdata + outpos + 16, block_samples);
      GST_WRITE_UINT32_LE (outdata + outpos + 20, flags);
      GST_WRITE_UINT32_LE (outdata + outpos + 24, crc);
      outpos += 28;

      memmove (outdata + outpos, data, blocksize);
      outpos += blocksize;
      data += blocksize;
      size -= blocksize;
    }
    gst_buffer_unmap (*buf, &map);
    gst_buffer_unref (*buf);

    if (newbuf)
      gst_buffer_unmap (newbuf, &outmap);

    *buf = newbuf;
    audiocontext->wvpk_block_index += block_samples;
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_matroska_demux_add_prores_header (GstElement * element,
    GstMatroskaTrackContext * stream, GstBuffer ** buf)
{
  GstBuffer *newbuf = gst_buffer_new_allocate (NULL, 8, NULL);
  GstMapInfo map;
  guint32 frame_size;

  if (!gst_buffer_map (newbuf, &map, GST_MAP_WRITE)) {
    GST_ERROR ("Failed to map newly allocated buffer");
    return GST_FLOW_ERROR;
  }

  frame_size = gst_buffer_get_size (*buf);

  GST_WRITE_UINT32_BE (map.data, frame_size);
  map.data[4] = 'i';
  map.data[5] = 'c';
  map.data[6] = 'p';
  map.data[7] = 'f';

  gst_buffer_unmap (newbuf, &map);
  *buf = gst_buffer_append (newbuf, *buf);

  return GST_FLOW_OK;
}

/* @text must be null-terminated */
static gboolean
gst_matroska_demux_subtitle_chunk_has_tag (GstElement * element,
    const gchar * text)
{
  gchar *tag;

  g_return_val_if_fail (text != NULL, FALSE);

  /* yes, this might all lead to false positives ... */
  tag = (gchar *) text;
  while ((tag = strchr (tag, '<'))) {
    tag++;
    if (*tag != '\0' && *(tag + 1) == '>') {
      /* some common convenience ones */
      /* maybe any character will do here ? */
      switch (*tag) {
        case 'b':
        case 'i':
        case 'u':
        case 's':
          return TRUE;
        default:
          return FALSE;
      }
    }
  }

  if (strstr (text, "<span"))
    return TRUE;

  return FALSE;
}

static GstFlowReturn
gst_matroska_demux_check_subtitle_buffer (GstElement * element,
    GstMatroskaTrackContext * stream, GstBuffer ** buf)
{
  GstMatroskaTrackSubtitleContext *sub_stream;
  const gchar *encoding;
  GError *err = NULL;
  GstBuffer *newbuf;
  gchar *utf8;
  GstMapInfo map;
  gboolean needs_unmap = TRUE;

  sub_stream = (GstMatroskaTrackSubtitleContext *) stream;

  if (!gst_buffer_get_size (*buf) || !gst_buffer_map (*buf, &map, GST_MAP_READ))
    return GST_FLOW_OK;

  /* The subtitle buffer we push out should not include a NUL terminator as
   * part of the data. */
  if (map.data[map.size - 1] == '\0') {
    gst_buffer_set_size (*buf, map.size - 1);
    gst_buffer_unmap (*buf, &map);
    gst_buffer_map (*buf, &map, GST_MAP_READ);
  }

  if (!sub_stream->invalid_utf8) {
    if (g_utf8_validate ((gchar *) map.data, map.size, NULL)) {
      goto next;
    }
    GST_WARNING_OBJECT (element, "subtitle stream %" G_GUINT64_FORMAT
        " is not valid UTF-8, this is broken according to the matroska"
        " specification", stream->num);
    sub_stream->invalid_utf8 = TRUE;
  }

  /* file with broken non-UTF8 subtitle, do the best we can do to fix it */
  encoding = g_getenv ("GST_SUBTITLE_ENCODING");
  if (encoding == NULL || *encoding == '\0') {
    /* if local encoding is UTF-8 and no encoding specified
     * via the environment variable, assume ISO-8859-15 */
    if (g_get_charset (&encoding)) {
      encoding = "ISO-8859-15";
    }
  }

  utf8 =
      g_convert_with_fallback ((gchar *) map.data, map.size, "UTF-8", encoding,
      (char *) "*", NULL, NULL, &err);

  if (err) {
    GST_LOG_OBJECT (element, "could not convert string from '%s' to UTF-8: %s",
        encoding, err->message);
    g_error_free (err);
    g_free (utf8);

    /* invalid input encoding, fall back to ISO-8859-15 (always succeeds) */
    encoding = "ISO-8859-15";
    utf8 =
        g_convert_with_fallback ((gchar *) map.data, map.size, "UTF-8",
        encoding, (char *) "*", NULL, NULL, NULL);
  }

  GST_LOG_OBJECT (element, "converted subtitle text from %s to UTF-8 %s",
      encoding, (err) ? "(using ISO-8859-15 as fallback)" : "");

  if (utf8 == NULL)
    utf8 = g_strdup ("invalid subtitle");

  newbuf = gst_buffer_new_wrapped (utf8, strlen (utf8));
  gst_buffer_unmap (*buf, &map);
  gst_buffer_copy_into (newbuf, *buf,
      GST_BUFFER_COPY_TIMESTAMPS | GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_META,
      0, -1);
  gst_buffer_unref (*buf);

  *buf = newbuf;
  gst_buffer_map (*buf, &map, GST_MAP_READ);

next:

  if (sub_stream->check_markup) {
    /* caps claim markup text, so we need to escape text,
     * except if text is already markup and then needs no further escaping */
    sub_stream->seen_markup_tag = sub_stream->seen_markup_tag ||
        gst_matroska_demux_subtitle_chunk_has_tag (element, (gchar *) map.data);

    if (!sub_stream->seen_markup_tag) {
      utf8 = g_markup_escape_text ((gchar *) map.data, map.size);

      newbuf = gst_buffer_new_wrapped (utf8, strlen (utf8));
      gst_buffer_unmap (*buf, &map);
      gst_buffer_copy_into (newbuf, *buf,
          GST_BUFFER_COPY_TIMESTAMPS | GST_BUFFER_COPY_FLAGS |
          GST_BUFFER_COPY_META, 0, -1);
      gst_buffer_unref (*buf);

      *buf = newbuf;
      needs_unmap = FALSE;
    }
  }

  if (needs_unmap)
    gst_buffer_unmap (*buf, &map);

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_matroska_demux_check_aac (GstElement * element,
    GstMatroskaTrackContext * stream, GstBuffer ** buf)
{
  guint8 data[2];
  guint size;

  gst_buffer_extract (*buf, 0, data, 2);
  size = gst_buffer_get_size (*buf);

  if (size > 2 && data[0] == 0xff && (data[1] >> 4 == 0x0f)) {
    GstStructure *s;

    /* tss, ADTS data, remove codec_data
     * still assume it is at least parsed */
    stream->caps = gst_caps_make_writable (stream->caps);
    s = gst_caps_get_structure (stream->caps, 0);
    g_assert (s);
    gst_structure_remove_field (s, "codec_data");
    gst_pad_set_caps (stream->pad, stream->caps);
    GST_DEBUG_OBJECT (element, "ADTS AAC audio data; removing codec-data, "
        "new caps: %" GST_PTR_FORMAT, stream->caps);
  }

  /* disable subsequent checking */
  stream->postprocess_frame = NULL;

  return GST_FLOW_OK;
}

static GstBuffer *
gst_matroska_demux_align_buffer (GstMatroskaDemux * demux,
    GstBuffer * buffer, gsize alignment)
{
  GstMapInfo map;

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  if (map.size < sizeof (guintptr)) {
    gst_buffer_unmap (buffer, &map);
    return buffer;
  }

  if (((guintptr) map.data) & (alignment - 1)) {
    GstBuffer *new_buffer;
    GstAllocationParams params = { 0, alignment - 1, 0, 0, };

    new_buffer = gst_buffer_new_allocate (NULL,
        gst_buffer_get_size (buffer), &params);

    /* Copy data "by hand", so ensure alignment is kept: */
    gst_buffer_fill (new_buffer, 0, map.data, map.size);

    gst_buffer_copy_into (new_buffer, buffer, GST_BUFFER_COPY_METADATA, 0, -1);
    GST_DEBUG_OBJECT (demux,
        "We want output aligned on %" G_GSIZE_FORMAT ", reallocated",
        alignment);

    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);

    return new_buffer;
  }

  gst_buffer_unmap (buffer, &map);
  return buffer;
}

static GstFlowReturn
gst_matroska_demux_parse_blockgroup_or_simpleblock (GstMatroskaDemux * demux,
    GstEbmlRead * ebml, guint64 cluster_time, guint64 cluster_offset,
    gboolean is_simpleblock)
{
  GstMatroskaTrackContext *stream = NULL;
  GstFlowReturn ret = GST_FLOW_OK;
  gboolean readblock = FALSE;
  guint32 id;
  guint64 block_duration = -1;
  gint64 block_discardpadding = 0;
  GstBuffer *buf = NULL;
  GstMapInfo map;
  gint stream_num = -1, n, laces = 0;
  guint size = 0;
  gint *lace_size = NULL;
  gint64 time = 0;
  gint flags = 0;
  gint64 referenceblock = 0;
  gint64 offset;
  GstClockTime buffer_timestamp;

  offset = gst_ebml_read_get_offset (ebml);

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if (!is_simpleblock) {
      if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK) {
        goto data_error;
      }
    } else {
      id = GST_MATROSKA_ID_SIMPLEBLOCK;
    }

    switch (id) {
        /* one block inside the group. Note, block parsing is one
         * of the harder things, so this code is a bit complicated.
         * See http://www.matroska.org/ for documentation. */
      case GST_MATROSKA_ID_SIMPLEBLOCK:
      case GST_MATROSKA_ID_BLOCK:
      {
        guint64 num;
        guint8 *data;

        if (buf) {
          gst_buffer_unmap (buf, &map);
          gst_buffer_unref (buf);
          buf = NULL;
        }
        if ((ret = gst_ebml_read_buffer (ebml, &id, &buf)) != GST_FLOW_OK)
          break;

        gst_buffer_map (buf, &map, GST_MAP_READ);
        data = map.data;
        size = map.size;

        /* first byte(s): blocknum */
        if ((n = gst_matroska_ebmlnum_uint (data, size, &num)) < 0)
          goto data_error;
        data += n;
        size -= n;

        /* fetch stream from num */
        stream_num = gst_matroska_read_common_stream_from_num (&demux->common,
            num);
        if (G_UNLIKELY (size < 3)) {
          GST_WARNING_OBJECT (demux, "Invalid size %u", size);
          /* non-fatal, try next block(group) */
          ret = GST_FLOW_OK;
          goto done;
        } else if (G_UNLIKELY (stream_num < 0 ||
                stream_num >= demux->common.num_streams)) {
          /* let's not give up on a stray invalid track number */
          GST_WARNING_OBJECT (demux,
              "Invalid stream %d for track number %" G_GUINT64_FORMAT
              "; ignoring block", stream_num, num);
          goto done;
        }

        stream = g_ptr_array_index (demux->common.src, stream_num);

        /* time (relative to cluster time) */
        time = ((gint16) GST_READ_UINT16_BE (data));
        data += 2;
        size -= 2;
        flags = GST_READ_UINT8 (data);
        data += 1;
        size -= 1;

        GST_LOG_OBJECT (demux, "time %" G_GUINT64_FORMAT ", flags %d", time,
            flags);

        switch ((flags & 0x06) >> 1) {
          case 0x0:            /* no lacing */
            laces = 1;
            lace_size = g_new (gint, 1);
            lace_size[0] = size;
            break;

          case 0x1:            /* xiph lacing */
          case 0x2:            /* fixed-size lacing */
          case 0x3:            /* EBML lacing */
            if (size == 0)
              goto invalid_lacing;
            laces = GST_READ_UINT8 (data) + 1;
            data += 1;
            size -= 1;
            lace_size = g_new0 (gint, laces);

            switch ((flags & 0x06) >> 1) {
              case 0x1:        /* xiph lacing */  {
                guint temp, total = 0;

                for (n = 0; ret == GST_FLOW_OK && n < laces - 1; n++) {
                  while (1) {
                    if (size == 0)
                      goto invalid_lacing;
                    temp = GST_READ_UINT8 (data);
                    lace_size[n] += temp;
                    data += 1;
                    size -= 1;
                    if (temp != 0xff)
                      break;
                  }
                  total += lace_size[n];
                }
                lace_size[n] = size - total;
                break;
              }

              case 0x2:        /* fixed-size lacing */
                for (n = 0; n < laces; n++)
                  lace_size[n] = size / laces;
                break;

              case 0x3:        /* EBML lacing */  {
                guint total;

                if ((n = gst_matroska_ebmlnum_uint (data, size, &num)) < 0)
                  goto data_error;
                data += n;
                size -= n;
                total = lace_size[0] = num;
                for (n = 1; ret == GST_FLOW_OK && n < laces - 1; n++) {
                  gint64 snum;
                  gint r;

                  if ((r = gst_matroska_ebmlnum_sint (data, size, &snum)) < 0)
                    goto data_error;
                  data += r;
                  size -= r;
                  lace_size[n] = lace_size[n - 1] + snum;
                  total += lace_size[n];
                }
                if (n < laces)
                  lace_size[n] = size - total;
                break;
              }
            }
            break;
        }

        if (ret != GST_FLOW_OK)
          break;

        readblock = TRUE;
        break;
      }

      case GST_MATROSKA_ID_BLOCKDURATION:{
        ret = gst_ebml_read_uint (ebml, &id, &block_duration);
        GST_DEBUG_OBJECT (demux, "BlockDuration: %" G_GUINT64_FORMAT,
            block_duration);
        break;
      }

      case GST_MATROSKA_ID_DISCARDPADDING:{
        ret = gst_ebml_read_sint (ebml, &id, &block_discardpadding);
        GST_DEBUG_OBJECT (demux, "DiscardPadding: %" GST_STIME_FORMAT,
            GST_STIME_ARGS (block_discardpadding));
        break;
      }

      case GST_MATROSKA_ID_REFERENCEBLOCK:{
        ret = gst_ebml_read_sint (ebml, &id, &referenceblock);
        GST_DEBUG_OBJECT (demux, "ReferenceBlock: %" G_GINT64_FORMAT,
            referenceblock);
        break;
      }

      case GST_MATROSKA_ID_CODECSTATE:{
        guint8 *data;
        guint64 data_len = 0;

        if ((ret =
                gst_ebml_read_binary (ebml, &id, &data,
                    &data_len)) != GST_FLOW_OK)
          break;

        if (G_UNLIKELY (stream == NULL)) {
          GST_WARNING_OBJECT (demux,
              "Unexpected CodecState subelement - ignoring");
          break;
        }

        g_free (stream->codec_state);
        stream->codec_state = data;
        stream->codec_state_size = data_len;

        /* Decode if necessary */
        if (stream->encodings && stream->encodings->len > 0
            && stream->codec_state && stream->codec_state_size > 0) {
          if (!gst_matroska_decode_data (stream->encodings,
                  &stream->codec_state, &stream->codec_state_size,
                  GST_MATROSKA_TRACK_ENCODING_SCOPE_CODEC_DATA, TRUE)) {
            GST_WARNING_OBJECT (demux, "Decoding codec state failed");
          }
        }

        GST_DEBUG_OBJECT (demux, "CodecState of %" G_GSIZE_FORMAT " bytes",
            stream->codec_state_size);
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&demux->common, ebml,
            "BlockGroup", id);
        break;

      case GST_MATROSKA_ID_BLOCKVIRTUAL:
      case GST_MATROSKA_ID_BLOCKADDITIONS:
      case GST_MATROSKA_ID_REFERENCEPRIORITY:
      case GST_MATROSKA_ID_REFERENCEVIRTUAL:
      case GST_MATROSKA_ID_SLICES:
        GST_DEBUG_OBJECT (demux,
            "Skipping BlockGroup subelement 0x%x - ignoring", id);
        ret = gst_ebml_read_skip (ebml);
        break;
    }

    if (is_simpleblock)
      break;
  }

  /* reading a number or so could have failed */
  if (ret != GST_FLOW_OK)
    goto data_error;

  if (ret == GST_FLOW_OK && readblock) {
    gboolean invisible_frame = FALSE;
    gboolean delta_unit = FALSE;
    guint64 duration = 0;
    gint64 lace_time = 0;
    GstEvent *protect_event;

    stream = g_ptr_array_index (demux->common.src, stream_num);

    if (cluster_time != GST_CLOCK_TIME_NONE) {
      /* FIXME: What to do with negative timestamps? Give timestamp 0 or -1?
       * Drop unless the lace contains timestamp 0? */
      if (time < 0 && (-time) > cluster_time) {
        lace_time = 0;
      } else {
        if (stream->timecodescale == 1.0)
          lace_time = (cluster_time + time) * demux->common.time_scale;
        else
          lace_time =
              gst_util_guint64_to_gdouble ((cluster_time + time) *
              demux->common.time_scale) * stream->timecodescale;
      }
    } else {
      lace_time = GST_CLOCK_TIME_NONE;
    }
    /* Send the GST_PROTECTION event */
    while ((protect_event = g_queue_pop_head (&stream->protection_event_queue))) {
      GST_TRACE_OBJECT (demux, "pushing protection event for stream %d:%s",
          stream->index, GST_STR_NULL (stream->name));
      gst_pad_push_event (stream->pad, protect_event);
    }

    /* need to refresh segment info ASAP */
    if (GST_CLOCK_TIME_IS_VALID (lace_time) && demux->need_segment) {
      GstSegment *segment = &demux->common.segment;
      guint64 clace_time;
      GstEvent *segment_event;

      if (!GST_CLOCK_TIME_IS_VALID (demux->stream_start_time)) {
        demux->stream_start_time = lace_time;
        GST_DEBUG_OBJECT (demux,
            "Setting stream start time to %" GST_TIME_FORMAT,
            GST_TIME_ARGS (lace_time));
      }
      clace_time = MAX (lace_time, demux->stream_start_time);
      if (GST_CLOCK_TIME_IS_VALID (demux->common.segment.position) &&
          demux->common.segment.position != 0) {
        GST_DEBUG_OBJECT (demux,
            "using stored seek position %" GST_TIME_FORMAT,
            GST_TIME_ARGS (demux->common.segment.position));
        clace_time = demux->common.segment.position;
        segment->position = GST_CLOCK_TIME_NONE;
      }
      segment->start = clace_time;
      segment->stop = GST_CLOCK_TIME_NONE;
      segment->time = segment->start - demux->stream_start_time;
      segment->position = segment->start - demux->stream_start_time;
      GST_DEBUG_OBJECT (demux,
          "generated segment starting at %" GST_TIME_FORMAT ": %"
          GST_SEGMENT_FORMAT, GST_TIME_ARGS (lace_time), segment);
      /* now convey our segment notion downstream */
      segment_event = gst_event_new_segment (segment);
      if (demux->segment_seqnum)
        gst_event_set_seqnum (segment_event, demux->segment_seqnum);
      gst_matroska_demux_send_event (demux, segment_event);
      demux->need_segment = FALSE;
      demux->segment_seqnum = 0;
    }

    /* send pending codec data headers for all streams,
     * before we perform sync across all streams */
    gst_matroska_demux_push_codec_data_all (demux);

    if (block_duration != -1) {
      if (stream->timecodescale == 1.0)
        duration = gst_util_uint64_scale (block_duration,
            demux->common.time_scale, 1);
      else
        duration =
            gst_util_gdouble_to_guint64 (gst_util_guint64_to_gdouble
            (gst_util_uint64_scale (block_duration, demux->common.time_scale,
                    1)) * stream->timecodescale);
    } else if (stream->default_duration) {
      duration = stream->default_duration * laces;
    }
    /* else duration is diff between timecode of this and next block */

    if (stream->type == GST_MATROSKA_TRACK_TYPE_VIDEO) {
      /* For SimpleBlock, look at the keyframe bit in flags. Otherwise,
         a ReferenceBlock implies that this is not a keyframe. In either
         case, it only makes sense for video streams. */
      if ((is_simpleblock && !(flags & 0x80)) || referenceblock) {
        delta_unit = TRUE;
        invisible_frame = ((flags & 0x08)) &&
            (!strcmp (stream->codec_id, GST_MATROSKA_CODEC_ID_VIDEO_VP8) ||
            !strcmp (stream->codec_id, GST_MATROSKA_CODEC_ID_VIDEO_VP9) ||
            !strcmp (stream->codec_id, GST_MATROSKA_CODEC_ID_VIDEO_AV1));
      }

      /* If we're doing a keyframe-only trickmode, only push keyframes on video
       * streams */
      if (delta_unit
          && demux->common.segment.
          flags & GST_SEGMENT_FLAG_TRICKMODE_KEY_UNITS) {
        GST_LOG_OBJECT (demux, "Skipping non-keyframe on stream %d",
            stream->index);
        ret = GST_FLOW_OK;
        goto done;
      }
    }

    for (n = 0; n < laces; n++) {
      GstBuffer *sub;

      if (G_UNLIKELY (lace_size[n] > size)) {
        GST_WARNING_OBJECT (demux, "Invalid lace size");
        break;
      }

      /* QoS for video track with an index. the assumption is that
         index entries point to keyframes, but if that is not true we
         will instead skip until the next keyframe. */
      if (GST_CLOCK_TIME_IS_VALID (lace_time) &&
          stream->type == GST_MATROSKA_TRACK_TYPE_VIDEO &&
          stream->index_table && demux->common.segment.rate > 0.0) {
        GstMatroskaTrackVideoContext *videocontext =
            (GstMatroskaTrackVideoContext *) stream;
        GstClockTime earliest_time;
        GstClockTime earliest_stream_time;

        GST_OBJECT_LOCK (demux);
        earliest_time = videocontext->earliest_time;
        GST_OBJECT_UNLOCK (demux);
        earliest_stream_time =
            gst_segment_position_from_running_time (&demux->common.segment,
            GST_FORMAT_TIME, earliest_time);

        if (GST_CLOCK_TIME_IS_VALID (lace_time) &&
            GST_CLOCK_TIME_IS_VALID (earliest_stream_time) &&
            lace_time <= earliest_stream_time) {
          /* find index entry (keyframe) <= earliest_stream_time */
          GstMatroskaIndex *entry =
              gst_util_array_binary_search (stream->index_table->data,
              stream->index_table->len, sizeof (GstMatroskaIndex),
              (GCompareDataFunc) gst_matroska_index_seek_find,
              GST_SEARCH_MODE_BEFORE, &earliest_stream_time, NULL);

          /* if that entry (keyframe) is after the current the current
             buffer, we can skip pushing (and thus decoding) all
             buffers until that keyframe. */
          if (entry && GST_CLOCK_TIME_IS_VALID (entry->time) &&
              entry->time > lace_time) {
            GST_LOG_OBJECT (demux, "Skipping lace before late keyframe");
            stream->set_discont = TRUE;
            goto next_lace;
          }
        }
      }

      sub = gst_buffer_copy_region (buf, GST_BUFFER_COPY_ALL,
          gst_buffer_get_size (buf) - size, lace_size[n]);
      GST_DEBUG_OBJECT (demux, "created subbuffer %p", sub);

      if (delta_unit)
        GST_BUFFER_FLAG_SET (sub, GST_BUFFER_FLAG_DELTA_UNIT);
      else
        GST_BUFFER_FLAG_UNSET (sub, GST_BUFFER_FLAG_DELTA_UNIT);

      if (invisible_frame)
        GST_BUFFER_FLAG_SET (sub, GST_BUFFER_FLAG_DECODE_ONLY);

      if (stream->encodings != NULL && stream->encodings->len > 0)
        sub = gst_matroska_decode_buffer (stream, sub);

      if (sub == NULL) {
        GST_WARNING_OBJECT (demux, "Decoding buffer failed");
        goto next_lace;
      }

      if (!stream->dts_only) {
        GST_BUFFER_PTS (sub) = lace_time;
      } else {
        GST_BUFFER_DTS (sub) = lace_time;
        if (stream->intra_only)
          GST_BUFFER_PTS (sub) = lace_time;
      }

      buffer_timestamp = gst_matroska_track_get_buffer_timestamp (stream, sub);

      if (GST_CLOCK_TIME_IS_VALID (lace_time)) {
        GstClockTime last_stop_end;

        /* Check if this stream is after segment stop */
        if (GST_CLOCK_TIME_IS_VALID (demux->common.segment.stop) &&
            lace_time >= demux->common.segment.stop) {
          GST_DEBUG_OBJECT (demux,
              "Stream %d after segment stop %" GST_TIME_FORMAT, stream->index,
              GST_TIME_ARGS (demux->common.segment.stop));
          gst_buffer_unref (sub);
          goto eos;
        }
        if (offset >= stream->to_offset
            || (GST_CLOCK_TIME_IS_VALID (demux->to_time)
                && lace_time > demux->to_time)) {
          GST_DEBUG_OBJECT (demux, "Stream %d after playback section",
              stream->index);
          gst_buffer_unref (sub);
          goto eos;
        }

        /* handle gaps, e.g. non-zero start-time, or an cue index entry
         * that landed us with timestamps not quite intended */
        GST_OBJECT_LOCK (demux);
        if (demux->max_gap_time &&
            GST_CLOCK_TIME_IS_VALID (demux->last_stop_end) &&
            demux->common.segment.rate > 0.0) {
          GstClockTimeDiff diff;

          /* only send segments with increasing start times,
           * otherwise if these go back and forth downstream (sinks) increase
           * accumulated time and running_time */
          diff = GST_CLOCK_DIFF (demux->last_stop_end, lace_time);
          if (diff > 0 && diff > demux->max_gap_time
              && lace_time > demux->common.segment.start
              && (!GST_CLOCK_TIME_IS_VALID (demux->common.segment.stop)
                  || lace_time < demux->common.segment.stop)) {
            GstEvent *event;
            GST_DEBUG_OBJECT (demux,
                "Gap of %" G_GINT64_FORMAT " ns detected in"
                "stream %d (%" GST_TIME_FORMAT " -> %" GST_TIME_FORMAT "). "
                "Sending updated SEGMENT events", diff,
                stream->index, GST_TIME_ARGS (stream->pos),
                GST_TIME_ARGS (lace_time));

            event = gst_event_new_gap (demux->last_stop_end, diff);
            GST_OBJECT_UNLOCK (demux);
            gst_pad_push_event (stream->pad, event);
            GST_OBJECT_LOCK (demux);
          }
        }

        if (!GST_CLOCK_TIME_IS_VALID (demux->common.segment.position)
            || demux->common.segment.position < lace_time) {
          demux->common.segment.position = lace_time;
        }
        GST_OBJECT_UNLOCK (demux);

        last_stop_end = lace_time;
        if (duration) {
          GST_BUFFER_DURATION (sub) = duration / laces;
          last_stop_end += GST_BUFFER_DURATION (sub);
        }

        if (!GST_CLOCK_TIME_IS_VALID (demux->last_stop_end) ||
            demux->last_stop_end < last_stop_end)
          demux->last_stop_end = last_stop_end;

        GST_OBJECT_LOCK (demux);
        if (demux->common.segment.duration == -1 ||
            demux->stream_start_time + demux->common.segment.duration <
            last_stop_end) {
          demux->common.segment.duration =
              last_stop_end - demux->stream_start_time;
          GST_OBJECT_UNLOCK (demux);
          if (!demux->invalid_duration) {
            gst_element_post_message (GST_ELEMENT_CAST (demux),
                gst_message_new_duration_changed (GST_OBJECT_CAST (demux)));
            demux->invalid_duration = TRUE;
          }
        } else {
          GST_OBJECT_UNLOCK (demux);
        }
      }

      stream->pos = lace_time;

      gst_matroska_demux_sync_streams (demux);

      if (stream->set_discont) {
        GST_DEBUG_OBJECT (demux, "marking DISCONT");
        GST_BUFFER_FLAG_SET (sub, GST_BUFFER_FLAG_DISCONT);
        stream->set_discont = FALSE;
      } else {
        GST_BUFFER_FLAG_UNSET (sub, GST_BUFFER_FLAG_DISCONT);
      }

      /* reverse playback book-keeping */
      if (!GST_CLOCK_TIME_IS_VALID (stream->from_time))
        stream->from_time = lace_time;
      if (stream->from_offset == -1)
        stream->from_offset = offset;

      GST_DEBUG_OBJECT (demux,
          "Pushing lace %d, data of size %" G_GSIZE_FORMAT
          " for stream %d, time=%" GST_TIME_FORMAT " and duration=%"
          GST_TIME_FORMAT, n, gst_buffer_get_size (sub), stream_num,
          GST_TIME_ARGS (buffer_timestamp),
          GST_TIME_ARGS (GST_BUFFER_DURATION (sub)));

#if 0
      if (demux->common.element_index) {
        if (stream->index_writer_id == -1)
          gst_index_get_writer_id (demux->common.element_index,
              GST_OBJECT (stream->pad), &stream->index_writer_id);

        GST_LOG_OBJECT (demux, "adding association %" GST_TIME_FORMAT "-> %"
            G_GUINT64_FORMAT " for writer id %d",
            GST_TIME_ARGS (buffer_timestamp), cluster_offset,
            stream->index_writer_id);
        gst_index_add_association (demux->common.element_index,
            stream->index_writer_id, GST_BUFFER_FLAG_IS_SET (sub,
                GST_BUFFER_FLAG_DELTA_UNIT) ? 0 : GST_ASSOCIATION_FLAG_KEY_UNIT,
            GST_FORMAT_TIME, buffer_timestamp, GST_FORMAT_BYTES, cluster_offset,
            NULL);
      }
#endif

      /* Postprocess the buffers depending on the codec used */
      if (stream->postprocess_frame) {
        GST_LOG_OBJECT (demux, "running post process");
        ret = stream->postprocess_frame (GST_ELEMENT (demux), stream, &sub);
      }

      /* At this point, we have a sub-buffer pointing at data within a larger
         buffer. This data might not be aligned with anything. If the data is
         raw samples though, we want it aligned to the raw type (eg, 4 bytes
         for 32 bit samples, etc), or bad things will happen downstream as
         elements typically assume minimal alignment.
         Therefore, create an aligned copy if necessary. */
      sub = gst_matroska_demux_align_buffer (demux, sub, stream->alignment);

      if (!strcmp (stream->codec_id, GST_MATROSKA_CODEC_ID_AUDIO_OPUS)) {
        guint64 start_clip = 0, end_clip = 0;

        /* Codec delay is part of the timestamps */
        if (GST_BUFFER_PTS_IS_VALID (sub) && stream->codec_delay) {
          if (GST_BUFFER_PTS (sub) > stream->codec_delay) {
            GST_BUFFER_PTS (sub) -= stream->codec_delay;
          } else {
            GST_BUFFER_PTS (sub) = 0;

            /* Opus GstAudioClippingMeta units are scaled by 48000/sample_rate.
               That is, if a Opus track has audio encoded at 24000 Hz and 132
               samples need to be clipped, GstAudioClippingMeta.start will be
               set to 264. (This is also the case for buffer offsets.)
               Opus sample rates are always divisors of 48000 Hz, which is the
               maximum allowed sample rate. */
            start_clip =
                gst_util_uint64_scale_round (stream->codec_delay, 48000,
                GST_SECOND);

            if (GST_BUFFER_DURATION_IS_VALID (sub)) {
              if (GST_BUFFER_DURATION (sub) > stream->codec_delay)
                GST_BUFFER_DURATION (sub) -= stream->codec_delay;
              else
                GST_BUFFER_DURATION (sub) = 0;
            }
          }
        }

        if (block_discardpadding) {
          end_clip =
              gst_util_uint64_scale_round (block_discardpadding, 48000,
              GST_SECOND);
        }

        if (start_clip || end_clip) {
          gst_buffer_add_audio_clipping_meta (sub, GST_FORMAT_DEFAULT,
              start_clip, end_clip);
        }
      }

      if (GST_BUFFER_PTS_IS_VALID (sub)) {
        stream->pos = GST_BUFFER_PTS (sub);
        if (GST_BUFFER_DURATION_IS_VALID (sub))
          stream->pos += GST_BUFFER_DURATION (sub);
      } else if (GST_BUFFER_DTS_IS_VALID (sub)) {
        stream->pos = GST_BUFFER_DTS (sub);
        if (GST_BUFFER_DURATION_IS_VALID (sub))
          stream->pos += GST_BUFFER_DURATION (sub);
      }

      ret = gst_pad_push (stream->pad, sub);

      if (demux->common.segment.rate < 0) {
        if (lace_time > demux->common.segment.stop && ret == GST_FLOW_EOS) {
          /* In reverse playback we can get a GST_FLOW_EOS when
           * we are at the end of the segment, so we just need to jump
           * back to the previous section. */
          GST_DEBUG_OBJECT (demux, "downstream has reached end of segment");
          ret = GST_FLOW_OK;
        }
      }
      /* combine flows */
      ret = gst_flow_combiner_update_pad_flow (demux->flowcombiner,
          stream->pad, ret);

    next_lace:
      size -= lace_size[n];
      if (lace_time != GST_CLOCK_TIME_NONE && duration)
        lace_time += duration / laces;
      else
        lace_time = GST_CLOCK_TIME_NONE;
    }
  }

done:
  if (buf) {
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
  }
  g_free (lace_size);

  return ret;

  /* EXITS */
eos:
  {
    stream->eos = TRUE;
    ret = GST_FLOW_OK;
    /* combine flows */
    ret = gst_flow_combiner_update_pad_flow (demux->flowcombiner, stream->pad,
        ret);
    goto done;
  }
invalid_lacing:
  {
    GST_ELEMENT_WARNING (demux, STREAM, DEMUX, (NULL), ("Invalid lacing size"));
    /* non-fatal, try next block(group) */
    ret = GST_FLOW_OK;
    goto done;
  }
data_error:
  {
    GST_ELEMENT_WARNING (demux, STREAM, DEMUX, (NULL), ("Data error"));
    /* non-fatal, try next block(group) */
    ret = GST_FLOW_OK;
    goto done;
  }
}

/* return FALSE if block(group) should be skipped (due to a seek) */
static inline gboolean
gst_matroska_demux_seek_block (GstMatroskaDemux * demux)
{
  if (G_UNLIKELY (demux->seek_block)) {
    if (!(--demux->seek_block)) {
      return TRUE;
    } else {
      GST_LOG_OBJECT (demux, "should skip block due to seek");
      return FALSE;
    }
  } else {
    return TRUE;
  }
}

static GstFlowReturn
gst_matroska_demux_parse_contents_seekentry (GstMatroskaDemux * demux,
    GstEbmlRead * ebml)
{
  GstFlowReturn ret;
  guint64 seek_pos = (guint64) - 1;
  guint32 seek_id = 0;
  guint32 id;

  DEBUG_ELEMENT_START (demux, ebml, "Seek");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (demux, ebml, "Seek", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_SEEKID:
      {
        guint64 t;

        if ((ret = gst_ebml_read_uint (ebml, &id, &t)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (demux, "SeekID: %" G_GUINT64_FORMAT, t);
        seek_id = t;
        break;
      }

      case GST_MATROSKA_ID_SEEKPOSITION:
      {
        guint64 t;

        if ((ret = gst_ebml_read_uint (ebml, &id, &t)) != GST_FLOW_OK)
          break;

        if (t > G_MAXINT64) {
          GST_WARNING_OBJECT (demux,
              "Too large SeekPosition %" G_GUINT64_FORMAT, t);
          break;
        }

        GST_DEBUG_OBJECT (demux, "SeekPosition: %" G_GUINT64_FORMAT, t);
        seek_pos = t;
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&demux->common, ebml,
            "SeekHead", id);
        break;
    }
  }

  if (ret != GST_FLOW_OK && ret != GST_FLOW_EOS)
    return ret;

  if (!seek_id || seek_pos == (guint64) - 1) {
    GST_WARNING_OBJECT (demux, "Incomplete seekhead entry (0x%x/%"
        G_GUINT64_FORMAT ")", seek_id, seek_pos);
    return GST_FLOW_OK;
  }

  switch (seek_id) {
    case GST_MATROSKA_ID_SEEKHEAD:
    {
    }
    case GST_MATROSKA_ID_CUES:
    case GST_MATROSKA_ID_TAGS:
    case GST_MATROSKA_ID_TRACKS:
    case GST_MATROSKA_ID_SEGMENTINFO:
    case GST_MATROSKA_ID_ATTACHMENTS:
    case GST_MATROSKA_ID_CHAPTERS:
    {
      guint64 before_pos, length;
      guint needed;

      /* remember */
      length = gst_matroska_read_common_get_length (&demux->common);
      before_pos = demux->common.offset;

      if (length == (guint64) - 1) {
        GST_DEBUG_OBJECT (demux, "no upstream length, skipping SeakHead entry");
        break;
      }

      /* check for validity */
      if (seek_pos + demux->common.ebml_segment_start + 12 >= length) {
        GST_WARNING_OBJECT (demux,
            "SeekHead reference lies outside file!" " (%"
            G_GUINT64_FORMAT "+%" G_GUINT64_FORMAT "+12 >= %"
            G_GUINT64_FORMAT ")", seek_pos, demux->common.ebml_segment_start,
            length);
        break;
      }

      /* only pick up index location when streaming */
      if (demux->streaming) {
        if (seek_id == GST_MATROSKA_ID_CUES) {
          demux->index_offset = seek_pos + demux->common.ebml_segment_start;
          GST_DEBUG_OBJECT (demux, "Cues located at offset %" G_GUINT64_FORMAT,
              demux->index_offset);
        }
        break;
      }

      /* seek */
      demux->common.offset = seek_pos + demux->common.ebml_segment_start;

      /* check ID */
      if ((ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
                  GST_ELEMENT_CAST (demux), &id, &length, &needed)) !=
          GST_FLOW_OK)
        goto finish;

      if (id != seek_id) {
        GST_WARNING_OBJECT (demux,
            "We looked for ID=0x%x but got ID=0x%x (pos=%" G_GUINT64_FORMAT ")",
            seek_id, id, seek_pos + demux->common.ebml_segment_start);
      } else {
        /* now parse */
        ret = gst_matroska_demux_parse_id (demux, id, length, needed);
      }

    finish:
      /* seek back */
      demux->common.offset = before_pos;
      break;
    }

    case GST_MATROSKA_ID_CLUSTER:
    {
      guint64 pos = seek_pos + demux->common.ebml_segment_start;

      GST_LOG_OBJECT (demux, "Cluster position");
      if (G_UNLIKELY (!demux->clusters))
        demux->clusters = g_array_sized_new (TRUE, TRUE, sizeof (guint64), 100);
      g_array_append_val (demux->clusters, pos);
      break;
    }

    default:
      GST_DEBUG_OBJECT (demux, "Ignoring Seek entry for ID=0x%x", seek_id);
      break;
  }
  DEBUG_ELEMENT_STOP (demux, ebml, "Seek", ret);

  return ret;
}

static GstFlowReturn
gst_matroska_demux_parse_contents (GstMatroskaDemux * demux, GstEbmlRead * ebml)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 id;

  DEBUG_ELEMENT_START (demux, ebml, "SeekHead");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (demux, ebml, "SeekHead", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_SEEKENTRY:
      {
        ret = gst_matroska_demux_parse_contents_seekentry (demux, ebml);
        /* Ignore EOS and errors here */
        if (ret != GST_FLOW_OK) {
          GST_DEBUG_OBJECT (demux, "Ignoring %s", gst_flow_get_name (ret));
          ret = GST_FLOW_OK;
        }
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&demux->common,
            ebml, "SeekHead", id);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (demux, ebml, "SeekHead", ret);

  /* Sort clusters by position for easier searching */
  if (demux->clusters)
    g_array_sort (demux->clusters, (GCompareFunc) gst_matroska_cluster_compare);

  return ret;
}

#define GST_FLOW_OVERFLOW   GST_FLOW_CUSTOM_ERROR

#define MAX_BLOCK_SIZE (15 * 1024 * 1024)

static inline GstFlowReturn
gst_matroska_demux_check_read_size (GstMatroskaDemux * demux, guint64 bytes)
{
  if (G_UNLIKELY (bytes > MAX_BLOCK_SIZE)) {
    /* only a few blocks are expected/allowed to be large,
     * and will be recursed into, whereas others will be read and must fit */
    if (demux->streaming) {
      /* fatal in streaming case, as we can't step over easily */
      GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL),
          ("reading large block of size %" G_GUINT64_FORMAT " not supported; "
              "file might be corrupt.", bytes));
      return GST_FLOW_ERROR;
    } else {
      /* indicate higher level to quietly give up */
      GST_DEBUG_OBJECT (demux,
          "too large block of size %" G_GUINT64_FORMAT, bytes);
      return GST_FLOW_ERROR;
    }
  } else {
    return GST_FLOW_OK;
  }
}

/* returns TRUE if we truly are in error state, and should give up */
static inline GstFlowReturn
gst_matroska_demux_check_parse_error (GstMatroskaDemux * demux)
{
  if (!demux->streaming && demux->next_cluster_offset > 0) {
    /* just repositioning to where next cluster should be and try from there */
    GST_WARNING_OBJECT (demux, "parse error, trying next cluster expected at %"
        G_GUINT64_FORMAT, demux->next_cluster_offset);
    demux->common.offset = demux->next_cluster_offset;
    demux->next_cluster_offset = 0;
    return GST_FLOW_OK;
  } else {
    gint64 pos;
    GstFlowReturn ret;

    /* sigh, one last attempt above and beyond call of duty ...;
     * search for cluster mark following current pos */
    pos = demux->common.offset;
    GST_WARNING_OBJECT (demux, "parse error, looking for next cluster");
    if ((ret = gst_matroska_demux_search_cluster (demux, &pos, TRUE)) !=
        GST_FLOW_OK) {
      /* did not work, give up */
      return ret;
    } else {
      GST_DEBUG_OBJECT (demux, "... found at  %" G_GUINT64_FORMAT, pos);
      /* try that position */
      demux->common.offset = pos;
      return GST_FLOW_OK;
    }
  }
}

static inline GstFlowReturn
gst_matroska_demux_flush (GstMatroskaDemux * demux, guint flush)
{
  GST_LOG_OBJECT (demux, "skipping %d bytes", flush);
  demux->common.offset += flush;
  if (demux->streaming) {
    GstFlowReturn ret;

    /* hard to skip large blocks when streaming */
    ret = gst_matroska_demux_check_read_size (demux, flush);
    if (ret != GST_FLOW_OK)
      return ret;
    if (flush <= gst_adapter_available (demux->common.adapter))
      gst_adapter_flush (demux->common.adapter, flush);
    else
      return GST_FLOW_EOS;
  }
  return GST_FLOW_OK;
}

/* initializes @ebml with @bytes from input stream at current offset.
 * Returns EOS if insufficient available,
 * ERROR if too much was attempted to read. */
static inline GstFlowReturn
gst_matroska_demux_take (GstMatroskaDemux * demux, guint64 bytes,
    GstEbmlRead * ebml)
{
  GstBuffer *buffer = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_LOG_OBJECT (demux, "taking %" G_GUINT64_FORMAT " bytes for parsing",
      bytes);
  ret = gst_matroska_demux_check_read_size (demux, bytes);
  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    if (!demux->streaming) {
      /* in pull mode, we can skip */
      if ((ret = gst_matroska_demux_flush (demux, bytes)) == GST_FLOW_OK)
        ret = GST_FLOW_OVERFLOW;
    } else {
      /* otherwise fatal */
      ret = GST_FLOW_ERROR;
    }
    goto exit;
  }
  if (demux->streaming) {
    if (gst_adapter_available (demux->common.adapter) >= bytes)
      buffer = gst_adapter_take_buffer (demux->common.adapter, bytes);
    else
      ret = GST_FLOW_EOS;
  } else
    ret = gst_matroska_read_common_peek_bytes (&demux->common,
        demux->common.offset, bytes, &buffer, NULL);
  if (G_LIKELY (buffer)) {
    gst_ebml_read_init (ebml, GST_ELEMENT_CAST (demux), buffer,
        demux->common.offset);
    demux->common.offset += bytes;
  }
exit:
  return ret;
}

static void
gst_matroska_demux_check_seekability (GstMatroskaDemux * demux)
{
  GstQuery *query;
  gboolean seekable = FALSE;
  gint64 start = -1, stop = -1;

  query = gst_query_new_seeking (GST_FORMAT_BYTES);
  if (!gst_pad_peer_query (demux->common.sinkpad, query)) {
    GST_DEBUG_OBJECT (demux, "seeking query failed");
    goto done;
  }

  gst_query_parse_seeking (query, NULL, &seekable, &start, &stop);

  /* try harder to query upstream size if we didn't get it the first time */
  if (seekable && stop == -1) {
    GST_DEBUG_OBJECT (demux, "doing duration query to fix up unset stop");
    gst_pad_peer_query_duration (demux->common.sinkpad, GST_FORMAT_BYTES,
        &stop);
  }

  /* if upstream doesn't know the size, it's likely that it's not seekable in
   * practice even if it technically may be seekable */
  if (seekable && (start != 0 || stop <= start)) {
    GST_DEBUG_OBJECT (demux, "seekable but unknown start/stop -> disable");
    seekable = FALSE;
  }

done:
  GST_INFO_OBJECT (demux, "seekable: %d (%" G_GUINT64_FORMAT " - %"
      G_GUINT64_FORMAT ")", seekable, start, stop);
  demux->seekable = seekable;

  gst_query_unref (query);
}

static GstFlowReturn
gst_matroska_demux_find_tracks (GstMatroskaDemux * demux)
{
  guint32 id;
  guint64 before_pos;
  guint64 length;
  guint needed;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_WARNING_OBJECT (demux,
      "Found Cluster element before Tracks, searching Tracks");

  /* remember */
  before_pos = demux->common.offset;

  /* Search Tracks element */
  while (TRUE) {
    ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
        GST_ELEMENT_CAST (demux), &id, &length, &needed);
    if (ret != GST_FLOW_OK)
      break;

    if (id != GST_MATROSKA_ID_TRACKS) {
      /* we may be skipping large cluster here, so forego size check etc */
      /* ... but we can't skip undefined size; force error */
      if (length == G_MAXUINT64) {
        ret = gst_matroska_demux_check_read_size (demux, length);
        break;
      } else {
        demux->common.offset += needed;
        demux->common.offset += length;
      }
      continue;
    }

    /* will lead to track parsing ... */
    ret = gst_matroska_demux_parse_id (demux, id, length, needed);
    break;
  }

  /* seek back */
  demux->common.offset = before_pos;

  return ret;
}

#define GST_READ_CHECK(stmt)  \
G_STMT_START { \
  if (G_UNLIKELY ((ret = (stmt)) != GST_FLOW_OK)) { \
    if (ret == GST_FLOW_OVERFLOW) { \
      ret = GST_FLOW_OK; \
    } \
    goto read_error; \
  } \
} G_STMT_END

static GstFlowReturn
gst_matroska_demux_parse_id (GstMatroskaDemux * demux, guint32 id,
    guint64 length, guint needed)
{
  GstEbmlRead ebml = { 0, };
  GstFlowReturn ret = GST_FLOW_OK;
  guint64 read;

  GST_LOG_OBJECT (demux, "Parsing Element id 0x%x, "
      "size %" G_GUINT64_FORMAT ", prefix %d", id, length, needed);

  /* if we plan to read and parse this element, we need prefix (id + length)
   * and the contents */
  /* mind about overflow wrap-around when dealing with undefined size */
  read = length;
  if (G_LIKELY (length != G_MAXUINT64))
    read += needed;

  switch (demux->common.state) {
    case GST_MATROSKA_READ_STATE_START:
      switch (id) {
        case GST_EBML_ID_HEADER:
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          ret = gst_matroska_read_common_parse_header (&demux->common, &ebml);
          if (ret != GST_FLOW_OK)
            goto parse_failed;
          demux->common.state = GST_MATROSKA_READ_STATE_SEGMENT;
          gst_matroska_demux_check_seekability (demux);
          break;
        default:
          goto invalid_header;
          break;
      }
      break;
    case GST_MATROSKA_READ_STATE_SEGMENT:
      switch (id) {
        case GST_MATROSKA_ID_SEGMENT:
          /* eat segment prefix */
          GST_READ_CHECK (gst_matroska_demux_flush (demux, needed));
          GST_DEBUG_OBJECT (demux,
              "Found Segment start at offset %" G_GUINT64_FORMAT " with size %"
              G_GUINT64_FORMAT, demux->common.offset, length);
          /* seeks are from the beginning of the segment,
           * after the segment ID/length */
          demux->common.ebml_segment_start = demux->common.offset;
          if (length == 0)
            length = G_MAXUINT64;
          demux->common.ebml_segment_length = length;
          demux->common.state = GST_MATROSKA_READ_STATE_HEADER;
          break;
        default:
          GST_WARNING_OBJECT (demux,
              "Expected a Segment ID (0x%x), but received 0x%x!",
              GST_MATROSKA_ID_SEGMENT, id);
          GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          break;
      }
      break;
    case GST_MATROSKA_READ_STATE_SCANNING:
      if (id != GST_MATROSKA_ID_CLUSTER &&
          id != GST_MATROSKA_ID_PREVSIZE &&
          id != GST_MATROSKA_ID_CLUSTERTIMECODE) {
        if (demux->common.start_resync_offset != -1) {
          /* we need to skip byte per byte if we are scanning for a new cluster
           * after invalid data is found
           */
          read = 1;
        }
        goto skip;
      } else {
        if (demux->common.start_resync_offset != -1) {
          GST_LOG_OBJECT (demux, "Resync done, new cluster found!");
          demux->common.start_resync_offset = -1;
          demux->common.state = demux->common.state_to_restore;
        }
      }
      /* fall-through */
    case GST_MATROSKA_READ_STATE_HEADER:
    case GST_MATROSKA_READ_STATE_DATA:
    case GST_MATROSKA_READ_STATE_SEEK:
      switch (id) {
        case GST_EBML_ID_HEADER:
          GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          demux->common.state = GST_MATROSKA_READ_STATE_SEGMENT;
          gst_matroska_demux_check_seekability (demux);
          break;
        case GST_MATROSKA_ID_SEGMENTINFO:
          if (!demux->common.segmentinfo_parsed) {
            GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
            ret = gst_matroska_read_common_parse_info (&demux->common,
                GST_ELEMENT_CAST (demux), &ebml);
            if (ret == GST_FLOW_OK)
              gst_matroska_demux_send_tags (demux);
          } else {
            GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          }
          break;
        case GST_MATROSKA_ID_TRACKS:
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          if (!demux->tracks_parsed) {
            ret = gst_matroska_demux_parse_tracks (demux, &ebml);
          } else {
            ret = gst_matroska_demux_update_tracks (demux, &ebml);
          }
          break;
        case GST_MATROSKA_ID_CLUSTER:
          if (G_UNLIKELY (!demux->tracks_parsed)) {
            if (demux->streaming) {
              GST_DEBUG_OBJECT (demux, "Cluster before Track");
              goto not_streamable;
            } else {
              ret = gst_matroska_demux_find_tracks (demux);
              if (!demux->tracks_parsed)
                goto no_tracks;
            }
          }
          if (demux->common.state == GST_MATROSKA_READ_STATE_HEADER) {
            demux->common.state = GST_MATROSKA_READ_STATE_DATA;
            demux->first_cluster_offset = demux->common.offset;

            if (!demux->streaming &&
                !GST_CLOCK_TIME_IS_VALID (demux->common.segment.duration)) {
              GstMatroskaIndex *last = NULL;

              GST_DEBUG_OBJECT (demux,
                  "estimating duration using last cluster");
              if ((last = gst_matroska_demux_search_pos (demux,
                          GST_CLOCK_TIME_NONE)) != NULL) {
                demux->last_cluster_offset =
                    last->pos + demux->common.ebml_segment_start;
                demux->stream_last_time = last->time;
                demux->common.segment.duration =
                    demux->stream_last_time - demux->stream_start_time;
                /* above estimate should not be taken all too strongly */
                demux->invalid_duration = TRUE;
                GST_DEBUG_OBJECT (demux,
                    "estimated duration as %" GST_TIME_FORMAT,
                    GST_TIME_ARGS (demux->common.segment.duration));

                g_free (last);
              }
            }

            /* Peek at second cluster in order to figure out if we have cluster
             * prev_size or not (which is never set on the first cluster for
             * obvious reasons). This is useful in case someone initiates a
             * seek or direction change before we reach the second cluster. */
            if (!demux->streaming) {
              ClusterInfo cluster = { 0, };

              if (gst_matroska_demux_peek_cluster_info (demux, &cluster,
                      demux->first_cluster_offset) && cluster.size > 0) {
                gst_matroska_demux_peek_cluster_info (demux, &cluster,
                    demux->first_cluster_offset + cluster.size);
              }
              demux->common.offset = demux->first_cluster_offset;
            }

            if (demux->deferred_seek_event) {
              GstEvent *seek_event;
              GstPad *seek_pad;
              seek_event = demux->deferred_seek_event;
              seek_pad = demux->deferred_seek_pad;
              demux->deferred_seek_event = NULL;
              demux->deferred_seek_pad = NULL;
              GST_DEBUG_OBJECT (demux,
                  "Handling deferred seek event: %" GST_PTR_FORMAT, seek_event);
              gst_matroska_demux_handle_seek_event (demux, seek_pad,
                  seek_event);
              gst_event_unref (seek_event);
            }

            /* send initial segment - we wait till we know the first
               incoming timestamp, so we can properly set the start of
               the segment. */
            demux->need_segment = TRUE;
          }
          demux->cluster_time = GST_CLOCK_TIME_NONE;
          demux->cluster_offset = demux->common.offset;
          demux->cluster_prevsize = 0;
          if (G_UNLIKELY (!demux->seek_first && demux->seek_block)) {
            GST_DEBUG_OBJECT (demux, "seek target block %" G_GUINT64_FORMAT
                " not found in Cluster, trying next Cluster's first block instead",
                demux->seek_block);
            demux->seek_block = 0;
          }
          demux->seek_first = FALSE;
          /* record next cluster for recovery */
          if (read != G_MAXUINT64)
            demux->next_cluster_offset = demux->cluster_offset + read;
          /* eat cluster prefix */
          gst_matroska_demux_flush (demux, needed);
          break;
        case GST_MATROSKA_ID_CLUSTERTIMECODE:
        {
          guint64 num;

          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          if ((ret = gst_ebml_read_uint (&ebml, &id, &num)) != GST_FLOW_OK)
            goto parse_failed;
          GST_DEBUG_OBJECT (demux, "ClusterTimeCode: %" G_GUINT64_FORMAT, num);
          demux->cluster_time = num;
          /* track last cluster */
          if (demux->cluster_offset > demux->last_cluster_offset) {
            demux->last_cluster_offset = demux->cluster_offset;
            demux->stream_last_time =
                demux->cluster_time * demux->common.time_scale;
          }
#if 0
          if (demux->common.element_index) {
            if (demux->common.element_index_writer_id == -1)
              gst_index_get_writer_id (demux->common.element_index,
                  GST_OBJECT (demux), &demux->common.element_index_writer_id);
            GST_LOG_OBJECT (demux, "adding association %" GST_TIME_FORMAT "-> %"
                G_GUINT64_FORMAT " for writer id %d",
                GST_TIME_ARGS (demux->cluster_time), demux->cluster_offset,
                demux->common.element_index_writer_id);
            gst_index_add_association (demux->common.element_index,
                demux->common.element_index_writer_id,
                GST_ASSOCIATION_FLAG_KEY_UNIT,
                GST_FORMAT_TIME, demux->cluster_time,
                GST_FORMAT_BYTES, demux->cluster_offset, NULL);
          }
#endif
          break;
        }
        case GST_MATROSKA_ID_BLOCKGROUP:
          if (!gst_matroska_demux_seek_block (demux))
            goto skip;
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          DEBUG_ELEMENT_START (demux, &ebml, "BlockGroup");
          if ((ret = gst_ebml_read_master (&ebml, &id)) == GST_FLOW_OK) {
            ret = gst_matroska_demux_parse_blockgroup_or_simpleblock (demux,
                &ebml, demux->cluster_time, demux->cluster_offset, FALSE);
          }
          DEBUG_ELEMENT_STOP (demux, &ebml, "BlockGroup", ret);
          break;
        case GST_MATROSKA_ID_SIMPLEBLOCK:
          if (!gst_matroska_demux_seek_block (demux))
            goto skip;
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          DEBUG_ELEMENT_START (demux, &ebml, "SimpleBlock");
          ret = gst_matroska_demux_parse_blockgroup_or_simpleblock (demux,
              &ebml, demux->cluster_time, demux->cluster_offset, TRUE);
          DEBUG_ELEMENT_STOP (demux, &ebml, "SimpleBlock", ret);
          break;
        case GST_MATROSKA_ID_ATTACHMENTS:
          if (!demux->common.attachments_parsed) {
            GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
            ret = gst_matroska_read_common_parse_attachments (&demux->common,
                GST_ELEMENT_CAST (demux), &ebml);
            if (ret == GST_FLOW_OK)
              gst_matroska_demux_send_tags (demux);
          } else {
            GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          }
          break;
        case GST_MATROSKA_ID_TAGS:
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          ret = gst_matroska_read_common_parse_metadata (&demux->common,
              GST_ELEMENT_CAST (demux), &ebml);
          if (ret == GST_FLOW_OK)
            gst_matroska_demux_send_tags (demux);
          break;
        case GST_MATROSKA_ID_CHAPTERS:
          if (!demux->common.chapters_parsed) {
            GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
            ret =
                gst_matroska_read_common_parse_chapters (&demux->common, &ebml);

            if (demux->common.toc) {
              gst_matroska_demux_send_event (demux,
                  gst_event_new_toc (demux->common.toc, FALSE));
            }
          } else
            GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          break;
        case GST_MATROSKA_ID_SEEKHEAD:
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          ret = gst_matroska_demux_parse_contents (demux, &ebml);
          break;
        case GST_MATROSKA_ID_CUES:
          if (demux->common.index_parsed) {
            GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
            break;
          }
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          ret = gst_matroska_read_common_parse_index (&demux->common, &ebml);
          /* only push based; delayed index building */
          if (ret == GST_FLOW_OK
              && demux->common.state == GST_MATROSKA_READ_STATE_SEEK) {
            GstEvent *event;

            GST_OBJECT_LOCK (demux);
            event = demux->seek_event;
            demux->seek_event = NULL;
            GST_OBJECT_UNLOCK (demux);

            g_assert (event);
            /* unlikely to fail, since we managed to seek to this point */
            if (!gst_matroska_demux_handle_seek_event (demux, NULL, event)) {
              gst_event_unref (event);
              goto seek_failed;
            }
            gst_event_unref (event);
            /* resume data handling, main thread clear to seek again */
            GST_OBJECT_LOCK (demux);
            demux->common.state = GST_MATROSKA_READ_STATE_DATA;
            GST_OBJECT_UNLOCK (demux);
          }
          break;
        case GST_MATROSKA_ID_PREVSIZE:{
          guint64 num;

          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          if ((ret = gst_ebml_read_uint (&ebml, &id, &num)) != GST_FLOW_OK)
            goto parse_failed;
          GST_LOG_OBJECT (demux, "ClusterPrevSize: %" G_GUINT64_FORMAT, num);
          demux->cluster_prevsize = num;
          demux->seen_cluster_prevsize = TRUE;
          break;
        }
        case GST_MATROSKA_ID_POSITION:
        case GST_MATROSKA_ID_ENCRYPTEDBLOCK:
          /* The WebM doesn't support the EncryptedBlock element.
           * The Matroska spec doesn't give us more detail, how to parse this element,
           * for example the field TransformID isn't specified yet.*/
        case GST_MATROSKA_ID_SILENTTRACKS:
          GST_DEBUG_OBJECT (demux,
              "Skipping Cluster subelement 0x%x - ignoring", id);
          /* fall-through */
        default:
        skip:
          GST_DEBUG_OBJECT (demux, "skipping Element 0x%x", id);
          GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          break;
      }
      break;
  }

  if (ret == GST_FLOW_PARSE)
    goto parse_failed;

exit:
  gst_ebml_read_clear (&ebml);
  return ret;

  /* ERRORS */
read_error:
  {
    /* simply exit, maybe not enough data yet */
    /* no ebml to clear if read error */
    return ret;
  }
parse_failed:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL),
        ("Failed to parse Element 0x%x", id));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
not_streamable:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL),
        ("File layout does not permit streaming"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
no_tracks:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL),
        ("No Tracks element found"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
invalid_header:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL), ("Invalid header"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
seek_failed:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL), ("Failed to seek"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
}

static void
gst_matroska_demux_loop (GstPad * pad)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (GST_PAD_PARENT (pad));
  GstFlowReturn ret;
  guint32 id;
  guint64 length;
  guint needed;

  /* If we have to close a segment, send a new segment to do this now */
  if (G_LIKELY (demux->common.state == GST_MATROSKA_READ_STATE_DATA)) {
    if (G_UNLIKELY (demux->new_segment)) {
      gst_matroska_demux_send_event (demux, demux->new_segment);
      demux->new_segment = NULL;
    }
  }

  ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
      GST_ELEMENT_CAST (demux), &id, &length, &needed);
  if (ret == GST_FLOW_EOS) {
    goto eos;
  } else if (ret == GST_FLOW_FLUSHING) {
    goto pause;
  } else if (ret != GST_FLOW_OK) {
    ret = gst_matroska_demux_check_parse_error (demux);

    /* Only handle EOS as no error if we're outside the segment already */
    if (ret == GST_FLOW_EOS && (demux->common.ebml_segment_length != G_MAXUINT64
            && demux->common.offset >=
            demux->common.ebml_segment_start +
            demux->common.ebml_segment_length))
      goto eos;
    else if (ret != GST_FLOW_OK)
      goto pause;
    else
      return;
  }

  GST_LOG_OBJECT (demux, "Offset %" G_GUINT64_FORMAT ", Element id 0x%x, "
      "size %" G_GUINT64_FORMAT ", needed %d", demux->common.offset, id,
      length, needed);

  ret = gst_matroska_demux_parse_id (demux, id, length, needed);
  if (ret == GST_FLOW_EOS)
    goto eos;
  if (ret != GST_FLOW_OK)
    goto pause;

  /* check if we're at the end of a configured segment */
  if (G_LIKELY (demux->common.src->len)) {
    guint i;

    g_assert (demux->common.num_streams == demux->common.src->len);
    for (i = 0; i < demux->common.src->len; i++) {
      GstMatroskaTrackContext *context = g_ptr_array_index (demux->common.src,
          i);
      GST_DEBUG_OBJECT (context->pad, "pos %" GST_TIME_FORMAT,
          GST_TIME_ARGS (context->pos));
      if (context->eos == FALSE)
        goto next;
    }

    GST_INFO_OBJECT (demux, "All streams are EOS");
    ret = GST_FLOW_EOS;
    goto eos;
  }

next:
  if (G_UNLIKELY (demux->cached_length == G_MAXUINT64 ||
          demux->common.offset >= demux->cached_length)) {
    demux->cached_length = gst_matroska_read_common_get_length (&demux->common);
    if (demux->common.offset == demux->cached_length) {
      GST_LOG_OBJECT (demux, "Reached end of stream");
      ret = GST_FLOW_EOS;
      goto eos;
    }
  }

  return;

  /* ERRORS */
eos:
  {
    if (demux->common.segment.rate < 0.0) {
      ret = gst_matroska_demux_seek_to_previous_keyframe (demux);
      if (ret == GST_FLOW_OK)
        return;
    }
    /* fall-through */
  }
pause:
  {
    const gchar *reason = gst_flow_get_name (ret);
    gboolean push_eos = FALSE;

    GST_LOG_OBJECT (demux, "pausing task, reason %s", reason);
    gst_pad_pause_task (demux->common.sinkpad);

    if (ret == GST_FLOW_EOS) {
      /* perform EOS logic */

      /* If we were in the headers, make sure we send no-more-pads.
         This will ensure decodebin does not get stuck thinking
         the chain is not complete yet, and waiting indefinitely. */
      if (G_UNLIKELY (demux->common.state == GST_MATROSKA_READ_STATE_HEADER)) {
        if (demux->common.src->len == 0) {
          GST_ELEMENT_ERROR (demux, STREAM, FAILED, (NULL),
              ("No pads created"));
        } else {
          GST_ELEMENT_WARNING (demux, STREAM, DEMUX, (NULL),
              ("Failed to finish reading headers"));
        }
        gst_element_no_more_pads (GST_ELEMENT (demux));
      }

      if (demux->common.segment.flags & GST_SEEK_FLAG_SEGMENT) {
        GstEvent *event;
        GstMessage *msg;
        gint64 stop;

        /* for segment playback we need to post when (in stream time)
         * we stopped, this is either stop (when set) or the duration. */
        if ((stop = demux->common.segment.stop) == -1)
          stop = demux->last_stop_end;

        GST_LOG_OBJECT (demux, "Sending segment done, at end of segment");
        msg = gst_message_new_segment_done (GST_OBJECT (demux), GST_FORMAT_TIME,
            stop);
        if (demux->segment_seqnum)
          gst_message_set_seqnum (msg, demux->segment_seqnum);
        gst_element_post_message (GST_ELEMENT (demux), msg);

        event = gst_event_new_segment_done (GST_FORMAT_TIME, stop);
        if (demux->segment_seqnum)
          gst_event_set_seqnum (event, demux->segment_seqnum);
        gst_matroska_demux_send_event (demux, event);
      } else {
        push_eos = TRUE;
      }
    } else if (ret == GST_FLOW_NOT_LINKED || ret < GST_FLOW_EOS) {
      /* for fatal errors we post an error message */
      GST_ELEMENT_FLOW_ERROR (demux, ret);
      push_eos = TRUE;
    }
    if (push_eos) {
      GstEvent *event;

      /* send EOS, and prevent hanging if no streams yet */
      GST_LOG_OBJECT (demux, "Sending EOS, at end of stream");
      event = gst_event_new_eos ();
      if (demux->segment_seqnum)
        gst_event_set_seqnum (event, demux->segment_seqnum);
      if (!gst_matroska_demux_send_event (demux, event) &&
          (ret == GST_FLOW_EOS)) {
        GST_ELEMENT_ERROR (demux, STREAM, DEMUX,
            (NULL), ("got eos but no streams (yet)"));
      }
    }
    return;
  }
}

/*
 * Create and push a flushing seek event upstream
 */
static gboolean
perform_seek_to_offset (GstMatroskaDemux * demux, gdouble rate, guint64 offset,
    guint32 seqnum, GstSeekFlags flags)
{
  GstEvent *event;
  gboolean res = 0;

  GST_DEBUG_OBJECT (demux, "Seeking to %" G_GUINT64_FORMAT, offset);

  event =
      gst_event_new_seek (rate, GST_FORMAT_BYTES,
      flags | GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
      GST_SEEK_TYPE_SET, offset, GST_SEEK_TYPE_NONE, -1);
  gst_event_set_seqnum (event, seqnum);

  res = gst_pad_push_event (demux->common.sinkpad, event);

  /* segment event will update offset */
  return res;
}

static GstFlowReturn
gst_matroska_demux_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (parent);
  guint available;
  GstFlowReturn ret = GST_FLOW_OK;
  guint needed = 0;
  guint32 id;
  guint64 length;

  if (G_UNLIKELY (GST_BUFFER_IS_DISCONT (buffer))) {
    GST_DEBUG_OBJECT (demux, "got DISCONT");
    gst_adapter_clear (demux->common.adapter);
    GST_OBJECT_LOCK (demux);
    gst_matroska_read_common_reset_streams (&demux->common,
        GST_CLOCK_TIME_NONE, FALSE);
    GST_OBJECT_UNLOCK (demux);
  }

  gst_adapter_push (demux->common.adapter, buffer);
  buffer = NULL;

next:
  available = gst_adapter_available (demux->common.adapter);

  ret = gst_matroska_read_common_peek_id_length_push (&demux->common,
      GST_ELEMENT_CAST (demux), &id, &length, &needed);
  if (G_UNLIKELY (ret != GST_FLOW_OK && ret != GST_FLOW_EOS)) {
    if (demux->common.ebml_segment_length != G_MAXUINT64
        && demux->common.offset >=
        demux->common.ebml_segment_start + demux->common.ebml_segment_length) {
      return GST_FLOW_OK;
    } else {
      gint64 bytes_scanned;
      if (demux->common.start_resync_offset == -1) {
        demux->common.start_resync_offset = demux->common.offset;
        demux->common.state_to_restore = demux->common.state;
      }
      bytes_scanned = demux->common.offset - demux->common.start_resync_offset;
      if (bytes_scanned <= INVALID_DATA_THRESHOLD) {
        GST_WARNING_OBJECT (demux,
            "parse error, looking for next cluster, actual offset %"
            G_GUINT64_FORMAT ", start resync offset %" G_GUINT64_FORMAT,
            demux->common.offset, demux->common.start_resync_offset);
        demux->common.state = GST_MATROSKA_READ_STATE_SCANNING;
        ret = GST_FLOW_OK;
      } else {
        GST_WARNING_OBJECT (demux,
            "unrecoverable parse error, next cluster not found and threshold "
            "exceeded, bytes scanned %" G_GINT64_FORMAT, bytes_scanned);
        return ret;
      }
    }
  }

  GST_LOG_OBJECT (demux, "Offset %" G_GUINT64_FORMAT ", Element id 0x%x, "
      "size %" G_GUINT64_FORMAT ", needed %d, available %d",
      demux->common.offset, id, length, needed, available);

  if (needed > available)
    return GST_FLOW_OK;

  ret = gst_matroska_demux_parse_id (demux, id, length, needed);
  if (ret == GST_FLOW_EOS) {
    /* need more data */
    return GST_FLOW_OK;
  } else if (ret != GST_FLOW_OK) {
    return ret;
  } else
    goto next;
}

static gboolean
gst_matroska_demux_handle_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res = TRUE;
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (parent);

  GST_DEBUG_OBJECT (demux,
      "have event type %s: %p on sink pad", GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    {
      const GstSegment *segment;

      /* some debug output */
      gst_event_parse_segment (event, &segment);
      /* FIXME: do we need to update segment base here (like accum in 0.10)? */
      GST_DEBUG_OBJECT (demux,
          "received format %d segment %" GST_SEGMENT_FORMAT, segment->format,
          segment);

      if (demux->common.state < GST_MATROSKA_READ_STATE_DATA) {
        GST_DEBUG_OBJECT (demux, "still starting");
        goto exit;
      }

      /* we only expect a BYTE segment, e.g. following a seek */
      if (segment->format != GST_FORMAT_BYTES) {
        GST_DEBUG_OBJECT (demux, "unsupported segment format, ignoring");
        goto exit;
      }

      GST_DEBUG_OBJECT (demux, "clearing segment state");
      GST_OBJECT_LOCK (demux);
      /* clear current segment leftover */
      gst_adapter_clear (demux->common.adapter);
      /* and some streaming setup */
      demux->common.offset = segment->start;
      /* accumulate base based on current position */
      if (GST_CLOCK_TIME_IS_VALID (demux->common.segment.position))
        demux->common.segment.base +=
            (MAX (demux->common.segment.position, demux->stream_start_time)
            - demux->stream_start_time) / fabs (demux->common.segment.rate);
      /* do not know where we are;
       * need to come across a cluster and generate segment */
      demux->common.segment.position = GST_CLOCK_TIME_NONE;
      demux->cluster_time = GST_CLOCK_TIME_NONE;
      demux->cluster_offset = 0;
      demux->cluster_prevsize = 0;
      demux->need_segment = TRUE;
      demux->segment_seqnum = gst_event_get_seqnum (event);
      /* but keep some of the upstream segment */
      demux->common.segment.rate = segment->rate;
      demux->common.segment.flags = segment->flags;
      /* also check if need to keep some of the requested seek position */
      if (demux->seek_offset == segment->start) {
        GST_DEBUG_OBJECT (demux, "position matches requested seek");
        demux->common.segment.position = demux->requested_seek_time;
      } else {
        GST_DEBUG_OBJECT (demux, "unexpected segment position");
      }
      demux->requested_seek_time = GST_CLOCK_TIME_NONE;
      demux->seek_offset = -1;
      GST_OBJECT_UNLOCK (demux);
    exit:
      /* chain will send initial segment after pads have been added,
       * or otherwise come up with one */
      GST_DEBUG_OBJECT (demux, "eating event");
      gst_event_unref (event);
      res = TRUE;
      break;
    }
    case GST_EVENT_EOS:
    {
      if (demux->common.state != GST_MATROSKA_READ_STATE_DATA
          && demux->common.state != GST_MATROSKA_READ_STATE_SCANNING) {
        gst_event_unref (event);
        GST_ELEMENT_ERROR (demux, STREAM, DEMUX,
            (NULL), ("got eos and didn't receive a complete header object"));
      } else if (demux->common.num_streams == 0) {
        GST_ELEMENT_ERROR (demux, STREAM, DEMUX,
            (NULL), ("got eos but no streams (yet)"));
      } else {
        gst_matroska_demux_send_event (demux, event);
      }
      break;
    }
    case GST_EVENT_FLUSH_STOP:
    {
      guint64 dur;

      gst_adapter_clear (demux->common.adapter);
      GST_OBJECT_LOCK (demux);
      gst_matroska_read_common_reset_streams (&demux->common,
          GST_CLOCK_TIME_NONE, TRUE);
      gst_flow_combiner_reset (demux->flowcombiner);
      dur = demux->common.segment.duration;
      gst_segment_init (&demux->common.segment, GST_FORMAT_TIME);
      demux->common.segment.duration = dur;
      demux->cluster_time = GST_CLOCK_TIME_NONE;
      demux->cluster_offset = 0;
      demux->cluster_prevsize = 0;
      GST_OBJECT_UNLOCK (demux);
      /* fall-through */
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static gboolean
gst_matroska_demux_sink_activate (GstPad * sinkpad, GstObject * parent)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (parent);
  GstQuery *query;
  gboolean pull_mode = FALSE;

  query = gst_query_new_scheduling ();

  if (gst_pad_peer_query (sinkpad, query))
    pull_mode = gst_query_has_scheduling_mode_with_flags (query,
        GST_PAD_MODE_PULL, GST_SCHEDULING_FLAG_SEEKABLE);

  gst_query_unref (query);

  if (pull_mode) {
    GST_DEBUG ("going to pull mode");
    demux->streaming = FALSE;
    return gst_pad_activate_mode (sinkpad, GST_PAD_MODE_PULL, TRUE);
  } else {
    GST_DEBUG ("going to push (streaming) mode");
    demux->streaming = TRUE;
    return gst_pad_activate_mode (sinkpad, GST_PAD_MODE_PUSH, TRUE);
  }
}

static gboolean
gst_matroska_demux_sink_activate_mode (GstPad * sinkpad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
  switch (mode) {
    case GST_PAD_MODE_PULL:
      if (active) {
        /* if we have a scheduler we can start the task */
        gst_pad_start_task (sinkpad, (GstTaskFunction) gst_matroska_demux_loop,
            sinkpad, NULL);
      } else {
        gst_pad_stop_task (sinkpad);
      }
      return TRUE;
    case GST_PAD_MODE_PUSH:
      return TRUE;
    default:
      return FALSE;
  }
}

static GstCaps *
gst_matroska_demux_video_caps (GstMatroskaTrackVideoContext *
    videocontext, const gchar * codec_id, guint8 * data, guint size,
    gchar ** codec_name, guint32 * riff_fourcc)
{
  GstMatroskaTrackContext *context = (GstMatroskaTrackContext *) videocontext;
  GstCaps *caps = NULL;

  g_assert (videocontext != NULL);
  g_assert (codec_name != NULL);

  if (riff_fourcc)
    *riff_fourcc = 0;

  /* TODO: check if we have all codec types from matroska-ids.h
   *       check if we have to do more special things with codec_private
   *
   * Add support for
   *  GST_MATROSKA_CODEC_ID_VIDEO_QUICKTIME
   *  GST_MATROSKA_CODEC_ID_VIDEO_SNOW
   */

  if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_VFW_FOURCC)) {
    gst_riff_strf_vids *vids = NULL;

    if (data) {
      GstBuffer *buf = NULL;

      vids = (gst_riff_strf_vids *) data;

      /* assure size is big enough */
      if (size < 24) {
        GST_WARNING ("Too small BITMAPINFOHEADER (%d bytes)", size);
        return NULL;
      }
      if (size < sizeof (gst_riff_strf_vids)) {
        vids = g_new (gst_riff_strf_vids, 1);
        memcpy (vids, data, size);
      }

      context->dts_only = TRUE; /* VFW files only store DTS */

      /* little-endian -> byte-order */
      vids->size = GUINT32_FROM_LE (vids->size);
      vids->width = GUINT32_FROM_LE (vids->width);
      vids->height = GUINT32_FROM_LE (vids->height);
      vids->planes = GUINT16_FROM_LE (vids->planes);
      vids->bit_cnt = GUINT16_FROM_LE (vids->bit_cnt);
      vids->compression = GUINT32_FROM_LE (vids->compression);
      vids->image_size = GUINT32_FROM_LE (vids->image_size);
      vids->xpels_meter = GUINT32_FROM_LE (vids->xpels_meter);
      vids->ypels_meter = GUINT32_FROM_LE (vids->ypels_meter);
      vids->num_colors = GUINT32_FROM_LE (vids->num_colors);
      vids->imp_colors = GUINT32_FROM_LE (vids->imp_colors);

      if (size > sizeof (gst_riff_strf_vids)) { /* some extra_data */
        gsize offset = sizeof (gst_riff_strf_vids);

        buf =
            gst_buffer_new_wrapped (g_memdup ((guint8 *) vids + offset,
                size - offset), size - offset);
      }

      if (riff_fourcc)
        *riff_fourcc = vids->compression;

      caps = gst_riff_create_video_caps (vids->compression, NULL, vids,
          buf, NULL, codec_name);

      if (caps == NULL) {
        GST_WARNING ("Unhandled RIFF fourcc %" GST_FOURCC_FORMAT,
            GST_FOURCC_ARGS (vids->compression));
      } else {
        static GstStaticCaps intra_caps = GST_STATIC_CAPS ("image/jpeg; "
            "video/x-raw; image/png; video/x-dv; video/x-huffyuv; video/x-ffv; "
            "video/x-compressed-yuv");
        context->intra_only =
            gst_caps_can_intersect (gst_static_caps_get (&intra_caps), caps);
      }

      if (buf)
        gst_buffer_unref (buf);

      if (vids != (gst_riff_strf_vids *) data)
        g_free (vids);
    }
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_UNCOMPRESSED)) {
    GstVideoInfo info;
    GstVideoFormat format;

    gst_video_info_init (&info);
    switch (videocontext->fourcc) {
      case GST_MAKE_FOURCC ('I', '4', '2', '0'):
        format = GST_VIDEO_FORMAT_I420;
        break;
      case GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'):
        format = GST_VIDEO_FORMAT_YUY2;
        break;
      case GST_MAKE_FOURCC ('Y', 'V', '1', '2'):
        format = GST_VIDEO_FORMAT_YV12;
        break;
      case GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'):
        format = GST_VIDEO_FORMAT_UYVY;
        break;
      case GST_MAKE_FOURCC ('A', 'Y', 'U', 'V'):
        format = GST_VIDEO_FORMAT_AYUV;
        break;
      case GST_MAKE_FOURCC ('Y', '8', '0', '0'):
      case GST_MAKE_FOURCC ('Y', '8', ' ', ' '):
        format = GST_VIDEO_FORMAT_GRAY8;
        break;
      case GST_MAKE_FOURCC ('R', 'G', 'B', 24):
        format = GST_VIDEO_FORMAT_RGB;
        break;
      case GST_MAKE_FOURCC ('B', 'G', 'R', 24):
        format = GST_VIDEO_FORMAT_BGR;
        break;
      default:
        GST_DEBUG ("Unknown fourcc %" GST_FOURCC_FORMAT,
            GST_FOURCC_ARGS (videocontext->fourcc));
        return NULL;
    }

    context->intra_only = TRUE;

    gst_video_info_set_format (&info, format, videocontext->pixel_width,
        videocontext->pixel_height);
    caps = gst_video_info_to_caps (&info);
    *codec_name = gst_pb_utils_get_codec_description (caps);
    context->alignment = 32;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_SP)) {
    caps = gst_caps_new_simple ("video/x-divx",
        "divxversion", G_TYPE_INT, 4, NULL);
    *codec_name = g_strdup ("MPEG-4 simple profile");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_ASP) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_AP)) {
    caps = gst_caps_new_simple ("video/mpeg",
        "mpegversion", G_TYPE_INT, 4,
        "systemstream", G_TYPE_BOOLEAN, FALSE, NULL);
    if (data) {
      GstBuffer *priv;

      priv = gst_buffer_new_wrapped (g_memdup (data, size), size);
      gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
      gst_buffer_unref (priv);

      gst_codec_utils_mpeg4video_caps_set_level_and_profile (caps, data, size);
    }
    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_ASP))
      *codec_name = g_strdup ("MPEG-4 advanced simple profile");
    else
      *codec_name = g_strdup ("MPEG-4 advanced profile");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MSMPEG4V3)) {
#if 0
    caps = gst_caps_new_full (gst_structure_new ("video/x-divx",
            "divxversion", G_TYPE_INT, 3, NULL),
        gst_structure_new ("video/x-msmpeg",
            "msmpegversion", G_TYPE_INT, 43, NULL), NULL);
#endif
    caps = gst_caps_new_simple ("video/x-msmpeg",
        "msmpegversion", G_TYPE_INT, 43, NULL);
    *codec_name = g_strdup ("Microsoft MPEG-4 v.3");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG1) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG2)) {
    gint mpegversion;

    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG1))
      mpegversion = 1;
    else
      mpegversion = 2;

    caps = gst_caps_new_simple ("video/mpeg",
        "systemstream", G_TYPE_BOOLEAN, FALSE,
        "mpegversion", G_TYPE_INT, mpegversion, NULL);
    *codec_name = g_strdup_printf ("MPEG-%d video", mpegversion);
    context->postprocess_frame = gst_matroska_demux_add_mpeg_seq_header;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MJPEG)) {
    caps = gst_caps_new_empty_simple ("image/jpeg");
    *codec_name = g_strdup ("Motion-JPEG");
    context->intra_only = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_AVC)) {
    caps = gst_caps_new_empty_simple ("video/x-h264");
    if (data) {
      GstBuffer *priv;

      /* First byte is the version, second is the profile indication, and third
       * is the 5 contraint_set_flags and 3 reserved bits. Fourth byte is the
       * level indication. */
      gst_codec_utils_h264_caps_set_level_and_profile (caps, data + 1,
          size - 1);

      priv = gst_buffer_new_wrapped (g_memdup (data, size), size);
      gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
      gst_buffer_unref (priv);

      gst_caps_set_simple (caps, "stream-format", G_TYPE_STRING, "avc",
          "alignment", G_TYPE_STRING, "au", NULL);
    } else {
      GST_WARNING ("No codec data found, assuming output is byte-stream");
      gst_caps_set_simple (caps, "stream-format", G_TYPE_STRING, "byte-stream",
          NULL);
    }
    *codec_name = g_strdup ("H264");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEGH_HEVC)) {
    caps = gst_caps_new_empty_simple ("video/x-h265");
    if (data) {
      GstBuffer *priv;

      gst_codec_utils_h265_caps_set_level_tier_and_profile (caps, data + 1,
          size - 1);

      priv = gst_buffer_new_wrapped (g_memdup (data, size), size);
      gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
      gst_buffer_unref (priv);

      gst_caps_set_simple (caps, "stream-format", G_TYPE_STRING, "hvc1",
          "alignment", G_TYPE_STRING, "au", NULL);
    } else {
      GST_WARNING ("No codec data found, assuming output is byte-stream");
      gst_caps_set_simple (caps, "stream-format", G_TYPE_STRING, "byte-stream",
          NULL);
    }
    *codec_name = g_strdup ("HEVC");
  } else if ((!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO1)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO2)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO3)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO4))) {
    gint rmversion = -1;

    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO1))
      rmversion = 1;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO2))
      rmversion = 2;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO3))
      rmversion = 3;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO4))
      rmversion = 4;

    caps = gst_caps_new_simple ("video/x-pn-realvideo",
        "rmversion", G_TYPE_INT, rmversion, NULL);
    GST_DEBUG ("data:%p, size:0x%x", data, size);
    /* We need to extract the extradata ! */
    if (data && (size >= 0x22)) {
      GstBuffer *priv;
      guint rformat;
      guint subformat;

      subformat = GST_READ_UINT32_BE (data + 0x1a);
      rformat = GST_READ_UINT32_BE (data + 0x1e);

      priv =
          gst_buffer_new_wrapped (g_memdup (data + 0x1a, size - 0x1a),
          size - 0x1a);
      gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, "format",
          G_TYPE_INT, rformat, "subformat", G_TYPE_INT, subformat, NULL);
      gst_buffer_unref (priv);

    }
    *codec_name = g_strdup_printf ("RealVideo %d.0", rmversion);
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_THEORA)) {
    caps = gst_caps_new_empty_simple ("video/x-theora");
    context->stream_headers =
        gst_matroska_parse_xiph_stream_headers (context->codec_priv,
        context->codec_priv_size);
    /* FIXME: mark stream as broken and skip if there are no stream headers */
    context->send_stream_headers = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_DIRAC)) {
    caps = gst_caps_new_empty_simple ("video/x-dirac");
    *codec_name = g_strdup_printf ("Dirac");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_VP8)) {
    caps = gst_caps_new_empty_simple ("video/x-vp8");
    *codec_name = g_strdup_printf ("On2 VP8");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_VP9)) {
    caps = gst_caps_new_empty_simple ("video/x-vp9");
    *codec_name = g_strdup_printf ("On2 VP9");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_AV1)) {
    caps = gst_caps_new_empty_simple ("video/x-av1");
    if (data) {
      GstBuffer *priv;

      priv = gst_buffer_new_wrapped (g_memdup (data, size), size);
      gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
      gst_buffer_unref (priv);
    } else {
      GST_WARNING ("No AV1 codec data found!");
    }
    *codec_name = g_strdup_printf ("AOM AV1");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_PRORES)) {
    guint32 fourcc;
    const gchar *variant, *variant_descr = "";

    /* Expect a fourcc in the codec private data */
    if (!data || size < 4) {
      GST_WARNING ("No or too small PRORESS fourcc (%d bytes)", size);
      return NULL;
    }

    fourcc = GST_STR_FOURCC (data);
    switch (fourcc) {
      case GST_MAKE_FOURCC ('a', 'p', 'c', 's'):
        variant_descr = " 4:2:2 LT";
        variant = "lt";
        break;
      case GST_MAKE_FOURCC ('a', 'p', 'c', 'h'):
        variant = "hq";
        variant_descr = " 4:2:2 HQ";
        break;
      case GST_MAKE_FOURCC ('a', 'p', '4', 'h'):
        variant = "4444";
        variant_descr = " 4:4:4:4";
        break;
      case GST_MAKE_FOURCC ('a', 'p', 'c', 'o'):
        variant = "proxy";
        variant_descr = " 4:2:2 Proxy";
        break;
      case GST_MAKE_FOURCC ('a', 'p', 'c', 'n'):
      default:
        variant = "standard";
        variant_descr = " 4:2:2 SD";
        break;
    }

    GST_LOG ("Prores video, codec fourcc %" GST_FOURCC_FORMAT,
        GST_FOURCC_ARGS (fourcc));

    caps = gst_caps_new_simple ("video/x-prores",
        "format", G_TYPE_STRING, variant, NULL);
    *codec_name = g_strdup_printf ("Apple ProRes%s", variant_descr);
    context->postprocess_frame = gst_matroska_demux_add_prores_header;
  } else {
    GST_WARNING ("Unknown codec '%s', cannot build Caps", codec_id);
    return NULL;
  }

  if (caps != NULL) {
    int i;
    GstStructure *structure;

    for (i = 0; i < gst_caps_get_size (caps); i++) {
      structure = gst_caps_get_structure (caps, i);

      /* FIXME: use the real unit here! */
      GST_DEBUG ("video size %dx%d, target display size %dx%d (any unit)",
          videocontext->pixel_width,
          videocontext->pixel_height,
          videocontext->display_width, videocontext->display_height);

      /* pixel width and height are the w and h of the video in pixels */
      if (videocontext->pixel_width > 0 && videocontext->pixel_height > 0) {
        gint w = videocontext->pixel_width;
        gint h = videocontext->pixel_height;

        gst_structure_set (structure,
            "width", G_TYPE_INT, w, "height", G_TYPE_INT, h, NULL);
      }

      if (videocontext->display_width > 0 || videocontext->display_height > 0) {
        int n, d;

        if (videocontext->display_width <= 0)
          videocontext->display_width = videocontext->pixel_width;
        if (videocontext->display_height <= 0)
          videocontext->display_height = videocontext->pixel_height;

        /* calculate the pixel aspect ratio using the display and pixel w/h */
        n = videocontext->display_width * videocontext->pixel_height;
        d = videocontext->display_height * videocontext->pixel_width;
        GST_DEBUG ("setting PAR to %d/%d", n, d);
        gst_structure_set (structure, "pixel-aspect-ratio",
            GST_TYPE_FRACTION,
            videocontext->display_width * videocontext->pixel_height,
            videocontext->display_height * videocontext->pixel_width, NULL);
      }

      if (videocontext->default_fps > 0.0) {
        gint fps_n, fps_d;

        gst_util_double_to_fraction (videocontext->default_fps, &fps_n, &fps_d);

        GST_DEBUG ("using default fps %d/%d", fps_n, fps_d);

        gst_structure_set (structure, "framerate", GST_TYPE_FRACTION, fps_n,
            fps_d, NULL);
      } else if (context->default_duration > 0) {
        int fps_n, fps_d;

        gst_video_guess_framerate (context->default_duration, &fps_n, &fps_d);

        GST_INFO ("using default duration %" G_GUINT64_FORMAT
            " framerate %d/%d", context->default_duration, fps_n, fps_d);

        gst_structure_set (structure, "framerate", GST_TYPE_FRACTION,
            fps_n, fps_d, NULL);
      } else {
        gst_structure_set (structure, "framerate", GST_TYPE_FRACTION,
            0, 1, NULL);
      }

      switch (videocontext->interlace_mode) {
        case GST_MATROSKA_INTERLACE_MODE_PROGRESSIVE:
          gst_structure_set (structure,
              "interlace-mode", G_TYPE_STRING, "progressive", NULL);
          break;
        case GST_MATROSKA_INTERLACE_MODE_INTERLACED:
          gst_structure_set (structure,
              "interlace-mode", G_TYPE_STRING, "interleaved", NULL);

          if (videocontext->field_order != GST_VIDEO_FIELD_ORDER_UNKNOWN)
            gst_structure_set (structure, "field-order", G_TYPE_STRING,
                gst_video_field_order_to_string (videocontext->field_order),
                NULL);
          break;
        default:
          break;
      }
    }
    if (videocontext->multiview_mode != GST_VIDEO_MULTIVIEW_MODE_NONE) {
      if (gst_video_multiview_guess_half_aspect (videocontext->multiview_mode,
              videocontext->pixel_width, videocontext->pixel_height,
              videocontext->display_width * videocontext->pixel_height,
              videocontext->display_height * videocontext->pixel_width)) {
        videocontext->multiview_flags |= GST_VIDEO_MULTIVIEW_FLAGS_HALF_ASPECT;
      }
      gst_caps_set_simple (caps,
          "multiview-mode", G_TYPE_STRING,
          gst_video_multiview_mode_to_caps_string
          (videocontext->multiview_mode), "multiview-flags",
          GST_TYPE_VIDEO_MULTIVIEW_FLAGSET, videocontext->multiview_flags,
          GST_FLAG_SET_MASK_EXACT, NULL);
    }

    if (videocontext->colorimetry.range != GST_VIDEO_COLOR_RANGE_UNKNOWN ||
        videocontext->colorimetry.matrix != GST_VIDEO_COLOR_MATRIX_UNKNOWN ||
        videocontext->colorimetry.transfer != GST_VIDEO_TRANSFER_UNKNOWN ||
        videocontext->colorimetry.primaries !=
        GST_VIDEO_COLOR_PRIMARIES_UNKNOWN) {
      gchar *colorimetry =
          gst_video_colorimetry_to_string (&videocontext->colorimetry);
      gst_caps_set_simple (caps, "colorimetry", G_TYPE_STRING, colorimetry,
          NULL);
      GST_DEBUG ("setting colorimetry to %s", colorimetry);
      g_free (colorimetry);
    }

    if (videocontext->mastering_display_info_present) {
      if (!gst_video_mastering_display_info_add_to_caps
          (&videocontext->mastering_display_info, caps)) {
        GST_WARNING ("couldn't set mastering display info to caps");
      }
    }

    if (videocontext->content_light_level.maxCLL_n &&
        videocontext->content_light_level.maxFALL_n) {
      if (!gst_video_content_light_level_add_to_caps
          (&videocontext->content_light_level, caps)) {
        GST_WARNING ("couldn't set content light level to caps");
      }
    }

    caps = gst_caps_simplify (caps);
  }

  return caps;
}

/*
 * Some AAC specific code... *sigh*
 * FIXME: maybe we should use '15' and code the sample rate explicitly
 * if the sample rate doesn't match the predefined rates exactly? (tpm)
 */

static gint
aac_rate_idx (gint rate)
{
  if (92017 <= rate)
    return 0;
  else if (75132 <= rate)
    return 1;
  else if (55426 <= rate)
    return 2;
  else if (46009 <= rate)
    return 3;
  else if (37566 <= rate)
    return 4;
  else if (27713 <= rate)
    return 5;
  else if (23004 <= rate)
    return 6;
  else if (18783 <= rate)
    return 7;
  else if (13856 <= rate)
    return 8;
  else if (11502 <= rate)
    return 9;
  else if (9391 <= rate)
    return 10;
  else
    return 11;
}

static gint
aac_profile_idx (const gchar * codec_id)
{
  gint profile;

  if (strlen (codec_id) <= 12)
    profile = 3;
  else if (!strncmp (&codec_id[12], "MAIN", 4))
    profile = 0;
  else if (!strncmp (&codec_id[12], "LC", 2))
    profile = 1;
  else if (!strncmp (&codec_id[12], "SSR", 3))
    profile = 2;
  else
    profile = 3;

  return profile;
}

static guint
round_up_pow2 (guint n)
{
  n = n - 1;
  n = n | (n >> 1);
  n = n | (n >> 2);
  n = n | (n >> 4);
  n = n | (n >> 8);
  n = n | (n >> 16);
  return n + 1;
}

#define AAC_SYNC_EXTENSION_TYPE 0x02b7

static GstCaps *
gst_matroska_demux_audio_caps (GstMatroskaTrackAudioContext *
    audiocontext, const gchar * codec_id, guint8 * data, guint size,
    gchar ** codec_name, guint16 * riff_audio_fmt, GstClockTime * lead_in_ts)
{
  GstMatroskaTrackContext *context = (GstMatroskaTrackContext *) audiocontext;
  GstCaps *caps = NULL;
  guint lead_in = 0;
  /* Max potential blocksize causing the longest possible lead_in_ts need, as
   * we don't have the exact number parsed out here */
  guint max_blocksize = 0;
  /* Original samplerate before SBR multiplications, as parsers would use */
  guint rate = audiocontext->samplerate;

  g_assert (audiocontext != NULL);
  g_assert (codec_name != NULL);

  if (riff_audio_fmt)
    *riff_audio_fmt = 0;

  /* TODO: check if we have all codec types from matroska-ids.h
   *       check if we have to do more special things with codec_private
   *       check if we need bitdepth in different places too
   *       implement channel position magic
   * Add support for:
   *  GST_MATROSKA_CODEC_ID_AUDIO_AC3_BSID9
   *  GST_MATROSKA_CODEC_ID_AUDIO_AC3_BSID10
   *  GST_MATROSKA_CODEC_ID_AUDIO_QUICKTIME_QDMC
   *  GST_MATROSKA_CODEC_ID_AUDIO_QUICKTIME_QDM2
   */

  if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L1) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L2) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L3)) {
    gint layer;

    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L1))
      layer = 1;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L2))
      layer = 2;
    else
      layer = 3;

    lead_in = 30;               /* Could mp2 need as much too? */
    max_blocksize = 1152;
    caps = gst_caps_new_simple ("audio/mpeg",
        "mpegversion", G_TYPE_INT, 1, "layer", G_TYPE_INT, layer, NULL);
    *codec_name = g_strdup_printf ("MPEG-1 layer %d", layer);
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_BE) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_LE)) {
    gboolean sign;
    gint endianness;
    GstAudioFormat format;

    sign = (audiocontext->bitdepth != 8);
    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_BE))
      endianness = G_BIG_ENDIAN;
    else
      endianness = G_LITTLE_ENDIAN;

    format = gst_audio_format_build_integer (sign, endianness,
        audiocontext->bitdepth, audiocontext->bitdepth);

    /* FIXME: Channel mask and reordering */
    caps = gst_caps_new_simple ("audio/x-raw",
        "format", G_TYPE_STRING, gst_audio_format_to_string (format),
        "layout", G_TYPE_STRING, "interleaved",
        "channel-mask", GST_TYPE_BITMASK,
        gst_audio_channel_get_fallback_mask (audiocontext->channels), NULL);

    *codec_name = g_strdup_printf ("Raw %d-bit PCM audio",
        audiocontext->bitdepth);
    context->alignment = GST_ROUND_UP_8 (audiocontext->bitdepth) / 8;
    context->alignment = round_up_pow2 (context->alignment);
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_PCM_FLOAT)) {
    const gchar *format;
    if (audiocontext->bitdepth == 32)
      format = "F32LE";
    else
      format = "F64LE";
    /* FIXME: Channel mask and reordering */
    caps = gst_caps_new_simple ("audio/x-raw",
        "format", G_TYPE_STRING, format,
        "layout", G_TYPE_STRING, "interleaved",
        "channel-mask", GST_TYPE_BITMASK,
        gst_audio_channel_get_fallback_mask (audiocontext->channels), NULL);
    *codec_name = g_strdup_printf ("Raw %d-bit floating-point audio",
        audiocontext->bitdepth);
    context->alignment = audiocontext->bitdepth / 8;
    context->alignment = round_up_pow2 (context->alignment);
  } else if (!strncmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AC3,
          strlen (GST_MATROSKA_CODEC_ID_AUDIO_AC3))) {
    lead_in = 2;
    max_blocksize = 1536;
    caps = gst_caps_new_simple ("audio/x-ac3",
        "framed", G_TYPE_BOOLEAN, TRUE, NULL);
    *codec_name = g_strdup ("AC-3 audio");
  } else if (!strncmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_EAC3,
          strlen (GST_MATROSKA_CODEC_ID_AUDIO_EAC3))) {
    lead_in = 2;
    max_blocksize = 1536;
    caps = gst_caps_new_simple ("audio/x-eac3",
        "framed", G_TYPE_BOOLEAN, TRUE, NULL);
    *codec_name = g_strdup ("E-AC-3 audio");
  } else if (!strncmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_TRUEHD,
          strlen (GST_MATROSKA_CODEC_ID_AUDIO_TRUEHD))) {
    caps = gst_caps_new_empty_simple ("audio/x-true-hd");
    *codec_name = g_strdup ("Dolby TrueHD");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_DTS)) {
    caps = gst_caps_new_empty_simple ("audio/x-dts");
    *codec_name = g_strdup ("DTS audio");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_VORBIS)) {
    caps = gst_caps_new_empty_simple ("audio/x-vorbis");
    context->stream_headers =
        gst_matroska_parse_xiph_stream_headers (context->codec_priv,
        context->codec_priv_size);
    /* FIXME: mark stream as broken and skip if there are no stream headers */
    context->send_stream_headers = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_FLAC)) {
    caps = gst_caps_new_empty_simple ("audio/x-flac");
    context->stream_headers =
        gst_matroska_parse_flac_stream_headers (context->codec_priv,
        context->codec_priv_size);
    /* FIXME: mark stream as broken and skip if there are no stream headers */
    context->send_stream_headers = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_SPEEX)) {
    caps = gst_caps_new_empty_simple ("audio/x-speex");
    context->stream_headers =
        gst_matroska_parse_speex_stream_headers (context->codec_priv,
        context->codec_priv_size);
    /* FIXME: mark stream as broken and skip if there are no stream headers */
    context->send_stream_headers = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_OPUS)) {
    GstBuffer *tmp;

    if (context->codec_priv_size >= 19) {
      if (audiocontext->samplerate)
        GST_WRITE_UINT32_LE ((guint8 *) context->codec_priv + 12,
            audiocontext->samplerate);
      if (context->codec_delay) {
        guint64 delay =
            gst_util_uint64_scale_round (context->codec_delay, 48000,
            GST_SECOND);
        GST_WRITE_UINT16_LE ((guint8 *) context->codec_priv + 10, delay);
      }

      tmp =
          gst_buffer_new_wrapped (g_memdup (context->codec_priv,
              context->codec_priv_size), context->codec_priv_size);
      caps = gst_codec_utils_opus_create_caps_from_header (tmp, NULL);
      gst_buffer_unref (tmp);
      *codec_name = g_strdup ("Opus");
    } else if (context->codec_priv_size == 0) {
      GST_WARNING ("No Opus codec data found, trying to create one");
      if (audiocontext->channels <= 2) {
        guint8 streams, coupled, channels;
        guint32 samplerate;

        samplerate =
            audiocontext->samplerate == 0 ? 48000 : audiocontext->samplerate;
        rate = samplerate;
        channels = audiocontext->channels == 0 ? 2 : audiocontext->channels;
        if (channels == 1) {
          streams = 1;
          coupled = 0;
        } else {
          streams = 1;
          coupled = 1;
        }

        caps =
            gst_codec_utils_opus_create_caps (samplerate, channels, 0, streams,
            coupled, NULL);
        if (caps) {
          *codec_name = g_strdup ("Opus");
        } else {
          GST_WARNING ("Failed to create Opus caps from audio context");
        }
      } else {
        GST_WARNING ("No Opus codec data, and not enough info to create one");
      }
    } else {
      GST_WARNING ("Invalid Opus codec data size (got %" G_GSIZE_FORMAT
          ", expected 19)", context->codec_priv_size);
    }
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_ACM)) {
    gst_riff_strf_auds auds;

    if (data && size >= 18) {
      GstBuffer *codec_data = NULL;

      /* little-endian -> byte-order */
      auds.format = GST_READ_UINT16_LE (data);
      auds.channels = GST_READ_UINT16_LE (data + 2);
      auds.rate = GST_READ_UINT32_LE (data + 4);
      auds.av_bps = GST_READ_UINT32_LE (data + 8);
      auds.blockalign = GST_READ_UINT16_LE (data + 12);
      auds.bits_per_sample = GST_READ_UINT16_LE (data + 16);

      /* 18 is the waveformatex size */
      if (size > 18) {
        codec_data = gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
            data + 18, size - 18, 0, size - 18, NULL, NULL);
      }

      if (riff_audio_fmt)
        *riff_audio_fmt = auds.format;

      /* FIXME: Handle reorder map */
      caps = gst_riff_create_audio_caps (auds.format, NULL, &auds, codec_data,
          NULL, codec_name, NULL);
      if (codec_data)
        gst_buffer_unref (codec_data);

      if (caps == NULL) {
        GST_WARNING ("Unhandled RIFF audio format 0x%02x", auds.format);
      }
    } else {
      GST_WARNING ("Invalid codec data size (%d expected, got %d)", 18, size);
    }
  } else if (g_str_has_prefix (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AAC)) {
    GstBuffer *priv = NULL;
    gint mpegversion;
    gint rate_idx, profile;
    guint8 *data = NULL;

    /* unspecified AAC profile with opaque private codec data */
    if (strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AAC) == 0) {
      if (context->codec_priv_size >= 2) {
        guint obj_type, freq_index, explicit_freq_bytes = 0;

        codec_id = GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG4;
        mpegversion = 4;
        freq_index = (GST_READ_UINT16_BE (context->codec_priv) & 0x780) >> 7;
        obj_type = (GST_READ_UINT16_BE (context->codec_priv) & 0xF800) >> 11;
        if (freq_index == 15)
          explicit_freq_bytes = 3;
        GST_DEBUG ("obj_type = %u, freq_index = %u", obj_type, freq_index);
        priv = gst_buffer_new_wrapped (g_memdup (context->codec_priv,
                context->codec_priv_size), context->codec_priv_size);
        /* assume SBR if samplerate <= 24kHz */
        if (obj_type == 5 || (freq_index >= 6 && freq_index != 15) ||
            (context->codec_priv_size == (5 + explicit_freq_bytes))) {
          /* TODO: Commonly aacparse will reset the rate in caps to
           * non-multiplied - which one is correct? */
          audiocontext->samplerate *= 2;
        }
      } else {
        GST_WARNING ("Opaque A_AAC codec ID, but no codec private data");
        /* this is pretty broken;
         * maybe we need to make up some default private,
         * or maybe ADTS data got dumped in.
         * Let's set up some private data now, and check actual data later */
        /* just try this and see what happens ... */
        codec_id = GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG4;
        context->postprocess_frame = gst_matroska_demux_check_aac;
      }
    }

    /* make up decoder-specific data if it is not supplied */
    if (priv == NULL) {
      GstMapInfo map;

      priv = gst_buffer_new_allocate (NULL, 5, NULL);
      gst_buffer_map (priv, &map, GST_MAP_WRITE);
      data = map.data;
      rate_idx = aac_rate_idx (audiocontext->samplerate);
      profile = aac_profile_idx (codec_id);

      data[0] = ((profile + 1) << 3) | ((rate_idx & 0xE) >> 1);
      data[1] = ((rate_idx & 0x1) << 7) | (audiocontext->channels << 3);

      if (!strncmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG2,
              strlen (GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG2))) {
        mpegversion = 2;
        gst_buffer_unmap (priv, &map);
        gst_buffer_set_size (priv, 2);
      } else if (!strncmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG4,
              strlen (GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG4))) {
        mpegversion = 4;

        if (g_strrstr (codec_id, "SBR")) {
          /* HE-AAC (aka SBR AAC) */
          audiocontext->samplerate *= 2;
          rate_idx = aac_rate_idx (audiocontext->samplerate);
          data[2] = AAC_SYNC_EXTENSION_TYPE >> 3;
          data[3] = ((AAC_SYNC_EXTENSION_TYPE & 0x07) << 5) | 5;
          data[4] = (1 << 7) | (rate_idx << 3);
          gst_buffer_unmap (priv, &map);
        } else {
          gst_buffer_unmap (priv, &map);
          gst_buffer_set_size (priv, 2);
        }
      } else {
        gst_buffer_unmap (priv, &map);
        gst_buffer_unref (priv);
        priv = NULL;
        GST_ERROR ("Unknown AAC profile and no codec private data");
      }
    }

    if (priv) {
      lead_in = 2;
      max_blocksize = 1024;
      caps = gst_caps_new_simple ("audio/mpeg",
          "mpegversion", G_TYPE_INT, mpegversion,
          "framed", G_TYPE_BOOLEAN, TRUE,
          "stream-format", G_TYPE_STRING, "raw", NULL);
      gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
      if (context->codec_priv && context->codec_priv_size > 0)
        gst_codec_utils_aac_caps_set_level_and_profile (caps,
            context->codec_priv, context->codec_priv_size);
      *codec_name = g_strdup_printf ("MPEG-%d AAC audio", mpegversion);
      gst_buffer_unref (priv);
    }
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_TTA)) {
    caps = gst_caps_new_simple ("audio/x-tta",
        "width", G_TYPE_INT, audiocontext->bitdepth, NULL);
    *codec_name = g_strdup ("TTA audio");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_WAVPACK4)) {
    caps = gst_caps_new_simple ("audio/x-wavpack",
        "width", G_TYPE_INT, audiocontext->bitdepth,
        "framed", G_TYPE_BOOLEAN, TRUE, NULL);
    *codec_name = g_strdup ("Wavpack audio");
    context->postprocess_frame = gst_matroska_demux_add_wvpk_header;
    audiocontext->wvpk_block_index = 0;
  } else if ((!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_14_4)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_28_8)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_COOK))) {
    gint raversion = -1;

    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_14_4))
      raversion = 1;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_COOK))
      raversion = 8;
    else
      raversion = 2;

    caps = gst_caps_new_simple ("audio/x-pn-realaudio",
        "raversion", G_TYPE_INT, raversion, NULL);
    /* Extract extra information from caps, mapping varies based on codec */
    if (data && (size >= 0x50)) {
      GstBuffer *priv;
      guint flavor;
      guint packet_size;
      guint height;
      guint leaf_size;
      guint sample_width;
      guint extra_data_size;

      GST_DEBUG ("real audio raversion:%d", raversion);
      if (raversion == 8) {
        /* COOK */
        flavor = GST_READ_UINT16_BE (data + 22);
        packet_size = GST_READ_UINT32_BE (data + 24);
        height = GST_READ_UINT16_BE (data + 40);
        leaf_size = GST_READ_UINT16_BE (data + 44);
        sample_width = GST_READ_UINT16_BE (data + 58);
        extra_data_size = GST_READ_UINT32_BE (data + 74);

        GST_DEBUG
            ("flavor:%d, packet_size:%d, height:%d, leaf_size:%d, sample_width:%d, extra_data_size:%d",
            flavor, packet_size, height, leaf_size, sample_width,
            extra_data_size);
        gst_caps_set_simple (caps, "flavor", G_TYPE_INT, flavor, "packet_size",
            G_TYPE_INT, packet_size, "height", G_TYPE_INT, height, "leaf_size",
            G_TYPE_INT, leaf_size, "width", G_TYPE_INT, sample_width, NULL);

        if ((size - 78) >= extra_data_size) {
          priv = gst_buffer_new_wrapped (g_memdup (data + 78, extra_data_size),
              extra_data_size);
          gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
          gst_buffer_unref (priv);
        }
      }
    }

    *codec_name = g_strdup_printf ("RealAudio %d.0", raversion);
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_SIPR)) {
    caps = gst_caps_new_empty_simple ("audio/x-sipro");
    *codec_name = g_strdup ("Sipro/ACELP.NET Voice Codec");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_RALF)) {
    caps = gst_caps_new_empty_simple ("audio/x-ralf-mpeg4-generic");
    *codec_name = g_strdup ("Real Audio Lossless");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_ATRC)) {
    caps = gst_caps_new_empty_simple ("audio/x-vnd.sony.atrac3");
    *codec_name = g_strdup ("Sony ATRAC3");
  } else {
    GST_WARNING ("Unknown codec '%s', cannot build Caps", codec_id);
    return NULL;
  }

  if (caps != NULL) {
    if (audiocontext->samplerate > 0 && audiocontext->channels > 0) {
      gint i;

      for (i = 0; i < gst_caps_get_size (caps); i++) {
        gst_structure_set (gst_caps_get_structure (caps, i),
            "channels", G_TYPE_INT, audiocontext->channels,
            "rate", G_TYPE_INT, audiocontext->samplerate, NULL);
      }
    }

    caps = gst_caps_simplify (caps);
  }

  if (lead_in_ts && lead_in && max_blocksize && rate) {
    *lead_in_ts =
        gst_util_uint64_scale (GST_SECOND, max_blocksize * lead_in, rate);
  }

  return caps;
}

static GstCaps *
gst_matroska_demux_subtitle_caps (GstMatroskaTrackSubtitleContext *
    subtitlecontext, const gchar * codec_id, gpointer data, guint size)
{
  GstCaps *caps = NULL;
  GstMatroskaTrackContext *context =
      (GstMatroskaTrackContext *) subtitlecontext;

  /* for backwards compatibility */
  if (!g_ascii_strcasecmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_ASCII))
    codec_id = GST_MATROSKA_CODEC_ID_SUBTITLE_UTF8;
  else if (!g_ascii_strcasecmp (codec_id, "S_SSA"))
    codec_id = GST_MATROSKA_CODEC_ID_SUBTITLE_SSA;
  else if (!g_ascii_strcasecmp (codec_id, "S_ASS"))
    codec_id = GST_MATROSKA_CODEC_ID_SUBTITLE_ASS;
  else if (!g_ascii_strcasecmp (codec_id, "S_USF"))
    codec_id = GST_MATROSKA_CODEC_ID_SUBTITLE_USF;

  /* TODO: Add GST_MATROSKA_CODEC_ID_SUBTITLE_BMP support
   * Check if we have to do something with codec_private */
  if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_UTF8)) {
    /* well, plain text simply does not have a lot of markup ... */
    caps = gst_caps_new_simple ("text/x-raw", "format", G_TYPE_STRING,
        "pango-markup", NULL);
    context->postprocess_frame = gst_matroska_demux_check_subtitle_buffer;
    subtitlecontext->check_markup = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_SSA)) {
    caps = gst_caps_new_empty_simple ("application/x-ssa");
    context->postprocess_frame = gst_matroska_demux_check_subtitle_buffer;
    subtitlecontext->check_markup = FALSE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_ASS)) {
    caps = gst_caps_new_empty_simple ("application/x-ass");
    context->postprocess_frame = gst_matroska_demux_check_subtitle_buffer;
    subtitlecontext->check_markup = FALSE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_USF)) {
    caps = gst_caps_new_empty_simple ("application/x-usf");
    context->postprocess_frame = gst_matroska_demux_check_subtitle_buffer;
    subtitlecontext->check_markup = FALSE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_VOBSUB)) {
    caps = gst_caps_new_empty_simple ("subpicture/x-dvd");
    ((GstMatroskaTrackContext *) subtitlecontext)->send_dvd_event = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_HDMVPGS)) {
    caps = gst_caps_new_empty_simple ("subpicture/x-pgs");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_KATE)) {
    caps = gst_caps_new_empty_simple ("subtitle/x-kate");
    context->stream_headers =
        gst_matroska_parse_xiph_stream_headers (context->codec_priv,
        context->codec_priv_size);
    /* FIXME: mark stream as broken and skip if there are no stream headers */
    context->send_stream_headers = TRUE;
  } else {
    GST_DEBUG ("Unknown subtitle stream: codec_id='%s'", codec_id);
    caps = gst_caps_new_empty_simple ("application/x-subtitle-unknown");
  }

  if (data != NULL && size > 0) {
    GstBuffer *buf;

    buf = gst_buffer_new_wrapped (g_memdup (data, size), size);
    gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, buf, NULL);
    gst_buffer_unref (buf);
  }

  return caps;
}

#if 0
static void
gst_matroska_demux_set_index (GstElement * element, GstIndex * index)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);

  GST_OBJECT_LOCK (demux);
  if (demux->common.element_index)
    gst_object_unref (demux->common.element_index);
  demux->common.element_index = index ? gst_object_ref (index) : NULL;
  GST_OBJECT_UNLOCK (demux);
  GST_DEBUG_OBJECT (demux, "Set index %" GST_PTR_FORMAT,
      demux->common.element_index);
}

static GstIndex *
gst_matroska_demux_get_index (GstElement * element)
{
  GstIndex *result = NULL;
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);

  GST_OBJECT_LOCK (demux);
  if (demux->common.element_index)
    result = gst_object_ref (demux->common.element_index);
  GST_OBJECT_UNLOCK (demux);

  GST_DEBUG_OBJECT (demux, "Returning index %" GST_PTR_FORMAT, result);

  return result;
}
#endif

static GstStateChangeReturn
gst_matroska_demux_change_state (GstElement * element,
    GstStateChange transition)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  /* handle upwards state changes here */
  switch (transition) {
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  /* handle downwards state changes */
  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_matroska_demux_reset (GST_ELEMENT (demux));
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_matroska_demux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstMatroskaDemux *demux;

  g_return_if_fail (GST_IS_MATROSKA_DEMUX (object));
  demux = GST_MATROSKA_DEMUX (object);

  switch (prop_id) {
    case PROP_MAX_GAP_TIME:
      GST_OBJECT_LOCK (demux);
      demux->max_gap_time = g_value_get_uint64 (value);
      GST_OBJECT_UNLOCK (demux);
      break;
    case PROP_MAX_BACKTRACK_DISTANCE:
      GST_OBJECT_LOCK (demux);
      demux->max_backtrack_distance = g_value_get_uint (value);
      GST_OBJECT_UNLOCK (demux);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_matroska_demux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstMatroskaDemux *demux;

  g_return_if_fail (GST_IS_MATROSKA_DEMUX (object));
  demux = GST_MATROSKA_DEMUX (object);

  switch (prop_id) {
    case PROP_MAX_GAP_TIME:
      GST_OBJECT_LOCK (demux);
      g_value_set_uint64 (value, demux->max_gap_time);
      GST_OBJECT_UNLOCK (demux);
      break;
    case PROP_MAX_BACKTRACK_DISTANCE:
      GST_OBJECT_LOCK (demux);
      g_value_set_uint (value, demux->max_backtrack_distance);
      GST_OBJECT_UNLOCK (demux);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static const gchar *
gst_matroska_track_encryption_algorithm_name (gint val)
{
  GEnumValue *en;
  GEnumClass *enum_class =
      g_type_class_ref (MATROSKA_TRACK_ENCRYPTION_ALGORITHM_TYPE);
  en = g_enum_get_value (G_ENUM_CLASS (enum_class), val);
  return en ? en->value_nick : NULL;
}

static const gchar *
gst_matroska_track_encryption_cipher_mode_name (gint val)
{
  GEnumValue *en;
  GEnumClass *enum_class =
      g_type_class_ref (MATROSKA_TRACK_ENCRYPTION_CIPHER_MODE_TYPE);
  en = g_enum_get_value (G_ENUM_CLASS (enum_class), val);
  return en ? en->value_nick : NULL;
}

static const gchar *
gst_matroska_track_encoding_scope_name (gint val)
{
  GEnumValue *en;
  GEnumClass *enum_class =
      g_type_class_ref (MATROSKA_TRACK_ENCODING_SCOPE_TYPE);

  en = g_enum_get_value (G_ENUM_CLASS (enum_class), val);
  return en ? en->value_nick : NULL;
}

gboolean
gst_matroska_demux_plugin_init (GstPlugin * plugin)
{
  gst_riff_init ();

  /* parser helper separate debug */
  GST_DEBUG_CATEGORY_INIT (ebmlread_debug, "ebmlread",
      0, "EBML stream helper class");

  /* create an elementfactory for the matroska_demux element */
  if (!gst_element_register (plugin, "matroskademux",
          GST_RANK_PRIMARY, GST_TYPE_MATROSKA_DEMUX))
    return FALSE;

  return TRUE;
}
