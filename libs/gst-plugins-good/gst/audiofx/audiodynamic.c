/* 
 * GStreamer
 * Copyright (C) 2007 Sebastian Dröge <slomo@circular-chaos.org>
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
 * SECTION:element-audiodynamic
 * @title: audiodynamic
 *
 * This element can act as a compressor or expander. A compressor changes the
 * amplitude of all samples above a specific threshold with a specific ratio,
 * a expander does the same for all samples below a specific threshold. If
 * soft-knee mode is selected the ratio is applied smoothly.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc wave=saw ! audiodynamic characteristics=soft-knee mode=compressor threshold=0.5 ratio=0.5 ! alsasink
 * gst-launch-1.0 filesrc location="melo1.ogg" ! oggdemux ! vorbisdec ! audioconvert ! audiodynamic characteristics=hard-knee mode=expander threshold=0.2 ratio=4.0 ! alsasink
 * gst-launch-1.0 audiotestsrc wave=saw ! audioconvert ! audiodynamic ! audioconvert ! alsasink
 * ]|
 *
 */

/* TODO: Implement attack and release parameters */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#include "audiodynamic.h"

#define GST_CAT_DEFAULT gst_audio_dynamic_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CHARACTERISTICS,
  PROP_MODE,
  PROP_THRESHOLD,
  PROP_RATIO
};

#define ALLOWED_CAPS \
    "audio/x-raw,"                                                \
    " format=(string) {"GST_AUDIO_NE(S16)","GST_AUDIO_NE(F32)"}," \
    " rate=(int)[1,MAX],"                                         \
    " channels=(int)[1,MAX],"                                     \
    " layout=(string) {interleaved, non-interleaved}"

G_DEFINE_TYPE (GstAudioDynamic, gst_audio_dynamic, GST_TYPE_AUDIO_FILTER);

static void gst_audio_dynamic_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_audio_dynamic_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_audio_dynamic_setup (GstAudioFilter * filter,
    const GstAudioInfo * info);
static GstFlowReturn gst_audio_dynamic_transform_ip (GstBaseTransform * base,
    GstBuffer * buf);

static void
gst_audio_dynamic_transform_hard_knee_compressor_int (GstAudioDynamic * filter,
    gint16 * data, guint num_samples);
static void
gst_audio_dynamic_transform_hard_knee_compressor_float (GstAudioDynamic *
    filter, gfloat * data, guint num_samples);
static void
gst_audio_dynamic_transform_soft_knee_compressor_int (GstAudioDynamic * filter,
    gint16 * data, guint num_samples);
static void
gst_audio_dynamic_transform_soft_knee_compressor_float (GstAudioDynamic *
    filter, gfloat * data, guint num_samples);
static void gst_audio_dynamic_transform_hard_knee_expander_int (GstAudioDynamic
    * filter, gint16 * data, guint num_samples);
static void
gst_audio_dynamic_transform_hard_knee_expander_float (GstAudioDynamic * filter,
    gfloat * data, guint num_samples);
static void gst_audio_dynamic_transform_soft_knee_expander_int (GstAudioDynamic
    * filter, gint16 * data, guint num_samples);
static void
gst_audio_dynamic_transform_soft_knee_expander_float (GstAudioDynamic * filter,
    gfloat * data, guint num_samples);

static const GstAudioDynamicProcessFunc process_functions[] = {
  (GstAudioDynamicProcessFunc)
      gst_audio_dynamic_transform_hard_knee_compressor_int,
  (GstAudioDynamicProcessFunc)
      gst_audio_dynamic_transform_hard_knee_compressor_float,
  (GstAudioDynamicProcessFunc)
      gst_audio_dynamic_transform_soft_knee_compressor_int,
  (GstAudioDynamicProcessFunc)
      gst_audio_dynamic_transform_soft_knee_compressor_float,
  (GstAudioDynamicProcessFunc)
      gst_audio_dynamic_transform_hard_knee_expander_int,
  (GstAudioDynamicProcessFunc)
      gst_audio_dynamic_transform_hard_knee_expander_float,
  (GstAudioDynamicProcessFunc)
      gst_audio_dynamic_transform_soft_knee_expander_int,
  (GstAudioDynamicProcessFunc)
  gst_audio_dynamic_transform_soft_knee_expander_float
};

enum
{
  CHARACTERISTICS_HARD_KNEE = 0,
  CHARACTERISTICS_SOFT_KNEE
};

#define GST_TYPE_AUDIO_DYNAMIC_CHARACTERISTICS (gst_audio_dynamic_characteristics_get_type ())
static GType
gst_audio_dynamic_characteristics_get_type (void)
{
  static GType gtype = 0;

  if (gtype == 0) {
    static const GEnumValue values[] = {
      {CHARACTERISTICS_HARD_KNEE, "Hard Knee (default)",
          "hard-knee"},
      {CHARACTERISTICS_SOFT_KNEE, "Soft Knee (smooth)",
          "soft-knee"},
      {0, NULL, NULL}
    };

    gtype = g_enum_register_static ("GstAudioDynamicCharacteristics", values);
  }
  return gtype;
}

enum
{
  MODE_COMPRESSOR = 0,
  MODE_EXPANDER
};

#define GST_TYPE_AUDIO_DYNAMIC_MODE (gst_audio_dynamic_mode_get_type ())
static GType
gst_audio_dynamic_mode_get_type (void)
{
  static GType gtype = 0;

  if (gtype == 0) {
    static const GEnumValue values[] = {
      {MODE_COMPRESSOR, "Compressor (default)",
          "compressor"},
      {MODE_EXPANDER, "Expander", "expander"},
      {0, NULL, NULL}
    };

    gtype = g_enum_register_static ("GstAudioDynamicMode", values);
  }
  return gtype;
}

static void
gst_audio_dynamic_set_process_function (GstAudioDynamic * filter,
    const GstAudioInfo * info)
{
  gint func_index;

  func_index = (filter->mode == MODE_COMPRESSOR) ? 0 : 4;
  func_index += (filter->characteristics == CHARACTERISTICS_HARD_KNEE) ? 0 : 2;
  func_index += (GST_AUDIO_INFO_FORMAT (info) == GST_AUDIO_FORMAT_F32) ? 1 : 0;

  g_assert (func_index >= 0 && func_index < G_N_ELEMENTS (process_functions));

  filter->process = process_functions[func_index];
}

/* GObject vmethod implementations */

static void
gst_audio_dynamic_class_init (GstAudioDynamicClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstCaps *caps;

  GST_DEBUG_CATEGORY_INIT (gst_audio_dynamic_debug, "audiodynamic", 0,
      "audiodynamic element");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_audio_dynamic_set_property;
  gobject_class->get_property = gst_audio_dynamic_get_property;

  g_object_class_install_property (gobject_class, PROP_CHARACTERISTICS,
      g_param_spec_enum ("characteristics", "Characteristics",
          "Selects whether the ratio should be applied smooth (soft-knee) "
          "or hard (hard-knee).",
          GST_TYPE_AUDIO_DYNAMIC_CHARACTERISTICS, CHARACTERISTICS_HARD_KNEE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode",
          "Selects whether the filter should work on loud samples (compressor) or"
          "quiet samples (expander).",
          GST_TYPE_AUDIO_DYNAMIC_MODE, MODE_COMPRESSOR,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_THRESHOLD,
      g_param_spec_float ("threshold", "Threshold",
          "Threshold until the filter is activated", 0.0, 1.0,
          0.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RATIO,
      g_param_spec_float ("ratio", "Ratio",
          "Ratio that should be applied", 0.0, G_MAXFLOAT,
          1.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "Dynamic range controller", "Filter/Effect/Audio",
      "Compressor and Expander", "Sebastian Dröge <slomo@circular-chaos.org>");

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (GST_AUDIO_FILTER_CLASS (klass),
      caps);
  gst_caps_unref (caps);

  GST_AUDIO_FILTER_CLASS (klass)->setup =
      GST_DEBUG_FUNCPTR (gst_audio_dynamic_setup);

  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
      GST_DEBUG_FUNCPTR (gst_audio_dynamic_transform_ip);
  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip_on_passthrough = FALSE;
}

static void
gst_audio_dynamic_init (GstAudioDynamic * filter)
{
  filter->ratio = 1.0;
  filter->threshold = 0.0;
  filter->characteristics = CHARACTERISTICS_HARD_KNEE;
  filter->mode = MODE_COMPRESSOR;
  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (filter), TRUE);
  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (filter), TRUE);
}

static void
gst_audio_dynamic_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioDynamic *filter = GST_AUDIO_DYNAMIC (object);

  switch (prop_id) {
    case PROP_CHARACTERISTICS:
      filter->characteristics = g_value_get_enum (value);
      gst_audio_dynamic_set_process_function (filter,
          GST_AUDIO_FILTER_INFO (filter));
      break;
    case PROP_MODE:
      filter->mode = g_value_get_enum (value);
      gst_audio_dynamic_set_process_function (filter,
          GST_AUDIO_FILTER_INFO (filter));
      break;
    case PROP_THRESHOLD:
      filter->threshold = g_value_get_float (value);
      break;
    case PROP_RATIO:
      filter->ratio = g_value_get_float (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_dynamic_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioDynamic *filter = GST_AUDIO_DYNAMIC (object);

  switch (prop_id) {
    case PROP_CHARACTERISTICS:
      g_value_set_enum (value, filter->characteristics);
      break;
    case PROP_MODE:
      g_value_set_enum (value, filter->mode);
      break;
    case PROP_THRESHOLD:
      g_value_set_float (value, filter->threshold);
      break;
    case PROP_RATIO:
      g_value_set_float (value, filter->ratio);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstAudioFilter vmethod implementations */

static gboolean
gst_audio_dynamic_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  GstAudioDynamic *filter = GST_AUDIO_DYNAMIC (base);

  gst_audio_dynamic_set_process_function (filter, info);
  return TRUE;
}

static void
gst_audio_dynamic_transform_hard_knee_compressor_int (GstAudioDynamic * filter,
    gint16 * data, guint num_samples)
{
  glong val;
  glong thr_p = filter->threshold * G_MAXINT16;
  glong thr_n = filter->threshold * G_MININT16;

  /* Nothing to do for us if ratio is 1.0 or if the threshold
   * equals 1.0. */
  if (filter->threshold == 1.0 || filter->ratio == 1.0)
    return;

  for (; num_samples; num_samples--) {
    val = *data;

    if (val > thr_p) {
      val = thr_p + (val - thr_p) * filter->ratio;
    } else if (val < thr_n) {
      val = thr_n + (val - thr_n) * filter->ratio;
    }
    *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
  }
}

static void
gst_audio_dynamic_transform_hard_knee_compressor_float (GstAudioDynamic *
    filter, gfloat * data, guint num_samples)
{
  gdouble val, threshold = filter->threshold;

  /* Nothing to do for us if ratio == 1.0.
   * As float values can be above 1.0 we have to do something
   * if threshold is greater than 1.0. */
  if (filter->ratio == 1.0)
    return;

  for (; num_samples; num_samples--) {
    val = *data;

    if (val > threshold) {
      val = threshold + (val - threshold) * filter->ratio;
    } else if (val < -threshold) {
      val = -threshold + (val + threshold) * filter->ratio;
    }
    *data++ = (gfloat) val;
  }
}

static void
gst_audio_dynamic_transform_soft_knee_compressor_int (GstAudioDynamic * filter,
    gint16 * data, guint num_samples)
{
  glong val;
  glong thr_p = filter->threshold * G_MAXINT16;
  glong thr_n = filter->threshold * G_MININT16;
  gdouble a_p, b_p, c_p;
  gdouble a_n, b_n, c_n;

  /* Nothing to do for us if ratio is 1.0 or if the threshold
   * equals 1.0. */
  if (filter->threshold == 1.0 || filter->ratio == 1.0)
    return;

  /* We build a 2nd degree polynomial here for
   * values greater than threshold or small than
   * -threshold with:
   * f(t) = t, f'(t) = 1, f'(m) = r
   * =>
   * a = (1-r)/(2*(t-m))
   * b = (r*t - m)/(t-m)
   * c = t * (1 - b - a*t)
   * f(x) = ax^2 + bx + c
   */

  /* shouldn't happen because this would only be the case
   * for threshold == 1.0 which we catch above */
  g_assert (thr_p - G_MAXINT16 != 0);
  g_assert (thr_n - G_MININT != 0);

  a_p = (1 - filter->ratio) / (2 * (thr_p - G_MAXINT16));
  b_p = (filter->ratio * thr_p - G_MAXINT16) / (thr_p - G_MAXINT16);
  c_p = thr_p * (1 - b_p - a_p * thr_p);
  a_n = (1 - filter->ratio) / (2 * (thr_n - G_MININT16));
  b_n = (filter->ratio * thr_n - G_MININT16) / (thr_n - G_MININT16);
  c_n = thr_n * (1 - b_n - a_n * thr_n);

  for (; num_samples; num_samples--) {
    val = *data;

    if (val > thr_p) {
      val = a_p * val * val + b_p * val + c_p;
    } else if (val < thr_n) {
      val = a_n * val * val + b_n * val + c_n;
    }
    *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
  }
}

static void
gst_audio_dynamic_transform_soft_knee_compressor_float (GstAudioDynamic *
    filter, gfloat * data, guint num_samples)
{
  gdouble val;
  gdouble threshold = filter->threshold;
  gdouble a_p, b_p, c_p;
  gdouble a_n, b_n, c_n;

  /* Nothing to do for us if ratio == 1.0.
   * As float values can be above 1.0 we have to do something
   * if threshold is greater than 1.0. */
  if (filter->ratio == 1.0)
    return;

  /* We build a 2nd degree polynomial here for
   * values greater than threshold or small than
   * -threshold with:
   * f(t) = t, f'(t) = 1, f'(m) = r
   * =>
   * a = (1-r)/(2*(t-m))
   * b = (r*t - m)/(t-m)
   * c = t * (1 - b - a*t)
   * f(x) = ax^2 + bx + c
   */

  /* FIXME: If threshold is the same as the maximum
   * we need to raise it a bit to prevent
   * division by zero. */
  if (threshold == 1.0)
    threshold = 1.0 + 0.00001;

  a_p = (1.0 - filter->ratio) / (2.0 * (threshold - 1.0));
  b_p = (filter->ratio * threshold - 1.0) / (threshold - 1.0);
  c_p = threshold * (1.0 - b_p - a_p * threshold);
  a_n = (1.0 - filter->ratio) / (2.0 * (-threshold + 1.0));
  b_n = (-filter->ratio * threshold + 1.0) / (-threshold + 1.0);
  c_n = -threshold * (1.0 - b_n + a_n * threshold);

  for (; num_samples; num_samples--) {
    val = *data;

    if (val > 1.0) {
      val = 1.0 + (val - 1.0) * filter->ratio;
    } else if (val > threshold) {
      val = a_p * val * val + b_p * val + c_p;
    } else if (val < -1.0) {
      val = -1.0 + (val + 1.0) * filter->ratio;
    } else if (val < -threshold) {
      val = a_n * val * val + b_n * val + c_n;
    }
    *data++ = (gfloat) val;
  }
}

static void
gst_audio_dynamic_transform_hard_knee_expander_int (GstAudioDynamic * filter,
    gint16 * data, guint num_samples)
{
  glong val;
  glong thr_p = filter->threshold * G_MAXINT16;
  glong thr_n = filter->threshold * G_MININT16;
  gdouble zero_p, zero_n;

  /* Nothing to do for us here if threshold equals 0.0
   * or ratio equals 1.0 */
  if (filter->threshold == 0.0 || filter->ratio == 1.0)
    return;

  /* zero crossing of our function */
  if (filter->ratio != 0.0) {
    zero_p = thr_p - thr_p / filter->ratio;
    zero_n = thr_n - thr_n / filter->ratio;
  } else {
    zero_p = zero_n = 0.0;
  }

  if (zero_p < 0.0)
    zero_p = 0.0;
  if (zero_n > 0.0)
    zero_n = 0.0;

  for (; num_samples; num_samples--) {
    val = *data;

    if (val < thr_p && val > zero_p) {
      val = filter->ratio * val + thr_p * (1 - filter->ratio);
    } else if ((val <= zero_p && val > 0) || (val >= zero_n && val < 0)) {
      val = 0;
    } else if (val > thr_n && val < zero_n) {
      val = filter->ratio * val + thr_n * (1 - filter->ratio);
    }
    *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
  }
}

static void
gst_audio_dynamic_transform_hard_knee_expander_float (GstAudioDynamic * filter,
    gfloat * data, guint num_samples)
{
  gdouble val, threshold = filter->threshold, zero;

  /* Nothing to do for us here if threshold equals 0.0
   * or ratio equals 1.0 */
  if (filter->threshold == 0.0 || filter->ratio == 1.0)
    return;

  /* zero crossing of our function */
  if (filter->ratio != 0.0)
    zero = threshold - threshold / filter->ratio;
  else
    zero = 0.0;

  if (zero < 0.0)
    zero = 0.0;

  for (; num_samples; num_samples--) {
    val = *data;

    if (val < threshold && val > zero) {
      val = filter->ratio * val + threshold * (1.0 - filter->ratio);
    } else if ((val <= zero && val > 0.0) || (val >= -zero && val < 0.0)) {
      val = 0.0;
    } else if (val > -threshold && val < -zero) {
      val = filter->ratio * val - threshold * (1.0 - filter->ratio);
    }
    *data++ = (gfloat) val;
  }
}

static void
gst_audio_dynamic_transform_soft_knee_expander_int (GstAudioDynamic * filter,
    gint16 * data, guint num_samples)
{
  glong val;
  glong thr_p = filter->threshold * G_MAXINT16;
  glong thr_n = filter->threshold * G_MININT16;
  gdouble zero_p, zero_n;
  gdouble a_p, b_p, c_p;
  gdouble a_n, b_n, c_n;
  gdouble r2;

  /* Nothing to do for us here if threshold equals 0.0
   * or ratio equals 1.0 */
  if (filter->threshold == 0.0 || filter->ratio == 1.0)
    return;

  /* zero crossing of our function */
  zero_p = (thr_p * (filter->ratio - 1.0)) / (1.0 + filter->ratio);
  zero_n = (thr_n * (filter->ratio - 1.0)) / (1.0 + filter->ratio);

  if (zero_p < 0.0)
    zero_p = 0.0;
  if (zero_n > 0.0)
    zero_n = 0.0;

  /* shouldn't happen as this would only happen
   * with threshold == 0.0 */
  g_assert (thr_p != 0);
  g_assert (thr_n != 0);

  /* We build a 2n degree polynomial here for values between
   * 0 and threshold or 0 and -threshold with:
   * f(t) = t, f'(t) = 1, f(z) = 0, f'(z) = r
   * z between 0 and t
   * =>
   * a = (1 - r^2) / (4 * t)
   * b = (1 + r^2) / 2
   * c = t * (1.0 - b - a*t)
   * f(x) = ax^2 + bx + c */
  r2 = filter->ratio * filter->ratio;
  a_p = (1.0 - r2) / (4.0 * thr_p);
  b_p = (1.0 + r2) / 2.0;
  c_p = thr_p * (1.0 - b_p - a_p * thr_p);
  a_n = (1.0 - r2) / (4.0 * thr_n);
  b_n = (1.0 + r2) / 2.0;
  c_n = thr_n * (1.0 - b_n - a_n * thr_n);

  for (; num_samples; num_samples--) {
    val = *data;

    if (val < thr_p && val > zero_p) {
      val = a_p * val * val + b_p * val + c_p;
    } else if ((val <= zero_p && val > 0) || (val >= zero_n && val < 0)) {
      val = 0;
    } else if (val > thr_n && val < zero_n) {
      val = a_n * val * val + b_n * val + c_n;
    }
    *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
  }
}

static void
gst_audio_dynamic_transform_soft_knee_expander_float (GstAudioDynamic * filter,
    gfloat * data, guint num_samples)
{
  gdouble val;
  gdouble threshold = filter->threshold;
  gdouble zero;
  gdouble a_p, b_p, c_p;
  gdouble a_n, b_n, c_n;
  gdouble r2;

  /* Nothing to do for us here if threshold equals 0.0
   * or ratio equals 1.0 */
  if (filter->threshold == 0.0 || filter->ratio == 1.0)
    return;

  /* zero crossing of our function */
  zero = (threshold * (filter->ratio - 1.0)) / (1.0 + filter->ratio);

  if (zero < 0.0)
    zero = 0.0;

  /* shouldn't happen as this only happens with
   * threshold == 0.0 */
  g_assert (threshold != 0.0);

  /* We build a 2n degree polynomial here for values between
   * 0 and threshold or 0 and -threshold with:
   * f(t) = t, f'(t) = 1, f(z) = 0, f'(z) = r
   * z between 0 and t
   * =>
   * a = (1 - r^2) / (4 * t)
   * b = (1 + r^2) / 2
   * c = t * (1.0 - b - a*t)
   * f(x) = ax^2 + bx + c */
  r2 = filter->ratio * filter->ratio;
  a_p = (1.0 - r2) / (4.0 * threshold);
  b_p = (1.0 + r2) / 2.0;
  c_p = threshold * (1.0 - b_p - a_p * threshold);
  a_n = (1.0 - r2) / (-4.0 * threshold);
  b_n = (1.0 + r2) / 2.0;
  c_n = -threshold * (1.0 - b_n + a_n * threshold);

  for (; num_samples; num_samples--) {
    val = *data;

    if (val < threshold && val > zero) {
      val = a_p * val * val + b_p * val + c_p;
    } else if ((val <= zero && val > 0.0) || (val >= -zero && val < 0.0)) {
      val = 0.0;
    } else if (val > -threshold && val < -zero) {
      val = a_n * val * val + b_n * val + c_n;
    }
    *data++ = (gfloat) val;
  }
}

/* GstBaseTransform vmethod implementations */
static GstFlowReturn
gst_audio_dynamic_transform_ip (GstBaseTransform * base, GstBuffer * buf)
{
  GstAudioDynamic *filter = GST_AUDIO_DYNAMIC (base);
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

  if (G_UNLIKELY (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_GAP)))
    return GST_FLOW_OK;

  gst_buffer_map (buf, &map, GST_MAP_READWRITE);
  num_samples = map.size / GST_AUDIO_FILTER_BPS (filter);

  filter->process (filter, map.data, num_samples);

  gst_buffer_unmap (buf, &map);

  return GST_FLOW_OK;
}
