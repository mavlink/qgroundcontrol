/* GStreamer A-Law to PCM conversion
 * Copyright (C) 2000 by Abramo Bagnara <abramo@alsa-project.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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
 * SECTION:element-alawdec
 * @title: alawdec
 *
 * This element decodes alaw audio. Alaw coding is also known as G.711.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "alaw-decode.h"

extern GstStaticPadTemplate alaw_dec_src_factory;
extern GstStaticPadTemplate alaw_dec_sink_factory;

GST_DEBUG_CATEGORY_STATIC (alaw_dec_debug);
#define GST_CAT_DEFAULT alaw_dec_debug

static gboolean gst_alaw_dec_set_format (GstAudioDecoder * dec, GstCaps * caps);
static GstFlowReturn gst_alaw_dec_handle_frame (GstAudioDecoder * dec,
    GstBuffer * buffer);

#define gst_alaw_dec_parent_class parent_class
G_DEFINE_TYPE (GstALawDec, gst_alaw_dec, GST_TYPE_AUDIO_DECODER);

/* some day we might have defines in gstconfig.h that tell us about the
 * desired cpu/memory/binary size trade-offs */
#define GST_ALAW_DEC_USE_TABLE

#ifdef GST_ALAW_DEC_USE_TABLE

static const gint alaw_to_s16_table[256] = {
  -5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736,
  -7552, -7296, -8064, -7808, -6528, -6272, -7040, -6784,
  -2752, -2624, -3008, -2880, -2240, -2112, -2496, -2368,
  -3776, -3648, -4032, -3904, -3264, -3136, -3520, -3392,
  -22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
  -30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136,
  -11008, -10496, -12032, -11520, -8960, -8448, -9984, -9472,
  -15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568,
  -344, -328, -376, -360, -280, -264, -312, -296,
  -472, -456, -504, -488, -408, -392, -440, -424,
  -88, -72, -120, -104, -24, -8, -56, -40,
  -216, -200, -248, -232, -152, -136, -184, -168,
  -1376, -1312, -1504, -1440, -1120, -1056, -1248, -1184,
  -1888, -1824, -2016, -1952, -1632, -1568, -1760, -1696,
  -688, -656, -752, -720, -560, -528, -624, -592,
  -944, -912, -1008, -976, -816, -784, -880, -848,
  5504, 5248, 6016, 5760, 4480, 4224, 4992, 4736,
  7552, 7296, 8064, 7808, 6528, 6272, 7040, 6784,
  2752, 2624, 3008, 2880, 2240, 2112, 2496, 2368,
  3776, 3648, 4032, 3904, 3264, 3136, 3520, 3392,
  22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944,
  30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136,
  11008, 10496, 12032, 11520, 8960, 8448, 9984, 9472,
  15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568,
  344, 328, 376, 360, 280, 264, 312, 296,
  472, 456, 504, 488, 408, 392, 440, 424,
  88, 72, 120, 104, 24, 8, 56, 40,
  216, 200, 248, 232, 152, 136, 184, 168,
  1376, 1312, 1504, 1440, 1120, 1056, 1248, 1184,
  1888, 1824, 2016, 1952, 1632, 1568, 1760, 1696,
  688, 656, 752, 720, 560, 528, 624, 592,
  944, 912, 1008, 976, 816, 784, 880, 848
};

static inline gint
alaw_to_s16 (guint8 a_val)
{
  return alaw_to_s16_table[a_val];
}

#else /* GST_ALAW_DEC_USE_TABLE */

static inline gint
alaw_to_s16 (guint8 a_val)
{
  gint t;
  gint seg;

  a_val ^= 0x55;
  t = a_val & 0x7f;
  if (t < 16)
    t = (t << 4) + 8;
  else {
    seg = (t >> 4) & 0x07;
    t = ((t & 0x0f) << 4) + 0x108;
    t <<= seg - 1;
  }
  return ((a_val & 0x80) ? t : -t);
}

#endif /* GST_ALAW_DEC_USE_TABLE */

static gboolean
gst_alaw_dec_set_format (GstAudioDecoder * dec, GstCaps * caps)
{
  GstALawDec *alawdec = GST_ALAW_DEC (dec);
  GstStructure *structure;
  int rate, channels;
  GstAudioInfo info;

  structure = gst_caps_get_structure (caps, 0);
  if (!structure) {
    GST_ERROR_OBJECT (dec, "failed to get structure from caps");
    return FALSE;
  }

  if (!gst_structure_get_int (structure, "rate", &rate)) {
    GST_ERROR_OBJECT (dec, "failed to find field rate in input caps");
    return FALSE;
  }

  if (!gst_structure_get_int (structure, "channels", &channels)) {
    GST_ERROR_OBJECT (dec, "failed to find field channels in input caps");
    return FALSE;
  }

  gst_audio_info_init (&info);
  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S16, rate, channels, NULL);

  GST_DEBUG_OBJECT (alawdec, "rate=%d, channels=%d", rate, channels);

  return gst_audio_decoder_set_output_format (dec, &info);
}

static GstFlowReturn
gst_alaw_dec_handle_frame (GstAudioDecoder * dec, GstBuffer * buffer)
{
  GstMapInfo inmap, outmap;
  gint16 *linear_data;
  guint8 *alaw_data;
  gsize alaw_size, linear_size;
  GstBuffer *outbuf;
  gint i;

  if (!buffer) {
    return GST_FLOW_OK;
  }

  if (!gst_buffer_map (buffer, &inmap, GST_MAP_READ)) {
    GST_ERROR_OBJECT (dec, "failed to map input buffer");
    goto error_failed_map_input_buffer;
  }

  alaw_data = inmap.data;
  alaw_size = inmap.size;

  linear_size = alaw_size * 2;

  outbuf = gst_audio_decoder_allocate_output_buffer (dec, linear_size);
  if (!gst_buffer_map (outbuf, &outmap, GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (dec, "failed to map input buffer");
    goto error_failed_map_output_buffer;
  }

  linear_data = (gint16 *) outmap.data;
  for (i = 0; i < alaw_size; i++) {
    linear_data[i] = alaw_to_s16 (alaw_data[i]);
  }

  gst_buffer_unmap (outbuf, &outmap);
  gst_buffer_unmap (buffer, &inmap);

  return gst_audio_decoder_finish_frame (dec, outbuf, -1);

error_failed_map_output_buffer:
  gst_buffer_unref (outbuf);
  gst_buffer_unmap (buffer, &inmap);

error_failed_map_input_buffer:
  return GST_FLOW_ERROR;
}

static gboolean
gst_alaw_dec_start (GstAudioDecoder * dec)
{
  gst_audio_decoder_set_estimate_rate (dec, TRUE);

  return TRUE;
}

static void
gst_alaw_dec_class_init (GstALawDecClass * klass)
{
  GstElementClass *element_class = (GstElementClass *) klass;
  GstAudioDecoderClass *audiodec_class = GST_AUDIO_DECODER_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &alaw_dec_src_factory);
  gst_element_class_add_static_pad_template (element_class,
      &alaw_dec_sink_factory);

  audiodec_class->start = GST_DEBUG_FUNCPTR (gst_alaw_dec_start);
  audiodec_class->set_format = GST_DEBUG_FUNCPTR (gst_alaw_dec_set_format);
  audiodec_class->handle_frame = GST_DEBUG_FUNCPTR (gst_alaw_dec_handle_frame);

  gst_element_class_set_static_metadata (element_class, "A Law audio decoder",
      "Codec/Decoder/Audio",
      "Convert 8bit A law to 16bit PCM",
      "Zaheer Abbas Merali <zaheerabbas at merali dot org>");

  GST_DEBUG_CATEGORY_INIT (alaw_dec_debug, "alawdec", 0, "A Law audio decoder");
}

static void
gst_alaw_dec_init (GstALawDec * alawdec)
{
  gst_audio_decoder_set_needs_format (GST_AUDIO_DECODER (alawdec), TRUE);
  gst_audio_decoder_set_use_default_pad_acceptcaps (GST_AUDIO_DECODER_CAST
      (alawdec), TRUE);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_AUDIO_DECODER_SINK_PAD (alawdec));
}
