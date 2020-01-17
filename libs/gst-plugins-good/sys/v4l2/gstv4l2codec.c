/*
 * Copyright (C) 2019 Igalia S.L.
 *    Author: Philippe Normand <philn@igalia.com>
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

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "gstv4l2codec.h"
#include "ext/videodev2.h"

#include <gst/gst.h>

static GValue *
probe_controls (gint video_fd, guint32 cid,
    const gchar * (transform_control) (gint))
{
  GValue *controls = NULL;
  struct v4l2_queryctrl query_ctrl;

  memset (&query_ctrl, 0, sizeof (query_ctrl));
  query_ctrl.id = cid;

  if (ioctl (video_fd, VIDIOC_QUERYCTRL, &query_ctrl) == 0) {
    if (query_ctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
      return NULL;
    }

    if (query_ctrl.type == V4L2_CTRL_TYPE_MENU) {
      struct v4l2_querymenu query_menu;

      memset (&query_menu, 0, sizeof (query_menu));
      query_menu.id = query_ctrl.id;

      controls = g_new0 (GValue, 1);
      g_value_init (controls, GST_TYPE_LIST);
      for (query_menu.index = query_ctrl.minimum;
          query_menu.index <= query_ctrl.maximum; query_menu.index++) {
        if (ioctl (video_fd, VIDIOC_QUERYMENU, &query_menu) >= 0) {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, transform_control (query_menu.index));
          gst_value_list_append_and_take_value (controls, &value);
        }
      }
      if (gst_value_list_get_size (controls) == 0) {
        g_value_unset (controls);
        controls = NULL;
      }
    }
  }

  return controls;
}

GValue *
gst_v4l2_codec_probe_profiles (const GstV4l2Codec * codec, gint video_fd)
{
  return probe_controls (video_fd, codec->profile_cid,
      codec->profile_to_string);
}

GValue *
gst_v4l2_codec_probe_levels (const GstV4l2Codec * codec, gint video_fd)
{
  GValue *controls = NULL;
  struct v4l2_queryctrl query_ctrl;

  memset (&query_ctrl, 0, sizeof (query_ctrl));
  query_ctrl.id = codec->level_cid;

  if (ioctl (video_fd, VIDIOC_QUERYCTRL, &query_ctrl) == 0) {
    if (query_ctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
      return NULL;
    }

    if (query_ctrl.type == V4L2_CTRL_TYPE_MENU) {
      struct v4l2_querymenu query_menu;

      memset (&query_menu, 0, sizeof (query_menu));
      query_menu.id = query_ctrl.id;
      query_menu.index = query_ctrl.maximum;

      if (ioctl (video_fd, VIDIOC_QUERYMENU, &query_menu) >= 0) {
        gint32 i;

        controls = g_new0 (GValue, 1);
        g_value_init (controls, GST_TYPE_LIST);

        /* Assume that all levels below the highest one reported by the driver are supported. */
        for (i = query_ctrl.minimum; i <= query_ctrl.maximum; i++) {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, codec->level_to_string (i));
          gst_value_list_append_and_take_value (controls, &value);
        }
      }
    }
  }

  return controls;
}
