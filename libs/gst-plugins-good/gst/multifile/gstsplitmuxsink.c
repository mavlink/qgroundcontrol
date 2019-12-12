/* GStreamer Muxer bin that splits output stream by size/time
 * Copyright (C) <2014-2019> Jan Schmidt <jan@centricular.com>
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

/**
 * SECTION:element-splitmuxsink
 * @title: splitmuxsink
 * @short_description: Muxer wrapper for splitting output stream by size or time
 *
 * This element wraps a muxer and a sink, and starts a new file when the mux
 * contents are about to cross a threshold of maximum size of maximum time,
 * splitting at video keyframe boundaries. Exactly one input video stream
 * can be muxed, with as many accompanying audio and subtitle streams as
 * desired.
 *
 * By default, it uses mp4mux and filesink, but they can be changed via
 * the 'muxer' and 'sink' properties.
 *
 * The minimum file size is 1 GOP, however - so limits may be overrun if the
 * distance between any 2 keyframes is larger than the limits.
 *
 * If a video stream is available, the splitting process is driven by the video
 * stream contents, and the video stream must contain closed GOPs for the output
 * file parts to be played individually correctly. In the absence of a video
 * stream, the first available stream is used as reference for synchronization.
 *
 * In the async-finalize mode, when the threshold is crossed, the old muxer
 * and sink is disconnected from the pipeline and left to finish the file
 * asynchronously, and a new muxer and sink is created to continue with the
 * next fragment. For that reason, instead of muxer and sink objects, the
 * muxer-factory and sink-factory properties are used to construct the new
 * objects, together with muxer-properties and sink-properties.
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -e v4l2src num-buffers=500 ! video/x-raw,width=320,height=240 ! videoconvert ! queue ! timeoverlay ! x264enc key-int-max=10 ! h264parse ! splitmuxsink location=video%02d.mov max-size-time=10000000000 max-size-bytes=1000000
 * ]|
 * Records a video stream captured from a v4l2 device and muxes it into
 * ISO mp4 files, splitting as needed to limit size/duration to 10 seconds
 * and 1MB maximum size.
 *
 * |[
 * gst-launch-1.0 -e v4l2src num-buffers=500 ! video/x-raw,width=320,height=240 ! videoconvert ! queue ! timeoverlay ! x264enc key-int-max=10 ! h264parse ! splitmuxsink location=video%02d.mkv max-size-time=10000000000 muxer-factory=matroskamux muxer-properties="properties,streamable=true"
 * ]|
 * Records a video stream captured from a v4l2 device and muxer it into
 * streamable Matroska files, splitting as needed to limit size/duration to 10
 * seconds. Each file will finalize asynchronously.
 *
 * |[
 * gst-launch-1.0 videotestsrc num-buffers=10 ! jpegenc ! .video splitmuxsink muxer=qtmux muxer-pad-map=x-pad-map,video=video_1 location=test%05d.mp4 -v
 * ]|
 * Records 10 frames to an mp4 file, using a muxer-pad-map to make explicit mappings between the splitmuxsink sink pad and the corresponding muxer pad
 * it will deliver to.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib/gstdio.h>
#include <gst/video/video.h>
#include "gstsplitmuxsink.h"

GST_DEBUG_CATEGORY_STATIC (splitmux_debug);
#define GST_CAT_DEFAULT splitmux_debug

#define GST_SPLITMUX_LOCK(s) g_mutex_lock(&(s)->lock)
#define GST_SPLITMUX_UNLOCK(s) g_mutex_unlock(&(s)->lock)
#define GST_SPLITMUX_WAIT_INPUT(s) g_cond_wait (&(s)->input_cond, &(s)->lock)
#define GST_SPLITMUX_BROADCAST_INPUT(s) g_cond_broadcast (&(s)->input_cond)

#define GST_SPLITMUX_WAIT_OUTPUT(s) g_cond_wait (&(s)->output_cond, &(s)->lock)
#define GST_SPLITMUX_BROADCAST_OUTPUT(s) g_cond_broadcast (&(s)->output_cond)

static void split_now (GstSplitMuxSink * splitmux);
static void split_after (GstSplitMuxSink * splitmux);
static void split_at_running_time (GstSplitMuxSink * splitmux,
    GstClockTime split_time);

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_START_INDEX,
  PROP_MAX_SIZE_TIME,
  PROP_MAX_SIZE_BYTES,
  PROP_MAX_SIZE_TIMECODE,
  PROP_SEND_KEYFRAME_REQUESTS,
  PROP_MAX_FILES,
  PROP_MUXER_OVERHEAD,
  PROP_USE_ROBUST_MUXING,
  PROP_ALIGNMENT_THRESHOLD,
  PROP_MUXER,
  PROP_SINK,
  PROP_RESET_MUXER,
  PROP_ASYNC_FINALIZE,
  PROP_MUXER_FACTORY,
  PROP_MUXER_PROPERTIES,
  PROP_SINK_FACTORY,
  PROP_SINK_PROPERTIES,
  PROP_MUXERPAD_MAP
};

#define DEFAULT_MAX_SIZE_TIME       0
#define DEFAULT_MAX_SIZE_BYTES      0
#define DEFAULT_MAX_FILES           0
#define DEFAULT_MUXER_OVERHEAD      0.02
#define DEFAULT_SEND_KEYFRAME_REQUESTS FALSE
#define DEFAULT_ALIGNMENT_THRESHOLD 0
#define DEFAULT_MUXER "mp4mux"
#define DEFAULT_SINK "filesink"
#define DEFAULT_USE_ROBUST_MUXING FALSE
#define DEFAULT_RESET_MUXER TRUE
#define DEFAULT_ASYNC_FINALIZE FALSE
#define DEFAULT_START_INDEX 0

typedef struct _AsyncEosHelper
{
  MqStreamCtx *ctx;
  GstPad *pad;
} AsyncEosHelper;

enum
{
  SIGNAL_FORMAT_LOCATION,
  SIGNAL_FORMAT_LOCATION_FULL,
  SIGNAL_SPLIT_NOW,
  SIGNAL_SPLIT_AFTER,
  SIGNAL_SPLIT_AT_RUNNING_TIME,
  SIGNAL_MUXER_ADDED,
  SIGNAL_SINK_ADDED,
  SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

static GstStaticPadTemplate video_sink_template =
GST_STATIC_PAD_TEMPLATE ("video",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate video_aux_sink_template =
GST_STATIC_PAD_TEMPLATE ("video_aux_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate audio_sink_template =
GST_STATIC_PAD_TEMPLATE ("audio_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate subtitle_sink_template =
GST_STATIC_PAD_TEMPLATE ("subtitle_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate caption_sink_template =
GST_STATIC_PAD_TEMPLATE ("caption_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GQuark PAD_CONTEXT;
static GQuark EOS_FROM_US;
static GQuark RUNNING_TIME;
/* EOS_FROM_US is only valid in async-finalize mode. We need to know whether
 * to forward an incoming EOS message, but we cannot rely on the state of the
 * splitmux anymore, so we set this qdata on the sink instead.
 * The muxer and sink must be destroyed after both of these things have
 * finished:
 * 1) The EOS message has been sent when the fragment is ending
 * 2) The muxer has been unlinked and relinked
 * Therefore, EOS_FROM_US can have these two values:
 * 0: EOS was not requested from us. Forward the message. The muxer and the
 * sink will be destroyed together with the rest of the bin.
 * 1: EOS was requested from us, but the other of the two tasks hasn't
 * finished. Set EOS_FROM_US to 2 and do your stuff.
 * 2: EOS was requested from us and the other of the two tasks has finished.
 * Now we can destroy the muxer and the sink.
 */

static void
_do_init (void)
{
  PAD_CONTEXT = g_quark_from_static_string ("pad-context");
  EOS_FROM_US = g_quark_from_static_string ("eos-from-us");
  RUNNING_TIME = g_quark_from_static_string ("running-time");
}

#define gst_splitmux_sink_parent_class parent_class
G_DEFINE_TYPE_EXTENDED (GstSplitMuxSink, gst_splitmux_sink, GST_TYPE_BIN, 0,
    _do_init ());

static gboolean create_muxer (GstSplitMuxSink * splitmux);
static gboolean create_sink (GstSplitMuxSink * splitmux);
static void gst_splitmux_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_splitmux_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_splitmux_sink_dispose (GObject * object);
static void gst_splitmux_sink_finalize (GObject * object);

static GstPad *gst_splitmux_sink_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static void gst_splitmux_sink_release_pad (GstElement * element, GstPad * pad);

static GstStateChangeReturn gst_splitmux_sink_change_state (GstElement *
    element, GstStateChange transition);

static void bus_handler (GstBin * bin, GstMessage * msg);
static void set_next_filename (GstSplitMuxSink * splitmux, MqStreamCtx * ctx);
static void start_next_fragment (GstSplitMuxSink * splitmux, MqStreamCtx * ctx);
static void mq_stream_ctx_free (MqStreamCtx * ctx);
static void grow_blocked_queues (GstSplitMuxSink * splitmux);

static void gst_splitmux_sink_ensure_max_files (GstSplitMuxSink * splitmux);
static GstElement *create_element (GstSplitMuxSink * splitmux,
    const gchar * factory, const gchar * name, gboolean locked);

static void do_async_done (GstSplitMuxSink * splitmux);

static MqStreamBuf *
mq_stream_buf_new (void)
{
  return g_slice_new0 (MqStreamBuf);
}

static void
mq_stream_buf_free (MqStreamBuf * data)
{
  g_slice_free (MqStreamBuf, data);
}

static SplitMuxOutputCommand *
out_cmd_buf_new (void)
{
  return g_slice_new0 (SplitMuxOutputCommand);
}

static void
out_cmd_buf_free (SplitMuxOutputCommand * data)
{
  g_slice_free (SplitMuxOutputCommand, data);
}

static void
gst_splitmux_sink_class_init (GstSplitMuxSinkClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBinClass *gstbin_class = (GstBinClass *) klass;

  gobject_class->set_property = gst_splitmux_sink_set_property;
  gobject_class->get_property = gst_splitmux_sink_get_property;
  gobject_class->dispose = gst_splitmux_sink_dispose;
  gobject_class->finalize = gst_splitmux_sink_finalize;

  gst_element_class_set_static_metadata (gstelement_class,
      "Split Muxing Bin", "Generic/Bin/Muxer",
      "Convenience bin that muxes incoming streams into multiple time/size limited files",
      "Jan Schmidt <jan@centricular.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &video_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &video_aux_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &audio_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &subtitle_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &caption_sink_template);

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_splitmux_sink_change_state);
  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_splitmux_sink_request_new_pad);
  gstelement_class->release_pad =
      GST_DEBUG_FUNCPTR (gst_splitmux_sink_release_pad);

  gstbin_class->handle_message = bus_handler;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "File Output Pattern",
          "Format string pattern for the location of the files to write (e.g. video%05d.mp4)",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MUXER_OVERHEAD,
      g_param_spec_double ("mux-overhead", "Muxing Overhead",
          "Extra size overhead of muxing (0.02 = 2%)", 0.0, 1.0,
          DEFAULT_MUXER_OVERHEAD,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_SIZE_TIME,
      g_param_spec_uint64 ("max-size-time", "Max. size (ns)",
          "Max. amount of time per file (in ns, 0=disable)", 0, G_MAXUINT64,
          DEFAULT_MAX_SIZE_TIME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MAX_SIZE_BYTES,
      g_param_spec_uint64 ("max-size-bytes", "Max. size bytes",
          "Max. amount of data per file (in bytes, 0=disable)", 0, G_MAXUINT64,
          DEFAULT_MAX_SIZE_BYTES, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MAX_SIZE_TIMECODE,
      g_param_spec_string ("max-size-timecode", "Maximum timecode difference",
          "Maximum difference in timecode between first and last frame. "
          "Separator is assumed to be \":\" everywhere (e.g. 01:00:00:00). "
          "Will only be effective if a timecode track is present.",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SEND_KEYFRAME_REQUESTS,
      g_param_spec_boolean ("send-keyframe-requests",
          "Request keyframes at max-size-time",
          "Request a keyframe every max-size-time ns to try splitting at that point. "
          "Needs max-size-bytes to be 0 in order to be effective.",
          DEFAULT_SEND_KEYFRAME_REQUESTS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MAX_FILES,
      g_param_spec_uint ("max-files", "Max files",
          "Maximum number of files to keep on disk. Once the maximum is reached,"
          "old files start to be deleted to make room for new ones.", 0,
          G_MAXUINT, DEFAULT_MAX_FILES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_ALIGNMENT_THRESHOLD,
      g_param_spec_uint64 ("alignment-threshold", "Alignment threshold (ns)",
          "Allow non-reference streams to be that many ns before the reference"
          " stream",
          0, G_MAXUINT64, DEFAULT_ALIGNMENT_THRESHOLD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MUXER,
      g_param_spec_object ("muxer", "Muxer",
          "The muxer element to use (NULL = default mp4mux). "
          "Valid only for async-finalize = FALSE",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SINK,
      g_param_spec_object ("sink", "Sink",
          "The sink element (or element chain) to use (NULL = default filesink). "
          "Valid only for async-finalize = FALSE",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_USE_ROBUST_MUXING,
      g_param_spec_boolean ("use-robust-muxing",
          "Support robust-muxing mode of some muxers",
          "Check if muxers support robust muxing via the reserved-max-duration and "
          "reserved-duration-remaining properties and use them if so. "
          "(Only present on qtmux and mp4mux for now). splitmuxsink may then also "
          " create new fragments if the reserved header space is about to overflow. "
          "Note that for mp4mux and qtmux, reserved-moov-update-period must be set "
          "manually by the app to a non-zero value for robust muxing to have an effect.",
          DEFAULT_USE_ROBUST_MUXING,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RESET_MUXER,
      g_param_spec_boolean ("reset-muxer",
          "Reset Muxer",
          "Reset the muxer after each segment. Disabling this will not work for most muxers.",
          DEFAULT_RESET_MUXER, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ASYNC_FINALIZE,
      g_param_spec_boolean ("async-finalize",
          "Finalize fragments asynchronously",
          "Finalize each fragment asynchronously and start a new one",
          DEFAULT_ASYNC_FINALIZE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MUXER_FACTORY,
      g_param_spec_string ("muxer-factory", "Muxer factory",
          "The muxer element factory to use (default = mp4mux). "
          "Valid only for async-finalize = TRUE",
          "mp4mux", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MUXER_PROPERTIES,
      g_param_spec_boxed ("muxer-properties", "Muxer properties",
          "The muxer element properties to use. "
          "Example: {properties,boolean-prop=true,string-prop=\"hi\"}. "
          "Valid only for async-finalize = TRUE",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SINK_FACTORY,
      g_param_spec_string ("sink-factory", "Sink factory",
          "The sink element factory to use (default = filesink). "
          "Valid only for async-finalize = TRUE",
          "filesink", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SINK_PROPERTIES,
      g_param_spec_boxed ("sink-properties", "Sink properties",
          "The sink element properties to use. "
          "Example: {properties,boolean-prop=true,string-prop=\"hi\"}. "
          "Valid only for async-finalize = TRUE",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_START_INDEX,
      g_param_spec_int ("start-index", "Start Index",
          "Start value of fragment index.",
          0, G_MAXINT, DEFAULT_START_INDEX,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstSplitMuxSink::muxer-pad-map
   *
   * An optional GstStructure that provides a map from splitmuxsink sinkpad
   * names to muxer pad names they should feed. Splitmuxsink has some default
   * mapping behaviour to link video to video pads and audio to audio pads
   * that usually works fine. This property is useful if you need to ensure
   * a particular mapping to muxed streams.
   *
   * The GstStructure contains string fields like so:
   *   splitmuxsink muxer-pad-map=x-pad-map,video=video_1
   *
   * Since: 1.18
   */
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_MUXERPAD_MAP,
      g_param_spec_boxed ("muxer-pad-map", "Muxer pad map",
          "A GstStructure specifies the mapping from splitmuxsink sink pads to muxer pads",
          GST_TYPE_STRUCTURE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  /**
   * GstSplitMuxSink::format-location:
   * @splitmux: the #GstSplitMuxSink
   * @fragment_id: the sequence number of the file to be created
   *
   * Returns: the location to be used for the next output file
   */
  signals[SIGNAL_FORMAT_LOCATION] =
      g_signal_new ("format-location", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_STRING, 1, G_TYPE_UINT);

  /**
   * GstSplitMuxSink::format-location-full:
   * @splitmux: the #GstSplitMuxSink
   * @fragment_id: the sequence number of the file to be created
   * @first_sample: A #GstSample containing the first buffer
   *   from the reference stream in the new file
   *
   * Returns: the location to be used for the next output file
   */
  signals[SIGNAL_FORMAT_LOCATION_FULL] =
      g_signal_new ("format-location-full", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_STRING, 2, G_TYPE_UINT,
      GST_TYPE_SAMPLE);

  /**
   * GstSplitMuxSink::split-now:
   * @splitmux: the #GstSplitMuxSink
   *
   * When called by the user, this action signal splits the video file (and begins a new one) immediately.
   * The current GOP will be output to the new file.
   *
   * Since: 1.14
   */
  signals[SIGNAL_SPLIT_NOW] =
      g_signal_new ("split-now", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstSplitMuxSinkClass, split_now), NULL, NULL, NULL,
      G_TYPE_NONE, 0);

  /**
   * GstSplitMuxSink::split-after:
   * @splitmux: the #GstSplitMuxSink
   *
   * When called by the user, this action signal splits the video file (and begins a new one) immediately.
   * Unlike the 'split-now' signal, with 'split-after', the current GOP will be output to the old file.
   *
   * Since: 1.16
   */
  signals[SIGNAL_SPLIT_AFTER] =
      g_signal_new ("split-after", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstSplitMuxSinkClass, split_after), NULL, NULL, NULL,
      G_TYPE_NONE, 0);

  /**
   * GstSplitMuxSink::split-at-running-time:
   * @splitmux: the #GstSplitMuxSink
   *
   * When called by the user, this action signal splits the video file (and
   * begins a new one) as soon as the given running time is reached. If this
   * action signal is called multiple times, running times are queued up and
   * processed in the order they were given.
   *
   * Note that this is prone to race conditions, where said running time is
   * reached and surpassed before we had a chance to split. The file will
   * still split immediately, but in order to make sure that the split doesn't
   * happen too late, it is recommended to call this action signal from
   * something that will prevent further buffers from flowing into
   * splitmuxsink before the split is completed, such as a pad probe before
   * splitmuxsink.
   *
   *
   * Since: 1.16
   */
  signals[SIGNAL_SPLIT_AT_RUNNING_TIME] =
      g_signal_new ("split-at-running-time", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstSplitMuxSinkClass, split_at_running_time), NULL, NULL,
      NULL, G_TYPE_NONE, 1, G_TYPE_UINT64);

  /**
   * GstSplitMuxSink::muxer-added:
   * @splitmux: the #GstSplitMuxSink
   * @muxer: the newly added muxer element
   *
   * Since: 1.14
   */
  signals[SIGNAL_MUXER_ADDED] =
      g_signal_new ("muxer-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_ELEMENT);

  /**
   * GstSplitMuxSink::sink-added:
   * @splitmux: the #GstSplitMuxSink
   * @sink: the newly added sink element
   *
   * Since: 1.14
   */
  signals[SIGNAL_SINK_ADDED] =
      g_signal_new ("sink-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_ELEMENT);

  klass->split_now = split_now;
  klass->split_after = split_after;
  klass->split_at_running_time = split_at_running_time;
}

static void
gst_splitmux_sink_init (GstSplitMuxSink * splitmux)
{
  g_mutex_init (&splitmux->lock);
  g_cond_init (&splitmux->input_cond);
  g_cond_init (&splitmux->output_cond);
  g_queue_init (&splitmux->out_cmd_q);

  splitmux->mux_overhead = DEFAULT_MUXER_OVERHEAD;
  splitmux->threshold_time = DEFAULT_MAX_SIZE_TIME;
  splitmux->threshold_bytes = DEFAULT_MAX_SIZE_BYTES;
  splitmux->max_files = DEFAULT_MAX_FILES;
  splitmux->send_keyframe_requests = DEFAULT_SEND_KEYFRAME_REQUESTS;
  splitmux->next_max_tc_time = GST_CLOCK_TIME_NONE;
  splitmux->alignment_threshold = DEFAULT_ALIGNMENT_THRESHOLD;
  splitmux->use_robust_muxing = DEFAULT_USE_ROBUST_MUXING;
  splitmux->reset_muxer = DEFAULT_RESET_MUXER;

  splitmux->threshold_timecode_str = NULL;

  splitmux->async_finalize = DEFAULT_ASYNC_FINALIZE;
  splitmux->muxer_factory = g_strdup (DEFAULT_MUXER);
  splitmux->muxer_properties = NULL;
  splitmux->sink_factory = g_strdup (DEFAULT_SINK);
  splitmux->sink_properties = NULL;

  GST_OBJECT_FLAG_SET (splitmux, GST_ELEMENT_FLAG_SINK);
  splitmux->split_requested = FALSE;
  splitmux->do_split_next_gop = FALSE;
  splitmux->times_to_split = gst_queue_array_new_for_struct (8, 8);
}

static void
gst_splitmux_reset (GstSplitMuxSink * splitmux)
{
  if (splitmux->muxer) {
    gst_element_set_locked_state (splitmux->muxer, TRUE);
    gst_element_set_state (splitmux->muxer, GST_STATE_NULL);
    gst_bin_remove (GST_BIN (splitmux), splitmux->muxer);
  }
  if (splitmux->active_sink) {
    gst_element_set_locked_state (splitmux->active_sink, TRUE);
    gst_element_set_state (splitmux->active_sink, GST_STATE_NULL);
    gst_bin_remove (GST_BIN (splitmux), splitmux->active_sink);
  }

  splitmux->sink = splitmux->active_sink = splitmux->muxer = NULL;
}

static void
gst_splitmux_sink_dispose (GObject * object)
{
  GstSplitMuxSink *splitmux = GST_SPLITMUX_SINK (object);

  /* Calling parent dispose invalidates all child pointers */
  splitmux->sink = splitmux->active_sink = splitmux->muxer = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_splitmux_sink_finalize (GObject * object)
{
  GstSplitMuxSink *splitmux = GST_SPLITMUX_SINK (object);
  g_cond_clear (&splitmux->input_cond);
  g_cond_clear (&splitmux->output_cond);
  g_mutex_clear (&splitmux->lock);
  g_queue_foreach (&splitmux->out_cmd_q, (GFunc) out_cmd_buf_free, NULL);
  g_queue_clear (&splitmux->out_cmd_q);

  if (splitmux->muxerpad_map)
    gst_structure_free (splitmux->muxerpad_map);

  if (splitmux->provided_sink)
    gst_object_unref (splitmux->provided_sink);
  if (splitmux->provided_muxer)
    gst_object_unref (splitmux->provided_muxer);

  if (splitmux->muxer_factory)
    g_free (splitmux->muxer_factory);
  if (splitmux->muxer_properties)
    gst_structure_free (splitmux->muxer_properties);
  if (splitmux->sink_factory)
    g_free (splitmux->sink_factory);
  if (splitmux->sink_properties)
    gst_structure_free (splitmux->sink_properties);

  if (splitmux->threshold_timecode_str)
    g_free (splitmux->threshold_timecode_str);

  if (splitmux->times_to_split)
    gst_queue_array_free (splitmux->times_to_split);

  g_free (splitmux->location);

  /* Make sure to free any un-released contexts. There should not be any,
   * because the dispose will have freed all request pads though */
  g_list_foreach (splitmux->contexts, (GFunc) mq_stream_ctx_free, NULL);
  g_list_free (splitmux->contexts);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*
 * Set any time threshold to the muxer, if it has
 * reserved-max-duration and reserved-duration-remaining
 * properties. Called when creating/claiming the muxer
 * in create_elements() */
static void
update_muxer_properties (GstSplitMuxSink * sink)
{
  GObjectClass *klass;
  GstClockTime threshold_time;

  sink->muxer_has_reserved_props = FALSE;
  if (sink->muxer == NULL)
    return;
  klass = G_OBJECT_GET_CLASS (sink->muxer);
  if (g_object_class_find_property (klass, "reserved-max-duration") == NULL)
    return;
  if (g_object_class_find_property (klass,
          "reserved-duration-remaining") == NULL)
    return;
  sink->muxer_has_reserved_props = TRUE;

  GST_LOG_OBJECT (sink, "Setting muxer reserved time to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (sink->threshold_time));
  GST_OBJECT_LOCK (sink);
  threshold_time = sink->threshold_time;
  GST_OBJECT_UNLOCK (sink);

  if (threshold_time > 0) {
    /* Tell the muxer how much space to reserve */
    GstClockTime muxer_threshold = threshold_time;
    g_object_set (sink->muxer, "reserved-max-duration", muxer_threshold, NULL);
  }
}

static void
gst_splitmux_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSplitMuxSink *splitmux = GST_SPLITMUX_SINK (object);

  switch (prop_id) {
    case PROP_LOCATION:{
      GST_OBJECT_LOCK (splitmux);
      g_free (splitmux->location);
      splitmux->location = g_value_dup_string (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    }
    case PROP_START_INDEX:
      GST_OBJECT_LOCK (splitmux);
      splitmux->start_index = g_value_get_int (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MAX_SIZE_BYTES:
      GST_OBJECT_LOCK (splitmux);
      splitmux->threshold_bytes = g_value_get_uint64 (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MAX_SIZE_TIME:
      GST_OBJECT_LOCK (splitmux);
      splitmux->threshold_time = g_value_get_uint64 (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MAX_SIZE_TIMECODE:
      GST_OBJECT_LOCK (splitmux);
      splitmux->threshold_timecode_str = g_value_dup_string (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_SEND_KEYFRAME_REQUESTS:
      GST_OBJECT_LOCK (splitmux);
      splitmux->send_keyframe_requests = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MAX_FILES:
      GST_OBJECT_LOCK (splitmux);
      splitmux->max_files = g_value_get_uint (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXER_OVERHEAD:
      GST_OBJECT_LOCK (splitmux);
      splitmux->mux_overhead = g_value_get_double (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_USE_ROBUST_MUXING:
      GST_OBJECT_LOCK (splitmux);
      splitmux->use_robust_muxing = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (splitmux);
      if (splitmux->use_robust_muxing)
        update_muxer_properties (splitmux);
      break;
    case PROP_ALIGNMENT_THRESHOLD:
      GST_OBJECT_LOCK (splitmux);
      splitmux->alignment_threshold = g_value_get_uint64 (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_SINK:
      GST_OBJECT_LOCK (splitmux);
      if (splitmux->provided_sink)
        gst_object_unref (splitmux->provided_sink);
      splitmux->provided_sink = g_value_get_object (value);
      gst_object_ref_sink (splitmux->provided_sink);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXER:
      GST_OBJECT_LOCK (splitmux);
      if (splitmux->provided_muxer)
        gst_object_unref (splitmux->provided_muxer);
      splitmux->provided_muxer = g_value_get_object (value);
      gst_object_ref_sink (splitmux->provided_muxer);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_RESET_MUXER:
      GST_OBJECT_LOCK (splitmux);
      splitmux->reset_muxer = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_ASYNC_FINALIZE:
      GST_OBJECT_LOCK (splitmux);
      splitmux->async_finalize = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXER_FACTORY:
      GST_OBJECT_LOCK (splitmux);
      if (splitmux->muxer_factory)
        g_free (splitmux->muxer_factory);
      splitmux->muxer_factory = g_value_dup_string (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXER_PROPERTIES:
      GST_OBJECT_LOCK (splitmux);
      if (splitmux->muxer_properties)
        gst_structure_free (splitmux->muxer_properties);
      if (gst_value_get_structure (value))
        splitmux->muxer_properties =
            gst_structure_copy (gst_value_get_structure (value));
      else
        splitmux->muxer_properties = NULL;
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_SINK_FACTORY:
      GST_OBJECT_LOCK (splitmux);
      if (splitmux->sink_factory)
        g_free (splitmux->sink_factory);
      splitmux->sink_factory = g_value_dup_string (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_SINK_PROPERTIES:
      GST_OBJECT_LOCK (splitmux);
      if (splitmux->sink_properties)
        gst_structure_free (splitmux->sink_properties);
      if (gst_value_get_structure (value))
        splitmux->sink_properties =
            gst_structure_copy (gst_value_get_structure (value));
      else
        splitmux->sink_properties = NULL;
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXERPAD_MAP:
    {
      const GstStructure *s = gst_value_get_structure (value);
      GST_SPLITMUX_LOCK (splitmux);
      if (splitmux->muxerpad_map) {
        gst_structure_free (splitmux->muxerpad_map);
      }
      if (s)
        splitmux->muxerpad_map = gst_structure_copy (s);
      else
        splitmux->muxerpad_map = NULL;
      GST_SPLITMUX_UNLOCK (splitmux);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_splitmux_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSplitMuxSink *splitmux = GST_SPLITMUX_SINK (object);

  switch (prop_id) {
    case PROP_LOCATION:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_string (value, splitmux->location);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_START_INDEX:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_int (value, splitmux->start_index);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MAX_SIZE_BYTES:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_uint64 (value, splitmux->threshold_bytes);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MAX_SIZE_TIME:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_uint64 (value, splitmux->threshold_time);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MAX_SIZE_TIMECODE:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_string (value, splitmux->threshold_timecode_str);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_SEND_KEYFRAME_REQUESTS:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_boolean (value, splitmux->send_keyframe_requests);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MAX_FILES:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_uint (value, splitmux->max_files);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXER_OVERHEAD:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_double (value, splitmux->mux_overhead);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_USE_ROBUST_MUXING:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_boolean (value, splitmux->use_robust_muxing);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_ALIGNMENT_THRESHOLD:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_uint64 (value, splitmux->alignment_threshold);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_SINK:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_object (value, splitmux->provided_sink);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXER:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_object (value, splitmux->provided_muxer);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_RESET_MUXER:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_boolean (value, splitmux->reset_muxer);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_ASYNC_FINALIZE:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_boolean (value, splitmux->async_finalize);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXER_FACTORY:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_string (value, splitmux->muxer_factory);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXER_PROPERTIES:
      GST_OBJECT_LOCK (splitmux);
      gst_value_set_structure (value, splitmux->muxer_properties);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_SINK_FACTORY:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_string (value, splitmux->sink_factory);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_SINK_PROPERTIES:
      GST_OBJECT_LOCK (splitmux);
      gst_value_set_structure (value, splitmux->sink_properties);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    case PROP_MUXERPAD_MAP:
      GST_SPLITMUX_LOCK (splitmux);
      gst_value_set_structure (value, splitmux->muxerpad_map);
      GST_SPLITMUX_UNLOCK (splitmux);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* Convenience function */
static inline GstClockTimeDiff
my_segment_to_running_time (GstSegment * segment, GstClockTime val)
{
  GstClockTimeDiff res = GST_CLOCK_STIME_NONE;

  if (GST_CLOCK_TIME_IS_VALID (val)) {
    gboolean sign =
        gst_segment_to_running_time_full (segment, GST_FORMAT_TIME, val, &val);
    if (sign > 0)
      res = val;
    else if (sign < 0)
      res = -val;
  }
  return res;
}

static MqStreamCtx *
mq_stream_ctx_new (GstSplitMuxSink * splitmux)
{
  MqStreamCtx *ctx;

  ctx = g_new0 (MqStreamCtx, 1);
  ctx->splitmux = splitmux;
  gst_segment_init (&ctx->in_segment, GST_FORMAT_UNDEFINED);
  gst_segment_init (&ctx->out_segment, GST_FORMAT_UNDEFINED);
  ctx->in_running_time = ctx->out_running_time = GST_CLOCK_STIME_NONE;
  g_queue_init (&ctx->queued_bufs);
  return ctx;
}

static void
mq_stream_ctx_free (MqStreamCtx * ctx)
{
  if (ctx->q) {
    GstObject *parent = gst_object_get_parent (GST_OBJECT (ctx->q));

    g_signal_handler_disconnect (ctx->q, ctx->q_overrun_id);

    if (parent == GST_OBJECT_CAST (ctx->splitmux)) {
      gst_element_set_locked_state (ctx->q, TRUE);
      gst_element_set_state (ctx->q, GST_STATE_NULL);
      gst_bin_remove (GST_BIN (ctx->splitmux), ctx->q);
      gst_object_unref (parent);
    }
    gst_object_unref (ctx->q);
  }
  gst_buffer_replace (&ctx->prev_in_keyframe, NULL);
  gst_object_unref (ctx->sinkpad);
  gst_object_unref (ctx->srcpad);
  g_queue_foreach (&ctx->queued_bufs, (GFunc) mq_stream_buf_free, NULL);
  g_queue_clear (&ctx->queued_bufs);
  g_free (ctx);
}

static void
send_fragment_opened_closed_msg (GstSplitMuxSink * splitmux, gboolean opened,
    GstElement * sink)
{
  gchar *location = NULL;
  GstMessage *msg;
  const gchar *msg_name = opened ?
      "splitmuxsink-fragment-opened" : "splitmuxsink-fragment-closed";
  GstClockTime running_time = splitmux->reference_ctx->out_running_time;

  if (!opened) {
    GstClockTime *rtime = g_object_get_qdata (G_OBJECT (sink), RUNNING_TIME);
    if (rtime)
      running_time = *rtime;
  }

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (splitmux->sink),
          "location") != NULL)
    g_object_get (sink, "location", &location, NULL);


  /* If it's in the middle of a teardown, the reference_ctc might have become
   * NULL */
  if (splitmux->reference_ctx) {
    msg = gst_message_new_element (GST_OBJECT (splitmux),
        gst_structure_new (msg_name,
            "location", G_TYPE_STRING, location,
            "running-time", GST_TYPE_CLOCK_TIME, running_time, NULL));
    gst_element_post_message (GST_ELEMENT_CAST (splitmux), msg);
  }

  g_free (location);
}

static void
send_eos_async (GstSplitMuxSink * splitmux, AsyncEosHelper * helper)
{
  GstEvent *eos;
  GstPad *pad;
  MqStreamCtx *ctx;

  eos = gst_event_new_eos ();
  pad = helper->pad;
  ctx = helper->ctx;

  GST_SPLITMUX_LOCK (splitmux);
  if (!pad)
    pad = gst_pad_get_peer (ctx->srcpad);
  GST_SPLITMUX_UNLOCK (splitmux);

  gst_pad_send_event (pad, eos);
  GST_INFO_OBJECT (splitmux, "Sent async EOS on %" GST_PTR_FORMAT, pad);

  gst_object_unref (pad);
  g_free (helper);
}

/* Called with lock held, drops the lock to send EOS to the
 * pad
 */
static void
send_eos (GstSplitMuxSink * splitmux, MqStreamCtx * ctx)
{
  GstEvent *eos;
  GstPad *pad;

  eos = gst_event_new_eos ();
  pad = gst_pad_get_peer (ctx->srcpad);

  ctx->out_eos = TRUE;

  GST_INFO_OBJECT (splitmux, "Sending EOS on %" GST_PTR_FORMAT, pad);
  GST_SPLITMUX_UNLOCK (splitmux);
  gst_pad_send_event (pad, eos);
  GST_SPLITMUX_LOCK (splitmux);

  gst_object_unref (pad);
}

/* Called with lock held. Schedules an EOS event to the ctx pad
 * to happen in another thread */
static void
eos_context_async (MqStreamCtx * ctx, GstSplitMuxSink * splitmux)
{
  AsyncEosHelper *helper = g_new0 (AsyncEosHelper, 1);
  GstPad *srcpad, *sinkpad;

  srcpad = ctx->srcpad;
  sinkpad = gst_pad_get_peer (srcpad);

  helper->ctx = ctx;
  helper->pad = sinkpad;        /* Takes the reference */

  ctx->out_eos_async_done = TRUE;
  /* HACK: Here, we explicitly unset the SINK flag on the target sink element
   * that's about to be asynchronously disposed, so that it no longer
   * participates in GstBin EOS logic. This fixes a race where if
   * splitmuxsink really reaches EOS before an asynchronous background
   * element has finished, then the bin won't actually send EOS to the
   * pipeline. Even after finishing and removing the old element, the
   * bin doesn't re-check EOS status on removing a SINK element. This
   * should be fixed in core, making this hack unnecessary. */
  GST_OBJECT_FLAG_UNSET (splitmux->active_sink, GST_ELEMENT_FLAG_SINK);

  GST_DEBUG_OBJECT (splitmux, "scheduled EOS to pad %" GST_PTR_FORMAT " ctx %p",
      sinkpad, ctx);

  g_assert_nonnull (helper->pad);
  gst_element_call_async (GST_ELEMENT (splitmux),
      (GstElementCallAsyncFunc) send_eos_async, helper, NULL);
}

/* Called with lock held. TRUE iff all contexts have a
 * pending (or delivered) async eos event */
static gboolean
all_contexts_are_async_eos (GstSplitMuxSink * splitmux)
{
  gboolean ret = TRUE;
  GList *item;

  for (item = splitmux->contexts; item; item = item->next) {
    MqStreamCtx *ctx = item->data;
    ret &= ctx->out_eos_async_done;
  }
  return ret;
}

/* Called with splitmux lock held to check if this output
 * context needs to sleep to wait for the release of the
 * next GOP, or to send EOS to close out the current file
 */
static void
complete_or_wait_on_out (GstSplitMuxSink * splitmux, MqStreamCtx * ctx)
{
  if (ctx->caps_change)
    return;

  do {
    /* When first starting up, the reference stream has to output
     * the first buffer to prepare the muxer and sink */
    gboolean can_output = (ctx->is_reference || splitmux->ready_for_output);
    GstClockTimeDiff my_max_out_running_time = splitmux->max_out_running_time;

    if (!(splitmux->max_out_running_time == 0 ||
            splitmux->max_out_running_time == GST_CLOCK_STIME_NONE ||
            splitmux->alignment_threshold == 0 ||
            splitmux->max_out_running_time < splitmux->alignment_threshold)) {
      my_max_out_running_time -= splitmux->alignment_threshold;
      GST_LOG_OBJECT (ctx->srcpad,
          "Max out running time currently %" GST_STIME_FORMAT
          ", with threshold applied it is %" GST_STIME_FORMAT,
          GST_STIME_ARGS (splitmux->max_out_running_time),
          GST_STIME_ARGS (my_max_out_running_time));
    }

    if (ctx->flushing
        || splitmux->output_state == SPLITMUX_OUTPUT_STATE_STOPPED)
      return;

    GST_LOG_OBJECT (ctx->srcpad,
        "Checking running time %" GST_STIME_FORMAT " against max %"
        GST_STIME_FORMAT, GST_STIME_ARGS (ctx->out_running_time),
        GST_STIME_ARGS (my_max_out_running_time));

    if (can_output) {
      if (splitmux->max_out_running_time == GST_CLOCK_STIME_NONE ||
          ctx->out_running_time < my_max_out_running_time) {
        return;
      }

      switch (splitmux->output_state) {
        case SPLITMUX_OUTPUT_STATE_OUTPUT_GOP:
          /* We only get here if we've finished outputting a GOP and need to know
           * what to do next */
          splitmux->output_state = SPLITMUX_OUTPUT_STATE_AWAITING_COMMAND;
          GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);
          continue;

        case SPLITMUX_OUTPUT_STATE_ENDING_FILE:
          /* We've reached the max out running_time to get here, so end this file now */
          if (ctx->out_eos == FALSE) {
            if (splitmux->async_finalize) {
              /* We must set EOS asynchronously at this point. We cannot defer
               * it, because we need all contexts to wake up, for the
               * reference context to eventually give us something at
               * START_NEXT_FILE. Otherwise, collectpads might choose another
               * context to give us the first buffer, and format-location-full
               * will not contain a valid sample. */
              g_object_set_qdata ((GObject *) splitmux->sink, EOS_FROM_US,
                  GINT_TO_POINTER (1));
              eos_context_async (ctx, splitmux);
              if (all_contexts_are_async_eos (splitmux)) {
                GST_INFO_OBJECT (splitmux,
                    "All contexts are async_eos. Moving to the next file.");
                /* We can start the next file once we've asked each pad to go EOS */
                splitmux->output_state = SPLITMUX_OUTPUT_STATE_START_NEXT_FILE;
                GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);
                continue;
              }
            } else {
              send_eos (splitmux, ctx);
              continue;
            }
          } else {
            GST_INFO_OBJECT (splitmux,
                "At end-of-file state, but context %p is already EOS", ctx);
          }
          break;
        case SPLITMUX_OUTPUT_STATE_START_NEXT_FILE:
          if (ctx->is_reference) {
            /* Special handling on the reference ctx to start new fragments
             * and collect commands from the command queue */
            /* drops the splitmux lock briefly: */
            /* We must have reference ctx in order for format-location-full to
             * have a sample */
            start_next_fragment (splitmux, ctx);
            continue;
          }
          break;
        case SPLITMUX_OUTPUT_STATE_AWAITING_COMMAND:{
          do {
            SplitMuxOutputCommand *cmd =
                g_queue_pop_tail (&splitmux->out_cmd_q);
            if (cmd != NULL) {
              /* If we pop the last command, we need to make our queues bigger */
              if (g_queue_get_length (&splitmux->out_cmd_q) == 0)
                grow_blocked_queues (splitmux);

              if (cmd->start_new_fragment) {
                GST_DEBUG_OBJECT (splitmux, "Got cmd to start new fragment");
                splitmux->output_state = SPLITMUX_OUTPUT_STATE_ENDING_FILE;
              } else {
                GST_DEBUG_OBJECT (splitmux,
                    "Got new output cmd for time %" GST_STIME_FORMAT,
                    GST_STIME_ARGS (cmd->max_output_ts));

                /* Extend the output range immediately */
                splitmux->max_out_running_time = cmd->max_output_ts;
                splitmux->output_state = SPLITMUX_OUTPUT_STATE_OUTPUT_GOP;
              }
              GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);

              out_cmd_buf_free (cmd);
              break;
            } else {
              GST_SPLITMUX_WAIT_OUTPUT (splitmux);
            }
          } while (splitmux->output_state ==
              SPLITMUX_OUTPUT_STATE_AWAITING_COMMAND);
          /* loop and re-check the state */
          continue;
        }
        case SPLITMUX_OUTPUT_STATE_STOPPED:
          return;
      }
    } else {
      GST_LOG_OBJECT (ctx->srcpad, "Not yet ready for output");
    }

    GST_INFO_OBJECT (ctx->srcpad,
        "Sleeping for running time %"
        GST_STIME_FORMAT " (max %" GST_STIME_FORMAT ") or state change.",
        GST_STIME_ARGS (ctx->out_running_time),
        GST_STIME_ARGS (splitmux->max_out_running_time));
    GST_SPLITMUX_WAIT_OUTPUT (splitmux);
    GST_INFO_OBJECT (ctx->srcpad,
        "Woken for new max running time %" GST_STIME_FORMAT,
        GST_STIME_ARGS (splitmux->max_out_running_time));
  }
  while (1);
}

static GstClockTime
calculate_next_max_timecode (GstSplitMuxSink * splitmux,
    const GstVideoTimeCode * cur_tc)
{
  GstVideoTimeCode *target_tc;
  GstVideoTimeCodeInterval *tc_inter;
  GstClockTime cur_tc_time, target_tc_time, next_max_tc_time;

  if (cur_tc == NULL || splitmux->threshold_timecode_str == NULL)
    return GST_CLOCK_TIME_NONE;

  tc_inter =
      gst_video_time_code_interval_new_from_string
      (splitmux->threshold_timecode_str);
  target_tc = gst_video_time_code_add_interval (cur_tc, tc_inter);
  gst_video_time_code_interval_free (tc_inter);

  /* Convert to ns */
  target_tc_time = gst_video_time_code_nsec_since_daily_jam (target_tc);
  cur_tc_time = gst_video_time_code_nsec_since_daily_jam (cur_tc);

  /* Add fragment_start_time, accounting for wraparound */
  if (target_tc_time >= cur_tc_time) {
    next_max_tc_time =
        target_tc_time - cur_tc_time + splitmux->fragment_start_time;
  } else {
    GstClockTime day_in_ns = 24 * 60 * 60 * GST_SECOND;

    if ((cur_tc->config.flags & GST_VIDEO_TIME_CODE_FLAGS_DROP_FRAME) &&
        (cur_tc->config.fps_d == 1001)) {
      /* Checking fps_d is probably unneeded, but better safe than sorry
       * (e.g. someone accidentally set a flag) */
      GstVideoTimeCode *tc_for_offset;

      /* Here, the duration of the 24:00:00;00 timecode isn't exactly one day,
       * but slightly less. Calculate that duration from a fake timecode. The
       * problem is that 24:00:00;00 isn't a valid timecode, so the workaround
       * is to add one frame to 23:59:59;29 */
      tc_for_offset =
          gst_video_time_code_new (cur_tc->config.fps_n, cur_tc->config.fps_d,
          NULL, cur_tc->config.flags, 23, 59, 59,
          cur_tc->config.fps_n / cur_tc->config.fps_d, 0);
      day_in_ns =
          gst_video_time_code_nsec_since_daily_jam (tc_for_offset) +
          gst_util_uint64_scale (GST_SECOND, cur_tc->config.fps_d,
          cur_tc->config.fps_n);
      gst_video_time_code_free (tc_for_offset);
    }
    next_max_tc_time =
        day_in_ns - cur_tc_time + target_tc_time +
        splitmux->fragment_start_time;
  }

  GST_INFO_OBJECT (splitmux, "Next max TC time: %" GST_TIME_FORMAT
      " from ref TC: %" GST_TIME_FORMAT, GST_TIME_ARGS (next_max_tc_time),
      GST_TIME_ARGS (cur_tc_time));
  gst_video_time_code_free (target_tc);

  return next_max_tc_time;
}

static gboolean
request_next_keyframe (GstSplitMuxSink * splitmux, GstBuffer * buffer)
{
  GstEvent *ev;
  GstClockTime target_time;
  gboolean timecode_based = FALSE;

  splitmux->next_max_tc_time = GST_CLOCK_TIME_NONE;
  if (splitmux->threshold_timecode_str) {
    GstVideoTimeCodeMeta *tc_meta;

    if (buffer != NULL) {
      tc_meta = gst_buffer_get_video_time_code_meta (buffer);
      if (tc_meta) {
        splitmux->next_max_tc_time =
            calculate_next_max_timecode (splitmux, &tc_meta->tc);
        timecode_based = (splitmux->next_max_tc_time != GST_CLOCK_TIME_NONE);
      }
    } else {
      /* This can happen in the presence of GAP events that trigger
       * a new fragment start */
      GST_WARNING_OBJECT (splitmux,
          "No buffer available to calculate next timecode");
    }
  }

  if (splitmux->send_keyframe_requests == FALSE
      || (splitmux->threshold_time == 0 && !timecode_based)
      || splitmux->threshold_bytes != 0)
    return TRUE;

  if (timecode_based) {
    /* We might have rounding errors: aim slightly earlier */
    target_time = splitmux->next_max_tc_time - 5 * GST_USECOND;
  } else {
    target_time = splitmux->fragment_start_time + splitmux->threshold_time;
  }
  ev = gst_video_event_new_upstream_force_key_unit (target_time, TRUE, 0);
  GST_INFO_OBJECT (splitmux, "Requesting next keyframe at %" GST_TIME_FORMAT,
      GST_TIME_ARGS (target_time));
  return gst_pad_push_event (splitmux->reference_ctx->sinkpad, ev);
}

static GstPadProbeReturn
handle_mq_output (GstPad * pad, GstPadProbeInfo * info, MqStreamCtx * ctx)
{
  GstSplitMuxSink *splitmux = ctx->splitmux;
  MqStreamBuf *buf_info = NULL;

  GST_LOG_OBJECT (pad, "Fired probe type 0x%x", info->type);

  /* FIXME: Handle buffer lists, until then make it clear they won't work */
  if (info->type & GST_PAD_PROBE_TYPE_BUFFER_LIST) {
    g_warning ("Buffer list handling not implemented");
    return GST_PAD_PROBE_DROP;
  }
  if (info->type & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM ||
      info->type & GST_PAD_PROBE_TYPE_EVENT_FLUSH) {
    GstEvent *event = gst_pad_probe_info_get_event (info);
    gboolean locked = FALSE;

    GST_LOG_OBJECT (pad, "Event %" GST_PTR_FORMAT, event);

    switch (GST_EVENT_TYPE (event)) {
      case GST_EVENT_SEGMENT:
        gst_event_copy_segment (event, &ctx->out_segment);
        break;
      case GST_EVENT_FLUSH_STOP:
        GST_SPLITMUX_LOCK (splitmux);
        locked = TRUE;
        gst_segment_init (&ctx->out_segment, GST_FORMAT_UNDEFINED);
        g_queue_foreach (&ctx->queued_bufs, (GFunc) mq_stream_buf_free, NULL);
        g_queue_clear (&ctx->queued_bufs);
        ctx->flushing = FALSE;
        break;
      case GST_EVENT_FLUSH_START:
        GST_SPLITMUX_LOCK (splitmux);
        locked = TRUE;
        GST_LOG_OBJECT (pad, "Flush start");
        ctx->flushing = TRUE;
        GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);
        break;
      case GST_EVENT_EOS:
        GST_SPLITMUX_LOCK (splitmux);
        locked = TRUE;
        if (splitmux->output_state == SPLITMUX_OUTPUT_STATE_STOPPED)
          goto beach;
        ctx->out_eos = TRUE;
        GST_INFO_OBJECT (splitmux,
            "Have EOS event at pad %" GST_PTR_FORMAT " ctx %p", pad, ctx);
        break;
      case GST_EVENT_GAP:{
        GstClockTime gap_ts;
        GstClockTimeDiff rtime;

        gst_event_parse_gap (event, &gap_ts, NULL);
        if (gap_ts == GST_CLOCK_TIME_NONE)
          break;

        GST_SPLITMUX_LOCK (splitmux);
        locked = TRUE;

        if (splitmux->output_state == SPLITMUX_OUTPUT_STATE_STOPPED)
          goto beach;

        /* When we get a gap event on the
         * reference stream and we're trying to open a
         * new file, we need to store it until we get
         * the buffer afterwards
         */
        if (ctx->is_reference &&
            (splitmux->output_state != SPLITMUX_OUTPUT_STATE_OUTPUT_GOP)) {
          GST_DEBUG_OBJECT (pad, "Storing GAP event until buffer arrives");
          gst_event_replace (&ctx->pending_gap, event);
          GST_SPLITMUX_UNLOCK (splitmux);
          return GST_PAD_PROBE_HANDLED;
        }

        rtime = my_segment_to_running_time (&ctx->out_segment, gap_ts);

        GST_LOG_OBJECT (pad, "Have GAP w/ ts %" GST_STIME_FORMAT,
            GST_STIME_ARGS (rtime));

        if (rtime != GST_CLOCK_STIME_NONE) {
          ctx->out_running_time = rtime;
          complete_or_wait_on_out (splitmux, ctx);
        }
        break;
      }
      case GST_EVENT_CUSTOM_DOWNSTREAM:{
        const GstStructure *s;
        GstClockTimeDiff ts = 0;

        s = gst_event_get_structure (event);
        if (!gst_structure_has_name (s, "splitmuxsink-unblock"))
          break;

        gst_structure_get_int64 (s, "timestamp", &ts);

        GST_SPLITMUX_LOCK (splitmux);
        locked = TRUE;

        if (splitmux->output_state == SPLITMUX_OUTPUT_STATE_STOPPED)
          goto beach;
        ctx->out_running_time = ts;
        if (!ctx->is_reference)
          complete_or_wait_on_out (splitmux, ctx);
        GST_SPLITMUX_UNLOCK (splitmux);
        return GST_PAD_PROBE_DROP;
      }
      case GST_EVENT_CAPS:{
        GstPad *peer;

        if (!ctx->is_reference)
          break;

        peer = gst_pad_get_peer (pad);
        if (peer) {
          gboolean ok = gst_pad_send_event (peer, gst_event_ref (event));

          gst_object_unref (peer);

          if (ok)
            break;

        } else {
          break;
        }
        /* This is in the case the muxer doesn't allow this change of caps */
        GST_SPLITMUX_LOCK (splitmux);
        locked = TRUE;
        ctx->caps_change = TRUE;

        if (splitmux->output_state != SPLITMUX_OUTPUT_STATE_START_NEXT_FILE) {
          GST_DEBUG_OBJECT (splitmux,
              "New caps were not accepted. Switching output file");
          if (ctx->out_eos == FALSE) {
            splitmux->output_state = SPLITMUX_OUTPUT_STATE_ENDING_FILE;
            GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);
          }
        }

        /* Lets it fall through, if it fails again, then the muxer just can't
         * support this format, but at least we have a closed file.
         */
        break;
      }
      default:
        break;
    }

    /* We need to make sure events aren't passed
     * until the muxer / sink are ready for it */
    if (!locked)
      GST_SPLITMUX_LOCK (splitmux);
    if (!ctx->is_reference)
      complete_or_wait_on_out (splitmux, ctx);
    GST_SPLITMUX_UNLOCK (splitmux);

    /* Don't try to forward sticky events before the next buffer is there
     * because it would cause a new file to be created without the first
     * buffer being available.
     */
    if (ctx->caps_change && GST_EVENT_IS_STICKY (event)) {
      gst_event_unref (event);
      return GST_PAD_PROBE_HANDLED;
    } else
      return GST_PAD_PROBE_PASS;
  }

  /* Allow everything through until the configured next stopping point */
  GST_SPLITMUX_LOCK (splitmux);

  buf_info = g_queue_pop_tail (&ctx->queued_bufs);
  if (buf_info == NULL)
    /* Can only happen due to a poorly timed flush */
    goto beach;

  /* If we have popped a keyframe, decrement the queued_gop count */
  if (buf_info->keyframe && splitmux->queued_keyframes > 0)
    splitmux->queued_keyframes--;

  ctx->out_running_time = buf_info->run_ts;
  ctx->cur_out_buffer = gst_pad_probe_info_get_buffer (info);

  GST_LOG_OBJECT (splitmux,
      "Pad %" GST_PTR_FORMAT " buffer with run TS %" GST_STIME_FORMAT
      " size %" G_GUINT64_FORMAT,
      pad, GST_STIME_ARGS (ctx->out_running_time), buf_info->buf_size);

  ctx->caps_change = FALSE;

  complete_or_wait_on_out (splitmux, ctx);

  splitmux->muxed_out_bytes += buf_info->buf_size;

#ifndef GST_DISABLE_GST_DEBUG
  {
    GstBuffer *buf = gst_pad_probe_info_get_buffer (info);
    GST_LOG_OBJECT (pad, "Returning to pass buffer %" GST_PTR_FORMAT
        " run ts %" GST_STIME_FORMAT, buf,
        GST_STIME_ARGS (ctx->out_running_time));
  }
#endif

  ctx->cur_out_buffer = NULL;
  GST_SPLITMUX_UNLOCK (splitmux);

  /* pending_gap is protected by the STREAM lock */
  if (ctx->pending_gap) {
    /* If we previously stored a gap event, send it now */
    GstPad *peer = gst_pad_get_peer (ctx->srcpad);

    GST_DEBUG_OBJECT (splitmux,
        "Pad %" GST_PTR_FORMAT " sending pending GAP event", ctx->srcpad);

    gst_pad_send_event (peer, ctx->pending_gap);
    ctx->pending_gap = NULL;

    gst_object_unref (peer);
  }

  mq_stream_buf_free (buf_info);

  return GST_PAD_PROBE_PASS;

beach:
  GST_SPLITMUX_UNLOCK (splitmux);
  return GST_PAD_PROBE_DROP;
}

static gboolean
resend_sticky (GstPad * pad, GstEvent ** event, GstPad * peer)
{
  return gst_pad_send_event (peer, gst_event_ref (*event));
}

static void
unlock_context (MqStreamCtx * ctx, GstSplitMuxSink * splitmux)
{
  if (ctx->fragment_block_id > 0) {
    gst_pad_remove_probe (ctx->srcpad, ctx->fragment_block_id);
    ctx->fragment_block_id = 0;
  }
}

static void
restart_context (MqStreamCtx * ctx, GstSplitMuxSink * splitmux)
{
  GstPad *peer = gst_pad_get_peer (ctx->srcpad);

  gst_pad_sticky_events_foreach (ctx->srcpad,
      (GstPadStickyEventsForeachFunction) (resend_sticky), peer);

  /* Clear EOS flag if not actually EOS */
  ctx->out_eos = GST_PAD_IS_EOS (ctx->srcpad);
  ctx->out_eos_async_done = ctx->out_eos;

  gst_object_unref (peer);
}

static void
relink_context (MqStreamCtx * ctx, GstSplitMuxSink * splitmux)
{
  GstPad *sinkpad, *srcpad, *newpad;
  GstPadTemplate *templ;

  srcpad = ctx->srcpad;
  sinkpad = gst_pad_get_peer (srcpad);

  templ = sinkpad->padtemplate;
  newpad =
      gst_element_request_pad (splitmux->muxer, templ,
      GST_PAD_NAME (sinkpad), NULL);

  GST_DEBUG_OBJECT (splitmux, "Relinking ctx %p to pad %" GST_PTR_FORMAT, ctx,
      newpad);
  if (!gst_pad_unlink (srcpad, sinkpad)) {
    gst_object_unref (sinkpad);
    goto fail;
  }
  if (gst_pad_link_full (srcpad, newpad,
          GST_PAD_LINK_CHECK_NO_RECONFIGURE) != GST_PAD_LINK_OK) {
    gst_element_release_request_pad (splitmux->muxer, newpad);
    gst_object_unref (sinkpad);
    gst_object_unref (newpad);
    goto fail;
  }
  gst_object_unref (newpad);
  gst_object_unref (sinkpad);
  return;

fail:
  GST_ELEMENT_ERROR (splitmux, RESOURCE, SETTINGS,
      ("Could not create the new muxer/sink"), NULL);
}

static GstPadProbeReturn
_block_pad (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  return GST_PAD_PROBE_OK;
}

static void
block_context (MqStreamCtx * ctx, GstSplitMuxSink * splitmux)
{
  ctx->fragment_block_id =
      gst_pad_add_probe (ctx->srcpad, GST_PAD_PROBE_TYPE_BLOCK, _block_pad,
      NULL, NULL);
}

static gboolean
_set_property_from_structure (GQuark field_id, const GValue * value,
    gpointer user_data)
{
  const gchar *property_name = g_quark_to_string (field_id);
  GObject *element = G_OBJECT (user_data);

  g_object_set_property (element, property_name, value);

  return TRUE;
}

static void
_lock_and_set_to_null (GstElement * element, GstSplitMuxSink * splitmux)
{
  gst_element_set_locked_state (element, TRUE);
  gst_element_set_state (element, GST_STATE_NULL);
  GST_LOG_OBJECT (splitmux, "Removing old element %" GST_PTR_FORMAT, element);
  gst_bin_remove (GST_BIN (splitmux), element);
}


static void
_send_event (const GValue * value, gpointer user_data)
{
  GstPad *pad = g_value_get_object (value);
  GstEvent *ev = user_data;

  gst_pad_send_event (pad, gst_event_ref (ev));
}

/* Called with lock held when a fragment
 * reaches EOS and it is time to restart
 * a new fragment
 */
static void
start_next_fragment (GstSplitMuxSink * splitmux, MqStreamCtx * ctx)
{
  GstElement *muxer, *sink;

  g_assert (ctx->is_reference);

  /* 1 change to new file */
  splitmux->switching_fragment = TRUE;

  /* We need to drop the splitmux lock to acquire the state lock
   * here and ensure there's no racy state change going on elsewhere */
  muxer = gst_object_ref (splitmux->muxer);
  sink = gst_object_ref (splitmux->active_sink);

  GST_SPLITMUX_UNLOCK (splitmux);
  GST_STATE_LOCK (splitmux);

  if (splitmux->async_finalize) {
    if (splitmux->muxed_out_bytes > 0
        || splitmux->fragment_id != splitmux->start_index) {
      gchar *newname;
      GstElement *new_sink, *new_muxer;

      GST_DEBUG_OBJECT (splitmux, "Starting fragment %u",
          splitmux->fragment_id);
      g_list_foreach (splitmux->contexts, (GFunc) block_context, splitmux);
      newname = g_strdup_printf ("sink_%u", splitmux->fragment_id);
      GST_SPLITMUX_LOCK (splitmux);
      if ((splitmux->sink =
              create_element (splitmux, splitmux->sink_factory, newname,
                  TRUE)) == NULL)
        goto fail;
      if (splitmux->sink_properties)
        gst_structure_foreach (splitmux->sink_properties,
            _set_property_from_structure, splitmux->sink);
      splitmux->active_sink = splitmux->sink;
      g_signal_emit (splitmux, signals[SIGNAL_SINK_ADDED], 0, splitmux->sink);
      g_free (newname);
      newname = g_strdup_printf ("muxer_%u", splitmux->fragment_id);
      if ((splitmux->muxer =
              create_element (splitmux, splitmux->muxer_factory, newname,
                  TRUE)) == NULL)
        goto fail;
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (splitmux->sink),
              "async") != NULL) {
        /* async child elements are causing state change races and weird
         * failures, so let's try and turn that off */
        g_object_set (splitmux->sink, "async", FALSE, NULL);
      }
      if (splitmux->muxer_properties)
        gst_structure_foreach (splitmux->muxer_properties,
            _set_property_from_structure, splitmux->muxer);
      g_signal_emit (splitmux, signals[SIGNAL_MUXER_ADDED], 0, splitmux->muxer);
      g_free (newname);
      new_sink = splitmux->sink;
      new_muxer = splitmux->muxer;
      GST_SPLITMUX_UNLOCK (splitmux);
      g_list_foreach (splitmux->contexts, (GFunc) relink_context, splitmux);
      gst_element_link (new_muxer, new_sink);

      if (g_object_get_qdata ((GObject *) sink, EOS_FROM_US)) {
        if (GPOINTER_TO_INT (g_object_get_qdata ((GObject *) sink,
                    EOS_FROM_US)) == 2) {
          _lock_and_set_to_null (muxer, splitmux);
          _lock_and_set_to_null (sink, splitmux);
        } else {
          g_object_set_qdata ((GObject *) sink, EOS_FROM_US,
              GINT_TO_POINTER (2));
        }
      }
      gst_object_unref (muxer);
      gst_object_unref (sink);
      muxer = new_muxer;
      sink = new_sink;
      gst_object_ref (muxer);
      gst_object_ref (sink);
    }
  } else {

    gst_element_set_locked_state (muxer, TRUE);
    gst_element_set_locked_state (sink, TRUE);
    gst_element_set_state (sink, GST_STATE_NULL);

    if (splitmux->reset_muxer) {
      gst_element_set_state (muxer, GST_STATE_NULL);
    } else {
      GstIterator *it = gst_element_iterate_sink_pads (muxer);
      GstEvent *ev;
      guint32 seqnum;

      ev = gst_event_new_flush_start ();
      seqnum = gst_event_get_seqnum (ev);
      while (gst_iterator_foreach (it, _send_event, ev) == GST_ITERATOR_RESYNC);
      gst_event_unref (ev);

      gst_iterator_resync (it);

      ev = gst_event_new_flush_stop (TRUE);
      gst_event_set_seqnum (ev, seqnum);
      while (gst_iterator_foreach (it, _send_event, ev) == GST_ITERATOR_RESYNC);
      gst_event_unref (ev);

      gst_iterator_free (it);
    }
  }

  GST_SPLITMUX_LOCK (splitmux);
  if (splitmux->muxed_out_bytes > 0
      || splitmux->fragment_id == splitmux->start_index)
    set_next_filename (splitmux, ctx);
  splitmux->muxed_out_bytes = 0;
  GST_SPLITMUX_UNLOCK (splitmux);

  gst_element_set_state (sink, GST_STATE_TARGET (splitmux));
  gst_element_set_state (muxer, GST_STATE_TARGET (splitmux));
  gst_element_set_locked_state (muxer, FALSE);
  gst_element_set_locked_state (sink, FALSE);

  gst_object_unref (sink);
  gst_object_unref (muxer);

  GST_SPLITMUX_LOCK (splitmux);
  GST_STATE_UNLOCK (splitmux);
  splitmux->switching_fragment = FALSE;
  do_async_done (splitmux);

  splitmux->ready_for_output = TRUE;

  g_list_foreach (splitmux->contexts, (GFunc) unlock_context, splitmux);
  g_list_foreach (splitmux->contexts, (GFunc) restart_context, splitmux);

  send_fragment_opened_closed_msg (splitmux, TRUE, sink);

  /* FIXME: Is this always the correct next state? */
  GST_LOG_OBJECT (splitmux, "Resetting state to AWAITING_COMMAND");
  splitmux->output_state = SPLITMUX_OUTPUT_STATE_AWAITING_COMMAND;
  GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);
  return;

fail:
  GST_STATE_UNLOCK (splitmux);
  GST_ELEMENT_ERROR (splitmux, RESOURCE, SETTINGS,
      ("Could not create the new muxer/sink"), NULL);
}

static void
bus_handler (GstBin * bin, GstMessage * message)
{
  GstSplitMuxSink *splitmux = GST_SPLITMUX_SINK (bin);

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:{
      /* If the state is draining out the current file, drop this EOS */
      GstElement *sink;

      sink = GST_ELEMENT (GST_MESSAGE_SRC (message));
      GST_SPLITMUX_LOCK (splitmux);

      send_fragment_opened_closed_msg (splitmux, FALSE, sink);

      if (splitmux->async_finalize) {

        if (g_object_get_qdata ((GObject *) sink, EOS_FROM_US)) {
          if (GPOINTER_TO_INT (g_object_get_qdata ((GObject *) sink,
                      EOS_FROM_US)) == 2) {
            GstElement *muxer;
            GstPad *sinksink, *muxersrc;

            sinksink = gst_element_get_static_pad (sink, "sink");
            muxersrc = gst_pad_get_peer (sinksink);
            muxer = gst_pad_get_parent_element (muxersrc);
            gst_object_unref (sinksink);
            gst_object_unref (muxersrc);

            gst_element_call_async (muxer,
                (GstElementCallAsyncFunc) _lock_and_set_to_null,
                gst_object_ref (splitmux), gst_object_unref);
            gst_element_call_async (sink,
                (GstElementCallAsyncFunc) _lock_and_set_to_null,
                gst_object_ref (splitmux), gst_object_unref);
            gst_object_unref (muxer);
          } else {
            g_object_set_qdata ((GObject *) sink, EOS_FROM_US,
                GINT_TO_POINTER (2));
          }
          GST_DEBUG_OBJECT (splitmux,
              "Caught async EOS from previous muxer+sink. Dropping.");
          /* We forward the EOS so that it gets aggregated as normal. If the sink
           * finishes and is removed before the end, it will be de-aggregated */
          gst_message_unref (message);
          GST_SPLITMUX_UNLOCK (splitmux);
          return;
        }
      } else if (splitmux->output_state == SPLITMUX_OUTPUT_STATE_ENDING_FILE) {
        GST_DEBUG_OBJECT (splitmux, "Caught EOS at end of fragment, dropping");
        splitmux->output_state = SPLITMUX_OUTPUT_STATE_START_NEXT_FILE;
        GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);

        gst_message_unref (message);
        GST_SPLITMUX_UNLOCK (splitmux);
        return;
      } else {
        GST_DEBUG_OBJECT (splitmux,
            "Passing EOS message. Output state %d max_out_running_time %"
            GST_STIME_FORMAT, splitmux->output_state,
            GST_STIME_ARGS (splitmux->max_out_running_time));
      }
      GST_SPLITMUX_UNLOCK (splitmux);
      break;
    }
    case GST_MESSAGE_ASYNC_START:
    case GST_MESSAGE_ASYNC_DONE:
      /* Ignore state changes from our children while switching */
      GST_SPLITMUX_LOCK (splitmux);
      if (splitmux->switching_fragment) {
        if (GST_MESSAGE_SRC (message) == (GstObject *) splitmux->active_sink
            || GST_MESSAGE_SRC (message) == (GstObject *) splitmux->muxer) {
          GST_LOG_OBJECT (splitmux,
              "Ignoring state change from child %" GST_PTR_FORMAT
              " while switching", GST_MESSAGE_SRC (message));
          gst_message_unref (message);
          GST_SPLITMUX_UNLOCK (splitmux);
          return;
        }
      }
      GST_SPLITMUX_UNLOCK (splitmux);
      break;
    case GST_MESSAGE_WARNING:
    {
      GError *gerror = NULL;

      gst_message_parse_warning (message, &gerror, NULL);

      if (g_error_matches (gerror, GST_STREAM_ERROR, GST_STREAM_ERROR_FORMAT)) {
        GList *item;
        gboolean caps_change = FALSE;

        GST_SPLITMUX_LOCK (splitmux);

        for (item = splitmux->contexts; item; item = item->next) {
          MqStreamCtx *ctx = item->data;

          if (ctx->caps_change) {
            caps_change = TRUE;
            break;
          }
        }

        GST_SPLITMUX_UNLOCK (splitmux);

        if (caps_change) {
          GST_LOG_OBJECT (splitmux,
              "Ignoring warning change from child %" GST_PTR_FORMAT
              " while switching caps", GST_MESSAGE_SRC (message));
          gst_message_unref (message);
          return;
        }
      }
      break;
    }
    default:
      break;
  }

  GST_BIN_CLASS (parent_class)->handle_message (bin, message);
}

static void
ctx_set_unblock (MqStreamCtx * ctx)
{
  ctx->need_unblock = TRUE;
}

static gboolean
need_new_fragment (GstSplitMuxSink * splitmux,
    GstClockTime queued_time, GstClockTime queued_gop_time,
    guint64 queued_bytes)
{
  guint64 thresh_bytes;
  GstClockTime thresh_time;
  gboolean check_robust_muxing;
  GstClockTime time_to_split = GST_CLOCK_TIME_NONE;
  GstClockTime *ptr_to_time;

  GST_OBJECT_LOCK (splitmux);
  thresh_bytes = splitmux->threshold_bytes;
  thresh_time = splitmux->threshold_time;
  ptr_to_time = (GstClockTime *)
      gst_queue_array_peek_head_struct (splitmux->times_to_split);
  if (ptr_to_time)
    time_to_split = *ptr_to_time;
  check_robust_muxing = splitmux->use_robust_muxing
      && splitmux->muxer_has_reserved_props;
  GST_OBJECT_UNLOCK (splitmux);

  /* Have we muxed anything into the new file at all? */
  if (splitmux->fragment_total_bytes <= 0)
    return FALSE;

  /* User told us to split now */
  if (g_atomic_int_get (&(splitmux->do_split_next_gop)) == TRUE) {
    GST_TRACE_OBJECT (splitmux, "Forcing because split_next_gop is set");
    return TRUE;
  }

  /* User told us to split at this running time */
  if (splitmux->reference_ctx->in_running_time > time_to_split) {
    GST_OBJECT_LOCK (splitmux);
    /* Dequeue running time */
    gst_queue_array_pop_head_struct (splitmux->times_to_split);
    /* Empty any running times after this that are past now */
    ptr_to_time = gst_queue_array_peek_head_struct (splitmux->times_to_split);
    while (ptr_to_time) {
      time_to_split = *ptr_to_time;
      if (splitmux->reference_ctx->in_running_time <= time_to_split) {
        break;
      }
      gst_queue_array_pop_head_struct (splitmux->times_to_split);
      ptr_to_time = gst_queue_array_peek_head_struct (splitmux->times_to_split);
    }
    GST_OBJECT_UNLOCK (splitmux);
    return TRUE;
  }

  if (thresh_bytes > 0 && queued_bytes > thresh_bytes) {
    GST_TRACE_OBJECT (splitmux,
        "queued bytes %" G_GUINT64_FORMAT " overruns byte limit", queued_bytes);
    return TRUE;                /* Would overrun byte limit */
  }

  if (thresh_time > 0 && queued_time > thresh_time) {
    GST_TRACE_OBJECT (splitmux,
        "queued time %" GST_STIME_FORMAT " overruns time limit",
        GST_STIME_ARGS (queued_time));
    return TRUE;                /* Would overrun time limit */
  }

  /* Timecode-based threshold accounts for possible rounding errors:
   * 5us should be bigger than all possible rounding errors but nowhere near
   * big enough to skip to another frame */
  if (splitmux->next_max_tc_time != GST_CLOCK_TIME_NONE &&
      splitmux->reference_ctx->in_running_time >
      splitmux->next_max_tc_time + 5 * GST_USECOND) {
    GST_TRACE_OBJECT (splitmux, "Splitting at timecode mark");
    return TRUE;                /* Timecode threshold */
  }

  if (check_robust_muxing) {
    GstClockTime mux_reserved_remain;

    g_object_get (splitmux->muxer,
        "reserved-duration-remaining", &mux_reserved_remain, NULL);

    GST_LOG_OBJECT (splitmux,
        "Muxer robust muxing report - %" G_GUINT64_FORMAT
        " remaining. New GOP would enqueue %" G_GUINT64_FORMAT,
        mux_reserved_remain, queued_gop_time);

    if (queued_gop_time >= mux_reserved_remain) {
      GST_INFO_OBJECT (splitmux,
          "File is about to run out of header room - %" G_GUINT64_FORMAT
          " remaining. New GOP would enqueue %" G_GUINT64_FORMAT
          ". Switching to new file", mux_reserved_remain, queued_gop_time);
      return TRUE;
    }
  }

  /* Continue and mux this GOP */
  return FALSE;
}

/* Called with splitmux lock held */
/* Called when entering ProcessingCompleteGop state
 * Assess if mq contents overflowed the current file
 *   -> If yes, need to switch to new file
 *   -> if no, set max_out_running_time to let this GOP in and
 *      go to COLLECTING_GOP_START state
 */
static void
handle_gathered_gop (GstSplitMuxSink * splitmux)
{
  guint64 queued_bytes;
  GstClockTimeDiff queued_time = 0;
  GstClockTimeDiff queued_gop_time = 0;
  GstClockTimeDiff new_out_ts = splitmux->reference_ctx->in_running_time;
  SplitMuxOutputCommand *cmd;

  /* Assess if the multiqueue contents overflowed the current file */
  /* When considering if a newly gathered GOP overflows
   * the time limit for the file, only consider the running time of the
   * reference stream. Other streams might have run ahead a little bit,
   * but extra pieces won't be released to the muxer beyond the reference
   * stream cut-off anyway - so it forms the limit. */
  queued_bytes = splitmux->fragment_total_bytes + splitmux->gop_total_bytes;
  queued_time = splitmux->reference_ctx->in_running_time;
  /* queued_gop_time tracks how much unwritten data there is waiting to
   * be written to this fragment including this GOP */
  if (splitmux->reference_ctx->out_running_time != GST_CLOCK_STIME_NONE)
    queued_gop_time =
        splitmux->reference_ctx->in_running_time -
        splitmux->reference_ctx->out_running_time;
  else
    queued_gop_time =
        splitmux->reference_ctx->in_running_time - splitmux->gop_start_time;

  GST_LOG_OBJECT (splitmux, " queued_bytes %" G_GUINT64_FORMAT, queued_bytes);

  g_assert (queued_gop_time >= 0);
  g_assert (queued_time >= splitmux->fragment_start_time);

  queued_time -= splitmux->fragment_start_time;
  if (queued_time < queued_gop_time)
    queued_gop_time = queued_time;

  /* Expand queued bytes estimate by muxer overhead */
  queued_bytes += (queued_bytes * splitmux->mux_overhead);

  GST_LOG_OBJECT (splitmux, "mq at TS %" GST_STIME_FORMAT
      " bytes %" G_GUINT64_FORMAT, GST_STIME_ARGS (queued_time), queued_bytes);
  if (splitmux->next_max_tc_time != GST_CLOCK_TIME_NONE) {
    GST_LOG_OBJECT (splitmux,
        "timecode mq TS %" GST_TIME_FORMAT " vs target %" GST_TIME_FORMAT,
        GST_TIME_ARGS (splitmux->reference_ctx->in_running_time),
        GST_TIME_ARGS (splitmux->next_max_tc_time + 5 * GST_USECOND));
  }

  /* Check for overrun - have we output at least one byte and overrun
   * either threshold? */
  if (need_new_fragment (splitmux, queued_time, queued_gop_time, queued_bytes)) {
    if (splitmux->async_finalize) {
      GstClockTime *sink_running_time = g_new (GstClockTime, 1);
      *sink_running_time = splitmux->reference_ctx->out_running_time;
      g_object_set_qdata_full (G_OBJECT (splitmux->sink),
          RUNNING_TIME, sink_running_time, g_free);
    }
    g_atomic_int_set (&(splitmux->do_split_next_gop), FALSE);
    /* Tell the output side to start a new fragment */
    GST_INFO_OBJECT (splitmux,
        "This GOP (dur %" GST_STIME_FORMAT
        ") would overflow the fragment, Sending start_new_fragment cmd",
        GST_STIME_ARGS (splitmux->reference_ctx->in_running_time -
            splitmux->gop_start_time));
    cmd = out_cmd_buf_new ();
    cmd->start_new_fragment = TRUE;
    g_queue_push_head (&splitmux->out_cmd_q, cmd);
    GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);

    new_out_ts = splitmux->reference_ctx->in_running_time;
    splitmux->fragment_start_time = splitmux->gop_start_time;
    splitmux->fragment_total_bytes = 0;

    if (request_next_keyframe (splitmux,
            splitmux->reference_ctx->prev_in_keyframe) == FALSE) {
      GST_WARNING_OBJECT (splitmux,
          "Could not request a keyframe. Files may not split at the exact location they should");
    }
    gst_buffer_replace (&splitmux->reference_ctx->prev_in_keyframe, NULL);
  }

  /* And set up to collect the next GOP */
  if (!splitmux->reference_ctx->in_eos) {
    splitmux->input_state = SPLITMUX_INPUT_STATE_COLLECTING_GOP_START;
    splitmux->gop_start_time = new_out_ts;
  } else {
    /* This is probably already the current state, but just in case: */
    splitmux->input_state = SPLITMUX_INPUT_STATE_FINISHING_UP;
    new_out_ts = GST_CLOCK_STIME_NONE;  /* EOS runs until forever */
  }

  /* And wake all input contexts to send a wake-up event */
  g_list_foreach (splitmux->contexts, (GFunc) ctx_set_unblock, NULL);
  GST_SPLITMUX_BROADCAST_INPUT (splitmux);

  /* Now either way - either there was no overflow, or we requested a new fragment: release this GOP */
  splitmux->fragment_total_bytes += splitmux->gop_total_bytes;

  if (splitmux->gop_total_bytes > 0) {
    GST_LOG_OBJECT (splitmux,
        "Releasing GOP to output. Bytes in fragment now %" G_GUINT64_FORMAT
        " time %" GST_STIME_FORMAT,
        splitmux->fragment_total_bytes, GST_STIME_ARGS (queued_time));

    /* Send this GOP to the output command queue */
    cmd = out_cmd_buf_new ();
    cmd->start_new_fragment = FALSE;
    cmd->max_output_ts = new_out_ts;
    GST_LOG_OBJECT (splitmux, "Sending GOP cmd to output for TS %"
        GST_STIME_FORMAT, GST_STIME_ARGS (new_out_ts));
    g_queue_push_head (&splitmux->out_cmd_q, cmd);

    GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);
  }

  splitmux->gop_total_bytes = 0;
}

/* Called with splitmux lock held */
/* Called from each input pad when it is has all the pieces
 * for a GOP or EOS, starting with the reference pad which has set the
 * splitmux->max_in_running_time
 */
static void
check_completed_gop (GstSplitMuxSink * splitmux, MqStreamCtx * ctx)
{
  GList *cur;
  GstEvent *event;

  /* On ENDING_FILE, the reference stream sends a command to start a new
   * fragment, then releases the GOP for output in the new fragment.
   *  If some streams received no buffer during the last GOP that overran,
   * because its next buffer has a timestamp bigger than
   * ctx->max_in_running_time, its queue is empty. In that case the only
   * way to wakeup the output thread is by injecting an event in the
   * queue. This usually happen with subtitle streams.
   * See https://bugzilla.gnome.org/show_bug.cgi?id=763711. */
  if (ctx->need_unblock) {
    GST_LOG_OBJECT (ctx->sinkpad, "Sending splitmuxsink-unblock event");
    event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM |
        GST_EVENT_TYPE_SERIALIZED,
        gst_structure_new ("splitmuxsink-unblock", "timestamp",
            G_TYPE_INT64, splitmux->max_in_running_time, NULL));

    GST_SPLITMUX_UNLOCK (splitmux);
    gst_pad_send_event (ctx->sinkpad, event);
    GST_SPLITMUX_LOCK (splitmux);

    ctx->need_unblock = FALSE;
    GST_SPLITMUX_BROADCAST_INPUT (splitmux);
    /* state may have changed while we were unlocked. Loop again if so */
    if (splitmux->input_state != SPLITMUX_INPUT_STATE_WAITING_GOP_COLLECT)
      return;
  }

  if (splitmux->input_state == SPLITMUX_INPUT_STATE_WAITING_GOP_COLLECT) {
    gboolean ready = TRUE;

    /* Iterate each pad, and check that the input running time is at least
     * up to the reference running time, and if so handle the collected GOP */
    GST_LOG_OBJECT (splitmux, "Checking GOP collected, Max in running time %"
        GST_STIME_FORMAT " ctx %p",
        GST_STIME_ARGS (splitmux->max_in_running_time), ctx);
    for (cur = g_list_first (splitmux->contexts); cur != NULL;
        cur = g_list_next (cur)) {
      MqStreamCtx *tmpctx = (MqStreamCtx *) (cur->data);

      GST_LOG_OBJECT (splitmux,
          "Context %p sink pad %" GST_PTR_FORMAT " @ TS %" GST_STIME_FORMAT
          " EOS %d", tmpctx, tmpctx->sinkpad,
          GST_STIME_ARGS (tmpctx->in_running_time), tmpctx->in_eos);

      if (splitmux->max_in_running_time != GST_CLOCK_STIME_NONE &&
          tmpctx->in_running_time < splitmux->max_in_running_time &&
          !tmpctx->in_eos) {
        GST_LOG_OBJECT (splitmux,
            "Context %p sink pad %" GST_PTR_FORMAT " not ready. We'll sleep",
            tmpctx, tmpctx->sinkpad);
        ready = FALSE;
        break;
      }
    }
    if (ready) {
      GST_DEBUG_OBJECT (splitmux,
          "Collected GOP is complete. Processing (ctx %p)", ctx);
      /* All pads have a complete GOP, release it into the multiqueue */
      handle_gathered_gop (splitmux);

      /* The user has requested a split, we can split now that the previous GOP
       * has been collected to the correct location */
      if (g_atomic_int_compare_and_exchange (&(splitmux->split_requested), TRUE,
              FALSE)) {
        g_atomic_int_set (&(splitmux->do_split_next_gop), TRUE);
      }
    }
  }

  /* If upstream reached EOS we are not expecting more data, no need to wait
   * here. */
  if (ctx->in_eos)
    return;

  /* Some pad is not yet ready, or GOP is being pushed
   * either way, sleep and wait to get woken */
  while (splitmux->input_state == SPLITMUX_INPUT_STATE_WAITING_GOP_COLLECT &&
      !ctx->flushing &&
      (ctx->in_running_time >= splitmux->max_in_running_time) &&
      (splitmux->max_in_running_time != GST_CLOCK_STIME_NONE)) {

    GST_LOG_OBJECT (splitmux, "Sleeping for GOP collection (ctx %p)", ctx);
    GST_SPLITMUX_WAIT_INPUT (splitmux);
    GST_LOG_OBJECT (splitmux, "Done waiting for complete GOP (ctx %p)", ctx);
  }
}

static GstPadProbeReturn
handle_mq_input (GstPad * pad, GstPadProbeInfo * info, MqStreamCtx * ctx)
{
  GstSplitMuxSink *splitmux = ctx->splitmux;
  GstBuffer *buf;
  MqStreamBuf *buf_info = NULL;
  GstClockTime ts;
  gboolean loop_again;
  gboolean keyframe = FALSE;

  GST_LOG_OBJECT (pad, "Fired probe type 0x%x", info->type);

  /* FIXME: Handle buffer lists, until then make it clear they won't work */
  if (info->type & GST_PAD_PROBE_TYPE_BUFFER_LIST) {
    g_warning ("Buffer list handling not implemented");
    return GST_PAD_PROBE_DROP;
  }
  if (info->type & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM ||
      info->type & GST_PAD_PROBE_TYPE_EVENT_FLUSH) {
    GstEvent *event = gst_pad_probe_info_get_event (info);

    GST_LOG_OBJECT (pad, "Event %" GST_PTR_FORMAT, event);

    switch (GST_EVENT_TYPE (event)) {
      case GST_EVENT_SEGMENT:
        gst_event_copy_segment (event, &ctx->in_segment);
        break;
      case GST_EVENT_FLUSH_STOP:
        GST_SPLITMUX_LOCK (splitmux);
        gst_segment_init (&ctx->in_segment, GST_FORMAT_UNDEFINED);
        ctx->in_eos = FALSE;
        ctx->in_running_time = GST_CLOCK_STIME_NONE;
        GST_SPLITMUX_UNLOCK (splitmux);
        break;
      case GST_EVENT_EOS:
        GST_SPLITMUX_LOCK (splitmux);
        ctx->in_eos = TRUE;

        if (splitmux->input_state == SPLITMUX_INPUT_STATE_STOPPED)
          goto beach;

        if (ctx->is_reference) {
          GST_INFO_OBJECT (splitmux, "Got Reference EOS. Finishing up");
          /* check_completed_gop will act as if this is a new keyframe with infinite timestamp */
          splitmux->input_state = SPLITMUX_INPUT_STATE_WAITING_GOP_COLLECT;
          /* Wake up other input pads to collect this GOP */
          GST_SPLITMUX_BROADCAST_INPUT (splitmux);
          check_completed_gop (splitmux, ctx);
        } else if (splitmux->input_state ==
            SPLITMUX_INPUT_STATE_WAITING_GOP_COLLECT) {
          /* If we are waiting for a GOP to be completed (ie, for aux
           * pads to catch up), then this pad is complete, so check
           * if the whole GOP is.
           */
          check_completed_gop (splitmux, ctx);
        }
        GST_SPLITMUX_UNLOCK (splitmux);
        break;
      case GST_EVENT_GAP:{
        GstClockTime gap_ts;
        GstClockTimeDiff rtime;

        gst_event_parse_gap (event, &gap_ts, NULL);
        if (gap_ts == GST_CLOCK_TIME_NONE)
          break;

        GST_SPLITMUX_LOCK (splitmux);

        if (splitmux->input_state == SPLITMUX_INPUT_STATE_STOPPED)
          goto beach;
        rtime = my_segment_to_running_time (&ctx->in_segment, gap_ts);

        GST_LOG_OBJECT (pad, "Have GAP w/ ts %" GST_STIME_FORMAT,
            GST_STIME_ARGS (rtime));

        if (ctx->is_reference
            && splitmux->fragment_start_time == GST_CLOCK_STIME_NONE) {
          splitmux->gop_start_time = splitmux->fragment_start_time = rtime;
          GST_LOG_OBJECT (splitmux, "Mux start time now %" GST_STIME_FORMAT,
              GST_STIME_ARGS (splitmux->fragment_start_time));
          /* Also take this as the first start time when starting up,
           * so that we start counting overflow from the first frame */
          if (!GST_CLOCK_STIME_IS_VALID (splitmux->max_in_running_time))
            splitmux->max_in_running_time = splitmux->fragment_start_time;
        }

        GST_SPLITMUX_UNLOCK (splitmux);
        break;
      }
      default:
        break;
    }
    return GST_PAD_PROBE_PASS;
  } else if (info->type & GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM) {
    switch (GST_QUERY_TYPE (GST_QUERY (info->data))) {
      case GST_QUERY_ALLOCATION:
        return GST_PAD_PROBE_DROP;
      default:
        return GST_PAD_PROBE_PASS;
    }
  }

  buf = gst_pad_probe_info_get_buffer (info);
  buf_info = mq_stream_buf_new ();

  if (GST_BUFFER_PTS_IS_VALID (buf))
    ts = GST_BUFFER_PTS (buf);
  else
    ts = GST_BUFFER_DTS (buf);

  GST_LOG_OBJECT (pad, "Buffer TS is %" GST_TIME_FORMAT, GST_TIME_ARGS (ts));

  GST_SPLITMUX_LOCK (splitmux);

  if (splitmux->input_state == SPLITMUX_INPUT_STATE_STOPPED)
    goto beach;

  /* If this buffer has a timestamp, advance the input timestamp of the
   * stream */
  if (GST_CLOCK_TIME_IS_VALID (ts)) {
    GstClockTimeDiff running_time =
        my_segment_to_running_time (&ctx->in_segment, ts);

    GST_LOG_OBJECT (pad, "Buffer running TS is %" GST_STIME_FORMAT,
        GST_STIME_ARGS (running_time));

    if (GST_CLOCK_STIME_IS_VALID (running_time)
        && running_time > ctx->in_running_time)
      ctx->in_running_time = running_time;
  }

  /* Try to make sure we have a valid running time */
  if (!GST_CLOCK_STIME_IS_VALID (ctx->in_running_time)) {
    ctx->in_running_time =
        my_segment_to_running_time (&ctx->in_segment, ctx->in_segment.start);
  }

  GST_LOG_OBJECT (pad, "in running time now %" GST_STIME_FORMAT,
      GST_STIME_ARGS (ctx->in_running_time));

  buf_info->run_ts = ctx->in_running_time;
  buf_info->buf_size = gst_buffer_get_size (buf);
  buf_info->duration = GST_BUFFER_DURATION (buf);

  /* initialize fragment_start_time */
  if (ctx->is_reference
      && splitmux->fragment_start_time == GST_CLOCK_STIME_NONE) {
    splitmux->gop_start_time = splitmux->fragment_start_time = buf_info->run_ts;
    GST_LOG_OBJECT (splitmux, "Mux start time now %" GST_STIME_FORMAT,
        GST_STIME_ARGS (splitmux->fragment_start_time));
    gst_buffer_replace (&ctx->prev_in_keyframe, buf);

    /* Also take this as the first start time when starting up,
     * so that we start counting overflow from the first frame */
    if (!GST_CLOCK_STIME_IS_VALID (splitmux->max_in_running_time))
      splitmux->max_in_running_time = splitmux->fragment_start_time;
    if (request_next_keyframe (splitmux, ctx->prev_in_keyframe) == FALSE) {
      GST_WARNING_OBJECT (splitmux,
          "Could not request a keyframe. Files may not split at the exact location they should");
    }
    gst_buffer_replace (&splitmux->reference_ctx->prev_in_keyframe, NULL);
  }

  GST_DEBUG_OBJECT (pad, "Buf TS %" GST_STIME_FORMAT
      " total GOP bytes %" G_GUINT64_FORMAT,
      GST_STIME_ARGS (buf_info->run_ts), splitmux->gop_total_bytes);

  loop_again = TRUE;
  do {
    if (ctx->flushing)
      break;

    switch (splitmux->input_state) {
      case SPLITMUX_INPUT_STATE_COLLECTING_GOP_START:
        if (ctx->is_reference) {
          /* This is the reference context. If it's a keyframe,
           * it marks the start of a new GOP and we should wait in
           * check_completed_gop before continuing, but either way
           * (keyframe or no, we'll pass this buffer through after
           * so set loop_again to FALSE */
          loop_again = FALSE;

          if (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT)) {
            /* Allow other input pads to catch up to here too */
            splitmux->max_in_running_time = ctx->in_running_time;
            GST_LOG_OBJECT (splitmux,
                "Max in running time now %" GST_TIME_FORMAT,
                GST_TIME_ARGS (splitmux->max_in_running_time));
            GST_SPLITMUX_BROADCAST_INPUT (splitmux);
            break;
          }
          GST_INFO_OBJECT (pad,
              "Have keyframe with running time %" GST_STIME_FORMAT,
              GST_STIME_ARGS (ctx->in_running_time));
          keyframe = TRUE;
          splitmux->input_state = SPLITMUX_INPUT_STATE_WAITING_GOP_COLLECT;
          splitmux->max_in_running_time = ctx->in_running_time;
          GST_LOG_OBJECT (splitmux, "Max in running time now %" GST_TIME_FORMAT,
              GST_TIME_ARGS (splitmux->max_in_running_time));
          /* Wake up other input pads to collect this GOP */
          GST_SPLITMUX_BROADCAST_INPUT (splitmux);
          check_completed_gop (splitmux, ctx);
          /* Store this new keyframe to remember the start of GOP */
          gst_buffer_replace (&ctx->prev_in_keyframe, buf);
        } else {
          /* Pass this buffer if the reference ctx is far enough ahead */
          if (ctx->in_running_time < splitmux->max_in_running_time) {
            loop_again = FALSE;
            break;
          }

          /* We're still waiting for a keyframe on the reference pad, sleep */
          GST_LOG_OBJECT (pad, "Sleeping for GOP start");
          GST_SPLITMUX_WAIT_INPUT (splitmux);
          GST_LOG_OBJECT (pad,
              "Done sleeping for GOP start input state now %d",
              splitmux->input_state);
        }
        break;
      case SPLITMUX_INPUT_STATE_WAITING_GOP_COLLECT:{
        /* We're collecting a GOP. If this is the reference context,
         * we need to check if this is a keyframe that marks the start
         * of the next GOP. If it is, it marks the end of the GOP we're
         * collecting, so sleep and wait until all the other pads also
         * reach that timestamp - at which point, we have an entire GOP
         * and either go to ENDING_FILE or release this GOP to the muxer and
         * go back to COLLECT_GOP_START. */

        /* If we overran the target timestamp, it might be time to process
         * the GOP, otherwise bail out for more data
         */
        GST_LOG_OBJECT (pad,
            "Checking TS %" GST_STIME_FORMAT " against max %"
            GST_STIME_FORMAT, GST_STIME_ARGS (ctx->in_running_time),
            GST_STIME_ARGS (splitmux->max_in_running_time));

        if (ctx->in_running_time < splitmux->max_in_running_time) {
          loop_again = FALSE;
          break;
        }

        GST_LOG_OBJECT (pad,
            "Collected last packet of GOP. Checking other pads");
        check_completed_gop (splitmux, ctx);
        break;
      }
      case SPLITMUX_INPUT_STATE_FINISHING_UP:
        loop_again = FALSE;
        break;
      default:
        loop_again = FALSE;
        break;
    }
  }
  while (loop_again);

  if (keyframe) {
    splitmux->queued_keyframes++;
    buf_info->keyframe = TRUE;
  }

  /* Update total input byte counter for overflow detect */
  splitmux->gop_total_bytes += buf_info->buf_size;

  /* Now add this buffer to the queue just before returning */
  g_queue_push_head (&ctx->queued_bufs, buf_info);

  GST_LOG_OBJECT (pad, "Returning to queue buffer %" GST_PTR_FORMAT
      " run ts %" GST_STIME_FORMAT, buf, GST_STIME_ARGS (ctx->in_running_time));

  GST_SPLITMUX_UNLOCK (splitmux);
  return GST_PAD_PROBE_PASS;

beach:
  GST_SPLITMUX_UNLOCK (splitmux);
  if (buf_info)
    mq_stream_buf_free (buf_info);
  return GST_PAD_PROBE_PASS;
}

static void
grow_blocked_queues (GstSplitMuxSink * splitmux)
{
  GList *cur;

  /* Scan other queues for full-ness and grow them */
  for (cur = g_list_first (splitmux->contexts);
      cur != NULL; cur = g_list_next (cur)) {
    MqStreamCtx *tmpctx = (MqStreamCtx *) (cur->data);
    guint cur_limit;
    guint cur_len = g_queue_get_length (&tmpctx->queued_bufs);

    g_object_get (tmpctx->q, "max-size-buffers", &cur_limit, NULL);
    GST_LOG_OBJECT (tmpctx->q, "Queue len %u", cur_len);

    if (cur_len >= cur_limit) {
      cur_limit = cur_len + 1;
      GST_DEBUG_OBJECT (tmpctx->q,
          "Queue overflowed and needs enlarging. Growing to %u buffers",
          cur_limit);
      g_object_set (tmpctx->q, "max-size-buffers", cur_limit, NULL);
    }
  }
}

static void
handle_q_underrun (GstElement * q, gpointer user_data)
{
  MqStreamCtx *ctx = (MqStreamCtx *) (user_data);
  GstSplitMuxSink *splitmux = ctx->splitmux;

  GST_SPLITMUX_LOCK (splitmux);
  GST_DEBUG_OBJECT (q,
      "Queue reported underrun with %d keyframes and %d cmds enqueued",
      splitmux->queued_keyframes, g_queue_get_length (&splitmux->out_cmd_q));
  grow_blocked_queues (splitmux);
  GST_SPLITMUX_UNLOCK (splitmux);
}

static void
handle_q_overrun (GstElement * q, gpointer user_data)
{
  MqStreamCtx *ctx = (MqStreamCtx *) (user_data);
  GstSplitMuxSink *splitmux = ctx->splitmux;
  gboolean allow_grow = FALSE;

  GST_SPLITMUX_LOCK (splitmux);
  GST_DEBUG_OBJECT (q,
      "Queue reported overrun with %d keyframes and %d cmds enqueued",
      splitmux->queued_keyframes, g_queue_get_length (&splitmux->out_cmd_q));

  if (splitmux->queued_keyframes < 2) {
    /* Less than a full GOP queued, grow the queue */
    allow_grow = TRUE;
  } else if (g_queue_get_length (&splitmux->out_cmd_q) < 1) {
    allow_grow = TRUE;
  } else {
    /* If another queue is starved, grow */
    GList *cur;
    for (cur = g_list_first (splitmux->contexts);
        cur != NULL; cur = g_list_next (cur)) {
      MqStreamCtx *tmpctx = (MqStreamCtx *) (cur->data);
      if (tmpctx != ctx && g_queue_get_length (&tmpctx->queued_bufs) < 1) {
        allow_grow = TRUE;
      }
    }
  }
  GST_SPLITMUX_UNLOCK (splitmux);

  if (allow_grow) {
    guint cur_limit;

    g_object_get (q, "max-size-buffers", &cur_limit, NULL);
    cur_limit++;

    GST_DEBUG_OBJECT (q,
        "Queue overflowed and needs enlarging. Growing to %u buffers",
        cur_limit);

    g_object_set (q, "max-size-buffers", cur_limit, NULL);
  }
}

/* Called with SPLITMUX lock held */
static const gchar *
lookup_muxer_pad (GstSplitMuxSink * splitmux, const gchar * sinkpad_name)
{
  const gchar *ret = NULL;

  if (splitmux->muxerpad_map == NULL)
    return NULL;

  if (sinkpad_name == NULL) {
    GST_WARNING_OBJECT (splitmux,
        "Can't look up request pad in pad map without providing a pad name");
    return NULL;
  }

  ret = gst_structure_get_string (splitmux->muxerpad_map, sinkpad_name);
  if (ret) {
    GST_INFO_OBJECT (splitmux, "Sink pad %s maps to muxer pad %s", sinkpad_name,
        ret);
    return g_strdup (ret);
  }

  return NULL;
}

static GstPad *
gst_splitmux_sink_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstSplitMuxSink *splitmux = (GstSplitMuxSink *) element;
  GstPadTemplate *mux_template = NULL;
  GstPad *ret = NULL, *muxpad = NULL;
  GstElement *q;
  GstPad *q_sink = NULL, *q_src = NULL;
  gchar *gname, *qname;
  gboolean is_primary_video = FALSE, is_video = FALSE,
      muxer_is_requestpad = FALSE;
  MqStreamCtx *ctx;
  const gchar *muxer_padname = NULL;

  GST_DEBUG_OBJECT (splitmux, "templ:%s, name:%s", templ->name_template, name);

  GST_SPLITMUX_LOCK (splitmux);
  if (!create_muxer (splitmux))
    goto fail;
  g_signal_emit (splitmux, signals[SIGNAL_MUXER_ADDED], 0, splitmux->muxer);

  if (g_str_equal (templ->name_template, "video") ||
      g_str_has_prefix (templ->name_template, "video_aux_")) {
    is_primary_video = g_str_equal (templ->name_template, "video");
    if (is_primary_video && splitmux->have_video)
      goto already_have_video;
    is_video = TRUE;
  }

  /* See if there's a pad map and it lists this pad */
  muxer_padname = lookup_muxer_pad (splitmux, name);

  if (muxer_padname == NULL) {
    if (is_video) {
      /* FIXME: Look for a pad template with matching caps, rather than by name */
      GST_DEBUG_OBJECT (element,
          "searching for pad-template with name 'video_%%u'");
      mux_template =
          gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS
          (splitmux->muxer), "video_%u");

      /* Fallback to find sink pad templates named 'video' (flvmux) */
      if (!mux_template) {
        GST_DEBUG_OBJECT (element,
            "searching for pad-template with name 'video'");
        mux_template =
            gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS
            (splitmux->muxer), "video");
      }
      name = NULL;
    } else {
      GST_DEBUG_OBJECT (element, "searching for pad-template with name '%s'",
          templ->name_template);
      mux_template =
          gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS
          (splitmux->muxer), templ->name_template);

      /* Fallback to find sink pad templates named 'audio' (flvmux) */
      if (!mux_template && g_str_has_prefix (templ->name_template, "audio_")) {
        GST_DEBUG_OBJECT (element,
            "searching for pad-template with name 'audio'");
        mux_template =
            gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS
            (splitmux->muxer), "audio");
        name = NULL;
      }
    }

    if (mux_template == NULL) {
      GST_DEBUG_OBJECT (element,
          "searching for pad-template with name 'sink_%%d'");
      mux_template =
          gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS
          (splitmux->muxer), "sink_%d");
      name = NULL;
    }
    if (mux_template == NULL) {
      GST_DEBUG_OBJECT (element, "searching for pad-template with name 'sink'");
      mux_template =
          gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS
          (splitmux->muxer), "sink");
      name = NULL;
    }

    if (mux_template == NULL) {
      GST_ERROR_OBJECT (element,
          "unable to find a suitable sink pad-template on the muxer");
      goto fail;
    }
    GST_DEBUG_OBJECT (element, "found sink pad-template '%s' on the muxer",
        mux_template->name_template);

    if (mux_template->presence == GST_PAD_REQUEST) {
      GST_DEBUG_OBJECT (element, "requesting pad from pad-template");

      muxpad =
          gst_element_request_pad (splitmux->muxer, mux_template, name, caps);
      muxer_is_requestpad = TRUE;
    } else if (mux_template->presence == GST_PAD_ALWAYS) {
      GST_DEBUG_OBJECT (element, "accessing always pad from pad-template");

      muxpad =
          gst_element_get_static_pad (splitmux->muxer,
          mux_template->name_template);
    } else {
      GST_ERROR_OBJECT (element,
          "unexpected pad presence %d", mux_template->presence);
      goto fail;
    }
  } else {
    /* Have a muxer pad name */
    if (!(muxpad = gst_element_get_static_pad (splitmux->muxer, muxer_padname))) {
      if ((muxpad =
              gst_element_get_request_pad (splitmux->muxer, muxer_padname)))
        muxer_is_requestpad = TRUE;
    }
    g_free ((gchar *) muxer_padname);
    muxer_padname = NULL;
  }

  /* One way or another, we must have a muxer pad by now */
  if (muxpad == NULL)
    goto fail;

  if (is_primary_video)
    gname = g_strdup ("video");
  else if (name == NULL)
    gname = gst_pad_get_name (muxpad);
  else
    gname = g_strdup (name);

  qname = g_strdup_printf ("queue_%s", gname);
  if ((q = create_element (splitmux, "queue", qname, FALSE)) == NULL) {
    g_free (qname);
    goto fail;
  }
  g_free (qname);

  gst_element_set_state (q, GST_STATE_TARGET (splitmux));

  g_object_set (q, "max-size-bytes", 0, "max-size-time", (guint64) (0),
      "max-size-buffers", 5, NULL);

  q_sink = gst_element_get_static_pad (q, "sink");
  q_src = gst_element_get_static_pad (q, "src");

  if (gst_pad_link (q_src, muxpad) != GST_PAD_LINK_OK) {
    if (muxer_is_requestpad)
      gst_element_release_request_pad (splitmux->muxer, muxpad);
    gst_object_unref (GST_OBJECT (muxpad));
    goto fail;
  }

  gst_object_unref (GST_OBJECT (muxpad));

  ctx = mq_stream_ctx_new (splitmux);
  /* Context holds a ref: */
  ctx->q = gst_object_ref (q);
  ctx->srcpad = q_src;
  ctx->sinkpad = q_sink;
  ctx->q_overrun_id =
      g_signal_connect (q, "overrun", (GCallback) handle_q_overrun, ctx);
  g_signal_connect (q, "underrun", (GCallback) handle_q_underrun, ctx);

  ctx->src_pad_block_id =
      gst_pad_add_probe (q_src,
      GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM | GST_PAD_PROBE_TYPE_EVENT_FLUSH,
      (GstPadProbeCallback) handle_mq_output, ctx, NULL);
  if (is_primary_video && splitmux->reference_ctx != NULL) {
    splitmux->reference_ctx->is_reference = FALSE;
    splitmux->reference_ctx = NULL;
  }
  if (splitmux->reference_ctx == NULL) {
    splitmux->reference_ctx = ctx;
    ctx->is_reference = TRUE;
  }

  ret = gst_ghost_pad_new_from_template (gname, q_sink, templ);
  g_object_set_qdata ((GObject *) (ret), PAD_CONTEXT, ctx);

  ctx->sink_pad_block_id =
      gst_pad_add_probe (q_sink,
      GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM | GST_PAD_PROBE_TYPE_EVENT_FLUSH |
      GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM,
      (GstPadProbeCallback) handle_mq_input, ctx, NULL);

  GST_DEBUG_OBJECT (splitmux, "splitmuxsink pad %" GST_PTR_FORMAT
      " feeds queue pad %" GST_PTR_FORMAT, ret, q_sink);

  splitmux->contexts = g_list_append (splitmux->contexts, ctx);

  g_free (gname);

  if (is_primary_video)
    splitmux->have_video = TRUE;

  gst_pad_set_active (ret, TRUE);
  gst_element_add_pad (GST_ELEMENT (splitmux), ret);

  GST_SPLITMUX_UNLOCK (splitmux);

  return ret;
fail:
  GST_SPLITMUX_UNLOCK (splitmux);

  if (q_sink)
    gst_object_unref (q_sink);
  if (q_src)
    gst_object_unref (q_src);
  return NULL;
already_have_video:
  GST_DEBUG_OBJECT (splitmux, "video sink pad already requested");
  GST_SPLITMUX_UNLOCK (splitmux);
  return NULL;
}

static void
gst_splitmux_sink_release_pad (GstElement * element, GstPad * pad)
{
  GstSplitMuxSink *splitmux = (GstSplitMuxSink *) element;
  GstPad *muxpad = NULL;
  MqStreamCtx *ctx =
      (MqStreamCtx *) (g_object_get_qdata ((GObject *) (pad), PAD_CONTEXT));

  GST_SPLITMUX_LOCK (splitmux);

  if (splitmux->muxer == NULL)
    goto fail;                  /* Elements don't exist yet - nothing to release */

  GST_INFO_OBJECT (pad, "releasing request pad");

  muxpad = gst_pad_get_peer (ctx->srcpad);

  /* Remove the context from our consideration */
  splitmux->contexts = g_list_remove (splitmux->contexts, ctx);

  if (ctx->sink_pad_block_id)
    gst_pad_remove_probe (ctx->sinkpad, ctx->sink_pad_block_id);

  if (ctx->src_pad_block_id)
    gst_pad_remove_probe (ctx->srcpad, ctx->src_pad_block_id);

  /* Can release the context now */
  mq_stream_ctx_free (ctx);
  if (ctx == splitmux->reference_ctx)
    splitmux->reference_ctx = NULL;

  /* Release and free the muxer input */
  if (muxpad) {
    gst_element_release_request_pad (splitmux->muxer, muxpad);
    gst_object_unref (muxpad);
  }

  if (GST_PAD_PAD_TEMPLATE (pad) &&
      g_str_equal (GST_PAD_TEMPLATE_NAME_TEMPLATE (GST_PAD_PAD_TEMPLATE
              (pad)), "video"))
    splitmux->have_video = FALSE;

  gst_element_remove_pad (element, pad);

  /* Reset the internal elements only after all request pads are released */
  if (splitmux->contexts == NULL)
    gst_splitmux_reset (splitmux);

fail:
  GST_SPLITMUX_UNLOCK (splitmux);
}

static GstElement *
create_element (GstSplitMuxSink * splitmux,
    const gchar * factory, const gchar * name, gboolean locked)
{
  GstElement *ret = gst_element_factory_make (factory, name);
  if (ret == NULL) {
    g_warning ("Failed to create %s - splitmuxsink will not work", name);
    return NULL;
  }

  if (locked) {
    /* Ensure the sink starts in locked state and NULL - it will be changed
     * by the filename setting code */
    gst_element_set_locked_state (ret, TRUE);
    gst_element_set_state (ret, GST_STATE_NULL);
  }

  if (!gst_bin_add (GST_BIN (splitmux), ret)) {
    g_warning ("Could not add %s element - splitmuxsink will not work", name);
    gst_object_unref (ret);
    return NULL;
  }

  return ret;
}

static gboolean
create_muxer (GstSplitMuxSink * splitmux)
{
  /* Create internal elements */
  if (splitmux->muxer == NULL) {
    GstElement *provided_muxer = NULL;

    GST_OBJECT_LOCK (splitmux);
    if (splitmux->provided_muxer != NULL)
      provided_muxer = gst_object_ref (splitmux->provided_muxer);
    GST_OBJECT_UNLOCK (splitmux);

    if ((!splitmux->async_finalize && provided_muxer == NULL) ||
        (splitmux->async_finalize && splitmux->muxer_factory == NULL)) {
      if ((splitmux->muxer =
              create_element (splitmux, DEFAULT_MUXER, "muxer", FALSE)) == NULL)
        goto fail;
    } else if (splitmux->async_finalize) {
      if ((splitmux->muxer =
              create_element (splitmux, splitmux->muxer_factory, "muxer",
                  FALSE)) == NULL)
        goto fail;
      if (splitmux->muxer_properties)
        gst_structure_foreach (splitmux->muxer_properties,
            _set_property_from_structure, splitmux->muxer);
    } else {
      /* Ensure it's not in locked state (we might be reusing an old element) */
      gst_element_set_locked_state (provided_muxer, FALSE);
      if (!gst_bin_add (GST_BIN (splitmux), provided_muxer)) {
        g_warning ("Could not add muxer element - splitmuxsink will not work");
        gst_object_unref (provided_muxer);
        goto fail;
      }

      splitmux->muxer = provided_muxer;
      gst_object_unref (provided_muxer);
    }

    if (splitmux->use_robust_muxing) {
      update_muxer_properties (splitmux);
    }
  }

  return TRUE;
fail:
  return FALSE;
}

static GstElement *
find_sink (GstElement * e)
{
  GstElement *res = NULL;
  GstIterator *iter;
  gboolean done = FALSE;
  GValue data = { 0, };

  if (!GST_IS_BIN (e))
    return e;

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (e), "location") != NULL)
    return e;

  iter = gst_bin_iterate_sinks (GST_BIN (e));
  while (!done) {
    switch (gst_iterator_next (iter, &data)) {
      case GST_ITERATOR_OK:
      {
        GstElement *child = g_value_get_object (&data);
        if (g_object_class_find_property (G_OBJECT_GET_CLASS (child),
                "location") != NULL) {
          res = child;
          done = TRUE;
        }
        g_value_reset (&data);
        break;
      }
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (iter);
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
      case GST_ITERATOR_ERROR:
        g_assert_not_reached ();
        break;
    }
  }
  g_value_unset (&data);
  gst_iterator_free (iter);

  return res;
}

static gboolean
create_sink (GstSplitMuxSink * splitmux)
{
  GstElement *provided_sink = NULL;

  if (splitmux->active_sink == NULL) {

    GST_OBJECT_LOCK (splitmux);
    if (splitmux->provided_sink != NULL)
      provided_sink = gst_object_ref (splitmux->provided_sink);
    GST_OBJECT_UNLOCK (splitmux);

    if ((!splitmux->async_finalize && provided_sink == NULL) ||
        (splitmux->async_finalize && splitmux->sink_factory == NULL)) {
      if ((splitmux->sink =
              create_element (splitmux, DEFAULT_SINK, "sink", TRUE)) == NULL)
        goto fail;
      splitmux->active_sink = splitmux->sink;
    } else if (splitmux->async_finalize) {
      if ((splitmux->sink =
              create_element (splitmux, splitmux->sink_factory, "sink",
                  TRUE)) == NULL)
        goto fail;
      if (splitmux->sink_properties)
        gst_structure_foreach (splitmux->sink_properties,
            _set_property_from_structure, splitmux->sink);
      splitmux->active_sink = splitmux->sink;
    } else {
      /* Ensure the sink starts in locked state and NULL - it will be changed
       * by the filename setting code */
      gst_element_set_locked_state (provided_sink, TRUE);
      gst_element_set_state (provided_sink, GST_STATE_NULL);
      if (!gst_bin_add (GST_BIN (splitmux), provided_sink)) {
        g_warning ("Could not add sink elements - splitmuxsink will not work");
        gst_object_unref (provided_sink);
        goto fail;
      }

      splitmux->active_sink = provided_sink;

      /* The bin holds a ref now, we can drop our tmp ref */
      gst_object_unref (provided_sink);

      /* Find the sink element */
      splitmux->sink = find_sink (splitmux->active_sink);
      if (splitmux->sink == NULL) {
        g_warning
            ("Could not locate sink element in provided sink - splitmuxsink will not work");
        goto fail;
      }
    }

#if 1
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (splitmux->sink),
            "async") != NULL) {
      /* async child elements are causing state change races and weird
       * failures, so let's try and turn that off */
      g_object_set (splitmux->sink, "async", FALSE, NULL);
    }
#endif

    if (!gst_element_link (splitmux->muxer, splitmux->active_sink)) {
      g_warning ("Failed to link muxer and sink- splitmuxsink will not work");
      goto fail;
    }
  }

  return TRUE;
fail:
  return FALSE;
}

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
static void
set_next_filename (GstSplitMuxSink * splitmux, MqStreamCtx * ctx)
{
  gchar *fname = NULL;
  GstSample *sample;
  GstCaps *caps;

  gst_splitmux_sink_ensure_max_files (splitmux);

  if (ctx->cur_out_buffer == NULL) {
    GST_WARNING_OBJECT (splitmux, "Starting next file without buffer");
  }

  caps = gst_pad_get_current_caps (ctx->srcpad);
  sample = gst_sample_new (ctx->cur_out_buffer, caps, &ctx->out_segment, NULL);
  g_signal_emit (splitmux, signals[SIGNAL_FORMAT_LOCATION_FULL], 0,
      splitmux->fragment_id, sample, &fname);
  gst_sample_unref (sample);
  if (caps)
    gst_caps_unref (caps);

  if (fname == NULL) {
    /* Fallback to the old signal if the new one returned nothing */
    g_signal_emit (splitmux, signals[SIGNAL_FORMAT_LOCATION], 0,
        splitmux->fragment_id, &fname);
  }

  if (!fname)
    fname = splitmux->location ?
        g_strdup_printf (splitmux->location, splitmux->fragment_id) : NULL;

  if (fname) {
    GST_INFO_OBJECT (splitmux, "Setting file to %s", fname);
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (splitmux->sink),
            "location") != NULL)
      g_object_set (splitmux->sink, "location", fname, NULL);
    g_free (fname);

    splitmux->fragment_id++;
  }
}

/* called with GST_SPLITMUX_LOCK */
static void
do_async_start (GstSplitMuxSink * splitmux)
{
  GstMessage *message;

  if (!splitmux->need_async_start) {
    GST_INFO_OBJECT (splitmux, "no async_start needed");
    return;
  }

  splitmux->async_pending = TRUE;

  GST_INFO_OBJECT (splitmux, "Sending async_start message");
  message = gst_message_new_async_start (GST_OBJECT_CAST (splitmux));

  GST_SPLITMUX_UNLOCK (splitmux);
  GST_BIN_CLASS (parent_class)->handle_message (GST_BIN_CAST
      (splitmux), message);
  GST_SPLITMUX_LOCK (splitmux);
}

/* called with GST_SPLITMUX_LOCK */
static void
do_async_done (GstSplitMuxSink * splitmux)
{
  GstMessage *message;

  if (splitmux->async_pending) {
    GST_INFO_OBJECT (splitmux, "Sending async_done message");
    splitmux->async_pending = FALSE;
    GST_SPLITMUX_UNLOCK (splitmux);

    message =
        gst_message_new_async_done (GST_OBJECT_CAST (splitmux),
        GST_CLOCK_TIME_NONE);
    GST_BIN_CLASS (parent_class)->handle_message (GST_BIN_CAST
        (splitmux), message);
    GST_SPLITMUX_LOCK (splitmux);
  }

  splitmux->need_async_start = FALSE;
}

static GstStateChangeReturn
gst_splitmux_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstSplitMuxSink *splitmux = (GstSplitMuxSink *) element;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:{
      GST_SPLITMUX_LOCK (splitmux);
      if (!create_muxer (splitmux) || !create_sink (splitmux)) {
        ret = GST_STATE_CHANGE_FAILURE;
        GST_SPLITMUX_UNLOCK (splitmux);
        goto beach;
      }
      g_signal_emit (splitmux, signals[SIGNAL_MUXER_ADDED], 0, splitmux->muxer);
      g_signal_emit (splitmux, signals[SIGNAL_SINK_ADDED], 0, splitmux->sink);
      GST_SPLITMUX_UNLOCK (splitmux);
      splitmux->fragment_id = splitmux->start_index;
      break;
    }
    case GST_STATE_CHANGE_READY_TO_PAUSED:{
      GST_SPLITMUX_LOCK (splitmux);
      /* Start by collecting one input on each pad */
      splitmux->input_state = SPLITMUX_INPUT_STATE_COLLECTING_GOP_START;
      splitmux->output_state = SPLITMUX_OUTPUT_STATE_START_NEXT_FILE;
      splitmux->max_in_running_time = GST_CLOCK_STIME_NONE;
      splitmux->gop_start_time = splitmux->fragment_start_time =
          GST_CLOCK_STIME_NONE;
      splitmux->muxed_out_bytes = 0;
      splitmux->ready_for_output = FALSE;
      GST_SPLITMUX_UNLOCK (splitmux);
      break;
    }
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      g_atomic_int_set (&(splitmux->split_requested), FALSE);
      g_atomic_int_set (&(splitmux->do_split_next_gop), FALSE);
    case GST_STATE_CHANGE_READY_TO_NULL:
      GST_SPLITMUX_LOCK (splitmux);
      gst_queue_array_clear (splitmux->times_to_split);
      splitmux->output_state = SPLITMUX_OUTPUT_STATE_STOPPED;
      splitmux->input_state = SPLITMUX_INPUT_STATE_STOPPED;
      /* Wake up any blocked threads */
      GST_LOG_OBJECT (splitmux,
          "State change -> NULL or READY. Waking threads");
      GST_SPLITMUX_BROADCAST_INPUT (splitmux);
      GST_SPLITMUX_BROADCAST_OUTPUT (splitmux);
      GST_SPLITMUX_UNLOCK (splitmux);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto beach;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      splitmux->need_async_start = TRUE;
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:{
      /* Change state async, because our child sink might not
       * be ready to do that for us yet if it's state is still locked */

      splitmux->need_async_start = TRUE;
      /* we want to go async to PAUSED until we managed to configure and add the
       * sink */
      GST_SPLITMUX_LOCK (splitmux);
      do_async_start (splitmux);
      GST_SPLITMUX_UNLOCK (splitmux);
      ret = GST_STATE_CHANGE_ASYNC;
      break;
    }
    case GST_STATE_CHANGE_READY_TO_NULL:
      GST_SPLITMUX_LOCK (splitmux);
      splitmux->fragment_id = 0;
      /* Reset internal elements only if no pad contexts are using them */
      if (splitmux->contexts == NULL)
        gst_splitmux_reset (splitmux);
      do_async_done (splitmux);
      GST_SPLITMUX_UNLOCK (splitmux);
      break;
    default:
      break;
  }

beach:
  if (transition == GST_STATE_CHANGE_NULL_TO_READY &&
      ret == GST_STATE_CHANGE_FAILURE) {
    /* Cleanup elements on failed transition out of NULL */
    gst_splitmux_reset (splitmux);
    GST_SPLITMUX_LOCK (splitmux);
    do_async_done (splitmux);
    GST_SPLITMUX_UNLOCK (splitmux);
  }
  return ret;
}

gboolean
register_splitmuxsink (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (splitmux_debug, "splitmuxsink", 0,
      "Split File Muxing Sink");

  return gst_element_register (plugin, "splitmuxsink", GST_RANK_NONE,
      GST_TYPE_SPLITMUX_SINK);
}

static void
gst_splitmux_sink_ensure_max_files (GstSplitMuxSink * splitmux)
{
  if (splitmux->max_files && splitmux->fragment_id >= splitmux->max_files) {
    splitmux->fragment_id = 0;
  }
}

static void
split_now (GstSplitMuxSink * splitmux)
{
  g_atomic_int_set (&(splitmux->do_split_next_gop), TRUE);
}

static void
split_after (GstSplitMuxSink * splitmux)
{
  g_atomic_int_set (&(splitmux->split_requested), TRUE);
}

static void
split_at_running_time (GstSplitMuxSink * splitmux, GstClockTime split_time)
{
  gboolean send_keyframe_requests;

  GST_SPLITMUX_LOCK (splitmux);
  gst_queue_array_push_tail_struct (splitmux->times_to_split, &split_time);
  send_keyframe_requests = splitmux->send_keyframe_requests;
  GST_SPLITMUX_UNLOCK (splitmux);

  if (send_keyframe_requests) {
    GstEvent *ev =
        gst_video_event_new_upstream_force_key_unit (split_time, TRUE, 0);
    GST_INFO_OBJECT (splitmux, "Requesting next keyframe at %" GST_TIME_FORMAT,
        GST_TIME_ARGS (split_time));
    if (!gst_pad_push_event (splitmux->reference_ctx->sinkpad, ev)) {
      GST_WARNING_OBJECT (splitmux,
          "Could not request keyframe at %" GST_TIME_FORMAT,
          GST_TIME_ARGS (split_time));
    }
  }
}
