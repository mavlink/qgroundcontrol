/* 
 * GStreamer
 * Copyright (C) 2007 Sebastian Dröge <slomo@circular-chaos.org>
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
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
 * SECTION:element-audioinvert
 * @title: audioinvert
 *
 * Swaps upper and lower half of audio samples. Mixing an inverted sample on top of
 * the original with a slight delay can produce effects that sound like resonance.
 * Creating a stereo sample from a mono source, with one channel inverted produces wide-stereo sounds.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc wave=saw ! audioinvert degree=0.4 ! alsasink
 * gst-launch-1.0 filesrc location="melo1.ogg" ! oggdemux ! vorbisdec ! audioconvert ! audioinvert degree=0.4 ! alsasink
 * gst-launch-1.0 audiotestsrc wave=saw ! audioconvert ! audioinvert degree=0.4 ! audioconvert ! alsasink
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#include "audioinvert.h"

#define GST_CAT_DEFAULT gst_audio_invert_debug
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
  PROP_DEGREE
};

#define ALLOWED_CAPS \
    "audio/x-raw,"                                                     \
    " format=(string) {"GST_AUDIO_NE(S16)","GST_AUDIO_NE(F32)"},"      \
    " rate=(int)[1,MAX],"                                              \
    " channels=(int)[1,MAX],"                                          \
    " layout=(string) {interleaved, non-interleaved}"

G_DEFINE_TYPE (GstAudioInvert, gst_audio_invert, GST_TYPE_AUDIO_FILTER);

static void gst_audio_invert_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_audio_invert_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_audio_invert_setup (GstAudioFilter * filter,
    const GstAudioInfo * info);
static GstFlowReturn gst_audio_invert_transform_ip (GstBaseTransform * base,
    GstBuffer * buf);

static void gst_audio_invert_transform_int (GstAudioInvert * filter,
    gint16 * data, guint num_samples);
static void gst_audio_invert_transform_float (GstAudioInvert * filter,
    gfloat * data, guint num_samples);

/* GObject vmethod implementations */

static void
gst_audio_invert_class_init (GstAudioInvertClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstCaps *caps;

  GST_DEBUG_CATEGORY_INIT (gst_audio_invert_debug, "audioinvert", 0,
      "audioinvert element");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_audio_invert_set_property;
  gobject_class->get_property = gst_audio_invert_get_property;

  g_object_class_install_property (gobject_class, PROP_DEGREE,
      g_param_spec_float ("degree", "Degree",
          "Degree of inversion", 0.0, 1.0,
          0.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class, "Audio inversion",
      "Filter/Effect/Audio",
      "Swaps upper and lower half of audio samples",
      "Sebastian Dröge <slomo@circular-chaos.org>");

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (GST_AUDIO_FILTER_CLASS (klass),
      caps);
  gst_caps_unref (caps);

  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
      GST_DEBUG_FUNCPTR (gst_audio_invert_transform_ip);
  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip_on_passthrough = FALSE;

  GST_AUDIO_FILTER_CLASS (klass)->setup =
      GST_DEBUG_FUNCPTR (gst_audio_invert_setup);
}

static void
gst_audio_invert_init (GstAudioInvert * filter)
{
  filter->degree = 0.0;
  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (filter), TRUE);
  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (filter), TRUE);
}

static void
gst_audio_invert_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioInvert *filter = GST_AUDIO_INVERT (object);

  switch (prop_id) {
    case PROP_DEGREE:
      filter->degree = g_value_get_float (value);
      gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (filter),
          filter->degree == 0.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_invert_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioInvert *filter = GST_AUDIO_INVERT (object);

  switch (prop_id) {
    case PROP_DEGREE:
      g_value_set_float (value, filter->degree);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstAudioFilter vmethod implementations */

static gboolean
gst_audio_invert_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  GstAudioInvert *filter = GST_AUDIO_INVERT (base);
  gboolean ret = TRUE;

  switch (GST_AUDIO_INFO_FORMAT (info)) {
    case GST_AUDIO_FORMAT_S16:
      filter->process = (GstAudioInvertProcessFunc)
          gst_audio_invert_transform_int;
      break;
    case GST_AUDIO_FORMAT_F32:
      filter->process = (GstAudioInvertProcessFunc)
          gst_audio_invert_transform_float;
      break;
    default:
      ret = FALSE;
      break;
  }
  return ret;
}

static void
gst_audio_invert_transform_int (GstAudioInvert * filter,
    gint16 * data, guint num_samples)
{
  gint i;
  gfloat dry = 1.0 - filter->degree;
  glong val;

  for (i = 0; i < num_samples; i++) {
    val = (*data) * dry + (-1 - (*data)) * filter->degree;
    *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
  }
}

static void
gst_audio_invert_transform_float (GstAudioInvert * filter,
    gfloat * data, guint num_samples)
{
  gint i;
  gfloat dry = 1.0 - filter->degree;
  glong val;

  for (i = 0; i < num_samples; i++) {
    val = (*data) * dry - (*data) * filter->degree;
    *data++ = val;
  }
}

/* GstBaseTransform vmethod implementations */
static GstFlowReturn
gst_audio_invert_transform_ip (GstBaseTransform * base, GstBuffer * buf)
{
  GstAudioInvert *filter = GST_AUDIO_INVERT (base);
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
