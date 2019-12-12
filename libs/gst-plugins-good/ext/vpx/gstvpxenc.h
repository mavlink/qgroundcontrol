/* VP8
 * Copyright (C) 2006 David Schleef <ds@schleef.org>
 * Copyright (C) 2010 Entropy Wave Inc
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
#ifndef __GST_VPX_ENC_H__
#define __GST_VPX_ENC_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_VP8_ENCODER) || defined(HAVE_VP9_ENCODER)

#include <gst/gst.h>
#include <gst/video/gstvideoencoder.h>

/* FIXME: Undef HAVE_CONFIG_H because vpx_codec.h uses it,
 * which causes compilation failures */
#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif

#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>

G_BEGIN_DECLS

#define GST_TYPE_VPX_ENC \
  (gst_vpx_enc_get_type())
#define GST_VPX_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VPX_ENC,GstVPXEnc))
#define GST_VPX_ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VPX_ENC,GstVPXEncClass))
#define GST_IS_VPX_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VPX_ENC))
#define GST_IS_VPX_ENC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VPX_ENC))
#define GST_VPX_ENC_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_VPX_ENC, GstVPXEncClass))

typedef struct _GstVPXEnc GstVPXEnc;
typedef struct _GstVPXEncClass GstVPXEncClass;

struct _GstVPXEnc
{
  GstVideoEncoder base_video_encoder;

  /* < private > */
  vpx_codec_ctx_t encoder;
  GMutex encoder_lock;

  /* properties */
  vpx_codec_enc_cfg_t cfg;
  gboolean have_default_config;
  gboolean rc_target_bitrate_set;
  gint n_ts_target_bitrate;
  gint n_ts_rate_decimator;
  gint n_ts_layer_id;
  /* Global two-pass options */
  gchar *multipass_cache_file;
  gchar *multipass_cache_prefix;
  guint multipass_cache_idx;
  GByteArray *first_pass_cache_content;

  /* Encode parameter */
  gint64 deadline;

  /* Controls */
  VPX_SCALING_MODE h_scaling_mode;
  VPX_SCALING_MODE v_scaling_mode;
  int cpu_used;
  gboolean enable_auto_alt_ref;
  unsigned int noise_sensitivity;
  unsigned int sharpness;
  unsigned int static_threshold;
  vp8e_token_partitions token_partitions;
  unsigned int arnr_maxframes;
  unsigned int arnr_strength;
  unsigned int arnr_type;
  vp8e_tuning tuning;
  unsigned int cq_level;
  unsigned int max_intra_bitrate_pct;
  /* Timebase - a value of 0 will use the framerate */
  unsigned int timebase_n;
  unsigned int timebase_d;

  /* state */
  gboolean inited;

  vpx_image_t image;

  GstClockTime last_pts;

  GstVideoCodecState *input_state;
};

struct _GstVPXEncClass
{
  GstVideoEncoderClass base_video_encoder_class;
  /*virtual function to get supported algo*/
  vpx_codec_iface_t* (*get_algo) (GstVPXEnc *enc);
  /*enabled scaling*/
  gboolean (*enable_scaling) (GstVPXEnc *enc);
  /*set image format info*/
  void (*set_image_format) (GstVPXEnc *enc, vpx_image_t *image);
  /*get new simple caps*/
  GstCaps* (*get_new_vpx_caps) (GstVPXEnc *enc);
  /*set stream info*/
  void (*set_stream_info) (GstVPXEnc *enc, GstCaps *caps, GstVideoInfo *info);
  /*process user data*/
  void* (*process_frame_user_data) (GstVPXEnc *enc, GstVideoCodecFrame* frame);
  /*set frame user data*/
  void (*set_frame_user_data) (GstVPXEnc *enc, GstVideoCodecFrame* frame, vpx_image_t *image);
  /*Handle invisible frame*/
  GstFlowReturn (*handle_invisible_frame_buffer) (GstVPXEnc *enc, void* user_data, GstBuffer* buffer);
};

GType gst_vpx_enc_get_type (void);

G_END_DECLS

#endif

#endif /* __GST_VPX_ENC_H__ */
