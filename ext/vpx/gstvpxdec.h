/* VPX
 * Copyright (C) 2006 David Schleef <ds@schleef.org>
 * Copyright (C) 2008,2009,2010 Entropy Wave Inc
 * Copyright (C) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
 *
 */

#ifndef __GST_VPX_DEC_H__
#define __GST_VPX_DEC_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_VP8_DECODER) || defined(HAVE_VP9_DECODER)

#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>

/* FIXME: Undef HAVE_CONFIG_H because vpx_codec.h uses it,
 * which causes compilation failures */
#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif

#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

G_BEGIN_DECLS

#define GST_TYPE_VPX_DEC \
  (gst_vpx_dec_get_type())
#define GST_VPX_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VPX_DEC,GstVPXDec))
#define GST_VPX_DEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VPX_DEC,GstVPXDecClass))
#define GST_IS_VPX_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VPX_DEC))
#define GST_IS_VPX_DEC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VPX_DEC))
#define GST_VPX_DEC_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_VPX_DEC, GstVPXDecClass))

typedef struct _GstVPXDec GstVPXDec;
typedef struct _GstVPXDecClass GstVPXDecClass;

struct _GstVPXDec
{
  GstVideoDecoder base_video_decoder;

  /* < private > */
  vpx_codec_ctx_t decoder;

  /* state */
  gboolean decoder_inited;

  /* properties */
  gboolean post_processing;
  enum vp8_postproc_level post_processing_flags;
  gint deblocking_level;
  gint noise_level;
  gint threads;

  GstVideoCodecState *input_state;
  GstVideoCodecState *output_state;

  /* allocation */
  gboolean have_video_meta;
  GstBufferPool *pool;
  gsize buf_size;
};

struct _GstVPXDecClass
{
  GstVideoDecoderClass base_video_decoder_class;
  const char* video_codec_tag;
  /*supported vpx algo*/
  vpx_codec_iface_t* codec_algo;
  /*virtual function to open_codec*/
  GstFlowReturn (*open_codec) (GstVPXDec * dec, GstVideoCodecFrame * frame);
  /*virtual function to send tags*/
  void (*send_tags) (GstVPXDec* dec);
  /*virtual function to set/correct the stream info*/
  void (*set_stream_info) (GstVPXDec *dec, vpx_codec_stream_info_t *stream_info);
  /*virtual function to set default format while opening codec*/
  void (*set_default_format) (GstVPXDec *dec, GstVideoFormat fmt, int width, int height);
  /*virtual function to negotiate format while handling frame*/
  void (*handle_resolution_change) (GstVPXDec *dec, vpx_image_t *img, GstVideoFormat fmt);
  /*virtual function to check valid format*/
  gboolean (*get_frame_format)(GstVPXDec *dec, vpx_image_t *img, GstVideoFormat* fmt);
};

GType gst_vpx_dec_get_type (void);

G_END_DECLS

#endif

#endif /* __GST_VP8_DEC_H__ */
