/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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
 * SECTION:element-mulawdec
 * @title: mulawdec
 *
 * This element decodes mulaw audio. Mulaw coding is also known as G.711.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gst/gst.h>

#include "mulaw-decode.h"
#include "mulaw-conversion.h"

extern GstStaticPadTemplate mulaw_dec_src_factory;
extern GstStaticPadTemplate mulaw_dec_sink_factory;

static gboolean gst_mulawdec_set_format (GstAudioDecoder * dec, GstCaps * caps);
static GstFlowReturn gst_mulawdec_handle_frame (GstAudioDecoder * dec,
    GstBuffer * buffer);


/* Stereo signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0
};

#define gst_mulawdec_parent_class parent_class
G_DEFINE_TYPE (GstMuLawDec, gst_mulawdec, GST_TYPE_AUDIO_DECODER);

static gboolean
gst_mulawdec_set_format (GstAudioDecoder * dec, GstCaps * caps)
{
  GstMuLawDec *mulawdec = GST_MULAWDEC (dec);
  GstStructure *structure;
  int rate, channels;
  GstAudioInfo info;

  structure = gst_caps_get_structure (caps, 0);
  if (!structure) {
    GST_ERROR ("failed to get structure from caps");
    goto error_failed_get_structure;
  }

  if (!gst_structure_get_int (structure, "rate", &rate)) {
    GST_ERROR ("failed to find field rate in input caps");
    goto error_failed_find_rate;
  }

  if (!gst_structure_get_int (structure, "channels", &channels)) {
    GST_ERROR ("failed to find field channels in input caps");
    goto error_failed_find_channel;
  }

  gst_audio_info_init (&info);
  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S16, rate, channels, NULL);

  GST_DEBUG_OBJECT (mulawdec, "rate=%d, channels=%d", rate, channels);

  return gst_audio_decoder_set_output_format (dec, &info);

error_failed_find_channel:
error_failed_find_rate:
error_failed_get_structure:
  return FALSE;
}

static GstFlowReturn
gst_mulawdec_handle_frame (GstAudioDecoder * dec, GstBuffer * buffer)
{
  GstMapInfo inmap, outmap;
  gint16 *linear_data;
  guint8 *mulaw_data;
  gsize mulaw_size, linear_size;
  GstBuffer *outbuf;

  if (!buffer) {
    return GST_FLOW_OK;
  }

  if (!gst_buffer_map (buffer, &inmap, GST_MAP_READ)) {
    GST_ERROR ("failed to map input buffer");
    goto error_failed_map_input_buffer;
  }

  mulaw_data = inmap.data;
  mulaw_size = inmap.size;

  linear_size = mulaw_size * 2;

  outbuf = gst_audio_decoder_allocate_output_buffer (dec, linear_size);
  if (!gst_buffer_map (outbuf, &outmap, GST_MAP_WRITE)) {
    GST_ERROR ("failed to map input buffer");
    goto error_failed_map_output_buffer;
  }

  linear_data = (gint16 *) outmap.data;

  mulaw_decode (mulaw_data, linear_data, mulaw_size);

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
gst_mulawdec_start (GstAudioDecoder * dec)
{
  gst_audio_decoder_set_estimate_rate (dec, TRUE);

  return TRUE;
}

static void
gst_mulawdec_class_init (GstMuLawDecClass * klass)
{
  GstElementClass *element_class = (GstElementClass *) klass;
  GstAudioDecoderClass *audiodec_class = GST_AUDIO_DECODER_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &mulaw_dec_src_factory);
  gst_element_class_add_static_pad_template (element_class,
      &mulaw_dec_sink_factory);


  audiodec_class->start = GST_DEBUG_FUNCPTR (gst_mulawdec_start);
  audiodec_class->set_format = GST_DEBUG_FUNCPTR (gst_mulawdec_set_format);
  audiodec_class->handle_frame = GST_DEBUG_FUNCPTR (gst_mulawdec_handle_frame);

  gst_element_class_set_static_metadata (element_class, "Mu Law audio decoder",
      "Codec/Decoder/Audio",
      "Convert 8bit mu law to 16bit PCM",
      "Zaheer Abbas Merali <zaheerabbas at merali dot org>");
}

static void
gst_mulawdec_init (GstMuLawDec * mulawdec)
{
  gst_audio_decoder_set_needs_format (GST_AUDIO_DECODER (mulawdec), TRUE);
  gst_audio_decoder_set_use_default_pad_acceptcaps (GST_AUDIO_DECODER_CAST
      (mulawdec), TRUE);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_AUDIO_DECODER_SINK_PAD (mulawdec));
}
