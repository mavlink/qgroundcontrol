/* GStreamer
 *
 * Copyright (C) 2015 Samsung Electronics. All rights reserved.
 *   Author: Thiago Santos <thiagoss@osg.samsung.com>
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
/* demo for showing v4l2src renegotiating */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

/* Options */
static const gchar *device = "/dev/video0";
static const gchar *videosink = "autovideosink";
static const gchar *io_mode = "mmap";
static const gchar *def_resolutions[] = {
  "320x240",
  "1280x720",
  "640x480",
  NULL
};

static const gchar **resolutions = def_resolutions;

static GOptionEntry entries[] = {
  {"device", 'd', 0, G_OPTION_ARG_STRING, &device, "V4L2 Camera Device",
      NULL},
  {"videosink", 's', 0, G_OPTION_ARG_STRING, &videosink, "Video Sink to use",
      NULL},
  {"io-mode", 'z', 0, G_OPTION_ARG_STRING, &io_mode,
      "Configure the \"io-mode\" property on v4l2scr", NULL},
  {"resolution", 'r', 0, G_OPTION_ARG_STRING_ARRAY, &resolutions,
      "Add a resolution to the list", NULL},
  {NULL}
};

static GMainLoop *loop;
static GstElement *pipeline;
static GstElement *src, *capsfilter;
static gint resolution_index = 0;

static gboolean
bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  switch (message->type) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:{
      GError *gerror;
      gchar *debug;

      gst_message_parse_error (message, &gerror, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      g_error_free (gerror);
      g_free (debug);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static gboolean
change_caps (gpointer data)
{
  GstCaps *caps;
  GStrv res;
  gchar *caps_str;

  if (!resolutions[resolution_index]) {
    gst_element_send_event (pipeline, gst_event_new_eos ());
    return FALSE;
  }

  g_print ("Setting resolution to '%s'\n", resolutions[resolution_index]);

  res = g_strsplit (resolutions[resolution_index++], "x", 2);
  if (!res[0] || !res[1]) {
    g_warning ("Can't parse resolution: %s", resolutions[resolution_index - 1]);
    g_strfreev (res);
    return TRUE;
  }

  caps_str = g_strdup_printf ("video/x-raw,width=%s,height=%s", res[0], res[1]);
  caps = gst_caps_from_string (caps_str);
  g_object_set (capsfilter, "caps", caps, NULL);

  g_strfreev (res);
  g_free (caps_str);
  gst_caps_unref (caps);

  return TRUE;
}

gint
main (gint argc, gchar ** argv)
{
  GstBus *bus;
  GError *error = NULL;
  GOptionContext *context;
  gchar *desc;
  gboolean ret;

  context = g_option_context_new ("- test v4l2src live renegotition");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gst_init_get_option_group ());
  ret = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);

  if (!ret) {
    g_print ("option parsing failed: %s\n", error->message);
    g_error_free (error);
    return 1;
  }

  loop = g_main_loop_new (NULL, FALSE);

  desc = g_strdup_printf ("v4l2src name=src device=\"%s\" io-mode=\"%s\" "
      "! capsfilter name=cf ! %s", device, io_mode, videosink);
  pipeline = gst_parse_launch (desc, &error);
  g_free (desc);
  if (!pipeline) {
    g_print ("failed to create pipeline: %s", error->message);
    g_error_free (error);
    return 1;
  }

  src = gst_bin_get_by_name (GST_BIN (pipeline), "src");
  capsfilter = gst_bin_get_by_name (GST_BIN (pipeline), "cf");

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_callback, NULL);
  gst_object_unref (bus);

  change_caps (NULL);

  /* play and wait */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  g_timeout_add_seconds (3, change_caps, NULL);

  /* mainloop and wait for eos */
  g_main_loop_run (loop);

  /* stop and cleanup */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (src);
  gst_object_unref (capsfilter);
  gst_object_unref (GST_OBJECT (pipeline));
  g_main_loop_unref (loop);
  return 0;
}
