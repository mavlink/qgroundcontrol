/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2005 Michal Benes <michal.benes@xeris.cz>
 * (c) 2008 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 * (c) 2011 Mark Nauwelaerts <mark.nauwelaerts@collabora.co.uk>
 *
 * matroska-mux.c: matroska file/stream muxer
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

/* TODO: - check everywhere that we don't write invalid values
 *       - make sure timestamps are correctly scaled everywhere
 */

/**
 * SECTION:element-matroskamux
 * @title: matroskamux
 *
 * matroskamux muxes different input streams into a Matroska file.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v filesrc location=/path/to/mp3 ! mpegaudioparse ! matroskamux name=mux ! filesink location=test.mkv  filesrc location=/path/to/theora.ogg ! oggdemux ! theoraparse ! mux.
 * ]| This pipeline muxes an MP3 file and a Ogg Theora video into a Matroska file.
 * |[
 * gst-launch-1.0 -v audiotestsrc num-buffers=100 ! audioconvert ! vorbisenc ! matroskamux ! filesink location=test.mka
 * ]| This pipeline muxes a 440Hz sine wave encoded with the Vorbis codec into a Matroska file.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <gst/audio/audio.h>
#include <gst/riff/riff-media.h>
#include <gst/tag/tag.h>
#include <gst/pbutils/codec-utils.h>

#include "matroska-mux.h"
#include "matroska-ids.h"

#define GST_MATROSKA_MUX_CHAPLANG "und"

GST_DEBUG_CATEGORY_STATIC (matroskamux_debug);
#define GST_CAT_DEFAULT matroskamux_debug

enum
{
  PROP_0,
  PROP_WRITING_APP,
  PROP_DOCTYPE_VERSION,
  PROP_MIN_INDEX_INTERVAL,
  PROP_STREAMABLE,
  PROP_TIMECODESCALE,
  PROP_MIN_CLUSTER_DURATION,
  PROP_MAX_CLUSTER_DURATION,
  PROP_OFFSET_TO_ZERO,
  PROP_CREATION_TIME,
};

#define  DEFAULT_DOCTYPE_VERSION         2
#define  DEFAULT_WRITING_APP             "GStreamer Matroska muxer"
#define  DEFAULT_MIN_INDEX_INTERVAL      0
#define  DEFAULT_STREAMABLE              FALSE
#define  DEFAULT_TIMECODESCALE           GST_MSECOND
#define  DEFAULT_MIN_CLUSTER_DURATION    500 * GST_MSECOND
#define  DEFAULT_MAX_CLUSTER_DURATION    65535 * GST_MSECOND
#define  DEFAULT_OFFSET_TO_ZERO          FALSE

/* WAVEFORMATEX is gst_riff_strf_auds + an extra guint16 extension size */
#define WAVEFORMATEX_SIZE  (2 + sizeof (gst_riff_strf_auds))

static GstStaticPadTemplate src_templ = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-matroska; video/x-matroska-3d; audio/x-matroska")
    );

#define COMMON_VIDEO_CAPS \
  "width = (int) [ 16, MAX ], " \
  "height = (int) [ 16, MAX ] "

/* FIXME:
 * * require codec data, etc as needed
 */

static GstStaticPadTemplate videosink_templ =
    GST_STATIC_PAD_TEMPLATE ("video_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("video/mpeg, "
        "mpegversion = (int) { 1, 2, 4 }, "
        "systemstream = (boolean) false, "
        COMMON_VIDEO_CAPS "; "
        "video/x-h264, stream-format=avc, alignment=au, "
        COMMON_VIDEO_CAPS "; "
        "video/x-h265, stream-format=hvc1, alignment=au, "
        COMMON_VIDEO_CAPS "; "
        "video/x-divx, "
        COMMON_VIDEO_CAPS "; "
        "video/x-huffyuv, "
        COMMON_VIDEO_CAPS "; "
        "video/x-dv, "
        COMMON_VIDEO_CAPS "; "
        "video/x-h263, "
        COMMON_VIDEO_CAPS "; "
        "video/x-msmpeg, "
        COMMON_VIDEO_CAPS "; "
        "image/jpeg, "
        COMMON_VIDEO_CAPS "; "
        "video/x-theora; "
        "video/x-dirac, "
        COMMON_VIDEO_CAPS "; "
        "video/x-pn-realvideo, "
        "rmversion = (int) [1, 4], "
        COMMON_VIDEO_CAPS "; "
        "video/x-vp8, "
        COMMON_VIDEO_CAPS "; "
        "video/x-vp9, "
        COMMON_VIDEO_CAPS "; "
        "video/x-raw, "
        "format = (string) { YUY2, I420, YV12, UYVY, AYUV, GRAY8, BGR, RGB }, "
        COMMON_VIDEO_CAPS "; "
        "video/x-prores, "
        COMMON_VIDEO_CAPS "; "
        "video/x-wmv, " "wmvversion = (int) [ 1, 3 ], " COMMON_VIDEO_CAPS "; "
        "video/x-av1, " COMMON_VIDEO_CAPS)
    );

#define COMMON_AUDIO_CAPS \
  "channels = (int) [ 1, MAX ], " \
  "rate = (int) [ 1, MAX ]"

/* FIXME:
 * * require codec data, etc as needed
 */
static GstStaticPadTemplate audiosink_templ =
    GST_STATIC_PAD_TEMPLATE ("audio_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("audio/mpeg, "
        "mpegversion = (int) 1, "
        "layer = (int) [ 1, 3 ], "
        COMMON_AUDIO_CAPS "; "
        "audio/mpeg, "
        "mpegversion = (int) { 2, 4 }, "
        "stream-format = (string) raw, "
        COMMON_AUDIO_CAPS "; "
        "audio/x-ac3, "
        COMMON_AUDIO_CAPS "; "
        "audio/x-eac3, "
        COMMON_AUDIO_CAPS "; "
        "audio/x-dts, "
        COMMON_AUDIO_CAPS "; "
        "audio/x-vorbis, "
        COMMON_AUDIO_CAPS "; "
        "audio/x-flac, "
        COMMON_AUDIO_CAPS "; "
        "audio/x-opus; "
        "audio/x-speex, "
        COMMON_AUDIO_CAPS "; "
        "audio/x-raw, "
        "format = (string) { U8, S16BE, S16LE, S24BE, S24LE, S32BE, S32LE, F32LE, F64LE }, "
        "layout = (string) interleaved, "
        COMMON_AUDIO_CAPS ";"
        "audio/x-tta, "
        "width = (int) { 8, 16, 24 }, "
        "channels = (int) { 1, 2 }, " "rate = (int) [ 8000, 96000 ]; "
        "audio/x-pn-realaudio, "
        "raversion = (int) { 1, 2, 8 }, " COMMON_AUDIO_CAPS "; "
        "audio/x-wma, " "wmaversion = (int) [ 1, 3 ], "
        "block_align = (int) [ 0, 65535 ], bitrate = (int) [ 0, 524288 ], "
        COMMON_AUDIO_CAPS ";"
        "audio/x-alaw, "
        "channels = (int) {1, 2}, " "rate = (int) [ 8000, 192000 ]; "
        "audio/x-mulaw, "
        "channels = (int) {1, 2}, " "rate = (int) [ 8000, 192000 ]; "
        "audio/x-adpcm, "
        "layout = (string)dvi, "
        "block_align = (int)[64, 8192], "
        "channels = (int) { 1, 2 }, " "rate = (int) [ 8000, 96000 ]; "
        "audio/G722, "
        "channels = (int)1," "rate = (int)16000; "
        "audio/x-adpcm, "
        "layout = (string)g726, " "channels = (int)1," "rate = (int)8000; ")
    );

static GstStaticPadTemplate subtitlesink_templ =
    GST_STATIC_PAD_TEMPLATE ("subtitle_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("subtitle/x-kate; "
        "text/x-raw, format=utf8; application/x-ssa; application/x-ass; "
        "application/x-usf; subpicture/x-dvd; "
        "application/x-subtitle-unknown")
    );

static gpointer parent_class;   /* NULL */

/* Matroska muxer destructor */
static void gst_matroska_mux_class_init (GstMatroskaMuxClass * klass);
static void gst_matroska_mux_init (GstMatroskaMux * mux, gpointer g_class);
static void gst_matroska_mux_finalize (GObject * object);

/* Pads collected callback */
static GstFlowReturn gst_matroska_mux_handle_buffer (GstCollectPads * pads,
    GstCollectData * data, GstBuffer * buf, gpointer user_data);
static gboolean gst_matroska_mux_handle_sink_event (GstCollectPads * pads,
    GstCollectData * data, GstEvent * event, gpointer user_data);

/* pad functions */
static gboolean gst_matroska_mux_handle_src_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static GstPad *gst_matroska_mux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static void gst_matroska_mux_release_pad (GstElement * element, GstPad * pad);

/* gst internal change state handler */
static GstStateChangeReturn
gst_matroska_mux_change_state (GstElement * element, GstStateChange transition);

/* gobject bla bla */
static void gst_matroska_mux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_matroska_mux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

/* reset muxer */
static void gst_matroska_mux_reset (GstElement * element);

/* uid generation */
static guint64 gst_matroska_mux_create_uid (GstMatroskaMux * mux);

static gboolean theora_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context);
static gboolean vorbis_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context);
static gboolean speex_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context);
static gboolean kate_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context);
static gboolean flac_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context);
static void
gst_matroska_mux_write_simple_tag (const GstTagList * list, const gchar * tag,
    gpointer data);
static gboolean gst_matroska_mux_tag_list_is_empty (const GstTagList * list);
static void gst_matroska_mux_write_streams_tags (GstMatroskaMux * mux);
static gboolean gst_matroska_mux_streams_have_tags (GstMatroskaMux * mux);

/* Cannot use boilerplate macros here because we need the full init function
 * signature with the additional class argument, so we use the right template
 * for the sink caps */
GType
gst_matroska_mux_get_type (void)
{
  static GType object_type;     /* 0 */

  if (object_type == 0) {
    static const GTypeInfo object_info = {
      sizeof (GstMatroskaMuxClass),
      NULL,                     /* base_init */
      NULL,                     /* base_finalize */
      (GClassInitFunc) gst_matroska_mux_class_init,
      NULL,                     /* class_finalize */
      NULL,                     /* class_data */
      sizeof (GstMatroskaMux),
      0,                        /* n_preallocs */
      (GInstanceInitFunc) gst_matroska_mux_init
    };
    const GInterfaceInfo iface_info = { NULL };

    object_type = g_type_register_static (GST_TYPE_ELEMENT,
        "GstMatroskaMux", &object_info, (GTypeFlags) 0);

    g_type_add_interface_static (object_type, GST_TYPE_TAG_SETTER, &iface_info);
    g_type_add_interface_static (object_type, GST_TYPE_TOC_SETTER, &iface_info);
  }

  return object_type;
}

static void
gst_matroska_mux_class_init (GstMatroskaMuxClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &videosink_templ);
  gst_element_class_add_static_pad_template (gstelement_class,
      &audiosink_templ);
  gst_element_class_add_static_pad_template (gstelement_class,
      &subtitlesink_templ);
  gst_element_class_add_static_pad_template (gstelement_class, &src_templ);
  gst_element_class_set_static_metadata (gstelement_class, "Matroska muxer",
      "Codec/Muxer",
      "Muxes video/audio/subtitle streams into a matroska stream",
      "GStreamer maintainers <gstreamer-devel@lists.freedesktop.org>");

  GST_DEBUG_CATEGORY_INIT (matroskamux_debug, "matroskamux", 0,
      "Matroska muxer");

  gobject_class->finalize = gst_matroska_mux_finalize;

  gobject_class->get_property = gst_matroska_mux_get_property;
  gobject_class->set_property = gst_matroska_mux_set_property;

  g_object_class_install_property (gobject_class, PROP_WRITING_APP,
      g_param_spec_string ("writing-app", "Writing application.",
          "The name the application that creates the matroska file.",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DOCTYPE_VERSION,
      g_param_spec_int ("version", "DocType version",
          "This parameter determines what Matroska features can be used.",
          1, 2, DEFAULT_DOCTYPE_VERSION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MIN_INDEX_INTERVAL,
      g_param_spec_int64 ("min-index-interval", "Minimum time between index "
          "entries", "An index entry is created every so many nanoseconds.",
          0, G_MAXINT64, DEFAULT_MIN_INDEX_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_STREAMABLE,
      g_param_spec_boolean ("streamable", "Determines whether output should "
          "be streamable", "If set to true, the output should be as if it is "
          "to be streamed and hence no indexes written or duration written.",
          DEFAULT_STREAMABLE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_TIMECODESCALE,
      g_param_spec_int64 ("timecodescale", "Timecode Scale",
          "TimecodeScale used to calculate the Raw Timecode of a Block", 1,
          GST_SECOND, DEFAULT_TIMECODESCALE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MIN_CLUSTER_DURATION,
      g_param_spec_int64 ("min-cluster-duration", "Minimum cluster duration",
          "Desired cluster duration as nanoseconds. A new cluster will be "
          "created irrespective of this property if a force key unit event "
          "is received. 0 means create a new cluster for each video keyframe "
          "or for each audio buffer in audio only streams.", 0,
          G_MAXINT64, DEFAULT_MIN_CLUSTER_DURATION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MAX_CLUSTER_DURATION,
      g_param_spec_int64 ("max-cluster-duration", "Maximum cluster duration",
          "A new cluster will be created if its duration exceeds this value. "
          "0 means no maximum duration.", 0,
          G_MAXINT64, DEFAULT_MAX_CLUSTER_DURATION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OFFSET_TO_ZERO,
      g_param_spec_boolean ("offset-to-zero", "Offset To Zero",
          "Offsets all streams so that the " "earliest stream starts at 0.",
          DEFAULT_OFFSET_TO_ZERO, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CREATION_TIME,
      g_param_spec_boxed ("creation-time", "Creation Time",
          "Date and time of creation. This will be used for the DateUTC field."
          " NULL means that the current time will be used.",
          G_TYPE_DATE_TIME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_matroska_mux_change_state);
  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_matroska_mux_request_new_pad);
  gstelement_class->release_pad =
      GST_DEBUG_FUNCPTR (gst_matroska_mux_release_pad);

  parent_class = g_type_class_peek_parent (klass);
}

/*
 * Start of pad option handler code
 */
#define DEFAULT_PAD_FRAME_DURATION TRUE

enum
{
  PROP_PAD_0,
  PROP_PAD_FRAME_DURATION
};

typedef struct
{
  GstPad parent;
  gboolean frame_duration;
  gboolean frame_duration_user;
} GstMatroskamuxPad;

typedef GstPadClass GstMatroskamuxPadClass;

GType gst_matroskamux_pad_get_type (void);
G_DEFINE_TYPE (GstMatroskamuxPad, gst_matroskamux_pad, GST_TYPE_PAD);

#define GST_TYPE_MATROSKAMUX_PAD (gst_matroskamux_pad_get_type())
#define GST_MATROSKAMUX_PAD(pad) (G_TYPE_CHECK_INSTANCE_CAST((pad),GST_TYPE_MATROSKAMUX_PAD,GstMatroskamuxPad))
#define GST_MATROSKAMUX_PAD_CAST(pad) ((GstMatroskamuxPad *) pad)
#define GST_IS_MATROSKAMUX_PAD(pad) (G_TYPE_CHECK_INSTANCE_TYPE((pad),GST_TYPE_MATROSKAMUX_PAD))

static void
gst_matroskamux_pad_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstMatroskamuxPad *pad = GST_MATROSKAMUX_PAD (object);

  switch (prop_id) {
    case PROP_PAD_FRAME_DURATION:
      g_value_set_boolean (value, pad->frame_duration);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_matroskamux_pad_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMatroskamuxPad *pad = GST_MATROSKAMUX_PAD (object);

  switch (prop_id) {
    case PROP_PAD_FRAME_DURATION:
      pad->frame_duration = g_value_get_boolean (value);
      pad->frame_duration_user = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_matroskamux_pad_class_init (GstMatroskamuxPadClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gst_matroskamux_pad_set_property;
  gobject_class->get_property = gst_matroskamux_pad_get_property;

  g_object_class_install_property (gobject_class, PROP_PAD_FRAME_DURATION,
      g_param_spec_boolean ("frame-duration", "Frame duration",
          "Default frame duration", DEFAULT_PAD_FRAME_DURATION,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}

static void
gst_matroskamux_pad_init (GstMatroskamuxPad * pad)
{
  pad->frame_duration = DEFAULT_PAD_FRAME_DURATION;
  pad->frame_duration_user = FALSE;
}

/*
 * End of pad option handler code
 **/

static void
gst_matroska_mux_init (GstMatroskaMux * mux, gpointer g_class)
{
  GstPadTemplate *templ;

  templ =
      gst_element_class_get_pad_template (GST_ELEMENT_CLASS (g_class), "src");
  mux->srcpad = gst_pad_new_from_template (templ, "src");

  gst_pad_set_event_function (mux->srcpad, gst_matroska_mux_handle_src_event);
  gst_element_add_pad (GST_ELEMENT (mux), mux->srcpad);
  gst_pad_use_fixed_caps (mux->srcpad);

  mux->collect = gst_collect_pads_new ();
  gst_collect_pads_set_clip_function (mux->collect,
      GST_DEBUG_FUNCPTR (gst_collect_pads_clip_running_time), mux);
  gst_collect_pads_set_buffer_function (mux->collect,
      GST_DEBUG_FUNCPTR (gst_matroska_mux_handle_buffer), mux);
  gst_collect_pads_set_event_function (mux->collect,
      GST_DEBUG_FUNCPTR (gst_matroska_mux_handle_sink_event), mux);

  mux->ebml_write = gst_ebml_write_new (mux->srcpad);
  mux->doctype = GST_MATROSKA_DOCTYPE_MATROSKA;

  /* property defaults */
  mux->doctype_version = DEFAULT_DOCTYPE_VERSION;
  mux->writing_app = g_strdup (DEFAULT_WRITING_APP);
  mux->min_index_interval = DEFAULT_MIN_INDEX_INTERVAL;
  mux->ebml_write->streamable = DEFAULT_STREAMABLE;
  mux->time_scale = DEFAULT_TIMECODESCALE;
  mux->min_cluster_duration = DEFAULT_MIN_CLUSTER_DURATION;
  mux->max_cluster_duration = DEFAULT_MAX_CLUSTER_DURATION;

  /* initialize internal variables */
  mux->index = NULL;
  mux->num_streams = 0;
  mux->num_a_streams = 0;
  mux->num_t_streams = 0;
  mux->num_v_streams = 0;
  mux->internal_toc = NULL;

  /* initialize remaining variables */
  gst_matroska_mux_reset (GST_ELEMENT (mux));
}


/**
 * gst_matroska_mux_finalize:
 * @object: #GstMatroskaMux that should be finalized.
 *
 * Finalize matroska muxer.
 */
static void
gst_matroska_mux_finalize (GObject * object)
{
  GstMatroskaMux *mux = GST_MATROSKA_MUX (object);

  gst_event_replace (&mux->force_key_unit_event, NULL);

  gst_object_unref (mux->collect);
  gst_object_unref (mux->ebml_write);
  g_free (mux->writing_app);
  g_clear_pointer (&mux->creation_time, g_date_time_unref);

  if (mux->internal_toc) {
    gst_toc_unref (mux->internal_toc);
    mux->internal_toc = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/**
 * gst_matroska_mux_create_uid:
 * @mux: #GstMatroskaMux to generate UID for.
 *
 * Generate new track UID.
 *
 * Returns: New track UID.
 */
static guint64
gst_matroska_mux_create_uid (GstMatroskaMux * mux)
{
  return (((guint64) g_random_int ()) << 32) | g_random_int ();
}


/**
 * gst_matroska_pad_reset:
 * @collect_pad: the #GstMatroskaPad
 *
 * Reset and/or release resources of a matroska collect pad.
 */
static void
gst_matroska_pad_reset (GstMatroskaPad * collect_pad, gboolean full)
{
  gchar *name = NULL;
  GstMatroskaTrackType type = 0;

  /* free track information */
  if (collect_pad->track != NULL) {
    /* retrieve for optional later use */
    name = collect_pad->track->name;
    type = collect_pad->track->type;
    /* extra for video */
    if (type == GST_MATROSKA_TRACK_TYPE_VIDEO) {
      GstMatroskaTrackVideoContext *ctx =
          (GstMatroskaTrackVideoContext *) collect_pad->track;

      if (ctx->dirac_unit) {
        gst_buffer_unref (ctx->dirac_unit);
        ctx->dirac_unit = NULL;
      }
    }
    g_free (collect_pad->track->codec_id);
    g_free (collect_pad->track->codec_name);
    if (full)
      g_free (collect_pad->track->name);
    g_free (collect_pad->track->language);
    g_free (collect_pad->track->codec_priv);
    g_free (collect_pad->track);
    collect_pad->track = NULL;
    if (collect_pad->tags) {
      gst_tag_list_unref (collect_pad->tags);
      collect_pad->tags = NULL;
    }
  }

  if (!full && type != 0) {
    GstMatroskaTrackContext *context;

    /* create a fresh context */
    switch (type) {
      case GST_MATROSKA_TRACK_TYPE_VIDEO:
        context = (GstMatroskaTrackContext *)
            g_new0 (GstMatroskaTrackVideoContext, 1);
        break;
      case GST_MATROSKA_TRACK_TYPE_AUDIO:
        context = (GstMatroskaTrackContext *)
            g_new0 (GstMatroskaTrackAudioContext, 1);
        break;
      case GST_MATROSKA_TRACK_TYPE_SUBTITLE:
        context = (GstMatroskaTrackContext *)
            g_new0 (GstMatroskaTrackSubtitleContext, 1);
        break;
      default:
        g_assert_not_reached ();
        return;
    }

    context->type = type;
    context->name = name;
    context->uid = gst_matroska_mux_create_uid (collect_pad->mux);
    /* TODO: check default values for the context */
    context->flags = GST_MATROSKA_TRACK_ENABLED | GST_MATROSKA_TRACK_DEFAULT;
    collect_pad->track = context;
    collect_pad->start_ts = GST_CLOCK_TIME_NONE;
    collect_pad->end_ts = GST_CLOCK_TIME_NONE;
    collect_pad->tags = gst_tag_list_new_empty ();
    gst_tag_list_set_scope (collect_pad->tags, GST_TAG_SCOPE_STREAM);
  }
}

/**
 * gst_matroska_pad_free:
 * @collect_pad: the #GstMatroskaPad
 *
 * Release resources of a matroska collect pad.
 */
static void
gst_matroska_pad_free (GstPad * collect_pad)
{
  gst_matroska_pad_reset ((GstMatroskaPad *) collect_pad, TRUE);
}


/**
 * gst_matroska_mux_reset:
 * @element: #GstMatroskaMux that should be reset.
 *
 * Reset matroska muxer back to initial state.
 */
static void
gst_matroska_mux_reset (GstElement * element)
{
  GstMatroskaMux *mux = GST_MATROSKA_MUX (element);
  GSList *walk;

  /* reset EBML write */
  gst_ebml_write_reset (mux->ebml_write);

  /* reset input */
  mux->state = GST_MATROSKA_MUX_STATE_START;

  /* clean up existing streams */

  for (walk = mux->collect->data; walk; walk = g_slist_next (walk)) {
    GstMatroskaPad *collect_pad;

    collect_pad = (GstMatroskaPad *) walk->data;

    /* reset collect pad to pristine state */
    gst_matroska_pad_reset (collect_pad, FALSE);
  }

  /* reset indexes */
  mux->num_indexes = 0;
  g_free (mux->index);
  mux->index = NULL;

  /* reset timers */
  mux->duration = 0;

  /* reset cluster */
  mux->cluster = 0;
  mux->cluster_time = 0;
  mux->cluster_pos = 0;
  mux->prev_cluster_size = 0;

  /* reset tags */
  gst_tag_setter_reset_tags (GST_TAG_SETTER (mux));

  mux->tags_pos = 0;

  /* reset chapters */
  gst_toc_setter_reset (GST_TOC_SETTER (mux));
  if (mux->internal_toc) {
    gst_toc_unref (mux->internal_toc);
    mux->internal_toc = NULL;
  }

  mux->chapters_pos = 0;
}

/**
 * gst_matroska_mux_handle_src_event:
 * @pad: Pad which received the event.
 * @event: Received event.
 *
 * handle events - copied from oggmux without understanding
 *
 * Returns: %TRUE on success.
 */
static gboolean
gst_matroska_mux_handle_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstEventType type;

  type = event ? GST_EVENT_TYPE (event) : GST_EVENT_UNKNOWN;

  switch (type) {
    case GST_EVENT_SEEK:
      /* disable seeking for now */
      return FALSE;
    default:
      break;
  }

  return gst_pad_event_default (pad, parent, event);
}


static void
gst_matroska_mux_free_codec_priv (GstMatroskaTrackContext * context)
{
  if (context->codec_priv != NULL) {
    g_free (context->codec_priv);
    context->codec_priv = NULL;
    context->codec_priv_size = 0;
  }
}

static void
gst_matroska_mux_build_vobsub_private (GstMatroskaTrackContext * context,
    const guint * clut)
{
  gchar *clutv[17];
  gchar *sclut;
  gint i;
  guint32 col;
  gdouble y, u, v;
  guint8 r, g, b;

  /* produce comma-separated list in hex format */
  for (i = 0; i < 16; ++i) {
    col = clut[i];
    /* replicate vobsub's slightly off RGB conversion calculation */
    y = (((col >> 16) & 0xff) - 16) * 255 / 219;
    u = ((col >> 8) & 0xff) - 128;
    v = (col & 0xff) - 128;
    r = CLAMP (1.0 * y + 1.4022 * u, 0, 255);
    g = CLAMP (1.0 * y - 0.3456 * u - 0.7145 * v, 0, 255);
    b = CLAMP (1.0 * y + 1.7710 * v, 0, 255);
    clutv[i] = g_strdup_printf ("%02x%02x%02x", r, g, b);
  }
  clutv[i] = NULL;
  sclut = g_strjoinv (",", clutv);

  /* build codec private; only palette for now */
  gst_matroska_mux_free_codec_priv (context);
  context->codec_priv = (guint8 *) g_strdup_printf ("palette: %s", sclut);
  /* include terminating 0 */
  context->codec_priv_size = strlen ((gchar *) context->codec_priv) + 1;
  g_free (sclut);
  for (i = 0; i < 16; ++i) {
    g_free (clutv[i]);
  }
}


/**
 * gst_matroska_mux_handle_sink_event:
 * @pad: Pad which received the event.
 * @event: Received event.
 *
 * handle events - informational ones like tags
 *
 * Returns: %TRUE on success.
 */
static gboolean
gst_matroska_mux_handle_sink_event (GstCollectPads * pads,
    GstCollectData * data, GstEvent * event, gpointer user_data)
{
  GstMatroskaPad *collect_pad;
  GstMatroskaTrackContext *context;
  GstMatroskaMux *mux;
  GstPad *pad;
  GstTagList *list;
  gboolean ret = TRUE;

  mux = GST_MATROSKA_MUX (user_data);
  collect_pad = (GstMatroskaPad *) data;
  pad = data->pad;
  context = collect_pad->track;
  g_assert (context);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:{
      GstCaps *caps;

      collect_pad = (GstMatroskaPad *) gst_pad_get_element_private (pad);
      gst_event_parse_caps (event, &caps);

      ret = collect_pad->capsfunc (pad, caps);
      gst_event_unref (event);
      event = NULL;
      break;
    }
    case GST_EVENT_TAG:{
      gchar *lang = NULL;

      GST_DEBUG_OBJECT (mux, "received tag event");
      gst_event_parse_tag (event, &list);

      /* Matroska wants ISO 639-2B code, taglist most likely contains 639-1 */
      if (gst_tag_list_get_string (list, GST_TAG_LANGUAGE_CODE, &lang)) {
        const gchar *lang_code;

        lang_code = gst_tag_get_language_code_iso_639_2B (lang);
        if (lang_code) {
          GST_INFO_OBJECT (pad, "Setting language to '%s'", lang_code);
          g_free (context->language);
          context->language = g_strdup (lang_code);
        } else {
          GST_WARNING_OBJECT (pad, "Did not get language code for '%s'", lang);
        }
        g_free (lang);
      }

      /* FIXME: what about stream-specific tags? */
      if (gst_tag_list_get_scope (list) == GST_TAG_SCOPE_GLOBAL) {
        gst_tag_setter_merge_tags (GST_TAG_SETTER (mux), list,
            gst_tag_setter_get_tag_merge_mode (GST_TAG_SETTER (mux)));
      } else {
        gst_tag_list_insert (collect_pad->tags, list, GST_TAG_MERGE_REPLACE);
      }

      gst_event_unref (event);
      /* handled this, don't want collectpads to forward it downstream */
      event = NULL;
      ret = TRUE;
      break;
    }
    case GST_EVENT_TOC:{
      GstToc *toc, *old_toc;

      if (mux->chapters_pos > 0)
        break;

      GST_DEBUG_OBJECT (mux, "received toc event");
      gst_event_parse_toc (event, &toc, NULL);

      if (toc != NULL) {
        old_toc = gst_toc_setter_get_toc (GST_TOC_SETTER (mux));
        if (old_toc != NULL) {
          if (old_toc != toc)
            GST_INFO_OBJECT (pad, "Replacing TOC with a new one");
          gst_toc_unref (old_toc);
        }

        gst_toc_setter_set_toc (GST_TOC_SETTER (mux), toc);
        gst_toc_unref (toc);
      }

      gst_event_unref (event);
      /* handled this, don't want collectpads to forward it downstream */
      event = NULL;
      break;
    }
    case GST_EVENT_CUSTOM_DOWNSTREAM:
    case GST_EVENT_CUSTOM_DOWNSTREAM_STICKY:{
      const GstStructure *structure;

      structure = gst_event_get_structure (event);
      if (gst_structure_has_name (structure, "GstForceKeyUnit")) {
        gst_event_replace (&mux->force_key_unit_event, NULL);
        mux->force_key_unit_event = event;
        event = NULL;
      } else if (gst_structure_has_name (structure, "application/x-gst-dvd") &&
          !strcmp ("dvd-spu-clut-change",
              gst_structure_get_string (structure, "event"))) {
        gchar name[16];
        gint i, value;
        guint clut[16];

        GST_DEBUG_OBJECT (pad, "New DVD colour table received");
        if (context->type != GST_MATROSKA_TRACK_TYPE_SUBTITLE) {
          GST_DEBUG_OBJECT (pad, "... discarding");
          break;
        }
        /* first transform event data into table form */
        for (i = 0; i < 16; i++) {
          g_snprintf (name, sizeof (name), "clut%02d", i);
          if (!gst_structure_get_int (structure, name, &value)) {
            GST_ERROR_OBJECT (mux, "dvd-spu-clut-change event did not "
                "contain %s field", name);
            goto break_hard;
          }
          clut[i] = value;
        }

        /* transform into private data for stream; text form */
        gst_matroska_mux_build_vobsub_private (context, clut);
      }
    }
      /* fall through */
    default:
      break;
  }

break_hard:
  if (event != NULL)
    return gst_collect_pads_event_default (pads, data, event, FALSE);

  return ret;
}

static void
gst_matroska_mux_set_codec_id (GstMatroskaTrackContext * context,
    const char *id)
{
  g_assert (context && id);
  g_free (context->codec_id);
  context->codec_id = g_strdup (id);
}

/**
 * gst_matroska_mux_video_pad_setcaps:
 * @pad: Pad which got the caps.
 * @caps: New caps.
 *
 * Setcaps function for video sink pad.
 *
 * Returns: %TRUE on success.
 */
static gboolean
gst_matroska_mux_video_pad_setcaps (GstPad * pad, GstCaps * caps)
{
  GstMatroskaTrackContext *context = NULL;
  GstMatroskaTrackVideoContext *videocontext;
  GstMatroskaMux *mux;
  GstMatroskaPad *collect_pad;
  GstStructure *structure;
  const gchar *mimetype;
  const gchar *interlace_mode, *s;
  const GValue *value = NULL;
  GstBuffer *codec_buf = NULL;
  gint width, height, pixel_width, pixel_height;
  gint fps_d, fps_n;
  guint multiview_flags;
  GstCaps *old_caps;

  mux = GST_MATROSKA_MUX (GST_PAD_PARENT (pad));

  if ((old_caps = gst_pad_get_current_caps (pad))) {
    if (mux->state >= GST_MATROSKA_MUX_STATE_HEADER
        && !gst_caps_is_equal (caps, old_caps)) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("Caps changes are not supported by Matroska"));
      gst_caps_unref (old_caps);
      goto refuse_caps;
    }
    gst_caps_unref (old_caps);
  }

  /* find context */
  collect_pad = (GstMatroskaPad *) gst_pad_get_element_private (pad);
  g_assert (collect_pad);
  context = collect_pad->track;
  g_assert (context);
  g_assert (context->type == GST_MATROSKA_TRACK_TYPE_VIDEO);
  videocontext = (GstMatroskaTrackVideoContext *) context;

  /* gst -> matroska ID'ing */
  structure = gst_caps_get_structure (caps, 0);

  mimetype = gst_structure_get_name (structure);

  interlace_mode = gst_structure_get_string (structure, "interlace-mode");
  if (interlace_mode != NULL) {
    if (strcmp (interlace_mode, "progressive") == 0)
      videocontext->interlace_mode = GST_MATROSKA_INTERLACE_MODE_PROGRESSIVE;
    else
      videocontext->interlace_mode = GST_MATROSKA_INTERLACE_MODE_INTERLACED;
  } else {
    videocontext->interlace_mode = GST_MATROSKA_INTERLACE_MODE_UNKNOWN;
  }

  if (!strcmp (mimetype, "video/x-theora")) {
    /* we'll extract the details later from the theora identification header */
    goto skip_details;
  }

  /* get general properties */
  /* spec says it is mandatory */
  if (!gst_structure_get_int (structure, "width", &width) ||
      !gst_structure_get_int (structure, "height", &height))
    goto refuse_caps;

  videocontext->pixel_width = width;
  videocontext->pixel_height = height;

  if (GST_MATROSKAMUX_PAD_CAST (pad)->frame_duration
      && gst_structure_get_fraction (structure, "framerate", &fps_n, &fps_d)
      && fps_n > 0) {
    context->default_duration =
        gst_util_uint64_scale_int (GST_SECOND, fps_d, fps_n);
    GST_LOG_OBJECT (pad, "default duration = %" GST_TIME_FORMAT,
        GST_TIME_ARGS (context->default_duration));
  } else {
    context->default_duration = 0;
  }
  if (gst_structure_get_fraction (structure, "pixel-aspect-ratio",
          &pixel_width, &pixel_height)) {
    if (pixel_width > pixel_height) {
      videocontext->display_width = width * pixel_width / pixel_height;
      videocontext->display_height = height;
    } else if (pixel_width < pixel_height) {
      videocontext->display_width = width;
      videocontext->display_height = height * pixel_height / pixel_width;
    } else {
      videocontext->display_width = 0;
      videocontext->display_height = 0;
    }
  } else {
    videocontext->display_width = 0;
    videocontext->display_height = 0;
  }

  if ((s = gst_structure_get_string (structure, "colorimetry"))) {
    if (!gst_video_colorimetry_from_string (&videocontext->colorimetry, s)) {
      GST_WARNING_OBJECT (pad, "Could not parse colorimetry %s", s);
    }
  }

  if ((s = gst_structure_get_string (structure, "mastering-display-info"))) {
    if (!gst_video_mastering_display_info_from_string
        (&videocontext->mastering_display_info, s)) {
      GST_WARNING_OBJECT (pad, "Could not parse mastering-display-metadata %s",
          s);
    } else {
      videocontext->mastering_display_info_present = TRUE;
    }
  }

  if ((s = gst_structure_get_string (structure, "content-light-level"))) {
    if (!gst_video_content_light_level_from_string
        (&videocontext->content_light_level, s))
      GST_WARNING_OBJECT (pad, "Could not parse content-light-level %s", s);
  }

  /* Collect stereoscopic info, if any */
  if ((s = gst_structure_get_string (structure, "multiview-mode")))
    videocontext->multiview_mode =
        gst_video_multiview_mode_from_caps_string (s);
  gst_structure_get_flagset (structure, "multiview-flags", &multiview_flags,
      NULL);
  videocontext->multiview_flags = multiview_flags;


skip_details:

  videocontext->asr_mode = GST_MATROSKA_ASPECT_RATIO_MODE_FREE;
  videocontext->fourcc = 0;

  /* TODO: - check if we handle all codecs by the spec, i.e. codec private
   *         data and other settings
   *       - add new formats
   */

  /* extract codec_data, may turn out needed */
  value = gst_structure_get_value (structure, "codec_data");
  if (value)
    codec_buf = (GstBuffer *) gst_value_get_buffer (value);

  /* find type */
  if (!strcmp (mimetype, "video/x-raw")) {
    const gchar *fstr;
    gst_matroska_mux_set_codec_id (context,
        GST_MATROSKA_CODEC_ID_VIDEO_UNCOMPRESSED);
    fstr = gst_structure_get_string (structure, "format");
    if (fstr) {
      if (strlen (fstr) == 4)
        videocontext->fourcc = GST_STR_FOURCC (fstr);
      else if (!strcmp (fstr, "GRAY8"))
        videocontext->fourcc = GST_MAKE_FOURCC ('Y', '8', '0', '0');
      else if (!strcmp (fstr, "BGR"))
        videocontext->fourcc = GST_MAKE_FOURCC ('B', 'G', 'R', 24);
      else if (!strcmp (fstr, "RGB"))
        videocontext->fourcc = GST_MAKE_FOURCC ('R', 'G', 'B', 24);
    }
  } else if (!strcmp (mimetype, "video/x-huffyuv")      /* MS/VfW compatibility cases */
      ||!strcmp (mimetype, "video/x-divx")
      || !strcmp (mimetype, "video/x-dv")
      || !strcmp (mimetype, "video/x-h263")
      || !strcmp (mimetype, "video/x-msmpeg")
      || !strcmp (mimetype, "video/x-wmv")
      || !strcmp (mimetype, "image/jpeg")) {
    gst_riff_strf_vids *bih;
    gint size = sizeof (gst_riff_strf_vids);
    guint32 fourcc = 0;

    if (!strcmp (mimetype, "video/x-huffyuv"))
      fourcc = GST_MAKE_FOURCC ('H', 'F', 'Y', 'U');
    else if (!strcmp (mimetype, "video/x-dv"))
      fourcc = GST_MAKE_FOURCC ('D', 'V', 'S', 'D');
    else if (!strcmp (mimetype, "video/x-h263"))
      fourcc = GST_MAKE_FOURCC ('H', '2', '6', '3');
    else if (!strcmp (mimetype, "video/x-divx")) {
      gint divxversion;

      gst_structure_get_int (structure, "divxversion", &divxversion);
      switch (divxversion) {
        case 3:
          fourcc = GST_MAKE_FOURCC ('D', 'I', 'V', '3');
          break;
        case 4:
          fourcc = GST_MAKE_FOURCC ('D', 'I', 'V', 'X');
          break;
        case 5:
          fourcc = GST_MAKE_FOURCC ('D', 'X', '5', '0');
          break;
      }
    } else if (!strcmp (mimetype, "video/x-msmpeg")) {
      gint msmpegversion;

      gst_structure_get_int (structure, "msmpegversion", &msmpegversion);
      switch (msmpegversion) {
        case 41:
          fourcc = GST_MAKE_FOURCC ('M', 'P', 'G', '4');
          break;
        case 42:
          fourcc = GST_MAKE_FOURCC ('M', 'P', '4', '2');
          break;
        case 43:
          goto msmpeg43;
          break;
      }
    } else if (!strcmp (mimetype, "video/x-wmv")) {
      gint wmvversion;
      const gchar *fstr;

      fstr = gst_structure_get_string (structure, "format");
      if (fstr && strlen (fstr) == 4) {
        fourcc = GST_STR_FOURCC (fstr);
      } else if (gst_structure_get_int (structure, "wmvversion", &wmvversion)) {
        if (wmvversion == 2) {
          fourcc = GST_MAKE_FOURCC ('W', 'M', 'V', '2');
        } else if (wmvversion == 1) {
          fourcc = GST_MAKE_FOURCC ('W', 'M', 'V', '1');
        } else if (wmvversion == 3) {
          fourcc = GST_MAKE_FOURCC ('W', 'M', 'V', '3');
        }
      }
    } else if (!strcmp (mimetype, "image/jpeg")) {
      fourcc = GST_MAKE_FOURCC ('M', 'J', 'P', 'G');
    }

    if (!fourcc)
      goto refuse_caps;

    bih = g_new0 (gst_riff_strf_vids, 1);
    GST_WRITE_UINT32_LE (&bih->size, size);
    GST_WRITE_UINT32_LE (&bih->width, videocontext->pixel_width);
    GST_WRITE_UINT32_LE (&bih->height, videocontext->pixel_height);
    GST_WRITE_UINT32_LE (&bih->compression, fourcc);
    GST_WRITE_UINT16_LE (&bih->planes, (guint16) 1);
    GST_WRITE_UINT16_LE (&bih->bit_cnt, (guint16) 24);
    GST_WRITE_UINT32_LE (&bih->image_size, videocontext->pixel_width *
        videocontext->pixel_height * 3);

    /* process codec private/initialization data, if any */
    if (codec_buf) {
      size += gst_buffer_get_size (codec_buf);
      bih = g_realloc (bih, size);
      GST_WRITE_UINT32_LE (&bih->size, size);
      gst_buffer_extract (codec_buf, 0,
          (guint8 *) bih + sizeof (gst_riff_strf_vids), -1);
    }

    gst_matroska_mux_set_codec_id (context,
        GST_MATROSKA_CODEC_ID_VIDEO_VFW_FOURCC);
    gst_matroska_mux_free_codec_priv (context);
    context->codec_priv = (gpointer) bih;
    context->codec_priv_size = size;
    context->dts_only = TRUE;
  } else if (!strcmp (mimetype, "video/x-h264")) {
    gst_matroska_mux_set_codec_id (context,
        GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_AVC);
    gst_matroska_mux_free_codec_priv (context);
    /* Create avcC header */
    if (codec_buf != NULL) {
      context->codec_priv_size = gst_buffer_get_size (codec_buf);
      context->codec_priv = g_malloc0 (context->codec_priv_size);
      gst_buffer_extract (codec_buf, 0, context->codec_priv, -1);
    }
  } else if (!strcmp (mimetype, "video/x-h265")) {
    gst_matroska_mux_set_codec_id (context,
        GST_MATROSKA_CODEC_ID_VIDEO_MPEGH_HEVC);
    gst_matroska_mux_free_codec_priv (context);
    /* Create hvcC header */
    if (codec_buf != NULL) {
      context->codec_priv_size = gst_buffer_get_size (codec_buf);
      context->codec_priv = g_malloc0 (context->codec_priv_size);
      gst_buffer_extract (codec_buf, 0, context->codec_priv, -1);
    }
  } else if (!strcmp (mimetype, "video/x-theora")) {
    const GValue *streamheader;

    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_VIDEO_THEORA);

    gst_matroska_mux_free_codec_priv (context);

    streamheader = gst_structure_get_value (structure, "streamheader");
    if (!theora_streamheader_to_codecdata (streamheader, context)) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("theora stream headers missing or malformed"));
      goto refuse_caps;
    }
  } else if (!strcmp (mimetype, "video/x-dirac")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_VIDEO_DIRAC);
  } else if (!strcmp (mimetype, "video/x-vp8")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_VIDEO_VP8);
  } else if (!strcmp (mimetype, "video/x-vp9")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_VIDEO_VP9);
  } else if (!strcmp (mimetype, "video/x-av1")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_VIDEO_AV1);
    gst_matroska_mux_free_codec_priv (context);
    /* Create av1C header */
    if (codec_buf != NULL)
      gst_buffer_extract_dup (codec_buf, 0, gst_buffer_get_size (codec_buf),
          &context->codec_priv, &context->codec_priv_size);
  } else if (!strcmp (mimetype, "video/mpeg")) {
    gint mpegversion;

    gst_structure_get_int (structure, "mpegversion", &mpegversion);
    switch (mpegversion) {
      case 1:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_VIDEO_MPEG1);
        break;
      case 2:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_VIDEO_MPEG2);
        break;
      case 4:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_ASP);
        break;
      default:
        goto refuse_caps;
    }

    /* global headers may be in codec data */
    if (codec_buf != NULL) {
      gst_matroska_mux_free_codec_priv (context);
      context->codec_priv_size = gst_buffer_get_size (codec_buf);
      context->codec_priv = g_malloc0 (context->codec_priv_size);
      gst_buffer_extract (codec_buf, 0, context->codec_priv, -1);
    }
  } else if (!strcmp (mimetype, "video/x-msmpeg")) {
  msmpeg43:
    /* can only make it here if preceding case verified it was version 3 */
    gst_matroska_mux_set_codec_id (context,
        GST_MATROSKA_CODEC_ID_VIDEO_MSMPEG4V3);
  } else if (!strcmp (mimetype, "video/x-pn-realvideo")) {
    gint rmversion;
    const GValue *mdpr_data;

    gst_structure_get_int (structure, "rmversion", &rmversion);
    switch (rmversion) {
      case 1:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO1);
        break;
      case 2:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO2);
        break;
      case 3:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO3);
        break;
      case 4:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO4);
        break;
      default:
        goto refuse_caps;
    }

    mdpr_data = gst_structure_get_value (structure, "mdpr_data");
    if (mdpr_data != NULL) {
      guint8 *priv_data = NULL;
      guint priv_data_size = 0;

      GstBuffer *codec_data_buf = g_value_peek_pointer (mdpr_data);

      priv_data_size = gst_buffer_get_size (codec_data_buf);
      priv_data = g_malloc0 (priv_data_size);

      gst_buffer_extract (codec_data_buf, 0, priv_data, -1);

      gst_matroska_mux_free_codec_priv (context);
      context->codec_priv = priv_data;
      context->codec_priv_size = priv_data_size;
    }
  } else if (strcmp (mimetype, "video/x-prores") == 0) {
    const gchar *variant;

    gst_matroska_mux_free_codec_priv (context);

    variant = gst_structure_get_string (structure, "format");
    if (!variant || !g_strcmp0 (variant, "standard"))
      context->codec_priv = g_strdup ("apcn");
    else if (!g_strcmp0 (variant, "hq"))
      context->codec_priv = g_strdup ("apch");
    else if (!g_strcmp0 (variant, "lt"))
      context->codec_priv = g_strdup ("apcs");
    else if (!g_strcmp0 (variant, "proxy"))
      context->codec_priv = g_strdup ("apco");
    else if (!g_strcmp0 (variant, "4444"))
      context->codec_priv = g_strdup ("ap4h");
    else {
      GST_WARNING_OBJECT (mux, "Unhandled prores format: %s", variant);

      goto refuse_caps;
    }

    context->codec_priv_size = sizeof (guint32);
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_VIDEO_PRORES);
  }

  return TRUE;

  /* ERRORS */
refuse_caps:
  {
    GST_WARNING_OBJECT (mux, "pad %s refused caps %" GST_PTR_FORMAT,
        GST_PAD_NAME (pad), caps);
    return FALSE;
  }
}

/* N > 0 to expect a particular number of headers, negative if the
   number of headers is variable */
static gboolean
xiphN_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context, GstBuffer ** p_buf0, int N)
{
  GstBuffer **buf = NULL;
  GArray *bufarr;
  guint8 *priv_data;
  guint bufi, i, offset, priv_data_size;

  if (streamheader == NULL)
    goto no_stream_headers;

  if (G_VALUE_TYPE (streamheader) != GST_TYPE_ARRAY)
    goto wrong_type;

  bufarr = g_value_peek_pointer (streamheader);
  if (bufarr->len <= 0 || bufarr->len > 255)    /* at least one header, and count stored in a byte */
    goto wrong_count;
  if (N > 0 && bufarr->len != N)
    goto wrong_count;

  context->xiph_headers_to_skip = bufarr->len;

  buf = (GstBuffer **) g_malloc0 (sizeof (GstBuffer *) * bufarr->len);
  for (i = 0; i < bufarr->len; i++) {
    GValue *bufval = &g_array_index (bufarr, GValue, i);

    if (G_VALUE_TYPE (bufval) != GST_TYPE_BUFFER) {
      g_free (buf);
      goto wrong_content_type;
    }

    buf[i] = g_value_peek_pointer (bufval);
  }

  priv_data_size = 1;
  if (bufarr->len > 0) {
    for (i = 0; i < bufarr->len - 1; i++) {
      priv_data_size += gst_buffer_get_size (buf[i]) / 0xff + 1;
    }
  }

  for (i = 0; i < bufarr->len; ++i) {
    priv_data_size += gst_buffer_get_size (buf[i]);
  }

  priv_data = g_malloc0 (priv_data_size);

  priv_data[0] = bufarr->len - 1;
  offset = 1;

  if (bufarr->len > 0) {
    for (bufi = 0; bufi < bufarr->len - 1; bufi++) {
      for (i = 0; i < gst_buffer_get_size (buf[bufi]) / 0xff; ++i) {
        priv_data[offset++] = 0xff;
      }
      priv_data[offset++] = gst_buffer_get_size (buf[bufi]) % 0xff;
    }
  }

  for (i = 0; i < bufarr->len; ++i) {
    gst_buffer_extract (buf[i], 0, priv_data + offset, -1);
    offset += gst_buffer_get_size (buf[i]);
  }

  gst_matroska_mux_free_codec_priv (context);
  context->codec_priv = priv_data;
  context->codec_priv_size = priv_data_size;

  if (p_buf0)
    *p_buf0 = gst_buffer_ref (buf[0]);

  g_free (buf);

  return TRUE;

/* ERRORS */
no_stream_headers:
  {
    GST_WARNING ("required streamheaders missing in sink caps!");
    return FALSE;
  }
wrong_type:
  {
    GST_WARNING ("streamheaders are not a GST_TYPE_ARRAY, but a %s",
        G_VALUE_TYPE_NAME (streamheader));
    return FALSE;
  }
wrong_count:
  {
    GST_WARNING ("got %u streamheaders, not %d as expected", bufarr->len, N);
    return FALSE;
  }
wrong_content_type:
  {
    GST_WARNING ("streamheaders array does not contain GstBuffers");
    return FALSE;
  }
}

static gboolean
vorbis_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context)
{
  GstBuffer *buf0 = NULL;

  if (!xiphN_streamheader_to_codecdata (streamheader, context, &buf0, 3))
    return FALSE;

  if (buf0 == NULL || gst_buffer_get_size (buf0) < 1 + 6 + 4) {
    GST_WARNING ("First vorbis header too small, ignoring");
  } else {
    if (gst_buffer_memcmp (buf0, 1, "vorbis", 6) == 0) {
      GstMatroskaTrackAudioContext *audiocontext;
      GstMapInfo map;
      guint8 *hdr;

      gst_buffer_map (buf0, &map, GST_MAP_READ);
      hdr = map.data + 1 + 6 + 4;
      audiocontext = (GstMatroskaTrackAudioContext *) context;
      audiocontext->channels = GST_READ_UINT8 (hdr);
      audiocontext->samplerate = GST_READ_UINT32_LE (hdr + 1);
      gst_buffer_unmap (buf0, &map);
    }
  }

  if (buf0)
    gst_buffer_unref (buf0);

  return TRUE;
}

static gboolean
theora_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context)
{
  GstBuffer *buf0 = NULL;

  if (!xiphN_streamheader_to_codecdata (streamheader, context, &buf0, 3))
    return FALSE;

  if (buf0 == NULL || gst_buffer_get_size (buf0) < 1 + 6 + 26) {
    GST_WARNING ("First theora header too small, ignoring");
  } else if (gst_buffer_memcmp (buf0, 0, "\200theora\003\002", 9) != 0) {
    GST_WARNING ("First header not a theora identification header, ignoring");
  } else {
    GstMatroskaTrackVideoContext *videocontext;
    guint fps_num, fps_denom, par_num, par_denom;
    GstMapInfo map;
    guint8 *hdr;

    gst_buffer_map (buf0, &map, GST_MAP_READ);
    hdr = map.data + 1 + 6 + 3 + 2 + 2;

    videocontext = (GstMatroskaTrackVideoContext *) context;
    videocontext->pixel_width = GST_READ_UINT32_BE (hdr) >> 8;
    videocontext->pixel_height = GST_READ_UINT32_BE (hdr + 3) >> 8;
    hdr += 3 + 3 + 1 + 1;
    fps_num = GST_READ_UINT32_BE (hdr);
    fps_denom = GST_READ_UINT32_BE (hdr + 4);
    context->default_duration = gst_util_uint64_scale_int (GST_SECOND,
        fps_denom, fps_num);
    hdr += 4 + 4;
    par_num = GST_READ_UINT32_BE (hdr) >> 8;
    par_denom = GST_READ_UINT32_BE (hdr + 3) >> 8;
    if (par_num > 0 && par_denom > 0) {
      if (par_num > par_denom) {
        videocontext->display_width =
            videocontext->pixel_width * par_num / par_denom;
        videocontext->display_height = videocontext->pixel_height;
      } else if (par_num < par_denom) {
        videocontext->display_width = videocontext->pixel_width;
        videocontext->display_height =
            videocontext->pixel_height * par_denom / par_num;
      } else {
        videocontext->display_width = 0;
        videocontext->display_height = 0;
      }
    } else {
      videocontext->display_width = 0;
      videocontext->display_height = 0;
    }

    gst_buffer_unmap (buf0, &map);
  }

  if (buf0)
    gst_buffer_unref (buf0);

  return TRUE;
}

static gboolean
kate_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context)
{
  GstBuffer *buf0 = NULL;

  if (!xiphN_streamheader_to_codecdata (streamheader, context, &buf0, -1))
    return FALSE;

  if (buf0 == NULL || gst_buffer_get_size (buf0) < 64) {        /* Kate ID header is 64 bytes */
    GST_WARNING ("First kate header too small, ignoring");
  } else if (gst_buffer_memcmp (buf0, 0, "\200kate\0\0\0", 8) != 0) {
    GST_WARNING ("First header not a kate identification header, ignoring");
  }

  if (buf0)
    gst_buffer_unref (buf0);

  return TRUE;
}

static gboolean
flac_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context)
{
  GArray *bufarr;
  gint i;
  GValue *bufval;
  GstBuffer *buffer;

  if (streamheader == NULL || G_VALUE_TYPE (streamheader) != GST_TYPE_ARRAY) {
    GST_WARNING ("No or invalid streamheader field in the caps");
    return FALSE;
  }

  bufarr = g_value_peek_pointer (streamheader);
  if (bufarr->len < 2) {
    GST_WARNING ("Too few headers in streamheader field");
    return FALSE;
  }

  context->xiph_headers_to_skip = bufarr->len + 1;

  bufval = &g_array_index (bufarr, GValue, 0);
  if (G_VALUE_TYPE (bufval) != GST_TYPE_BUFFER) {
    GST_WARNING ("streamheaders array does not contain GstBuffers");
    return FALSE;
  }

  buffer = g_value_peek_pointer (bufval);

  /* Need at least OggFLAC mapping header, fLaC marker and STREAMINFO block */
  if (gst_buffer_get_size (buffer) < 9 + 4 + 4 + 34
      || gst_buffer_memcmp (buffer, 1, "FLAC", 4) != 0
      || gst_buffer_memcmp (buffer, 9, "fLaC", 4) != 0) {
    GST_WARNING ("Invalid streamheader for FLAC");
    return FALSE;
  }

  gst_matroska_mux_free_codec_priv (context);
  context->codec_priv_size = gst_buffer_get_size (buffer) - 9;
  context->codec_priv = g_malloc (context->codec_priv_size);
  gst_buffer_extract (buffer, 9, context->codec_priv, -1);

  for (i = 1; i < bufarr->len; i++) {
    guint old_size;
    bufval = &g_array_index (bufarr, GValue, i);

    if (G_VALUE_TYPE (bufval) != GST_TYPE_BUFFER) {
      gst_matroska_mux_free_codec_priv (context);
      GST_WARNING ("streamheaders array does not contain GstBuffers");
      return FALSE;
    }

    buffer = g_value_peek_pointer (bufval);

    old_size = context->codec_priv_size;
    context->codec_priv_size += gst_buffer_get_size (buffer);

    context->codec_priv = g_realloc (context->codec_priv,
        context->codec_priv_size);
    gst_buffer_extract (buffer, 0,
        (guint8 *) context->codec_priv + old_size, -1);
  }

  return TRUE;
}

static gboolean
speex_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context)
{
  GArray *bufarr;
  GValue *bufval;
  GstBuffer *buffer;
  guint old_size;

  if (streamheader == NULL || G_VALUE_TYPE (streamheader) != GST_TYPE_ARRAY) {
    GST_WARNING ("No or invalid streamheader field in the caps");
    return FALSE;
  }

  bufarr = g_value_peek_pointer (streamheader);
  if (bufarr->len != 2) {
    GST_WARNING ("Too few headers in streamheader field");
    return FALSE;
  }

  context->xiph_headers_to_skip = bufarr->len + 1;

  bufval = &g_array_index (bufarr, GValue, 0);
  if (G_VALUE_TYPE (bufval) != GST_TYPE_BUFFER) {
    GST_WARNING ("streamheaders array does not contain GstBuffers");
    return FALSE;
  }

  buffer = g_value_peek_pointer (bufval);

  if (gst_buffer_get_size (buffer) < 80
      || gst_buffer_memcmp (buffer, 0, "Speex   ", 8) != 0) {
    GST_WARNING ("Invalid streamheader for Speex");
    return FALSE;
  }

  gst_matroska_mux_free_codec_priv (context);
  context->codec_priv_size = gst_buffer_get_size (buffer);
  context->codec_priv = g_malloc (context->codec_priv_size);
  gst_buffer_extract (buffer, 0, context->codec_priv, -1);

  bufval = &g_array_index (bufarr, GValue, 1);

  if (G_VALUE_TYPE (bufval) != GST_TYPE_BUFFER) {
    gst_matroska_mux_free_codec_priv (context);
    GST_WARNING ("streamheaders array does not contain GstBuffers");
    return FALSE;
  }

  buffer = g_value_peek_pointer (bufval);

  old_size = context->codec_priv_size;
  context->codec_priv_size += gst_buffer_get_size (buffer);
  context->codec_priv = g_realloc (context->codec_priv,
      context->codec_priv_size);
  gst_buffer_extract (buffer, 0, (guint8 *) context->codec_priv + old_size, -1);

  return TRUE;
}

static gboolean
opus_streamheader_to_codecdata (const GValue * streamheader,
    GstMatroskaTrackContext * context)
{
  GArray *bufarr;
  GValue *bufval;
  GstBuffer *buf;

  if (G_VALUE_TYPE (streamheader) != GST_TYPE_ARRAY)
    goto wrong_type;

  bufarr = g_value_peek_pointer (streamheader);
  if (bufarr->len != 1 && bufarr->len != 2)     /* one header, and count stored in a byte */
    goto wrong_count;

  /* Opus headers are not in-band */
  context->xiph_headers_to_skip = 0;

  bufval = &g_array_index (bufarr, GValue, 0);
  if (G_VALUE_TYPE (bufval) != GST_TYPE_BUFFER) {
    goto wrong_content_type;
  }
  buf = g_value_peek_pointer (bufval);

  gst_matroska_mux_free_codec_priv (context);

  context->codec_priv_size = gst_buffer_get_size (buf);
  context->codec_priv = g_malloc0 (context->codec_priv_size);
  gst_buffer_extract (buf, 0, context->codec_priv, -1);

  context->codec_delay =
      GST_READ_UINT16_LE ((guint8 *) context->codec_priv + 10);
  context->codec_delay =
      gst_util_uint64_scale_round (context->codec_delay, GST_SECOND, 48000);
  context->seek_preroll = 80 * GST_MSECOND;

  return TRUE;

/* ERRORS */
wrong_type:
  {
    GST_WARNING ("streamheaders are not a GST_TYPE_ARRAY, but a %s",
        G_VALUE_TYPE_NAME (streamheader));
    return FALSE;
  }
wrong_count:
  {
    GST_WARNING ("got %u streamheaders, not 1 or 2 as expected", bufarr->len);
    return FALSE;
  }
wrong_content_type:
  {
    GST_WARNING ("streamheaders array does not contain GstBuffers");
    return FALSE;
  }
}

static gboolean
opus_make_codecdata (GstMatroskaTrackContext * context, GstCaps * caps)
{
  guint32 rate;
  guint8 channels;
  guint8 channel_mapping_family;
  guint8 stream_count, coupled_count, channel_mapping[256];
  GstBuffer *buffer;
  GstMapInfo map;

  /* Opus headers are not in-band */
  context->xiph_headers_to_skip = 0;

  context->codec_delay = 0;
  context->seek_preroll = 80 * GST_MSECOND;

  if (!gst_codec_utils_opus_parse_caps (caps, &rate, &channels,
          &channel_mapping_family, &stream_count, &coupled_count,
          channel_mapping)) {
    GST_WARNING ("Failed to parse caps for Opus");
    return FALSE;
  }

  buffer =
      gst_codec_utils_opus_create_header (rate, channels,
      channel_mapping_family, stream_count, coupled_count, channel_mapping, 0,
      0);
  if (!buffer) {
    GST_WARNING ("Failed to create Opus header from caps");
    return FALSE;
  }

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  context->codec_priv_size = map.size;
  context->codec_priv = g_malloc (context->codec_priv_size);
  memcpy (context->codec_priv, map.data, map.size);
  gst_buffer_unmap (buffer, &map);
  gst_buffer_unref (buffer);

  return TRUE;
}

/**
 * gst_matroska_mux_audio_pad_setcaps:
 * @pad: Pad which got the caps.
 * @caps: New caps.
 *
 * Setcaps function for audio sink pad.
 *
 * Returns: %TRUE on success.
 */
static gboolean
gst_matroska_mux_audio_pad_setcaps (GstPad * pad, GstCaps * caps)
{
  GstMatroskaTrackContext *context = NULL;
  GstMatroskaTrackAudioContext *audiocontext;
  GstMatroskaMux *mux;
  GstMatroskaPad *collect_pad;
  const gchar *mimetype;
  gint samplerate = 0, channels = 0;
  GstStructure *structure;
  const GValue *codec_data = NULL;
  GstBuffer *buf = NULL;
  const gchar *stream_format = NULL;
  GstCaps *old_caps;

  mux = GST_MATROSKA_MUX (GST_PAD_PARENT (pad));

  if ((old_caps = gst_pad_get_current_caps (pad))) {
    if (mux->state >= GST_MATROSKA_MUX_STATE_HEADER
        && !gst_caps_is_equal (caps, old_caps)) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("Caps changes are not supported by Matroska"));
      gst_caps_unref (old_caps);
      goto refuse_caps;
    }
    gst_caps_unref (old_caps);
  }

  /* find context */
  collect_pad = (GstMatroskaPad *) gst_pad_get_element_private (pad);
  g_assert (collect_pad);
  context = collect_pad->track;
  g_assert (context);
  g_assert (context->type == GST_MATROSKA_TRACK_TYPE_AUDIO);
  audiocontext = (GstMatroskaTrackAudioContext *) context;

  structure = gst_caps_get_structure (caps, 0);
  mimetype = gst_structure_get_name (structure);

  /* general setup */
  gst_structure_get_int (structure, "rate", &samplerate);
  gst_structure_get_int (structure, "channels", &channels);

  audiocontext->samplerate = samplerate;
  audiocontext->channels = channels;
  audiocontext->bitdepth = 0;
  context->default_duration = 0;

  codec_data = gst_structure_get_value (structure, "codec_data");
  if (codec_data)
    buf = gst_value_get_buffer (codec_data);

  /* TODO: - check if we handle all codecs by the spec, i.e. codec private
   *         data and other settings
   *       - add new formats
   */

  if (!strcmp (mimetype, "audio/mpeg")) {
    gint mpegversion = 0;

    gst_structure_get_int (structure, "mpegversion", &mpegversion);
    switch (mpegversion) {
      case 1:{
        gint layer;
        gint version = 1;
        gint spf;

        gst_structure_get_int (structure, "layer", &layer);

        if (!gst_structure_get_int (structure, "mpegaudioversion", &version)) {
          GST_WARNING_OBJECT (mux,
              "Unable to determine MPEG audio version, assuming 1");
          version = 1;
        }

        if (layer == 1)
          spf = 384;
        else if (layer == 2)
          spf = 1152;
        else if (version == 2)
          spf = 576;
        else
          spf = 1152;

        context->default_duration =
            gst_util_uint64_scale (GST_SECOND, spf, audiocontext->samplerate);

        switch (layer) {
          case 1:
            gst_matroska_mux_set_codec_id (context,
                GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L1);
            break;
          case 2:
            gst_matroska_mux_set_codec_id (context,
                GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L2);
            break;
          case 3:
            gst_matroska_mux_set_codec_id (context,
                GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L3);
            break;
          default:
            goto refuse_caps;
        }
        break;
      }
      case 2:
      case 4:
        stream_format = gst_structure_get_string (structure, "stream-format");
        /* check this is raw aac */
        if (stream_format) {
          if (strcmp (stream_format, "raw") != 0) {
            GST_WARNING_OBJECT (mux, "AAC stream-format must be 'raw', not %s",
                stream_format);
          }
        } else {
          GST_WARNING_OBJECT (mux, "AAC stream-format not specified, "
              "assuming 'raw'");
        }

        if (buf) {
          gst_matroska_mux_set_codec_id (context,
              GST_MATROSKA_CODEC_ID_AUDIO_AAC);
          context->codec_priv_size = gst_buffer_get_size (buf);
          context->codec_priv = g_malloc (context->codec_priv_size);
          gst_buffer_extract (buf, 0, context->codec_priv,
              context->codec_priv_size);
        } else {
          GST_DEBUG_OBJECT (mux, "no AAC codec_data; not packetized");
          goto refuse_caps;
        }
        break;
      default:
        goto refuse_caps;
    }
  } else if (!strcmp (mimetype, "audio/x-raw")) {
    GstAudioInfo info;

    gst_audio_info_init (&info);
    if (!gst_audio_info_from_caps (&info, caps)) {
      GST_DEBUG_OBJECT (mux,
          "broken caps, rejected by gst_audio_info_from_caps");
      goto refuse_caps;
    }

    switch (GST_AUDIO_INFO_FORMAT (&info)) {
      case GST_AUDIO_FORMAT_U8:
      case GST_AUDIO_FORMAT_S16BE:
      case GST_AUDIO_FORMAT_S16LE:
      case GST_AUDIO_FORMAT_S24BE:
      case GST_AUDIO_FORMAT_S24LE:
      case GST_AUDIO_FORMAT_S32BE:
      case GST_AUDIO_FORMAT_S32LE:
        if (GST_AUDIO_INFO_WIDTH (&info) != GST_AUDIO_INFO_DEPTH (&info)) {
          GST_DEBUG_OBJECT (mux, "width must be same as depth!");
          goto refuse_caps;
        }
        if (GST_AUDIO_INFO_IS_BIG_ENDIAN (&info))
          gst_matroska_mux_set_codec_id (context,
              GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_BE);
        else
          gst_matroska_mux_set_codec_id (context,
              GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_LE);
        break;
      case GST_AUDIO_FORMAT_F32LE:
      case GST_AUDIO_FORMAT_F64LE:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_AUDIO_PCM_FLOAT);
        break;

      default:
        GST_DEBUG_OBJECT (mux, "wrong format in raw audio caps");
        goto refuse_caps;
    }

    audiocontext->bitdepth = GST_AUDIO_INFO_WIDTH (&info);
  } else if (!strcmp (mimetype, "audio/x-vorbis")) {
    const GValue *streamheader;

    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_AUDIO_VORBIS);

    gst_matroska_mux_free_codec_priv (context);

    streamheader = gst_structure_get_value (structure, "streamheader");
    if (!vorbis_streamheader_to_codecdata (streamheader, context)) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("vorbis stream headers missing or malformed"));
      goto refuse_caps;
    }
  } else if (!strcmp (mimetype, "audio/x-flac")) {
    const GValue *streamheader;

    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_AUDIO_FLAC);

    gst_matroska_mux_free_codec_priv (context);

    streamheader = gst_structure_get_value (structure, "streamheader");
    if (!flac_streamheader_to_codecdata (streamheader, context)) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("flac stream headers missing or malformed"));
      goto refuse_caps;
    }
  } else if (!strcmp (mimetype, "audio/x-speex")) {
    const GValue *streamheader;

    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_AUDIO_SPEEX);
    gst_matroska_mux_free_codec_priv (context);

    streamheader = gst_structure_get_value (structure, "streamheader");
    if (!speex_streamheader_to_codecdata (streamheader, context)) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("speex stream headers missing or malformed"));
      goto refuse_caps;
    }
  } else if (!strcmp (mimetype, "audio/x-opus")) {
    const GValue *streamheader;

    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_AUDIO_OPUS);

    streamheader = gst_structure_get_value (structure, "streamheader");
    if (streamheader) {
      gst_matroska_mux_free_codec_priv (context);
      if (!opus_streamheader_to_codecdata (streamheader, context)) {
        GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
            ("opus stream headers missing or malformed"));
        goto refuse_caps;
      }
    } else {
      /* no streamheader, but we need to have one, so we make one up
         based on caps */
      gst_matroska_mux_free_codec_priv (context);
      if (!opus_make_codecdata (context, caps)) {
        GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
            ("opus stream headers missing or malformed"));
        goto refuse_caps;
      }
    }
  } else if (!strcmp (mimetype, "audio/x-ac3")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_AUDIO_AC3);
  } else if (!strcmp (mimetype, "audio/x-eac3")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_AUDIO_EAC3);
  } else if (!strcmp (mimetype, "audio/x-dts")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_AUDIO_DTS);
  } else if (!strcmp (mimetype, "audio/x-tta")) {
    gint width;

    /* TTA frame duration */
    context->default_duration = 1.04489795918367346939 * GST_SECOND;

    gst_structure_get_int (structure, "width", &width);
    audiocontext->bitdepth = width;
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_AUDIO_TTA);

  } else if (!strcmp (mimetype, "audio/x-pn-realaudio")) {
    gint raversion;
    const GValue *mdpr_data;

    gst_structure_get_int (structure, "raversion", &raversion);
    switch (raversion) {
      case 1:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_AUDIO_REAL_14_4);
        break;
      case 2:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_AUDIO_REAL_28_8);
        break;
      case 8:
        gst_matroska_mux_set_codec_id (context,
            GST_MATROSKA_CODEC_ID_AUDIO_REAL_COOK);
        break;
      default:
        goto refuse_caps;
    }

    mdpr_data = gst_structure_get_value (structure, "mdpr_data");
    if (mdpr_data != NULL) {
      guint8 *priv_data = NULL;
      guint priv_data_size = 0;

      GstBuffer *codec_data_buf = g_value_peek_pointer (mdpr_data);

      priv_data_size = gst_buffer_get_size (codec_data_buf);
      priv_data = g_malloc0 (priv_data_size);

      gst_buffer_extract (codec_data_buf, 0, priv_data, -1);

      gst_matroska_mux_free_codec_priv (context);

      context->codec_priv = priv_data;
      context->codec_priv_size = priv_data_size;
    }

  } else if (!strcmp (mimetype, "audio/x-wma")
      || !strcmp (mimetype, "audio/x-alaw")
      || !strcmp (mimetype, "audio/x-mulaw")
      || !strcmp (mimetype, "audio/x-adpcm")
      || !strcmp (mimetype, "audio/G722")) {
    guint8 *codec_priv;
    guint codec_priv_size;
    guint16 format = 0;
    gint block_align = 0;
    gint bitrate = 0;

    if (samplerate == 0 || channels == 0) {
      GST_WARNING_OBJECT (mux, "Missing channels/samplerate on caps");
      goto refuse_caps;
    }

    if (!strcmp (mimetype, "audio/x-wma")) {
      gint wmaversion;
      gint depth;

      if (!gst_structure_get_int (structure, "wmaversion", &wmaversion)
          || !gst_structure_get_int (structure, "block_align", &block_align)
          || !gst_structure_get_int (structure, "bitrate", &bitrate)) {
        GST_WARNING_OBJECT (mux, "Missing wmaversion/block_align/bitrate"
            " on WMA caps");
        goto refuse_caps;
      }

      switch (wmaversion) {
        case 1:
          format = GST_RIFF_WAVE_FORMAT_WMAV1;
          break;
        case 2:
          format = GST_RIFF_WAVE_FORMAT_WMAV2;
          break;
        case 3:
          format = GST_RIFF_WAVE_FORMAT_WMAV3;
          break;
        default:
          GST_WARNING_OBJECT (mux, "Unexpected WMA version: %d", wmaversion);
          goto refuse_caps;
      }

      if (gst_structure_get_int (structure, "depth", &depth))
        audiocontext->bitdepth = depth;
    } else if (!strcmp (mimetype, "audio/x-alaw")
        || !strcmp (mimetype, "audio/x-mulaw")) {
      audiocontext->bitdepth = 8;
      if (!strcmp (mimetype, "audio/x-alaw"))
        format = GST_RIFF_WAVE_FORMAT_ALAW;
      else
        format = GST_RIFF_WAVE_FORMAT_MULAW;

      block_align = channels;
      bitrate = block_align * samplerate;
    } else if (!strcmp (mimetype, "audio/x-adpcm")) {
      const char *layout;

      layout = gst_structure_get_string (structure, "layout");
      if (!layout) {
        GST_WARNING_OBJECT (mux, "Missing layout on adpcm caps");
        goto refuse_caps;
      }

      if (!gst_structure_get_int (structure, "block_align", &block_align)) {
        GST_WARNING_OBJECT (mux, "Missing block_align on adpcm caps");
        goto refuse_caps;
      }

      if (!strcmp (layout, "dvi")) {
        format = GST_RIFF_WAVE_FORMAT_DVI_ADPCM;
      } else if (!strcmp (layout, "g726")) {
        format = GST_RIFF_WAVE_FORMAT_ITU_G726_ADPCM;
        if (!gst_structure_get_int (structure, "bitrate", &bitrate)) {
          GST_WARNING_OBJECT (mux, "Missing bitrate on adpcm g726 caps");
          goto refuse_caps;
        }
      } else {
        GST_WARNING_OBJECT (mux, "Unknown layout on adpcm caps");
        goto refuse_caps;
      }

    } else if (!strcmp (mimetype, "audio/G722")) {
      format = GST_RIFF_WAVE_FORMAT_ADPCM_G722;
    }
    g_assert (format != 0);

    codec_priv_size = WAVEFORMATEX_SIZE;
    if (buf)
      codec_priv_size += gst_buffer_get_size (buf);

    /* serialize waveformatex structure */
    codec_priv = g_malloc0 (codec_priv_size);
    GST_WRITE_UINT16_LE (codec_priv, format);
    GST_WRITE_UINT16_LE (codec_priv + 2, channels);
    GST_WRITE_UINT32_LE (codec_priv + 4, samplerate);
    GST_WRITE_UINT32_LE (codec_priv + 8, bitrate / 8);
    GST_WRITE_UINT16_LE (codec_priv + 12, block_align);
    GST_WRITE_UINT16_LE (codec_priv + 14, 0);
    if (buf)
      GST_WRITE_UINT16_LE (codec_priv + 16, gst_buffer_get_size (buf));
    else
      GST_WRITE_UINT16_LE (codec_priv + 16, 0);

    /* process codec private/initialization data, if any */
    if (buf) {
      gst_buffer_extract (buf, 0,
          (guint8 *) codec_priv + WAVEFORMATEX_SIZE, -1);
    }

    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_AUDIO_ACM);
    gst_matroska_mux_free_codec_priv (context);
    context->codec_priv = (gpointer) codec_priv;
    context->codec_priv_size = codec_priv_size;
  }

  return TRUE;

  /* ERRORS */
refuse_caps:
  {
    GST_WARNING_OBJECT (mux, "pad %s refused caps %" GST_PTR_FORMAT,
        GST_PAD_NAME (pad), caps);
    return FALSE;
  }
}

/* we probably don't have the data at start,
 * so have to reserve (a maximum) space to write this at the end.
 * bit spacy, but some formats can hold quite some */
#define SUBTITLE_MAX_CODEC_PRIVATE   2048       /* must be > 128 */

/**
 * gst_matroska_mux_subtitle_pad_setcaps:
 * @pad: Pad which got the caps.
 * @caps: New caps.
 *
 * Setcaps function for subtitle sink pad.
 *
 * Returns: %TRUE on success.
 */
static gboolean
gst_matroska_mux_subtitle_pad_setcaps (GstPad * pad, GstCaps * caps)
{
  /* There is now (at least) one such alement (kateenc), and I'm going
     to handle it here and claim it works when it can be piped back
     through GStreamer and VLC */

  GstMatroskaTrackContext *context = NULL;
  GstMatroskaTrackSubtitleContext *scontext;
  GstMatroskaMux *mux;
  GstMatroskaPad *collect_pad;
  const gchar *mimetype;
  GstStructure *structure;
  const GValue *value = NULL;
  GstBuffer *buf = NULL;
  gboolean ret = TRUE;
  GstCaps *old_caps;

  mux = GST_MATROSKA_MUX (GST_PAD_PARENT (pad));

  if ((old_caps = gst_pad_get_current_caps (pad))) {
    if (mux->state >= GST_MATROSKA_MUX_STATE_HEADER
        && !gst_caps_is_equal (caps, old_caps)) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("Caps changes are not supported by Matroska"));
      gst_caps_unref (old_caps);
      goto refuse_caps;
    }
    gst_caps_unref (old_caps);
  }

  /* find context */
  collect_pad = (GstMatroskaPad *) gst_pad_get_element_private (pad);
  g_assert (collect_pad);
  context = collect_pad->track;
  g_assert (context);
  g_assert (context->type == GST_MATROSKA_TRACK_TYPE_SUBTITLE);
  scontext = (GstMatroskaTrackSubtitleContext *) context;

  structure = gst_caps_get_structure (caps, 0);
  mimetype = gst_structure_get_name (structure);

  /* general setup */
  scontext->check_utf8 = 1;
  scontext->invalid_utf8 = 0;
  context->default_duration = 0;

  if (!strcmp (mimetype, "subtitle/x-kate")) {
    const GValue *streamheader;

    gst_matroska_mux_set_codec_id (context,
        GST_MATROSKA_CODEC_ID_SUBTITLE_KATE);

    gst_matroska_mux_free_codec_priv (context);

    streamheader = gst_structure_get_value (structure, "streamheader");
    if (!kate_streamheader_to_codecdata (streamheader, context)) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("kate stream headers missing or malformed"));
      ret = FALSE;
      goto exit;
    }
  } else if (!strcmp (mimetype, "text/x-raw")) {
    gst_matroska_mux_set_codec_id (context,
        GST_MATROSKA_CODEC_ID_SUBTITLE_UTF8);
  } else if (!strcmp (mimetype, "application/x-ssa")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_SUBTITLE_SSA);
  } else if (!strcmp (mimetype, "application/x-ass")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_SUBTITLE_ASS);
  } else if (!strcmp (mimetype, "application/x-usf")) {
    gst_matroska_mux_set_codec_id (context, GST_MATROSKA_CODEC_ID_SUBTITLE_USF);
  } else if (!strcmp (mimetype, "subpicture/x-dvd")) {
    gst_matroska_mux_set_codec_id (context,
        GST_MATROSKA_CODEC_ID_SUBTITLE_VOBSUB);
  } else {
    ret = FALSE;
    goto exit;
  }

  /* maybe some private data, e.g. vobsub */
  value = gst_structure_get_value (structure, "codec_data");
  if (value)
    buf = gst_value_get_buffer (value);
  if (buf != NULL) {
    GstMapInfo map;
    guint8 *priv_data = NULL;

    gst_buffer_map (buf, &map, GST_MAP_READ);

    if (map.size > SUBTITLE_MAX_CODEC_PRIVATE) {
      GST_WARNING_OBJECT (mux, "pad %" GST_PTR_FORMAT " subtitle private data"
          " exceeded maximum (%d); discarding", pad,
          SUBTITLE_MAX_CODEC_PRIVATE);
      gst_buffer_unmap (buf, &map);
      return TRUE;
    }

    gst_matroska_mux_free_codec_priv (context);

    priv_data = g_malloc0 (map.size);
    memcpy (priv_data, map.data, map.size);
    context->codec_priv = priv_data;
    context->codec_priv_size = map.size;
    gst_buffer_unmap (buf, &map);
  }

  GST_DEBUG_OBJECT (pad, "codec_id %s, codec data size %" G_GSIZE_FORMAT,
      GST_STR_NULL (context->codec_id), context->codec_priv_size);

exit:

  return ret;

  /* ERRORS */
refuse_caps:
  {
    GST_WARNING_OBJECT (mux, "pad %s refused caps %" GST_PTR_FORMAT,
        GST_PAD_NAME (pad), caps);
    return FALSE;
  }
}


/**
 * gst_matroska_mux_request_new_pad:
 * @element: #GstMatroskaMux.
 * @templ: #GstPadTemplate.
 * @pad_name: New pad name.
 *
 * Request pad function for sink templates.
 *
 * Returns: New #GstPad.
 */
static GstPad *
gst_matroska_mux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (element);
  GstMatroskaMux *mux = GST_MATROSKA_MUX (element);
  GstMatroskaPad *collect_pad;
  GstMatroskamuxPad *newpad;
  gchar *name = NULL;
  const gchar *pad_name = NULL;
  GstMatroskaCapsFunc capsfunc = NULL;
  GstMatroskaTrackContext *context = NULL;
  gint pad_id;
  gboolean locked = TRUE;
  const gchar *id = NULL;

  if (templ == gst_element_class_get_pad_template (klass, "audio_%u")) {
    /* don't mix named and unnamed pads, if the pad already exists we fail when
     * trying to add it */
    if (req_name != NULL && sscanf (req_name, "audio_%u", &pad_id) == 1) {
      pad_name = req_name;
    } else {
      name = g_strdup_printf ("audio_%u", mux->num_a_streams++);
      pad_name = name;
    }
    capsfunc = GST_DEBUG_FUNCPTR (gst_matroska_mux_audio_pad_setcaps);
    context = (GstMatroskaTrackContext *)
        g_new0 (GstMatroskaTrackAudioContext, 1);
    context->type = GST_MATROSKA_TRACK_TYPE_AUDIO;
    context->name = g_strdup ("Audio");
  } else if (templ == gst_element_class_get_pad_template (klass, "video_%u")) {
    /* don't mix named and unnamed pads, if the pad already exists we fail when
     * trying to add it */
    if (req_name != NULL && sscanf (req_name, "video_%u", &pad_id) == 1) {
      pad_name = req_name;
    } else {
      name = g_strdup_printf ("video_%u", mux->num_v_streams++);
      pad_name = name;
    }
    capsfunc = GST_DEBUG_FUNCPTR (gst_matroska_mux_video_pad_setcaps);
    context = (GstMatroskaTrackContext *)
        g_new0 (GstMatroskaTrackVideoContext, 1);
    context->type = GST_MATROSKA_TRACK_TYPE_VIDEO;
    context->name = g_strdup ("Video");
  } else if (templ == gst_element_class_get_pad_template (klass, "subtitle_%u")) {
    /* don't mix named and unnamed pads, if the pad already exists we fail when
     * trying to add it */
    if (req_name != NULL && sscanf (req_name, "subtitle_%u", &pad_id) == 1) {
      pad_name = req_name;
    } else {
      name = g_strdup_printf ("subtitle_%u", mux->num_t_streams++);
      pad_name = name;
    }
    capsfunc = GST_DEBUG_FUNCPTR (gst_matroska_mux_subtitle_pad_setcaps);
    context = (GstMatroskaTrackContext *)
        g_new0 (GstMatroskaTrackSubtitleContext, 1);
    context->type = GST_MATROSKA_TRACK_TYPE_SUBTITLE;
    context->name = g_strdup ("Subtitle");
    /* setcaps may only provide proper one a lot later */
    id = "S_SUB_UNKNOWN";
    locked = FALSE;
  } else {
    GST_WARNING_OBJECT (mux, "This is not our template!");
    return NULL;
  }

  newpad = g_object_new (GST_TYPE_MATROSKAMUX_PAD,
      "name", pad_name, "direction", templ->direction, "template", templ, NULL);

  gst_matroskamux_pad_init (newpad);
  collect_pad = (GstMatroskaPad *)
      gst_collect_pads_add_pad (mux->collect, GST_PAD (newpad),
      sizeof (GstMatroskaPad),
      (GstCollectDataDestroyNotify) gst_matroska_pad_free, locked);

  collect_pad->mux = mux;
  collect_pad->track = context;
  gst_matroska_pad_reset (collect_pad, FALSE);
  if (id)
    gst_matroska_mux_set_codec_id (collect_pad->track, id);
  collect_pad->track->dts_only = FALSE;

  collect_pad->capsfunc = capsfunc;
  gst_pad_set_active (GST_PAD (newpad), TRUE);
  if (!gst_element_add_pad (element, GST_PAD (newpad)))
    goto pad_add_failed;

  g_free (name);

  mux->num_streams++;

  GST_DEBUG_OBJECT (newpad, "Added new request pad");

  return GST_PAD (newpad);

  /* ERROR cases */
pad_add_failed:
  {
    GST_WARNING_OBJECT (mux, "Adding the new pad '%s' failed", pad_name);
    g_free (name);
    gst_object_unref (newpad);
    return NULL;
  }
}

/**
 * gst_matroska_mux_release_pad:
 * @element: #GstMatroskaMux.
 * @pad: Pad to release.
 *
 * Release a previously requested pad.
*/
static void
gst_matroska_mux_release_pad (GstElement * element, GstPad * pad)
{
  GstMatroskaMux *mux;
  GSList *walk;

  mux = GST_MATROSKA_MUX (GST_PAD_PARENT (pad));

  for (walk = mux->collect->data; walk; walk = g_slist_next (walk)) {
    GstCollectData *cdata = (GstCollectData *) walk->data;
    GstMatroskaPad *collect_pad = (GstMatroskaPad *) cdata;

    if (cdata->pad == pad) {
      /*
       * observed duration, this will remain GST_CLOCK_TIME_NONE
       * only if the pad is reset
       */
      GstClockTime collected_duration = GST_CLOCK_TIME_NONE;

      if (GST_CLOCK_TIME_IS_VALID (collect_pad->start_ts) &&
          GST_CLOCK_TIME_IS_VALID (collect_pad->end_ts)) {
        collected_duration =
            GST_CLOCK_DIFF (collect_pad->start_ts, collect_pad->end_ts);
      }

      if (GST_CLOCK_TIME_IS_VALID (collected_duration)
          && mux->duration < collected_duration)
        mux->duration = collected_duration;

      break;
    }
  }

  gst_collect_pads_remove_pad (mux->collect, pad);
  if (gst_element_remove_pad (element, pad))
    mux->num_streams--;
}

static void
gst_matroska_mux_write_mastering_metadata (GstMatroskaMux * mux,
    GstMatroskaTrackVideoContext * videocontext)
{
  GstEbmlWrite *ebml = mux->ebml_write;
  guint64 master;
  GstVideoMasteringDisplayInfo *minfo = &videocontext->mastering_display_info;
  gdouble value;

  if (!videocontext->mastering_display_info_present)
    return;

  master =
      gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_MASTERINGMETADATA);

  gst_util_fraction_to_double (minfo->Rx_n, minfo->Rx_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_PRIMARYRCHROMATICITYX, value);

  gst_util_fraction_to_double (minfo->Ry_n, minfo->Ry_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_PRIMARYRCHROMATICITYY, value);

  gst_util_fraction_to_double (minfo->Gx_n, minfo->Gx_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_PRIMARYGCHROMATICITYX, value);

  gst_util_fraction_to_double (minfo->Gy_n, minfo->Gy_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_PRIMARYGCHROMATICITYY, value);

  gst_util_fraction_to_double (minfo->Bx_n, minfo->Bx_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_PRIMARYBCHROMATICITYX, value);

  gst_util_fraction_to_double (minfo->By_n, minfo->By_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_PRIMARYBCHROMATICITYY, value);

  gst_util_fraction_to_double (minfo->Wx_n, minfo->Wx_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_WHITEPOINTCHROMATICITYX, value);

  gst_util_fraction_to_double (minfo->Wy_n, minfo->Wy_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_WHITEPOINTCHROMATICITYY, value);

  gst_util_fraction_to_double (minfo->max_luma_n, minfo->max_luma_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_LUMINANCEMAX, value);

  gst_util_fraction_to_double (minfo->min_luma_n, minfo->min_luma_d, &value);
  gst_ebml_write_float (ebml, GST_MATROSKA_ID_LUMINANCEMIN, value);

  gst_ebml_write_master_finish (ebml, master);
  return;
}

static void
gst_matroska_mux_write_colour (GstMatroskaMux * mux,
    GstMatroskaTrackVideoContext * videocontext)
{
  GstEbmlWrite *ebml = mux->ebml_write;
  guint64 master;
  guint matrix_id = 0;
  guint range_id = 0;
  guint transfer_id = 0;
  guint primaries_id = 0;

  master = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_VIDEOCOLOUR);

  switch (videocontext->colorimetry.range) {
    case GST_VIDEO_COLOR_RANGE_UNKNOWN:
      range_id = 0;
      break;
    case GST_VIDEO_COLOR_RANGE_16_235:
      range_id = 1;
      break;
    case GST_VIDEO_COLOR_RANGE_0_255:
      range_id = 2;
  }

  matrix_id = gst_video_color_matrix_to_iso (videocontext->colorimetry.matrix);
  transfer_id =
      gst_video_color_transfer_to_iso (videocontext->colorimetry.transfer);
  primaries_id =
      gst_video_color_primaries_to_iso (videocontext->colorimetry.primaries);

  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEORANGE, range_id);
  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEOMATRIXCOEFFICIENTS,
      matrix_id);
  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEOTRANSFERCHARACTERISTICS,
      transfer_id);
  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEOPRIMARIES, primaries_id);
  if (videocontext->content_light_level.maxCLL_n &&
      videocontext->content_light_level.maxFALL_n) {
    gdouble maxCLL = 0, maxFALL = 0;

    gst_util_fraction_to_double (videocontext->content_light_level.maxCLL_n,
        videocontext->content_light_level.maxCLL_d, &maxCLL);
    gst_util_fraction_to_double (videocontext->content_light_level.maxFALL_n,
        videocontext->content_light_level.maxFALL_d, &maxFALL);
    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_MAXCLL, (guint) maxCLL);
    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_MAXFALL, (guint) maxFALL);
  }

  gst_matroska_mux_write_mastering_metadata (mux, videocontext);
  gst_ebml_write_master_finish (ebml, master);
}

/**
 * gst_matroska_mux_track_header:
 * @mux: #GstMatroskaMux
 * @context: Tack context.
 *
 * Write a track header.
 */
static void
gst_matroska_mux_track_header (GstMatroskaMux * mux,
    GstMatroskaTrackContext * context)
{
  GstEbmlWrite *ebml = mux->ebml_write;
  guint64 master;

  /* TODO: check if everything necessary is written and check default values */

  /* track type goes before the type-specific stuff */
  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_TRACKNUMBER, context->num);
  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_TRACKTYPE, context->type);

  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_TRACKUID, context->uid);
  if (context->default_duration) {
    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_TRACKDEFAULTDURATION,
        context->default_duration);
  }
  if (context->language) {
    gst_ebml_write_utf8 (ebml, GST_MATROSKA_ID_TRACKLANGUAGE,
        context->language);
  }

  /* FIXME: until we have a nice way of getting the codecname
   * out of the caps, I'm not going to enable this. Too much
   * (useless, double, boring) work... */
  /* TODO: Use value from tags if any */
  /*gst_ebml_write_utf8 (ebml, GST_MATROSKA_ID_CODECNAME,
     context->codec_name); */
  gst_ebml_write_utf8 (ebml, GST_MATROSKA_ID_TRACKNAME, context->name);

  /* type-specific stuff */
  switch (context->type) {
    case GST_MATROSKA_TRACK_TYPE_VIDEO:{
      GstMatroskaTrackVideoContext *videocontext =
          (GstMatroskaTrackVideoContext *) context;

      master = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TRACKVIDEO);
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEOPIXELWIDTH,
          videocontext->pixel_width);
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEOPIXELHEIGHT,
          videocontext->pixel_height);
      if (videocontext->display_width && videocontext->display_height) {
        gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEODISPLAYWIDTH,
            videocontext->display_width);
        gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEODISPLAYHEIGHT,
            videocontext->display_height);
      }
      switch (videocontext->interlace_mode) {
        case GST_MATROSKA_INTERLACE_MODE_INTERLACED:
          gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEOFLAGINTERLACED, 1);
          break;
        case GST_MATROSKA_INTERLACE_MODE_PROGRESSIVE:
          gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEOFLAGINTERLACED, 2);
          break;
        default:
          break;
      }

      if (videocontext->fourcc) {
        guint32 fcc_le = GUINT32_TO_LE (videocontext->fourcc);

        gst_ebml_write_binary (ebml, GST_MATROSKA_ID_VIDEOCOLOURSPACE,
            (gpointer) & fcc_le, 4);
      }
      gst_matroska_mux_write_colour (mux, videocontext);
      if (videocontext->multiview_mode != GST_VIDEO_MULTIVIEW_MODE_NONE) {
        guint64 stereo_mode = 0;

        switch (videocontext->multiview_mode) {
          case GST_VIDEO_MULTIVIEW_MODE_MONO:
            break;
          case GST_VIDEO_MULTIVIEW_MODE_SIDE_BY_SIDE:
            if (videocontext->multiview_flags &
                GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST)
              stereo_mode = GST_MATROSKA_STEREO_MODE_SBS_RL;
            else
              stereo_mode = GST_MATROSKA_STEREO_MODE_SBS_LR;
            break;
          case GST_VIDEO_MULTIVIEW_MODE_TOP_BOTTOM:
            if (videocontext->multiview_flags &
                GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST)
              stereo_mode = GST_MATROSKA_STEREO_MODE_TB_RL;
            else
              stereo_mode = GST_MATROSKA_STEREO_MODE_TB_LR;
            break;
          case GST_VIDEO_MULTIVIEW_MODE_CHECKERBOARD:
            if (videocontext->multiview_flags &
                GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST)
              stereo_mode = GST_MATROSKA_STEREO_MODE_CHECKER_RL;
            else
              stereo_mode = GST_MATROSKA_STEREO_MODE_CHECKER_LR;
            break;
          case GST_VIDEO_MULTIVIEW_MODE_FRAME_BY_FRAME:
            if (videocontext->multiview_flags &
                GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST)
              stereo_mode = GST_MATROSKA_STEREO_MODE_FBF_RL;
            else
              stereo_mode = GST_MATROSKA_STEREO_MODE_FBF_LR;
            /* FIXME: In frame-by-frame mode, left/right frame buffers need to be
             * laced within one block. See http://www.matroska.org/technical/specs/index.html#StereoMode */
            GST_FIXME_OBJECT (mux,
                "Frame-by-frame stereoscopic mode not fully implemented");
            break;
          default:
            GST_WARNING_OBJECT (mux,
                "Multiview mode %d not supported in Matroska/WebM",
                videocontext->multiview_mode);
            break;
        }

        if (stereo_mode != 0)
          gst_ebml_write_uint (ebml, GST_MATROSKA_ID_VIDEOSTEREOMODE,
              stereo_mode);
      }
      gst_ebml_write_master_finish (ebml, master);

      break;
    }

    case GST_MATROSKA_TRACK_TYPE_AUDIO:{
      GstMatroskaTrackAudioContext *audiocontext =
          (GstMatroskaTrackAudioContext *) context;

      master = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TRACKAUDIO);
      if (audiocontext->samplerate != 8000)
        gst_ebml_write_float (ebml, GST_MATROSKA_ID_AUDIOSAMPLINGFREQ,
            audiocontext->samplerate);
      if (audiocontext->channels != 1)
        gst_ebml_write_uint (ebml, GST_MATROSKA_ID_AUDIOCHANNELS,
            audiocontext->channels);
      if (audiocontext->bitdepth) {
        gst_ebml_write_uint (ebml, GST_MATROSKA_ID_AUDIOBITDEPTH,
            audiocontext->bitdepth);
      }

      gst_ebml_write_master_finish (ebml, master);

      break;
    }

    case GST_MATROSKA_TRACK_TYPE_SUBTITLE:{
      break;
    }
    default:
      /* doesn't need type-specific data */
      break;
  }

  gst_ebml_write_ascii (ebml, GST_MATROSKA_ID_CODECID, context->codec_id);
  if (context->codec_priv)
    gst_ebml_write_binary (ebml, GST_MATROSKA_ID_CODECPRIVATE,
        context->codec_priv, context->codec_priv_size);

  if (context->seek_preroll) {
    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_SEEKPREROLL,
        context->seek_preroll);
  }

  if (context->codec_delay) {
    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CODECDELAY,
        context->codec_delay);
  }
}

static void
gst_matroska_mux_write_chapter_title (const gchar * title, GstEbmlWrite * ebml)
{
  guint64 title_master;

  title_master =
      gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_CHAPTERDISPLAY);

  gst_ebml_write_utf8 (ebml, GST_MATROSKA_ID_CHAPSTRING, title);
  gst_ebml_write_ascii (ebml, GST_MATROSKA_ID_CHAPLANGUAGE,
      GST_MATROSKA_MUX_CHAPLANG);

  gst_ebml_write_master_finish (ebml, title_master);
}

static GstTocEntry *
gst_matroska_mux_write_chapter (GstMatroskaMux * mux, GstTocEntry * edition,
    GstTocEntry * entry, GstEbmlWrite * ebml, guint64 * master_chapters,
    guint64 * master_edition)
{
  guint64 master_chapteratom;
  GList *cur;
  guint count, i;
  gchar *title;
  gint64 start, stop;
  guint64 uid;
  gchar s_uid[32];
  GstTocEntry *internal_chapter, *internal_nested;
  GstTagList *tags;

  if (G_UNLIKELY (master_chapters != NULL && *master_chapters == 0))
    *master_chapters =
        gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_CHAPTERS);

  if (G_UNLIKELY (master_edition != NULL && *master_edition == 0)) {
    /* create uid for the parent */
    *master_edition =
        gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_EDITIONENTRY);

    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_EDITIONUID,
        g_ascii_strtoull (gst_toc_entry_get_uid (edition), NULL, 10));
    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_EDITIONFLAGHIDDEN, 0);
    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_EDITIONFLAGDEFAULT, 0);
    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_EDITIONFLAGORDERED, 0);
  }

  gst_toc_entry_get_start_stop_times (entry, &start, &stop);
  tags = gst_toc_entry_get_tags (entry);
  if (tags != NULL) {
    tags = gst_tag_list_copy (tags);
  }

  /* build internal chapter */
  uid = gst_matroska_mux_create_uid (mux);
  g_snprintf (s_uid, sizeof (s_uid), "%" G_GINT64_FORMAT, uid);
  internal_chapter = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_CHAPTER, s_uid);

  /* Write the chapter entry */
  master_chapteratom =
      gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_CHAPTERATOM);

  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CHAPTERUID, uid);
  /* Store the user provided UID in the ChapterStringUID */
  gst_ebml_write_utf8 (ebml, GST_MATROSKA_ID_CHAPTERSTRINGUID,
      gst_toc_entry_get_uid (entry));
  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CHAPTERTIMESTART, start);
  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CHAPTERTIMESTOP, stop);
  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CHAPTERFLAGHIDDEN, 0);
  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CHAPTERFLAGENABLED, 1);

  /* write current ChapterDisplays before the nested chapters */
  if (G_LIKELY (tags != NULL)) {
    count = gst_tag_list_get_tag_size (tags, GST_TAG_TITLE);

    for (i = 0; i < count; ++i) {
      gst_tag_list_get_string_index (tags, GST_TAG_TITLE, i, &title);
      /* FIXME: handle ChapterLanguage entries */
      gst_matroska_mux_write_chapter_title (title, ebml);
      g_free (title);
    }

    /* remove title tag */
    if (G_LIKELY (count > 0))
      gst_tag_list_remove_tag (tags, GST_TAG_TITLE);

    gst_toc_entry_set_tags (internal_chapter, tags);
  }

  /* Write nested chapters */
  for (cur = gst_toc_entry_get_sub_entries (entry); cur != NULL;
      cur = cur->next) {
    internal_nested = gst_matroska_mux_write_chapter (mux, NULL, cur->data,
        ebml, NULL, NULL);

    gst_toc_entry_append_sub_entry (internal_chapter, internal_nested);
  }

  gst_ebml_write_master_finish (ebml, master_chapteratom);

  return internal_chapter;
}

static GstTocEntry *
gst_matroska_mux_write_chapter_edition (GstMatroskaMux * mux,
    GstTocEntry * edition, GList * chapters, GstEbmlWrite * ebml,
    guint64 * master_chapters)
{
  guint64 master_edition = 0;
  gchar s_uid[32];
  GList *cur;
  GstTocEntry *internal_edition, *internal_chapter;
  GstTagList *tags = NULL;

  g_snprintf (s_uid, sizeof (s_uid), "%" G_GINT64_FORMAT,
      gst_matroska_mux_create_uid (mux));

  if (edition != NULL) {
    /* Edition entry defined, get its tags */
    tags = gst_toc_entry_get_tags (edition);
    if (tags != NULL) {
      tags = gst_tag_list_copy (tags);
    }
  }

  internal_edition = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_EDITION, s_uid);
  if (tags != NULL) {
    gst_toc_entry_set_tags (internal_edition, tags);
  }

  for (cur = g_list_first (chapters); cur != NULL; cur = cur->next) {
    internal_chapter = gst_matroska_mux_write_chapter (mux, internal_edition,
        cur->data, ebml, master_chapters, &master_edition);

    gst_toc_entry_append_sub_entry (internal_edition, internal_chapter);
  }

  if (G_LIKELY (master_edition != 0))
    gst_ebml_write_master_finish (ebml, master_edition);

  return internal_edition;
}

/**
 * gst_matroska_mux_start:
 * @mux: #GstMatroskaMux
 *
 * Start a new matroska file (write headers etc...)
 */
static void
gst_matroska_mux_start (GstMatroskaMux * mux, GstMatroskaPad * first_pad,
    GstBuffer * first_pad_buf)
{
  GstEbmlWrite *ebml = mux->ebml_write;
  const gchar *doctype;
  guint32 seekhead_id[] = { GST_MATROSKA_ID_SEGMENTINFO,
    GST_MATROSKA_ID_TRACKS,
    GST_MATROSKA_ID_CHAPTERS,
    GST_MATROSKA_ID_CUES,
    GST_MATROSKA_ID_TAGS,
    0
  };
  const gchar *media_type;
  gboolean audio_only;
  guint64 master, child;
  GSList *collected;
  int i;
  guint tracknum = 1;
  GstClockTime earliest_time = GST_CLOCK_TIME_NONE;
  GstClockTime duration = 0;
  guint32 segment_uid[4];
  gint64 time;
  gchar s_id[32];
  GstToc *toc;

  /* if not streaming, check if downstream is seekable */
  if (!mux->ebml_write->streamable) {
    gboolean seekable;
    GstQuery *query;

    query = gst_query_new_seeking (GST_FORMAT_BYTES);
    if (gst_pad_peer_query (mux->srcpad, query)) {
      gst_query_parse_seeking (query, NULL, &seekable, NULL, NULL);
      GST_INFO_OBJECT (mux, "downstream is %sseekable", seekable ? "" : "not ");
    } else {
      /* assume seeking is not supported if query not handled downstream */
      GST_WARNING_OBJECT (mux, "downstream did not handle seeking query");
      seekable = FALSE;
    }
    if (!seekable) {
      mux->ebml_write->streamable = TRUE;
      g_object_notify (G_OBJECT (mux), "streamable");
      GST_WARNING_OBJECT (mux, "downstream is not seekable, but "
          "streamable=false. Will ignore that and create streamable output "
          "instead");
    }
    gst_query_unref (query);
  }

  /* stream-start (FIXME: create id based on input ids) */
  g_snprintf (s_id, sizeof (s_id), "matroskamux-%08x", g_random_int ());
  gst_pad_push_event (mux->srcpad, gst_event_new_stream_start (s_id));

  /* output caps */
  audio_only = mux->num_v_streams == 0 && mux->num_a_streams > 0;
  if (mux->is_webm) {
    media_type = (audio_only) ? "audio/webm" : "video/webm";
  } else {
    media_type = (audio_only) ? "audio/x-matroska" : "video/x-matroska";
  }
  ebml->caps = gst_caps_new_empty_simple (media_type);
  gst_pad_set_caps (mux->srcpad, ebml->caps);
  /* we start with a EBML header */
  doctype = mux->doctype;
  GST_INFO_OBJECT (ebml, "DocType: %s, Version: %d",
      doctype, mux->doctype_version);
  gst_ebml_write_header (ebml, doctype, mux->doctype_version);

  /* the rest of the header is cached */
  gst_ebml_write_set_cache (ebml, 0x1000);

  /* start a segment */
  mux->segment_pos =
      gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_SEGMENT);
  mux->segment_master = ebml->pos;

  if (!mux->ebml_write->streamable) {
    /* seekhead (table of contents) - we set the positions later */
    mux->seekhead_pos = ebml->pos;
    master = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_SEEKHEAD);
    for (i = 0; seekhead_id[i] != 0; i++) {
      child = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_SEEKENTRY);
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_SEEKID, seekhead_id[i]);
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_SEEKPOSITION, -1);
      gst_ebml_write_master_finish (ebml, child);
    }
    gst_ebml_write_master_finish (ebml, master);
  }

  if (mux->ebml_write->streamable) {
    const GstTagList *tags;
    gboolean has_main_tags;

    /* tags */
    tags = gst_tag_setter_get_tag_list (GST_TAG_SETTER (mux));
    has_main_tags = tags != NULL && !gst_matroska_mux_tag_list_is_empty (tags);

    if (has_main_tags || gst_matroska_mux_streams_have_tags (mux)) {
      guint64 master_tags, master_tag;

      GST_DEBUG_OBJECT (mux, "Writing tags");

      mux->tags_pos = ebml->pos;
      master_tags = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TAGS);
      if (has_main_tags) {
        master_tag = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TAG);
        gst_tag_list_foreach (tags, gst_matroska_mux_write_simple_tag, ebml);
        gst_ebml_write_master_finish (ebml, master_tag);
      }
      gst_matroska_mux_write_streams_tags (mux);
      gst_ebml_write_master_finish (ebml, master_tags);
    }
  }

  /* segment info */
  mux->info_pos = ebml->pos;
  master = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_SEGMENTINFO);

  /* WebM does not support SegmentUID field on SegmentInfo */
  if (!mux->is_webm) {
    for (i = 0; i < 4; i++) {
      segment_uid[i] = g_random_int ();
    }
    gst_ebml_write_binary (ebml, GST_MATROSKA_ID_SEGMENTUID,
        (guint8 *) segment_uid, 16);
  }

  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_TIMECODESCALE, mux->time_scale);
  mux->duration_pos = ebml->pos;
  /* get duration */
  if (!mux->ebml_write->streamable) {
    for (collected = mux->collect->data; collected;
        collected = g_slist_next (collected)) {
      GstMatroskaPad *collect_pad;
      GstPad *thepad;
      gint64 trackduration;

      collect_pad = (GstMatroskaPad *) collected->data;
      thepad = collect_pad->collect.pad;

      /* Query the total length of the track. */
      GST_DEBUG_OBJECT (thepad, "querying peer duration");
      if (gst_pad_peer_query_duration (thepad, GST_FORMAT_TIME, &trackduration)) {
        GST_DEBUG_OBJECT (thepad, "duration: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (trackduration));
        if (trackduration != GST_CLOCK_TIME_NONE && trackduration > duration) {
          duration = (GstClockTime) trackduration;
        }
      }
    }
    gst_ebml_write_float (ebml, GST_MATROSKA_ID_DURATION,
        gst_guint64_to_gdouble (duration) /
        gst_guint64_to_gdouble (mux->time_scale));
  }
  gst_ebml_write_utf8 (ebml, GST_MATROSKA_ID_MUXINGAPP,
      "GStreamer matroskamux version " PACKAGE_VERSION);
  if (mux->writing_app && mux->writing_app[0]) {
    gst_ebml_write_utf8 (ebml, GST_MATROSKA_ID_WRITINGAPP, mux->writing_app);
  }
  if (mux->creation_time != NULL) {
    time = g_date_time_to_unix (mux->creation_time) * GST_SECOND;
    time += g_date_time_get_microsecond (mux->creation_time) * GST_USECOND;
  } else {
    time = g_get_real_time () * GST_USECOND;
  }
  gst_ebml_write_date (ebml, GST_MATROSKA_ID_DATEUTC, time);
  gst_ebml_write_master_finish (ebml, master);

  /* tracks */
  mux->tracks_pos = ebml->pos;
  master = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TRACKS);

  for (collected = mux->collect->data; collected;
      collected = g_slist_next (collected)) {
    GstMatroskaPad *collect_pad;
    GstBuffer *buf;

    collect_pad = (GstMatroskaPad *) collected->data;

    /* This will cause an error at a later time */
    if (collect_pad->track->codec_id == NULL)
      continue;

    /* Find the smallest timestamp so we can offset all streams by this to
     * start at 0 */
    if (mux->offset_to_zero) {
      GstClockTime ts;

      if (collect_pad == first_pad)
        buf = first_pad_buf ? gst_buffer_ref (first_pad_buf) : NULL;
      else
        buf = gst_collect_pads_peek (mux->collect, collected->data);

      if (buf) {
        ts = gst_matroska_track_get_buffer_timestamp (collect_pad->track, buf);

        if (earliest_time == GST_CLOCK_TIME_NONE)
          earliest_time = ts;
        else if (ts != GST_CLOCK_TIME_NONE && ts < earliest_time)
          earliest_time = ts;
      }

      if (buf)
        gst_buffer_unref (buf);
    }

    /* For audio tracks, use the first buffers duration as the default
     * duration if we didn't get any better idea from the caps event already
     */
    if (collect_pad->track->type == GST_MATROSKA_TRACK_TYPE_AUDIO &&
        collect_pad->track->default_duration == 0) {
      if (collect_pad == first_pad)
        buf = first_pad_buf ? gst_buffer_ref (first_pad_buf) : NULL;
      else
        buf = gst_collect_pads_peek (mux->collect, collected->data);

      if (buf && GST_BUFFER_DURATION_IS_VALID (buf))
        collect_pad->track->default_duration =
            GST_BUFFER_DURATION (buf) + collect_pad->track->codec_delay;
      if (buf)
        gst_buffer_unref (buf);
    }

    collect_pad->track->num = tracknum++;
    child = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TRACKENTRY);
    gst_matroska_mux_track_header (mux, collect_pad->track);
    gst_ebml_write_master_finish (ebml, child);
    /* some remaining pad/track setup */
    collect_pad->default_duration_scaled =
        gst_util_uint64_scale (collect_pad->track->default_duration,
        1, mux->time_scale);
  }
  gst_ebml_write_master_finish (ebml, master);

  mux->earliest_time = earliest_time == GST_CLOCK_TIME_NONE ? 0 : earliest_time;

  /* chapters */
  toc = gst_toc_setter_get_toc (GST_TOC_SETTER (mux));
  if (toc != NULL && !mux->ebml_write->streamable) {
    guint64 master_chapters = 0;
    GstTocEntry *internal_edition;
    GList *cur, *chapters;

    GST_DEBUG ("Writing chapters");

    /* There are two UIDs for Chapters:
     * - The ChapterUID is a mandatory unsigned integer which internally
     * refers to a given chapter. Except for the title & language which use
     * dedicated fields, this UID can also be used to add tags to the Chapter.
     * The tags come in a separate section of the container.
     * - The ChapterStringUID is an optional UTF-8 string which also uniquely
     * refers to a chapter but from an external perspective. It can act as a
     * "WebVTT cue identifier" which "can be used to reference a specific cue,
     * for example from script or CSS".
     *
     * The ChapterUID will be generated and checked for unicity, while the
     * ChapterStringUID will receive the user defined UID.
     *
     * In order to be able to refer to chapters from the tags section,
     * we must maintain an internal Toc tree with the generated ChapterUID
     * (see gst_matroska_mux_write_toc_entry_tags) */

    /* Check whether we have editions or chapters at the root level. */
    cur = gst_toc_get_entries (toc);
    if (cur != NULL) {
      mux->chapters_pos = ebml->pos;

      mux->internal_toc = gst_toc_new (GST_TOC_SCOPE_GLOBAL);

      if (gst_toc_entry_get_entry_type (cur->data) ==
          GST_TOC_ENTRY_TYPE_EDITION) {
        /* Editions at the root level */
        for (; cur != NULL; cur = cur->next) {
          chapters = gst_toc_entry_get_sub_entries (cur->data);
          internal_edition = gst_matroska_mux_write_chapter_edition (mux,
              cur->data, chapters, ebml, &master_chapters);
          gst_toc_append_entry (mux->internal_toc, internal_edition);
        }
      } else {
        /* Chapters at the root level */
        internal_edition = gst_matroska_mux_write_chapter_edition (mux,
            NULL, cur, ebml, &master_chapters);
        gst_toc_append_entry (mux->internal_toc, internal_edition);
      }

      /* close master element if any edition was written */
      if (G_LIKELY (master_chapters != 0))
        gst_ebml_write_master_finish (ebml, master_chapters);
    }
  }

  /* lastly, flush the cache */
  gst_ebml_write_flush_cache (ebml, FALSE, 0);

  if (toc != NULL)
    gst_toc_unref (toc);
}

/* TODO: more sensible tag mappings */
static const struct
{
  const gchar *matroska_tagname;
  const gchar *gstreamer_tagname;
}
gst_matroska_tag_conv[] = {
  {
  GST_MATROSKA_TAG_ID_TITLE, GST_TAG_TITLE}, {
  GST_MATROSKA_TAG_ID_ARTIST, GST_TAG_ARTIST}, {
  GST_MATROSKA_TAG_ID_ALBUM, GST_TAG_ALBUM}, {
  GST_MATROSKA_TAG_ID_COMMENTS, GST_TAG_COMMENT}, {
  GST_MATROSKA_TAG_ID_BITSPS, GST_TAG_BITRATE}, {
  GST_MATROSKA_TAG_ID_BPS, GST_TAG_BITRATE}, {
  GST_MATROSKA_TAG_ID_ENCODER, GST_TAG_ENCODER}, {
  GST_MATROSKA_TAG_ID_DATE, GST_TAG_DATE}, {
  GST_MATROSKA_TAG_ID_ISRC, GST_TAG_ISRC}, {
  GST_MATROSKA_TAG_ID_COPYRIGHT, GST_TAG_COPYRIGHT}, {
  GST_MATROSKA_TAG_ID_BPM, GST_TAG_BEATS_PER_MINUTE}, {
  GST_MATROSKA_TAG_ID_TERMS_OF_USE, GST_TAG_LICENSE}, {
  GST_MATROSKA_TAG_ID_COMPOSER, GST_TAG_COMPOSER}, {
  GST_MATROSKA_TAG_ID_LEAD_PERFORMER, GST_TAG_PERFORMER}, {
  GST_MATROSKA_TAG_ID_GENRE, GST_TAG_GENRE}
};

/* Every stagefright implementation on android up to and including 6.0.1 is using
 libwebm with bug in matroska parsing, where it will choke on empty tag elements;
 so before outputting tags and tag elements we better make sure that there are
 actually tags we are going to write */
static gboolean
gst_matroska_mux_tag_list_is_empty (const GstTagList * list)
{
  int i;
  for (i = 0; i < gst_tag_list_n_tags (list); i++) {
    const gchar *tag = gst_tag_list_nth_tag_name (list, i);
    int i;
    for (i = 0; i < G_N_ELEMENTS (gst_matroska_tag_conv); i++) {
      const gchar *tagname_gst = gst_matroska_tag_conv[i].gstreamer_tagname;
      if (strcmp (tagname_gst, tag) == 0) {
        GValue src = { 0, };
        gchar *dest;

        if (!gst_tag_list_copy_value (&src, list, tag))
          break;
        dest = gst_value_serialize (&src);

        g_value_unset (&src);
        if (dest) {
          g_free (dest);
          return FALSE;
        }
      }
    }
  }
  return TRUE;
}

static void
gst_matroska_mux_write_simple_tag (const GstTagList * list, const gchar * tag,
    gpointer data)
{
  GstEbmlWrite *ebml = (GstEbmlWrite *) data;
  guint i;
  guint64 simpletag_master;

  for (i = 0; i < G_N_ELEMENTS (gst_matroska_tag_conv); i++) {
    const gchar *tagname_gst = gst_matroska_tag_conv[i].gstreamer_tagname;
    const gchar *tagname_mkv = gst_matroska_tag_conv[i].matroska_tagname;

    if (strcmp (tagname_gst, tag) == 0) {
      GValue src = { 0, };
      gchar *dest;

      if (!gst_tag_list_copy_value (&src, list, tag))
        break;
      if ((dest = gst_value_serialize (&src))) {

        simpletag_master = gst_ebml_write_master_start (ebml,
            GST_MATROSKA_ID_SIMPLETAG);
        gst_ebml_write_ascii (ebml, GST_MATROSKA_ID_TAGNAME, tagname_mkv);
        gst_ebml_write_utf8 (ebml, GST_MATROSKA_ID_TAGSTRING, dest);
        gst_ebml_write_master_finish (ebml, simpletag_master);
        g_free (dest);
      } else {
        GST_WARNING ("Can't transform tag '%s' to string", tagname_mkv);
      }
      g_value_unset (&src);
      break;
    }
  }
}

static void
gst_matroska_mux_write_stream_tags (GstMatroskaMux * mux, GstMatroskaPad * mpad)
{
  guint64 master_tag, master_targets;
  GstEbmlWrite *ebml;

  ebml = mux->ebml_write;

  if (G_UNLIKELY (mpad->tags == NULL
          || gst_matroska_mux_tag_list_is_empty (mpad->tags)))
    return;

  master_tag = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TAG);
  master_targets = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TARGETS);

  gst_ebml_write_uint (ebml, GST_MATROSKA_ID_TARGETTRACKUID, mpad->track->uid);

  gst_ebml_write_master_finish (ebml, master_targets);
  gst_tag_list_foreach (mpad->tags, gst_matroska_mux_write_simple_tag, ebml);
  gst_ebml_write_master_finish (ebml, master_tag);
}

static void
gst_matroska_mux_write_streams_tags (GstMatroskaMux * mux)
{
  GSList *walk;

  for (walk = mux->collect->data; walk; walk = g_slist_next (walk)) {
    GstMatroskaPad *collect_pad;

    collect_pad = (GstMatroskaPad *) walk->data;

    gst_matroska_mux_write_stream_tags (mux, collect_pad);
  }
}

static gboolean
gst_matroska_mux_streams_have_tags (GstMatroskaMux * mux)
{
  GSList *walk;

  for (walk = mux->collect->data; walk; walk = g_slist_next (walk)) {
    GstMatroskaPad *collect_pad;

    collect_pad = (GstMatroskaPad *) walk->data;
    if (!gst_matroska_mux_tag_list_is_empty (collect_pad->tags))
      return TRUE;
  }
  return FALSE;
}

static void
gst_matroska_mux_write_toc_entry_tags (GstMatroskaMux * mux,
    const GstTocEntry * entry, guint64 * master_tags, gboolean * has_tags)
{
  guint64 master_tag, master_targets;
  GstEbmlWrite *ebml;
  GList *cur;
  const GstTagList *tags;

  ebml = mux->ebml_write;

  tags = gst_toc_entry_get_tags (entry);
  if (G_UNLIKELY (tags != NULL && !gst_matroska_mux_tag_list_is_empty (tags))) {
    *has_tags = TRUE;

    if (*master_tags == 0) {
      mux->tags_pos = ebml->pos;
      *master_tags = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TAGS);
    }

    master_tag = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TAG);
    master_targets =
        gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TARGETS);

    if (gst_toc_entry_get_entry_type (entry) == GST_TOC_ENTRY_TYPE_EDITION)
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_TARGETEDITIONUID,
          g_ascii_strtoull (gst_toc_entry_get_uid (entry), NULL, 10));
    else
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_TARGETCHAPTERUID,
          g_ascii_strtoull (gst_toc_entry_get_uid (entry), NULL, 10));

    gst_ebml_write_master_finish (ebml, master_targets);
    gst_tag_list_foreach (tags, gst_matroska_mux_write_simple_tag, ebml);
    gst_ebml_write_master_finish (ebml, master_tag);
  }

  for (cur = gst_toc_entry_get_sub_entries (entry); cur != NULL;
      cur = cur->next) {
    gst_matroska_mux_write_toc_entry_tags (mux, cur->data, master_tags,
        has_tags);
  }
}

/**
 * gst_matroska_mux_finish:
 * @mux: #GstMatroskaMux
 *
 * Finish a new matroska file (write index etc...)
 */
static void
gst_matroska_mux_finish (GstMatroskaMux * mux)
{
  GstEbmlWrite *ebml = mux->ebml_write;
  guint64 pos;
  guint64 duration = 0;
  GSList *collected;
  const GstTagList *tags, *toc_tags;
  const GstToc *toc;
  gboolean has_main_tags, toc_has_tags = FALSE;
  GList *cur;

  /* finish last cluster */
  if (mux->cluster) {
    gst_ebml_write_master_finish (ebml, mux->cluster);
  }

  /* cues */
  if (mux->index != NULL) {
    guint n;
    guint64 master, pointentry_master, trackpos_master;

    mux->cues_pos = ebml->pos;
    gst_ebml_write_set_cache (ebml, 12 + 41 * mux->num_indexes);
    master = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_CUES);

    for (n = 0; n < mux->num_indexes; n++) {
      GstMatroskaIndex *idx = &mux->index[n];

      pointentry_master = gst_ebml_write_master_start (ebml,
          GST_MATROSKA_ID_POINTENTRY);
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CUETIME,
          idx->time / mux->time_scale);
      trackpos_master = gst_ebml_write_master_start (ebml,
          GST_MATROSKA_ID_CUETRACKPOSITIONS);
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CUETRACK, idx->track);
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CUECLUSTERPOSITION,
          idx->pos - mux->segment_master);
      gst_ebml_write_master_finish (ebml, trackpos_master);
      gst_ebml_write_master_finish (ebml, pointentry_master);
    }

    gst_ebml_write_master_finish (ebml, master);
    gst_ebml_write_flush_cache (ebml, FALSE, GST_CLOCK_TIME_NONE);
  }

  /* tags */
  tags = gst_tag_setter_get_tag_list (GST_TAG_SETTER (mux));
  has_main_tags = tags != NULL && !gst_matroska_mux_tag_list_is_empty (tags);
  toc = gst_toc_setter_get_toc (GST_TOC_SETTER (mux));

  if (has_main_tags || gst_matroska_mux_streams_have_tags (mux) || toc != NULL) {
    guint64 master_tags = 0, master_tag;

    GST_DEBUG_OBJECT (mux, "Writing tags");

    if (has_main_tags) {
      /* TODO: maybe limit via the TARGETS id by looking at the source pad */
      mux->tags_pos = ebml->pos;
      master_tags = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TAGS);
      master_tag = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TAG);

      if (tags != NULL)
        gst_tag_list_foreach (tags, gst_matroska_mux_write_simple_tag, ebml);
      if (mux->internal_toc != NULL) {
        toc_tags = gst_toc_get_tags (mux->internal_toc);
        toc_has_tags = (toc_tags != NULL);
        gst_tag_list_foreach (toc_tags, gst_matroska_mux_write_simple_tag,
            ebml);
      }

      gst_ebml_write_master_finish (ebml, master_tag);
    }

    if (mux->internal_toc != NULL) {
      for (cur = gst_toc_get_entries (mux->internal_toc); cur != NULL;
          cur = cur->next) {
        gst_matroska_mux_write_toc_entry_tags (mux, cur->data, &master_tags,
            &toc_has_tags);
      }
    }

    if (master_tags == 0 && gst_matroska_mux_streams_have_tags (mux)) {
      mux->tags_pos = ebml->pos;
      master_tags = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_TAGS);
    }
    gst_matroska_mux_write_streams_tags (mux);

    if (master_tags != 0)
      gst_ebml_write_master_finish (ebml, master_tags);
  }

  /* update seekhead. We know that:
   * - a seekhead contains 5 entries.
   * - order of entries is as above.
   * - a seekhead has a 4-byte header + 8-byte length
   * - each entry is 2-byte master, 2-byte ID pointer,
   *     2-byte length pointer, all 8/1-byte length, 4-
   *     byte ID and 8-byte length pointer, where the
   *     length pointer starts at 20.
   * - all entries are local to the segment (so pos - segment_master).
   * - so each entry is at 12 + 20 + num * 28. */
  gst_ebml_replace_uint (ebml, mux->seekhead_pos + 32,
      mux->info_pos - mux->segment_master);
  gst_ebml_replace_uint (ebml, mux->seekhead_pos + 60,
      mux->tracks_pos - mux->segment_master);
  if (toc != NULL && mux->chapters_pos > 0) {
    gst_ebml_replace_uint (ebml, mux->seekhead_pos + 88,
        mux->chapters_pos - mux->segment_master);
  } else {
    /* void'ify */
    guint64 my_pos = ebml->pos;

    gst_ebml_write_seek (ebml, mux->seekhead_pos + 68);
    gst_ebml_write_buffer_header (ebml, GST_EBML_ID_VOID, 26);
    gst_ebml_write_seek (ebml, my_pos);
  }
  if (mux->index != NULL) {
    gst_ebml_replace_uint (ebml, mux->seekhead_pos + 116,
        mux->cues_pos - mux->segment_master);
  } else {
    /* void'ify */
    guint64 my_pos = ebml->pos;

    gst_ebml_write_seek (ebml, mux->seekhead_pos + 96);
    gst_ebml_write_buffer_header (ebml, GST_EBML_ID_VOID, 26);
    gst_ebml_write_seek (ebml, my_pos);
  }

  if (tags != NULL || toc_has_tags) {
    gst_ebml_replace_uint (ebml, mux->seekhead_pos + 144,
        mux->tags_pos - mux->segment_master);
  } else {
    /* void'ify */
    guint64 my_pos = ebml->pos;

    gst_ebml_write_seek (ebml, mux->seekhead_pos + 124);
    gst_ebml_write_buffer_header (ebml, GST_EBML_ID_VOID, 26);
    gst_ebml_write_seek (ebml, my_pos);
  }

  if (toc != NULL) {
    gst_toc_unref (toc);
  }

  /* loop tracks:
   * - first get the overall duration
   *   (a released track may have left a duration in here)
   * - write some track header data for subtitles
   */
  duration = mux->duration;
  pos = ebml->pos;
  for (collected = mux->collect->data; collected;
      collected = g_slist_next (collected)) {
    GstMatroskaPad *collect_pad;
    /*
     * observed duration, this will never remain GST_CLOCK_TIME_NONE
     * since this means buffer without timestamps that is not possible
     */
    GstClockTime collected_duration = GST_CLOCK_TIME_NONE;

    collect_pad = (GstMatroskaPad *) collected->data;

    GST_DEBUG_OBJECT (mux,
        "Pad %" GST_PTR_FORMAT " start ts %" GST_TIME_FORMAT
        " end ts %" GST_TIME_FORMAT, collect_pad,
        GST_TIME_ARGS (collect_pad->start_ts),
        GST_TIME_ARGS (collect_pad->end_ts));

    if (GST_CLOCK_TIME_IS_VALID (collect_pad->start_ts) &&
        GST_CLOCK_TIME_IS_VALID (collect_pad->end_ts)) {
      collected_duration =
          GST_CLOCK_DIFF (collect_pad->start_ts, collect_pad->end_ts);
      GST_DEBUG_OBJECT (collect_pad,
          "final track duration: %" GST_TIME_FORMAT,
          GST_TIME_ARGS (collected_duration));
    } else {
      GST_WARNING_OBJECT (collect_pad, "unable to get final track duration");
    }
    if (GST_CLOCK_TIME_IS_VALID (collected_duration) &&
        duration < collected_duration)
      duration = collected_duration;

  }

  /* seek back (optional, but do anyway) */
  gst_ebml_write_seek (ebml, pos);

  /* update duration */
  if (duration != 0) {
    GST_DEBUG_OBJECT (mux, "final total duration: %" GST_TIME_FORMAT,
        GST_TIME_ARGS (duration));
    pos = mux->ebml_write->pos;
    gst_ebml_write_seek (ebml, mux->duration_pos);
    gst_ebml_write_float (ebml, GST_MATROSKA_ID_DURATION,
        gst_guint64_to_gdouble (duration) /
        gst_guint64_to_gdouble (mux->time_scale));
    gst_ebml_write_seek (ebml, pos);
  } else {
    /* void'ify */
    guint64 my_pos = ebml->pos;

    gst_ebml_write_seek (ebml, mux->duration_pos);
    gst_ebml_write_buffer_header (ebml, GST_EBML_ID_VOID, 8);
    gst_ebml_write_seek (ebml, my_pos);
  }
  GST_DEBUG_OBJECT (mux, "finishing segment");
  /* finish segment - this also writes element length */
  gst_ebml_write_master_finish (ebml, mux->segment_pos);
}

/**
 * gst_matroska_mux_buffer_header:
 * @track: Track context.
 * @relative_timestamp: relative timestamp of the buffer
 * @flags: Buffer flags.
 *
 * Create a buffer containing buffer header.
 *
 * Returns: New buffer.
 */
static GstBuffer *
gst_matroska_mux_create_buffer_header (GstMatroskaTrackContext * track,
    gint16 relative_timestamp, int flags)
{
  GstBuffer *hdr;
  guint8 *data = g_malloc (4);

  hdr = gst_buffer_new_wrapped (data, 4);
  /* track num - FIXME: what if num >= 0x80 (unlikely)? */
  data[0] = track->num | 0x80;
  /* time relative to clustertime */
  GST_WRITE_UINT16_BE (data + 1, relative_timestamp);

  /* flags */
  data[3] = flags;

  return hdr;
}

#define DIRAC_PARSE_CODE_SEQUENCE_HEADER 0x00
#define DIRAC_PARSE_CODE_END_OF_SEQUENCE 0x10
#define DIRAC_PARSE_CODE_IS_PICTURE(x) ((x & 0x08) != 0)

static GstBuffer *
gst_matroska_mux_handle_dirac_packet (GstMatroskaMux * mux,
    GstMatroskaPad * collect_pad, GstBuffer * buf)
{
  GstMatroskaTrackVideoContext *ctx =
      (GstMatroskaTrackVideoContext *) collect_pad->track;
  GstMapInfo map;
  guint8 *data;
  gsize size;
  guint8 parse_code;
  guint32 next_parse_offset;
  GstBuffer *ret = NULL;
  gboolean is_muxing_unit = FALSE;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  data = map.data;
  size = map.size;

  if (size < 13) {
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return ret;
  }

  /* Check if this buffer contains a picture or end-of-sequence packet */
  while (size >= 13) {
    if (GST_READ_UINT32_BE (data) != 0x42424344 /* 'BBCD' */ ) {
      gst_buffer_unmap (buf, &map);
      gst_buffer_unref (buf);
      return ret;
    }

    parse_code = GST_READ_UINT8 (data + 4);
    if (parse_code == DIRAC_PARSE_CODE_SEQUENCE_HEADER) {
      if (ctx->dirac_unit) {
        gst_buffer_unref (ctx->dirac_unit);
        ctx->dirac_unit = NULL;
      }
    } else if (DIRAC_PARSE_CODE_IS_PICTURE (parse_code) ||
        parse_code == DIRAC_PARSE_CODE_END_OF_SEQUENCE) {
      is_muxing_unit = TRUE;
      break;
    }

    next_parse_offset = GST_READ_UINT32_BE (data + 5);

    if (G_UNLIKELY (next_parse_offset == 0 || next_parse_offset > size))
      break;

    data += next_parse_offset;
    size -= next_parse_offset;
  }

  if (ctx->dirac_unit)
    ctx->dirac_unit = gst_buffer_append (ctx->dirac_unit, gst_buffer_ref (buf));
  else
    ctx->dirac_unit = gst_buffer_ref (buf);

  gst_buffer_unmap (buf, &map);

  if (is_muxing_unit) {
    ret = gst_buffer_make_writable (ctx->dirac_unit);
    ctx->dirac_unit = NULL;
    gst_buffer_copy_into (ret, buf,
        GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS, 0, -1);
    gst_buffer_unref (buf);
  } else {
    gst_buffer_unref (buf);
    ret = NULL;
  }

  return ret;
}

static void
gst_matroska_mux_stop_streamheader (GstMatroskaMux * mux)
{
  GstCaps *caps;
  GstStructure *s;
  GValue streamheader = { 0 };
  GValue bufval = { 0 };
  GstBuffer *streamheader_buffer;
  GstEbmlWrite *ebml = mux->ebml_write;

  streamheader_buffer = gst_ebml_stop_streamheader (ebml);
  caps = gst_caps_copy (mux->ebml_write->caps);
  s = gst_caps_get_structure (caps, 0);
  g_value_init (&streamheader, GST_TYPE_ARRAY);
  g_value_init (&bufval, GST_TYPE_BUFFER);
  GST_BUFFER_FLAG_SET (streamheader_buffer, GST_BUFFER_FLAG_HEADER);
  gst_value_set_buffer (&bufval, streamheader_buffer);
  gst_value_array_append_value (&streamheader, &bufval);
  g_value_unset (&bufval);
  gst_structure_set_value (s, "streamheader", &streamheader);
  g_value_unset (&streamheader);
  gst_caps_replace (&ebml->caps, caps);
  gst_buffer_unref (streamheader_buffer);
  gst_pad_set_caps (mux->srcpad, caps);
  gst_caps_unref (caps);
}

/**
 * gst_matroska_mux_write_data:
 * @mux: #GstMatroskaMux
 * @collect_pad: #GstMatroskaPad with the data
 *
 * Write collected data (called from gst_matroska_mux_collected).
 *
 * Returns: Result of the gst_pad_push issued to write the data.
 */
static GstFlowReturn
gst_matroska_mux_write_data (GstMatroskaMux * mux, GstMatroskaPad * collect_pad,
    GstBuffer * buf)
{
  GstEbmlWrite *ebml = mux->ebml_write;
  GstBuffer *hdr;
  guint64 blockgroup;
  gboolean write_duration;
  gint16 relative_timestamp;
  gint64 relative_timestamp64;
  guint64 block_duration, duration_diff = 0;
  gboolean is_video_keyframe = FALSE;
  gboolean is_video_invisible = FALSE;
  gboolean is_audio_only = FALSE;
  gboolean is_min_duration_reached = FALSE;
  gboolean is_max_duration_exceeded = FALSE;
  GstMatroskamuxPad *pad;
  gint flags = 0;
  GstClockTime buffer_timestamp;
  GstAudioClippingMeta *cmeta = NULL;

  /* write data */
  pad = GST_MATROSKAMUX_PAD_CAST (collect_pad->collect.pad);

  /* vorbis/theora headers are retrieved from caps and put in CodecPrivate */
  if (collect_pad->track->xiph_headers_to_skip > 0) {
    --collect_pad->track->xiph_headers_to_skip;
    if (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_HEADER)) {
      GST_LOG_OBJECT (collect_pad->collect.pad, "dropping streamheader buffer");
      gst_buffer_unref (buf);
      return GST_FLOW_OK;
    }
  }

  /* for dirac we have to queue up everything up to a picture unit */
  if (!strcmp (collect_pad->track->codec_id, GST_MATROSKA_CODEC_ID_VIDEO_DIRAC)) {
    buf = gst_matroska_mux_handle_dirac_packet (mux, collect_pad, buf);
    if (!buf)
      return GST_FLOW_OK;
  } else if (!strcmp (collect_pad->track->codec_id,
          GST_MATROSKA_CODEC_ID_VIDEO_PRORES)) {
    /* Remove the 'Frame container atom' header' */
    buf = gst_buffer_make_writable (buf);
    gst_buffer_resize (buf, 8, gst_buffer_get_size (buf) - 8);
  }

  buffer_timestamp =
      gst_matroska_track_get_buffer_timestamp (collect_pad->track, buf);
  if (buffer_timestamp >= mux->earliest_time) {
    buffer_timestamp -= mux->earliest_time;
  } else {
    buffer_timestamp = 0;
  }

  /* hm, invalid timestamp (due to --to be fixed--- element upstream);
   * this would wreak havoc with time stored in matroska file */
  /* TODO: maybe calculate a timestamp by using the previous timestamp
   * and default duration */
  if (!GST_CLOCK_TIME_IS_VALID (buffer_timestamp)) {
    GST_WARNING_OBJECT (collect_pad->collect.pad,
        "Invalid buffer timestamp; dropping buffer");
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }

  if (!strcmp (collect_pad->track->codec_id, GST_MATROSKA_CODEC_ID_AUDIO_OPUS)
      && collect_pad->track->codec_delay) {
    /* All timestamps should include the codec delay */
    if (buffer_timestamp > collect_pad->track->codec_delay) {
      buffer_timestamp += collect_pad->track->codec_delay;
    } else {
      buffer_timestamp = 0;
      duration_diff = collect_pad->track->codec_delay - buffer_timestamp;
    }
  }

  /* set the timestamp for outgoing buffers */
  ebml->timestamp = buffer_timestamp;

  if (collect_pad->track->type == GST_MATROSKA_TRACK_TYPE_VIDEO) {
    if (!GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT)) {
      GST_LOG_OBJECT (mux, "have video keyframe, ts=%" GST_TIME_FORMAT,
          GST_TIME_ARGS (buffer_timestamp));
      is_video_keyframe = TRUE;
    } else if (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_DECODE_ONLY) &&
        (!strcmp (collect_pad->track->codec_id, GST_MATROSKA_CODEC_ID_VIDEO_VP8)
            || !strcmp (collect_pad->track->codec_id,
                GST_MATROSKA_CODEC_ID_VIDEO_VP9))) {
      GST_LOG_OBJECT (mux,
          "have VP8 video invisible frame, " "ts=%" GST_TIME_FORMAT,
          GST_TIME_ARGS (buffer_timestamp));
      is_video_invisible = TRUE;
    }
  }

  is_audio_only = (collect_pad->track->type == GST_MATROSKA_TRACK_TYPE_AUDIO) &&
      (mux->num_streams == 1);
  is_min_duration_reached = (mux->min_cluster_duration == 0
      || (buffer_timestamp > mux->cluster_time
          && (buffer_timestamp - mux->cluster_time) >=
          mux->min_cluster_duration));
  is_max_duration_exceeded = (mux->max_cluster_duration > 0
      && buffer_timestamp > mux->cluster_time
      && (buffer_timestamp - mux->cluster_time) >=
      MIN (G_MAXINT16 * mux->time_scale, mux->max_cluster_duration));

  if (mux->cluster) {
    /* start a new cluster at every keyframe, at every GstForceKeyUnit event,
     * or when we may be reaching the limit of the relative timestamp */
    if (is_max_duration_exceeded || (is_video_keyframe
            && is_min_duration_reached) || mux->force_key_unit_event
        || (is_audio_only && is_min_duration_reached)) {
      if (!mux->ebml_write->streamable)
        gst_ebml_write_master_finish (ebml, mux->cluster);

      /* Forward the GstForceKeyUnit event after finishing the cluster */
      if (mux->force_key_unit_event) {
        gst_pad_push_event (mux->srcpad, mux->force_key_unit_event);
        mux->force_key_unit_event = NULL;
      }

      mux->prev_cluster_size = ebml->pos - mux->cluster_pos;
      mux->cluster_pos = ebml->pos;
      gst_ebml_write_set_cache (ebml, 0x20);
      mux->cluster =
          gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_CLUSTER);
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CLUSTERTIMECODE,
          gst_util_uint64_scale (buffer_timestamp, 1, mux->time_scale));
      GST_LOG_OBJECT (mux, "cluster timestamp %" G_GUINT64_FORMAT,
          gst_util_uint64_scale (buffer_timestamp, 1, mux->time_scale));
      gst_ebml_write_flush_cache (ebml, is_video_keyframe
          || is_audio_only, buffer_timestamp);
      mux->cluster_time = buffer_timestamp;
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_PREVSIZE,
          mux->prev_cluster_size);
    }
  } else {
    /* first cluster */

    mux->cluster_pos = ebml->pos;
    gst_ebml_write_set_cache (ebml, 0x20);
    mux->cluster = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_CLUSTER);
    gst_ebml_write_uint (ebml, GST_MATROSKA_ID_CLUSTERTIMECODE,
        gst_util_uint64_scale (buffer_timestamp, 1, mux->time_scale));
    gst_ebml_write_flush_cache (ebml, TRUE, buffer_timestamp);
    mux->cluster_time = buffer_timestamp;
  }

  /* We currently write index entries for all video tracks or for the audio
   * track in a single-track audio file.  This could be improved by keeping the
   * index only for the *first* video track. */

  /* TODO: index is useful for every track, should contain the number of
   * the block in the cluster which contains the timestamp, should also work
   * for files with multiple audio tracks.
   */
  if (!mux->ebml_write->streamable && (is_video_keyframe || is_audio_only)) {
    gint last_idx = -1;

    if (mux->min_index_interval != 0) {
      for (last_idx = mux->num_indexes - 1; last_idx >= 0; last_idx--) {
        if (mux->index[last_idx].track == collect_pad->track->num)
          break;
      }
    }

    if (last_idx < 0 || mux->min_index_interval == 0 ||
        (GST_CLOCK_DIFF (mux->index[last_idx].time, buffer_timestamp)
            >= mux->min_index_interval)) {
      GstMatroskaIndex *idx;

      if (mux->num_indexes % 32 == 0) {
        mux->index = g_renew (GstMatroskaIndex, mux->index,
            mux->num_indexes + 32);
      }
      idx = &mux->index[mux->num_indexes++];

      idx->pos = mux->cluster_pos;
      idx->time = buffer_timestamp;
      idx->track = collect_pad->track->num;
    }
  }

  /* Check if the duration differs from the default duration. */
  write_duration = FALSE;
  block_duration = 0;
  if (pad->frame_duration && GST_BUFFER_DURATION_IS_VALID (buf)) {
    block_duration = GST_BUFFER_DURATION (buf) + duration_diff;
    block_duration = gst_util_uint64_scale (block_duration, 1, mux->time_scale);

    /* small difference should be ok. */
    if (block_duration > collect_pad->default_duration_scaled + 1 ||
        block_duration < collect_pad->default_duration_scaled - 1) {
      write_duration = TRUE;
    }
  }

  /* write the block, for doctype v2 use SimpleBlock if possible
   * one slice (*breath*).
   * FIXME: Need to do correct lacing! */
  relative_timestamp64 = buffer_timestamp - mux->cluster_time;
  if (relative_timestamp64 >= 0) {
    /* round the timestamp */
    relative_timestamp64 += gst_util_uint64_scale (mux->time_scale, 1, 2);
    relative_timestamp = gst_util_uint64_scale (relative_timestamp64, 1,
        mux->time_scale);
  } else {
    /* round the timestamp */
    relative_timestamp64 -= gst_util_uint64_scale (mux->time_scale, 1, 2);
    relative_timestamp =
        -((gint16) gst_util_uint64_scale (-relative_timestamp64, 1,
            mux->time_scale));
  }

  if (is_video_invisible)
    flags |= 0x08;

  if (!strcmp (collect_pad->track->codec_id, GST_MATROSKA_CODEC_ID_AUDIO_OPUS)) {
    cmeta = gst_buffer_get_audio_clipping_meta (buf);
    g_assert (!cmeta || cmeta->format == GST_FORMAT_DEFAULT);

    /* Start clipping is done via header and CodecDelay */
    if (cmeta && !cmeta->end)
      cmeta = NULL;
  }

  if (mux->doctype_version > 1 && !write_duration && !cmeta) {
    if (is_video_keyframe)
      flags |= 0x80;

    hdr =
        gst_matroska_mux_create_buffer_header (collect_pad->track,
        relative_timestamp, flags);
    gst_ebml_write_set_cache (ebml, 0x40);
    gst_ebml_write_buffer_header (ebml, GST_MATROSKA_ID_SIMPLEBLOCK,
        gst_buffer_get_size (buf) + gst_buffer_get_size (hdr));
    gst_ebml_write_buffer (ebml, hdr);
    gst_ebml_write_flush_cache (ebml, FALSE, buffer_timestamp);
    gst_ebml_write_buffer (ebml, buf);

    return gst_ebml_last_write_result (ebml);
  } else {
    gst_ebml_write_set_cache (ebml, gst_buffer_get_size (buf) * 2);
    /* write and call order slightly unnatural,
     * but avoids seek and minizes pushing */
    blockgroup = gst_ebml_write_master_start (ebml, GST_MATROSKA_ID_BLOCKGROUP);
    hdr =
        gst_matroska_mux_create_buffer_header (collect_pad->track,
        relative_timestamp, flags);
    if (write_duration)
      gst_ebml_write_uint (ebml, GST_MATROSKA_ID_BLOCKDURATION, block_duration);

    if (!strcmp (collect_pad->track->codec_id, GST_MATROSKA_CODEC_ID_AUDIO_OPUS)
        && cmeta) {
      /* Start clipping is done via header and CodecDelay */
      if (cmeta->end) {
        guint64 end =
            gst_util_uint64_scale_round (cmeta->end, GST_SECOND, 48000);
        gst_ebml_write_sint (ebml, GST_MATROSKA_ID_DISCARDPADDING, end);
      }
    }

    gst_ebml_write_buffer_header (ebml, GST_MATROSKA_ID_BLOCK,
        gst_buffer_get_size (buf) + gst_buffer_get_size (hdr));
    gst_ebml_write_buffer (ebml, hdr);
    gst_ebml_write_master_finish_full (ebml, blockgroup,
        gst_buffer_get_size (buf));
    gst_ebml_write_flush_cache (ebml, FALSE, buffer_timestamp);
    gst_ebml_write_buffer (ebml, buf);

    return gst_ebml_last_write_result (ebml);
  }
}

/**
 * gst_matroska_mux_handle_buffer:
 * @pads: #GstCollectPads
 * @uuser_data: #GstMatroskaMux
 *
 * Collectpads callback.
 *
 * Returns: #GstFlowReturn
 */
static GstFlowReturn
gst_matroska_mux_handle_buffer (GstCollectPads * pads, GstCollectData * data,
    GstBuffer * buf, gpointer user_data)
{
  GstClockTime buffer_timestamp;
  GstMatroskaMux *mux = GST_MATROSKA_MUX (user_data);
  GstEbmlWrite *ebml = mux->ebml_write;
  GstMatroskaPad *best = (GstMatroskaPad *) data;
  GstFlowReturn ret = GST_FLOW_OK;
  GST_DEBUG_OBJECT (mux, "Collected pads");

  /* start with a header */
  if (mux->state == GST_MATROSKA_MUX_STATE_START) {
    if (mux->collect->data == NULL) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("No input streams configured"));
      return GST_FLOW_ERROR;
    }
    mux->state = GST_MATROSKA_MUX_STATE_HEADER;
    gst_ebml_start_streamheader (ebml);
    gst_matroska_mux_start (mux, best, buf);
    gst_matroska_mux_stop_streamheader (mux);
    mux->state = GST_MATROSKA_MUX_STATE_DATA;
  }

  /* if there is no best pad, we have reached EOS */
  if (best == NULL) {
    GST_DEBUG_OBJECT (mux, "No best pad. Finishing...");
    if (!mux->ebml_write->streamable) {
      gst_matroska_mux_finish (mux);
    } else {
      GST_DEBUG_OBJECT (mux, "... but streamable, nothing to finish");
    }
    gst_pad_push_event (mux->srcpad, gst_event_new_eos ());
    ret = GST_FLOW_EOS;
    goto exit;
  }

  if (best->track->codec_id == NULL) {
    GST_ERROR_OBJECT (best->collect.pad, "No codec-id for pad");
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto exit;
  }

  /* if we have a best stream, should also have a buffer */
  g_assert (buf);

  buffer_timestamp = gst_matroska_track_get_buffer_timestamp (best->track, buf);
  if (buffer_timestamp >= mux->earliest_time) {
    buffer_timestamp -= mux->earliest_time;
  } else {
    GST_ERROR_OBJECT (mux,
        "PTS before first PTS (%" GST_TIME_FORMAT " < %" GST_TIME_FORMAT ")",
        GST_TIME_ARGS (buffer_timestamp), GST_TIME_ARGS (mux->earliest_time));
    buffer_timestamp = 0;
  }

  GST_DEBUG_OBJECT (best->collect.pad, "best pad - buffer ts %"
      GST_TIME_FORMAT " dur %" GST_TIME_FORMAT,
      GST_TIME_ARGS (buffer_timestamp),
      GST_TIME_ARGS (GST_BUFFER_DURATION (buf)));

  /* make note of first and last encountered timestamps, so we can calculate
   * the actual duration later when we send an updated header on eos */
  if (GST_CLOCK_TIME_IS_VALID (buffer_timestamp)) {
    GstClockTime start_ts = buffer_timestamp;
    GstClockTime end_ts = start_ts;

    if (GST_BUFFER_DURATION_IS_VALID (buf))
      end_ts += GST_BUFFER_DURATION (buf);
    else if (best->track->default_duration)
      end_ts += best->track->default_duration;

    if (!GST_CLOCK_TIME_IS_VALID (best->end_ts) || end_ts > best->end_ts)
      best->end_ts = end_ts;

    if (G_UNLIKELY (best->start_ts == GST_CLOCK_TIME_NONE ||
            start_ts < best->start_ts))
      best->start_ts = start_ts;
  }

  /* write one buffer */
  ret = gst_matroska_mux_write_data (mux, best, buf);

exit:
  return ret;
}


/**
 * gst_matroska_mux_change_state:
 * @element: #GstMatroskaMux
 * @transition: State change transition.
 *
 * Change the muxer state.
 *
 * Returns: #GstStateChangeReturn
 */
static GstStateChangeReturn
gst_matroska_mux_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstMatroskaMux *mux = GST_MATROSKA_MUX (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_collect_pads_start (mux->collect);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_collect_pads_stop (mux->collect);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_matroska_mux_reset (GST_ELEMENT (mux));
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_matroska_mux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstMatroskaMux *mux;

  g_return_if_fail (GST_IS_MATROSKA_MUX (object));
  mux = GST_MATROSKA_MUX (object);

  switch (prop_id) {
    case PROP_WRITING_APP:
      if (!g_value_get_string (value)) {
        GST_WARNING_OBJECT (mux, "writing-app property can not be NULL");
        break;
      }
      g_free (mux->writing_app);
      mux->writing_app = g_value_dup_string (value);
      break;
    case PROP_DOCTYPE_VERSION:
      mux->doctype_version = g_value_get_int (value);
      break;
    case PROP_MIN_INDEX_INTERVAL:
      mux->min_index_interval = g_value_get_int64 (value);
      break;
    case PROP_STREAMABLE:
      mux->ebml_write->streamable = g_value_get_boolean (value);
      break;
    case PROP_TIMECODESCALE:
      mux->time_scale = g_value_get_int64 (value);
      break;
    case PROP_MIN_CLUSTER_DURATION:
      mux->min_cluster_duration = g_value_get_int64 (value);
      break;
    case PROP_MAX_CLUSTER_DURATION:
      mux->max_cluster_duration = g_value_get_int64 (value);
      break;
    case PROP_OFFSET_TO_ZERO:
      mux->offset_to_zero = g_value_get_boolean (value);
      break;
    case PROP_CREATION_TIME:
      g_clear_pointer (&mux->creation_time, g_date_time_unref);
      mux->creation_time = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_matroska_mux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstMatroskaMux *mux;

  g_return_if_fail (GST_IS_MATROSKA_MUX (object));
  mux = GST_MATROSKA_MUX (object);

  switch (prop_id) {
    case PROP_WRITING_APP:
      g_value_set_string (value, mux->writing_app);
      break;
    case PROP_DOCTYPE_VERSION:
      g_value_set_int (value, mux->doctype_version);
      break;
    case PROP_MIN_INDEX_INTERVAL:
      g_value_set_int64 (value, mux->min_index_interval);
      break;
    case PROP_STREAMABLE:
      g_value_set_boolean (value, mux->ebml_write->streamable);
      break;
    case PROP_TIMECODESCALE:
      g_value_set_int64 (value, mux->time_scale);
      break;
    case PROP_MIN_CLUSTER_DURATION:
      g_value_set_int64 (value, mux->min_cluster_duration);
      break;
    case PROP_MAX_CLUSTER_DURATION:
      g_value_set_int64 (value, mux->max_cluster_duration);
      break;
    case PROP_OFFSET_TO_ZERO:
      g_value_set_boolean (value, mux->offset_to_zero);
      break;
    case PROP_CREATION_TIME:
      g_value_set_boxed (value, mux->creation_time);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
