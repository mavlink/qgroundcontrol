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
#include "gstv4l2h263enc.h"

#include <string.h>
#include <gst/gst-i18n-plugin.h>

GST_DEBUG_CATEGORY_STATIC (gst_v4l2_h263_enc_debug);
#define GST_CAT_DEFAULT gst_v4l2_h263_enc_debug


static GstStaticCaps src_template_caps =
GST_STATIC_CAPS ("video/x-h263, variant=(string) itu");

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS,
/* TODO add H263 controls */
};

#define gst_v4l2_h263_enc_parent_class parent_class
G_DEFINE_TYPE (GstV4l2H263Enc, gst_v4l2_h263_enc, GST_TYPE_V4L2_VIDEO_ENC);

static void
gst_v4l2_h263_enc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  /* TODO */
}

static void
gst_v4l2_h263_enc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  /* TODO */
}

static void
gst_v4l2_h263_enc_init (GstV4l2H263Enc * self)
{

}

static void
gst_v4l2_h263_enc_class_init (GstV4l2H263EncClass * klass)
{
  GstElementClass *element_class;
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  element_class = (GstElementClass *) klass;
  gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_v4l2_h263_enc_debug, "v4l2h263enc", 0,
      "V4L2 H.263 Encoder");

  gst_element_class_set_static_metadata (element_class,
      "V4L2 H.263 Encoder",
      "Codec/Encoder/Video/Hardware",
      "Encode H.263 video streams via V4L2 API",
      "Nicolas Dufresne <nicolas.dufresne@collabora.com>");

  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_v4l2_h263_enc_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_v4l2_h263_enc_get_property);
}

/* Probing functions */
gboolean
gst_v4l2_is_h263_enc (GstCaps * sink_caps, GstCaps * src_caps)
{
  return gst_v4l2_is_video_enc (sink_caps, src_caps,
      gst_static_caps_get (&src_template_caps));
}

void
gst_v4l2_h263_enc_register (GstPlugin * plugin, const gchar * basename,
    const gchar * device_path, GstCaps * sink_caps, GstCaps * src_caps)
{
  gst_v4l2_video_enc_register (plugin, GST_TYPE_V4L2_H263_ENC,
      "h263", basename, device_path, NULL, -1, sink_caps,
      gst_static_caps_get (&src_template_caps), src_caps);
}
