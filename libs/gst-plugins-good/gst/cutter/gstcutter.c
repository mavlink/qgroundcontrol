/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2002,2003,2005
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
 * SECTION:element-cutter
 * @title: cutter
 *
 * Analyses the audio signal for periods of silence. The start and end of
 * silence is signalled by bus messages named
 * `cutter`.
 *
 * The message's structure contains two fields:
 *
 * * #GstClockTime `timestamp`: the timestamp of the buffer that triggered the message.
 * * gboolean `above`: %TRUE for begin of silence and %FALSE for end of silence.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -m filesrc location=foo.ogg ! decodebin ! audioconvert ! cutter ! autoaudiosink
 * ]| Show cut messages.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include "gstcutter.h"
#include "math.h"

GST_DEBUG_CATEGORY_STATIC (cutter_debug);
#define GST_CAT_DEFAULT cutter_debug

#define CUTTER_DEFAULT_THRESHOLD_LEVEL    0.1
#define CUTTER_DEFAULT_THRESHOLD_LENGTH  (500 * GST_MSECOND)
#define CUTTER_DEFAULT_PRE_LENGTH        (200 * GST_MSECOND)

static GstStaticPadTemplate cutter_src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) { S8," GST_AUDIO_NE (S16) " }, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ], "
        "layout = (string) interleaved")
    );

static GstStaticPadTemplate cutter_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) { S8," GST_AUDIO_NE (S16) " }, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ], "
        "layout = (string) interleaved")
    );

enum
{
  PROP_0,
  PROP_THRESHOLD,
  PROP_THRESHOLD_DB,
  PROP_RUN_LENGTH,
  PROP_PRE_LENGTH,
  PROP_LEAKY
};

#define gst_cutter_parent_class parent_class
G_DEFINE_TYPE (GstCutter, gst_cutter, GST_TYPE_ELEMENT);

static GstStateChangeReturn
gst_cutter_change_state (GstElement * element, GstStateChange transition);

static void gst_cutter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_cutter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_cutter_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstFlowReturn gst_cutter_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

static void
gst_cutter_class_init (GstCutterClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_cutter_set_property;
  gobject_class->get_property = gst_cutter_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_THRESHOLD,
      g_param_spec_double ("threshold", "Threshold",
          "Volume threshold before trigger",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_THRESHOLD_DB,
      g_param_spec_double ("threshold-dB", "Threshold (dB)",
          "Volume threshold before trigger (in dB)",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_RUN_LENGTH,
      g_param_spec_uint64 ("run-length", "Run length",
          "Length of drop below threshold before cut_stop (in nanoseconds)",
          0, G_MAXUINT64, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PRE_LENGTH,
      g_param_spec_uint64 ("pre-length", "Pre-recording buffer length",
          "Length of pre-recording buffer (in nanoseconds)",
          0, G_MAXUINT64, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_LEAKY,
      g_param_spec_boolean ("leaky", "Leaky",
          "do we leak buffers when below threshold ?",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (cutter_debug, "cutter", 0, "Audio cutting");

  gst_element_class_add_static_pad_template (element_class,
      &cutter_src_factory);
  gst_element_class_add_static_pad_template (element_class,
      &cutter_sink_factory);
  gst_element_class_set_static_metadata (element_class, "Audio cutter",
      "Filter/Editor/Audio", "Audio Cutter to split audio into non-silent bits",
      "Thomas Vander Stichele <thomas at apestaart dot org>");
  element_class->change_state = gst_cutter_change_state;
}

static void
gst_cutter_init (GstCutter * filter)
{
  filter->sinkpad =
      gst_pad_new_from_static_template (&cutter_sink_factory, "sink");
  gst_pad_set_chain_function (filter->sinkpad, gst_cutter_chain);
  gst_pad_set_event_function (filter->sinkpad, gst_cutter_event);
  gst_pad_use_fixed_caps (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad =
      gst_pad_new_from_static_template (&cutter_src_factory, "src");
  gst_pad_use_fixed_caps (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->threshold_level = CUTTER_DEFAULT_THRESHOLD_LEVEL;
  filter->threshold_length = CUTTER_DEFAULT_THRESHOLD_LENGTH;
  filter->silent_run_length = 0 * GST_SECOND;
  filter->silent = TRUE;
  filter->silent_prev = FALSE;  /* previous value of silent */

  filter->pre_length = CUTTER_DEFAULT_PRE_LENGTH;
  filter->pre_run_length = 0 * GST_SECOND;
  filter->pre_buffer = NULL;
  filter->leaky = FALSE;
}

static GstMessage *
gst_cutter_message_new (GstCutter * c, gboolean above, GstClockTime timestamp)
{
  GstStructure *s;

  s = gst_structure_new ("cutter",
      "above", G_TYPE_BOOLEAN, above,
      "timestamp", GST_TYPE_CLOCK_TIME, timestamp, NULL);

  return gst_message_new_element (GST_OBJECT (c), s);
}

/* Calculate the Normalized Cumulative Square over a buffer of the given type
 * and over all channels combined */

#define DEFINE_CUTTER_CALCULATOR(TYPE, RESOLUTION)                            \
static void inline                                                            \
gst_cutter_calculate_##TYPE (TYPE * in, guint num,                            \
                            double *NCS)                                      \
{                                                                             \
  register int j;                                                             \
  double squaresum = 0.0;           /* square sum of the integer samples */   \
  register double square = 0.0;     /* Square */                              \
  gdouble normalizer;               /* divisor to get a [-1.0, 1.0] range */  \
                                                                              \
  *NCS = 0.0;                       /* Normalized Cumulative Square */        \
                                                                              \
  normalizer = (double) (1 << (RESOLUTION * 2));                              \
                                                                              \
  for (j = 0; j < num; j++)                                                   \
  {                                                                           \
    square = ((double) in[j]) * in[j];                                        \
    squaresum += square;                                                      \
  }                                                                           \
                                                                              \
                                                                              \
  *NCS = squaresum / normalizer;                                              \
}

DEFINE_CUTTER_CALCULATOR (gint16, 15);
DEFINE_CUTTER_CALCULATOR (gint8, 7);

static gboolean
gst_cutter_setcaps (GstCutter * filter, GstCaps * caps)
{
  GstAudioInfo info;

  if (!gst_audio_info_from_caps (&info, caps))
    return FALSE;

  filter->info = info;

  return gst_pad_set_caps (filter->srcpad, caps);
}

static GstStateChangeReturn
gst_cutter_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstCutter *filter = GST_CUTTER (element);

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      g_list_free_full (filter->pre_buffer, (GDestroyNotify) gst_buffer_unref);
      filter->pre_buffer = NULL;
      break;
    default:
      break;
  }
  return ret;
}

static gboolean
gst_cutter_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean ret;
  GstCutter *filter;

  filter = GST_CUTTER (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      ret = gst_cutter_setcaps (filter, caps);
      gst_event_unref (event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

static GstFlowReturn
gst_cutter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstCutter *filter;
  GstMapInfo map;
  gint16 *in_data;
  gint bpf, rate;
  gsize in_size;
  guint num_samples;
  gdouble NCS = 0.0;            /* Normalized Cumulative Square of buffer */
  gdouble RMS = 0.0;            /* RMS of signal in buffer */
  gdouble NMS = 0.0;            /* Normalized Mean Square of buffer */
  GstBuffer *prebuf;            /* pointer to a prebuffer element */
  GstClockTime duration;

  filter = GST_CUTTER (parent);

  if (GST_AUDIO_INFO_FORMAT (&filter->info) == GST_AUDIO_FORMAT_UNKNOWN)
    goto not_negotiated;

  bpf = GST_AUDIO_INFO_BPF (&filter->info);
  rate = GST_AUDIO_INFO_RATE (&filter->info);

  gst_buffer_map (buf, &map, GST_MAP_READ);
  in_data = (gint16 *) map.data;
  in_size = map.size;

  GST_LOG_OBJECT (filter, "length of prerec buffer: %" GST_TIME_FORMAT,
      GST_TIME_ARGS (filter->pre_run_length));

  /* calculate mean square value on buffer */
  switch (GST_AUDIO_INFO_FORMAT (&filter->info)) {
    case GST_AUDIO_FORMAT_S16:
      num_samples = in_size / 2;
      gst_cutter_calculate_gint16 (in_data, num_samples, &NCS);
      NMS = NCS / num_samples;
      break;
    case GST_AUDIO_FORMAT_S8:
      num_samples = in_size;
      gst_cutter_calculate_gint8 ((gint8 *) in_data, num_samples, &NCS);
      NMS = NCS / num_samples;
      break;
    default:
      /* this shouldn't happen */
      g_warning ("no mean square function for format");
      break;
  }

  gst_buffer_unmap (buf, &map);

  filter->silent_prev = filter->silent;

  duration = gst_util_uint64_scale (in_size / bpf, GST_SECOND, rate);

  RMS = sqrt (NMS);
  /* if RMS below threshold, add buffer length to silent run length count
   * if not, reset
   */
  GST_LOG_OBJECT (filter, "buffer stats: NMS %f, RMS %f, audio length %f", NMS,
      RMS, gst_guint64_to_gdouble (duration));

  if (RMS < filter->threshold_level)
    filter->silent_run_length += gst_guint64_to_gdouble (duration);
  else {
    filter->silent_run_length = 0 * GST_SECOND;
    filter->silent = FALSE;
  }

  if (filter->silent_run_length > filter->threshold_length)
    /* it has been silent long enough, flag it */
    filter->silent = TRUE;

  /* has the silent status changed ? if so, send right signal
   * and, if from silent -> not silent, flush pre_record buffer
   */
  if (filter->silent != filter->silent_prev) {
    if (filter->silent) {
      GstMessage *m =
          gst_cutter_message_new (filter, FALSE, GST_BUFFER_TIMESTAMP (buf));
      GST_DEBUG_OBJECT (filter, "signaling CUT_STOP");
      gst_element_post_message (GST_ELEMENT (filter), m);
    } else {
      gint count = 0;
      GstMessage *m =
          gst_cutter_message_new (filter, TRUE, GST_BUFFER_TIMESTAMP (buf));

      GST_DEBUG_OBJECT (filter, "signaling CUT_START");
      gst_element_post_message (GST_ELEMENT (filter), m);
      /* first of all, flush current buffer */
      GST_DEBUG_OBJECT (filter, "flushing buffer of length %" GST_TIME_FORMAT,
          GST_TIME_ARGS (filter->pre_run_length));

      while (filter->pre_buffer) {
        prebuf = (g_list_first (filter->pre_buffer))->data;
        filter->pre_buffer = g_list_remove (filter->pre_buffer, prebuf);
        gst_pad_push (filter->srcpad, prebuf);
        ++count;
      }
      GST_DEBUG_OBJECT (filter, "flushed %d buffers", count);
      filter->pre_run_length = 0 * GST_SECOND;
    }
  }
  /* now check if we have to send the new buffer to the internal buffer cache
   * or to the srcpad */
  if (filter->silent) {
    filter->pre_buffer = g_list_append (filter->pre_buffer, buf);
    filter->pre_run_length += gst_guint64_to_gdouble (duration);

    while (filter->pre_run_length > filter->pre_length) {
      GstClockTime pduration;
      gsize psize;

      prebuf = (g_list_first (filter->pre_buffer))->data;
      g_assert (GST_IS_BUFFER (prebuf));

      psize = gst_buffer_get_size (prebuf);
      pduration = gst_util_uint64_scale (psize / bpf, GST_SECOND, rate);

      filter->pre_buffer = g_list_remove (filter->pre_buffer, prebuf);
      filter->pre_run_length -= gst_guint64_to_gdouble (pduration);

      /* only pass buffers if we don't leak */
      if (!filter->leaky)
        ret = gst_pad_push (filter->srcpad, prebuf);
      else
        gst_buffer_unref (prebuf);
    }
  } else
    ret = gst_pad_push (filter->srcpad, buf);

  return ret;

  /* ERRORS */
not_negotiated:
  {
    return GST_FLOW_NOT_NEGOTIATED;
  }
}

static void
gst_cutter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCutter *filter;

  g_return_if_fail (GST_IS_CUTTER (object));
  filter = GST_CUTTER (object);

  switch (prop_id) {
    case PROP_THRESHOLD:
      filter->threshold_level = g_value_get_double (value);
      GST_DEBUG ("DEBUG: set threshold level to %f", filter->threshold_level);
      break;
    case PROP_THRESHOLD_DB:
      /* set the level given in dB
       * value in dB = 20 * log (value)
       * values in dB < 0 result in values between 0 and 1
       */
      filter->threshold_level = pow (10, g_value_get_double (value) / 20);
      GST_DEBUG_OBJECT (filter, "set threshold level to %f",
          filter->threshold_level);
      break;
    case PROP_RUN_LENGTH:
      /* set the minimum length of the silent run required */
      filter->threshold_length =
          gst_guint64_to_gdouble (g_value_get_uint64 (value));
      break;
    case PROP_PRE_LENGTH:
      /* set the length of the pre-record block */
      filter->pre_length = gst_guint64_to_gdouble (g_value_get_uint64 (value));
      break;
    case PROP_LEAKY:
      /* set if the pre-record buffer is leaky or not */
      filter->leaky = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_cutter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCutter *filter;

  g_return_if_fail (GST_IS_CUTTER (object));
  filter = GST_CUTTER (object);

  switch (prop_id) {
    case PROP_RUN_LENGTH:
      g_value_set_uint64 (value, filter->threshold_length);
      break;
    case PROP_THRESHOLD:
      g_value_set_double (value, filter->threshold_level);
      break;
    case PROP_THRESHOLD_DB:
      g_value_set_double (value, 20 * log (filter->threshold_level));
      break;
    case PROP_PRE_LENGTH:
      g_value_set_uint64 (value, filter->pre_length);
      break;
    case PROP_LEAKY:
      g_value_set_boolean (value, filter->leaky);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "cutter", GST_RANK_NONE, GST_TYPE_CUTTER))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    cutter,
    "Audio Cutter to split audio into non-silent bits",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
