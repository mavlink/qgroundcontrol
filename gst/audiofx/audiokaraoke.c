/* 
 * GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans@gmail.com>
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
 * SECTION:element-audiokaraoke
 * @title: audiokaraoke
 *
 * Remove the voice from audio by filtering the center channel.
 * This plugin is useful for karaoke applications.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=song.ogg ! oggdemux ! vorbisdec ! audiokaraoke ! audioconvert ! alsasink
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#include "audiokaraoke.h"

#define GST_CAT_DEFAULT gst_audio_karaoke_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

#define DEFAULT_LEVEL		1.0
#define DEFAULT_MONO_LEVEL	1.0
#define DEFAULT_FILTER_BAND	220.0
#define DEFAULT_FILTER_WIDTH	100.0

enum
{
  PROP_0,
  PROP_LEVEL,
  PROP_MONO_LEVEL,
  PROP_FILTER_BAND,
  PROP_FILTER_WIDTH
};

#define ALLOWED_CAPS \
    "audio/x-raw,"                                                \
    " format=(string){"GST_AUDIO_NE(S16)","GST_AUDIO_NE(F32)"},"  \
    " rate=(int)[1,MAX],"                                         \
    " channels=(int)2,"                                           \
    " channel-mask=(bitmask)0x3,"                                 \
    " layout=(string) interleaved"

G_DEFINE_TYPE (GstAudioKaraoke, gst_audio_karaoke, GST_TYPE_AUDIO_FILTER);

static void gst_audio_karaoke_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_audio_karaoke_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_audio_karaoke_setup (GstAudioFilter * filter,
    const GstAudioInfo * info);
static GstFlowReturn gst_audio_karaoke_transform_ip (GstBaseTransform * base,
    GstBuffer * buf);

static void gst_audio_karaoke_transform_int (GstAudioKaraoke * filter,
    gint16 * data, guint num_samples);
static void gst_audio_karaoke_transform_float (GstAudioKaraoke * filter,
    gfloat * data, guint num_samples);

/* GObject vmethod implementations */

static void
gst_audio_karaoke_class_init (GstAudioKaraokeClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstCaps *caps;

  GST_DEBUG_CATEGORY_INIT (gst_audio_karaoke_debug, "audiokaraoke", 0,
      "audiokaraoke element");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_audio_karaoke_set_property;
  gobject_class->get_property = gst_audio_karaoke_get_property;

  g_object_class_install_property (gobject_class, PROP_LEVEL,
      g_param_spec_float ("level", "Level",
          "Level of the effect (1.0 = full)", 0.0, 1.0, DEFAULT_LEVEL,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MONO_LEVEL,
      g_param_spec_float ("mono-level", "Mono Level",
          "Level of the mono channel (1.0 = full)", 0.0, 1.0, DEFAULT_LEVEL,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FILTER_BAND,
      g_param_spec_float ("filter-band", "Filter Band",
          "The Frequency band of the filter", 0.0, 441.0, DEFAULT_FILTER_BAND,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FILTER_WIDTH,
      g_param_spec_float ("filter-width", "Filter Width",
          "The Frequency width of the filter", 0.0, 100.0, DEFAULT_FILTER_WIDTH,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class, "AudioKaraoke",
      "Filter/Effect/Audio",
      "Removes voice from sound", "Wim Taymans <wim.taymans@gmail.com>");

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (GST_AUDIO_FILTER_CLASS (klass),
      caps);
  gst_caps_unref (caps);

  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
      GST_DEBUG_FUNCPTR (gst_audio_karaoke_transform_ip);
  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip_on_passthrough = FALSE;

  GST_AUDIO_FILTER_CLASS (klass)->setup =
      GST_DEBUG_FUNCPTR (gst_audio_karaoke_setup);
}

static void
gst_audio_karaoke_init (GstAudioKaraoke * filter)
{
  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (filter), TRUE);
  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (filter), TRUE);

  filter->level = DEFAULT_LEVEL;
  filter->mono_level = DEFAULT_MONO_LEVEL;
  filter->filter_band = DEFAULT_FILTER_BAND;
  filter->filter_width = DEFAULT_FILTER_WIDTH;
}

static void
update_filter (GstAudioKaraoke * filter, const GstAudioInfo * info)
{
  gfloat A, B, C;
  gint rate;

  if (info) {
    rate = GST_AUDIO_INFO_RATE (info);
  } else {
    rate = GST_AUDIO_FILTER_RATE (filter);
  }

  if (rate == 0)
    return;

  C = exp (-2 * G_PI * filter->filter_width / rate);
  B = -4 * C / (1 + C) * cos (2 * G_PI * filter->filter_band / rate);
  A = sqrt (1 - B * B / (4 * C)) * (1 - C);

  filter->A = A;
  filter->B = B;
  filter->C = C;
  filter->y1 = 0.0;
  filter->y2 = 0.0;
}

static void
gst_audio_karaoke_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioKaraoke *filter;

  filter = GST_AUDIO_KARAOKE (object);

  switch (prop_id) {
    case PROP_LEVEL:
      filter->level = g_value_get_float (value);
      break;
    case PROP_MONO_LEVEL:
      filter->mono_level = g_value_get_float (value);
      break;
    case PROP_FILTER_BAND:
      filter->filter_band = g_value_get_float (value);
      update_filter (filter, NULL);
      break;
    case PROP_FILTER_WIDTH:
      filter->filter_width = g_value_get_float (value);
      update_filter (filter, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_karaoke_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioKaraoke *filter;

  filter = GST_AUDIO_KARAOKE (object);

  switch (prop_id) {
    case PROP_LEVEL:
      g_value_set_float (value, filter->level);
      break;
    case PROP_MONO_LEVEL:
      g_value_set_float (value, filter->mono_level);
      break;
    case PROP_FILTER_BAND:
      g_value_set_float (value, filter->filter_band);
      break;
    case PROP_FILTER_WIDTH:
      g_value_set_float (value, filter->filter_width);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstAudioFilter vmethod implementations */

static gboolean
gst_audio_karaoke_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  GstAudioKaraoke *filter = GST_AUDIO_KARAOKE (base);
  gboolean ret = TRUE;

  switch (GST_AUDIO_INFO_FORMAT (info)) {
    case GST_AUDIO_FORMAT_S16:
      filter->process = (GstAudioKaraokeProcessFunc)
          gst_audio_karaoke_transform_int;
      break;
    case GST_AUDIO_FORMAT_F32:
      filter->process = (GstAudioKaraokeProcessFunc)
          gst_audio_karaoke_transform_float;
      break;
    default:
      ret = FALSE;
      break;
  }
  update_filter (filter, info);

  return ret;
}

static void
gst_audio_karaoke_transform_int (GstAudioKaraoke * filter,
    gint16 * data, guint num_samples)
{
  gint i, l, r, o, x;
  gint channels;
  gdouble y;
  gint level;

  channels = GST_AUDIO_FILTER_CHANNELS (filter);
  level = filter->level * 256;

  for (i = 0; i < num_samples; i += channels) {
    /* get left and right inputs */
    l = data[i];
    r = data[i + 1];
    /* do filtering */
    x = (l + r) / 2;
    y = (filter->A * x - filter->B * filter->y1) - filter->C * filter->y2;
    filter->y2 = filter->y1;
    filter->y1 = y;
    /* filter mono signal */
    o = (int) (y * filter->mono_level);
    o = CLAMP (o, G_MININT16, G_MAXINT16);
    o = (o * level) >> 8;
    /* now cut the center */
    x = l - ((r * level) >> 8) + o;
    r = r - ((l * level) >> 8) + o;
    data[i] = CLAMP (x, G_MININT16, G_MAXINT16);
    data[i + 1] = CLAMP (r, G_MININT16, G_MAXINT16);
  }
}

static void
gst_audio_karaoke_transform_float (GstAudioKaraoke * filter,
    gfloat * data, guint num_samples)
{
  gint i;
  gint channels;
  gdouble l, r, o;
  gdouble y;

  channels = GST_AUDIO_FILTER_CHANNELS (filter);

  for (i = 0; i < num_samples; i += channels) {
    /* get left and right inputs */
    l = data[i];
    r = data[i + 1];
    /* do filtering */
    y = (filter->A * ((l + r) / 2.0) - filter->B * filter->y1) -
        filter->C * filter->y2;
    filter->y2 = filter->y1;
    filter->y1 = y;
    /* filter mono signal */
    o = y * filter->mono_level * filter->level;
    /* now cut the center */
    data[i] = l - (r * filter->level) + o;
    data[i + 1] = r - (l * filter->level) + o;
  }
}

/* GstBaseTransform vmethod implementations */
static GstFlowReturn
gst_audio_karaoke_transform_ip (GstBaseTransform * base, GstBuffer * buf)
{
  GstAudioKaraoke *filter = GST_AUDIO_KARAOKE (base);
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
