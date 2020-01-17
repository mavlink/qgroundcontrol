/* GStreamer
 * Copyright (C) <2004> Benjamin Otte <otte@gnome.org>
 *               <2007> Stefan Kost <ensonic@users.sf.net>
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
 * SECTION:element-equalizer-nbands
 * @title: equalizer-nbands
 *
 * The n-band equalizer element is a fully parametric equalizer. It allows to
 * select between 1 and 64 bands and has properties on each band to change
 * the center frequency, band width and gain.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=song.ogg ! oggdemux ! vorbisdec ! audioconvert ! equalizer-nbands num-bands=15 band5::gain=6.0 ! alsasink
 * ]| This make the equalizer use 15 bands and raises the volume of the 5th band by 6 db.
 *
 * ## Example code
 * |[
 * #include &lt;gst/gst.h&gt;
 *
 * ...
 * typedef struct {
 *   gfloat freq;
 *   gfloat width;
 *   gfloat gain;
 * } GstEqualizerBandState;
 *
 * ...
 *
 *   GstElement *equalizer;
 *   GObject *band;
 *   gint i;
 *   GstEqualizerBandState state[] = {
 *     { 120.0,   50.0, - 3.0},
 *     { 500.0,   20.0,  12.0},
 *     {1503.0,    2.0, -20.0},
 *     {6000.0, 1000.0,   6.0},
 *     {3000.0,  120.0,   2.0}
 *   };
 *
 * ...
 *
 *   equalizer = gst_element_factory_make ("equalizer-nbands", "equalizer");
 *   g_object_set (G_OBJECT (equalizer), "num-bands", 5, NULL);
 *
 * ...
 *
 *   for (i = 0; i &lt; 5; i++) {
 *     band = gst_child_proxy_get_child_by_index (GST_CHILD_PROXY (equalizer), i);
 *     g_object_set (G_OBJECT (band), "freq", state[i].freq,
 *         "bandwidth", state[i].width,
 * 	"gain", state[i].gain);
 *     g_object_unref (G_OBJECT (band));
 *   }
 *
 * ...
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstiirequalizer.h"
#include "gstiirequalizernbands.h"


enum
{
  PROP_NUM_BANDS = 1
};

static void gst_iir_equalizer_nbands_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_iir_equalizer_nbands_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

GST_DEBUG_CATEGORY_EXTERN (equalizer_debug);
#define GST_CAT_DEFAULT equalizer_debug

#define gst_iir_equalizer_nbands_parent_class parent_class
G_DEFINE_TYPE (GstIirEqualizerNBands, gst_iir_equalizer_nbands,
    GST_TYPE_IIR_EQUALIZER);

/* equalizer implementation */

static void
gst_iir_equalizer_nbands_class_init (GstIirEqualizerNBandsClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_iir_equalizer_nbands_set_property;
  gobject_class->get_property = gst_iir_equalizer_nbands_get_property;

  g_object_class_install_property (gobject_class, PROP_NUM_BANDS,
      g_param_spec_uint ("num-bands", "num-bands",
          "number of different bands to use", 1, 64, 10,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));

  gst_element_class_set_static_metadata (gstelement_class, "N Band Equalizer",
      "Filter/Effect/Audio",
      "Direct Form IIR equalizer",
      "Benjamin Otte <otte@gnome.org>," " Stefan Kost <ensonic@users.sf.net>");
}

static void
gst_iir_equalizer_nbands_init (GstIirEqualizerNBands * equ_n)
{
  GstIirEqualizer *equ = GST_IIR_EQUALIZER (equ_n);

  gst_iir_equalizer_compute_frequencies (equ, 10);
}

static void
gst_iir_equalizer_nbands_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstIirEqualizer *equ = GST_IIR_EQUALIZER (object);

  switch (prop_id) {
    case PROP_NUM_BANDS:
      gst_iir_equalizer_compute_frequencies (equ, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_iir_equalizer_nbands_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstIirEqualizer *equ = GST_IIR_EQUALIZER (object);

  switch (prop_id) {
    case PROP_NUM_BANDS:
      g_value_set_uint (value, equ->freq_band_count);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
