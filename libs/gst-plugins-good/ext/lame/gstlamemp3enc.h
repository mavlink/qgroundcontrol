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


#ifndef __GST_LAMEMP3ENC_H__
#define __GST_LAMEMP3ENC_H__


#include <gst/gst.h>
#include <gst/audio/gstaudioencoder.h>
#include <gst/base/gstadapter.h>

G_BEGIN_DECLS

#include <lame/lame.h>

#define GST_TYPE_LAMEMP3ENC \
  (gst_lamemp3enc_get_type())
#define GST_LAMEMP3ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_LAMEMP3ENC,GstLameMP3Enc))
#define GST_LAMEMP3ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_LAMEMP3ENC,GstLameMP3EncClass))
#define GST_IS_LAMEMP3ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_LAMEMP3ENC))
#define GST_IS_LAMEMP3ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_LAMEMP3ENC))

typedef struct _GstLameMP3Enc GstLameMP3Enc;
typedef struct _GstLameMP3EncClass GstLameMP3EncClass;

/**
 * GstLameMP3Enc:
 *
 * Opaque data structure.
 */
struct _GstLameMP3Enc {
  GstAudioEncoder element;

  /*< private >*/
  gint samplerate;
  gint out_samplerate;
  gint num_channels;

  /* properties */
  gint target;
  gint bitrate;
  gboolean cbr;
  gfloat quality;
  gint encoding_engine_quality;
  gboolean mono;

  lame_global_flags *lgf;

  GstAdapter *adapter;
};

struct _GstLameMP3EncClass {
  GstAudioEncoderClass parent_class;
};

GType gst_lamemp3enc_get_type(void);
gboolean gst_lamemp3enc_register (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_LAMEMP3ENC_H__ */
