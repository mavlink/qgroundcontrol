/*
 * GStreamer
 * Copyright (C) 2006 Wim Taymans <wim@fluendo.com>
 * Copyright (C) 2008 Tristan Matthews <tristan@sat.qc.ca>
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

#ifndef __GST_JACK_RING_BUFFER_H__
#define __GST_JACK_RING_BUFFER_H__

#define GST_TYPE_JACK_RING_BUFFER               (gst_jack_ring_buffer_get_type())
#define GST_JACK_RING_BUFFER(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_JACK_RING_BUFFER,GstJackRingBuffer))
#define GST_JACK_RING_BUFFER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_JACK_RING_BUFFER,GstJackRingBufferClass))
#define GST_JACK_RING_BUFFER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_JACK_RING_BUFFER,GstJackRingBufferClass))
#define GST_JACK_RING_BUFFER_CAST(obj)          ((GstJackRingBuffer *)obj)
#define GST_IS_JACK_RING_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_JACK_RING_BUFFER))
#define GST_IS_JACK_RING_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_JACK_RING_BUFFER))

typedef struct _GstJackRingBuffer GstJackRingBuffer;
typedef struct _GstJackRingBufferClass GstJackRingBufferClass;

struct _GstJackRingBuffer
{
  GstAudioRingBuffer object;

  gint sample_rate;
  gint buffer_size;
  gint channels;
};

struct _GstJackRingBufferClass
{
  GstAudioRingBufferClass parent_class;
};

static void gst_jack_ring_buffer_class_init(GstJackRingBufferClass * klass);
static void gst_jack_ring_buffer_init(GstJackRingBuffer * ringbuffer,
    GstJackRingBufferClass * klass);

static GstAudioRingBufferClass *ring_parent_class = NULL;

static gboolean gst_jack_ring_buffer_open_device(GstAudioRingBuffer * buf);
static gboolean gst_jack_ring_buffer_close_device(GstAudioRingBuffer * buf);
static gboolean gst_jack_ring_buffer_acquire(GstAudioRingBuffer * buf,GstAudioRingBufferSpec * spec);
static gboolean gst_jack_ring_buffer_release(GstAudioRingBuffer * buf);
static gboolean gst_jack_ring_buffer_start(GstAudioRingBuffer * buf);
static gboolean gst_jack_ring_buffer_pause(GstAudioRingBuffer * buf);
static gboolean gst_jack_ring_buffer_stop(GstAudioRingBuffer * buf);
static guint gst_jack_ring_buffer_delay(GstAudioRingBuffer * buf);

#endif 
