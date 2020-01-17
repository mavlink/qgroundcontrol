/* GStreamer Wavpack plugin
 * Copyright (c) 2005 Arwed v. Merkatz <v.merkatz@gmx.net>
 * Copyright (c) 2006 Edward Hervey <bilboed@gmail.com>
 * Copyright (c) 2006 Sebastian Dröge <slomo@circular-chaos.org>
 *
 * gstwavpackdec.c: raw Wavpack bitstream decoder
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
 * SECTION:element-wavpackdec
 * @title: wavpackdec
 *
 * WavpackDec decodes framed (for example by the WavpackParse element)
 * Wavpack streams and decodes them to raw audio.
 * [Wavpack](http://www.wavpack.com/) is an open-source audio codec that
 * features both lossless and lossy encoding.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=test.wv ! wavpackparse ! wavpackdec ! audioconvert ! audioresample ! autoaudiosink
 * ]| This pipeline decodes the Wavpack file test.wv into raw audio buffers and
 * tries to play it back using an automatically found audio sink.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>

#include <math.h>
#include <string.h>

#include <wavpack/wavpack.h>
#include "gstwavpackdec.h"
#include "gstwavpackcommon.h"
#include "gstwavpackstreamreader.h"


GST_DEBUG_CATEGORY_STATIC (gst_wavpack_dec_debug);
#define GST_CAT_DEFAULT gst_wavpack_dec_debug

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-wavpack, "
        "depth = (int) [ 1, 32 ], "
        "channels = (int) [ 1, 8 ], "
        "rate = (int) [ 6000, 192000 ], " "framed = (boolean) true")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) S8, "
        "layout = (string) interleaved, "
        "channels = (int) [ 1, 8 ], "
        "rate = (int) [ 6000, 192000 ]; "
        "audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "channels = (int) [ 1, 8 ], "
        "rate = (int) [ 6000, 192000 ]; "
        "audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S32) ", "
        "layout = (string) interleaved, "
        "channels = (int) [ 1, 8 ], " "rate = (int) [ 6000, 192000 ]")
    );

static gboolean gst_wavpack_dec_start (GstAudioDecoder * dec);
static gboolean gst_wavpack_dec_stop (GstAudioDecoder * dec);
static gboolean gst_wavpack_dec_set_format (GstAudioDecoder * dec,
    GstCaps * caps);
static GstFlowReturn gst_wavpack_dec_handle_frame (GstAudioDecoder * dec,
    GstBuffer * buffer);

static void gst_wavpack_dec_finalize (GObject * object);
static void gst_wavpack_dec_post_tags (GstWavpackDec * dec);

#define gst_wavpack_dec_parent_class parent_class
G_DEFINE_TYPE (GstWavpackDec, gst_wavpack_dec, GST_TYPE_AUDIO_DECODER);

static void
gst_wavpack_dec_class_init (GstWavpackDecClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) (klass);
  GstAudioDecoderClass *base_class = (GstAudioDecoderClass *) (klass);

  gst_element_class_add_static_pad_template (element_class, &src_factory);
  gst_element_class_add_static_pad_template (element_class, &sink_factory);
  gst_element_class_set_static_metadata (element_class, "Wavpack audio decoder",
      "Codec/Decoder/Audio",
      "Decodes Wavpack audio data",
      "Arwed v. Merkatz <v.merkatz@gmx.net>, "
      "Sebastian Dröge <slomo@circular-chaos.org>");

  gobject_class->finalize = gst_wavpack_dec_finalize;

  base_class->start = GST_DEBUG_FUNCPTR (gst_wavpack_dec_start);
  base_class->stop = GST_DEBUG_FUNCPTR (gst_wavpack_dec_stop);
  base_class->set_format = GST_DEBUG_FUNCPTR (gst_wavpack_dec_set_format);
  base_class->handle_frame = GST_DEBUG_FUNCPTR (gst_wavpack_dec_handle_frame);
}

static void
gst_wavpack_dec_reset (GstWavpackDec * dec)
{
  dec->wv_id.buffer = NULL;
  dec->wv_id.position = dec->wv_id.length = 0;

  dec->channels = 0;
  dec->channel_mask = 0;
  dec->sample_rate = 0;
  dec->depth = 0;
}

static void
gst_wavpack_dec_init (GstWavpackDec * dec)
{
  dec->context = NULL;
  dec->stream_reader = gst_wavpack_stream_reader_new ();

  gst_audio_decoder_set_needs_format (GST_AUDIO_DECODER (dec), TRUE);
  gst_audio_decoder_set_use_default_pad_acceptcaps (GST_AUDIO_DECODER_CAST
      (dec), TRUE);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_AUDIO_DECODER_SINK_PAD (dec));

  gst_wavpack_dec_reset (dec);
}

static void
gst_wavpack_dec_finalize (GObject * object)
{
  GstWavpackDec *dec = GST_WAVPACK_DEC (object);

  g_free (dec->stream_reader);
  dec->stream_reader = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_wavpack_dec_start (GstAudioDecoder * dec)
{
  GST_DEBUG_OBJECT (dec, "start");

  /* never mind a few errors */
  gst_audio_decoder_set_max_errors (dec, 16);
  /* don't bother us with flushing */
  gst_audio_decoder_set_drainable (dec, FALSE);
  /* aim for some perfect timestamping */
  gst_audio_decoder_set_tolerance (dec, 10 * GST_MSECOND);

  return TRUE;
}

static gboolean
gst_wavpack_dec_stop (GstAudioDecoder * dec)
{
  GstWavpackDec *wpdec = GST_WAVPACK_DEC (dec);

  GST_DEBUG_OBJECT (dec, "stop");

  if (wpdec->context) {
    WavpackCloseFile (wpdec->context);
    wpdec->context = NULL;
  }

  gst_wavpack_dec_reset (wpdec);

  return TRUE;
}

static void
gst_wavpack_dec_negotiate (GstWavpackDec * dec)
{
  GstAudioInfo info;
  GstAudioFormat fmt;
  GstAudioChannelPosition pos[64] = { GST_AUDIO_CHANNEL_POSITION_INVALID, };

  /* arrange for 1, 2 or 4-byte width == depth output */
  dec->width = dec->depth;
  switch (dec->depth) {
    case 8:
      fmt = GST_AUDIO_FORMAT_S8;
      break;
    case 16:
      fmt = _GST_AUDIO_FORMAT_NE (S16);
      break;
    case 24:
    case 32:
      fmt = _GST_AUDIO_FORMAT_NE (S32);
      dec->width = 32;
      break;
    default:
      fmt = GST_AUDIO_FORMAT_UNKNOWN;
      g_assert_not_reached ();
      break;
  }

  g_assert (dec->channel_mask != 0);

  if (!gst_wavpack_get_channel_positions (dec->channels,
          dec->channel_mask, pos))
    GST_WARNING_OBJECT (dec, "Failed to set channel layout");

  gst_audio_info_init (&info);
  gst_audio_info_set_format (&info, fmt, dec->sample_rate, dec->channels, pos);

  gst_audio_channel_positions_to_valid_order (info.position, info.channels);
  gst_audio_get_channel_reorder_map (info.channels,
      info.position, pos, dec->channel_reorder_map);

  /* should always succeed */
  gst_audio_decoder_set_output_format (GST_AUDIO_DECODER (dec), &info);
}

static gboolean
gst_wavpack_dec_set_format (GstAudioDecoder * bdec, GstCaps * caps)
{
  /* pretty much nothing to do here,
   * we'll parse it all from the stream and setup then */

  return TRUE;
}

static void
gst_wavpack_dec_post_tags (GstWavpackDec * dec)
{
  GstTagList *list;
  GstFormat format_time = GST_FORMAT_TIME, format_bytes = GST_FORMAT_BYTES;
  gint64 duration, size;

  /* try to estimate the average bitrate */
  if (gst_pad_peer_query_duration (GST_AUDIO_DECODER_SINK_PAD (dec),
          format_bytes, &size) &&
      gst_pad_peer_query_duration (GST_AUDIO_DECODER_SINK_PAD (dec),
          format_time, &duration) && size > 0 && duration > 0) {
    guint64 bitrate;

    list = gst_tag_list_new_empty ();

    bitrate = gst_util_uint64_scale (size, 8 * GST_SECOND, duration);
    gst_tag_list_add (list, GST_TAG_MERGE_REPLACE, GST_TAG_BITRATE,
        (guint) bitrate, NULL);
    gst_audio_decoder_merge_tags (GST_AUDIO_DECODER (dec), list,
        GST_TAG_MERGE_REPLACE);
    gst_tag_list_unref (list);
  }
}

static GstFlowReturn
gst_wavpack_dec_handle_frame (GstAudioDecoder * bdec, GstBuffer * buf)
{
  GstWavpackDec *dec;
  GstBuffer *outbuf = NULL;
  GstFlowReturn ret = GST_FLOW_OK;
  WavpackHeader wph;
  int32_t decoded, unpacked_size;
  gboolean format_changed;
  gint width, depth, i, j, max;
  gint32 *dec_data = NULL;
  guint8 *out_data;
  GstMapInfo map, omap;

  dec = GST_WAVPACK_DEC (bdec);

  g_return_val_if_fail (buf != NULL, GST_FLOW_ERROR);

  gst_buffer_map (buf, &map, GST_MAP_READ);

  /* check input, we only accept framed input with complete chunks */
  if (map.size < sizeof (WavpackHeader))
    goto input_not_framed;

  if (!gst_wavpack_read_header (&wph, map.data))
    goto invalid_header;

  if (map.size < wph.ckSize + 4 * 1 + 4)
    goto input_not_framed;

  if (!(wph.flags & INITIAL_BLOCK))
    goto input_not_framed;

  dec->wv_id.buffer = map.data;
  dec->wv_id.length = map.size;
  dec->wv_id.position = 0;

  /* create a new wavpack context if there is none yet but if there
   * was already one (i.e. caps were set on the srcpad) check whether
   * the new one has the same caps */
  if (!dec->context) {
    gchar error_msg[80];

    dec->context = WavpackOpenFileInputEx (dec->stream_reader,
        &dec->wv_id, NULL, error_msg, OPEN_STREAMING, 0);

    /* expect this to work */
    if (!dec->context) {
      GST_WARNING_OBJECT (dec, "Couldn't decode buffer: %s", error_msg);
      goto context_failed;
    }
  }

  g_assert (dec->context != NULL);

  format_changed =
      (dec->sample_rate != WavpackGetSampleRate (dec->context)) ||
      (dec->channels != WavpackGetNumChannels (dec->context)) ||
      (dec->depth != WavpackGetBytesPerSample (dec->context) * 8) ||
      (dec->channel_mask != WavpackGetChannelMask (dec->context));

  if (!gst_pad_has_current_caps (GST_AUDIO_DECODER_SRC_PAD (dec)) ||
      format_changed) {
    gint channel_mask;

    dec->sample_rate = WavpackGetSampleRate (dec->context);
    dec->channels = WavpackGetNumChannels (dec->context);
    dec->depth = WavpackGetBytesPerSample (dec->context) * 8;

    channel_mask = WavpackGetChannelMask (dec->context);
    if (channel_mask == 0)
      channel_mask = gst_wavpack_get_default_channel_mask (dec->channels);

    dec->channel_mask = channel_mask;

    gst_wavpack_dec_negotiate (dec);

    /* send GST_TAG_AUDIO_CODEC and GST_TAG_BITRATE tags before something
     * is decoded or after the format has changed */
    gst_wavpack_dec_post_tags (dec);
  }

  /* alloc output buffer */
  dec_data = g_malloc (4 * wph.block_samples * dec->channels);

  /* decode */
  decoded = WavpackUnpackSamples (dec->context, dec_data, wph.block_samples);
  if (decoded != wph.block_samples)
    goto decode_error;

  unpacked_size = (dec->width / 8) * wph.block_samples * dec->channels;
  outbuf = gst_buffer_new_and_alloc (unpacked_size);

  /* legacy; pass along offset, whatever that might entail */
  GST_BUFFER_OFFSET (outbuf) = GST_BUFFER_OFFSET (buf);

  gst_buffer_map (outbuf, &omap, GST_MAP_WRITE);
  out_data = omap.data;

  width = dec->width;
  depth = dec->depth;
  max = dec->channels * wph.block_samples;
  if (width == 8) {
    gint8 *outbuffer = (gint8 *) out_data;
    gint *reorder_map = dec->channel_reorder_map;

    for (i = 0; i < max; i += dec->channels) {
      for (j = 0; j < dec->channels; j++)
        *outbuffer++ = (gint8) (dec_data[i + reorder_map[j]]);
    }
  } else if (width == 16) {
    gint16 *outbuffer = (gint16 *) out_data;
    gint *reorder_map = dec->channel_reorder_map;

    for (i = 0; i < max; i += dec->channels) {
      for (j = 0; j < dec->channels; j++)
        *outbuffer++ = (gint16) (dec_data[i + reorder_map[j]]);
    }
  } else if (dec->width == 32) {
    gint32 *outbuffer = (gint32 *) out_data;
    gint *reorder_map = dec->channel_reorder_map;

    if (width != depth) {
      for (i = 0; i < max; i += dec->channels) {
        for (j = 0; j < dec->channels; j++)
          *outbuffer++ =
              (gint32) (dec_data[i + reorder_map[j]] << (width - depth));
      }
    } else {
      for (i = 0; i < max; i += dec->channels) {
        for (j = 0; j < dec->channels; j++)
          *outbuffer++ = (gint32) (dec_data[i + reorder_map[j]]);
      }
    }
  } else {
    g_assert_not_reached ();
  }

  gst_buffer_unmap (outbuf, &omap);
  gst_buffer_unmap (buf, &map);
  buf = NULL;

  g_free (dec_data);

  ret = gst_audio_decoder_finish_frame (bdec, outbuf, 1);

out:
  if (buf)
    gst_buffer_unmap (buf, &map);

  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    GST_DEBUG_OBJECT (dec, "flow: %s", gst_flow_get_name (ret));
  }

  return ret;

/* ERRORS */
input_not_framed:
  {
    GST_ELEMENT_ERROR (dec, STREAM, DECODE, (NULL), ("Expected framed input"));
    ret = GST_FLOW_ERROR;
    goto out;
  }
invalid_header:
  {
    GST_ELEMENT_ERROR (dec, STREAM, DECODE, (NULL), ("Invalid wavpack header"));
    ret = GST_FLOW_ERROR;
    goto out;
  }
context_failed:
  {
    GST_AUDIO_DECODER_ERROR (bdec, 1, LIBRARY, INIT, (NULL),
        ("error creating Wavpack context"), ret);
    goto out;
  }
decode_error:
  {
    const gchar *reason = "unknown";

    if (dec->context) {
      reason = WavpackGetErrorMessage (dec->context);
    } else {
      reason = "couldn't create decoder context";
    }
    GST_AUDIO_DECODER_ERROR (bdec, 1, STREAM, DECODE, (NULL),
        ("decoding error: %s", reason), ret);
    g_free (dec_data);
    if (ret == GST_FLOW_OK)
      gst_audio_decoder_finish_frame (bdec, NULL, 1);
    goto out;
  }
}

gboolean
gst_wavpack_dec_plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "wavpackdec",
          GST_RANK_PRIMARY, GST_TYPE_WAVPACK_DEC))
    return FALSE;
  GST_DEBUG_CATEGORY_INIT (gst_wavpack_dec_debug, "wavpackdec", 0,
      "Wavpack decoder");
  return TRUE;
}
