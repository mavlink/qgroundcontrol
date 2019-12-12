/*
 * GStreamer
 * Copyright (C) 2005,2006 Zaheer Abbas Merali <zaheerabbas at merali dot org>
 * Copyright (C) 2008 Pioneers of the Inevitable <songbird@songbirdnest.com>
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
 */

/**
 * SECTION:element-osxaudiosrc
 * @title: osxaudiosrc
 *
 * This element captures raw audio samples using the CoreAudio api.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 osxaudiosrc ! wavenc ! filesink location=audio.wav
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include "gstosxaudiosrc.h"
#include "gstosxaudioelement.h"

GST_DEBUG_CATEGORY_STATIC (osx_audiosrc_debug);
#define GST_CAT_DEFAULT osx_audiosrc_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  ARG_0,
  ARG_DEVICE
};

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_OSX_AUDIO_SRC_CAPS)
    );

static void gst_osx_audio_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_osx_audio_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn
gst_osx_audio_src_change_state (GstElement * element,
    GstStateChange transition);

static GstCaps *gst_osx_audio_src_get_caps (GstBaseSrc * src, GstCaps * filter);

static GstAudioRingBuffer *gst_osx_audio_src_create_ringbuffer (GstAudioBaseSrc
    * src);
static void gst_osx_audio_src_osxelement_init (gpointer g_iface,
    gpointer iface_data);
static OSStatus gst_osx_audio_src_io_proc (GstOsxAudioRingBuffer * buf,
    AudioUnitRenderActionFlags * ioActionFlags,
    const AudioTimeStamp * inTimeStamp, UInt32 inBusNumber,
    UInt32 inNumberFrames, AudioBufferList * bufferList);

static void
gst_osx_audio_src_do_init (GType type)
{
  static const GInterfaceInfo osxelement_info = {
    gst_osx_audio_src_osxelement_init,
    NULL,
    NULL
  };

  GST_DEBUG_CATEGORY_INIT (osx_audiosrc_debug, "osxaudiosrc", 0,
      "OSX Audio Src");
  g_type_add_interface_static (type, GST_OSX_AUDIO_ELEMENT_TYPE,
      &osxelement_info);
}

#define gst_osx_audio_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstOsxAudioSrc, gst_osx_audio_src,
    GST_TYPE_AUDIO_BASE_SRC, gst_osx_audio_src_do_init (g_define_type_id));

static void
gst_osx_audio_src_class_init (GstOsxAudioSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstAudioBaseSrcClass *gstaudiobasesrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstaudiobasesrc_class = (GstAudioBaseSrcClass *) klass;

  gobject_class->set_property = gst_osx_audio_src_set_property;
  gobject_class->get_property = gst_osx_audio_src_get_property;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_osx_audio_src_change_state);

  gstbasesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_osx_audio_src_get_caps);

  g_object_class_install_property (gobject_class, ARG_DEVICE,
      g_param_spec_int ("device", "Device ID", "Device ID of input device",
          0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstaudiobasesrc_class->create_ringbuffer =
      GST_DEBUG_FUNCPTR (gst_osx_audio_src_create_ringbuffer);

  gst_element_class_add_static_pad_template (gstelement_class, &src_factory);

  gst_element_class_set_static_metadata (gstelement_class, "Audio Source (OSX)",
      "Source/Audio",
      "Input from a sound card in OS X",
      "Zaheer Abbas Merali <zaheerabbas at merali dot org>");
}

static void
gst_osx_audio_src_init (GstOsxAudioSrc * src)
{
  gst_base_src_set_live (GST_BASE_SRC (src), TRUE);

  src->device_id = kAudioDeviceUnknown;
}

static void
gst_osx_audio_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOsxAudioSrc *src = GST_OSX_AUDIO_SRC (object);

  switch (prop_id) {
    case ARG_DEVICE:
      src->device_id = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_osx_audio_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstOsxAudioSrc *src = GST_OSX_AUDIO_SRC (object);

  switch (prop_id) {
    case ARG_DEVICE:
      g_value_set_int (value, src->device_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_osx_audio_src_change_state (GstElement * element, GstStateChange transition)
{
  GstOsxAudioSrc *osxsrc = GST_OSX_AUDIO_SRC (element);
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
      /* The device is open now, so fix our device_id if it changed */
      ringbuffer =
          GST_OSX_AUDIO_RING_BUFFER (GST_AUDIO_BASE_SRC (osxsrc)->ringbuffer);
      if (ringbuffer->core_audio->device_id != osxsrc->device_id) {
        osxsrc->device_id = ringbuffer->core_audio->device_id;
        g_object_notify (G_OBJECT (osxsrc), "device");
      }
      break;

    default:
      break;
  }

out:
  return ret;
}

static GstCaps *
gst_osx_audio_src_get_caps (GstBaseSrc * src, GstCaps * filter)
{
  GstOsxAudioSrc *osxsrc;
  GstAudioRingBuffer *buf;
  GstOsxAudioRingBuffer *osxbuf;
  GstCaps *caps, *filtered_caps;

  osxsrc = GST_OSX_AUDIO_SRC (src);

  GST_OBJECT_LOCK (osxsrc);
  buf = GST_AUDIO_BASE_SRC (src)->ringbuffer;
  if (buf)
    gst_object_ref (buf);
  GST_OBJECT_UNLOCK (osxsrc);

  if (!buf) {
    GST_DEBUG_OBJECT (src, "no ring buffer, using template caps");
    return GST_BASE_SRC_CLASS (parent_class)->get_caps (src, filter);
  }

  osxbuf = GST_OSX_AUDIO_RING_BUFFER (buf);

  /* protect against cached_caps going away */
  GST_OBJECT_LOCK (buf);

  if (osxbuf->core_audio->cached_caps_valid) {
    GST_LOG_OBJECT (src, "Returning cached caps");
    caps = gst_caps_ref (osxbuf->core_audio->cached_caps);
  } else if (buf->open) {
    GstCaps *template_caps;

    /* Get template caps */
    template_caps =
        gst_pad_get_pad_template_caps (GST_AUDIO_BASE_SRC_PAD (osxsrc));

    /* Device is open, let's probe its caps */
    caps = gst_core_audio_probe_caps (osxbuf->core_audio, template_caps);
    gst_caps_replace (&osxbuf->core_audio->cached_caps, caps);

    gst_caps_unref (template_caps);
  } else {
    GST_DEBUG_OBJECT (src, "ring buffer not open, using template caps");
    caps = GST_BASE_SRC_CLASS (parent_class)->get_caps (src, NULL);
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

static GstAudioRingBuffer *
gst_osx_audio_src_create_ringbuffer (GstAudioBaseSrc * src)
{
  GstOsxAudioSrc *osxsrc;
  GstOsxAudioRingBuffer *ringbuffer;

  osxsrc = GST_OSX_AUDIO_SRC (src);

  GST_DEBUG_OBJECT (osxsrc, "Creating ringbuffer");
  ringbuffer = g_object_new (GST_TYPE_OSX_AUDIO_RING_BUFFER, NULL);
  GST_DEBUG_OBJECT (osxsrc, "osx src 0x%p element 0x%p  ioproc 0x%p", osxsrc,
      GST_OSX_AUDIO_ELEMENT_GET_INTERFACE (osxsrc),
      (void *) gst_osx_audio_src_io_proc);

  ringbuffer->core_audio->element =
      GST_OSX_AUDIO_ELEMENT_GET_INTERFACE (osxsrc);
  ringbuffer->core_audio->is_src = TRUE;

  /* By default the coreaudio instance created by the ringbuffer
   * has device_id==kAudioDeviceUnknown. The user might have
   * selected a different one here
   */
  if (ringbuffer->core_audio->device_id != osxsrc->device_id)
    ringbuffer->core_audio->device_id = osxsrc->device_id;

  return GST_AUDIO_RING_BUFFER (ringbuffer);
}

static OSStatus
gst_osx_audio_src_io_proc (GstOsxAudioRingBuffer * buf,
    AudioUnitRenderActionFlags * ioActionFlags,
    const AudioTimeStamp * inTimeStamp,
    UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList * bufferList)
{
  OSStatus status;
  guint8 *writeptr;
  gint writeseg;
  gint len;
  gint remaining;
  UInt32 n;
  gint offset = 0;

  /* Previous invoke of AudioUnitRender changed mDataByteSize into
   * number of bytes actually read. Reset the members. */
  for (n = 0; n < buf->core_audio->recBufferList->mNumberBuffers; ++n) {
    buf->core_audio->recBufferList->mBuffers[n].mDataByteSize =
        buf->core_audio->recBufferSize;
  }

  status = AudioUnitRender (buf->core_audio->audiounit, ioActionFlags,
      inTimeStamp, inBusNumber, inNumberFrames, buf->core_audio->recBufferList);

  if (status) {
    GST_WARNING_OBJECT (buf, "AudioUnitRender returned %d", (int) status);
    return status;
  }

  /* TODO: To support non-interleaved audio, go over all mBuffers,
   *       not just the first one. */

  remaining = buf->core_audio->recBufferList->mBuffers[0].mDataByteSize;

  while (remaining) {
    if (!gst_audio_ring_buffer_prepare_read (GST_AUDIO_RING_BUFFER (buf),
            &writeseg, &writeptr, &len))
      return 0;

    len -= buf->segoffset;

    if (len > remaining)
      len = remaining;

    memcpy (writeptr + buf->segoffset,
        (char *) buf->core_audio->recBufferList->mBuffers[0].mData + offset,
        len);

    buf->segoffset += len;
    offset += len;
    remaining -= len;

    if ((gint) buf->segoffset == GST_AUDIO_RING_BUFFER (buf)->spec.segsize) {
      /* we wrote one segment */
      gst_audio_ring_buffer_advance (GST_AUDIO_RING_BUFFER (buf), 1);

      buf->segoffset = 0;
    }
  }
  return 0;
}

static void
gst_osx_audio_src_osxelement_init (gpointer g_iface, gpointer iface_data)
{
  GstOsxAudioElementInterface *iface = (GstOsxAudioElementInterface *) g_iface;

  iface->io_proc = (AURenderCallback) gst_osx_audio_src_io_proc;
}
