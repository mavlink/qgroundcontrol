/* -*- c-basic-offset: 2 -*-
 * 
 * GStreamer
 * Copyright (C) 1999-2001 Erik Walthinsen <omega@cse.ogi.edu>
 *               2006 Dreamlab Technologies Ltd. <mathis.hofer@dreamlab.net>
 *               2007-2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <gst/audio/gstaudiofilter.h>

#include "audiofxbasefirfilter.h"

#define GST_CAT_DEFAULT gst_audio_fx_base_fir_filter_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

#define ALLOWED_CAPS \
    "audio/x-raw, "                                               \
    " format=(string){"GST_AUDIO_NE(F32)","GST_AUDIO_NE(F64)"}, " \
    " rate = (int) [ 1, MAX ], "                                  \
    " channels = (int) [ 1, MAX ], "                              \
    " layout=(string) interleaved"

/* Switch from time-domain to FFT convolution for kernels >= this */
#define FFT_THRESHOLD 32

enum
{
  PROP_0 = 0,
  PROP_LOW_LATENCY,
  PROP_DRAIN_ON_CHANGES
};

#define DEFAULT_LOW_LATENCY FALSE
#define DEFAULT_DRAIN_ON_CHANGES TRUE

#define gst_audio_fx_base_fir_filter_parent_class parent_class
G_DEFINE_TYPE (GstAudioFXBaseFIRFilter, gst_audio_fx_base_fir_filter,
    GST_TYPE_AUDIO_FILTER);

static GstFlowReturn gst_audio_fx_base_fir_filter_transform (GstBaseTransform *
    base, GstBuffer * inbuf, GstBuffer * outbuf);
static gboolean gst_audio_fx_base_fir_filter_start (GstBaseTransform * base);
static gboolean gst_audio_fx_base_fir_filter_stop (GstBaseTransform * base);
static gboolean gst_audio_fx_base_fir_filter_sink_event (GstBaseTransform *
    base, GstEvent * event);
static gboolean gst_audio_fx_base_fir_filter_transform_size (GstBaseTransform *
    base, GstPadDirection direction, GstCaps * caps, gsize size,
    GstCaps * othercaps, gsize * othersize);
static gboolean gst_audio_fx_base_fir_filter_setup (GstAudioFilter * base,
    const GstAudioInfo * info);

static gboolean gst_audio_fx_base_fir_filter_query (GstBaseTransform * trans,
    GstPadDirection direction, GstQuery * quer);

/*
 * The code below calculates the linear convolution:
 *
 * y[t] = \sum_{u=0}^{M-1} x[t - u] * h[u]
 *
 * where y is the output, x is the input, M is the length
 * of the filter kernel and h is the filter kernel. For x
 * holds: x[t] == 0 \forall t < 0.
 *
 * The runtime complexity of this is O (M) per sample.
 *
 */
#define DEFINE_PROCESS_FUNC(width,ctype) \
static guint \
process_##width (GstAudioFXBaseFIRFilter * self, const g##ctype * src, g##ctype * dst, guint input_samples) \
{ \
  gint channels = GST_AUDIO_FILTER_CHANNELS (self); \
  TIME_DOMAIN_CONVOLUTION_BODY (channels); \
}

#define DEFINE_PROCESS_FUNC_FIXED_CHANNELS(width,channels,ctype) \
static guint \
process_##channels##_##width (GstAudioFXBaseFIRFilter * self, const g##ctype * src, g##ctype * dst, guint input_samples) \
{ \
  TIME_DOMAIN_CONVOLUTION_BODY (channels); \
}

#define TIME_DOMAIN_CONVOLUTION_BODY(channels) G_STMT_START { \
  gint kernel_length = self->kernel_length; \
  gint i, j, k, l; \
  gint res_start; \
  gint from_input; \
  gint off; \
  gdouble *buffer = self->buffer; \
  gdouble *kernel = self->kernel; \
  \
  if (!buffer) { \
    self->buffer_length = kernel_length * channels; \
    self->buffer = buffer = g_new0 (gdouble, self->buffer_length); \
  } \
  \
  input_samples *= channels; \
  /* convolution */ \
  for (i = 0; i < input_samples; i++) { \
    dst[i] = 0.0; \
    k = i % channels; \
    l = i / channels; \
    from_input = MIN (l, kernel_length-1); \
    off = l * channels + k; \
    for (j = 0; j <= from_input; j++) { \
      dst[i] += src[off] * kernel[j]; \
      off -= channels; \
    } \
    /* j == from_input && off == (l - j) * channels + k */ \
    off += kernel_length * channels; \
    for (; j < kernel_length; j++) { \
      dst[i] += buffer[off] * kernel[j]; \
      off -= channels; \
    } \
  } \
  \
  /* copy the tail of the current input buffer to the residue, while \
   * keeping parts of the residue if the input buffer is smaller than \
   * the kernel length */ \
  /* from now on take kernel length as length over all channels */ \
  kernel_length *= channels; \
  if (input_samples < kernel_length) \
    res_start = kernel_length - input_samples; \
  else \
    res_start = 0; \
  \
  for (i = 0; i < res_start; i++) \
    buffer[i] = buffer[i + input_samples]; \
  /* i == res_start */ \
  for (; i < kernel_length; i++) \
    buffer[i] = src[input_samples - kernel_length + i]; \
  \
  self->buffer_fill += kernel_length - res_start; \
  if (self->buffer_fill > kernel_length) \
    self->buffer_fill = kernel_length; \
  \
  return input_samples / channels; \
} G_STMT_END

DEFINE_PROCESS_FUNC (32, float);
DEFINE_PROCESS_FUNC (64, double);

DEFINE_PROCESS_FUNC_FIXED_CHANNELS (32, 1, float);
DEFINE_PROCESS_FUNC_FIXED_CHANNELS (64, 1, double);

DEFINE_PROCESS_FUNC_FIXED_CHANNELS (32, 2, float);
DEFINE_PROCESS_FUNC_FIXED_CHANNELS (64, 2, double);

#undef TIME_DOMAIN_CONVOLUTION_BODY
#undef DEFINE_PROCESS_FUNC
#undef DEFINE_PROCESS_FUNC_FIXED_CHANNELS

/* This implements FFT convolution and uses the overlap-save algorithm.
 * See http://cnx.org/content/m12022/latest/ or your favorite
 * digital signal processing book for details.
 *
 * In every pass the following is calculated:
 *
 * y = IFFT (FFT(x) * FFT(h))
 *
 * where y is the output in the time domain, x the
 * input and h the filter kernel. * is the multiplication
 * of complex numbers.
 *
 * Due to the circular convolution theorem this
 * gives in the time domain:
 *
 * y[t] = \sum_{u=0}^{M-1} x[t - u] * h[u]
 *
 * where y is the output, M is the kernel length,
 * x the periodically extended[0] input and h the
 * filter kernel.
 *
 * ([0] Periodically extended means:    )
 * (    x[t] = x[t+kN] \forall k \in Z  )
 * (    where N is the length of x      )
 *
 * This means:
 * - Obviously x and h need to be of the same size for the FFT
 * - The first M-1 output values are useless because they're
 *   built from 1 up to M-1 values from the end of the input
 *   (circular convolusion!).
 * - The last M-1 input values are only used for 1 up to M-1
 *   output values, i.e. they need to be used again in the
 *   next pass for the first M-1 input values.
 *
 * => The first pass needs M-1 zeroes at the beginning of the
 * input and the last M-1 input values of every pass need to
 * be used as the first M-1 input values of the next pass.
 *
 * => x must be larger than h to give a useful number of output
 * samples and h needs to be padded by zeroes at the end to give
 * it virtually the same size as x (by M we denote the number of
 * non-padding samples of h). If len(x)==len(h)==M only 1 output
 * sample would be calculated per pass, len(x)==2*len(h) would
 * give M+1 output samples, etc. Usually a factor between 4 and 8
 * gives a low number of operations per output samples (see website
 * given above).
 *
 * Overall this gives a runtime complexity per sample of
 *
 *   (  N log N  )
 * O ( --------- ) compared to O (M) for the direct calculation.
 *   ( N - M + 1 )
 */
#define DEFINE_FFT_PROCESS_FUNC(width,ctype) \
static guint \
process_fft_##width (GstAudioFXBaseFIRFilter * self, const g##ctype * src, \
    g##ctype * dst, guint input_samples) \
{ \
  gint channels = GST_AUDIO_FILTER_CHANNELS (self); \
  FFT_CONVOLUTION_BODY (channels); \
}

#define DEFINE_FFT_PROCESS_FUNC_FIXED_CHANNELS(width,channels,ctype) \
static guint \
process_fft_##channels##_##width (GstAudioFXBaseFIRFilter * self, const g##ctype * src, \
    g##ctype * dst, guint input_samples) \
{ \
  FFT_CONVOLUTION_BODY (channels); \
}

#define FFT_CONVOLUTION_BODY(channels) G_STMT_START { \
  gint i, j; \
  guint pass; \
  guint kernel_length = self->kernel_length; \
  guint block_length = self->block_length; \
  guint buffer_length = self->buffer_length; \
  guint real_buffer_length = buffer_length + kernel_length - 1; \
  guint buffer_fill = self->buffer_fill; \
  GstFFTF64 *fft = self->fft; \
  GstFFTF64 *ifft = self->ifft; \
  GstFFTF64Complex *frequency_response = self->frequency_response; \
  GstFFTF64Complex *fft_buffer = self->fft_buffer; \
  guint frequency_response_length = self->frequency_response_length; \
  gdouble *buffer = self->buffer; \
  guint generated = 0; \
  gdouble re, im; \
  \
  if (!fft_buffer) \
    self->fft_buffer = fft_buffer = \
        g_new (GstFFTF64Complex, frequency_response_length); \
  \
  /* Buffer contains the time domain samples of input data for one chunk \
   * plus some more space for the inverse FFT below. \
   * \
   * The samples are put at offset kernel_length, the inverse FFT \
   * overwrites everything from offset 0 to length-kernel_length+1, keeping \
   * the last kernel_length-1 samples for copying to the next processing \
   * step. \
   */ \
  if (!buffer) { \
    self->buffer_length = buffer_length = block_length; \
    real_buffer_length = buffer_length + kernel_length - 1; \
    \
    self->buffer = buffer = g_new0 (gdouble, real_buffer_length * channels); \
    \
    /* Beginning has kernel_length-1 zeroes at the beginning */ \
    self->buffer_fill = buffer_fill = kernel_length - 1; \
  } \
  \
  g_assert (self->buffer_length == block_length); \
  \
  while (input_samples) { \
    pass = MIN (buffer_length - buffer_fill, input_samples); \
    \
    /* Deinterleave channels */ \
    for (i = 0; i < pass; i++) { \
      for (j = 0; j < channels; j++) { \
        buffer[real_buffer_length * j + buffer_fill + kernel_length - 1 + i] = \
            src[i * channels + j]; \
      } \
    } \
    buffer_fill += pass; \
    src += channels * pass; \
    input_samples -= pass; \
    \
    /* If we don't have a complete buffer go out */ \
    if (buffer_fill < buffer_length) \
      break; \
    \
    for (j = 0; j < channels; j++) { \
      /* Calculate FFT of input block */ \
      gst_fft_f64_fft (fft, \
          buffer + real_buffer_length * j + kernel_length - 1, fft_buffer); \
      \
      /* Complex multiplication of input and filter spectrum */ \
      for (i = 0; i < frequency_response_length; i++) { \
	re = fft_buffer[i].r; \
	im = fft_buffer[i].i; \
        \
        fft_buffer[i].r = \
            re * frequency_response[i].r - \
            im * frequency_response[i].i; \
        fft_buffer[i].i = \
            re * frequency_response[i].i + \
            im * frequency_response[i].r; \
      } \
      \
      /* Calculate inverse FFT of the result */ \
      gst_fft_f64_inverse_fft (ifft, fft_buffer, \
          buffer + real_buffer_length * j); \
      \
      /* Copy all except the first kernel_length-1 samples to the output */ \
      for (i = 0; i < buffer_length - kernel_length + 1; i++) { \
        dst[i * channels + j] = \
            buffer[real_buffer_length * j + kernel_length - 1 + i]; \
      } \
      \
      /* Copy the last kernel_length-1 samples to the beginning for the next block */ \
      for (i = 0; i < kernel_length - 1; i++) { \
        buffer[real_buffer_length * j + kernel_length - 1 + i] = \
            buffer[real_buffer_length * j + buffer_length + i]; \
      } \
    } \
    \
    generated += buffer_length - kernel_length + 1; \
    dst += channels * (buffer_length - kernel_length + 1); \
    \
    /* The the first kernel_length-1 samples are there already */ \
    buffer_fill = kernel_length - 1; \
  } \
  \
  /* Write back cached buffer_fill value */ \
  self->buffer_fill = buffer_fill; \
  \
  return generated; \
} G_STMT_END

DEFINE_FFT_PROCESS_FUNC (32, float);
DEFINE_FFT_PROCESS_FUNC (64, double);

DEFINE_FFT_PROCESS_FUNC_FIXED_CHANNELS (32, 1, float);
DEFINE_FFT_PROCESS_FUNC_FIXED_CHANNELS (64, 1, double);

DEFINE_FFT_PROCESS_FUNC_FIXED_CHANNELS (32, 2, float);
DEFINE_FFT_PROCESS_FUNC_FIXED_CHANNELS (64, 2, double);

#undef FFT_CONVOLUTION_BODY
#undef DEFINE_FFT_PROCESS_FUNC
#undef DEFINE_FFT_PROCESS_FUNC_FIXED_CHANNELS

/* Element class */
static void
    gst_audio_fx_base_fir_filter_calculate_frequency_response
    (GstAudioFXBaseFIRFilter * self)
{
  gst_fft_f64_free (self->fft);
  self->fft = NULL;
  gst_fft_f64_free (self->ifft);
  self->ifft = NULL;
  g_free (self->frequency_response);
  self->frequency_response_length = 0;
  g_free (self->fft_buffer);
  self->fft_buffer = NULL;

  if (self->kernel && self->kernel_length >= FFT_THRESHOLD
      && !self->low_latency) {
    guint block_length, i;
    gdouble *kernel_tmp, *kernel = self->kernel;

    /* We process 4 * kernel_length samples per pass in FFT mode */
    block_length = 4 * self->kernel_length;
    block_length = gst_fft_next_fast_length (block_length);
    self->block_length = block_length;

    kernel_tmp = g_new0 (gdouble, block_length);
    memcpy (kernel_tmp, kernel, self->kernel_length * sizeof (gdouble));

    self->fft = gst_fft_f64_new (block_length, FALSE);
    self->ifft = gst_fft_f64_new (block_length, TRUE);
    self->frequency_response_length = block_length / 2 + 1;
    self->frequency_response =
        g_new (GstFFTF64Complex, self->frequency_response_length);
    gst_fft_f64_fft (self->fft, kernel_tmp, self->frequency_response);
    g_free (kernel_tmp);

    /* Normalize to make sure IFFT(FFT(x)) == x */
    for (i = 0; i < self->frequency_response_length; i++) {
      self->frequency_response[i].r /= block_length;
      self->frequency_response[i].i /= block_length;
    }
  }
}

/* Must be called with base transform lock! */
static void
gst_audio_fx_base_fir_filter_select_process_function (GstAudioFXBaseFIRFilter *
    self, GstAudioFormat format, gint channels)
{
  switch (format) {
    case GST_AUDIO_FORMAT_F32:
      if (self->fft && !self->low_latency) {
        if (channels == 1)
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_fft_1_32;
        else if (channels == 2)
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_fft_2_32;
        else
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_fft_32;
      } else {
        if (channels == 1)
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_1_32;
        else if (channels == 2)
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_2_32;
        else
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_32;
      }
      break;
    case GST_AUDIO_FORMAT_F64:
      if (self->fft && !self->low_latency) {
        if (channels == 1)
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_fft_1_64;
        else if (channels == 2)
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_fft_2_64;
        else
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_fft_64;
      } else {
        if (channels == 1)
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_1_64;
        else if (channels == 2)
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_2_64;
        else
          self->process = (GstAudioFXBaseFIRFilterProcessFunc) process_64;
      }
      break;
    default:
      self->process = NULL;
      break;
  }
}

static void
gst_audio_fx_base_fir_filter_finalize (GObject * object)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (object);

  g_free (self->buffer);
  g_free (self->kernel);
  gst_fft_f64_free (self->fft);
  gst_fft_f64_free (self->ifft);
  g_free (self->frequency_response);
  g_free (self->fft_buffer);
  g_mutex_clear (&self->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_audio_fx_base_fir_filter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (object);

  switch (prop_id) {
    case PROP_LOW_LATENCY:{
      gboolean low_latency;

      if (GST_STATE (self) >= GST_STATE_PAUSED) {
        g_warning ("Changing the \"low-latency\" property "
            "is only allowed in states < PAUSED");
        return;
      }


      g_mutex_lock (&self->lock);
      low_latency = g_value_get_boolean (value);

      if (self->low_latency != low_latency) {
        self->low_latency = low_latency;
        gst_audio_fx_base_fir_filter_calculate_frequency_response (self);
        gst_audio_fx_base_fir_filter_select_process_function (self,
            GST_AUDIO_FILTER_FORMAT (self), GST_AUDIO_FILTER_CHANNELS (self));
      }
      g_mutex_unlock (&self->lock);
      break;
    }
    case PROP_DRAIN_ON_CHANGES:{
      g_mutex_lock (&self->lock);
      self->drain_on_changes = g_value_get_boolean (value);
      g_mutex_unlock (&self->lock);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_fx_base_fir_filter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (object);

  switch (prop_id) {
    case PROP_LOW_LATENCY:
      g_value_set_boolean (value, self->low_latency);
      break;
    case PROP_DRAIN_ON_CHANGES:
      g_value_set_boolean (value, self->drain_on_changes);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_fx_base_fir_filter_class_init (GstAudioFXBaseFIRFilterClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;
  GstAudioFilterClass *filter_class = (GstAudioFilterClass *) klass;
  GstCaps *caps;

  GST_DEBUG_CATEGORY_INIT (gst_audio_fx_base_fir_filter_debug,
      "audiofxbasefirfilter", 0, "FIR filter base class");

  gobject_class->finalize = gst_audio_fx_base_fir_filter_finalize;
  gobject_class->set_property = gst_audio_fx_base_fir_filter_set_property;
  gobject_class->get_property = gst_audio_fx_base_fir_filter_get_property;

  /**
   * GstAudioFXBaseFIRFilter:low-latency:
   *
   * Work in low-latency mode. This mode is much slower for large filter sizes
   * but the latency is always only the pre-latency of the filter.
   */
  g_object_class_install_property (gobject_class, PROP_LOW_LATENCY,
      g_param_spec_boolean ("low-latency", "Low latency",
          "Operate in low latency mode. This mode is slower but the "
          "latency will only be the filter pre-latency. "
          "Can only be changed in states < PAUSED!", DEFAULT_LOW_LATENCY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstAudioFXBaseFIRFilter:drain-on-changes:
   *
   * Whether the filter should be drained when its coefficients change
   *
   * Note: Currently this only works if the kernel size is not changed!
   * Support for drainless kernel size changes will be added in the future.
   */
  g_object_class_install_property (gobject_class, PROP_DRAIN_ON_CHANGES,
      g_param_spec_boolean ("drain-on-changes", "Drain on changes",
          "Drains the filter when its coefficients change",
          DEFAULT_DRAIN_ON_CHANGES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (GST_AUDIO_FILTER_CLASS (klass),
      caps);
  gst_caps_unref (caps);

  trans_class->transform =
      GST_DEBUG_FUNCPTR (gst_audio_fx_base_fir_filter_transform);
  trans_class->start = GST_DEBUG_FUNCPTR (gst_audio_fx_base_fir_filter_start);
  trans_class->stop = GST_DEBUG_FUNCPTR (gst_audio_fx_base_fir_filter_stop);
  trans_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_audio_fx_base_fir_filter_sink_event);
  trans_class->query = GST_DEBUG_FUNCPTR (gst_audio_fx_base_fir_filter_query);
  trans_class->transform_size =
      GST_DEBUG_FUNCPTR (gst_audio_fx_base_fir_filter_transform_size);
  filter_class->setup = GST_DEBUG_FUNCPTR (gst_audio_fx_base_fir_filter_setup);
}

static void
gst_audio_fx_base_fir_filter_init (GstAudioFXBaseFIRFilter * self)
{
  self->kernel = NULL;
  self->buffer = NULL;
  self->buffer_length = 0;

  self->start_ts = GST_CLOCK_TIME_NONE;
  self->start_off = GST_BUFFER_OFFSET_NONE;
  self->nsamples_out = 0;
  self->nsamples_in = 0;

  self->low_latency = DEFAULT_LOW_LATENCY;
  self->drain_on_changes = DEFAULT_DRAIN_ON_CHANGES;

  g_mutex_init (&self->lock);
}

void
gst_audio_fx_base_fir_filter_push_residue (GstAudioFXBaseFIRFilter * self)
{
  GstBuffer *outbuf;
  GstFlowReturn res;
  gint rate = GST_AUDIO_FILTER_RATE (self);
  gint channels = GST_AUDIO_FILTER_CHANNELS (self);
  gint bps = GST_AUDIO_FILTER_BPS (self);
  gint outsize, outsamples;
  GstMapInfo map;
  guint8 *in, *out;

  if (channels == 0 || rate == 0 || self->nsamples_in == 0) {
    self->buffer_fill = 0;
    g_free (self->buffer);
    self->buffer = NULL;
    return;
  }

  /* Calculate the number of samples and their memory size that
   * should be pushed from the residue */
  outsamples = self->nsamples_in - (self->nsamples_out - self->latency);
  if (outsamples <= 0) {
    self->buffer_fill = 0;
    g_free (self->buffer);
    self->buffer = NULL;
    return;
  }
  outsize = outsamples * channels * bps;

  if (!self->fft || self->low_latency) {
    gint64 diffsize, diffsamples;

    /* Process the difference between latency and residue length samples
     * to start at the actual data instead of starting at the zeros before
     * when we only got one buffer smaller than latency */
    diffsamples =
        ((gint64) self->latency) - ((gint64) self->buffer_fill) / channels;
    if (diffsamples > 0) {
      diffsize = diffsamples * channels * bps;
      in = g_new0 (guint8, diffsize);
      out = g_new0 (guint8, diffsize);
      self->nsamples_out += self->process (self, in, out, diffsamples);
      g_free (in);
      g_free (out);
    }

    outbuf = gst_buffer_new_and_alloc (outsize);

    /* Convolve the residue with zeros to get the actual remaining data */
    in = g_new0 (guint8, outsize);
    gst_buffer_map (outbuf, &map, GST_MAP_READWRITE);
    self->nsamples_out += self->process (self, in, map.data, outsamples);
    gst_buffer_unmap (outbuf, &map);

    g_free (in);
  } else {
    guint gensamples = 0;

    outbuf = gst_buffer_new_and_alloc (outsize);
    gst_buffer_map (outbuf, &map, GST_MAP_READWRITE);

    while (gensamples < outsamples) {
      guint step_insamples = self->block_length - self->buffer_fill;
      guint8 *zeroes = g_new0 (guint8, step_insamples * channels * bps);
      guint8 *out = g_new (guint8, self->block_length * channels * bps);
      guint step_gensamples;

      step_gensamples = self->process (self, zeroes, out, step_insamples);
      g_free (zeroes);

      memcpy (map.data + gensamples * bps, out, MIN (step_gensamples,
              outsamples - gensamples) * bps);
      gensamples += MIN (step_gensamples, outsamples - gensamples);

      g_free (out);
    }
    self->nsamples_out += gensamples;

    gst_buffer_unmap (outbuf, &map);
  }

  /* Set timestamp, offset, etc from the values we
   * saved when processing the regular buffers */
  if (GST_CLOCK_TIME_IS_VALID (self->start_ts))
    GST_BUFFER_TIMESTAMP (outbuf) = self->start_ts;
  else
    GST_BUFFER_TIMESTAMP (outbuf) = 0;
  GST_BUFFER_TIMESTAMP (outbuf) +=
      gst_util_uint64_scale_int (self->nsamples_out - outsamples -
      self->latency, GST_SECOND, rate);

  GST_BUFFER_DURATION (outbuf) =
      gst_util_uint64_scale_int (outsamples, GST_SECOND, rate);

  if (self->start_off != GST_BUFFER_OFFSET_NONE) {
    GST_BUFFER_OFFSET (outbuf) =
        self->start_off + self->nsamples_out - outsamples - self->latency;
    GST_BUFFER_OFFSET_END (outbuf) = GST_BUFFER_OFFSET (outbuf) + outsamples;
  }

  GST_DEBUG_OBJECT (self,
      "Pushing residue buffer of size %" G_GSIZE_FORMAT " with timestamp: %"
      GST_TIME_FORMAT ", duration: %" GST_TIME_FORMAT ", offset: %"
      G_GUINT64_FORMAT ", offset_end: %" G_GUINT64_FORMAT ", nsamples_out: %d",
      gst_buffer_get_size (outbuf),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (outbuf)), GST_BUFFER_OFFSET (outbuf),
      GST_BUFFER_OFFSET_END (outbuf), outsamples);

  res = gst_pad_push (GST_BASE_TRANSFORM_CAST (self)->srcpad, outbuf);

  if (G_UNLIKELY (res != GST_FLOW_OK)) {
    GST_WARNING_OBJECT (self, "failed to push residue");
  }

  self->buffer_fill = 0;
}

/* GstAudioFilter vmethod implementations */

/* get notified of caps and plug in the correct process function */
static gboolean
gst_audio_fx_base_fir_filter_setup (GstAudioFilter * base,
    const GstAudioInfo * info)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (base);

  g_mutex_lock (&self->lock);
  if (self->buffer) {
    gst_audio_fx_base_fir_filter_push_residue (self);
    g_free (self->buffer);
    self->buffer = NULL;
    self->buffer_fill = 0;
    self->buffer_length = 0;
    self->start_ts = GST_CLOCK_TIME_NONE;
    self->start_off = GST_BUFFER_OFFSET_NONE;
    self->nsamples_out = 0;
    self->nsamples_in = 0;
  }

  gst_audio_fx_base_fir_filter_select_process_function (self,
      GST_AUDIO_INFO_FORMAT (info), GST_AUDIO_INFO_CHANNELS (info));
  g_mutex_unlock (&self->lock);

  return (self->process != NULL);
}

/* GstBaseTransform vmethod implementations */

static gboolean
gst_audio_fx_base_fir_filter_transform_size (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (base);
  guint blocklen;
  GstAudioInfo info;
  gint bpf;

  if (!self->fft || self->low_latency || direction == GST_PAD_SRC) {
    *othersize = size;
    return TRUE;
  }

  if (!gst_audio_info_from_caps (&info, caps))
    return FALSE;

  bpf = GST_AUDIO_INFO_BPF (&info);

  size /= bpf;
  blocklen = self->block_length - self->kernel_length + 1;
  *othersize = ((size + blocklen - 1) / blocklen) * blocklen;
  *othersize *= bpf;

  return TRUE;
}

static GstFlowReturn
gst_audio_fx_base_fir_filter_transform (GstBaseTransform * base,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (base);
  GstClockTime timestamp, expected_timestamp;
  gint channels = GST_AUDIO_FILTER_CHANNELS (self);
  gint rate = GST_AUDIO_FILTER_RATE (self);
  gint bps = GST_AUDIO_FILTER_BPS (self);
  GstMapInfo inmap, outmap;
  guint input_samples;
  guint output_samples;
  guint generated_samples;
  guint64 output_offset;
  gint64 diff = 0;
  GstClockTime stream_time;

  timestamp = GST_BUFFER_TIMESTAMP (outbuf);

  if (!GST_CLOCK_TIME_IS_VALID (timestamp)
      && !GST_CLOCK_TIME_IS_VALID (self->start_ts)) {
    GST_ERROR_OBJECT (self, "Invalid timestamp");
    return GST_FLOW_ERROR;
  }

  g_mutex_lock (&self->lock);
  stream_time =
      gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (self, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (self), stream_time);

  g_return_val_if_fail (self->kernel != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (channels != 0, GST_FLOW_ERROR);

  if (GST_CLOCK_TIME_IS_VALID (self->start_ts))
    expected_timestamp =
        self->start_ts + gst_util_uint64_scale_int (self->nsamples_in,
        GST_SECOND, rate);
  else
    expected_timestamp = GST_CLOCK_TIME_NONE;

  /* Reset the residue if already existing on discont buffers */
  if (GST_BUFFER_IS_DISCONT (inbuf)
      || (GST_CLOCK_TIME_IS_VALID (expected_timestamp)
          && (ABS (GST_CLOCK_DIFF (timestamp,
                      expected_timestamp)) > 5 * GST_MSECOND))) {
    GST_DEBUG_OBJECT (self, "Discontinuity detected - flushing");
    if (GST_CLOCK_TIME_IS_VALID (expected_timestamp))
      gst_audio_fx_base_fir_filter_push_residue (self);
    self->buffer_fill = 0;
    g_free (self->buffer);
    self->buffer = NULL;
    self->start_ts = timestamp;
    self->start_off = GST_BUFFER_OFFSET (inbuf);
    self->nsamples_out = 0;
    self->nsamples_in = 0;
  } else if (!GST_CLOCK_TIME_IS_VALID (self->start_ts)) {
    self->start_ts = timestamp;
    self->start_off = GST_BUFFER_OFFSET (inbuf);
  }

  gst_buffer_map (inbuf, &inmap, GST_MAP_READ);
  gst_buffer_map (outbuf, &outmap, GST_MAP_WRITE);

  input_samples = (inmap.size / bps) / channels;
  output_samples = (outmap.size / bps) / channels;

  self->nsamples_in += input_samples;

  generated_samples =
      self->process (self, inmap.data, outmap.data, input_samples);

  gst_buffer_unmap (inbuf, &inmap);
  gst_buffer_unmap (outbuf, &outmap);

  g_assert (generated_samples <= output_samples);
  self->nsamples_out += generated_samples;
  if (generated_samples == 0)
    goto no_samples;

  /* Calculate the number of samples we can push out now without outputting
   * latency zeros in the beginning */
  diff = ((gint64) self->nsamples_out) - ((gint64) self->latency);
  if (diff < 0)
    goto no_samples;

  if (diff < generated_samples) {
    gint64 tmp = diff;
    diff = generated_samples - diff;
    generated_samples = tmp;
  } else {
    diff = 0;
  }

  gst_buffer_resize (outbuf, diff * bps * channels,
      generated_samples * bps * channels);

  output_offset = self->nsamples_out - self->latency - generated_samples;
  GST_BUFFER_TIMESTAMP (outbuf) =
      self->start_ts + gst_util_uint64_scale_int (output_offset, GST_SECOND,
      rate);
  GST_BUFFER_DURATION (outbuf) =
      gst_util_uint64_scale_int (output_samples, GST_SECOND, rate);
  if (self->start_off != GST_BUFFER_OFFSET_NONE) {
    GST_BUFFER_OFFSET (outbuf) = self->start_off + output_offset;
    GST_BUFFER_OFFSET_END (outbuf) =
        GST_BUFFER_OFFSET (outbuf) + generated_samples;
  } else {
    GST_BUFFER_OFFSET (outbuf) = GST_BUFFER_OFFSET_NONE;
    GST_BUFFER_OFFSET_END (outbuf) = GST_BUFFER_OFFSET_NONE;
  }
  g_mutex_unlock (&self->lock);

  GST_DEBUG_OBJECT (self,
      "Pushing buffer of size %" G_GSIZE_FORMAT " with timestamp: %"
      GST_TIME_FORMAT ", duration: %" GST_TIME_FORMAT ", offset: %"
      G_GUINT64_FORMAT ", offset_end: %" G_GUINT64_FORMAT ", nsamples_out: %d",
      gst_buffer_get_size (outbuf),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (outbuf)), GST_BUFFER_OFFSET (outbuf),
      GST_BUFFER_OFFSET_END (outbuf), generated_samples);

  return GST_FLOW_OK;

no_samples:
  {
    g_mutex_unlock (&self->lock);
    return GST_BASE_TRANSFORM_FLOW_DROPPED;
  }
}

static gboolean
gst_audio_fx_base_fir_filter_start (GstBaseTransform * base)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (base);

  self->buffer_fill = 0;
  g_free (self->buffer);
  self->buffer = NULL;
  self->start_ts = GST_CLOCK_TIME_NONE;
  self->start_off = GST_BUFFER_OFFSET_NONE;
  self->nsamples_out = 0;
  self->nsamples_in = 0;

  return TRUE;
}

static gboolean
gst_audio_fx_base_fir_filter_stop (GstBaseTransform * base)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (base);

  g_free (self->buffer);
  self->buffer = NULL;
  self->buffer_length = 0;

  return TRUE;
}

static gboolean
gst_audio_fx_base_fir_filter_query (GstBaseTransform * trans,
    GstPadDirection direction, GstQuery * query)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (trans);
  gboolean res = TRUE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:
    {
      GstClockTime min, max;
      gboolean live;
      guint64 latency;
      gint rate = GST_AUDIO_FILTER_RATE (self);

      if (rate == 0) {
        res = FALSE;
      } else if ((res =
              gst_pad_peer_query (GST_BASE_TRANSFORM (self)->sinkpad, query))) {
        gst_query_parse_latency (query, &live, &min, &max);

        GST_DEBUG_OBJECT (self, "Peer latency: min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min), GST_TIME_ARGS (max));

        if (self->fft && !self->low_latency)
          latency = self->block_length - self->kernel_length + 1;
        else
          latency = self->latency;

        /* add our own latency */
        latency = gst_util_uint64_scale_round (latency, GST_SECOND, rate);

        GST_DEBUG_OBJECT (self, "Our latency: %"
            GST_TIME_FORMAT, GST_TIME_ARGS (latency));

        min += latency;
        if (max != GST_CLOCK_TIME_NONE)
          max += latency;

        GST_DEBUG_OBJECT (self, "Calculated total latency : min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min), GST_TIME_ARGS (max));

        gst_query_set_latency (query, live, min, max);
      }
      break;
    }
    default:
      res =
          GST_BASE_TRANSFORM_CLASS (parent_class)->query (trans, direction,
          query);
      break;
  }
  return res;
}

static gboolean
gst_audio_fx_base_fir_filter_sink_event (GstBaseTransform * base,
    GstEvent * event)
{
  GstAudioFXBaseFIRFilter *self = GST_AUDIO_FX_BASE_FIR_FILTER (base);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      gst_audio_fx_base_fir_filter_push_residue (self);
      self->start_ts = GST_CLOCK_TIME_NONE;
      self->start_off = GST_BUFFER_OFFSET_NONE;
      self->nsamples_out = 0;
      self->nsamples_in = 0;
      break;
    default:
      break;
  }

  return GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (base, event);
}

void
gst_audio_fx_base_fir_filter_set_kernel (GstAudioFXBaseFIRFilter * self,
    gdouble * kernel, guint kernel_length, guint64 latency,
    const GstAudioInfo * info)
{
  gboolean latency_changed;
  GstAudioFormat format;
  gint channels;

  g_return_if_fail (kernel != NULL);
  g_return_if_fail (self != NULL);

  g_mutex_lock (&self->lock);

  latency_changed = (self->latency != latency
      || (!self->low_latency && self->kernel_length < FFT_THRESHOLD
          && kernel_length >= FFT_THRESHOLD)
      || (!self->low_latency && self->kernel_length >= FFT_THRESHOLD
          && kernel_length < FFT_THRESHOLD));

  /* FIXME: If the latency changes, the buffer size changes too and we
   * have to drain in any case until this is fixed in the future */
  if (self->buffer && (!self->drain_on_changes || latency_changed)) {
    gst_audio_fx_base_fir_filter_push_residue (self);
    self->start_ts = GST_CLOCK_TIME_NONE;
    self->start_off = GST_BUFFER_OFFSET_NONE;
    self->nsamples_out = 0;
    self->nsamples_in = 0;
    self->buffer_fill = 0;
  }

  g_free (self->kernel);
  if (!self->drain_on_changes || latency_changed) {
    g_free (self->buffer);
    self->buffer = NULL;
    self->buffer_fill = 0;
    self->buffer_length = 0;
  }

  self->kernel = kernel;
  self->kernel_length = kernel_length;

  if (info) {
    format = GST_AUDIO_INFO_FORMAT (info);
    channels = GST_AUDIO_INFO_CHANNELS (info);
  } else {
    format = GST_AUDIO_FILTER_FORMAT (self);
    channels = GST_AUDIO_FILTER_CHANNELS (self);
  }

  gst_audio_fx_base_fir_filter_calculate_frequency_response (self);
  gst_audio_fx_base_fir_filter_select_process_function (self, format, channels);

  if (latency_changed) {
    self->latency = latency;
    gst_element_post_message (GST_ELEMENT (self),
        gst_message_new_latency (GST_OBJECT (self)));
  }

  g_mutex_unlock (&self->lock);
}
