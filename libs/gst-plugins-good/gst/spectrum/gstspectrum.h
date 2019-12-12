/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
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


#ifndef __GST_SPECTRUM_H__
#define __GST_SPECTRUM_H__

#include <gst/gst.h>
#include <gst/audio/gstaudiofilter.h>
#include <gst/fft/gstfftf32.h>

G_BEGIN_DECLS

#define GST_TYPE_SPECTRUM            (gst_spectrum_get_type())
#define GST_SPECTRUM(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SPECTRUM,GstSpectrum))
#define GST_IS_SPECTRUM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SPECTRUM))
#define GST_SPECTRUM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SPECTRUM,GstSpectrumClass))
#define GST_IS_SPECTRUM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SPECTRUM))
typedef struct _GstSpectrum GstSpectrum;
typedef struct _GstSpectrumClass GstSpectrumClass;
typedef struct _GstSpectrumChannel GstSpectrumChannel;

typedef void (*GstSpectrumInputData)(const guint8 * in, gfloat * out,
    guint len, guint channels, gfloat max_value, guint op, guint nfft);

struct _GstSpectrumChannel
{
  gfloat *input;
  gfloat *input_tmp;
  GstFFTF32Complex *freqdata;
  gfloat *spect_magnitude;      /* accumulated mangitude and phase */
  gfloat *spect_phase;          /* will be scaled by num_fft before sending */
  GstFFTF32 *fft_ctx;
};

struct _GstSpectrum
{
  GstAudioFilter parent;

  /* properties */
  gboolean post_messages;       /* whether or not to post messages */
  gboolean message_magnitude;
  gboolean message_phase;
  guint64 interval;             /* how many nanoseconds between emits */
  guint64 frames_per_interval;  /* how many frames per interval */
  guint64 frames_todo;
  guint bands;                  /* number of spectrum bands */
  gint threshold;               /* energy level threshold */
  gboolean multi_channel;       /* send separate channel results */

  guint64 num_frames;           /* frame count (1 sample per channel)
                                 * since last emit */
  guint64 num_fft;              /* number of FFTs since last emit */
  GstClockTime message_ts;      /* starttime for next message */

  /* <private> */
  GstSpectrumChannel *channel_data;
  guint num_channels;

  guint input_pos;
  guint64 error_per_interval;
  guint64 accumulated_error;

  GMutex lock;

  GstSpectrumInputData input_data;
};

struct _GstSpectrumClass
{
  GstAudioFilterClass parent_class;
};

GType gst_spectrum_get_type (void);

G_END_DECLS

#endif /* __GST_SPECTRUM_H__ */
