/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2012 Collabora Ltd.
 *	Author : Edward Hervey <edward@collabora.com>
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
/**
 * SECTION:element-jpegenc
 * @title: jpegenc
 *
 * Encodes jpeg images.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 videotestsrc num-buffers=50 ! video/x-raw, framerate='(fraction)'5/1 ! jpegenc ! avimux ! filesink location=mjpeg.avi
 * ]| a pipeline to mux 5 JPEG frames per second into a 10 sec. long motion jpeg
 * avi.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "gstjpegenc.h"
#include "gstjpeg.h"
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/base/base.h>

/* experimental */
/* setting smoothig seems to have no effect in libjepeg
#define ENABLE_SMOOTHING 1
*/

GST_DEBUG_CATEGORY_STATIC (jpegenc_debug);
#define GST_CAT_DEFAULT jpegenc_debug

#define JPEG_DEFAULT_QUALITY 85
#define JPEG_DEFAULT_SMOOTHING 0
#define JPEG_DEFAULT_IDCT_METHOD	JDCT_FASTEST
#define JPEG_DEFAULT_SNAPSHOT		FALSE

/* JpegEnc signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_QUALITY,
  PROP_SMOOTHING,
  PROP_IDCT_METHOD,
  PROP_SNAPSHOT
};

static void gst_jpegenc_finalize (GObject * object);

static void gst_jpegenc_resync (GstJpegEnc * jpegenc);
static void gst_jpegenc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_jpegenc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_jpegenc_start (GstVideoEncoder * benc);
static gboolean gst_jpegenc_stop (GstVideoEncoder * benc);
static gboolean gst_jpegenc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state);
static GstFlowReturn gst_jpegenc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);
static gboolean gst_jpegenc_propose_allocation (GstVideoEncoder * encoder,
    GstQuery * query);

/* static guint gst_jpegenc_signals[LAST_SIGNAL] = { 0 }; */

#define gst_jpegenc_parent_class parent_class
G_DEFINE_TYPE (GstJpegEnc, gst_jpegenc, GST_TYPE_VIDEO_ENCODER);

/* *INDENT-OFF* */
static GstStaticPadTemplate gst_jpegenc_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE
        ("{ I420, YV12, YUY2, UYVY, Y41B, Y42B, YVYU, Y444, NV21, "
         "NV12, RGB, BGR, RGBx, xRGB, BGRx, xBGR, GRAY8 }"))
    );
/* *INDENT-ON* */

static GstStaticPadTemplate gst_jpegenc_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/jpeg, "
        "width = (int) [ 16, 65535 ], "
        "height = (int) [ 16, 65535 ], "
        "framerate = (fraction) [ 0/1, MAX ], "
        "sof-marker = (int) { 0, 1, 2, 4, 9 }")
    );

static void
gst_jpegenc_class_init (GstJpegEncClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstVideoEncoderClass *venc_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;
  venc_class = (GstVideoEncoderClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gst_jpegenc_finalize;
  gobject_class->set_property = gst_jpegenc_set_property;
  gobject_class->get_property = gst_jpegenc_get_property;

  g_object_class_install_property (gobject_class, PROP_QUALITY,
      g_param_spec_int ("quality", "Quality", "Quality of encoding",
          0, 100, JPEG_DEFAULT_QUALITY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_PLAYING));

#ifdef ENABLE_SMOOTHING
  /* disabled, since it doesn't seem to work */
  g_object_class_install_property (gobject_class, PROP_SMOOTHING,
      g_param_spec_int ("smoothing", "Smoothing", "Smoothing factor",
          0, 100, JPEG_DEFAULT_SMOOTHING,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif

  g_object_class_install_property (gobject_class, PROP_IDCT_METHOD,
      g_param_spec_enum ("idct-method", "IDCT Method",
          "The IDCT algorithm to use", GST_TYPE_IDCT_METHOD,
          JPEG_DEFAULT_IDCT_METHOD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstJpegEnc:snapshot:
   *
   * Send EOS after encoding a frame, useful for snapshots.
   *
   * Since: 1.14
   */
  g_object_class_install_property (gobject_class, PROP_SNAPSHOT,
      g_param_spec_boolean ("snapshot", "Snapshot",
          "Send EOS after encoding a frame, useful for snapshots",
          JPEG_DEFAULT_SNAPSHOT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (element_class,
      &gst_jpegenc_sink_pad_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_jpegenc_src_pad_template);

  gst_element_class_set_static_metadata (element_class, "JPEG image encoder",
      "Codec/Encoder/Image", "Encode images in JPEG format",
      "Wim Taymans <wim.taymans@tvd.be>");

  venc_class->start = gst_jpegenc_start;
  venc_class->stop = gst_jpegenc_stop;
  venc_class->set_format = gst_jpegenc_set_format;
  venc_class->handle_frame = gst_jpegenc_handle_frame;
  venc_class->propose_allocation = gst_jpegenc_propose_allocation;

  GST_DEBUG_CATEGORY_INIT (jpegenc_debug, "jpegenc", 0,
      "JPEG encoding element");
}

static void
gst_jpegenc_init_destination (j_compress_ptr cinfo)
{
  GST_DEBUG ("gst_jpegenc_chain: init_destination");
}

static void
ensure_memory (GstJpegEnc * jpegenc)
{
  GstMemory *new_memory;
  GstMapInfo map;
  gsize old_size, desired_size, new_size;
  guint8 *new_data;
  static GstAllocationParams params = { 0, 3, 0, 0, };

  old_size = jpegenc->output_map.size;
  if (old_size == 0)
    desired_size = jpegenc->bufsize;
  else
    desired_size = old_size * 2;

  /* Our output memory wasn't big enough.
   * Make a new memory that's twice the size, */
  new_memory = gst_allocator_alloc (NULL, desired_size, &params);
  gst_memory_map (new_memory, &map, GST_MAP_READWRITE);
  new_data = map.data;
  new_size = map.size;

  /* copy previous data if any */
  if (jpegenc->output_mem) {
    memcpy (new_data, jpegenc->output_map.data, old_size);
    gst_memory_unmap (jpegenc->output_mem, &jpegenc->output_map);
    gst_memory_unref (jpegenc->output_mem);
  }

  /* drop it into place, */
  jpegenc->output_mem = new_memory;
  jpegenc->output_map = map;

  /* and last, update libjpeg on where to work. */
  jpegenc->jdest.next_output_byte = new_data + old_size;
  jpegenc->jdest.free_in_buffer = new_size - old_size;
}

static boolean
gst_jpegenc_flush_destination (j_compress_ptr cinfo)
{
  GstJpegEnc *jpegenc = (GstJpegEnc *) (cinfo->client_data);

  GST_DEBUG_OBJECT (jpegenc,
      "gst_jpegenc_chain: flush_destination: buffer too small");

  ensure_memory (jpegenc);

  return TRUE;
}

static void
gst_jpegenc_term_destination (j_compress_ptr cinfo)
{
  GstBuffer *outbuf;
  GstJpegEnc *jpegenc = (GstJpegEnc *) (cinfo->client_data);
  gsize memory_size = jpegenc->output_map.size - jpegenc->jdest.free_in_buffer;
  GstByteReader reader =
      GST_BYTE_READER_INIT (jpegenc->output_map.data, memory_size);
  guint16 marker;
  gint sof_marker = -1;

  GST_DEBUG_OBJECT (jpegenc, "gst_jpegenc_chain: term_source");

  /* Find the SOF marker */
  while (gst_byte_reader_get_uint16_be (&reader, &marker)) {
    /* SOF marker */
    if (marker >> 4 == 0x0ffc) {
      sof_marker = marker & 0x4;
      break;
    }
  }

  gst_memory_unmap (jpegenc->output_mem, &jpegenc->output_map);
  /* Trim the buffer size. we will push it in the chain function */
  gst_memory_resize (jpegenc->output_mem, 0, memory_size);
  jpegenc->output_map.data = NULL;
  jpegenc->output_map.size = 0;

  if (jpegenc->sof_marker != sof_marker || jpegenc->input_caps_changed) {
    GstVideoCodecState *output;
    output =
        gst_video_encoder_set_output_state (GST_VIDEO_ENCODER (jpegenc),
        gst_caps_new_simple ("image/jpeg", "sof-marker", G_TYPE_INT, sof_marker,
            NULL), jpegenc->input_state);
    gst_video_codec_state_unref (output);
    jpegenc->sof_marker = sof_marker;
    jpegenc->input_caps_changed = FALSE;
  }

  outbuf = gst_buffer_new ();
  gst_buffer_copy_into (outbuf, jpegenc->current_frame->input_buffer,
      GST_BUFFER_COPY_METADATA, 0, -1);
  gst_buffer_append_memory (outbuf, jpegenc->output_mem);
  jpegenc->output_mem = NULL;

  jpegenc->current_frame->output_buffer = outbuf;

  gst_video_frame_unmap (&jpegenc->current_vframe);

  GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (jpegenc->current_frame);

  jpegenc->res = gst_video_encoder_finish_frame (GST_VIDEO_ENCODER (jpegenc),
      jpegenc->current_frame);
  jpegenc->current_frame = NULL;
}

static void
gst_jpegenc_init (GstJpegEnc * jpegenc)
{
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_ENCODER_SINK_PAD (jpegenc));

  /* setup jpeglib */
  memset (&jpegenc->cinfo, 0, sizeof (jpegenc->cinfo));
  memset (&jpegenc->jerr, 0, sizeof (jpegenc->jerr));
  jpegenc->cinfo.err = jpeg_std_error (&jpegenc->jerr);
  jpeg_create_compress (&jpegenc->cinfo);

  jpegenc->jdest.init_destination = gst_jpegenc_init_destination;
  jpegenc->jdest.empty_output_buffer = gst_jpegenc_flush_destination;
  jpegenc->jdest.term_destination = gst_jpegenc_term_destination;
  jpegenc->cinfo.dest = &jpegenc->jdest;
  jpegenc->cinfo.client_data = jpegenc;

  /* init properties */
  jpegenc->quality = JPEG_DEFAULT_QUALITY;
  jpegenc->smoothing = JPEG_DEFAULT_SMOOTHING;
  jpegenc->idct_method = JPEG_DEFAULT_IDCT_METHOD;
  jpegenc->snapshot = JPEG_DEFAULT_SNAPSHOT;
}

static void
gst_jpegenc_finalize (GObject * object)
{
  GstJpegEnc *filter = GST_JPEGENC (object);

  jpeg_destroy_compress (&filter->cinfo);

  if (filter->input_state)
    gst_video_codec_state_unref (filter->input_state);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_jpegenc_set_format (GstVideoEncoder * encoder, GstVideoCodecState * state)
{
  GstJpegEnc *enc = GST_JPEGENC (encoder);
  gint i;
  GstVideoInfo *info = &state->info;

  if (enc->input_state)
    gst_video_codec_state_unref (enc->input_state);
  enc->input_state = gst_video_codec_state_ref (state);

  /* prepare a cached image description  */
  enc->channels = GST_VIDEO_INFO_N_COMPONENTS (info);

  /* ... but any alpha is disregarded in encoding */
  if (GST_VIDEO_INFO_IS_GRAY (info))
    enc->channels = 1;

  enc->h_max_samp = 0;
  enc->v_max_samp = 0;
  for (i = 0; i < enc->channels; ++i) {
    enc->cwidth[i] = GST_VIDEO_INFO_COMP_WIDTH (info, i);
    enc->cheight[i] = GST_VIDEO_INFO_COMP_HEIGHT (info, i);
    enc->inc[i] = GST_VIDEO_INFO_COMP_PSTRIDE (info, i);
    enc->h_samp[i] =
        GST_ROUND_UP_4 (GST_VIDEO_INFO_WIDTH (info)) / enc->cwidth[i];
    enc->h_max_samp = MAX (enc->h_max_samp, enc->h_samp[i]);
    enc->v_samp[i] =
        GST_ROUND_UP_4 (GST_VIDEO_INFO_HEIGHT (info)) / enc->cheight[i];
    enc->v_max_samp = MAX (enc->v_max_samp, enc->v_samp[i]);
  }
  /* samp should only be 1, 2 or 4 */
  g_assert (enc->h_max_samp <= 4);
  g_assert (enc->v_max_samp <= 4);

  /* now invert */
  /* maximum is invariant, as one of the components should have samp 1 */
  for (i = 0; i < enc->channels; ++i) {
    GST_DEBUG ("%d %d", enc->h_samp[i], enc->h_max_samp);
    enc->h_samp[i] = enc->h_max_samp / enc->h_samp[i];
    enc->v_samp[i] = enc->v_max_samp / enc->v_samp[i];
  }
  enc->planar = (enc->inc[0] == 1 && enc->inc[1] == 1 && enc->inc[2] == 1);

  enc->input_caps_changed = TRUE;
  gst_jpegenc_resync (enc);

  return TRUE;
}

static void
gst_jpegenc_resync (GstJpegEnc * jpegenc)
{
  GstVideoInfo *info;
  gint width, height;
  gint i, j;

  GST_DEBUG_OBJECT (jpegenc, "resync");

  if (!jpegenc->input_state)
    return;

  info = &jpegenc->input_state->info;

  jpegenc->cinfo.image_width = width = GST_VIDEO_INFO_WIDTH (info);
  jpegenc->cinfo.image_height = height = GST_VIDEO_INFO_HEIGHT (info);
  jpegenc->cinfo.input_components = jpegenc->channels;

  GST_DEBUG_OBJECT (jpegenc, "width %d, height %d", width, height);
  GST_DEBUG_OBJECT (jpegenc, "format %d", GST_VIDEO_INFO_FORMAT (info));

  if (GST_VIDEO_INFO_IS_RGB (info)) {
    GST_DEBUG_OBJECT (jpegenc, "RGB");
    jpegenc->cinfo.in_color_space = JCS_RGB;
  } else if (GST_VIDEO_INFO_IS_GRAY (info)) {
    GST_DEBUG_OBJECT (jpegenc, "gray");
    jpegenc->cinfo.in_color_space = JCS_GRAYSCALE;
  } else {
    GST_DEBUG_OBJECT (jpegenc, "YUV");
    jpegenc->cinfo.in_color_space = JCS_YCbCr;
  }

  /* input buffer size as max output */
  jpegenc->bufsize = GST_VIDEO_INFO_SIZE (info);
  jpeg_set_defaults (&jpegenc->cinfo);
  jpegenc->cinfo.raw_data_in = TRUE;
  /* duh, libjpeg maps RGB to YUV ... and don't expect some conversion */
  if (jpegenc->cinfo.in_color_space == JCS_RGB)
    jpeg_set_colorspace (&jpegenc->cinfo, JCS_RGB);

  GST_DEBUG_OBJECT (jpegenc, "h_max_samp=%d, v_max_samp=%d",
      jpegenc->h_max_samp, jpegenc->v_max_samp);
  /* image dimension info */
  for (i = 0; i < jpegenc->channels; i++) {
    GST_DEBUG_OBJECT (jpegenc, "comp %i: h_samp=%d, v_samp=%d", i,
        jpegenc->h_samp[i], jpegenc->v_samp[i]);
    jpegenc->cinfo.comp_info[i].h_samp_factor = jpegenc->h_samp[i];
    jpegenc->cinfo.comp_info[i].v_samp_factor = jpegenc->v_samp[i];
    g_free (jpegenc->line[i]);
    jpegenc->line[i] = g_new (guchar *, jpegenc->v_max_samp * DCTSIZE);
    if (!jpegenc->planar) {
      for (j = 0; j < jpegenc->v_max_samp * DCTSIZE; j++) {
        g_free (jpegenc->row[i][j]);
        jpegenc->row[i][j] = g_malloc (width);
        jpegenc->line[i][j] = jpegenc->row[i][j];
      }
    }
  }

  /* guard against a potential error in gst_jpegenc_term_destination
     which occurs iff bufsize % 4 < free_space_remaining */
  jpegenc->bufsize = GST_ROUND_UP_4 (jpegenc->bufsize);

  jpeg_suppress_tables (&jpegenc->cinfo, TRUE);

  GST_DEBUG_OBJECT (jpegenc, "resync done");
}

static GstFlowReturn
gst_jpegenc_handle_frame (GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
  GstJpegEnc *jpegenc;
  guint height;
  guchar *base[3], *end[3];
  guint stride[3];
  gint i, j, k;
  static GstAllocationParams params = { 0, 0, 0, 3, };

  jpegenc = GST_JPEGENC (encoder);

  GST_LOG_OBJECT (jpegenc, "got new frame");

  if (!gst_video_frame_map (&jpegenc->current_vframe,
          &jpegenc->input_state->info, frame->input_buffer, GST_MAP_READ))
    goto invalid_frame;

  jpegenc->current_frame = frame;

  height = GST_VIDEO_INFO_HEIGHT (&jpegenc->input_state->info);

  for (i = 0; i < jpegenc->channels; i++) {
    base[i] = GST_VIDEO_FRAME_COMP_DATA (&jpegenc->current_vframe, i);
    stride[i] = GST_VIDEO_FRAME_COMP_STRIDE (&jpegenc->current_vframe, i);
    end[i] =
        base[i] + GST_VIDEO_FRAME_COMP_HEIGHT (&jpegenc->current_vframe,
        i) * stride[i];
  }

  jpegenc->res = GST_FLOW_OK;
  jpegenc->output_mem = gst_allocator_alloc (NULL, jpegenc->bufsize, &params);
  gst_memory_map (jpegenc->output_mem, &jpegenc->output_map, GST_MAP_READWRITE);

  jpegenc->jdest.next_output_byte = jpegenc->output_map.data;
  jpegenc->jdest.free_in_buffer = jpegenc->output_map.size;

  /* prepare for raw input */
#if JPEG_LIB_VERSION >= 70
  jpegenc->cinfo.do_fancy_downsampling = FALSE;
#endif

  GST_OBJECT_LOCK (jpegenc);
  jpegenc->cinfo.smoothing_factor = jpegenc->smoothing;
  jpegenc->cinfo.dct_method = jpegenc->idct_method;
  jpeg_set_quality (&jpegenc->cinfo, jpegenc->quality, TRUE);
  GST_OBJECT_UNLOCK (jpegenc);

  jpeg_start_compress (&jpegenc->cinfo, TRUE);

  GST_LOG_OBJECT (jpegenc, "compressing");

  if (jpegenc->planar) {
    for (i = 0; i < height; i += jpegenc->v_max_samp * DCTSIZE) {
      for (k = 0; k < jpegenc->channels; k++) {
        for (j = 0; j < jpegenc->v_samp[k] * DCTSIZE; j++) {
          jpegenc->line[k][j] = base[k];
          if (base[k] + stride[k] < end[k])
            base[k] += stride[k];
        }
      }
      jpeg_write_raw_data (&jpegenc->cinfo, jpegenc->line,
          jpegenc->v_max_samp * DCTSIZE);
    }
  } else {
    for (i = 0; i < height; i += jpegenc->v_max_samp * DCTSIZE) {
      for (k = 0; k < jpegenc->channels; k++) {
        for (j = 0; j < jpegenc->v_samp[k] * DCTSIZE; j++) {
          guchar *src, *dst;
          gint l;

          /* ouch, copy line */
          src = base[k];
          dst = jpegenc->line[k][j];
          for (l = jpegenc->cwidth[k]; l > 0; l--) {
            *dst = *src;
            src += jpegenc->inc[k];
            dst++;
          }
          if (base[k] + stride[k] < end[k])
            base[k] += stride[k];
        }
      }
      jpeg_write_raw_data (&jpegenc->cinfo, jpegenc->line,
          jpegenc->v_max_samp * DCTSIZE);
    }
  }

  /* This will ensure that gst_jpegenc_term_destination is called */
  jpeg_finish_compress (&jpegenc->cinfo);
  GST_LOG_OBJECT (jpegenc, "compressing done");

  return (jpegenc->snapshot) ? GST_FLOW_EOS : jpegenc->res;

invalid_frame:
  {
    GST_WARNING_OBJECT (jpegenc, "invalid frame received");
    return gst_video_encoder_finish_frame (encoder, frame);
  }
}

static gboolean
gst_jpegenc_propose_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  return GST_VIDEO_ENCODER_CLASS (parent_class)->propose_allocation (encoder,
      query);
}

static void
gst_jpegenc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstJpegEnc *jpegenc = GST_JPEGENC (object);

  GST_OBJECT_LOCK (jpegenc);

  switch (prop_id) {
    case PROP_QUALITY:
      jpegenc->quality = g_value_get_int (value);
      break;
#ifdef ENABLE_SMOOTHING
    case PROP_SMOOTHING:
      jpegenc->smoothing = g_value_get_int (value);
      break;
#endif
    case PROP_IDCT_METHOD:
      jpegenc->idct_method = g_value_get_enum (value);
      break;
    case PROP_SNAPSHOT:
      jpegenc->snapshot = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (jpegenc);
}

static void
gst_jpegenc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstJpegEnc *jpegenc = GST_JPEGENC (object);

  GST_OBJECT_LOCK (jpegenc);

  switch (prop_id) {
    case PROP_QUALITY:
      g_value_set_int (value, jpegenc->quality);
      break;
#ifdef ENABLE_SMOOTHING
    case PROP_SMOOTHING:
      g_value_set_int (value, jpegenc->smoothing);
      break;
#endif
    case PROP_IDCT_METHOD:
      g_value_set_enum (value, jpegenc->idct_method);
      break;
    case PROP_SNAPSHOT:
      g_value_set_boolean (value, jpegenc->snapshot);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (jpegenc);
}

static gboolean
gst_jpegenc_start (GstVideoEncoder * benc)
{
  GstJpegEnc *enc = (GstJpegEnc *) benc;

  enc->line[0] = NULL;
  enc->line[1] = NULL;
  enc->line[2] = NULL;
  enc->sof_marker = -1;

  return TRUE;
}

static gboolean
gst_jpegenc_stop (GstVideoEncoder * benc)
{
  GstJpegEnc *enc = (GstJpegEnc *) benc;
  gint i, j;

  g_free (enc->line[0]);
  g_free (enc->line[1]);
  g_free (enc->line[2]);
  enc->line[0] = NULL;
  enc->line[1] = NULL;
  enc->line[2] = NULL;
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 4 * DCTSIZE; j++) {
      g_free (enc->row[i][j]);
      enc->row[i][j] = NULL;
    }
  }

  return TRUE;
}
