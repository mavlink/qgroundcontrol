/*  MP3 decoding plugin for GStreamer using the mpg123 library
 *  Copyright (C) 2012 Carlos Rafael Giani
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GST_MPG123_AUDIO_DEC_H__
#define __GST_MPG123_AUDIO_DEC_H__

#include <gst/gst.h>
#include <gst/audio/gstaudiodecoder.h>
#include <mpg123.h>


G_BEGIN_DECLS


typedef struct _GstMpg123AudioDec GstMpg123AudioDec;
typedef struct _GstMpg123AudioDecClass GstMpg123AudioDecClass;


#define GST_TYPE_MPG123_AUDIO_DEC             (gst_mpg123_audio_dec_get_type())
#define GST_MPG123_AUDIO_DEC(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MPG123_AUDIO_DEC,GstMpg123AudioDec))
#define GST_MPG123_AUDIO_DEC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MPG123_AUDIO_DEC,GstMpg123AudioDecClass))
#define GST_IS_MPG123_AUDIO_DEC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MPG123_AUDIO_DEC))
#define GST_IS_MPG123_AUDIO_DEC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MPG123_AUDIO_DEC))

struct _GstMpg123AudioDec
{
  GstAudioDecoder parent;

  mpg123_handle *handle;

  GstAudioInfo next_audioinfo;
  gboolean has_next_audioinfo;

  off_t frame_offset;
};


struct _GstMpg123AudioDecClass
{
  GstAudioDecoderClass parent_class;
};

G_GNUC_INTERNAL GType gst_mpg123_audio_dec_get_type (void);

G_END_DECLS

#endif
