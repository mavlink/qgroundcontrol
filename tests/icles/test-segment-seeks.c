/* GStreamer interactive test for accurate segment seeking
 * Copyright (C) 2014 Tim-Philipp Müller <tim centricular com>
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

/* Plays the provided file one second at a time using segment seeks.
 * Theoretically this should be just as smooth as if we played the
 * file from start to stop in one go, certainly without hickups.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gst/gst.h>

#define SEGMENT_DURATION (1 * GST_SECOND)

int
main (int argc, char **argv)
{
  GstElement *playbin;
  GstMessage *msg;
  gchar *uri;
  gint64 dur, start, stop;
  gboolean prerolled = FALSE;

  if (argc < 2) {
    g_printerr ("Usage: %s FILENAME\n", argv[0]);
    return -1;
  }

  gst_init (&argc, &argv);

  if (gst_uri_is_valid (argv[1]))
    uri = g_strdup (argv[1]);
  else
    uri = gst_filename_to_uri (argv[1], NULL);

  g_print ("uri: %s\n", uri);

  playbin = gst_element_factory_make ("playbin", NULL);
  g_object_set (playbin, "uri", uri, NULL);

#if 0
  {
    GstElement *src;

    playbin = gst_parse_launch ("uridecodebin name=d ! queue ! "
        "filesink location=/tmp/raw1.data", NULL);
    src = gst_bin_get_by_name (GST_BIN (playbin), "d");
    g_object_set (src, "uri", uri, NULL);
    gst_object_unref (src);
  }
#endif

  gst_element_set_state (playbin, GST_STATE_PAUSED);

  /* wait for preroll */
  msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (playbin),
      GST_CLOCK_TIME_NONE, GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_ERROR);

  g_assert (GST_MESSAGE_TYPE (msg) != GST_MESSAGE_ERROR);
  prerolled = TRUE;

  gst_message_unref (msg);

  if (!gst_element_query_duration (playbin, GST_FORMAT_TIME, &dur))
    g_error ("Failed to query duration!\n");

  g_print ("Duration: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS (dur));

  start = 0;
  do {
    GstSeekFlags seek_flags;
    gboolean ret;
    gboolean segment_done = FALSE;

    seek_flags = GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_SEGMENT;

    if (start == 0) {
      prerolled = FALSE;
      seek_flags |= GST_SEEK_FLAG_FLUSH;
    }

    stop = start + SEGMENT_DURATION;

    g_print ("Segment: %" GST_TIME_FORMAT "-%" GST_TIME_FORMAT "\n",
        GST_TIME_ARGS (start), GST_TIME_ARGS (stop));

    ret = gst_element_seek (playbin, 1.0, GST_FORMAT_TIME, seek_flags,
        GST_SEEK_TYPE_SET, start, GST_SEEK_TYPE_SET, stop);

    g_assert (ret);

    if (!prerolled) {
      while (!prerolled) {
        /* wait for preroll again */
        msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (playbin),
            GST_CLOCK_TIME_NONE, GST_MESSAGE_SEGMENT_DONE |
            GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_ERROR);

        g_assert (GST_MESSAGE_TYPE (msg) != GST_MESSAGE_ERROR);
        if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_SEGMENT_DONE) {
          segment_done = TRUE;
        } else {
          prerolled = TRUE;
        }
        gst_message_unref (msg);
      }

      gst_element_set_state (playbin, GST_STATE_PLAYING);
    }

    /* wait for end of segment if we didn't get it above already */
    if (!segment_done) {
      msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (playbin),
          GST_CLOCK_TIME_NONE, GST_MESSAGE_SEGMENT_DONE | GST_MESSAGE_ERROR);

      g_assert (GST_MESSAGE_TYPE (msg) != GST_MESSAGE_ERROR);

      gst_message_unref (msg);
    }

    start = stop;
  }
  while (start < dur);

  gst_element_set_state (playbin, GST_STATE_NULL);
  gst_object_unref (playbin);
  return 0;
}
