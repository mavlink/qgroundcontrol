/*
 * GStreamer
 * Copyright (C) 2006 Zaheer Abbas Merali <zaheerabbas at merali dot org>
 * Copyright (C) 2007 Pioneers of the Inevitable <songbird@songbirdnest.com>
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

#ifndef __GST_OSX_AUDIO_ELEMENT_H__
#define __GST_OSX_AUDIO_ELEMENT_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#ifdef HAVE_IOS
#include <CoreAudio/CoreAudioTypes.h>
#else
#include <CoreAudio/CoreAudio.h>
#endif
#include <AudioUnit/AudioUnit.h>

G_BEGIN_DECLS

#define GST_OSX_AUDIO_ELEMENT_TYPE \
  (gst_osx_audio_element_get_type())
#define GST_OSX_AUDIO_ELEMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_OSX_AUDIO_ELEMENT_TYPE,GstOsxAudioElementInterface))
#define GST_IS_OSX_AUDIO_ELEMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_OSX_AUDIO_ELEMENT_TYPE))
#define GST_OSX_AUDIO_ELEMENT_GET_INTERFACE(inst) \
  (G_TYPE_INSTANCE_GET_INTERFACE((inst),GST_OSX_AUDIO_ELEMENT_TYPE,GstOsxAudioElementInterface))

typedef struct _GstOsxAudioElementInterface GstOsxAudioElementInterface;

struct _GstOsxAudioElementInterface
{
  GTypeInterface parent;

  OSStatus (*io_proc) (void * userdata,
      AudioUnitRenderActionFlags * ioActionFlags,
      const AudioTimeStamp * inTimeStamp,
      UInt32 inBusNumber, UInt32 inNumberFrames,
      AudioBufferList * bufferList);
};

GType gst_osx_audio_element_get_type (void);

G_END_DECLS

#endif /* __GST_OSX_AUDIO_ELEMENT_H__ */
