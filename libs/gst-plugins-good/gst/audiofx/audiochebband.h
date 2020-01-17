/* 
 * GStreamer
 * Copyright (C) 2007-2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#ifndef __GST_AUDIO_CHEB_BAND_H__
#define __GST_AUDIO_CHEB_BAND_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#include "audiofxbaseiirfilter.h"

G_BEGIN_DECLS
#define GST_TYPE_AUDIO_CHEB_BAND            (gst_audio_cheb_band_get_type())
#define GST_AUDIO_CHEB_BAND(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIO_CHEB_BAND,GstAudioChebBand))
#define GST_IS_AUDIO_CHEB_BAND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIO_CHEB_BAND))
#define GST_AUDIO_CHEB_BAND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_AUDIO_CHEB_BAND,GstAudioChebBandClass))
#define GST_IS_AUDIO_CHEB_BAND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_AUDIO_CHEB_BAND))
#define GST_AUDIO_CHEB_BAND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_AUDIO_CHEB_BAND,GstAudioChebBandClass))
typedef struct _GstAudioChebBand GstAudioChebBand;
typedef struct _GstAudioChebBandClass GstAudioChebBandClass;

struct _GstAudioChebBand
{
  GstAudioFXBaseIIRFilter parent;

  gint mode;
  gint type;
  gint poles;
  gfloat lower_frequency;
  gfloat upper_frequency;
  gfloat ripple;

  /* < private > */
  GMutex lock;
};

struct _GstAudioChebBandClass
{
  GstAudioFXBaseIIRFilterClass parent;
};

GType gst_audio_cheb_band_get_type (void);

G_END_DECLS
#endif /* __GST_AUDIO_CHEB_BAND_H__ */
