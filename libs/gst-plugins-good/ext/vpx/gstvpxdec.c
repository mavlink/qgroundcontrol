/* VPX
 * Copyright (C) 2006 David Schleef <ds@schleef.org>
 * Copyright (C) 2008,2009,2010 Entropy Wave Inc
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
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_VP8_DECODER) || defined(HAVE_VP9_DECODER)

#include <string.h>

#include "gstvpxdec.h"
#include "gstvp8utils.h"

#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

GST_DEBUG_CATEGORY_STATIC (gst_vpxdec_debug);
#define GST_CAT_DEFAULT gst_vpxdec_debug

#define DEFAULT_POST_PROCESSING FALSE
#define DEFAULT_POST_PROCESSING_FLAGS (VP8_DEBLOCK | VP8_DEMACROBLOCK | VP8_MFQE)
#define DEFAULT_DEBLOCKING_LEVEL 4
#define DEFAULT_NOISE_LEVEL 0
#define DEFAULT_THREADS 0
#define DEFAULT_VIDEO_CODEC_TAG NULL
#define DEFAULT_CODEC_ALGO NULL

enum
{
  PROP_0,
  PROP_POST_PROCESSING,
  PROP_POST_PROCESSING_FLAGS,
  PROP_DEBLOCKING_LEVEL,
  PROP_NOISE_LEVEL,
  PROP_THREADS
};

#define C_FLAGS(v) ((guint) v)
#define GST_VPX_DEC_TYPE_POST_PROCESSING_FLAGS (gst_vpx_dec_post_processing_flags_get_type())
static GType
gst_vpx_dec_post_processing_flags_get_type (void)
{
  static const GFlagsValue values[] = {
    {C_FLAGS (VP8_DEBLOCK), "Deblock", "deblock"},
    {C_FLAGS (VP8_DEMACROBLOCK), "Demacroblock", "demacroblock"},
    {C_FLAGS (VP8_ADDNOISE), "Add noise", "addnoise"},
#ifndef HAVE_VPX_1_8
    {C_FLAGS (VP8_DEBUG_TXT_FRAME_INFO),
          "Print frame information",
        "visualize-frame-info"},
    {C_FLAGS (VP8_DEBUG_TXT_MBLK_MODES),
          "Show macroblock mode selection overlaid on image",
        "visualize-macroblock-modes"},
    {C_FLAGS (VP8_DEBUG_TXT_DC_DIFF),
          "Show dc diff for each macro block overlaid on image",
        "visualize-dc-diff"},
    {C_FLAGS (VP8_DEBUG_TXT_RATE_INFO),
          "Print video rate info",
        "visualize-rate-info"},
#endif
    {C_FLAGS (VP8_MFQE), "Multi-frame quality enhancement", "mfqe"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_flags_register_static ("GstVPXDecPostProcessingFlags", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#undef C_FLAGS

static void gst_vpx_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_vpx_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_vpx_dec_start (GstVideoDecoder * decoder);
static gboolean gst_vpx_dec_stop (GstVideoDecoder * decoder);
static gboolean gst_vpx_dec_set_format (GstVideoDecoder * decoder,
    GstVideoCodecState * state);
static gboolean gst_vpx_dec_flush (GstVideoDecoder * decoder);
static GstFlowReturn
gst_vpx_dec_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame);
static gboolean gst_vpx_dec_decide_allocation (GstVideoDecoder * decoder,
    GstQuery * query);

static void gst_vpx_dec_image_to_buffer (GstVPXDec * dec,
    const vpx_image_t * img, GstBuffer * buffer);
static GstFlowReturn gst_vpx_dec_open_codec (GstVPXDec * dec,
    GstVideoCodecFrame * frame);
static void gst_vpx_dec_default_send_tags (GstVPXDec * dec);
static void gst_vpx_dec_set_stream_info (GstVPXDec * dec,
    vpx_codec_stream_info_t * stream_info);
static void gst_vpx_dec_set_default_format (GstVPXDec * dec, GstVideoFormat fmt,
    int width, int height);
static gboolean gst_vpx_dec_default_frame_format (GstVPXDec * dec,
    vpx_image_t * img, GstVideoFormat * fmt);
static void gst_vpx_dec_handle_resolution_change (GstVPXDec * dec,
    vpx_image_t * img, GstVideoFormat fmt);

#define parent_class gst_vpx_dec_parent_class
G_DEFINE_TYPE (GstVPXDec, gst_vpx_dec, GST_TYPE_VIDEO_DECODER);

static void
gst_vpx_dec_class_init (GstVPXDecClass * klass)
{
  GObjectClass *gobject_class;
  GstVideoDecoderClass *base_video_decoder_class;

  gobject_class = G_OBJECT_CLASS (klass);
  base_video_decoder_class = GST_VIDEO_DECODER_CLASS (klass);

  gobject_class->set_property = gst_vpx_dec_set_property;
  gobject_class->get_property = gst_vpx_dec_get_property;

  g_object_class_install_property (gobject_class, PROP_POST_PROCESSING,
      g_param_spec_boolean ("post-processing", "Post Processing",
          "Enable post processing", DEFAULT_POST_PROCESSING,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_POST_PROCESSING_FLAGS,
      g_param_spec_flags ("post-processing-flags", "Post Processing Flags",
          "Flags to control post processing",
          GST_VPX_DEC_TYPE_POST_PROCESSING_FLAGS, DEFAULT_POST_PROCESSING_FLAGS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEBLOCKING_LEVEL,
      g_param_spec_uint ("deblocking-level", "Deblocking Level",
          "Deblocking level",
          0, 16, DEFAULT_DEBLOCKING_LEVEL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NOISE_LEVEL,
      g_param_spec_uint ("noise-level", "Noise Level",
          "Noise level",
          0, 16, DEFAULT_NOISE_LEVEL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_THREADS,
      g_param_spec_uint ("threads", "Max Threads",
          "Maximum number of decoding threads",
          0, 16, DEFAULT_THREADS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  base_video_decoder_class->start = GST_DEBUG_FUNCPTR (gst_vpx_dec_start);
  base_video_decoder_class->stop = GST_DEBUG_FUNCPTR (gst_vpx_dec_stop);
  base_video_decoder_class->flush = GST_DEBUG_FUNCPTR (gst_vpx_dec_flush);
  base_video_decoder_class->set_format =
      GST_DEBUG_FUNCPTR (gst_vpx_dec_set_format);
  base_video_decoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_vpx_dec_handle_frame);;
  base_video_decoder_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_vpx_dec_decide_allocation);

  klass->video_codec_tag = DEFAULT_VIDEO_CODEC_TAG;
  klass->codec_algo = DEFAULT_CODEC_ALGO;
  klass->open_codec = GST_DEBUG_FUNCPTR (gst_vpx_dec_open_codec);
  klass->send_tags = GST_DEBUG_FUNCPTR (gst_vpx_dec_default_send_tags);
  klass->set_stream_info = NULL;
  klass->set_default_format = NULL;
  klass->handle_resolution_change = NULL;
  klass->get_frame_format =
      GST_DEBUG_FUNCPTR (gst_vpx_dec_default_frame_format);

  GST_DEBUG_CATEGORY_INIT (gst_vpxdec_debug, "vpxdec", 0, "VPX Decoder");
}

static void
gst_vpx_dec_init (GstVPXDec * gst_vpx_dec)
{
  GstVideoDecoder *decoder = (GstVideoDecoder *) gst_vpx_dec;

  GST_DEBUG_OBJECT (gst_vpx_dec, "gst_vpx_dec_init");
  gst_video_decoder_set_packetized (decoder, TRUE);
  gst_vpx_dec->post_processing = DEFAULT_POST_PROCESSING;
  gst_vpx_dec->post_processing_flags = DEFAULT_POST_PROCESSING_FLAGS;
  gst_vpx_dec->deblocking_level = DEFAULT_DEBLOCKING_LEVEL;
  gst_vpx_dec->noise_level = DEFAULT_NOISE_LEVEL;

  gst_video_decoder_set_needs_format (decoder, TRUE);
  gst_video_decoder_set_use_default_pad_acceptcaps (decoder, TRUE);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_DECODER_SINK_PAD (decoder));
}

static void
gst_vpx_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVPXDec *dec;

  g_return_if_fail (GST_IS_VPX_DEC (object));
  dec = GST_VPX_DEC (object);

  GST_DEBUG_OBJECT (object, "gst_vpx_dec_set_property");
  switch (prop_id) {
    case PROP_POST_PROCESSING:
      dec->post_processing = g_value_get_boolean (value);
      break;
    case PROP_POST_PROCESSING_FLAGS:
      dec->post_processing_flags = g_value_get_flags (value);
      break;
    case PROP_DEBLOCKING_LEVEL:
      dec->deblocking_level = g_value_get_uint (value);
      break;
    case PROP_NOISE_LEVEL:
      dec->noise_level = g_value_get_uint (value);
      break;
    case PROP_THREADS:
      dec->threads = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_vpx_dec_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstVPXDec *dec;

  g_return_if_fail (GST_IS_VPX_DEC (object));
  dec = GST_VPX_DEC (object);

  switch (prop_id) {
    case PROP_POST_PROCESSING:
      g_value_set_boolean (value, dec->post_processing);
      break;
    case PROP_POST_PROCESSING_FLAGS:
      g_value_set_flags (value, dec->post_processing_flags);
      break;
    case PROP_DEBLOCKING_LEVEL:
      g_value_set_uint (value, dec->deblocking_level);
      break;
    case PROP_NOISE_LEVEL:
      g_value_set_uint (value, dec->noise_level);
      break;
    case PROP_THREADS:
      g_value_set_uint (value, dec->threads);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_vpx_dec_start (GstVideoDecoder * decoder)
{
  GstVPXDec *gst_vpx_dec = GST_VPX_DEC (decoder);

  GST_DEBUG_OBJECT (gst_vpx_dec, "start");
  gst_vpx_dec->decoder_inited = FALSE;

  return TRUE;
}

static gboolean
gst_vpx_dec_stop (GstVideoDecoder * base_video_decoder)
{
  GstVPXDec *gst_vpx_dec = GST_VPX_DEC (base_video_decoder);

  GST_DEBUG_OBJECT (gst_vpx_dec, "stop");

  if (gst_vpx_dec->output_state) {
    gst_video_codec_state_unref (gst_vpx_dec->output_state);
    gst_vpx_dec->output_state = NULL;
  }

  if (gst_vpx_dec->input_state) {
    gst_video_codec_state_unref (gst_vpx_dec->input_state);
    gst_vpx_dec->input_state = NULL;
  }

  if (gst_vpx_dec->decoder_inited)
    vpx_codec_destroy (&gst_vpx_dec->decoder);
  gst_vpx_dec->decoder_inited = FALSE;

  if (gst_vpx_dec->pool) {
    gst_buffer_pool_set_active (gst_vpx_dec->pool, FALSE);
    gst_object_unref (gst_vpx_dec->pool);
    gst_vpx_dec->pool = NULL;
    gst_vpx_dec->buf_size = 0;
  }

  return TRUE;
}

static gboolean
gst_vpx_dec_set_format (GstVideoDecoder * decoder, GstVideoCodecState * state)
{
  GstVPXDec *gst_vpx_dec = GST_VPX_DEC (decoder);

  GST_DEBUG_OBJECT (gst_vpx_dec, "set_format");

  if (gst_vpx_dec->decoder_inited)
    vpx_codec_destroy (&gst_vpx_dec->decoder);
  gst_vpx_dec->decoder_inited = FALSE;

  if (gst_vpx_dec->output_state) {
    gst_video_codec_state_unref (gst_vpx_dec->output_state);
    gst_vpx_dec->output_state = NULL;
  }

  if (gst_vpx_dec->input_state) {
    gst_video_codec_state_unref (gst_vpx_dec->input_state);
  }

  gst_vpx_dec->input_state = gst_video_codec_state_ref (state);

  return TRUE;
}

static gboolean
gst_vpx_dec_flush (GstVideoDecoder * base_video_decoder)
{
  GstVPXDec *decoder;

  GST_DEBUG_OBJECT (base_video_decoder, "flush");

  decoder = GST_VPX_DEC (base_video_decoder);

  if (decoder->output_state) {
    gst_video_codec_state_unref (decoder->output_state);
    decoder->output_state = NULL;
  }

  if (decoder->decoder_inited)
    vpx_codec_destroy (&decoder->decoder);
  decoder->decoder_inited = FALSE;

  return TRUE;
}

static void
gst_vpx_dec_default_send_tags (GstVPXDec * dec)
{
  GstTagList *list;
  GstVPXDecClass *vpxclass = GST_VPX_DEC_GET_CLASS (dec);

  if (vpxclass->video_codec_tag == NULL)
    return;

  list = gst_tag_list_new_empty ();
  gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
      GST_TAG_VIDEO_CODEC, vpxclass->video_codec_tag, NULL);

  gst_pad_push_event (GST_VIDEO_DECODER_SRC_PAD (dec),
      gst_event_new_tag (list));
}

struct Frame
{
  GstMapInfo info;
  GstBuffer *buffer;
};

static GstBuffer *
gst_vpx_dec_prepare_image (GstVPXDec * dec, const vpx_image_t * img)
{
  gint comp;
  GstVideoMeta *vmeta;
  GstBuffer *buffer;
  struct Frame *frame = img->fb_priv;
  GstVideoInfo *info = &dec->output_state->info;

  buffer = gst_buffer_ref (frame->buffer);

  vmeta = gst_buffer_get_video_meta (buffer);
  vmeta->format = GST_VIDEO_INFO_FORMAT (info);
  vmeta->width = GST_VIDEO_INFO_WIDTH (info);
  vmeta->height = GST_VIDEO_INFO_HEIGHT (info);
  vmeta->n_planes = GST_VIDEO_INFO_N_PLANES (info);

  for (comp = 0; comp < 4; comp++) {
    vmeta->stride[comp] = img->stride[comp];
    vmeta->offset[comp] =
        img->planes[comp] ? img->planes[comp] - frame->info.data : 0;
  }

  /* FIXME This is a READ/WRITE mapped buffer see bug #754826 */

  return buffer;
}

static int
gst_vpx_dec_get_buffer_cb (gpointer priv, gsize min_size,
    vpx_codec_frame_buffer_t * fb)
{
  GstVPXDec *dec = priv;
  GstBuffer *buffer = NULL;
  struct Frame *frame;
  GstFlowReturn ret;

  if (!dec->pool || dec->buf_size != min_size) {
    GstBufferPool *pool;
    GstStructure *config;
    GstCaps *caps;
    GstAllocator *allocator;
    GstAllocationParams params;

    if (dec->pool) {
      gst_buffer_pool_set_active (dec->pool, FALSE);
      gst_object_unref (dec->pool);
      dec->pool = NULL;
      dec->buf_size = 0;
    }

    gst_video_decoder_get_allocator (GST_VIDEO_DECODER (dec), &allocator,
        &params);

    if (allocator &&
        GST_OBJECT_FLAG_IS_SET (allocator, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC)) {
      gst_object_unref (allocator);
      allocator = NULL;
    }

    pool = gst_buffer_pool_new ();
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_set_allocator (config, allocator, &params);
    caps = gst_caps_from_string ("video/internal");
    gst_buffer_pool_config_set_params (config, caps, min_size, 2, 0);
    gst_caps_unref (caps);
    gst_buffer_pool_set_config (pool, config);

    if (allocator)
      gst_object_unref (allocator);

    if (!gst_buffer_pool_set_active (pool, TRUE)) {
      GST_WARNING ("Failed to create internal pool");
      gst_object_unref (pool);
      return -1;
    }

    dec->pool = pool;
    dec->buf_size = min_size;
  }

  ret = gst_buffer_pool_acquire_buffer (dec->pool, &buffer, NULL);
  if (ret != GST_FLOW_OK) {
    GST_WARNING ("Failed to acquire buffer from internal pool.");
    return -1;
  }

  /* Add it now, while the buffer is writable */
  gst_buffer_add_video_meta (buffer, GST_VIDEO_FRAME_FLAG_NONE,
      GST_VIDEO_FORMAT_ENCODED, 0, 0);

  frame = g_new0 (struct Frame, 1);
  if (!gst_buffer_map (buffer, &frame->info, GST_MAP_READWRITE)) {
    gst_buffer_unref (buffer);
    g_free (frame);
    GST_WARNING ("Failed to map buffer from internal pool.");
    return -1;
  }

  fb->size = frame->info.size;
  fb->data = frame->info.data;
  frame->buffer = buffer;
  fb->priv = frame;

  GST_TRACE_OBJECT (priv, "Allocated buffer %p", frame->buffer);

  return 0;
}

static int
gst_vpx_dec_release_buffer_cb (gpointer priv, vpx_codec_frame_buffer_t * fb)
{
  struct Frame *frame = fb->priv;

  /* We're sometimes called without a frame */
  if (!frame)
    return 0;

  GST_TRACE_OBJECT (priv, "Release buffer %p", frame->buffer);

  gst_buffer_unmap (frame->buffer, &frame->info);
  gst_buffer_unref (frame->buffer);
  g_free (frame);
  fb->priv = NULL;

  return 0;
}

static void
gst_vpx_dec_image_to_buffer (GstVPXDec * dec, const vpx_image_t * img,
    GstBuffer * buffer)
{
  int deststride, srcstride, height, width, line, comp;
  guint8 *dest, *src;
  GstVideoFrame frame;
  GstVideoInfo *info = &dec->output_state->info;

  if (!gst_video_frame_map (&frame, info, buffer, GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (dec, "Could not map video buffer");
    return;
  }

  for (comp = 0; comp < 3; comp++) {
    dest = GST_VIDEO_FRAME_COMP_DATA (&frame, comp);
    src = img->planes[comp];
    width = GST_VIDEO_FRAME_COMP_WIDTH (&frame, comp)
        * GST_VIDEO_FRAME_COMP_PSTRIDE (&frame, comp);
    height = GST_VIDEO_FRAME_COMP_HEIGHT (&frame, comp);
    deststride = GST_VIDEO_FRAME_COMP_STRIDE (&frame, comp);
    srcstride = img->stride[comp];

    if (srcstride == deststride) {
      GST_TRACE_OBJECT (dec, "Stride matches. Comp %d: %d, copying full plane",
          comp, srcstride);
      memcpy (dest, src, srcstride * height);
    } else {
      GST_TRACE_OBJECT (dec, "Stride mismatch. Comp %d: %d != %d, copying "
          "line by line.", comp, srcstride, deststride);
      for (line = 0; line < height; line++) {
        memcpy (dest, src, width);
        dest += deststride;
        src += srcstride;
      }
    }
  }

  gst_video_frame_unmap (&frame);
}

static GstFlowReturn
gst_vpx_dec_open_codec (GstVPXDec * dec, GstVideoCodecFrame * frame)
{
  int flags = 0;
  vpx_codec_stream_info_t stream_info;
  vpx_codec_caps_t caps;
  vpx_codec_dec_cfg_t cfg;
  vpx_codec_err_t status;
  GstMapInfo minfo;
  GstVPXDecClass *vpxclass = GST_VPX_DEC_GET_CLASS (dec);

  g_return_val_if_fail (vpxclass->codec_algo != NULL, GST_FLOW_ERROR);

  memset (&stream_info, 0, sizeof (stream_info));
  memset (&cfg, 0, sizeof (cfg));
  stream_info.sz = sizeof (stream_info);

  if (!gst_buffer_map (frame->input_buffer, &minfo, GST_MAP_READ)) {
    GST_ERROR_OBJECT (dec, "Failed to map input buffer");
    return GST_FLOW_ERROR;
  }

  status = vpx_codec_peek_stream_info (vpxclass->codec_algo,
      minfo.data, minfo.size, &stream_info);

  gst_buffer_unmap (frame->input_buffer, &minfo);

  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (dec, "VPX preprocessing error: %s",
        gst_vpx_error_name (status));
    return GST_FLOW_CUSTOM_SUCCESS_1;
  }
  if (!stream_info.is_kf) {
    GST_WARNING_OBJECT (dec, "No keyframe, skipping");
    return GST_FLOW_CUSTOM_SUCCESS_1;
  }

  gst_vpx_dec_set_stream_info (dec, &stream_info);
  gst_vpx_dec_set_default_format (dec, GST_VIDEO_FORMAT_I420, stream_info.w,
      stream_info.h);

  cfg.w = stream_info.w;
  cfg.h = stream_info.h;

  if (dec->threads > 0)
    cfg.threads = dec->threads;
  else
    cfg.threads = g_get_num_processors ();

  caps = vpx_codec_get_caps (vpxclass->codec_algo);

  if (dec->post_processing) {
    if (!(caps & VPX_CODEC_CAP_POSTPROC)) {
      GST_WARNING_OBJECT (dec, "Decoder does not support post processing");
    } else {
      flags |= VPX_CODEC_USE_POSTPROC;
    }
  }

  status =
      vpx_codec_dec_init (&dec->decoder, vpxclass->codec_algo, &cfg, flags);
  if (status != VPX_CODEC_OK) {
    GST_ELEMENT_ERROR (dec, LIBRARY, INIT,
        ("Failed to initialize VP8 decoder"), ("%s",
            gst_vpx_error_name (status)));
    return GST_FLOW_ERROR;
  }

  if ((caps & VPX_CODEC_CAP_POSTPROC) && dec->post_processing) {
    vp8_postproc_cfg_t pp_cfg = { 0, };

    pp_cfg.post_proc_flag = dec->post_processing_flags;
    pp_cfg.deblocking_level = dec->deblocking_level;
    pp_cfg.noise_level = dec->noise_level;

    status = vpx_codec_control (&dec->decoder, VP8_SET_POSTPROC, &pp_cfg);
    if (status != VPX_CODEC_OK) {
      GST_WARNING_OBJECT (dec, "Couldn't set postprocessing settings: %s",
          gst_vpx_error_name (status));
    }
  }
  vpx_codec_set_frame_buffer_functions (&dec->decoder,
      gst_vpx_dec_get_buffer_cb, gst_vpx_dec_release_buffer_cb, dec);

  dec->decoder_inited = TRUE;

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_vpx_dec_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame)
{
  GstVPXDec *dec;
  GstFlowReturn ret = GST_FLOW_OK;
  vpx_codec_err_t status;
  vpx_codec_iter_t iter = NULL;
  vpx_image_t *img;
  long decoder_deadline = 0;
  GstClockTimeDiff deadline;
  GstMapInfo minfo;
  GstVPXDecClass *vpxclass;
  GstVideoFormat fmt;

  GST_LOG_OBJECT (decoder, "handle_frame");

  dec = GST_VPX_DEC (decoder);
  vpxclass = GST_VPX_DEC_GET_CLASS (dec);

  if (!dec->decoder_inited) {
    ret = vpxclass->open_codec (dec, frame);
    if (ret == GST_FLOW_CUSTOM_SUCCESS_1) {
      gst_video_decoder_drop_frame (decoder, frame);
      return GST_FLOW_OK;
    } else if (ret != GST_FLOW_OK) {
      gst_video_codec_frame_unref (frame);
      return ret;
    }
  }

  deadline = gst_video_decoder_get_max_decode_time (decoder, frame);
  if (deadline < 0) {
    decoder_deadline = 1;
  } else if (deadline == G_MAXINT64) {
    decoder_deadline = 0;
  } else {
    decoder_deadline = MAX (1, deadline / GST_MSECOND);
  }

  if (!gst_buffer_map (frame->input_buffer, &minfo, GST_MAP_READ)) {
    GST_ERROR_OBJECT (dec, "Failed to map input buffer");
    gst_video_codec_frame_unref (frame);
    return GST_FLOW_ERROR;
  }

  status = vpx_codec_decode (&dec->decoder,
      minfo.data, minfo.size, NULL, decoder_deadline);

  gst_buffer_unmap (frame->input_buffer, &minfo);

  if (status) {
    GST_VIDEO_DECODER_ERROR (decoder, 1, LIBRARY, ENCODE,
        ("Failed to decode frame"), ("%s", gst_vpx_error_name (status)), ret);
    gst_video_codec_frame_unref (frame);
    return ret;
  }

  img = vpx_codec_get_frame (&dec->decoder, &iter);
  if (img) {
    if (vpxclass->get_frame_format (dec, img, &fmt) == FALSE) {
      vpx_img_free (img);
      GST_ELEMENT_ERROR (decoder, LIBRARY, ENCODE,
          ("Failed to decode frame"), ("Unsupported color format %d",
              img->fmt));
      gst_video_codec_frame_unref (frame);
      return GST_FLOW_ERROR;
    }

    if (deadline < 0) {
      GST_LOG_OBJECT (dec, "Skipping late frame (%f s past deadline)",
          (double) -deadline / GST_SECOND);
      gst_video_decoder_drop_frame (decoder, frame);
    } else {
      gst_vpx_dec_handle_resolution_change (dec, img, fmt);
      if (img->fb_priv && dec->have_video_meta) {
        frame->output_buffer = gst_vpx_dec_prepare_image (dec, img);
        ret = gst_video_decoder_finish_frame (decoder, frame);
      } else {
        ret = gst_video_decoder_allocate_output_frame (decoder, frame);

        if (ret == GST_FLOW_OK) {
          gst_vpx_dec_image_to_buffer (dec, img, frame->output_buffer);
          ret = gst_video_decoder_finish_frame (decoder, frame);
        } else {
          gst_video_decoder_drop_frame (decoder, frame);
        }
      }
    }

    vpx_img_free (img);

    while ((img = vpx_codec_get_frame (&dec->decoder, &iter))) {
      GST_WARNING_OBJECT (decoder, "Multiple decoded frames... dropping");
      vpx_img_free (img);
    }
  } else {
    /* Invisible frame */
    GST_VIDEO_CODEC_FRAME_SET_DECODE_ONLY (frame);
    gst_video_decoder_finish_frame (decoder, frame);
  }

  return ret;
}

static gboolean
gst_vpx_dec_decide_allocation (GstVideoDecoder * bdec, GstQuery * query)
{
  GstVPXDec *dec = GST_VPX_DEC (bdec);
  GstBufferPool *pool;
  GstStructure *config;

  if (!GST_VIDEO_DECODER_CLASS (parent_class)->decide_allocation (bdec, query))
    return FALSE;

  g_assert (gst_query_get_n_allocation_pools (query) > 0);
  gst_query_parse_nth_allocation_pool (query, 0, &pool, NULL, NULL, NULL);
  g_assert (pool != NULL);

  config = gst_buffer_pool_get_config (pool);
  if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL)) {
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
    dec->have_video_meta = TRUE;
  }
  gst_buffer_pool_set_config (pool, config);
  gst_object_unref (pool);

  return TRUE;
}

static void
gst_vpx_dec_set_stream_info (GstVPXDec * dec,
    vpx_codec_stream_info_t * stream_info)
{
  GstVPXDecClass *vpxclass = GST_VPX_DEC_GET_CLASS (dec);
  if (vpxclass->set_stream_info != NULL) {
    vpxclass->set_stream_info (dec, stream_info);
  }
}

static void
gst_vpx_dec_set_default_format (GstVPXDec * dec, GstVideoFormat fmt, int width,
    int height)
{
  GstVPXDecClass *vpxclass = GST_VPX_DEC_GET_CLASS (dec);
  if (vpxclass->set_default_format != NULL) {
    vpxclass->set_default_format (dec, fmt, width, height);
  }
}

static gboolean
gst_vpx_dec_default_frame_format (GstVPXDec * dec, vpx_image_t * img,
    GstVideoFormat * fmt)
{
  if (img->fmt == VPX_IMG_FMT_I420) {
    *fmt = GST_VIDEO_FORMAT_I420;
    return TRUE;
  } else {
    return FALSE;
  }

}

static void
gst_vpx_dec_handle_resolution_change (GstVPXDec * dec, vpx_image_t * img,
    GstVideoFormat fmt)
{
  GstVPXDecClass *vpxclass = GST_VPX_DEC_GET_CLASS (dec);
  if (vpxclass->handle_resolution_change != NULL) {
    vpxclass->handle_resolution_change (dec, img, fmt);
  }
}

#endif /* HAVE_VP8_DECODER ||  HAVE_VP9_DECODER */
