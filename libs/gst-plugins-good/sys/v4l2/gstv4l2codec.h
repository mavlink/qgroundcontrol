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

#ifndef __GST_V4L2_CODEC_H__
#define __GST_V4L2_CODEC_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GstV4l2Codec GstV4l2Codec;

struct _GstV4l2Codec {
  guint32 profile_cid;
  const gchar * (*profile_to_string) (gint v4l2_profile);
  gint (*profile_from_string) (const gchar * profile);

  guint32 level_cid;
  const gchar * (*level_to_string) (gint v4l2_level);
  gint (*level_from_string) (const gchar * level);

};

GValue * gst_v4l2_codec_probe_profiles(const GstV4l2Codec * codec, gint video_fd);
GValue * gst_v4l2_codec_probe_levels(const GstV4l2Codec * codec, gint video_fd);

G_END_DECLS

#endif /* __GST_V4L2_CODEC_H__ */
