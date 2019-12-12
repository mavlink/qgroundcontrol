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

#include "gstv4l2h264codec.h"

#include <gst/gst.h>
#include "ext/v4l2-controls.h"


static gint
v4l2_profile_from_string (const gchar * profile)
{
  gint v4l2_profile = -1;

  if (g_str_equal (profile, "baseline")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE;
  } else if (g_str_equal (profile, "constrained-baseline")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE;
  } else if (g_str_equal (profile, "main")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_MAIN;
  } else if (g_str_equal (profile, "extended")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_EXTENDED;
  } else if (g_str_equal (profile, "high")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH;
  } else if (g_str_equal (profile, "high-10")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10;
  } else if (g_str_equal (profile, "high-4:2:2")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422;
  } else if (g_str_equal (profile, "high-4:4:4")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE;
  } else if (g_str_equal (profile, "high-10-intra")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10_INTRA;
  } else if (g_str_equal (profile, "high-4:2:2-intra")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422_INTRA;
  } else if (g_str_equal (profile, "high-4:4:4-intra")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_INTRA;
  } else if (g_str_equal (profile, "cavlc-4:4:4-intra")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_CAVLC_444_INTRA;
  } else if (g_str_equal (profile, "scalable-baseline")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_BASELINE;
  } else if (g_str_equal (profile, "scalable-high")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH;
  } else if (g_str_equal (profile, "scalable-high-intra")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH_INTRA;
  } else if (g_str_equal (profile, "stereo-high")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_STEREO_HIGH;
  } else if (g_str_equal (profile, "multiview-high")) {
    v4l2_profile = V4L2_MPEG_VIDEO_H264_PROFILE_MULTIVIEW_HIGH;
  } else {
    GST_WARNING ("Unsupported profile string '%s'", profile);
  }

  return v4l2_profile;
}

static const gchar *
v4l2_profile_to_string (gint v4l2_profile)
{
  switch (v4l2_profile) {
    case V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE:
      return "baseline";
    case V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE:
      return "constrained-baseline";
    case V4L2_MPEG_VIDEO_H264_PROFILE_MAIN:
      return "main";
    case V4L2_MPEG_VIDEO_H264_PROFILE_EXTENDED:
      return "extended";
    case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH:
      return "high";
    case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10:
      return "high-10";
    case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422:
      return "high-4:2:2";
    case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE:
      return "high-4:4:4";
    case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10_INTRA:
      return "high-10-intra";
    case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422_INTRA:
      return "high-4:2:2-intra";
    case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_INTRA:
      return "high-4:4:4-intra";
    case V4L2_MPEG_VIDEO_H264_PROFILE_CAVLC_444_INTRA:
      return "cavlc-4:4:4-intra";
    case V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_BASELINE:
      return "scalable-baseline";
    case V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH:
      return "scalable-high";
    case V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH_INTRA:
      return "scalable-high-intra";
    case V4L2_MPEG_VIDEO_H264_PROFILE_STEREO_HIGH:
      return "stereo-high";
    case V4L2_MPEG_VIDEO_H264_PROFILE_MULTIVIEW_HIGH:
      return "multiview-high";
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

  if (g_str_equal (level, "1"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_1_0;
  else if (g_str_equal (level, "1b"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_1B;
  else if (g_str_equal (level, "1.1"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_1_1;
  else if (g_str_equal (level, "1.2"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_1_2;
  else if (g_str_equal (level, "1.3"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_1_3;
  else if (g_str_equal (level, "2"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_2_0;
  else if (g_str_equal (level, "2.1"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_2_1;
  else if (g_str_equal (level, "2.2"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_2_2;
  else if (g_str_equal (level, "3"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_3_0;
  else if (g_str_equal (level, "3.1"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_3_1;
  else if (g_str_equal (level, "3.2"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_3_2;
  else if (g_str_equal (level, "4"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_4_0;
  else if (g_str_equal (level, "4.1"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_4_1;
  else if (g_str_equal (level, "4.2"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_4_2;
  else if (g_str_equal (level, "5"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_5_0;
  else if (g_str_equal (level, "5.1"))
    v4l2_level = V4L2_MPEG_VIDEO_H264_LEVEL_5_1;
  else
    GST_WARNING ("Unsupported level '%s'", level);

  return v4l2_level;
}

static const gchar *
v4l2_level_to_string (gint v4l2_level)
{
  switch (v4l2_level) {
    case V4L2_MPEG_VIDEO_H264_LEVEL_1_0:
      return "1";
    case V4L2_MPEG_VIDEO_H264_LEVEL_1B:
      return "1b";
    case V4L2_MPEG_VIDEO_H264_LEVEL_1_1:
      return "1.1";
    case V4L2_MPEG_VIDEO_H264_LEVEL_1_2:
      return "1.2";
    case V4L2_MPEG_VIDEO_H264_LEVEL_1_3:
      return "1.3";
    case V4L2_MPEG_VIDEO_H264_LEVEL_2_0:
      return "2";
    case V4L2_MPEG_VIDEO_H264_LEVEL_2_1:
      return "2.1";
    case V4L2_MPEG_VIDEO_H264_LEVEL_2_2:
      return "2.2";
    case V4L2_MPEG_VIDEO_H264_LEVEL_3_0:
      return "3";
    case V4L2_MPEG_VIDEO_H264_LEVEL_3_1:
      return "3.1";
    case V4L2_MPEG_VIDEO_H264_LEVEL_3_2:
      return "3.2";
    case V4L2_MPEG_VIDEO_H264_LEVEL_4_0:
      return "4";
    case V4L2_MPEG_VIDEO_H264_LEVEL_4_1:
      return "4.1";
    case V4L2_MPEG_VIDEO_H264_LEVEL_4_2:
      return "4.2";
    case V4L2_MPEG_VIDEO_H264_LEVEL_5_0:
      return "5";
    case V4L2_MPEG_VIDEO_H264_LEVEL_5_1:
      return "5.1";
    default:
      GST_WARNING ("Unsupported V4L2 level %i", v4l2_level);
      break;
  }

  return NULL;
}

const GstV4l2Codec *
gst_v4l2_h264_get_codec (void)
{
  static GstV4l2Codec *codec = NULL;
  if (g_once_init_enter (&codec)) {
    static GstV4l2Codec c;
    c.profile_cid = V4L2_CID_MPEG_VIDEO_H264_PROFILE;
    c.profile_to_string = v4l2_profile_to_string;
    c.profile_from_string = v4l2_profile_from_string;
    c.level_cid = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
    c.level_to_string = v4l2_level_to_string;
    c.level_from_string = v4l2_level_from_string;
    g_once_init_leave (&codec, &c);
  }
  return codec;
}
