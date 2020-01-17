/*
 * Copyright (C) 2017 Collabora Inc.
 *    Author: Nicolas Dufresne <nicolas.dufresne@collabora.com>
 * Factored out from gstv4l2mpeg4enc by Philippe Normand <philn@igalia.com>
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

#include "gstv4l2mpeg4codec.h"

#include <gst/gst.h>
#include "ext/v4l2-controls.h"

static gint
v4l2_profile_from_string (const gchar * profile)
{
  gint v4l2_profile = -1;

  if (g_str_equal (profile, "simple"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE;
  else if (g_str_equal (profile, "advanced-simple"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE;
  else if (g_str_equal (profile, "core"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG4_PROFILE_CORE;
  else if (g_str_equal (profile, "simple-scalable"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE_SCALABLE;
  else if (g_str_equal (profile, "advanced-coding-efficiency"))
    v4l2_profile = V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY;
  else
    GST_WARNING ("Unsupported profile string '%s'", profile);

  return v4l2_profile;
}

static const gchar *
v4l2_profile_to_string (gint v4l2_profile)
{
  switch (v4l2_profile) {
    case V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE:
      return "simple";
    case V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE:
      return "advanced-simple";
    case V4L2_MPEG_VIDEO_MPEG4_PROFILE_CORE:
      return "core";
    case V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE_SCALABLE:
      return "simple-scalable";
    case V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY:
      return "advanced-coding-efficiency";
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

  if (g_str_equal (level, "0"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_0;
  else if (g_str_equal (level, "0b"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_0B;
  else if (g_str_equal (level, "1"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_1;
  else if (g_str_equal (level, "2"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_2;
  else if (g_str_equal (level, "3"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_3;
  else if (g_str_equal (level, "3b"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_3B;
  else if (g_str_equal (level, "4"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_4;
  else if (g_str_equal (level, "5"))
    v4l2_level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_5;
  else
    GST_WARNING ("Unsupported level '%s'", level);

  return v4l2_level;
}

static const gchar *
v4l2_level_to_string (gint v4l2_level)
{
  switch (v4l2_level) {
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_0:
      return "0";
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_0B:
      return "0b";
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_1:
      return "1";
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_2:
      return "2";
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_3:
      return "3";
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_3B:
      return "3b";
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_4:
      return "4";
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_5:
      return "5";
    default:
      GST_WARNING ("Unsupported V4L2 level %i", v4l2_level);
      break;
  }

  return NULL;
}

const GstV4l2Codec *
gst_v4l2_mpeg4_get_codec (void)
{
  static GstV4l2Codec *codec = NULL;
  if (g_once_init_enter (&codec)) {
    static GstV4l2Codec c;
    c.profile_cid = V4L2_CID_MPEG_VIDEO_MPEG4_PROFILE;
    c.profile_to_string = v4l2_profile_to_string;
    c.profile_from_string = v4l2_profile_from_string;
    c.level_cid = V4L2_CID_MPEG_VIDEO_MPEG4_LEVEL;
    c.level_to_string = v4l2_level_to_string;
    c.level_from_string = v4l2_level_from_string;
    g_once_init_leave (&codec, &c);
  }
  return codec;
}
