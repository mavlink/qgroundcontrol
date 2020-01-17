/* GStreamer
 * Copyright (C) 2004 Wim Taymans <wim@fluendo.com>
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
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
 * SECTION:element-speexdec
 * @title: speexdec
 * @see_also: speexenc, oggdemux
 *
 * This element decodes a Speex stream to raw integer audio.
 * [Speex](http://www.speex.org/) is a royalty-free
 * audio codec maintained by the [Xiph.org Foundation](http://www.xiph.org/).
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v filesrc location=speex.ogg ! oggdemux ! speexdec ! audioconvert ! audioresample ! alsasink
 * ]| Decode an Ogg/Speex file. To create an Ogg/Speex file refer to the
 * documentation of speexenc.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "gstspeexdec.h"
#include <stdlib.h>
#include <string.h>
#include <gst/tag/tag.h>
#include <gst/audio/audio.h>

GST_DEBUG_CATEGORY_STATIC (speexdec_debug);
#define GST_CAT_DEFAULT speexdec_debug

#define DEFAULT_ENH   TRUE

enum
{
  ARG_0,
  ARG_ENH
};

#define FORMAT_STR GST_AUDIO_NE(S16)

static GstStaticPadTemplate speex_dec_src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " FORMAT_STR ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 6000, 48000 ], " "channels = (int) [ 1, 2 ]")
    );

static GstStaticPadTemplate speex_dec_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-speex")
    );

#define gst_speex_dec_parent_class parent_class
G_DEFINE_TYPE (GstSpeexDec, gst_speex_dec, GST_TYPE_AUDIO_DECODER);

static gboolean gst_speex_dec_start (GstAudioDecoder * dec);
static gboolean gst_speex_dec_stop (GstAudioDecoder * dec);
static gboolean gst_speex_dec_set_format (GstAudioDecoder * bdec,
    GstCaps * caps);
static GstFlowReturn gst_speex_dec_handle_frame (GstAudioDecoder * dec,
    GstBuffer * buffer);

static void gst_speex_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_speex_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void
gst_speex_dec_class_init (GstSpeexDecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstAudioDecoderClass *base_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  base_class = (GstAudioDecoderClass *) klass;

  gobject_class->set_property = gst_speex_dec_set_property;
  gobject_class->get_property = gst_speex_dec_get_property;

  base_class->start = GST_DEBUG_FUNCPTR (gst_speex_dec_start);
  base_class->stop = GST_DEBUG_FUNCPTR (gst_speex_dec_stop);
  base_class->set_format = GST_DEBUG_FUNCPTR (gst_speex_dec_set_format);
  base_class->handle_frame = GST_DEBUG_FUNCPTR (gst_speex_dec_handle_frame);

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_ENH,
      g_param_spec_boolean ("enh", "Enh", "Enable perceptual enhancement",
          DEFAULT_ENH, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class,
      &speex_dec_src_factory);
  gst_element_class_add_static_pad_template (gstelement_class,
      &speex_dec_sink_factory);
  gst_element_class_set_static_metadata (gstelement_class,
      "Speex audio decoder", "Codec/Decoder/Audio",
      "decode speex streams to audio", "Wim Taymans <wim@fluendo.com>");

  GST_DEBUG_CATEGORY_INIT (speexdec_debug, "speexdec", 0,
      "speex decoding element");
}

static void
gst_speex_dec_reset (GstSpeexDec * dec)
{
  dec->packetno = 0;
  dec->frame_size = 0;
  dec->frame_duration = 0;
  dec->mode = NULL;
  free (dec->header);
  dec->header = NULL;
  speex_bits_destroy (&dec->bits);
  speex_bits_set_bit_buffer (&dec->bits, NULL, 0);

  gst_buffer_replace (&dec->streamheader, NULL);
  gst_buffer_replace (&dec->vorbiscomment, NULL);

  if (dec->stereo) {
    speex_stereo_state_destroy (dec->stereo);
    dec->stereo = NULL;
  }

  if (dec->state) {
    speex_decoder_destroy (dec->state);
    dec->state = NULL;
  }
}

static void
gst_speex_dec_init (GstSpeexDec * dec)
{
  gst_audio_decoder_set_needs_format (GST_AUDIO_DECODER (dec), TRUE);
  gst_audio_decoder_set_use_default_pad_acceptcaps (GST_AUDIO_DECODER_CAST
      (dec), TRUE);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_AUDIO_DECODER_SINK_PAD (dec));

  dec->enh = DEFAULT_ENH;

  gst_speex_dec_reset (dec);
}

static gboolean
gst_speex_dec_start (GstAudioDecoder * dec)
{
  GstSpeexDec *sd = GST_SPEEX_DEC (dec);

  GST_DEBUG_OBJECT (dec, "start");
  gst_speex_dec_reset (sd);

  /* we know about concealment */
  gst_audio_decoder_set_plc_aware (dec, TRUE);

  return TRUE;
}

static gboolean
gst_speex_dec_stop (GstAudioDecoder * dec)
{
  GstSpeexDec *sd = GST_SPEEX_DEC (dec);

  GST_DEBUG_OBJECT (dec, "stop");
  gst_speex_dec_reset (sd);

  return TRUE;
}

static GstFlowReturn
gst_speex_dec_parse_header (GstSpeexDec * dec, GstBuffer * buf)
{
  GstMapInfo map;
  GstAudioInfo info;
  static const GstAudioChannelPosition chan_pos[2][2] = {
    {GST_AUDIO_CHANNEL_POSITION_MONO},
    {GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT}
  };

  /* get the header */
  gst_buffer_map (buf, &map, GST_MAP_READ);
  dec->header = speex_packet_to_header ((gchar *) map.data, map.size);
  gst_buffer_unmap (buf, &map);

  if (!dec->header)
    goto no_header;

  if (dec->header->mode >= SPEEX_NB_MODES || dec->header->mode < 0)
    goto mode_too_old;

  dec->mode = speex_lib_get_mode (dec->header->mode);

  /* initialize the decoder */
  dec->state = speex_decoder_init (dec->mode);
  if (!dec->state)
    goto init_failed;

  speex_decoder_ctl (dec->state, SPEEX_SET_ENH, &dec->enh);
  speex_decoder_ctl (dec->state, SPEEX_GET_FRAME_SIZE, &dec->frame_size);

  if (dec->header->nb_channels != 1) {
    dec->stereo = speex_stereo_state_init ();
    dec->callback.callback_id = SPEEX_INBAND_STEREO;
    dec->callback.func = speex_std_stereo_request_handler;
    dec->callback.data = dec->stereo;
    speex_decoder_ctl (dec->state, SPEEX_SET_HANDLER, &dec->callback);
  }

  speex_decoder_ctl (dec->state, SPEEX_SET_SAMPLING_RATE, &dec->header->rate);

  dec->frame_duration = gst_util_uint64_scale_int (dec->frame_size,
      GST_SECOND, dec->header->rate);

  speex_bits_init (&dec->bits);

  /* set caps */
  gst_audio_info_init (&info);
  gst_audio_info_set_format (&info,
      GST_AUDIO_FORMAT_S16,
      dec->header->rate,
      dec->header->nb_channels, chan_pos[dec->header->nb_channels - 1]);

  if (!gst_audio_decoder_set_output_format (GST_AUDIO_DECODER (dec), &info))
    goto nego_failed;

  return GST_FLOW_OK;

  /* ERRORS */
no_header:
  {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
        (NULL), ("couldn't read header"));
    return GST_FLOW_ERROR;
  }
mode_too_old:
  {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
        (NULL),
        ("Mode number %d does not (yet/any longer) exist in this version",
            dec->header->mode));
    return GST_FLOW_ERROR;
  }
init_failed:
  {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
        (NULL), ("couldn't initialize decoder"));
    return GST_FLOW_ERROR;
  }
nego_failed:
  {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
        (NULL), ("couldn't negotiate format"));
    return GST_FLOW_NOT_NEGOTIATED;
  }
}

static GstFlowReturn
gst_speex_dec_parse_comments (GstSpeexDec * dec, GstBuffer * buf)
{
  GstTagList *list;
  gchar *ver, *encoder = NULL;

  list = gst_tag_list_from_vorbiscomment_buffer (buf, NULL, 0, &encoder);

  if (!list) {
    GST_WARNING_OBJECT (dec, "couldn't decode comments");
    list = gst_tag_list_new_empty ();
  }

  if (encoder) {
    gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
        GST_TAG_ENCODER, encoder, NULL);
  }

  gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
      GST_TAG_AUDIO_CODEC, "Speex", NULL);

  ver = g_strndup (dec->header->speex_version, SPEEX_HEADER_VERSION_LENGTH);
  g_strstrip (ver);

  if (ver != NULL && *ver != '\0') {
    gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
        GST_TAG_ENCODER_VERSION, ver, NULL);
  }

  if (dec->header->bitrate > 0) {
    gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
        GST_TAG_BITRATE, (guint) dec->header->bitrate, NULL);
  }

  GST_INFO_OBJECT (dec, "tags: %" GST_PTR_FORMAT, list);

  gst_audio_decoder_merge_tags (GST_AUDIO_DECODER (dec), list,
      GST_TAG_MERGE_REPLACE);
  gst_tag_list_unref (list);

  g_free (encoder);
  g_free (ver);

  return GST_FLOW_OK;
}

static gboolean
gst_speex_dec_set_format (GstAudioDecoder * bdec, GstCaps * caps)
{
  GstSpeexDec *dec = GST_SPEEX_DEC (bdec);
  gboolean ret = TRUE;
  GstStructure *s;
  const GValue *streamheader;

  s = gst_caps_get_structure (caps, 0);
  if ((streamheader = gst_structure_get_value (s, "streamheader")) &&
      G_VALUE_HOLDS (streamheader, GST_TYPE_ARRAY) &&
      gst_value_array_get_size (streamheader) >= 2) {
    const GValue *header, *vorbiscomment;
    GstBuffer *buf;
    GstFlowReturn res = GST_FLOW_OK;

    header = gst_value_array_get_value (streamheader, 0);
    if (header && G_VALUE_HOLDS (header, GST_TYPE_BUFFER)) {
      buf = gst_value_get_buffer (header);
      res = gst_speex_dec_parse_header (dec, buf);
      if (res != GST_FLOW_OK)
        goto done;
      gst_buffer_replace (&dec->streamheader, buf);
    }

    vorbiscomment = gst_value_array_get_value (streamheader, 1);
    if (vorbiscomment && G_VALUE_HOLDS (vorbiscomment, GST_TYPE_BUFFER)) {
      buf = gst_value_get_buffer (vorbiscomment);
      res = gst_speex_dec_parse_comments (dec, buf);
      if (res != GST_FLOW_OK)
        goto done;
      gst_buffer_replace (&dec->vorbiscomment, buf);
    }
  }

done:
  return ret;
}

static GstFlowReturn
gst_speex_dec_parse_data (GstSpeexDec * dec, GstBuffer * buf)
{
  GstFlowReturn res = GST_FLOW_OK;
  gint i, fpp;
  SpeexBits *bits;
  GstMapInfo map;

  if (!dec->frame_duration)
    goto not_negotiated;

  if (G_LIKELY (gst_buffer_get_size (buf))) {
    /* send data to the bitstream */
    gst_buffer_map (buf, &map, GST_MAP_READ);
    speex_bits_read_from (&dec->bits, (gchar *) map.data, map.size);
    gst_buffer_unmap (buf, &map);

    fpp = dec->header->frames_per_packet;
    bits = &dec->bits;

    GST_DEBUG_OBJECT (dec, "received buffer of size %" G_GSIZE_FORMAT
        ", fpp %d, %d bits", map.size, fpp, speex_bits_remaining (bits));
  } else {
    /* FIXME ? actually consider how much concealment is needed */
    /* concealment data, pass NULL as the bits parameters */
    GST_DEBUG_OBJECT (dec, "creating concealment data");
    fpp = dec->header->frames_per_packet;
    bits = NULL;
  }

  /* now decode each frame, catering for unknown number of them (e.g. rtp) */
  for (i = 0; i < fpp; i++) {
    GstBuffer *outbuf;
    gboolean corrupted = FALSE;
    gint ret;

    GST_LOG_OBJECT (dec, "decoding frame %d/%d, %d bits remaining", i, fpp,
        bits ? speex_bits_remaining (bits) : -1);
#if 0
    res =
        gst_pad_alloc_buffer_and_set_caps (GST_AUDIO_DECODER_SRC_PAD (dec),
        GST_BUFFER_OFFSET_NONE, dec->frame_size * dec->header->nb_channels * 2,
        GST_PAD_CAPS (GST_AUDIO_DECODER_SRC_PAD (dec)), &outbuf);

    if (res != GST_FLOW_OK) {
      GST_DEBUG_OBJECT (dec, "buf alloc flow: %s", gst_flow_get_name (res));
      return res;
    }
#endif
    /* FIXME, we can use a bufferpool because we have fixed size buffers. We
     * could also use an allocator */
    outbuf =
        gst_buffer_new_allocate (NULL,
        dec->frame_size * dec->header->nb_channels * 2, NULL);

    gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
    ret = speex_decode_int (dec->state, bits, (spx_int16_t *) map.data);

    if (ret == -1) {
      /* uh? end of stream */
      GST_WARNING_OBJECT (dec, "Unexpected end of stream found");
      corrupted = TRUE;
    } else if (ret == -2) {
      GST_WARNING_OBJECT (dec, "Decoding error: corrupted stream?");
      corrupted = TRUE;
    }

    if (bits && speex_bits_remaining (bits) < 0) {
      GST_WARNING_OBJECT (dec, "Decoding overflow: corrupted stream?");
      corrupted = TRUE;
    }
    if (dec->header->nb_channels == 2)
      speex_decode_stereo_int ((spx_int16_t *) map.data, dec->frame_size,
          dec->stereo);

    gst_buffer_unmap (outbuf, &map);

    if (!corrupted) {
      res = gst_audio_decoder_finish_frame (GST_AUDIO_DECODER (dec), outbuf, 1);
    } else {
      res = gst_audio_decoder_finish_frame (GST_AUDIO_DECODER (dec), NULL, 1);
      gst_buffer_unref (outbuf);
    }

    if (res != GST_FLOW_OK) {
      GST_DEBUG_OBJECT (dec, "flow: %s", gst_flow_get_name (res));
      break;
    }
  }

  return res;

  /* ERRORS */
not_negotiated:
  {
    GST_ELEMENT_ERROR (dec, CORE, NEGOTIATION, (NULL),
        ("decoder not initialized"));
    return GST_FLOW_NOT_NEGOTIATED;
  }
}

static gboolean
memcmp_buffers (GstBuffer * buf1, GstBuffer * buf2)
{
  GstMapInfo map;
  gsize size1, size2;
  gboolean res;

  size1 = gst_buffer_get_size (buf1);
  size2 = gst_buffer_get_size (buf2);

  if (size1 != size2)
    return FALSE;

  gst_buffer_map (buf1, &map, GST_MAP_READ);
  res = gst_buffer_memcmp (buf2, 0, map.data, map.size) == 0;
  gst_buffer_unmap (buf1, &map);

  return res;
}

static GstFlowReturn
gst_speex_dec_handle_frame (GstAudioDecoder * bdec, GstBuffer * buf)
{
  GstFlowReturn res;
  GstSpeexDec *dec;

  /* no fancy draining */
  if (G_UNLIKELY (!buf))
    return GST_FLOW_OK;

  dec = GST_SPEEX_DEC (bdec);

  /* If we have the streamheader and vorbiscomment from the caps already
   * ignore them here */
  if (dec->streamheader && dec->vorbiscomment) {
    if (memcmp_buffers (dec->streamheader, buf)) {
      GST_DEBUG_OBJECT (dec, "found streamheader");
      gst_audio_decoder_finish_frame (bdec, NULL, 1);
      res = GST_FLOW_OK;
    } else if (memcmp_buffers (dec->vorbiscomment, buf)) {
      GST_DEBUG_OBJECT (dec, "found vorbiscomments");
      gst_audio_decoder_finish_frame (bdec, NULL, 1);
      res = GST_FLOW_OK;
    } else {
      res = gst_speex_dec_parse_data (dec, buf);
    }
  } else {
    /* Otherwise fall back to packet counting and assume that the
     * first two packets are the headers. */
    switch (dec->packetno) {
      case 0:
        GST_DEBUG_OBJECT (dec, "counted streamheader");
        res = gst_speex_dec_parse_header (dec, buf);
        gst_audio_decoder_finish_frame (bdec, NULL, 1);
        break;
      case 1:
        GST_DEBUG_OBJECT (dec, "counted vorbiscomments");
        res = gst_speex_dec_parse_comments (dec, buf);
        gst_audio_decoder_finish_frame (bdec, NULL, 1);
        break;
      default:
      {
        res = gst_speex_dec_parse_data (dec, buf);
        break;
      }
    }
  }

  dec->packetno++;

  return res;
}

static void
gst_speex_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSpeexDec *speexdec;

  speexdec = GST_SPEEX_DEC (object);

  switch (prop_id) {
    case ARG_ENH:
      g_value_set_boolean (value, speexdec->enh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_speex_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSpeexDec *speexdec;

  speexdec = GST_SPEEX_DEC (object);

  switch (prop_id) {
    case ARG_ENH:
      speexdec->enh = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
