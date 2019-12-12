/* 
 * GStreamer
 * Copyright (C) 2007 Sebastian Dröge <slomo@circular-chaos.org>
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
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

#ifndef __GST_AUDIO_INVERT_H__
#define __GST_AUDIO_INVERT_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

G_BEGIN_DECLS
#define GST_TYPE_AUDIO_INVERT            (gst_audio_invert_get_type())
#define GST_AUDIO_INVERT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIO_INVERT,GstAudioInvert))
#define GST_IS_AUDIO_INVERT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIO_INVERT))
#define GST_AUDIO_INVERT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_AUDIO_INVERT,GstAudioInvertClass))
#define GST_IS_AUDIO_INVERT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_AUDIO_INVERT))
#define GST_AUDIO_INVERT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_AUDIO_INVERT,GstAudioInvertClass))
typedef struct _GstAudioInvert GstAudioInvert;
typedef struct _GstAudioInvertClass GstAudioInvertClass;

typedef void (*GstAudioInvertProcessFunc) (GstAudioInvert *, guint8 *, guint);

struct _GstAudioInvert
{
  GstAudioFilter audiofilter;

  gfloat degree;

  /* < private > */
  GstAudioInvertProcessFunc process;
};

struct _GstAudioInvertClass
{
  GstAudioFilterClass parent;
};

GType gst_audio_invert_get_type (void);

G_END_DECLS
#endif /* __GST_AUDIO_INVERT_H__ */
