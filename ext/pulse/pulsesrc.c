/*
 *  GStreamer pulseaudio plugin
 *
 *  Copyright (c) 2004-2008 Lennart Poettering
 *
 *  gst-pulse is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  gst-pulse is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with gst-pulse; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 *  USA.
 */

/**
 * SECTION:element-pulsesrc
 * @title: pulsesrc
 * @see_also: pulsesink
 *
 * This element captures audio from a
 * [PulseAudio sound server](http://www.pulseaudio.org).
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v pulsesrc ! audioconvert ! vorbisenc ! oggmux ! filesink location=alsasrc.ogg
 * ]| Record from a sound card using pulseaudio and encode to Ogg/Vorbis.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>

#include <gst/base/gstbasesrc.h>
#include <gst/gsttaglist.h>
#include <gst/audio/audio.h>

#include "pulsesrc.h"
#include "pulseutil.h"

GST_DEBUG_CATEGORY_EXTERN (pulse_debug);
#define GST_CAT_DEFAULT pulse_debug

#define DEFAULT_SERVER            NULL
#define DEFAULT_DEVICE            NULL
#define DEFAULT_CURRENT_DEVICE    NULL
#define DEFAULT_DEVICE_NAME       NULL

#define DEFAULT_VOLUME          1.0
#define DEFAULT_MUTE            FALSE
#define MAX_VOLUME              10.0

/* See the pulsesink code for notes on how we interact with the PA mainloop
 * thread. */

enum
{
  PROP_0,
  PROP_SERVER,
  PROP_DEVICE,
  PROP_DEVICE_NAME,
  PROP_CURRENT_DEVICE,
  PROP_CLIENT_NAME,
  PROP_STREAM_PROPERTIES,
  PROP_SOURCE_OUTPUT_INDEX,
  PROP_VOLUME,
  PROP_MUTE,
  PROP_LAST
};

static void gst_pulsesrc_destroy_stream (GstPulseSrc * pulsesrc);
static void gst_pulsesrc_destroy_context (GstPulseSrc * pulsesrc);

static void gst_pulsesrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_pulsesrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_pulsesrc_finalize (GObject * object);

static gboolean gst_pulsesrc_set_corked (GstPulseSrc * psrc, gboolean corked,
    gboolean wait);
static gboolean gst_pulsesrc_open (GstAudioSrc * asrc);

static gboolean gst_pulsesrc_close (GstAudioSrc * asrc);

static gboolean gst_pulsesrc_prepare (GstAudioSrc * asrc,
    GstAudioRingBufferSpec * spec);

static gboolean gst_pulsesrc_unprepare (GstAudioSrc * asrc);

static guint gst_pulsesrc_read (GstAudioSrc * asrc, gpointer data,
    guint length, GstClockTime * timestamp);
static guint gst_pulsesrc_delay (GstAudioSrc * asrc);

static void gst_pulsesrc_reset (GstAudioSrc * src);

static gboolean gst_pulsesrc_negotiate (GstBaseSrc * basesrc);
static gboolean gst_pulsesrc_event (GstBaseSrc * basesrc, GstEvent * event);

static GstStateChangeReturn gst_pulsesrc_change_state (GstElement *
    element, GstStateChange transition);

static GstClockTime gst_pulsesrc_get_time (GstClock * clock, GstPulseSrc * src);

#define gst_pulsesrc_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstPulseSrc, gst_pulsesrc, GST_TYPE_AUDIO_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_STREAM_VOLUME, NULL));

static void
gst_pulsesrc_class_init (GstPulseSrcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstAudioSrcClass *gstaudiosrc_class = GST_AUDIO_SRC_CLASS (klass);
  GstBaseSrcClass *gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstCaps *caps;
  gchar *clientname;

  gobject_class->finalize = gst_pulsesrc_finalize;
  gobject_class->set_property = gst_pulsesrc_set_property;
  gobject_class->get_property = gst_pulsesrc_get_property;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_pulsesrc_change_state);

  gstbasesrc_class->event = GST_DEBUG_FUNCPTR (gst_pulsesrc_event);
  gstbasesrc_class->negotiate = GST_DEBUG_FUNCPTR (gst_pulsesrc_negotiate);

  gstaudiosrc_class->open = GST_DEBUG_FUNCPTR (gst_pulsesrc_open);
  gstaudiosrc_class->close = GST_DEBUG_FUNCPTR (gst_pulsesrc_close);
  gstaudiosrc_class->prepare = GST_DEBUG_FUNCPTR (gst_pulsesrc_prepare);
  gstaudiosrc_class->unprepare = GST_DEBUG_FUNCPTR (gst_pulsesrc_unprepare);
  gstaudiosrc_class->read = GST_DEBUG_FUNCPTR (gst_pulsesrc_read);
  gstaudiosrc_class->delay = GST_DEBUG_FUNCPTR (gst_pulsesrc_delay);
  gstaudiosrc_class->reset = GST_DEBUG_FUNCPTR (gst_pulsesrc_reset);

  /* Overwrite GObject fields */
  g_object_class_install_property (gobject_class,
      PROP_SERVER,
      g_param_spec_string ("server", "Server",
          "The PulseAudio server to connect to", DEFAULT_SERVER,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device",
          "The PulseAudio source device to connect to", DEFAULT_DEVICE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CURRENT_DEVICE,
      g_param_spec_string ("current-device", "Current Device",
          "The current PulseAudio source device", DEFAULT_CURRENT_DEVICE,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device name",
          "Human-readable name of the sound device", DEFAULT_DEVICE_NAME,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  clientname = gst_pulse_client_name ();
  /**
   * GstPulseSrc:client-name
   *
   * The PulseAudio client name to use.
   */
  g_object_class_install_property (gobject_class,
      PROP_CLIENT_NAME,
      g_param_spec_string ("client-name", "Client Name",
          "The PulseAudio client_name_to_use", clientname,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_READY));
  g_free (clientname);

  /**
   * GstPulseSrc:stream-properties:
   *
   * List of pulseaudio stream properties. A list of defined properties can be
   * found in the [pulseaudio api docs](http://0pointer.de/lennart/projects/pulseaudio/doxygen/proplist_8h.html).
   *
   * Below is an example for registering as a music application to pulseaudio.
   * |[
   * GstStructure *props;
   *
   * props = gst_structure_from_string ("props,media.role=music", NULL);
   * g_object_set (pulse, "stream-properties", props, NULL);
   * gst_structure_free (props);
   * ]|
   */
  g_object_class_install_property (gobject_class,
      PROP_STREAM_PROPERTIES,
      g_param_spec_boxed ("stream-properties", "stream properties",
          "list of pulseaudio stream properties",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstPulseSrc:source-output-index:
   *
   * The index of the PulseAudio source output corresponding to this element.
   */
  g_object_class_install_property (gobject_class,
      PROP_SOURCE_OUTPUT_INDEX,
      g_param_spec_uint ("source-output-index", "source output index",
          "The index of the PulseAudio source output corresponding to this "
          "record stream", 0, G_MAXUINT, PA_INVALID_INDEX,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "PulseAudio Audio Source",
      "Source/Audio",
      "Captures audio from a PulseAudio server", "Lennart Poettering");

  caps = gst_pulse_fix_pcm_caps (gst_caps_from_string (_PULSE_CAPS_PCM));
  gst_element_class_add_pad_template (gstelement_class,
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS, caps));
  gst_caps_unref (caps);

  /**
   * GstPulseSrc:volume:
   *
   * The volume of the record stream.
   */
  g_object_class_install_property (gobject_class,
      PROP_VOLUME, g_param_spec_double ("volume", "Volume",
          "Linear volume of this stream, 1.0=100%",
          0.0, MAX_VOLUME, DEFAULT_VOLUME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstPulseSrc:mute:
   *
   * Whether the stream is muted or not.
   */
  g_object_class_install_property (gobject_class,
      PROP_MUTE, g_param_spec_boolean ("mute", "Mute",
          "Mute state of this stream",
          DEFAULT_MUTE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_pulsesrc_init (GstPulseSrc * pulsesrc)
{
  pulsesrc->server = NULL;
  pulsesrc->device = NULL;
  pulsesrc->client_name = gst_pulse_client_name ();
  pulsesrc->device_description = NULL;

  pulsesrc->context = NULL;
  pulsesrc->stream = NULL;
  pulsesrc->stream_connected = FALSE;
  pulsesrc->source_output_idx = PA_INVALID_INDEX;

  pulsesrc->read_buffer = NULL;
  pulsesrc->read_buffer_length = 0;

  pa_sample_spec_init (&pulsesrc->sample_spec);

  pulsesrc->operation_success = FALSE;
  pulsesrc->paused = TRUE;
  pulsesrc->in_read = FALSE;

  pulsesrc->volume = DEFAULT_VOLUME;
  pulsesrc->volume_set = FALSE;

  pulsesrc->mute = DEFAULT_MUTE;
  pulsesrc->mute_set = FALSE;

  pulsesrc->notify = 0;

  pulsesrc->properties = NULL;
  pulsesrc->proplist = NULL;

  /* this should be the default but it isn't yet */
  gst_audio_base_src_set_slave_method (GST_AUDIO_BASE_SRC (pulsesrc),
      GST_AUDIO_BASE_SRC_SLAVE_SKEW);

  /* override with a custom clock */
  if (GST_AUDIO_BASE_SRC (pulsesrc)->clock)
    gst_object_unref (GST_AUDIO_BASE_SRC (pulsesrc)->clock);

  GST_AUDIO_BASE_SRC (pulsesrc)->clock =
      gst_audio_clock_new ("GstPulseSrcClock",
      (GstAudioClockGetTimeFunc) gst_pulsesrc_get_time, pulsesrc, NULL);
}

static void
gst_pulsesrc_destroy_stream (GstPulseSrc * pulsesrc)
{
  if (pulsesrc->stream) {
    pa_stream_disconnect (pulsesrc->stream);
    pa_stream_unref (pulsesrc->stream);
    pulsesrc->stream = NULL;
    pulsesrc->stream_connected = FALSE;
    pulsesrc->source_output_idx = PA_INVALID_INDEX;
    g_object_notify (G_OBJECT (pulsesrc), "source-output-index");
  }

  g_free (pulsesrc->device_description);
  pulsesrc->device_description = NULL;
}

static void
gst_pulsesrc_destroy_context (GstPulseSrc * pulsesrc)
{

  gst_pulsesrc_destroy_stream (pulsesrc);

  if (pulsesrc->context) {
    pa_context_disconnect (pulsesrc->context);

    /* Make sure we don't get any further callbacks */
    pa_context_set_state_callback (pulsesrc->context, NULL, NULL);
    pa_context_set_subscribe_callback (pulsesrc->context, NULL, NULL);

    pa_context_unref (pulsesrc->context);

    pulsesrc->context = NULL;
  }
}

static void
gst_pulsesrc_finalize (GObject * object)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (object);

  g_free (pulsesrc->server);
  g_free (pulsesrc->device);
  g_free (pulsesrc->client_name);
  g_free (pulsesrc->current_source_name);

  if (pulsesrc->properties)
    gst_structure_free (pulsesrc->properties);
  if (pulsesrc->proplist)
    pa_proplist_free (pulsesrc->proplist);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define CONTEXT_OK(c) ((c) && PA_CONTEXT_IS_GOOD (pa_context_get_state ((c))))
#define STREAM_OK(s) ((s) && PA_STREAM_IS_GOOD (pa_stream_get_state ((s))))

static gboolean
gst_pulsesrc_is_dead (GstPulseSrc * pulsesrc, gboolean check_stream)
{
  if (!pulsesrc->stream_connected)
    return TRUE;

  if (!CONTEXT_OK (pulsesrc->context))
    goto error;

  if (check_stream && !STREAM_OK (pulsesrc->stream))
    goto error;

  return FALSE;

error:
  {
    const gchar *err_str = pulsesrc->context ?
        pa_strerror (pa_context_errno (pulsesrc->context)) : NULL;
    GST_ELEMENT_ERROR ((pulsesrc), RESOURCE, FAILED, ("Disconnected: %s",
            err_str), (NULL));
    return TRUE;
  }
}

static void
gst_pulsesrc_source_info_cb (pa_context * c, const pa_source_info * i, int eol,
    void *userdata)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (userdata);

  if (!i)
    goto done;

  g_free (pulsesrc->device_description);
  pulsesrc->device_description = g_strdup (i->description);

done:
  pa_threaded_mainloop_signal (pulsesrc->mainloop, 0);
}

static gchar *
gst_pulsesrc_device_description (GstPulseSrc * pulsesrc)
{
  pa_operation *o = NULL;
  gchar *t;

  if (!pulsesrc->mainloop)
    goto no_mainloop;

  pa_threaded_mainloop_lock (pulsesrc->mainloop);

  if (!(o = pa_context_get_source_info_by_name (pulsesrc->context,
              pulsesrc->device, gst_pulsesrc_source_info_cb, pulsesrc))) {

    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_stream_get_source_info() failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock;
  }

  while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {

    if (gst_pulsesrc_is_dead (pulsesrc, FALSE))
      goto unlock;

    pa_threaded_mainloop_wait (pulsesrc->mainloop);
  }

unlock:

  if (o)
    pa_operation_unref (o);

  t = g_strdup (pulsesrc->device_description);

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return t;

no_mainloop:
  {
    GST_DEBUG_OBJECT (pulsesrc, "have no mainloop");
    return NULL;
  }
}

static void
gst_pulsesrc_source_output_info_cb (pa_context * c,
    const pa_source_output_info * i, int eol, void *userdata)
{
  GstPulseSrc *psrc;

  psrc = GST_PULSESRC_CAST (userdata);

  if (!i)
    goto done;

  /* If the index doesn't match our current stream,
   * it implies we just recreated the stream (caps change)
   */
  if (i->index == psrc->source_output_idx) {
    psrc->volume = pa_sw_volume_to_linear (pa_cvolume_max (&i->volume));
    psrc->mute = i->mute;
    psrc->current_source_idx = i->source;

    if (G_UNLIKELY (psrc->volume > MAX_VOLUME)) {
      GST_WARNING_OBJECT (psrc, "Clipped volume from %f to %f",
          psrc->volume, MAX_VOLUME);
      psrc->volume = MAX_VOLUME;
    }
  }

done:
  pa_threaded_mainloop_signal (psrc->mainloop, 0);
}

static void
gst_pulsesrc_get_source_output_info (GstPulseSrc * pulsesrc, gdouble * volume,
    gboolean * mute)
{
  pa_operation *o = NULL;

  if (!pulsesrc->mainloop)
    goto no_mainloop;

  if (pulsesrc->source_output_idx == PA_INVALID_INDEX)
    goto no_index;

  pa_threaded_mainloop_lock (pulsesrc->mainloop);

  if (!(o = pa_context_get_source_output_info (pulsesrc->context,
              pulsesrc->source_output_idx, gst_pulsesrc_source_output_info_cb,
              pulsesrc)))
    goto info_failed;

  while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
    pa_threaded_mainloop_wait (pulsesrc->mainloop);
    if (gst_pulsesrc_is_dead (pulsesrc, TRUE))
      goto unlock;
  }

unlock:

  if (volume)
    *volume = pulsesrc->volume;
  if (mute)
    *mute = pulsesrc->mute;

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return;

  /* ERRORS */
no_mainloop:
  {
    GST_DEBUG_OBJECT (pulsesrc, "we have no mainloop");
    if (volume)
      *volume = pulsesrc->volume;
    if (mute)
      *mute = pulsesrc->mute;
    return;
  }
no_index:
  {
    GST_DEBUG_OBJECT (pulsesrc, "we don't have a stream index");
    if (volume)
      *volume = pulsesrc->volume;
    if (mute)
      *mute = pulsesrc->mute;
    return;
  }
info_failed:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_context_get_source_output_info() failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock;
  }
}

static void
gst_pulsesrc_current_source_info_cb (pa_context * c, const pa_source_info * i,
    int eol, void *userdata)
{
  GstPulseSrc *psrc;

  psrc = GST_PULSESRC_CAST (userdata);

  if (!i)
    goto done;

  /* If the index doesn't match our current stream,
   * it implies we just recreated the stream (caps change)
   */
  if (i->index == psrc->current_source_idx) {
    g_free (psrc->current_source_name);
    psrc->current_source_name = g_strdup (i->name);
  }

done:
  pa_threaded_mainloop_signal (psrc->mainloop, 0);
}

static gchar *
gst_pulsesrc_get_current_device (GstPulseSrc * pulsesrc)
{
  pa_operation *o = NULL;
  gchar *current_src;

  if (!pulsesrc->mainloop)
    goto no_mainloop;

  if (pulsesrc->source_output_idx == PA_INVALID_INDEX)
    goto no_index;

  gst_pulsesrc_get_source_output_info (pulsesrc, NULL, NULL);

  pa_threaded_mainloop_lock (pulsesrc->mainloop);


  if (!(o = pa_context_get_source_info_by_index (pulsesrc->context,
              pulsesrc->current_source_idx, gst_pulsesrc_current_source_info_cb,
              pulsesrc)))
    goto info_failed;

  while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
    pa_threaded_mainloop_wait (pulsesrc->mainloop);
    if (gst_pulsesrc_is_dead (pulsesrc, TRUE))
      goto unlock;
  }

unlock:

  current_src = g_strdup (pulsesrc->current_source_name);

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return current_src;

  /* ERRORS */
no_mainloop:
  {
    GST_DEBUG_OBJECT (pulsesrc, "we have no mainloop");
    return NULL;
  }
no_index:
  {
    GST_DEBUG_OBJECT (pulsesrc, "we don't have a stream index");
    return NULL;
  }
info_failed:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_context_get_source_output_info() failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock;
  }
}

static void
gst_pulsesrc_set_stream_volume (GstPulseSrc * pulsesrc, gdouble volume)
{
  pa_cvolume v;
  pa_operation *o = NULL;

  if (!pulsesrc->mainloop)
    goto no_mainloop;

  if (pulsesrc->source_output_idx == PA_INVALID_INDEX)
    goto no_index;

  pa_threaded_mainloop_lock (pulsesrc->mainloop);

  GST_DEBUG_OBJECT (pulsesrc, "setting volume to %f", volume);

  gst_pulse_cvolume_from_linear (&v, pulsesrc->sample_spec.channels, volume);

  if (!(o = pa_context_set_source_output_volume (pulsesrc->context,
              pulsesrc->source_output_idx, &v, NULL, NULL)))
    goto volume_failed;

  /* We don't really care about the result of this call */
unlock:

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return;

  /* ERRORS */
no_mainloop:
  {
    pulsesrc->volume = volume;
    pulsesrc->volume_set = TRUE;
    GST_DEBUG_OBJECT (pulsesrc, "we have no mainloop");
    return;
  }
no_index:
  {
    pulsesrc->volume = volume;
    pulsesrc->volume_set = TRUE;
    GST_DEBUG_OBJECT (pulsesrc, "we don't have a stream index");
    return;
  }
volume_failed:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_stream_set_source_output_volume() failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock;
  }
}

static void
gst_pulsesrc_set_stream_mute (GstPulseSrc * pulsesrc, gboolean mute)
{
  pa_operation *o = NULL;

  if (!pulsesrc->mainloop)
    goto no_mainloop;

  if (pulsesrc->source_output_idx == PA_INVALID_INDEX)
    goto no_index;

  pa_threaded_mainloop_lock (pulsesrc->mainloop);

  GST_DEBUG_OBJECT (pulsesrc, "setting mute state to %d", mute);

  if (!(o = pa_context_set_source_output_mute (pulsesrc->context,
              pulsesrc->source_output_idx, mute, NULL, NULL)))
    goto mute_failed;

  /* We don't really care about the result of this call */
unlock:

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return;

  /* ERRORS */
no_mainloop:
  {
    pulsesrc->mute = mute;
    pulsesrc->mute_set = TRUE;
    GST_DEBUG_OBJECT (pulsesrc, "we have no mainloop");
    return;
  }
no_index:
  {
    pulsesrc->mute = mute;
    pulsesrc->mute_set = TRUE;
    GST_DEBUG_OBJECT (pulsesrc, "we don't have a stream index");
    return;
  }
mute_failed:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_stream_set_source_output_mute() failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock;
  }
}

static void
gst_pulsesrc_set_stream_device (GstPulseSrc * pulsesrc, const gchar * device)
{
  pa_operation *o = NULL;

  if (!pulsesrc->mainloop)
    goto no_mainloop;

  if (pulsesrc->source_output_idx == PA_INVALID_INDEX)
    goto no_index;

  pa_threaded_mainloop_lock (pulsesrc->mainloop);

  GST_DEBUG_OBJECT (pulsesrc, "setting stream device to %s", device);

  if (!(o = pa_context_move_source_output_by_name (pulsesrc->context,
              pulsesrc->source_output_idx, device, NULL, NULL)))
    goto move_failed;

unlock:

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return;

  /* ERRORS */
no_mainloop:
  {
    GST_DEBUG_OBJECT (pulsesrc, "we have no mainloop");
    return;
  }
no_index:
  {
    GST_DEBUG_OBJECT (pulsesrc, "we don't have a stream index");
    return;
  }
move_failed:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_context_move_source_output_by_name(%s) failed: %s",
            device, pa_strerror (pa_context_errno (pulsesrc->context))),
        (NULL));
    goto unlock;
  }
}

static void
gst_pulsesrc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{

  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (object);

  switch (prop_id) {
    case PROP_SERVER:
      g_free (pulsesrc->server);
      pulsesrc->server = g_value_dup_string (value);
      break;
    case PROP_DEVICE:
      g_free (pulsesrc->device);
      pulsesrc->device = g_value_dup_string (value);
      gst_pulsesrc_set_stream_device (pulsesrc, pulsesrc->device);
      break;
    case PROP_CLIENT_NAME:
      g_free (pulsesrc->client_name);
      if (!g_value_get_string (value)) {
        GST_WARNING_OBJECT (pulsesrc,
            "Empty PulseAudio client name not allowed. Resetting to default value");
        pulsesrc->client_name = gst_pulse_client_name ();
      } else
        pulsesrc->client_name = g_value_dup_string (value);
      break;
    case PROP_STREAM_PROPERTIES:
      if (pulsesrc->properties)
        gst_structure_free (pulsesrc->properties);
      pulsesrc->properties =
          gst_structure_copy (gst_value_get_structure (value));
      if (pulsesrc->proplist)
        pa_proplist_free (pulsesrc->proplist);
      pulsesrc->proplist = gst_pulse_make_proplist (pulsesrc->properties);
      break;
    case PROP_VOLUME:
      gst_pulsesrc_set_stream_volume (pulsesrc, g_value_get_double (value));
      break;
    case PROP_MUTE:
      gst_pulsesrc_set_stream_mute (pulsesrc, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_pulsesrc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{

  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (object);

  switch (prop_id) {
    case PROP_SERVER:
      g_value_set_string (value, pulsesrc->server);
      break;
    case PROP_DEVICE:
      g_value_set_string (value, pulsesrc->device);
      break;
    case PROP_CURRENT_DEVICE:
    {
      gchar *current_device = gst_pulsesrc_get_current_device (pulsesrc);
      if (current_device)
        g_value_take_string (value, current_device);
      else
        g_value_set_string (value, "");
      break;
    }
    case PROP_DEVICE_NAME:
      g_value_take_string (value, gst_pulsesrc_device_description (pulsesrc));
      break;
    case PROP_CLIENT_NAME:
      g_value_set_string (value, pulsesrc->client_name);
      break;
    case PROP_STREAM_PROPERTIES:
      gst_value_set_structure (value, pulsesrc->properties);
      break;
    case PROP_SOURCE_OUTPUT_INDEX:
      g_value_set_uint (value, pulsesrc->source_output_idx);
      break;
    case PROP_VOLUME:
    {
      gdouble volume;
      gst_pulsesrc_get_source_output_info (pulsesrc, &volume, NULL);
      g_value_set_double (value, volume);
      break;
    }
    case PROP_MUTE:
    {
      gboolean mute;
      gst_pulsesrc_get_source_output_info (pulsesrc, NULL, &mute);
      g_value_set_boolean (value, mute);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_pulsesrc_context_state_cb (pa_context * c, void *userdata)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (userdata);

  switch (pa_context_get_state (c)) {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_FAILED:
      pa_threaded_mainloop_signal (pulsesrc->mainloop, 0);
      break;

    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;
  }
}

static void
gst_pulsesrc_stream_state_cb (pa_stream * s, void *userdata)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (userdata);

  switch (pa_stream_get_state (s)) {

    case PA_STREAM_READY:
    case PA_STREAM_FAILED:
    case PA_STREAM_TERMINATED:
      pa_threaded_mainloop_signal (pulsesrc->mainloop, 0);
      break;

    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:
      break;
  }
}

static void
gst_pulsesrc_stream_request_cb (pa_stream * s, size_t length, void *userdata)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (userdata);

  GST_LOG_OBJECT (pulsesrc, "got request for length %" G_GSIZE_FORMAT, length);

  if (pulsesrc->in_read) {
    /* only signal when reading */
    pa_threaded_mainloop_signal (pulsesrc->mainloop, 0);
  }
}

static void
gst_pulsesrc_stream_latency_update_cb (pa_stream * s, void *userdata)
{
  const pa_timing_info *info;
  pa_usec_t source_usec;

  info = pa_stream_get_timing_info (s);

  if (!info) {
    GST_LOG_OBJECT (GST_PULSESRC_CAST (userdata),
        "latency update (information unknown)");
    return;
  }
  source_usec = info->configured_source_usec;

  GST_LOG_OBJECT (GST_PULSESRC_CAST (userdata),
      "latency_update, %" G_GUINT64_FORMAT ", %d:%" G_GINT64_FORMAT ", %d:%"
      G_GUINT64_FORMAT ", %" G_GUINT64_FORMAT ", %" G_GUINT64_FORMAT,
      GST_TIMEVAL_TO_TIME (info->timestamp), info->write_index_corrupt,
      info->write_index, info->read_index_corrupt, info->read_index,
      info->source_usec, source_usec);
}

static void
gst_pulsesrc_stream_underflow_cb (pa_stream * s, void *userdata)
{
  GST_WARNING_OBJECT (GST_PULSESRC_CAST (userdata), "Got underflow");
}

static void
gst_pulsesrc_stream_overflow_cb (pa_stream * s, void *userdata)
{
  GST_WARNING_OBJECT (GST_PULSESRC_CAST (userdata), "Got overflow");
}

static void
gst_pulsesrc_context_subscribe_cb (pa_context * c,
    pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
  GstPulseSrc *psrc = GST_PULSESRC (userdata);

  if (t != (PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT | PA_SUBSCRIPTION_EVENT_CHANGE)
      && t != (PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT | PA_SUBSCRIPTION_EVENT_NEW))
    return;

  if (idx != psrc->source_output_idx)
    return;

  /* Actually this event is also triggered when other properties of the stream
   * change that are unrelated to the volume. However it is probably cheaper to
   * signal the change here and check for the volume when the GObject property
   * is read instead of querying it always. */

  /* inform streaming thread to notify */
  g_atomic_int_compare_and_exchange (&psrc->notify, 0, 1);
}

static gboolean
gst_pulsesrc_open (GstAudioSrc * asrc)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (asrc);

  pa_threaded_mainloop_lock (pulsesrc->mainloop);

  g_assert (!pulsesrc->context);
  g_assert (!pulsesrc->stream);

  GST_DEBUG_OBJECT (pulsesrc, "opening device");

  if (!(pulsesrc->context =
          pa_context_new (pa_threaded_mainloop_get_api (pulsesrc->mainloop),
              pulsesrc->client_name))) {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED, ("Failed to create context"),
        (NULL));
    goto unlock_and_fail;
  }

  pa_context_set_state_callback (pulsesrc->context,
      gst_pulsesrc_context_state_cb, pulsesrc);
  pa_context_set_subscribe_callback (pulsesrc->context,
      gst_pulsesrc_context_subscribe_cb, pulsesrc);

  GST_DEBUG_OBJECT (pulsesrc, "connect to server %s",
      GST_STR_NULL (pulsesrc->server));

  if (pa_context_connect (pulsesrc->context, pulsesrc->server, 0, NULL) < 0) {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED, ("Failed to connect: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock_and_fail;
  }

  for (;;) {
    pa_context_state_t state;

    state = pa_context_get_state (pulsesrc->context);

    if (!PA_CONTEXT_IS_GOOD (state)) {
      GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED, ("Failed to connect: %s",
              pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
      goto unlock_and_fail;
    }

    if (state == PA_CONTEXT_READY)
      break;

    /* Wait until the context is ready */
    pa_threaded_mainloop_wait (pulsesrc->mainloop);
  }
  GST_DEBUG_OBJECT (pulsesrc, "connected");

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return TRUE;

  /* ERRORS */
unlock_and_fail:
  {
    gst_pulsesrc_destroy_context (pulsesrc);

    pa_threaded_mainloop_unlock (pulsesrc->mainloop);

    return FALSE;
  }
}

static gboolean
gst_pulsesrc_close (GstAudioSrc * asrc)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (asrc);

  pa_threaded_mainloop_lock (pulsesrc->mainloop);
  gst_pulsesrc_destroy_context (pulsesrc);
  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return TRUE;
}

static gboolean
gst_pulsesrc_unprepare (GstAudioSrc * asrc)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (asrc);

  pa_threaded_mainloop_lock (pulsesrc->mainloop);
  gst_pulsesrc_destroy_stream (pulsesrc);

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  pulsesrc->read_buffer = NULL;
  pulsesrc->read_buffer_length = 0;

  return TRUE;
}

static guint
gst_pulsesrc_read (GstAudioSrc * asrc, gpointer data, guint length,
    GstClockTime * timestamp)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (asrc);
  size_t sum = 0;

  if (g_atomic_int_compare_and_exchange (&pulsesrc->notify, 1, 0)) {
    g_object_notify (G_OBJECT (pulsesrc), "volume");
    g_object_notify (G_OBJECT (pulsesrc), "mute");
    g_object_notify (G_OBJECT (pulsesrc), "current-device");
  }

  pa_threaded_mainloop_lock (pulsesrc->mainloop);
  pulsesrc->in_read = TRUE;

  if (!pulsesrc->stream_connected)
    goto not_connected;

  if (pulsesrc->paused)
    goto was_paused;

  while (length > 0) {
    size_t l;

    GST_LOG_OBJECT (pulsesrc, "reading %u bytes", length);

    /*check if we have a leftover buffer */
    if (!pulsesrc->read_buffer) {
      for (;;) {
        if (gst_pulsesrc_is_dead (pulsesrc, TRUE))
          goto unlock_and_fail;

        /* read all available data, we keep a pointer to the data and the length
         * and take from it what we need. */
        if (pa_stream_peek (pulsesrc->stream, &pulsesrc->read_buffer,
                &pulsesrc->read_buffer_length) < 0)
          goto peek_failed;

        GST_LOG_OBJECT (pulsesrc, "have data of %" G_GSIZE_FORMAT " bytes",
            pulsesrc->read_buffer_length);

        /* if we have data, process if */
        if (pulsesrc->read_buffer && pulsesrc->read_buffer_length)
          break;

        /* now wait for more data to become available */
        GST_LOG_OBJECT (pulsesrc, "waiting for data");
        pa_threaded_mainloop_wait (pulsesrc->mainloop);

        if (pulsesrc->paused)
          goto was_paused;
      }
    }

    l = pulsesrc->read_buffer_length >
        length ? length : pulsesrc->read_buffer_length;

    memcpy (data, pulsesrc->read_buffer, l);

    pulsesrc->read_buffer = (const guint8 *) pulsesrc->read_buffer + l;
    pulsesrc->read_buffer_length -= l;

    data = (guint8 *) data + l;
    length -= l;
    sum += l;

    if (pulsesrc->read_buffer_length <= 0) {
      /* we copied all of the data, drop it now */
      if (pa_stream_drop (pulsesrc->stream) < 0)
        goto drop_failed;

      /* reset pointer to data */
      pulsesrc->read_buffer = NULL;
      pulsesrc->read_buffer_length = 0;
    }
  }

  pulsesrc->in_read = FALSE;
  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return sum;

  /* ERRORS */
not_connected:
  {
    GST_LOG_OBJECT (pulsesrc, "we are not connected");
    goto unlock_and_fail;
  }
was_paused:
  {
    GST_LOG_OBJECT (pulsesrc, "we are paused");
    goto unlock_and_fail;
  }
peek_failed:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_stream_peek() failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock_and_fail;
  }
drop_failed:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_stream_drop() failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock_and_fail;
  }
unlock_and_fail:
  {
    pulsesrc->in_read = FALSE;
    pa_threaded_mainloop_unlock (pulsesrc->mainloop);

    return (guint) - 1;
  }
}

/* return the delay in samples */
static guint
gst_pulsesrc_delay (GstAudioSrc * asrc)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (asrc);
  pa_usec_t t;
  int negative, res;
  guint result;

  pa_threaded_mainloop_lock (pulsesrc->mainloop);
  if (gst_pulsesrc_is_dead (pulsesrc, TRUE))
    goto server_dead;

  /* get the latency, this can fail when we don't have a latency update yet.
   * We don't want to wait for latency updates here but we just return 0. */
  res = pa_stream_get_latency (pulsesrc->stream, &t, &negative);

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  if (res < 0) {
    GST_DEBUG_OBJECT (pulsesrc, "could not get latency");
    result = 0;
  } else {
    if (negative)
      result = 0;
    else
      result = (guint) ((t * pulsesrc->sample_spec.rate) / 1000000LL);
  }
  return result;

  /* ERRORS */
server_dead:
  {
    GST_DEBUG_OBJECT (pulsesrc, "the server is dead");
    pa_threaded_mainloop_unlock (pulsesrc->mainloop);
    return 0;
  }
}

static gboolean
gst_pulsesrc_create_stream (GstPulseSrc * pulsesrc, GstCaps ** caps,
    GstAudioRingBufferSpec * rspec)
{
  pa_channel_map channel_map;
  const pa_channel_map *m;
  GstStructure *s;
  gboolean need_channel_layout = FALSE;
  GstAudioRingBufferSpec new_spec, *spec = NULL;
  const gchar *name;
  int i;

  /* If we already have a stream (renegotiation), free it first */
  if (pulsesrc->stream)
    gst_pulsesrc_destroy_stream (pulsesrc);

  if (rspec) {
    /* Post-negotiation, we already have a ringbuffer spec, so we just need to
     * use it to create a stream. */
    spec = rspec;

    /* At this point, we expect the channel-mask to be set in caps, so we just
     * use that */
    if (!gst_pulse_gst_to_channel_map (&channel_map, spec))
      goto invalid_spec;

  } else if (caps) {
    /* At negotiation time, we get a fixed caps and use it to set up a stream */
    s = gst_caps_get_structure (*caps, 0);
    gst_structure_get_int (s, "channels", &new_spec.info.channels);
    if (!gst_structure_has_field (s, "channel-mask")) {
      if (new_spec.info.channels == 1) {
        pa_channel_map_init_mono (&channel_map);
      } else if (new_spec.info.channels == 2) {
        pa_channel_map_init_stereo (&channel_map);
      } else {
        need_channel_layout = TRUE;
        gst_structure_set (s, "channel-mask", GST_TYPE_BITMASK,
            G_GUINT64_CONSTANT (0), NULL);
      }
    }

    memset (&new_spec, 0, sizeof (GstAudioRingBufferSpec));
    new_spec.latency_time = GST_SECOND;
    if (!gst_audio_ring_buffer_parse_caps (&new_spec, *caps))
      goto invalid_caps;

    /* Keep the refcount of the caps at 1 to make them writable */
    gst_caps_unref (new_spec.caps);

    if (!need_channel_layout
        && !gst_pulse_gst_to_channel_map (&channel_map, &new_spec)) {
      need_channel_layout = TRUE;
      gst_structure_set (s, "channel-mask", GST_TYPE_BITMASK,
          G_GUINT64_CONSTANT (0), NULL);
      for (i = 0; i < G_N_ELEMENTS (new_spec.info.position); i++)
        new_spec.info.position[i] = GST_AUDIO_CHANNEL_POSITION_INVALID;
    }

    spec = &new_spec;
  } else {
    /* !rspec && !caps */
    g_assert_not_reached ();
  }

  if (!gst_pulse_fill_sample_spec (spec, &pulsesrc->sample_spec))
    goto invalid_spec;

  pa_threaded_mainloop_lock (pulsesrc->mainloop);

  if (!pulsesrc->context)
    goto bad_context;

  name = "Record Stream";
  if (pulsesrc->proplist) {
    if (!(pulsesrc->stream = pa_stream_new_with_proplist (pulsesrc->context,
                name, &pulsesrc->sample_spec,
                (need_channel_layout) ? NULL : &channel_map,
                pulsesrc->proplist)))
      goto create_failed;

  } else if (!(pulsesrc->stream = pa_stream_new (pulsesrc->context,
              name, &pulsesrc->sample_spec,
              (need_channel_layout) ? NULL : &channel_map)))
    goto create_failed;

  if (caps) {
    m = pa_stream_get_channel_map (pulsesrc->stream);
    gst_pulse_channel_map_to_gst (m, &new_spec);
    gst_audio_channel_positions_to_valid_order (new_spec.info.position,
        new_spec.info.channels);
    gst_caps_unref (*caps);
    *caps = gst_audio_info_to_caps (&new_spec.info);

    GST_DEBUG_OBJECT (pulsesrc, "Caps are %" GST_PTR_FORMAT, *caps);
  }


  pa_stream_set_state_callback (pulsesrc->stream, gst_pulsesrc_stream_state_cb,
      pulsesrc);
  pa_stream_set_read_callback (pulsesrc->stream, gst_pulsesrc_stream_request_cb,
      pulsesrc);
  pa_stream_set_underflow_callback (pulsesrc->stream,
      gst_pulsesrc_stream_underflow_cb, pulsesrc);
  pa_stream_set_overflow_callback (pulsesrc->stream,
      gst_pulsesrc_stream_overflow_cb, pulsesrc);
  pa_stream_set_latency_update_callback (pulsesrc->stream,
      gst_pulsesrc_stream_latency_update_cb, pulsesrc);

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return TRUE;

  /* ERRORS */
invalid_caps:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, SETTINGS,
        ("Can't parse caps."), (NULL));
    goto fail;
  }
invalid_spec:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, SETTINGS,
        ("Invalid sample specification."), (NULL));
    goto fail;
  }
bad_context:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED, ("Bad context"), (NULL));
    goto unlock_and_fail;
  }
create_failed:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("Failed to create stream: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock_and_fail;
  }
unlock_and_fail:
  {
    gst_pulsesrc_destroy_stream (pulsesrc);

    pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  fail:
    return FALSE;
  }
}

static gboolean
gst_pulsesrc_event (GstBaseSrc * basesrc, GstEvent * event)
{
  GST_DEBUG_OBJECT (basesrc, "handle event %" GST_PTR_FORMAT, event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_RECONFIGURE:
      gst_pad_check_reconfigure (GST_BASE_SRC_PAD (basesrc));
      break;
    default:
      break;
  }
  return GST_BASE_SRC_CLASS (parent_class)->event (basesrc, event);
}

/* This is essentially gst_base_src_negotiate_default() but the caps
 * are guaranteed to have a channel layout for > 2 channels
 */
static gboolean
gst_pulsesrc_negotiate (GstBaseSrc * basesrc)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (basesrc);
  GstCaps *thiscaps;
  GstCaps *caps = NULL;
  GstCaps *peercaps = NULL;
  gboolean result = FALSE;

  /* first see what is possible on our source pad */
  thiscaps = gst_pad_query_caps (GST_BASE_SRC_PAD (basesrc), NULL);
  GST_DEBUG_OBJECT (basesrc, "caps of src: %" GST_PTR_FORMAT, thiscaps);
  /* nothing or anything is allowed, we're done */
  if (thiscaps == NULL || gst_caps_is_any (thiscaps))
    goto no_nego_needed;

  /* get the peer caps */
  peercaps = gst_pad_peer_query_caps (GST_BASE_SRC_PAD (basesrc), NULL);
  GST_DEBUG_OBJECT (basesrc, "caps of peer: %" GST_PTR_FORMAT, peercaps);
  if (peercaps) {
    /* get intersection */
    caps = gst_caps_intersect (thiscaps, peercaps);
    GST_DEBUG_OBJECT (basesrc, "intersect: %" GST_PTR_FORMAT, caps);
    gst_caps_unref (thiscaps);
    gst_caps_unref (peercaps);
  } else {
    /* no peer, work with our own caps then */
    caps = thiscaps;
  }
  if (caps) {
    /* take first (and best, since they are sorted) possibility */
    caps = gst_caps_truncate (caps);

    /* now fixate */
    if (!gst_caps_is_empty (caps)) {
      caps = GST_BASE_SRC_CLASS (parent_class)->fixate (basesrc, caps);
      GST_DEBUG_OBJECT (basesrc, "fixated to: %" GST_PTR_FORMAT, caps);

      if (gst_caps_is_any (caps)) {
        /* hmm, still anything, so element can do anything and
         * nego is not needed */
        result = TRUE;
      } else if (gst_caps_is_fixed (caps)) {
        /* yay, fixed caps, use those then */
        result = gst_pulsesrc_create_stream (pulsesrc, &caps, NULL);
        if (result)
          result = gst_base_src_set_caps (basesrc, caps);
      }
    }
    gst_caps_unref (caps);
  }
  return result;

no_nego_needed:
  {
    GST_DEBUG_OBJECT (basesrc, "no negotiation needed");
    if (thiscaps)
      gst_caps_unref (thiscaps);
    return TRUE;
  }
}

static gboolean
gst_pulsesrc_prepare (GstAudioSrc * asrc, GstAudioRingBufferSpec * spec)
{
  pa_buffer_attr wanted;
  const pa_buffer_attr *actual;
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (asrc);
  pa_stream_flags_t flags;
  pa_operation *o;
  GstAudioClock *clock;

  pa_threaded_mainloop_lock (pulsesrc->mainloop);

  if (!pulsesrc->stream)
    gst_pulsesrc_create_stream (pulsesrc, NULL, spec);

  {
    GstAudioRingBufferSpec s = *spec;
    const pa_channel_map *m;

    m = pa_stream_get_channel_map (pulsesrc->stream);
    gst_pulse_channel_map_to_gst (m, &s);
    gst_audio_ring_buffer_set_channel_positions (GST_AUDIO_BASE_SRC
        (pulsesrc)->ringbuffer, s.info.position);
  }

  /* enable event notifications */
  GST_LOG_OBJECT (pulsesrc, "subscribing to context events");
  if (!(o = pa_context_subscribe (pulsesrc->context,
              PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT, NULL, NULL))) {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_context_subscribe() failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock_and_fail;
  }

  pa_operation_unref (o);

  /* There's a bit of a disconnect here between the audio ringbuffer and what
   * PulseAudio provides. The audio ringbuffer provide a total of buffer_time
   * worth of buffering, divided into segments of latency_time size. We're
   * asking PulseAudio to provide a total latency of latency_time, which, with
   * PA_STREAM_ADJUST_LATENCY, effectively sets itself up as a ringbuffer with
   * one segment being the hardware buffer, and the other the software buffer.
   * This segment size is returned as the fragsize.
   *
   * Since the two concepts don't map very well, what we do is keep segsize as
   * it is (unless fragsize is even larger, in which case we use that). We'll
   * get data from PulseAudio in smaller chunks than we want to pass on as an
   * element, and we coalesce those chunks in the ringbuffer memory and pass it
   * on in the expected chunk size. */
  wanted.maxlength = spec->segsize * spec->segtotal;
  wanted.tlength = -1;
  wanted.prebuf = 0;
  wanted.minreq = -1;
  wanted.fragsize = spec->segsize;

  GST_INFO_OBJECT (pulsesrc, "maxlength: %d", wanted.maxlength);
  GST_INFO_OBJECT (pulsesrc, "tlength:   %d", wanted.tlength);
  GST_INFO_OBJECT (pulsesrc, "prebuf:    %d", wanted.prebuf);
  GST_INFO_OBJECT (pulsesrc, "minreq:    %d", wanted.minreq);
  GST_INFO_OBJECT (pulsesrc, "fragsize:  %d", wanted.fragsize);

  flags = PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE |
      PA_STREAM_NOT_MONOTONIC | PA_STREAM_ADJUST_LATENCY |
      PA_STREAM_START_CORKED;

  if (pa_stream_connect_record (pulsesrc->stream, pulsesrc->device, &wanted,
          flags) < 0) {
    goto connect_failed;
  }

  /* our clock will now start from 0 again */
  clock = GST_AUDIO_CLOCK (GST_AUDIO_BASE_SRC (pulsesrc)->clock);
  gst_audio_clock_reset (clock, 0);

  pulsesrc->corked = TRUE;

  for (;;) {
    pa_stream_state_t state;

    state = pa_stream_get_state (pulsesrc->stream);

    if (!PA_STREAM_IS_GOOD (state))
      goto stream_is_bad;

    if (state == PA_STREAM_READY)
      break;

    /* Wait until the stream is ready */
    pa_threaded_mainloop_wait (pulsesrc->mainloop);
  }
  pulsesrc->stream_connected = TRUE;

  /* store the source output index so it can be accessed via a property */
  pulsesrc->source_output_idx = pa_stream_get_index (pulsesrc->stream);
  g_object_notify (G_OBJECT (pulsesrc), "source-output-index");

  /* Although source output stream muting is supported, there is a bug in
   * PulseAudio that doesn't allow us to do this at startup, so we mute
   * manually post-connect. This should be moved back pre-connect once things
   * are fixed on the PulseAudio side. */
  if (pulsesrc->mute_set && pulsesrc->mute) {
    gst_pulsesrc_set_stream_mute (pulsesrc, pulsesrc->mute);
    pulsesrc->mute_set = FALSE;
  }

  if (pulsesrc->volume_set) {
    gst_pulsesrc_set_stream_volume (pulsesrc, pulsesrc->volume);
    pulsesrc->volume_set = FALSE;
  }

  /* get the actual buffering properties now */
  actual = pa_stream_get_buffer_attr (pulsesrc->stream);

  GST_INFO_OBJECT (pulsesrc, "maxlength: %d", actual->maxlength);
  GST_INFO_OBJECT (pulsesrc, "tlength:   %d (wanted: %d)",
      actual->tlength, wanted.tlength);
  GST_INFO_OBJECT (pulsesrc, "prebuf:    %d", actual->prebuf);
  GST_INFO_OBJECT (pulsesrc, "minreq:    %d (wanted %d)", actual->minreq,
      wanted.minreq);
  GST_INFO_OBJECT (pulsesrc, "fragsize:  %d (wanted %d)",
      actual->fragsize, wanted.fragsize);

  if (actual->fragsize >= spec->segsize) {
    spec->segsize = actual->fragsize;
  } else {
    /* fragsize is smaller than what we wanted, so let the read function
     * coalesce the smaller chunks as they come in */
  }

  /* Fix up the total ringbuffer size based on what we actually got */
  spec->segtotal = actual->maxlength / spec->segsize;
  /* Don't buffer less than 2 segments as the ringbuffer can't deal with it */
  if (spec->segtotal < 2)
    spec->segtotal = 2;

  if (!pulsesrc->paused) {
    GST_DEBUG_OBJECT (pulsesrc, "uncorking because we are playing");
    gst_pulsesrc_set_corked (pulsesrc, FALSE, FALSE);
  }
  pa_threaded_mainloop_unlock (pulsesrc->mainloop);

  return TRUE;

  /* ERRORS */
connect_failed:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("Failed to connect stream: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock_and_fail;
  }
stream_is_bad:
  {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("Failed to connect stream: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock_and_fail;
  }
unlock_and_fail:
  {
    gst_pulsesrc_destroy_stream (pulsesrc);

    pa_threaded_mainloop_unlock (pulsesrc->mainloop);
    return FALSE;
  }
}

static void
gst_pulsesrc_success_cb (pa_stream * s, int success, void *userdata)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (userdata);

  pulsesrc->operation_success = ! !success;
  pa_threaded_mainloop_signal (pulsesrc->mainloop, 0);
}

static void
gst_pulsesrc_reset (GstAudioSrc * asrc)
{
  GstPulseSrc *pulsesrc = GST_PULSESRC_CAST (asrc);
  pa_operation *o = NULL;

  pa_threaded_mainloop_lock (pulsesrc->mainloop);
  GST_DEBUG_OBJECT (pulsesrc, "reset");

  if (gst_pulsesrc_is_dead (pulsesrc, TRUE))
    goto unlock_and_fail;

  if (!(o =
          pa_stream_flush (pulsesrc->stream, gst_pulsesrc_success_cb,
              pulsesrc))) {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED,
        ("pa_stream_flush() failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock_and_fail;
  }

  pulsesrc->paused = TRUE;
  /* Inform anyone waiting in _write() call that it shall wakeup */
  if (pulsesrc->in_read) {
    pa_threaded_mainloop_signal (pulsesrc->mainloop, 0);
  }

  pulsesrc->operation_success = FALSE;
  while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {

    if (gst_pulsesrc_is_dead (pulsesrc, TRUE))
      goto unlock_and_fail;

    pa_threaded_mainloop_wait (pulsesrc->mainloop);
  }

  if (!pulsesrc->operation_success) {
    GST_ELEMENT_ERROR (pulsesrc, RESOURCE, FAILED, ("Flush failed: %s",
            pa_strerror (pa_context_errno (pulsesrc->context))), (NULL));
    goto unlock_and_fail;
  }

unlock_and_fail:

  if (o) {
    pa_operation_cancel (o);
    pa_operation_unref (o);
  }

  pa_threaded_mainloop_unlock (pulsesrc->mainloop);
}

/* update the corked state of a stream, must be called with the mainloop
 * lock */
static gboolean
gst_pulsesrc_set_corked (GstPulseSrc * psrc, gboolean corked, gboolean wait)
{
  pa_operation *o = NULL;
  gboolean res = FALSE;

  GST_DEBUG_OBJECT (psrc, "setting corked state to %d", corked);
  if (!psrc->stream_connected)
    return TRUE;

  if (psrc->corked != corked) {
    if (!(o = pa_stream_cork (psrc->stream, corked,
                gst_pulsesrc_success_cb, psrc)))
      goto cork_failed;

    while (wait && pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
      pa_threaded_mainloop_wait (psrc->mainloop);
      if (gst_pulsesrc_is_dead (psrc, TRUE))
        goto server_dead;
    }
    psrc->corked = corked;
  } else {
    GST_DEBUG_OBJECT (psrc, "skipping, already in requested state");
  }
  res = TRUE;

cleanup:
  if (o)
    pa_operation_unref (o);

  return res;

  /* ERRORS */
server_dead:
  {
    GST_DEBUG_OBJECT (psrc, "the server is dead");
    goto cleanup;
  }
cork_failed:
  {
    GST_ELEMENT_ERROR (psrc, RESOURCE, FAILED,
        ("pa_stream_cork() failed: %s",
            pa_strerror (pa_context_errno (psrc->context))), (NULL));
    goto cleanup;
  }
}

/* start/resume playback ASAP */
static gboolean
gst_pulsesrc_play (GstPulseSrc * psrc)
{
  pa_threaded_mainloop_lock (psrc->mainloop);
  GST_DEBUG_OBJECT (psrc, "playing");
  psrc->paused = FALSE;
  gst_pulsesrc_set_corked (psrc, FALSE, FALSE);
  pa_threaded_mainloop_unlock (psrc->mainloop);

  return TRUE;
}

/* pause/stop playback ASAP */
static gboolean
gst_pulsesrc_pause (GstPulseSrc * psrc)
{
  pa_threaded_mainloop_lock (psrc->mainloop);
  GST_DEBUG_OBJECT (psrc, "pausing");
  /* make sure the commit method stops writing */
  psrc->paused = TRUE;
  if (psrc->in_read) {
    /* we are waiting in a read, signal */
    GST_DEBUG_OBJECT (psrc, "signal read");
    pa_threaded_mainloop_signal (psrc->mainloop, 0);
  }
  pa_threaded_mainloop_unlock (psrc->mainloop);

  return TRUE;
}

static GstStateChangeReturn
gst_pulsesrc_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstPulseSrc *this = GST_PULSESRC_CAST (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!(this->mainloop = pa_threaded_mainloop_new ()))
        goto mainloop_failed;
      if (pa_threaded_mainloop_start (this->mainloop) < 0) {
        pa_threaded_mainloop_free (this->mainloop);
        this->mainloop = NULL;
        goto mainloop_start_failed;
      }
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_element_post_message (element,
          gst_message_new_clock_provide (GST_OBJECT_CAST (element),
              GST_AUDIO_BASE_SRC (this)->clock, TRUE));
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      /* uncork and start recording */
      gst_pulsesrc_play (this);
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      /* stop recording ASAP by corking */
      pa_threaded_mainloop_lock (this->mainloop);
      GST_DEBUG_OBJECT (this, "corking");
      gst_pulsesrc_set_corked (this, TRUE, FALSE);
      pa_threaded_mainloop_unlock (this->mainloop);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      /* now make sure we get out of the _read method */
      gst_pulsesrc_pause (this);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      if (this->mainloop)
        pa_threaded_mainloop_stop (this->mainloop);

      gst_pulsesrc_destroy_context (this);

      if (this->mainloop) {
        pa_threaded_mainloop_free (this->mainloop);
        this->mainloop = NULL;
      }
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      /* format_lost is reset in release() in baseaudiosink */
      gst_element_post_message (element,
          gst_message_new_clock_lost (GST_OBJECT_CAST (element),
              GST_AUDIO_BASE_SRC (this)->clock));
      break;
    default:
      break;
  }

  return ret;

  /* ERRORS */
mainloop_failed:
  {
    GST_ELEMENT_ERROR (this, RESOURCE, FAILED,
        ("pa_threaded_mainloop_new() failed"), (NULL));
    return GST_STATE_CHANGE_FAILURE;
  }
mainloop_start_failed:
  {
    GST_ELEMENT_ERROR (this, RESOURCE, FAILED,
        ("pa_threaded_mainloop_start() failed"), (NULL));
    return GST_STATE_CHANGE_FAILURE;
  }
}

static GstClockTime
gst_pulsesrc_get_time (GstClock * clock, GstPulseSrc * src)
{
  pa_usec_t time = 0;

  if (src->mainloop == NULL)
    goto out;

  pa_threaded_mainloop_lock (src->mainloop);
  if (!src->stream)
    goto unlock_and_out;

  if (gst_pulsesrc_is_dead (src, TRUE))
    goto unlock_and_out;

  if (pa_stream_get_time (src->stream, &time) < 0) {
    GST_DEBUG_OBJECT (src, "could not get time");
    time = GST_CLOCK_TIME_NONE;
  } else {
    time *= 1000;
  }


unlock_and_out:
  pa_threaded_mainloop_unlock (src->mainloop);

out:
  return time;
}
