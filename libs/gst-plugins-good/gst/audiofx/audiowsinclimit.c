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
 * 
 * this windowed sinc filter is taken from the freely downloadable DSP book,
 * "The Scientist and Engineer's Guide to Digital Signal Processing",
 * chapter 16
 * available at http://www.dspguide.com/
 *
 * For the window functions see
 * http://en.wikipedia.org/wiki/Window_function
 */

/**
 * SECTION:element-audiowsinclimit
 * @title: audiowsinclimit
 *
 * Attenuates all frequencies above the cutoff frequency (low-pass) or all frequencies below the
 * cutoff frequency (high-pass). The length parameter controls the rolloff, the window parameter
 * controls rolloff and stopband attenuation. The Hamming window provides a faster rolloff but a bit
 * worse stopband attenuation, the other way around for the Blackman window.
 *
 * This element has the advantage over the Chebyshev lowpass and highpass filter that it has
 * a much better rolloff when using a larger kernel size and almost linear phase. The only
 * disadvantage is the much slower execution time with larger kernels.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc freq=1500 ! audioconvert ! audiowsinclimit mode=low-pass cutoff=1000 length=501 ! audioconvert ! alsasink
 * gst-launch-1.0 filesrc location="melo1.ogg" ! oggdemux ! vorbisdec ! audioconvert ! audiowsinclimit mode=high-pass cutoff=15000 length=501 ! audioconvert ! alsasink
 * gst-launch-1.0 audiotestsrc wave=white-noise ! audioconvert ! audiowsinclimit mode=low-pass cutoff=1000 length=10001 window=blackman ! audioconvert ! alsasink
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <gst/audio/gstaudiofilter.h>

#include "audiowsinclimit.h"

#include "gst/glib-compat-private.h"

#define GST_CAT_DEFAULT gst_audio_wsinclimit_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  PROP_0,
  PROP_LENGTH,
  PROP_FREQUENCY,
  PROP_MODE,
  PROP_WINDOW
};

enum
{
  MODE_LOW_PASS = 0,
  MODE_HIGH_PASS
};

#define GST_TYPE_AUDIO_WSINC_LIMIT_MODE (gst_audio_wsinclimit_mode_get_type ())
static GType
gst_audio_wsinclimit_mode_get_type (void)
{
  static GType gtype = 0;

  if (gtype == 0) {
    static const GEnumValue values[] = {
      {MODE_LOW_PASS, "Low pass (default)",
          "low-pass"},
      {MODE_HIGH_PASS, "High pass",
          "high-pass"},
      {0, NULL, NULL}
    };

    gtype = g_enum_register_static ("GstAudioWSincLimitMode", values);
  }
  return gtype;
}

enum
{
  WINDOW_HAMMING = 0,
  WINDOW_BLACKMAN,
  WINDOW_GAUSSIAN,
  WINDOW_COSINE,
  WINDOW_HANN
};

#define GST_TYPE_AUDIO_WSINC_LIMIT_WINDOW (gst_audio_wsinclimit_window_get_type ())
static GType
gst_audio_wsinclimit_window_get_type (void)
{
  static GType gtype = 0;

  if (gtype == 0) {
    static const GEnumValue values[] = {
      {WINDOW_HAMMING, "Hamming window (default)",
          "hamming"},
      {WINDOW_BLACKMAN, "Blackman window",
          "blackman"},
      {WINDOW_GAUSSIAN, "Gaussian window",
          "gaussian"},
      {WINDOW_COSINE, "Cosine window",
          "cosine"},
      {WINDOW_HANN, "Hann window",
          "hann"},
      {0, NULL, NULL}
    };

    gtype = g_enum_register_static ("GstAudioWSincLimitWindow", values);
  }
  return gtype;
}

#define gst_audio_wsinclimit_parent_class parent_class
G_DEFINE_TYPE (GstAudioWSincLimit, gst_audio_wsinclimit,
    GST_TYPE_AUDIO_FX_BASE_FIR_FILTER);

static void gst_audio_wsinclimit_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_audio_wsinclimit_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_audio_wsinclimit_finalize (GObject * object);

static gboolean gst_audio_wsinclimit_setup (GstAudioFilter * base,
    const GstAudioInfo * info);


#define POW2(x)  (x)*(x)

static void
gst_audio_wsinclimit_class_init (GstAudioWSincLimitClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstAudioFilterClass *filter_class = (GstAudioFilterClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_audio_wsinclimit_debug, "audiowsinclimit", 0,
      "Low-pass and High-pass Windowed sinc filter plugin");

  gobject_class->set_property = gst_audio_wsinclimit_set_property;
  gobject_class->get_property = gst_audio_wsinclimit_get_property;
  gobject_class->finalize = gst_audio_wsinclimit_finalize;

  /* FIXME: Don't use the complete possible range but restrict the upper boundary
   * so automatically generated UIs can use a slider */
  g_object_class_install_property (gobject_class, PROP_FREQUENCY,
      g_param_spec_float ("cutoff", "Cutoff",
          "Cut-off Frequency (Hz)", 0.0, 100000.0, 0.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_LENGTH,
      g_param_spec_int ("length", "Length",
          "Filter kernel length, will be rounded to the next odd number",
          3, 256000, 101,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode",
          "Low pass or high pass mode", GST_TYPE_AUDIO_WSINC_LIMIT_MODE,
          MODE_LOW_PASS,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WINDOW,
      g_param_spec_enum ("window", "Window",
          "Window function to use", GST_TYPE_AUDIO_WSINC_LIMIT_WINDOW,
          WINDOW_HAMMING,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "Low pass & high pass filter", "Filter/Effect/Audio",
      "Low pass and high pass windowed sinc filter",
      "Thomas Vander Stichele <thomas at apestaart dot org>, "
      "Steven W. Smith, "
      "Dreamlab Technologies Ltd. <mathis.hofer@dreamlab.net>, "
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  filter_class->setup = GST_DEBUG_FUNCPTR (gst_audio_wsinclimit_setup);
}

static void
gst_audio_wsinclimit_init (GstAudioWSincLimit * self)
{
  self->mode = MODE_LOW_PASS;
  self->window = WINDOW_HAMMING;
  self->kernel_length = 101;
  self->cutoff = 0.0;

  g_mutex_init (&self->lock);
}

static void
gst_audio_wsinclimit_build_kernel (GstAudioWSincLimit * self,
    const GstAudioInfo * info)
{
  gint i = 0;
  gdouble sum = 0.0;
  gint len = 0;
  gdouble w;
  gdouble *kernel = NULL;
  gint rate, channels;

  len = self->kernel_length;

  if (info) {
    rate = GST_AUDIO_INFO_RATE (info);
    channels = GST_AUDIO_INFO_CHANNELS (info);
  } else {
    rate = GST_AUDIO_FILTER_RATE (self);
    channels = GST_AUDIO_FILTER_CHANNELS (self);
  }

  if (rate == 0) {
    GST_DEBUG ("rate not set yet");
    return;
  }

  if (channels == 0) {
    GST_DEBUG ("channels not set yet");
    return;
  }

  /* Clamp cutoff frequency between 0 and the nyquist frequency */
  self->cutoff = CLAMP (self->cutoff, 0.0, rate / 2);

  GST_DEBUG ("gst_audio_wsinclimit_: initializing filter kernel of length %d "
      "with cutoff %.2lf Hz "
      "for mode %s",
      len, self->cutoff,
      (self->mode == MODE_LOW_PASS) ? "low-pass" : "high-pass");

  /* fill the kernel */
  w = 2 * G_PI * (self->cutoff / rate);

  kernel = g_new (gdouble, len);

  for (i = 0; i < len; ++i) {
    if (i == (len - 1) / 2.0)
      kernel[i] = w;
    else
      kernel[i] = sin (w * (i - (len - 1) / 2)) / (i - (len - 1) / 2.0);

    /* windowing */
    switch (self->window) {
      case WINDOW_HAMMING:
        kernel[i] *= (0.54 - 0.46 * cos (2 * G_PI * i / (len - 1)));
        break;
      case WINDOW_BLACKMAN:
        kernel[i] *= (0.42 - 0.5 * cos (2 * G_PI * i / (len - 1)) +
            0.08 * cos (4 * G_PI * i / (len - 1)));
        break;
      case WINDOW_GAUSSIAN:
        kernel[i] *= exp (-0.5 * POW2 (3.0 / len * (2 * i - (len - 1))));
        break;
      case WINDOW_COSINE:
        kernel[i] *= cos (G_PI * i / (len - 1) - G_PI / 2);
        break;
      case WINDOW_HANN:
        kernel[i] *= 0.5 * (1 - cos (2 * G_PI * i / (len - 1)));
        break;
    }
  }

  /* normalize for unity gain at DC */
  for (i = 0; i < len; ++i)
    sum += kernel[i];
  for (i = 0; i < len; ++i)
    kernel[i] /= sum;

  /* convert to highpass if specified */
  if (self->mode == MODE_HIGH_PASS) {
    for (i = 0; i < len; ++i)
      kernel[i] = -kernel[i];

    if (len % 2 == 1) {
      kernel[(len - 1) / 2] += 1.0;
    } else {
      kernel[len / 2 - 1] += 0.5;
      kernel[len / 2] += 0.5;
    }
  }

  gst_audio_fx_base_fir_filter_set_kernel (GST_AUDIO_FX_BASE_FIR_FILTER (self),
      kernel, self->kernel_length, (len - 1) / 2, info);
}

/* GstAudioFilter vmethod implementations */

/* get notified of caps and plug in the correct process function */
static gboolean
gst_audio_wsinclimit_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  GstAudioWSincLimit *self = GST_AUDIO_WSINC_LIMIT (base);

  gst_audio_wsinclimit_build_kernel (self, info);

  return GST_AUDIO_FILTER_CLASS (parent_class)->setup (base, info);
}

static void
gst_audio_wsinclimit_finalize (GObject * object)
{
  GstAudioWSincLimit *self = GST_AUDIO_WSINC_LIMIT (object);

  g_mutex_clear (&self->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_audio_wsinclimit_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioWSincLimit *self = GST_AUDIO_WSINC_LIMIT (object);

  g_return_if_fail (GST_IS_AUDIO_WSINC_LIMIT (self));

  switch (prop_id) {
    case PROP_LENGTH:{
      gint val;

      g_mutex_lock (&self->lock);
      val = g_value_get_int (value);
      if (val % 2 == 0)
        val++;

      if (val != self->kernel_length) {
        gst_audio_fx_base_fir_filter_push_residue (GST_AUDIO_FX_BASE_FIR_FILTER
            (self));
        self->kernel_length = val;
        gst_audio_wsinclimit_build_kernel (self, NULL);
      }
      g_mutex_unlock (&self->lock);
      break;
    }
    case PROP_FREQUENCY:
      g_mutex_lock (&self->lock);
      self->cutoff = g_value_get_float (value);
      gst_audio_wsinclimit_build_kernel (self, NULL);
      g_mutex_unlock (&self->lock);
      break;
    case PROP_MODE:
      g_mutex_lock (&self->lock);
      self->mode = g_value_get_enum (value);
      gst_audio_wsinclimit_build_kernel (self, NULL);
      g_mutex_unlock (&self->lock);
      break;
    case PROP_WINDOW:
      g_mutex_lock (&self->lock);
      self->window = g_value_get_enum (value);
      gst_audio_wsinclimit_build_kernel (self, NULL);
      g_mutex_unlock (&self->lock);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_wsinclimit_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioWSincLimit *self = GST_AUDIO_WSINC_LIMIT (object);

  switch (prop_id) {
    case PROP_LENGTH:
      g_value_set_int (value, self->kernel_length);
      break;
    case PROP_FREQUENCY:
      g_value_set_float (value, self->cutoff);
      break;
    case PROP_MODE:
      g_value_set_enum (value, self->mode);
      break;
    case PROP_WINDOW:
      g_value_set_enum (value, self->window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
