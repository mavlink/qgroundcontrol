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

#ifndef __GST_AUDIO_FX_BASE_IIR_FILTER_H__
#define __GST_AUDIO_FX_BASE_IIR_FILTER_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_AUDIO_FX_BASE_IIR_FILTER            (gst_audio_fx_base_iir_filter_get_type())
#define GST_AUDIO_FX_BASE_IIR_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIO_FX_BASE_IIR_FILTER,GstAudioFXBaseIIRFilter))
#define GST_IS_AUDIO_FX_BASE_IIR_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIO_FX_BASE_IIR_FILTER))
#define GST_AUDIO_FX_BASE_IIR_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_AUDIO_FX_BASE_IIR_FILTER,GstAudioFXBaseIIRFilterClass))
#define GST_IS_AUDIO_FX_BASE_IIR_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_AUDIO_FX_BASE_IIR_FILTER))
#define GST_AUDIO_FX_BASE_IIR_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_AUDIO_FX_BASE_IIR_FILTER,GstAudioFXBaseIIRFilterClass))
typedef struct _GstAudioFXBaseIIRFilter GstAudioFXBaseIIRFilter;
typedef struct _GstAudioFXBaseIIRFilterClass GstAudioFXBaseIIRFilterClass;

typedef void (*GstAudioFXBaseIIRFilterProcessFunc) (GstAudioFXBaseIIRFilter *, guint8 *, guint);

typedef struct
{
  gdouble *x;
  gint x_pos;
  gdouble *y;
  gint y_pos;
} GstAudioFXBaseIIRFilterChannelCtx;

struct _GstAudioFXBaseIIRFilter
{
  GstAudioFilter audiofilter;

  /* < private > */
  GstAudioFXBaseIIRFilterProcessFunc process;

  gdouble *a;
  guint na;
  gdouble *b;
  guint nb;
  GstAudioFXBaseIIRFilterChannelCtx *channels;
  guint nchannels;

  GMutex lock;
};

struct _GstAudioFXBaseIIRFilterClass
{
  GstAudioFilterClass parent;
};

GType gst_audio_fx_base_iir_filter_get_type (void);
void gst_audio_fx_base_iir_filter_set_coefficients (GstAudioFXBaseIIRFilter *filter, gdouble *a, guint na, gdouble *b, guint nb);
gdouble gst_audio_fx_base_iir_filter_calculate_gain (gdouble *a, guint na, gdouble *b, guint nb, gdouble zr, gdouble zi);

G_END_DECLS

#endif /* __GST_AUDIO_FX_BASE_IIR_FILTER_H__ */
