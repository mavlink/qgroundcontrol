/* GStreamer ReplayGain volume adjustment
 *
 * Copyright (C) 2007 Rene Stadler <mail@renestadler.de>
 * 
 * gstrgvolume.c: Element to apply ReplayGain volume adjustment
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
 * SECTION:element-rgvolume
 * @title: rgvolume
 * @see_also: #GstRgLimiter, #GstRgAnalysis
 *
 * This element applies volume changes to streams as lined out in the proposed
 * [ReplayGain standard](https://wiki.hydrogenaud.io/index.php?title=ReplayGain).
 * It interprets the ReplayGain meta data tags and carries out the adjustment
 * (by using a volume element internally).
 *
 * The relevant tags are:
 * * #GST_TAG_TRACK_GAIN
 * * #GST_TAG_TRACK_PEAK
 * * #GST_TAG_ALBUM_GAIN
 * * #GST_TAG_ALBUM_PEAK
 * * #GST_TAG_REFERENCE_LEVEL
 *
 * The information carried by these tags must have been calculated beforehand by
 * performing the ReplayGain analysis.  This is implemented by the <link
 * linkend="GstRgAnalysis">rganalysis</link> element.
 *
 * The signal compression/limiting recommendations outlined in the proposed
 * standard are not implemented by this element.  This has to be handled by
 * separate elements because applications might want to have additional filters
 * between the volume adjustment and the limiting stage.  A basic limiter is
 * included with this plugin: The <link linkend="GstRgLimiter">rglimiter</link>
 * element applies -6 dB hard limiting as mentioned in the ReplayGain standard.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=filename.ext ! decodebin ! audioconvert \
 *     ! rgvolume ! audioconvert ! audioresample ! alsasink
 * ]| Playback of a file
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <gst/audio/audio.h>
#include <math.h>

#include "gstrgvolume.h"
#include "replaygain.h"

GST_DEBUG_CATEGORY_STATIC (gst_rg_volume_debug);
#define GST_CAT_DEFAULT gst_rg_volume_debug

enum
{
  PROP_0,
  PROP_ALBUM_MODE,
  PROP_HEADROOM,
  PROP_PRE_AMP,
  PROP_FALLBACK_GAIN,
  PROP_TARGET_GAIN,
  PROP_RESULT_GAIN
};

#define DEFAULT_ALBUM_MODE TRUE
#define DEFAULT_HEADROOM 0.0
#define DEFAULT_PRE_AMP 0.0
#define DEFAULT_FALLBACK_GAIN 0.0

#define DB_TO_LINEAR(x) pow (10., (x) / 20.)
#define LINEAR_TO_DB(x) (20. * log10 (x))

#define GAIN_FORMAT "+.02f dB"
#define PEAK_FORMAT ".06f"

#define VALID_GAIN(x) ((x) > -60.00 && (x) < 60.00)
#define VALID_PEAK(x) ((x) > 0.)

/* Same template caps as GstVolume, for I don't like having just ANY caps. */

#define FORMAT "{ "GST_AUDIO_NE(F32)","GST_AUDIO_NE(S16)" }"

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " FORMAT ", "
        "layout = (string) { interleaved, non-interleaved }, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]"));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " FORMAT ", "
        "layout = (string) { interleaved, non-interleaved }, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]"));

#define gst_rg_volume_parent_class parent_class
G_DEFINE_TYPE (GstRgVolume, gst_rg_volume, GST_TYPE_BIN);

static void gst_rg_volume_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rg_volume_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_rg_volume_dispose (GObject * object);

static GstStateChangeReturn gst_rg_volume_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_rg_volume_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

static GstEvent *gst_rg_volume_tag_event (GstRgVolume * self, GstEvent * event);
static void gst_rg_volume_reset (GstRgVolume * self);
static void gst_rg_volume_update_gain (GstRgVolume * self);
static inline void gst_rg_volume_determine_gain (GstRgVolume * self,
    gdouble * target_gain, gdouble * result_gain);

static void
gst_rg_volume_class_init (GstRgVolumeClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBinClass *bin_class;

  gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gst_rg_volume_set_property;
  gobject_class->get_property = gst_rg_volume_get_property;
  gobject_class->dispose = gst_rg_volume_dispose;

  /**
   * GstRgVolume:album-mode:
   *
   * Whether to prefer album gain over track gain.
   *
   * If set to %TRUE, use album gain instead of track gain if both are
   * available.  This keeps the relative loudness levels of tracks from the same
   * album intact.
   *
   * If set to %FALSE, track mode is used instead.  This effectively leads to
   * more extensive normalization.
   *
   * If album mode is enabled but the album gain tag is absent in the stream,
   * the track gain is used instead.  If both gain tags are missing, the value
   * of the #GstRgVolume:fallback-gain property is used instead.
   */
  g_object_class_install_property (gobject_class, PROP_ALBUM_MODE,
      g_param_spec_boolean ("album-mode", "Album mode",
          "Prefer album over track gain", DEFAULT_ALBUM_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRgVolume:headroom:
   *
   * Extra headroom [dB].  This controls the amount by which the output can
   * exceed digital full scale.
   *
   * Only set this to a value greater than 0.0 if signal compression/limiting of
   * a suitable form is applied to the output (or output is brought into the
   * correct range by some other transformation).
   *
   * This element internally uses a volume element, which also supports
   * operating on integer audio formats.  These formats do not allow exceeding
   * digital full scale.  If extra headroom is used, make sure that the raw
   * audio data format is floating point (F32).  Otherwise,
   * clipping distortion might be introduced as part of the volume adjustment
   * itself.
   */
  g_object_class_install_property (gobject_class, PROP_HEADROOM,
      g_param_spec_double ("headroom", "Headroom", "Extra headroom [dB]",
          0., 60., DEFAULT_HEADROOM,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRgVolume:pre-amp:
   *
   * Additional gain to apply globally [dB].  This controls the trade-off
   * between uniformity of normalization and utilization of available dynamic
   * range.
   *
   * Note that the default value is 0 dB because the ReplayGain reference value
   * was adjusted by +6 dB (from 83 to 89 dB). The original proposal stated
   * that a proper default pre-amp value is +6 dB, this translates to the used 0
   * dB.
   */
  g_object_class_install_property (gobject_class, PROP_PRE_AMP,
      g_param_spec_double ("pre-amp", "Pre-amp", "Extra gain [dB]",
          -60., 60., DEFAULT_PRE_AMP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRgVolume:fallback-gain:
   *
   * Fallback gain [dB] for streams missing ReplayGain tags.
   */
  g_object_class_install_property (gobject_class, PROP_FALLBACK_GAIN,
      g_param_spec_double ("fallback-gain", "Fallback gain",
          "Gain for streams missing tags [dB]",
          -60., 60., DEFAULT_FALLBACK_GAIN,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRgVolume:result-gain:
   *
   * Applied gain [dB].  This gain is applied to processed buffer data.
   *
   * This is set to the #GstRgVolume:target-gain if amplification by that amount
   * can be applied safely. "Safely" means that the volume adjustment does not
   * inflict clipping distortion.  Should this not be the case, the result gain
   * is set to an appropriately reduced value (by applying peak normalization).
   * The proposed standard calls this "clipping prevention".
   *
   * The difference between target and result gain reflects the necessary amount
   * of reduction.  Applications can make use of this information to temporarily
   * reduce the #GstRgVolume:pre-amp for subsequent streams, as recommended by
   * the ReplayGain standard.
   *
   * Note that target and result gain differing for a great majority of streams
   * indicates a problem: What happens in this case is that most streams receive
   * peak normalization instead of amplification by the ideal replay gain.  To
   * prevent this, the #GstRgVolume:pre-amp has to be lowered and/or a limiter
   * has to be used which facilitates the use of #GstRgVolume:headroom.
   */
  g_object_class_install_property (gobject_class, PROP_RESULT_GAIN,
      g_param_spec_double ("result-gain", "Result-gain", "Applied gain [dB]",
          -120., 120., 0., G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  /**
   * GstRgVolume:target-gain:
   *
   * Applicable gain [dB].  This gain is supposed to be applied.
   *
   * Depending on the value of the #GstRgVolume:album-mode property and the
   * presence of ReplayGain tags in the stream, this is set according to one of
   * these simple formulas:
   *
   *
   * * #GstRgVolume:pre-amp + album gain of the stream
   * * #GstRgVolume:pre-amp + track gain of the stream
   * * #GstRgVolume:pre-amp + #GstRgVolume:fallback-gain
   *
   */
  g_object_class_install_property (gobject_class, PROP_TARGET_GAIN,
      g_param_spec_double ("target-gain", "Target-gain",
          "Applicable gain [dB]", -120., 120., 0.,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  element_class = (GstElementClass *) klass;
  element_class->change_state = GST_DEBUG_FUNCPTR (gst_rg_volume_change_state);

  bin_class = (GstBinClass *) klass;
  /* Setting these to NULL makes gst_bin_add and _remove refuse to let anyone
   * mess with our internals. */
  bin_class->add_element = NULL;
  bin_class->remove_element = NULL;

  gst_element_class_add_static_pad_template (element_class, &src_template);
  gst_element_class_add_static_pad_template (element_class, &sink_template);
  gst_element_class_set_static_metadata (element_class, "ReplayGain volume",
      "Filter/Effect/Audio",
      "Apply ReplayGain volume adjustment",
      "Ren\xc3\xa9 Stadler <mail@renestadler.de>");

  GST_DEBUG_CATEGORY_INIT (gst_rg_volume_debug, "rgvolume", 0,
      "ReplayGain volume element");
}

static void
gst_rg_volume_init (GstRgVolume * self)
{
  GObjectClass *volume_class;
  GstPad *volume_pad, *ghost_pad;

  self->album_mode = DEFAULT_ALBUM_MODE;
  self->headroom = DEFAULT_HEADROOM;
  self->pre_amp = DEFAULT_PRE_AMP;
  self->fallback_gain = DEFAULT_FALLBACK_GAIN;
  self->target_gain = 0.0;
  self->result_gain = 0.0;

  self->volume_element = gst_element_factory_make ("volume", "rgvolume-volume");
  if (G_UNLIKELY (self->volume_element == NULL)) {
    GstMessage *msg;

    GST_WARNING_OBJECT (self, "could not create volume element");
    msg = gst_missing_element_message_new (GST_ELEMENT_CAST (self), "volume");
    gst_element_post_message (GST_ELEMENT_CAST (self), msg);

    /* Nothing else to do, we will refuse the state change from NULL to READY to
     * indicate that something went very wrong.  It is doubtful that someone
     * attempts changing our state though, since we end up having no pads! */
    return;
  }

  volume_class = G_OBJECT_GET_CLASS (G_OBJECT (self->volume_element));
  self->max_volume = G_PARAM_SPEC_DOUBLE
      (g_object_class_find_property (volume_class, "volume"))->maximum;

  GST_BIN_CLASS (parent_class)->add_element (GST_BIN_CAST (self),
      self->volume_element);

  volume_pad = gst_element_get_static_pad (self->volume_element, "sink");
  ghost_pad = gst_ghost_pad_new_from_template ("sink", volume_pad,
      GST_PAD_PAD_TEMPLATE (volume_pad));
  gst_object_unref (volume_pad);
  gst_pad_set_event_function (ghost_pad, gst_rg_volume_sink_event);
  gst_element_add_pad (GST_ELEMENT_CAST (self), ghost_pad);

  volume_pad = gst_element_get_static_pad (self->volume_element, "src");
  ghost_pad = gst_ghost_pad_new_from_template ("src", volume_pad,
      GST_PAD_PAD_TEMPLATE (volume_pad));
  gst_object_unref (volume_pad);
  gst_element_add_pad (GST_ELEMENT_CAST (self), ghost_pad);
}

static void
gst_rg_volume_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRgVolume *self = GST_RG_VOLUME (object);

  switch (prop_id) {
    case PROP_ALBUM_MODE:
      self->album_mode = g_value_get_boolean (value);
      break;
    case PROP_HEADROOM:
      self->headroom = g_value_get_double (value);
      break;
    case PROP_PRE_AMP:
      self->pre_amp = g_value_get_double (value);
      break;
    case PROP_FALLBACK_GAIN:
      self->fallback_gain = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  gst_rg_volume_update_gain (self);
}

static void
gst_rg_volume_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRgVolume *self = GST_RG_VOLUME (object);

  switch (prop_id) {
    case PROP_ALBUM_MODE:
      g_value_set_boolean (value, self->album_mode);
      break;
    case PROP_HEADROOM:
      g_value_set_double (value, self->headroom);
      break;
    case PROP_PRE_AMP:
      g_value_set_double (value, self->pre_amp);
      break;
    case PROP_FALLBACK_GAIN:
      g_value_set_double (value, self->fallback_gain);
      break;
    case PROP_TARGET_GAIN:
      g_value_set_double (value, self->target_gain);
      break;
    case PROP_RESULT_GAIN:
      g_value_set_double (value, self->result_gain);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rg_volume_dispose (GObject * object)
{
  GstRgVolume *self = GST_RG_VOLUME (object);

  if (self->volume_element != NULL) {
    /* Manually remove our child using the bin implementation of remove_element.
     * This is needed because we prevent gst_bin_remove from working, which the
     * parent dispose handler would use if we had any children left. */
    GST_BIN_CLASS (parent_class)->remove_element (GST_BIN_CAST (self),
        self->volume_element);
    self->volume_element = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static GstStateChangeReturn
gst_rg_volume_change_state (GstElement * element, GstStateChange transition)
{
  GstRgVolume *self = GST_RG_VOLUME (element);
  GstStateChangeReturn res;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:

      if (G_UNLIKELY (self->volume_element == NULL)) {
        /* Creating our child volume element in _init failed. */
        return GST_STATE_CHANGE_FAILURE;
      }
      break;

    case GST_STATE_CHANGE_READY_TO_PAUSED:

      gst_rg_volume_reset (self);
      break;

    default:
      break;
  }

  res = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  return res;
}

/* Event function for the ghost sink pad. */
static gboolean
gst_rg_volume_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstRgVolume *self;
  GstEvent *send_event = event;
  gboolean res;

  self = GST_RG_VOLUME (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:

      GST_LOG_OBJECT (self, "received tag event");

      send_event = gst_rg_volume_tag_event (self, event);

      if (send_event == NULL)
        GST_LOG_OBJECT (self, "all tags handled, dropping event");

      break;

    case GST_EVENT_EOS:

      gst_rg_volume_reset (self);
      break;

    default:
      break;
  }

  if (G_LIKELY (send_event != NULL))
    res = gst_pad_event_default (pad, parent, send_event);
  else
    res = TRUE;

  return res;
}

static GstEvent *
gst_rg_volume_tag_event (GstRgVolume * self, GstEvent * event)
{
  GstTagList *tag_list;
  gboolean has_track_gain, has_track_peak, has_album_gain, has_album_peak;
  gboolean has_ref_level;

  g_return_val_if_fail (event != NULL, NULL);
  g_return_val_if_fail (GST_EVENT_TYPE (event) == GST_EVENT_TAG, event);

  gst_event_parse_tag (event, &tag_list);

  if (gst_tag_list_is_empty (tag_list))
    return event;

  has_track_gain = gst_tag_list_get_double (tag_list, GST_TAG_TRACK_GAIN,
      &self->track_gain);
  has_track_peak = gst_tag_list_get_double (tag_list, GST_TAG_TRACK_PEAK,
      &self->track_peak);
  has_album_gain = gst_tag_list_get_double (tag_list, GST_TAG_ALBUM_GAIN,
      &self->album_gain);
  has_album_peak = gst_tag_list_get_double (tag_list, GST_TAG_ALBUM_PEAK,
      &self->album_peak);
  has_ref_level = gst_tag_list_get_double (tag_list, GST_TAG_REFERENCE_LEVEL,
      &self->reference_level);

  if (!has_track_gain && !has_track_peak && !has_album_gain && !has_album_peak)
    return event;

  if (has_ref_level && (has_track_gain || has_album_gain)
      && (ABS (self->reference_level - RG_REFERENCE_LEVEL) > 1.e-6)) {
    /* Log a message stating the amount of adjustment that is applied below. */
    GST_DEBUG_OBJECT (self,
        "compensating for reference level difference by %" GAIN_FORMAT,
        RG_REFERENCE_LEVEL - self->reference_level);
  }
  if (has_track_gain) {
    self->track_gain += RG_REFERENCE_LEVEL - self->reference_level;
  }
  if (has_album_gain) {
    self->album_gain += RG_REFERENCE_LEVEL - self->reference_level;
  }

  /* Ignore values that are obviously invalid. */
  if (G_UNLIKELY (has_track_gain && !VALID_GAIN (self->track_gain))) {
    GST_DEBUG_OBJECT (self,
        "ignoring bogus track gain value %" GAIN_FORMAT, self->track_gain);
    has_track_gain = FALSE;
  }
  if (G_UNLIKELY (has_track_peak && !VALID_PEAK (self->track_peak))) {
    GST_DEBUG_OBJECT (self,
        "ignoring bogus track peak value %" PEAK_FORMAT, self->track_peak);
    has_track_peak = FALSE;
  }
  if (G_UNLIKELY (has_album_gain && !VALID_GAIN (self->album_gain))) {
    GST_DEBUG_OBJECT (self,
        "ignoring bogus album gain value %" GAIN_FORMAT, self->album_gain);
    has_album_gain = FALSE;
  }
  if (G_UNLIKELY (has_album_peak && !VALID_PEAK (self->album_peak))) {
    GST_DEBUG_OBJECT (self,
        "ignoring bogus album peak value %" PEAK_FORMAT, self->album_peak);
    has_album_peak = FALSE;
  }

  /* Clamp peaks >1.0.  Float based decoders can produce spurious samples >1.0,
   * cutting these files back to 1.0 should not cause any audible distortion.
   * This is most often seen with Vorbis files. */
  if (has_track_peak && self->track_peak > 1.) {
    GST_DEBUG_OBJECT (self,
        "clamping track peak %" PEAK_FORMAT " to 1.0", self->track_peak);
    self->track_peak = 1.0;
  }
  if (has_album_peak && self->album_peak > 1.) {
    GST_DEBUG_OBJECT (self,
        "clamping album peak %" PEAK_FORMAT " to 1.0", self->album_peak);
    self->album_peak = 1.0;
  }

  self->has_track_gain |= has_track_gain;
  self->has_track_peak |= has_track_peak;
  self->has_album_gain |= has_album_gain;
  self->has_album_peak |= has_album_peak;

  tag_list = gst_tag_list_copy (tag_list);
  gst_tag_list_remove_tag (tag_list, GST_TAG_TRACK_GAIN);
  gst_tag_list_remove_tag (tag_list, GST_TAG_TRACK_PEAK);
  gst_tag_list_remove_tag (tag_list, GST_TAG_ALBUM_GAIN);
  gst_tag_list_remove_tag (tag_list, GST_TAG_ALBUM_PEAK);
  gst_tag_list_remove_tag (tag_list, GST_TAG_REFERENCE_LEVEL);

  gst_rg_volume_update_gain (self);

  gst_event_unref (event);
  if (gst_tag_list_is_empty (tag_list)) {
    gst_tag_list_unref (tag_list);
    return NULL;
  }

  return gst_event_new_tag (tag_list);
}

static void
gst_rg_volume_reset (GstRgVolume * self)
{
  self->has_track_gain = FALSE;
  self->has_track_peak = FALSE;
  self->has_album_gain = FALSE;
  self->has_album_peak = FALSE;

  self->reference_level = RG_REFERENCE_LEVEL;

  gst_rg_volume_update_gain (self);
}

static void
gst_rg_volume_update_gain (GstRgVolume * self)
{
  gdouble target_gain, result_gain, result_volume;
  gboolean target_gain_changed, result_gain_changed;

  gst_rg_volume_determine_gain (self, &target_gain, &result_gain);

  result_volume = DB_TO_LINEAR (result_gain);

  /* Ensure that the result volume is within the range that the volume element
   * can handle.  Currently, the limit is 10. (+20 dB), which should not be
   * restrictive. */
  if (G_UNLIKELY (result_volume > self->max_volume)) {
    GST_INFO_OBJECT (self,
        "cannot handle result gain of %" GAIN_FORMAT " (%0.6f), adjusting",
        result_gain, result_volume);

    result_volume = self->max_volume;
    result_gain = LINEAR_TO_DB (result_volume);
  }

  /* Direct comparison is OK in this case. */
  if (target_gain == result_gain) {
    GST_INFO_OBJECT (self,
        "result gain is %" GAIN_FORMAT " (%0.6f), matching target",
        result_gain, result_volume);
  } else {
    GST_INFO_OBJECT (self,
        "result gain is %" GAIN_FORMAT " (%0.6f), target is %" GAIN_FORMAT,
        result_gain, result_volume, target_gain);
  }

  target_gain_changed = (self->target_gain != target_gain);
  result_gain_changed = (self->result_gain != result_gain);

  self->target_gain = target_gain;
  self->result_gain = result_gain;

  g_object_set (self->volume_element, "volume", result_volume, NULL);

  if (target_gain_changed)
    g_object_notify ((GObject *) self, "target-gain");
  if (result_gain_changed)
    g_object_notify ((GObject *) self, "result-gain");
}

static inline void
gst_rg_volume_determine_gain (GstRgVolume * self, gdouble * target_gain,
    gdouble * result_gain)
{
  gdouble gain, peak;

  if (!self->has_track_gain && !self->has_album_gain) {

    GST_DEBUG_OBJECT (self, "using fallback gain");
    gain = self->fallback_gain;
    peak = 1.0;

  } else if ((self->album_mode && self->has_album_gain)
      || (!self->album_mode && !self->has_track_gain)) {

    gain = self->album_gain;
    if (G_LIKELY (self->has_album_peak)) {
      peak = self->album_peak;
    } else {
      GST_DEBUG_OBJECT (self, "album peak missing, assuming 1.0");
      peak = 1.0;
    }
    /* Falling back from track to album gain shouldn't really happen. */
    if (G_UNLIKELY (!self->album_mode))
      GST_INFO_OBJECT (self, "falling back to album gain");

  } else {
    /* !album_mode && !has_album_gain || album_mode && has_track_gain */

    gain = self->track_gain;
    if (G_LIKELY (self->has_track_peak)) {
      peak = self->track_peak;
    } else {
      GST_DEBUG_OBJECT (self, "track peak missing, assuming 1.0");
      peak = 1.0;
    }
    if (self->album_mode)
      GST_INFO_OBJECT (self, "falling back to track gain");
  }

  gain += self->pre_amp;

  *target_gain = gain;
  *result_gain = gain;

  if (LINEAR_TO_DB (peak) + gain > self->headroom) {
    *result_gain = LINEAR_TO_DB (1. / peak) + self->headroom;
  }
}
