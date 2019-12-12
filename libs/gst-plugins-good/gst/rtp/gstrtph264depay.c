/* GStreamer
 * Copyright (C) <2006> Wim Taymans <wim.taymans@gmail.com>
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
#include <gst/pbutils/pbutils.h>
#include <gst/video/video.h>
#include "gstrtph264depay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtph264depay_debug);
#define GST_CAT_DEFAULT (rtph264depay_debug)

/* This is what we'll default to when downstream hasn't
 * expressed a restriction or preference via caps */
#define DEFAULT_BYTE_STREAM   TRUE
#define DEFAULT_ACCESS_UNIT   FALSE

/* 3 zero bytes syncword */
static const guint8 sync_bytes[] = { 0, 0, 0, 1 };

static GstStaticPadTemplate gst_rtp_h264_depay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h264, "
        "stream-format = (string) avc, alignment = (string) au; "
        "video/x-h264, "
        "stream-format = (string) byte-stream, alignment = (string) { nal, au }")
    );

static GstStaticPadTemplate gst_rtp_h264_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H264\"")
    /* optional parameters */
    /* "profile-level-id = (string) ANY, " */
    /* "max-mbps = (string) ANY, " */
    /* "max-fs = (string) ANY, " */
    /* "max-cpb = (string) ANY, " */
    /* "max-dpb = (string) ANY, " */
    /* "max-br = (string) ANY, " */
    /* "redundant-pic-cap = (string) { \"0\", \"1\" }, " */
    /* "sprop-parameter-sets = (string) ANY, " */
    /* "parameter-add = (string) { \"0\", \"1\" }, " */
    /* "packetization-mode = (string) { \"0\", \"1\", \"2\" }, " */
    /* "sprop-interleaving-depth = (string) ANY, " */
    /* "sprop-deint-buf-req = (string) ANY, " */
    /* "deint-buf-cap = (string) ANY, " */
    /* "sprop-init-buf-time = (string) ANY, " */
    /* "sprop-max-don-diff = (string) ANY, " */
    /* "max-rcmd-nalu-size = (string) ANY " */
    );

#define gst_rtp_h264_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpH264Depay, gst_rtp_h264_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static void gst_rtp_h264_depay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_h264_depay_change_state (GstElement *
    element, GstStateChange transition);

static GstBuffer *gst_rtp_h264_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
static gboolean gst_rtp_h264_depay_setcaps (GstRTPBaseDepayload * filter,
    GstCaps * caps);
static gboolean gst_rtp_h264_depay_handle_event (GstRTPBaseDepayload * depay,
    GstEvent * event);
static GstBuffer *gst_rtp_h264_complete_au (GstRtpH264Depay * rtph264depay,
    GstClockTime * out_timestamp, gboolean * out_keyframe);
static void gst_rtp_h264_depay_push (GstRtpH264Depay * rtph264depay,
    GstBuffer * outbuf, gboolean keyframe, GstClockTime timestamp,
    gboolean marker);

static void
gst_rtp_h264_depay_class_init (GstRtpH264DepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gobject_class->finalize = gst_rtp_h264_depay_finalize;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h264_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h264_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP H264 depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts H264 video from RTP packets (RFC 3984)",
      "Wim Taymans <wim.taymans@gmail.com>");
  gstelement_class->change_state = gst_rtp_h264_depay_change_state;

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_h264_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_h264_depay_setcaps;
  gstrtpbasedepayload_class->handle_event = gst_rtp_h264_depay_handle_event;
}

static void
gst_rtp_h264_depay_init (GstRtpH264Depay * rtph264depay)
{
  rtph264depay->adapter = gst_adapter_new ();
  rtph264depay->picture_adapter = gst_adapter_new ();
  rtph264depay->byte_stream = DEFAULT_BYTE_STREAM;
  rtph264depay->merge = DEFAULT_ACCESS_UNIT;
  rtph264depay->sps = g_ptr_array_new_with_free_func (
      (GDestroyNotify) gst_buffer_unref);
  rtph264depay->pps = g_ptr_array_new_with_free_func (
      (GDestroyNotify) gst_buffer_unref);
}

static void
gst_rtp_h264_depay_reset (GstRtpH264Depay * rtph264depay, gboolean hard)
{
  gst_adapter_clear (rtph264depay->adapter);
  rtph264depay->wait_start = TRUE;
  gst_adapter_clear (rtph264depay->picture_adapter);
  rtph264depay->picture_start = FALSE;
  rtph264depay->last_keyframe = FALSE;
  rtph264depay->last_ts = 0;
  rtph264depay->current_fu_type = 0;
  rtph264depay->new_codec_data = FALSE;
  g_ptr_array_set_size (rtph264depay->sps, 0);
  g_ptr_array_set_size (rtph264depay->pps, 0);

  if (hard) {
    if (rtph264depay->allocator != NULL) {
      gst_object_unref (rtph264depay->allocator);
      rtph264depay->allocator = NULL;
    }
    gst_allocation_params_init (&rtph264depay->params);
  }
}

static void
gst_rtp_h264_depay_drain (GstRtpH264Depay * rtph264depay)
{
  GstClockTime timestamp;
  gboolean keyframe;
  GstBuffer *outbuf;

  if (!rtph264depay->picture_start)
    return;

  outbuf = gst_rtp_h264_complete_au (rtph264depay, &timestamp, &keyframe);
  if (outbuf)
    gst_rtp_h264_depay_push (rtph264depay, outbuf, keyframe, timestamp, FALSE);
}

static void
gst_rtp_h264_depay_finalize (GObject * object)
{
  GstRtpH264Depay *rtph264depay;

  rtph264depay = GST_RTP_H264_DEPAY (object);

  if (rtph264depay->codec_data)
    gst_buffer_unref (rtph264depay->codec_data);

  g_object_unref (rtph264depay->adapter);
  g_object_unref (rtph264depay->picture_adapter);

  g_ptr_array_free (rtph264depay->sps, TRUE);
  g_ptr_array_free (rtph264depay->pps, TRUE);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_h264_depay_negotiate (GstRtpH264Depay * rtph264depay)
{
  GstCaps *caps;
  gint byte_stream = -1;
  gint merge = -1;

  caps =
      gst_pad_get_allowed_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (rtph264depay));

  GST_DEBUG_OBJECT (rtph264depay, "allowed caps: %" GST_PTR_FORMAT, caps);

  if (caps) {
    if (gst_caps_get_size (caps) > 0) {
      GstStructure *s = gst_caps_get_structure (caps, 0);
      const gchar *str = NULL;

      if ((str = gst_structure_get_string (s, "stream-format"))) {
        if (strcmp (str, "avc") == 0) {
          byte_stream = FALSE;
        } else if (strcmp (str, "byte-stream") == 0) {
          byte_stream = TRUE;
        } else {
          GST_DEBUG_OBJECT (rtph264depay, "unknown stream-format: %s", str);
        }
      }

      if ((str = gst_structure_get_string (s, "alignment"))) {
        if (strcmp (str, "au") == 0) {
          merge = TRUE;
        } else if (strcmp (str, "nal") == 0) {
          merge = FALSE;
        } else {
          GST_DEBUG_OBJECT (rtph264depay, "unknown alignment: %s", str);
        }
      }
    }
    gst_caps_unref (caps);
  }

  if (byte_stream != -1) {
    GST_DEBUG_OBJECT (rtph264depay, "downstream requires byte-stream %d",
        byte_stream);
    rtph264depay->byte_stream = byte_stream;
  } else {
    GST_DEBUG_OBJECT (rtph264depay, "defaulting to byte-stream %d",
        DEFAULT_BYTE_STREAM);
    rtph264depay->byte_stream = DEFAULT_BYTE_STREAM;
  }
  if (merge != -1) {
    GST_DEBUG_OBJECT (rtph264depay, "downstream requires merge %d", merge);
    rtph264depay->merge = merge;
  } else {
    GST_DEBUG_OBJECT (rtph264depay, "defaulting to merge %d",
        DEFAULT_ACCESS_UNIT);
    rtph264depay->merge = DEFAULT_ACCESS_UNIT;
  }
}

static gboolean
parse_sps (GstMapInfo * map, guint32 * sps_id)
{
  GstBitReader br = GST_BIT_READER_INIT (map->data + 4,
      map->size - 4);

  if (map->size < 5)
    return FALSE;

  if (!gst_rtp_read_golomb (&br, sps_id))
    return FALSE;

  return TRUE;
}

static gboolean
parse_pps (GstMapInfo * map, guint32 * sps_id, guint32 * pps_id)
{
  GstBitReader br = GST_BIT_READER_INIT (map->data + 1,
      map->size - 1);

  if (map->size < 2)
    return FALSE;

  if (!gst_rtp_read_golomb (&br, pps_id))
    return FALSE;
  if (!gst_rtp_read_golomb (&br, sps_id))
    return FALSE;

  return TRUE;
}

static gboolean
gst_rtp_h264_depay_set_output_caps (GstRtpH264Depay * rtph264depay,
    GstCaps * caps)
{
  GstAllocationParams params;
  GstAllocator *allocator = NULL;
  GstPad *srcpad;
  gboolean res;

  gst_allocation_params_init (&params);

  srcpad = GST_RTP_BASE_DEPAYLOAD_SRCPAD (rtph264depay);
  res = gst_pad_set_caps (srcpad, caps);
  if (res) {
    GstQuery *query;

    query = gst_query_new_allocation (caps, TRUE);
    if (!gst_pad_peer_query (srcpad, query)) {
      GST_DEBUG_OBJECT (rtph264depay, "downstream ALLOCATION query failed");
    }

    if (gst_query_get_n_allocation_params (query) > 0) {
      gst_query_parse_nth_allocation_param (query, 0, &allocator, &params);
    }

    gst_query_unref (query);
  }

  if (rtph264depay->allocator)
    gst_object_unref (rtph264depay->allocator);

  rtph264depay->allocator = allocator;
  rtph264depay->params = params;

  return res;
}

static gboolean
gst_rtp_h264_set_src_caps (GstRtpH264Depay * rtph264depay)
{
  gboolean res = TRUE;
  GstCaps *srccaps;
  GstCaps *old_caps;
  GstPad *srcpad;

  if (!rtph264depay->byte_stream &&
      (!rtph264depay->new_codec_data ||
          rtph264depay->sps->len == 0 || rtph264depay->pps->len == 0))
    return TRUE;

  srccaps = gst_caps_new_simple ("video/x-h264",
      "stream-format", G_TYPE_STRING,
      rtph264depay->byte_stream ? "byte-stream" : "avc",
      "alignment", G_TYPE_STRING, rtph264depay->merge ? "au" : "nal", NULL);

  if (!rtph264depay->byte_stream) {
    GstBuffer *codec_data;
    GstMapInfo map;
    GstMapInfo nalmap;
    guint8 *data;
    guint len;
    guint new_size;
    guint i;
    guchar level = 0;
    guchar profile_compat = G_MAXUINT8;

    /* start with 7 bytes header */
    len = 7;
    /* count sps & pps */
    for (i = 0; i < rtph264depay->sps->len; i++)
      len += 2 + gst_buffer_get_size (g_ptr_array_index (rtph264depay->sps, i));
    for (i = 0; i < rtph264depay->pps->len; i++)
      len += 2 + gst_buffer_get_size (g_ptr_array_index (rtph264depay->pps, i));

    codec_data = gst_buffer_new_and_alloc (len);
    gst_buffer_map (codec_data, &map, GST_MAP_READWRITE);
    data = map.data;

    /* 8 bits version == 1 */
    *data++ = 1;

    /* According to: ISO/IEC 14496-15:2004(E) section 5.2.4.1
     * The level is the max level of all SPSes
     * A profile compat bit can only be set if all SPSes include that bit
     */
    for (i = 0; i < rtph264depay->sps->len; i++) {
      gst_buffer_map (g_ptr_array_index (rtph264depay->sps, i), &nalmap,
          GST_MAP_READ);
      profile_compat &= nalmap.data[2];
      level = MAX (level, nalmap.data[3]);
      gst_buffer_unmap (g_ptr_array_index (rtph264depay->sps, i), &nalmap);
    }

    /* Assume all SPSes use the same profile, so extract from the first SPS */
    gst_buffer_map (g_ptr_array_index (rtph264depay->sps, 0), &nalmap,
        GST_MAP_READ);
    *data++ = nalmap.data[1];
    gst_buffer_unmap (g_ptr_array_index (rtph264depay->sps, 0), &nalmap);
    *data++ = profile_compat;
    *data++ = level;

    /* 6 bits reserved | 2 bits lengthSizeMinusOn */
    *data++ = 0xff;
    /* 3 bits reserved | 5 bits numOfSequenceParameterSets */
    *data++ = 0xe0 | (rtph264depay->sps->len & 0x1f);

    /* copy all SPS */
    for (i = 0; i < rtph264depay->sps->len; i++) {
      gst_buffer_map (g_ptr_array_index (rtph264depay->sps, i), &nalmap,
          GST_MAP_READ);

      GST_DEBUG_OBJECT (rtph264depay, "copy SPS %d of length %u", i,
          (guint) nalmap.size);
      GST_WRITE_UINT16_BE (data, nalmap.size);
      data += 2;
      memcpy (data, nalmap.data, nalmap.size);
      data += nalmap.size;
      gst_buffer_unmap (g_ptr_array_index (rtph264depay->sps, i), &nalmap);
    }

    /* 8 bits numOfPictureParameterSets */
    *data++ = rtph264depay->pps->len;
    /* copy all PPS */
    for (i = 0; i < rtph264depay->pps->len; i++) {
      gst_buffer_map (g_ptr_array_index (rtph264depay->pps, i), &nalmap,
          GST_MAP_READ);

      GST_DEBUG_OBJECT (rtph264depay, "copy PPS %d of length %u", i,
          (guint) nalmap.size);
      GST_WRITE_UINT16_BE (data, nalmap.size);
      data += 2;
      memcpy (data, nalmap.data, nalmap.size);
      data += nalmap.size;
      gst_buffer_unmap (g_ptr_array_index (rtph264depay->pps, i), &nalmap);
    }

    new_size = data - map.data;
    gst_buffer_unmap (codec_data, &map);
    gst_buffer_set_size (codec_data, new_size);

    gst_caps_set_simple (srccaps,
        "codec_data", GST_TYPE_BUFFER, codec_data, NULL);
    gst_buffer_unref (codec_data);
  }

  /* Set profile a level from SPS */
  {
    gint i;
    GstBuffer *max_level_sps = NULL;
    gint level = 0;
    GstMapInfo nalmap;

    /* Get the SPS with the highest level. We assume
     * all SPS have the same profile */
    for (i = 0; i < rtph264depay->sps->len; i++) {
      gst_buffer_map (g_ptr_array_index (rtph264depay->sps, i), &nalmap,
          GST_MAP_READ);
      if (level == 0 || level < nalmap.data[3]) {
        max_level_sps = g_ptr_array_index (rtph264depay->sps, i);
        level = nalmap.data[3];
      }
      gst_buffer_unmap (g_ptr_array_index (rtph264depay->sps, i), &nalmap);
    }

    if (max_level_sps) {
      gst_buffer_map (max_level_sps, &nalmap, GST_MAP_READ);
      gst_codec_utils_h264_caps_set_level_and_profile (srccaps, nalmap.data + 1,
          nalmap.size - 1);
      gst_buffer_unmap (max_level_sps, &nalmap);
    }
  }

  srcpad = GST_RTP_BASE_DEPAYLOAD_SRCPAD (rtph264depay);

  old_caps = gst_pad_get_current_caps (srcpad);

  if (old_caps == NULL || !gst_caps_is_equal (srccaps, old_caps)) {
    res = gst_rtp_h264_depay_set_output_caps (rtph264depay, srccaps);
  }

  gst_clear_caps (&old_caps);
  gst_caps_unref (srccaps);

  /* Insert SPS and PPS into the stream on next opportunity (if bytestream) */
  if (rtph264depay->byte_stream
      && (rtph264depay->sps->len > 0 || rtph264depay->pps->len > 0)) {
    gint i;
    GstBuffer *codec_data;
    GstMapInfo map;
    guint8 *data;
    guint len = 0;

    for (i = 0; i < rtph264depay->sps->len; i++) {
      len += 4 + gst_buffer_get_size (g_ptr_array_index (rtph264depay->sps, i));
    }

    for (i = 0; i < rtph264depay->pps->len; i++) {
      len += 4 + gst_buffer_get_size (g_ptr_array_index (rtph264depay->pps, i));
    }

    codec_data = gst_buffer_new_and_alloc (len);
    gst_buffer_map (codec_data, &map, GST_MAP_WRITE);
    data = map.data;

    for (i = 0; i < rtph264depay->sps->len; i++) {
      GstBuffer *sps_buf = g_ptr_array_index (rtph264depay->sps, i);
      guint sps_size = gst_buffer_get_size (sps_buf);

      if (rtph264depay->byte_stream)
        memcpy (data, sync_bytes, sizeof (sync_bytes));
      else
        GST_WRITE_UINT32_BE (data, sps_size);
      gst_buffer_extract (sps_buf, 0, data + 4, -1);
      data += 4 + sps_size;
    }

    for (i = 0; i < rtph264depay->pps->len; i++) {
      GstBuffer *pps_buf = g_ptr_array_index (rtph264depay->pps, i);
      guint pps_size = gst_buffer_get_size (pps_buf);

      if (rtph264depay->byte_stream)
        memcpy (data, sync_bytes, sizeof (sync_bytes));
      else
        GST_WRITE_UINT32_BE (data, pps_size);
      gst_buffer_extract (pps_buf, 0, data + 4, -1);
      data += 4 + pps_size;
    }

    gst_buffer_unmap (codec_data, &map);
    if (rtph264depay->codec_data)
      gst_buffer_unref (rtph264depay->codec_data);
    rtph264depay->codec_data = codec_data;
  }

  if (res)
    rtph264depay->new_codec_data = FALSE;

  return res;
}

gboolean
gst_rtp_h264_add_sps_pps (GstElement * rtph264, GPtrArray * sps_array,
    GPtrArray * pps_array, GstBuffer * nal)
{
  GstMapInfo map;
  guchar type;
  guint i;

  gst_buffer_map (nal, &map, GST_MAP_READ);

  type = map.data[0] & 0x1f;

  if (type == 7) {
    guint32 sps_id;

    if (!parse_sps (&map, &sps_id)) {
      GST_WARNING_OBJECT (rtph264, "Invalid SPS,"
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
          GST_LOG_OBJECT (rtph264, "Unchanged SPS %u, not updating", sps_id);
          gst_buffer_unmap (sps, &spsmap);
          goto drop;
        } else {
          gst_buffer_unmap (sps, &spsmap);
          g_ptr_array_remove_index_fast (sps_array, i);
          g_ptr_array_add (sps_array, nal);
          GST_LOG_OBJECT (rtph264, "Modified SPS %u, replacing", sps_id);
          goto done;
        }
      }
      gst_buffer_unmap (sps, &spsmap);
    }
    GST_LOG_OBJECT (rtph264, "Adding new SPS %u", sps_id);
    g_ptr_array_add (sps_array, nal);
  } else if (type == 8) {
    guint32 sps_id;
    guint32 pps_id;

    if (!parse_pps (&map, &sps_id, &pps_id)) {
      GST_WARNING_OBJECT (rtph264, "Invalid PPS,"
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
          GST_LOG_OBJECT (rtph264, "Unchanged PPS %u:%u, not updating", sps_id,
              pps_id);
          gst_buffer_unmap (pps, &ppsmap);
          goto drop;
        } else {
          gst_buffer_unmap (pps, &ppsmap);
          g_ptr_array_remove_index_fast (pps_array, i);
          g_ptr_array_add (pps_array, nal);
          GST_LOG_OBJECT (rtph264, "Modified PPS %u:%u, replacing",
              sps_id, pps_id);
          goto done;
        }
      }
      gst_buffer_unmap (pps, &ppsmap);
    }
    GST_LOG_OBJECT (rtph264, "Adding new PPS %u:%i", sps_id, pps_id);
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
gst_rtp_h264_depay_add_sps_pps (GstRtpH264Depay * rtph264depay, GstBuffer * nal)
{
  if (gst_rtp_h264_add_sps_pps (GST_ELEMENT (rtph264depay),
          rtph264depay->sps, rtph264depay->pps, nal))
    rtph264depay->new_codec_data = TRUE;
}

static gboolean
gst_rtp_h264_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  gint clock_rate;
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  GstRtpH264Depay *rtph264depay;
  const gchar *ps;
  GstBuffer *codec_data;
  GstMapInfo map;
  guint8 *ptr;

  rtph264depay = GST_RTP_H264_DEPAY (depayload);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;
  depayload->clock_rate = clock_rate;

  /* Base64 encoded, comma separated config NALs */
  ps = gst_structure_get_string (structure, "sprop-parameter-sets");

  /* negotiate with downstream w.r.t. output format and alignment */
  gst_rtp_h264_depay_negotiate (rtph264depay);

  if (rtph264depay->byte_stream && ps != NULL) {
    /* for bytestream we only need the parameter sets but we don't error out
     * when they are not there, we assume they are in the stream. */
    gchar **params;
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
    if (rtph264depay->codec_data)
      gst_buffer_unref (rtph264depay->codec_data);
    rtph264depay->codec_data = codec_data;
  } else if (!rtph264depay->byte_stream) {
    gchar **params;
    gint i;

    if (ps == NULL)
      goto incomplete_caps;

    params = g_strsplit (ps, ",", 0);

    GST_DEBUG_OBJECT (depayload, "we have %d params", g_strv_length (params));

    /* start with 7 bytes header */
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
          ((nalmap.data[0] & 0x1f) == 7) ? "SPS" : "PPS");

      gst_buffer_unmap (nal, &nalmap);
      gst_buffer_set_size (nal, nal_len);

      gst_rtp_h264_depay_add_sps_pps (rtph264depay, nal);
    }
    g_strfreev (params);

    if (rtph264depay->sps->len == 0 || rtph264depay->pps->len == 0)
      goto incomplete_caps;
  }

  return gst_rtp_h264_set_src_caps (rtph264depay);

  /* ERRORS */
incomplete_caps:
  {
    GST_DEBUG_OBJECT (depayload, "we have incomplete caps,"
        " doing setcaps later");
    return TRUE;
  }
}

static GstBuffer *
gst_rtp_h264_depay_allocate_output_buffer (GstRtpH264Depay * depay, gsize size)
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
gst_rtp_h264_complete_au (GstRtpH264Depay * rtph264depay,
    GstClockTime * out_timestamp, gboolean * out_keyframe)
{
  GstBufferList *list;
  GstMapInfo outmap;
  GstBuffer *outbuf;
  guint outsize, offset = 0;
  gint b, n_bufs, m, n_mem;

  /* we had a picture in the adapter and we completed it */
  GST_DEBUG_OBJECT (rtph264depay, "taking completed AU");
  outsize = gst_adapter_available (rtph264depay->picture_adapter);

  outbuf = gst_rtp_h264_depay_allocate_output_buffer (rtph264depay, outsize);

  if (outbuf == NULL)
    return NULL;

  if (!gst_buffer_map (outbuf, &outmap, GST_MAP_WRITE))
    return NULL;

  list = gst_adapter_take_buffer_list (rtph264depay->picture_adapter, outsize);

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

    gst_rtp_copy_video_meta (rtph264depay, outbuf, buf);
  }
  gst_buffer_list_unref (list);
  gst_buffer_unmap (outbuf, &outmap);

  *out_timestamp = rtph264depay->last_ts;
  *out_keyframe = rtph264depay->last_keyframe;

  rtph264depay->last_keyframe = FALSE;
  rtph264depay->picture_start = FALSE;

  return outbuf;
}

static void
gst_rtp_h264_depay_push (GstRtpH264Depay * rtph264depay, GstBuffer * outbuf,
    gboolean keyframe, GstClockTime timestamp, gboolean marker)
{
  /* prepend codec_data */
  if (rtph264depay->codec_data) {
    GST_DEBUG_OBJECT (rtph264depay, "prepending codec_data");
    gst_rtp_copy_video_meta (rtph264depay, rtph264depay->codec_data, outbuf);
    outbuf = gst_buffer_append (rtph264depay->codec_data, outbuf);
    rtph264depay->codec_data = NULL;
    keyframe = TRUE;
  }
  outbuf = gst_buffer_make_writable (outbuf);

  gst_rtp_drop_non_video_meta (rtph264depay, outbuf);

  GST_BUFFER_PTS (outbuf) = timestamp;

  if (keyframe)
    GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);
  else
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

  if (marker)
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_MARKER);

  gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (rtph264depay), outbuf);
}

/* SPS/PPS/IDR considered key, all others DELTA;
 * so downstream waiting for keyframe can pick up at SPS/PPS/IDR */
#define NAL_TYPE_IS_KEY(nt) (((nt) == 5) || ((nt) == 7) || ((nt) == 8))

static void
gst_rtp_h264_depay_handle_nal (GstRtpH264Depay * rtph264depay, GstBuffer * nal,
    GstClockTime in_timestamp, gboolean marker)
{
  GstRTPBaseDepayload *depayload = GST_RTP_BASE_DEPAYLOAD (rtph264depay);
  gint nal_type;
  GstMapInfo map;
  GstBuffer *outbuf = NULL;
  GstClockTime out_timestamp;
  gboolean keyframe, out_keyframe;

  gst_buffer_map (nal, &map, GST_MAP_READ);
  if (G_UNLIKELY (map.size < 5))
    goto short_nal;

  nal_type = map.data[4] & 0x1f;
  GST_DEBUG_OBJECT (rtph264depay, "handle NAL type %d", nal_type);

  keyframe = NAL_TYPE_IS_KEY (nal_type);

  out_keyframe = keyframe;
  out_timestamp = in_timestamp;

  if (!rtph264depay->byte_stream) {
    if (nal_type == 7 || nal_type == 8) {
      gst_rtp_h264_depay_add_sps_pps (rtph264depay,
          gst_buffer_copy_region (nal, GST_BUFFER_COPY_ALL,
              4, gst_buffer_get_size (nal) - 4));
      gst_buffer_unmap (nal, &map);
      gst_buffer_unref (nal);
      return;
    } else if (rtph264depay->sps->len == 0 || rtph264depay->pps->len == 0) {
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

    if (rtph264depay->new_codec_data &&
        rtph264depay->sps->len > 0 && rtph264depay->pps->len > 0)
      gst_rtp_h264_set_src_caps (rtph264depay);
  }


  if (rtph264depay->merge) {
    gboolean start = FALSE, complete = FALSE;

    /* marker bit isn't mandatory so in the following code we try to guess
     * an AU boundary by detecting a new picture start */
    if (!marker) {
      /* consider a coded slices (IDR or not) to start a picture,
       * (so ending the previous one) if first_mb_in_slice == 0
       * (non-0 is part of previous one) */
      /* NOTE this is not entirely according to Access Unit specs in 7.4.1.2.4,
       * but in practice it works in sane cases, needs not much parsing,
       * and also works with broken frame_num in NAL (where spec-wise would fail) */
      /* FIXME: this code isn't correct for interlaced content as AUs should be
       * constructed with pairs of fields and the guess here will just push out
       * AUs with a single field in it */
      if (nal_type == 1 || nal_type == 2 || nal_type == 5) {
        /* we have a picture start */
        start = TRUE;
        if (map.data[5] & 0x80) {
          /* first_mb_in_slice == 0 completes a picture */
          complete = TRUE;
        }
      } else if (nal_type >= 6 && nal_type <= 9) {
        /* SEI, SPS, PPS, AU terminate picture */
        complete = TRUE;
      }
      GST_DEBUG_OBJECT (depayload, "start %d, complete %d", start, complete);

      if (complete && rtph264depay->picture_start)
        outbuf = gst_rtp_h264_complete_au (rtph264depay, &out_timestamp,
            &out_keyframe);
    }
    /* add to adapter */
    gst_buffer_unmap (nal, &map);

    GST_DEBUG_OBJECT (depayload, "adding NAL to picture adapter");
    gst_adapter_push (rtph264depay->picture_adapter, nal);
    rtph264depay->last_ts = in_timestamp;
    rtph264depay->last_keyframe |= keyframe;
    rtph264depay->picture_start |= start;

    if (marker)
      outbuf = gst_rtp_h264_complete_au (rtph264depay, &out_timestamp,
          &out_keyframe);
  } else {
    /* no merge, output is input nal */
    GST_DEBUG_OBJECT (depayload, "using NAL as output");
    outbuf = nal;
    gst_buffer_unmap (nal, &map);
  }

  if (outbuf) {
    gst_rtp_h264_depay_push (rtph264depay, outbuf, out_keyframe, out_timestamp,
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
gst_rtp_h264_finish_fragmentation_unit (GstRtpH264Depay * rtph264depay)
{
  guint outsize;
  GstMapInfo map;
  GstBuffer *outbuf;

  outsize = gst_adapter_available (rtph264depay->adapter);
  outbuf = gst_adapter_take_buffer (rtph264depay->adapter, outsize);

  gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
  GST_DEBUG_OBJECT (rtph264depay, "output %d bytes", outsize);

  if (rtph264depay->byte_stream) {
    memcpy (map.data, sync_bytes, sizeof (sync_bytes));
  } else {
    outsize -= 4;
    map.data[0] = (outsize >> 24);
    map.data[1] = (outsize >> 16);
    map.data[2] = (outsize >> 8);
    map.data[3] = (outsize);
  }
  gst_buffer_unmap (outbuf, &map);

  rtph264depay->current_fu_type = 0;

  gst_rtp_h264_depay_handle_nal (rtph264depay, outbuf,
      rtph264depay->fu_timestamp, rtph264depay->fu_marker);
}

static GstBuffer *
gst_rtp_h264_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpH264Depay *rtph264depay;
  GstBuffer *outbuf = NULL;
  guint8 nal_unit_type;

  rtph264depay = GST_RTP_H264_DEPAY (depayload);

  /* flush remaining data on discont */
  if (GST_BUFFER_IS_DISCONT (rtp->buffer)) {
    gst_adapter_clear (rtph264depay->adapter);
    rtph264depay->wait_start = TRUE;
    rtph264depay->current_fu_type = 0;
  }

  {
    gint payload_len;
    guint8 *payload;
    guint header_len;
    guint8 nal_ref_idc;
    GstMapInfo map;
    guint outsize, nalu_size;
    GstClockTime timestamp;
    gboolean marker;

    timestamp = GST_BUFFER_PTS (rtp->buffer);

    payload_len = gst_rtp_buffer_get_payload_len (rtp);
    payload = gst_rtp_buffer_get_payload (rtp);
    marker = gst_rtp_buffer_get_marker (rtp);

    GST_DEBUG_OBJECT (rtph264depay, "receiving %d bytes", payload_len);

    if (payload_len == 0)
      goto empty_packet;

    /* +---------------+
     * |0|1|2|3|4|5|6|7|
     * +-+-+-+-+-+-+-+-+
     * |F|NRI|  Type   |
     * +---------------+
     *
     * F must be 0.
     */
    nal_ref_idc = (payload[0] & 0x60) >> 5;
    nal_unit_type = payload[0] & 0x1f;

    /* at least one byte header with type */
    header_len = 1;

    GST_DEBUG_OBJECT (rtph264depay, "NRI %d, Type %d %s", nal_ref_idc,
        nal_unit_type, marker ? "marker" : "");

    /* If FU unit was being processed, but the current nal is of a different
     * type.  Assume that the remote payloader is buggy (didn't set the end bit
     * when the FU ended) and send out what we gathered thusfar */
    if (G_UNLIKELY (rtph264depay->current_fu_type != 0 &&
            nal_unit_type != rtph264depay->current_fu_type))
      gst_rtp_h264_finish_fragmentation_unit (rtph264depay);

    switch (nal_unit_type) {
      case 0:
      case 30:
      case 31:
        /* undefined */
        goto undefined_type;
      case 25:
        /* STAP-B    Single-time aggregation packet     5.7.1 */
        /* 2 byte extra header for DON */
        header_len += 2;
        /* fallthrough */
      case 24:
      {
        /* strip headers */
        payload += header_len;
        payload_len -= header_len;

        rtph264depay->wait_start = FALSE;


        /* STAP-A    Single-time aggregation packet     5.7.1 */
        while (payload_len > 2) {
          gboolean last = FALSE;

          /*                      1
           *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
           * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           * |         NALU Size             |
           * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           */
          nalu_size = (payload[0] << 8) | payload[1];

          /* don't include nalu_size */
          if (nalu_size > (payload_len - 2))
            nalu_size = payload_len - 2;

          outsize = nalu_size + sizeof (sync_bytes);
          outbuf = gst_buffer_new_and_alloc (outsize);

          gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
          if (rtph264depay->byte_stream) {
            memcpy (map.data, sync_bytes, sizeof (sync_bytes));
          } else {
            map.data[0] = map.data[1] = 0;
            map.data[2] = payload[0];
            map.data[3] = payload[1];
          }

          /* strip NALU size */
          payload += 2;
          payload_len -= 2;

          memcpy (map.data + sizeof (sync_bytes), payload, nalu_size);
          gst_buffer_unmap (outbuf, &map);

          gst_rtp_copy_video_meta (rtph264depay, outbuf, rtp->buffer);

          if (payload_len - nalu_size <= 2)
            last = TRUE;

          gst_rtp_h264_depay_handle_nal (rtph264depay, outbuf, timestamp,
              marker && last);

          payload += nalu_size;
          payload_len -= nalu_size;
        }
        break;
      }
      case 26:
        /* MTAP16    Multi-time aggregation packet      5.7.2 */
        // header_len = 5;
        /* fallthrough, not implemented */
      case 27:
        /* MTAP24    Multi-time aggregation packet      5.7.2 */
        // header_len = 6;
        goto not_implemented;
        break;
      case 28:
      case 29:
      {
        /* FU-A      Fragmentation unit                 5.8 */
        /* FU-B      Fragmentation unit                 5.8 */
        gboolean S, E;

        /* +---------------+
         * |0|1|2|3|4|5|6|7|
         * +-+-+-+-+-+-+-+-+
         * |S|E|R|  Type   |
         * +---------------+
         *
         * R is reserved and always 0
         */
        S = (payload[1] & 0x80) == 0x80;
        E = (payload[1] & 0x40) == 0x40;

        GST_DEBUG_OBJECT (rtph264depay, "S %d, E %d", S, E);

        if (rtph264depay->wait_start && !S)
          goto waiting_start;

        if (S) {
          /* NAL unit starts here */
          guint8 nal_header;

          /* If a new FU unit started, while still processing an older one.
           * Assume that the remote payloader is buggy (doesn't set the end
           * bit) and send out what we've gathered thusfar */
          if (G_UNLIKELY (rtph264depay->current_fu_type != 0))
            gst_rtp_h264_finish_fragmentation_unit (rtph264depay);

          rtph264depay->current_fu_type = nal_unit_type;
          rtph264depay->fu_timestamp = timestamp;

          rtph264depay->wait_start = FALSE;

          /* reconstruct NAL header */
          nal_header = (payload[0] & 0xe0) | (payload[1] & 0x1f);

          /* strip type header, keep FU header, we'll reuse it to reconstruct
           * the NAL header. */
          payload += 1;
          payload_len -= 1;

          nalu_size = payload_len;
          outsize = nalu_size + sizeof (sync_bytes);
          outbuf = gst_buffer_new_and_alloc (outsize);

          gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
          memcpy (map.data + sizeof (sync_bytes), payload, nalu_size);
          map.data[sizeof (sync_bytes)] = nal_header;
          gst_buffer_unmap (outbuf, &map);

          gst_rtp_copy_video_meta (rtph264depay, outbuf, rtp->buffer);

          GST_DEBUG_OBJECT (rtph264depay, "queueing %d bytes", outsize);

          /* and assemble in the adapter */
          gst_adapter_push (rtph264depay->adapter, outbuf);
        } else {
          /* strip off FU indicator and FU header bytes */
          payload += 2;
          payload_len -= 2;

          outsize = payload_len;
          outbuf = gst_buffer_new_and_alloc (outsize);
          gst_buffer_fill (outbuf, 0, payload, outsize);

          gst_rtp_copy_video_meta (rtph264depay, outbuf, rtp->buffer);

          GST_DEBUG_OBJECT (rtph264depay, "queueing %d bytes", outsize);

          /* and assemble in the adapter */
          gst_adapter_push (rtph264depay->adapter, outbuf);
        }

        outbuf = NULL;
        rtph264depay->fu_marker = marker;

        /* if NAL unit ends, flush the adapter */
        if (E)
          gst_rtp_h264_finish_fragmentation_unit (rtph264depay);
        break;
      }
      default:
      {
        rtph264depay->wait_start = FALSE;

        /* 1-23   NAL unit  Single NAL unit packet per H.264   5.6 */
        /* the entire payload is the output buffer */
        nalu_size = payload_len;
        outsize = nalu_size + sizeof (sync_bytes);
        outbuf = gst_buffer_new_and_alloc (outsize);

        gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
        if (rtph264depay->byte_stream) {
          memcpy (map.data, sync_bytes, sizeof (sync_bytes));
        } else {
          map.data[0] = map.data[1] = 0;
          map.data[2] = nalu_size >> 8;
          map.data[3] = nalu_size & 0xff;
        }
        memcpy (map.data + sizeof (sync_bytes), payload, nalu_size);
        gst_buffer_unmap (outbuf, &map);

        gst_rtp_copy_video_meta (rtph264depay, outbuf, rtp->buffer);

        gst_rtp_h264_depay_handle_nal (rtph264depay, outbuf, timestamp, marker);
        break;
      }
    }
  }

  return NULL;

  /* ERRORS */
empty_packet:
  {
    GST_DEBUG_OBJECT (rtph264depay, "empty packet");
    return NULL;
  }
undefined_type:
  {
    GST_ELEMENT_WARNING (rtph264depay, STREAM, DECODE,
        (NULL), ("Undefined packet type"));
    return NULL;
  }
waiting_start:
  {
    GST_DEBUG_OBJECT (rtph264depay, "waiting for start");
    return NULL;
  }
not_implemented:
  {
    GST_ELEMENT_ERROR (rtph264depay, STREAM, FORMAT,
        (NULL), ("NAL unit type %d not supported yet", nal_unit_type));
    return NULL;
  }
}

static gboolean
gst_rtp_h264_depay_handle_event (GstRTPBaseDepayload * depay, GstEvent * event)
{
  GstRtpH264Depay *rtph264depay;

  rtph264depay = GST_RTP_H264_DEPAY (depay);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_h264_depay_reset (rtph264depay, FALSE);
      break;
    case GST_EVENT_EOS:
      gst_rtp_h264_depay_drain (rtph264depay);
      break;
    default:
      break;
  }

  return
      GST_RTP_BASE_DEPAYLOAD_CLASS (parent_class)->handle_event (depay, event);
}

static GstStateChangeReturn
gst_rtp_h264_depay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpH264Depay *rtph264depay;
  GstStateChangeReturn ret;

  rtph264depay = GST_RTP_H264_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_h264_depay_reset (rtph264depay, TRUE);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_h264_depay_reset (rtph264depay, TRUE);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_h264_depay_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (rtph264depay_debug, "rtph264depay", 0,
      "H264 Video RTP Depayloader");

  return gst_element_register (plugin, "rtph264depay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_H264_DEPAY);
}
