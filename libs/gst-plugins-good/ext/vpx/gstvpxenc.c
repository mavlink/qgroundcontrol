/* VPX
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_VP8_ENCODER) || defined(HAVE_VP9_ENCODER)

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
#include "gstvpxenc.h"

GST_DEBUG_CATEGORY_STATIC (gst_vpxenc_debug);
#define GST_CAT_DEFAULT gst_vpxenc_debug

/* From vp8/vp8_cx_iface.c and vp9/vp9_cx_iface.c */
#define DEFAULT_PROFILE 0

#define DEFAULT_RC_END_USAGE VPX_VBR
#define DEFAULT_RC_TARGET_BITRATE 256000
#define DEFAULT_RC_MIN_QUANTIZER 4
#define DEFAULT_RC_MAX_QUANTIZER 63

#define DEFAULT_RC_DROPFRAME_THRESH 0
#define DEFAULT_RC_RESIZE_ALLOWED 0
#define DEFAULT_RC_RESIZE_UP_THRESH 30
#define DEFAULT_RC_RESIZE_DOWN_THRESH 60
#define DEFAULT_RC_UNDERSHOOT_PCT 100
#define DEFAULT_RC_OVERSHOOT_PCT 100
#define DEFAULT_RC_BUF_SZ 6000
#define DEFAULT_RC_BUF_INITIAL_SZ 4000
#define DEFAULT_RC_BUF_OPTIMAL_SZ 5000
#define DEFAULT_RC_2PASS_VBR_BIAS_PCT 50
#define DEFAULT_RC_2PASS_VBR_MINSECTION_PCT 0
#define DEFAULT_RC_2PASS_VBR_MAXSECTION_PCT 400

#define DEFAULT_KF_MODE VPX_KF_AUTO
#define DEFAULT_KF_MAX_DIST 128

#define DEFAULT_MULTIPASS_MODE VPX_RC_ONE_PASS
#define DEFAULT_MULTIPASS_CACHE_FILE "multipass.cache"

#define DEFAULT_TS_NUMBER_LAYERS 1
#define DEFAULT_TS_TARGET_BITRATE NULL
#define DEFAULT_TS_RATE_DECIMATOR NULL
#define DEFAULT_TS_PERIODICITY 0
#define DEFAULT_TS_LAYER_ID NULL

#define DEFAULT_ERROR_RESILIENT 0
#define DEFAULT_LAG_IN_FRAMES 0

#define DEFAULT_THREADS 0

#define DEFAULT_H_SCALING_MODE VP8E_NORMAL
#define DEFAULT_V_SCALING_MODE VP8E_NORMAL
#define DEFAULT_CPU_USED 0
#define DEFAULT_ENABLE_AUTO_ALT_REF FALSE
#define DEFAULT_DEADLINE VPX_DL_BEST_QUALITY
#define DEFAULT_NOISE_SENSITIVITY 0
#define DEFAULT_SHARPNESS 0
#define DEFAULT_STATIC_THRESHOLD 0
#define DEFAULT_TOKEN_PARTITIONS 0
#define DEFAULT_ARNR_MAXFRAMES 0
#define DEFAULT_ARNR_STRENGTH 3
#define DEFAULT_ARNR_TYPE 3
#define DEFAULT_TUNING VP8_TUNE_PSNR
#define DEFAULT_CQ_LEVEL 10
#define DEFAULT_MAX_INTRA_BITRATE_PCT 0
#define DEFAULT_TIMEBASE_N 0
#define DEFAULT_TIMEBASE_D 1

enum
{
  PROP_0,
  PROP_RC_END_USAGE,
  PROP_RC_TARGET_BITRATE,
  PROP_RC_MIN_QUANTIZER,
  PROP_RC_MAX_QUANTIZER,
  PROP_RC_DROPFRAME_THRESH,
  PROP_RC_RESIZE_ALLOWED,
  PROP_RC_RESIZE_UP_THRESH,
  PROP_RC_RESIZE_DOWN_THRESH,
  PROP_RC_UNDERSHOOT_PCT,
  PROP_RC_OVERSHOOT_PCT,
  PROP_RC_BUF_SZ,
  PROP_RC_BUF_INITIAL_SZ,
  PROP_RC_BUF_OPTIMAL_SZ,
  PROP_RC_2PASS_VBR_BIAS_PCT,
  PROP_RC_2PASS_VBR_MINSECTION_PCT,
  PROP_RC_2PASS_VBR_MAXSECTION_PCT,
  PROP_KF_MODE,
  PROP_KF_MAX_DIST,
  PROP_TS_NUMBER_LAYERS,
  PROP_TS_TARGET_BITRATE,
  PROP_TS_RATE_DECIMATOR,
  PROP_TS_PERIODICITY,
  PROP_TS_LAYER_ID,
  PROP_MULTIPASS_MODE,
  PROP_MULTIPASS_CACHE_FILE,
  PROP_ERROR_RESILIENT,
  PROP_LAG_IN_FRAMES,
  PROP_THREADS,
  PROP_DEADLINE,
  PROP_H_SCALING_MODE,
  PROP_V_SCALING_MODE,
  PROP_CPU_USED,
  PROP_ENABLE_AUTO_ALT_REF,
  PROP_NOISE_SENSITIVITY,
  PROP_SHARPNESS,
  PROP_STATIC_THRESHOLD,
  PROP_TOKEN_PARTITIONS,
  PROP_ARNR_MAXFRAMES,
  PROP_ARNR_STRENGTH,
  PROP_ARNR_TYPE,
  PROP_TUNING,
  PROP_CQ_LEVEL,
  PROP_MAX_INTRA_BITRATE_PCT,
  PROP_TIMEBASE
};


#define GST_VPX_ENC_END_USAGE_TYPE (gst_vpx_enc_end_usage_get_type())
static GType
gst_vpx_enc_end_usage_get_type (void)
{
  static const GEnumValue values[] = {
    {VPX_VBR, "Variable Bit Rate (VBR) mode", "vbr"},
    {VPX_CBR, "Constant Bit Rate (CBR) mode", "cbr"},
    {VPX_CQ, "Constant Quality Mode (CQ) mode", "cq"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstVPXEncEndUsage", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define GST_VPX_ENC_MULTIPASS_MODE_TYPE (gst_vpx_enc_multipass_mode_get_type())
static GType
gst_vpx_enc_multipass_mode_get_type (void)
{
  static const GEnumValue values[] = {
    {VPX_RC_ONE_PASS, "One pass encoding (default)", "one-pass"},
    {VPX_RC_FIRST_PASS, "First pass of multipass encoding", "first-pass"},
    {VPX_RC_LAST_PASS, "Last pass of multipass encoding", "last-pass"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstVPXEncMultipassMode", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define GST_VPX_ENC_KF_MODE_TYPE (gst_vpx_enc_kf_mode_get_type())
static GType
gst_vpx_enc_kf_mode_get_type (void)
{
  static const GEnumValue values[] = {
    {VPX_KF_AUTO, "Determine optimal placement automatically", "auto"},
    {VPX_KF_DISABLED, "Don't automatically place keyframes", "disabled"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstVPXEncKfMode", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define GST_VPX_ENC_TUNING_TYPE (gst_vpx_enc_tuning_get_type())
static GType
gst_vpx_enc_tuning_get_type (void)
{
  static const GEnumValue values[] = {
    {VP8_TUNE_PSNR, "Tune for PSNR", "psnr"},
    {VP8_TUNE_SSIM, "Tune for SSIM", "ssim"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstVPXEncTuning", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define GST_VPX_ENC_SCALING_MODE_TYPE (gst_vpx_enc_scaling_mode_get_type())
static GType
gst_vpx_enc_scaling_mode_get_type (void)
{
  static const GEnumValue values[] = {
    {VP8E_NORMAL, "Normal", "normal"},
    {VP8E_FOURFIVE, "4:5", "4:5"},
    {VP8E_THREEFIVE, "3:5", "3:5"},
    {VP8E_ONETWO, "1:2", "1:2"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstVPXEncScalingMode", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define GST_VPX_ENC_TOKEN_PARTITIONS_TYPE (gst_vpx_enc_token_partitions_get_type())
static GType
gst_vpx_enc_token_partitions_get_type (void)
{
  static const GEnumValue values[] = {
    {VP8_ONE_TOKENPARTITION, "One token partition", "1"},
    {VP8_TWO_TOKENPARTITION, "Two token partitions", "2"},
    {VP8_FOUR_TOKENPARTITION, "Four token partitions", "4"},
    {VP8_EIGHT_TOKENPARTITION, "Eight token partitions", "8"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_enum_register_static ("GstVPXEncTokenPartitions", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

#define GST_VPX_ENC_ER_FLAGS_TYPE (gst_vpx_enc_er_flags_get_type())
static GType
gst_vpx_enc_er_flags_get_type (void)
{
  static const GFlagsValue values[] = {
    {VPX_ERROR_RESILIENT_DEFAULT, "Default error resilience", "default"},
    {VPX_ERROR_RESILIENT_PARTITIONS,
        "Allow partitions to be decoded independently", "partitions"},
    {0, NULL, NULL}
  };
  static volatile GType id = 0;

  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;

    _id = g_flags_register_static ("GstVPXEncErFlags", values);

    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}

static void gst_vpx_enc_finalize (GObject * object);
static void gst_vpx_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_vpx_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_vpx_enc_start (GstVideoEncoder * encoder);
static gboolean gst_vpx_enc_stop (GstVideoEncoder * encoder);
static gboolean gst_vpx_enc_set_format (GstVideoEncoder *
    video_encoder, GstVideoCodecState * state);
static GstFlowReturn gst_vpx_enc_finish (GstVideoEncoder * video_encoder);
static gboolean gst_vpx_enc_flush (GstVideoEncoder * video_encoder);
static GstFlowReturn gst_vpx_enc_drain (GstVideoEncoder * video_encoder);
static GstFlowReturn gst_vpx_enc_handle_frame (GstVideoEncoder *
    video_encoder, GstVideoCodecFrame * frame);
static gboolean gst_vpx_enc_sink_event (GstVideoEncoder *
    video_encoder, GstEvent * event);
static gboolean gst_vpx_enc_propose_allocation (GstVideoEncoder * encoder,
    GstQuery * query);

#define parent_class gst_vpx_enc_parent_class
G_DEFINE_TYPE_WITH_CODE (GstVPXEnc, gst_vpx_enc, GST_TYPE_VIDEO_ENCODER,
    G_IMPLEMENT_INTERFACE (GST_TYPE_TAG_SETTER, NULL);
    G_IMPLEMENT_INTERFACE (GST_TYPE_PRESET, NULL););

static void
gst_vpx_enc_class_init (GstVPXEncClass * klass)
{
  GObjectClass *gobject_class;
  GstVideoEncoderClass *video_encoder_class;

  gobject_class = G_OBJECT_CLASS (klass);
  video_encoder_class = GST_VIDEO_ENCODER_CLASS (klass);

  gobject_class->set_property = gst_vpx_enc_set_property;
  gobject_class->get_property = gst_vpx_enc_get_property;
  gobject_class->finalize = gst_vpx_enc_finalize;

  video_encoder_class->start = gst_vpx_enc_start;
  video_encoder_class->stop = gst_vpx_enc_stop;
  video_encoder_class->handle_frame = gst_vpx_enc_handle_frame;
  video_encoder_class->set_format = gst_vpx_enc_set_format;
  video_encoder_class->flush = gst_vpx_enc_flush;
  video_encoder_class->finish = gst_vpx_enc_finish;
  video_encoder_class->sink_event = gst_vpx_enc_sink_event;
  video_encoder_class->propose_allocation = gst_vpx_enc_propose_allocation;

  g_object_class_install_property (gobject_class, PROP_RC_END_USAGE,
      g_param_spec_enum ("end-usage", "Rate control mode",
          "Rate control mode",
          GST_VPX_ENC_END_USAGE_TYPE, DEFAULT_RC_END_USAGE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_TARGET_BITRATE,
      g_param_spec_int ("target-bitrate", "Target bitrate",
          "Target bitrate (in bits/sec)",
          0, G_MAXINT, DEFAULT_RC_TARGET_BITRATE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_MIN_QUANTIZER,
      g_param_spec_int ("min-quantizer", "Minimum Quantizer",
          "Minimum Quantizer (best)",
          0, 63, DEFAULT_RC_MIN_QUANTIZER,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_MAX_QUANTIZER,
      g_param_spec_int ("max-quantizer", "Maximum Quantizer",
          "Maximum Quantizer (worst)",
          0, 63, DEFAULT_RC_MAX_QUANTIZER,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_DROPFRAME_THRESH,
      g_param_spec_int ("dropframe-threshold", "Drop Frame Threshold",
          "Temporal resampling threshold (buf %)",
          0, 100, DEFAULT_RC_DROPFRAME_THRESH,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_RESIZE_ALLOWED,
      g_param_spec_boolean ("resize-allowed", "Resize Allowed",
          "Allow spatial resampling",
          DEFAULT_RC_RESIZE_ALLOWED,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_RESIZE_UP_THRESH,
      g_param_spec_int ("resize-up-threshold", "Resize Up Threshold",
          "Upscale threshold (buf %)",
          0, 100, DEFAULT_RC_RESIZE_UP_THRESH,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_RESIZE_DOWN_THRESH,
      g_param_spec_int ("resize-down-threshold", "Resize Down Threshold",
          "Downscale threshold (buf %)",
          0, 100, DEFAULT_RC_RESIZE_DOWN_THRESH,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_UNDERSHOOT_PCT,
      g_param_spec_int ("undershoot", "Undershoot PCT",
          "Datarate undershoot (min) target (%)",
          0, 1000, DEFAULT_RC_UNDERSHOOT_PCT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_OVERSHOOT_PCT,
      g_param_spec_int ("overshoot", "Overshoot PCT",
          "Datarate overshoot (max) target (%)",
          0, 1000, DEFAULT_RC_OVERSHOOT_PCT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_BUF_SZ,
      g_param_spec_int ("buffer-size", "Buffer size",
          "Client buffer size (ms)",
          0, G_MAXINT, DEFAULT_RC_BUF_SZ,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_BUF_INITIAL_SZ,
      g_param_spec_int ("buffer-initial-size", "Buffer initial size",
          "Initial client buffer size (ms)",
          0, G_MAXINT, DEFAULT_RC_BUF_INITIAL_SZ,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_BUF_OPTIMAL_SZ,
      g_param_spec_int ("buffer-optimal-size", "Buffer optimal size",
          "Optimal client buffer size (ms)",
          0, G_MAXINT, DEFAULT_RC_BUF_OPTIMAL_SZ,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_RC_2PASS_VBR_BIAS_PCT,
      g_param_spec_int ("twopass-vbr-bias", "2-pass VBR bias",
          "CBR/VBR bias (0=CBR, 100=VBR)",
          0, 100, DEFAULT_RC_2PASS_VBR_BIAS_PCT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class,
      PROP_RC_2PASS_VBR_MINSECTION_PCT,
      g_param_spec_int ("twopass-vbr-minsection", "2-pass GOP min bitrate",
          "GOP minimum bitrate (% target)", 0, G_MAXINT,
          DEFAULT_RC_2PASS_VBR_MINSECTION_PCT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class,
      PROP_RC_2PASS_VBR_MAXSECTION_PCT,
      g_param_spec_int ("twopass-vbr-maxsection", "2-pass GOP max bitrate",
          "GOP maximum bitrate (% target)", 0, G_MAXINT,
          DEFAULT_RC_2PASS_VBR_MINSECTION_PCT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_KF_MODE,
      g_param_spec_enum ("keyframe-mode", "Keyframe Mode",
          "Keyframe placement",
          GST_VPX_ENC_KF_MODE_TYPE, DEFAULT_KF_MODE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_KF_MAX_DIST,
      g_param_spec_int ("keyframe-max-dist", "Keyframe max distance",
          "Maximum distance between keyframes (number of frames)",
          0, G_MAXINT, DEFAULT_KF_MAX_DIST,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_MULTIPASS_MODE,
      g_param_spec_enum ("multipass-mode", "Multipass Mode",
          "Multipass encode mode",
          GST_VPX_ENC_MULTIPASS_MODE_TYPE, DEFAULT_MULTIPASS_MODE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_MULTIPASS_CACHE_FILE,
      g_param_spec_string ("multipass-cache-file", "Multipass Cache File",
          "Multipass cache file. "
          "If stream caps reinited, multiple files will be created: "
          "file, file.1, file.2, ... and so on.",
          DEFAULT_MULTIPASS_CACHE_FILE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_TS_NUMBER_LAYERS,
      g_param_spec_int ("temporal-scalability-number-layers",
          "Number of coding layers", "Number of coding layers to use", 1, 5,
          DEFAULT_TS_NUMBER_LAYERS,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_TS_TARGET_BITRATE,
      g_param_spec_value_array ("temporal-scalability-target-bitrate",
          "Coding layer target bitrates",
          "Target bitrates for coding layers (one per layer, decreasing)",
          g_param_spec_int ("target-bitrate", "Target bitrate",
              "Target bitrate", 0, G_MAXINT, DEFAULT_RC_TARGET_BITRATE,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TS_RATE_DECIMATOR,
      g_param_spec_value_array ("temporal-scalability-rate-decimator",
          "Coding layer rate decimator",
          "Rate decimation factors for each layer",
          g_param_spec_int ("rate-decimator", "Rate decimator",
              "Rate decimator", 0, 1000000000, 0,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TS_PERIODICITY,
      g_param_spec_int ("temporal-scalability-periodicity",
          "Coding layer periodicity",
          "Length of sequence that defines layer membership periodicity", 0, 16,
          DEFAULT_TS_PERIODICITY,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_TS_LAYER_ID,
      g_param_spec_value_array ("temporal-scalability-layer-id",
          "Coding layer identification",
          "Sequence defining coding layer membership",
          g_param_spec_int ("layer-id", "Layer ID", "Layer ID", 0, 4, 0,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LAG_IN_FRAMES,
      g_param_spec_int ("lag-in-frames", "Lag in frames",
          "Maximum number of frames to lag",
          0, 25, DEFAULT_LAG_IN_FRAMES,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ERROR_RESILIENT,
      g_param_spec_flags ("error-resilient", "Error resilient",
          "Error resilience flags",
          GST_VPX_ENC_ER_FLAGS_TYPE, DEFAULT_ERROR_RESILIENT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_THREADS,
      g_param_spec_int ("threads", "Threads",
          "Number of threads to use",
          0, 64, DEFAULT_THREADS,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_DEADLINE,
      g_param_spec_int64 ("deadline", "Deadline",
          "Deadline per frame (usec, 0=disabled)",
          0, G_MAXINT64, DEFAULT_DEADLINE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_H_SCALING_MODE,
      g_param_spec_enum ("horizontal-scaling-mode", "Horizontal scaling mode",
          "Horizontal scaling mode",
          GST_VPX_ENC_SCALING_MODE_TYPE, DEFAULT_H_SCALING_MODE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_V_SCALING_MODE,
      g_param_spec_enum ("vertical-scaling-mode", "Vertical scaling mode",
          "Vertical scaling mode",
          GST_VPX_ENC_SCALING_MODE_TYPE, DEFAULT_V_SCALING_MODE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_CPU_USED,
      g_param_spec_int ("cpu-used", "CPU used",
          "CPU used",
          -16, 16, DEFAULT_CPU_USED,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ENABLE_AUTO_ALT_REF,
      g_param_spec_boolean ("auto-alt-ref", "Auto alt reference frames",
          "Automatically generate AltRef frames",
          DEFAULT_ENABLE_AUTO_ALT_REF,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_NOISE_SENSITIVITY,
      g_param_spec_int ("noise-sensitivity", "Noise sensitivity",
          "Noise sensisivity (frames to blur)",
          0, 6, DEFAULT_NOISE_SENSITIVITY,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_SHARPNESS,
      g_param_spec_int ("sharpness", "Sharpness",
          "Filter sharpness",
          0, 7, DEFAULT_SHARPNESS,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_STATIC_THRESHOLD,
      g_param_spec_int ("static-threshold", "Static Threshold",
          "Motion detection threshold",
          0, G_MAXINT, DEFAULT_STATIC_THRESHOLD,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_TOKEN_PARTITIONS,
      g_param_spec_enum ("token-partitions", "Token partitions",
          "Number of token partitions",
          GST_VPX_ENC_TOKEN_PARTITIONS_TYPE, DEFAULT_TOKEN_PARTITIONS,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ARNR_MAXFRAMES,
      g_param_spec_int ("arnr-maxframes", "AltRef max frames",
          "AltRef maximum number of frames",
          0, 15, DEFAULT_ARNR_MAXFRAMES,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ARNR_STRENGTH,
      g_param_spec_int ("arnr-strength", "AltRef strength",
          "AltRef strength",
          0, 6, DEFAULT_ARNR_STRENGTH,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ARNR_TYPE,
      g_param_spec_int ("arnr-type", "AltRef type",
          "AltRef type",
          1, 3, DEFAULT_ARNR_TYPE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
              G_PARAM_DEPRECATED)));

  g_object_class_install_property (gobject_class, PROP_TUNING,
      g_param_spec_enum ("tuning", "Tuning",
          "Tuning",
          GST_VPX_ENC_TUNING_TYPE, DEFAULT_TUNING,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_CQ_LEVEL,
      g_param_spec_int ("cq-level", "Constrained quality level",
          "Constrained quality level",
          0, 63, DEFAULT_CQ_LEVEL,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_MAX_INTRA_BITRATE_PCT,
      g_param_spec_int ("max-intra-bitrate", "Max Intra bitrate",
          "Maximum Intra frame bitrate",
          0, G_MAXINT, DEFAULT_MAX_INTRA_BITRATE_PCT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_TIMEBASE,
      gst_param_spec_fraction ("timebase", "Shortest interframe time",
          "Fraction of one second that is the shortest interframe time - normally left as zero which will default to the framerate",
          0, 1, G_MAXINT, 1, DEFAULT_TIMEBASE_N, DEFAULT_TIMEBASE_D,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (gst_vpxenc_debug, "vpxenc", 0, "VPX Encoder");
}

static void
gst_vpx_enc_init (GstVPXEnc * gst_vpx_enc)
{
  GST_DEBUG_OBJECT (gst_vpx_enc, "init");
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_ENCODER_SINK_PAD (gst_vpx_enc));

  gst_vpx_enc->cfg.rc_end_usage = DEFAULT_RC_END_USAGE;
  gst_vpx_enc->cfg.rc_target_bitrate = DEFAULT_RC_TARGET_BITRATE / 1000;
  gst_vpx_enc->rc_target_bitrate_set = FALSE;
  gst_vpx_enc->cfg.rc_min_quantizer = DEFAULT_RC_MIN_QUANTIZER;
  gst_vpx_enc->cfg.rc_max_quantizer = DEFAULT_RC_MAX_QUANTIZER;
  gst_vpx_enc->cfg.rc_dropframe_thresh = DEFAULT_RC_DROPFRAME_THRESH;
  gst_vpx_enc->cfg.rc_resize_allowed = DEFAULT_RC_RESIZE_ALLOWED;
  gst_vpx_enc->cfg.rc_resize_up_thresh = DEFAULT_RC_RESIZE_UP_THRESH;
  gst_vpx_enc->cfg.rc_resize_down_thresh = DEFAULT_RC_RESIZE_DOWN_THRESH;
  gst_vpx_enc->cfg.rc_undershoot_pct = DEFAULT_RC_UNDERSHOOT_PCT;
  gst_vpx_enc->cfg.rc_overshoot_pct = DEFAULT_RC_OVERSHOOT_PCT;
  gst_vpx_enc->cfg.rc_buf_sz = DEFAULT_RC_BUF_SZ;
  gst_vpx_enc->cfg.rc_buf_initial_sz = DEFAULT_RC_BUF_INITIAL_SZ;
  gst_vpx_enc->cfg.rc_buf_optimal_sz = DEFAULT_RC_BUF_OPTIMAL_SZ;
  gst_vpx_enc->cfg.rc_2pass_vbr_bias_pct = DEFAULT_RC_2PASS_VBR_BIAS_PCT;
  gst_vpx_enc->cfg.rc_2pass_vbr_minsection_pct =
      DEFAULT_RC_2PASS_VBR_MINSECTION_PCT;
  gst_vpx_enc->cfg.rc_2pass_vbr_maxsection_pct =
      DEFAULT_RC_2PASS_VBR_MAXSECTION_PCT;
  gst_vpx_enc->cfg.kf_mode = DEFAULT_KF_MODE;
  gst_vpx_enc->cfg.kf_max_dist = DEFAULT_KF_MAX_DIST;
  gst_vpx_enc->cfg.g_pass = DEFAULT_MULTIPASS_MODE;
  gst_vpx_enc->multipass_cache_prefix = g_strdup (DEFAULT_MULTIPASS_CACHE_FILE);
  gst_vpx_enc->multipass_cache_file = NULL;
  gst_vpx_enc->multipass_cache_idx = 0;
  gst_vpx_enc->cfg.ts_number_layers = DEFAULT_TS_NUMBER_LAYERS;
  gst_vpx_enc->n_ts_target_bitrate = 0;
  gst_vpx_enc->n_ts_rate_decimator = 0;
  gst_vpx_enc->cfg.ts_periodicity = DEFAULT_TS_PERIODICITY;
  gst_vpx_enc->n_ts_layer_id = 0;
  gst_vpx_enc->cfg.g_error_resilient = DEFAULT_ERROR_RESILIENT;
  gst_vpx_enc->cfg.g_lag_in_frames = DEFAULT_LAG_IN_FRAMES;
  gst_vpx_enc->cfg.g_threads = DEFAULT_THREADS;
  gst_vpx_enc->deadline = DEFAULT_DEADLINE;
  gst_vpx_enc->h_scaling_mode = DEFAULT_H_SCALING_MODE;
  gst_vpx_enc->v_scaling_mode = DEFAULT_V_SCALING_MODE;
  gst_vpx_enc->cpu_used = DEFAULT_CPU_USED;
  gst_vpx_enc->enable_auto_alt_ref = DEFAULT_ENABLE_AUTO_ALT_REF;
  gst_vpx_enc->noise_sensitivity = DEFAULT_NOISE_SENSITIVITY;
  gst_vpx_enc->sharpness = DEFAULT_SHARPNESS;
  gst_vpx_enc->static_threshold = DEFAULT_STATIC_THRESHOLD;
  gst_vpx_enc->token_partitions = DEFAULT_TOKEN_PARTITIONS;
  gst_vpx_enc->arnr_maxframes = DEFAULT_ARNR_MAXFRAMES;
  gst_vpx_enc->arnr_strength = DEFAULT_ARNR_STRENGTH;
  gst_vpx_enc->arnr_type = DEFAULT_ARNR_TYPE;
  gst_vpx_enc->tuning = DEFAULT_TUNING;
  gst_vpx_enc->cq_level = DEFAULT_CQ_LEVEL;
  gst_vpx_enc->max_intra_bitrate_pct = DEFAULT_MAX_INTRA_BITRATE_PCT;
  gst_vpx_enc->timebase_n = DEFAULT_TIMEBASE_N;
  gst_vpx_enc->timebase_d = DEFAULT_TIMEBASE_D;

  gst_vpx_enc->cfg.g_profile = DEFAULT_PROFILE;

  g_mutex_init (&gst_vpx_enc->encoder_lock);
}

static void
gst_vpx_enc_finalize (GObject * object)
{
  GstVPXEnc *gst_vpx_enc;

  GST_DEBUG_OBJECT (object, "finalize");

  g_return_if_fail (GST_IS_VPX_ENC (object));
  gst_vpx_enc = GST_VPX_ENC (object);

  g_free (gst_vpx_enc->multipass_cache_prefix);
  g_free (gst_vpx_enc->multipass_cache_file);
  gst_vpx_enc->multipass_cache_idx = 0;


  if (gst_vpx_enc->input_state)
    gst_video_codec_state_unref (gst_vpx_enc->input_state);

  g_mutex_clear (&gst_vpx_enc->encoder_lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_vpx_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVPXEnc *gst_vpx_enc;
  gboolean global = FALSE;
  vpx_codec_err_t status;

  g_return_if_fail (GST_IS_VPX_ENC (object));
  gst_vpx_enc = GST_VPX_ENC (object);

  GST_DEBUG_OBJECT (object, "gst_vpx_enc_set_property");
  g_mutex_lock (&gst_vpx_enc->encoder_lock);
  switch (prop_id) {
    case PROP_RC_END_USAGE:
      gst_vpx_enc->cfg.rc_end_usage = g_value_get_enum (value);
      global = TRUE;
      break;
    case PROP_RC_TARGET_BITRATE:
      gst_vpx_enc->cfg.rc_target_bitrate = g_value_get_int (value) / 1000;
      gst_vpx_enc->rc_target_bitrate_set = TRUE;
      global = TRUE;
      break;
    case PROP_RC_MIN_QUANTIZER:
      gst_vpx_enc->cfg.rc_min_quantizer = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_MAX_QUANTIZER:
      gst_vpx_enc->cfg.rc_max_quantizer = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_DROPFRAME_THRESH:
      gst_vpx_enc->cfg.rc_dropframe_thresh = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_RESIZE_ALLOWED:
      gst_vpx_enc->cfg.rc_resize_allowed = g_value_get_boolean (value);
      global = TRUE;
      break;
    case PROP_RC_RESIZE_UP_THRESH:
      gst_vpx_enc->cfg.rc_resize_up_thresh = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_RESIZE_DOWN_THRESH:
      gst_vpx_enc->cfg.rc_resize_down_thresh = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_UNDERSHOOT_PCT:
      gst_vpx_enc->cfg.rc_undershoot_pct = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_OVERSHOOT_PCT:
      gst_vpx_enc->cfg.rc_overshoot_pct = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_BUF_SZ:
      gst_vpx_enc->cfg.rc_buf_sz = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_BUF_INITIAL_SZ:
      gst_vpx_enc->cfg.rc_buf_initial_sz = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_BUF_OPTIMAL_SZ:
      gst_vpx_enc->cfg.rc_buf_optimal_sz = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_2PASS_VBR_BIAS_PCT:
      gst_vpx_enc->cfg.rc_2pass_vbr_bias_pct = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_2PASS_VBR_MINSECTION_PCT:
      gst_vpx_enc->cfg.rc_2pass_vbr_minsection_pct = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_RC_2PASS_VBR_MAXSECTION_PCT:
      gst_vpx_enc->cfg.rc_2pass_vbr_maxsection_pct = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_KF_MODE:
      gst_vpx_enc->cfg.kf_mode = g_value_get_enum (value);
      global = TRUE;
      break;
    case PROP_KF_MAX_DIST:
      gst_vpx_enc->cfg.kf_max_dist = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_MULTIPASS_MODE:
      gst_vpx_enc->cfg.g_pass = g_value_get_enum (value);
      global = TRUE;
      break;
    case PROP_MULTIPASS_CACHE_FILE:
      if (gst_vpx_enc->multipass_cache_prefix)
        g_free (gst_vpx_enc->multipass_cache_prefix);
      gst_vpx_enc->multipass_cache_prefix = g_value_dup_string (value);
      break;
    case PROP_TS_NUMBER_LAYERS:
      gst_vpx_enc->cfg.ts_number_layers = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_TS_TARGET_BITRATE:{
      GValueArray *va = g_value_get_boxed (value);

      memset (&gst_vpx_enc->cfg.ts_target_bitrate, 0,
          sizeof (gst_vpx_enc->cfg.ts_target_bitrate));
      if (va == NULL) {
        gst_vpx_enc->n_ts_target_bitrate = 0;
      } else if (va->n_values > VPX_TS_MAX_LAYERS) {
        g_warning ("%s: Only %d layers allowed at maximum",
            GST_ELEMENT_NAME (gst_vpx_enc), VPX_TS_MAX_LAYERS);
      } else {
        gint i;

        for (i = 0; i < va->n_values; i++)
          gst_vpx_enc->cfg.ts_target_bitrate[i] =
              g_value_get_int (g_value_array_get_nth (va, i));
        gst_vpx_enc->n_ts_target_bitrate = va->n_values;
      }
      global = TRUE;
      break;
    }
    case PROP_TS_RATE_DECIMATOR:{
      GValueArray *va = g_value_get_boxed (value);

      memset (&gst_vpx_enc->cfg.ts_rate_decimator, 0,
          sizeof (gst_vpx_enc->cfg.ts_rate_decimator));
      if (va == NULL) {
        gst_vpx_enc->n_ts_rate_decimator = 0;
      } else if (va->n_values > VPX_TS_MAX_LAYERS) {
        g_warning ("%s: Only %d layers allowed at maximum",
            GST_ELEMENT_NAME (gst_vpx_enc), VPX_TS_MAX_LAYERS);
      } else {
        gint i;

        for (i = 0; i < va->n_values; i++)
          gst_vpx_enc->cfg.ts_rate_decimator[i] =
              g_value_get_int (g_value_array_get_nth (va, i));
        gst_vpx_enc->n_ts_rate_decimator = va->n_values;
      }
      global = TRUE;
      break;
    }
    case PROP_TS_PERIODICITY:
      gst_vpx_enc->cfg.ts_periodicity = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_TS_LAYER_ID:{
      GValueArray *va = g_value_get_boxed (value);

      memset (&gst_vpx_enc->cfg.ts_layer_id, 0,
          sizeof (gst_vpx_enc->cfg.ts_layer_id));
      if (va && va->n_values > VPX_TS_MAX_PERIODICITY) {
        g_warning ("%s: Only %d sized layer sequences allowed at maximum",
            GST_ELEMENT_NAME (gst_vpx_enc), VPX_TS_MAX_PERIODICITY);
      } else if (va) {
        gint i;

        for (i = 0; i < va->n_values; i++)
          gst_vpx_enc->cfg.ts_layer_id[i] =
              g_value_get_int (g_value_array_get_nth (va, i));
        gst_vpx_enc->n_ts_layer_id = va->n_values;
      } else {
        gst_vpx_enc->n_ts_layer_id = 0;
      }
      global = TRUE;
      break;
    }
    case PROP_ERROR_RESILIENT:
      gst_vpx_enc->cfg.g_error_resilient = g_value_get_flags (value);
      global = TRUE;
      break;
    case PROP_LAG_IN_FRAMES:
      gst_vpx_enc->cfg.g_lag_in_frames = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_THREADS:
      gst_vpx_enc->cfg.g_threads = g_value_get_int (value);
      global = TRUE;
      break;
    case PROP_DEADLINE:
      gst_vpx_enc->deadline = g_value_get_int64 (value);
      break;
    case PROP_H_SCALING_MODE:
      gst_vpx_enc->h_scaling_mode = g_value_get_enum (value);
      if (gst_vpx_enc->inited) {
        vpx_scaling_mode_t sm;

        sm.h_scaling_mode = gst_vpx_enc->h_scaling_mode;
        sm.v_scaling_mode = gst_vpx_enc->v_scaling_mode;

        status =
            vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_SCALEMODE, &sm);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_SCALEMODE: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_V_SCALING_MODE:
      gst_vpx_enc->v_scaling_mode = g_value_get_enum (value);
      if (gst_vpx_enc->inited) {
        vpx_scaling_mode_t sm;

        sm.h_scaling_mode = gst_vpx_enc->h_scaling_mode;
        sm.v_scaling_mode = gst_vpx_enc->v_scaling_mode;

        status =
            vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_SCALEMODE, &sm);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_SCALEMODE: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_CPU_USED:
      gst_vpx_enc->cpu_used = g_value_get_int (value);
      if (gst_vpx_enc->inited) {
        status =
            vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_CPUUSED,
            gst_vpx_enc->cpu_used);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc, "Failed to set VP8E_SET_CPUUSED: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_ENABLE_AUTO_ALT_REF:
      gst_vpx_enc->enable_auto_alt_ref = g_value_get_boolean (value);
      if (gst_vpx_enc->inited) {
        status =
            vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_ENABLEAUTOALTREF,
            (gst_vpx_enc->enable_auto_alt_ref ? 1 : 0));
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_ENABLEAUTOALTREF: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_NOISE_SENSITIVITY:
      gst_vpx_enc->noise_sensitivity = g_value_get_int (value);
      if (gst_vpx_enc->inited) {
        status =
            vpx_codec_control (&gst_vpx_enc->encoder,
            VP8E_SET_NOISE_SENSITIVITY, gst_vpx_enc->noise_sensitivity);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_NOISE_SENSITIVITY: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_SHARPNESS:
      gst_vpx_enc->sharpness = g_value_get_int (value);
      if (gst_vpx_enc->inited) {
        status = vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_SHARPNESS,
            gst_vpx_enc->sharpness);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_SHARPNESS: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_STATIC_THRESHOLD:
      gst_vpx_enc->static_threshold = g_value_get_int (value);
      if (gst_vpx_enc->inited) {
        status =
            vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_STATIC_THRESHOLD,
            gst_vpx_enc->static_threshold);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_STATIC_THRESHOLD: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_TOKEN_PARTITIONS:
      gst_vpx_enc->token_partitions = g_value_get_enum (value);
      if (gst_vpx_enc->inited) {
        status =
            vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_TOKEN_PARTITIONS,
            gst_vpx_enc->token_partitions);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_TOKEN_PARTIONS: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_ARNR_MAXFRAMES:
      gst_vpx_enc->arnr_maxframes = g_value_get_int (value);
      if (gst_vpx_enc->inited) {
        status =
            vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_ARNR_MAXFRAMES,
            gst_vpx_enc->arnr_maxframes);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_ARNR_MAXFRAMES: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_ARNR_STRENGTH:
      gst_vpx_enc->arnr_strength = g_value_get_int (value);
      if (gst_vpx_enc->inited) {
        status =
            vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_ARNR_STRENGTH,
            gst_vpx_enc->arnr_strength);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_ARNR_STRENGTH: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_ARNR_TYPE:
      gst_vpx_enc->arnr_type = g_value_get_int (value);
      g_warning ("arnr-type is a no-op since control has been deprecated "
          "in libvpx");
      break;
    case PROP_TUNING:
      gst_vpx_enc->tuning = g_value_get_enum (value);
      if (gst_vpx_enc->inited) {
        status = vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_TUNING,
            gst_vpx_enc->tuning);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_TUNING: %s", gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_CQ_LEVEL:
      gst_vpx_enc->cq_level = g_value_get_int (value);
      if (gst_vpx_enc->inited) {
        status = vpx_codec_control (&gst_vpx_enc->encoder, VP8E_SET_CQ_LEVEL,
            gst_vpx_enc->cq_level);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_CQ_LEVEL: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_MAX_INTRA_BITRATE_PCT:
      gst_vpx_enc->max_intra_bitrate_pct = g_value_get_int (value);
      if (gst_vpx_enc->inited) {
        status =
            vpx_codec_control (&gst_vpx_enc->encoder,
            VP8E_SET_MAX_INTRA_BITRATE_PCT, gst_vpx_enc->max_intra_bitrate_pct);
        if (status != VPX_CODEC_OK) {
          GST_WARNING_OBJECT (gst_vpx_enc,
              "Failed to set VP8E_SET_MAX_INTRA_BITRATE_PCT: %s",
              gst_vpx_error_name (status));
        }
      }
      break;
    case PROP_TIMEBASE:
      gst_vpx_enc->timebase_n = gst_value_get_fraction_numerator (value);
      gst_vpx_enc->timebase_d = gst_value_get_fraction_denominator (value);
      break;
    default:
      break;
  }

  if (global &&gst_vpx_enc->inited) {
    status =
        vpx_codec_enc_config_set (&gst_vpx_enc->encoder, &gst_vpx_enc->cfg);
    if (status != VPX_CODEC_OK) {
      g_mutex_unlock (&gst_vpx_enc->encoder_lock);
      GST_ELEMENT_ERROR (gst_vpx_enc, LIBRARY, INIT,
          ("Failed to set encoder configuration"), ("%s",
              gst_vpx_error_name (status)));
    } else {
      g_mutex_unlock (&gst_vpx_enc->encoder_lock);
    }
  } else {
    g_mutex_unlock (&gst_vpx_enc->encoder_lock);
  }
}

static void
gst_vpx_enc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstVPXEnc *gst_vpx_enc;

  g_return_if_fail (GST_IS_VPX_ENC (object));
  gst_vpx_enc = GST_VPX_ENC (object);

  g_mutex_lock (&gst_vpx_enc->encoder_lock);
  switch (prop_id) {
    case PROP_RC_END_USAGE:
      g_value_set_enum (value, gst_vpx_enc->cfg.rc_end_usage);
      break;
    case PROP_RC_TARGET_BITRATE:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_target_bitrate * 1000);
      break;
    case PROP_RC_MIN_QUANTIZER:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_min_quantizer);
      break;
    case PROP_RC_MAX_QUANTIZER:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_max_quantizer);
      break;
    case PROP_RC_DROPFRAME_THRESH:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_dropframe_thresh);
      break;
    case PROP_RC_RESIZE_ALLOWED:
      g_value_set_boolean (value, gst_vpx_enc->cfg.rc_resize_allowed);
      break;
    case PROP_RC_RESIZE_UP_THRESH:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_resize_up_thresh);
      break;
    case PROP_RC_RESIZE_DOWN_THRESH:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_resize_down_thresh);
      break;
    case PROP_RC_UNDERSHOOT_PCT:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_undershoot_pct);
      break;
    case PROP_RC_OVERSHOOT_PCT:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_overshoot_pct);
      break;
    case PROP_RC_BUF_SZ:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_buf_sz);
      break;
    case PROP_RC_BUF_INITIAL_SZ:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_buf_initial_sz);
      break;
    case PROP_RC_BUF_OPTIMAL_SZ:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_buf_optimal_sz);
      break;
    case PROP_RC_2PASS_VBR_BIAS_PCT:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_2pass_vbr_bias_pct);
      break;
    case PROP_RC_2PASS_VBR_MINSECTION_PCT:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_2pass_vbr_minsection_pct);
      break;
    case PROP_RC_2PASS_VBR_MAXSECTION_PCT:
      g_value_set_int (value, gst_vpx_enc->cfg.rc_2pass_vbr_maxsection_pct);
      break;
    case PROP_KF_MODE:
      g_value_set_enum (value, gst_vpx_enc->cfg.kf_mode);
      break;
    case PROP_KF_MAX_DIST:
      g_value_set_int (value, gst_vpx_enc->cfg.kf_max_dist);
      break;
    case PROP_MULTIPASS_MODE:
      g_value_set_enum (value, gst_vpx_enc->cfg.g_pass);
      break;
    case PROP_MULTIPASS_CACHE_FILE:
      g_value_set_string (value, gst_vpx_enc->multipass_cache_prefix);
      break;
    case PROP_TS_NUMBER_LAYERS:
      g_value_set_int (value, gst_vpx_enc->cfg.ts_number_layers);
      break;
    case PROP_TS_TARGET_BITRATE:{
      GValueArray *va;

      if (gst_vpx_enc->n_ts_target_bitrate == 0) {
        g_value_set_boxed (value, NULL);
      } else {
        gint i;

        va = g_value_array_new (gst_vpx_enc->n_ts_target_bitrate);
        for (i = 0; i < gst_vpx_enc->n_ts_target_bitrate; i++) {
          GValue v = { 0, };

          g_value_init (&v, G_TYPE_INT);
          g_value_set_int (&v, gst_vpx_enc->cfg.ts_target_bitrate[i]);
          g_value_array_append (va, &v);
          g_value_unset (&v);
        }
        g_value_set_boxed (value, va);
        g_value_array_free (va);
      }
      break;
    }
    case PROP_TS_RATE_DECIMATOR:{
      GValueArray *va;

      if (gst_vpx_enc->n_ts_rate_decimator == 0) {
        g_value_set_boxed (value, NULL);
      } else {
        gint i;

        va = g_value_array_new (gst_vpx_enc->n_ts_rate_decimator);
        for (i = 0; i < gst_vpx_enc->n_ts_rate_decimator; i++) {
          GValue v = { 0, };

          g_value_init (&v, G_TYPE_INT);
          g_value_set_int (&v, gst_vpx_enc->cfg.ts_rate_decimator[i]);
          g_value_array_append (va, &v);
          g_value_unset (&v);
        }
        g_value_set_boxed (value, va);
        g_value_array_free (va);
      }
      break;
    }
    case PROP_TS_PERIODICITY:
      g_value_set_int (value, gst_vpx_enc->cfg.ts_periodicity);
      break;
    case PROP_TS_LAYER_ID:{
      GValueArray *va;

      if (gst_vpx_enc->n_ts_layer_id == 0) {
        g_value_set_boxed (value, NULL);
      } else {
        gint i;

        va = g_value_array_new (gst_vpx_enc->n_ts_layer_id);
        for (i = 0; i < gst_vpx_enc->n_ts_layer_id; i++) {
          GValue v = { 0, };

          g_value_init (&v, G_TYPE_INT);
          g_value_set_int (&v, gst_vpx_enc->cfg.ts_layer_id[i]);
          g_value_array_append (va, &v);
          g_value_unset (&v);
        }
        g_value_set_boxed (value, va);
        g_value_array_free (va);
      }
      break;
    }
    case PROP_ERROR_RESILIENT:
      g_value_set_flags (value, gst_vpx_enc->cfg.g_error_resilient);
      break;
    case PROP_LAG_IN_FRAMES:
      g_value_set_int (value, gst_vpx_enc->cfg.g_lag_in_frames);
      break;
    case PROP_THREADS:
      g_value_set_int (value, gst_vpx_enc->cfg.g_threads);
      break;
    case PROP_DEADLINE:
      g_value_set_int64 (value, gst_vpx_enc->deadline);
      break;
    case PROP_H_SCALING_MODE:
      g_value_set_enum (value, gst_vpx_enc->h_scaling_mode);
      break;
    case PROP_V_SCALING_MODE:
      g_value_set_enum (value, gst_vpx_enc->v_scaling_mode);
      break;
    case PROP_CPU_USED:
      g_value_set_int (value, gst_vpx_enc->cpu_used);
      break;
    case PROP_ENABLE_AUTO_ALT_REF:
      g_value_set_boolean (value, gst_vpx_enc->enable_auto_alt_ref);
      break;
    case PROP_NOISE_SENSITIVITY:
      g_value_set_int (value, gst_vpx_enc->noise_sensitivity);
      break;
    case PROP_SHARPNESS:
      g_value_set_int (value, gst_vpx_enc->sharpness);
      break;
    case PROP_STATIC_THRESHOLD:
      g_value_set_int (value, gst_vpx_enc->static_threshold);
      break;
    case PROP_TOKEN_PARTITIONS:
      g_value_set_enum (value, gst_vpx_enc->token_partitions);
      break;
    case PROP_ARNR_MAXFRAMES:
      g_value_set_int (value, gst_vpx_enc->arnr_maxframes);
      break;
    case PROP_ARNR_STRENGTH:
      g_value_set_int (value, gst_vpx_enc->arnr_strength);
      break;
    case PROP_ARNR_TYPE:
      g_value_set_int (value, gst_vpx_enc->arnr_type);
      break;
    case PROP_TUNING:
      g_value_set_enum (value, gst_vpx_enc->tuning);
      break;
    case PROP_CQ_LEVEL:
      g_value_set_int (value, gst_vpx_enc->cq_level);
      break;
    case PROP_MAX_INTRA_BITRATE_PCT:
      g_value_set_int (value, gst_vpx_enc->max_intra_bitrate_pct);
      break;
    case PROP_TIMEBASE:
      gst_value_set_fraction (value, gst_vpx_enc->timebase_n,
          gst_vpx_enc->timebase_d);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  g_mutex_unlock (&gst_vpx_enc->encoder_lock);
}

static gboolean
gst_vpx_enc_start (GstVideoEncoder * video_encoder)
{
  GstVPXEnc *encoder = GST_VPX_ENC (video_encoder);

  GST_DEBUG_OBJECT (video_encoder, "start");

  if (!encoder->have_default_config) {
    GST_ELEMENT_ERROR (encoder, LIBRARY, INIT,
        ("Failed to get default encoder configuration"), (NULL));
    return FALSE;
  }

  return TRUE;
}

static void
gst_vpx_enc_destroy_encoder (GstVPXEnc * encoder)
{
  g_mutex_lock (&encoder->encoder_lock);
  if (encoder->inited) {
    vpx_codec_destroy (&encoder->encoder);
    encoder->inited = FALSE;
  }

  if (encoder->first_pass_cache_content) {
    g_byte_array_free (encoder->first_pass_cache_content, TRUE);
    encoder->first_pass_cache_content = NULL;
  }

  if (encoder->cfg.rc_twopass_stats_in.buf) {
    g_free (encoder->cfg.rc_twopass_stats_in.buf);
    encoder->cfg.rc_twopass_stats_in.buf = NULL;
    encoder->cfg.rc_twopass_stats_in.sz = 0;
  }
  g_mutex_unlock (&encoder->encoder_lock);
}

static gboolean
gst_vpx_enc_stop (GstVideoEncoder * video_encoder)
{
  GstVPXEnc *encoder;

  GST_DEBUG_OBJECT (video_encoder, "stop");

  encoder = GST_VPX_ENC (video_encoder);

  gst_vpx_enc_destroy_encoder (encoder);

  gst_tag_setter_reset_tags (GST_TAG_SETTER (encoder));

  g_free (encoder->multipass_cache_file);
  encoder->multipass_cache_file = NULL;
  encoder->multipass_cache_idx = 0;

  return TRUE;
}

static gint
gst_vpx_enc_get_downstream_profile (GstVPXEnc * encoder)
{
  GstCaps *allowed;
  GstStructure *s;
  gint profile = DEFAULT_PROFILE;

  allowed = gst_pad_get_allowed_caps (GST_VIDEO_ENCODER_SRC_PAD (encoder));
  if (allowed) {
    allowed = gst_caps_truncate (allowed);
    s = gst_caps_get_structure (allowed, 0);
    if (gst_structure_has_field (s, "profile")) {
      const GValue *v = gst_structure_get_value (s, "profile");
      const gchar *profile_str = NULL;

      if (GST_VALUE_HOLDS_LIST (v) && gst_value_list_get_size (v) > 0) {
        profile_str = g_value_get_string (gst_value_list_get_value (v, 0));
      } else if (G_VALUE_HOLDS_STRING (v)) {
        profile_str = g_value_get_string (v);
      }

      if (profile_str) {
        gchar *endptr = NULL;

        profile = g_ascii_strtoull (profile_str, &endptr, 10);
        if (*endptr != '\0' || profile < 0 || profile > 3) {
          GST_ERROR_OBJECT (encoder, "Invalid profile '%s'", profile_str);
          profile = DEFAULT_PROFILE;
        }
      }
    }
    gst_caps_unref (allowed);
  }

  GST_DEBUG_OBJECT (encoder, "Using profile %d", profile);

  return profile;
}

static gboolean
gst_vpx_enc_set_format (GstVideoEncoder * video_encoder,
    GstVideoCodecState * state)
{
  GstVPXEnc *encoder;
  vpx_codec_err_t status;
  vpx_image_t *image;
  GstCaps *caps;
  gboolean ret = TRUE;
  GstVideoInfo *info = &state->info;
  GstVideoCodecState *output_state;
  GstClockTime latency;
  GstVPXEncClass *vpx_enc_class;

  encoder = GST_VPX_ENC (video_encoder);
  vpx_enc_class = GST_VPX_ENC_GET_CLASS (encoder);
  GST_DEBUG_OBJECT (video_encoder, "set_format");

  if (encoder->inited) {
    gst_vpx_enc_drain (video_encoder);
    g_mutex_lock (&encoder->encoder_lock);
    vpx_codec_destroy (&encoder->encoder);
    encoder->inited = FALSE;
    encoder->multipass_cache_idx++;
  } else {
    g_mutex_lock (&encoder->encoder_lock);
  }

  encoder->cfg.g_profile = gst_vpx_enc_get_downstream_profile (encoder);

  /* Scale default bitrate to our size */
  if (!encoder->rc_target_bitrate_set)
    encoder->cfg.rc_target_bitrate =
        gst_util_uint64_scale (DEFAULT_RC_TARGET_BITRATE,
        GST_VIDEO_INFO_WIDTH (info) * GST_VIDEO_INFO_HEIGHT (info),
        320 * 240 * 1000);

  encoder->cfg.g_w = GST_VIDEO_INFO_WIDTH (info);
  encoder->cfg.g_h = GST_VIDEO_INFO_HEIGHT (info);

  if (encoder->timebase_n != 0 && encoder->timebase_d != 0) {
    GST_DEBUG_OBJECT (video_encoder, "Using timebase configuration");
    encoder->cfg.g_timebase.num = encoder->timebase_n;
    encoder->cfg.g_timebase.den = encoder->timebase_d;
  } else {
    /* Zero framerate and max-framerate but still need to setup the timebase to avoid
     * a divide by zero error. Presuming the lowest common denominator will be RTP -
     * VP8 payload draft states clock rate of 90000 which should work for anyone where
     * FPS < 90000 (shouldn't be too many cases where it's higher) though wouldn't be optimal. RTP specification
     * http://tools.ietf.org/html/draft-ietf-payload-vp8-01 section 6.3.1 */
    encoder->cfg.g_timebase.num = 1;
    encoder->cfg.g_timebase.den = 90000;
  }

  if (encoder->cfg.g_pass == VPX_RC_FIRST_PASS ||
      encoder->cfg.g_pass == VPX_RC_LAST_PASS) {
    if (!encoder->multipass_cache_prefix) {
      GST_ELEMENT_ERROR (encoder, RESOURCE, OPEN_READ,
          ("No multipass cache file provided"), (NULL));
      g_mutex_unlock (&encoder->encoder_lock);
      return FALSE;
    }

    g_free (encoder->multipass_cache_file);

    if (encoder->multipass_cache_idx > 0)
      encoder->multipass_cache_file = g_strdup_printf ("%s.%u",
          encoder->multipass_cache_prefix, encoder->multipass_cache_idx);
    else
      encoder->multipass_cache_file =
          g_strdup (encoder->multipass_cache_prefix);
  }

  if (encoder->cfg.g_pass == VPX_RC_FIRST_PASS) {
    if (encoder->first_pass_cache_content != NULL)
      g_byte_array_free (encoder->first_pass_cache_content, TRUE);

    encoder->first_pass_cache_content = g_byte_array_sized_new (4096);

  } else if (encoder->cfg.g_pass == VPX_RC_LAST_PASS) {
    GError *err = NULL;

    if (encoder->cfg.rc_twopass_stats_in.buf != NULL) {
      g_free (encoder->cfg.rc_twopass_stats_in.buf);
      encoder->cfg.rc_twopass_stats_in.buf = NULL;
      encoder->cfg.rc_twopass_stats_in.sz = 0;
    }

    if (!g_file_get_contents (encoder->multipass_cache_file,
            (gchar **) & encoder->cfg.rc_twopass_stats_in.buf,
            &encoder->cfg.rc_twopass_stats_in.sz, &err)) {
      GST_ELEMENT_ERROR (encoder, RESOURCE, OPEN_READ,
          ("Failed to read multipass cache file provided"), ("%s",
              err->message));
      g_error_free (err);
      g_mutex_unlock (&encoder->encoder_lock);
      return FALSE;
    }
  }

  status =
      vpx_codec_enc_init (&encoder->encoder, vpx_enc_class->get_algo (encoder),
      &encoder->cfg, 0);
  if (status != VPX_CODEC_OK) {
    GST_ELEMENT_ERROR (encoder, LIBRARY, INIT,
        ("Failed to initialize encoder"), ("%s", gst_vpx_error_name (status)));
    g_mutex_unlock (&encoder->encoder_lock);
    return FALSE;
  }

  if (vpx_enc_class->enable_scaling (encoder)) {
    vpx_scaling_mode_t sm;

    sm.h_scaling_mode = encoder->h_scaling_mode;
    sm.v_scaling_mode = encoder->v_scaling_mode;

    status = vpx_codec_control (&encoder->encoder, VP8E_SET_SCALEMODE, &sm);
    if (status != VPX_CODEC_OK) {
      GST_WARNING_OBJECT (encoder, "Failed to set VP8E_SET_SCALEMODE: %s",
          gst_vpx_error_name (status));
    }
  }

  status =
      vpx_codec_control (&encoder->encoder, VP8E_SET_CPUUSED,
      encoder->cpu_used);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder, "Failed to set VP8E_SET_CPUUSED: %s",
        gst_vpx_error_name (status));
  }

  status =
      vpx_codec_control (&encoder->encoder, VP8E_SET_ENABLEAUTOALTREF,
      (encoder->enable_auto_alt_ref ? 1 : 0));
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_ENABLEAUTOALTREF: %s",
        gst_vpx_error_name (status));
  }
  status = vpx_codec_control (&encoder->encoder, VP8E_SET_NOISE_SENSITIVITY,
      encoder->noise_sensitivity);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_NOISE_SENSITIVITY: %s",
        gst_vpx_error_name (status));
  }
  status = vpx_codec_control (&encoder->encoder, VP8E_SET_SHARPNESS,
      encoder->sharpness);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_SHARPNESS: %s", gst_vpx_error_name (status));
  }
  status = vpx_codec_control (&encoder->encoder, VP8E_SET_STATIC_THRESHOLD,
      encoder->static_threshold);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_STATIC_THRESHOLD: %s",
        gst_vpx_error_name (status));
  }
  status = vpx_codec_control (&encoder->encoder, VP8E_SET_TOKEN_PARTITIONS,
      encoder->token_partitions);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_TOKEN_PARTIONS: %s",
        gst_vpx_error_name (status));
  }
  status = vpx_codec_control (&encoder->encoder, VP8E_SET_ARNR_MAXFRAMES,
      encoder->arnr_maxframes);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_ARNR_MAXFRAMES: %s",
        gst_vpx_error_name (status));
  }
  status = vpx_codec_control (&encoder->encoder, VP8E_SET_ARNR_STRENGTH,
      encoder->arnr_strength);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_ARNR_STRENGTH: %s",
        gst_vpx_error_name (status));
  }
  status = vpx_codec_control (&encoder->encoder, VP8E_SET_TUNING,
      encoder->tuning);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_TUNING: %s", gst_vpx_error_name (status));
  }
  status = vpx_codec_control (&encoder->encoder, VP8E_SET_CQ_LEVEL,
      encoder->cq_level);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_CQ_LEVEL: %s", gst_vpx_error_name (status));
  }
  status = vpx_codec_control (&encoder->encoder, VP8E_SET_MAX_INTRA_BITRATE_PCT,
      encoder->max_intra_bitrate_pct);
  if (status != VPX_CODEC_OK) {
    GST_WARNING_OBJECT (encoder,
        "Failed to set VP8E_SET_MAX_INTRA_BITRATE_PCT: %s",
        gst_vpx_error_name (status));
  }

  if (GST_VIDEO_INFO_FPS_D (info) == 0 || GST_VIDEO_INFO_FPS_N (info) == 0) {
    /* FIXME: Assume 25fps for unknown framerates. Better than reporting
     * that we introduce no latency while we actually do
     */
    latency = gst_util_uint64_scale (encoder->cfg.g_lag_in_frames,
        1 * GST_SECOND, 25);
  } else {
    latency = gst_util_uint64_scale (encoder->cfg.g_lag_in_frames,
        GST_VIDEO_INFO_FPS_D (info) * GST_SECOND, GST_VIDEO_INFO_FPS_N (info));
  }
  gst_video_encoder_set_latency (video_encoder, latency, latency);
  encoder->inited = TRUE;

  /* Store input state */
  if (encoder->input_state)
    gst_video_codec_state_unref (encoder->input_state);
  encoder->input_state = gst_video_codec_state_ref (state);

  /* prepare cached image buffer setup */
  image = &encoder->image;
  memset (image, 0, sizeof (*image));

  vpx_enc_class->set_image_format (encoder, image);

  image->w = image->d_w = GST_VIDEO_INFO_WIDTH (info);
  image->h = image->d_h = GST_VIDEO_INFO_HEIGHT (info);

  image->stride[VPX_PLANE_Y] = GST_VIDEO_INFO_COMP_STRIDE (info, 0);
  image->stride[VPX_PLANE_U] = GST_VIDEO_INFO_COMP_STRIDE (info, 1);
  image->stride[VPX_PLANE_V] = GST_VIDEO_INFO_COMP_STRIDE (info, 2);

  caps = vpx_enc_class->get_new_vpx_caps (encoder);

  vpx_enc_class->set_stream_info (encoder, caps, info);

  g_mutex_unlock (&encoder->encoder_lock);

  output_state =
      gst_video_encoder_set_output_state (video_encoder, caps, state);
  gst_video_codec_state_unref (output_state);

  gst_video_encoder_negotiate (GST_VIDEO_ENCODER (encoder));

  return ret;
}

static GstFlowReturn
gst_vpx_enc_process (GstVPXEnc * encoder)
{
  vpx_codec_iter_t iter = NULL;
  const vpx_codec_cx_pkt_t *pkt;
  GstVideoEncoder *video_encoder;
  void *user_data;
  GstVideoCodecFrame *frame;
  GstFlowReturn ret = GST_FLOW_OK;
  GstVPXEncClass *vpx_enc_class;
  vpx_codec_pts_t pts;

  video_encoder = GST_VIDEO_ENCODER (encoder);
  vpx_enc_class = GST_VPX_ENC_GET_CLASS (encoder);

  g_mutex_lock (&encoder->encoder_lock);
  pkt = vpx_codec_get_cx_data (&encoder->encoder, &iter);
  while (pkt != NULL) {
    GstBuffer *buffer;
    gboolean invisible;

    GST_DEBUG_OBJECT (encoder, "packet %u type %d", (guint) pkt->data.frame.sz,
        pkt->kind);

    if (pkt->kind == VPX_CODEC_STATS_PKT
        && encoder->cfg.g_pass == VPX_RC_FIRST_PASS) {
      GST_LOG_OBJECT (encoder, "handling STATS packet");

      g_byte_array_append (encoder->first_pass_cache_content,
          pkt->data.twopass_stats.buf, pkt->data.twopass_stats.sz);

      frame = gst_video_encoder_get_oldest_frame (video_encoder);
      if (frame != NULL) {
        buffer = gst_buffer_new ();
        GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_LIVE);
        frame->output_buffer = buffer;
        g_mutex_unlock (&encoder->encoder_lock);
        ret = gst_video_encoder_finish_frame (video_encoder, frame);
        g_mutex_lock (&encoder->encoder_lock);
      }

      pkt = vpx_codec_get_cx_data (&encoder->encoder, &iter);
      continue;
    } else if (pkt->kind != VPX_CODEC_CX_FRAME_PKT) {
      GST_LOG_OBJECT (encoder, "non frame pkt: %d", pkt->kind);
      pkt = vpx_codec_get_cx_data (&encoder->encoder, &iter);
      continue;
    }

    invisible = (pkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE) != 0;

    /* discard older frames that were dropped by libvpx */
    frame = NULL;
    do {
      if (frame)
        gst_video_encoder_finish_frame (video_encoder, frame);
      frame = gst_video_encoder_get_oldest_frame (video_encoder);
      pts =
          gst_util_uint64_scale (frame->pts,
          encoder->cfg.g_timebase.den,
          encoder->cfg.g_timebase.num * (GstClockTime) GST_SECOND);
      GST_TRACE_OBJECT (encoder, "vpx pts: %" G_GINT64_FORMAT
          ", gst frame pts: %" G_GINT64_FORMAT, (gint64) pkt->data.frame.pts,
          (gint64) pts);
    } while (pkt->data.frame.pts > pts);

    g_assert (frame != NULL);
    if ((pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0)
      GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (frame);
    else
      GST_VIDEO_CODEC_FRAME_UNSET_SYNC_POINT (frame);

    /* FIXME : It would be nice to avoid the memory copy ... */
    buffer =
        gst_buffer_new_wrapped (g_memdup (pkt->data.frame.buf,
            pkt->data.frame.sz), pkt->data.frame.sz);

    user_data = vpx_enc_class->process_frame_user_data (encoder, frame);

    if (invisible) {
      ret =
          vpx_enc_class->handle_invisible_frame_buffer (encoder, user_data,
          buffer);
      gst_video_codec_frame_unref (frame);
    } else {
      frame->output_buffer = buffer;
      g_mutex_unlock (&encoder->encoder_lock);
      ret = gst_video_encoder_finish_frame (video_encoder, frame);
      g_mutex_lock (&encoder->encoder_lock);
    }

    pkt = vpx_codec_get_cx_data (&encoder->encoder, &iter);
  }
  g_mutex_unlock (&encoder->encoder_lock);

  return ret;
}

/* This function should be called holding then stream lock*/
static GstFlowReturn
gst_vpx_enc_drain (GstVideoEncoder * video_encoder)
{
  GstVPXEnc *encoder;
  int flags = 0;
  vpx_codec_err_t status;
  gint64 deadline;
  vpx_codec_pts_t pts;

  encoder = GST_VPX_ENC (video_encoder);

  g_mutex_lock (&encoder->encoder_lock);
  deadline = encoder->deadline;

  pts =
      gst_util_uint64_scale (encoder->last_pts,
      encoder->cfg.g_timebase.den,
      encoder->cfg.g_timebase.num * (GstClockTime) GST_SECOND);

  status = vpx_codec_encode (&encoder->encoder, NULL, pts, 0, flags, deadline);
  g_mutex_unlock (&encoder->encoder_lock);

  if (status != 0) {
    GST_ERROR_OBJECT (encoder, "encode returned %d %s", status,
        gst_vpx_error_name (status));
    return GST_FLOW_ERROR;
  }

  /* dispatch remaining frames */
  gst_vpx_enc_process (encoder);

  g_mutex_lock (&encoder->encoder_lock);
  if (encoder->cfg.g_pass == VPX_RC_FIRST_PASS && encoder->multipass_cache_file) {
    GError *err = NULL;

    if (!g_file_set_contents (encoder->multipass_cache_file,
            (const gchar *) encoder->first_pass_cache_content->data,
            encoder->first_pass_cache_content->len, &err)) {
      GST_ELEMENT_ERROR (encoder, RESOURCE, WRITE, (NULL),
          ("Failed to write multipass cache file: %s", err->message));
      g_error_free (err);
    }
  }
  g_mutex_unlock (&encoder->encoder_lock);

  return GST_FLOW_OK;
}

static gboolean
gst_vpx_enc_flush (GstVideoEncoder * video_encoder)
{
  GstVPXEnc *encoder;

  GST_DEBUG_OBJECT (video_encoder, "flush");

  encoder = GST_VPX_ENC (video_encoder);

  gst_vpx_enc_destroy_encoder (encoder);
  if (encoder->input_state) {
    gst_video_codec_state_ref (encoder->input_state);
    gst_vpx_enc_set_format (video_encoder, encoder->input_state);
    gst_video_codec_state_unref (encoder->input_state);
  }

  return TRUE;
}

static GstFlowReturn
gst_vpx_enc_finish (GstVideoEncoder * video_encoder)
{
  GstVPXEnc *encoder;
  GstFlowReturn ret;

  GST_DEBUG_OBJECT (video_encoder, "finish");

  encoder = GST_VPX_ENC (video_encoder);

  if (encoder->inited) {
    ret = gst_vpx_enc_drain (video_encoder);
  } else {
    ret = GST_FLOW_OK;
  }

  return ret;
}

static vpx_image_t *
gst_vpx_enc_buffer_to_image (GstVPXEnc * enc, GstVideoFrame * frame)
{
  vpx_image_t *image = g_slice_new (vpx_image_t);

  memcpy (image, &enc->image, sizeof (*image));

  image->planes[VPX_PLANE_Y] = GST_VIDEO_FRAME_COMP_DATA (frame, 0);
  image->planes[VPX_PLANE_U] = GST_VIDEO_FRAME_COMP_DATA (frame, 1);
  image->planes[VPX_PLANE_V] = GST_VIDEO_FRAME_COMP_DATA (frame, 2);

  image->stride[VPX_PLANE_Y] = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0);
  image->stride[VPX_PLANE_U] = GST_VIDEO_FRAME_COMP_STRIDE (frame, 1);
  image->stride[VPX_PLANE_V] = GST_VIDEO_FRAME_COMP_STRIDE (frame, 2);

  return image;
}

static GstFlowReturn
gst_vpx_enc_handle_frame (GstVideoEncoder * video_encoder,
    GstVideoCodecFrame * frame)
{
  GstVPXEnc *encoder;
  vpx_codec_err_t status;
  int flags = 0;
  vpx_image_t *image;
  GstVideoFrame vframe;
  vpx_codec_pts_t pts;
  unsigned long duration;
  GstVPXEncClass *vpx_enc_class;

  GST_DEBUG_OBJECT (video_encoder, "handle_frame");

  encoder = GST_VPX_ENC (video_encoder);
  vpx_enc_class = GST_VPX_ENC_GET_CLASS (encoder);

  GST_DEBUG_OBJECT (video_encoder, "size %d %d",
      GST_VIDEO_INFO_WIDTH (&encoder->input_state->info),
      GST_VIDEO_INFO_HEIGHT (&encoder->input_state->info));

  gst_video_frame_map (&vframe, &encoder->input_state->info,
      frame->input_buffer, GST_MAP_READ);
  image = gst_vpx_enc_buffer_to_image (encoder, &vframe);

  vpx_enc_class->set_frame_user_data (encoder, frame, image);

  if (GST_VIDEO_CODEC_FRAME_IS_FORCE_KEYFRAME (frame)) {
    flags |= VPX_EFLAG_FORCE_KF;
  }

  g_mutex_lock (&encoder->encoder_lock);
  pts =
      gst_util_uint64_scale (frame->pts,
      encoder->cfg.g_timebase.den,
      encoder->cfg.g_timebase.num * (GstClockTime) GST_SECOND);
  encoder->last_pts = frame->pts;

  if (frame->duration != GST_CLOCK_TIME_NONE) {
    duration =
        gst_util_uint64_scale (frame->duration, encoder->cfg.g_timebase.den,
        encoder->cfg.g_timebase.num * (GstClockTime) GST_SECOND);

    if (duration > 0) {
      encoder->last_pts += frame->duration;
    } else {
      /* We force the path ignoring the duration if we end up with a zero
       * value for duration after scaling (e.g. duration value too small) */
      GST_WARNING_OBJECT (encoder,
          "Ignoring too small frame duration %" GST_TIME_FORMAT,
          GST_TIME_ARGS (frame->duration));
      duration = 1;
    }
  } else {
    duration = 1;
  }

  status = vpx_codec_encode (&encoder->encoder, image,
      pts, duration, flags, encoder->deadline);

  g_mutex_unlock (&encoder->encoder_lock);
  gst_video_frame_unmap (&vframe);

  if (status != 0) {
    GST_ELEMENT_ERROR (encoder, LIBRARY, ENCODE,
        ("Failed to encode frame"), ("%s", gst_vpx_error_name (status)));
    gst_video_codec_frame_set_user_data (frame, NULL, NULL);
    gst_video_codec_frame_unref (frame);

    return GST_FLOW_ERROR;
  }
  gst_video_codec_frame_unref (frame);
  return gst_vpx_enc_process (encoder);
}

static gboolean
gst_vpx_enc_sink_event (GstVideoEncoder * benc, GstEvent * event)
{
  GstVPXEnc *enc = GST_VPX_ENC (benc);

  /* FIXME : Move this to base encoder class */

  if (GST_EVENT_TYPE (event) == GST_EVENT_TAG) {
    GstTagList *list;
    GstTagSetter *setter = GST_TAG_SETTER (enc);
    const GstTagMergeMode mode = gst_tag_setter_get_tag_merge_mode (setter);

    gst_event_parse_tag (event, &list);
    gst_tag_setter_merge_tags (setter, list, mode);
  }

  /* just peeked, baseclass handles the rest */
  return GST_VIDEO_ENCODER_CLASS (parent_class)->sink_event (benc, event);
}

static gboolean
gst_vpx_enc_propose_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  return GST_VIDEO_ENCODER_CLASS (parent_class)->propose_allocation (encoder,
      query);
}

#endif /* HAVE_VP8_ENCODER || HAVE_VP9_ENCODER */
