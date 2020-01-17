/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2009> Tim-Philipp Müller <tim centricular net>
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
 * SECTION:element-jpegdec
 * @title: jpegdec
 *
 * Decodes jpeg images.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v filesrc location=mjpeg.avi ! avidemux !  queue ! jpegdec ! videoconvert ! videoscale ! autovideosink
 * ]| The above pipeline decode the mjpeg stream and renders it to the screen.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "gstjpegdec.h"
#include "gstjpeg.h"
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>
#include "gst/gst-i18n-plugin.h"
#include <jerror.h>

#define MIN_WIDTH  1
#define MAX_WIDTH  65535
#define MIN_HEIGHT 1
#define MAX_HEIGHT 65535

#define CINFO_GET_JPEGDEC(cinfo_ptr) \
        (((struct GstJpegDecSourceMgr*)((cinfo_ptr)->src))->dec)

#define JPEG_DEFAULT_IDCT_METHOD	JDCT_FASTEST
#define JPEG_DEFAULT_MAX_ERRORS 	0

enum
{
  PROP_0,
  PROP_IDCT_METHOD,
  PROP_MAX_ERRORS
};

/* *INDENT-OFF* */
static GstStaticPadTemplate gst_jpeg_dec_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE
        ("{ I420, RGB, BGR, RGBx, xRGB, BGRx, xBGR, GRAY8 }"))
    );
/* *INDENT-ON* */

/* FIXME: sof-marker is for IJG libjpeg 8, should be different for 6.2 */
/* FIXME: add back "sof-marker = (int) { 0, 1, 2, 5, 6, 7, 9, 10, 13, 14 }"
 * once we have a parser and/or demuxer set caps properly */
static GstStaticPadTemplate gst_jpeg_dec_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/jpeg")
    );

GST_DEBUG_CATEGORY_STATIC (jpeg_dec_debug);
#define GST_CAT_DEFAULT jpeg_dec_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_PERFORMANCE);

static void gst_jpeg_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_jpeg_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_jpeg_dec_set_format (GstVideoDecoder * dec,
    GstVideoCodecState * state);
static gboolean gst_jpeg_dec_start (GstVideoDecoder * bdec);
static gboolean gst_jpeg_dec_stop (GstVideoDecoder * bdec);
static gboolean gst_jpeg_dec_flush (GstVideoDecoder * bdec);
static GstFlowReturn gst_jpeg_dec_parse (GstVideoDecoder * bdec,
    GstVideoCodecFrame * frame, GstAdapter * adapter, gboolean at_eos);
static GstFlowReturn gst_jpeg_dec_handle_frame (GstVideoDecoder * bdec,
    GstVideoCodecFrame * frame);
static gboolean gst_jpeg_dec_decide_allocation (GstVideoDecoder * bdec,
    GstQuery * query);
static gboolean gst_jpeg_dec_sink_event (GstVideoDecoder * bdec,
    GstEvent * event);

#define gst_jpeg_dec_parent_class parent_class
G_DEFINE_TYPE (GstJpegDec, gst_jpeg_dec, GST_TYPE_VIDEO_DECODER);

static void
gst_jpeg_dec_finalize (GObject * object)
{
  GstJpegDec *dec = GST_JPEG_DEC (object);

  jpeg_destroy_decompress (&dec->cinfo);
  if (dec->input_state)
    gst_video_codec_state_unref (dec->input_state);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_jpeg_dec_class_init (GstJpegDecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstVideoDecoderClass *vdec_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;
  vdec_class = (GstVideoDecoderClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gst_jpeg_dec_finalize;
  gobject_class->set_property = gst_jpeg_dec_set_property;
  gobject_class->get_property = gst_jpeg_dec_get_property;

  g_object_class_install_property (gobject_class, PROP_IDCT_METHOD,
      g_param_spec_enum ("idct-method", "IDCT Method",
          "The IDCT algorithm to use", GST_TYPE_IDCT_METHOD,
          JPEG_DEFAULT_IDCT_METHOD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstJpegDec:max-errors:
   *
   * Error out after receiving N consecutive decoding errors
   * (-1 = never error out, 0 = automatic, 1 = fail on first error, etc.)
   *
   * Deprecated: 1.3.1: Property wasn't used internally
   */
#ifndef GST_REMOVE_DEPRECATED
  g_object_class_install_property (gobject_class, PROP_MAX_ERRORS,
      g_param_spec_int ("max-errors", "Maximum Consecutive Decoding Errors",
          "(Deprecated) Error out after receiving N consecutive decoding errors"
          " (-1 = never fail, 0 = automatic, 1 = fail on first error)",
          -1, G_MAXINT, JPEG_DEFAULT_MAX_ERRORS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));
#endif

  gst_element_class_add_static_pad_template (element_class,
      &gst_jpeg_dec_src_pad_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_jpeg_dec_sink_pad_template);
  gst_element_class_set_static_metadata (element_class, "JPEG image decoder",
      "Codec/Decoder/Image", "Decode images from JPEG format",
      "Wim Taymans <wim@fluendo.com>");

  vdec_class->start = gst_jpeg_dec_start;
  vdec_class->stop = gst_jpeg_dec_stop;
  vdec_class->flush = gst_jpeg_dec_flush;
  vdec_class->parse = gst_jpeg_dec_parse;
  vdec_class->set_format = gst_jpeg_dec_set_format;
  vdec_class->handle_frame = gst_jpeg_dec_handle_frame;
  vdec_class->decide_allocation = gst_jpeg_dec_decide_allocation;
  vdec_class->sink_event = gst_jpeg_dec_sink_event;

  GST_DEBUG_CATEGORY_INIT (jpeg_dec_debug, "jpegdec", 0, "JPEG decoder");
  GST_DEBUG_CATEGORY_GET (GST_CAT_PERFORMANCE, "GST_PERFORMANCE");
}

static boolean
gst_jpeg_dec_fill_input_buffer (j_decompress_ptr cinfo)
{
  /* We pass in full frame initially, if this get called, the frame is most likely
   * corrupted */
  return FALSE;
}

static void
gst_jpeg_dec_init_source (j_decompress_ptr cinfo)
{
  GST_LOG_OBJECT (CINFO_GET_JPEGDEC (cinfo), "init_source");
}


static void
gst_jpeg_dec_skip_input_data (j_decompress_ptr cinfo, glong num_bytes)
{
  GstJpegDec *dec = CINFO_GET_JPEGDEC (cinfo);

  GST_DEBUG_OBJECT (dec, "skip %ld bytes", num_bytes);

  if (num_bytes > 0 && cinfo->src->bytes_in_buffer >= num_bytes) {
    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
  }
}

static boolean
gst_jpeg_dec_resync_to_restart (j_decompress_ptr cinfo, gint desired)
{
  GST_LOG_OBJECT (CINFO_GET_JPEGDEC (cinfo), "resync_to_start");
  return TRUE;
}

static void
gst_jpeg_dec_term_source (j_decompress_ptr cinfo)
{
  GST_LOG_OBJECT (CINFO_GET_JPEGDEC (cinfo), "term_source");
  return;
}

METHODDEF (void)
    gst_jpeg_dec_my_output_message (j_common_ptr cinfo)
{
  return;                       /* do nothing */
}

METHODDEF (void)
    gst_jpeg_dec_my_emit_message (j_common_ptr cinfo, int msg_level)
{
  /* GST_LOG_OBJECT (CINFO_GET_JPEGDEC (&cinfo), "msg_level=%d", msg_level); */
  return;
}

METHODDEF (void)
    gst_jpeg_dec_my_error_exit (j_common_ptr cinfo)
{
  struct GstJpegDecErrorMgr *err_mgr = (struct GstJpegDecErrorMgr *) cinfo->err;

  (*cinfo->err->output_message) (cinfo);
  longjmp (err_mgr->setjmp_buffer, 1);
}

static void
gst_jpeg_dec_init (GstJpegDec * dec)
{
  GST_DEBUG ("initializing");

  /* setup jpeglib */
  memset (&dec->cinfo, 0, sizeof (dec->cinfo));
  memset (&dec->jerr, 0, sizeof (dec->jerr));
  dec->cinfo.err = jpeg_std_error (&dec->jerr.pub);
  dec->jerr.pub.output_message = gst_jpeg_dec_my_output_message;
  dec->jerr.pub.emit_message = gst_jpeg_dec_my_emit_message;
  dec->jerr.pub.error_exit = gst_jpeg_dec_my_error_exit;

  jpeg_create_decompress (&dec->cinfo);

  dec->cinfo.src = (struct jpeg_source_mgr *) &dec->jsrc;
  dec->cinfo.src->init_source = gst_jpeg_dec_init_source;
  dec->cinfo.src->fill_input_buffer = gst_jpeg_dec_fill_input_buffer;
  dec->cinfo.src->skip_input_data = gst_jpeg_dec_skip_input_data;
  dec->cinfo.src->resync_to_restart = gst_jpeg_dec_resync_to_restart;
  dec->cinfo.src->term_source = gst_jpeg_dec_term_source;
  dec->jsrc.dec = dec;

  /* init properties */
  dec->idct_method = JPEG_DEFAULT_IDCT_METHOD;
  dec->max_errors = JPEG_DEFAULT_MAX_ERRORS;

  gst_video_decoder_set_use_default_pad_acceptcaps (GST_VIDEO_DECODER_CAST
      (dec), TRUE);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_DECODER_SINK_PAD (dec));
}

static inline gboolean
gst_jpeg_dec_parse_tag_has_entropy_segment (guint8 tag)
{
  if (tag == 0xda || (tag >= 0xd0 && tag <= 0xd7))
    return TRUE;
  return FALSE;
}

static GstFlowReturn
gst_jpeg_dec_parse (GstVideoDecoder * bdec, GstVideoCodecFrame * frame,
    GstAdapter * adapter, gboolean at_eos)
{
  guint size;
  gint toadd = 0;
  gboolean resync;
  gint offset = 0, noffset;
  GstJpegDec *dec = (GstJpegDec *) bdec;

  GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (frame);

  /* FIXME : The overhead of using scan_uint32 is massive */

  size = gst_adapter_available (adapter);
  GST_DEBUG ("Parsing jpeg image data (%u bytes)", size);

  if (at_eos) {
    GST_DEBUG ("Flushing all data out");
    toadd = size;

    /* If we have leftover data, throw it away */
    if (!dec->saw_header)
      goto drop_frame;
    goto have_full_frame;
  }

  if (size < 8)
    goto need_more_data;

  if (!dec->saw_header) {
    gint ret;
    /* we expect at least 4 bytes, first of which start marker */
    ret =
        gst_adapter_masked_scan_uint32 (adapter, 0xffff0000, 0xffd80000, 0,
        size - 4);

    GST_DEBUG ("ret:%d", ret);
    if (ret < 0)
      goto need_more_data;

    if (ret) {
      gst_adapter_flush (adapter, ret);
      size -= ret;
    }
    dec->saw_header = TRUE;
  }

  while (1) {
    guint frame_len;
    guint32 value;

    GST_DEBUG ("offset:%d, size:%d", offset, size);

    noffset =
        gst_adapter_masked_scan_uint32_peek (adapter, 0x0000ff00, 0x0000ff00,
        offset, size - offset, &value);

    /* lost sync if 0xff marker not where expected */
    if ((resync = (noffset != offset))) {
      GST_DEBUG ("Lost sync at 0x%08x, resyncing", offset + 2);
    }
    /* may have marker, but could have been resyncng */
    resync = resync || dec->parse_resync;
    /* Skip over extra 0xff */
    while ((noffset >= 0) && ((value & 0xff) == 0xff)) {
      noffset++;
      noffset =
          gst_adapter_masked_scan_uint32_peek (adapter, 0x0000ff00, 0x0000ff00,
          noffset, size - noffset, &value);
    }
    /* enough bytes left for marker? (we need 0xNN after the 0xff) */
    if (noffset < 0) {
      GST_DEBUG ("at end of input and no EOI marker found, need more data");
      goto need_more_data;
    }

    /* now lock on the marker we found */
    offset = noffset;
    value = value & 0xff;
    if (value == 0xd9) {
      GST_DEBUG ("0x%08x: EOI marker", offset + 2);
      /* clear parse state */
      dec->saw_header = FALSE;
      dec->parse_resync = FALSE;
      toadd = offset + 4;
      goto have_full_frame;
    }
    if (value == 0xd8) {
      GST_DEBUG ("0x%08x: SOI marker before EOI marker", offset + 2);

      /* clear parse state */
      dec->saw_header = FALSE;
      dec->parse_resync = FALSE;
      toadd = offset;
      goto have_full_frame;
    }


    if (value >= 0xd0 && value <= 0xd7)
      frame_len = 0;
    else {
      /* peek tag and subsequent length */
      if (offset + 2 + 4 > size)
        goto need_more_data;
      else
        gst_adapter_masked_scan_uint32_peek (adapter, 0x0, 0x0, offset + 2, 4,
            &frame_len);
      frame_len = frame_len & 0xffff;
    }
    GST_DEBUG ("0x%08x: tag %02x, frame_len=%u", offset + 2, value, frame_len);
    /* the frame length includes the 2 bytes for the length; here we want at
     * least 2 more bytes at the end for an end marker */
    if (offset + 2 + 2 + frame_len + 2 > size) {
      goto need_more_data;
    }

    if (gst_jpeg_dec_parse_tag_has_entropy_segment (value)) {
      guint eseglen = dec->parse_entropy_len;

      GST_DEBUG ("0x%08x: finding entropy segment length (eseglen:%d)",
          offset + 2, eseglen);
      if (size < offset + 2 + frame_len + eseglen)
        goto need_more_data;
      noffset = offset + 2 + frame_len + dec->parse_entropy_len;
      while (1) {
        GST_DEBUG ("noffset:%d, size:%d, size - noffset:%d",
            noffset, size, size - noffset);
        noffset = gst_adapter_masked_scan_uint32_peek (adapter, 0x0000ff00,
            0x0000ff00, noffset, size - noffset, &value);
        if (noffset < 0) {
          /* need more data */
          dec->parse_entropy_len = size - offset - 4 - frame_len - 2;
          goto need_more_data;
        }
        if ((value & 0xff) != 0x00) {
          eseglen = noffset - offset - frame_len - 2;
          break;
        }
        noffset++;
      }
      dec->parse_entropy_len = 0;
      frame_len += eseglen;
      GST_DEBUG ("entropy segment length=%u => frame_len=%u", eseglen,
          frame_len);
    }
    if (resync) {
      /* check if we will still be in sync if we interpret
       * this as a sync point and skip this frame */
      noffset = offset + frame_len + 2;
      noffset = gst_adapter_masked_scan_uint32 (adapter, 0x0000ff00, 0x0000ff00,
          noffset, 4);
      if (noffset < 0) {
        /* ignore and continue resyncing until we hit the end
         * of our data or find a sync point that looks okay */
        offset++;
        continue;
      }
      GST_DEBUG ("found sync at 0x%x", offset + 2);
    }

    /* Add current data to output buffer */
    toadd += frame_len + 2;
    offset += frame_len + 2;
  }

need_more_data:
  if (toadd)
    gst_video_decoder_add_to_frame (bdec, toadd);
  return GST_VIDEO_DECODER_FLOW_NEED_DATA;

have_full_frame:
  if (toadd)
    gst_video_decoder_add_to_frame (bdec, toadd);
  GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (frame);
  return gst_video_decoder_have_frame (bdec);

drop_frame:
  gst_adapter_flush (adapter, size);
  return GST_FLOW_OK;
}


/* shamelessly ripped from jpegutils.c in mjpegtools */
static void
add_huff_table (j_decompress_ptr dinfo,
    JHUFF_TBL ** htblptr, const UINT8 * bits, const UINT8 * val)
/* Define a Huffman table */
{
  int nsymbols, len;

  if (*htblptr == NULL)
    *htblptr = jpeg_alloc_huff_table ((j_common_ptr) dinfo);

  g_assert (*htblptr);

  /* Copy the number-of-symbols-of-each-code-length counts */
  memcpy ((*htblptr)->bits, bits, sizeof ((*htblptr)->bits));

  /* Validate the counts.  We do this here mainly so we can copy the right
   * number of symbols from the val[] array, without risking marching off
   * the end of memory.  jchuff.c will do a more thorough test later.
   */
  nsymbols = 0;
  for (len = 1; len <= 16; len++)
    nsymbols += bits[len];
  if (nsymbols < 1 || nsymbols > 256)
    g_error ("jpegutils.c:  add_huff_table failed badly. ");

  memcpy ((*htblptr)->huffval, val, nsymbols * sizeof (UINT8));
}



static void
std_huff_tables (j_decompress_ptr dinfo)
/* Set up the standard Huffman tables (cf. JPEG standard section K.3) */
/* IMPORTANT: these are only valid for 8-bit data precision! */
{
  static const UINT8 bits_dc_luminance[17] =
      { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_luminance[] =
      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  static const UINT8 bits_dc_chrominance[17] =
      { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_chrominance[] =
      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  static const UINT8 bits_ac_luminance[17] =
      { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const UINT8 val_ac_luminance[] =
      { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
    0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
    0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
  };

  static const UINT8 bits_ac_chrominance[17] =
      { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const UINT8 val_ac_chrominance[] =
      { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
    0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
    0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
  };

  add_huff_table (dinfo, &dinfo->dc_huff_tbl_ptrs[0],
      bits_dc_luminance, val_dc_luminance);
  add_huff_table (dinfo, &dinfo->ac_huff_tbl_ptrs[0],
      bits_ac_luminance, val_ac_luminance);
  add_huff_table (dinfo, &dinfo->dc_huff_tbl_ptrs[1],
      bits_dc_chrominance, val_dc_chrominance);
  add_huff_table (dinfo, &dinfo->ac_huff_tbl_ptrs[1],
      bits_ac_chrominance, val_ac_chrominance);
}



static void
guarantee_huff_tables (j_decompress_ptr dinfo)
{
  if ((dinfo->dc_huff_tbl_ptrs[0] == NULL) &&
      (dinfo->dc_huff_tbl_ptrs[1] == NULL) &&
      (dinfo->ac_huff_tbl_ptrs[0] == NULL) &&
      (dinfo->ac_huff_tbl_ptrs[1] == NULL)) {
    GST_DEBUG ("Generating standard Huffman tables for this frame.");
    std_huff_tables (dinfo);
  }
}

static gboolean
gst_jpeg_dec_set_format (GstVideoDecoder * dec, GstVideoCodecState * state)
{
  GstJpegDec *jpeg = GST_JPEG_DEC (dec);

  if (jpeg->input_state)
    gst_video_codec_state_unref (jpeg->input_state);
  jpeg->input_state = gst_video_codec_state_ref (state);

  return TRUE;
}


/* yuk */
static void
hresamplecpy1 (guint8 * dest, const guint8 * src, guint len)
{
  gint i;

  for (i = 0; i < len; ++i) {
    /* equivalent to: dest[i] = src[i << 1] */
    *dest = *src;
    ++dest;
    ++src;
    ++src;
  }
}

static void
gst_jpeg_dec_free_buffers (GstJpegDec * dec)
{
  gint i;

  for (i = 0; i < 16; i++) {
    g_free (dec->idr_y[i]);
    g_free (dec->idr_u[i]);
    g_free (dec->idr_v[i]);
    dec->idr_y[i] = NULL;
    dec->idr_u[i] = NULL;
    dec->idr_v[i] = NULL;
  }

  dec->idr_width_allocated = 0;
}

static inline gboolean
gst_jpeg_dec_ensure_buffers (GstJpegDec * dec, guint maxrowbytes)
{
  gint i;

  if (G_LIKELY (dec->idr_width_allocated == maxrowbytes))
    return TRUE;

  /* FIXME: maybe just alloc one or three blocks altogether? */
  for (i = 0; i < 16; i++) {
    dec->idr_y[i] = g_try_realloc (dec->idr_y[i], maxrowbytes);
    dec->idr_u[i] = g_try_realloc (dec->idr_u[i], maxrowbytes);
    dec->idr_v[i] = g_try_realloc (dec->idr_v[i], maxrowbytes);

    if (G_UNLIKELY (!dec->idr_y[i] || !dec->idr_u[i] || !dec->idr_v[i])) {
      GST_WARNING_OBJECT (dec, "out of memory, i=%d, bytes=%u", i, maxrowbytes);
      return FALSE;
    }
  }

  dec->idr_width_allocated = maxrowbytes;
  GST_LOG_OBJECT (dec, "allocated temp memory, %u bytes/row", maxrowbytes);
  return TRUE;
}

static void
gst_jpeg_dec_decode_grayscale (GstJpegDec * dec, GstVideoFrame * frame,
    guint field, guint num_fields)
{
  guchar *rows[16];
  guchar **scanarray[1] = { rows };
  gint i, j, k;
  gint lines;
  guint8 *base[1];
  gint width, height;
  gint pstride, rstride;

  GST_DEBUG_OBJECT (dec, "indirect decoding of grayscale");

  width = GST_VIDEO_FRAME_WIDTH (frame);
  height = GST_VIDEO_FRAME_HEIGHT (frame) / num_fields;

  if (G_UNLIKELY (!gst_jpeg_dec_ensure_buffers (dec, GST_ROUND_UP_32 (width))))
    return;

  base[0] = GST_VIDEO_FRAME_COMP_DATA (frame, 0);
  if (field == 2) {
    base[0] += GST_VIDEO_FRAME_COMP_STRIDE (frame, 0);
  }

  pstride = GST_VIDEO_FRAME_COMP_PSTRIDE (frame, 0);
  rstride = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0) * num_fields;

  memcpy (rows, dec->idr_y, 16 * sizeof (gpointer));

  i = 0;
  while (i < height) {
    lines = jpeg_read_raw_data (&dec->cinfo, scanarray, DCTSIZE);
    if (G_LIKELY (lines > 0)) {
      for (j = 0; (j < DCTSIZE) && (i < height); j++, i++) {
        gint p;

        p = 0;
        for (k = 0; k < width; k++) {
          base[0][p] = rows[j][k];
          p += pstride;
        }
        base[0] += rstride;
      }
    } else {
      GST_INFO_OBJECT (dec, "jpeg_read_raw_data() returned 0");
    }
  }
}

static void
gst_jpeg_dec_decode_rgb (GstJpegDec * dec, GstVideoFrame * frame,
    guint field, guint num_fields)
{
  guchar *r_rows[16], *g_rows[16], *b_rows[16];
  guchar **scanarray[3] = { r_rows, g_rows, b_rows };
  gint i, j, k;
  gint lines;
  guint8 *base[3];
  guint pstride, rstride;
  gint width, height;

  GST_DEBUG_OBJECT (dec, "indirect decoding of RGB");

  width = GST_VIDEO_FRAME_WIDTH (frame);
  height = GST_VIDEO_FRAME_HEIGHT (frame) / num_fields;

  if (G_UNLIKELY (!gst_jpeg_dec_ensure_buffers (dec, GST_ROUND_UP_32 (width))))
    return;

  for (i = 0; i < 3; i++) {
    base[i] = GST_VIDEO_FRAME_COMP_DATA (frame, i);
    if (field == 2)
      base[i] += GST_VIDEO_FRAME_COMP_STRIDE (frame, i);
  }

  pstride = GST_VIDEO_FRAME_COMP_PSTRIDE (frame, 0);
  rstride = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0) * num_fields;

  memcpy (r_rows, dec->idr_y, 16 * sizeof (gpointer));
  memcpy (g_rows, dec->idr_u, 16 * sizeof (gpointer));
  memcpy (b_rows, dec->idr_v, 16 * sizeof (gpointer));

  i = 0;
  while (i < height) {
    lines = jpeg_read_raw_data (&dec->cinfo, scanarray, DCTSIZE);
    if (G_LIKELY (lines > 0)) {
      for (j = 0; (j < DCTSIZE) && (i < height); j++, i++) {
        gint p;

        p = 0;
        for (k = 0; k < width; k++) {
          base[0][p] = r_rows[j][k];
          base[1][p] = g_rows[j][k];
          base[2][p] = b_rows[j][k];
          p += pstride;
        }
        base[0] += rstride;
        base[1] += rstride;
        base[2] += rstride;
      }
    } else {
      GST_INFO_OBJECT (dec, "jpeg_read_raw_data() returned 0");
    }
  }
}

static void
gst_jpeg_dec_decode_indirect (GstJpegDec * dec, GstVideoFrame * frame, gint r_v,
    gint r_h, gint comp, guint field, guint num_fields)
{
  guchar *y_rows[16], *u_rows[16], *v_rows[16];
  guchar **scanarray[3] = { y_rows, u_rows, v_rows };
  gint i, j, k;
  gint lines;
  guchar *base[3], *last[3];
  gint rowsize[3], stride[3];
  gint width, height;

  GST_DEBUG_OBJECT (dec,
      "unadvantageous width or r_h, taking slow route involving memcpy");

  width = GST_VIDEO_FRAME_WIDTH (frame);
  height = GST_VIDEO_FRAME_HEIGHT (frame);

  if (G_UNLIKELY (!gst_jpeg_dec_ensure_buffers (dec, GST_ROUND_UP_32 (width))))
    return;

  for (i = 0; i < 3; i++) {
    base[i] = GST_VIDEO_FRAME_COMP_DATA (frame, i);
    stride[i] = GST_VIDEO_FRAME_COMP_STRIDE (frame, i) * num_fields;
    rowsize[i] = GST_VIDEO_FRAME_COMP_STRIDE (frame, i);
    /* make sure we don't make jpeglib write beyond our buffer,
     * which might happen if (height % (r_v*DCTSIZE)) != 0 */
    last[i] = base[i] + (GST_VIDEO_FRAME_COMP_STRIDE (frame, i) *
        (GST_VIDEO_FRAME_COMP_HEIGHT (frame, i) - 1));

    if (field == 2) {
      base[i] += GST_VIDEO_FRAME_COMP_STRIDE (frame, i);
    }
  }

  memcpy (y_rows, dec->idr_y, 16 * sizeof (gpointer));
  memcpy (u_rows, dec->idr_u, 16 * sizeof (gpointer));
  memcpy (v_rows, dec->idr_v, 16 * sizeof (gpointer));

  /* fill chroma components for grayscale */
  if (comp == 1) {
    GST_DEBUG_OBJECT (dec, "grayscale, filling chroma");
    for (i = 0; i < 16; i++) {
      memset (u_rows[i], GST_ROUND_UP_32 (width), 0x80);
      memset (v_rows[i], GST_ROUND_UP_32 (width), 0x80);
    }
  }

  for (i = 0; i < height; i += r_v * DCTSIZE) {
    lines = jpeg_read_raw_data (&dec->cinfo, scanarray, r_v * DCTSIZE);
    if (G_LIKELY (lines > 0)) {
      for (j = 0, k = 0; j < (r_v * DCTSIZE); j += r_v, k++) {
        if (G_LIKELY (base[0] <= last[0])) {
          memcpy (base[0], y_rows[j], rowsize[0]);
          base[0] += stride[0];
        }
        if (r_v == 2) {
          if (G_LIKELY (base[0] <= last[0])) {
            memcpy (base[0], y_rows[j + 1], rowsize[0]);
            base[0] += stride[0];
          }
        }
        if (G_LIKELY (base[1] <= last[1] && base[2] <= last[2])) {
          if (r_h == 2) {
            memcpy (base[1], u_rows[k], rowsize[1]);
            memcpy (base[2], v_rows[k], rowsize[2]);
          } else if (r_h == 1) {
            hresamplecpy1 (base[1], u_rows[k], rowsize[1]);
            hresamplecpy1 (base[2], v_rows[k], rowsize[2]);
          } else {
            /* FIXME: implement (at least we avoid crashing by doing nothing) */
          }
        }

        if (r_v == 2 || (k & 1) != 0) {
          base[1] += stride[1];
          base[2] += stride[2];
        }
      }
    } else {
      GST_INFO_OBJECT (dec, "jpeg_read_raw_data() returned 0");
    }
  }
}

static GstFlowReturn
gst_jpeg_dec_decode_direct (GstJpegDec * dec, GstVideoFrame * frame,
    guint field, guint num_fields)
{
  guchar **line[3];             /* the jpeg line buffer         */
  guchar *y[4 * DCTSIZE] = { NULL, };   /* alloc enough for the lines   */
  guchar *u[4 * DCTSIZE] = { NULL, };   /* r_v will be <4               */
  guchar *v[4 * DCTSIZE] = { NULL, };
  gint i, j;
  gint lines, v_samp[3];
  guchar *base[3], *last[3];
  gint stride[3];
  guint height;

  line[0] = y;
  line[1] = u;
  line[2] = v;

  v_samp[0] = dec->cinfo.comp_info[0].v_samp_factor;
  v_samp[1] = dec->cinfo.comp_info[1].v_samp_factor;
  v_samp[2] = dec->cinfo.comp_info[2].v_samp_factor;

  if (G_UNLIKELY (v_samp[0] > 2 || v_samp[1] > 2 || v_samp[2] > 2))
    goto format_not_supported;

  height = GST_VIDEO_FRAME_HEIGHT (frame);

  for (i = 0; i < 3; i++) {
    base[i] = GST_VIDEO_FRAME_COMP_DATA (frame, i);
    stride[i] = GST_VIDEO_FRAME_COMP_STRIDE (frame, i) * num_fields;
    /* make sure we don't make jpeglib write beyond our buffer,
     * which might happen if (height % (r_v*DCTSIZE)) != 0 */
    last[i] = base[i] + (GST_VIDEO_FRAME_COMP_STRIDE (frame, i) *
        (GST_VIDEO_FRAME_COMP_HEIGHT (frame, i) - 1));

    if (field == 2) {
      base[i] += GST_VIDEO_FRAME_COMP_STRIDE (frame, i);
    }
  }

  if (height % (v_samp[0] * DCTSIZE) && (dec->scratch_size < stride[0])) {
    g_free (dec->scratch);
    dec->scratch = g_malloc (stride[0]);
    dec->scratch_size = stride[0];
  }

  /* let jpeglib decode directly into our final buffer */
  GST_DEBUG_OBJECT (dec, "decoding directly into output buffer");

  for (i = 0; i < height; i += v_samp[0] * DCTSIZE) {
    for (j = 0; j < (v_samp[0] * DCTSIZE); ++j) {
      /* Y */
      line[0][j] = base[0] + (i + j) * stride[0];
      if (G_UNLIKELY (line[0][j] > last[0]))
        line[0][j] = dec->scratch;
      /* U */
      if (v_samp[1] == v_samp[0]) {
        line[1][j] = base[1] + ((i + j) / 2) * stride[1];
      } else if (j < (v_samp[1] * DCTSIZE)) {
        line[1][j] = base[1] + ((i / 2) + j) * stride[1];
      }
      if (G_UNLIKELY (line[1][j] > last[1]))
        line[1][j] = dec->scratch;
      /* V */
      if (v_samp[2] == v_samp[0]) {
        line[2][j] = base[2] + ((i + j) / 2) * stride[2];
      } else if (j < (v_samp[2] * DCTSIZE)) {
        line[2][j] = base[2] + ((i / 2) + j) * stride[2];
      }
      if (G_UNLIKELY (line[2][j] > last[2]))
        line[2][j] = dec->scratch;
    }

    lines = jpeg_read_raw_data (&dec->cinfo, line, v_samp[0] * DCTSIZE);
    if (G_UNLIKELY (!lines)) {
      GST_INFO_OBJECT (dec, "jpeg_read_raw_data() returned 0");
    }
  }
  return GST_FLOW_OK;

format_not_supported:
  {
    gboolean ret = GST_FLOW_OK;

    GST_VIDEO_DECODER_ERROR (dec, 1, STREAM, DECODE,
        (_("Failed to decode JPEG image")),
        ("Unsupported subsampling schema: v_samp factors: %u %u %u", v_samp[0],
            v_samp[1], v_samp[2]), ret);

    return ret;
  }
}

static void
gst_jpeg_dec_negotiate (GstJpegDec * dec, gint width, gint height, gint clrspc,
    gboolean interlaced)
{
  GstVideoCodecState *outstate;
  GstVideoInfo *info;
  GstVideoFormat format;

  switch (clrspc) {
    case JCS_RGB:
      format = GST_VIDEO_FORMAT_RGB;
      break;
    case JCS_GRAYSCALE:
      format = GST_VIDEO_FORMAT_GRAY8;
      break;
    default:
      format = GST_VIDEO_FORMAT_I420;
      break;
  }

  /* Compare to currently configured output state */
  outstate = gst_video_decoder_get_output_state (GST_VIDEO_DECODER (dec));
  if (outstate) {
    info = &outstate->info;

    if (width == GST_VIDEO_INFO_WIDTH (info) &&
        height == GST_VIDEO_INFO_HEIGHT (info) &&
        format == GST_VIDEO_INFO_FORMAT (info)) {
      gst_video_codec_state_unref (outstate);
      return;
    }
    gst_video_codec_state_unref (outstate);
  }

  outstate =
      gst_video_decoder_set_output_state (GST_VIDEO_DECODER (dec), format,
      width, height, dec->input_state);

  switch (clrspc) {
    case JCS_RGB:
    case JCS_GRAYSCALE:
      break;
    default:
      outstate->info.colorimetry.range = GST_VIDEO_COLOR_RANGE_0_255;
      outstate->info.colorimetry.matrix = GST_VIDEO_COLOR_MATRIX_BT601;
      outstate->info.colorimetry.transfer = GST_VIDEO_TRANSFER_UNKNOWN;
      outstate->info.colorimetry.primaries = GST_VIDEO_COLOR_PRIMARIES_UNKNOWN;
      break;
  }

  if (interlaced) {
    outstate->info.interlace_mode = GST_VIDEO_INTERLACE_MODE_INTERLEAVED;
    GST_VIDEO_INFO_FIELD_ORDER (&outstate->info) =
        GST_VIDEO_FIELD_ORDER_TOP_FIELD_FIRST;
  }

  gst_video_codec_state_unref (outstate);

  gst_video_decoder_negotiate (GST_VIDEO_DECODER (dec));

  GST_DEBUG_OBJECT (dec, "max_v_samp_factor=%d", dec->cinfo.max_v_samp_factor);
  GST_DEBUG_OBJECT (dec, "max_h_samp_factor=%d", dec->cinfo.max_h_samp_factor);
}

static GstFlowReturn
gst_jpeg_dec_prepare_decode (GstJpegDec * dec)
{
  G_GNUC_UNUSED GstFlowReturn ret;
  guint r_h, r_v, hdr_ok;

  /* read header */
  hdr_ok = jpeg_read_header (&dec->cinfo, TRUE);
  if (G_UNLIKELY (hdr_ok != JPEG_HEADER_OK)) {
    GST_WARNING_OBJECT (dec, "reading the header failed, %d", hdr_ok);
  }

  GST_LOG_OBJECT (dec, "num_components=%d", dec->cinfo.num_components);
  GST_LOG_OBJECT (dec, "jpeg_color_space=%d", dec->cinfo.jpeg_color_space);

  if (!dec->cinfo.num_components || !dec->cinfo.comp_info)
    goto components_not_supported;

  r_h = dec->cinfo.comp_info[0].h_samp_factor;
  r_v = dec->cinfo.comp_info[0].v_samp_factor;

  GST_LOG_OBJECT (dec, "r_h = %d, r_v = %d", r_h, r_v);

  if (dec->cinfo.num_components > 3)
    goto components_not_supported;

  /* verify color space expectation to avoid going *boom* or bogus output */
  if (dec->cinfo.jpeg_color_space != JCS_YCbCr &&
      dec->cinfo.jpeg_color_space != JCS_GRAYSCALE &&
      dec->cinfo.jpeg_color_space != JCS_RGB)
    goto unsupported_colorspace;

#ifndef GST_DISABLE_GST_DEBUG
  {
    gint i;

    for (i = 0; i < dec->cinfo.num_components; ++i) {
      GST_LOG_OBJECT (dec, "[%d] h_samp_factor=%d, v_samp_factor=%d, cid=%d",
          i, dec->cinfo.comp_info[i].h_samp_factor,
          dec->cinfo.comp_info[i].v_samp_factor,
          dec->cinfo.comp_info[i].component_id);
    }
  }
#endif

  /* prepare for raw output */
  dec->cinfo.do_fancy_upsampling = FALSE;
  dec->cinfo.do_block_smoothing = FALSE;
  dec->cinfo.out_color_space = dec->cinfo.jpeg_color_space;
  dec->cinfo.dct_method = dec->idct_method;
  dec->cinfo.raw_data_out = TRUE;

  GST_LOG_OBJECT (dec, "starting decompress");
  guarantee_huff_tables (&dec->cinfo);
  if (!jpeg_start_decompress (&dec->cinfo)) {
    GST_WARNING_OBJECT (dec, "failed to start decompression cycle");
  }

  /* sanity checks to get safe and reasonable output */
  switch (dec->cinfo.jpeg_color_space) {
    case JCS_GRAYSCALE:
      if (dec->cinfo.num_components != 1)
        goto invalid_yuvrgbgrayscale;
      break;
    case JCS_RGB:
      if (dec->cinfo.num_components != 3 || dec->cinfo.max_v_samp_factor > 1 ||
          dec->cinfo.max_h_samp_factor > 1)
        goto invalid_yuvrgbgrayscale;
      break;
    case JCS_YCbCr:
      if (dec->cinfo.num_components != 3 ||
          r_v > 2 || r_v < dec->cinfo.comp_info[0].v_samp_factor ||
          r_v < dec->cinfo.comp_info[1].v_samp_factor ||
          r_h < dec->cinfo.comp_info[0].h_samp_factor ||
          r_h < dec->cinfo.comp_info[1].h_samp_factor)
        goto invalid_yuvrgbgrayscale;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  if (G_UNLIKELY (dec->cinfo.output_width < MIN_WIDTH ||
          dec->cinfo.output_width > MAX_WIDTH ||
          dec->cinfo.output_height < MIN_HEIGHT ||
          dec->cinfo.output_height > MAX_HEIGHT))
    goto wrong_size;

  return GST_FLOW_OK;

/* ERRORS */
wrong_size:
  {
    ret = GST_FLOW_ERROR;
    GST_VIDEO_DECODER_ERROR (dec, 1, STREAM, DECODE,
        (_("Failed to decode JPEG image")),
        ("Picture is too small or too big (%ux%u)", dec->cinfo.output_width,
            dec->cinfo.output_height), ret);
    return GST_FLOW_ERROR;
  }
components_not_supported:
  {
    ret = GST_FLOW_ERROR;
    GST_VIDEO_DECODER_ERROR (dec, 1, STREAM, DECODE,
        (_("Failed to decode JPEG image")),
        ("number of components not supported: %d (max 3)",
            dec->cinfo.num_components), ret);
    jpeg_abort_decompress (&dec->cinfo);
    return GST_FLOW_ERROR;
  }
unsupported_colorspace:
  {
    ret = GST_FLOW_ERROR;
    GST_VIDEO_DECODER_ERROR (dec, 1, STREAM, DECODE,
        (_("Failed to decode JPEG image")),
        ("Picture has unknown or unsupported colourspace"), ret);
    jpeg_abort_decompress (&dec->cinfo);
    return GST_FLOW_ERROR;
  }
invalid_yuvrgbgrayscale:
  {
    ret = GST_FLOW_ERROR;
    GST_VIDEO_DECODER_ERROR (dec, 1, STREAM, DECODE,
        (_("Failed to decode JPEG image")),
        ("Picture is corrupt or unhandled YUV/RGB/grayscale layout"), ret);
    jpeg_abort_decompress (&dec->cinfo);
    return GST_FLOW_ERROR;
  }
}

static GstFlowReturn
gst_jpeg_dec_decode (GstJpegDec * dec, GstVideoFrame * vframe, guint width,
    guint height, guint field, guint num_fields)
{
  GstFlowReturn ret = GST_FLOW_OK;

  if (dec->cinfo.jpeg_color_space == JCS_RGB) {
    gst_jpeg_dec_decode_rgb (dec, vframe, field, num_fields);
  } else if (dec->cinfo.jpeg_color_space == JCS_GRAYSCALE) {
    gst_jpeg_dec_decode_grayscale (dec, vframe, field, num_fields);
  } else {
    GST_LOG_OBJECT (dec, "decompressing (required scanline buffer height = %u)",
        dec->cinfo.rec_outbuf_height);

    /* For some widths jpeglib requires more horizontal padding than I420 
     * provides. In those cases we need to decode into separate buffers and then
     * copy over the data into our final picture buffer, otherwise jpeglib might
     * write over the end of a line into the beginning of the next line,
     * resulting in blocky artifacts on the left side of the picture. */
    if (G_UNLIKELY (width % (dec->cinfo.max_h_samp_factor * DCTSIZE) != 0
            || dec->cinfo.comp_info[0].h_samp_factor != 2
            || dec->cinfo.comp_info[1].h_samp_factor != 1
            || dec->cinfo.comp_info[2].h_samp_factor != 1)) {
      GST_CAT_LOG_OBJECT (GST_CAT_PERFORMANCE, dec,
          "indirect decoding using extra buffer copy");
      gst_jpeg_dec_decode_indirect (dec, vframe,
          dec->cinfo.comp_info[0].v_samp_factor,
          dec->cinfo.comp_info[0].h_samp_factor, dec->cinfo.num_components,
          field, num_fields);
    } else {
      ret = gst_jpeg_dec_decode_direct (dec, vframe, field, num_fields);
    }
  }

  GST_LOG_OBJECT (dec, "decompressing finished: %s", gst_flow_get_name (ret));

  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    jpeg_abort_decompress (&dec->cinfo);
  } else {
    jpeg_finish_decompress (&dec->cinfo);
  }

  return ret;
}

static GstFlowReturn
gst_jpeg_dec_handle_frame (GstVideoDecoder * bdec, GstVideoCodecFrame * frame)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstJpegDec *dec = (GstJpegDec *) bdec;
  GstVideoFrame vframe;
  gint num_fields;              /* number of fields (1 or 2) */
  gint output_height;           /* height of output image (one or two fields) */
  gint height;                  /* height of current frame (whole image or a field) */
  gint width;
  guint code;
  gboolean need_unmap = TRUE;
  GstVideoCodecState *state = NULL;
  gboolean release_frame = TRUE;
  gboolean has_eoi;
  guint8 *data;
  gsize nbytes;

  gst_buffer_map (frame->input_buffer, &dec->current_frame_map, GST_MAP_READ);

  data = dec->current_frame_map.data;
  nbytes = dec->current_frame_map.size;
  has_eoi = ((data[nbytes - 2] == 0xff) && (data[nbytes - 1] == 0xd9));

  /* some cameras fail to send an end-of-image marker (EOI),
   * add it if that is the case. */
  if (!has_eoi) {
    GstMapInfo map;
    GstBuffer *eoibuf = gst_buffer_new_and_alloc (2);

    /* unmap, will add EOI and remap at the end */
    gst_buffer_unmap (frame->input_buffer, &dec->current_frame_map);

    gst_buffer_map (eoibuf, &map, GST_MAP_WRITE);
    map.data[0] = 0xff;
    map.data[1] = 0xd9;
    gst_buffer_unmap (eoibuf, &map);

    /* append to input buffer, and remap */
    frame->input_buffer = gst_buffer_append (frame->input_buffer, eoibuf);

    gst_buffer_map (frame->input_buffer, &dec->current_frame_map, GST_MAP_READ);
    GST_DEBUG ("fixup EOI marker added");
  }

  dec->current_frame = frame;
  dec->cinfo.src->next_input_byte = dec->current_frame_map.data;
  dec->cinfo.src->bytes_in_buffer = dec->current_frame_map.size;

  if (setjmp (dec->jerr.setjmp_buffer)) {
    code = dec->jerr.pub.msg_code;

    if (code == JERR_INPUT_EOF) {
      GST_DEBUG ("jpeg input EOF error, we probably need more data");
      goto need_more_data;
    }
    goto decode_error;
  }

  /* read header and check values */
  ret = gst_jpeg_dec_prepare_decode (dec);
  if (G_UNLIKELY (ret == GST_FLOW_ERROR))
    goto done;

  width = dec->cinfo.output_width;
  height = dec->cinfo.output_height;

  /* is it interlaced MJPEG? (we really don't want to scan the jpeg data
   * to see if there are two SOF markers in the packet to detect this) */
  if (gst_video_decoder_get_packetized (bdec) &&
      dec->input_state &&
      dec->input_state->info.height > height &&
      dec->input_state->info.height <= (height * 2)
      && dec->input_state->info.width == width) {
    GST_LOG_OBJECT (dec,
        "looks like an interlaced image: "
        "input width/height of %dx%d with JPEG frame width/height of %dx%d",
        dec->input_state->info.width, dec->input_state->info.height, width,
        height);
    output_height = dec->input_state->info.height;
    height = dec->input_state->info.height / 2;
    num_fields = 2;
    GST_LOG_OBJECT (dec, "field height=%d", height);
  } else {
    output_height = height;
    num_fields = 1;
  }

  gst_jpeg_dec_negotiate (dec, width, output_height,
      dec->cinfo.jpeg_color_space, num_fields == 2);

  state = gst_video_decoder_get_output_state (bdec);
  ret = gst_video_decoder_allocate_output_frame (bdec, frame);
  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto alloc_failed;

  if (!gst_video_frame_map (&vframe, &state->info, frame->output_buffer,
          GST_MAP_READWRITE))
    goto alloc_failed;

  if (setjmp (dec->jerr.setjmp_buffer)) {
    code = dec->jerr.pub.msg_code;
    gst_video_frame_unmap (&vframe);
    goto decode_error;
  }

  GST_LOG_OBJECT (dec, "width %d, height %d, fields %d", width, output_height,
      num_fields);

  ret = gst_jpeg_dec_decode (dec, &vframe, width, height, 1, num_fields);
  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    gst_video_frame_unmap (&vframe);
    goto decode_failed;
  }

  if (setjmp (dec->jerr.setjmp_buffer)) {
    code = dec->jerr.pub.msg_code;
    gst_video_frame_unmap (&vframe);
    goto decode_error;
  }

  /* decode second field if there is one */
  if (num_fields == 2) {
    GstVideoFormat field2_format;

    /* Checked above before setting num_fields to 2 */
    g_assert (dec->input_state != NULL);

    /* skip any chunk or padding bytes before the next SOI marker; both fields
     * are in one single buffer here, so direct access should be fine here */
    while (dec->jsrc.pub.bytes_in_buffer > 2 &&
        GST_READ_UINT16_BE (dec->jsrc.pub.next_input_byte) != 0xffd8) {
      --dec->jsrc.pub.bytes_in_buffer;
      ++dec->jsrc.pub.next_input_byte;
    }

    if (gst_jpeg_dec_prepare_decode (dec) != GST_FLOW_OK) {
      GST_WARNING_OBJECT (dec, "problem reading jpeg header of 2nd field");
      /* FIXME: post a warning message here? */
      gst_video_frame_unmap (&vframe);
      goto decode_failed;
    }

    /* check if format has changed for the second field */
    switch (dec->cinfo.jpeg_color_space) {
      case JCS_RGB:
        field2_format = GST_VIDEO_FORMAT_RGB;
        break;
      case JCS_GRAYSCALE:
        field2_format = GST_VIDEO_FORMAT_GRAY8;
        break;
      default:
        field2_format = GST_VIDEO_FORMAT_I420;
        break;
    }

    GST_LOG_OBJECT (dec,
        "got for second field of interlaced image: "
        "input width/height of %dx%d with JPEG frame width/height of %dx%d",
        dec->input_state->info.width, dec->input_state->info.height,
        dec->cinfo.output_width, dec->cinfo.output_height);

    if (dec->cinfo.output_width != GST_VIDEO_INFO_WIDTH (&state->info) ||
        GST_VIDEO_INFO_HEIGHT (&state->info) <= dec->cinfo.output_height ||
        GST_VIDEO_INFO_HEIGHT (&state->info) > (dec->cinfo.output_height * 2) ||
        field2_format != GST_VIDEO_INFO_FORMAT (&state->info)) {
      GST_WARNING_OBJECT (dec, "second field has different format than first");
      gst_video_frame_unmap (&vframe);
      goto decode_failed;
    }

    ret = gst_jpeg_dec_decode (dec, &vframe, width, height, 2, 2);
    if (G_UNLIKELY (ret != GST_FLOW_OK)) {
      gst_video_frame_unmap (&vframe);
      goto decode_failed;
    }
  }
  gst_video_frame_unmap (&vframe);

  gst_buffer_unmap (frame->input_buffer, &dec->current_frame_map);
  ret = gst_video_decoder_finish_frame (bdec, frame);
  release_frame = FALSE;
  need_unmap = FALSE;

done:

exit:

  if (need_unmap)
    gst_buffer_unmap (frame->input_buffer, &dec->current_frame_map);

  if (release_frame)
    gst_video_decoder_release_frame (bdec, frame);

  if (state)
    gst_video_codec_state_unref (state);

  return ret;

  /* special cases */
need_more_data:
  {
    GST_LOG_OBJECT (dec, "we need more data");
    ret = GST_FLOW_OK;
    goto exit;
  }
  /* ERRORS */
decode_error:
  {
    gchar err_msg[JMSG_LENGTH_MAX];

    dec->jerr.pub.format_message ((j_common_ptr) (&dec->cinfo), err_msg);

    GST_VIDEO_DECODER_ERROR (dec, 1, STREAM, DECODE,
        (_("Failed to decode JPEG image")), ("Decode error #%u: %s", code,
            err_msg), ret);

    gst_buffer_unmap (frame->input_buffer, &dec->current_frame_map);
    gst_video_decoder_drop_frame (bdec, frame);
    release_frame = FALSE;
    need_unmap = FALSE;
    jpeg_abort_decompress (&dec->cinfo);

    goto done;
  }
decode_failed:
  {
    /* already posted an error message */
    goto done;
  }
alloc_failed:
  {
    const gchar *reason;

    reason = gst_flow_get_name (ret);

    GST_DEBUG_OBJECT (dec, "failed to alloc buffer, reason %s", reason);
    /* Reset for next time */
    jpeg_abort_decompress (&dec->cinfo);
    if (ret != GST_FLOW_EOS && ret != GST_FLOW_FLUSHING &&
        ret != GST_FLOW_NOT_LINKED) {
      GST_VIDEO_DECODER_ERROR (dec, 1, STREAM, DECODE,
          (_("Failed to decode JPEG image")),
          ("Buffer allocation failed, reason: %s", reason), ret);
      jpeg_abort_decompress (&dec->cinfo);
    }
    goto exit;
  }
}

static gboolean
gst_jpeg_dec_decide_allocation (GstVideoDecoder * bdec, GstQuery * query)
{
  GstBufferPool *pool = NULL;
  GstStructure *config;

  if (!GST_VIDEO_DECODER_CLASS (parent_class)->decide_allocation (bdec, query))
    return FALSE;

  if (gst_query_get_n_allocation_pools (query) > 0)
    gst_query_parse_nth_allocation_pool (query, 0, &pool, NULL, NULL, NULL);

  if (pool == NULL)
    return FALSE;

  config = gst_buffer_pool_get_config (pool);
  if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL)) {
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }
  gst_buffer_pool_set_config (pool, config);
  gst_object_unref (pool);

  return TRUE;
}

static gboolean
gst_jpeg_dec_sink_event (GstVideoDecoder * bdec, GstEvent * event)
{
  const GstSegment *segment;

  if (GST_EVENT_TYPE (event) != GST_EVENT_SEGMENT)
    goto done;

  gst_event_parse_segment (event, &segment);

  if (segment->format == GST_FORMAT_TIME)
    gst_video_decoder_set_packetized (bdec, TRUE);
  else
    gst_video_decoder_set_packetized (bdec, FALSE);

done:
  return GST_VIDEO_DECODER_CLASS (parent_class)->sink_event (bdec, event);
}

static gboolean
gst_jpeg_dec_start (GstVideoDecoder * bdec)
{
  GstJpegDec *dec = (GstJpegDec *) bdec;

  dec->saw_header = FALSE;
  dec->parse_entropy_len = 0;
  dec->parse_resync = FALSE;

  gst_video_decoder_set_packetized (bdec, FALSE);

  return TRUE;
}

static gboolean
gst_jpeg_dec_flush (GstVideoDecoder * bdec)
{
  GstJpegDec *dec = (GstJpegDec *) bdec;

  jpeg_abort_decompress (&dec->cinfo);
  dec->parse_entropy_len = 0;
  dec->parse_resync = FALSE;
  dec->saw_header = FALSE;

  return TRUE;
}

static void
gst_jpeg_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstJpegDec *dec;

  dec = GST_JPEG_DEC (object);

  switch (prop_id) {
    case PROP_IDCT_METHOD:
      dec->idct_method = g_value_get_enum (value);
      break;
#ifndef GST_REMOVE_DEPRECATED
    case PROP_MAX_ERRORS:
      g_atomic_int_set (&dec->max_errors, g_value_get_int (value));
      break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_jpeg_dec_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstJpegDec *dec;

  dec = GST_JPEG_DEC (object);

  switch (prop_id) {
    case PROP_IDCT_METHOD:
      g_value_set_enum (value, dec->idct_method);
      break;
#ifndef GST_REMOVE_DEPRECATED
    case PROP_MAX_ERRORS:
      g_value_set_int (value, g_atomic_int_get (&dec->max_errors));
      break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_jpeg_dec_stop (GstVideoDecoder * bdec)
{
  GstJpegDec *dec = (GstJpegDec *) bdec;

  gst_jpeg_dec_free_buffers (dec);

  g_free (dec->scratch);
  dec->scratch = NULL;
  dec->scratch_size = 0;

  return TRUE;
}
