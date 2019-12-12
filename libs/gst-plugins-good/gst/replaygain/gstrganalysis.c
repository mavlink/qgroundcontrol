/* GStreamer ReplayGain analysis
 *
 * Copyright (C) 2006 Rene Stadler <mail@renestadler.de>
 * 
 * gstrganalysis.c: Element that performs the ReplayGain analysis
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

/**
 * SECTION:element-rganalysis
 * @title: rganalysis
 * @see_also: #GstRgVolume
 *
 * This element analyzes raw audio sample data in accordance with the proposed
 * [ReplayGain standard](https://wiki.hydrogenaud.io/index.php?title=ReplayGain) for
 * calculating the ideal replay gain for music tracks and albums.  The element
 * is designed as a pass-through filter that never modifies any data.  As it
 * receives an EOS event, it finalizes the ongoing analysis and generates a tag
 * list containing the results.  It is sent downstream with a tag event and
 * posted on the message bus with a tag message.  The EOS event is forwarded as
 * normal afterwards.  Result tag lists at least contain the tags
 * #GST_TAG_TRACK_GAIN, #GST_TAG_TRACK_PEAK and #GST_TAG_REFERENCE_LEVEL.
 *
 * Because the generated metadata tags become available at the end of streams,
 * downstream muxer and encoder elements are normally unable to save them in
 * their output since they generally save metadata in the file header.
 * Therefore, it is often necessary that applications read the results in a bus
 * event handler for the tag message.  Obtaining the values this way is always
 * needed for album processing (see #GstRgAnalysis:num-tracks property) since
 * the album gain and peak values need to be associated with all tracks of an
 * album, not just the last one.
 *
 * ## Example launch lines
 * |[
 * gst-launch-1.0 -t audiotestsrc wave=sine num-buffers=512 ! rganalysis ! fakesink
 * ]| Analyze a simple test waveform
 * |[
 * gst-launch-1.0 -t filesrc location=filename.ext ! decodebin \
 *     ! audioconvert ! audioresample ! rganalysis ! fakesink
 * ]| Analyze a given file
 * |[
 * gst-launch-1.0 -t gnomevfssrc location=http://replaygain.hydrogenaudio.org/ref_pink.wav \
 *     ! wavparse ! rganalysis ! fakesink
 * ]| Analyze the pink noise reference file
 *
 * The above launch line yields a result gain of +6 dB (instead of the expected
 * +0 dB).  This is not in error, refer to the #GstRgAnalysis:reference-level
 * property documentation for more information.
 *
 * ## Acknowledgements
 *
 * This element is based on code used in the [vorbisgain](https://sjeng.org/vorbisgain.html)
 * program and many others.  The relevant parts are copyrighted by David Robinson, Glen Sawyer
 * and Frank Klemm.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>

#include "gstrganalysis.h"
#include "replaygain.h"

GST_DEBUG_CATEGORY_STATIC (gst_rg_analysis_debug);
#define GST_CAT_DEFAULT gst_rg_analysis_debug

/* Default property value. */
#define FORCED_DEFAULT TRUE
#define DEFAULT_MESSAGE FALSE

enum
{
  PROP_0,
  PROP_NUM_TRACKS,
  PROP_FORCED,
  PROP_REFERENCE_LEVEL,
  PROP_MESSAGE
};

/* The ReplayGain algorithm is intended for use with mono and stereo
 * audio.  The used implementation has filter coefficients for the
 * "usual" sample rates in the 8000 to 48000 Hz range. */
#define REPLAY_GAIN_CAPS "audio/x-raw," \
  "format = (string) { "GST_AUDIO_NE(F32)","GST_AUDIO_NE(S16)" }, "     \
  "layout = (string) interleaved, "                                     \
  "channels = (int) 1, "                                                \
  "rate = (int) { 8000, 11025, 12000, 16000, 22050, 24000, 32000, "     \
  "44100, 48000 }; "                                                    \
  "audio/x-raw,"                                                        \
  "format = (string) { "GST_AUDIO_NE(F32)","GST_AUDIO_NE(S16)" }, "     \
  "layout = (string) interleaved, "                                     \
  "channels = (int) 2, "                                                \
  "channel-mask = (bitmask) 0x3, "                                      \
  "rate = (int) { 8000, 11025, 12000, 16000, 22050, 24000, 32000, "     \
  "44100, 48000 }"

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (REPLAY_GAIN_CAPS));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (REPLAY_GAIN_CAPS));

#define gst_rg_analysis_parent_class parent_class
G_DEFINE_TYPE (GstRgAnalysis, gst_rg_analysis, GST_TYPE_BASE_TRANSFORM);

static void gst_rg_analysis_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rg_analysis_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_rg_analysis_start (GstBaseTransform * base);
static gboolean gst_rg_analysis_set_caps (GstBaseTransform * base,
    GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_rg_analysis_transform_ip (GstBaseTransform * base,
    GstBuffer * buf);
static gboolean gst_rg_analysis_sink_event (GstBaseTransform * base,
    GstEvent * event);
static gboolean gst_rg_analysis_stop (GstBaseTransform * base);

static void gst_rg_analysis_handle_tags (GstRgAnalysis * filter,
    const GstTagList * tag_list);
static void gst_rg_analysis_handle_eos (GstRgAnalysis * filter);
static gboolean gst_rg_analysis_track_result (GstRgAnalysis * filter,
    GstTagList ** tag_list);
static gboolean gst_rg_analysis_album_result (GstRgAnalysis * filter,
    GstTagList ** tag_list);

static void
gst_rg_analysis_class_init (GstRgAnalysisClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseTransformClass *trans_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rg_analysis_set_property;
  gobject_class->get_property = gst_rg_analysis_get_property;

  /**
   * GstRgAnalysis:num-tracks:
   *
   * Number of remaining album tracks.
   *
   * Analyzing several streams sequentially and assigning them a common result
   * gain is known as "album processing".  If this gain is used during playback
   * (by switching to "album mode"), all tracks of an album receive the same
   * amplification.  This keeps the relative volume levels between the tracks
   * intact.  To enable this, set this property to the number of streams that
   * will be processed as album tracks.
   *
   * Every time an EOS event is received, the value of this property is
   * decremented by one.  As it reaches zero, it is assumed that the last track
   * of the album finished.  The tag list for the final stream will contain the
   * additional tags #GST_TAG_ALBUM_GAIN and #GST_TAG_ALBUM_PEAK.  All other
   * streams just get the two track tags posted because the values for the album
   * tags are not known before all tracks are analyzed.  Applications need to
   * ensure that the album gain and peak values are also associated with the
   * other tracks when storing the results.
   *
   * If the total number of album tracks is unknown beforehand, just ensure that
   * the value is greater than 1 before each track starts.  Then before the end
   * of the last track, set it to the value 1.
   *
   * To perform album processing, the element has to preserve data between
   * streams.  This cannot survive a state change to the NULL or READY state.
   * If you change your pipeline's state to NULL or READY between tracks, lock
   * the element's state using gst_element_set_locked_state() when it is in
   * PAUSED or PLAYING.
   */
  g_object_class_install_property (gobject_class, PROP_NUM_TRACKS,
      g_param_spec_int ("num-tracks", "Number of album tracks",
          "Number of remaining album tracks", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRgAnalysis:forced:
   *
   * Whether to analyze streams even when ReplayGain tags exist.
   *
   * For assisting transcoder/converter applications, the element can silently
   * skip the processing of streams that already contain the necessary tags.
   * Data will flow as usual but the element will not consume CPU time and will
   * not generate result tags.  To enable possible skipping, set this property
   * to %FALSE.
   *
   * If used in conjunction with <link linkend="GstRgAnalysis--num-tracks">album
   * processing</link>, the element will skip the number of remaining album
   * tracks if a full set of tags is found for the first track.  If a subsequent
   * track of the album is missing tags, processing cannot start again.  If this
   * is undesired, the application has to scan all files beforehand and enable
   * forcing of processing if needed.
   */
  g_object_class_install_property (gobject_class, PROP_FORCED,
      g_param_spec_boolean ("forced", "Forced",
          "Analyze even if ReplayGain tags exist",
          FORCED_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRgAnalysis:reference-level:
   *
   * Reference level [dB].
   *
   * Analyzing the ReplayGain pink noise reference waveform computes a result of
   * +6 dB instead of the expected 0 dB.  This is because the default reference
   * level is 89 dB.  To obtain values as lined out in the original proposal of
   * ReplayGain, set this property to 83.
   *
   * Almost all software uses 89 dB as a reference however, and this value has
   * become the new official value, and that change has been acclaimed by the
   * original author of the ReplayGain proposal.
   *
   * The value was changed because the original proposal recommends a default
   * pre-amp value of +6 dB for playback.  This seemed a bit odd, as it means
   * that the algorithm has the general tendency to produce adjustment values
   * that are 6 dB too low.  Bumping the reference level by 6 dB compensated for
   * this.
   *
   * The problem of the reference level being ambiguous for lack of concise
   * standardization is to be solved by adopting the #GST_TAG_REFERENCE_LEVEL
   * tag, which allows to store the used value alongside the gain values.
   */
  g_object_class_install_property (gobject_class, PROP_REFERENCE_LEVEL,
      g_param_spec_double ("reference-level", "Reference level",
          "Reference level [dB]", 0.0, 150., RG_REFERENCE_LEVEL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_MESSAGE,
      g_param_spec_boolean ("message", "Message",
          "Post statics messages",
          DEFAULT_MESSAGE,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  trans_class = (GstBaseTransformClass *) klass;
  trans_class->start = GST_DEBUG_FUNCPTR (gst_rg_analysis_start);
  trans_class->set_caps = GST_DEBUG_FUNCPTR (gst_rg_analysis_set_caps);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_rg_analysis_transform_ip);
  trans_class->sink_event = GST_DEBUG_FUNCPTR (gst_rg_analysis_sink_event);
  trans_class->stop = GST_DEBUG_FUNCPTR (gst_rg_analysis_stop);
  trans_class->passthrough_on_same_caps = TRUE;

  gst_element_class_add_static_pad_template (element_class, &src_factory);
  gst_element_class_add_static_pad_template (element_class, &sink_factory);
  gst_element_class_set_static_metadata (element_class, "ReplayGain analysis",
      "Filter/Analyzer/Audio",
      "Perform the ReplayGain analysis",
      "Ren\xc3\xa9 Stadler <mail@renestadler.de>");

  GST_DEBUG_CATEGORY_INIT (gst_rg_analysis_debug, "rganalysis", 0,
      "ReplayGain analysis element");
}

static void
gst_rg_analysis_init (GstRgAnalysis * filter)
{
  GstBaseTransform *base = GST_BASE_TRANSFORM (filter);

  gst_base_transform_set_gap_aware (base, TRUE);

  filter->num_tracks = 0;
  filter->forced = FORCED_DEFAULT;
  filter->message = DEFAULT_MESSAGE;
  filter->reference_level = RG_REFERENCE_LEVEL;

  filter->ctx = NULL;
  filter->analyze = NULL;
}

static void
gst_rg_analysis_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRgAnalysis *filter = GST_RG_ANALYSIS (object);

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_NUM_TRACKS:
      filter->num_tracks = g_value_get_int (value);
      break;
    case PROP_FORCED:
      filter->forced = g_value_get_boolean (value);
      break;
    case PROP_REFERENCE_LEVEL:
      filter->reference_level = g_value_get_double (value);
      break;
    case PROP_MESSAGE:
      filter->message = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static void
gst_rg_analysis_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRgAnalysis *filter = GST_RG_ANALYSIS (object);

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_NUM_TRACKS:
      g_value_set_int (value, filter->num_tracks);
      break;
    case PROP_FORCED:
      g_value_set_boolean (value, filter->forced);
      break;
    case PROP_REFERENCE_LEVEL:
      g_value_set_double (value, filter->reference_level);
      break;
    case PROP_MESSAGE:
      g_value_set_boolean (value, filter->message);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static void
gst_rg_analysis_post_message (gpointer rganalysis, GstClockTime timestamp,
    GstClockTime duration, gdouble rglevel)
{
  GstRgAnalysis *filter = GST_RG_ANALYSIS (rganalysis);
  if (filter->message) {
    GstMessage *m;

    m = gst_message_new_element (GST_OBJECT_CAST (rganalysis),
        gst_structure_new ("rganalysis",
            "timestamp", G_TYPE_UINT64, timestamp,
            "duration", G_TYPE_UINT64, duration,
            "rglevel", G_TYPE_DOUBLE, rglevel, NULL));

    gst_element_post_message (GST_ELEMENT_CAST (rganalysis), m);
  }
}


static gboolean
gst_rg_analysis_start (GstBaseTransform * base)
{
  GstRgAnalysis *filter = GST_RG_ANALYSIS (base);

  filter->ignore_tags = FALSE;
  filter->skip = FALSE;
  filter->has_track_gain = FALSE;
  filter->has_track_peak = FALSE;
  filter->has_album_gain = FALSE;
  filter->has_album_peak = FALSE;

  filter->ctx = rg_analysis_new ();
  GST_OBJECT_LOCK (filter);
  rg_analysis_init_silence_detection (filter->ctx, gst_rg_analysis_post_message,
      filter);
  GST_OBJECT_UNLOCK (filter);
  filter->analyze = NULL;

  GST_LOG_OBJECT (filter, "started");

  return TRUE;
}

static gboolean
gst_rg_analysis_set_caps (GstBaseTransform * base, GstCaps * in_caps,
    GstCaps * out_caps)
{
  GstRgAnalysis *filter = GST_RG_ANALYSIS (base);
  GstAudioInfo info;
  gint rate, channels;

  g_return_val_if_fail (filter->ctx != NULL, FALSE);

  GST_DEBUG_OBJECT (filter,
      "set_caps in %" GST_PTR_FORMAT " out %" GST_PTR_FORMAT,
      in_caps, out_caps);

  if (!gst_audio_info_from_caps (&info, in_caps))
    goto invalid_format;

  rate = GST_AUDIO_INFO_RATE (&info);

  if (!rg_analysis_set_sample_rate (filter->ctx, rate))
    goto invalid_format;

  channels = GST_AUDIO_INFO_CHANNELS (&info);

  if (channels < 1 || channels > 2)
    goto invalid_format;

  switch (GST_AUDIO_INFO_FORMAT (&info)) {
    case GST_AUDIO_FORMAT_F32:
      /* The depth is not variable for float formats of course.  It just
       * makes the transform function nice and simple if the
       * rg_analysis_analyze_* functions have a common signature. */
      filter->depth = sizeof (gfloat) * 8;

      if (channels == 1)
        filter->analyze = rg_analysis_analyze_mono_float;
      else
        filter->analyze = rg_analysis_analyze_stereo_float;

      break;
    case GST_AUDIO_FORMAT_S16:
      filter->depth = sizeof (gint16) * 8;

      if (channels == 1)
        filter->analyze = rg_analysis_analyze_mono_int16;
      else
        filter->analyze = rg_analysis_analyze_stereo_int16;
      break;
    default:
      goto invalid_format;
  }

  return TRUE;

  /* Errors. */
invalid_format:
  {
    filter->analyze = NULL;
    GST_ELEMENT_ERROR (filter, CORE, NEGOTIATION,
        ("Invalid incoming caps: %" GST_PTR_FORMAT, in_caps), (NULL));
    return FALSE;
  }
}

static GstFlowReturn
gst_rg_analysis_transform_ip (GstBaseTransform * base, GstBuffer * buf)
{
  GstRgAnalysis *filter = GST_RG_ANALYSIS (base);
  GstMapInfo map;

  g_return_val_if_fail (filter->ctx != NULL, GST_FLOW_FLUSHING);
  g_return_val_if_fail (filter->analyze != NULL, GST_FLOW_NOT_NEGOTIATED);

  if (filter->skip)
    return GST_FLOW_OK;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  GST_LOG_OBJECT (filter, "processing buffer of size %" G_GSIZE_FORMAT,
      map.size);

  rg_analysis_start_buffer (filter->ctx, GST_BUFFER_TIMESTAMP (buf));
  filter->analyze (filter->ctx, map.data, map.size, filter->depth);

  gst_buffer_unmap (buf, &map);

  return GST_FLOW_OK;
}

static gboolean
gst_rg_analysis_sink_event (GstBaseTransform * base, GstEvent * event)
{
  GstRgAnalysis *filter = GST_RG_ANALYSIS (base);

  g_return_val_if_fail (filter->ctx != NULL, TRUE);

  switch (GST_EVENT_TYPE (event)) {

    case GST_EVENT_EOS:
    {
      GST_LOG_OBJECT (filter, "received EOS event");

      gst_rg_analysis_handle_eos (filter);

      GST_LOG_OBJECT (filter, "passing on EOS event");

      break;
    }
    case GST_EVENT_TAG:
    {
      GstTagList *tag_list;

      /* The reference to the tag list is borrowed. */
      gst_event_parse_tag (event, &tag_list);
      gst_rg_analysis_handle_tags (filter, tag_list);

      break;
    }
    default:
      break;
  }

  return GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (base, event);
}

static gboolean
gst_rg_analysis_stop (GstBaseTransform * base)
{
  GstRgAnalysis *filter = GST_RG_ANALYSIS (base);

  g_return_val_if_fail (filter->ctx != NULL, FALSE);

  rg_analysis_destroy (filter->ctx);
  filter->ctx = NULL;

  GST_LOG_OBJECT (filter, "stopped");

  return TRUE;
}

/* FIXME: handle global vs. stream-tags? */
static void
gst_rg_analysis_handle_tags (GstRgAnalysis * filter,
    const GstTagList * tag_list)
{
  gboolean album_processing = (filter->num_tracks > 0);
  gdouble dummy;

  if (!album_processing)
    filter->ignore_tags = FALSE;

  if (filter->skip && album_processing) {
    GST_DEBUG_OBJECT (filter, "ignoring tag event: skipping album");
    return;
  } else if (filter->skip) {
    GST_DEBUG_OBJECT (filter, "ignoring tag event: skipping track");
    return;
  } else if (filter->ignore_tags) {
    GST_DEBUG_OBJECT (filter, "ignoring tag event: cannot skip anyways");
    return;
  }

  filter->has_track_gain |= gst_tag_list_get_double (tag_list,
      GST_TAG_TRACK_GAIN, &dummy);
  filter->has_track_peak |= gst_tag_list_get_double (tag_list,
      GST_TAG_TRACK_PEAK, &dummy);
  filter->has_album_gain |= gst_tag_list_get_double (tag_list,
      GST_TAG_ALBUM_GAIN, &dummy);
  filter->has_album_peak |= gst_tag_list_get_double (tag_list,
      GST_TAG_ALBUM_PEAK, &dummy);

  if (!(filter->has_track_gain && filter->has_track_peak)) {
    GST_DEBUG_OBJECT (filter, "track tags not complete yet");
    return;
  }

  if (album_processing && !(filter->has_album_gain && filter->has_album_peak)) {
    GST_DEBUG_OBJECT (filter, "album tags not complete yet");
    return;
  }

  if (filter->forced) {
    GST_DEBUG_OBJECT (filter,
        "existing tags are sufficient, but processing anyway (forced)");
    return;
  }

  filter->skip = TRUE;
  rg_analysis_reset (filter->ctx);

  if (!album_processing) {
    GST_DEBUG_OBJECT (filter,
        "existing tags are sufficient, will not process this track");
  } else {
    GST_DEBUG_OBJECT (filter,
        "existing tags are sufficient, will not process this album");
  }
}

static void
gst_rg_analysis_handle_eos (GstRgAnalysis * filter)
{
  gboolean album_processing = (filter->num_tracks > 0);
  gboolean album_finished = (filter->num_tracks == 1);
  gboolean album_skipping = album_processing && filter->skip;

  filter->has_track_gain = FALSE;
  filter->has_track_peak = FALSE;

  if (album_finished) {
    filter->ignore_tags = FALSE;
    filter->skip = FALSE;
    filter->has_album_gain = FALSE;
    filter->has_album_peak = FALSE;
  } else if (!album_skipping) {
    filter->skip = FALSE;
  }

  /* We might have just fully processed a track because it has
   * incomplete tags.  If we do album processing and allow skipping
   * (not forced), prevent switching to skipping if a later track with
   * full tags comes along: */
  if (!filter->forced && album_processing && !album_finished)
    filter->ignore_tags = TRUE;

  if (!filter->skip) {
    GstTagList *tag_list = NULL;
    gboolean track_success;
    gboolean album_success = FALSE;

    track_success = gst_rg_analysis_track_result (filter, &tag_list);

    if (album_finished)
      album_success = gst_rg_analysis_album_result (filter, &tag_list);
    else if (!album_processing)
      rg_analysis_reset_album (filter->ctx);

    if (track_success || album_success) {
      GST_LOG_OBJECT (filter, "posting tag list with results");
      gst_tag_list_add (tag_list, GST_TAG_MERGE_APPEND,
          GST_TAG_REFERENCE_LEVEL, filter->reference_level, NULL);
      /* This takes ownership of our reference to the list */
      gst_pad_push_event (GST_BASE_TRANSFORM_SRC_PAD (filter),
          gst_event_new_tag (tag_list));
      tag_list = NULL;
    }
  }

  if (album_processing) {
    filter->num_tracks--;

    if (!album_finished) {
      GST_DEBUG_OBJECT (filter, "album not finished yet (num-tracks is now %u)",
          filter->num_tracks);
    } else {
      GST_DEBUG_OBJECT (filter, "album finished (num-tracks is now 0)");
    }
  }

  if (album_processing)
    g_object_notify (G_OBJECT (filter), "num-tracks");
}

/* FIXME: return tag list (lists?) based on input tags.. */
static gboolean
gst_rg_analysis_track_result (GstRgAnalysis * filter, GstTagList ** tag_list)
{
  gboolean track_success;
  gdouble track_gain, track_peak;

  track_success = rg_analysis_track_result (filter->ctx, &track_gain,
      &track_peak);

  if (track_success) {
    track_gain += filter->reference_level - RG_REFERENCE_LEVEL;
    GST_INFO_OBJECT (filter, "track gain is %+.2f dB, peak %.6f", track_gain,
        track_peak);
  } else {
    GST_INFO_OBJECT (filter, "track was too short to analyze");
  }

  if (track_success) {
    if (*tag_list == NULL)
      *tag_list = gst_tag_list_new_empty ();
    gst_tag_list_add (*tag_list, GST_TAG_MERGE_APPEND,
        GST_TAG_TRACK_PEAK, track_peak, GST_TAG_TRACK_GAIN, track_gain, NULL);
  }

  return track_success;
}

static gboolean
gst_rg_analysis_album_result (GstRgAnalysis * filter, GstTagList ** tag_list)
{
  gboolean album_success;
  gdouble album_gain, album_peak;

  album_success = rg_analysis_album_result (filter->ctx, &album_gain,
      &album_peak);

  if (album_success) {
    album_gain += filter->reference_level - RG_REFERENCE_LEVEL;
    GST_INFO_OBJECT (filter, "album gain is %+.2f dB, peak %.6f", album_gain,
        album_peak);
  } else {
    GST_INFO_OBJECT (filter, "album was too short to analyze");
  }

  if (album_success) {
    if (*tag_list == NULL)
      *tag_list = gst_tag_list_new_empty ();
    gst_tag_list_add (*tag_list, GST_TAG_MERGE_APPEND,
        GST_TAG_ALBUM_PEAK, album_peak, GST_TAG_ALBUM_GAIN, album_gain, NULL);
  }

  return album_success;
}
