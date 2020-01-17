/* 
 * GStreamer
 * Copyright (C) 2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

/**
 * SECTION:element-audiofirfilter
 * @title: audiofirfilter
 *
 * audiofirfilter implements a generic audio
 * [FIR filter](http://en.wikipedia.org/wiki/Finite_impulse_response). Before
 * usage the "kernel" property has to be set to the filter kernel that should be
 * used and the "latency" property has to be set to the latency (in samples)
 * that is introduced by the filter kernel. Setting a latency of n samples
 * will lead to the first n samples being dropped from the output and
 * n samples added to the end.
 *
 * The filter kernel describes the impulse response of the filter. To
 * calculate the frequency response of the filter you have to calculate
 * the Fourier Transform of the impulse response.
 *
 * To change the filter kernel whenever the sampling rate changes the
 * "rate-changed" signal can be used. This should be done for most
 * FIR filters as they're depending on the sampling rate.
 *
 * ## Example application
 * <programlisting language="C">
 * <xi:include xmlns:xi="http://www.w3.org/2003/XInclude" parse="text" href="../../../../tests/examples/audiofx/firfilter-example.c" />
 * ]|
 *
 */

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <gst/audio/gstaudiofilter.h>

#include "audiofirfilter.h"

#include "gst/glib-compat-private.h"

#define GST_CAT_DEFAULT gst_audio_fir_filter_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  SIGNAL_RATE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_KERNEL,
  PROP_LATENCY
};

static guint gst_audio_fir_filter_signals[LAST_SIGNAL] = { 0, };

#define gst_audio_fir_filter_parent_class parent_class
G_DEFINE_TYPE (GstAudioFIRFilter, gst_audio_fir_filter,
    GST_TYPE_AUDIO_FX_BASE_FIR_FILTER);

static void gst_audio_fir_filter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_audio_fir_filter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_audio_fir_filter_finalize (GObject * object);

static gboolean gst_audio_fir_filter_setup (GstAudioFilter * base,
    const GstAudioInfo * info);


static void
gst_audio_fir_filter_class_init (GstAudioFIRFilterClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstAudioFilterClass *filter_class = (GstAudioFilterClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_audio_fir_filter_debug, "audiofirfilter", 0,
      "Generic audio FIR filter plugin");

  gobject_class->set_property = gst_audio_fir_filter_set_property;
  gobject_class->get_property = gst_audio_fir_filter_get_property;
  gobject_class->finalize = gst_audio_fir_filter_finalize;

  g_object_class_install_property (gobject_class, PROP_KERNEL,
      g_param_spec_value_array ("kernel", "Filter Kernel",
          "Filter kernel for the FIR filter",
          g_param_spec_double ("Element", "Filter Kernel Element",
              "Element of the filter kernel", -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_LATENCY,
      g_param_spec_uint64 ("latency", "Latecy",
          "Filter latency in samples",
          0, G_MAXUINT64, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  filter_class->setup = GST_DEBUG_FUNCPTR (gst_audio_fir_filter_setup);

  /**
   * GstAudioFIRFilter::rate-changed:
   * @filter: the filter on which the signal is emitted
   * @rate: the new sampling rate
   *
   * Will be emitted when the sampling rate changes. The callbacks
   * will be called from the streaming thread and processing will
   * stop until the event is handled.
   */
  gst_audio_fir_filter_signals[SIGNAL_RATE_CHANGED] =
      g_signal_new ("rate-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstAudioFIRFilterClass, rate_changed),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_INT);

  gst_element_class_set_static_metadata (gstelement_class,
      "Audio FIR filter", "Filter/Effect/Audio",
      "Generic audio FIR filter with custom filter kernel",
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");
}

static void
gst_audio_fir_filter_update_kernel (GstAudioFIRFilter * self, GValueArray * va)
{
  gdouble *kernel;
  guint i;

  if (va) {
    if (self->kernel)
      g_value_array_free (self->kernel);

    self->kernel = va;
  }

  kernel = g_new (gdouble, self->kernel->n_values);

  for (i = 0; i < self->kernel->n_values; i++) {
    GValue *v = g_value_array_get_nth (self->kernel, i);
    kernel[i] = g_value_get_double (v);
  }

  gst_audio_fx_base_fir_filter_set_kernel (GST_AUDIO_FX_BASE_FIR_FILTER (self),
      kernel, self->kernel->n_values, self->latency, NULL);
}

static void
gst_audio_fir_filter_init (GstAudioFIRFilter * self)
{
  GValue v = { 0, };
  GValueArray *va;

  self->latency = 0;
  va = g_value_array_new (1);

  g_value_init (&v, G_TYPE_DOUBLE);
  g_value_set_double (&v, 1.0);
  g_value_array_append (va, &v);
  g_value_unset (&v);
  gst_audio_fir_filter_update_kernel (self, va);

  g_mutex_init (&self->lock);
}

/* GstAudioFilter vmethod implementations */

/* get notified of caps and plug in the correct process function */
static gboolean
gst_audio_fir_filter_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  GstAudioFIRFilter *self = GST_AUDIO_FIR_FILTER (base);
  gint new_rate = GST_AUDIO_INFO_RATE (info);

  if (GST_AUDIO_FILTER_RATE (self) != new_rate) {
    g_signal_emit (G_OBJECT (self),
        gst_audio_fir_filter_signals[SIGNAL_RATE_CHANGED], 0, new_rate);
  }

  return GST_AUDIO_FILTER_CLASS (parent_class)->setup (base, info);
}

static void
gst_audio_fir_filter_finalize (GObject * object)
{
  GstAudioFIRFilter *self = GST_AUDIO_FIR_FILTER (object);

  g_mutex_clear (&self->lock);

  if (self->kernel)
    g_value_array_free (self->kernel);
  self->kernel = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_audio_fir_filter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioFIRFilter *self = GST_AUDIO_FIR_FILTER (object);

  g_return_if_fail (GST_IS_AUDIO_FIR_FILTER (self));

  switch (prop_id) {
    case PROP_KERNEL:
      g_mutex_lock (&self->lock);
      /* update kernel already pushes residues */
      gst_audio_fir_filter_update_kernel (self, g_value_dup_boxed (value));
      g_mutex_unlock (&self->lock);
      break;
    case PROP_LATENCY:
      g_mutex_lock (&self->lock);
      self->latency = g_value_get_uint64 (value);
      gst_audio_fir_filter_update_kernel (self, NULL);
      g_mutex_unlock (&self->lock);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_fir_filter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioFIRFilter *self = GST_AUDIO_FIR_FILTER (object);

  switch (prop_id) {
    case PROP_KERNEL:
      g_value_set_boxed (value, self->kernel);
      break;
    case PROP_LATENCY:
      g_value_set_uint64 (value, self->latency);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
