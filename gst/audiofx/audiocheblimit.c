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

/* 
 * Chebyshev type 1 filter design based on
 * "The Scientist and Engineer's Guide to DSP", Chapter 20.
 * http://www.dspguide.com/
 *
 * For type 2 and Chebyshev filters in general read
 * http://en.wikipedia.org/wiki/Chebyshev_filter
 *
 */

/**
 * SECTION:element-audiocheblimit
 * @title: audiocheblimit
 *
 * Attenuates all frequencies above the cutoff frequency (low-pass) or all frequencies below the
 * cutoff frequency (high-pass). The number of poles and the ripple parameter control the rolloff.
 *
 * This element has the advantage over the windowed sinc lowpass and highpass filter that it is
 * much faster and produces almost as good results. It's only disadvantages are the highly
 * non-linear phase and the slower rolloff compared to a windowed sinc filter with a large kernel.
 *
 * For type 1 the ripple parameter specifies how much ripple in dB is allowed in the passband, i.e.
 * some frequencies in the passband will be amplified by that value. A higher ripple value will allow
 * a faster rolloff.
 *
 * For type 2 the ripple parameter specifies the stopband attenuation. In the stopband the gain will
 * be at most this value. A lower ripple value will allow a faster rolloff.
 *
 * As a special case, a Chebyshev type 1 filter with no ripple is a Butterworth filter.
 *
 * > Be warned that a too large number of poles can produce noise. The most poles are possible with
 * > a cutoff frequency at a quarter of the sampling rate.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc freq=1500 ! audioconvert ! audiocheblimit mode=low-pass cutoff=1000 poles=4 ! audioconvert ! alsasink
 * gst-launch-1.0 filesrc location="melo1.ogg" ! oggdemux ! vorbisdec ! audioconvert ! audiocheblimit mode=high-pass cutoff=400 ripple=0.2 ! audioconvert ! alsasink
 * gst-launch-1.0 audiotestsrc wave=white-noise ! audioconvert ! audiocheblimit mode=low-pass cutoff=800 type=2 ! audioconvert ! alsasink
 * ]|
 *
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

#include "math_compat.h"

#include "audiocheblimit.h"

#include "gst/glib-compat-private.h"

#define GST_CAT_DEFAULT gst_audio_cheb_limit_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  PROP_0,
  PROP_MODE,
  PROP_TYPE,
  PROP_CUTOFF,
  PROP_RIPPLE,
  PROP_POLES
};

#define gst_audio_cheb_limit_parent_class parent_class
G_DEFINE_TYPE (GstAudioChebLimit,
    gst_audio_cheb_limit, GST_TYPE_AUDIO_FX_BASE_IIR_FILTER);

static void gst_audio_cheb_limit_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_audio_cheb_limit_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_audio_cheb_limit_finalize (GObject * object);

static gboolean gst_audio_cheb_limit_setup (GstAudioFilter * filter,
    const GstAudioInfo * info);

enum
{
  MODE_LOW_PASS = 0,
  MODE_HIGH_PASS
};

#define GST_TYPE_AUDIO_CHEBYSHEV_FREQ_LIMIT_MODE (gst_audio_cheb_limit_mode_get_type ())
static GType
gst_audio_cheb_limit_mode_get_type (void)
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

    gtype = g_enum_register_static ("GstAudioChebLimitMode", values);
  }
  return gtype;
}

/* GObject vmethod implementations */

static void
gst_audio_cheb_limit_class_init (GstAudioChebLimitClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstAudioFilterClass *filter_class = (GstAudioFilterClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_audio_cheb_limit_debug, "audiocheblimit", 0,
      "audiocheblimit element");

  gobject_class->set_property = gst_audio_cheb_limit_set_property;
  gobject_class->get_property = gst_audio_cheb_limit_get_property;
  gobject_class->finalize = gst_audio_cheb_limit_finalize;

  g_object_class_install_property (gobject_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode",
          "Low pass or high pass mode",
          GST_TYPE_AUDIO_CHEBYSHEV_FREQ_LIMIT_MODE, MODE_LOW_PASS,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_TYPE,
      g_param_spec_int ("type", "Type", "Type of the chebychev filter", 1, 2, 1,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  /* FIXME: Don't use the complete possible range but restrict the upper boundary
   * so automatically generated UIs can use a slider without */
  g_object_class_install_property (gobject_class, PROP_CUTOFF,
      g_param_spec_float ("cutoff", "Cutoff", "Cut off frequency (Hz)", 0.0,
          100000.0, 0.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_RIPPLE,
      g_param_spec_float ("ripple", "Ripple", "Amount of ripple (dB)", 0.0,
          200.0, 0.25,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  /* FIXME: What to do about this upper boundary? With a cutoff frequency of
   * rate/4 32 poles are completely possible, with a cutoff frequency very low
   * or very high 16 poles already produces only noise */
  g_object_class_install_property (gobject_class, PROP_POLES,
      g_param_spec_int ("poles", "Poles",
          "Number of poles to use, will be rounded up to the next even number",
          2, 32, 4,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "Low pass & high pass filter",
      "Filter/Effect/Audio",
      "Chebyshev low pass and high pass filter",
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  filter_class->setup = GST_DEBUG_FUNCPTR (gst_audio_cheb_limit_setup);
}

static void
gst_audio_cheb_limit_init (GstAudioChebLimit * filter)
{
  filter->cutoff = 0.0;
  filter->mode = MODE_LOW_PASS;
  filter->type = 1;
  filter->poles = 4;
  filter->ripple = 0.25;

  g_mutex_init (&filter->lock);
}

static void
generate_biquad_coefficients (GstAudioChebLimit * filter,
    gint p, gint rate, gdouble * b0, gdouble * b1, gdouble * b2,
    gdouble * a1, gdouble * a2)
{
  gint np = filter->poles;
  gdouble ripple = filter->ripple;

  /* pole location in s-plane */
  gdouble rp, ip;

  /* zero location in s-plane */
  gdouble iz = 0.0;

  /* transfer function coefficients for the z-plane */
  gdouble x0, x1, x2, y1, y2;
  gint type = filter->type;

  /* Calculate pole location for lowpass at frequency 1 */
  {
    gdouble angle = (G_PI / 2.0) * (2.0 * p - 1) / np;

    rp = -sin (angle);
    ip = cos (angle);
  }

  /* If we allow ripple, move the pole from the unit
   * circle to an ellipse and keep cutoff at frequency 1 */
  if (ripple > 0 && type == 1) {
    gdouble es, vx;

    es = sqrt (pow (10.0, ripple / 10.0) - 1.0);

    vx = (1.0 / np) * asinh (1.0 / es);
    rp = rp * sinh (vx);
    ip = ip * cosh (vx);
  } else if (type == 2) {
    gdouble es, vx;

    es = sqrt (pow (10.0, ripple / 10.0) - 1.0);
    vx = (1.0 / np) * asinh (es);
    rp = rp * sinh (vx);
    ip = ip * cosh (vx);
  }

  /* Calculate inverse of the pole location to convert from
   * type I to type II */
  if (type == 2) {
    gdouble mag2 = rp * rp + ip * ip;

    rp /= mag2;
    ip /= mag2;
  }

  /* Calculate zero location for frequency 1 on the
   * unit circle for type 2 */
  if (type == 2) {
    gdouble angle = G_PI / (np * 2.0) + ((p - 1) * G_PI) / (np);
    gdouble mag2;

    iz = cos (angle);
    mag2 = iz * iz;
    iz /= mag2;
  }

  /* Convert from s-domain to z-domain by
   * using the bilinear Z-transform, i.e.
   * substitute s by (2/t)*((z-1)/(z+1))
   * with t = 2 * tan(0.5).
   */
  if (type == 1) {
    gdouble t, m, d;

    t = 2.0 * tan (0.5);
    m = rp * rp + ip * ip;
    d = 4.0 - 4.0 * rp * t + m * t * t;

    x0 = (t * t) / d;
    x1 = 2.0 * x0;
    x2 = x0;
    y1 = (8.0 - 2.0 * m * t * t) / d;
    y2 = (-4.0 - 4.0 * rp * t - m * t * t) / d;
  } else {
    gdouble t, m, d;

    t = 2.0 * tan (0.5);
    m = rp * rp + ip * ip;
    d = 4.0 - 4.0 * rp * t + m * t * t;

    x0 = (t * t * iz * iz + 4.0) / d;
    x1 = (-8.0 + 2.0 * iz * iz * t * t) / d;
    x2 = x0;
    y1 = (8.0 - 2.0 * m * t * t) / d;
    y2 = (-4.0 - 4.0 * rp * t - m * t * t) / d;
  }

  /* Convert from lowpass at frequency 1 to either lowpass
   * or highpass.
   *
   * For lowpass substitute z^(-1) with:
   *  -1
   * z   - k
   * ------------
   *          -1
   * 1 - k * z
   *
   * k = sin((1-w)/2) / sin((1+w)/2)
   *
   * For highpass substitute z^(-1) with:
   *
   *   -1
   * -z   - k
   * ------------
   *          -1
   * 1 + k * z
   *
   * k = -cos((1+w)/2) / cos((1-w)/2)
   *
   */
  {
    gdouble k, d;
    gdouble omega = 2.0 * G_PI * (filter->cutoff / rate);

    if (filter->mode == MODE_LOW_PASS)
      k = sin ((1.0 - omega) / 2.0) / sin ((1.0 + omega) / 2.0);
    else
      k = -cos ((omega + 1.0) / 2.0) / cos ((omega - 1.0) / 2.0);

    d = 1.0 + y1 * k - y2 * k * k;
    *b0 = (x0 + k * (-x1 + k * x2)) / d;
    *b1 = (x1 + k * k * x1 - 2.0 * k * (x0 + x2)) / d;
    *b2 = (x0 * k * k - x1 * k + x2) / d;
    *a1 = (2.0 * k + y1 + y1 * k * k - 2.0 * y2 * k) / d;
    *a2 = (-k * k - y1 * k + y2) / d;

    if (filter->mode == MODE_HIGH_PASS) {
      *a1 = -*a1;
      *b1 = -*b1;
    }
  }
}

static void
generate_coefficients (GstAudioChebLimit * filter, const GstAudioInfo * info)
{
  gint rate;

  if (info) {
    rate = GST_AUDIO_INFO_RATE (info);
  } else {
    rate = GST_AUDIO_FILTER_RATE (filter);
  }

  GST_LOG_OBJECT (filter, "cutoff %f", filter->cutoff);

  if (rate == 0) {
    gdouble *a = g_new0 (gdouble, 1);
    gdouble *b = g_new0 (gdouble, 1);

    a[0] = 1.0;
    b[0] = 1.0;
    gst_audio_fx_base_iir_filter_set_coefficients (GST_AUDIO_FX_BASE_IIR_FILTER
        (filter), a, 1, b, 1);

    GST_LOG_OBJECT (filter, "rate was not set yet");
    return;
  }

  if (filter->cutoff >= rate / 2.0) {
    gdouble *a = g_new0 (gdouble, 1);
    gdouble *b = g_new0 (gdouble, 1);

    a[0] = 1.0;
    b[0] = (filter->mode == MODE_LOW_PASS) ? 1.0 : 0.0;
    gst_audio_fx_base_iir_filter_set_coefficients (GST_AUDIO_FX_BASE_IIR_FILTER
        (filter), a, 1, b, 1);
    GST_LOG_OBJECT (filter, "cutoff was higher than nyquist frequency");
    return;
  } else if (filter->cutoff <= 0.0) {
    gdouble *a = g_new0 (gdouble, 1);
    gdouble *b = g_new0 (gdouble, 1);

    a[0] = 1.0;
    b[0] = (filter->mode == MODE_LOW_PASS) ? 0.0 : 1.0;
    gst_audio_fx_base_iir_filter_set_coefficients (GST_AUDIO_FX_BASE_IIR_FILTER
        (filter), a, 1, b, 1);
    GST_LOG_OBJECT (filter, "cutoff is lower than zero");
    return;
  }

  /* Calculate coefficients for the chebyshev filter */
  {
    gint np = filter->poles;
    gdouble *a, *b;
    gint i, p;

    a = g_new0 (gdouble, np + 3);
    b = g_new0 (gdouble, np + 3);

    /* Calculate transfer function coefficients */
    a[2] = 1.0;
    b[2] = 1.0;

    for (p = 1; p <= np / 2; p++) {
      gdouble b0, b1, b2, a1, a2;
      gdouble *ta = g_new0 (gdouble, np + 3);
      gdouble *tb = g_new0 (gdouble, np + 3);

      generate_biquad_coefficients (filter, p, rate, &b0, &b1, &b2, &a1, &a2);

      memcpy (ta, a, sizeof (gdouble) * (np + 3));
      memcpy (tb, b, sizeof (gdouble) * (np + 3));

      /* add the new coefficients for the new two poles
       * to the cascade by multiplication of the transfer
       * functions */
      for (i = 2; i < np + 3; i++) {
        b[i] = b0 * tb[i] + b1 * tb[i - 1] + b2 * tb[i - 2];
        a[i] = ta[i] - a1 * ta[i - 1] - a2 * ta[i - 2];
      }
      g_free (ta);
      g_free (tb);
    }

    /* Move coefficients to the beginning of the array to move from
     * the transfer function's coefficients to the difference
     * equation's coefficients */
    for (i = 0; i <= np; i++) {
      a[i] = a[i + 2];
      b[i] = b[i + 2];
    }

    /* Normalize to unity gain at frequency 0 for lowpass
     * and frequency 0.5 for highpass */
    {
      gdouble gain;

      if (filter->mode == MODE_LOW_PASS)
        gain =
            gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1, b, np + 1,
            1.0, 0.0);
      else
        gain =
            gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1, b, np + 1,
            -1.0, 0.0);

      for (i = 0; i <= np; i++) {
        b[i] /= gain;
      }
    }

    gst_audio_fx_base_iir_filter_set_coefficients (GST_AUDIO_FX_BASE_IIR_FILTER
        (filter), a, np + 1, b, np + 1);

    GST_LOG_OBJECT (filter,
        "Generated IIR coefficients for the Chebyshev filter");
    GST_LOG_OBJECT (filter,
        "mode: %s, type: %d, poles: %d, cutoff: %.2f Hz, ripple: %.2f dB",
        (filter->mode == MODE_LOW_PASS) ? "low-pass" : "high-pass",
        filter->type, filter->poles, filter->cutoff, filter->ripple);
    GST_LOG_OBJECT (filter, "%.2f dB gain @ 0 Hz",
        20.0 * log10 (gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1, b,
                np + 1, 1.0, 0.0)));

#ifndef GST_DISABLE_GST_DEBUG
    {
      gdouble wc = 2.0 * G_PI * (filter->cutoff / rate);
      gdouble zr = cos (wc), zi = sin (wc);

      GST_LOG_OBJECT (filter, "%.2f dB gain @ %d Hz",
          20.0 * log10 (gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1,
                  b, np + 1, zr, zi)), (int) filter->cutoff);
    }
#endif

    GST_LOG_OBJECT (filter, "%.2f dB gain @ %d Hz",
        20.0 * log10 (gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1, b,
                np + 1, -1.0, 0.0)), rate);
  }
}

static void
gst_audio_cheb_limit_finalize (GObject * object)
{
  GstAudioChebLimit *filter = GST_AUDIO_CHEB_LIMIT (object);

  g_mutex_clear (&filter->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_audio_cheb_limit_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioChebLimit *filter = GST_AUDIO_CHEB_LIMIT (object);

  switch (prop_id) {
    case PROP_MODE:
      g_mutex_lock (&filter->lock);
      filter->mode = g_value_get_enum (value);
      generate_coefficients (filter, NULL);
      g_mutex_unlock (&filter->lock);
      break;
    case PROP_TYPE:
      g_mutex_lock (&filter->lock);
      filter->type = g_value_get_int (value);
      generate_coefficients (filter, NULL);
      g_mutex_unlock (&filter->lock);
      break;
    case PROP_CUTOFF:
      g_mutex_lock (&filter->lock);
      filter->cutoff = g_value_get_float (value);
      generate_coefficients (filter, NULL);
      g_mutex_unlock (&filter->lock);
      break;
    case PROP_RIPPLE:
      g_mutex_lock (&filter->lock);
      filter->ripple = g_value_get_float (value);
      generate_coefficients (filter, NULL);
      g_mutex_unlock (&filter->lock);
      break;
    case PROP_POLES:
      g_mutex_lock (&filter->lock);
      filter->poles = GST_ROUND_UP_2 (g_value_get_int (value));
      generate_coefficients (filter, NULL);
      g_mutex_unlock (&filter->lock);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_cheb_limit_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioChebLimit *filter = GST_AUDIO_CHEB_LIMIT (object);

  switch (prop_id) {
    case PROP_MODE:
      g_value_set_enum (value, filter->mode);
      break;
    case PROP_TYPE:
      g_value_set_int (value, filter->type);
      break;
    case PROP_CUTOFF:
      g_value_set_float (value, filter->cutoff);
      break;
    case PROP_RIPPLE:
      g_value_set_float (value, filter->ripple);
      break;
    case PROP_POLES:
      g_value_set_int (value, filter->poles);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstAudioFilter vmethod implementations */

static gboolean
gst_audio_cheb_limit_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  GstAudioChebLimit *filter = GST_AUDIO_CHEB_LIMIT (base);

  generate_coefficients (filter, info);

  return GST_AUDIO_FILTER_CLASS (parent_class)->setup (base, info);
}
