/* GStreamer Speex Encoder
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_SPEEX_ENC_H__
#define __GST_SPEEX_ENC_H__


#include <gst/gst.h>
#include <gst/audio/gstaudioencoder.h>

#include <speex/speex.h>
#include <speex/speex_header.h>

G_BEGIN_DECLS

#define GST_TYPE_SPEEX_ENC \
  (gst_speex_enc_get_type())
#define GST_SPEEX_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SPEEX_ENC,GstSpeexEnc))
#define GST_SPEEX_ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SPEEX_ENC,GstSpeexEncClass))
#define GST_IS_SPEEX_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SPEEX_ENC))
#define GST_IS_SPEEX_ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SPEEX_ENC))

typedef enum
{
  GST_SPEEX_ENC_MODE_AUTO,
  GST_SPEEX_ENC_MODE_UWB,
  GST_SPEEX_ENC_MODE_WB,
  GST_SPEEX_ENC_MODE_NB
} GstSpeexMode;

typedef struct _GstSpeexEnc GstSpeexEnc;
typedef struct _GstSpeexEncClass GstSpeexEncClass;

struct _GstSpeexEnc {
  GstAudioEncoder   element;

  SpeexBits             bits;
  SpeexHeader           header;
  const SpeexMode       *speex_mode;
  void                  *state;

  /* properties */
  GstSpeexMode          mode;
  gfloat                quality;
  gint                  bitrate;
  gboolean              vbr;
  gint                  abr;
  gboolean              vad;
  gboolean              dtx;
  gint                  complexity;
  gint                  nframes;
  gchar                 *last_message;

  gint                  channels;
  gint                  rate;

  gboolean              header_sent;
  guint64               encoded_samples;

  GstTagList            *tags;

  gint                  frame_size;
  gint                  lookahead;

  guint8                *comments;
  gint                  comment_len;
};

struct _GstSpeexEncClass {
  GstAudioEncoderClass parent_class;
};

GType gst_speex_enc_get_type (void);

G_END_DECLS

#endif /* __GST_SPEEXENC_H__ */
