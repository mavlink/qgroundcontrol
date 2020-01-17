/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2006,2011> Tim-Philipp Müller <tim centricular net>
 * Copyright (C) <2006> Jan Schmidt <thaytan at mad scientist com>
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
 * SECTION:element-flacdec
 * @title: flacdec
 * @see_also: #GstFlacEnc
 *
 * flacdec decodes FLAC streams.
 * [FLAC](http://flac.sourceforge.net/) is a Free Lossless Audio Codec.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=media/small/dark.441-16-s.flac ! flacparse ! flacdec ! audioconvert ! audioresample ! autoaudiosink
 * ]|
 * |[
 * gst-launch-1.0 souphttpsrc location=http://gstreamer.freedesktop.org/media/small/dark.441-16-s.flac ! flacparse ! flacdec ! audioconvert ! audioresample ! queue min-threshold-buffers=10 ! autoaudiosink
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gstflacdec.h"
#include <gst/gst-i18n-plugin.h>
#include <gst/tag/tag.h>

/* Taken from http://flac.sourceforge.net/format.html#frame_header */
static const GstAudioChannelPosition channel_positions[8][8] = {
  {GST_AUDIO_CHANNEL_POSITION_MONO},
  {GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
      GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
      GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
      GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
      GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
      GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT},
  /* FIXME: 7/8 channel layouts are not defined in the FLAC specs */
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
      GST_AUDIO_CHANNEL_POSITION_REAR_CENTER}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
        GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
      GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT}
};

GST_DEBUG_CATEGORY_STATIC (flacdec_debug);
#define GST_CAT_DEFAULT flacdec_debug

static FLAC__StreamDecoderReadStatus
gst_flac_dec_read_stream (const FLAC__StreamDecoder * decoder,
    FLAC__byte buffer[], size_t * bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus
gst_flac_dec_write_stream (const FLAC__StreamDecoder * decoder,
    const FLAC__Frame * frame,
    const FLAC__int32 * const buffer[], void *client_data);
static gboolean
gst_flac_dec_handle_decoder_error (GstFlacDec * dec, gboolean msg);
static void gst_flac_dec_metadata_cb (const FLAC__StreamDecoder *
    decoder, const FLAC__StreamMetadata * metadata, void *client_data);
static void gst_flac_dec_error_cb (const FLAC__StreamDecoder *
    decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

static void gst_flac_dec_flush (GstAudioDecoder * audio_dec, gboolean hard);
static gboolean gst_flac_dec_set_format (GstAudioDecoder * dec, GstCaps * caps);
static gboolean gst_flac_dec_start (GstAudioDecoder * dec);
static gboolean gst_flac_dec_stop (GstAudioDecoder * dec);
static GstFlowReturn gst_flac_dec_handle_frame (GstAudioDecoder * audio_dec,
    GstBuffer * buf);

G_DEFINE_TYPE (GstFlacDec, gst_flac_dec, GST_TYPE_AUDIO_DECODER);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define FORMATS "{ S8, S16LE, S24_32LE, S32LE } "
#else
#define FORMATS "{ S8, S16BE, S24_32BE, S32BE } "
#endif

#define GST_FLAC_DEC_SRC_CAPS                             \
    "audio/x-raw, "                                       \
    "format = (string) " FORMATS ", "                     \
    "layout = (string) interleaved, "                     \
    "rate = (int) [ 1, 655350 ], "                        \
    "channels = (int) [ 1, 8 ]"

#define GST_FLAC_DEC_SINK_CAPS                            \
    "audio/x-flac, "                                      \
    "framed = (boolean) true, "                           \
    "rate = (int) [ 1, 655350 ], "                        \
    "channels = (int) [ 1, 8 ]"

static GstStaticPadTemplate flac_dec_src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_FLAC_DEC_SRC_CAPS));
static GstStaticPadTemplate flac_dec_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_FLAC_DEC_SINK_CAPS));

static void
gst_flac_dec_class_init (GstFlacDecClass * klass)
{
  GstAudioDecoderClass *audiodecoder_class;
  GstElementClass *gstelement_class;

  audiodecoder_class = (GstAudioDecoderClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  GST_DEBUG_CATEGORY_INIT (flacdec_debug, "flacdec", 0, "flac decoder");

  audiodecoder_class->stop = GST_DEBUG_FUNCPTR (gst_flac_dec_stop);
  audiodecoder_class->start = GST_DEBUG_FUNCPTR (gst_flac_dec_start);
  audiodecoder_class->flush = GST_DEBUG_FUNCPTR (gst_flac_dec_flush);
  audiodecoder_class->set_format = GST_DEBUG_FUNCPTR (gst_flac_dec_set_format);
  audiodecoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_flac_dec_handle_frame);

  gst_element_class_add_static_pad_template (gstelement_class,
      &flac_dec_src_factory);
  gst_element_class_add_static_pad_template (gstelement_class,
      &flac_dec_sink_factory);

  gst_element_class_set_static_metadata (gstelement_class, "FLAC audio decoder",
      "Codec/Decoder/Audio", "Decodes FLAC lossless audio streams",
      "Tim-Philipp Müller <tim@centricular.net>, "
      "Wim Taymans <wim.taymans@gmail.com>");
}

static void
gst_flac_dec_init (GstFlacDec * flacdec)
{
  flacdec->do_resync = FALSE;
  gst_audio_decoder_set_needs_format (GST_AUDIO_DECODER (flacdec), TRUE);
  gst_audio_decoder_set_use_default_pad_acceptcaps (GST_AUDIO_DECODER_CAST
      (flacdec), TRUE);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_AUDIO_DECODER_SINK_PAD (flacdec));
}

static gboolean
gst_flac_dec_start (GstAudioDecoder * audio_dec)
{
  FLAC__StreamDecoderInitStatus s;
  GstFlacDec *dec;

  dec = GST_FLAC_DEC (audio_dec);

  dec->adapter = gst_adapter_new ();

  dec->decoder = FLAC__stream_decoder_new ();

  gst_audio_info_init (&dec->info);
  dec->depth = 0;

  /* no point calculating MD5 since it's never checked here */
  FLAC__stream_decoder_set_md5_checking (dec->decoder, false);

  GST_DEBUG_OBJECT (dec, "initializing decoder");
  s = FLAC__stream_decoder_init_stream (dec->decoder,
      gst_flac_dec_read_stream, NULL, NULL, NULL, NULL,
      gst_flac_dec_write_stream, gst_flac_dec_metadata_cb,
      gst_flac_dec_error_cb, dec);

  if (s != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), LIBRARY, INIT, (NULL), (NULL));
    return FALSE;
  }

  dec->got_headers = FALSE;

  return TRUE;
}

static gboolean
gst_flac_dec_stop (GstAudioDecoder * dec)
{
  GstFlacDec *flacdec = GST_FLAC_DEC (dec);

  if (flacdec->decoder) {
    FLAC__stream_decoder_delete (flacdec->decoder);
    flacdec->decoder = NULL;
  }

  if (flacdec->adapter) {
    gst_adapter_clear (flacdec->adapter);
    g_object_unref (flacdec->adapter);
    flacdec->adapter = NULL;
  }

  return TRUE;
}

static gint64
gst_flac_dec_get_latency (GstFlacDec * flacdec)
{
  /* The FLAC specification states that the data is processed in blocks,
   * regardless of the number of channels. Thus, The latency can be calculated
   * using the blocksize and rate. For example a 1 second block sampled at
   * 44.1KHz has a blocksize of 44100 */

  /* Make sure the rate is valid */
  if (!flacdec->info.rate)
    return 0;

  /* Calculate the latecy */
  return (flacdec->max_blocksize * GST_SECOND) / flacdec->info.rate;
}

static gboolean
gst_flac_dec_set_format (GstAudioDecoder * dec, GstCaps * caps)
{
  const GValue *headers;
  GstFlacDec *flacdec;
  GstStructure *s;
  guint i, num;

  flacdec = GST_FLAC_DEC (dec);

  GST_LOG_OBJECT (dec, "sink caps: %" GST_PTR_FORMAT, caps);

  s = gst_caps_get_structure (caps, 0);
  headers = gst_structure_get_value (s, "streamheader");
  if (headers == NULL || !GST_VALUE_HOLDS_ARRAY (headers)) {
    GST_WARNING_OBJECT (dec, "no 'streamheader' field in input caps, try "
        "adding a flacparse element upstream");
    return FALSE;
  }

  if (gst_adapter_available (flacdec->adapter) > 0) {
    GST_WARNING_OBJECT (dec, "unexpected data left in adapter");
    gst_adapter_clear (flacdec->adapter);
  }

  FLAC__stream_decoder_reset (flacdec->decoder);
  flacdec->got_headers = FALSE;

  num = gst_value_array_get_size (headers);
  for (i = 0; i < num; ++i) {
    const GValue *header_val;
    GstBuffer *header_buf;

    header_val = gst_value_array_get_value (headers, i);
    if (header_val == NULL || !GST_VALUE_HOLDS_BUFFER (header_val))
      return FALSE;

    header_buf = g_value_dup_boxed (header_val);
    GST_INFO_OBJECT (dec, "pushing header buffer of %" G_GSIZE_FORMAT " bytes "
        "into adapter", gst_buffer_get_size (header_buf));
    gst_adapter_push (flacdec->adapter, header_buf);
  }

  GST_DEBUG_OBJECT (dec, "Processing headers and metadata");
  if (!FLAC__stream_decoder_process_until_end_of_metadata (flacdec->decoder)) {
    GST_WARNING_OBJECT (dec, "process_until_end_of_metadata failed");
    if (FLAC__stream_decoder_get_state (flacdec->decoder) ==
        FLAC__STREAM_DECODER_ABORTED) {
      GST_WARNING_OBJECT (flacdec, "Read callback caused internal abort");
      /* allow recovery */
      gst_adapter_clear (flacdec->adapter);
      FLAC__stream_decoder_flush (flacdec->decoder);
      gst_flac_dec_handle_decoder_error (flacdec, TRUE);
    }
  }
  GST_INFO_OBJECT (dec, "headers and metadata are now processed");

  return TRUE;
}

/* CRC-8, poly = x^8 + x^2 + x^1 + x^0, init = 0 */
static const guint8 crc8_table[256] = {
  0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
  0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
  0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
  0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
  0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
  0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
  0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
  0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
  0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
  0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
  0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
  0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
  0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
  0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
  0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
  0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
  0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
  0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
  0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
  0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
  0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
  0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
  0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
  0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
  0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
  0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
  0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
  0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
  0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
  0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
  0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
  0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

static guint8
gst_flac_calculate_crc8 (const guint8 * data, guint length)
{
  guint8 crc = 0;

  while (length--) {
    crc = crc8_table[crc ^ *data];
    ++data;
  }

  return crc;
}

/* FIXME: for our purposes it's probably enough to just check for the sync
 * marker - we just want to know if it's a header frame or not */
static gboolean
gst_flac_dec_scan_got_frame (GstFlacDec * flacdec, const guint8 * data,
    guint size)
{
  guint headerlen;
  guint sr_from_end = 0;        /* can be 0, 8 or 16 */
  guint bs_from_end = 0;        /* can be 0, 8 or 16 */
  guint32 val = 0;
  guint8 bs, sr, ca, ss, pb;
  gboolean vbs;

  if (size < 10)
    return FALSE;

  /* sync */
  if (data[0] != 0xFF || (data[1] & 0xFC) != 0xF8)
    return FALSE;

  vbs = ! !(data[1] & 1);       /* variable blocksize */
  bs = (data[2] & 0xF0) >> 4;   /* blocksize marker   */
  sr = (data[2] & 0x0F);        /* samplerate marker  */
  ca = (data[3] & 0xF0) >> 4;   /* channel assignment */
  ss = (data[3] & 0x0F) >> 1;   /* sample size marker */
  pb = (data[3] & 0x01);        /* padding bit        */

  GST_LOG_OBJECT (flacdec,
      "got sync, vbs=%d,bs=%x,sr=%x,ca=%x,ss=%x,pb=%x", vbs, bs, sr, ca, ss,
      pb);

  if (bs == 0 || sr == 0x0F || ca >= 0x0B || ss == 0x03 || ss == 0x07) {
    return FALSE;
  }

  /* read block size from end of header? */
  if (bs == 6)
    bs_from_end = 8;
  else if (bs == 7)
    bs_from_end = 16;

  /* read sample rate from end of header? */
  if (sr == 0x0C)
    sr_from_end = 8;
  else if (sr == 0x0D || sr == 0x0E)
    sr_from_end = 16;

  val = data[4];
  /* This is slightly faster than a loop */
  if (!(val & 0x80)) {
    val = 0;
  } else if ((val & 0xc0) && !(val & 0x20)) {
    val = 1;
  } else if ((val & 0xe0) && !(val & 0x10)) {
    val = 2;
  } else if ((val & 0xf0) && !(val & 0x08)) {
    val = 3;
  } else if ((val & 0xf8) && !(val & 0x04)) {
    val = 4;
  } else if ((val & 0xfc) && !(val & 0x02)) {
    val = 5;
  } else if ((val & 0xfe) && !(val & 0x01)) {
    val = 6;
  } else {
    GST_LOG_OBJECT (flacdec, "failed to read sample/frame");
    return FALSE;
  }

  val++;
  headerlen = 4 + val + (bs_from_end / 8) + (sr_from_end / 8);

  if (gst_flac_calculate_crc8 (data, headerlen) != data[headerlen]) {
    GST_LOG_OBJECT (flacdec, "invalid checksum");
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_flac_dec_handle_decoder_error (GstFlacDec * dec, gboolean msg)
{
  gboolean ret;

  dec->error_count++;
  if (dec->error_count > 10) {
    if (msg)
      GST_ELEMENT_ERROR (dec, STREAM, DECODE, (NULL), (NULL));
    dec->last_flow = GST_FLOW_ERROR;
    ret = TRUE;
  } else {
    GST_DEBUG_OBJECT (dec, "ignoring error for now at count %d",
        dec->error_count);
    ret = FALSE;
  }

  return ret;
}

static void
gst_flac_dec_metadata_cb (const FLAC__StreamDecoder * decoder,
    const FLAC__StreamMetadata * metadata, void *client_data)
{
  GstFlacDec *flacdec = GST_FLAC_DEC (client_data);
  GstAudioDecoder *dec = GST_AUDIO_DECODER (client_data);
  GstAudioChannelPosition position[8];
  guint64 curr_latency = 0, old_latency = gst_flac_dec_get_latency (flacdec);

  GST_LOG_OBJECT (flacdec, "metadata type: %d", metadata->type);

  switch (metadata->type) {
    case FLAC__METADATA_TYPE_STREAMINFO:{
      gint64 samples;
      guint depth, width, gdepth, channels;

      samples = metadata->data.stream_info.total_samples;

      flacdec->min_blocksize = metadata->data.stream_info.min_blocksize;
      flacdec->max_blocksize = metadata->data.stream_info.max_blocksize;
      flacdec->depth = depth = metadata->data.stream_info.bits_per_sample;

      if (depth < 9) {
        gdepth = width = 8;
      } else if (depth < 17) {
        gdepth = width = 16;
      } else if (depth < 25) {
        gdepth = 24;
        width = 32;
      } else {
        gdepth = width = 32;
      }

      channels = metadata->data.stream_info.channels;
      memcpy (position, channel_positions[channels - 1], sizeof (position));
      gst_audio_channel_positions_to_valid_order (position, channels);
      /* Note: we create the inverse reordering map here */
      gst_audio_get_channel_reorder_map (channels,
          position, channel_positions[channels - 1],
          flacdec->channel_reorder_map);

      gst_audio_info_set_format (&flacdec->info,
          gst_audio_format_build_integer (TRUE, G_BYTE_ORDER, width, gdepth),
          metadata->data.stream_info.sample_rate,
          metadata->data.stream_info.channels, position);

      gst_audio_decoder_set_output_format (GST_AUDIO_DECODER (flacdec),
          &flacdec->info);

      gst_audio_decoder_negotiate (GST_AUDIO_DECODER (flacdec));

      GST_DEBUG_OBJECT (flacdec, "blocksize: min=%u, max=%u",
          flacdec->min_blocksize, flacdec->max_blocksize);
      GST_DEBUG_OBJECT (flacdec, "sample rate: %u, channels: %u",
          flacdec->info.rate, flacdec->info.channels);
      GST_DEBUG_OBJECT (flacdec, "depth: %u, width: %u", flacdec->depth,
          flacdec->info.finfo->width);

      GST_DEBUG_OBJECT (flacdec, "total samples = %" G_GINT64_FORMAT, samples);
      break;
    }
    default:
      break;
  }

  /* Update the latency if it has changed */
  curr_latency = gst_flac_dec_get_latency (flacdec);
  if (old_latency != curr_latency)
    gst_audio_decoder_set_latency (dec, curr_latency, curr_latency);
}

static void
gst_flac_dec_error_cb (const FLAC__StreamDecoder * d,
    FLAC__StreamDecoderErrorStatus status, void *client_data)
{
  const gchar *error;
  GstFlacDec *dec;

  dec = GST_FLAC_DEC (client_data);

  switch (status) {
    case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
      dec->do_resync = TRUE;
      return;
    case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
      error = "bad header";
      break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
      error = "CRC mismatch";
      break;
    default:
      error = "unknown error";
      break;
  }

  if (gst_flac_dec_handle_decoder_error (dec, FALSE))
    GST_ELEMENT_ERROR (dec, STREAM, DECODE, (NULL), ("%s (%d)", error, status));
}

static FLAC__StreamDecoderReadStatus
gst_flac_dec_read_stream (const FLAC__StreamDecoder * decoder,
    FLAC__byte buffer[], size_t * bytes, void *client_data)
{
  GstFlacDec *dec = GST_FLAC_DEC (client_data);
  guint len;

  len = MIN (gst_adapter_available (dec->adapter), *bytes);

  if (len == 0) {
    GST_LOG_OBJECT (dec, "0 bytes available at the moment");
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
  }

  GST_LOG_OBJECT (dec, "feeding %u bytes to decoder "
      "(available=%" G_GSIZE_FORMAT ", bytes=%u)",
      len, gst_adapter_available (dec->adapter), (guint) * bytes);
  gst_adapter_copy (dec->adapter, buffer, 0, len);
  *bytes = len;

  gst_adapter_flush (dec->adapter, len);

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderWriteStatus
gst_flac_dec_write (GstFlacDec * flacdec, const FLAC__Frame * frame,
    const FLAC__int32 * const buffer[])
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstBuffer *outbuf;
  guint depth = frame->header.bits_per_sample;
  guint width, gdepth;
  guint sample_rate = frame->header.sample_rate;
  guint channels = frame->header.channels;
  guint samples = frame->header.blocksize;
  guint j, i;
  GstMapInfo map;
  gboolean caps_changed;
  GstAudioChannelPosition chanpos[8];

  GST_LOG_OBJECT (flacdec, "samples in frame header: %d", samples);

  if (depth == 0) {
    if (flacdec->depth < 4 || flacdec->depth > 32) {
      GST_ERROR_OBJECT (flacdec, "unsupported depth %d from STREAMINFO",
          flacdec->depth);
      ret = GST_FLOW_ERROR;
      goto done;
    }

    depth = flacdec->depth;
  }

  switch (depth) {
    case 8:
      gdepth = width = 8;
      break;
    case 12:
    case 16:
      gdepth = width = 16;
      break;
    case 20:
    case 24:
      gdepth = 24;
      width = 32;
      break;
    case 32:
      gdepth = width = 32;
      break;
    default:
      GST_ERROR_OBJECT (flacdec, "unsupported depth %d", depth);
      ret = GST_FLOW_ERROR;
      goto done;
  }

  if (sample_rate == 0) {
    if (flacdec->info.rate != 0) {
      sample_rate = flacdec->info.rate;
    } else {
      GST_ERROR_OBJECT (flacdec, "unknown sample rate");
      ret = GST_FLOW_ERROR;
      goto done;
    }
  }

  caps_changed = (sample_rate != GST_AUDIO_INFO_RATE (&flacdec->info))
      || (width != GST_AUDIO_INFO_WIDTH (&flacdec->info))
      || (gdepth != GST_AUDIO_INFO_DEPTH (&flacdec->info))
      || (channels != GST_AUDIO_INFO_CHANNELS (&flacdec->info));

  if (caps_changed
      || !gst_pad_has_current_caps (GST_AUDIO_DECODER_SRC_PAD (flacdec))) {
    GST_DEBUG_OBJECT (flacdec, "Negotiating %d Hz @ %d channels", sample_rate,
        channels);

    memcpy (chanpos, channel_positions[channels - 1], sizeof (chanpos));
    gst_audio_channel_positions_to_valid_order (chanpos, channels);
    gst_audio_info_set_format (&flacdec->info,
        gst_audio_format_build_integer (TRUE, G_BYTE_ORDER, width, gdepth),
        sample_rate, channels, chanpos);

    /* Note: we create the inverse reordering map here */
    gst_audio_get_channel_reorder_map (flacdec->info.channels,
        flacdec->info.position, channel_positions[flacdec->info.channels - 1],
        flacdec->channel_reorder_map);

    flacdec->depth = depth;

    gst_audio_decoder_set_output_format (GST_AUDIO_DECODER (flacdec),
        &flacdec->info);
  }

  outbuf =
      gst_buffer_new_allocate (NULL, samples * channels * (width / 8), NULL);

  gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
  if (width == 8) {
    gint8 *outbuffer = (gint8 *) map.data;
    gint *reorder_map = flacdec->channel_reorder_map;

    g_assert (gdepth == 8 && depth == 8);
    for (i = 0; i < samples; i++) {
      for (j = 0; j < channels; j++) {
        *outbuffer++ = (gint8) buffer[reorder_map[j]][i];
      }
    }
  } else if (width == 16) {
    gint16 *outbuffer = (gint16 *) map.data;
    gint *reorder_map = flacdec->channel_reorder_map;

    if (gdepth != depth) {
      for (i = 0; i < samples; i++) {
        for (j = 0; j < channels; j++) {
          *outbuffer++ =
              (gint16) (buffer[reorder_map[j]][i] << (gdepth - depth));
        }
      }
    } else {
      for (i = 0; i < samples; i++) {
        for (j = 0; j < channels; j++) {
          *outbuffer++ = (gint16) buffer[reorder_map[j]][i];
        }
      }
    }
  } else if (width == 32) {
    gint32 *outbuffer = (gint32 *) map.data;
    gint *reorder_map = flacdec->channel_reorder_map;

    if (gdepth != depth) {
      for (i = 0; i < samples; i++) {
        for (j = 0; j < channels; j++) {
          *outbuffer++ =
              (gint32) (buffer[reorder_map[j]][i] << (gdepth - depth));
        }
      }
    } else {
      for (i = 0; i < samples; i++) {
        for (j = 0; j < channels; j++) {
          *outbuffer++ = (gint32) buffer[reorder_map[j]][i];
        }
      }
    }
  } else {
    g_assert_not_reached ();
  }
  gst_buffer_unmap (outbuf, &map);

  GST_DEBUG_OBJECT (flacdec, "pushing %d samples", samples);
  if (flacdec->error_count)
    flacdec->error_count--;

  ret = gst_audio_decoder_finish_frame (GST_AUDIO_DECODER (flacdec), outbuf, 1);

  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    GST_DEBUG_OBJECT (flacdec, "finish_frame flow %s", gst_flow_get_name (ret));
  }

done:

  /* we act on the flow return value later in the handle_frame function, as we
   * don't want to mess up the internal decoder state by returning ABORT when
   * the error is in fact non-fatal (like a pad in flushing mode) and we want
   * to continue later. So just pretend everything's dandy and act later. */
  flacdec->last_flow = ret;

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static FLAC__StreamDecoderWriteStatus
gst_flac_dec_write_stream (const FLAC__StreamDecoder * decoder,
    const FLAC__Frame * frame,
    const FLAC__int32 * const buffer[], void *client_data)
{
  return gst_flac_dec_write (GST_FLAC_DEC (client_data), frame, buffer);
}

static void
gst_flac_dec_flush (GstAudioDecoder * audio_dec, gboolean hard)
{
  GstFlacDec *dec = GST_FLAC_DEC (audio_dec);

  if (!hard) {
    guint available = gst_adapter_available (dec->adapter);

    if (available > 0) {
      GST_INFO_OBJECT (dec, "draining, %u bytes left in adapter", available);
      FLAC__stream_decoder_process_until_end_of_stream (dec->decoder);
    }
  }

  dec->do_resync = FALSE;
  FLAC__stream_decoder_flush (dec->decoder);
  gst_adapter_clear (dec->adapter);
}

static GstFlowReturn
gst_flac_dec_handle_frame (GstAudioDecoder * audio_dec, GstBuffer * buf)
{
  GstFlacDec *dec;

  dec = GST_FLAC_DEC (audio_dec);

  /* drain remaining data? */
  if (G_UNLIKELY (buf == NULL)) {
    gst_flac_dec_flush (audio_dec, FALSE);
    return GST_FLOW_OK;
  }

  if (dec->do_resync) {
    GST_WARNING_OBJECT (dec, "Lost sync, flushing decoder");
    FLAC__stream_decoder_flush (dec->decoder);
    dec->do_resync = FALSE;
  }

  GST_LOG_OBJECT (dec, "frame: ts %" GST_TIME_FORMAT ", flags 0x%04x, "
      "%" G_GSIZE_FORMAT " bytes", GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)),
      GST_BUFFER_FLAGS (buf), gst_buffer_get_size (buf));

  /* drop any in-stream headers, we've processed those in set_format already */
  if (G_UNLIKELY (!dec->got_headers)) {
    gboolean got_audio_frame;
    GstMapInfo map;

    /* check if this is a flac audio frame (rather than a header or junk) */
    gst_buffer_map (buf, &map, GST_MAP_READ);
    got_audio_frame = gst_flac_dec_scan_got_frame (dec, map.data, map.size);
    gst_buffer_unmap (buf, &map);

    if (!got_audio_frame) {
      GST_INFO_OBJECT (dec, "dropping in-stream header, %" G_GSIZE_FORMAT " "
          "bytes", map.size);
      gst_audio_decoder_finish_frame (audio_dec, NULL, 1);
      return GST_FLOW_OK;
    }

    GST_INFO_OBJECT (dec, "first audio frame, got all in-stream headers now");
    dec->got_headers = TRUE;
  }

  gst_adapter_push (dec->adapter, gst_buffer_ref (buf));
  buf = NULL;

  dec->last_flow = GST_FLOW_OK;

  /* framed - there should always be enough data to decode something */
  GST_LOG_OBJECT (dec, "%" G_GSIZE_FORMAT " bytes available",
      gst_adapter_available (dec->adapter));

  if (!FLAC__stream_decoder_process_single (dec->decoder)) {
    GST_INFO_OBJECT (dec, "process_single failed");
    if (FLAC__stream_decoder_get_state (dec->decoder) ==
        FLAC__STREAM_DECODER_ABORTED) {
      GST_WARNING_OBJECT (dec, "Read callback caused internal abort");
      /* allow recovery */
      gst_adapter_clear (dec->adapter);
      FLAC__stream_decoder_flush (dec->decoder);
      gst_flac_dec_handle_decoder_error (dec, TRUE);
    }
  }

  return dec->last_flow;
}
