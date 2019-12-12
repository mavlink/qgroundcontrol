/* VP9
 * Copyright (C) 2006 David Schleef <ds@schleef.org>
 * Copyright (C) 2010 Entropy Wave Inc
 * Copyright (C) 2010-2013 Sebastian Dröge <slomo@circular-chaos.org>
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
/**
 * SECTION:element-vp9enc
 * @title: vp9enc
 * @see_also: vp9dec, webmmux, oggmux
 *
 * This element encodes raw video into a VP9 stream.
 * [VP9](http://www.webmproject.org) is a royalty-free video codec maintained by
 * [Google](http://www.google.com/). It's the successor of On2 VP3, which was
 * the base of the Theora video codec.
 *
 * To control the quality of the encoding, the #GstVP9Enc:target-bitrate,
 * #GstVP9Enc:min-quantizer, #GstVP9Enc:max-quantizer or #GstVP9Enc:cq-level
 * properties can be used. Which one is used depends on the mode selected by
 * the #GstVP9Enc:end-usage property.
 * See [Encoder Parameters](http://www.webmproject.org/docs/encoder-parameters/)
 * for explanation, examples for useful encoding parameters and more details
 * on the encoding parameters.
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 -v videotestsrc num-buffers=1000 ! vp9enc ! webmmux ! filesink location=videotestsrc.webm
 * ]| This example pipeline will encode a test video source to VP9 muxed in an
 * WebM container.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_VP9_ENCODER

/* glib decided in 2.32 it would be a great idea to deprecated GValueArray without
 * providing an alternative
 *
 * See https://bugzilla.gnome.org/show_bug.cgi?id=667228
 * */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <gst/tag/tag.h>
#include <gst/video/video.h>
#include <string.h>

#include "gstvp8utils.h"
#include "gstvp9enc.h"

GST_DEBUG_CATEGORY_STATIC (gst_vp9enc_debug);
#define GST_CAT_DEFAULT gst_vp9enc_debug


/* FIXME: Y42B and Y444 do not work yet it seems */
static GstStaticPadTemplate gst_vp9_enc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    /*GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ I420, YV12, Y42B, Y444 }")) */
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ I420, YV12 }"))
    );

static GstStaticPadTemplate gst_vp9_enc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-vp9, " "profile = (string) {0, 1, 2, 3}")
    );

#define parent_class gst_vp9_enc_parent_class
G_DEFINE_TYPE (GstVP9Enc, gst_vp9_enc, GST_TYPE_VPX_ENC);

static vpx_codec_iface_t *gst_vp9_enc_get_algo (GstVPXEnc * enc);
static gboolean gst_vp9_enc_enable_scaling (GstVPXEnc * enc);
static void gst_vp9_enc_set_image_format (GstVPXEnc * enc, vpx_image_t * image);
static GstCaps *gst_vp9_enc_get_new_simple_caps (GstVPXEnc * enc);
static void gst_vp9_enc_set_stream_info (GstVPXEnc * enc, GstCaps * caps,
    GstVideoInfo * info);
static void *gst_vp9_enc_process_frame_user_data (GstVPXEnc * enc,
    GstVideoCodecFrame * frame);
static GstFlowReturn gst_vp9_enc_handle_invisible_frame_buffer (GstVPXEnc * enc,
    void *user_data, GstBuffer * buffer);
static void gst_vp9_enc_set_frame_user_data (GstVPXEnc * enc,
    GstVideoCodecFrame * frame, vpx_image_t * image);

static void
gst_vp9_enc_class_init (GstVP9EncClass * klass)
{
  GstElementClass *element_class;
  GstVPXEncClass *vpx_encoder_class;

  element_class = GST_ELEMENT_CLASS (klass);
  vpx_encoder_class = GST_VPX_ENC_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_vp9_enc_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_vp9_enc_sink_template);

  gst_element_class_set_static_metadata (element_class,
      "On2 VP9 Encoder",
      "Codec/Encoder/Video",
      "Encode VP9 video streams", "David Schleef <ds@entropywave.com>, "
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  vpx_encoder_class->get_algo = gst_vp9_enc_get_algo;
  vpx_encoder_class->enable_scaling = gst_vp9_enc_enable_scaling;
  vpx_encoder_class->set_image_format = gst_vp9_enc_set_image_format;
  vpx_encoder_class->get_new_vpx_caps = gst_vp9_enc_get_new_simple_caps;
  vpx_encoder_class->set_stream_info = gst_vp9_enc_set_stream_info;
  vpx_encoder_class->process_frame_user_data =
      gst_vp9_enc_process_frame_user_data;
  vpx_encoder_class->handle_invisible_frame_buffer =
      gst_vp9_enc_handle_invisible_frame_buffer;
  vpx_encoder_class->set_frame_user_data = gst_vp9_enc_set_frame_user_data;

  GST_DEBUG_CATEGORY_INIT (gst_vp9enc_debug, "vp9enc", 0, "VP9 Encoder");
}

static void
gst_vp9_enc_init (GstVP9Enc * gst_vp9_enc)
{
  vpx_codec_err_t status;
  GstVPXEnc *gst_vpx_enc = GST_VPX_ENC (gst_vp9_enc);
  GST_DEBUG_OBJECT (gst_vp9_enc, "gst_vp9_enc_init");
  status =
      vpx_codec_enc_config_default (gst_vp9_enc_get_algo (gst_vpx_enc),
      &gst_vpx_enc->cfg, 0);
  if (status != VPX_CODEC_OK) {
    GST_ERROR_OBJECT (gst_vpx_enc,
        "Failed to get default encoder configuration: %s",
        gst_vpx_error_name (status));
    gst_vpx_enc->have_default_config = FALSE;
  } else {
    gst_vpx_enc->have_default_config = TRUE;
  }
}

static vpx_codec_iface_t *
gst_vp9_enc_get_algo (GstVPXEnc * enc)
{
  return &vpx_codec_vp9_cx_algo;
}

static gboolean
gst_vp9_enc_enable_scaling (GstVPXEnc * enc)
{
  return FALSE;
}

static void
gst_vp9_enc_set_image_format (GstVPXEnc * enc, vpx_image_t * image)
{
  switch (enc->input_state->info.finfo->format) {
    case GST_VIDEO_FORMAT_I420:
      image->fmt = VPX_IMG_FMT_I420;
      image->bps = 12;
      image->x_chroma_shift = image->y_chroma_shift = 1;
      break;
    case GST_VIDEO_FORMAT_YV12:
      image->fmt = VPX_IMG_FMT_YV12;
      image->bps = 12;
      image->x_chroma_shift = image->y_chroma_shift = 1;
      break;
    case GST_VIDEO_FORMAT_Y42B:
      image->fmt = VPX_IMG_FMT_I422;
      image->bps = 16;
      image->x_chroma_shift = 1;
      image->y_chroma_shift = 0;
      break;
    case GST_VIDEO_FORMAT_Y444:
      image->fmt = VPX_IMG_FMT_I444;
      image->bps = 24;
      image->x_chroma_shift = image->y_chroma_shift = 0;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

static GstCaps *
gst_vp9_enc_get_new_simple_caps (GstVPXEnc * enc)
{
  GstCaps *caps;
  gchar *profile_str = g_strdup_printf ("%d", enc->cfg.g_profile);
  caps = gst_caps_new_simple ("video/x-vp9",
      "profile", G_TYPE_STRING, profile_str, NULL);
  g_free (profile_str);
  return caps;
}

static void
gst_vp9_enc_set_stream_info (GstVPXEnc * enc, GstCaps * caps,
    GstVideoInfo * info)
{
  return;
}

static void *
gst_vp9_enc_process_frame_user_data (GstVPXEnc * enc,
    GstVideoCodecFrame * frame)
{
  return NULL;
}

static GstFlowReturn
gst_vp9_enc_handle_invisible_frame_buffer (GstVPXEnc * enc, void *user_data,
    GstBuffer * buffer)
{
  GstFlowReturn ret;
  g_mutex_unlock (&enc->encoder_lock);
  ret = gst_pad_push (GST_VIDEO_ENCODER_SRC_PAD (enc), buffer);
  g_mutex_lock (&enc->encoder_lock);
  return ret;
}

static void
gst_vp9_enc_user_data_free (vpx_image_t * image)
{
  g_slice_free (vpx_image_t, image);
}

static void
gst_vp9_enc_set_frame_user_data (GstVPXEnc * enc, GstVideoCodecFrame * frame,
    vpx_image_t * image)
{
  gst_video_codec_frame_set_user_data (frame, image,
      (GDestroyNotify) gst_vp9_enc_user_data_free);
  return;
}

#endif /* HAVE_VP9_ENCODER */
