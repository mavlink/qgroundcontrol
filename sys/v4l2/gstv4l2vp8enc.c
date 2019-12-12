/*
 * Copyright (C) 2017 Collabora Inc.
 *    Author: Nicolas Dufresne <nicolas.dufresne@collabora.com>
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
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "gstv4l2object.h"
#include "gstv4l2vp8enc.h"
#include "gstv4l2vp8codec.h"

#include <string.h>
#include <gst/gst-i18n-plugin.h>

GST_DEBUG_CATEGORY_STATIC (gst_v4l2_vp8_enc_debug);
#define GST_CAT_DEFAULT gst_v4l2_vp8_enc_debug

static GstStaticCaps src_template_caps =
GST_STATIC_CAPS ("video/x-vp8, profile=(string) { 0, 1, 2, 3 }");

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS,
  /* TODO */
};

#define gst_v4l2_vp8_enc_parent_class parent_class
G_DEFINE_TYPE (GstV4l2Vp8Enc, gst_v4l2_vp8_enc, GST_TYPE_V4L2_VIDEO_ENC);

static void
gst_v4l2_vp8_enc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  /* TODO */
}

static void
gst_v4l2_vp8_enc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  /* TODO */
}

static void
gst_v4l2_vp8_enc_init (GstV4l2Vp8Enc * self)
{
}

static void
gst_v4l2_vp8_enc_class_init (GstV4l2Vp8EncClass * klass)
{
  GstElementClass *element_class;
  GObjectClass *gobject_class;
  GstV4l2VideoEncClass *baseclass;

  parent_class = g_type_class_peek_parent (klass);

  element_class = (GstElementClass *) klass;
  gobject_class = (GObjectClass *) klass;
  baseclass = (GstV4l2VideoEncClass *) (klass);


  GST_DEBUG_CATEGORY_INIT (gst_v4l2_vp8_enc_debug, "v4l2vp8enc", 0,
      "V4L2 VP8 Encoder");

  gst_element_class_set_static_metadata (element_class,
      "V4L2 VP8 Encoder",
      "Codec/Encoder/Video/Hardware",
      "Encode VP8 video streams via V4L2 API",
      "Nicolas Dufresne <nicolas.dufresne@collabora.com");

  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_v4l2_vp8_enc_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_v4l2_vp8_enc_get_property);

  baseclass->codec_name = "VP8";
}

/* Probing functions */
gboolean
gst_v4l2_is_vp8_enc (GstCaps * sink_caps, GstCaps * src_caps)
{
  return gst_v4l2_is_video_enc (sink_caps, src_caps,
      gst_static_caps_get (&src_template_caps));
}

void
gst_v4l2_vp8_enc_register (GstPlugin * plugin, const gchar * basename,
    const gchar * device_path, gint video_fd, GstCaps * sink_caps,
    GstCaps * src_caps)
{
  const GstV4l2Codec *codec = gst_v4l2_vp8_get_codec ();
  gst_v4l2_video_enc_register (plugin, GST_TYPE_V4L2_VP8_ENC,
      "vp8", basename, device_path, codec, video_fd, sink_caps,
      gst_static_caps_get (&src_template_caps), src_caps);
}
