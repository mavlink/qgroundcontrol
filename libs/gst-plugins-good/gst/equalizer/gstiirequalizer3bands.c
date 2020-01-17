/* GStreamer
 * Copyright (C) <2007> Stefan Kost <ensonic@users.sf.net>
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
 * SECTION:element-equalizer-3bands
 * @title: equalizer-3bands
 *
 * The 3-band equalizer element allows to change the gain of a low frequency,
 * medium frequency and high frequency band.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=song.ogg ! oggdemux ! vorbisdec ! audioconvert ! equalizer-3bands band1=6.0 ! alsasink
 * ]| This raises the volume of the 2nd band, which is at 1110 Hz, by 6 db.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstiirequalizer.h"
#include "gstiirequalizer3bands.h"

enum
{
  PROP_BAND0 = 1,
  PROP_BAND1,
  PROP_BAND2,
};

static void gst_iir_equalizer_3bands_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_iir_equalizer_3bands_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

GST_DEBUG_CATEGORY_EXTERN (equalizer_debug);
#define GST_CAT_DEFAULT equalizer_debug

#define gst_iir_equalizer_3bands_parent_class parent_class
G_DEFINE_TYPE (GstIirEqualizer3Bands, gst_iir_equalizer_3bands,
    GST_TYPE_IIR_EQUALIZER);

/* equalizer implementation */

static void
gst_iir_equalizer_3bands_class_init (GstIirEqualizer3BandsClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_iir_equalizer_3bands_set_property;
  gobject_class->get_property = gst_iir_equalizer_3bands_get_property;

  g_object_class_install_property (gobject_class, PROP_BAND0,
      g_param_spec_double ("band0", "110 Hz",
          "gain for the frequency band 100 Hz, ranging from -24.0 to +12.0",
          -24.0, 12.0, 0.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_BAND1,
      g_param_spec_double ("band1", "1100 Hz",
          "gain for the frequency band 1100 Hz, ranging from -24.0 to +12.0",
          -24.0, 12.0, 0.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_BAND2,
      g_param_spec_double ("band2", "11 kHz",
          "gain for the frequency band 11 kHz, ranging from -24.0 to +12.0",
          -24.0, 12.0, 0.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  gst_element_class_set_static_metadata (gstelement_class, "3 Band Equalizer",
      "Filter/Effect/Audio",
      "Direct Form 3 band IIR equalizer", "Stefan Kost <ensonic@users.sf.net>");
}

static void
gst_iir_equalizer_3bands_init (GstIirEqualizer3Bands * equ_n)
{
  GstIirEqualizer *equ = GST_IIR_EQUALIZER (equ_n);

  gst_iir_equalizer_compute_frequencies (equ, 3);
}

static void
gst_iir_equalizer_3bands_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstChildProxy *equ = GST_CHILD_PROXY (object);

  switch (prop_id) {
    case PROP_BAND0:
      gst_child_proxy_set_property (equ, "band0::gain", value);
      break;
    case PROP_BAND1:
      gst_child_proxy_set_property (equ, "band1::gain", value);
      break;
    case PROP_BAND2:
      gst_child_proxy_set_property (equ, "band2::gain", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_iir_equalizer_3bands_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstChildProxy *equ = GST_CHILD_PROXY (object);

  switch (prop_id) {
    case PROP_BAND0:
      gst_child_proxy_get_property (equ, "band0::gain", value);
      break;
    case PROP_BAND1:
      gst_child_proxy_get_property (equ, "band1::gain", value);
      break;
    case PROP_BAND2:
      gst_child_proxy_get_property (equ, "band2::gain", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
