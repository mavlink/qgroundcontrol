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
 * Transformation from lowpass to bandpass/bandreject:
 * http://docs.dewresearch.com/DspHelp/html/IDH_LinearSystems_LowpassToBandPassZ.htm
 * http://docs.dewresearch.com/DspHelp/html/IDH_LinearSystems_LowpassToBandStopZ.htm
 * 
 */

/**
 * SECTION:element-audiochebband
 * @title: audiochebband
 *
 * Attenuates all frequencies outside (bandpass) or inside (bandreject) of a frequency
 * band. The number of poles and the ripple parameter control the rolloff.
 *
 * This element has the advantage over the windowed sinc bandpass and bandreject filter that it is
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
 * gst-launch-1.0 audiotestsrc freq=1500 ! audioconvert ! audiochebband mode=band-pass lower-frequency=1000 upper-frequency=6000 poles=4 ! audioconvert ! alsasink
 * gst-launch-1.0 filesrc location="melo1.ogg" ! oggdemux ! vorbisdec ! audioconvert ! audiochebband mode=band-reject lower-frequency=1000 upper-frequency=4000 ripple=0.2 ! audioconvert ! alsasink
 * gst-launch-1.0 audiotestsrc wave=white-noise ! audioconvert ! audiochebband mode=band-pass lower-frequency=1000 upper-frequency=4000 type=2 ! audioconvert ! alsasink
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

#include "audiochebband.h"

#include "gst/glib-compat-private.h"

#define GST_CAT_DEFAULT gst_audio_cheb_band_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  PROP_0,
  PROP_MODE,
  PROP_TYPE,
  PROP_LOWER_FREQUENCY,
  PROP_UPPER_FREQUENCY,
  PROP_RIPPLE,
  PROP_POLES
};

#define gst_audio_cheb_band_parent_class parent_class
G_DEFINE_TYPE (GstAudioChebBand, gst_audio_cheb_band,
    GST_TYPE_AUDIO_FX_BASE_IIR_FILTER);

static void gst_audio_cheb_band_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_audio_cheb_band_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_audio_cheb_band_finalize (GObject * object);

static gboolean gst_audio_cheb_band_setup (GstAudioFilter * filter,
    const GstAudioInfo * info);

enum
{
  MODE_BAND_PASS = 0,
  MODE_BAND_REJECT
};

#define GST_TYPE_AUDIO_CHEBYSHEV_FREQ_BAND_MODE (gst_audio_cheb_band_mode_get_type ())
static GType
gst_audio_cheb_band_mode_get_type (void)
{
  static GType gtype = 0;

  if (gtype == 0) {
    static const GEnumValue values[] = {
      {MODE_BAND_PASS, "Band pass (default)",
          "band-pass"},
      {MODE_BAND_REJECT, "Band reject",
          "band-reject"},
      {0, NULL, NULL}
    };

    gtype = g_enum_register_static ("GstAudioChebBandMode", values);
  }
  return gtype;
}

/* GObject vmethod implementations */

static void
gst_audio_cheb_band_class_init (GstAudioChebBandClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstAudioFilterClass *filter_class = (GstAudioFilterClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_audio_cheb_band_debug, "audiochebband", 0,
      "audiochebband element");

  gobject_class->set_property = gst_audio_cheb_band_set_property;
  gobject_class->get_property = gst_audio_cheb_band_get_property;
  gobject_class->finalize = gst_audio_cheb_band_finalize;

  g_object_class_install_property (gobject_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode",
          "Low pass or high pass mode", GST_TYPE_AUDIO_CHEBYSHEV_FREQ_BAND_MODE,
          MODE_BAND_PASS,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_TYPE,
      g_param_spec_int ("type", "Type", "Type of the chebychev filter", 1, 2, 1,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  /* FIXME: Don't use the complete possible range but restrict the upper boundary
   * so automatically generated UIs can use a slider without */
  g_object_class_install_property (gobject_class, PROP_LOWER_FREQUENCY,
      g_param_spec_float ("lower-frequency", "Lower frequency",
          "Start frequency of the band (Hz)", 0.0, 100000.0,
          0.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_UPPER_FREQUENCY,
      g_param_spec_float ("upper-frequency", "Upper frequency",
          "Stop frequency of the band (Hz)", 0.0, 100000.0, 0.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_RIPPLE,
      g_param_spec_float ("ripple", "Ripple", "Amount of ripple (dB)", 0.0,
          200.0, 0.25,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  /* FIXME: What to do about this upper boundary? With a frequencies near
   * rate/4 32 poles are completely possible, with frequencies very low
   * or very high 16 poles already produces only noise */
  g_object_class_install_property (gobject_class, PROP_POLES,
      g_param_spec_int ("poles", "Poles",
          "Number of poles to use, will be rounded up to the next multiply of four",
          4, 32, 4,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "Band pass & band reject filter", "Filter/Effect/Audio",
      "Chebyshev band pass and band reject filter",
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  filter_class->setup = GST_DEBUG_FUNCPTR (gst_audio_cheb_band_setup);
}

static void
gst_audio_cheb_band_init (GstAudioChebBand * filter)
{
  filter->lower_frequency = filter->upper_frequency = 0.0;
  filter->mode = MODE_BAND_PASS;
  filter->type = 1;
  filter->poles = 4;
  filter->ripple = 0.25;

  g_mutex_init (&filter->lock);
}

static void
generate_biquad_coefficients (GstAudioChebBand * filter,
    gint p, gint rate, gdouble * b0, gdouble * b1, gdouble * b2, gdouble * b3,
    gdouble * b4, gdouble * a1, gdouble * a2, gdouble * a3, gdouble * a4)
{
  gint np = filter->poles / 2;
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

  /* Calculate inverse of the pole location to move from
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

  /* Convert from lowpass at frequency 1 to either bandpass
   * or band reject.
   *
   * For bandpass substitute z^(-1) with:
   *
   *   -2            -1
   * -z   + alpha * z   - beta
   * ----------------------------
   *         -2            -1
   * beta * z   - alpha * z   + 1
   *
   * alpha = (2*a*b)/(1+b)
   * beta = (b-1)/(b+1)
   * a = cos((w1 + w0)/2) / cos((w1 - w0)/2)
   * b = tan(1/2) * cot((w1 - w0)/2)
   *
   * For bandreject substitute z^(-1) with:
   * 
   *  -2            -1
   * z   - alpha * z   + beta
   * ----------------------------
   *         -2            -1
   * beta * z   - alpha * z   + 1
   *
   * alpha = (2*a)/(1+b)
   * beta = (1-b)/(1+b)
   * a = cos((w1 + w0)/2) / cos((w1 - w0)/2)
   * b = tan(1/2) * tan((w1 - w0)/2)
   *
   */
  {
    gdouble a, b, d;
    gdouble alpha, beta;
    gdouble w0 = 2.0 * G_PI * (filter->lower_frequency / rate);
    gdouble w1 = 2.0 * G_PI * (filter->upper_frequency / rate);

    if (filter->mode == MODE_BAND_PASS) {
      a = cos ((w1 + w0) / 2.0) / cos ((w1 - w0) / 2.0);
      b = tan (1.0 / 2.0) / tan ((w1 - w0) / 2.0);

      alpha = (2.0 * a * b) / (1.0 + b);
      beta = (b - 1.0) / (b + 1.0);

      d = 1.0 + beta * (y1 - beta * y2);

      *b0 = (x0 + beta * (-x1 + beta * x2)) / d;
      *b1 = (alpha * (-2.0 * x0 + x1 + beta * x1 - 2.0 * beta * x2)) / d;
      *b2 =
          (-x1 - beta * beta * x1 + 2.0 * beta * (x0 + x2) +
          alpha * alpha * (x0 - x1 + x2)) / d;
      *b3 = (alpha * (x1 + beta * (-2.0 * x0 + x1) - 2.0 * x2)) / d;
      *b4 = (beta * (beta * x0 - x1) + x2) / d;
      *a1 = (alpha * (2.0 + y1 + beta * y1 - 2.0 * beta * y2)) / d;
      *a2 =
          (-y1 - beta * beta * y1 - alpha * alpha * (1.0 + y1 - y2) +
          2.0 * beta * (-1.0 + y2)) / d;
      *a3 = (alpha * (y1 + beta * (2.0 + y1) - 2.0 * y2)) / d;
      *a4 = (-beta * beta - beta * y1 + y2) / d;
    } else {
      a = cos ((w1 + w0) / 2.0) / cos ((w1 - w0) / 2.0);
      b = tan (1.0 / 2.0) * tan ((w1 - w0) / 2.0);

      alpha = (2.0 * a) / (1.0 + b);
      beta = (1.0 - b) / (1.0 + b);

      d = -1.0 + beta * (beta * y2 + y1);

      *b0 = (-x0 - beta * x1 - beta * beta * x2) / d;
      *b1 = (alpha * (2.0 * x0 + x1 + beta * x1 + 2.0 * beta * x2)) / d;
      *b2 =
          (-x1 - beta * beta * x1 - 2.0 * beta * (x0 + x2) -
          alpha * alpha * (x0 + x1 + x2)) / d;
      *b3 = (alpha * (x1 + beta * (2.0 * x0 + x1) + 2.0 * x2)) / d;
      *b4 = (-beta * beta * x0 - beta * x1 - x2) / d;
      *a1 = (alpha * (-2.0 + y1 + beta * y1 + 2.0 * beta * y2)) / d;
      *a2 =
          -(y1 + beta * beta * y1 + 2.0 * beta * (-1.0 + y2) +
          alpha * alpha * (-1.0 + y1 + y2)) / d;
      *a3 = (alpha * (beta * (-2.0 + y1) + y1 + 2.0 * y2)) / d;
      *a4 = -(-beta * beta + beta * y1 + y2) / d;
    }
  }
}

static void
generate_coefficients (GstAudioChebBand * filter, const GstAudioInfo * info)
{
  gint rate;

  if (info) {
    rate = GST_AUDIO_INFO_RATE (info);
  } else {
    rate = GST_AUDIO_FILTER_RATE (filter);
  }

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

  if (filter->upper_frequency <= filter->lower_frequency) {
    gdouble *a = g_new0 (gdouble, 1);
    gdouble *b = g_new0 (gdouble, 1);

    a[0] = 1.0;
    b[0] = (filter->mode == MODE_BAND_PASS) ? 0.0 : 1.0;
    gst_audio_fx_base_iir_filter_set_coefficients (GST_AUDIO_FX_BASE_IIR_FILTER
        (filter), a, 1, b, 1);

    GST_LOG_OBJECT (filter, "frequency band had no or negative dimension");
    return;
  }

  if (filter->upper_frequency > rate / 2) {
    filter->upper_frequency = rate / 2;
    GST_LOG_OBJECT (filter, "clipped upper frequency to nyquist frequency");
  }

  if (filter->lower_frequency < 0.0) {
    filter->lower_frequency = 0.0;
    GST_LOG_OBJECT (filter, "clipped lower frequency to 0.0");
  }

  /* Calculate coefficients for the chebyshev filter */
  {
    gint np = filter->poles;
    gdouble *a, *b;
    gint i, p;

    a = g_new0 (gdouble, np + 5);
    b = g_new0 (gdouble, np + 5);

    /* Calculate transfer function coefficients */
    a[4] = 1.0;
    b[4] = 1.0;

    for (p = 1; p <= np / 4; p++) {
      gdouble b0, b1, b2, b3, b4, a1, a2, a3, a4;
      gdouble *ta = g_new0 (gdouble, np + 5);
      gdouble *tb = g_new0 (gdouble, np + 5);

      generate_biquad_coefficients (filter, p, rate,
          &b0, &b1, &b2, &b3, &b4, &a1, &a2, &a3, &a4);

      memcpy (ta, a, sizeof (gdouble) * (np + 5));
      memcpy (tb, b, sizeof (gdouble) * (np + 5));

      /* add the new coefficients for the new two poles
       * to the cascade by multiplication of the transfer
       * functions */
      for (i = 4; i < np + 5; i++) {
        b[i] =
            b0 * tb[i] + b1 * tb[i - 1] + b2 * tb[i - 2] + b3 * tb[i - 3] +
            b4 * tb[i - 4];
        a[i] =
            ta[i] - a1 * ta[i - 1] - a2 * ta[i - 2] - a3 * ta[i - 3] -
            a4 * ta[i - 4];
      }
      g_free (ta);
      g_free (tb);
    }

    /* Move coefficients to the beginning of the array to move from
     * the transfer function's coefficients to the difference
     * equation's coefficients */
    for (i = 0; i <= np; i++) {
      a[i] = a[i + 4];
      b[i] = b[i + 4];
    }

    /* Normalize to unity gain at frequency 0 and frequency
     * 0.5 for bandreject and unity gain at band center frequency
     * for bandpass */
    if (filter->mode == MODE_BAND_REJECT) {
      /* gain is sqrt(H(0)*H(0.5)) */

      gdouble gain1 =
          gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1, b, np + 1,
          1.0, 0.0);
      gdouble gain2 =
          gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1, b, np + 1,
          -1.0, 0.0);

      gain1 = sqrt (gain1 * gain2);

      for (i = 0; i <= np; i++) {
        b[i] /= gain1;
      }
    } else {
      /* gain is H(wc), wc = center frequency */

      gdouble w1 = 2.0 * G_PI * (filter->lower_frequency / rate);
      gdouble w2 = 2.0 * G_PI * (filter->upper_frequency / rate);
      gdouble w0 = (w2 + w1) / 2.0;
      gdouble zr = cos (w0), zi = sin (w0);
      gdouble gain =
          gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1, b, np + 1, zr,
          zi);

      for (i = 0; i <= np; i++) {
        b[i] /= gain;
      }
    }

    gst_audio_fx_base_iir_filter_set_coefficients (GST_AUDIO_FX_BASE_IIR_FILTER
        (filter), a, np + 1, b, np + 1);

    GST_LOG_OBJECT (filter,
        "Generated IIR coefficients for the Chebyshev filter");
    GST_LOG_OBJECT (filter,
        "mode: %s, type: %d, poles: %d, lower-frequency: %.2f Hz, upper-frequency: %.2f Hz, ripple: %.2f dB",
        (filter->mode == MODE_BAND_PASS) ? "band-pass" : "band-reject",
        filter->type, filter->poles, filter->lower_frequency,
        filter->upper_frequency, filter->ripple);

    GST_LOG_OBJECT (filter, "%.2f dB gain @ 0Hz",
        20.0 * log10 (gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1, b,
                np + 1, 1.0, 0.0)));
    {
      gdouble w1 = 2.0 * G_PI * (filter->lower_frequency / rate);
      gdouble w2 = 2.0 * G_PI * (filter->upper_frequency / rate);
      gdouble w0 = (w2 + w1) / 2.0;
      gdouble zr, zi;

      zr = cos (w1);
      zi = sin (w1);
      GST_LOG_OBJECT (filter, "%.2f dB gain @ %dHz",
          20.0 * log10 (gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1,
                  b, np + 1, zr, zi)), (int) filter->lower_frequency);
      zr = cos (w0);
      zi = sin (w0);
      GST_LOG_OBJECT (filter, "%.2f dB gain @ %dHz",
          20.0 * log10 (gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1,
                  b, np + 1, zr, zi)),
          (int) ((filter->lower_frequency + filter->upper_frequency) / 2.0));
      zr = cos (w2);
      zi = sin (w2);
      GST_LOG_OBJECT (filter, "%.2f dB gain @ %dHz",
          20.0 * log10 (gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1,
                  b, np + 1, zr, zi)), (int) filter->upper_frequency);
    }
    GST_LOG_OBJECT (filter, "%.2f dB gain @ %dHz",
        20.0 * log10 (gst_audio_fx_base_iir_filter_calculate_gain (a, np + 1, b,
                np + 1, -1.0, 0.0)), rate / 2);
  }
}

static void
gst_audio_cheb_band_finalize (GObject * object)
{
  GstAudioChebBand *filter = GST_AUDIO_CHEB_BAND (object);

  g_mutex_clear (&filter->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_audio_cheb_band_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioChebBand *filter = GST_AUDIO_CHEB_BAND (object);

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
    case PROP_LOWER_FREQUENCY:
      g_mutex_lock (&filter->lock);
      filter->lower_frequency = g_value_get_float (value);
      generate_coefficients (filter, NULL);
      g_mutex_unlock (&filter->lock);
      break;
    case PROP_UPPER_FREQUENCY:
      g_mutex_lock (&filter->lock);
      filter->upper_frequency = g_value_get_float (value);
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
      filter->poles = GST_ROUND_UP_4 (g_value_get_int (value));
      generate_coefficients (filter, NULL);
      g_mutex_unlock (&filter->lock);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_cheb_band_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioChebBand *filter = GST_AUDIO_CHEB_BAND (object);

  switch (prop_id) {
    case PROP_MODE:
      g_value_set_enum (value, filter->mode);
      break;
    case PROP_TYPE:
      g_value_set_int (value, filter->type);
      break;
    case PROP_LOWER_FREQUENCY:
      g_value_set_float (value, filter->lower_frequency);
      break;
    case PROP_UPPER_FREQUENCY:
      g_value_set_float (value, filter->upper_frequency);
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
gst_audio_cheb_band_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  GstAudioChebBand *filter = GST_AUDIO_CHEB_BAND (base);

  generate_coefficients (filter, info);

  return GST_AUDIO_FILTER_CLASS (parent_class)->setup (base, info);
}
