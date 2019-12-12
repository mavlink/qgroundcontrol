/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *               <2006,2011> Stefan Kost <ensonic@users.sf.net>
 *               <2007-2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
 * SECTION:element-spectrum
 * @title: spectrum
 *
 * The Spectrum element analyzes the frequency spectrum of an audio signal.
 * If the #GstSpectrum:post-messages property is %TRUE, it sends analysis results
 * as element messages named
 * `spectrum` after each interval of time given
 * by the #GstSpectrum:interval property.
 *
 * The message's structure contains some combination of these fields:
 *
 * * #GstClockTime `timestamp`: the timestamp of the buffer that triggered the message.
 * * #GstClockTime `stream-time`: the stream time of the buffer.
 * * #GstClockTime `running-time`: the running_time of the buffer.
 * * #GstClockTime `duration`: the duration of the buffer.
 * * #GstClockTime `endtime`: the end time of the buffer that triggered the message as stream time (this
 *   is deprecated, as it can be calculated from stream-time + duration)
 * * A #GST_TYPE_LIST value of #gfloat `magnitude`: the level for each frequency band in dB.
 *   All values below the value of the
 *   #GstSpectrum:threshold property will be set to the threshold. Only present
 *   if the #GstSpectrum:message-magnitude property is %TRUE.
 * * A #GST_TYPE_LIST of #gfloat `phase`: The phase for each frequency band. The value is between -pi and pi. Only
 *   present if the #GstSpectrum:message-phase property is %TRUE.
 *
 * If #GstSpectrum:multi-channel property is set to true. magnitude and phase
 * fields will be each a nested #GST_TYPE_ARRAY value. The first dimension are the
 * channels and the second dimension are the values.
 *
 * ## Example application
 *
 * {{ tests/examples/spectrum/spectrum-example.c }}
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include "gstspectrum.h"

GST_DEBUG_CATEGORY_STATIC (gst_spectrum_debug);
#define GST_CAT_DEFAULT gst_spectrum_debug

/* elementfactory information */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
# define FORMATS "{ S16LE, S24LE, S32LE, F32LE, F64LE }"
#else
# define FORMATS "{ S16BE, S24BE, S32BE, F32BE, F64BE }"
#endif

#define ALLOWED_CAPS \
  GST_AUDIO_CAPS_MAKE (FORMATS) ", " \
  "layout = (string) interleaved"

/* Spectrum properties */
#define DEFAULT_POST_MESSAGES	        TRUE
#define DEFAULT_MESSAGE_MAGNITUDE	TRUE
#define DEFAULT_MESSAGE_PHASE		FALSE
#define DEFAULT_INTERVAL		(GST_SECOND / 10)
#define DEFAULT_BANDS			128
#define DEFAULT_THRESHOLD		-60
#define DEFAULT_MULTI_CHANNEL		FALSE

enum
{
  PROP_0,
  PROP_POST_MESSAGES,
  PROP_MESSAGE_MAGNITUDE,
  PROP_MESSAGE_PHASE,
  PROP_INTERVAL,
  PROP_BANDS,
  PROP_THRESHOLD,
  PROP_MULTI_CHANNEL
};

#define gst_spectrum_parent_class parent_class
G_DEFINE_TYPE (GstSpectrum, gst_spectrum, GST_TYPE_AUDIO_FILTER);

static void gst_spectrum_finalize (GObject * object);
static void gst_spectrum_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_spectrum_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static gboolean gst_spectrum_start (GstBaseTransform * trans);
static gboolean gst_spectrum_stop (GstBaseTransform * trans);
static GstFlowReturn gst_spectrum_transform_ip (GstBaseTransform * trans,
    GstBuffer * in);
static gboolean gst_spectrum_setup (GstAudioFilter * base,
    const GstAudioInfo * info);

static void
gst_spectrum_class_init (GstSpectrumClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstAudioFilterClass *filter_class = GST_AUDIO_FILTER_CLASS (klass);
  GstCaps *caps;

  gobject_class->set_property = gst_spectrum_set_property;
  gobject_class->get_property = gst_spectrum_get_property;
  gobject_class->finalize = gst_spectrum_finalize;

  trans_class->start = GST_DEBUG_FUNCPTR (gst_spectrum_start);
  trans_class->stop = GST_DEBUG_FUNCPTR (gst_spectrum_stop);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_spectrum_transform_ip);
  trans_class->passthrough_on_same_caps = TRUE;

  filter_class->setup = GST_DEBUG_FUNCPTR (gst_spectrum_setup);

  g_object_class_install_property (gobject_class, PROP_POST_MESSAGES,
      g_param_spec_boolean ("post-messages", "Post Messages",
          "Whether to post a 'spectrum' element message on the bus for each "
          "passed interval", DEFAULT_POST_MESSAGES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MESSAGE_MAGNITUDE,
      g_param_spec_boolean ("message-magnitude", "Magnitude",
          "Whether to add a 'magnitude' field to the structure of any "
          "'spectrum' element messages posted on the bus",
          DEFAULT_MESSAGE_MAGNITUDE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MESSAGE_PHASE,
      g_param_spec_boolean ("message-phase", "Phase",
          "Whether to add a 'phase' field to the structure of any "
          "'spectrum' element messages posted on the bus",
          DEFAULT_MESSAGE_PHASE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INTERVAL,
      g_param_spec_uint64 ("interval", "Interval",
          "Interval of time between message posts (in nanoseconds)",
          1, G_MAXUINT64, DEFAULT_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BANDS,
      g_param_spec_uint ("bands", "Bands", "Number of frequency bands",
          2, ((guint) G_MAXINT + 2) / 2, DEFAULT_BANDS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_THRESHOLD,
      g_param_spec_int ("threshold", "Threshold",
          "dB threshold for result. All lower values will be set to this",
          G_MININT, 0, DEFAULT_THRESHOLD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MULTI_CHANNEL,
      g_param_spec_boolean ("multi-channel", "Multichannel results",
          "Send separate results for each channel",
          DEFAULT_MULTI_CHANNEL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (gst_spectrum_debug, "spectrum", 0,
      "audio spectrum analyser element");

  gst_element_class_set_static_metadata (element_class, "Spectrum analyzer",
      "Filter/Analyzer/Audio",
      "Run an FFT on the audio signal, output spectrum data",
      "Erik Walthinsen <omega@cse.ogi.edu>, "
      "Stefan Kost <ensonic@users.sf.net>, "
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (filter_class, caps);
  gst_caps_unref (caps);
}

static void
gst_spectrum_init (GstSpectrum * spectrum)
{
  spectrum->post_messages = DEFAULT_POST_MESSAGES;
  spectrum->message_magnitude = DEFAULT_MESSAGE_MAGNITUDE;
  spectrum->message_phase = DEFAULT_MESSAGE_PHASE;
  spectrum->interval = DEFAULT_INTERVAL;
  spectrum->bands = DEFAULT_BANDS;
  spectrum->threshold = DEFAULT_THRESHOLD;

  g_mutex_init (&spectrum->lock);
}

static void
gst_spectrum_alloc_channel_data (GstSpectrum * spectrum)
{
  gint i;
  GstSpectrumChannel *cd;
  guint bands = spectrum->bands;
  guint nfft = 2 * bands - 2;

  g_assert (spectrum->channel_data == NULL);

  spectrum->num_channels = (spectrum->multi_channel) ?
      GST_AUDIO_FILTER_CHANNELS (spectrum) : 1;

  GST_DEBUG_OBJECT (spectrum, "allocating data for %d channels",
      spectrum->num_channels);

  spectrum->channel_data = g_new (GstSpectrumChannel, spectrum->num_channels);
  for (i = 0; i < spectrum->num_channels; i++) {
    cd = &spectrum->channel_data[i];
    cd->fft_ctx = gst_fft_f32_new (nfft, FALSE);
    cd->input = g_new0 (gfloat, nfft);
    cd->input_tmp = g_new0 (gfloat, nfft);
    cd->freqdata = g_new0 (GstFFTF32Complex, bands);
    cd->spect_magnitude = g_new0 (gfloat, bands);
    cd->spect_phase = g_new0 (gfloat, bands);
  }
}

static void
gst_spectrum_free_channel_data (GstSpectrum * spectrum)
{
  if (spectrum->channel_data) {
    gint i;
    GstSpectrumChannel *cd;

    GST_DEBUG_OBJECT (spectrum, "freeing data for %d channels",
        spectrum->num_channels);

    for (i = 0; i < spectrum->num_channels; i++) {
      cd = &spectrum->channel_data[i];
      if (cd->fft_ctx)
        gst_fft_f32_free (cd->fft_ctx);
      g_free (cd->input);
      g_free (cd->input_tmp);
      g_free (cd->freqdata);
      g_free (cd->spect_magnitude);
      g_free (cd->spect_phase);
    }
    g_free (spectrum->channel_data);
    spectrum->channel_data = NULL;
  }
}

static void
gst_spectrum_flush (GstSpectrum * spectrum)
{
  spectrum->num_frames = 0;
  spectrum->num_fft = 0;

  spectrum->accumulated_error = 0;
}

static void
gst_spectrum_reset_state (GstSpectrum * spectrum)
{
  GST_DEBUG_OBJECT (spectrum, "resetting state");

  gst_spectrum_free_channel_data (spectrum);
  gst_spectrum_flush (spectrum);
}

static void
gst_spectrum_finalize (GObject * object)
{
  GstSpectrum *spectrum = GST_SPECTRUM (object);

  gst_spectrum_reset_state (spectrum);
  g_mutex_clear (&spectrum->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_spectrum_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSpectrum *filter = GST_SPECTRUM (object);

  switch (prop_id) {
    case PROP_POST_MESSAGES:
      filter->post_messages = g_value_get_boolean (value);
      break;
    case PROP_MESSAGE_MAGNITUDE:
      filter->message_magnitude = g_value_get_boolean (value);
      break;
    case PROP_MESSAGE_PHASE:
      filter->message_phase = g_value_get_boolean (value);
      break;
    case PROP_INTERVAL:{
      guint64 interval = g_value_get_uint64 (value);
      g_mutex_lock (&filter->lock);
      if (filter->interval != interval) {
        filter->interval = interval;
        gst_spectrum_reset_state (filter);
      }
      g_mutex_unlock (&filter->lock);
      break;
    }
    case PROP_BANDS:{
      guint bands = g_value_get_uint (value);
      g_mutex_lock (&filter->lock);
      if (filter->bands != bands) {
        filter->bands = bands;
        gst_spectrum_reset_state (filter);
      }
      g_mutex_unlock (&filter->lock);
      break;
    }
    case PROP_THRESHOLD:
      filter->threshold = g_value_get_int (value);
      break;
    case PROP_MULTI_CHANNEL:{
      gboolean multi_channel = g_value_get_boolean (value);
      g_mutex_lock (&filter->lock);
      if (filter->multi_channel != multi_channel) {
        filter->multi_channel = multi_channel;
        gst_spectrum_reset_state (filter);
      }
      g_mutex_unlock (&filter->lock);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_spectrum_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSpectrum *filter = GST_SPECTRUM (object);

  switch (prop_id) {
    case PROP_POST_MESSAGES:
      g_value_set_boolean (value, filter->post_messages);
      break;
    case PROP_MESSAGE_MAGNITUDE:
      g_value_set_boolean (value, filter->message_magnitude);
      break;
    case PROP_MESSAGE_PHASE:
      g_value_set_boolean (value, filter->message_phase);
      break;
    case PROP_INTERVAL:
      g_value_set_uint64 (value, filter->interval);
      break;
    case PROP_BANDS:
      g_value_set_uint (value, filter->bands);
      break;
    case PROP_THRESHOLD:
      g_value_set_int (value, filter->threshold);
      break;
    case PROP_MULTI_CHANNEL:
      g_value_set_boolean (value, filter->multi_channel);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_spectrum_start (GstBaseTransform * trans)
{
  GstSpectrum *spectrum = GST_SPECTRUM (trans);

  gst_spectrum_reset_state (spectrum);

  return TRUE;
}

static gboolean
gst_spectrum_stop (GstBaseTransform * trans)
{
  GstSpectrum *spectrum = GST_SPECTRUM (trans);

  gst_spectrum_reset_state (spectrum);

  return TRUE;
}

/* mixing data readers */

static void
input_data_mixed_float (const guint8 * _in, gfloat * out, guint len,
    guint channels, gfloat max_value, guint op, guint nfft)
{
  guint i, j, ip = 0;
  gfloat v;
  gfloat *in = (gfloat *) _in;

  for (j = 0; j < len; j++) {
    v = in[ip++];
    for (i = 1; i < channels; i++)
      v += in[ip++];
    out[op] = v / channels;
    op = (op + 1) % nfft;
  }
}

static void
input_data_mixed_double (const guint8 * _in, gfloat * out, guint len,
    guint channels, gfloat max_value, guint op, guint nfft)
{
  guint i, j, ip = 0;
  gfloat v;
  gdouble *in = (gdouble *) _in;

  for (j = 0; j < len; j++) {
    v = in[ip++];
    for (i = 1; i < channels; i++)
      v += in[ip++];
    out[op] = v / channels;
    op = (op + 1) % nfft;
  }
}

static void
input_data_mixed_int32_max (const guint8 * _in, gfloat * out, guint len,
    guint channels, gfloat max_value, guint op, guint nfft)
{
  guint i, j, ip = 0;
  gint32 *in = (gint32 *) _in;
  gfloat v;

  for (j = 0; j < len; j++) {
    v = in[ip++] / max_value;
    for (i = 1; i < channels; i++)
      v += in[ip++] / max_value;
    out[op] = v / channels;
    op = (op + 1) % nfft;
  }
}

static void
input_data_mixed_int24_max (const guint8 * _in, gfloat * out, guint len,
    guint channels, gfloat max_value, guint op, guint nfft)
{
  guint i, j;
  gfloat v = 0.0;

  for (j = 0; j < len; j++) {
    for (i = 0; i < channels; i++) {
#if G_BYTE_ORDER == G_BIG_ENDIAN
      gint32 value = GST_READ_UINT24_BE (_in);
#else
      gint32 value = GST_READ_UINT24_LE (_in);
#endif
      if (value & 0x00800000)
        value |= 0xff000000;
      v += value / max_value;
      _in += 3;
    }
    out[op] = v / channels;
    op = (op + 1) % nfft;
  }
}

static void
input_data_mixed_int16_max (const guint8 * _in, gfloat * out, guint len,
    guint channels, gfloat max_value, guint op, guint nfft)
{
  guint i, j, ip = 0;
  gint16 *in = (gint16 *) _in;
  gfloat v;

  for (j = 0; j < len; j++) {
    v = in[ip++] / max_value;
    for (i = 1; i < channels; i++)
      v += in[ip++] / max_value;
    out[op] = v / channels;
    op = (op + 1) % nfft;
  }
}

/* non mixing data readers */

static void
input_data_float (const guint8 * _in, gfloat * out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
  guint j, ip;
  gfloat *in = (gfloat *) _in;

  for (j = 0, ip = 0; j < len; j++, ip += channels) {
    out[op] = in[ip];
    op = (op + 1) % nfft;
  }
}

static void
input_data_double (const guint8 * _in, gfloat * out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
  guint j, ip;
  gdouble *in = (gdouble *) _in;

  for (j = 0, ip = 0; j < len; j++, ip += channels) {
    out[op] = in[ip];
    op = (op + 1) % nfft;
  }
}

static void
input_data_int32_max (const guint8 * _in, gfloat * out, guint len,
    guint channels, gfloat max_value, guint op, guint nfft)
{
  guint j, ip;
  gint32 *in = (gint32 *) _in;

  for (j = 0, ip = 0; j < len; j++, ip += channels) {
    out[op] = in[ip] / max_value;
    op = (op + 1) % nfft;
  }
}

static void
input_data_int24_max (const guint8 * _in, gfloat * out, guint len,
    guint channels, gfloat max_value, guint op, guint nfft)
{
  guint j;

  for (j = 0; j < len; j++) {
#if G_BYTE_ORDER == G_BIG_ENDIAN
    gint32 v = GST_READ_UINT24_BE (_in);
#else
    gint32 v = GST_READ_UINT24_LE (_in);
#endif
    if (v & 0x00800000)
      v |= 0xff000000;
    _in += 3 * channels;
    out[op] = v / max_value;
    op = (op + 1) % nfft;
  }
}

static void
input_data_int16_max (const guint8 * _in, gfloat * out, guint len,
    guint channels, gfloat max_value, guint op, guint nfft)
{
  guint j, ip;
  gint16 *in = (gint16 *) _in;

  for (j = 0, ip = 0; j < len; j++, ip += channels) {
    out[op] = in[ip] / max_value;
    op = (op + 1) % nfft;
  }
}

static gboolean
gst_spectrum_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  GstSpectrum *spectrum = GST_SPECTRUM (base);
  gboolean multi_channel = spectrum->multi_channel;
  GstSpectrumInputData input_data = NULL;

  g_mutex_lock (&spectrum->lock);
  switch (GST_AUDIO_INFO_FORMAT (info)) {
    case GST_AUDIO_FORMAT_S16:
      input_data =
          multi_channel ? input_data_int16_max : input_data_mixed_int16_max;
      break;
    case GST_AUDIO_FORMAT_S24:
      input_data =
          multi_channel ? input_data_int24_max : input_data_mixed_int24_max;
      break;
    case GST_AUDIO_FORMAT_S32:
      input_data =
          multi_channel ? input_data_int32_max : input_data_mixed_int32_max;
      break;
    case GST_AUDIO_FORMAT_F32:
      input_data = multi_channel ? input_data_float : input_data_mixed_float;
      break;
    case GST_AUDIO_FORMAT_F64:
      input_data = multi_channel ? input_data_double : input_data_mixed_double;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
  spectrum->input_data = input_data;

  gst_spectrum_reset_state (spectrum);
  g_mutex_unlock (&spectrum->lock);

  return TRUE;
}

static GValue *
gst_spectrum_message_add_container (GstStructure * s, GType type,
    const gchar * name)
{
  GValue v = { 0, };

  g_value_init (&v, type);
  /* will copy-by-value */
  gst_structure_set_value (s, name, &v);
  g_value_unset (&v);
  return (GValue *) gst_structure_get_value (s, name);
}

static void
gst_spectrum_message_add_list (GValue * cv, gfloat * data, guint num_values)
{
  GValue v = { 0, };
  guint i;

  g_value_init (&v, G_TYPE_FLOAT);
  for (i = 0; i < num_values; i++) {
    g_value_set_float (&v, data[i]);
    gst_value_list_append_value (cv, &v);       /* copies by value */
  }
  g_value_unset (&v);
}

static void
gst_spectrum_message_add_array (GValue * cv, gfloat * data, guint num_values)
{
  GValue v = { 0, };
  GValue a = { 0, };
  guint i;

  g_value_init (&a, GST_TYPE_ARRAY);

  g_value_init (&v, G_TYPE_FLOAT);
  for (i = 0; i < num_values; i++) {
    g_value_set_float (&v, data[i]);
    gst_value_array_append_value (&a, &v);      /* copies by value */
  }
  g_value_unset (&v);

  gst_value_array_append_value (cv, &a);        /* copies by value */
  g_value_unset (&a);
}

static GstMessage *
gst_spectrum_message_new (GstSpectrum * spectrum, GstClockTime timestamp,
    GstClockTime duration)
{
  GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (spectrum);
  GstSpectrumChannel *cd;
  GstStructure *s;
  GValue *mcv = NULL, *pcv = NULL;
  GstClockTime endtime, running_time, stream_time;

  GST_DEBUG_OBJECT (spectrum, "preparing message, bands =%d ", spectrum->bands);

  running_time = gst_segment_to_running_time (&trans->segment, GST_FORMAT_TIME,
      timestamp);
  stream_time = gst_segment_to_stream_time (&trans->segment, GST_FORMAT_TIME,
      timestamp);
  /* endtime is for backwards compatibility */
  endtime = stream_time + duration;

  s = gst_structure_new ("spectrum",
      "endtime", GST_TYPE_CLOCK_TIME, endtime,
      "timestamp", G_TYPE_UINT64, timestamp,
      "stream-time", G_TYPE_UINT64, stream_time,
      "running-time", G_TYPE_UINT64, running_time,
      "duration", G_TYPE_UINT64, duration, NULL);

  if (!spectrum->multi_channel) {
    cd = &spectrum->channel_data[0];

    if (spectrum->message_magnitude) {
      /* FIXME 0.11: this should be an array, not a list */
      mcv = gst_spectrum_message_add_container (s, GST_TYPE_LIST, "magnitude");
      gst_spectrum_message_add_list (mcv, cd->spect_magnitude, spectrum->bands);
    }
    if (spectrum->message_phase) {
      /* FIXME 0.11: this should be an array, not a list */
      pcv = gst_spectrum_message_add_container (s, GST_TYPE_LIST, "phase");
      gst_spectrum_message_add_list (pcv, cd->spect_phase, spectrum->bands);
    }
  } else {
    guint c;
    guint channels = GST_AUDIO_FILTER_CHANNELS (spectrum);

    if (spectrum->message_magnitude) {
      mcv = gst_spectrum_message_add_container (s, GST_TYPE_ARRAY, "magnitude");
    }
    if (spectrum->message_phase) {
      pcv = gst_spectrum_message_add_container (s, GST_TYPE_ARRAY, "phase");
    }

    for (c = 0; c < channels; c++) {
      cd = &spectrum->channel_data[c];

      if (spectrum->message_magnitude) {
        gst_spectrum_message_add_array (mcv, cd->spect_magnitude,
            spectrum->bands);
      }
      if (spectrum->message_phase) {
        gst_spectrum_message_add_array (pcv, cd->spect_phase, spectrum->bands);
      }
    }
  }
  return gst_message_new_element (GST_OBJECT (spectrum), s);
}

static void
gst_spectrum_run_fft (GstSpectrum * spectrum, GstSpectrumChannel * cd,
    guint input_pos)
{
  guint i;
  guint bands = spectrum->bands;
  guint nfft = 2 * bands - 2;
  gint threshold = spectrum->threshold;
  gfloat *input = cd->input;
  gfloat *input_tmp = cd->input_tmp;
  gfloat *spect_magnitude = cd->spect_magnitude;
  gfloat *spect_phase = cd->spect_phase;
  GstFFTF32Complex *freqdata = cd->freqdata;
  GstFFTF32 *fft_ctx = cd->fft_ctx;

  for (i = 0; i < nfft; i++)
    input_tmp[i] = input[(input_pos + i) % nfft];

  gst_fft_f32_window (fft_ctx, input_tmp, GST_FFT_WINDOW_HAMMING);

  gst_fft_f32_fft (fft_ctx, input_tmp, freqdata);

  if (spectrum->message_magnitude) {
    gdouble val;
    /* Calculate magnitude in db */
    for (i = 0; i < bands; i++) {
      val = freqdata[i].r * freqdata[i].r;
      val += freqdata[i].i * freqdata[i].i;
      val /= nfft * nfft;
      val = 10.0 * log10 (val);
      if (val < threshold)
        val = threshold;
      spect_magnitude[i] += val;
    }
  }

  if (spectrum->message_phase) {
    /* Calculate phase */
    for (i = 0; i < bands; i++)
      spect_phase[i] += atan2 (freqdata[i].i, freqdata[i].r);
  }
}

static void
gst_spectrum_prepare_message_data (GstSpectrum * spectrum,
    GstSpectrumChannel * cd)
{
  guint i;
  guint bands = spectrum->bands;
  guint num_fft = spectrum->num_fft;

  /* Calculate average */
  if (spectrum->message_magnitude) {
    gfloat *spect_magnitude = cd->spect_magnitude;
    for (i = 0; i < bands; i++)
      spect_magnitude[i] /= num_fft;
  }
  if (spectrum->message_phase) {
    gfloat *spect_phase = cd->spect_phase;
    for (i = 0; i < bands; i++)
      spect_phase[i] /= num_fft;
  }
}

static void
gst_spectrum_reset_message_data (GstSpectrum * spectrum,
    GstSpectrumChannel * cd)
{
  guint bands = spectrum->bands;
  gfloat *spect_magnitude = cd->spect_magnitude;
  gfloat *spect_phase = cd->spect_phase;

  /* reset spectrum accumulators */
  memset (spect_magnitude, 0, bands * sizeof (gfloat));
  memset (spect_phase, 0, bands * sizeof (gfloat));
}

static GstFlowReturn
gst_spectrum_transform_ip (GstBaseTransform * trans, GstBuffer * buffer)
{
  GstSpectrum *spectrum = GST_SPECTRUM (trans);
  guint rate = GST_AUDIO_FILTER_RATE (spectrum);
  guint channels = GST_AUDIO_FILTER_CHANNELS (spectrum);
  guint bps = GST_AUDIO_FILTER_BPS (spectrum);
  guint bpf = GST_AUDIO_FILTER_BPF (spectrum);
  guint output_channels = spectrum->multi_channel ? channels : 1;
  guint c;
  gfloat max_value = (1UL << ((bps << 3) - 1)) - 1;
  guint bands = spectrum->bands;
  guint nfft = 2 * bands - 2;
  guint input_pos;
  gfloat *input;
  GstMapInfo map;
  const guint8 *data;
  gsize size;
  guint fft_todo, msg_todo, block_size;
  gboolean have_full_interval;
  GstSpectrumChannel *cd;
  GstSpectrumInputData input_data;

  g_mutex_lock (&spectrum->lock);
  gst_buffer_map (buffer, &map, GST_MAP_READ);
  data = map.data;
  size = map.size;

  GST_LOG_OBJECT (spectrum, "input size: %" G_GSIZE_FORMAT " bytes", size);

  if (GST_BUFFER_IS_DISCONT (buffer)) {
    GST_DEBUG_OBJECT (spectrum, "Discontinuity detected -- flushing");
    gst_spectrum_flush (spectrum);
  }

  /* If we don't have a FFT context yet (or it was reset due to parameter
   * changes) get one and allocate memory for everything
   */
  if (spectrum->channel_data == NULL) {
    GST_DEBUG_OBJECT (spectrum, "allocating for bands %u", bands);

    gst_spectrum_alloc_channel_data (spectrum);

    /* number of sample frames we process before posting a message
     * interval is in ns */
    spectrum->frames_per_interval =
        gst_util_uint64_scale (spectrum->interval, rate, GST_SECOND);
    spectrum->frames_todo = spectrum->frames_per_interval;
    /* rounding error for frames_per_interval in ns,
     * aggregated it in accumulated_error */
    spectrum->error_per_interval = (spectrum->interval * rate) % GST_SECOND;
    if (spectrum->frames_per_interval == 0)
      spectrum->frames_per_interval = 1;

    GST_INFO_OBJECT (spectrum, "interval %" GST_TIME_FORMAT ", fpi %"
        G_GUINT64_FORMAT ", error %" GST_TIME_FORMAT,
        GST_TIME_ARGS (spectrum->interval), spectrum->frames_per_interval,
        GST_TIME_ARGS (spectrum->error_per_interval));

    spectrum->input_pos = 0;

    gst_spectrum_flush (spectrum);
  }

  if (spectrum->num_frames == 0)
    spectrum->message_ts = GST_BUFFER_TIMESTAMP (buffer);

  input_pos = spectrum->input_pos;
  input_data = spectrum->input_data;

  while (size >= bpf) {
    /* run input_data for a chunk of data */
    fft_todo = nfft - (spectrum->num_frames % nfft);
    msg_todo = spectrum->frames_todo - spectrum->num_frames;
    GST_LOG_OBJECT (spectrum,
        "message frames todo: %u, fft frames todo: %u, input frames %"
        G_GSIZE_FORMAT, msg_todo, fft_todo, (size / bpf));
    block_size = msg_todo;
    if (block_size > (size / bpf))
      block_size = (size / bpf);
    if (block_size > fft_todo)
      block_size = fft_todo;

    for (c = 0; c < output_channels; c++) {
      cd = &spectrum->channel_data[c];
      input = cd->input;
      /* Move the current frames into our ringbuffers */
      input_data (data + c * bps, input, block_size, channels, max_value,
          input_pos, nfft);
    }
    data += block_size * bpf;
    size -= block_size * bpf;
    input_pos = (input_pos + block_size) % nfft;
    spectrum->num_frames += block_size;

    have_full_interval = (spectrum->num_frames == spectrum->frames_todo);

    GST_LOG_OBJECT (spectrum,
        "size: %" G_GSIZE_FORMAT ", do-fft = %d, do-message = %d", size,
        (spectrum->num_frames % nfft == 0), have_full_interval);

    /* If we have enough frames for an FFT or we have all frames required for
     * the interval and we haven't run a FFT, then run an FFT */
    if ((spectrum->num_frames % nfft == 0) ||
        (have_full_interval && !spectrum->num_fft)) {
      for (c = 0; c < output_channels; c++) {
        cd = &spectrum->channel_data[c];
        gst_spectrum_run_fft (spectrum, cd, input_pos);
      }
      spectrum->num_fft++;
    }

    /* Do we have the FFTs for one interval? */
    if (have_full_interval) {
      GST_DEBUG_OBJECT (spectrum, "nfft: %u frames: %" G_GUINT64_FORMAT
          " fpi: %" G_GUINT64_FORMAT " error: %" GST_TIME_FORMAT, nfft,
          spectrum->num_frames, spectrum->frames_per_interval,
          GST_TIME_ARGS (spectrum->accumulated_error));

      spectrum->frames_todo = spectrum->frames_per_interval;
      if (spectrum->accumulated_error >= GST_SECOND) {
        spectrum->accumulated_error -= GST_SECOND;
        spectrum->frames_todo++;
      }
      spectrum->accumulated_error += spectrum->error_per_interval;

      if (spectrum->post_messages) {
        GstMessage *m;

        for (c = 0; c < output_channels; c++) {
          cd = &spectrum->channel_data[c];
          gst_spectrum_prepare_message_data (spectrum, cd);
        }

        m = gst_spectrum_message_new (spectrum, spectrum->message_ts,
            spectrum->interval);

        gst_element_post_message (GST_ELEMENT (spectrum), m);
      }

      if (GST_CLOCK_TIME_IS_VALID (spectrum->message_ts))
        spectrum->message_ts +=
            gst_util_uint64_scale (spectrum->num_frames, GST_SECOND, rate);

      for (c = 0; c < output_channels; c++) {
        cd = &spectrum->channel_data[c];
        gst_spectrum_reset_message_data (spectrum, cd);
      }
      spectrum->num_frames = 0;
      spectrum->num_fft = 0;
    }
  }

  spectrum->input_pos = input_pos;

  gst_buffer_unmap (buffer, &map);
  g_mutex_unlock (&spectrum->lock);

  g_assert (size == 0);

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "spectrum", GST_RANK_NONE,
      GST_TYPE_SPECTRUM);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    spectrum,
    "Run an FFT on the audio signal, output spectrum data",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
