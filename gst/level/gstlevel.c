/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2000,2001,2002,2003,2005
 *           Thomas Vander Stichele <thomas at apestaart dot org>
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
 * SECTION:element-level
 * @title: level
 *
 * Level analyses incoming audio buffers and, if the #GstLevel:message property
 * is %TRUE, generates an element message named
 * `level`: after each interval of time given by the #GstLevel:interval property.
 * The message's structure contains these fields:
 *
 * * #GstClockTime `timestamp`: the timestamp of the buffer that triggered the message.
 * * #GstClockTime `stream-time`: the stream time of the buffer.
 * * #GstClockTime `running-time`: the running_time of the buffer.
 * * #GstClockTime `duration`: the duration of the buffer.
 * * #GstClockTime `endtime`: the end time of the buffer that triggered the message as
 *   stream time (this is deprecated, as it can be calculated from stream-time + duration)
 * * #GValueArray of #gdouble `peak`: the peak power level in dB for each channel
 * * #GValueArray of #gdouble `decay`: the decaying peak power level in dB for each channel
 *   The decaying peak level follows the peak level, but starts dropping if no
 *   new peak is reached after the time given by the #GstLevel:peak-ttl.
 *   When the decaying peak level drops, it does so at the decay rate as
 *   specified by the #GstLevel:peak-falloff.
 * * #GValueArray of #gdouble `rms`: the Root Mean Square (or average power) level in dB
 *   for each channel
 *
 * ## Example application
 *
 * {{ tests/examples/level/level-example.c }}
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <gst/audio/audio.h>

#include "gstlevel.h"

GST_DEBUG_CATEGORY_STATIC (level_debug);
#define GST_CAT_DEFAULT level_debug

#define EPSILON 1e-35f

static GstStaticPadTemplate sink_template_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) { S8, " GST_AUDIO_NE (S16) ", " GST_AUDIO_NE (S32)
        ", " GST_AUDIO_NE (F32) "," GST_AUDIO_NE (F64) " },"
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]")
    );

static GstStaticPadTemplate src_template_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) { S8, " GST_AUDIO_NE (S16) ", " GST_AUDIO_NE (S32)
        ", " GST_AUDIO_NE (F32) "," GST_AUDIO_NE (F64) " },"
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]")
    );

enum
{
  PROP_0,
  PROP_POST_MESSAGES,
  PROP_MESSAGE,
  PROP_INTERVAL,
  PROP_PEAK_TTL,
  PROP_PEAK_FALLOFF
};

#define gst_level_parent_class parent_class
G_DEFINE_TYPE (GstLevel, gst_level, GST_TYPE_BASE_TRANSFORM);

static void gst_level_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_level_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_level_finalize (GObject * obj);

static gboolean gst_level_set_caps (GstBaseTransform * trans, GstCaps * in,
    GstCaps * out);
static gboolean gst_level_start (GstBaseTransform * trans);
static GstFlowReturn gst_level_transform_ip (GstBaseTransform * trans,
    GstBuffer * in);
static void gst_level_post_message (GstLevel * filter);
static gboolean gst_level_sink_event (GstBaseTransform * trans,
    GstEvent * event);
static void gst_level_recalc_interval_frames (GstLevel * level);

static void
gst_level_class_init (GstLevelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class->set_property = gst_level_set_property;
  gobject_class->get_property = gst_level_get_property;
  gobject_class->finalize = gst_level_finalize;

  /**
   * GstLevel:post-messages
   *
   * Post messages on the bus with level information.
   *
   * Since: 1.1.0
   */
  g_object_class_install_property (gobject_class, PROP_POST_MESSAGES,
      g_param_spec_boolean ("post-messages", "Post Messages",
          "Whether to post a 'level' element message on the bus for each "
          "passed interval", TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /* FIXME(2.0): remove this property */
  /**
   * GstLevel:post-messages
   *
   * Post messages on the bus with level information.
   *
   * Deprecated: use the #GstLevel:post-messages property
   */
#ifndef GST_REMOVE_DEPRECATED
  g_object_class_install_property (gobject_class, PROP_MESSAGE,
      g_param_spec_boolean ("message", "message",
          "Post a 'level' message for each passed interval "
          "(deprecated, use the post-messages property instead)", TRUE,
          G_PARAM_READWRITE | G_PARAM_DEPRECATED | G_PARAM_STATIC_STRINGS));
#endif
  g_object_class_install_property (gobject_class, PROP_INTERVAL,
      g_param_spec_uint64 ("interval", "Interval",
          "Interval of time between message posts (in nanoseconds)",
          1, G_MAXUINT64, GST_SECOND / 10,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PEAK_TTL,
      g_param_spec_uint64 ("peak-ttl", "Peak TTL",
          "Time To Live of decay peak before it falls back (in nanoseconds)",
          0, G_MAXUINT64, GST_SECOND / 10 * 3,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PEAK_FALLOFF,
      g_param_spec_double ("peak-falloff", "Peak Falloff",
          "Decay rate of decay peak after TTL (in dB/sec)",
          0.0, G_MAXDOUBLE, 10.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (level_debug, "level", 0, "Level calculation");

  gst_element_class_add_static_pad_template (element_class,
      &sink_template_factory);
  gst_element_class_add_static_pad_template (element_class,
      &src_template_factory);
  gst_element_class_set_static_metadata (element_class, "Level",
      "Filter/Analyzer/Audio",
      "RMS/Peak/Decaying Peak Level messager for audio/raw",
      "Thomas Vander Stichele <thomas at apestaart dot org>");

  trans_class->set_caps = GST_DEBUG_FUNCPTR (gst_level_set_caps);
  trans_class->start = GST_DEBUG_FUNCPTR (gst_level_start);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_level_transform_ip);
  trans_class->sink_event = GST_DEBUG_FUNCPTR (gst_level_sink_event);
  trans_class->passthrough_on_same_caps = TRUE;
}

static void
gst_level_init (GstLevel * filter)
{
  filter->CS = NULL;
  filter->peak = NULL;
  filter->last_peak = NULL;
  filter->decay_peak = NULL;
  filter->decay_peak_base = NULL;
  filter->decay_peak_age = NULL;

  gst_audio_info_init (&filter->info);

  filter->interval = GST_SECOND / 10;
  filter->decay_peak_ttl = GST_SECOND / 10 * 3;
  filter->decay_peak_falloff = 10.0;    /* dB falloff (/sec) */

  filter->post_messages = TRUE;

  filter->process = NULL;

  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (filter), TRUE);
}

static void
gst_level_finalize (GObject * obj)
{
  GstLevel *filter = GST_LEVEL (obj);

  g_free (filter->CS);
  g_free (filter->peak);
  g_free (filter->last_peak);
  g_free (filter->decay_peak);
  g_free (filter->decay_peak_base);
  g_free (filter->decay_peak_age);

  filter->CS = NULL;
  filter->peak = NULL;
  filter->last_peak = NULL;
  filter->decay_peak = NULL;
  filter->decay_peak_base = NULL;
  filter->decay_peak_age = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_level_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstLevel *filter = GST_LEVEL (object);

  switch (prop_id) {
    case PROP_POST_MESSAGES:
      /* fall-through */
    case PROP_MESSAGE:
      filter->post_messages = g_value_get_boolean (value);
      break;
    case PROP_INTERVAL:
      filter->interval = g_value_get_uint64 (value);
      /* Not exactly thread-safe, but property does not advertise that it
       * can be changed at runtime anyway */
      if (GST_AUDIO_INFO_RATE (&filter->info)) {
        gst_level_recalc_interval_frames (filter);
      }
      break;
    case PROP_PEAK_TTL:
      filter->decay_peak_ttl =
          gst_guint64_to_gdouble (g_value_get_uint64 (value));
      break;
    case PROP_PEAK_FALLOFF:
      filter->decay_peak_falloff = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_level_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstLevel *filter = GST_LEVEL (object);

  switch (prop_id) {
    case PROP_POST_MESSAGES:
      /* fall-through */
    case PROP_MESSAGE:
      g_value_set_boolean (value, filter->post_messages);
      break;
    case PROP_INTERVAL:
      g_value_set_uint64 (value, filter->interval);
      break;
    case PROP_PEAK_TTL:
      g_value_set_uint64 (value, filter->decay_peak_ttl);
      break;
    case PROP_PEAK_FALLOFF:
      g_value_set_double (value, filter->decay_peak_falloff);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


/* process one (interleaved) channel of incoming samples
 * calculate square sum of samples
 * normalize and average over number of samples
 * returns a normalized cumulative square value, which can be averaged
 * to return the average power as a double between 0 and 1
 * also returns the normalized peak power (square of the highest amplitude)
 *
 * caller must assure num is a multiple of channels
 * samples for multiple channels are interleaved
 * input sample data enters in *in_data and is not modified
 * this filter only accepts signed audio data, so mid level is always 0
 *
 * for integers, this code considers the non-existent positive max value to be
 * full-scale; so max-1 will not map to 1.0
 */

#define DEFINE_INT_LEVEL_CALCULATOR(TYPE, RESOLUTION)                         \
static void inline                                                            \
gst_level_calculate_##TYPE (gpointer data, guint num, guint channels,         \
                            gdouble *NCS, gdouble *NPS)                       \
{                                                                             \
  TYPE * in = (TYPE *)data;                                                   \
  register guint j;                                                           \
  gdouble squaresum = 0.0;           /* square sum of the input samples */    \
  register gdouble square = 0.0;     /* Square */                             \
  register gdouble peaksquare = 0.0; /* Peak Square Sample */                 \
  gdouble normalizer;                /* divisor to get a [-1.0, 1.0] range */ \
                                                                              \
  /* *NCS = 0.0; Normalized Cumulative Square */                              \
  /* *NPS = 0.0; Normalized Peak Square */                                    \
                                                                              \
  for (j = 0; j < num; j += channels) {                                       \
    square = ((gdouble) in[j]) * in[j];                                       \
    if (square > peaksquare) peaksquare = square;                             \
    squaresum += square;                                                      \
  }                                                                           \
                                                                              \
  normalizer = (gdouble) (G_GINT64_CONSTANT(1) << (RESOLUTION * 2));          \
  *NCS = squaresum / normalizer;                                              \
  *NPS = peaksquare / normalizer;                                             \
}

DEFINE_INT_LEVEL_CALCULATOR (gint32, 31);
DEFINE_INT_LEVEL_CALCULATOR (gint16, 15);
DEFINE_INT_LEVEL_CALCULATOR (gint8, 7);

/* FIXME: use orc to calculate squaresums? */
#define DEFINE_FLOAT_LEVEL_CALCULATOR(TYPE)                                   \
static void inline                                                            \
gst_level_calculate_##TYPE (gpointer data, guint num, guint channels,         \
                            gdouble *NCS, gdouble *NPS)                       \
{                                                                             \
  TYPE * in = (TYPE *)data;                                                   \
  register guint j;                                                           \
  gdouble squaresum = 0.0;           /* square sum of the input samples */    \
  register gdouble square = 0.0;     /* Square */                             \
  register gdouble peaksquare = 0.0; /* Peak Square Sample */                 \
                                                                              \
  /* *NCS = 0.0; Normalized Cumulative Square */                              \
  /* *NPS = 0.0; Normalized Peak Square */                                    \
                                                                              \
  /* orc_level_squaresum_f64(&squaresum,in,num); */                           \
  for (j = 0; j < num; j += channels) {                                       \
    square = ((gdouble) in[j]) * in[j];                                       \
    if (square > peaksquare) peaksquare = square;                             \
    squaresum += square;                                                      \
  }                                                                           \
                                                                              \
  *NCS = squaresum;                                                           \
  *NPS = peaksquare;                                                          \
}

DEFINE_FLOAT_LEVEL_CALCULATOR (gfloat);
DEFINE_FLOAT_LEVEL_CALCULATOR (gdouble);

/* we would need stride to deinterleave also
static void inline
gst_level_calculate_gdouble (gpointer data, guint num, guint channels,
                            gdouble *NCS, gdouble *NPS)
{
  orc_level_squaresum_f64(NCS,(gdouble *)data,num);
  *NPS = 0.0;
}
*/

static void
gst_level_recalc_interval_frames (GstLevel * level)
{
  GstClockTime interval = level->interval;
  guint sample_rate = GST_AUDIO_INFO_RATE (&level->info);
  guint interval_frames;

  interval_frames = GST_CLOCK_TIME_TO_FRAMES (interval, sample_rate);

  if (interval_frames == 0) {
    GST_WARNING_OBJECT (level, "interval %" GST_TIME_FORMAT " is too small, "
        "should be at least %" GST_TIME_FORMAT " for sample rate %u",
        GST_TIME_ARGS (interval),
        GST_TIME_ARGS (GST_FRAMES_TO_CLOCK_TIME (1, sample_rate)), sample_rate);
    interval_frames = 1;
  }

  level->interval_frames = interval_frames;

  GST_INFO_OBJECT (level, "interval_frames now %u for interval "
      "%" GST_TIME_FORMAT " and sample rate %u", interval_frames,
      GST_TIME_ARGS (interval), sample_rate);
}

static gboolean
gst_level_set_caps (GstBaseTransform * trans, GstCaps * in, GstCaps * out)
{
  GstLevel *filter = GST_LEVEL (trans);
  GstAudioInfo info;
  gint i, channels;

  if (!gst_audio_info_from_caps (&info, in))
    return FALSE;

  switch (GST_AUDIO_INFO_FORMAT (&info)) {
    case GST_AUDIO_FORMAT_S8:
      filter->process = gst_level_calculate_gint8;
      break;
    case GST_AUDIO_FORMAT_S16:
      filter->process = gst_level_calculate_gint16;
      break;
    case GST_AUDIO_FORMAT_S32:
      filter->process = gst_level_calculate_gint32;
      break;
    case GST_AUDIO_FORMAT_F32:
      filter->process = gst_level_calculate_gfloat;
      break;
    case GST_AUDIO_FORMAT_F64:
      filter->process = gst_level_calculate_gdouble;
      break;
    default:
      filter->process = NULL;
      break;
  }

  filter->info = info;

  channels = GST_AUDIO_INFO_CHANNELS (&info);

  /* allocate channel variable arrays */
  g_free (filter->CS);
  g_free (filter->peak);
  g_free (filter->last_peak);
  g_free (filter->decay_peak);
  g_free (filter->decay_peak_base);
  g_free (filter->decay_peak_age);
  filter->CS = g_new (gdouble, channels);
  filter->peak = g_new (gdouble, channels);
  filter->last_peak = g_new (gdouble, channels);
  filter->decay_peak = g_new (gdouble, channels);
  filter->decay_peak_base = g_new (gdouble, channels);

  filter->decay_peak_age = g_new (GstClockTime, channels);

  for (i = 0; i < channels; ++i) {
    filter->CS[i] = filter->peak[i] = filter->last_peak[i] =
        filter->decay_peak[i] = filter->decay_peak_base[i] = 0.0;
    filter->decay_peak_age[i] = G_GUINT64_CONSTANT (0);
  }

  gst_level_recalc_interval_frames (filter);

  return TRUE;
}

static gboolean
gst_level_start (GstBaseTransform * trans)
{
  GstLevel *filter = GST_LEVEL (trans);

  filter->num_frames = 0;
  filter->message_ts = GST_CLOCK_TIME_NONE;

  return TRUE;
}

static GstMessage *
gst_level_message_new (GstLevel * level, GstClockTime timestamp,
    GstClockTime duration)
{
  GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (level);
  GstStructure *s;
  GValue v = { 0, };
  GstClockTime endtime, running_time, stream_time;

  running_time = gst_segment_to_running_time (&trans->segment, GST_FORMAT_TIME,
      timestamp);
  stream_time = gst_segment_to_stream_time (&trans->segment, GST_FORMAT_TIME,
      timestamp);
  /* endtime is for backwards compatibility */
  endtime = stream_time + duration;

  s = gst_structure_new ("level",
      "endtime", GST_TYPE_CLOCK_TIME, endtime,
      "timestamp", G_TYPE_UINT64, timestamp,
      "stream-time", G_TYPE_UINT64, stream_time,
      "running-time", G_TYPE_UINT64, running_time,
      "duration", G_TYPE_UINT64, duration, NULL);

  g_value_init (&v, G_TYPE_VALUE_ARRAY);
  g_value_take_boxed (&v, g_value_array_new (0));
  gst_structure_take_value (s, "rms", &v);

  g_value_init (&v, G_TYPE_VALUE_ARRAY);
  g_value_take_boxed (&v, g_value_array_new (0));
  gst_structure_take_value (s, "peak", &v);

  g_value_init (&v, G_TYPE_VALUE_ARRAY);
  g_value_take_boxed (&v, g_value_array_new (0));
  gst_structure_take_value (s, "decay", &v);

  return gst_message_new_element (GST_OBJECT (level), s);
}

static void
gst_level_message_append_channel (GstMessage * m, gdouble rms, gdouble peak,
    gdouble decay)
{
  const GValue *array_val;
  GstStructure *s;
  GValueArray *arr;
  GValue v = { 0, };

  g_value_init (&v, G_TYPE_DOUBLE);

  s = (GstStructure *) gst_message_get_structure (m);

  array_val = gst_structure_get_value (s, "rms");
  arr = (GValueArray *) g_value_get_boxed (array_val);
  g_value_set_double (&v, rms);
  g_value_array_append (arr, &v);       /* copies by value */

  array_val = gst_structure_get_value (s, "peak");
  arr = (GValueArray *) g_value_get_boxed (array_val);
  g_value_set_double (&v, peak);
  g_value_array_append (arr, &v);       /* copies by value */

  array_val = gst_structure_get_value (s, "decay");
  arr = (GValueArray *) g_value_get_boxed (array_val);
  g_value_set_double (&v, decay);
  g_value_array_append (arr, &v);       /* copies by value */

  g_value_unset (&v);
}

static GstFlowReturn
gst_level_transform_ip (GstBaseTransform * trans, GstBuffer * in)
{
  GstLevel *filter;
  GstMapInfo map;
  guint8 *in_data;
  gsize in_size;
  gdouble CS;
  guint i;
  guint num_frames;
  guint num_int_samples = 0;    /* number of interleaved samples
                                 * ie. total count for all channels combined */
  guint block_size, block_int_size;     /* we subdivide buffers to not skip message
                                         * intervals */
  GstClockTimeDiff falloff_time;
  gint channels, rate, bps;

  filter = GST_LEVEL (trans);

  channels = GST_AUDIO_INFO_CHANNELS (&filter->info);
  bps = GST_AUDIO_INFO_BPS (&filter->info);
  rate = GST_AUDIO_INFO_RATE (&filter->info);

  gst_buffer_map (in, &map, GST_MAP_READ);
  in_data = map.data;
  in_size = map.size;

  num_int_samples = in_size / bps;

  GST_LOG_OBJECT (filter, "analyzing %u sample frames at ts %" GST_TIME_FORMAT,
      num_int_samples, GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (in)));

  g_return_val_if_fail (num_int_samples % channels == 0, GST_FLOW_ERROR);

  if (GST_BUFFER_FLAG_IS_SET (in, GST_BUFFER_FLAG_DISCONT)) {
    filter->message_ts = GST_BUFFER_TIMESTAMP (in);
    filter->num_frames = 0;
  }
  if (G_UNLIKELY (!GST_CLOCK_TIME_IS_VALID (filter->message_ts))) {
    filter->message_ts = GST_BUFFER_TIMESTAMP (in);
  }

  num_frames = num_int_samples / channels;
  while (num_frames > 0) {
    block_size = filter->interval_frames - filter->num_frames;
    block_size = MIN (block_size, num_frames);
    block_int_size = block_size * channels;

    for (i = 0; i < channels; ++i) {
      if (!GST_BUFFER_FLAG_IS_SET (in, GST_BUFFER_FLAG_GAP)) {
        filter->process (in_data + (bps * i), block_int_size, channels, &CS,
            &filter->peak[i]);
        GST_LOG_OBJECT (filter,
            "[%d]: cumulative squares %lf, over %d samples/%d channels",
            i, CS, block_int_size, channels);
        filter->CS[i] += CS;
      } else {
        filter->peak[i] = 0.0;
      }

      filter->decay_peak_age[i] += GST_FRAMES_TO_CLOCK_TIME (num_frames, rate);
      GST_LOG_OBJECT (filter,
          "[%d]: peak %f, last peak %f, decay peak %f, age %" GST_TIME_FORMAT,
          i, filter->peak[i], filter->last_peak[i], filter->decay_peak[i],
          GST_TIME_ARGS (filter->decay_peak_age[i]));

      /* update running peak */
      if (filter->peak[i] > filter->last_peak[i])
        filter->last_peak[i] = filter->peak[i];

      /* make decay peak fall off if too old */
      falloff_time =
          GST_CLOCK_DIFF (gst_gdouble_to_guint64 (filter->decay_peak_ttl),
          filter->decay_peak_age[i]);
      if (falloff_time > 0) {
        gdouble falloff_dB;
        gdouble falloff;
        gdouble length;         /* length of falloff time in seconds */

        length = (gdouble) falloff_time / (gdouble) GST_SECOND;
        falloff_dB = filter->decay_peak_falloff * length;
        falloff = pow (10, falloff_dB / -20.0);

        GST_LOG_OBJECT (filter,
            "falloff: current %f, base %f, interval %" GST_TIME_FORMAT
            ", dB falloff %f, factor %e",
            filter->decay_peak[i], filter->decay_peak_base[i],
            GST_TIME_ARGS (falloff_time), falloff_dB, falloff);
        filter->decay_peak[i] = filter->decay_peak_base[i] * falloff;
        GST_LOG_OBJECT (filter,
            "peak is %" GST_TIME_FORMAT " old, decayed with factor %e to %f",
            GST_TIME_ARGS (filter->decay_peak_age[i]), falloff,
            filter->decay_peak[i]);
      } else {
        GST_LOG_OBJECT (filter, "peak not old enough, not decaying");
      }

      /* if the peak of this run is higher, the decay peak gets reset */
      if (filter->peak[i] >= filter->decay_peak[i]) {
        GST_LOG_OBJECT (filter, "new peak, %f", filter->peak[i]);
        filter->decay_peak[i] = filter->peak[i];
        filter->decay_peak_base[i] = filter->peak[i];
        filter->decay_peak_age[i] = G_GINT64_CONSTANT (0);
      }
    }
    in_data += block_size * bps * channels;

    filter->num_frames += block_size;
    num_frames -= block_size;

    /* do we need to message ? */
    if (filter->num_frames >= filter->interval_frames) {
      gst_level_post_message (filter);
    }
  }

  gst_buffer_unmap (in, &map);

  return GST_FLOW_OK;
}

static void
gst_level_post_message (GstLevel * filter)
{
  guint i;
  gint channels, rate, frames = filter->num_frames;
  GstClockTime duration;

  channels = GST_AUDIO_INFO_CHANNELS (&filter->info);
  rate = GST_AUDIO_INFO_RATE (&filter->info);
  duration = GST_FRAMES_TO_CLOCK_TIME (frames, rate);

  if (filter->post_messages) {
    GstMessage *m =
        gst_level_message_new (filter, filter->message_ts, duration);

    GST_LOG_OBJECT (filter,
        "message: ts %" GST_TIME_FORMAT ", duration %" GST_TIME_FORMAT
        ", num_frames %d", GST_TIME_ARGS (filter->message_ts),
        GST_TIME_ARGS (duration), frames);

    for (i = 0; i < channels; ++i) {
      gdouble RMS;
      gdouble RMSdB, peakdB, decaydB;

      RMS = sqrt (filter->CS[i] / frames);
      GST_LOG_OBJECT (filter,
          "message: channel %d, CS %f, RMS %f", i, filter->CS[i], RMS);
      GST_LOG_OBJECT (filter,
          "message: last_peak: %f, decay_peak: %f",
          filter->last_peak[i], filter->decay_peak[i]);
      /* RMS values are calculated in amplitude, so 20 * log 10 */
      RMSdB = 20 * log10 (RMS + EPSILON);
      /* peak values are square sums, ie. power, so 10 * log 10 */
      peakdB = 10 * log10 (filter->last_peak[i] + EPSILON);
      decaydB = 10 * log10 (filter->decay_peak[i] + EPSILON);

      if (filter->decay_peak[i] < filter->last_peak[i]) {
        /* this can happen in certain cases, for example when
         * the last peak is between decay_peak and decay_peak_base */
        GST_DEBUG_OBJECT (filter,
            "message: decay peak dB %f smaller than last peak dB %f, copying",
            decaydB, peakdB);
        filter->decay_peak[i] = filter->last_peak[i];
      }
      GST_LOG_OBJECT (filter,
          "message: RMS %f dB, peak %f dB, decay %f dB",
          RMSdB, peakdB, decaydB);

      gst_level_message_append_channel (m, RMSdB, peakdB, decaydB);

      /* reset cumulative and normal peak */
      filter->CS[i] = 0.0;
      filter->last_peak[i] = 0.0;
    }

    gst_element_post_message (GST_ELEMENT (filter), m);

  }
  filter->num_frames -= frames;
  filter->message_ts += duration;
}


static gboolean
gst_level_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  if (GST_EVENT_TYPE (event) == GST_EVENT_EOS) {
    GstLevel *filter = GST_LEVEL (trans);

    gst_level_post_message (filter);
  }

  return GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (trans, event);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "level", GST_RANK_NONE, GST_TYPE_LEVEL);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    level,
    "Audio level plugin",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
