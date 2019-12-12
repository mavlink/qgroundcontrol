/* GStreamer
 * Copyright (C) <2006> Zaheer Abbas Merali <zaheerabbas at merali dot org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

static GMainLoop *loop;

static gboolean
terminate_playback (GstElement * pipeline)
{
  g_print ("Terminating playback\n");
  g_main_loop_quit (loop);
  return FALSE;
}

int
main (int argc, char **argv)
{
  GstElement *pipeline;
  GstState state;
  GError *error = NULL;

  gst_init (&argc, &argv);

  pipeline = gst_parse_launch ("ximagesrc ! fakesink", &error);
  if (error) {
    g_print ("Error while parsing pipeline description: %s\n", error->message);
    return -1;
  }

  loop = g_main_loop_new (NULL, FALSE);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* lets check it gets to PLAYING */
  if (gst_element_get_state (pipeline, &state, NULL,
          GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_FAILURE ||
      state != GST_STATE_PLAYING) {
    g_warning ("State change to playing failed");
  }

  /* We want to get out after 5 seconds */
  g_timeout_add_seconds (5, (GSourceFunc) terminate_playback, pipeline);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);

  return 0;
}
