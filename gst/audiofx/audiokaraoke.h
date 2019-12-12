/* 
 * GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans@gmail.com>
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

#ifndef __GST_AUDIO_KARAOKE_H__
#define __GST_AUDIO_KARAOKE_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

G_BEGIN_DECLS
#define GST_TYPE_AUDIO_KARAOKE            (gst_audio_karaoke_get_type())
#define GST_AUDIO_KARAOKE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIO_KARAOKE,GstAudioKaraoke))
#define GST_IS_AUDIO_KARAOKE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIO_KARAOKE))
#define GST_AUDIO_KARAOKE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_AUDIO_KARAOKE,GstAudioKaraokeClass))
#define GST_IS_AUDIO_KARAOKE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_AUDIO_KARAOKE))
#define GST_AUDIO_KARAOKE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_AUDIO_KARAOKE,GstAudioKaraokeClass))
typedef struct _GstAudioKaraoke GstAudioKaraoke;
typedef struct _GstAudioKaraokeClass GstAudioKaraokeClass;

typedef void (*GstAudioKaraokeProcessFunc) (GstAudioKaraoke *, guint8 *, guint);

struct _GstAudioKaraoke
{
  GstAudioFilter audiofilter;

  /* properties */
  gfloat level;
  gfloat mono_level;
  gfloat filter_band;
  gfloat filter_width;

  /* filter coef */
  gfloat A, B, C;
  gfloat y1, y2;

  /* < private > */
  GstAudioKaraokeProcessFunc process;
};

struct _GstAudioKaraokeClass
{
  GstAudioFilterClass parent;
};

GType gst_audio_karaoke_get_type (void);

G_END_DECLS
#endif /* __GST_AUDIO_KARAOKE_H__ */
