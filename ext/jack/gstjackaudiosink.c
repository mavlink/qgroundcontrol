/* GStreamer
 * Copyright (C) 2006 Wim Taymans <wim@fluendo.com>
 *
 * gstjackaudiosink.c: jack audio sink implementation
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
 * SECTION:element-jackaudiosink
 * @title: jackaudiosink
 * @see_also: #GstAudioBaseSink, #GstAudioRingBuffer
 *
 * A Sink that outputs data to Jack ports.
 *
 * It will create N Jack ports named out_&lt;name&gt;_&lt;num&gt; where
 * &lt;name&gt; is the element name and &lt;num&gt; is starting from 1.
 * Each port corresponds to a gstreamer channel.
 *
 * The samplerate as exposed on the caps is always the same as the samplerate of
 * the jack server.
 *
 * When the #GstJackAudioSink:connect property is set to auto, this element
 * will try to connect each output port to a random physical jack input pin. In
 * this mode, the sink will expose the number of physical channels on its pad
 * caps.
 *
 * When the #GstJackAudioSink:connect property is set to none, the element will
 * accept any number of input channels and will create (but not connect) an
 * output port for each channel.
 *
 * The element will generate an error when the Jack server is shut down when it
 * was PAUSED or PLAYING. This element does not support dynamic rate and buffer
 * size changes at runtime.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc ! jackaudiosink
 * ]| Play a sine wave to using jack.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst-i18n-plugin.h>
#include <stdlib.h>
#include <string.h>
#include <gst/audio/audio.h>

#include "gstjackaudiosink.h"
#include "gstjackringbuffer.h"

GST_DEBUG_CATEGORY_STATIC (gst_jack_audio_sink_debug);
#define GST_CAT_DEFAULT gst_jack_audio_sink_debug

static gboolean
gst_jack_audio_sink_allocate_channels (GstJackAudioSink * sink, gint channels)
{
  jack_client_t *client;

  client = gst_jack_audio_client_get_client (sink->client);

  /* remove ports we don't need */
  while (sink->port_count > channels) {
    jack_port_unregister (client, sink->ports[--sink->port_count]);
  }

  /* alloc enough output ports */
  sink->ports = g_realloc (sink->ports, sizeof (jack_port_t *) * channels);
  sink->buffers = g_realloc (sink->buffers, sizeof (sample_t *) * channels);

  /* create an output port for each channel */
  while (sink->port_count < channels) {
    gchar *name;

    /* port names start from 1 and are local to the element */
    name =
        g_strdup_printf ("out_%s_%d", GST_ELEMENT_NAME (sink),
        sink->port_count + 1);
    sink->ports[sink->port_count] =
        jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsOutput, 0);
    if (sink->ports[sink->port_count] == NULL)
      return FALSE;

    sink->port_count++;

    g_free (name);
  }
  return TRUE;
}

static void
gst_jack_audio_sink_free_channels (GstJackAudioSink * sink)
{
  gint res, i = 0;
  jack_client_t *client;

  client = gst_jack_audio_client_get_client (sink->client);

  /* get rid of all ports */
  while (sink->port_count) {
    GST_LOG_OBJECT (sink, "unregister port %d", i);
    if ((res = jack_port_unregister (client, sink->ports[i++]))) {
      GST_DEBUG_OBJECT (sink, "unregister of port failed (%d)", res);
    }
    sink->port_count--;
  }
  g_free (sink->ports);
  sink->ports = NULL;
  g_free (sink->buffers);
  sink->buffers = NULL;
}

/* ringbuffer abstract base class */
static GType
gst_jack_ring_buffer_get_type (void)
{
  static volatile gsize ringbuffer_type = 0;

  if (g_once_init_enter (&ringbuffer_type)) {
    static const GTypeInfo ringbuffer_info = {
      sizeof (GstJackRingBufferClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_jack_ring_buffer_class_init,
      NULL,
      NULL,
      sizeof (GstJackRingBuffer),
      0,
      (GInstanceInitFunc) gst_jack_ring_buffer_init,
      NULL
    };
    GType tmp = g_type_register_static (GST_TYPE_AUDIO_RING_BUFFER,
        "GstJackAudioSinkRingBuffer", &ringbuffer_info, 0);
    g_once_init_leave (&ringbuffer_type, tmp);
  }

  return (GType) ringbuffer_type;
}

static void
gst_jack_ring_buffer_class_init (GstJackRingBufferClass * klass)
{
  GstAudioRingBufferClass *gstringbuffer_class;

  gstringbuffer_class = (GstAudioRingBufferClass *) klass;

  ring_parent_class = g_type_class_peek_parent (klass);

  gstringbuffer_class->open_device =
      GST_DEBUG_FUNCPTR (gst_jack_ring_buffer_open_device);
  gstringbuffer_class->close_device =
      GST_DEBUG_FUNCPTR (gst_jack_ring_buffer_close_device);
  gstringbuffer_class->acquire =
      GST_DEBUG_FUNCPTR (gst_jack_ring_buffer_acquire);
  gstringbuffer_class->release =
      GST_DEBUG_FUNCPTR (gst_jack_ring_buffer_release);
  gstringbuffer_class->start = GST_DEBUG_FUNCPTR (gst_jack_ring_buffer_start);
  gstringbuffer_class->pause = GST_DEBUG_FUNCPTR (gst_jack_ring_buffer_pause);
  gstringbuffer_class->resume = GST_DEBUG_FUNCPTR (gst_jack_ring_buffer_start);
  gstringbuffer_class->stop = GST_DEBUG_FUNCPTR (gst_jack_ring_buffer_stop);

  gstringbuffer_class->delay = GST_DEBUG_FUNCPTR (gst_jack_ring_buffer_delay);
}

/* this is the callback of jack. This should RT-safe.
 */
static int
jack_process_cb (jack_nframes_t nframes, void *arg)
{
  GstJackAudioSink *sink;
  GstAudioRingBuffer *buf;
  gint readseg, len;
  guint8 *readptr;
  gint i, j, flen, channels;
  sample_t *data;

  buf = GST_AUDIO_RING_BUFFER_CAST (arg);
  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));

  channels = GST_AUDIO_INFO_CHANNELS (&buf->spec.info);

  /* get target buffers */
  for (i = 0; i < channels; i++) {
    sink->buffers[i] =
        (sample_t *) jack_port_get_buffer (sink->ports[i], nframes);
  }

  if (gst_audio_ring_buffer_prepare_read (buf, &readseg, &readptr, &len)) {
    flen = len / channels;

    /* the number of samples must be exactly the segment size */
    if (nframes * sizeof (sample_t) != flen)
      goto wrong_size;

    GST_DEBUG_OBJECT (sink, "copy %d frames: %p, %d bytes, %d channels",
        nframes, readptr, flen, channels);
    data = (sample_t *) readptr;

    /* the samples in the ringbuffer have the channels interleaved, we need to
     * deinterleave into the jack target buffers */
    for (i = 0; i < nframes; i++) {
      for (j = 0; j < channels; j++) {
        sink->buffers[j][i] = *data++;
      }
    }

    /* clear written samples in the ringbuffer */
    gst_audio_ring_buffer_clear (buf, readseg);

    /* we wrote one segment */
    gst_audio_ring_buffer_advance (buf, 1);
  } else {
    GST_DEBUG_OBJECT (sink, "write %d frames silence", nframes);
    /* We are not allowed to read from the ringbuffer, write silence to all
     * jack output buffers */
    for (i = 0; i < channels; i++) {
      memset (sink->buffers[i], 0, nframes * sizeof (sample_t));
    }
  }
  return 0;

  /* ERRORS */
wrong_size:
  {
    GST_ERROR_OBJECT (sink, "nbytes (%d) != flen (%d)",
        (gint) (nframes * sizeof (sample_t)), flen);
    return 1;
  }
}

/* we error out */
static int
jack_sample_rate_cb (jack_nframes_t nframes, void *arg)
{
  GstJackAudioSink *sink;
  GstJackRingBuffer *abuf;

  abuf = GST_JACK_RING_BUFFER_CAST (arg);
  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (arg));

  if (abuf->sample_rate != -1 && abuf->sample_rate != nframes)
    goto not_supported;

  return 0;

  /* ERRORS */
not_supported:
  {
    GST_ELEMENT_ERROR (sink, RESOURCE, SETTINGS,
        (NULL), ("Jack changed the sample rate, which is not supported"));
    return 1;
  }
}

/* we error out */
static int
jack_buffer_size_cb (jack_nframes_t nframes, void *arg)
{
  GstJackAudioSink *sink;
  GstJackRingBuffer *abuf;

  abuf = GST_JACK_RING_BUFFER_CAST (arg);
  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (arg));

  if (abuf->buffer_size != -1 && abuf->buffer_size != nframes)
    goto not_supported;

  return 0;

  /* ERRORS */
not_supported:
  {
    GST_ELEMENT_ERROR (sink, RESOURCE, SETTINGS,
        (NULL), ("Jack changed the buffer size, which is not supported"));
    return 1;
  }
}

static void
jack_shutdown_cb (void *arg)
{
  GstJackAudioSink *sink;

  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (arg));

  GST_DEBUG_OBJECT (sink, "shutdown");

  GST_ELEMENT_ERROR (sink, RESOURCE, NOT_FOUND,
      (NULL), ("Jack server shutdown"));
}

static void
gst_jack_ring_buffer_init (GstJackRingBuffer * buf,
    GstJackRingBufferClass * g_class)
{
  buf->channels = -1;
  buf->buffer_size = -1;
  buf->sample_rate = -1;
}

/* the _open_device method should make a connection with the server
 */
static gboolean
gst_jack_ring_buffer_open_device (GstAudioRingBuffer * buf)
{
  GstJackAudioSink *sink;
  jack_status_t status = 0;
  const gchar *name;

  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));

  GST_DEBUG_OBJECT (sink, "open");

  if (sink->client_name) {
    name = sink->client_name;
  } else {
    name = g_get_application_name ();
  }
  if (!name)
    name = "GStreamer";

  sink->client = gst_jack_audio_client_new (name, sink->server,
      sink->jclient,
      GST_JACK_CLIENT_SINK,
      jack_shutdown_cb,
      jack_process_cb, jack_buffer_size_cb, jack_sample_rate_cb, buf, &status);
  if (sink->client == NULL)
    goto could_not_open;

  GST_DEBUG_OBJECT (sink, "opened");

  return TRUE;

  /* ERRORS */
could_not_open:
  {
    if (status & JackServerFailed) {
      GST_ELEMENT_ERROR (sink, RESOURCE, NOT_FOUND,
          (_("Jack server not found")),
          ("Cannot connect to the Jack server (status %d)", status));
    } else {
      GST_ELEMENT_ERROR (sink, RESOURCE, OPEN_WRITE,
          (NULL), ("Jack client open error (status %d)", status));
    }
    return FALSE;
  }
}

/* close the connection with the server
 */
static gboolean
gst_jack_ring_buffer_close_device (GstAudioRingBuffer * buf)
{
  GstJackAudioSink *sink;

  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));

  GST_DEBUG_OBJECT (sink, "close");

  gst_jack_audio_sink_free_channels (sink);
  gst_jack_audio_client_free (sink->client);
  sink->client = NULL;

  return TRUE;
}

/* allocate a buffer and setup resources to process the audio samples of
 * the format as specified in @spec.
 *
 * We allocate N jack ports, one for each channel. If we are asked to
 * automatically make a connection with physical ports, we connect as many
 * ports as there are physical ports, leaving leftover ports unconnected.
 *
 * It is assumed that samplerate and number of channels are acceptable since our
 * getcaps method will always provide correct values. If unacceptable caps are
 * received for some reason, we fail here.
 */
static gboolean
gst_jack_ring_buffer_acquire (GstAudioRingBuffer * buf,
    GstAudioRingBufferSpec * spec)
{
  GstJackAudioSink *sink;
  GstJackRingBuffer *abuf;
  const char **ports;
  gint sample_rate, buffer_size;
  gint i, rate, bpf, channels, res;
  jack_client_t *client;

  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));
  abuf = GST_JACK_RING_BUFFER_CAST (buf);

  GST_DEBUG_OBJECT (sink, "acquire");

  client = gst_jack_audio_client_get_client (sink->client);

  rate = GST_AUDIO_INFO_RATE (&spec->info);

  /* sample rate must be that of the server */
  sample_rate = jack_get_sample_rate (client);
  if (sample_rate != rate)
    goto wrong_samplerate;

  channels = GST_AUDIO_INFO_CHANNELS (&spec->info);
  bpf = GST_AUDIO_INFO_BPF (&spec->info);

  if (!gst_jack_audio_sink_allocate_channels (sink, channels))
    goto out_of_ports;

  buffer_size = jack_get_buffer_size (client);

  /* the segment size in bytes, this is large enough to hold a buffer of 32bit floats
   * for all channels  */
  spec->segsize = buffer_size * sizeof (gfloat) * channels;
  spec->latency_time = gst_util_uint64_scale (spec->segsize,
      (GST_SECOND / GST_USECOND), rate * bpf);
  /* segtotal based on buffer-time latency */
  spec->segtotal = spec->buffer_time / spec->latency_time;
  if (spec->segtotal < 2) {
    spec->segtotal = 2;
    spec->buffer_time = spec->latency_time * spec->segtotal;
  }

  GST_DEBUG_OBJECT (sink, "buffer time: %" G_GINT64_FORMAT " usec",
      spec->buffer_time);
  GST_DEBUG_OBJECT (sink, "latency time: %" G_GINT64_FORMAT " usec",
      spec->latency_time);
  GST_DEBUG_OBJECT (sink, "buffer_size %d, segsize %d, segtotal %d",
      buffer_size, spec->segsize, spec->segtotal);

  /* allocate the ringbuffer memory now */
  buf->size = spec->segtotal * spec->segsize;
  buf->memory = g_malloc0 (buf->size);

  if ((res = gst_jack_audio_client_set_active (sink->client, TRUE)))
    goto could_not_activate;

  /* if we need to automatically connect the ports, do so now. We must do this
   * after activating the client. */
  if (sink->connect == GST_JACK_CONNECT_AUTO
      || sink->connect == GST_JACK_CONNECT_AUTO_FORCED) {
    /* find all the physical input ports. A physical input port is a port
     * associated with a hardware device. Someone needs connect to a physical
     * port in order to hear something. */
    if (sink->port_pattern == NULL) {
      ports = jack_get_ports (client, NULL, NULL,
          JackPortIsPhysical | JackPortIsInput);
    } else {
      ports = jack_get_ports (client, sink->port_pattern, NULL,
          JackPortIsInput);
    }
    if (ports == NULL) {
      /* no ports? fine then we don't do anything except for posting a warning
       * message. */
      GST_ELEMENT_WARNING (sink, RESOURCE, NOT_FOUND, (NULL),
          ("No physical input ports found, leaving ports unconnected"));
      goto done;
    }

    for (i = 0; i < channels; i++) {
      /* stop when all input ports are exhausted */
      if (ports[i] == NULL) {
        /* post a warning that we could not connect all ports */
        GST_ELEMENT_WARNING (sink, RESOURCE, NOT_FOUND, (NULL),
            ("No more physical ports, leaving some ports unconnected"));
        break;
      }
      GST_DEBUG_OBJECT (sink, "try connecting to %s",
          jack_port_name (sink->ports[i]));
      /* connect the port to a physical port */
      res = jack_connect (client, jack_port_name (sink->ports[i]), ports[i]);
      if (res != 0 && res != EEXIST)
        goto cannot_connect;
    }
    jack_free (ports);
  }
done:

  abuf->sample_rate = sample_rate;
  abuf->buffer_size = buffer_size;
  abuf->channels = channels;

  return TRUE;

  /* ERRORS */
wrong_samplerate:
  {
    GST_ELEMENT_ERROR (sink, RESOURCE, SETTINGS, (NULL),
        ("Wrong samplerate, server is running at %d and we received %d",
            sample_rate, rate));
    return FALSE;
  }
out_of_ports:
  {
    GST_ELEMENT_ERROR (sink, RESOURCE, SETTINGS, (NULL),
        ("Cannot allocate more Jack ports"));
    return FALSE;
  }
could_not_activate:
  {
    GST_ELEMENT_ERROR (sink, RESOURCE, SETTINGS, (NULL),
        ("Could not activate client (%d:%s)", res, g_strerror (res)));
    return FALSE;
  }
cannot_connect:
  {
    GST_ELEMENT_ERROR (sink, RESOURCE, SETTINGS, (NULL),
        ("Could not connect output ports to physical ports (%d:%s)",
            res, g_strerror (res)));
    jack_free (ports);
    return FALSE;
  }
}

/* function is called with LOCK */
static gboolean
gst_jack_ring_buffer_release (GstAudioRingBuffer * buf)
{
  GstJackAudioSink *sink;
  GstJackRingBuffer *abuf;
  gint res;

  abuf = GST_JACK_RING_BUFFER_CAST (buf);
  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));

  GST_DEBUG_OBJECT (sink, "release");

  if ((res = gst_jack_audio_client_set_active (sink->client, FALSE))) {
    /* we only warn, this means the server is probably shut down and the client
     * is gone anyway. */
    GST_ELEMENT_WARNING (sink, RESOURCE, CLOSE, (NULL),
        ("Could not deactivate Jack client (%d)", res));
  }

  abuf->channels = -1;
  abuf->buffer_size = -1;
  abuf->sample_rate = -1;

  /* free the buffer */
  g_free (buf->memory);
  buf->memory = NULL;

  return TRUE;
}

static gboolean
gst_jack_ring_buffer_start (GstAudioRingBuffer * buf)
{
  GstJackAudioSink *sink;

  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));

  GST_DEBUG_OBJECT (sink, "start");

  if (sink->transport & GST_JACK_TRANSPORT_MASTER) {
    jack_client_t *client;

    client = gst_jack_audio_client_get_client (sink->client);
    jack_transport_start (client);
  }

  return TRUE;
}

static gboolean
gst_jack_ring_buffer_pause (GstAudioRingBuffer * buf)
{
  GstJackAudioSink *sink;

  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));

  GST_DEBUG_OBJECT (sink, "pause");

  if (sink->transport & GST_JACK_TRANSPORT_MASTER) {
    jack_client_t *client;

    client = gst_jack_audio_client_get_client (sink->client);
    jack_transport_stop (client);
  }

  return TRUE;
}

static gboolean
gst_jack_ring_buffer_stop (GstAudioRingBuffer * buf)
{
  GstJackAudioSink *sink;

  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));

  GST_DEBUG_OBJECT (sink, "stop");

  if (sink->transport & GST_JACK_TRANSPORT_MASTER) {
    jack_client_t *client;

    client = gst_jack_audio_client_get_client (sink->client);
    jack_transport_stop (client);
  }

  return TRUE;
}

#if defined (HAVE_JACK_0_120_1) || defined(HAVE_JACK_1_9_7)
static guint
gst_jack_ring_buffer_delay (GstAudioRingBuffer * buf)
{
  GstJackAudioSink *sink;
  guint i, res = 0;
  jack_latency_range_t range;

  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));

  for (i = 0; i < sink->port_count; i++) {
    jack_port_get_latency_range (sink->ports[i], JackPlaybackLatency, &range);
    if (range.max > res)
      res = range.max;
  }

  GST_LOG_OBJECT (sink, "delay %u", res);

  return res;
}
#else /* !(defined (HAVE_JACK_0_120_1) || defined(HAVE_JACK_1_9_7)) */
static guint
gst_jack_ring_buffer_delay (GstAudioRingBuffer * buf)
{
  GstJackAudioSink *sink;
  guint i, res = 0;
  guint latency;
  jack_client_t *client;

  sink = GST_JACK_AUDIO_SINK (GST_OBJECT_PARENT (buf));
  client = gst_jack_audio_client_get_client (sink->client);

  for (i = 0; i < sink->port_count; i++) {
    latency = jack_port_get_total_latency (client, sink->ports[i]);
    if (latency > res)
      res = latency;
  }

  GST_LOG_OBJECT (sink, "delay %u", res);

  return res;
}
#endif

static GstStaticPadTemplate jackaudiosink_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_JACK_FORMAT_STR ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]")
    );

/* AudioSink signals and args */
enum
{
  /* FILL ME */
  SIGNAL_LAST
};

#define DEFAULT_PROP_CONNECT 		GST_JACK_CONNECT_AUTO
#define DEFAULT_PROP_SERVER 		NULL
#define DEFAULT_PROP_CLIENT_NAME	NULL
#define DEFAULT_PROP_PORT_PATTERN      	NULL
#define DEFAULT_PROP_TRANSPORT	GST_JACK_TRANSPORT_AUTONOMOUS

enum
{
  PROP_0,
  PROP_CONNECT,
  PROP_SERVER,
  PROP_CLIENT,
  PROP_CLIENT_NAME,
  PROP_PORT_PATTERN,
  PROP_TRANSPORT,
  PROP_LAST
};

#define gst_jack_audio_sink_parent_class parent_class
G_DEFINE_TYPE (GstJackAudioSink, gst_jack_audio_sink, GST_TYPE_AUDIO_BASE_SINK);

static void gst_jack_audio_sink_dispose (GObject * object);
static void gst_jack_audio_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_jack_audio_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstCaps *gst_jack_audio_sink_getcaps (GstBaseSink * bsink,
    GstCaps * filter);
static GstAudioRingBuffer
    * gst_jack_audio_sink_create_ringbuffer (GstAudioBaseSink * sink);

static void
gst_jack_audio_sink_class_init (GstJackAudioSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstAudioBaseSinkClass *gstaudiobasesink_class;

  GST_DEBUG_CATEGORY_INIT (gst_jack_audio_sink_debug, "jacksink", 0,
      "jacksink element");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstaudiobasesink_class = (GstAudioBaseSinkClass *) klass;

  gobject_class->dispose = gst_jack_audio_sink_dispose;
  gobject_class->get_property = gst_jack_audio_sink_get_property;
  gobject_class->set_property = gst_jack_audio_sink_set_property;

  g_object_class_install_property (gobject_class, PROP_CONNECT,
      g_param_spec_enum ("connect", "Connect",
          "Specify how the output ports will be connected",
          GST_TYPE_JACK_CONNECT, DEFAULT_PROP_CONNECT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SERVER,
      g_param_spec_string ("server", "Server",
          "The Jack server to connect to (NULL = default)",
          DEFAULT_PROP_SERVER, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstJackAudioSink:client-name:
   *
   * The client name to use.
   */
  g_object_class_install_property (gobject_class, PROP_CLIENT_NAME,
      g_param_spec_string ("client-name", "Client name",
          "The client name of the Jack instance (NULL = default)",
          DEFAULT_PROP_CLIENT_NAME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CLIENT,
      g_param_spec_boxed ("client", "JackClient", "Handle for jack client",
          GST_TYPE_JACK_CLIENT,
          GST_PARAM_MUTABLE_READY | G_PARAM_READWRITE |
          G_PARAM_STATIC_STRINGS));

   /**
   * GstJackAudioSink:port-pattern
   *
   * autoconnect to ports matching pattern, when NULL connect to physical ports
   *
   * Since: 1.6
   */
  g_object_class_install_property (gobject_class, PROP_PORT_PATTERN,
      g_param_spec_string ("port-pattern", "port pattern",
          "A pattern to select which ports to connect to (NULL = first physical ports)",
          DEFAULT_PROP_PORT_PATTERN,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstJackAudioSink:transport:
   *
   * The jack transport behaviour for the client.
   */
  g_object_class_install_property (gobject_class, PROP_TRANSPORT,
      g_param_spec_flags ("transport", "Transport mode",
          "Jack transport behaviour of the client",
          GST_TYPE_JACK_TRANSPORT, DEFAULT_PROP_TRANSPORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class, "Audio Sink (Jack)",
      "Sink/Audio", "Output audio to a JACK server",
      "Wim Taymans <wim.taymans@gmail.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &jackaudiosink_sink_factory);

  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_jack_audio_sink_getcaps);

  gstaudiobasesink_class->create_ringbuffer =
      GST_DEBUG_FUNCPTR (gst_jack_audio_sink_create_ringbuffer);

  /* ref class from a thread-safe context to work around missing bit of
   * thread-safety in GObject */
  g_type_class_ref (GST_TYPE_JACK_RING_BUFFER);

  gst_jack_audio_client_init ();
}

static void
gst_jack_audio_sink_init (GstJackAudioSink * sink)
{
  sink->connect = DEFAULT_PROP_CONNECT;
  sink->server = g_strdup (DEFAULT_PROP_SERVER);
  sink->jclient = NULL;
  sink->ports = NULL;
  sink->port_count = 0;
  sink->buffers = NULL;
  sink->client_name = g_strdup (DEFAULT_PROP_CLIENT_NAME);
  sink->transport = DEFAULT_PROP_TRANSPORT;
}

static void
gst_jack_audio_sink_dispose (GObject * object)
{
  GstJackAudioSink *sink = GST_JACK_AUDIO_SINK (object);

  gst_caps_replace (&sink->caps, NULL);

  if (sink->client_name != NULL) {
    g_free (sink->client_name);
    sink->client_name = NULL;
  }

  if (sink->port_pattern != NULL) {
    g_free (sink->port_pattern);
    sink->port_pattern = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_jack_audio_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstJackAudioSink *sink;

  sink = GST_JACK_AUDIO_SINK (object);

  switch (prop_id) {
    case PROP_CLIENT_NAME:
      g_free (sink->client_name);
      sink->client_name = g_value_dup_string (value);
      break;
    case PROP_PORT_PATTERN:
      g_free (sink->port_pattern);
      sink->port_pattern = g_value_dup_string (value);
      break;
    case PROP_CONNECT:
      sink->connect = g_value_get_enum (value);
      break;
    case PROP_SERVER:
      g_free (sink->server);
      sink->server = g_value_dup_string (value);
      break;
    case PROP_CLIENT:
      if (GST_STATE (sink) == GST_STATE_NULL ||
          GST_STATE (sink) == GST_STATE_READY) {
        sink->jclient = g_value_get_boxed (value);
      }
      break;
    case PROP_TRANSPORT:
      sink->transport = g_value_get_flags (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_jack_audio_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstJackAudioSink *sink;

  sink = GST_JACK_AUDIO_SINK (object);

  switch (prop_id) {
    case PROP_CLIENT_NAME:
      g_value_set_string (value, sink->client_name);
      break;
    case PROP_PORT_PATTERN:
      g_value_set_string (value, sink->port_pattern);
      break;
    case PROP_CONNECT:
      g_value_set_enum (value, sink->connect);
      break;
    case PROP_SERVER:
      g_value_set_string (value, sink->server);
      break;
    case PROP_CLIENT:
      g_value_set_boxed (value, sink->jclient);
      break;
    case PROP_TRANSPORT:
      g_value_set_flags (value, sink->transport);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstCaps *
gst_jack_audio_sink_getcaps (GstBaseSink * bsink, GstCaps * filter)
{
  GstJackAudioSink *sink = GST_JACK_AUDIO_SINK (bsink);
  const char **ports;
  gint min, max;
  gint rate;
  jack_client_t *client;

  if (sink->client == NULL)
    goto no_client;

  client = gst_jack_audio_client_get_client (sink->client);

  if (sink->connect == GST_JACK_CONNECT_AUTO) {
    /* get a port count, this is the number of channels we can automatically
     * connect. */
    ports = jack_get_ports (client, NULL, NULL,
        JackPortIsPhysical | JackPortIsInput);
    max = 0;
    if (ports != NULL) {
      for (; ports[max]; max++);
      jack_free (ports);
    } else
      max = 0;
  } else {
    /* we allow any number of pads, something else is going to connect the
     * pads. */
    max = G_MAXINT;
  }
  min = MIN (1, max);

  rate = jack_get_sample_rate (client);

  GST_DEBUG_OBJECT (sink, "got %d-%d ports, samplerate: %d", min, max, rate);

  if (!sink->caps) {
    sink->caps = gst_caps_new_simple ("audio/x-raw",
        "format", G_TYPE_STRING, GST_JACK_FORMAT_STR,
        "layout", G_TYPE_STRING, "interleaved",
        "rate", G_TYPE_INT, rate,
        "channels", GST_TYPE_INT_RANGE, min, max, NULL);
  }
  GST_INFO_OBJECT (sink, "returning caps %" GST_PTR_FORMAT, sink->caps);

  return gst_caps_ref (sink->caps);

  /* ERRORS */
no_client:
  {
    GST_DEBUG_OBJECT (sink, "device not open, using template caps");
    /* base class will get template caps for us when we return NULL */
    return NULL;
  }
}

static GstAudioRingBuffer *
gst_jack_audio_sink_create_ringbuffer (GstAudioBaseSink * sink)
{
  GstAudioRingBuffer *buffer;

  buffer = g_object_new (GST_TYPE_JACK_RING_BUFFER, NULL);
  GST_DEBUG_OBJECT (sink, "created ringbuffer @%p", buffer);

  return buffer;
}
