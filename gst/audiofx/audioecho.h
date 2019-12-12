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

#ifndef __GST_AUDIO_ECHO_H__
#define __GST_AUDIO_ECHO_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_AUDIO_ECHO            (gst_audio_echo_get_type())
#define GST_AUDIO_ECHO(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIO_ECHO,GstAudioEcho))
#define GST_IS_AUDIO_ECHO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIO_ECHO))
#define GST_AUDIO_ECHO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_AUDIO_ECHO,GstAudioEchoClass))
#define GST_IS_AUDIO_ECHO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_AUDIO_ECHO))
#define GST_AUDIO_ECHO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_AUDIO_ECHO,GstAudioEchoClass))
typedef struct _GstAudioEcho GstAudioEcho;
typedef struct _GstAudioEchoClass GstAudioEchoClass;

typedef void (*GstAudioEchoProcessFunc) (GstAudioEcho *, guint8 *, guint);

struct _GstAudioEcho
{
  GstAudioFilter audiofilter;

  guint64 delay;
  guint64 max_delay;
  gfloat intensity;
  gfloat feedback;
  gboolean surdelay;
  guint64 surround_mask;

  /* < private > */
  GstAudioEchoProcessFunc process;
  guint delay_frames;
  guint8 *buffer;
  guint buffer_pos;
  guint buffer_size;
  guint buffer_size_frames;

  GMutex lock;
};

struct _GstAudioEchoClass
{
  GstAudioFilterClass parent;
};

GType gst_audio_echo_get_type (void);

G_END_DECLS

#endif /* __GST_AUDIO_ECHO_H__ */
