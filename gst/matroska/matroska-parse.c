/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2006 Tim-Philipp Müller <tim centricular net>
 * (c) 2008 Sebastian Dröge <slomo@circular-chaos.org>
 * (c) 2011 Debarshi Ray <rishi@gnu.org>
 *
 * matroska-parse.c: matroska file/stream parser
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
 * TODO: check if parsing is done correct for all codecs according to spec
 * TODO: seeking with incomplete or without CUE
 */

/**
 * SECTION:element-matroskaparse
 * @title: matroskaparse
 *
 * matroskaparse parsees a Matroska file into the different contained streams.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v filesrc location=/path/to/mkv ! matroskaparse ! vorbisdec ! audioconvert ! audioresample ! autoaudiosink
 * ]| This pipeline parsees a Matroska file and outputs the contained Vorbis audio.
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>
#include <glib/gprintf.h>

/* For AVI compatibility mode
   and for fourcc stuff */
#include <gst/riff/riff-read.h>
#include <gst/riff/riff-ids.h>
#include <gst/riff/riff-media.h>

#include <gst/tag/tag.h>

#include <gst/pbutils/pbutils.h>

#include "matroska-parse.h"
#include "matroska-ids.h"

GST_DEBUG_CATEGORY_STATIC (matroskaparse_debug);
#define GST_CAT_DEFAULT matroskaparse_debug

#define DEBUG_ELEMENT_START(parse, ebml, element) \
    GST_DEBUG_OBJECT (parse, "Parsing " element " element at offset %" \
        G_GUINT64_FORMAT, gst_ebml_read_get_pos (ebml))

#define DEBUG_ELEMENT_STOP(parse, ebml, element, ret) \
    GST_DEBUG_OBJECT (parse, "Parsing " element " element " \
        " finished with '%s'", gst_flow_get_name (ret))

#define INVALID_DATA_THRESHOLD (2 * 1024 * 1024)

enum
{
  PROP_0
};

static GstStaticPadTemplate sink_templ = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-matroska; video/x-matroska; "
        "video/x-matroska-3d; audio/webm; video/webm")
    );

static GstStaticPadTemplate src_templ = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-matroska; video/x-matroska; "
        "video/x-matroska-3d; audio/webm; video/webm")
    );

static GstFlowReturn gst_matroska_parse_parse_id (GstMatroskaParse * parse,
    guint32 id, guint64 length, guint needed);

/* element functions */
//static void gst_matroska_parse_loop (GstPad * pad);

static gboolean gst_matroska_parse_element_send_event (GstElement * element,
    GstEvent * event);
static gboolean gst_matroska_parse_element_query (GstElement * element,
    GstQuery * query);

/* pad functions */
static gboolean gst_matroska_parse_handle_seek_event (GstMatroskaParse * parse,
    GstPad * pad, GstEvent * event);
static gboolean gst_matroska_parse_handle_src_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_matroska_parse_handle_src_query (GstPad * pad,
    GstObject * parent, GstQuery * query);

static gboolean gst_matroska_parse_handle_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static GstFlowReturn gst_matroska_parse_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);

static GstStateChangeReturn
gst_matroska_parse_change_state (GstElement * element,
    GstStateChange transition);
#if 0
static void
gst_matroska_parse_set_index (GstElement * element, GstIndex * index);
static GstIndex *gst_matroska_parse_get_index (GstElement * element);
#endif

/* stream methods */
static void gst_matroska_parse_reset (GstElement * element);
static gboolean perform_seek_to_offset (GstMatroskaParse * parse,
    guint64 offset);
static GstCaps *gst_matroska_parse_forge_caps (gboolean is_webm,
    gboolean has_video);

GType gst_matroska_parse_get_type (void);
#define parent_class gst_matroska_parse_parent_class
G_DEFINE_TYPE (GstMatroskaParse, gst_matroska_parse, GST_TYPE_ELEMENT);

static void
gst_matroska_parse_finalize (GObject * object)
{
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (object);

  gst_matroska_read_common_finalize (&parse->common);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_matroska_parse_class_init (GstMatroskaParseClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;

  GST_DEBUG_CATEGORY_INIT (matroskaparse_debug, "matroskaparse", 0,
      "Matroska parser");

  gobject_class->finalize = gst_matroska_parse_finalize;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_matroska_parse_change_state);
  gstelement_class->send_event =
      GST_DEBUG_FUNCPTR (gst_matroska_parse_element_send_event);
  gstelement_class->query =
      GST_DEBUG_FUNCPTR (gst_matroska_parse_element_query);

#if 0
  gstelement_class->set_index =
      GST_DEBUG_FUNCPTR (gst_matroska_parse_set_index);
  gstelement_class->get_index =
      GST_DEBUG_FUNCPTR (gst_matroska_parse_get_index);
#endif

  gst_element_class_add_static_pad_template (gstelement_class, &src_templ);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_templ);

  gst_element_class_set_static_metadata (gstelement_class,
      "Matroska parser", "Codec/Parser",
      "Parses Matroska/WebM streams into video/audio/subtitles",
      "GStreamer maintainers <gstreamer-devel@lists.freedesktop.org>");
}

static void
gst_matroska_parse_init (GstMatroskaParse * parse)
{
  parse->common.sinkpad = gst_pad_new_from_static_template (&sink_templ,
      "sink");
  gst_pad_set_chain_function (parse->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_parse_chain));
  gst_pad_set_event_function (parse->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_parse_handle_sink_event));
  gst_element_add_pad (GST_ELEMENT (parse), parse->common.sinkpad);

  parse->srcpad = gst_pad_new_from_static_template (&src_templ, "src");
  gst_pad_set_event_function (parse->srcpad,
      GST_DEBUG_FUNCPTR (gst_matroska_parse_handle_src_event));
  gst_pad_set_query_function (parse->srcpad,
      GST_DEBUG_FUNCPTR (gst_matroska_parse_handle_src_query));
  gst_pad_use_fixed_caps (parse->srcpad);

  gst_element_add_pad (GST_ELEMENT (parse), parse->srcpad);

  /* init defaults for common read context */
  gst_matroska_read_common_init (&parse->common);

  GST_OBJECT_FLAG_SET (parse, GST_ELEMENT_FLAG_INDEXABLE);

  /* finish off */
  gst_matroska_parse_reset (GST_ELEMENT (parse));
}

static void
gst_matroska_parse_reset (GstElement * element)
{
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (element);

  GST_DEBUG_OBJECT (parse, "Resetting state");

  gst_matroska_read_common_reset (GST_ELEMENT (parse), &parse->common);

  parse->num_a_streams = 0;
  parse->num_t_streams = 0;
  parse->num_v_streams = 0;

  parse->clock = NULL;
  parse->tracks_parsed = FALSE;

  g_list_foreach (parse->seek_parsed,
      (GFunc) gst_matroska_read_common_free_parsed_el, NULL);
  g_list_free (parse->seek_parsed);
  parse->seek_parsed = NULL;

  parse->last_stop_end = GST_CLOCK_TIME_NONE;
  parse->seek_block = 0;
  parse->cluster_time = GST_CLOCK_TIME_NONE;
  parse->cluster_offset = 0;
  parse->next_cluster_offset = 0;
  parse->index_offset = 0;
  parse->seekable = FALSE;
  parse->need_newsegment = TRUE;
  parse->building_index = FALSE;
  if (parse->seek_event) {
    gst_event_unref (parse->seek_event);
    parse->seek_event = NULL;
  }

  parse->seek_index = NULL;
  parse->seek_entry = 0;

  if (parse->close_segment) {
    gst_event_unref (parse->close_segment);
    parse->close_segment = NULL;
  }

  if (parse->new_segment) {
    gst_event_unref (parse->new_segment);
    parse->new_segment = NULL;
  }

  if (parse->streamheader != NULL) {
    gst_buffer_unref (parse->streamheader);
    parse->streamheader = NULL;
  }
}

static GstFlowReturn
gst_matroska_parse_add_stream (GstMatroskaParse * parse, GstEbmlRead * ebml)
{
  GstMatroskaTrackContext *context;
  GstFlowReturn ret;
  guint32 id;

  DEBUG_ELEMENT_START (parse, ebml, "TrackEntry");

  /* start with the master */
  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (parse, ebml, "TrackEntry", ret);
    return ret;
  }

  /* allocate generic... if we know the type, we'll g_renew()
   * with the precise type */
  context = g_new0 (GstMatroskaTrackContext, 1);
  g_ptr_array_add (parse->common.src, context);
  context->index = parse->common.num_streams;
  context->index_writer_id = -1;
  context->type = 0;            /* no type yet */
  context->default_duration = 0;
  context->pos = 0;
  context->set_discont = TRUE;
  context->timecodescale = 1.0;
  context->flags =
      GST_MATROSKA_TRACK_ENABLED | GST_MATROSKA_TRACK_DEFAULT |
      GST_MATROSKA_TRACK_LACING;
  context->to_offset = G_MAXINT64;
  context->alignment = 1;
  parse->common.num_streams++;
  g_assert (parse->common.src->len == parse->common.num_streams);

  GST_DEBUG_OBJECT (parse, "Stream number %d", context->index);

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
          GST_ERROR_OBJECT (parse, "Invalid TrackNumber 0");
          ret = GST_FLOW_ERROR;
          break;
        } else if (!gst_matroska_read_common_tracknumber_unique (&parse->common,
                num)) {
          GST_ERROR_OBJECT (parse, "TrackNumber %" G_GUINT64_FORMAT
              " is not unique", num);
          ret = GST_FLOW_ERROR;
          break;
        }

        GST_DEBUG_OBJECT (parse, "TrackNumber: %" G_GUINT64_FORMAT, num);
        context->num = num;
        break;
      }
        /* track UID (unique identifier) */
      case GST_MATROSKA_ID_TRACKUID:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num == 0) {
          GST_ERROR_OBJECT (parse, "Invalid TrackUID 0");
          ret = GST_FLOW_ERROR;
          break;
        }

        GST_DEBUG_OBJECT (parse, "TrackUID: %" G_GUINT64_FORMAT, num);
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
          GST_WARNING_OBJECT (parse,
              "More than one tracktype defined in a TrackEntry - skipping");
          break;
        } else if (track_type < 1 || track_type > 254) {
          GST_WARNING_OBJECT (parse, "Invalid TrackType %" G_GUINT64_FORMAT,
              track_type);
          break;
        }

        GST_DEBUG_OBJECT (parse, "TrackType: %" G_GUINT64_FORMAT, track_type);

        /* ok, so we're actually going to reallocate this thing */
        switch (track_type) {
          case GST_MATROSKA_TRACK_TYPE_VIDEO:
            gst_matroska_track_init_video_context (&context);
            parse->common.has_video = TRUE;
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
            GST_WARNING_OBJECT (parse,
                "Unknown or unsupported TrackType %" G_GUINT64_FORMAT,
                track_type);
            context->type = 0;
            break;
        }
        g_ptr_array_index (parse->common.src, parse->common.num_streams - 1)
            = context;
        break;
      }

        /* tracktype specific stuff for video */
      case GST_MATROSKA_ID_TRACKVIDEO:{
        GstMatroskaTrackVideoContext *videocontext;

        DEBUG_ELEMENT_START (parse, ebml, "TrackVideo");

        if (!gst_matroska_track_init_video_context (&context)) {
          GST_WARNING_OBJECT (parse,
              "TrackVideo element in non-video track - ignoring track");
          ret = GST_FLOW_ERROR;
          break;
        } else if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
          break;
        }
        videocontext = (GstMatroskaTrackVideoContext *) context;
        g_ptr_array_index (parse->common.src, parse->common.num_streams - 1)
            = context;

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
                GST_WARNING_OBJECT (parse, "Invalid TrackDefaultDuration 0");
                break;
              }

              GST_DEBUG_OBJECT (parse,
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
                GST_WARNING_OBJECT (parse, "Invalid TrackVideoFPS %lf", num);
                break;
              }

              GST_DEBUG_OBJECT (parse, "TrackVideoFrameRate: %lf", num);
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
                GST_WARNING_OBJECT (parse, "Invalid TrackVideoDisplayWidth 0");
                break;
              }

              GST_DEBUG_OBJECT (parse,
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
                GST_WARNING_OBJECT (parse, "Invalid TrackVideoDisplayHeight 0");
                break;
              }

              GST_DEBUG_OBJECT (parse,
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
                GST_WARNING_OBJECT (parse, "Invalid TrackVideoPixelWidth 0");
                break;
              }

              GST_DEBUG_OBJECT (parse,
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
                GST_WARNING_OBJECT (parse, "Invalid TrackVideoPixelHeight 0");
                break;
              }

              GST_DEBUG_OBJECT (parse,
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

              GST_DEBUG_OBJECT (parse, "video track interlacing mode: %d",
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
                GST_WARNING_OBJECT (parse,
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
                GST_FIXME_OBJECT (parse,
                    "Unknown or unsupported FieldOrder %" G_GUINT64_FORMAT,
                    num);
                videocontext->field_order = GST_VIDEO_FIELD_ORDER_UNKNOWN;
              }

              GST_DEBUG_OBJECT (parse, "video track field order: %d",
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
                GST_WARNING_OBJECT (parse,
                    "Unknown TrackVideoAspectRatioType 0x%x", (guint) num);
                break;
              }
              GST_DEBUG_OBJECT (parse,
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
                GST_WARNING_OBJECT (parse,
                    "Invalid TrackVideoColourSpace length %" G_GUINT64_FORMAT,
                    datalen);
                break;
              }

              memcpy (&videocontext->fourcc, data, 4);
              GST_DEBUG_OBJECT (parse,
                  "TrackVideoColourSpace: %" GST_FOURCC_FORMAT,
                  GST_FOURCC_ARGS (videocontext->fourcc));
              g_free (data);
              break;
            }

            default:
              GST_WARNING_OBJECT (parse,
                  "Unknown TrackVideo subelement 0x%x - ignoring", id);
              /* fall through */
            case GST_MATROSKA_ID_VIDEOSTEREOMODE:
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

        DEBUG_ELEMENT_STOP (parse, ebml, "TrackVideo", ret);
        break;
      }

        /* tracktype specific stuff for audio */
      case GST_MATROSKA_ID_TRACKAUDIO:{
        GstMatroskaTrackAudioContext *audiocontext;

        DEBUG_ELEMENT_START (parse, ebml, "TrackAudio");

        if (!gst_matroska_track_init_audio_context (&context)) {
          GST_WARNING_OBJECT (parse,
              "TrackAudio element in non-audio track - ignoring track");
          ret = GST_FLOW_ERROR;
          break;
        }

        if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK)
          break;

        audiocontext = (GstMatroskaTrackAudioContext *) context;
        g_ptr_array_index (parse->common.src, parse->common.num_streams - 1)
            = context;

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
                GST_WARNING_OBJECT (parse,
                    "Invalid TrackAudioSamplingFrequency %lf", num);
                break;
              }

              GST_DEBUG_OBJECT (parse, "TrackAudioSamplingFrequency: %lf", num);
              audiocontext->samplerate = num;
              break;
            }

              /* bitdepth */
            case GST_MATROSKA_ID_AUDIOBITDEPTH:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (parse, "Invalid TrackAudioBitDepth 0");
                break;
              }

              GST_DEBUG_OBJECT (parse, "TrackAudioBitDepth: %" G_GUINT64_FORMAT,
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
                GST_WARNING_OBJECT (parse, "Invalid TrackAudioChannels 0");
                break;
              }

              GST_DEBUG_OBJECT (parse, "TrackAudioChannels: %" G_GUINT64_FORMAT,
                  num);
              audiocontext->channels = num;
              break;
            }

            default:
              GST_WARNING_OBJECT (parse,
                  "Unknown TrackAudio subelement 0x%x - ignoring", id);
              /* fall through */
            case GST_MATROSKA_ID_AUDIOCHANNELPOSITIONS:
            case GST_MATROSKA_ID_AUDIOOUTPUTSAMPLINGFREQ:
              ret = gst_ebml_read_skip (ebml);
              break;
          }
        }

        DEBUG_ELEMENT_STOP (parse, ebml, "TrackAudio", ret);

        break;
      }

        /* codec identifier */
      case GST_MATROSKA_ID_CODECID:{
        gchar *text;

        if ((ret = gst_ebml_read_ascii (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (parse, "CodecID: %s", GST_STR_NULL (text));
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

        GST_DEBUG_OBJECT (parse, "CodecPrivate of size %" G_GUINT64_FORMAT,
            size);
        break;
      }

        /* name of the codec */
      case GST_MATROSKA_ID_CODECNAME:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (parse, "CodecName: %s", GST_STR_NULL (text));
        context->codec_name = text;
        break;
      }

        /* name of this track */
      case GST_MATROSKA_ID_TRACKNAME:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        context->name = text;
        GST_DEBUG_OBJECT (parse, "TrackName: %s", GST_STR_NULL (text));
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

        GST_DEBUG_OBJECT (parse, "TrackLanguage: %s",
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

        GST_DEBUG_OBJECT (parse, "TrackEnabled: %d",
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

        GST_DEBUG_OBJECT (parse, "TrackDefault: %d",
            (context->flags & GST_MATROSKA_TRACK_ENABLED) ? 1 : 0);
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

        GST_DEBUG_OBJECT (parse, "TrackForced: %d",
            (context->flags & GST_MATROSKA_TRACK_ENABLED) ? 1 : 0);
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

        GST_DEBUG_OBJECT (parse, "TrackLacing: %d",
            (context->flags & GST_MATROSKA_TRACK_ENABLED) ? 1 : 0);
        break;
      }

        /* default length (in time) of one data block in this track */
      case GST_MATROSKA_ID_TRACKDEFAULTDURATION:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;


        if (num == 0) {
          GST_WARNING_OBJECT (parse, "Invalid TrackDefaultDuration 0");
          break;
        }

        GST_DEBUG_OBJECT (parse, "TrackDefaultDuration: %" G_GUINT64_FORMAT,
            num);
        context->default_duration = num;
        break;
      }

      case GST_MATROSKA_ID_CONTENTENCODINGS:{
        ret = gst_matroska_read_common_read_track_encodings (&parse->common,
            ebml, context);
        break;
      }

      case GST_MATROSKA_ID_TRACKTIMECODESCALE:{
        gdouble num;

        if ((ret = gst_ebml_read_float (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num <= 0.0) {
          GST_WARNING_OBJECT (parse, "Invalid TrackTimeCodeScale %lf", num);
          break;
        }

        GST_DEBUG_OBJECT (parse, "TrackTimeCodeScale: %lf", num);
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

  DEBUG_ELEMENT_STOP (parse, ebml, "TrackEntry", ret);

  /* Decode codec private data if necessary */
  if (context->encodings && context->encodings->len > 0 && context->codec_priv
      && context->codec_priv_size > 0) {
    if (!gst_matroska_decode_data (context->encodings,
            &context->codec_priv, &context->codec_priv_size,
            GST_MATROSKA_TRACK_ENCODING_SCOPE_CODEC_DATA, TRUE)) {
      GST_WARNING_OBJECT (parse, "Decoding codec private data failed");
      ret = GST_FLOW_ERROR;
    }
  }

  if (context->type == 0 || context->codec_id == NULL || (ret != GST_FLOW_OK
          && ret != GST_FLOW_EOS)) {
    if (ret == GST_FLOW_OK || ret == GST_FLOW_EOS)
      GST_WARNING_OBJECT (ebml, "Unknown stream/codec in track entry header");

    parse->common.num_streams--;
    g_ptr_array_remove_index (parse->common.src, parse->common.num_streams);
    g_assert (parse->common.src->len == parse->common.num_streams);
    gst_matroska_track_free (context);

    return ret;
  }

  if ((context->language == NULL || *context->language == '\0') &&
      (context->type == GST_MATROSKA_TRACK_TYPE_AUDIO ||
          context->type == GST_MATROSKA_TRACK_TYPE_SUBTITLE)) {
    GST_LOG ("stream %d: language=eng (assuming default)", context->index);
    context->language = g_strdup ("eng");
  }


  /* tadaah! */
  return ret;
}

static gboolean
gst_matroska_parse_query (GstMatroskaParse * parse, GstPad * pad,
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

      if (format == GST_FORMAT_TIME) {
        GST_OBJECT_LOCK (parse);
        if (context)
          gst_query_set_position (query, GST_FORMAT_TIME, context->pos);
        else
          gst_query_set_position (query, GST_FORMAT_TIME,
              parse->common.segment.position);
        GST_OBJECT_UNLOCK (parse);
      } else if (format == GST_FORMAT_DEFAULT && context
          && context->default_duration) {
        GST_OBJECT_LOCK (parse);
        gst_query_set_position (query, GST_FORMAT_DEFAULT,
            context->pos / context->default_duration);
        GST_OBJECT_UNLOCK (parse);
      } else {
        GST_DEBUG_OBJECT (parse,
            "only position query in TIME and DEFAULT format is supported");
      }

      res = TRUE;
      break;
    }
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      if (format == GST_FORMAT_TIME) {
        GST_OBJECT_LOCK (parse);
        gst_query_set_duration (query, GST_FORMAT_TIME,
            parse->common.segment.duration);
        GST_OBJECT_UNLOCK (parse);
      } else if (format == GST_FORMAT_DEFAULT && context
          && context->default_duration) {
        GST_OBJECT_LOCK (parse);
        gst_query_set_duration (query, GST_FORMAT_DEFAULT,
            parse->common.segment.duration / context->default_duration);
        GST_OBJECT_UNLOCK (parse);
      } else {
        GST_DEBUG_OBJECT (parse,
            "only duration query in TIME and DEFAULT format is supported");
      }

      res = TRUE;
      break;
    }

    case GST_QUERY_SEEKING:
    {
      GstFormat fmt;

      gst_query_parse_seeking (query, &fmt, NULL, NULL, NULL);
      if (fmt == GST_FORMAT_TIME) {
        gboolean seekable;

        /* assuming we'll be able to get an index ... */
        seekable = parse->seekable;

        gst_query_set_seeking (query, GST_FORMAT_TIME, seekable,
            0, parse->common.segment.duration);
        res = TRUE;
      }
      break;
    }
    default:
      if (pad)
        res = gst_pad_query_default (pad, (GstObject *) parse, query);
      break;
  }

  return res;
}

static gboolean
gst_matroska_parse_element_query (GstElement * element, GstQuery * query)
{
  return gst_matroska_parse_query (GST_MATROSKA_PARSE (element), NULL, query);
}

static gboolean
gst_matroska_parse_handle_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  gboolean ret;
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (parent);

  ret = gst_matroska_parse_query (parse, pad, query);

  return ret;
}

static void
gst_matroska_parse_send_tags (GstMatroskaParse * parse)
{
  if (G_UNLIKELY (parse->common.global_tags_changed)) {
    GstEvent *tag_event;
    gst_tag_list_add (parse->common.global_tags, GST_TAG_MERGE_REPLACE,
        GST_TAG_CONTAINER_FORMAT, "Matroska", NULL);
    GST_DEBUG_OBJECT (parse, "Sending global_tags %p : %" GST_PTR_FORMAT,
        parse->common.global_tags, parse->common.global_tags);

    /* Send a copy as we want to keep our local ref writable to add more tags
     * if any are found */
    tag_event =
        gst_event_new_tag (gst_tag_list_copy (parse->common.global_tags));

    gst_pad_push_event (parse->srcpad, tag_event);

    parse->common.global_tags_changed = FALSE;
  }
}

/* returns FALSE if there are no pads to deliver event to,
 * otherwise TRUE (whatever the outcome of event sending),
 * takes ownership of the passed event! */
static gboolean
gst_matroska_parse_send_event (GstMatroskaParse * parse, GstEvent * event)
{
  gboolean ret = FALSE;

  g_return_val_if_fail (event != NULL, FALSE);

  GST_DEBUG_OBJECT (parse, "Sending event of type %s to all source pads",
      GST_EVENT_TYPE_NAME (event));

  gst_pad_push_event (parse->srcpad, event);

  return ret;
}

static gboolean
gst_matroska_parse_element_send_event (GstElement * element, GstEvent * event)
{
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (element);
  gboolean res;

  g_return_val_if_fail (event != NULL, FALSE);

  if (GST_EVENT_TYPE (event) == GST_EVENT_SEEK) {
    res = gst_matroska_parse_handle_seek_event (parse, NULL, event);
  } else {
    GST_WARNING_OBJECT (parse, "Unhandled event of type %s",
        GST_EVENT_TYPE_NAME (event));
    res = FALSE;
  }
  gst_event_unref (event);
  return res;
}

#if 0
/* searches for a cluster start from @pos,
 * return GST_FLOW_OK and cluster position in @pos if found */
static GstFlowReturn
gst_matroska_parse_search_cluster (GstMatroskaParse * parse, gint64 * pos)
{
  gint64 newpos = *pos;
  gint64 orig_offset;
  GstFlowReturn ret = GST_FLOW_OK;
  const guint chunk = 64 * 1024;
  GstBuffer *buf;
  GstMapInfo map;
  gpointer data;
  gsize size;
  guint64 length;
  guint32 id;
  guint needed;

  orig_offset = parse->common.offset;

  /* read in at newpos and scan for ebml cluster id */
  while (1) {
    GstByteReader reader;
    gint cluster_pos;

    buf = NULL;
    ret = gst_pad_pull_range (parse->common.sinkpad, newpos, chunk, &buf);
    if (ret != GST_FLOW_OK)
      break;
    GST_DEBUG_OBJECT (parse,
        "read buffer size %" G_GSIZE_FORMAT " at offset %" G_GINT64_FORMAT,
        gst_buffer_get_size (buf), newpos);
    gst_buffer_map (buf, &map, GST_MAP_READ);
    data = map.data;
    size = map.size;
    gst_byte_reader_init (&reader, data, size);
    cluster_pos = 0;
  resume:
    cluster_pos = gst_byte_reader_masked_scan_uint32 (&reader, 0xffffffff,
        GST_MATROSKA_ID_CLUSTER, cluster_pos, size - cluster_pos);
    if (cluster_pos >= 0) {
      newpos += cluster_pos;
      GST_DEBUG_OBJECT (parse,
          "found cluster ebml id at offset %" G_GINT64_FORMAT, newpos);
      /* extra checks whether we really sync'ed to a cluster:
       * - either it is the first and only cluster
       * - either there is a cluster after this one
       * - either cluster length is undefined
       */
      /* ok if first cluster (there may not a subsequent one) */
      if (newpos == parse->first_cluster_offset) {
        GST_DEBUG_OBJECT (parse, "cluster is first cluster -> OK");
        break;
      }
      parse->common.offset = newpos;
      ret = gst_matroska_read_common_peek_id_length_pull (&parse->common,
          GST_ELEMENT_CAST (parse), &id, &length, &needed);
      if (ret != GST_FLOW_OK)
        goto resume;
      g_assert (id == GST_MATROSKA_ID_CLUSTER);
      GST_DEBUG_OBJECT (parse, "cluster size %" G_GUINT64_FORMAT ", prefix %d",
          length, needed);
      /* ok if undefined length or first cluster */
      if (length == G_MAXUINT64) {
        GST_DEBUG_OBJECT (parse, "cluster has undefined length -> OK");
        break;
      }
      /* skip cluster */
      parse->common.offset += length + needed;
      ret = gst_matroska_read_common_peek_id_length_pull (&parse->common,
          GST_ELEMENT_CAST (parse), &id, &length, &needed);
      if (ret != GST_FLOW_OK)
        goto resume;
      GST_DEBUG_OBJECT (parse, "next element is %scluster",
          id == GST_MATROSKA_ID_CLUSTER ? "" : "not ");
      if (id == GST_MATROSKA_ID_CLUSTER)
        break;
      /* not ok, resume */
      goto resume;
    } else {
      /* partial cluster id may have been in tail of buffer */
      newpos += MAX (size, 4) - 3;
      gst_buffer_unmap (buf, &map);
      gst_buffer_unref (buf);
      buf = NULL;
    }
  }

  if (buf) {
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    buf = NULL;
  }

  parse->common.offset = orig_offset;
  *pos = newpos;
  return ret;
}
#endif

static gboolean
gst_matroska_parse_handle_seek_event (GstMatroskaParse * parse,
    GstPad * pad, GstEvent * event)
{
  GstMatroskaIndex *entry = NULL;
  GstSeekFlags flags;
  GstSeekType cur_type, stop_type;
  GstFormat format;
  gdouble rate;
  gint64 cur, stop;
  GstMatroskaTrackContext *track = NULL;
  GstSegment seeksegment = { 0, };
  gboolean update;
  GstSearchMode snap_dir;

  if (pad)
    track = gst_pad_get_element_private (pad);

  track = gst_matroska_read_common_get_seek_track (&parse->common, track);

  gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur,
      &stop_type, &stop);

  /* we can only seek on time */
  if (format != GST_FORMAT_TIME) {
    GST_DEBUG_OBJECT (parse, "Can only seek on TIME");
    return FALSE;
  }

  /* copy segment, we need this because we still need the old
   * segment when we close the current segment. */
  memcpy (&seeksegment, &parse->common.segment, sizeof (GstSegment));

  if (event) {
    GST_DEBUG_OBJECT (parse, "configuring seek");
    gst_segment_do_seek (&seeksegment, rate, format, flags,
        cur_type, cur, stop_type, stop, &update);
  }

  GST_DEBUG_OBJECT (parse, "New segment %" GST_SEGMENT_FORMAT, &seeksegment);

  if (seeksegment.rate < 0)
    snap_dir = GST_SEARCH_MODE_AFTER;
  else
    snap_dir = GST_SEARCH_MODE_BEFORE;

  /* check sanity before we start flushing and all that */
  GST_OBJECT_LOCK (parse);
  if ((entry = gst_matroska_read_common_do_index_seek (&parse->common, track,
              seeksegment.position, &parse->seek_index, &parse->seek_entry,
              snap_dir)) == NULL) {
    /* pull mode without index can scan later on */
    GST_DEBUG_OBJECT (parse, "No matching seek entry in index");
    GST_OBJECT_UNLOCK (parse);
    return FALSE;
  }
  GST_DEBUG_OBJECT (parse, "Seek position looks sane");
  GST_OBJECT_UNLOCK (parse);

  /* need to seek to cluster start to pick up cluster time */
  /* upstream takes care of flushing and all that
   * ... and newsegment event handling takes care of the rest */
  return perform_seek_to_offset (parse, entry->pos
      + parse->common.ebml_segment_start);
}

/*
 * Handle whether we can perform the seek event or if we have to let the chain
 * function handle seeks to build the seek indexes first.
 */
static gboolean
gst_matroska_parse_handle_seek_push (GstMatroskaParse * parse, GstPad * pad,
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
    GST_DEBUG_OBJECT (parse, "Can only seek on TIME");
    return FALSE;
  }

  if (stop_type != GST_SEEK_TYPE_NONE && stop != GST_CLOCK_TIME_NONE) {
    GST_DEBUG_OBJECT (parse, "Seek end-time not supported in streaming mode");
    return FALSE;
  }

  if (!(flags & GST_SEEK_FLAG_FLUSH)) {
    GST_DEBUG_OBJECT (parse,
        "Non-flushing seek not supported in streaming mode");
    return FALSE;
  }

  if (flags & GST_SEEK_FLAG_SEGMENT) {
    GST_DEBUG_OBJECT (parse, "Segment seek not supported in streaming mode");
    return FALSE;
  }

  /* check for having parsed index already */
  if (!parse->common.index_parsed) {
    gboolean building_index;
    guint64 offset = 0;

    if (!parse->index_offset) {
      GST_DEBUG_OBJECT (parse, "no index (location); no seek in push mode");
      return FALSE;
    }

    GST_OBJECT_LOCK (parse);
    /* handle the seek event in the chain function */
    parse->common.state = GST_MATROSKA_READ_STATE_SEEK;
    /* no more seek can be issued until state reset to _DATA */

    /* copy the event */
    if (parse->seek_event)
      gst_event_unref (parse->seek_event);
    parse->seek_event = gst_event_ref (event);

    /* set the building_index flag so that only one thread can setup the
     * structures for index seeking. */
    building_index = parse->building_index;
    if (!building_index) {
      parse->building_index = TRUE;
      offset = parse->index_offset;
    }
    GST_OBJECT_UNLOCK (parse);

    if (!building_index) {
      /* seek to the first subindex or legacy index */
      GST_INFO_OBJECT (parse, "Seeking to Cues at %" G_GUINT64_FORMAT, offset);
      return perform_seek_to_offset (parse, offset);
    }

    /* well, we are handling it already */
    return TRUE;
  }

  /* delegate to tweaked regular seek */
  return gst_matroska_parse_handle_seek_event (parse, pad, event);
}

static gboolean
gst_matroska_parse_handle_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (parent);
  gboolean res = TRUE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      /* no seeking until we are (safely) ready */
      if (parse->common.state != GST_MATROSKA_READ_STATE_DATA) {
        GST_DEBUG_OBJECT (parse, "not ready for seeking yet");
        return FALSE;
      }
      res = gst_matroska_parse_handle_seek_push (parse, pad, event);
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

        GST_OBJECT_LOCK (parse);
        videocontext->earliest_time = timestamp + diff;
        GST_OBJECT_UNLOCK (parse);
      }
      res = TRUE;
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
      res = gst_pad_push_event (parse->common.sinkpad, event);
      break;
  }

  return res;
}

static GstFlowReturn
gst_matroska_parse_parse_tracks (GstMatroskaParse * parse, GstEbmlRead * ebml)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 id;

  DEBUG_ELEMENT_START (parse, ebml, "Tracks");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (parse, ebml, "Tracks", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* one track within the "all-tracks" header */
      case GST_MATROSKA_ID_TRACKENTRY:
        ret = gst_matroska_parse_add_stream (parse, ebml);
        break;

      default:
        ret = gst_matroska_read_common_parse_skip (&parse->common, ebml,
            "Track", id);
        break;
    }
  }
  DEBUG_ELEMENT_STOP (parse, ebml, "Tracks", ret);

  parse->tracks_parsed = TRUE;

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

static GstFlowReturn
gst_matroska_parse_parse_blockgroup_or_simpleblock (GstMatroskaParse * parse,
    GstEbmlRead * ebml, guint64 cluster_time, guint64 cluster_offset,
    gboolean is_simpleblock)
{
  GstMatroskaTrackContext *stream = NULL;
  GstFlowReturn ret = GST_FLOW_OK;
  gboolean readblock = FALSE;
  guint32 id;
  guint64 block_duration = 0;
  GstBuffer *buf = NULL;
  GstMapInfo map;
  gint stream_num = -1, n, laces = 0;
  guint size = 0;
  gint *lace_size = NULL;
  gint64 time = 0;
  gint flags = 0;
  gint64 referenceblock = 0;

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
        stream_num = gst_matroska_read_common_stream_from_num (&parse->common,
            num);
        if (G_UNLIKELY (size < 3)) {
          GST_WARNING_OBJECT (parse, "Invalid size %u", size);
          /* non-fatal, try next block(group) */
          ret = GST_FLOW_OK;
          goto done;
        } else if (G_UNLIKELY (stream_num < 0 ||
                stream_num >= parse->common.num_streams)) {
          /* let's not give up on a stray invalid track number */
          GST_WARNING_OBJECT (parse,
              "Invalid stream %d for track number %" G_GUINT64_FORMAT
              "; ignoring block", stream_num, num);
          goto done;
        }

        stream = g_ptr_array_index (parse->common.src, stream_num);

        /* time (relative to cluster time) */
        time = ((gint16) GST_READ_UINT16_BE (data));
        data += 2;
        size -= 2;
        flags = GST_READ_UINT8 (data);
        data += 1;
        size -= 1;

        GST_LOG_OBJECT (parse, "time %" G_GUINT64_FORMAT ", flags %d", time,
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
        GST_DEBUG_OBJECT (parse, "BlockDuration: %" G_GUINT64_FORMAT,
            block_duration);
        break;
      }

      case GST_MATROSKA_ID_REFERENCEBLOCK:{
        ret = gst_ebml_read_sint (ebml, &id, &referenceblock);
        GST_DEBUG_OBJECT (parse, "ReferenceBlock: %" G_GINT64_FORMAT,
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
          GST_WARNING_OBJECT (parse,
              "Unexpected CodecState subelement - ignoring");
          break;
        }

        g_free (stream->codec_state);
        stream->codec_state = data;
        stream->codec_state_size = data_len;

        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&parse->common, ebml,
            "BlockGroup", id);
        break;

      case GST_MATROSKA_ID_BLOCKVIRTUAL:
      case GST_MATROSKA_ID_BLOCKADDITIONS:
      case GST_MATROSKA_ID_REFERENCEPRIORITY:
      case GST_MATROSKA_ID_REFERENCEVIRTUAL:
      case GST_MATROSKA_ID_SLICES:
        GST_DEBUG_OBJECT (parse,
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
    guint64 duration = 0;
    gint64 lace_time = 0;
    gboolean delta_unit;

    stream = g_ptr_array_index (parse->common.src, stream_num);

    if (cluster_time != GST_CLOCK_TIME_NONE) {
      /* FIXME: What to do with negative timestamps? Give timestamp 0 or -1?
       * Drop unless the lace contains timestamp 0? */
      if (time < 0 && (-time) > cluster_time) {
        lace_time = 0;
      } else {
        if (stream->timecodescale == 1.0)
          lace_time = (cluster_time + time) * parse->common.time_scale;
        else
          lace_time =
              gst_util_guint64_to_gdouble ((cluster_time + time) *
              parse->common.time_scale) * stream->timecodescale;
      }
    } else {
      lace_time = GST_CLOCK_TIME_NONE;
    }

    if (lace_time != GST_CLOCK_TIME_NONE) {
      parse->last_timestamp = lace_time;
    }
    /* need to refresh segment info ASAP */
    if (GST_CLOCK_TIME_IS_VALID (lace_time) && parse->need_newsegment) {
      GstSegment segment;
      GST_DEBUG_OBJECT (parse,
          "generating segment starting at %" GST_TIME_FORMAT,
          GST_TIME_ARGS (lace_time));
      /* pretend we seeked here */
      gst_segment_do_seek (&parse->common.segment, parse->common.segment.rate,
          GST_FORMAT_TIME, 0, GST_SEEK_TYPE_SET, lace_time,
          GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE, NULL);
      /* now convey our segment notion downstream */
      segment = parse->common.segment;
      segment.position = segment.start;
      gst_matroska_parse_send_event (parse, gst_event_new_segment (&segment));
      parse->need_newsegment = FALSE;
    }

    if (block_duration) {
      if (stream->timecodescale == 1.0)
        duration = gst_util_uint64_scale (block_duration,
            parse->common.time_scale, 1);
      else
        duration =
            gst_util_gdouble_to_guint64 (gst_util_guint64_to_gdouble
            (gst_util_uint64_scale (block_duration, parse->common.time_scale,
                    1)) * stream->timecodescale);
    } else if (stream->default_duration) {
      duration = stream->default_duration * laces;
    }
    /* else duration is diff between timecode of this and next block */

    /* For SimpleBlock, look at the keyframe bit in flags. Otherwise,
       a ReferenceBlock implies that this is not a keyframe. In either
       case, it only makes sense for video streams. */
    delta_unit = stream->type == GST_MATROSKA_TRACK_TYPE_VIDEO &&
        ((is_simpleblock && !(flags & 0x80)) || referenceblock);

    if (delta_unit && stream->set_discont) {
      /* When doing seeks or such, we need to restart on key frames or
       * decoders might choke. */
      GST_DEBUG_OBJECT (parse, "skipping delta unit");
      goto done;
    }

    for (n = 0; n < laces; n++) {
      if (G_UNLIKELY (lace_size[n] > size)) {
        GST_WARNING_OBJECT (parse, "Invalid lace size");
        break;
      }

      /* QoS for video track with an index. the assumption is that
         index entries point to keyframes, but if that is not true we
         will instead skip until the next keyframe. */
      if (GST_CLOCK_TIME_IS_VALID (lace_time) &&
          stream->type == GST_MATROSKA_TRACK_TYPE_VIDEO &&
          stream->index_table && parse->common.segment.rate > 0.0) {
        GstMatroskaTrackVideoContext *videocontext =
            (GstMatroskaTrackVideoContext *) stream;
        GstClockTime earliest_time;
        GstClockTime earliest_stream_time;

        GST_OBJECT_LOCK (parse);
        earliest_time = videocontext->earliest_time;
        GST_OBJECT_UNLOCK (parse);
        earliest_stream_time =
            gst_segment_position_from_running_time (&parse->common.segment,
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
            GST_LOG_OBJECT (parse, "Skipping lace before late keyframe");
            stream->set_discont = TRUE;
            goto next_lace;
          }
        }
      }
#if 0
      sub = gst_buffer_create_sub (buf,
          GST_BUFFER_SIZE (buf) - size, lace_size[n]);
      GST_DEBUG_OBJECT (parse, "created subbuffer %p", sub);

      if (delta_unit)
        GST_BUFFER_FLAG_SET (sub, GST_BUFFER_FLAG_DELTA_UNIT);
      else
        GST_BUFFER_FLAG_UNSET (sub, GST_BUFFER_FLAG_DELTA_UNIT);

      if (stream->encodings != NULL && stream->encodings->len > 0)
        sub = gst_matroska_decode_buffer (stream, sub);

      if (sub == NULL) {
        GST_WARNING_OBJECT (parse, "Decoding buffer failed");
        goto next_lace;
      }

      GST_BUFFER_TIMESTAMP (sub) = lace_time;

      if (GST_CLOCK_TIME_IS_VALID (lace_time)) {
        GstClockTime last_stop_end;

        /* Check if this stream is after segment stop */
        if (GST_CLOCK_TIME_IS_VALID (parse->common.segment.stop) &&
            lace_time >= parse->common.segment.stop) {
          GST_DEBUG_OBJECT (parse,
              "Stream %d after segment stop %" GST_TIME_FORMAT, stream->index,
              GST_TIME_ARGS (parse->common.segment.stop));
          gst_buffer_unref (sub);
          goto eos;
        }
        if (offset >= stream->to_offset) {
          GST_DEBUG_OBJECT (parse, "Stream %d after playback section",
              stream->index);
          gst_buffer_unref (sub);
          goto eos;
        }

        /* handle gaps, e.g. non-zero start-time, or an cue index entry
         * that landed us with timestamps not quite intended */
        if (GST_CLOCK_TIME_IS_VALID (parse->segment.last_stop) &&
            parse->segment.rate > 0.0) {
          GstClockTimeDiff diff;

          /* only send newsegments with increasing start times,
           * otherwise if these go back and forth downstream (sinks) increase
           * accumulated time and running_time */
          diff = GST_CLOCK_DIFF (parse->segment.last_stop, lace_time);
          if (diff > 2 * GST_SECOND && lace_time > parse->segment.start &&
              (!GST_CLOCK_TIME_IS_VALID (parse->segment.stop) ||
                  lace_time < parse->segment.stop)) {
            GST_DEBUG_OBJECT (parse,
                "Gap of %" G_GINT64_FORMAT " ns detected in"
                "stream %d (%" GST_TIME_FORMAT " -> %" GST_TIME_FORMAT "). "
                "Sending updated NEWSEGMENT events", diff,
                stream->index, GST_TIME_ARGS (stream->pos),
                GST_TIME_ARGS (lace_time));
            /* send newsegment events such that the gap is not accounted in
             * accum time, hence running_time */
            /* close ahead of gap */
            gst_matroska_parse_send_event (parse,
                gst_event_new_new_segment (TRUE, parse->segment.rate,
                    parse->segment.format, parse->segment.last_stop,
                    parse->segment.last_stop, parse->segment.last_stop));
            /* skip gap */
            gst_matroska_parse_send_event (parse,
                gst_event_new_new_segment (FALSE, parse->segment.rate,
                    parse->segment.format, lace_time, parse->segment.stop,
                    lace_time));
            /* align segment view with downstream,
             * prevents double-counting accum when closing segment */
            gst_segment_set_newsegment (&parse->segment, FALSE,
                parse->segment.rate, parse->segment.format, lace_time,
                parse->segment.stop, lace_time);
            parse->segment.last_stop = lace_time;
          }
        }

        if (!GST_CLOCK_TIME_IS_VALID (parse->segment.last_stop)
            || parse->segment.last_stop < lace_time) {
          parse->segment.last_stop = lace_time;
        }

        last_stop_end = lace_time;
        if (duration) {
          GST_BUFFER_DURATION (sub) = duration / laces;
          last_stop_end += GST_BUFFER_DURATION (sub);
        }

        if (!GST_CLOCK_TIME_IS_VALID (parse->last_stop_end) ||
            parse->last_stop_end < last_stop_end)
          parse->last_stop_end = last_stop_end;

        if (parse->segment.duration == -1 ||
            parse->segment.duration < lace_time) {
          gst_segment_set_duration (&parse->segment, GST_FORMAT_TIME,
              last_stop_end);
          gst_element_post_message (GST_ELEMENT_CAST (parse),
              gst_message_new_duration (GST_OBJECT_CAST (parse),
                  GST_FORMAT_TIME, GST_CLOCK_TIME_NONE));
        }
      }

      stream->pos = lace_time;

      gst_matroska_parse_sync_streams (parse);

      if (stream->set_discont) {
        GST_DEBUG_OBJECT (parse, "marking DISCONT");
        GST_BUFFER_FLAG_SET (sub, GST_BUFFER_FLAG_DISCONT);
        stream->set_discont = FALSE;
      }

      /* reverse playback book-keeping */
      if (!GST_CLOCK_TIME_IS_VALID (stream->from_time))
        stream->from_time = lace_time;
      if (stream->from_offset == -1)
        stream->from_offset = offset;

      GST_DEBUG_OBJECT (parse,
          "Pushing lace %d, data of size %d for stream %d, time=%"
          GST_TIME_FORMAT " and duration=%" GST_TIME_FORMAT, n,
          GST_BUFFER_SIZE (sub), stream_num,
          GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (sub)),
          GST_TIME_ARGS (GST_BUFFER_DURATION (sub)));

      if (parse->element_index) {
        if (stream->index_writer_id == -1)
          gst_index_get_writer_id (parse->element_index,
              GST_OBJECT (stream->pad), &stream->index_writer_id);

        GST_LOG_OBJECT (parse, "adding association %" GST_TIME_FORMAT "-> %"
            G_GUINT64_FORMAT " for writer id %d",
            GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (sub)), cluster_offset,
            stream->index_writer_id);
        gst_index_add_association (parse->element_index,
            stream->index_writer_id, GST_BUFFER_FLAG_IS_SET (sub,
                GST_BUFFER_FLAG_DELTA_UNIT) ? 0 : GST_ASSOCIATION_FLAG_KEY_UNIT,
            GST_FORMAT_TIME, GST_BUFFER_TIMESTAMP (sub), GST_FORMAT_BYTES,
            cluster_offset, NULL);
      }

      gst_buffer_set_caps (sub, GST_PAD_CAPS (parse->srcpad));

      /* Postprocess the buffers depending on the codec used */
      if (stream->postprocess_frame) {
        GST_LOG_OBJECT (parse, "running post process");
        ret = stream->postprocess_frame (GST_ELEMENT (parse), stream, &sub);
      }

      ret = gst_pad_push (stream->pad, sub);
      if (parse->segment.rate < 0) {
        if (lace_time > parse->segment.stop && ret == GST_FLOW_EOS) {
          /* In reverse playback we can get a GST_FLOW_EOS when
           * we are at the end of the segment, so we just need to jump
           * back to the previous section. */
          GST_DEBUG_OBJECT (parse, "downstream has reached end of segment");
          ret = GST_FLOW_OK;
        }
      }
      /* combine flows */
      ret = gst_matroska_parse_combine_flows (parse, stream, ret);
#endif

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
invalid_lacing:
  {
    GST_ELEMENT_WARNING (parse, STREAM, DEMUX, (NULL), ("Invalid lacing size"));
    /* non-fatal, try next block(group) */
    ret = GST_FLOW_OK;
    goto done;
  }
data_error:
  {
    GST_ELEMENT_WARNING (parse, STREAM, DEMUX, (NULL), ("Data error"));
    /* non-fatal, try next block(group) */
    ret = GST_FLOW_OK;
    goto done;
  }
}

/* return FALSE if block(group) should be skipped (due to a seek) */
static inline gboolean
gst_matroska_parse_seek_block (GstMatroskaParse * parse)
{
  if (G_UNLIKELY (parse->seek_block)) {
    if (!(--parse->seek_block)) {
      return TRUE;
    } else {
      GST_LOG_OBJECT (parse, "should skip block due to seek");
      return FALSE;
    }
  } else {
    return TRUE;
  }
}

static GstFlowReturn
gst_matroska_parse_parse_contents_seekentry (GstMatroskaParse * parse,
    GstEbmlRead * ebml)
{
  GstFlowReturn ret;
  guint64 seek_pos = (guint64) - 1;
  guint32 seek_id = 0;
  guint32 id;

  DEBUG_ELEMENT_START (parse, ebml, "Seek");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (parse, ebml, "Seek", ret);
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

        GST_DEBUG_OBJECT (parse, "SeekID: %" G_GUINT64_FORMAT, t);
        seek_id = t;
        break;
      }

      case GST_MATROSKA_ID_SEEKPOSITION:
      {
        guint64 t;

        if ((ret = gst_ebml_read_uint (ebml, &id, &t)) != GST_FLOW_OK)
          break;

        if (t > G_MAXINT64) {
          GST_WARNING_OBJECT (parse,
              "Too large SeekPosition %" G_GUINT64_FORMAT, t);
          break;
        }

        GST_DEBUG_OBJECT (parse, "SeekPosition: %" G_GUINT64_FORMAT, t);
        seek_pos = t;
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&parse->common, ebml,
            "SeekHead", id);
        break;
    }
  }

  if (ret != GST_FLOW_OK && ret != GST_FLOW_EOS)
    return ret;

  if (!seek_id || seek_pos == (guint64) - 1) {
    GST_WARNING_OBJECT (parse, "Incomplete seekhead entry (0x%x/%"
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
      guint64 length;

      /* remember */
      length = gst_matroska_read_common_get_length (&parse->common);

      if (length == (guint64) - 1) {
        GST_DEBUG_OBJECT (parse, "no upstream length, skipping SeakHead entry");
        break;
      }

      /* check for validity */
      if (seek_pos + parse->common.ebml_segment_start + 12 >= length) {
        GST_WARNING_OBJECT (parse,
            "SeekHead reference lies outside file!" " (%"
            G_GUINT64_FORMAT "+%" G_GUINT64_FORMAT "+12 >= %"
            G_GUINT64_FORMAT ")", seek_pos, parse->common.ebml_segment_start,
            length);
        break;
      }

      /* only pick up index location when streaming */
      if (seek_id == GST_MATROSKA_ID_CUES) {
        parse->index_offset = seek_pos + parse->common.ebml_segment_start;
        GST_DEBUG_OBJECT (parse, "Cues located at offset %" G_GUINT64_FORMAT,
            parse->index_offset);
      }
      break;
    }

    default:
      GST_DEBUG_OBJECT (parse, "Ignoring Seek entry for ID=0x%x", seek_id);
      break;
  }
  DEBUG_ELEMENT_STOP (parse, ebml, "Seek", ret);

  return ret;
}

static GstFlowReturn
gst_matroska_parse_parse_contents (GstMatroskaParse * parse, GstEbmlRead * ebml)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 id;

  DEBUG_ELEMENT_START (parse, ebml, "SeekHead");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (parse, ebml, "SeekHead", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_SEEKENTRY:
      {
        ret = gst_matroska_parse_parse_contents_seekentry (parse, ebml);
        /* Ignore EOS and errors here */
        if (ret != GST_FLOW_OK) {
          GST_DEBUG_OBJECT (parse, "Ignoring %s", gst_flow_get_name (ret));
          ret = GST_FLOW_OK;
        }
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&parse->common, ebml,
            "SeekHead", id);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (parse, ebml, "SeekHead", ret);

  return ret;
}

#define GST_FLOW_OVERFLOW   GST_FLOW_CUSTOM_ERROR

#define MAX_BLOCK_SIZE (15 * 1024 * 1024)

static inline GstFlowReturn
gst_matroska_parse_check_read_size (GstMatroskaParse * parse, guint64 bytes)
{
  if (G_UNLIKELY (bytes > MAX_BLOCK_SIZE)) {
    /* only a few blocks are expected/allowed to be large,
     * and will be recursed into, whereas others will be read and must fit */
    /* fatal in streaming case, as we can't step over easily */
    GST_ELEMENT_ERROR (parse, STREAM, DEMUX, (NULL),
        ("reading large block of size %" G_GUINT64_FORMAT " not supported; "
            "file might be corrupt.", bytes));
    return GST_FLOW_ERROR;
  } else {
    return GST_FLOW_OK;
  }
}

#if 0
/* returns TRUE if we truly are in error state, and should give up */
static inline gboolean
gst_matroska_parse_check_parse_error (GstMatroskaParse * parse)
{
  gint64 pos;

  /* sigh, one last attempt above and beyond call of duty ...;
   * search for cluster mark following current pos */
  pos = parse->common.offset;
  GST_WARNING_OBJECT (parse, "parse error, looking for next cluster");
  if (gst_matroska_parse_search_cluster (parse, &pos) != GST_FLOW_OK) {
    /* did not work, give up */
    return TRUE;
  } else {
    GST_DEBUG_OBJECT (parse, "... found at  %" G_GUINT64_FORMAT, pos);
    /* try that position */
    parse->common.offset = pos;
    return FALSE;
  }
}
#endif

/* initializes @ebml with @bytes from input stream at current offset.
 * Returns EOS if insufficient available,
 * ERROR if too much was attempted to read. */
static inline GstFlowReturn
gst_matroska_parse_take (GstMatroskaParse * parse, guint64 bytes,
    GstEbmlRead * ebml)
{
  GstBuffer *buffer = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_LOG_OBJECT (parse, "taking %" G_GUINT64_FORMAT " bytes for parsing",
      bytes);
  ret = gst_matroska_parse_check_read_size (parse, bytes);
  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    /* otherwise fatal */
    ret = GST_FLOW_ERROR;
    goto exit;
  }
  if (gst_adapter_available (parse->common.adapter) < bytes)
    return GST_FLOW_EOS;

  buffer = gst_adapter_take_buffer (parse->common.adapter, bytes);
  if (G_LIKELY (buffer)) {
    gst_ebml_read_init (ebml, GST_ELEMENT_CAST (parse), buffer,
        parse->common.offset);
    parse->common.offset += bytes;
  } else {
    ret = GST_FLOW_ERROR;
  }
exit:

  return ret;
}

static void
gst_matroska_parse_check_seekability (GstMatroskaParse * parse)
{
  GstQuery *query;
  gboolean seekable = FALSE;
  gint64 start = -1, stop = -1;

  query = gst_query_new_seeking (GST_FORMAT_BYTES);
  if (!gst_pad_peer_query (parse->common.sinkpad, query)) {
    GST_DEBUG_OBJECT (parse, "seeking query failed");
    goto done;
  }

  gst_query_parse_seeking (query, NULL, &seekable, &start, &stop);

  /* try harder to query upstream size if we didn't get it the first time */
  if (seekable && stop == -1) {
    GST_DEBUG_OBJECT (parse, "doing duration query to fix up unset stop");
    gst_pad_peer_query_duration (parse->common.sinkpad, GST_FORMAT_BYTES,
        &stop);
  }

  /* if upstream doesn't know the size, it's likely that it's not seekable in
   * practice even if it technically may be seekable */
  if (seekable && (start != 0 || stop <= start)) {
    GST_DEBUG_OBJECT (parse, "seekable but unknown start/stop -> disable");
    seekable = FALSE;
  }

done:
  GST_INFO_OBJECT (parse, "seekable: %d (%" G_GUINT64_FORMAT " - %"
      G_GUINT64_FORMAT ")", seekable, start, stop);
  parse->seekable = seekable;

  gst_query_unref (query);
}

#if 0
static GstFlowReturn
gst_matroska_parse_find_tracks (GstMatroskaParse * parse)
{
  guint32 id;
  guint64 before_pos;
  guint64 length;
  guint needed;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_WARNING_OBJECT (parse,
      "Found Cluster element before Tracks, searching Tracks");

  /* remember */
  before_pos = parse->common.offset;

  /* Search Tracks element */
  while (TRUE) {
    ret = gst_matroska_read_common_peek_id_length_pull (&parse->common,
        GST_ELEMENT_CAST (parse), &id, &length, &needed);
    if (ret != GST_FLOW_OK)
      break;

    if (id != GST_MATROSKA_ID_TRACKS) {
      /* we may be skipping large cluster here, so forego size check etc */
      /* ... but we can't skip undefined size; force error */
      if (length == G_MAXUINT64) {
        ret = gst_matroska_parse_check_read_size (parse, length);
        break;
      } else {
        parse->common.offset += needed;
        parse->offset += length;
      }
      continue;
    }

    /* will lead to track parsing ... */
    ret = gst_matroska_parse_parse_id (parse, id, length, needed);
    break;
  }

  /* seek back */
  parse->offset = before_pos;

  return ret;
}
#endif

#define GST_READ_CHECK(stmt)  \
G_STMT_START { \
  if (G_UNLIKELY ((ret = (stmt)) != GST_FLOW_OK)) { \
    if (ret == GST_FLOW_OVERFLOW) { \
      ret = GST_FLOW_OK; \
    } \
    goto read_error; \
  } \
} G_STMT_END

static void
gst_matroska_parse_accumulate_streamheader (GstMatroskaParse * parse,
    GstBuffer * buffer)
{
  if (parse->pushed_headers) {
    GST_WARNING_OBJECT (parse,
        "Accumulating headers, but headers are already pushed");
  }

  if (parse->streamheader) {
    parse->streamheader = gst_buffer_append (parse->streamheader,
        gst_buffer_ref (buffer));
  } else {
    parse->streamheader = gst_buffer_ref (buffer);
  }

  GST_DEBUG ("%" G_GSIZE_FORMAT, gst_buffer_get_size (parse->streamheader));
}

static GstFlowReturn
gst_matroska_parse_output (GstMatroskaParse * parse, GstBuffer * buffer,
    gboolean keyframe)
{
  GstFlowReturn ret;

  if (!parse->pushed_headers) {
    GstCaps *caps;
    GstStructure *s;
    GValue streamheader = { 0 };
    GValue bufval = { 0 };
    GstBuffer *buf;

    caps = gst_pad_get_current_caps (parse->common.sinkpad);
    if (caps == NULL) {
      caps = gst_matroska_parse_forge_caps (parse->common.is_webm,
          parse->common.has_video);
    } else
      caps = gst_caps_make_writable (caps);

    s = gst_caps_get_structure (caps, 0);
    g_value_init (&streamheader, GST_TYPE_ARRAY);
    g_value_init (&bufval, GST_TYPE_BUFFER);
    buf = gst_buffer_copy (parse->streamheader);
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_HEADER);
    gst_value_set_buffer (&bufval, buf);
    gst_buffer_unref (buf);
    gst_value_array_append_value (&streamheader, &bufval);
    g_value_unset (&bufval);
    gst_structure_set_value (s, "streamheader", &streamheader);
    g_value_unset (&streamheader);
    //gst_caps_replace (parse->caps, caps);
    gst_pad_set_caps (parse->srcpad, caps);

    if (parse->need_newsegment) {
      gst_pad_push_event (parse->srcpad,
          gst_event_new_segment (&parse->common.segment));
      parse->need_newsegment = FALSE;
    }

    buf = gst_buffer_copy (parse->streamheader);
    gst_caps_unref (caps);

    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_HEADER);
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT);

    ret = gst_pad_push (parse->srcpad, buf);
    if (ret != GST_FLOW_OK) {
      GST_WARNING_OBJECT (parse, "Failed to push buffer");
      return ret;
    }

    parse->pushed_headers = TRUE;
  }

  if (!keyframe) {
    GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);
  } else {
    GST_BUFFER_FLAG_UNSET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);
  }
  if (GST_BUFFER_TIMESTAMP (buffer) != GST_CLOCK_TIME_NONE) {
    parse->last_timestamp = GST_BUFFER_TIMESTAMP (buffer);
  } else {
    GST_BUFFER_TIMESTAMP (buffer) = parse->last_timestamp;
  }

  return gst_pad_push (parse->srcpad, gst_buffer_ref (buffer));
}

static GstFlowReturn
gst_matroska_parse_parse_id (GstMatroskaParse * parse, guint32 id,
    guint64 length, guint needed)
{
  GstEbmlRead ebml = { 0, };
  GstFlowReturn ret = GST_FLOW_OK;
  guint64 read;
  //GstBuffer *buffer;

  GST_DEBUG_OBJECT (parse, "Parsing Element id 0x%x, "
      "size %" G_GUINT64_FORMAT ", prefix %d", id, length, needed);

#if 0
  if (gst_adapter_available (parse->adapter) >= length + needed) {
    buffer = gst_adapter_take_buffer (parse->adapter, length + needed);
    gst_pad_push (parse->srcpad, buffer);
  } else {
    ret = GST_FLOW_EOS;
  }
  //GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));

  return ret;
#endif



  /* if we plan to read and parse this element, we need prefix (id + length)
   * and the contents */
  /* mind about overflow wrap-around when dealing with undefined size */
  read = length;
  if (G_LIKELY (length != G_MAXUINT64))
    read += needed;

  switch (parse->common.state) {
    case GST_MATROSKA_READ_STATE_START:
      switch (id) {
        case GST_EBML_ID_HEADER:
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          ret = gst_matroska_read_common_parse_header (&parse->common, &ebml);
          if (ret != GST_FLOW_OK)
            goto parse_failed;
          parse->common.state = GST_MATROSKA_READ_STATE_SEGMENT;
          gst_matroska_parse_check_seekability (parse);
          gst_matroska_parse_accumulate_streamheader (parse, ebml.buf);
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
          GST_READ_CHECK (gst_matroska_parse_take (parse, needed, &ebml));
          GST_DEBUG_OBJECT (parse,
              "Found Segment start at offset %" G_GUINT64_FORMAT " with size %"
              G_GUINT64_FORMAT, parse->common.offset, length);
          /* seeks are from the beginning of the segment,
           * after the segment ID/length */
          parse->common.ebml_segment_start = parse->common.offset;
          if (length == 0)
            length = G_MAXUINT64;
          parse->common.ebml_segment_length = length;
          parse->common.state = GST_MATROSKA_READ_STATE_HEADER;
          gst_matroska_parse_accumulate_streamheader (parse, ebml.buf);
          break;
        default:
          GST_WARNING_OBJECT (parse,
              "Expected a Segment ID (0x%x), but received 0x%x!",
              GST_MATROSKA_ID_SEGMENT, id);
          GST_READ_CHECK (gst_matroska_parse_take (parse, needed, &ebml));
          gst_matroska_parse_accumulate_streamheader (parse, ebml.buf);
          break;
      }
      break;
    case GST_MATROSKA_READ_STATE_SCANNING:
      if (id != GST_MATROSKA_ID_CLUSTER &&
          id != GST_MATROSKA_ID_CLUSTERTIMECODE) {
        /* we need to skip byte per byte if we are scanning for a new cluster */
        read = 1;
        goto skip;
      } else {
        GST_LOG_OBJECT (parse, "Resync done, new cluster found!");
        parse->common.start_resync_offset = -1;
        parse->common.state = parse->common.state_to_restore;
      }
      /* fall-through */
    case GST_MATROSKA_READ_STATE_HEADER:
    case GST_MATROSKA_READ_STATE_DATA:
    case GST_MATROSKA_READ_STATE_SEEK:
      switch (id) {
        case GST_MATROSKA_ID_SEGMENTINFO:
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          if (!parse->common.segmentinfo_parsed) {
            ret = gst_matroska_read_common_parse_info (&parse->common,
                GST_ELEMENT_CAST (parse), &ebml);
            if (ret == GST_FLOW_OK)
              gst_matroska_parse_send_tags (parse);
          }
          gst_matroska_parse_accumulate_streamheader (parse, ebml.buf);
          break;
        case GST_MATROSKA_ID_TRACKS:
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          if (!parse->tracks_parsed) {
            ret = gst_matroska_parse_parse_tracks (parse, &ebml);
          }
          gst_matroska_parse_accumulate_streamheader (parse, ebml.buf);
          break;
        case GST_MATROSKA_ID_CLUSTER:
          if (G_UNLIKELY (!parse->tracks_parsed)) {
            GST_DEBUG_OBJECT (parse, "Cluster before Track");
            goto not_streamable;
          }
          if (G_UNLIKELY (parse->common.state
                  == GST_MATROSKA_READ_STATE_HEADER)) {
            parse->common.state = GST_MATROSKA_READ_STATE_DATA;
            parse->first_cluster_offset = parse->common.offset;
            GST_DEBUG_OBJECT (parse, "signaling no more pads");
          }
          parse->cluster_time = GST_CLOCK_TIME_NONE;
          parse->cluster_offset = parse->common.offset;
          if (G_UNLIKELY (!parse->seek_first && parse->seek_block)) {
            GST_DEBUG_OBJECT (parse, "seek target block %" G_GUINT64_FORMAT
                " not found in Cluster, trying next Cluster's first block instead",
                parse->seek_block);
            parse->seek_block = 0;
          }
          parse->seek_first = FALSE;
          /* record next cluster for recovery */
          if (read != G_MAXUINT64)
            parse->next_cluster_offset = parse->cluster_offset + read;
          /* eat cluster prefix */
          GST_READ_CHECK (gst_matroska_parse_take (parse, needed, &ebml));
          ret = gst_matroska_parse_output (parse, ebml.buf, TRUE);
          //gst_matroska_parse_accumulate_streamheader (parse, ebml.buf);
          break;
        case GST_MATROSKA_ID_CLUSTERTIMECODE:
        {
          guint64 num;

          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          if ((ret = gst_ebml_read_uint (&ebml, &id, &num)) != GST_FLOW_OK)
            goto parse_failed;
          GST_DEBUG_OBJECT (parse, "ClusterTimeCode: %" G_GUINT64_FORMAT, num);
          parse->cluster_time = num;
#if 0
          if (parse->common.element_index) {
            if (parse->common.element_index_writer_id == -1)
              gst_index_get_writer_id (parse->common.element_index,
                  GST_OBJECT (parse), &parse->common.element_index_writer_id);
            GST_LOG_OBJECT (parse, "adding association %" GST_TIME_FORMAT "-> %"
                G_GUINT64_FORMAT " for writer id %d",
                GST_TIME_ARGS (parse->cluster_time), parse->cluster_offset,
                parse->common.element_index_writer_id);
            gst_index_add_association (parse->common.element_index,
                parse->common.element_index_writer_id,
                GST_ASSOCIATION_FLAG_KEY_UNIT,
                GST_FORMAT_TIME, parse->cluster_time,
                GST_FORMAT_BYTES, parse->cluster_offset, NULL);
          }
#endif
          gst_matroska_parse_output (parse, ebml.buf, FALSE);
          break;
        }
        case GST_MATROSKA_ID_BLOCKGROUP:
          if (!gst_matroska_parse_seek_block (parse))
            goto skip;
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          DEBUG_ELEMENT_START (parse, &ebml, "BlockGroup");
          if ((ret = gst_ebml_read_master (&ebml, &id)) == GST_FLOW_OK) {
            ret = gst_matroska_parse_parse_blockgroup_or_simpleblock (parse,
                &ebml, parse->cluster_time, parse->cluster_offset, FALSE);
          }
          DEBUG_ELEMENT_STOP (parse, &ebml, "BlockGroup", ret);
          gst_matroska_parse_output (parse, ebml.buf, FALSE);
          break;
        case GST_MATROSKA_ID_SIMPLEBLOCK:
          if (!gst_matroska_parse_seek_block (parse))
            goto skip;
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          DEBUG_ELEMENT_START (parse, &ebml, "SimpleBlock");
          ret = gst_matroska_parse_parse_blockgroup_or_simpleblock (parse,
              &ebml, parse->cluster_time, parse->cluster_offset, TRUE);
          DEBUG_ELEMENT_STOP (parse, &ebml, "SimpleBlock", ret);
          gst_matroska_parse_output (parse, ebml.buf, FALSE);
          break;
        case GST_MATROSKA_ID_ATTACHMENTS:
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          if (!parse->common.attachments_parsed) {
            ret = gst_matroska_read_common_parse_attachments (&parse->common,
                GST_ELEMENT_CAST (parse), &ebml);
            if (ret == GST_FLOW_OK)
              gst_matroska_parse_send_tags (parse);
          }
          gst_matroska_parse_output (parse, ebml.buf, FALSE);
          break;
        case GST_MATROSKA_ID_TAGS:
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          ret = gst_matroska_read_common_parse_metadata (&parse->common,
              GST_ELEMENT_CAST (parse), &ebml);
          if (ret == GST_FLOW_OK)
            gst_matroska_parse_send_tags (parse);
          gst_matroska_parse_accumulate_streamheader (parse, ebml.buf);
          break;
        case GST_MATROSKA_ID_CHAPTERS:
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          ret = gst_matroska_read_common_parse_chapters (&parse->common, &ebml);
          gst_matroska_parse_output (parse, ebml.buf, FALSE);
          break;
        case GST_MATROSKA_ID_SEEKHEAD:
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          ret = gst_matroska_parse_parse_contents (parse, &ebml);
          gst_matroska_parse_output (parse, ebml.buf, FALSE);
          break;
        case GST_MATROSKA_ID_CUES:
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          if (!parse->common.index_parsed) {
            ret = gst_matroska_read_common_parse_index (&parse->common, &ebml);
            /* only push based; delayed index building */
            if (ret == GST_FLOW_OK
                && parse->common.state == GST_MATROSKA_READ_STATE_SEEK) {
              GstEvent *event;

              GST_OBJECT_LOCK (parse);
              event = parse->seek_event;
              parse->seek_event = NULL;
              GST_OBJECT_UNLOCK (parse);

              g_assert (event);
              /* unlikely to fail, since we managed to seek to this point */
              if (!gst_matroska_parse_handle_seek_event (parse, NULL, event))
                goto seek_failed;
              /* resume data handling, main thread clear to seek again */
              GST_OBJECT_LOCK (parse);
              parse->common.state = GST_MATROSKA_READ_STATE_DATA;
              GST_OBJECT_UNLOCK (parse);
            }
          }
          gst_matroska_parse_output (parse, ebml.buf, FALSE);
          break;
        case GST_MATROSKA_ID_POSITION:
        case GST_MATROSKA_ID_PREVSIZE:
        case GST_MATROSKA_ID_ENCRYPTEDBLOCK:
        case GST_MATROSKA_ID_SILENTTRACKS:
          GST_DEBUG_OBJECT (parse,
              "Skipping Cluster subelement 0x%x - ignoring", id);
          /* fall-through */
        default:
        skip:
          GST_DEBUG_OBJECT (parse, "skipping Element 0x%x", id);
          GST_READ_CHECK (gst_matroska_parse_take (parse, read, &ebml));
          gst_matroska_parse_output (parse, ebml.buf, FALSE);
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
    GST_ELEMENT_ERROR (parse, STREAM, DEMUX, (NULL),
        ("Failed to parse Element 0x%x", id));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
not_streamable:
  {
    GST_ELEMENT_ERROR (parse, STREAM, DEMUX, (NULL),
        ("File layout does not permit streaming"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
#if 0
no_tracks:
  {
    GST_ELEMENT_ERROR (parse, STREAM, DEMUX, (NULL),
        ("No Tracks element found"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
#endif
invalid_header:
  {
    GST_ELEMENT_ERROR (parse, STREAM, DEMUX, (NULL), ("Invalid header"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
seek_failed:
  {
    GST_ELEMENT_ERROR (parse, STREAM, DEMUX, (NULL), ("Failed to seek"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
}

#if 0
static void
gst_matroska_parse_loop (GstPad * pad)
{
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (GST_PAD_PARENT (pad));
  GstFlowReturn ret;
  guint32 id;
  guint64 length;
  guint needed;

  /* If we have to close a segment, send a new segment to do this now */
  if (G_LIKELY (parse->common.state == GST_MATROSKA_READ_STATE_DATA)) {
    if (G_UNLIKELY (parse->close_segment)) {
      gst_matroska_parse_send_event (parse, parse->close_segment);
      parse->close_segment = NULL;
    }
    if (G_UNLIKELY (parse->new_segment)) {
      gst_matroska_parse_send_event (parse, parse->new_segment);
      parse->new_segment = NULL;
    }
  }

  ret = gst_matroska_read_common_peek_id_length_pull (&parse->common,
      GST_ELEMENT_CAST (parse), &id, &length, &needed);
  if (ret == GST_FLOW_EOS)
    goto eos;
  if (ret != GST_FLOW_OK) {
    if (gst_matroska_parse_check_parse_error (parse))
      goto pause;
    else
      return;
  }

  GST_LOG_OBJECT (parse, "Offset %" G_GUINT64_FORMAT ", Element id 0x%x, "
      "size %" G_GUINT64_FORMAT ", needed %d", parse->offset, id,
      length, needed);

  ret = gst_matroska_parse_parse_id (parse, id, length, needed);
  if (ret == GST_FLOW_EOS)
    goto eos;
  if (ret != GST_FLOW_OK)
    goto pause;

  /* check if we're at the end of a configured segment */
  if (G_LIKELY (parse->src->len)) {
    guint i;

    g_assert (parse->num_streams == parse->src->len);
    for (i = 0; i < parse->src->len; i++) {
      GstMatroskaTrackContext *context = g_ptr_array_index (parse->src, i);
      GST_DEBUG_OBJECT (context->pad, "pos %" GST_TIME_FORMAT,
          GST_TIME_ARGS (context->pos));
      if (context->eos == FALSE)
        goto next;
    }

    GST_INFO_OBJECT (parse, "All streams are EOS");
    ret = GST_FLOW_EOS;
    goto eos;
  }

next:
  if (G_UNLIKELY (parse->offset ==
          gst_matroska_read_common_get_length (&parse->common))) {
    GST_LOG_OBJECT (parse, "Reached end of stream");
    ret = GST_FLOW_EOS;
    goto eos;
  }

  return;

  /* ERRORS */
eos:
  {
    if (parse->segment.rate < 0.0) {
      ret = gst_matroska_parse_seek_to_previous_keyframe (parse);
      if (ret == GST_FLOW_OK)
        return;
    }
    /* fall-through */
  }
pause:
  {
    const gchar *reason = gst_flow_get_name (ret);
    gboolean push_eos = FALSE;

    GST_LOG_OBJECT (parse, "pausing task, reason %s", reason);
    parse->segment_running = FALSE;
    gst_pad_pause_task (parse->common.sinkpad);

    if (ret == GST_FLOW_EOS) {
      /* perform EOS logic */

      /* Close the segment, i.e. update segment stop with the duration
       * if no stop was set */
      if (GST_CLOCK_TIME_IS_VALID (parse->last_stop_end) &&
          !GST_CLOCK_TIME_IS_VALID (parse->segment.stop)) {
        GstEvent *event =
            gst_event_new_new_segment_full (TRUE, parse->segment.rate,
            parse->segment.applied_rate, parse->segment.format,
            parse->segment.start,
            MAX (parse->last_stop_end, parse->segment.start),
            parse->segment.time);
        gst_matroska_parse_send_event (parse, event);
      }

      if (parse->segment.flags & GST_SEEK_FLAG_SEGMENT) {
        gint64 stop;

        /* for segment playback we need to post when (in stream time)
         * we stopped, this is either stop (when set) or the duration. */
        if ((stop = parse->segment.stop) == -1)
          stop = parse->last_stop_end;

        GST_LOG_OBJECT (parse, "Sending segment done, at end of segment");
        gst_element_post_message (GST_ELEMENT (parse),
            gst_message_new_segment_done (GST_OBJECT (parse), GST_FORMAT_TIME,
                stop));
        gst_matroska_parse_send_event (parse,
            gst_event_new_segment_done (GST_FORMAT_TIME, stop));
      } else {
        push_eos = TRUE;
      }
    } else if (ret == GST_FLOW_NOT_LINKED || ret < GST_FLOW_EOS) {
      /* for fatal errors we post an error message */
      GST_ELEMENT_FLOW_ERROR (parse, ret);
      push_eos = TRUE;
    }
    if (push_eos) {
      /* send EOS, and prevent hanging if no streams yet */
      GST_LOG_OBJECT (parse, "Sending EOS, at end of stream");
      if (!gst_matroska_parse_send_event (parse, gst_event_new_eos ()) &&
          (ret == GST_FLOW_EOS)) {
        GST_ELEMENT_ERROR (parse, STREAM, DEMUX,
            (NULL), ("got eos but no streams (yet)"));
      }
    }
    return;
  }
}
#endif

/*
 * Create and push a flushing seek event upstream
 */
static gboolean
perform_seek_to_offset (GstMatroskaParse * parse, guint64 offset)
{
  GstEvent *event;
  gboolean res = 0;

  GST_DEBUG_OBJECT (parse, "Seeking to %" G_GUINT64_FORMAT, offset);

  event =
      gst_event_new_seek (1.0, GST_FORMAT_BYTES,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET, offset,
      GST_SEEK_TYPE_NONE, -1);

  res = gst_pad_push_event (parse->common.sinkpad, event);

  /* newsegment event will update offset */
  return res;
}

/*
 * Forge empty default caps when all we know is the stream's EBML
 * type and whether it has video or not.
 *
 * FIXME: Do something with video/x-matroska-3d if possible
 */
static GstCaps *
gst_matroska_parse_forge_caps (gboolean is_webm, gboolean has_video)
{
  GstCaps *caps;

  if (is_webm) {
    if (has_video)
      caps = gst_caps_new_empty_simple ("video/webm");
    else
      caps = gst_caps_new_empty_simple ("audio/webm");
  } else {
    if (has_video)
      caps = gst_caps_new_empty_simple ("video/x-matroska");
    else
      caps = gst_caps_new_empty_simple ("audio/x-matroska");
  }
  return caps;
}

static GstFlowReturn
gst_matroska_parse_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (parent);
  guint available;
  GstFlowReturn ret = GST_FLOW_OK;
  guint needed = 0;
  guint32 id;
  guint64 length;

  if (G_UNLIKELY (GST_BUFFER_IS_DISCONT (buffer))) {
    GST_DEBUG_OBJECT (parse, "got DISCONT");
    gst_adapter_clear (parse->common.adapter);
    GST_OBJECT_LOCK (parse);
    gst_matroska_read_common_reset_streams (&parse->common,
        GST_CLOCK_TIME_NONE, FALSE);
    GST_OBJECT_UNLOCK (parse);
  }

  gst_adapter_push (parse->common.adapter, buffer);
  buffer = NULL;

next:
  available = gst_adapter_available (parse->common.adapter);

  ret = gst_matroska_read_common_peek_id_length_push (&parse->common,
      GST_ELEMENT_CAST (parse), &id, &length, &needed);
  if (G_UNLIKELY (ret != GST_FLOW_OK && ret != GST_FLOW_EOS)) {
    if (parse->common.ebml_segment_length != G_MAXUINT64
        && parse->common.offset >=
        parse->common.ebml_segment_start + parse->common.ebml_segment_length) {
      return GST_FLOW_EOS;
    } else {
      /*
       * parsing error: we need to flush a byte from the adapter if the id is
       * not a cluster and so on until we found a new cluser or the
       * INVALID_DATA_THRESHOLD is exceeded, we reuse gst_matroska_parse_parse_id
       * setting the state to GST_MATROSKA_READ_STATE_SCANNING so the bytes
       * are skipped until a new cluster is found
       */
      gint64 bytes_scanned;
      if (parse->common.start_resync_offset == -1) {
        parse->common.start_resync_offset = parse->common.offset;
        parse->common.state_to_restore = parse->common.state;
      }
      bytes_scanned = parse->common.offset - parse->common.start_resync_offset;
      if (bytes_scanned <= INVALID_DATA_THRESHOLD) {
        GST_WARNING_OBJECT (parse,
            "parse error, looking for next cluster, actual offset %"
            G_GUINT64_FORMAT ", start resync offset %" G_GUINT64_FORMAT,
            parse->common.offset, parse->common.start_resync_offset);
        parse->common.state = GST_MATROSKA_READ_STATE_SCANNING;
        ret = GST_FLOW_OK;
      } else {
        GST_WARNING_OBJECT (parse,
            "unrecoverable parse error, next cluster not found and threshold "
            "exceeded, bytes scanned %" G_GINT64_FORMAT, bytes_scanned);
        return ret;
      }
    }
  }

  GST_LOG_OBJECT (parse, "Offset %" G_GUINT64_FORMAT ", Element id 0x%x, "
      "size %" G_GUINT64_FORMAT ", needed %d, available %d",
      parse->common.offset, id, length, needed, available);

  if (needed > available)
    return GST_FLOW_OK;

  ret = gst_matroska_parse_parse_id (parse, id, length, needed);
  if (ret == GST_FLOW_EOS) {
    /* need more data */
    return GST_FLOW_OK;
  } else if (ret != GST_FLOW_OK) {
    return ret;
  } else
    goto next;
}

static gboolean
gst_matroska_parse_handle_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res = TRUE;
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (GST_PAD_PARENT (pad));

  GST_DEBUG_OBJECT (parse,
      "have event type %s: %p on sink pad", GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    {
      const GstSegment *segment;

      /* some debug output */
      gst_event_parse_segment (event, &segment);
      GST_DEBUG_OBJECT (parse,
          "received format %d newsegment %" GST_SEGMENT_FORMAT,
          segment->format, segment);

      if (parse->common.state < GST_MATROSKA_READ_STATE_DATA) {
        GST_DEBUG_OBJECT (parse, "still starting");
        goto exit;
      }

      /* we only expect a BYTE segment, e.g. following a seek */
      if (segment->format != GST_FORMAT_BYTES) {
        GST_DEBUG_OBJECT (parse, "unsupported segment format, ignoring");
        goto exit;
      }

      GST_DEBUG_OBJECT (parse, "clearing segment state");
      /* clear current segment leftover */
      gst_adapter_clear (parse->common.adapter);
      /* and some streaming setup */
      parse->common.offset = segment->start;
      /* do not know where we are;
       * need to come across a cluster and generate newsegment */
      parse->common.segment.position = GST_CLOCK_TIME_NONE;
      parse->cluster_time = GST_CLOCK_TIME_NONE;
      parse->cluster_offset = 0;
      parse->need_newsegment = TRUE;
      /* but keep some of the upstream segment */
      parse->common.segment.rate = segment->rate;
    exit:
      /* chain will send initial newsegment after pads have been added,
       * or otherwise come up with one */
      GST_DEBUG_OBJECT (parse, "eating event");
      gst_event_unref (event);
      res = TRUE;
      break;
    }
    case GST_EVENT_EOS:
    {
      if (parse->common.state != GST_MATROSKA_READ_STATE_DATA
          && parse->common.state != GST_MATROSKA_READ_STATE_SCANNING) {
        gst_event_unref (event);
        GST_ELEMENT_ERROR (parse, STREAM, DEMUX,
            (NULL), ("got eos and didn't receive a complete header object"));
      } else if (parse->common.num_streams == 0) {
        GST_ELEMENT_ERROR (parse, STREAM, DEMUX,
            (NULL), ("got eos but no streams (yet)"));
      } else {
        gst_matroska_parse_send_event (parse, event);
      }
      break;
    }
    case GST_EVENT_FLUSH_STOP:
    {
      gst_adapter_clear (parse->common.adapter);
      GST_OBJECT_LOCK (parse);
      gst_matroska_read_common_reset_streams (&parse->common,
          GST_CLOCK_TIME_NONE, TRUE);
      GST_OBJECT_UNLOCK (parse);
      parse->common.segment.position = GST_CLOCK_TIME_NONE;
      parse->cluster_time = GST_CLOCK_TIME_NONE;
      parse->cluster_offset = 0;
      /* fall-through */
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

#if 0
static void
gst_matroska_parse_set_index (GstElement * element, GstIndex * index)
{
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (element);

  GST_OBJECT_LOCK (parse);
  if (parse->common.element_index)
    gst_object_unref (parse->common.element_index);
  parse->common.element_index = index ? gst_object_ref (index) : NULL;
  GST_OBJECT_UNLOCK (parse);
  GST_DEBUG_OBJECT (parse, "Set index %" GST_PTR_FORMAT,
      parse->common.element_index);
}

static GstIndex *
gst_matroska_parse_get_index (GstElement * element)
{
  GstIndex *result = NULL;
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (element);

  GST_OBJECT_LOCK (parse);
  if (parse->common.element_index)
    result = gst_object_ref (parse->common.element_index);
  GST_OBJECT_UNLOCK (parse);

  GST_DEBUG_OBJECT (parse, "Returning index %" GST_PTR_FORMAT, result);

  return result;
}
#endif

static GstStateChangeReturn
gst_matroska_parse_change_state (GstElement * element,
    GstStateChange transition)
{
  GstMatroskaParse *parse = GST_MATROSKA_PARSE (element);
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
      gst_matroska_parse_reset (GST_ELEMENT (parse));
      break;
    default:
      break;
  }

  return ret;
}

gboolean
gst_matroska_parse_plugin_init (GstPlugin * plugin)
{
  gst_riff_init ();

  /* create an elementfactory for the matroska_parse element */
  if (!gst_element_register (plugin, "matroskaparse",
          GST_RANK_NONE, GST_TYPE_MATROSKA_PARSE))
    return FALSE;

  return TRUE;
}
