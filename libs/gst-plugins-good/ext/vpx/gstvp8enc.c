/* VP8
 * Copyright (C) 2006 David Schleef <ds@schleef.org>
 * Copyright (C) 2010 Entropy Wave Inc
 * Copyright (C) 2010-2012 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
 * SECTION:element-vp8enc
 * @title: vp8enc
 * @see_also: vp8dec, webmmux, oggmux
 *
 * This element encodes raw video into a VP8 stream.
 * [VP8](http://www.webmproject.org) is a royalty-free video codec maintained by
 * [Google](http://www.google.com/). It's the successor of On2 VP3, which was
 * the base of the Theora video codec.
 *
 * To control the quality of the encoding, the #GstVP8Enc:target-bitrate,
 * #GstVP8Enc:min-quantizer, #GstVP8Enc:max-quantizer or #GstVP8Enc:cq-level
 * properties can be used. Which one is used depends on the mode selected by
 * the #GstVP8Enc:end-usage property.
 * See [Encoder Parameters](http://www.webmproject.org/docs/encoder-parameters/)
 * for explanation, examples for useful encoding parameters and more details
 * on the encoding parameters.
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 -v videotestsrc num-buffers=1000 ! vp8enc ! webmmux ! filesink location=videotestsrc.webm
 * ]| This example pipeline will encode a test video source to VP8 muxed in an
 * WebM container.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_VP8_ENCODER

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
#include "gstvp8enc.h"

GST_DEBUG_CATEGORY_STATIC (gst_vp8enc_debug);
#define GST_CAT_DEFAULT gst_vp8enc_debug

typedef struct
{
  vpx_image_t *image;
  GList *invisible;
} GstVP8EncUserData;

static void
_gst_mini_object_unref0 (GstMiniObject * obj)
{
  if (obj)
    gst_mini_object_unref (obj);
}

static void
gst_vp8_enc_user_data_free (GstVP8EncUserData * user_data)
{
  if (user_data->image)
    g_slice_free (vpx_image_t, user_data->image);

  g_list_foreach (user_data->invisible, (GFunc) _gst_mini_object_unref0, NULL);
  g_list_free (user_data->invisible);
  g_slice_free (GstVP8EncUserData, user_data);
}

static vpx_codec_iface_t *gst_vp8_enc_get_algo (GstVPXEnc * enc);
static gboolean gst_vp8_enc_enable_scaling (GstVPXEnc * enc);
static void gst_vp8_enc_set_image_format (GstVPXEnc * enc, vpx_image_t * image);
static GstCaps *gst_vp8_enc_get_new_simple_caps (GstVPXEnc * enc);
static void gst_vp8_enc_set_stream_info (GstVPXEnc * enc, GstCaps * caps,
    GstVideoInfo * info);
static void *gst_vp8_enc_process_frame_user_data (GstVPXEnc * enc,
    GstVideoCodecFrame * frame);
static GstFlowReturn gst_vp8_enc_handle_invisible_frame_buffer (GstVPXEnc * enc,
    void *user_data, GstBuffer * buffer);
static void gst_vp8_enc_set_frame_user_data (GstVPXEnc * enc,
    GstVideoCodecFrame * frame, vpx_image_t * image);

static GstFlowReturn gst_vp8_enc_pre_push (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);

static GstStaticPadTemplate gst_vp8_enc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) \"I420\", "
        "width = (int) [1, 16383], "
        "height = (int) [1, 16383], framerate = (fraction) [ 0/1, MAX ]")
    );

static GstStaticPadTemplate gst_vp8_enc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-vp8, " "profile = (string) {0, 1, 2, 3}")
    );

#define parent_class gst_vp8_enc_parent_class
G_DEFINE_TYPE (GstVP8Enc, gst_vp8_enc, GST_TYPE_VPX_ENC);

static void
gst_vp8_enc_class_init (GstVP8EncClass * klass)
{
  GstElementClass *element_class;
  GstVideoEncoderClass *video_encoder_class;
  GstVPXEncClass *vpx_encoder_class;

  element_class = GST_ELEMENT_CLASS (klass);
  video_encoder_class = GST_VIDEO_ENCODER_CLASS (klass);
  vpx_encoder_class = GST_VPX_ENC_CLASS (klass);


  gst_element_class_add_static_pad_template (element_class,
      &gst_vp8_enc_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_vp8_enc_sink_template);

  gst_element_class_set_static_metadata (element_class,
      "On2 VP8 Encoder",
      "Codec/Encoder/Video",
      "Encode VP8 video streams", "David Schleef <ds@entropywave.com>, "
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  video_encoder_class->pre_push = gst_vp8_enc_pre_push;

  vpx_encoder_class->get_algo = gst_vp8_enc_get_algo;
  vpx_encoder_class->enable_scaling = gst_vp8_enc_enable_scaling;
  vpx_encoder_class->set_image_format = gst_vp8_enc_set_image_format;
  vpx_encoder_class->get_new_vpx_caps = gst_vp8_enc_get_new_simple_caps;
  vpx_encoder_class->set_stream_info = gst_vp8_enc_set_stream_info;
  vpx_encoder_class->process_frame_user_data =
      gst_vp8_enc_process_frame_user_data;
  vpx_encoder_class->handle_invisible_frame_buffer =
      gst_vp8_enc_handle_invisible_frame_buffer;
  vpx_encoder_class->set_frame_user_data = gst_vp8_enc_set_frame_user_data;

  GST_DEBUG_CATEGORY_INIT (gst_vp8enc_debug, "vp8enc", 0, "VP8 Encoder");
}

static void
gst_vp8_enc_init (GstVP8Enc * gst_vp8_enc)
{
  vpx_codec_err_t status;
  GstVPXEnc *gst_vpx_enc = GST_VPX_ENC (gst_vp8_enc);
  GST_DEBUG_OBJECT (gst_vp8_enc, "gst_vp8_enc_init");
  status =
      vpx_codec_enc_config_default (gst_vp8_enc_get_algo (gst_vpx_enc),
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
gst_vp8_enc_get_algo (GstVPXEnc * enc)
{
  return &vpx_codec_vp8_cx_algo;
}

static gboolean
gst_vp8_enc_enable_scaling (GstVPXEnc * enc)
{
  return TRUE;
}

static void
gst_vp8_enc_set_image_format (GstVPXEnc * enc, vpx_image_t * image)
{
  image->fmt = VPX_IMG_FMT_I420;
  image->bps = 12;
  image->x_chroma_shift = image->y_chroma_shift = 1;
}

static GstCaps *
gst_vp8_enc_get_new_simple_caps (GstVPXEnc * enc)
{
  GstCaps *caps;
  gchar *profile_str = g_strdup_printf ("%d", enc->cfg.g_profile);
  caps = gst_caps_new_simple ("video/x-vp8",
      "profile", G_TYPE_STRING, profile_str, NULL);
  g_free (profile_str);
  return caps;
}

static void
gst_vp8_enc_set_stream_info (GstVPXEnc * enc, GstCaps * caps,
    GstVideoInfo * info)
{
  GstStructure *s;
  GstVideoEncoder *video_encoder;
  GstBuffer *stream_hdr, *vorbiscomment;
  const GstTagList *iface_tags;
  GValue array = { 0, };
  GValue value = { 0, };
  guint8 *data = NULL;
  GstMapInfo map;

  video_encoder = GST_VIDEO_ENCODER (enc);
  s = gst_caps_get_structure (caps, 0);

  /* put buffers in a fixed list */
  g_value_init (&array, GST_TYPE_ARRAY);
  g_value_init (&value, GST_TYPE_BUFFER);

  /* Create Ogg stream-info */
  stream_hdr = gst_buffer_new_and_alloc (26);
  gst_buffer_map (stream_hdr, &map, GST_MAP_WRITE);
  data = map.data;

  GST_WRITE_UINT8 (data, 0x4F);
  GST_WRITE_UINT32_BE (data + 1, 0x56503830);   /* "VP80" */
  GST_WRITE_UINT8 (data + 5, 0x01);     /* stream info header */
  GST_WRITE_UINT8 (data + 6, 1);        /* Major version 1 */
  GST_WRITE_UINT8 (data + 7, 0);        /* Minor version 0 */
  GST_WRITE_UINT16_BE (data + 8, GST_VIDEO_INFO_WIDTH (info));
  GST_WRITE_UINT16_BE (data + 10, GST_VIDEO_INFO_HEIGHT (info));
  GST_WRITE_UINT24_BE (data + 12, GST_VIDEO_INFO_PAR_N (info));
  GST_WRITE_UINT24_BE (data + 15, GST_VIDEO_INFO_PAR_D (info));
  GST_WRITE_UINT32_BE (data + 18, GST_VIDEO_INFO_FPS_N (info));
  GST_WRITE_UINT32_BE (data + 22, GST_VIDEO_INFO_FPS_D (info));

  gst_buffer_unmap (stream_hdr, &map);

  GST_BUFFER_FLAG_SET (stream_hdr, GST_BUFFER_FLAG_HEADER);
  gst_value_set_buffer (&value, stream_hdr);
  gst_value_array_append_value (&array, &value);
  g_value_unset (&value);
  gst_buffer_unref (stream_hdr);

  iface_tags = gst_tag_setter_get_tag_list (GST_TAG_SETTER (video_encoder));
  if (iface_tags) {
    vorbiscomment =
        gst_tag_list_to_vorbiscomment_buffer (iface_tags,
        (const guint8 *) "OVP80\2 ", 7,
        "Encoded with GStreamer vp8enc " PACKAGE_VERSION);

    GST_BUFFER_FLAG_SET (vorbiscomment, GST_BUFFER_FLAG_HEADER);

    g_value_init (&value, GST_TYPE_BUFFER);
    gst_value_set_buffer (&value, vorbiscomment);
    gst_value_array_append_value (&array, &value);
    g_value_unset (&value);
    gst_buffer_unref (vorbiscomment);
  }

  gst_structure_set_value (s, "streamheader", &array);
  g_value_unset (&array);

}

static void *
gst_vp8_enc_process_frame_user_data (GstVPXEnc * enc,
    GstVideoCodecFrame * frame)
{
  GstVP8EncUserData *user_data;

  user_data = gst_video_codec_frame_get_user_data (frame);

  if (!user_data) {
    GST_ERROR_OBJECT (enc, "Have no frame user data");
    return NULL;
  }

  if (user_data->image)
    g_slice_free (vpx_image_t, user_data->image);
  user_data->image = NULL;
  return user_data;
}

static GstFlowReturn
gst_vp8_enc_handle_invisible_frame_buffer (GstVPXEnc * enc, void *user_data,
    GstBuffer * buffer)
{
  GstVP8EncUserData *vp8_user_data = (GstVP8EncUserData *) user_data;

  if (!vp8_user_data) {
    GST_ERROR_OBJECT (enc, "Have no frame user data");
    return GST_FLOW_ERROR;
  }

  vp8_user_data->invisible = g_list_append (vp8_user_data->invisible, buffer);

  return GST_FLOW_OK;
}

static void
gst_vp8_enc_set_frame_user_data (GstVPXEnc * enc, GstVideoCodecFrame * frame,
    vpx_image_t * image)
{
  GstVP8EncUserData *user_data;
  user_data = g_slice_new0 (GstVP8EncUserData);
  user_data->image = image;
  gst_video_codec_frame_set_user_data (frame, user_data,
      (GDestroyNotify) gst_vp8_enc_user_data_free);
  return;
}

static guint64
_to_granulepos (guint64 frame_end_number, guint inv_count, guint keyframe_dist)
{
  guint64 granulepos;
  guint inv;

  inv = (inv_count == 0) ? 0x3 : inv_count - 1;

  granulepos = (frame_end_number << 32) | (inv << 30) | (keyframe_dist << 3);
  return granulepos;
}

static GstFlowReturn
gst_vp8_enc_pre_push (GstVideoEncoder * video_encoder,
    GstVideoCodecFrame * frame)
{
  GstVP8Enc *encoder;
  GstVPXEnc *vpx_enc;
  GstBuffer *buf;
  GstFlowReturn ret = GST_FLOW_OK;
  GstVP8EncUserData *user_data = gst_video_codec_frame_get_user_data (frame);
  GList *l;
  gint inv_count;
  GstVideoInfo *info;

  GST_DEBUG_OBJECT (video_encoder, "pre_push");

  encoder = GST_VP8_ENC (video_encoder);
  vpx_enc = GST_VPX_ENC (encoder);

  info = &vpx_enc->input_state->info;

  g_assert (user_data != NULL);

  for (inv_count = 0, l = user_data->invisible; l; inv_count++, l = l->next) {
    buf = l->data;
    l->data = NULL;

    /* FIXME : All of this should have already been handled by base classes, no ? */
    if (l == user_data->invisible
        && GST_VIDEO_CODEC_FRAME_IS_SYNC_POINT (frame)) {
      GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DELTA_UNIT);
      encoder->keyframe_distance = 0;
    } else {
      GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT);
      encoder->keyframe_distance++;
    }

    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DECODE_ONLY);
    GST_BUFFER_TIMESTAMP (buf) = GST_BUFFER_TIMESTAMP (frame->output_buffer);
    GST_BUFFER_DURATION (buf) = 0;
    if (GST_VIDEO_INFO_FPS_D (info) == 0 || GST_VIDEO_INFO_FPS_N (info) == 0) {
      GST_BUFFER_OFFSET_END (buf) = GST_BUFFER_OFFSET_NONE;
      GST_BUFFER_OFFSET (buf) = GST_BUFFER_OFFSET_NONE;
    } else {
      GST_BUFFER_OFFSET_END (buf) =
          _to_granulepos (frame->presentation_frame_number + 1,
          inv_count, encoder->keyframe_distance);
      GST_BUFFER_OFFSET (buf) =
          gst_util_uint64_scale (frame->presentation_frame_number + 1,
          GST_SECOND * GST_VIDEO_INFO_FPS_D (info),
          GST_VIDEO_INFO_FPS_N (info));
    }

    ret = gst_pad_push (GST_VIDEO_ENCODER_SRC_PAD (video_encoder), buf);

    if (ret != GST_FLOW_OK) {
      GST_WARNING_OBJECT (encoder, "flow error %d", ret);
      goto done;
    }
  }

  buf = frame->output_buffer;

  /* FIXME : All of this should have already been handled by base classes, no ? */
  if (!user_data->invisible && GST_VIDEO_CODEC_FRAME_IS_SYNC_POINT (frame)) {
    GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DELTA_UNIT);
    encoder->keyframe_distance = 0;
  } else {
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT);
    encoder->keyframe_distance++;
  }

  if (GST_VIDEO_INFO_FPS_D (info) == 0 || GST_VIDEO_INFO_FPS_N (info) == 0) {
    GST_BUFFER_OFFSET_END (buf) = GST_BUFFER_OFFSET_NONE;
    GST_BUFFER_OFFSET (buf) = GST_BUFFER_OFFSET_NONE;
  } else {
    GST_BUFFER_OFFSET_END (buf) =
        _to_granulepos (frame->presentation_frame_number + 1, 0,
        encoder->keyframe_distance);
    GST_BUFFER_OFFSET (buf) =
        gst_util_uint64_scale (frame->presentation_frame_number + 1,
        GST_SECOND * GST_VIDEO_INFO_FPS_D (info), GST_VIDEO_INFO_FPS_N (info));
  }

  GST_LOG_OBJECT (video_encoder, "src ts: %" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)));

done:
  return ret;
}

#endif /* HAVE_VP8_ENCODER */
