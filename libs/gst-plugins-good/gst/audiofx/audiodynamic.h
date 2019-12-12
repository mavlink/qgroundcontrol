/* 
 * GStreamer
 * Copyright (C) 2007 Sebastian Dröge <slomo@circular-chaos.org>
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

#ifndef __GST_AUDIO_DYNAMIC_H__
#define __GST_AUDIO_DYNAMIC_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_AUDIO_DYNAMIC            (gst_audio_dynamic_get_type())
#define GST_AUDIO_DYNAMIC(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIO_DYNAMIC,GstAudioDynamic))
#define GST_IS_AUDIO_DYNAMIC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIO_DYNAMIC))
#define GST_AUDIO_DYNAMIC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_AUDIO_DYNAMIC,GstAudioDynamicClass))
#define GST_IS_AUDIO_DYNAMIC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_AUDIO_DYNAMIC))
#define GST_AUDIO_DYNAMIC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_AUDIO_DYNAMIC,GstAudioDynamicClass))
typedef struct _GstAudioDynamic GstAudioDynamic;
typedef struct _GstAudioDynamicClass GstAudioDynamicClass;

typedef void (*GstAudioDynamicProcessFunc) (GstAudioDynamic *, guint8 *, guint);

struct _GstAudioDynamic
{
  GstAudioFilter audiofilter;

  /* < private > */
  GstAudioDynamicProcessFunc process;
  gint characteristics;
  gint mode;
  gfloat threshold;
  gfloat ratio;
};

struct _GstAudioDynamicClass
{
  GstAudioFilterClass parent;
};

GType gst_audio_dynamic_get_type (void);

G_END_DECLS

#endif /* __GST_AUDIO_DYNAMIC_H__ */
