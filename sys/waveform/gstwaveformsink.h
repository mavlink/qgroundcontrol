/* GStreamer
 * Copyright (C)  2005 Sebastien Moutte <sebastien@moutte.net>
 *
 * gstwaveformsink.h: 
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

#ifndef __GST_WAVEFORMSINK_H__
#define __GST_WAVEFORMSINK_H__

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiosink.h>

#include <windows.h>
#include <mmsystem.h>

#ifndef WAVE_FORMAT_96M08
#define WAVE_FORMAT_96M08       0x00001000       /* 96   kHz, Mono,   8-bit  */
#endif

#ifndef WAVE_FORMAT_96S08
#define WAVE_FORMAT_96S08       0x00002000       /* 96   kHz, Stereo, 8-bit  */
#endif

#ifndef WAVE_FORMAT_96M16
#define WAVE_FORMAT_96M16       0x00004000       /* 96   kHz, Mono,   16-bit */
#endif

#ifndef WAVE_FORMAT_96S16
#define WAVE_FORMAT_96S16       0x00008000       /* 96   kHz, Stereo, 16-bit */
#endif

#define ERROR_LENGTH MAXERRORLENGTH+50
#define BUFFER_COUNT 20
#define BUFFER_SIZE 8192

G_BEGIN_DECLS
#define GST_TYPE_WAVEFORM_SINK                (gst_waveform_sink_get_type())
#define GST_WAVEFORM_SINK(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_WAVEFORM_SINK,GstWaveFormSink))
#define GST_WAVEFORM_SINK_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_WAVEFORM_SINK,GstWaveFormSinkClass))
#define GST_IS_WAVEFORM_SINK(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_WAVEFORM_SINK))
#define GST_IS_WAVEFORM_SINK_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_WAVEFORM_SINK))
typedef struct _GstWaveFormSink GstWaveFormSink;
typedef struct _GstWaveFormSinkClass GstWaveFormSinkClass;

struct _GstWaveFormSink
{
  /* parent object */
  GstAudioSink sink;

  /* supported caps */
  GstCaps *cached_caps;
  
  /* handle to the waveform-audio output device */
  HWAVEOUT hwaveout;
  
  /* table of buffer headers */
  WAVEHDR *wave_buffers;

  /* critical section protecting access to the number of free buffers */
  CRITICAL_SECTION critic_wave;

  /* number of free buffers available */
  guint free_buffers_count;
  
  /* current free buffer where you have to write incoming data */
  guint write_buffer;
  
  /* size of buffers streamed to the device */
  guint buffer_size;

  /* number of buffers streamed to the device */
  guint buffer_count;

  /* total of bytes in queue before they are written to the device */
  guint bytes_in_queue;

  /* bytes per sample from setcaps used to evaluate the number samples returned by delay */
  guint bytes_per_sample;

  /* wave form error string */
  gchar error_string[ERROR_LENGTH];
};

struct _GstWaveFormSinkClass
{
  GstAudioSinkClass parent_class;
};

GType gst_waveform_sink_get_type (void);

G_END_DECLS
#endif /* __GST_WAVEFORMSINK_H__ */
