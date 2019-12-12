/* GStreamer
 * Copyright (C) 2002, Iain Holmes <iain@prettypeople.org>
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


#ifndef __GST_WAV_ENC_H__
#define __GST_WAV_ENC_H__


#include <gst/gst.h>
#include <gst/audio/audio.h>

G_BEGIN_DECLS

#define GST_TYPE_WAVENC \
  (gst_wavenc_get_type())
#define GST_WAVENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_WAVENC,GstWavEnc))
#define GST_WAVENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_WAVENC,GstWavEncClass))
#define GST_IS_WAVENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_WAVENC))
#define GST_IS_WAVENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_WAVENC))

typedef struct _GstWavEnc GstWavEnc;
typedef struct _GstWavEncClass GstWavEncClass;

struct _GstWavEnc {
  GstElement element;

  GstPad    *sinkpad;
  GstPad    *srcpad;

  GstTagList *tags;
  GstToc    *toc;
  GList     *cues;
  GList     *labls;
  GList     *notes;

  /* useful audio data */
  GstAudioFormat audio_format;
  guint16    format;
  guint      width;
  guint      rate;
  guint      channels;
  guint64    channel_mask;
  GstAudioChannelPosition srcPos[64];
  GstAudioChannelPosition destPos[64];
  
  /* data sizes */
  guint64    audio_length;
  guint32    meta_length;

  gboolean   use_rf64;
  gboolean   sent_header;
  gboolean   finished_properly;
};

struct _GstWavEncClass {
  GstElementClass parent_class;
};

GType gst_wavenc_get_type (void);

G_END_DECLS

#endif /* __GST_WAV_ENC_H__ */
