/* 
 * GStreamer
 * Copyright (C) 2007-2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#include <math.h>

#include "audiofxbaseiirfilter.h"

#define GST_CAT_DEFAULT gst_audio_fx_base_iir_filter_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

#define ALLOWED_CAPS \
    "audio/x-raw,"                                                \
    " format=(string){"GST_AUDIO_NE(F32)","GST_AUDIO_NE(F64)"},"  \
    " rate = (int) [ 1, MAX ],"                                   \
    " channels = (int) [ 1, MAX ],"                               \
    " layout=(string) interleaved"

#define gst_audio_fx_base_iir_filter_parent_class parent_class
G_DEFINE_TYPE (GstAudioFXBaseIIRFilter,
    gst_audio_fx_base_iir_filter, GST_TYPE_AUDIO_FILTER);

static gboolean gst_audio_fx_base_iir_filter_setup (GstAudioFilter * filter,
    const GstAudioInfo * info);
static GstFlowReturn
gst_audio_fx_base_iir_filter_transform_ip (GstBaseTransform * base,
    GstBuffer * buf);
static gboolean gst_audio_fx_base_iir_filter_stop (GstBaseTransform * base);

static void process_64 (GstAudioFXBaseIIRFilter * filter,
    gdouble * data, guint num_samples);
static void process_32 (GstAudioFXBaseIIRFilter * filter,
    gfloat * data, guint num_samples);

/* GObject vmethod implementations */

static void
gst_audio_fx_base_iir_filter_finalize (GObject * object)
{
  GstAudioFXBaseIIRFilter *filter = GST_AUDIO_FX_BASE_IIR_FILTER (object);

  if (filter->a) {
    g_free (filter->a);
    filter->a = NULL;
  }

  if (filter->b) {
    g_free (filter->b);
    filter->b = NULL;
  }

  if (filter->channels) {
    GstAudioFXBaseIIRFilterChannelCtx *ctx;
    guint i;

    for (i = 0; i < filter->nchannels; i++) {
      ctx = &filter->channels[i];
      g_free (ctx->x);
      g_free (ctx->y);
    }

    g_free (filter->channels);
    filter->channels = NULL;
  }
  g_mutex_clear (&filter->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_audio_fx_base_iir_filter_class_init (GstAudioFXBaseIIRFilterClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;
  GstAudioFilterClass *filter_class = (GstAudioFilterClass *) klass;
  GstCaps *caps;

  GST_DEBUG_CATEGORY_INIT (gst_audio_fx_base_iir_filter_debug,
      "audiofxbaseiirfilter", 0, "Audio IIR Filter Base Class");

  gobject_class->finalize = gst_audio_fx_base_iir_filter_finalize;

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (GST_AUDIO_FILTER_CLASS (klass),
      caps);
  gst_caps_unref (caps);

  filter_class->setup = GST_DEBUG_FUNCPTR (gst_audio_fx_base_iir_filter_setup);

  trans_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_audio_fx_base_iir_filter_transform_ip);
  trans_class->transform_ip_on_passthrough = FALSE;
  trans_class->stop = GST_DEBUG_FUNCPTR (gst_audio_fx_base_iir_filter_stop);
}

static void
gst_audio_fx_base_iir_filter_init (GstAudioFXBaseIIRFilter * filter)
{
  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (filter), TRUE);

  filter->a = NULL;
  filter->na = 0;
  filter->b = NULL;
  filter->nb = 0;
  filter->channels = NULL;
  filter->nchannels = 0;

  g_mutex_init (&filter->lock);
}

/* Evaluate the transfer function that corresponds to the IIR
 * coefficients at (zr + zi*I)^-1 and return the magnitude */
gdouble
gst_audio_fx_base_iir_filter_calculate_gain (gdouble * a, guint na, gdouble * b,
    guint nb, gdouble zr, gdouble zi)
{
  gdouble sum_ar, sum_ai;
  gdouble sum_br, sum_bi;
  gdouble gain_r, gain_i;

  gdouble sum_r_old;
  gdouble sum_i_old;

  gint i;

  sum_ar = a[na - 1];
  sum_ai = 0.0;
  for (i = na - 2; i >= 0; i--) {
    sum_r_old = sum_ar;
    sum_i_old = sum_ai;

    sum_ar = (sum_r_old * zr - sum_i_old * zi) + a[i];
    sum_ai = (sum_r_old * zi + sum_i_old * zr) + 0.0;
  }

  sum_br = b[nb - 1];
  sum_bi = 0.0;
  for (i = nb - 2; i >= 0; i--) {
    sum_r_old = sum_br;
    sum_i_old = sum_bi;

    sum_br = (sum_r_old * zr - sum_i_old * zi) + b[i];
    sum_bi = (sum_r_old * zi + sum_i_old * zr) + 0.0;
  }

  gain_r =
      (sum_br * sum_ar + sum_bi * sum_ai) / (sum_ar * sum_ar + sum_ai * sum_ai);
  gain_i =
      (sum_bi * sum_ar - sum_br * sum_ai) / (sum_ar * sum_ar + sum_ai * sum_ai);

  return (sqrt (gain_r * gain_r + gain_i * gain_i));
}

void
gst_audio_fx_base_iir_filter_set_coefficients (GstAudioFXBaseIIRFilter * filter,
    gdouble * a, guint na, gdouble * b, guint nb)
{
  guint i;

  g_return_if_fail (GST_IS_AUDIO_FX_BASE_IIR_FILTER (filter));

  g_mutex_lock (&filter->lock);

  g_free (filter->a);
  g_free (filter->b);

  filter->a = filter->b = NULL;

  if (filter->channels) {
    GstAudioFXBaseIIRFilterChannelCtx *ctx;
    gboolean free = (na != filter->na || nb != filter->nb);

    for (i = 0; i < filter->nchannels; i++) {
      ctx = &filter->channels[i];

      if (free)
        g_free (ctx->x);
      else
        memset (ctx->x, 0, filter->nb * sizeof (gdouble));

      if (free)
        g_free (ctx->y);
      else
        memset (ctx->y, 0, filter->na * sizeof (gdouble));
    }

    g_free (filter->channels);
    filter->channels = NULL;
  }

  filter->na = na;
  filter->nb = nb;

  filter->a = a;
  filter->b = b;

  if (filter->nchannels && !filter->channels) {
    GstAudioFXBaseIIRFilterChannelCtx *ctx;

    filter->channels =
        g_new0 (GstAudioFXBaseIIRFilterChannelCtx, filter->nchannels);
    for (i = 0; i < filter->nchannels; i++) {
      ctx = &filter->channels[i];

      ctx->x = g_new0 (gdouble, filter->nb);
      ctx->y = g_new0 (gdouble, filter->na);
    }
  }

  g_mutex_unlock (&filter->lock);
}

/* GstAudioFilter vmethod implementations */

static gboolean
gst_audio_fx_base_iir_filter_setup (GstAudioFilter * base,
    const GstAudioInfo * info)
{
  GstAudioFXBaseIIRFilter *filter = GST_AUDIO_FX_BASE_IIR_FILTER (base);
  gboolean ret = TRUE;
  gint channels;

  g_mutex_lock (&filter->lock);
  switch (GST_AUDIO_INFO_FORMAT (info)) {
    case GST_AUDIO_FORMAT_F32:
      filter->process = (GstAudioFXBaseIIRFilterProcessFunc)
          process_32;
      break;
    case GST_AUDIO_FORMAT_F64:
      filter->process = (GstAudioFXBaseIIRFilterProcessFunc)
          process_64;
      break;
    default:
      ret = FALSE;
      break;
  }

  channels = GST_AUDIO_INFO_CHANNELS (info);

  if (channels != filter->nchannels) {
    guint i;
    GstAudioFXBaseIIRFilterChannelCtx *ctx;

    if (filter->channels) {
      for (i = 0; i < filter->nchannels; i++) {
        ctx = &filter->channels[i];

        g_free (ctx->x);
        g_free (ctx->y);
      }
      g_free (filter->channels);
    }

    filter->channels = g_new0 (GstAudioFXBaseIIRFilterChannelCtx, channels);
    for (i = 0; i < channels; i++) {
      ctx = &filter->channels[i];

      ctx->x = g_new0 (gdouble, filter->nb);
      ctx->y = g_new0 (gdouble, filter->na);
    }
    filter->nchannels = channels;
  }
  g_mutex_unlock (&filter->lock);

  return ret;
}

static inline gdouble
process (GstAudioFXBaseIIRFilter * filter,
    GstAudioFXBaseIIRFilterChannelCtx * ctx, gdouble x0)
{
  gdouble val = filter->b[0] * x0;
  gint i, j;

  for (i = 1, j = ctx->x_pos; i < filter->nb; i++) {
    val += filter->b[i] * ctx->x[j];
    j--;
    if (j < 0)
      j = filter->nb - 1;
  }

  for (i = 1, j = ctx->y_pos; i < filter->na; i++) {
    val -= filter->a[i] * ctx->y[j];
    j--;
    if (j < 0)
      j = filter->na - 1;
  }
  val /= filter->a[0];

  if (ctx->x) {
    ctx->x_pos++;
    if (ctx->x_pos >= filter->nb)
      ctx->x_pos = 0;
    ctx->x[ctx->x_pos] = x0;
  }
  if (ctx->y) {
    ctx->y_pos++;
    if (ctx->y_pos >= filter->na)
      ctx->y_pos = 0;

    ctx->y[ctx->y_pos] = val;
  }

  return val;
}

#define DEFINE_PROCESS_FUNC(width,ctype) \
static void \
process_##width (GstAudioFXBaseIIRFilter * filter, \
    g##ctype * data, guint num_samples) \
{ \
  gint i, j, channels = filter->nchannels; \
  gdouble val; \
  \
  for (i = 0; i < num_samples / channels; i++) { \
    for (j = 0; j < channels; j++) { \
      val = process (filter, &filter->channels[j], *data); \
      *data++ = val; \
    } \
  } \
}

DEFINE_PROCESS_FUNC (32, float);
DEFINE_PROCESS_FUNC (64, double);

#undef DEFINE_PROCESS_FUNC

/* GstBaseTransform vmethod implementations */
static GstFlowReturn
gst_audio_fx_base_iir_filter_transform_ip (GstBaseTransform * base,
    GstBuffer * buf)
{
  GstAudioFXBaseIIRFilter *filter = GST_AUDIO_FX_BASE_IIR_FILTER (base);
  guint num_samples;
  GstClockTime timestamp, stream_time;
  GstMapInfo map;

  timestamp = GST_BUFFER_TIMESTAMP (buf);
  stream_time =
      gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (filter, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (filter), stream_time);

  gst_buffer_map (buf, &map, GST_MAP_READWRITE);
  num_samples = map.size / GST_AUDIO_FILTER_BPS (filter);

  g_mutex_lock (&filter->lock);
  if (filter->a == NULL || filter->b == NULL) {
    g_warn_if_fail (filter->a != NULL && filter->b != NULL);
    gst_buffer_unmap (buf, &map);
    g_mutex_unlock (&filter->lock);
    return GST_FLOW_ERROR;
  }
  filter->process (filter, map.data, num_samples);
  g_mutex_unlock (&filter->lock);

  gst_buffer_unmap (buf, &map);

  return GST_FLOW_OK;
}


static gboolean
gst_audio_fx_base_iir_filter_stop (GstBaseTransform * base)
{
  GstAudioFXBaseIIRFilter *filter = GST_AUDIO_FX_BASE_IIR_FILTER (base);
  guint channels = filter->nchannels;
  GstAudioFXBaseIIRFilterChannelCtx *ctx;
  guint i;

  /* Reset the history of input and output values if
   * already existing */
  if (channels && filter->channels) {
    for (i = 0; i < channels; i++) {
      ctx = &filter->channels[i];
      g_free (ctx->x);
      g_free (ctx->y);
    }
    g_free (filter->channels);
  }
  filter->channels = NULL;
  filter->nchannels = 0;

  return TRUE;
}
