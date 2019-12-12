/*
 * GStreamer
 * Copyright (C) 2005,2006 Zaheer Abbas Merali <zaheerabbas at merali dot org>
 * Copyright (C) 2007,2008 Pioneers of the Inevitable <songbird@songbirdnest.com>
 * Copyright (C) 2012 Fluendo S.A. <support@fluendo.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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
 * The development of this code was made possible due to the involvement of
 * Pioneers of the Inevitable, the creators of the Songbird Music player
 *
 */

/**
 * SECTION:element-osxaudiosink
 * @title: osxaudiosink
 *
 * This element renders raw audio samples using the CoreAudio api.
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 filesrc location=sine.ogg ! oggdemux ! vorbisdec ! audioconvert ! audioresample ! osxaudiosink
 * ]| Play an Ogg/Vorbis file.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/audio-channels.h>
#include <gst/audio/gstaudioiec61937.h>

#include "gstosxaudiosink.h"
#include "gstosxaudioelement.h"

GST_DEBUG_CATEGORY_STATIC (osx_audiosink_debug);
#define GST_CAT_DEFAULT osx_audiosink_debug

#include "gstosxcoreaudio.h"

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  ARG_0,
  ARG_DEVICE,
  ARG_VOLUME
};

#define DEFAULT_VOLUME 1.0

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_OSX_AUDIO_SINK_CAPS)
    );

static void gst_osx_audio_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_osx_audio_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn
gst_osx_audio_sink_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_osx_audio_sink_query (GstBaseSink * base, GstQuery * query);

static GstCaps *gst_osx_audio_sink_getcaps (GstBaseSink * base,
    GstCaps * filter);
static gboolean gst_osx_audio_sink_acceptcaps (GstOsxAudioSink * sink,
    GstCaps * caps);

static GstBuffer *gst_osx_audio_sink_sink_payload (GstAudioBaseSink * sink,
    GstBuffer * buf);
static GstAudioRingBuffer
    * gst_osx_audio_sink_create_ringbuffer (GstAudioBaseSink * sink);
static void gst_osx_audio_sink_osxelement_init (gpointer g_iface,
    gpointer iface_data);
static void gst_osx_audio_sink_set_volume (GstOsxAudioSink * sink);

static OSStatus gst_osx_audio_sink_io_proc (GstOsxAudioRingBuffer * buf,
    AudioUnitRenderActionFlags * ioActionFlags,
    const AudioTimeStamp * inTimeStamp,
    UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList * bufferList);

static void
gst_osx_audio_sink_do_init (GType type)
{
  static const GInterfaceInfo osxelement_info = {
    gst_osx_audio_sink_osxelement_init,
    NULL,
    NULL
  };

  GST_DEBUG_CATEGORY_INIT (osx_audiosink_debug, "osxaudiosink", 0,
      "OSX Audio Sink");
  gst_core_audio_init_debug ();
  GST_DEBUG ("Adding static interface");
  g_type_add_interface_static (type, GST_OSX_AUDIO_ELEMENT_TYPE,
      &osxelement_info);
}

#define gst_osx_audio_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstOsxAudioSink, gst_osx_audio_sink,
    GST_TYPE_AUDIO_BASE_SINK, gst_osx_audio_sink_do_init (g_define_type_id));

static void
gst_osx_audio_sink_class_init (GstOsxAudioSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstAudioBaseSinkClass *gstaudiobasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstaudiobasesink_class = (GstAudioBaseSinkClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = gst_osx_audio_sink_set_property;
  gobject_class->get_property = gst_osx_audio_sink_get_property;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_osx_audio_sink_change_state);

#ifndef HAVE_IOS
  g_object_class_install_property (gobject_class, ARG_DEVICE,
      g_param_spec_int ("device", "Device ID", "Device ID of output device",
          0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif

  gstbasesink_class->query = GST_DEBUG_FUNCPTR (gst_osx_audio_sink_query);

  g_object_class_install_property (gobject_class, ARG_VOLUME,
      g_param_spec_double ("volume", "Volume", "Volume of this stream",
          0, 1.0, 1.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_osx_audio_sink_getcaps);

  gstaudiobasesink_class->create_ringbuffer =
      GST_DEBUG_FUNCPTR (gst_osx_audio_sink_create_ringbuffer);
  gstaudiobasesink_class->payload =
      GST_DEBUG_FUNCPTR (gst_osx_audio_sink_sink_payload);

  gst_element_class_add_static_pad_template (gstelement_class, &sink_factory);

  gst_element_class_set_static_metadata (gstelement_class, "Audio Sink (OSX)",
      "Sink/Audio",
      "Output to a sound card in OS X",
      "Zaheer Abbas Merali <zaheerabbas at merali dot org>");
}

static void
gst_osx_audio_sink_init (GstOsxAudioSink * sink)
{
  GST_DEBUG ("Initialising object");

  sink->device_id = kAudioDeviceUnknown;
  sink->volume = DEFAULT_VOLUME;
}

static void
gst_osx_audio_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOsxAudioSink *sink = GST_OSX_AUDIO_SINK (object);

  switch (prop_id) {
#ifndef HAVE_IOS
    case ARG_DEVICE:
      sink->device_id = g_value_get_int (value);
      break;
#endif
    case ARG_VOLUME:
      sink->volume = g_value_get_double (value);
      gst_osx_audio_sink_set_volume (sink);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_osx_audio_sink_change_state (GstElement * element,
    GstStateChange transition)
{
  GstOsxAudioSink *osxsink = GST_OSX_AUDIO_SINK (element);
  GstOsxAudioRingBuffer *ringbuffer;
  GstStateChangeReturn ret;

  switch (transition) {
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto out;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      /* Device has been selected, AudioUnit set up, so initialize volume */
      gst_osx_audio_sink_set_volume (osxsink);

      /* The device is open now, so fix our device_id if it changed */
      ringbuffer =
          GST_OSX_AUDIO_RING_BUFFER (GST_AUDIO_BASE_SINK (osxsink)->ringbuffer);
      if (ringbuffer->core_audio->device_id != osxsink->device_id) {
        osxsink->device_id = ringbuffer->core_audio->device_id;
        g_object_notify (G_OBJECT (osxsink), "device");
      }
      break;

    default:
      break;
  }

out:
  return ret;
}

static void
gst_osx_audio_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstOsxAudioSink *sink = GST_OSX_AUDIO_SINK (object);
  switch (prop_id) {
#ifndef HAVE_IOS
    case ARG_DEVICE:
      g_value_set_int (value, sink->device_id);
      break;
#endif
    case ARG_VOLUME:
      g_value_set_double (value, sink->volume);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_osx_audio_sink_query (GstBaseSink * base, GstQuery * query)
{
  GstOsxAudioSink *sink = GST_OSX_AUDIO_SINK (base);
  gboolean ret = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_ACCEPT_CAPS:
    {
      GstCaps *caps = NULL;

      gst_query_parse_accept_caps (query, &caps);
      ret = gst_osx_audio_sink_acceptcaps (sink, caps);
      gst_query_set_accept_caps_result (query, ret);
      ret = TRUE;
      break;
    }
    default:
      ret = GST_BASE_SINK_CLASS (parent_class)->query (base, query);
      break;
  }
  return ret;
}

static GstCaps *
gst_osx_audio_sink_getcaps (GstBaseSink * sink, GstCaps * filter)
{
  GstOsxAudioSink *osxsink;
  GstAudioRingBuffer *buf;
  GstOsxAudioRingBuffer *osxbuf;
  GstCaps *caps, *filtered_caps;

  osxsink = GST_OSX_AUDIO_SINK (sink);

  GST_OBJECT_LOCK (osxsink);
  buf = GST_AUDIO_BASE_SINK (sink)->ringbuffer;
  if (buf)
    gst_object_ref (buf);
  GST_OBJECT_UNLOCK (osxsink);

  if (!buf) {
    GST_DEBUG_OBJECT (sink, "no ring buffer, returning NULL caps");
    return GST_BASE_SINK_CLASS (parent_class)->get_caps (sink, filter);
  }

  osxbuf = GST_OSX_AUDIO_RING_BUFFER (buf);

  /* protect against cached_caps going away */
  GST_OBJECT_LOCK (buf);

  if (osxbuf->core_audio->cached_caps_valid) {
    GST_LOG_OBJECT (sink, "Returning cached caps");
    caps = gst_caps_ref (osxbuf->core_audio->cached_caps);
  } else if (buf->open) {
    GstCaps *template_caps;

    /* Get template caps */
    template_caps =
        gst_pad_get_pad_template_caps (GST_AUDIO_BASE_SINK_PAD (osxsink));

    /* Device is open, let's probe its caps */
    caps = gst_core_audio_probe_caps (osxbuf->core_audio, template_caps);
    gst_caps_replace (&osxbuf->core_audio->cached_caps, caps);

    gst_caps_unref (template_caps);
  } else {
    GST_DEBUG_OBJECT (sink, "ring buffer not open, returning NULL caps");
    caps = NULL;
  }

  GST_OBJECT_UNLOCK (buf);

  gst_object_unref (buf);

  if (!caps)
    return NULL;

  if (!filter)
    return caps;

  /* Take care of filtered caps */
  filtered_caps =
      gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
  gst_caps_unref (caps);
  return filtered_caps;
}

static gboolean
gst_osx_audio_sink_acceptcaps (GstOsxAudioSink * sink, GstCaps * caps)
{
  GstCaps *pad_caps;
  GstStructure *st;
  gboolean ret = FALSE;
  GstAudioRingBufferSpec spec = { 0 };
  gchar *caps_string = NULL;

  caps_string = gst_caps_to_string (caps);
  GST_DEBUG_OBJECT (sink, "acceptcaps called with %s", caps_string);
  g_free (caps_string);

  pad_caps = gst_pad_query_caps (GST_BASE_SINK_PAD (sink), caps);
  if (pad_caps) {
    gboolean cret = gst_caps_can_intersect (pad_caps, caps);
    gst_caps_unref (pad_caps);
    if (!cret)
      goto done;
  }

  /* If we've not got fixed caps, creating a stream might fail,
   * so let's just return from here with default acceptcaps
   * behaviour */
  if (!gst_caps_is_fixed (caps))
    goto done;

  /* parse helper expects this set, so avoid nasty warning
   * will be set properly later on anyway  */
  spec.latency_time = GST_SECOND;
  if (!gst_audio_ring_buffer_parse_caps (&spec, caps))
    goto done;

  /* Make sure input is framed and can be payloaded */
  switch (spec.type) {
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_AC3:
    {
      gboolean framed = FALSE;

      st = gst_caps_get_structure (caps, 0);

      gst_structure_get_boolean (st, "framed", &framed);
      if (!framed || gst_audio_iec61937_frame_size (&spec) <= 0)
        goto done;
      break;
    }
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_DTS:
    {
      gboolean parsed = FALSE;

      st = gst_caps_get_structure (caps, 0);

      gst_structure_get_boolean (st, "parsed", &parsed);
      if (!parsed || gst_audio_iec61937_frame_size (&spec) <= 0)
        goto done;
      break;
    }
    default:
      break;
  }
  ret = TRUE;

done:
  return ret;
}

static GstBuffer *
gst_osx_audio_sink_sink_payload (GstAudioBaseSink * sink, GstBuffer * buf)
{
  if (RINGBUFFER_IS_SPDIF (sink->ringbuffer->spec.type)) {
    gint framesize = gst_audio_iec61937_frame_size (&sink->ringbuffer->spec);
    GstBuffer *out;
    GstMapInfo inmap, outmap;
    gboolean res;

    if (framesize <= 0)
      return NULL;

    out = gst_buffer_new_and_alloc (framesize);

    gst_buffer_map (buf, &inmap, GST_MAP_READ);
    gst_buffer_map (out, &outmap, GST_MAP_WRITE);

    /* FIXME: the endianness needs to be queried and then set */
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

  } else {
    return gst_buffer_ref (buf);
  }
}

static GstAudioRingBuffer *
gst_osx_audio_sink_create_ringbuffer (GstAudioBaseSink * sink)
{
  GstOsxAudioSink *osxsink;
  GstOsxAudioRingBuffer *ringbuffer;

  osxsink = GST_OSX_AUDIO_SINK (sink);

  GST_DEBUG_OBJECT (sink, "Creating ringbuffer");
  ringbuffer = g_object_new (GST_TYPE_OSX_AUDIO_RING_BUFFER, NULL);
  GST_DEBUG_OBJECT (sink, "osx sink %p element %p  ioproc %p", osxsink,
      GST_OSX_AUDIO_ELEMENT_GET_INTERFACE (osxsink),
      (void *) gst_osx_audio_sink_io_proc);

  ringbuffer->core_audio->element =
      GST_OSX_AUDIO_ELEMENT_GET_INTERFACE (osxsink);
  ringbuffer->core_audio->is_src = FALSE;

  /* By default the coreaudio instance created by the ringbuffer
   * has device_id==kAudioDeviceUnknown. The user might have
   * selected a different one here
   */
  if (ringbuffer->core_audio->device_id != osxsink->device_id)
    ringbuffer->core_audio->device_id = osxsink->device_id;

  return GST_AUDIO_RING_BUFFER (ringbuffer);
}

/* HALOutput AudioUnit will request fairly arbitrarily-sized chunks
 * of data, not of a fixed size. So, we keep track of where in
 * the current ringbuffer segment we are, and only advance the segment
 * once we've read the whole thing */
static OSStatus
gst_osx_audio_sink_io_proc (GstOsxAudioRingBuffer * buf,
    AudioUnitRenderActionFlags * ioActionFlags,
    const AudioTimeStamp * inTimeStamp,
    UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList * bufferList)
{
  guint8 *readptr;
  gint readseg;
  gint len;
  gint stream_idx = buf->core_audio->stream_idx;
  gint remaining = bufferList->mBuffers[stream_idx].mDataByteSize;
  gint offset = 0;

  while (remaining) {
    if (!gst_audio_ring_buffer_prepare_read (GST_AUDIO_RING_BUFFER (buf),
            &readseg, &readptr, &len))
      return 0;

    len -= buf->segoffset;

    if (len > remaining)
      len = remaining;

    memcpy ((char *) bufferList->mBuffers[stream_idx].mData + offset,
        readptr + buf->segoffset, len);

    buf->segoffset += len;
    offset += len;
    remaining -= len;

    if ((gint) buf->segoffset == GST_AUDIO_RING_BUFFER (buf)->spec.segsize) {
      /* clear written samples */
      gst_audio_ring_buffer_clear (GST_AUDIO_RING_BUFFER (buf), readseg);

      /* we wrote one segment */
      gst_audio_ring_buffer_advance (GST_AUDIO_RING_BUFFER (buf), 1);

      buf->segoffset = 0;
    }
  }
  return 0;
}

static void
gst_osx_audio_sink_osxelement_init (gpointer g_iface, gpointer iface_data)
{
  GstOsxAudioElementInterface *iface = (GstOsxAudioElementInterface *) g_iface;

  iface->io_proc = (AURenderCallback) gst_osx_audio_sink_io_proc;
}

static void
gst_osx_audio_sink_set_volume (GstOsxAudioSink * sink)
{
  GstOsxAudioRingBuffer *osxbuf;

  osxbuf = GST_OSX_AUDIO_RING_BUFFER (GST_AUDIO_BASE_SINK (sink)->ringbuffer);
  if (!osxbuf)
    return;

  gst_core_audio_set_volume (osxbuf->core_audio, sink->volume);
}
