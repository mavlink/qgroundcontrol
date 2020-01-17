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
 * SECTION:element-mulawenc
 * @title: mulawenc
 *
 * This element encode mulaw audio. Mulaw coding is also known as G.711.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gst/gst.h>
#include <gst/audio/audio.h>

#include "mulaw-encode.h"
#include "mulaw-conversion.h"

extern GstStaticPadTemplate mulaw_enc_src_factory;
extern GstStaticPadTemplate mulaw_enc_sink_factory;

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

static gboolean gst_mulawenc_start (GstAudioEncoder * audioenc);
static gboolean gst_mulawenc_set_format (GstAudioEncoder * enc,
    GstAudioInfo * info);
static GstFlowReturn gst_mulawenc_handle_frame (GstAudioEncoder * enc,
    GstBuffer * buffer);
static void gst_mulawenc_set_tags (GstMuLawEnc * mulawenc);


#define gst_mulawenc_parent_class parent_class
G_DEFINE_TYPE (GstMuLawEnc, gst_mulawenc, GST_TYPE_AUDIO_ENCODER);

/*static guint gst_stereo_signals[LAST_SIGNAL] = { 0 }; */

static gboolean
gst_mulawenc_start (GstAudioEncoder * audioenc)
{
  GstMuLawEnc *mulawenc = GST_MULAWENC (audioenc);

  mulawenc->channels = 0;
  mulawenc->rate = 0;

  return TRUE;
}


static void
gst_mulawenc_set_tags (GstMuLawEnc * mulawenc)
{
  GstTagList *taglist;
  guint bitrate;

  /* bitrate of mulaw is 8 bits/sample * sample rate * number of channels */
  bitrate = 8 * mulawenc->rate * mulawenc->channels;

  taglist = gst_tag_list_new_empty ();
  gst_tag_list_add (taglist, GST_TAG_MERGE_REPLACE,
      GST_TAG_MAXIMUM_BITRATE, bitrate, NULL);
  gst_tag_list_add (taglist, GST_TAG_MERGE_REPLACE,
      GST_TAG_MINIMUM_BITRATE, bitrate, NULL);
  gst_tag_list_add (taglist, GST_TAG_MERGE_REPLACE,
      GST_TAG_BITRATE, bitrate, NULL);

  gst_audio_encoder_merge_tags (GST_AUDIO_ENCODER (mulawenc),
      taglist, GST_TAG_MERGE_REPLACE);

  gst_tag_list_unref (taglist);
}


static gboolean
gst_mulawenc_set_format (GstAudioEncoder * audioenc, GstAudioInfo * info)
{
  GstCaps *base_caps;
  GstStructure *structure;
  GstMuLawEnc *mulawenc = GST_MULAWENC (audioenc);
  gboolean ret;

  mulawenc->rate = info->rate;
  mulawenc->channels = info->channels;

  base_caps =
      gst_pad_get_pad_template_caps (GST_AUDIO_ENCODER_SRC_PAD (audioenc));
  g_assert (base_caps);
  base_caps = gst_caps_make_writable (base_caps);
  g_assert (base_caps);

  structure = gst_caps_get_structure (base_caps, 0);
  g_assert (structure);
  gst_structure_set (structure, "rate", G_TYPE_INT, mulawenc->rate, NULL);
  gst_structure_set (structure, "channels", G_TYPE_INT, mulawenc->channels,
      NULL);

  gst_mulawenc_set_tags (mulawenc);

  ret = gst_audio_encoder_set_output_format (audioenc, base_caps);
  gst_caps_unref (base_caps);

  return ret;
}

static GstFlowReturn
gst_mulawenc_handle_frame (GstAudioEncoder * audioenc, GstBuffer * buffer)
{
  GstMuLawEnc *mulawenc;
  GstMapInfo inmap, outmap;
  gint16 *linear_data;
  gsize linear_size;
  guint8 *mulaw_data;
  guint mulaw_size;
  GstBuffer *outbuf;
  GstFlowReturn ret;

  if (!buffer) {
    ret = GST_FLOW_OK;
    goto done;
  }

  mulawenc = GST_MULAWENC (audioenc);

  if (!mulawenc->rate || !mulawenc->channels)
    goto not_negotiated;

  gst_buffer_map (buffer, &inmap, GST_MAP_READ);
  linear_data = (gint16 *) inmap.data;
  linear_size = inmap.size;

  mulaw_size = linear_size / 2;

  outbuf = gst_audio_encoder_allocate_output_buffer (audioenc, mulaw_size);

  g_assert (outbuf);

  gst_buffer_map (outbuf, &outmap, GST_MAP_WRITE);
  mulaw_data = outmap.data;

  mulaw_encode (linear_data, mulaw_data, mulaw_size);

  gst_buffer_unmap (outbuf, &outmap);
  gst_buffer_unmap (buffer, &inmap);

  ret = gst_audio_encoder_finish_frame (audioenc, outbuf, -1);

done:

  return ret;

not_negotiated:
  {
    GST_DEBUG_OBJECT (mulawenc, "no format negotiated");
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto done;
  }
}



static void
gst_mulawenc_class_init (GstMuLawEncClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstAudioEncoderClass *audio_encoder_class = GST_AUDIO_ENCODER_CLASS (klass);

  audio_encoder_class->start = GST_DEBUG_FUNCPTR (gst_mulawenc_start);
  audio_encoder_class->set_format = GST_DEBUG_FUNCPTR (gst_mulawenc_set_format);
  audio_encoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_mulawenc_handle_frame);

  gst_element_class_add_static_pad_template (element_class,
      &mulaw_enc_src_factory);
  gst_element_class_add_static_pad_template (element_class,
      &mulaw_enc_sink_factory);

  gst_element_class_set_static_metadata (element_class, "Mu Law audio encoder",
      "Codec/Encoder/Audio",
      "Convert 16bit PCM to 8bit mu law",
      "Zaheer Abbas Merali <zaheerabbas at merali dot org>");
}

static void
gst_mulawenc_init (GstMuLawEnc * mulawenc)
{
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_AUDIO_ENCODER_SINK_PAD (mulawenc));
}
