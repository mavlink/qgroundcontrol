/*-*- Mode: C; c-basic-offset: 2 -*-*/

/*  GStreamer pulseaudio plugin
 *
 *  Copyright (c) 2004-2008 Lennart Poettering
 *            (c) 2009      Wim Taymans
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
 * SECTION:element-pulsesink
 * @title: pulsesink
 * @see_also: pulsesrc
 *
 * This element outputs audio to a
 * [PulseAudio sound server](http://www.pulseaudio.org).
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v filesrc location=sine.ogg ! oggdemux ! vorbisdec ! audioconvert ! audioresample ! pulsesink
 * ]| Play an Ogg/Vorbis file.
 * |[
 * gst-launch-1.0 -v audiotestsrc ! audioconvert ! volume volume=0.4 ! pulsesink
 * ]| Play a 440Hz sine wave.
 * |[
 * gst-launch-1.0 -v audiotestsrc ! pulsesink stream-properties="props,media.title=test"
 * ]| Play a sine wave and set a stream property. The property can be checked
 * with "pactl list".
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>

#include <gst/base/gstbasesink.h>
#include <gst/gsttaglist.h>
#include <gst/audio/audio.h>
#include <gst/gst-i18n-plugin.h>

#include <gst/pbutils/pbutils.h>        /* only used for GST_PLUGINS_BASE_VERSION_* */

#include <gst/glib-compat-private.h>

#include "pulsesink.h"
#include "pulseutil.h"

GST_DEBUG_CATEGORY_EXTERN (pulse_debug);
#define GST_CAT_DEFAULT pulse_debug

#define DEFAULT_SERVER          NULL
#define DEFAULT_DEVICE          NULL
#define DEFAULT_CURRENT_DEVICE  NULL
#define DEFAULT_DEVICE_NAME     NULL
#define DEFAULT_VOLUME          1.0
#define DEFAULT_MUTE            FALSE
#define MAX_VOLUME              10.0

enum
{
  PROP_0,
  PROP_SERVER,
  PROP_DEVICE,
  PROP_CURRENT_DEVICE,
  PROP_DEVICE_NAME,
  PROP_VOLUME,
  PROP_MUTE,
  PROP_CLIENT_NAME,
  PROP_STREAM_PROPERTIES,
  PROP_LAST
};

#define GST_TYPE_PULSERING_BUFFER        \
        (gst_pulseringbuffer_get_type())
#define GST_PULSERING_BUFFER(obj)        \
        (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PULSERING_BUFFER,GstPulseRingBuffer))
#define GST_PULSERING_BUFFER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PULSERING_BUFFER,GstPulseRingBufferClass))
#define GST_PULSERING_BUFFER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_PULSERING_BUFFER, GstPulseRingBufferClass))
#define GST_PULSERING_BUFFER_CAST(obj)        \
        ((GstPulseRingBuffer *)obj)
#define GST_IS_PULSERING_BUFFER(obj)     \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PULSERING_BUFFER))
#define GST_IS_PULSERING_BUFFER_CLASS(klass)\
        (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PULSERING_BUFFER))

typedef struct _GstPulseRingBuffer GstPulseRingBuffer;
typedef struct _GstPulseRingBufferClass GstPulseRingBufferClass;

typedef struct _GstPulseContext GstPulseContext;

/* A note on threading.
 *
 * We use a pa_threaded_mainloop to interact with the PulseAudio server. This
 * starts up a separate thread that runs a mainloop to carry back events,
 * messages and timing updates from the PulseAudio server.
 *
 * In most cases, the PulseAudio API we use communicates with the server and
 * processes replies asynchronously. Operations on PA objects that result in
 * such communication are protected with a pa_threaded_mainloop_lock() and
 * pa_threaded_mainloop_unlock(). These guarantee mutual exclusion with the
 * mainloop thread -- when an iteration of the mainloop thread begins, it first
 * tries to acquire this lock, and cannot do so if our code also holds that
 * lock.
 *
 * When we need to complete an operation synchronously, we use
 * pa_threaded_mainloop_wait() and pa_threaded_mainloop_signal(). These work
 * much as pthread conditionals do. pa_threaded_mainloop_wait() is called with
 * the mainloop lock held. It releases the lock (thereby allowing the mainloop
 * to execute), and waits till one of our callbacks to be executed by the
 * mainloop thread calls pa_threaded_mainloop_signal(). At the end of the
 * mainloop iteration, the pa_threaded_mainloop_wait() will reacquire the
 * mainloop lock and return control to the caller.
 */

/* Store the PA contexts in a hash table to allow easy sharing among
 * multiple instances of the sink. Keys are $context_name@$server_name
 * (strings) and values should be GstPulseContext pointers.
 */
struct _GstPulseContext
{
  pa_context *context;
  GSList *ring_buffers;
};

static GHashTable *gst_pulse_shared_contexts = NULL;

/* use one static main-loop for all instances
 * this is needed to make the context sharing work as the contexts are
 * released when releasing their parent main-loop
 */
static pa_threaded_mainloop *mainloop = NULL;
static guint mainloop_ref_ct = 0;

/* lock for access to shared resources */
static GMutex pa_shared_resource_mutex;

/* We keep a custom ringbuffer that is backed up by data allocated by
 * pulseaudio. We must also override the commit function to write into
 * pulseaudio memory instead. */
struct _GstPulseRingBuffer
{
  GstAudioRingBuffer object;

  gchar *context_name;
  gchar *stream_name;

  pa_context *context;
  pa_stream *stream;
  pa_stream *probe_stream;

  pa_format_info *format;
  guint channels;
  gboolean is_pcm;

  void *m_data;
  size_t m_towrite;
  size_t m_writable;
  gint64 m_offset;
  gint64 m_lastoffset;

  gboolean corked:1;
  gboolean in_commit:1;
  gboolean paused:1;
};
struct _GstPulseRingBufferClass
{
  GstAudioRingBufferClass parent_class;
};

static GType gst_pulseringbuffer_get_type (void);
static void gst_pulseringbuffer_finalize (GObject * object);

static GstAudioRingBufferClass *ring_parent_class = NULL;

static gboolean gst_pulseringbuffer_open_device (GstAudioRingBuffer * buf);
static gboolean gst_pulseringbuffer_close_device (GstAudioRingBuffer * buf);
static gboolean gst_pulseringbuffer_acquire (GstAudioRingBuffer * buf,
    GstAudioRingBufferSpec * spec);
static gboolean gst_pulseringbuffer_release (GstAudioRingBuffer * buf);
static gboolean gst_pulseringbuffer_start (GstAudioRingBuffer * buf);
static gboolean gst_pulseringbuffer_pause (GstAudioRingBuffer * buf);
static gboolean gst_pulseringbuffer_stop (GstAudioRingBuffer * buf);
static void gst_pulseringbuffer_clear (GstAudioRingBuffer * buf);
static guint gst_pulseringbuffer_commit (GstAudioRingBuffer * buf,
    guint64 * sample, guchar * data, gint in_samples, gint out_samples,
    gint * accum);

G_DEFINE_TYPE (GstPulseRingBuffer, gst_pulseringbuffer,
    GST_TYPE_AUDIO_RING_BUFFER);

static void
gst_pulsesink_init_contexts (void)
{
  g_mutex_init (&pa_shared_resource_mutex);
  gst_pulse_shared_contexts = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, NULL);
}

static void
gst_pulseringbuffer_class_init (GstPulseRingBufferClass * klass)
{
  GObjectClass *gobject_class;
  GstAudioRingBufferClass *gstringbuffer_class;

  gobject_class = (GObjectClass *) klass;
  gstringbuffer_class = (GstAudioRingBufferClass *) klass;

  ring_parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gst_pulseringbuffer_finalize;

  gstringbuffer_class->open_device =
      GST_DEBUG_FUNCPTR (gst_pulseringbuffer_open_device);
  gstringbuffer_class->close_device =
      GST_DEBUG_FUNCPTR (gst_pulseringbuffer_close_device);
  gstringbuffer_class->acquire =
      GST_DEBUG_FUNCPTR (gst_pulseringbuffer_acquire);
  gstringbuffer_class->release =
      GST_DEBUG_FUNCPTR (gst_pulseringbuffer_release);
  gstringbuffer_class->start = GST_DEBUG_FUNCPTR (gst_pulseringbuffer_start);
  gstringbuffer_class->pause = GST_DEBUG_FUNCPTR (gst_pulseringbuffer_pause);
  gstringbuffer_class->resume = GST_DEBUG_FUNCPTR (gst_pulseringbuffer_start);
  gstringbuffer_class->stop = GST_DEBUG_FUNCPTR (gst_pulseringbuffer_stop);
  gstringbuffer_class->clear_all =
      GST_DEBUG_FUNCPTR (gst_pulseringbuffer_clear);

  gstringbuffer_class->commit = GST_DEBUG_FUNCPTR (gst_pulseringbuffer_commit);
}

static void
gst_pulseringbuffer_init (GstPulseRingBuffer * pbuf)
{
  pbuf->stream_name = NULL;
  pbuf->context = NULL;
  pbuf->stream = NULL;
  pbuf->probe_stream = NULL;

  pbuf->format = NULL;
  pbuf->channels = 0;
  pbuf->is_pcm = FALSE;

  pbuf->m_data = NULL;
  pbuf->m_towrite = 0;
  pbuf->m_writable = 0;
  pbuf->m_offset = 0;
  pbuf->m_lastoffset = 0;

  pbuf->corked = TRUE;
  pbuf->in_commit = FALSE;
  pbuf->paused = FALSE;
}

/* Call with mainloop lock held if wait == TRUE) */
static void
gst_pulse_destroy_stream (pa_stream * stream, gboolean wait)
{
  /* Make sure we don't get any further callbacks */
  pa_stream_set_write_callback (stream, NULL, NULL);
  pa_stream_set_underflow_callback (stream, NULL, NULL);
  pa_stream_set_overflow_callback (stream, NULL, NULL);

  pa_stream_disconnect (stream);

  if (wait)
    pa_threaded_mainloop_wait (mainloop);

  pa_stream_set_state_callback (stream, NULL, NULL);
  pa_stream_unref (stream);
}

static void
gst_pulsering_destroy_stream (GstPulseRingBuffer * pbuf)
{
  if (pbuf->probe_stream) {
    gst_pulse_destroy_stream (pbuf->probe_stream, FALSE);
    pbuf->probe_stream = NULL;
  }

  if (pbuf->stream) {

    if (pbuf->m_data) {
      /* drop shm memory buffer */
      pa_stream_cancel_write (pbuf->stream);

      /* reset internal variables */
      pbuf->m_data = NULL;
      pbuf->m_towrite = 0;
      pbuf->m_writable = 0;
      pbuf->m_offset = 0;
      pbuf->m_lastoffset = 0;
    }
    if (pbuf->format) {
      pa_format_info_free (pbuf->format);
      pbuf->format = NULL;
      pbuf->channels = 0;
      pbuf->is_pcm = FALSE;
    }

    pa_stream_disconnect (pbuf->stream);

    /* Make sure we don't get any further callbacks */
    pa_stream_set_state_callback (pbuf->stream, NULL, NULL);
    pa_stream_set_write_callback (pbuf->stream, NULL, NULL);
    pa_stream_set_underflow_callback (pbuf->stream, NULL, NULL);
    pa_stream_set_overflow_callback (pbuf->stream, NULL, NULL);

    pa_stream_unref (pbuf->stream);
    pbuf->stream = NULL;
  }

  g_free (pbuf->stream_name);
  pbuf->stream_name = NULL;
}

static void
gst_pulsering_destroy_context (GstPulseRingBuffer * pbuf)
{
  g_mutex_lock (&pa_shared_resource_mutex);

  GST_DEBUG_OBJECT (pbuf, "destroying ringbuffer %p", pbuf);

  gst_pulsering_destroy_stream (pbuf);

  if (pbuf->context) {
    pa_context_unref (pbuf->context);
    pbuf->context = NULL;
  }

  if (pbuf->context_name) {
    GstPulseContext *pctx;

    pctx = g_hash_table_lookup (gst_pulse_shared_contexts, pbuf->context_name);

    GST_DEBUG_OBJECT (pbuf, "releasing context with name %s, pbuf=%p, pctx=%p",
        pbuf->context_name, pbuf, pctx);

    if (pctx) {
      pctx->ring_buffers = g_slist_remove (pctx->ring_buffers, pbuf);
      if (pctx->ring_buffers == NULL) {
        GST_DEBUG_OBJECT (pbuf,
            "destroying final context with name %s, pbuf=%p, pctx=%p",
            pbuf->context_name, pbuf, pctx);

        pa_context_disconnect (pctx->context);

        /* Make sure we don't get any further callbacks */
        pa_context_set_state_callback (pctx->context, NULL, NULL);
        pa_context_set_subscribe_callback (pctx->context, NULL, NULL);

        g_hash_table_remove (gst_pulse_shared_contexts, pbuf->context_name);

        pa_context_unref (pctx->context);
        g_slice_free (GstPulseContext, pctx);
      }
    }
    g_free (pbuf->context_name);
    pbuf->context_name = NULL;
  }
  g_mutex_unlock (&pa_shared_resource_mutex);
}

static void
gst_pulseringbuffer_finalize (GObject * object)
{
  GstPulseRingBuffer *ringbuffer;

  ringbuffer = GST_PULSERING_BUFFER_CAST (object);

  gst_pulsering_destroy_context (ringbuffer);
  G_OBJECT_CLASS (ring_parent_class)->finalize (object);
}


#define CONTEXT_OK(c) ((c) && PA_CONTEXT_IS_GOOD (pa_context_get_state ((c))))
#define STREAM_OK(s) ((s) && PA_STREAM_IS_GOOD (pa_stream_get_state ((s))))

static gboolean
gst_pulsering_is_dead (GstPulseSink * psink, GstPulseRingBuffer * pbuf,
    gboolean check_stream)
{
  if (!CONTEXT_OK (pbuf->context))
    goto error;

  if (check_stream && !STREAM_OK (pbuf->stream))
    goto error;

  return FALSE;

error:
  {
    const gchar *err_str =
        pbuf->context ? pa_strerror (pa_context_errno (pbuf->context)) : NULL;
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED, ("Disconnected: %s",
            err_str), (NULL));
    return TRUE;
  }
}

static void
gst_pulsering_context_state_cb (pa_context * c, void *userdata)
{
  pa_context_state_t state;
  pa_threaded_mainloop *mainloop = (pa_threaded_mainloop *) userdata;

  state = pa_context_get_state (c);

  GST_LOG ("got new context state %d", state);

  switch (state) {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_FAILED:
      GST_LOG ("signaling");
      pa_threaded_mainloop_signal (mainloop, 0);
      break;

    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;
  }
}

static void
gst_pulsering_context_subscribe_cb (pa_context * c,
    pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
  GstPulseSink *psink;
  GstPulseContext *pctx = (GstPulseContext *) userdata;
  GSList *walk;

  if (t != (PA_SUBSCRIPTION_EVENT_SINK_INPUT | PA_SUBSCRIPTION_EVENT_CHANGE) &&
      t != (PA_SUBSCRIPTION_EVENT_SINK_INPUT | PA_SUBSCRIPTION_EVENT_NEW))
    return;

  for (walk = pctx->ring_buffers; walk; walk = g_slist_next (walk)) {
    GstPulseRingBuffer *pbuf = (GstPulseRingBuffer *) walk->data;
    psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

    GST_LOG_OBJECT (psink, "type %04x, idx %u", t, idx);

    if (!pbuf->stream)
      continue;

    if (idx != pa_stream_get_index (pbuf->stream))
      continue;

    if (psink->device && pbuf->is_pcm &&
        !g_str_equal (psink->device,
            pa_stream_get_device_name (pbuf->stream))) {
      /* Underlying sink changed. And this is not a passthrough stream. Let's
       * see if someone upstream wants to try to renegotiate. */
      GstEvent *renego;

      g_free (psink->device);
      psink->device = g_strdup (pa_stream_get_device_name (pbuf->stream));

      GST_INFO_OBJECT (psink, "emitting sink-changed");

      /* FIXME: send reconfigure event instead and let decodebin/playbin
       * handle that. Also take care of ac3 alignment. See "pulse-format-lost" */
      renego = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
          gst_structure_new_empty ("pulse-sink-changed"));

      if (!gst_pad_push_event (GST_BASE_SINK (psink)->sinkpad, renego))
        GST_DEBUG_OBJECT (psink, "Emitted sink-changed - nobody was listening");
    }

    /* Actually this event is also triggered when other properties of
     * the stream change that are unrelated to the volume. However it is
     * probably cheaper to signal the change here and check for the
     * volume when the GObject property is read instead of querying it always. */

    /* inform streaming thread to notify */
    g_atomic_int_compare_and_exchange (&psink->notify, 0, 1);
  }
}

/* will be called when the device should be opened. In this case we will connect
 * to the server. We should not try to open any streams in this state. */
static gboolean
gst_pulseringbuffer_open_device (GstAudioRingBuffer * buf)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;
  GstPulseContext *pctx;
  pa_mainloop_api *api;
  gboolean need_unlock_shared;

  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (buf));
  pbuf = GST_PULSERING_BUFFER_CAST (buf);

  g_assert (!pbuf->stream);
  g_assert (psink->client_name);

  if (psink->server)
    pbuf->context_name = g_strdup_printf ("%s@%s", psink->client_name,
        psink->server);
  else
    pbuf->context_name = g_strdup (psink->client_name);

  pa_threaded_mainloop_lock (mainloop);

  g_mutex_lock (&pa_shared_resource_mutex);
  need_unlock_shared = TRUE;

  pctx = g_hash_table_lookup (gst_pulse_shared_contexts, pbuf->context_name);
  if (pctx == NULL) {
    pctx = g_slice_new0 (GstPulseContext);

    /* get the mainloop api and create a context */
    GST_INFO_OBJECT (psink, "new context with name %s, pbuf=%p, pctx=%p",
        pbuf->context_name, pbuf, pctx);
    api = pa_threaded_mainloop_get_api (mainloop);
    if (!(pctx->context = pa_context_new (api, pbuf->context_name)))
      goto create_failed;

    pctx->ring_buffers = g_slist_prepend (pctx->ring_buffers, pbuf);
    g_hash_table_insert (gst_pulse_shared_contexts,
        g_strdup (pbuf->context_name), (gpointer) pctx);
    /* register some essential callbacks */
    pa_context_set_state_callback (pctx->context,
        gst_pulsering_context_state_cb, mainloop);
    pa_context_set_subscribe_callback (pctx->context,
        gst_pulsering_context_subscribe_cb, pctx);

    /* try to connect to the server and wait for completion, we don't want to
     * autospawn a daemon */
    GST_LOG_OBJECT (psink, "connect to server %s",
        GST_STR_NULL (psink->server));
    if (pa_context_connect (pctx->context, psink->server,
            PA_CONTEXT_NOAUTOSPAWN, NULL) < 0)
      goto connect_failed;
  } else {
    GST_INFO_OBJECT (psink,
        "reusing shared context with name %s, pbuf=%p, pctx=%p",
        pbuf->context_name, pbuf, pctx);
    pctx->ring_buffers = g_slist_prepend (pctx->ring_buffers, pbuf);
  }

  g_mutex_unlock (&pa_shared_resource_mutex);
  need_unlock_shared = FALSE;

  /* context created or shared okay */
  pbuf->context = pa_context_ref (pctx->context);

  for (;;) {
    pa_context_state_t state;

    state = pa_context_get_state (pbuf->context);

    GST_LOG_OBJECT (psink, "context state is now %d", state);

    if (!PA_CONTEXT_IS_GOOD (state))
      goto connect_failed;

    if (state == PA_CONTEXT_READY)
      break;

    /* Wait until the context is ready */
    GST_LOG_OBJECT (psink, "waiting..");
    pa_threaded_mainloop_wait (mainloop);
  }

  if (pa_context_get_server_protocol_version (pbuf->context) < 22) {
    /* We need PulseAudio >= 1.0 on the server side for the extended API */
    goto bad_server_version;
  }

  GST_LOG_OBJECT (psink, "opened the device");

  pa_threaded_mainloop_unlock (mainloop);

  return TRUE;

  /* ERRORS */
unlock_and_fail:
  {
    if (need_unlock_shared)
      g_mutex_unlock (&pa_shared_resource_mutex);
    gst_pulsering_destroy_context (pbuf);
    pa_threaded_mainloop_unlock (mainloop);
    return FALSE;
  }
create_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("Failed to create context"), (NULL));
    g_slice_free (GstPulseContext, pctx);
    goto unlock_and_fail;
  }
connect_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED, ("Failed to connect: %s",
            pa_strerror (pa_context_errno (pctx->context))), (NULL));
    goto unlock_and_fail;
  }
bad_server_version:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED, ("PulseAudio server version "
            "is too old."), (NULL));
    goto unlock_and_fail;
  }
}

/* close the device */
static gboolean
gst_pulseringbuffer_close_device (GstAudioRingBuffer * buf)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;

  pbuf = GST_PULSERING_BUFFER_CAST (buf);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (buf));

  GST_LOG_OBJECT (psink, "closing device");

  pa_threaded_mainloop_lock (mainloop);
  gst_pulsering_destroy_context (pbuf);
  pa_threaded_mainloop_unlock (mainloop);

  GST_LOG_OBJECT (psink, "closed device");

  return TRUE;
}

static void
gst_pulsering_stream_state_cb (pa_stream * s, void *userdata)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;
  pa_stream_state_t state;

  pbuf = GST_PULSERING_BUFFER_CAST (userdata);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  state = pa_stream_get_state (s);
  GST_LOG_OBJECT (psink, "got new stream state %d", state);

  switch (state) {
    case PA_STREAM_READY:
    case PA_STREAM_FAILED:
    case PA_STREAM_TERMINATED:
      GST_LOG_OBJECT (psink, "signaling");
      pa_threaded_mainloop_signal (mainloop, 0);
      break;
    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:
      break;
  }
}

static void
gst_pulsering_stream_request_cb (pa_stream * s, size_t length, void *userdata)
{
  GstPulseSink *psink;
  GstAudioRingBuffer *rbuf;
  GstPulseRingBuffer *pbuf;

  rbuf = GST_AUDIO_RING_BUFFER_CAST (userdata);
  pbuf = GST_PULSERING_BUFFER_CAST (userdata);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  GST_LOG_OBJECT (psink, "got request for length %" G_GSIZE_FORMAT, length);

  if (pbuf->in_commit && (length >= rbuf->spec.segsize)) {
    /* only signal when we are waiting in the commit thread
     * and got request for at least a segment */
    pa_threaded_mainloop_signal (mainloop, 0);
  }
}

static void
gst_pulsering_stream_underflow_cb (pa_stream * s, void *userdata)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;

  pbuf = GST_PULSERING_BUFFER_CAST (userdata);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  GST_WARNING_OBJECT (psink, "Got underflow");
}

static void
gst_pulsering_stream_overflow_cb (pa_stream * s, void *userdata)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;

  pbuf = GST_PULSERING_BUFFER_CAST (userdata);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  GST_WARNING_OBJECT (psink, "Got overflow");
}

static void
gst_pulsering_stream_latency_cb (pa_stream * s, void *userdata)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;
  GstAudioRingBuffer *ringbuf;
  const pa_timing_info *info;
  pa_usec_t sink_usec;

  info = pa_stream_get_timing_info (s);

  pbuf = GST_PULSERING_BUFFER_CAST (userdata);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));
  ringbuf = GST_AUDIO_RING_BUFFER (pbuf);

  if (!info) {
    GST_LOG_OBJECT (psink, "latency update (information unknown)");
    return;
  }

  if (!info->read_index_corrupt) {
    /* Update segdone based on the read index. segdone is of segment
     * granularity, while the read index is at byte granularity. We take the
     * ceiling while converting the latter to the former since it is more
     * conservative to report that we've read more than we have than to report
     * less. One concern here is that latency updates happen every 100ms, which
     * means segdone is not updated very often, but increasing the update
     * frequency would mean more communication overhead. */
    g_atomic_int_set (&ringbuf->segdone,
        (int) gst_util_uint64_scale_ceil (info->read_index, 1,
            ringbuf->spec.segsize));
  }

  sink_usec = info->configured_sink_usec;

  GST_LOG_OBJECT (psink,
      "latency_update, %" G_GUINT64_FORMAT ", %d:%" G_GINT64_FORMAT ", %d:%"
      G_GUINT64_FORMAT ", %" G_GUINT64_FORMAT ", %" G_GUINT64_FORMAT,
      GST_TIMEVAL_TO_TIME (info->timestamp), info->write_index_corrupt,
      info->write_index, info->read_index_corrupt, info->read_index,
      info->sink_usec, sink_usec);
}

static void
gst_pulsering_stream_suspended_cb (pa_stream * p, void *userdata)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;

  pbuf = GST_PULSERING_BUFFER_CAST (userdata);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  if (pa_stream_is_suspended (p))
    GST_DEBUG_OBJECT (psink, "stream suspended");
  else
    GST_DEBUG_OBJECT (psink, "stream resumed");
}

static void
gst_pulsering_stream_started_cb (pa_stream * p, void *userdata)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;

  pbuf = GST_PULSERING_BUFFER_CAST (userdata);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  GST_DEBUG_OBJECT (psink, "stream started");
}

static void
gst_pulsering_stream_event_cb (pa_stream * p, const char *name,
    pa_proplist * pl, void *userdata)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;

  pbuf = GST_PULSERING_BUFFER_CAST (userdata);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  if (!strcmp (name, PA_STREAM_EVENT_REQUEST_CORK)) {
    /* the stream wants to PAUSE, post a message for the application. */
    GST_DEBUG_OBJECT (psink, "got request for CORK");
    gst_element_post_message (GST_ELEMENT_CAST (psink),
        gst_message_new_request_state (GST_OBJECT_CAST (psink),
            GST_STATE_PAUSED));

  } else if (!strcmp (name, PA_STREAM_EVENT_REQUEST_UNCORK)) {
    GST_DEBUG_OBJECT (psink, "got request for UNCORK");
    gst_element_post_message (GST_ELEMENT_CAST (psink),
        gst_message_new_request_state (GST_OBJECT_CAST (psink),
            GST_STATE_PLAYING));
  } else if (!strcmp (name, PA_STREAM_EVENT_FORMAT_LOST)) {
    GstEvent *renego;

    if (g_atomic_int_get (&psink->format_lost)) {
      /* Duplicate event before we're done reconfiguring, discard */
      return;
    }

    GST_DEBUG_OBJECT (psink, "got FORMAT LOST");
    g_atomic_int_set (&psink->format_lost, 1);
    psink->format_lost_time = g_ascii_strtoull (pa_proplist_gets (pl,
            "stream-time"), NULL, 0) * 1000;

    g_free (psink->device);
    psink->device = g_strdup (pa_proplist_gets (pl, "device"));

    /* FIXME: send reconfigure event instead and let decodebin/playbin
     * handle that. Also take care of ac3 alignment */
    renego = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
        gst_structure_new_empty ("pulse-format-lost"));

#if 0
    if (g_str_equal (gst_structure_get_name (st), "audio/x-eac3")) {
      GstStructure *event_st = gst_structure_new ("ac3parse-set-alignment",
          "alignment", G_TYPE_STRING, pbin->dbin ? "frame" : "iec61937", NULL);

      if (!gst_pad_push_event (pbin->sinkpad,
              gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, event_st)))
        GST_WARNING_OBJECT (pbin->sinkpad, "Could not update alignment");
    }
#endif

    if (!gst_pad_push_event (GST_BASE_SINK (psink)->sinkpad, renego)) {
      /* Nobody handled the format change - emit an error */
      GST_ELEMENT_ERROR (psink, STREAM, FORMAT, ("Sink format changed"),
          ("Sink format changed"));
    }
  } else {
    GST_DEBUG_OBJECT (psink, "got unknown event %s", name);
  }
}

/* Called with the mainloop locked */
static gboolean
gst_pulsering_wait_for_stream_ready (GstPulseSink * psink, pa_stream * stream)
{
  pa_stream_state_t state;

  for (;;) {
    state = pa_stream_get_state (stream);

    GST_LOG_OBJECT (psink, "stream state is now %d", state);

    if (!PA_STREAM_IS_GOOD (state))
      return FALSE;

    if (state == PA_STREAM_READY)
      return TRUE;

    /* Wait until the stream is ready */
    pa_threaded_mainloop_wait (mainloop);
  }
}


/* This method should create a new stream of the given @spec. No playback should
 * start yet so we start in the corked state. */
static gboolean
gst_pulseringbuffer_acquire (GstAudioRingBuffer * buf,
    GstAudioRingBufferSpec * spec)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;
  pa_buffer_attr wanted;
  const pa_buffer_attr *actual;
  pa_channel_map channel_map;
  pa_operation *o = NULL;
  pa_cvolume v;
  pa_cvolume *pv = NULL;
  pa_stream_flags_t flags;
  const gchar *name;
  GstAudioClock *clock;
  pa_format_info *formats[1];
#ifndef GST_DISABLE_GST_DEBUG
  gchar print_buf[PA_FORMAT_INFO_SNPRINT_MAX];
#endif

  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (buf));
  pbuf = GST_PULSERING_BUFFER_CAST (buf);

  GST_LOG_OBJECT (psink, "creating sample spec");
  /* convert the gstreamer sample spec to the pulseaudio format */
  if (!gst_pulse_fill_format_info (spec, &pbuf->format, &pbuf->channels))
    goto invalid_spec;
  pbuf->is_pcm = pa_format_info_is_pcm (pbuf->format);

  pa_threaded_mainloop_lock (mainloop);

  /* we need a context and a no stream */
  g_assert (pbuf->context);
  g_assert (!pbuf->stream);

  /* if we have a probe, disconnect it first so that if we're creating a
   * compressed stream, it doesn't get blocked by a PCM stream */
  if (pbuf->probe_stream) {
    gst_pulse_destroy_stream (pbuf->probe_stream, TRUE);
    pbuf->probe_stream = NULL;
  }

  /* enable event notifications */
  GST_LOG_OBJECT (psink, "subscribing to context events");
  if (!(o = pa_context_subscribe (pbuf->context,
              PA_SUBSCRIPTION_MASK_SINK_INPUT, NULL, NULL)))
    goto subscribe_failed;

  pa_operation_unref (o);

  /* initialize the channel map */
  if (pbuf->is_pcm && gst_pulse_gst_to_channel_map (&channel_map, spec))
    pa_format_info_set_channel_map (pbuf->format, &channel_map);

  /* find a good name for the stream */
  if (psink->stream_name)
    name = psink->stream_name;
  else
    name = "Playback Stream";

  /* create a stream */
  formats[0] = pbuf->format;
  if (!(pbuf->stream = pa_stream_new_extended (pbuf->context, name, formats, 1,
              psink->proplist)))
    goto stream_failed;

  /* install essential callbacks */
  pa_stream_set_state_callback (pbuf->stream,
      gst_pulsering_stream_state_cb, pbuf);
  pa_stream_set_write_callback (pbuf->stream,
      gst_pulsering_stream_request_cb, pbuf);
  pa_stream_set_underflow_callback (pbuf->stream,
      gst_pulsering_stream_underflow_cb, pbuf);
  pa_stream_set_overflow_callback (pbuf->stream,
      gst_pulsering_stream_overflow_cb, pbuf);
  pa_stream_set_latency_update_callback (pbuf->stream,
      gst_pulsering_stream_latency_cb, pbuf);
  pa_stream_set_suspended_callback (pbuf->stream,
      gst_pulsering_stream_suspended_cb, pbuf);
  pa_stream_set_started_callback (pbuf->stream,
      gst_pulsering_stream_started_cb, pbuf);
  pa_stream_set_event_callback (pbuf->stream,
      gst_pulsering_stream_event_cb, pbuf);

  /* buffering requirements. When setting prebuf to 0, the stream will not pause
   * when we cause an underrun, which causes time to continue. */
  memset (&wanted, 0, sizeof (wanted));
  wanted.tlength = spec->segtotal * spec->segsize;
  wanted.maxlength = -1;
  wanted.prebuf = 0;
  wanted.minreq = spec->segsize;

  GST_INFO_OBJECT (psink, "tlength:   %d", wanted.tlength);
  GST_INFO_OBJECT (psink, "maxlength: %d", wanted.maxlength);
  GST_INFO_OBJECT (psink, "prebuf:    %d", wanted.prebuf);
  GST_INFO_OBJECT (psink, "minreq:    %d", wanted.minreq);

  /* configure volume when we changed it, else we leave the default */
  if (psink->volume_set) {
    GST_LOG_OBJECT (psink, "have volume of %f", psink->volume);
    pv = &v;
    if (pbuf->is_pcm)
      gst_pulse_cvolume_from_linear (pv, pbuf->channels, psink->volume);
    else {
      GST_DEBUG_OBJECT (psink, "passthrough stream, not setting volume");
      pv = NULL;
    }
  } else {
    pv = NULL;
  }

  /* construct the flags */
  flags = PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE |
      PA_STREAM_ADJUST_LATENCY | PA_STREAM_START_CORKED;

  if (psink->mute_set) {
    if (psink->mute)
      flags |= PA_STREAM_START_MUTED;
    else
      flags |= PA_STREAM_START_UNMUTED;
  }

  /* we always start corked (see flags above) */
  pbuf->corked = TRUE;

  /* try to connect now */
  GST_LOG_OBJECT (psink, "connect for playback to device %s",
      GST_STR_NULL (psink->device));
  if (pa_stream_connect_playback (pbuf->stream, psink->device,
          &wanted, flags, pv, NULL) < 0)
    goto connect_failed;

  /* our clock will now start from 0 again */
  clock = GST_AUDIO_CLOCK (GST_AUDIO_BASE_SINK (psink)->provided_clock);
  gst_audio_clock_reset (clock, 0);

  if (!gst_pulsering_wait_for_stream_ready (psink, pbuf->stream))
    goto connect_failed;

  g_free (psink->device);
  psink->device = g_strdup (pa_stream_get_device_name (pbuf->stream));

#ifndef GST_DISABLE_GST_DEBUG
  pa_format_info_snprint (print_buf, sizeof (print_buf),
      pa_stream_get_format_info (pbuf->stream));
  GST_INFO_OBJECT (psink, "negotiated to: %s", print_buf);
#endif

  /* After we passed the volume off of to PA we never want to set it
     again, since it is PA's job to save/restore volumes.  */
  psink->volume_set = psink->mute_set = FALSE;

  GST_LOG_OBJECT (psink, "stream is acquired now");

  /* get the actual buffering properties now */
  actual = pa_stream_get_buffer_attr (pbuf->stream);

  GST_INFO_OBJECT (psink, "tlength:   %d (wanted: %d)", actual->tlength,
      wanted.tlength);
  GST_INFO_OBJECT (psink, "maxlength: %d", actual->maxlength);
  GST_INFO_OBJECT (psink, "prebuf:    %d", actual->prebuf);
  GST_INFO_OBJECT (psink, "minreq:    %d (wanted %d)", actual->minreq,
      wanted.minreq);

  spec->segsize = actual->minreq;
  spec->segtotal = actual->tlength / spec->segsize;

  pa_threaded_mainloop_unlock (mainloop);

  return TRUE;

  /* ERRORS */
unlock_and_fail:
  {
    gst_pulsering_destroy_stream (pbuf);
    pa_threaded_mainloop_unlock (mainloop);

    return FALSE;
  }
invalid_spec:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, SETTINGS,
        ("Invalid sample specification."), (NULL));
    return FALSE;
  }
subscribe_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_context_subscribe() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock_and_fail;
  }
stream_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("Failed to create stream: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock_and_fail;
  }
connect_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("Failed to connect stream: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock_and_fail;
  }
}

/* free the stream that we acquired before */
static gboolean
gst_pulseringbuffer_release (GstAudioRingBuffer * buf)
{
  GstPulseRingBuffer *pbuf;

  pbuf = GST_PULSERING_BUFFER_CAST (buf);

  pa_threaded_mainloop_lock (mainloop);
  gst_pulsering_destroy_stream (pbuf);
  pa_threaded_mainloop_unlock (mainloop);

  {
    GstPulseSink *psink;

    psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));
    g_atomic_int_set (&psink->format_lost, FALSE);
    psink->format_lost_time = GST_CLOCK_TIME_NONE;
  }

  return TRUE;
}

static void
gst_pulsering_success_cb (pa_stream * s, int success, void *userdata)
{
  pa_threaded_mainloop_signal (mainloop, 0);
}

/* update the corked state of a stream, must be called with the mainloop
 * lock */
static gboolean
gst_pulsering_set_corked (GstPulseRingBuffer * pbuf, gboolean corked,
    gboolean wait)
{
  pa_operation *o = NULL;
  GstPulseSink *psink;
  gboolean res = FALSE;

  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  if (g_atomic_int_get (&psink->format_lost)) {
    /* Sink format changed, stream's gone so fake being paused */
    return TRUE;
  }

  GST_DEBUG_OBJECT (psink, "setting corked state to %d", corked);
  if (pbuf->corked != corked) {
    if (!(o = pa_stream_cork (pbuf->stream, corked,
                gst_pulsering_success_cb, pbuf)))
      goto cork_failed;

    while (wait && pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
      pa_threaded_mainloop_wait (mainloop);
      if (gst_pulsering_is_dead (psink, pbuf, TRUE))
        goto server_dead;
    }
    pbuf->corked = corked;
  } else {
    GST_DEBUG_OBJECT (psink, "skipping, already in requested state");
  }
  res = TRUE;

cleanup:
  if (o)
    pa_operation_unref (o);

  return res;

  /* ERRORS */
server_dead:
  {
    GST_DEBUG_OBJECT (psink, "the server is dead");
    goto cleanup;
  }
cork_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_stream_cork() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto cleanup;
  }
}

static void
gst_pulseringbuffer_clear (GstAudioRingBuffer * buf)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;
  pa_operation *o = NULL;

  pbuf = GST_PULSERING_BUFFER_CAST (buf);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  pa_threaded_mainloop_lock (mainloop);
  GST_DEBUG_OBJECT (psink, "clearing");
  if (pbuf->stream) {
    /* don't wait for the flush to complete */
    if ((o = pa_stream_flush (pbuf->stream, NULL, pbuf)))
      pa_operation_unref (o);
  }
  pa_threaded_mainloop_unlock (mainloop);
}

#if 0
/* called from pulse thread with the mainloop lock */
static void
mainloop_enter_defer_cb (pa_mainloop_api * api, void *userdata)
{
  GstPulseSink *pulsesink = GST_PULSESINK (userdata);
  GstMessage *message;
  GValue val = { 0 };

  GST_DEBUG_OBJECT (pulsesink, "posting ENTER stream status");
  message = gst_message_new_stream_status (GST_OBJECT (pulsesink),
      GST_STREAM_STATUS_TYPE_ENTER, GST_ELEMENT (pulsesink));
  g_value_init (&val, GST_TYPE_G_THREAD);
  g_value_set_boxed (&val, g_thread_self ());
  gst_message_set_stream_status_object (message, &val);
  g_value_unset (&val);

  gst_element_post_message (GST_ELEMENT (pulsesink), message);

  g_return_if_fail (pulsesink->defer_pending);
  pulsesink->defer_pending--;
  pa_threaded_mainloop_signal (mainloop, 0);
}
#endif

/* start/resume playback ASAP, we don't uncork here but in the commit method */
static gboolean
gst_pulseringbuffer_start (GstAudioRingBuffer * buf)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;

  pbuf = GST_PULSERING_BUFFER_CAST (buf);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  pa_threaded_mainloop_lock (mainloop);

  GST_DEBUG_OBJECT (psink, "starting");
  pbuf->paused = FALSE;

  /* EOS needs running clock */
  if (GST_BASE_SINK_CAST (psink)->eos ||
      g_atomic_int_get (&GST_AUDIO_BASE_SINK (psink)->eos_rendering))
    gst_pulsering_set_corked (pbuf, FALSE, FALSE);

#if 0
  GST_DEBUG_OBJECT (psink, "scheduling stream status");
  psink->defer_pending++;
  pa_mainloop_api_once (pa_threaded_mainloop_get_api (mainloop),
      mainloop_enter_defer_cb, psink);

  /* Wait for the stream status message to be posted. This needs to be done
   * synchronously because the callback will take the mainloop lock
   * (implicitly) and then take the GST_OBJECT_LOCK. Everywhere else, we take
   * the locks in the reverse order, so not doing this synchronously could
   * cause a deadlock. */
  GST_DEBUG_OBJECT (psink, "waiting for stream status (ENTER) to be posted");
  pa_threaded_mainloop_wait (mainloop);
#endif

  pa_threaded_mainloop_unlock (mainloop);

  return TRUE;
}

/* pause/stop playback ASAP */
static gboolean
gst_pulseringbuffer_pause (GstAudioRingBuffer * buf)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;
  gboolean res;

  pbuf = GST_PULSERING_BUFFER_CAST (buf);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  pa_threaded_mainloop_lock (mainloop);
  GST_DEBUG_OBJECT (psink, "pausing and corking");
  /* make sure the commit method stops writing */
  pbuf->paused = TRUE;
  res = gst_pulsering_set_corked (pbuf, TRUE, TRUE);
  if (pbuf->in_commit) {
    /* we are waiting in a commit, signal */
    GST_DEBUG_OBJECT (psink, "signal commit");
    pa_threaded_mainloop_signal (mainloop, 0);
  }
  pa_threaded_mainloop_unlock (mainloop);

  return res;
}

#if 0
/* called from pulse thread with the mainloop lock */
static void
mainloop_leave_defer_cb (pa_mainloop_api * api, void *userdata)
{
  GstPulseSink *pulsesink = GST_PULSESINK (userdata);
  GstMessage *message;
  GValue val = { 0 };

  GST_DEBUG_OBJECT (pulsesink, "posting LEAVE stream status");
  message = gst_message_new_stream_status (GST_OBJECT (pulsesink),
      GST_STREAM_STATUS_TYPE_LEAVE, GST_ELEMENT (pulsesink));
  g_value_init (&val, GST_TYPE_G_THREAD);
  g_value_set_boxed (&val, g_thread_self ());
  gst_message_set_stream_status_object (message, &val);
  g_value_unset (&val);

  gst_element_post_message (GST_ELEMENT (pulsesink), message);

  g_return_if_fail (pulsesink->defer_pending);
  pulsesink->defer_pending--;
  pa_threaded_mainloop_signal (mainloop, 0);
}
#endif

/* stop playback, we flush everything. */
static gboolean
gst_pulseringbuffer_stop (GstAudioRingBuffer * buf)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;
  gboolean res = FALSE;
  pa_operation *o = NULL;

  pbuf = GST_PULSERING_BUFFER_CAST (buf);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  pa_threaded_mainloop_lock (mainloop);

  pbuf->paused = TRUE;
  res = gst_pulsering_set_corked (pbuf, TRUE, TRUE);

  /* Inform anyone waiting in _commit() call that it shall wakeup */
  if (pbuf->in_commit) {
    GST_DEBUG_OBJECT (psink, "signal commit thread");
    pa_threaded_mainloop_signal (mainloop, 0);
  }
  if (g_atomic_int_get (&psink->format_lost)) {
    /* Don't try to flush, the stream's probably gone by now */
    res = TRUE;
    goto cleanup;
  }

  /* then try to flush, it's not fatal when this fails */
  GST_DEBUG_OBJECT (psink, "flushing");
  if ((o = pa_stream_flush (pbuf->stream, gst_pulsering_success_cb, pbuf))) {
    while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
      GST_DEBUG_OBJECT (psink, "wait for completion");
      pa_threaded_mainloop_wait (mainloop);
      if (gst_pulsering_is_dead (psink, pbuf, TRUE))
        goto server_dead;
    }
    GST_DEBUG_OBJECT (psink, "flush completed");
  }
  res = TRUE;

cleanup:
  if (o) {
    pa_operation_cancel (o);
    pa_operation_unref (o);
  }
#if 0
  GST_DEBUG_OBJECT (psink, "scheduling stream status");
  psink->defer_pending++;
  pa_mainloop_api_once (pa_threaded_mainloop_get_api (mainloop),
      mainloop_leave_defer_cb, psink);

  /* Wait for the stream status message to be posted. This needs to be done
   * synchronously because the callback will take the mainloop lock
   * (implicitly) and then take the GST_OBJECT_LOCK. Everywhere else, we take
   * the locks in the reverse order, so not doing this synchronously could
   * cause a deadlock. */
  GST_DEBUG_OBJECT (psink, "waiting for stream status (LEAVE) to be posted");
  pa_threaded_mainloop_wait (mainloop);
#endif

  pa_threaded_mainloop_unlock (mainloop);

  return res;

  /* ERRORS */
server_dead:
  {
    GST_DEBUG_OBJECT (psink, "the server is dead");
    goto cleanup;
  }
}

/* in_samples >= out_samples, rate > 1.0 */
#define FWD_UP_SAMPLES(s,se,d,de)               \
G_STMT_START {                                  \
  guint8 *sb = s, *db = d;                      \
  while (s <= se && d < de) {                   \
    memcpy (d, s, bpf);                         \
    s += bpf;                                   \
    *accum += outr;                             \
    if ((*accum << 1) >= inr) {                 \
      *accum -= inr;                            \
      d += bpf;                                 \
    }                                           \
  }                                             \
  in_samples -= (s - sb)/bpf;                   \
  out_samples -= (d - db)/bpf;                  \
  GST_DEBUG ("fwd_up end %d/%d",*accum,*toprocess);     \
} G_STMT_END

/* out_samples > in_samples, for rates smaller than 1.0 */
#define FWD_DOWN_SAMPLES(s,se,d,de)             \
G_STMT_START {                                  \
  guint8 *sb = s, *db = d;                      \
  while (s <= se && d < de) {                   \
    memcpy (d, s, bpf);                         \
    d += bpf;                                   \
    *accum += inr;                              \
    if ((*accum << 1) >= outr) {                \
      *accum -= outr;                           \
      s += bpf;                                 \
    }                                           \
  }                                             \
  in_samples -= (s - sb)/bpf;                   \
  out_samples -= (d - db)/bpf;                  \
  GST_DEBUG ("fwd_down end %d/%d",*accum,*toprocess);   \
} G_STMT_END

#define REV_UP_SAMPLES(s,se,d,de)               \
G_STMT_START {                                  \
  guint8 *sb = se, *db = d;                     \
  while (s <= se && d < de) {                   \
    memcpy (d, se, bpf);                        \
    se -= bpf;                                  \
    *accum += outr;                             \
    while (d < de && (*accum << 1) >= inr) {    \
      *accum -= inr;                            \
      d += bpf;                                 \
    }                                           \
  }                                             \
  in_samples -= (sb - se)/bpf;                  \
  out_samples -= (d - db)/bpf;                  \
  GST_DEBUG ("rev_up end %d/%d",*accum,*toprocess);     \
} G_STMT_END

#define REV_DOWN_SAMPLES(s,se,d,de)             \
G_STMT_START {                                  \
  guint8 *sb = se, *db = d;                     \
  while (s <= se && d < de) {                   \
    memcpy (d, se, bpf);                        \
    d += bpf;                                   \
    *accum += inr;                              \
    while (s <= se && (*accum << 1) >= outr) {  \
      *accum -= outr;                           \
      se -= bpf;                                \
    }                                           \
  }                                             \
  in_samples -= (sb - se)/bpf;                  \
  out_samples -= (d - db)/bpf;                  \
  GST_DEBUG ("rev_down end %d/%d",*accum,*toprocess);   \
} G_STMT_END

/* our custom commit function because we write into the buffer of pulseaudio
 * instead of keeping our own buffer */
static guint
gst_pulseringbuffer_commit (GstAudioRingBuffer * buf, guint64 * sample,
    guchar * data, gint in_samples, gint out_samples, gint * accum)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;
  guint result;
  guint8 *data_end;
  gboolean reverse;
  gint *toprocess;
  gint inr, outr, bpf;
  gint64 offset;
  guint bufsize;

  pbuf = GST_PULSERING_BUFFER_CAST (buf);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  /* FIXME post message rather than using a signal (as mixer interface) */
  if (g_atomic_int_compare_and_exchange (&psink->notify, 1, 0)) {
    g_object_notify (G_OBJECT (psink), "volume");
    g_object_notify (G_OBJECT (psink), "mute");
    g_object_notify (G_OBJECT (psink), "current-device");
  }

  /* make sure the ringbuffer is started */
  if (G_UNLIKELY (g_atomic_int_get (&buf->state) !=
          GST_AUDIO_RING_BUFFER_STATE_STARTED)) {
    /* see if we are allowed to start it */
    if (G_UNLIKELY (g_atomic_int_get (&buf->may_start) == FALSE))
      goto no_start;

    GST_DEBUG_OBJECT (buf, "start!");
    if (!gst_audio_ring_buffer_start (buf))
      goto start_failed;
  }

  pa_threaded_mainloop_lock (mainloop);

  GST_DEBUG_OBJECT (psink, "entering commit");
  pbuf->in_commit = TRUE;

  bpf = GST_AUDIO_INFO_BPF (&buf->spec.info);
  bufsize = buf->spec.segsize * buf->spec.segtotal;

  /* our toy resampler for trick modes */
  reverse = out_samples < 0;
  out_samples = ABS (out_samples);

  if (in_samples >= out_samples)
    toprocess = &in_samples;
  else
    toprocess = &out_samples;

  inr = in_samples - 1;
  outr = out_samples - 1;

  GST_DEBUG_OBJECT (psink, "in %d, out %d", inr, outr);

  /* data_end points to the last sample we have to write, not past it. This is
   * needed to properly handle reverse playback: it points to the last sample. */
  data_end = data + (bpf * inr);

  if (g_atomic_int_get (&psink->format_lost)) {
    /* Sink format changed, drop the data and hope upstream renegotiates */
    goto fake_done;
  }

  if (pbuf->paused)
    goto was_paused;

  /* offset is in bytes */
  offset = *sample * bpf;

  while (*toprocess > 0) {
    size_t avail;
    guint towrite;

    GST_LOG_OBJECT (psink,
        "need to write %d samples at offset %" G_GINT64_FORMAT, *toprocess,
        offset);

    if (offset != pbuf->m_lastoffset)
      GST_LOG_OBJECT (psink, "discontinuity, offset is %" G_GINT64_FORMAT ", "
          "last offset was %" G_GINT64_FORMAT, offset, pbuf->m_lastoffset);

    towrite = out_samples * bpf;

    /* Wait for at least segsize bytes to become available */
    if (towrite > buf->spec.segsize)
      towrite = buf->spec.segsize;

    if ((pbuf->m_writable < towrite) || (offset != pbuf->m_lastoffset)) {
      /* if no room left or discontinuity in offset,
         we need to flush data and get a new buffer */

      /* flush the buffer if possible */
      if ((pbuf->m_data != NULL) && (pbuf->m_towrite > 0)) {

        GST_LOG_OBJECT (psink,
            "flushing %u samples at offset %" G_GINT64_FORMAT,
            (guint) pbuf->m_towrite / bpf, pbuf->m_offset);

        if (pa_stream_write (pbuf->stream, (uint8_t *) pbuf->m_data,
                pbuf->m_towrite, NULL, pbuf->m_offset, PA_SEEK_ABSOLUTE) < 0) {
          goto write_failed;
        }
      }
      pbuf->m_towrite = 0;
      pbuf->m_offset = offset;  /* keep track of current offset */

      /* get a buffer to write in for now on */
      for (;;) {
        pbuf->m_writable = pa_stream_writable_size (pbuf->stream);

        if (g_atomic_int_get (&psink->format_lost)) {
          /* Sink format changed, give up and hope upstream renegotiates */
          goto fake_done;
        }

        if (pbuf->m_writable == (size_t) - 1)
          goto writable_size_failed;

        pbuf->m_writable /= bpf;
        pbuf->m_writable *= bpf;        /* handle only complete samples */

        if (pbuf->m_writable >= towrite)
          break;

        /* see if we need to uncork because we have no free space */
        if (pbuf->corked) {
          if (!gst_pulsering_set_corked (pbuf, FALSE, FALSE))
            goto uncork_failed;
        }

        /* we can't write segsize bytes, wait a bit */
        GST_LOG_OBJECT (psink, "waiting for free space");
        pa_threaded_mainloop_wait (mainloop);

        if (pbuf->paused)
          goto was_paused;
      }

      /* Recalculate what we can write in the next chunk */
      towrite = out_samples * bpf;
      if (pbuf->m_writable > towrite)
        pbuf->m_writable = towrite;

      GST_LOG_OBJECT (psink, "requesting %" G_GSIZE_FORMAT " bytes of "
          "shared memory", pbuf->m_writable);

      if (pa_stream_begin_write (pbuf->stream, &pbuf->m_data,
              &pbuf->m_writable) < 0) {
        GST_LOG_OBJECT (psink, "pa_stream_begin_write() failed");
        goto writable_size_failed;
      }

      GST_LOG_OBJECT (psink, "got %" G_GSIZE_FORMAT " bytes of shared memory",
          pbuf->m_writable);

    }

    if (towrite > pbuf->m_writable)
      towrite = pbuf->m_writable;
    avail = towrite / bpf;

    GST_LOG_OBJECT (psink, "writing %u samples at offset %" G_GUINT64_FORMAT,
        (guint) avail, offset);

    /* No trick modes for passthrough streams */
    if (G_UNLIKELY (!pbuf->is_pcm && (inr != outr || reverse))) {
      GST_WARNING_OBJECT (psink, "Passthrough stream can't run in trick mode");
      goto unlock_and_fail;
    }

    if (G_LIKELY (inr == outr && !reverse)) {
      /* no rate conversion, simply write out the samples */
      /* copy the data into internal buffer */

      memcpy ((guint8 *) pbuf->m_data + pbuf->m_towrite, data, towrite);
      pbuf->m_towrite += towrite;
      pbuf->m_writable -= towrite;

      data += towrite;
      in_samples -= avail;
      out_samples -= avail;
    } else {
      guint8 *dest, *d, *d_end;

      /* write into the PulseAudio shm buffer */
      dest = d = (guint8 *) pbuf->m_data + pbuf->m_towrite;
      d_end = d + towrite;

      if (!reverse) {
        if (inr >= outr)
          /* forward speed up */
          FWD_UP_SAMPLES (data, data_end, d, d_end);
        else
          /* forward slow down */
          FWD_DOWN_SAMPLES (data, data_end, d, d_end);
      } else {
        if (inr >= outr)
          /* reverse speed up */
          REV_UP_SAMPLES (data, data_end, d, d_end);
        else
          /* reverse slow down */
          REV_DOWN_SAMPLES (data, data_end, d, d_end);
      }
      /* see what we have left to write */
      towrite = (d - dest);
      pbuf->m_towrite += towrite;
      pbuf->m_writable -= towrite;

      avail = towrite / bpf;
    }

    /* flush the buffer if it's full */
    if ((pbuf->m_data != NULL) && (pbuf->m_towrite > 0)
        && (pbuf->m_writable == 0)) {
      GST_LOG_OBJECT (psink, "flushing %u samples at offset %" G_GINT64_FORMAT,
          (guint) pbuf->m_towrite / bpf, pbuf->m_offset);

      if (pa_stream_write (pbuf->stream, (uint8_t *) pbuf->m_data,
              pbuf->m_towrite, NULL, pbuf->m_offset, PA_SEEK_ABSOLUTE) < 0) {
        goto write_failed;
      }
      pbuf->m_towrite = 0;
      pbuf->m_offset = offset + towrite;        /* keep track of current offset */
    }

    *sample += avail;
    offset += avail * bpf;
    pbuf->m_lastoffset = offset;

    /* check if we need to uncork after writing the samples */
    if (pbuf->corked) {
      const pa_timing_info *info;

      if ((info = pa_stream_get_timing_info (pbuf->stream))) {
        GST_LOG_OBJECT (psink,
            "read_index at %" G_GUINT64_FORMAT ", offset %" G_GINT64_FORMAT,
            info->read_index, offset);

        /* we uncork when the read_index is too far behind the offset we need
         * to write to. */
        if (info->read_index + bufsize <= offset) {
          if (!gst_pulsering_set_corked (pbuf, FALSE, FALSE))
            goto uncork_failed;
        }
      } else {
        GST_LOG_OBJECT (psink, "no timing info available yet");
      }
    }
  }

fake_done:
  /* we consumed all samples here */
  data = data_end + bpf;

  pbuf->in_commit = FALSE;
  pa_threaded_mainloop_unlock (mainloop);

done:
  result = inr - ((data_end - data) / bpf);
  GST_LOG_OBJECT (psink, "wrote %d samples", result);

  return result;

  /* ERRORS */
unlock_and_fail:
  {
    pbuf->in_commit = FALSE;
    GST_LOG_OBJECT (psink, "we are reset");
    pa_threaded_mainloop_unlock (mainloop);
    goto done;
  }
no_start:
  {
    GST_LOG_OBJECT (psink, "we can not start");
    return 0;
  }
start_failed:
  {
    GST_LOG_OBJECT (psink, "failed to start the ringbuffer");
    return 0;
  }
uncork_failed:
  {
    pbuf->in_commit = FALSE;
    GST_ERROR_OBJECT (psink, "uncork failed");
    pa_threaded_mainloop_unlock (mainloop);
    goto done;
  }
was_paused:
  {
    pbuf->in_commit = FALSE;
    GST_LOG_OBJECT (psink, "we are paused");
    pa_threaded_mainloop_unlock (mainloop);
    goto done;
  }
writable_size_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_stream_writable_size() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock_and_fail;
  }
write_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_stream_write() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock_and_fail;
  }
}

/* write pending local samples, must be called with the mainloop lock */
static void
gst_pulsering_flush (GstPulseRingBuffer * pbuf)
{
  GstPulseSink *psink;

  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));
  GST_DEBUG_OBJECT (psink, "entering flush");

  /* flush the buffer if possible */
  if (pbuf->stream && (pbuf->m_data != NULL) && (pbuf->m_towrite > 0)) {
#ifndef GST_DISABLE_GST_DEBUG
    gint bpf;

    bpf = (GST_AUDIO_RING_BUFFER_CAST (pbuf))->spec.info.bpf;
    GST_LOG_OBJECT (psink,
        "flushing %u samples at offset %" G_GINT64_FORMAT,
        (guint) pbuf->m_towrite / bpf, pbuf->m_offset);
#endif

    if (pa_stream_write (pbuf->stream, (uint8_t *) pbuf->m_data,
            pbuf->m_towrite, NULL, pbuf->m_offset, PA_SEEK_ABSOLUTE) < 0) {
      goto write_failed;
    }

    pbuf->m_towrite = 0;
    pbuf->m_offset += pbuf->m_towrite;  /* keep track of current offset */
  }

done:
  return;

  /* ERRORS */
write_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_stream_write() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto done;
  }
}

static void gst_pulsesink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_pulsesink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_pulsesink_finalize (GObject * object);

static gboolean gst_pulsesink_event (GstBaseSink * sink, GstEvent * event);
static gboolean gst_pulsesink_query (GstBaseSink * sink, GstQuery * query);

static GstStateChangeReturn gst_pulsesink_change_state (GstElement * element,
    GstStateChange transition);

#define gst_pulsesink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstPulseSink, gst_pulsesink, GST_TYPE_AUDIO_BASE_SINK,
    gst_pulsesink_init_contexts ();
    G_IMPLEMENT_INTERFACE (GST_TYPE_STREAM_VOLUME, NULL)
    );

static GstAudioRingBuffer *
gst_pulsesink_create_ringbuffer (GstAudioBaseSink * sink)
{
  GstAudioRingBuffer *buffer;

  GST_DEBUG_OBJECT (sink, "creating ringbuffer");
  buffer = g_object_new (GST_TYPE_PULSERING_BUFFER, NULL);
  GST_DEBUG_OBJECT (sink, "created ringbuffer @%p", buffer);

  return buffer;
}

static GstBuffer *
gst_pulsesink_payload (GstAudioBaseSink * sink, GstBuffer * buf)
{
  switch (sink->ringbuffer->spec.type) {
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_AC3:
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_EAC3:
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_DTS:
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MPEG:
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MPEG2_AAC:
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MPEG4_AAC:
    {
      /* FIXME: alloc memory from PA if possible */
      gint framesize = gst_audio_iec61937_frame_size (&sink->ringbuffer->spec);
      GstBuffer *out;
      GstMapInfo inmap, outmap;
      gboolean res;

      if (framesize <= 0)
        return NULL;

      out = gst_buffer_new_and_alloc (framesize);

      gst_buffer_map (buf, &inmap, GST_MAP_READ);
      gst_buffer_map (out, &outmap, GST_MAP_WRITE);

      res = gst_audio_iec61937_payload (inmap.data, inmap.size,
          outmap.data, outmap.size, &sink->ringbuffer->spec, G_BIG_ENDIAN);

      gst_buffer_unmap (buf, &inmap);
      gst_buffer_unmap (out, &outmap);

      if (!res) {
        gst_buffer_unref (out);
        return NULL;
      }

      gst_buffer_copy_into (out, buf, GST_BUFFER_COPY_METADATA, 0, -1);
      return out;
    }

    default:
      return gst_buffer_ref (buf);
  }
}

static void
gst_pulsesink_class_init (GstPulseSinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSinkClass *gstbasesink_class = GST_BASE_SINK_CLASS (klass);
  GstBaseSinkClass *bc;
  GstAudioBaseSinkClass *gstaudiosink_class = GST_AUDIO_BASE_SINK_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstCaps *caps;
  gchar *clientname;

  gobject_class->finalize = gst_pulsesink_finalize;
  gobject_class->set_property = gst_pulsesink_set_property;
  gobject_class->get_property = gst_pulsesink_get_property;

  gstbasesink_class->event = GST_DEBUG_FUNCPTR (gst_pulsesink_event);
  gstbasesink_class->query = GST_DEBUG_FUNCPTR (gst_pulsesink_query);

  /* restore the original basesink pull methods */
  bc = g_type_class_peek (GST_TYPE_BASE_SINK);
  gstbasesink_class->activate_pull = GST_DEBUG_FUNCPTR (bc->activate_pull);

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_pulsesink_change_state);

  gstaudiosink_class->create_ringbuffer =
      GST_DEBUG_FUNCPTR (gst_pulsesink_create_ringbuffer);
  gstaudiosink_class->payload = GST_DEBUG_FUNCPTR (gst_pulsesink_payload);

  /* Overwrite GObject fields */
  g_object_class_install_property (gobject_class,
      PROP_SERVER,
      g_param_spec_string ("server", "Server",
          "The PulseAudio server to connect to", DEFAULT_SERVER,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device",
          "The PulseAudio sink device to connect to", DEFAULT_DEVICE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CURRENT_DEVICE,
      g_param_spec_string ("current-device", "Current Device",
          "The current PulseAudio sink device", DEFAULT_CURRENT_DEVICE,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device name",
          "Human-readable name of the sound device", DEFAULT_DEVICE_NAME,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_VOLUME,
      g_param_spec_double ("volume", "Volume",
          "Linear volume of this stream, 1.0=100%", 0.0, MAX_VOLUME,
          DEFAULT_VOLUME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_MUTE,
      g_param_spec_boolean ("mute", "Mute",
          "Mute state of this stream", DEFAULT_MUTE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstPulseSink:client-name:
   *
   * The PulseAudio client name to use.
   */
  clientname = gst_pulse_client_name ();
  g_object_class_install_property (gobject_class,
      PROP_CLIENT_NAME,
      g_param_spec_string ("client-name", "Client Name",
          "The PulseAudio client name to use", clientname,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_READY));
  g_free (clientname);

  /**
   * GstPulseSink:stream-properties:
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
   * gst_structure_free
   * ]|
   */
  g_object_class_install_property (gobject_class,
      PROP_STREAM_PROPERTIES,
      g_param_spec_boxed ("stream-properties", "stream properties",
          "list of pulseaudio stream properties",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "PulseAudio Audio Sink",
      "Sink/Audio", "Plays audio to a PulseAudio server", "Lennart Poettering");

  caps =
      gst_pulse_fix_pcm_caps (gst_caps_from_string (PULSE_SINK_TEMPLATE_CAPS));
  gst_element_class_add_pad_template (gstelement_class,
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS, caps));
  gst_caps_unref (caps);
}

static void
free_device_info (GstPulseDeviceInfo * device_info)
{
  GList *l;

  g_free (device_info->description);

  for (l = g_list_first (device_info->formats); l; l = g_list_next (l))
    pa_format_info_free ((pa_format_info *) l->data);

  g_list_free (device_info->formats);
}

/* Returns the current time of the sink ringbuffer. The timing_info is updated
 * on every data write/flush and every 100ms (PA_STREAM_AUTO_TIMING_UPDATE).
 */
static GstClockTime
gst_pulsesink_get_time (GstClock * clock, GstAudioBaseSink * sink)
{
  GstPulseSink *psink;
  GstPulseRingBuffer *pbuf;
  pa_usec_t time;

  if (!sink->ringbuffer || !sink->ringbuffer->acquired)
    return GST_CLOCK_TIME_NONE;

  pbuf = GST_PULSERING_BUFFER_CAST (sink->ringbuffer);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  if (g_atomic_int_get (&psink->format_lost)) {
    /* Stream was lost in a format change, it'll get set up again once
     * upstream renegotiates */
    return psink->format_lost_time;
  }

  pa_threaded_mainloop_lock (mainloop);
  if (gst_pulsering_is_dead (psink, pbuf, TRUE))
    goto server_dead;

  /* if we don't have enough data to get a timestamp, just return NONE, which
   * will return the last reported time */
  if (pa_stream_get_time (pbuf->stream, &time) < 0) {
    GST_DEBUG_OBJECT (psink, "could not get time");
    time = GST_CLOCK_TIME_NONE;
  } else
    time *= 1000;
  pa_threaded_mainloop_unlock (mainloop);

  GST_LOG_OBJECT (psink, "current time is %" GST_TIME_FORMAT,
      GST_TIME_ARGS (time));

  return time;

  /* ERRORS */
server_dead:
  {
    GST_DEBUG_OBJECT (psink, "the server is dead");
    pa_threaded_mainloop_unlock (mainloop);

    return GST_CLOCK_TIME_NONE;
  }
}

static void
gst_pulsesink_sink_info_cb (pa_context * c, const pa_sink_info * i, int eol,
    void *userdata)
{
  GstPulseDeviceInfo *device_info = (GstPulseDeviceInfo *) userdata;
  guint8 j;

  if (!i)
    goto done;

  device_info->description = g_strdup (i->description);

  device_info->formats = NULL;
  for (j = 0; j < i->n_formats; j++)
    device_info->formats = g_list_prepend (device_info->formats,
        pa_format_info_copy (i->formats[j]));

done:
  pa_threaded_mainloop_signal (mainloop, 0);
}

/* Call with mainloop lock held */
static pa_stream *
gst_pulsesink_create_probe_stream (GstPulseSink * psink,
    GstPulseRingBuffer * pbuf, pa_format_info * format)
{
  pa_format_info *formats[1] = { format };
  pa_stream *stream;
  pa_stream_flags_t flags;

  GST_LOG_OBJECT (psink, "Creating probe stream");

  if (!(stream = pa_stream_new_extended (pbuf->context, "pulsesink probe",
              formats, 1, psink->proplist)))
    goto error;

  /* construct the flags */
  flags = PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE |
      PA_STREAM_ADJUST_LATENCY | PA_STREAM_START_CORKED;

  pa_stream_set_state_callback (stream, gst_pulsering_stream_state_cb, pbuf);

  if (pa_stream_connect_playback (stream, psink->device, NULL, flags, NULL,
          NULL) < 0)
    goto error;

  if (!gst_pulsering_wait_for_stream_ready (psink, stream))
    goto error;

  return stream;

error:
  if (stream)
    pa_stream_unref (stream);
  return NULL;
}

static GstCaps *
gst_pulsesink_query_getcaps (GstPulseSink * psink, GstCaps * filter)
{
  GstPulseRingBuffer *pbuf = NULL;
  GstPulseDeviceInfo device_info = { NULL, NULL };
  GstCaps *ret = NULL;
  GList *i;
  pa_operation *o = NULL;
  pa_stream *stream;

  GST_OBJECT_LOCK (psink);
  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);
  if (pbuf != NULL)
    gst_object_ref (pbuf);
  GST_OBJECT_UNLOCK (psink);

  if (!pbuf) {
    ret = gst_pad_get_pad_template_caps (GST_AUDIO_BASE_SINK_PAD (psink));
    goto out;
  }

  GST_OBJECT_LOCK (pbuf);
  pa_threaded_mainloop_lock (mainloop);

  if (!pbuf->context) {
    ret = gst_pad_get_pad_template_caps (GST_AUDIO_BASE_SINK_PAD (psink));
    goto unlock;
  }

  ret = gst_caps_new_empty ();

  if (pbuf->stream) {
    /* We're in PAUSED or higher */
    stream = pbuf->stream;

  } else if (pbuf->probe_stream) {
    /* We're not paused, but have a cached probe stream */
    stream = pbuf->probe_stream;

  } else {
    /* We're not yet in PAUSED and still need to create a probe stream.
     *
     * FIXME: PA doesn't accept "any" format. We fix something reasonable since
     * this is merely a probe. This should eventually be fixed in PA and
     * hard-coding the format should be dropped. */
    pa_format_info *format = pa_format_info_new ();
    format->encoding = PA_ENCODING_PCM;
    pa_format_info_set_sample_format (format, PA_SAMPLE_S16LE);
    pa_format_info_set_rate (format, GST_AUDIO_DEF_RATE);
    pa_format_info_set_channels (format, GST_AUDIO_DEF_CHANNELS);

    pbuf->probe_stream = gst_pulsesink_create_probe_stream (psink, pbuf,
        format);

    pa_format_info_free (format);

    if (!pbuf->probe_stream) {
      GST_WARNING_OBJECT (psink, "Could not create probe stream");
      goto unlock;
    }

    stream = pbuf->probe_stream;
  }

  if (!(o = pa_context_get_sink_info_by_name (pbuf->context,
              pa_stream_get_device_name (stream), gst_pulsesink_sink_info_cb,
              &device_info)))
    goto info_failed;

  while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
    pa_threaded_mainloop_wait (mainloop);
    if (gst_pulsering_is_dead (psink, pbuf, FALSE))
      goto unlock;
  }

  for (i = g_list_first (device_info.formats); i; i = g_list_next (i)) {
    GstCaps *caps = gst_pulse_format_info_to_caps ((pa_format_info *) i->data);
    if (caps)
      gst_caps_append (ret, caps);
  }

unlock:
  pa_threaded_mainloop_unlock (mainloop);
  /* FIXME: this could be freed after device_name is got */
  GST_OBJECT_UNLOCK (pbuf);

  if (filter) {
    GstCaps *tmp = gst_caps_intersect_full (filter, ret,
        GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (ret);
    ret = tmp;
  }

out:
  free_device_info (&device_info);

  if (o)
    pa_operation_unref (o);

  if (pbuf)
    gst_object_unref (pbuf);

  GST_DEBUG_OBJECT (psink, "caps %" GST_PTR_FORMAT, ret);

  return ret;

info_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_context_get_sink_input_info() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock;
  }
}

static gboolean
gst_pulsesink_query_acceptcaps (GstPulseSink * psink, GstCaps * caps)
{
  GstPulseRingBuffer *pbuf = NULL;
  GstPulseDeviceInfo device_info = { NULL, NULL };
  GstCaps *pad_caps;
  GstStructure *st;
  gboolean ret = FALSE;

  GstAudioRingBufferSpec spec = { 0 };
  pa_operation *o = NULL;
  pa_channel_map channel_map;
  pa_format_info *format = NULL;
  guint channels;

  pad_caps = gst_pad_get_pad_template_caps (GST_BASE_SINK_PAD (psink));
  ret = gst_caps_is_subset (caps, pad_caps);
  gst_caps_unref (pad_caps);

  GST_DEBUG_OBJECT (psink, "caps %" GST_PTR_FORMAT, caps);

  /* Template caps didn't match */
  if (!ret)
    goto done;

  /* If we've not got fixed caps, creating a stream might fail, so let's just
   * return from here with default acceptcaps behaviour */
  if (!gst_caps_is_fixed (caps))
    goto done;

  GST_OBJECT_LOCK (psink);
  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);
  if (pbuf != NULL)
    gst_object_ref (pbuf);
  GST_OBJECT_UNLOCK (psink);

  /* We're still in NULL state */
  if (pbuf == NULL)
    goto done;

  GST_OBJECT_LOCK (pbuf);
  pa_threaded_mainloop_lock (mainloop);

  if (pbuf->context == NULL)
    goto out;

  ret = FALSE;

  spec.latency_time = GST_AUDIO_BASE_SINK (psink)->latency_time;
  if (!gst_audio_ring_buffer_parse_caps (&spec, caps))
    goto out;

  if (!gst_pulse_fill_format_info (&spec, &format, &channels))
    goto out;

  /* Make sure input is framed (one frame per buffer) and can be payloaded */
  if (!pa_format_info_is_pcm (format)) {
    gboolean framed = FALSE, parsed = FALSE;
    st = gst_caps_get_structure (caps, 0);

    gst_structure_get_boolean (st, "framed", &framed);
    gst_structure_get_boolean (st, "parsed", &parsed);
    if ((!framed && !parsed) || gst_audio_iec61937_frame_size (&spec) <= 0)
      goto out;
  }

  /* initialize the channel map */
  if (pa_format_info_is_pcm (format) &&
      gst_pulse_gst_to_channel_map (&channel_map, &spec))
    pa_format_info_set_channel_map (format, &channel_map);

  if (pbuf->stream || pbuf->probe_stream) {
    /* We're already in PAUSED or above, so just reuse this stream to query
     * sink formats and use those. */
    GList *i;
    const char *device_name = pa_stream_get_device_name (pbuf->stream ?
        pbuf->stream : pbuf->probe_stream);

    if (!(o = pa_context_get_sink_info_by_name (pbuf->context, device_name,
                gst_pulsesink_sink_info_cb, &device_info)))
      goto info_failed;

    while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
      pa_threaded_mainloop_wait (mainloop);
      if (gst_pulsering_is_dead (psink, pbuf, FALSE))
        goto out;
    }

    for (i = g_list_first (device_info.formats); i; i = g_list_next (i)) {
      if (pa_format_info_is_compatible ((pa_format_info *) i->data, format)) {
        ret = TRUE;
        break;
      }
    }
  } else {
    /* We're in READY, let's connect a stream to see if the format is
     * accepted by whatever sink we're routed to */
    pbuf->probe_stream = gst_pulsesink_create_probe_stream (psink, pbuf,
        format);
    if (pbuf->probe_stream)
      ret = TRUE;
  }

out:
  if (format)
    pa_format_info_free (format);

  free_device_info (&device_info);

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (mainloop);
  GST_OBJECT_UNLOCK (pbuf);

  gst_caps_replace (&spec.caps, NULL);
  gst_object_unref (pbuf);

done:

  return ret;

info_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_context_get_sink_input_info() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto out;
  }
}

static void
gst_pulsesink_init (GstPulseSink * pulsesink)
{
  pulsesink->server = NULL;
  pulsesink->device = NULL;
  pulsesink->device_info.description = NULL;
  pulsesink->client_name = gst_pulse_client_name ();

  pulsesink->device_info.formats = NULL;

  pulsesink->volume = DEFAULT_VOLUME;
  pulsesink->volume_set = FALSE;

  pulsesink->mute = DEFAULT_MUTE;
  pulsesink->mute_set = FALSE;

  pulsesink->notify = 0;

  g_atomic_int_set (&pulsesink->format_lost, FALSE);
  pulsesink->format_lost_time = GST_CLOCK_TIME_NONE;

  pulsesink->properties = NULL;
  pulsesink->proplist = NULL;

  /* override with a custom clock */
  if (GST_AUDIO_BASE_SINK (pulsesink)->provided_clock)
    gst_object_unref (GST_AUDIO_BASE_SINK (pulsesink)->provided_clock);

  GST_AUDIO_BASE_SINK (pulsesink)->provided_clock =
      gst_audio_clock_new ("GstPulseSinkClock",
      (GstAudioClockGetTimeFunc) gst_pulsesink_get_time, pulsesink, NULL);
}

static void
gst_pulsesink_finalize (GObject * object)
{
  GstPulseSink *pulsesink = GST_PULSESINK_CAST (object);

  g_free (pulsesink->server);
  g_free (pulsesink->device);
  g_free (pulsesink->client_name);
  g_free (pulsesink->current_sink_name);

  free_device_info (&pulsesink->device_info);

  if (pulsesink->properties)
    gst_structure_free (pulsesink->properties);
  if (pulsesink->proplist)
    pa_proplist_free (pulsesink->proplist);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_pulsesink_set_volume (GstPulseSink * psink, gdouble volume)
{
  pa_cvolume v;
  pa_operation *o = NULL;
  GstPulseRingBuffer *pbuf;
  uint32_t idx;

  if (!mainloop)
    goto no_mainloop;

  pa_threaded_mainloop_lock (mainloop);

  GST_DEBUG_OBJECT (psink, "setting volume to %f", volume);

  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);
  if (pbuf == NULL || pbuf->stream == NULL)
    goto no_buffer;

  if ((idx = pa_stream_get_index (pbuf->stream)) == PA_INVALID_INDEX)
    goto no_index;

  if (pbuf->is_pcm)
    gst_pulse_cvolume_from_linear (&v, pbuf->channels, volume);
  else
    /* FIXME: this will eventually be superseded by checks to see if the volume
     * is readable/writable */
    goto unlock;

  if (!(o = pa_context_set_sink_input_volume (pbuf->context, idx,
              &v, NULL, NULL)))
    goto volume_failed;

  /* We don't really care about the result of this call */
unlock:

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (mainloop);

  return;

  /* ERRORS */
no_mainloop:
  {
    psink->volume = volume;
    psink->volume_set = TRUE;

    GST_DEBUG_OBJECT (psink, "we have no mainloop");
    return;
  }
no_buffer:
  {
    psink->volume = volume;
    psink->volume_set = TRUE;

    GST_DEBUG_OBJECT (psink, "we have no ringbuffer");
    goto unlock;
  }
no_index:
  {
    GST_DEBUG_OBJECT (psink, "we don't have a stream index");
    goto unlock;
  }
volume_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_stream_set_sink_input_volume() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock;
  }
}

static void
gst_pulsesink_set_mute (GstPulseSink * psink, gboolean mute)
{
  pa_operation *o = NULL;
  GstPulseRingBuffer *pbuf;
  uint32_t idx;

  if (!mainloop)
    goto no_mainloop;

  pa_threaded_mainloop_lock (mainloop);

  GST_DEBUG_OBJECT (psink, "setting mute state to %d", mute);

  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);
  if (pbuf == NULL || pbuf->stream == NULL)
    goto no_buffer;

  if ((idx = pa_stream_get_index (pbuf->stream)) == PA_INVALID_INDEX)
    goto no_index;

  if (!(o = pa_context_set_sink_input_mute (pbuf->context, idx,
              mute, NULL, NULL)))
    goto mute_failed;

  /* We don't really care about the result of this call */
unlock:

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (mainloop);

  return;

  /* ERRORS */
no_mainloop:
  {
    psink->mute = mute;
    psink->mute_set = TRUE;

    GST_DEBUG_OBJECT (psink, "we have no mainloop");
    return;
  }
no_buffer:
  {
    psink->mute = mute;
    psink->mute_set = TRUE;

    GST_DEBUG_OBJECT (psink, "we have no ringbuffer");
    goto unlock;
  }
no_index:
  {
    GST_DEBUG_OBJECT (psink, "we don't have a stream index");
    goto unlock;
  }
mute_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_stream_set_sink_input_mute() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock;
  }
}

static void
gst_pulsesink_sink_input_info_cb (pa_context * c, const pa_sink_input_info * i,
    int eol, void *userdata)
{
  GstPulseRingBuffer *pbuf;
  GstPulseSink *psink;

  pbuf = GST_PULSERING_BUFFER_CAST (userdata);
  psink = GST_PULSESINK_CAST (GST_OBJECT_PARENT (pbuf));

  if (!i)
    goto done;

  if (!pbuf->stream)
    goto done;

  /* If the index doesn't match our current stream,
   * it implies we just recreated the stream (caps change)
   */
  if (i->index == pa_stream_get_index (pbuf->stream)) {
    psink->volume = pa_sw_volume_to_linear (pa_cvolume_max (&i->volume));
    psink->mute = i->mute;
    psink->current_sink_idx = i->sink;

    if (psink->volume > MAX_VOLUME) {
      GST_WARNING_OBJECT (psink, "Clipped volume from %f to %f", psink->volume,
          MAX_VOLUME);
      psink->volume = MAX_VOLUME;
    }
  }

done:
  pa_threaded_mainloop_signal (mainloop, 0);
}

static void
gst_pulsesink_get_sink_input_info (GstPulseSink * psink, gdouble * volume,
    gboolean * mute)
{
  GstPulseRingBuffer *pbuf;
  pa_operation *o = NULL;
  uint32_t idx;

  if (!mainloop)
    goto no_mainloop;

  pa_threaded_mainloop_lock (mainloop);

  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);
  if (pbuf == NULL || pbuf->stream == NULL)
    goto no_buffer;

  if ((idx = pa_stream_get_index (pbuf->stream)) == PA_INVALID_INDEX)
    goto no_index;

  if (!(o = pa_context_get_sink_input_info (pbuf->context, idx,
              gst_pulsesink_sink_input_info_cb, pbuf)))
    goto info_failed;

  while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
    pa_threaded_mainloop_wait (mainloop);
    if (gst_pulsering_is_dead (psink, pbuf, TRUE))
      goto unlock;
  }

unlock:
  if (volume)
    *volume = psink->volume;
  if (mute)
    *mute = psink->mute;

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (mainloop);

  return;

  /* ERRORS */
no_mainloop:
  {
    if (volume)
      *volume = psink->volume;
    if (mute)
      *mute = psink->mute;

    GST_DEBUG_OBJECT (psink, "we have no mainloop");
    return;
  }
no_buffer:
  {
    GST_DEBUG_OBJECT (psink, "we have no ringbuffer");
    goto unlock;
  }
no_index:
  {
    GST_DEBUG_OBJECT (psink, "we don't have a stream index");
    goto unlock;
  }
info_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_context_get_sink_input_info() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock;
  }
}

static void
gst_pulsesink_current_sink_info_cb (pa_context * c, const pa_sink_info * i,
    int eol, void *userdata)
{
  GstPulseSink *psink;

  psink = GST_PULSESINK_CAST (userdata);

  if (!i)
    goto done;

  /* If the index doesn't match our current stream,
   * it implies we just recreated the stream (caps change)
   */
  if (i->index == psink->current_sink_idx) {
    g_free (psink->current_sink_name);
    psink->current_sink_name = g_strdup (i->name);
  }

done:
  pa_threaded_mainloop_signal (mainloop, 0);
}

static gchar *
gst_pulsesink_get_current_device (GstPulseSink * pulsesink)
{
  pa_operation *o = NULL;
  GstPulseRingBuffer *pbuf;
  gchar *current_sink;

  if (!mainloop)
    goto no_mainloop;

  pbuf =
      GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (pulsesink)->ringbuffer);
  if (pbuf == NULL || pbuf->stream == NULL)
    goto no_buffer;

  gst_pulsesink_get_sink_input_info (pulsesink, NULL, NULL);

  pa_threaded_mainloop_lock (mainloop);

  if (!(o = pa_context_get_sink_info_by_index (pbuf->context,
              pulsesink->current_sink_idx, gst_pulsesink_current_sink_info_cb,
              pulsesink)))
    goto info_failed;

  while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
    pa_threaded_mainloop_wait (mainloop);
    if (gst_pulsering_is_dead (pulsesink, pbuf, TRUE))
      goto unlock;
  }

unlock:

  current_sink = g_strdup (pulsesink->current_sink_name);

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (mainloop);

  return current_sink;

  /* ERRORS */
no_mainloop:
  {
    GST_DEBUG_OBJECT (pulsesink, "we have no mainloop");
    return NULL;
  }
no_buffer:
  {
    GST_DEBUG_OBJECT (pulsesink, "we have no ringbuffer");
    return NULL;
  }
info_failed:
  {
    GST_ELEMENT_ERROR (pulsesink, RESOURCE, FAILED,
        ("pa_context_get_sink_input_info() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock;
  }
}

static gchar *
gst_pulsesink_device_description (GstPulseSink * psink)
{
  GstPulseRingBuffer *pbuf;
  pa_operation *o = NULL;
  gchar *t;

  if (!mainloop)
    goto no_mainloop;

  pa_threaded_mainloop_lock (mainloop);
  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);
  if (pbuf == NULL)
    goto no_buffer;

  free_device_info (&psink->device_info);
  if (!(o = pa_context_get_sink_info_by_name (pbuf->context,
              psink->device, gst_pulsesink_sink_info_cb, &psink->device_info)))
    goto info_failed;

  while (pa_operation_get_state (o) == PA_OPERATION_RUNNING) {
    pa_threaded_mainloop_wait (mainloop);
    if (gst_pulsering_is_dead (psink, pbuf, FALSE))
      goto unlock;
  }

unlock:
  if (o)
    pa_operation_unref (o);

  t = g_strdup (psink->device_info.description);
  pa_threaded_mainloop_unlock (mainloop);

  return t;

  /* ERRORS */
no_mainloop:
  {
    GST_DEBUG_OBJECT (psink, "we have no mainloop");
    return NULL;
  }
no_buffer:
  {
    GST_DEBUG_OBJECT (psink, "we have no ringbuffer");
    goto unlock;
  }
info_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_context_get_sink_info_by_index() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock;
  }
}

static void
gst_pulsesink_set_stream_device (GstPulseSink * psink, const gchar * device)
{
  pa_operation *o = NULL;
  GstPulseRingBuffer *pbuf;
  uint32_t idx;

  if (!mainloop)
    goto no_mainloop;

  pa_threaded_mainloop_lock (mainloop);

  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);
  if (pbuf == NULL || pbuf->stream == NULL)
    goto no_buffer;

  if ((idx = pa_stream_get_index (pbuf->stream)) == PA_INVALID_INDEX)
    goto no_index;


  GST_DEBUG_OBJECT (psink, "setting stream device to %s", device);

  if (!(o = pa_context_move_sink_input_by_name (pbuf->context, idx, device,
              NULL, NULL)))
    goto move_failed;

unlock:

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (mainloop);

  return;

  /* ERRORS */
no_mainloop:
  {
    GST_DEBUG_OBJECT (psink, "we have no mainloop");
    return;
  }
no_buffer:
  {
    GST_DEBUG_OBJECT (psink, "we have no ringbuffer");
    goto unlock;
  }
no_index:
  {
    GST_DEBUG_OBJECT (psink, "we don't have a stream index");
    return;
  }
move_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_context_move_sink_input_by_name(%s) failed: %s", device,
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock;
  }
}


static void
gst_pulsesink_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstPulseSink *pulsesink = GST_PULSESINK_CAST (object);

  switch (prop_id) {
    case PROP_SERVER:
      g_free (pulsesink->server);
      pulsesink->server = g_value_dup_string (value);
      break;
    case PROP_DEVICE:
      g_free (pulsesink->device);
      pulsesink->device = g_value_dup_string (value);
      gst_pulsesink_set_stream_device (pulsesink, pulsesink->device);
      break;
    case PROP_VOLUME:
      gst_pulsesink_set_volume (pulsesink, g_value_get_double (value));
      break;
    case PROP_MUTE:
      gst_pulsesink_set_mute (pulsesink, g_value_get_boolean (value));
      break;
    case PROP_CLIENT_NAME:
      g_free (pulsesink->client_name);
      if (!g_value_get_string (value)) {
        GST_WARNING_OBJECT (pulsesink,
            "Empty PulseAudio client name not allowed. Resetting to default value");
        pulsesink->client_name = gst_pulse_client_name ();
      } else
        pulsesink->client_name = g_value_dup_string (value);
      break;
    case PROP_STREAM_PROPERTIES:
      if (pulsesink->properties)
        gst_structure_free (pulsesink->properties);
      pulsesink->properties =
          gst_structure_copy (gst_value_get_structure (value));
      if (pulsesink->proplist)
        pa_proplist_free (pulsesink->proplist);
      pulsesink->proplist = gst_pulse_make_proplist (pulsesink->properties);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_pulsesink_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{

  GstPulseSink *pulsesink = GST_PULSESINK_CAST (object);

  switch (prop_id) {
    case PROP_SERVER:
      g_value_set_string (value, pulsesink->server);
      break;
    case PROP_DEVICE:
      g_value_set_string (value, pulsesink->device);
      break;
    case PROP_CURRENT_DEVICE:
    {
      gchar *current_device = gst_pulsesink_get_current_device (pulsesink);
      if (current_device)
        g_value_take_string (value, current_device);
      else
        g_value_set_string (value, "");
      break;
    }
    case PROP_DEVICE_NAME:
      g_value_take_string (value, gst_pulsesink_device_description (pulsesink));
      break;
    case PROP_VOLUME:
    {
      gdouble volume;

      gst_pulsesink_get_sink_input_info (pulsesink, &volume, NULL);
      g_value_set_double (value, volume);
      break;
    }
    case PROP_MUTE:
    {
      gboolean mute;

      gst_pulsesink_get_sink_input_info (pulsesink, NULL, &mute);
      g_value_set_boolean (value, mute);
      break;
    }
    case PROP_CLIENT_NAME:
      g_value_set_string (value, pulsesink->client_name);
      break;
    case PROP_STREAM_PROPERTIES:
      gst_value_set_structure (value, pulsesink->properties);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_pulsesink_change_title (GstPulseSink * psink, const gchar * t)
{
  pa_operation *o = NULL;
  GstPulseRingBuffer *pbuf;

  pa_threaded_mainloop_lock (mainloop);

  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);

  if (pbuf == NULL || pbuf->stream == NULL)
    goto no_buffer;

  g_free (pbuf->stream_name);
  pbuf->stream_name = g_strdup (t);

  if (!(o = pa_stream_set_name (pbuf->stream, pbuf->stream_name, NULL, NULL)))
    goto name_failed;

  /* We're not interested if this operation failed or not */
unlock:

  if (o)
    pa_operation_unref (o);
  pa_threaded_mainloop_unlock (mainloop);

  return;

  /* ERRORS */
no_buffer:
  {
    GST_DEBUG_OBJECT (psink, "we have no ringbuffer");
    goto unlock;
  }
name_failed:
  {
    GST_ELEMENT_ERROR (psink, RESOURCE, FAILED,
        ("pa_stream_set_name() failed: %s",
            pa_strerror (pa_context_errno (pbuf->context))), (NULL));
    goto unlock;
  }
}

static void
gst_pulsesink_change_props (GstPulseSink * psink, GstTagList * l)
{
  static const gchar *const map[] = {
    GST_TAG_TITLE, PA_PROP_MEDIA_TITLE,

    /* might get overridden in the next iteration by GST_TAG_ARTIST */
    GST_TAG_PERFORMER, PA_PROP_MEDIA_ARTIST,

    GST_TAG_ARTIST, PA_PROP_MEDIA_ARTIST,
    GST_TAG_LANGUAGE_CODE, PA_PROP_MEDIA_LANGUAGE,
    GST_TAG_LOCATION, PA_PROP_MEDIA_FILENAME,
    /* We might add more here later on ... */
    NULL
  };
  pa_proplist *pl = NULL;
  const gchar *const *t;
  gboolean empty = TRUE;
  pa_operation *o = NULL;
  GstPulseRingBuffer *pbuf;

  pl = pa_proplist_new ();

  for (t = map; *t; t += 2) {
    gchar *n = NULL;

    if (gst_tag_list_get_string (l, *t, &n)) {

      if (n && *n) {
        pa_proplist_sets (pl, *(t + 1), n);
        empty = FALSE;
      }

      g_free (n);
    }
  }
  if (empty)
    goto finish;

  pa_threaded_mainloop_lock (mainloop);
  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);
  if (pbuf == NULL || pbuf->stream == NULL)
    goto no_buffer;

  /* We're not interested if this operation failed or not */
  if (!(o = pa_stream_proplist_update (pbuf->stream, PA_UPDATE_REPLACE,
              pl, NULL, NULL))) {
    GST_DEBUG_OBJECT (psink, "pa_stream_proplist_update() failed");
  }

unlock:

  if (o)
    pa_operation_unref (o);

  pa_threaded_mainloop_unlock (mainloop);

finish:

  if (pl)
    pa_proplist_free (pl);

  return;

  /* ERRORS */
no_buffer:
  {
    GST_DEBUG_OBJECT (psink, "we have no ringbuffer");
    goto unlock;
  }
}

static void
gst_pulsesink_flush_ringbuffer (GstPulseSink * psink)
{
  GstPulseRingBuffer *pbuf;

  pa_threaded_mainloop_lock (mainloop);

  pbuf = GST_PULSERING_BUFFER_CAST (GST_AUDIO_BASE_SINK (psink)->ringbuffer);

  if (pbuf == NULL || pbuf->stream == NULL)
    goto no_buffer;

  gst_pulsering_flush (pbuf);

  /* Uncork if we haven't already (happens when waiting to get enough data
   * to send out the first time) */
  if (pbuf->corked)
    gst_pulsering_set_corked (pbuf, FALSE, FALSE);

  /* We're not interested if this operation failed or not */
unlock:
  pa_threaded_mainloop_unlock (mainloop);

  return;

  /* ERRORS */
no_buffer:
  {
    GST_DEBUG_OBJECT (psink, "we have no ringbuffer");
    goto unlock;
  }
}

static gboolean
gst_pulsesink_event (GstBaseSink * sink, GstEvent * event)
{
  GstPulseSink *pulsesink = GST_PULSESINK_CAST (sink);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:{
      gchar *title = NULL, *artist = NULL, *location = NULL, *description =
          NULL, *t = NULL, *buf = NULL;
      GstTagList *l;

      gst_event_parse_tag (event, &l);

      gst_tag_list_get_string (l, GST_TAG_TITLE, &title);
      gst_tag_list_get_string (l, GST_TAG_ARTIST, &artist);
      gst_tag_list_get_string (l, GST_TAG_LOCATION, &location);
      gst_tag_list_get_string (l, GST_TAG_DESCRIPTION, &description);

      if (!artist)
        gst_tag_list_get_string (l, GST_TAG_PERFORMER, &artist);

      if (title && artist)
        /* TRANSLATORS: 'song title' by 'artist name' */
        t = buf = g_strdup_printf (_("'%s' by '%s'"), g_strstrip (title),
            g_strstrip (artist));
      else if (title)
        t = g_strstrip (title);
      else if (description)
        t = g_strstrip (description);
      else if (location)
        t = g_strstrip (location);

      if (t)
        gst_pulsesink_change_title (pulsesink, t);

      g_free (title);
      g_free (artist);
      g_free (location);
      g_free (description);
      g_free (buf);

      gst_pulsesink_change_props (pulsesink, l);

      break;
    }
    case GST_EVENT_GAP:{
      GstClockTime timestamp, duration;

      gst_event_parse_gap (event, &timestamp, &duration);
      if (duration == GST_CLOCK_TIME_NONE)
        gst_pulsesink_flush_ringbuffer (pulsesink);
      break;
    }
    case GST_EVENT_EOS:
      gst_pulsesink_flush_ringbuffer (pulsesink);
      break;
    default:
      ;
  }

  return GST_BASE_SINK_CLASS (parent_class)->event (sink, event);
}

static gboolean
gst_pulsesink_query (GstBaseSink * sink, GstQuery * query)
{
  GstPulseSink *pulsesink = GST_PULSESINK_CAST (sink);
  gboolean ret = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *caps, *filter;

      gst_query_parse_caps (query, &filter);
      caps = gst_pulsesink_query_getcaps (pulsesink, filter);

      if (caps) {
        gst_query_set_caps_result (query, caps);
        gst_caps_unref (caps);
        ret = TRUE;
      }
      break;
    }
    case GST_QUERY_ACCEPT_CAPS:
    {
      GstCaps *caps;

      gst_query_parse_accept_caps (query, &caps);
      ret = gst_pulsesink_query_acceptcaps (pulsesink, caps);
      gst_query_set_accept_caps_result (query, ret);
      ret = TRUE;
      break;
    }
    default:
      ret = GST_BASE_SINK_CLASS (parent_class)->query (sink, query);
      break;
  }
  return ret;
}

static void
gst_pulsesink_release_mainloop (GstPulseSink * psink)
{
  if (!mainloop)
    return;

  pa_threaded_mainloop_lock (mainloop);
  while (psink->defer_pending) {
    GST_DEBUG_OBJECT (psink, "waiting for stream status message emission");
    pa_threaded_mainloop_wait (mainloop);
  }
  pa_threaded_mainloop_unlock (mainloop);

  g_mutex_lock (&pa_shared_resource_mutex);
  mainloop_ref_ct--;
  if (!mainloop_ref_ct) {
    GST_INFO_OBJECT (psink, "terminating pa main loop thread");
    pa_threaded_mainloop_stop (mainloop);
    pa_threaded_mainloop_free (mainloop);
    mainloop = NULL;
  }
  g_mutex_unlock (&pa_shared_resource_mutex);
}

static GstStateChangeReturn
gst_pulsesink_change_state (GstElement * element, GstStateChange transition)
{
  GstPulseSink *pulsesink = GST_PULSESINK (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      g_mutex_lock (&pa_shared_resource_mutex);
      if (!mainloop_ref_ct) {
        GST_INFO_OBJECT (element, "new pa main loop thread");
        if (!(mainloop = pa_threaded_mainloop_new ()))
          goto mainloop_failed;
        if (pa_threaded_mainloop_start (mainloop) < 0) {
          pa_threaded_mainloop_free (mainloop);
          goto mainloop_start_failed;
        }
        mainloop_ref_ct = 1;
        g_mutex_unlock (&pa_shared_resource_mutex);
      } else {
        GST_INFO_OBJECT (element, "reusing pa main loop thread");
        mainloop_ref_ct++;
        g_mutex_unlock (&pa_shared_resource_mutex);
      }
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_element_post_message (element,
          gst_message_new_clock_provide (GST_OBJECT_CAST (element),
              GST_AUDIO_BASE_SINK (pulsesink)->provided_clock, TRUE));
      break;

    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto state_failure;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      /* format_lost is reset in release() in audiobasesink */
      gst_element_post_message (element,
          gst_message_new_clock_lost (GST_OBJECT_CAST (element),
              GST_AUDIO_BASE_SINK (pulsesink)->provided_clock));
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_pulsesink_release_mainloop (pulsesink);
      break;
    default:
      break;
  }

  return ret;

  /* ERRORS */
mainloop_failed:
  {
    g_mutex_unlock (&pa_shared_resource_mutex);
    GST_ELEMENT_ERROR (pulsesink, RESOURCE, FAILED,
        ("pa_threaded_mainloop_new() failed"), (NULL));
    return GST_STATE_CHANGE_FAILURE;
  }
mainloop_start_failed:
  {
    g_mutex_unlock (&pa_shared_resource_mutex);
    GST_ELEMENT_ERROR (pulsesink, RESOURCE, FAILED,
        ("pa_threaded_mainloop_start() failed"), (NULL));
    return GST_STATE_CHANGE_FAILURE;
  }
state_failure:
  {
    if (transition == GST_STATE_CHANGE_NULL_TO_READY) {
      /* Clear the PA mainloop if audiobasesink failed to open the ring_buffer */
      g_assert (mainloop);
      gst_pulsesink_release_mainloop (pulsesink);
    }
    return ret;
  }
}
