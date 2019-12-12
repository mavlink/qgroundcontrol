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
 */

/**
 * SECTION:element-audioecho
 * @title: audioecho
 *
 * audioecho adds an echo or (simple) reverb effect to an audio stream. The echo
 * delay, intensity and the percentage of feedback can be configured.
 *
 * For getting an echo effect you have to set the delay to a larger value,
 * for example 200ms and more. Everything below will result in a simple
 * reverb effect, which results in a slightly metallic sound.
 *
 * Use the max-delay property to set the maximum amount of delay that
 * will be used. This can only be set before going to the PAUSED or PLAYING
 * state and will be set to the current delay by default.
 *
 * audioecho can also be used to apply a configurable delay to audio channels
 * by setting surround-delay=true. In that mode, it just delays "surround
 * channels" by the delay amount instead of performing an echo. The
 * channels that are configured surround channels for the delay are
 * selected using the surround-channels mask property.
 *
 * ## Example launch lines
 * |[
 * gst-launch-1.0 autoaudiosrc ! audioconvert ! audioecho delay=500000000 intensity=0.6 feedback=0.4 ! audioconvert ! autoaudiosink
 * gst-launch-1.0 filesrc location="melo1.ogg" ! decodebin ! audioconvert ! audioecho delay=50000000 intensity=0.6 feedback=0.4 ! audioconvert ! autoaudiosink
 * gst-launch-1.0 audiotestsrc ! audioconvert ! audio/x-raw,channels=4 ! audioecho surround-delay=true delay=500000000 ! audioconvert ! autoaudiosink
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

#include "audioecho.h"

#define GST_CAT_DEFAULT gst_audio_echo_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

/* Everything except the first 2 channels are considered surround */
#define DEFAULT_SURROUND_MASK ~((guint64)(0x3))

enum
{
  PROP_0,
  PROP_DELAY,
  PROP_MAX_DELAY,
  PROP_INTENSITY,
  PROP_FEEDBACK,
  PROP_SUR_DELAY,
  PROP_SUR_MASK
};

#define ALLOWED_CAPS \
    "audio/x-raw,"                                                 \
    " format=(string) {"GST_AUDIO_NE(F32)","GST_AUDIO_NE(F64)"}, " \
    " rate=(int)[1,MAX],"                                          \
    " channels=(int)[1,MAX],"                                      \
    " layout=(string) interleaved"

#define gst_audio_echo_parent_class parent_class
G_DEFINE_TYPE (GstAudioEcho, gst_audio_echo, GST_TYPE_AUDIO_FILTER);

static void gst_audio_echo_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_audio_echo_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_audio_echo_finalize (GObject * object);

static gboolean gst_audio_echo_setup (GstAudioFilter * self,
    const GstAudioInfo * info);
static gboolean gst_audio_echo_stop (GstBaseTransform * base);
static GstFlowReturn gst_audio_echo_transform_ip (GstBaseTransform * base,
    GstBuffer * buf);

static void gst_audio_echo_transform_float (GstAudioEcho * self,
    gfloat * data, guint num_samples);
static void gst_audio_echo_transform_double (GstAudioEcho * self,
    gdouble * data, guint num_samples);

/* GObject vmethod implementations */

static void
gst_audio_echo_class_init (GstAudioEchoClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *basetransform_class = (GstBaseTransformClass *) klass;
  GstAudioFilterClass *audioself_class = (GstAudioFilterClass *) klass;
  GstCaps *caps;

  GST_DEBUG_CATEGORY_INIT (gst_audio_echo_debug, "audioecho", 0,
      "audioecho element");

  gobject_class->set_property = gst_audio_echo_set_property;
  gobject_class->get_property = gst_audio_echo_get_property;
  gobject_class->finalize = gst_audio_echo_finalize;

  g_object_class_install_property (gobject_class, PROP_DELAY,
      g_param_spec_uint64 ("delay", "Delay",
          "Delay of the echo in nanoseconds", 1, G_MAXUINT64,
          1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
          | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_MAX_DELAY,
      g_param_spec_uint64 ("max-delay", "Maximum Delay",
          "Maximum delay of the echo in nanoseconds"
          " (can't be changed in PLAYING or PAUSED state)",
          1, G_MAXUINT64, 1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_READY));

  g_object_class_install_property (gobject_class, PROP_INTENSITY,
      g_param_spec_float ("intensity", "Intensity",
          "Intensity of the echo", 0.0, 1.0,
          0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
          | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_FEEDBACK,
      g_param_spec_float ("feedback", "Feedback",
          "Amount of feedback", 0.0, 1.0,
          0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
          | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_SUR_DELAY,
      g_param_spec_boolean ("surround-delay", "Enable Surround Delay",
          "Delay Surround Channels when TRUE instead of applying an echo effect",
          FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_SUR_MASK,
      g_param_spec_uint64 ("surround-mask", "Surround Mask",
          "A bitmask of channels that are considered surround and delayed when surround-delay = TRUE",
          1, G_MAXUINT64, DEFAULT_SURROUND_MASK,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_READY));

  gst_element_class_set_static_metadata (gstelement_class, "Audio echo",
      "Filter/Effect/Audio",
      "Adds an echo or reverb effect to an audio stream",
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (GST_AUDIO_FILTER_CLASS (klass),
      caps);
  gst_caps_unref (caps);

  audioself_class->setup = GST_DEBUG_FUNCPTR (gst_audio_echo_setup);
  basetransform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_audio_echo_transform_ip);
  basetransform_class->stop = GST_DEBUG_FUNCPTR (gst_audio_echo_stop);
}

static void
gst_audio_echo_init (GstAudioEcho * self)
{
  self->delay = 1;
  self->max_delay = 1;
  self->intensity = 0.0;
  self->feedback = 0.0;
  self->surdelay = FALSE;
  self->surround_mask = DEFAULT_SURROUND_MASK;

  g_mutex_init (&self->lock);

  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (self), TRUE);
}

static void
gst_audio_echo_finalize (GObject * object)
{
  GstAudioEcho *self = GST_AUDIO_ECHO (object);

  g_free (self->buffer);
  self->buffer = NULL;

  g_mutex_clear (&self->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_audio_echo_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioEcho *self = GST_AUDIO_ECHO (object);

  switch (prop_id) {
    case PROP_DELAY:{
      guint64 max_delay, delay;
      guint rate;

      g_mutex_lock (&self->lock);
      delay = g_value_get_uint64 (value);
      max_delay = self->max_delay;

      if (delay > max_delay && GST_STATE (self) > GST_STATE_READY) {
        GST_WARNING_OBJECT (self, "New delay (%" GST_TIME_FORMAT ") "
            "is larger than maximum delay (%" GST_TIME_FORMAT ")",
            GST_TIME_ARGS (delay), GST_TIME_ARGS (max_delay));
        self->delay = max_delay;
      } else {
        self->delay = delay;
        self->max_delay = MAX (delay, max_delay);
        if (delay > max_delay) {
          g_free (self->buffer);
          self->buffer = NULL;
        }
      }
      rate = GST_AUDIO_FILTER_RATE (self);
      if (rate > 0)
        self->delay_frames =
            MAX (gst_util_uint64_scale (self->delay, rate, GST_SECOND), 1);

      g_mutex_unlock (&self->lock);
      break;
    }
    case PROP_MAX_DELAY:{
      guint64 max_delay;

      g_mutex_lock (&self->lock);
      max_delay = g_value_get_uint64 (value);

      if (GST_STATE (self) > GST_STATE_READY) {
        GST_ERROR_OBJECT (self, "Can't change maximum delay in"
            " PLAYING or PAUSED state");
      } else {
        self->max_delay = max_delay;
        g_free (self->buffer);
        self->buffer = NULL;
      }
      g_mutex_unlock (&self->lock);
      break;
    }
    case PROP_INTENSITY:{
      g_mutex_lock (&self->lock);
      self->intensity = g_value_get_float (value);
      g_mutex_unlock (&self->lock);
      break;
    }
    case PROP_FEEDBACK:{
      g_mutex_lock (&self->lock);
      self->feedback = g_value_get_float (value);
      g_mutex_unlock (&self->lock);
      break;
    }
    case PROP_SUR_DELAY:{
      g_mutex_lock (&self->lock);
      self->surdelay = g_value_get_boolean (value);
      g_mutex_unlock (&self->lock);
      break;
    }
    case PROP_SUR_MASK:{
      g_mutex_lock (&self->lock);
      self->surround_mask = g_value_get_uint64 (value);
      g_mutex_unlock (&self->lock);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_echo_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioEcho *self = GST_AUDIO_ECHO (object);

  switch (prop_id) {
    case PROP_DELAY:
      g_mutex_lock (&self->lock);
      g_value_set_uint64 (value, self->delay);
      g_mutex_unlock (&self->lock);
      break;
    case PROP_MAX_DELAY:
      g_mutex_lock (&self->lock);
      g_value_set_uint64 (value, self->max_delay);
      g_mutex_unlock (&self->lock);
      break;
    case PROP_INTENSITY:
      g_mutex_lock (&self->lock);
      g_value_set_float (value, self->intensity);
      g_mutex_unlock (&self->lock);
      break;
    case PROP_FEEDBACK:
      g_mutex_lock (&self->lock);
      g_value_set_float (value, self->feedback);
      g_mutex_unlock (&self->lock);
      break;
    case PROP_SUR_DELAY:
      g_mutex_lock (&self->lock);
      g_value_set_boolean (value, self->surdelay);
      g_mutex_unlock (&self->lock);
      break;
    case PROP_SUR_MASK:{
      g_mutex_lock (&self->lock);
      g_value_set_uint64 (value, self->surround_mask);
      g_mutex_unlock (&self->lock);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstAudioFilter vmethod implementations */

static gboolean
gst_audio_echo_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  GstAudioEcho *self = GST_AUDIO_ECHO (base);
  gboolean ret = TRUE;

  switch (GST_AUDIO_INFO_FORMAT (info)) {
    case GST_AUDIO_FORMAT_F32:
      self->process = (GstAudioEchoProcessFunc)
          gst_audio_echo_transform_float;
      break;
    case GST_AUDIO_FORMAT_F64:
      self->process = (GstAudioEchoProcessFunc)
          gst_audio_echo_transform_double;
      break;
    default:
      ret = FALSE;
      break;
  }

  g_free (self->buffer);
  self->buffer = NULL;
  self->buffer_pos = 0;
  self->buffer_size = 0;
  self->buffer_size_frames = 0;

  return ret;
}

static gboolean
gst_audio_echo_stop (GstBaseTransform * base)
{
  GstAudioEcho *self = GST_AUDIO_ECHO (base);

  g_free (self->buffer);
  self->buffer = NULL;
  self->buffer_pos = 0;
  self->buffer_size = 0;
  self->buffer_size_frames = 0;

  return TRUE;
}

#define TRANSFORM_FUNC(name, type) \
static void \
gst_audio_echo_transform_##name (GstAudioEcho * self, \
    type * data, guint num_samples) \
{ \
  type *buffer = (type *) self->buffer; \
  guint channels = GST_AUDIO_FILTER_CHANNELS (self); \
  guint i, j; \
  guint echo_offset = self->buffer_size_frames - self->delay_frames; \
  gdouble intensity = self->intensity; \
  gdouble feedback = self->feedback; \
  guint buffer_pos = self->buffer_pos; \
  guint buffer_size_frames = self->buffer_size_frames; \
  \
  if (self->surdelay == FALSE) { \
    guint read_pos = ((echo_offset + buffer_pos) % buffer_size_frames) * channels; \
    guint write_pos = (buffer_pos % buffer_size_frames) * channels; \
    guint buffer_size = buffer_size_frames * channels; \
    for (i = 0; i < num_samples; i++) { \
      gdouble in = *data; \
      gdouble echo = buffer[read_pos]; \
      type out = in + intensity * echo; \
      \
      *data = out; \
      \
      buffer[write_pos] = in + feedback * echo; \
      read_pos = (read_pos + 1) % buffer_size; \
      write_pos = (write_pos + 1) % buffer_size; \
      data++; \
    } \
    buffer_pos = write_pos / channels; \
  } else { \
    guint64 surround_mask = self->surround_mask; \
    guint read_pos = ((echo_offset + buffer_pos) % buffer_size_frames) * channels; \
    guint write_pos = (buffer_pos % buffer_size_frames) * channels; \
    guint buffer_size = buffer_size_frames * channels; \
    \
    num_samples /= channels; \
    \
    for (i = 0; i < num_samples; i++) { \
      guint64 channel_mask = 1; \
      \
      for (j = 0; j < channels; j++) { \
        if (channel_mask & surround_mask) { \
          gdouble in = data[j]; \
          gdouble echo = buffer[read_pos + j]; \
          type out = echo; \
          \
          data[j] = out; \
          \
          buffer[write_pos + j] = in; \
        } else { \
          gdouble in = data[j]; \
          gdouble echo = buffer[read_pos + j]; \
          type out = in + intensity * echo; \
          \
          data[j] = out; \
          \
          buffer[write_pos + j] = in + feedback * echo; \
        } \
        channel_mask <<= 1; \
      } \
      read_pos = (read_pos + channels) % buffer_size; \
      write_pos = (write_pos + channels) % buffer_size; \
      data += channels; \
    } \
    buffer_pos = write_pos / channels; \
  } \
  self->buffer_pos = buffer_pos; \
}

TRANSFORM_FUNC (float, gfloat);
TRANSFORM_FUNC (double, gdouble);

/* GstBaseTransform vmethod implementations */
static GstFlowReturn
gst_audio_echo_transform_ip (GstBaseTransform * base, GstBuffer * buf)
{
  GstAudioEcho *self = GST_AUDIO_ECHO (base);
  guint num_samples;
  GstClockTime timestamp, stream_time;
  GstMapInfo map;

  g_mutex_lock (&self->lock);
  timestamp = GST_BUFFER_TIMESTAMP (buf);
  stream_time =
      gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (self, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (self), stream_time);

  if (self->buffer == NULL) {
    guint bpf, rate;

    bpf = GST_AUDIO_FILTER_BPF (self);
    rate = GST_AUDIO_FILTER_RATE (self);

    self->delay_frames =
        MAX (gst_util_uint64_scale (self->delay, rate, GST_SECOND), 1);
    self->buffer_size_frames =
        MAX (gst_util_uint64_scale (self->max_delay, rate, GST_SECOND), 1);

    self->buffer_size = self->buffer_size_frames * bpf;
    self->buffer = g_try_malloc0 (self->buffer_size);
    self->buffer_pos = 0;

    if (self->buffer == NULL) {
      g_mutex_unlock (&self->lock);
      GST_ERROR_OBJECT (self, "Failed to allocate %u bytes", self->buffer_size);
      return GST_FLOW_ERROR;
    }
  }

  gst_buffer_map (buf, &map, GST_MAP_READWRITE);
  num_samples = map.size / GST_AUDIO_FILTER_BPS (self);

  self->process (self, map.data, num_samples);

  gst_buffer_unmap (buf, &map);
  g_mutex_unlock (&self->lock);

  return GST_FLOW_OK;
}
