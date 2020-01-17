/* GStreamer interactive videobox test
 *
 * Copyright (C) 2008 Wim Taymans <wim.taymans@gmail.com>
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

#include <stdlib.h>

#include <gst/gst.h>

static GstElement *
make_pipeline (gint type)
{
  GstElement *result;
  gchar *pstr;

  switch (type) {
    case 0:
      pstr = g_strdup_printf ("videotestsrc ! videobox name=box ! "
          "xvimagesink");
      break;
    default:
      return NULL;
  }

  result = gst_parse_launch_full (pstr, NULL, GST_PARSE_FLAG_NONE, NULL);
  g_print ("created test %d: \"%s\"\n", type, pstr);
  g_free (pstr);

  return result;
}

#define MAX_ROUND 500

int
main (int argc, char **argv)
{
  GstElement *pipe, *box;
  gint left, right;
  gint top, bottom;
  gint ldir, rdir;
  gint tdir, bdir;
  gint round, type, stop;

  gst_init (&argc, &argv);

  type = 0;
  stop = -1;

  if (argc > 1) {
    type = atoi (argv[1]);
    stop = type + 1;
  }

  while (TRUE) {
    GstMessage *message;

    pipe = make_pipeline (type);
    if (pipe == NULL)
      break;

    box = gst_bin_get_by_name (GST_BIN (pipe), "box");
    g_assert (box);

    top = bottom = left = right = 0;
    tdir = bdir = -10;
    ldir = rdir = 10;

    for (round = 0; round < MAX_ROUND; round++) {
      g_print ("box to %4d %4d %4d %4d (%d/%d)   \r", top, bottom, left, right,
          round, MAX_ROUND);

      g_object_set (box, "top", top, "bottom", bottom, "left", left, "right",
          right, NULL);

      if (round == 0)
        gst_element_set_state (pipe, GST_STATE_PLAYING);

      top += tdir;
      if (top >= 50)
        tdir = -10;
      else if (top < -50)
        tdir = 10;

      bottom += bdir;
      if (bottom >= 40)
        bdir = -10;
      else if (bottom < -60)
        bdir = 10;

      left += ldir;
      if (left >= 60)
        ldir = -10;
      else if (left < -80)
        ldir = 10;

      right += rdir;
      if (right >= 80)
        rdir = -10;
      else if (right < -90)
        rdir = 10;

      message =
          gst_bus_poll (GST_ELEMENT_BUS (pipe), GST_MESSAGE_ERROR,
          50 * GST_MSECOND);
      if (message) {
        g_print ("got error           \n");

        gst_message_unref (message);
      }
    }
    g_print ("test %d done                    \n", type);

    gst_object_unref (box);
    gst_element_set_state (pipe, GST_STATE_NULL);
    gst_object_unref (pipe);

    type++;
    if (type == stop)
      break;
  }
  return 0;
}
