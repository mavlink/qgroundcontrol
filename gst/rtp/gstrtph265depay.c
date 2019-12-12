/* GStreamer
 * Copyright (C) <2006> Wim Taymans <wim.taymans@gmail.com>
 * Copyright (C) <2014> Jurgen Slowack <jurgenslowack@gmail.com>
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
#  include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include <gst/base/gstbitreader.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>
#include "gstrtph265depay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtph265depay_debug);
#define GST_CAT_DEFAULT (rtph265depay_debug)

/* This is what we'll default to when downstream hasn't
 * expressed a restriction or preference via caps */
#define DEFAULT_STREAM_FORMAT GST_H265_STREAM_FORMAT_BYTESTREAM
#define DEFAULT_ACCESS_UNIT   FALSE

/* 3 zero bytes syncword */
static const guint8 sync_bytes[] = { 0, 0, 0, 1 };

static GstStaticPadTemplate gst_rtp_h265_depay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS
    ("video/x-h265, stream-format=(string)hvc1, alignment=(string)au; "
        /* FIXME: hev1 format is not supported yet */
        /* "video/x-h265, "
           "stream-format = (string) hev1, alignment = (string) au; " */
        "video/x-h265, "
        "stream-format = (string) byte-stream, alignment = (string) { nal, au }")
    );

static GstStaticPadTemplate gst_rtp_h265_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H265\"")
    /* optional parameters */
    /* "profile-space = (int) [ 0, 3 ], " */
    /* "profile-id = (int) [ 0, 31 ], " */
    /* "tier-flag = (int) [ 0, 1 ], " */
    /* "level-id = (int) [ 0, 255 ], " */
    /* "interop-constraints = (string) ANY, " */
    /* "profile-compatibility-indicator = (string) ANY, " */
    /* "sprop-sub-layer-id = (int) [ 0, 6 ], " */
    /* "recv-sub-layer-id = (int) [ 0, 6 ], " */
    /* "max-recv-level-id = (int) [ 0, 255 ], " */
    /* "tx-mode = (string) {MST , SST}, " */
    /* "sprop-vps = (string) ANY, " */
    /* "sprop-sps = (string) ANY, " */
    /* "sprop-pps = (string) ANY, " */
    /* "sprop-sei = (string) ANY, " */
    /* "max-lsr = (int) ANY, " *//* MUST be in the range of MaxLumaSR to 16 * MaxLumaSR, inclusive */
    /* "max-lps = (int) ANY, " *//* MUST be in the range of MaxLumaPS to 16 * MaxLumaPS, inclusive */
    /* "max-cpb = (int) ANY, " *//* MUST be in the range of MaxCPB to 16 * MaxCPB, inclusive */
    /* "max-dpb = (int) [1, 16], " */
    /* "max-br = (int) ANY, " *//* MUST be in the range of MaxBR to 16 * MaxBR, inclusive, for the highest level */
    /* "max-tr = (int) ANY, " *//* MUST be in the range of MaxTileRows to 16 * MaxTileRows, inclusive, for the highest level */
    /* "max-tc = (int) ANY, " *//* MUST be in the range of MaxTileCols to 16 * MaxTileCols, inclusive, for the highest level */
    /* "max-fps = (int) ANY, " */
    /* "sprop-max-don-diff = (int) [0, 32767], " */
    /* "sprop-depack-buf-nalus = (int) [0, 32767], " */
    /* "sprop-depack-buf-nalus = (int) [0, 4294967295], " */
    /* "depack-buf-cap = (int) [1, 4294967295], " */
    /* "sprop-segmentation-id = (int) [0, 3], " */
    /* "sprop-spatial-segmentation-idc = (string) ANY, " */
    /* "dec-parallel-cap = (string) ANY, " */
    );

#define gst_rtp_h265_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpH265Depay, gst_rtp_h265_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static void gst_rtp_h265_depay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_h265_depay_change_state (GstElement *
    element, GstStateChange transition);

static GstBuffer *gst_rtp_h265_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
static gboolean gst_rtp_h265_depay_setcaps (GstRTPBaseDepayload * filter,
    GstCaps * caps);
static gboolean gst_rtp_h265_depay_handle_event (GstRTPBaseDepayload * depay,
    GstEvent * event);
static GstBuffer *gst_rtp_h265_complete_au (GstRtpH265Depay * rtph265depay,
    GstClockTime * out_timestamp, gboolean * out_keyframe);
static void gst_rtp_h265_depay_push (GstRtpH265Depay * rtph265depay,
    GstBuffer * outbuf, gboolean keyframe, GstClockTime timestamp,
    gboolean marker);


static void
gst_rtp_h265_depay_class_init (GstRtpH265DepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gobject_class->finalize = gst_rtp_h265_depay_finalize;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h265_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h265_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP H265 depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts H265 video from RTP packets (RFC 7798)",
      "Jurgen Slowack <jurgenslowack@gmail.com>");
  gstelement_class->change_state = gst_rtp_h265_depay_change_state;

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_h265_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_h265_depay_setcaps;
  gstrtpbasedepayload_class->handle_event = gst_rtp_h265_depay_handle_event;
}

static void
gst_rtp_h265_depay_init (GstRtpH265Depay * rtph265depay)
{
  rtph265depay->adapter = gst_adapter_new ();
  rtph265depay->picture_adapter = gst_adapter_new ();
  rtph265depay->output_format = DEFAULT_STREAM_FORMAT;
  rtph265depay->byte_stream =
      (DEFAULT_STREAM_FORMAT == GST_H265_STREAM_FORMAT_BYTESTREAM);
  rtph265depay->stream_format = NULL;
  rtph265depay->merge = DEFAULT_ACCESS_UNIT;
  rtph265depay->vps = g_ptr_array_new_with_free_func (
      (GDestroyNotify) gst_buffer_unref);
  rtph265depay->sps = g_ptr_array_new_with_free_func (
      (GDestroyNotify) gst_buffer_unref);
  rtph265depay->pps = g_ptr_array_new_with_free_func (
      (GDestroyNotify) gst_buffer_unref);
}

static void
gst_rtp_h265_depay_reset (GstRtpH265Depay * rtph265depay, gboolean hard)
{
  gst_adapter_clear (rtph265depay->adapter);
  rtph265depay->wait_start = TRUE;
  gst_adapter_clear (rtph265depay->picture_adapter);
  rtph265depay->picture_start = FALSE;
  rtph265depay->last_keyframe = FALSE;
  rtph265depay->last_ts = 0;
  rtph265depay->current_fu_type = 0;
  rtph265depay->new_codec_data = FALSE;
  g_ptr_array_set_size (rtph265depay->vps, 0);
  g_ptr_array_set_size (rtph265depay->sps, 0);
  g_ptr_array_set_size (rtph265depay->pps, 0);

  if (hard) {
    if (rtph265depay->allocator != NULL) {
      gst_object_unref (rtph265depay->allocator);
      rtph265depay->allocator = NULL;
    }
    gst_allocation_params_init (&rtph265depay->params);
  }
}

static void
gst_rtp_h265_depay_drain (GstRtpH265Depay * rtph265depay)
{
  GstClockTime timestamp;
  gboolean keyframe;
  GstBuffer *outbuf;

  if (!rtph265depay->picture_start)
    return;

  outbuf = gst_rtp_h265_complete_au (rtph265depay, &timestamp, &keyframe);
  if (outbuf)
    gst_rtp_h265_depay_push (rtph265depay, outbuf, keyframe, timestamp, FALSE);
}

static void
gst_rtp_h265_depay_finalize (GObject * object)
{
  GstRtpH265Depay *rtph265depay;

  rtph265depay = GST_RTP_H265_DEPAY (object);

  if (rtph265depay->codec_data)
    gst_buffer_unref (rtph265depay->codec_data);

  g_object_unref (rtph265depay->adapter);
  g_object_unref (rtph265depay->picture_adapter);

  g_ptr_array_free (rtph265depay->vps, TRUE);
  g_ptr_array_free (rtph265depay->sps, TRUE);
  g_ptr_array_free (rtph265depay->pps, TRUE);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static inline const gchar *
stream_format_get_nick (GstH265StreamFormat fmt)
{
  switch (fmt) {
    case GST_H265_STREAM_FORMAT_BYTESTREAM:
      return "byte-stream";
    case GST_H265_STREAM_FORMAT_HVC1:
      return "hvc1";
    case GST_H265_STREAM_FORMAT_HEV1:
      return "hev1";
    default:
      break;
  }
  return "unknown";
}

static void
gst_rtp_h265_depay_negotiate (GstRtpH265Depay * rtph265depay)
{
  GstH265StreamFormat stream_format = GST_H265_STREAM_FORMAT_UNKNOWN;
  GstCaps *caps;
  gint merge = -1;

  caps =
      gst_pad_get_allowed_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (rtph265depay));

  GST_DEBUG_OBJECT (rtph265depay, "allowed caps: %" GST_PTR_FORMAT, caps);

  if (caps) {
    if (gst_caps_get_size (caps) > 0) {
      GstStructure *s = gst_caps_get_structure (caps, 0);
      const gchar *str = NULL;

      if ((str = gst_structure_get_string (s, "stream-format"))) {
        rtph265depay->stream_format = g_intern_string (str);

        if (strcmp (str, "hev1") == 0) {
          stream_format = GST_H265_STREAM_FORMAT_HEV1;
        } else if (strcmp (str, "hvc1") == 0) {
          stream_format = GST_H265_STREAM_FORMAT_HVC1;
        } else if (strcmp (str, "byte-stream") == 0) {
          stream_format = GST_H265_STREAM_FORMAT_BYTESTREAM;
        } else {
          GST_DEBUG_OBJECT (rtph265depay, "unknown stream-format: %s", str);
        }
      }

      if ((str = gst_structure_get_string (s, "alignment"))) {
        if (strcmp (str, "au") == 0) {
          merge = TRUE;
        } else if (strcmp (str, "nal") == 0) {
          merge = FALSE;
        } else {
          GST_DEBUG_OBJECT (rtph265depay, "unknown alignment: %s", str);
        }
      }
    }
    gst_caps_unref (caps);
  }

  if (stream_format != GST_H265_STREAM_FORMAT_UNKNOWN) {
    GST_DEBUG_OBJECT (rtph265depay, "downstream wants stream-format %s",
        stream_format_get_nick (stream_format));
    rtph265depay->output_format = stream_format;
  } else {
    GST_DEBUG_OBJECT (rtph265depay, "defaulting to output stream-format %s",
        stream_format_get_nick (DEFAULT_STREAM_FORMAT));
    rtph265depay->stream_format =
        stream_format_get_nick (DEFAULT_STREAM_FORMAT);
    rtph265depay->output_format = DEFAULT_STREAM_FORMAT;
  }
  rtph265depay->byte_stream =
      (rtph265depay->output_format == GST_H265_STREAM_FORMAT_BYTESTREAM);

  if (merge != -1) {
    GST_DEBUG_OBJECT (rtph265depay, "downstream requires merge %d", merge);
    rtph265depay->merge = merge;
  } else {
    GST_DEBUG_OBJECT (rtph265depay, "defaulting to merge %d",
        DEFAULT_ACCESS_UNIT);
    rtph265depay->merge = DEFAULT_ACCESS_UNIT;
  }
}

static gboolean
parse_sps (GstMapInfo * map, guint32 * sps_id)
{                               /* To parse seq_parameter_set_id */
  GstBitReader br = GST_BIT_READER_INIT (map->data + 15,
      map->size - 15);

  GST_MEMDUMP ("SPS", map->data, map->size);

  if (map->size < 16)
    return FALSE;

  if (!gst_rtp_read_golomb (&br, sps_id))
    return FALSE;

  return TRUE;
}

static gboolean
parse_pps (GstMapInfo * map, guint32 * sps_id, guint32 * pps_id)
{                               /* To parse picture_parameter_set_id */
  GstBitReader br = GST_BIT_READER_INIT (map->data + 2,
      map->size - 2);

  GST_MEMDUMP ("PPS", map->data, map->size);

  if (map->size < 3)
    return FALSE;

  if (!gst_rtp_read_golomb (&br, pps_id))
    return FALSE;
  if (!gst_rtp_read_golomb (&br, sps_id))
    return FALSE;

  return TRUE;
}

static gboolean
gst_rtp_h265_depay_set_output_caps (GstRtpH265Depay * rtph265depay,
    GstCaps * caps)
{
  GstAllocationParams params;
  GstAllocator *allocator = NULL;
  GstPad *srcpad;
  gboolean res;

  gst_allocation_params_init (&params);

  srcpad = GST_RTP_BASE_DEPAYLOAD_SRCPAD (rtph265depay);

  res = gst_pad_set_caps (srcpad, caps);

  if (res) {
    GstQuery *query;

    query = gst_query_new_allocation (caps, TRUE);
    if (!gst_pad_peer_query (srcpad, query)) {
      GST_DEBUG_OBJECT (rtph265depay, "downstream ALLOCATION query failed");
    }

    if (gst_query_get_n_allocation_params (query) > 0) {
      gst_query_parse_nth_allocation_param (query, 0, &allocator, &params);
    }

    gst_query_unref (query);
  }

  if (rtph265depay->allocator)
    gst_object_unref (rtph265depay->allocator);

  rtph265depay->allocator = allocator;
  rtph265depay->params = params;

  return res;
}

static gboolean
gst_rtp_h265_set_src_caps (GstRtpH265Depay * rtph265depay)
{
  gboolean res, update_caps;
  GstCaps *old_caps;
  GstCaps *srccaps;
  GstPad *srcpad;

  if (!rtph265depay->byte_stream &&
      (!rtph265depay->new_codec_data ||
          rtph265depay->vps->len == 0 || rtph265depay->sps->len == 0
          || rtph265depay->pps->len == 0))
    return TRUE;

  srccaps = gst_caps_new_simple ("video/x-h265",
      "stream-format", G_TYPE_STRING, rtph265depay->stream_format,
      "alignment", G_TYPE_STRING, rtph265depay->merge ? "au" : "nal", NULL);

  if (!rtph265depay->byte_stream) {
    GstBuffer *codec_data;
    gint i = 0;
    gint len;
    guint num_vps = rtph265depay->vps->len;
    guint num_sps = rtph265depay->sps->len;
    guint num_pps = rtph265depay->pps->len;
    GstMapInfo map, nalmap;
    guint8 *data;
    guint8 num_arrays = 0;
    guint new_size;
    GstBitReader br;
    guint32 tmp;
    guint8 tmp8 = 0;
    guint32 max_sub_layers_minus1, temporal_id_nesting_flag, chroma_format_idc,
        bit_depth_luma_minus8, bit_depth_chroma_minus8,
        min_spatial_segmentation_idc;

    /* Fixme: Current implementation is not embedding SEI in codec_data */

    if (num_sps == 0)
      return FALSE;

    /* start with 23 bytes header */
    len = 23;

    num_arrays = (num_vps > 0) + (num_sps > 0) + (num_pps > 0);
    len += 3 * num_arrays;

    /* add size of vps, sps & pps */
    for (i = 0; i < num_vps; i++)
      len += 2 + gst_buffer_get_size (g_ptr_array_index (rtph265depay->vps, i));
    for (i = 0; i < num_sps; i++)
      len += 2 + gst_buffer_get_size (g_ptr_array_index (rtph265depay->sps, i));
    for (i = 0; i < num_pps; i++)
      len += 2 + gst_buffer_get_size (g_ptr_array_index (rtph265depay->pps, i));

    GST_DEBUG_OBJECT (rtph265depay,
        "constructing codec_data: num_vps =%d num_sps=%d, num_pps=%d", num_vps,
        num_sps, num_pps);

    codec_data = gst_buffer_new_and_alloc (len);
    gst_buffer_map (codec_data, &map, GST_MAP_READWRITE);
    data = map.data;

    memset (data, 0, map.size);

    /* Parsing sps to get the info required further on */

    gst_buffer_map (g_ptr_array_index (rtph265depay->sps, 0), &nalmap,
        GST_MAP_READ);

    max_sub_layers_minus1 = ((nalmap.data[2]) >> 1) & 0x07;
    temporal_id_nesting_flag = nalmap.data[2] & 0x01;

    gst_bit_reader_init (&br, nalmap.data + 15, nalmap.size - 15);

    gst_rtp_read_golomb (&br, &tmp);    /* sps_seq_parameter_set_id */
    gst_rtp_read_golomb (&br, &chroma_format_idc);      /* chroma_format_idc */

    if (chroma_format_idc == 3)

      gst_bit_reader_get_bits_uint8 (&br, &tmp8, 1);    /* separate_colour_plane_flag */

    gst_rtp_read_golomb (&br, &tmp);    /* pic_width_in_luma_samples */
    gst_rtp_read_golomb (&br, &tmp);    /* pic_height_in_luma_samples */

    gst_bit_reader_get_bits_uint8 (&br, &tmp8, 1);      /* conformance_window_flag */
    if (tmp8) {
      gst_rtp_read_golomb (&br, &tmp);  /* conf_win_left_offset */
      gst_rtp_read_golomb (&br, &tmp);  /* conf_win_right_offset */
      gst_rtp_read_golomb (&br, &tmp);  /* conf_win_top_offset */
      gst_rtp_read_golomb (&br, &tmp);  /* conf_win_bottom_offset */
    }

    gst_rtp_read_golomb (&br, &bit_depth_luma_minus8);  /* bit_depth_luma_minus8 */
    gst_rtp_read_golomb (&br, &bit_depth_chroma_minus8);        /* bit_depth_chroma_minus8 */

    GST_DEBUG_OBJECT (rtph265depay,
        "Ignoring min_spatial_segmentation for now (assuming zero)");

    min_spatial_segmentation_idc = 0;   /* NOTE - we ignore this for now, but in a perfect world, we should continue parsing to obtain the real value */

    gst_buffer_unmap (g_ptr_array_index (rtph265depay->sps, 0), &nalmap);

    /* HEVCDecoderConfigurationVersion = 1 */
    data[0] = 1;

    /* Copy from profile_tier_level (Rec. ITU-T H.265 (04/2013) section 7.3.3
     *
     * profile_space | tier_flat | profile_idc |
     * profile_compatibility_flags | constraint_indicator_flags |
     * level_idc | progressive_source_flag | interlaced_source_flag
     * non_packed_constraint_flag | frame_only_constraint_flag
     * reserved_zero_44bits | level_idc */
    gst_buffer_map (g_ptr_array_index (rtph265depay->sps, 0), &nalmap,
        GST_MAP_READ);
    for (i = 0; i < 12; i++)
      data[i + 1] = nalmap.data[i];
    gst_buffer_unmap (g_ptr_array_index (rtph265depay->sps, 0), &nalmap);

    /* min_spatial_segmentation_idc */
    GST_WRITE_UINT16_BE (data + 13, min_spatial_segmentation_idc);
    data[13] |= 0xf0;
    data[15] = 0xfc;            /* keeping parrallelismType as zero (unknown) */
    data[16] = 0xfc | chroma_format_idc;
    data[17] = 0xf8 | bit_depth_luma_minus8;
    data[18] = 0xf8 | bit_depth_chroma_minus8;
    data[19] = 0x00;            /* keep avgFrameRate as unspecified */
    data[20] = 0x00;            /* keep avgFrameRate as unspecified */
    /* constFrameRate(2 bits): 0, stream may or may not be of constant framerate
     * numTemporalLayers (3 bits): number of temporal layers, value from SPS
     * TemporalIdNested (1 bit): sps_temporal_id_nesting_flag from SPS
     * lengthSizeMinusOne (2 bits): plus 1 indicates the length of the NALUnitLength */
    /* we always output NALs with 4-byte nal unit length markers (or sync code) */
    data[21] = rtph265depay->byte_stream ? 0x00 : 0x03;
    data[21] |= ((max_sub_layers_minus1 + 1) << 3);
    data[21] |= (temporal_id_nesting_flag << 2);
    GST_WRITE_UINT8 (data + 22, num_arrays);    /* numOfArrays */

    data += 23;

    /* copy all VPS */
    if (num_vps > 0) {
      /* array_completeness | reserved_zero bit | nal_unit_type */
      data[0] = 0x00 | 0x20;
      data++;

      GST_WRITE_UINT16_BE (data, num_vps);
      data += 2;

      for (i = 0; i < num_vps; i++) {
        gsize nal_size =
            gst_buffer_get_size (g_ptr_array_index (rtph265depay->vps, i));
        GST_WRITE_UINT16_BE (data, nal_size);
        gst_buffer_extract (g_ptr_array_index (rtph265depay->vps, i), 0,
            data + 2, nal_size);
        data += 2 + nal_size;
        GST_DEBUG_OBJECT (rtph265depay, "Copied VPS %d of length %u", i,
            (guint) nal_size);
      }
    }

    /* copy all SPS */
    if (num_sps > 0) {
      /* array_completeness | reserved_zero bit | nal_unit_type */
      data[0] = 0x00 | 0x21;
      data++;

      GST_WRITE_UINT16_BE (data, num_sps);
      data += 2;

      for (i = 0; i < num_sps; i++) {
        gsize nal_size =
            gst_buffer_get_size (g_ptr_array_index (rtph265depay->sps, i));
        GST_WRITE_UINT16_BE (data, nal_size);
        gst_buffer_extract (g_ptr_array_index (rtph265depay->sps, i), 0,
            data + 2, nal_size);
        data += 2 + nal_size;
        GST_DEBUG_OBJECT (rtph265depay, "Copied SPS %d of length %u", i,
            (guint) nal_size);
      }
    }

    /* copy all PPS */
    if (num_pps > 0) {
      /* array_completeness | reserved_zero bit | nal_unit_type */
      data[0] = 0x00 | 0x22;
      data++;

      GST_WRITE_UINT16_BE (data, num_pps);
      data += 2;

      for (i = 0; i < num_pps; i++) {
        gsize nal_size =
            gst_buffer_get_size (g_ptr_array_index (rtph265depay->pps, i));
        GST_WRITE_UINT16_BE (data, nal_size);
        gst_buffer_extract (g_ptr_array_index (rtph265depay->pps, i), 0,
            data + 2, nal_size);
        data += 2 + nal_size;
        GST_DEBUG_OBJECT (rtph265depay, "Copied PPS %d of length %u", i,
            (guint) nal_size);
      }
    }

    new_size = data - map.data;
    gst_buffer_unmap (codec_data, &map);
    gst_buffer_set_size (codec_data, new_size);

    gst_caps_set_simple (srccaps,
        "codec_data", GST_TYPE_BUFFER, codec_data, NULL);
    gst_buffer_unref (codec_data);
  }

  srcpad = GST_RTP_BASE_DEPAYLOAD_SRCPAD (rtph265depay);

  old_caps = gst_pad_get_current_caps (srcpad);
  if (old_caps != NULL) {

    /* Only update the caps if they are not equal. For
     * AVC we don't update caps if only the codec_data
     * changes. This is the same behaviour as in h264parse
     * and gstrtph264depay
     */
    if (rtph265depay->byte_stream) {
      update_caps = !gst_caps_is_equal (srccaps, old_caps);
    } else {
      GstCaps *tmp_caps = gst_caps_copy (srccaps);
      GstStructure *old_s, *tmp_s;

      old_s = gst_caps_get_structure (old_caps, 0);
      tmp_s = gst_caps_get_structure (tmp_caps, 0);
      if (gst_structure_has_field (old_s, "codec_data"))
        gst_structure_set_value (tmp_s, "codec_data",
            gst_structure_get_value (old_s, "codec_data"));

      update_caps = !gst_caps_is_equal (old_caps, tmp_caps);
      gst_caps_unref (tmp_caps);
    }
    gst_caps_unref (old_caps);
  } else {
    update_caps = TRUE;
  }

  if (update_caps) {
    res = gst_rtp_h265_depay_set_output_caps (rtph265depay, srccaps);
  } else {
    res = TRUE;
  }

  gst_caps_unref (srccaps);

  /* Insert SPS and PPS into the stream on next opportunity */
  if (rtph265depay->output_format != GST_H265_STREAM_FORMAT_HVC1
      && (rtph265depay->sps->len > 0 || rtph265depay->pps->len > 0)) {
    gint i;
    GstBuffer *codec_data;
    GstMapInfo map;
    guint8 *data;
    guint len = 0;

    for (i = 0; i < rtph265depay->sps->len; i++) {
      len += 4 + gst_buffer_get_size (g_ptr_array_index (rtph265depay->sps, i));
    }

    for (i = 0; i < rtph265depay->pps->len; i++) {
      len += 4 + gst_buffer_get_size (g_ptr_array_index (rtph265depay->pps, i));
    }

    codec_data = gst_buffer_new_and_alloc (len);
    gst_buffer_map (codec_data, &map, GST_MAP_WRITE);
    data = map.data;

    for (i = 0; i < rtph265depay->sps->len; i++) {
      GstBuffer *sps_buf = g_ptr_array_index (rtph265depay->sps, i);
      guint sps_size = gst_buffer_get_size (sps_buf);

      if (rtph265depay->byte_stream)
        memcpy (data, sync_bytes, sizeof (sync_bytes));
      else
        GST_WRITE_UINT32_BE (data, sps_size);
      gst_buffer_extract (sps_buf, 0, data + 4, -1);
      data += 4 + sps_size;
    }

    for (i = 0; i < rtph265depay->pps->len; i++) {
      GstBuffer *pps_buf = g_ptr_array_index (rtph265depay->pps, i);
      guint pps_size = gst_buffer_get_size (pps_buf);

      if (rtph265depay->byte_stream)
        memcpy (data, sync_bytes, sizeof (sync_bytes));
      else
        GST_WRITE_UINT32_BE (data, pps_size);
      gst_buffer_extract (pps_buf, 0, data + 4, -1);
      data += 4 + pps_size;
    }

    gst_buffer_unmap (codec_data, &map);
    if (rtph265depay->codec_data)
      gst_buffer_unref (rtph265depay->codec_data);
    rtph265depay->codec_data = codec_data;
  }

  if (res)
    rtph265depay->new_codec_data = FALSE;

  return res;
}

gboolean
gst_rtp_h265_add_vps_sps_pps (GstElement * rtph265, GPtrArray * vps_array,
    GPtrArray * sps_array, GPtrArray * pps_array, GstBuffer * nal)
{
  GstMapInfo map;
  guchar type;
  guint i;

  gst_buffer_map (nal, &map, GST_MAP_READ);

  type = (map.data[0] >> 1) & 0x3f;

  if (type == GST_H265_VPS_NUT) {
    guint32 vps_id = (map.data[2] >> 4) & 0x0f;

    for (i = 0; i < vps_array->len; i++) {
      GstBuffer *vps = g_ptr_array_index (vps_array, i);
      GstMapInfo vpsmap;
      guint32 tmp_vps_id;

      gst_buffer_map (vps, &vpsmap, GST_MAP_READ);
      tmp_vps_id = (vpsmap.data[2] >> 4) & 0x0f;

      if (vps_id == tmp_vps_id) {
        if (map.size == vpsmap.size &&
            memcmp (map.data, vpsmap.data, vpsmap.size) == 0) {
          GST_LOG_OBJECT (rtph265, "Unchanged VPS %u, not updating", vps_id);
          gst_buffer_unmap (vps, &vpsmap);
          goto drop;
        } else {
          gst_buffer_unmap (vps, &vpsmap);
          g_ptr_array_remove_index_fast (vps_array, i);
          g_ptr_array_add (vps_array, nal);
          GST_LOG_OBJECT (rtph265, "Modified VPS %u, replacing", vps_id);
          goto done;
        }
      }
      gst_buffer_unmap (vps, &vpsmap);
    }
    GST_LOG_OBJECT (rtph265, "Adding new VPS %u", vps_id);
    g_ptr_array_add (vps_array, nal);
  } else if (type == GST_H265_SPS_NUT) {
    guint32 sps_id;

    if (!parse_sps (&map, &sps_id)) {
      GST_WARNING_OBJECT (rtph265, "Invalid SPS,"
          " can't parse seq_parameter_set_id");
      goto drop;
    }

    for (i = 0; i < sps_array->len; i++) {
      GstBuffer *sps = g_ptr_array_index (sps_array, i);
      GstMapInfo spsmap;
      guint32 tmp_sps_id;

      gst_buffer_map (sps, &spsmap, GST_MAP_READ);
      parse_sps (&spsmap, &tmp_sps_id);

      if (sps_id == tmp_sps_id) {
        if (map.size == spsmap.size &&
            memcmp (map.data, spsmap.data, spsmap.size) == 0) {
          GST_LOG_OBJECT (rtph265, "Unchanged SPS %u, not updating", sps_id);
          gst_buffer_unmap (sps, &spsmap);
          goto drop;
        } else {
          gst_buffer_unmap (sps, &spsmap);
          g_ptr_array_remove_index_fast (sps_array, i);
          g_ptr_array_add (sps_array, nal);
          GST_LOG_OBJECT (rtph265, "Modified SPS %u, replacing", sps_id);
          goto done;
        }
      }
      gst_buffer_unmap (sps, &spsmap);
    }
    GST_LOG_OBJECT (rtph265, "Adding new SPS %u", sps_id);
    g_ptr_array_add (sps_array, nal);
  } else if (type == GST_H265_PPS_NUT) {
    guint32 sps_id;
    guint32 pps_id;

    if (!parse_pps (&map, &sps_id, &pps_id)) {
      GST_WARNING_OBJECT (rtph265, "Invalid PPS,"
          " can't parse seq_parameter_set_id or pic_parameter_set_id");
      goto drop;
    }

    for (i = 0; i < pps_array->len; i++) {
      GstBuffer *pps = g_ptr_array_index (pps_array, i);
      GstMapInfo ppsmap;
      guint32 tmp_sps_id;
      guint32 tmp_pps_id;


      gst_buffer_map (pps, &ppsmap, GST_MAP_READ);
      parse_pps (&ppsmap, &tmp_sps_id, &tmp_pps_id);

      if (pps_id == tmp_pps_id) {
        if (map.size == ppsmap.size &&
            memcmp (map.data, ppsmap.data, ppsmap.size) == 0) {
          GST_LOG_OBJECT (rtph265, "Unchanged PPS %u:%u, not updating", sps_id,
              pps_id);
          gst_buffer_unmap (pps, &ppsmap);
          goto drop;
        } else {
          gst_buffer_unmap (pps, &ppsmap);
          g_ptr_array_remove_index_fast (pps_array, i);
          g_ptr_array_add (pps_array, nal);
          GST_LOG_OBJECT (rtph265, "Modified PPS %u:%u, replacing",
              sps_id, pps_id);
          goto done;
        }
      }
      gst_buffer_unmap (pps, &ppsmap);
    }
    GST_LOG_OBJECT (rtph265, "Adding new PPS %u:%i", sps_id, pps_id);
    g_ptr_array_add (pps_array, nal);
  } else {
    goto drop;
  }

done:
  gst_buffer_unmap (nal, &map);

  return TRUE;

drop:
  gst_buffer_unmap (nal, &map);
  gst_buffer_unref (nal);

  return FALSE;
}


static void
gst_rtp_h265_depay_add_vps_sps_pps (GstRtpH265Depay * rtph265depay,
    GstBuffer * nal)
{
  if (gst_rtp_h265_add_vps_sps_pps (GST_ELEMENT (rtph265depay),
          rtph265depay->vps, rtph265depay->sps, rtph265depay->pps, nal))
    rtph265depay->new_codec_data = TRUE;
}

static gboolean
gst_rtp_h265_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  gint clock_rate;
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  GstRtpH265Depay *rtph265depay;
  const gchar *vps;
  const gchar *sps;
  const gchar *pps;
  gchar *ps;
  GstMapInfo map;
  guint8 *ptr;

  rtph265depay = GST_RTP_H265_DEPAY (depayload);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;
  depayload->clock_rate = clock_rate;

  /* Base64 encoded, comma separated config NALs */
  vps = gst_structure_get_string (structure, "sprop-vps");
  sps = gst_structure_get_string (structure, "sprop-sps");
  pps = gst_structure_get_string (structure, "sprop-pps");
  if (vps == NULL || sps == NULL || pps == NULL) {
    ps = NULL;
  } else {
    ps = g_strdup_printf ("%s,%s,%s", vps, sps, pps);
  }

  /* negotiate with downstream w.r.t. output format and alignment */
  gst_rtp_h265_depay_negotiate (rtph265depay);

  if (rtph265depay->byte_stream && ps != NULL) {
    /* for bytestream we only need the parameter sets but we don't error out
     * when they are not there, we assume they are in the stream. */
    gchar **params;
    GstBuffer *codec_data;
    guint len, total;
    gint i;

    params = g_strsplit (ps, ",", 0);

    /* count total number of bytes in base64. Also include the sync bytes in
     * front of the params. */
    len = 0;
    for (i = 0; params[i]; i++) {
      len += strlen (params[i]);
      len += sizeof (sync_bytes);
    }
    /* we seriously overshoot the length, but it's fine. */
    codec_data = gst_buffer_new_and_alloc (len);

    gst_buffer_map (codec_data, &map, GST_MAP_WRITE);
    ptr = map.data;
    total = 0;
    for (i = 0; params[i]; i++) {
      guint save = 0;
      gint state = 0;

      GST_DEBUG_OBJECT (depayload, "decoding param %d (%s)", i, params[i]);
      memcpy (ptr, sync_bytes, sizeof (sync_bytes));
      ptr += sizeof (sync_bytes);
      len =
          g_base64_decode_step (params[i], strlen (params[i]), ptr, &state,
          &save);
      GST_DEBUG_OBJECT (depayload, "decoded %d bytes", len);
      total += len + sizeof (sync_bytes);
      ptr += len;
    }
    gst_buffer_unmap (codec_data, &map);
    gst_buffer_resize (codec_data, 0, total);
    g_strfreev (params);

    /* keep the codec_data, we need to send it as the first buffer. We cannot
     * push it in the adapter because the adapter might be flushed on discont.
     */
    if (rtph265depay->codec_data)
      gst_buffer_unref (rtph265depay->codec_data);
    rtph265depay->codec_data = codec_data;
  } else if (!rtph265depay->byte_stream) {
    gchar **params;
    gint i;

    if (ps == NULL)
      goto incomplete_caps;

    params = g_strsplit (ps, ",", 0);

    GST_DEBUG_OBJECT (depayload, "we have %d params", g_strv_length (params));

    /* start with 23 bytes header */
    for (i = 0; params[i]; i++) {
      GstBuffer *nal;
      GstMapInfo nalmap;
      gsize nal_len;
      guint save = 0;
      gint state = 0;

      nal_len = strlen (params[i]);
      if (nal_len == 0) {
        GST_WARNING_OBJECT (depayload, "empty param '%s' (#%d)", params[i], i);
        continue;
      }
      nal = gst_buffer_new_and_alloc (nal_len);
      gst_buffer_map (nal, &nalmap, GST_MAP_READWRITE);

      nal_len =
          g_base64_decode_step (params[i], nal_len, nalmap.data, &state, &save);

      GST_DEBUG_OBJECT (depayload, "adding param %d as %s", i,
          (((nalmap.data[0] >> 1) & 0x3f) ==
              32) ? "VPS" : (((nalmap.data[0] >> 1) & 0x3f) ==
              33) ? "SPS" : "PPS");

      gst_buffer_unmap (nal, &nalmap);
      gst_buffer_set_size (nal, nal_len);

      gst_rtp_h265_depay_add_vps_sps_pps (rtph265depay, nal);
    }
    g_strfreev (params);

    if (rtph265depay->vps->len == 0 || rtph265depay->sps->len == 0 ||
        rtph265depay->pps->len == 0) {
      goto incomplete_caps;
    }
  }

  g_free (ps);

  return gst_rtp_h265_set_src_caps (rtph265depay);

  /* ERRORS */
incomplete_caps:
  {
    GST_DEBUG_OBJECT (depayload, "we have incomplete caps,"
        " doing setcaps later");
    g_free (ps);
    return TRUE;
  }
}

static GstBuffer *
gst_rtp_h265_depay_allocate_output_buffer (GstRtpH265Depay * depay, gsize size)
{
  GstBuffer *buffer = NULL;

  g_return_val_if_fail (size > 0, NULL);

  GST_LOG_OBJECT (depay, "want output buffer of %u bytes", (guint) size);

  buffer = gst_buffer_new_allocate (depay->allocator, size, &depay->params);
  if (buffer == NULL) {
    GST_INFO_OBJECT (depay, "couldn't allocate output buffer");
    buffer = gst_buffer_new_allocate (NULL, size, NULL);
  }

  return buffer;
}

static GstBuffer *
gst_rtp_h265_complete_au (GstRtpH265Depay * rtph265depay,
    GstClockTime * out_timestamp, gboolean * out_keyframe)
{
  GstBufferList *list;
  GstMapInfo outmap;
  GstBuffer *outbuf;
  guint outsize, offset = 0;
  gint b, n_bufs, m, n_mem;

  /* we had a picture in the adapter and we completed it */
  GST_DEBUG_OBJECT (rtph265depay, "taking completed AU");
  outsize = gst_adapter_available (rtph265depay->picture_adapter);

  outbuf = gst_rtp_h265_depay_allocate_output_buffer (rtph265depay, outsize);

  if (outbuf == NULL)
    return NULL;

  if (!gst_buffer_map (outbuf, &outmap, GST_MAP_WRITE))
    return NULL;

  list = gst_adapter_take_buffer_list (rtph265depay->picture_adapter, outsize);

  n_bufs = gst_buffer_list_length (list);
  for (b = 0; b < n_bufs; ++b) {
    GstBuffer *buf = gst_buffer_list_get (list, b);

    n_mem = gst_buffer_n_memory (buf);
    for (m = 0; m < n_mem; ++m) {
      GstMemory *mem = gst_buffer_peek_memory (buf, m);
      gsize mem_size = gst_memory_get_sizes (mem, NULL, NULL);
      GstMapInfo mem_map;

      if (gst_memory_map (mem, &mem_map, GST_MAP_READ)) {
        memcpy (outmap.data + offset, mem_map.data, mem_size);
        gst_memory_unmap (mem, &mem_map);
      } else {
        memset (outmap.data + offset, 0, mem_size);
      }
      offset += mem_size;
    }

    gst_rtp_copy_video_meta (rtph265depay, outbuf, buf);
  }
  gst_buffer_list_unref (list);
  gst_buffer_unmap (outbuf, &outmap);

  *out_timestamp = rtph265depay->last_ts;
  *out_keyframe = rtph265depay->last_keyframe;

  rtph265depay->last_keyframe = FALSE;
  rtph265depay->picture_start = FALSE;

  return outbuf;
}

/* VPS/SPS/PPS/RADL/TSA/RASL/IDR/CRA is considered key, all others DELTA;
 * so downstream waiting for keyframe can pick up at VPS/SPS/PPS/IDR */

#define NAL_TYPE_IS_PARAMETER_SET(nt) (		((nt) == GST_H265_VPS_NUT)\
										||  ((nt) == GST_H265_SPS_NUT)\
										||  ((nt) == GST_H265_PPS_NUT)				)

#define NAL_TYPE_IS_CODED_SLICE_SEGMENT(nt) (		((nt) == GST_H265_NAL_SLICE_TRAIL_N)\
												|| 	((nt) == GST_H265_NAL_SLICE_TRAIL_R)\
												||  ((nt) == GST_H265_NAL_SLICE_TSA_N)\
												||  ((nt) == GST_H265_NAL_SLICE_TSA_R)\
												||  ((nt) == GST_H265_NAL_SLICE_STSA_N)\
												||  ((nt) == GST_H265_NAL_SLICE_STSA_R)\
												||  ((nt) == GST_H265_NAL_SLICE_RASL_N)\
												||  ((nt) == GST_H265_NAL_SLICE_RASL_R)\
												||  ((nt) == GST_H265_NAL_SLICE_BLA_W_LP)\
												||  ((nt) == GST_H265_NAL_SLICE_BLA_W_RADL)\
												||  ((nt) == GST_H265_NAL_SLICE_BLA_N_LP)\
												||  ((nt) == GST_H265_NAL_SLICE_IDR_W_RADL)\
												||  ((nt) == GST_H265_NAL_SLICE_IDR_N_LP)\
												||  ((nt) == GST_H265_NAL_SLICE_CRA_NUT)		)

/* Intra random access point */
#define NAL_TYPE_IS_IRAP(nt)   (((nt) == GST_H265_NAL_SLICE_BLA_W_LP)   \
                             || ((nt) == GST_H265_NAL_SLICE_BLA_W_RADL) \
                             || ((nt) == GST_H265_NAL_SLICE_BLA_N_LP)   \
                             || ((nt) == GST_H265_NAL_SLICE_IDR_W_RADL) \
                             || ((nt) == GST_H265_NAL_SLICE_IDR_N_LP)   \
                             || ((nt) == GST_H265_NAL_SLICE_CRA_NUT))

#define NAL_TYPE_IS_KEY(nt) (NAL_TYPE_IS_PARAMETER_SET(nt) || NAL_TYPE_IS_IRAP(nt))

static void
gst_rtp_h265_depay_push (GstRtpH265Depay * rtph265depay, GstBuffer * outbuf,
    gboolean keyframe, GstClockTime timestamp, gboolean marker)
{
  /* prepend codec_data */
  if (rtph265depay->codec_data) {
    GST_DEBUG_OBJECT (rtph265depay, "prepending codec_data");
    gst_rtp_copy_video_meta (rtph265depay, rtph265depay->codec_data, outbuf);
    outbuf = gst_buffer_append (rtph265depay->codec_data, outbuf);
    rtph265depay->codec_data = NULL;
    keyframe = TRUE;
  }
  outbuf = gst_buffer_make_writable (outbuf);

  gst_rtp_drop_non_video_meta (rtph265depay, outbuf);

  GST_BUFFER_PTS (outbuf) = timestamp;

  if (keyframe)
    GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);
  else
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

  if (marker)
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_MARKER);

  gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (rtph265depay), outbuf);
}

static void
gst_rtp_h265_depay_handle_nal (GstRtpH265Depay * rtph265depay, GstBuffer * nal,
    GstClockTime in_timestamp, gboolean marker)
{
  GstRTPBaseDepayload *depayload = GST_RTP_BASE_DEPAYLOAD (rtph265depay);
  gint nal_type;
  GstMapInfo map;
  GstBuffer *outbuf = NULL;
  GstClockTime out_timestamp;
  gboolean keyframe, out_keyframe;

  gst_buffer_map (nal, &map, GST_MAP_READ);
  if (G_UNLIKELY (map.size < 5))
    goto short_nal;

  nal_type = (map.data[4] >> 1) & 0x3f;
  GST_DEBUG_OBJECT (rtph265depay, "handle NAL type %d (RTP marker bit %d)",
      nal_type, marker);

  keyframe = NAL_TYPE_IS_KEY (nal_type);

  out_keyframe = keyframe;
  out_timestamp = in_timestamp;

  if (!rtph265depay->byte_stream) {
    if (NAL_TYPE_IS_PARAMETER_SET (nal_type)) {
      gst_rtp_h265_depay_add_vps_sps_pps (rtph265depay,
          gst_buffer_copy_region (nal, GST_BUFFER_COPY_ALL,
              4, gst_buffer_get_size (nal) - 4));
      gst_buffer_unmap (nal, &map);
      gst_buffer_unref (nal);
      return;
    } else if (rtph265depay->sps->len == 0 || rtph265depay->pps->len == 0) {
      /* Down push down any buffer in non-bytestream mode if the SPS/PPS haven't
       * go through yet
       */
      gst_pad_push_event (GST_RTP_BASE_DEPAYLOAD_SINKPAD (depayload),
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
              gst_structure_new ("GstForceKeyUnit",
                  "all-headers", G_TYPE_BOOLEAN, TRUE, NULL)));
      gst_buffer_unmap (nal, &map);
      gst_buffer_unref (nal);
      return;
    }

    if (rtph265depay->new_codec_data &&
        rtph265depay->sps->len > 0 && rtph265depay->pps->len > 0)
      gst_rtp_h265_set_src_caps (rtph265depay);
  }

  if (rtph265depay->merge) {
    gboolean start = FALSE, complete = FALSE;

    /* marker bit isn't mandatory so in the following code we try to detect
     * an AU boundary (see H.265 spec section 7.4.2.4.4) */
    if (!marker) {
      if (NAL_TYPE_IS_CODED_SLICE_SEGMENT (nal_type)) {
        /* A NAL unit (X) ends an access unit if the next-occurring VCL NAL unit (Y) has the high-order bit of the first byte after its NAL unit header equal to 1 */
        start = TRUE;
        if (((map.data[6] >> 7) & 0x01) == 1) {
          complete = TRUE;
        }
      } else if ((nal_type >= 32 && nal_type <= 35)
          || nal_type == 39 || (nal_type >= 41 && nal_type <= 44)
          || (nal_type >= 48 && nal_type <= 55)) {
        /* VPS, SPS, PPS, SEI, ... terminate an access unit */
        complete = TRUE;
      }
      GST_DEBUG_OBJECT (depayload, "start %d, complete %d", start, complete);

      if (complete && rtph265depay->picture_start)
        outbuf = gst_rtp_h265_complete_au (rtph265depay, &out_timestamp,
            &out_keyframe);
    }
    /* add to adapter */
    gst_buffer_unmap (nal, &map);

    GST_DEBUG_OBJECT (depayload, "adding NAL to picture adapter");
    gst_adapter_push (rtph265depay->picture_adapter, nal);
    rtph265depay->last_ts = in_timestamp;
    rtph265depay->last_keyframe |= keyframe;
    rtph265depay->picture_start |= start;

    if (marker)
      outbuf = gst_rtp_h265_complete_au (rtph265depay, &out_timestamp,
          &out_keyframe);
  } else {
    /* no merge, output is input nal */
    GST_DEBUG_OBJECT (depayload, "using NAL as output");
    outbuf = nal;
    gst_buffer_unmap (nal, &map);
  }

  if (outbuf) {
    gst_rtp_h265_depay_push (rtph265depay, outbuf, out_keyframe, out_timestamp,
        marker);
  }

  return;

  /* ERRORS */
short_nal:
  {
    GST_WARNING_OBJECT (depayload, "dropping short NAL");
    gst_buffer_unmap (nal, &map);
    gst_buffer_unref (nal);
    return;
  }
}

static void
gst_rtp_h265_finish_fragmentation_unit (GstRtpH265Depay * rtph265depay)
{
  guint outsize;
  GstMapInfo map;
  GstBuffer *outbuf;

  outsize = gst_adapter_available (rtph265depay->adapter);
  g_assert (outsize >= 4);

  outbuf = gst_adapter_take_buffer (rtph265depay->adapter, outsize);

  gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
  GST_DEBUG_OBJECT (rtph265depay, "output %d bytes", outsize);

  if (rtph265depay->byte_stream) {
    memcpy (map.data, sync_bytes, sizeof (sync_bytes));
  } else {
    GST_WRITE_UINT32_BE (map.data, outsize - 4);
  }
  gst_buffer_unmap (outbuf, &map);

  rtph265depay->current_fu_type = 0;

  gst_rtp_h265_depay_handle_nal (rtph265depay, outbuf,
      rtph265depay->fu_timestamp, rtph265depay->fu_marker);
}

static GstBuffer *
gst_rtp_h265_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpH265Depay *rtph265depay;
  GstBuffer *outbuf = NULL;
  guint8 nal_unit_type;

  rtph265depay = GST_RTP_H265_DEPAY (depayload);

  /* flush remaining data on discont */
  if (GST_BUFFER_IS_DISCONT (rtp->buffer)) {
    gst_adapter_clear (rtph265depay->adapter);
    rtph265depay->wait_start = TRUE;
    rtph265depay->current_fu_type = 0;
  }

  {
    gint payload_len;
    guint8 *payload;
    guint header_len;
    GstMapInfo map;
    guint outsize, nalu_size;
    GstClockTime timestamp;
    gboolean marker;
    guint8 nuh_layer_id, nuh_temporal_id_plus1;
    guint8 S, E;
    guint16 nal_header;
#if 0
    gboolean donl_present = FALSE;
#endif

    timestamp = GST_BUFFER_PTS (rtp->buffer);

    payload_len = gst_rtp_buffer_get_payload_len (rtp);
    payload = gst_rtp_buffer_get_payload (rtp);
    marker = gst_rtp_buffer_get_marker (rtp);

    GST_DEBUG_OBJECT (rtph265depay, "receiving %d bytes", payload_len);

    if (payload_len == 0)
      goto empty_packet;

    /* +---------------+---------------+
     * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |F|   Type    |  LayerId  | TID |
     * +-------------+-----------------+
     *
     * F must be 0.
     *
     */
    nal_unit_type = (payload[0] >> 1) & 0x3f;
    nuh_layer_id = ((payload[0] & 0x01) << 5) | (payload[1] >> 3);      /* should be zero for now but this could change in future HEVC extensions */
    nuh_temporal_id_plus1 = payload[1] & 0x03;

    /* At least two byte header with type */
    header_len = 2;

    GST_DEBUG_OBJECT (rtph265depay,
        "NAL header nal_unit_type %d, nuh_temporal_id_plus1 %d", nal_unit_type,
        nuh_temporal_id_plus1);

    GST_FIXME_OBJECT (rtph265depay, "Assuming DONL field is not present");

    /* FIXME - assuming DONL field is not present for now */
    /*donl_present = (tx-mode == "MST") || (sprop-max-don-diff > 0); */

    /* If FU unit was being processed, but the current nal is of a different
     * type.  Assume that the remote payloader is buggy (didn't set the end bit
     * when the FU ended) and send out what we gathered thusfar */
    if (G_UNLIKELY (rtph265depay->current_fu_type != 0 &&
            nal_unit_type != rtph265depay->current_fu_type))
      gst_rtp_h265_finish_fragmentation_unit (rtph265depay);

    switch (nal_unit_type) {
      case 48:
      {
        GST_DEBUG_OBJECT (rtph265depay, "Processing aggregation packet");

        /* Aggregation packet (section 4.7) */

        /*  An example of an AP packet containing two aggregation units
           without the DONL and DOND fields

           0                   1                   2                   3
           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           |                          RTP Header                           |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           |   PayloadHdr (Type=48)        |         NALU 1 Size           |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           |          NALU 1 HDR           |                               |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         NALU 1 Data           |
           |                   . . .                                       |
           |                                                               |
           +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           |  . . .        | NALU 2 Size                   | NALU 2 HDR    |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           | NALU 2 HDR    |                                               |
           +-+-+-+-+-+-+-+-+              NALU 2 Data                      |
           |                   . . .                                       |
           |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           |                               :...OPTIONAL RTP padding        |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */

        /* strip headers */
        payload += header_len;
        payload_len -= header_len;

        rtph265depay->wait_start = FALSE;

#if 0
        if (donl_present)
          goto not_implemented_donl_present;
#endif

        while (payload_len > 2) {
          gboolean last = FALSE;

          nalu_size = (payload[0] << 8) | payload[1];

          /* don't include nalu_size */
          if (nalu_size > (payload_len - 2))
            nalu_size = payload_len - 2;

          outsize = nalu_size + sizeof (sync_bytes);
          outbuf = gst_buffer_new_and_alloc (outsize);

          gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
          if (rtph265depay->byte_stream) {
            memcpy (map.data, sync_bytes, sizeof (sync_bytes));
          } else {
            GST_WRITE_UINT32_BE (map.data, nalu_size);
          }

          /* strip NALU size */
          payload += 2;
          payload_len -= 2;

          memcpy (map.data + sizeof (sync_bytes), payload, nalu_size);
          gst_buffer_unmap (outbuf, &map);

          gst_rtp_copy_video_meta (rtph265depay, outbuf, rtp->buffer);

          if (payload_len - nalu_size <= 2)
            last = TRUE;

          gst_rtp_h265_depay_handle_nal (rtph265depay, outbuf, timestamp,
              marker && last);

          payload += nalu_size;
          payload_len -= nalu_size;
        }
        break;
      }
      case 49:
      {
        GST_DEBUG_OBJECT (rtph265depay, "Processing Fragmentation Unit");

        /* Fragmentation units (FUs)  Section 4.8 */

        /*    The structure of a Fragmentation Unit (FU)
         *
         *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |    PayloadHdr (Type=49)       |   FU header   | DONL (cond)   |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
         | DONL (cond)   |                                               |
         |-+-+-+-+-+-+-+-+                                               |
         |                         FU payload                            |
         |                                                               |
         |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                               :...OPTIONAL RTP padding        |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *
         *
         */

        /* strip headers */
        payload += header_len;
        payload_len -= header_len;

        /* processing FU header */
        S = (payload[0] & 0x80) == 0x80;
        E = (payload[0] & 0x40) == 0x40;

        GST_DEBUG_OBJECT (rtph265depay,
            "FU header with S %d, E %d, nal_unit_type %d", S, E,
            payload[0] & 0x3f);

        if (rtph265depay->wait_start && !S)
          goto waiting_start;

#if 0
        if (donl_present)
          goto not_implemented_donl_present;
#endif

        if (S) {

          GST_DEBUG_OBJECT (rtph265depay, "Start of Fragmentation Unit");

          /* If a new FU unit started, while still processing an older one.
           * Assume that the remote payloader is buggy (doesn't set the end
           * bit) and send out what we've gathered thusfar */
          if (G_UNLIKELY (rtph265depay->current_fu_type != 0))
            gst_rtp_h265_finish_fragmentation_unit (rtph265depay);

          rtph265depay->current_fu_type = nal_unit_type;
          rtph265depay->fu_timestamp = timestamp;

          rtph265depay->wait_start = FALSE;

          /* reconstruct NAL header */
          nal_header =
              ((payload[0] & 0x3f) << 9) | (nuh_layer_id << 3) |
              nuh_temporal_id_plus1;

          /* go back one byte so we can copy the payload + two bytes more in the front which
           * will be overwritten by the nal_header
           */
          payload -= 1;
          payload_len += 1;

          nalu_size = payload_len;
          outsize = nalu_size + sizeof (sync_bytes);
          outbuf = gst_buffer_new_and_alloc (outsize);

          gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
          if (rtph265depay->byte_stream) {
            GST_WRITE_UINT32_BE (map.data, 0x00000001);
          } else {
            /* will be fixed up in finish_fragmentation_unit() */
            GST_WRITE_UINT32_BE (map.data, 0xffffffff);
          }
          memcpy (map.data + sizeof (sync_bytes), payload, nalu_size);
          map.data[4] = nal_header >> 8;
          map.data[5] = nal_header & 0xff;
          gst_buffer_unmap (outbuf, &map);

          gst_rtp_copy_video_meta (rtph265depay, outbuf, rtp->buffer);

          GST_DEBUG_OBJECT (rtph265depay, "queueing %d bytes", outsize);

          /* and assemble in the adapter */
          gst_adapter_push (rtph265depay->adapter, outbuf);
        } else {

          GST_DEBUG_OBJECT (rtph265depay,
              "Following part of Fragmentation Unit");

          /* strip off FU header byte */
          payload += 1;
          payload_len -= 1;

          outsize = payload_len;
          outbuf = gst_buffer_new_and_alloc (outsize);
          gst_buffer_fill (outbuf, 0, payload, outsize);

          gst_rtp_copy_video_meta (rtph265depay, outbuf, rtp->buffer);

          GST_DEBUG_OBJECT (rtph265depay, "queueing %d bytes", outsize);

          /* and assemble in the adapter */
          gst_adapter_push (rtph265depay->adapter, outbuf);
        }

        outbuf = NULL;
        rtph265depay->fu_marker = marker;

        /* if NAL unit ends, flush the adapter */
        if (E) {
          gst_rtp_h265_finish_fragmentation_unit (rtph265depay);
          GST_DEBUG_OBJECT (rtph265depay, "End of Fragmentation Unit");
        }
        break;
      }
      case 50:
        goto not_implemented;   /* PACI packets  Section 4.9 */
      default:
      {
        rtph265depay->wait_start = FALSE;

        /* All other cases: Single NAL unit packet   Section 4.6 */
        /* the entire payload is the output buffer */

#if 0
        if (donl_present)
          goto not_implemented_donl_present;
#endif

        nalu_size = payload_len;
        outsize = nalu_size + sizeof (sync_bytes);
        outbuf = gst_buffer_new_and_alloc (outsize);

        gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
        if (rtph265depay->byte_stream) {
          memcpy (map.data, sync_bytes, sizeof (sync_bytes));
        } else {
          GST_WRITE_UINT32_BE (map.data, nalu_size);
        }
        memcpy (map.data + 4, payload, nalu_size);
        gst_buffer_unmap (outbuf, &map);

        gst_rtp_copy_video_meta (rtph265depay, outbuf, rtp->buffer);

        gst_rtp_h265_depay_handle_nal (rtph265depay, outbuf, timestamp, marker);
        break;
      }
    }
  }

  return NULL;

  /* ERRORS */
empty_packet:
  {
    GST_DEBUG_OBJECT (rtph265depay, "empty packet");
    return NULL;
  }
waiting_start:
  {
    GST_DEBUG_OBJECT (rtph265depay, "waiting for start");
    return NULL;
  }
#if 0
not_implemented_donl_present:
  {
    GST_ELEMENT_ERROR (rtph265depay, STREAM, FORMAT,
        (NULL), ("DONL field present not supported yet"));
    return NULL;
  }
#endif
not_implemented:
  {
    GST_ELEMENT_ERROR (rtph265depay, STREAM, FORMAT,
        (NULL), ("NAL unit type %d not supported yet", nal_unit_type));
    return NULL;
  }
}

static gboolean
gst_rtp_h265_depay_handle_event (GstRTPBaseDepayload * depay, GstEvent * event)
{
  GstRtpH265Depay *rtph265depay;

  rtph265depay = GST_RTP_H265_DEPAY (depay);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_h265_depay_reset (rtph265depay, FALSE);
      break;
    case GST_EVENT_EOS:
      gst_rtp_h265_depay_drain (rtph265depay);
      break;
    default:
      break;
  }

  return
      GST_RTP_BASE_DEPAYLOAD_CLASS (parent_class)->handle_event (depay, event);
}

static GstStateChangeReturn
gst_rtp_h265_depay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpH265Depay *rtph265depay;
  GstStateChangeReturn ret;

  rtph265depay = GST_RTP_H265_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_h265_depay_reset (rtph265depay, TRUE);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_h265_depay_reset (rtph265depay, TRUE);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_h265_depay_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (rtph265depay_debug, "rtph265depay", 0,
      "H265 Video RTP Depayloader");

  return gst_element_register (plugin, "rtph265depay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_H265_DEPAY);
}
