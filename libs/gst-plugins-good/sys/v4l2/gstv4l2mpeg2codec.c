/*
 * Copyright (C) 2019 Philipp Zabel <philipp.zabel@gmail.com>
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

#include "gstv4l2mpeg2codec.h"

#include <gst/gst.h>
#include "ext/v4l2-controls.h"

static gint
v4l2_profile_from_string (const gchar * profile)
{
  gint v4l2_profile = -1;

  if (g_str_equal (profile, "simple"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG2_PROFILE_SIMPLE;
  else if (g_str_equal (profile, "main"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG2_PROFILE_MAIN;
  else if (g_str_equal (profile, "snr"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG2_PROFILE_SNR_SCALABLE;
  else if (g_str_equal (profile, "spatial"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG2_PROFILE_SPATIALLY_SCALABLE;
  else if (g_str_equal (profile, "high"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG2_PROFILE_HIGH;
  else if (g_str_equal (profile, "multiview"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG2_PROFILE_MULTIVIEW;
  else
    GST_WARNING ("Unsupported profile string '%s'", profile);

  return v4l2_profile;
}

static const gchar *
v4l2_profile_to_string (gint v4l2_profile)
{
  switch (v4l2_profile) {
    case V4L2_MPEG_VIDEO_MPEG2_PROFILE_SIMPLE:
      return "simple";
    case V4L2_MPEG_VIDEO_MPEG2_PROFILE_MAIN:
      return "main";
    case V4L2_MPEG_VIDEO_MPEG2_PROFILE_SNR_SCALABLE:
      return "snr";
    case V4L2_MPEG_VIDEO_MPEG2_PROFILE_SPATIALLY_SCALABLE:
      return "spatial";
    case V4L2_MPEG_VIDEO_MPEG2_PROFILE_HIGH:
      return "high";
    case V4L2_MPEG_VIDEO_MPEG2_PROFILE_MULTIVIEW:
      return "multiview";
    default:
      GST_WARNING ("Unsupported V4L2 profile %i", v4l2_profile);
      break;
  }

  return NULL;
}

static gint
v4l2_level_from_string (const gchar * level)
{
  gint v4l2_level = -1;

  if (g_str_equal (level, "low"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG2_LEVEL_LOW;
  else if (g_str_equal (level, "main"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG2_LEVEL_MAIN;
  else if (g_str_equal (level, "high-1440"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG2_LEVEL_HIGH_1440;
  else if (g_str_equal (level, "high"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG2_LEVEL_HIGH;
  else
    GST_WARNING ("Unsupported level '%s'", level);

  return v4l2_level;
}

static const gchar *
v4l2_level_to_string (gint v4l2_level)
{
  switch (v4l2_level) {
    case V4L2_MPEG_VIDEO_MPEG2_LEVEL_LOW:
      return "low";
    case V4L2_MPEG_VIDEO_MPEG2_LEVEL_MAIN:
      return "main";
    case V4L2_MPEG_VIDEO_MPEG2_LEVEL_HIGH_1440:
      return "high-1440";
    case V4L2_MPEG_VIDEO_MPEG2_LEVEL_HIGH:
      return "high";
    default:
      GST_WARNING ("Unsupported V4L2 level %i", v4l2_level);
      break;
  }

  return NULL;
}

const GstV4l2Codec *
gst_v4l2_mpeg2_get_codec (void)
{
  static GstV4l2Codec *codec = NULL;
  if (g_once_init_enter (&codec)) {
    static GstV4l2Codec c;
    c.profile_cid = V4L2_CID_MPEG_VIDEO_MPEG2_PROFILE;
    c.profile_to_string = v4l2_profile_to_string;
    c.profile_from_string = v4l2_profile_from_string;
    c.level_cid = V4L2_CID_MPEG_VIDEO_MPEG2_LEVEL;
    c.level_to_string = v4l2_level_to_string;
    c.level_from_string = v4l2_level_from_string;
    g_once_init_leave (&codec, &c);
  }
  return codec;
}
