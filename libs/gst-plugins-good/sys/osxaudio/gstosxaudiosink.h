/*
 * GStreamer
 * Copyright (C) 2005-2006 Zaheer Abbas Merali <zaheerabbas at merali dot org>
 * Copyright (C) 2007 Pioneers of the Inevitable <songbird@songbirdnest.com>
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

#ifndef __GST_OSXAUDIOSINK_H__
#define __GST_OSXAUDIOSINK_H__

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiobasesink.h>
#include "gstosxaudioringbuffer.h"

G_BEGIN_DECLS

#define GST_OSX_AUDIO_SINK_CAPS "audio/x-raw, " \
        "format = (string) " GST_AUDIO_FORMATS_ALL ", " \
        "layout = (string) interleaved, " \
        "rate = (int) [1, MAX], " \
        "channels = (int) [1, 9];" \
        "audio/x-ac3, framed = (boolean) true;" \
        "audio/x-dts, framed = (boolean) true"

#define GST_TYPE_OSX_AUDIO_SINK \
  (gst_osx_audio_sink_get_type())
#define GST_OSX_AUDIO_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_OSX_AUDIO_SINK,GstOsxAudioSink))
#define GST_OSX_AUDIO_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_OSX_AUDIO_SINK,GstOsxAudioSinkClass))
#define GST_IS_OSX_AUDIO_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_OSX_AUDIO_SINK))

typedef struct _GstOsxAudioSink GstOsxAudioSink;
typedef struct _GstOsxAudioSinkClass GstOsxAudioSinkClass;

struct _GstOsxAudioSink
{
  GstAudioBaseSink sink;

  AudioDeviceID device_id;

  AudioUnit audiounit;
  double volume;

  guint channels;
};

struct _GstOsxAudioSinkClass
{
  GstAudioBaseSinkClass parent_class;
};

GType gst_osx_audio_sink_get_type (void);

G_END_DECLS

#endif /* __GST_OSXAUDIOSINK_H__ */

