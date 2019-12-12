/*
 * Copyright (C) 2017 Collabora Inc.
 *    Author: Nicolas Dufresne <nicolas.dufresne@collabora.com>
 * Factored out from gstv4l2vp9enc by Philippe Normand <philn@igalia.com>
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

#include "gstv4l2vp9codec.h"

#include <gst/gst.h>
#include "ext/v4l2-controls.h"


static gint
v4l2_profile_from_string (const gchar * profile)
{
  gint v4l2_profile = -1;

  if (g_str_equal (profile, "0"))
    v4l2_profile = 0;
  else if (g_str_equal (profile, "1"))
    v4l2_profile = 1;
  else if (g_str_equal (profile, "2"))
    v4l2_profile = 2;
  else if (g_str_equal (profile, "3"))
    v4l2_profile = 3;
  else
    GST_WARNING ("Unsupported profile string '%s'", profile);

  return v4l2_profile;
}

static const gchar *
v4l2_profile_to_string (gint v4l2_profile)
{
  switch (v4l2_profile) {
    case 0:
      return "0";
    case 1:
      return "1";
    case 2:
      return "2";
    case 3:
      return "3";
    default:
      GST_WARNING ("Unsupported V4L2 profile %i", v4l2_profile);
      break;
  }

  return NULL;
}

const GstV4l2Codec *
gst_v4l2_vp9_get_codec (void)
{
  static GstV4l2Codec *codec = NULL;
  if (g_once_init_enter (&codec)) {
    static GstV4l2Codec c;
    c.profile_cid = V4L2_CID_MPEG_VIDEO_VPX_PROFILE;
    c.profile_to_string = v4l2_profile_to_string;
    c.profile_from_string = v4l2_profile_from_string;
    g_once_init_leave (&codec, &c);
  }
  return codec;
}
