/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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

/* This effect is borrowed from xmms-0.6.1, though I mangled it so badly in
 * the process of copying it over that the xmms people probably won't want
 * any credit for it ;-)
 */
/**
 * SECTION:element-stereo
 * @title: stereo
 *
 * Create a wide stereo effect.
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v filesrc location=sine.ogg ! oggdemux ! vorbisdec ! audioconvert ! stereo ! audioconvert ! audioresample ! alsasink
 * ]| Play an Ogg/Vorbis file.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gststereo.h"

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#define ALLOWED_CAPS \
    "audio/x-raw,"                     \
    " format = "GST_AUDIO_NE (S16) "," \
    " rate = (int) [ 1, MAX ],"        \
    " channels = (int) 2"

/* Stereo signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ACTIVE,
  PROP_STEREO
};

static void gst_stereo_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_stereo_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_stereo_transform_ip (GstBaseTransform * base,
    GstBuffer * outbuf);

G_DEFINE_TYPE (GstStereo, gst_stereo, GST_TYPE_AUDIO_FILTER);

static void
gst_stereo_class_init (GstStereoClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstAudioFilterClass *audiofilter_class = GST_AUDIO_FILTER_CLASS (klass);
  GstCaps *caps;

  gst_element_class_set_static_metadata (element_class, "Stereo effect",
      "Filter/Effect/Audio",
      "Muck with the stereo signal to enhance its 'stereo-ness'",
      "Erik Walthinsen <omega@cse.ogi.edu>");

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (audiofilter_class, caps);
  gst_caps_unref (caps);

  gobject_class->set_property = gst_stereo_set_property;
  gobject_class->get_property = gst_stereo_get_property;

  g_object_class_install_property (gobject_class, PROP_ACTIVE,
      g_param_spec_boolean ("active", "active", "active",
          TRUE,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STEREO,
      g_param_spec_float ("stereo", "stereo", "stereo",
          0.0, 1.0, 0.1f,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_stereo_transform_ip);
}

static void
gst_stereo_init (GstStereo * stereo)
{
  stereo->active = TRUE;
  stereo->stereo = 0.1f;
}

static GstFlowReturn
gst_stereo_transform_ip (GstBaseTransform * base, GstBuffer * outbuf)
{
  GstStereo *stereo = GST_STEREO (base);
  gint samples;
  gint i;
  gdouble avg, ldiff, rdiff, tmp;
  gdouble mul = stereo->stereo;
  gint16 *data;
  GstMapInfo info;

  if (!gst_buffer_map (outbuf, &info, GST_MAP_READWRITE))
    return GST_FLOW_ERROR;

  data = (gint16 *) info.data;
  samples = info.size / 2;

  if (GST_CLOCK_TIME_IS_VALID (GST_BUFFER_TIMESTAMP (outbuf)))
    gst_object_sync_values (GST_OBJECT (stereo), GST_BUFFER_TIMESTAMP (outbuf));

  if (stereo->active) {
    for (i = 0; i < samples / 2; i += 2) {
      avg = (data[i] + data[i + 1]) / 2;
      ldiff = data[i] - avg;
      rdiff = data[i + 1] - avg;

      tmp = avg + ldiff * mul;
      if (tmp < -32768)
        tmp = -32768;
      if (tmp > 32767)
        tmp = 32767;
      data[i] = tmp;

      tmp = avg + rdiff * mul;
      if (tmp < -32768)
        tmp = -32768;
      if (tmp > 32767)
        tmp = 32767;
      data[i + 1] = tmp;
    }
  }

  gst_buffer_unmap (outbuf, &info);

  return GST_FLOW_OK;
}

static void
gst_stereo_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstStereo *stereo = GST_STEREO (object);

  switch (prop_id) {
    case PROP_ACTIVE:
      stereo->active = g_value_get_boolean (value);
      break;
    case PROP_STEREO:
      stereo->stereo = g_value_get_float (value) * 10.0;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_stereo_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstStereo *stereo = GST_STEREO (object);

  switch (prop_id) {
    case PROP_ACTIVE:
      g_value_set_boolean (value, stereo->active);
      break;
    case PROP_STEREO:
      g_value_set_float (value, stereo->stereo / 10.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
