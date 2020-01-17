/* GStreamer interactive test for the videocrop element
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
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
# include "config.h"
#endif

#include <gst/gst.h>

#include <stdlib.h>
#include <math.h>

GST_DEBUG_CATEGORY_STATIC (videocrop_test_debug);
#define GST_CAT_DEFAULT videocrop_test_debug

#define OUT_WIDTH      640
#define OUT_HEIGHT     480
#define TIME_PER_TEST   10      /* seconds each format is tested */
#define FRAMERATE       15      /* frames per second             */

#ifndef DEFAULT_VIDEOSINK
#define DEFAULT_VIDEOSINK "xvimagesink"
#endif

static gboolean
check_bus_for_errors (GstBus * bus, GstClockTime max_wait_time)
{
  GstMessage *msg;

  msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, max_wait_time);

  if (msg) {
    GError *err = NULL;
    gchar *debug = NULL;

    g_assert (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR);
    gst_message_parse_error (msg, &err, &debug);
    GST_ERROR ("ERROR: %s [%s]", err->message, debug);
    g_print ("\n===========> ERROR: %s\n%s\n\n", err->message, debug);
    g_clear_error (&err);
    g_free (debug);
    gst_message_unref (msg);
  }

  return (msg != NULL);
}

static void
test_with_caps (GstElement * src, GstElement * videocrop, GstCaps * caps)
{
  GstClockTime time_run;
  GstElement *pipeline;
  GTimer *timer;
  GstBus *bus;
  GstPad *pad;
  guint hcrop;
  guint vcrop;

  /* caps must be writable, we can't check that here though */
  g_assert (GST_CAPS_REFCOUNT_VALUE (caps) == 1);

  timer = g_timer_new ();
  vcrop = 0;
  hcrop = 0;

  pipeline = GST_ELEMENT (gst_element_get_parent (videocrop));
  g_assert (GST_IS_PIPELINE (pipeline));

  /* at this point the pipeline is in PLAYING state; we only want to capture
   * errors resulting from our on-the-fly changing of the filtercaps */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

  /* pad to block */
  pad = gst_element_get_static_pad (src, "src");

  time_run = 0;
  do {
    GstClockTime wait_time, waited_for_block;

    if (check_bus_for_errors (bus, 0))
      break;

    wait_time = GST_SECOND / FRAMERATE;

    GST_LOG ("hcrop = %3d, vcrop = %3d", vcrop, hcrop);

    g_timer_reset (timer);

    /* need to block the streaming thread while changing these properties,
     * otherwise we might get random not-negotiated errors (when caps are
     * changed in between upstream calling pad_alloc_buffer() and pushing
     * the processed buffer?)  FIXME should not be needed */
    /* gst_pad_set_blocked (pad, TRUE); */
    g_object_set (videocrop, "left", hcrop, "top", vcrop, NULL);
    /* gst_pad_set_blocked (pad, FALSE); */

    waited_for_block = g_timer_elapsed (timer, NULL) * (double) GST_SECOND;
    /* GST_LOG ("waited: %" GST_TIME_FORMAT ", frame len: %" GST_TIME_FORMAT,
       GST_TIME_ARGS (waited_for_block), GST_TIME_ARGS (wait_time)); */
    ++vcrop;
    ++hcrop;

    if (wait_time > waited_for_block) {
      g_usleep ((wait_time - waited_for_block) / GST_MSECOND);
    }

    time_run += wait_time;
  }
  while (time_run < (TIME_PER_TEST * GST_SECOND));

  g_timer_destroy (timer);
  gst_object_unref (bus);
  gst_object_unref (pad);
  gst_object_unref (pipeline);
}

/* return a list of caps where we only need to set
 * width and height to get fixed caps */
static GList *
video_crop_get_test_caps (GstElement * videocrop)
{
  const GstCaps *allowed_caps;
  GstPad *srcpad;
  GList *list = NULL;
  guint i;

  srcpad = gst_element_get_static_pad (videocrop, "src");
  g_assert (srcpad != NULL);
  allowed_caps = gst_pad_get_pad_template_caps (srcpad);
  g_assert (allowed_caps != NULL);

  for (i = 0; i < gst_caps_get_size (allowed_caps); ++i) {
    GstStructure *new_structure;
    GstCaps *single_caps;

    single_caps = gst_caps_new_empty ();
    new_structure =
        gst_structure_copy (gst_caps_get_structure (allowed_caps, i));
    gst_structure_set (new_structure, "framerate", GST_TYPE_FRACTION,
        FRAMERATE, 1, NULL);
    gst_structure_remove_field (new_structure, "width");
    gst_structure_remove_field (new_structure, "height");
    gst_caps_append_structure (single_caps, new_structure);

    /* should be fixed without width/height */
    g_assert (gst_caps_is_fixed (single_caps));

    list = g_list_prepend (list, single_caps);
  }

  gst_object_unref (srcpad);

  return list;
}

static gchar *opt_videosink_str;        /* NULL */
static gchar *opt_filtercaps_str;       /* NULL */
static gboolean opt_with_videoconvert;  /* FALSE */

int
main (int argc, char **argv)
{
  static const GOptionEntry test_goptions[] = {
    {"videosink", '\0', 0, G_OPTION_ARG_STRING, &opt_videosink_str,
        "videosink to use (default: " DEFAULT_VIDEOSINK ")", NULL},
    {"caps", '\0', 0, G_OPTION_ARG_STRING, &opt_filtercaps_str,
        "filter caps to narrow down formats to test", NULL},
    {"with-videoconvert", '\0', 0, G_OPTION_ARG_NONE,
          &opt_with_videoconvert,
          "whether to add an videoconvert element in front of the sink",
        NULL},
    {NULL, '\0', 0, 0, NULL, NULL, NULL}
  };
  GOptionContext *ctx;
  GError *opt_err = NULL;

  GstElement *pipeline, *src, *filter1, *crop, *scale, *filter2, *csp, *sink;
  GstCaps *filter_caps = NULL;
  GList *caps_list, *l;

  /* command line option parsing */
  ctx = g_option_context_new ("");
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  g_option_context_add_main_entries (ctx, test_goptions, NULL);

  if (!g_option_context_parse (ctx, &argc, &argv, &opt_err)) {
    g_error ("Error parsing command line options: %s", opt_err->message);
    g_option_context_free (ctx);
    g_clear_error (&opt_err);
    return -1;
  }
  g_option_context_free (ctx);

  GST_DEBUG_CATEGORY_INIT (videocrop_test_debug, "videocroptest", 0, "vctest");

  pipeline = gst_pipeline_new ("pipeline");
  src = gst_element_factory_make ("videotestsrc", "videotestsrc");
  g_assert (src != NULL);
  filter1 = gst_element_factory_make ("capsfilter", "capsfilter1");
  g_assert (filter1 != NULL);
  crop = gst_element_factory_make ("videocrop", "videocrop");
  g_assert (crop != NULL);
  scale = gst_element_factory_make ("videoscale", "videoscale");
  g_assert (scale != NULL);
  filter2 = gst_element_factory_make ("capsfilter", "capsfilter2");
  g_assert (filter2 != NULL);

  if (opt_with_videoconvert) {
    g_print ("Adding videoconvert\n");
    csp = gst_element_factory_make ("videoconvert", "colorspace");
  } else {
    csp = gst_element_factory_make ("identity", "colorspace");
  }
  g_assert (csp != NULL);

  if (opt_filtercaps_str) {
    filter_caps = gst_caps_from_string (opt_filtercaps_str);
    if (filter_caps == NULL) {
      g_error ("Invalid filter caps string '%s'", opt_filtercaps_str);
    } else {
      g_print ("Using filter caps '%s'\n", opt_filtercaps_str);
    }
  }

  if (opt_videosink_str) {
    g_print ("Trying videosink '%s' ...", opt_videosink_str);
    sink = gst_element_factory_make (opt_videosink_str, "sink");
    g_print ("%s\n", (sink) ? "ok" : "element couldn't be created");
  } else {
    sink = NULL;
  }

  if (sink == NULL) {
    g_print ("Trying videosink '%s' ...", DEFAULT_VIDEOSINK);
    sink = gst_element_factory_make (DEFAULT_VIDEOSINK, "sink");
    g_print ("%s\n", (sink) ? "ok" : "element couldn't be created");
  }
  if (sink == NULL) {
    g_print ("Trying videosink '%s' ...", "xvimagesink");
    sink = gst_element_factory_make ("xvimagesink", "sink");
    g_print ("%s\n", (sink) ? "ok" : "element couldn't be created");
  }
  if (sink == NULL) {
    g_print ("Trying videosink '%s' ...", "ximagesink");
    sink = gst_element_factory_make ("ximagesink", "sink");
    g_print ("%s\n", (sink) ? "ok" : "element couldn't be created");
  }

  g_assert (sink != NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, filter1, crop, scale, filter2,
      csp, sink, NULL);

  if (!gst_element_link (src, filter1))
    g_error ("Failed to link videotestsrc to capsfilter1");

  if (!gst_element_link (filter1, crop))
    g_error ("Failed to link capsfilter1 to videocrop");

  if (!gst_element_link (crop, scale))
    g_error ("Failed to link videocrop to videoscale");

  if (!gst_element_link (scale, filter2))
    g_error ("Failed to link videoscale to capsfilter2");

  if (!gst_element_link (filter2, csp))
    g_error ("Failed to link capsfilter2 to videoconvert");

  if (!gst_element_link (csp, sink))
    g_error ("Failed to link videoconvert to video sink");

  caps_list = video_crop_get_test_caps (crop);
  for (l = caps_list; l != NULL; l = l->next) {
    GstStateChangeReturn ret;
    GstCaps *caps, *out_caps;
    gboolean skip = FALSE;
    gchar *s;

    if (filter_caps) {
      GstCaps *icaps;

      icaps = gst_caps_intersect (filter_caps, GST_CAPS (l->data));
      skip = gst_caps_is_empty (icaps);
      gst_caps_unref (icaps);
    }

    /* this is the size of our window (stays fixed) */
    out_caps = gst_caps_copy (GST_CAPS (l->data));
    gst_structure_set (gst_caps_get_structure (out_caps, 0), "width",
        G_TYPE_INT, OUT_WIDTH, "height", G_TYPE_INT, OUT_HEIGHT, NULL);

    g_object_set (filter2, "caps", out_caps, NULL);

    /* filter1 gets these too to prevent videotestsrc from renegotiating */
    g_object_set (filter1, "caps", out_caps, NULL);
    gst_caps_unref (out_caps);

    caps = gst_caps_copy (GST_CAPS (l->data));
    GST_INFO ("testing format: %" GST_PTR_FORMAT, caps);

    s = gst_caps_to_string (caps);

    if (skip) {
      g_print ("Skipping format: %s\n", s);
      g_free (s);
      continue;
    }

    g_print ("Format: %s\n", s);

    caps = gst_caps_make_writable (caps);

    /* FIXME: check return values */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret != GST_STATE_CHANGE_FAILURE) {
      ret = gst_element_get_state (pipeline, NULL, NULL, -1);

      if (ret != GST_STATE_CHANGE_FAILURE) {
        test_with_caps (src, crop, caps);
      } else {
        g_print ("Format: %s not supported (failed to go to PLAYING)\n", s);
      }
    } else {
      g_print ("Format: %s not supported\n", s);
    }

    gst_element_set_state (pipeline, GST_STATE_NULL);

    gst_caps_unref (caps);
    g_free (s);
  }

  g_list_foreach (caps_list, (GFunc) gst_caps_unref, NULL);
  g_list_free (caps_list);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  return 0;
}
