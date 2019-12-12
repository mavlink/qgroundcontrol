/* GStreamer
 * Copyright (C) 2015 Sebastian Dröge <sebastian@centricular.com>
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

#include "gstrtputils.h"

typedef struct
{
  GstElement *element;
  GstBuffer *outbuf;
  GQuark copy_tag;
} CopyMetaData;

GQuark rtp_quark_meta_tag_video;
GQuark rtp_quark_meta_tag_audio;

static gboolean
foreach_metadata_copy (GstBuffer * inbuf, GstMeta ** meta, gpointer user_data)
{
  CopyMetaData *data = user_data;
  GstElement *element = data->element;
  GstBuffer *outbuf = data->outbuf;
  GQuark copy_tag = data->copy_tag;
  const GstMetaInfo *info = (*meta)->info;
  const gchar *const *tags = gst_meta_api_type_get_tags (info->api);

  if (!tags || (copy_tag != 0 && g_strv_length ((gchar **) tags) == 1
          && gst_meta_api_type_has_tag (info->api, copy_tag))) {
    GstMetaTransformCopy copy_data = { FALSE, 0, -1 };
    GST_DEBUG_OBJECT (element, "copy metadata %s", g_type_name (info->api));
    /* simply copy then */
    info->transform_func (outbuf, *meta, inbuf,
        _gst_meta_transform_copy, &copy_data);
  } else {
    GST_DEBUG_OBJECT (element, "not copying metadata %s",
        g_type_name (info->api));
  }

  return TRUE;
}

/* TODO: Should probably make copy_tag an array at some point */
void
gst_rtp_copy_meta (GstElement * element, GstBuffer * outbuf, GstBuffer * inbuf,
    GQuark copy_tag)
{
  CopyMetaData data = { element, outbuf, copy_tag };

  gst_buffer_foreach_meta (inbuf, foreach_metadata_copy, &data);
}

void
gst_rtp_copy_video_meta (gpointer element, GstBuffer * outbuf,
    GstBuffer * inbuf)
{
  gst_rtp_copy_meta (element, outbuf, inbuf, rtp_quark_meta_tag_video);
}

void
gst_rtp_copy_audio_meta (gpointer element, GstBuffer * outbuf,
    GstBuffer * inbuf)
{
  gst_rtp_copy_meta (element, outbuf, inbuf, rtp_quark_meta_tag_audio);
}

typedef struct
{
  GstElement *element;
  GQuark keep_tag;
} DropMetaData;

static gboolean
foreach_metadata_drop (GstBuffer * inbuf, GstMeta ** meta, gpointer user_data)
{
  DropMetaData *data = user_data;
  GstElement *element = data->element;
  GQuark keep_tag = data->keep_tag;
  const GstMetaInfo *info = (*meta)->info;
  const gchar *const *tags = gst_meta_api_type_get_tags (info->api);

  if (!tags || (keep_tag != 0 && g_strv_length ((gchar **) tags) == 1
          && gst_meta_api_type_has_tag (info->api, keep_tag))) {
    GST_DEBUG_OBJECT (element, "keeping metadata %s", g_type_name (info->api));
  } else {
    GST_DEBUG_OBJECT (element, "dropping metadata %s", g_type_name (info->api));
    *meta = NULL;
  }

  return TRUE;
}

/* TODO: Should probably make keep_tag an array at some point */
void
gst_rtp_drop_meta (GstElement * element, GstBuffer * buf, GQuark keep_tag)
{
  DropMetaData data = { element, keep_tag };

  gst_buffer_foreach_meta (buf, foreach_metadata_drop, &data);
}

void
gst_rtp_drop_non_audio_meta (gpointer element, GstBuffer * buf)
{
  gst_rtp_drop_meta (element, buf, rtp_quark_meta_tag_audio);
}

void
gst_rtp_drop_non_video_meta (gpointer element, GstBuffer * buf)
{
  gst_rtp_drop_meta (element, buf, rtp_quark_meta_tag_video);
}

/* Stolen from bad/gst/mpegtsdemux/payloader_parsers.c */
/* variable length Exp-Golomb parsing according to H.265 spec section 9.2*/
gboolean
gst_rtp_read_golomb (GstBitReader * br, guint32 * value)
{
  guint8 b, leading_zeros = -1;
  *value = 1;

  for (b = 0; !b; leading_zeros++) {
    if (!gst_bit_reader_get_bits_uint8 (br, &b, 1))
      return FALSE;
    *value *= 2;
  }

  *value = (*value >> 1) - 1;
  if (leading_zeros > 0) {
    guint32 tmp = 0;
    if (!gst_bit_reader_get_bits_uint32 (br, &tmp, leading_zeros))
      return FALSE;
    *value += tmp;
  }

  return TRUE;
}
