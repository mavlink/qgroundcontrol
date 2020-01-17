/* GStreamer
 * Copyright (C) 2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#include <gst/gst.h>
#include <gst/controller/gstlfocontrolsource.h>
#include <gst/controller/gstdirectcontrolbinding.h>

#include <stdlib.h>

static gboolean
on_message (GstBus * bus, GstMessage * message, gpointer user_data)
{
  GMainLoop *loop = (GMainLoop *) user_data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *debug = NULL;

      g_warning ("Got ERROR");
      gst_message_parse_error (message, &err, &debug);
      g_warning ("%s: %s", err->message, debug);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_WARNING:{
      GError *err = NULL;
      gchar *debug = NULL;

      g_warning ("Got WARNING");
      gst_message_parse_error (message, &err, &debug);
      g_warning ("%s: %s", err->message, debug);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    default:
      break;
  }

  return TRUE;
}

gint
main (gint argc, gchar ** argv)
{
  GstElement *pipeline;
  GstElement *shapewipe;
  GstControlSource *cs;
  GMainLoop *loop;
  GstBus *bus;
  gchar *pipeline_string;
  gfloat border = 0.05;

  if (argc < 2) {
    g_print ("Usage: shapewipe mask.png <border>\n");
    return -1;
  }

  gst_init (&argc, &argv);

  if (argc > 2) {
    border = atof (argv[2]);
  }

  pipeline_string =
      g_strdup_printf
      ("videotestsrc ! video/x-raw,format=(string)AYUV,width=640,height=480 ! shapewipe name=shape border=%f ! videomixer name=mixer ! videoconvert ! autovideosink     filesrc location=%s ! typefind ! decodebin ! videoconvert ! videoscale ! queue ! shape.mask_sink    videotestsrc pattern=snow ! video/x-raw,format=(string)AYUV,width=640,height=480 ! queue ! mixer.",
      border, argv[1]);

  pipeline = gst_parse_launch (pipeline_string, NULL);
  g_free (pipeline_string);

  if (pipeline == NULL) {
    g_print ("Failed to create pipeline\n");
    return -2;
  }

  shapewipe = gst_bin_get_by_name (GST_BIN (pipeline), "shape");

  cs = gst_lfo_control_source_new ();

  gst_object_add_control_binding (GST_OBJECT_CAST (shapewipe),
      gst_direct_control_binding_new (GST_OBJECT_CAST (shapewipe), "position",
          cs));
  gst_object_unref (shapewipe);

  g_object_set (cs,
      "amplitude", 0.5,
      "offset", 0.5, "frequency", 0.25, "timeshift", 500 * GST_MSECOND, NULL);

  g_object_unref (cs);

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (on_message), loop);
  gst_object_unref (GST_OBJECT (bus));

  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_error ("Failed to go into PLAYING state");
    return -4;
  }

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_main_loop_unref (loop);

  gst_object_unref (G_OBJECT (pipeline));

  return 0;
}
