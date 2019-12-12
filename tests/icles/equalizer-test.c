/* GStreamer test for the equalizer element
 * Copyright (C) 2007 Tim-Philipp Müller <tim centricular net>
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

/*
 * Will tests the equalizer by fading all bands in and out one by one and
 * finaly all together.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gst/gst.h>

#include <stdlib.h>
#include <math.h>

GST_DEBUG_CATEGORY_STATIC (equalizer_test_debug);
#define GST_CAT_DEFAULT equalizer_test_debug

static GstBus *pipeline_bus;

static gboolean
check_bus (GstClockTime max_wait_time)
{
  GstMessage *msg;

  msg = gst_bus_poll (pipeline_bus, GST_MESSAGE_ERROR | GST_MESSAGE_EOS,
      max_wait_time);

  if (msg == NULL)
    return FALSE;

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
    GError *err = NULL;
    gchar *debug = NULL;

    g_assert (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR);
    gst_message_parse_error (msg, &err, &debug);
    GST_ERROR ("ERROR: %s [%s]", err->message, debug);
    g_print ("\n===========> ERROR: %s\n%s\n\n", err->message, debug);
    g_clear_error (&err);
    g_free (debug);
  }

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS) {
    g_print ("\n === EOS ===\n\n");
  }

  gst_message_unref (msg);
  return TRUE;
}

// fix below

static void
equalizer_set_band_value (GstElement * eq, guint band, gdouble val)
{
  GObject *child;

  child = gst_child_proxy_get_child_by_index (GST_CHILD_PROXY (eq), band);
  g_object_set (child, "gain", val, NULL);
  g_object_unref (child);
  g_print ("Band %2d: %.2f\n", band, val);
}

static void
equalizer_set_all_band_values (GstElement * eq, guint num, gdouble val)
{
  gint i;
  GObject *child;

  for (i = 0; i < num; i++) {
    child = gst_child_proxy_get_child_by_index (GST_CHILD_PROXY (eq), i);
    g_object_set (child, "gain", val, NULL);
    g_object_unref (child);
  }
  g_print ("All bands: %.2f\n", val);
}

// fix above

static gboolean
equalizer_set_band_value_and_wait (GstElement * eq, guint band, gdouble val)
{
  equalizer_set_band_value (eq, band, val);
  return check_bus (100 * GST_MSECOND);
}

static gboolean
equalizer_set_all_band_values_and_wait (GstElement * eq, guint num, gdouble val)
{
  equalizer_set_all_band_values (eq, num, val);
  return check_bus (100 * GST_MSECOND);
}

static void
do_slider_fiddling (GstElement * playbin, GstElement * eq)
{
  gboolean stop;
  guint num_bands, i;
  gdouble d, step = 0.5;

  stop = FALSE;

  g_object_get (eq, "num-bands", &num_bands, NULL);

  g_print ("%u bands.\n", num_bands);

  while (!stop) {
    for (i = 0; !stop && i < num_bands; ++i) {
      d = -24.0;
      while (!stop && d <= 12.0) {
        stop = equalizer_set_band_value_and_wait (eq, i, d);
        d += step;
      }
      d = 12.0;
      while (!stop && d >= -24.0) {
        stop = equalizer_set_band_value_and_wait (eq, i, d);
        d -= step;
      }
      d = -24.0;
      while (!stop && d <= 12.0) {
        stop = equalizer_set_band_value_and_wait (eq, i, d);
        d += step;
      }
    }

    d = 0.0;
    while (!stop && d <= 12.0) {
      stop = equalizer_set_all_band_values_and_wait (eq, num_bands, d);
      d += step;
    }
    d = 12.0;
    while (!stop && d >= -24.0) {
      stop = equalizer_set_all_band_values_and_wait (eq, num_bands, d);
      d -= step;
    }
    d = -24.0;
    while (!stop && d <= 0.0) {
      stop = equalizer_set_all_band_values_and_wait (eq, num_bands, d);
      d += step;
    }
  }
}

int
main (int argc, char **argv)
{
  gchar *opt_audiosink_str = NULL;
  gchar **filenames = NULL;
  const GOptionEntry test_goptions[] = {
    {"audiosink", '\0', 0, G_OPTION_ARG_STRING, &opt_audiosink_str,
        "audiosink to use (default: autoaudiosink)", NULL},
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL},
    {NULL, '\0', 0, 0, NULL, NULL, NULL}
  };
  GOptionContext *ctx;
  GError *opt_err = NULL;

  GstStateChangeReturn ret;
  GstElement *playbin, *sink, *bin, *eq, *auconv;
  GstPad *eq_sinkpad;
  gchar *uri;

  /* command line option parsing */
  ctx = g_option_context_new ("FILENAME");
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  g_option_context_add_main_entries (ctx, test_goptions, NULL);

  if (!g_option_context_parse (ctx, &argc, &argv, &opt_err)) {
    g_error ("Error parsing command line options: %s", opt_err->message);
    g_option_context_free (ctx);
    g_clear_error (&opt_err);
    return -1;
  }
  g_option_context_free (ctx);

  GST_DEBUG_CATEGORY_INIT (equalizer_test_debug, "equalizertest", 0, "eqtest");

  if (filenames == NULL || *filenames == NULL) {
    g_printerr ("Please specify a file to play back\n");
    return -1;
  }

  playbin = gst_element_factory_make ("playbin", "playbin");
  if (playbin == NULL) {
    g_error ("Couldn't create 'playbin' element");
    return -1;
  }

  if (opt_audiosink_str) {
    g_print ("Trying audiosink '%s' ...", opt_audiosink_str);
    sink = gst_element_factory_make (opt_audiosink_str, "sink");
    g_print ("%s\n", (sink) ? "ok" : "element couldn't be created");
  } else {
    sink = NULL;
  }
  if (sink == NULL) {
    g_print ("Trying audiosink '%s' ...", "autoaudiosink");
    sink = gst_element_factory_make ("autoaudiosink", "sink");
    g_print ("%s\n", (sink) ? "ok" : "element couldn't be created");
  }
  if (sink == NULL) {
    g_print ("Trying audiosink '%s' ...", "alsasink");
    sink = gst_element_factory_make ("alsasink", "sink");
    g_print ("%s\n", (sink) ? "ok" : "element couldn't be created");
  }
  if (sink == NULL) {
    g_print ("Trying audiosink '%s' ...", "osssink");
    sink = gst_element_factory_make ("osssink", "sink");
    g_print ("%s\n", (sink) ? "ok" : "element couldn't be created");
  }

  g_assert (sink != NULL);

  bin = gst_bin_new ("ausinkbin");
  g_assert (bin != NULL);

  eq = gst_element_factory_make ("equalizer-nbands", "equalizer");
  g_assert (eq != NULL);

  auconv = gst_element_factory_make ("audioconvert", "eqauconv");
  g_assert (auconv != NULL);

  gst_bin_add_many (GST_BIN (bin), eq, auconv, sink, NULL);

  if (!gst_element_link (eq, auconv))
    g_error ("Failed to link equalizer to audioconvert");

  if (!gst_element_link (auconv, sink))
    g_error ("Failed to link audioconvert to audio sink");

  eq_sinkpad = gst_element_get_static_pad (eq, "sink");
  g_assert (eq_sinkpad != NULL);

  gst_element_add_pad (bin, gst_ghost_pad_new (NULL, eq_sinkpad));
  gst_object_unref (eq_sinkpad);

  g_object_set (playbin, "audio-sink", bin, NULL);

  /* won't work: uri = gst_uri_construct ("file", filenames[0]); */
  uri = g_strdup_printf ("file://%s", filenames[0]);
  g_object_set (playbin, "uri", uri, NULL);
  g_free (uri);

  pipeline_bus = GST_ELEMENT_BUS (playbin);

  ret = gst_element_set_state (playbin, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Failed to set playbin to PLAYING\n");
    check_bus (1 * GST_SECOND);
    return -1;
  }

  ret = gst_element_get_state (playbin, NULL, NULL, 5 * GST_SECOND);
  if (ret == GST_STATE_CHANGE_ASYNC) {
    g_printerr ("Failed to go to PLAYING in 5 seconds, bailing out\n");
    return -1;
  } else if (ret != GST_STATE_CHANGE_SUCCESS) {
    g_printerr ("State change to PLAYING failed\n");
    check_bus (1 * GST_SECOND);
    return -1;
  }

  g_print ("Playing ...\n");
  do_slider_fiddling (playbin, eq);

  gst_element_set_state (playbin, GST_STATE_NULL);
  gst_object_unref (playbin);

  return 0;
}
