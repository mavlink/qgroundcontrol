/* GStreamer
 *
 * Copyright (C) 2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <gst/gst.h>
#include <gst/check/gstcheck.h>

static gboolean have_eos = FALSE;

static gboolean
on_message (GstBus * bus, GstMessage * message, gpointer user_data)
{
  GMainLoop *loop = (GMainLoop *) user_data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING:
      g_assert_not_reached ();
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_EOS:
      have_eos = TRUE;
      g_main_loop_quit (loop);
      break;
    default:
      break;
  }

  return TRUE;
}

static void
on_rate_changed (GstElement * element, gint rate, gpointer user_data)
{
  GValueArray *va;
  GValue v = { 0, };

  fail_unless (rate > 0);

  va = g_value_array_new (6);

  g_value_init (&v, G_TYPE_DOUBLE);
  g_value_set_double (&v, 0.0);
  g_value_array_append (va, &v);
  g_value_reset (&v);
  g_value_set_double (&v, 0.0);
  g_value_array_append (va, &v);
  g_value_reset (&v);
  g_value_set_double (&v, 0.0);
  g_value_array_append (va, &v);
  g_value_reset (&v);
  g_value_set_double (&v, 0.0);
  g_value_array_append (va, &v);
  g_value_reset (&v);
  g_value_set_double (&v, 0.0);
  g_value_array_append (va, &v);
  g_value_reset (&v);
  g_value_set_double (&v, 1.0);
  g_value_array_append (va, &v);
  g_value_reset (&v);

  g_object_set (G_OBJECT (element), "b", va, NULL);

  g_value_array_free (va);

  va = g_value_array_new (6);

  g_value_set_double (&v, 1.0);
  g_value_array_append (va, &v);
  g_value_reset (&v);

  g_object_set (G_OBJECT (element), "a", va, NULL);

  g_value_array_free (va);
}

static gboolean have_data = FALSE;

static void
on_handoff (GstElement * object, GstBuffer * buffer, GstPad * pad,
    gpointer user_data)
{
  if (!have_data) {
    GstMapInfo map;
    gfloat *data;

    fail_unless (gst_buffer_map (buffer, &map, GST_MAP_READ));
    data = (gfloat *) map.data;

    fail_unless (map.size > 5 * sizeof (gdouble));
    fail_unless (data[0] == 0.0);
    fail_unless (data[1] == 0.0);
    fail_unless (data[2] == 0.0);
    fail_unless (data[3] == 0.0);
    fail_unless (data[4] == 0.0);
    fail_unless (data[5] != 0.0);

    gst_buffer_unmap (buffer, &map);
    have_data = TRUE;
  }
}

GST_START_TEST (test_pipeline)
{
  GstElement *pipeline, *src, *filter, *sink;
  GstBus *bus;
  GMainLoop *loop;

  have_data = FALSE;
  have_eos = FALSE;

  pipeline = gst_element_factory_make ("pipeline", NULL);
  fail_unless (pipeline != NULL);

  src = gst_element_factory_make ("audiotestsrc", NULL);
  fail_unless (src != NULL);
  g_object_set (G_OBJECT (src), "num-buffers", 1000, NULL);

  filter = gst_element_factory_make ("audioiirfilter", NULL);
  fail_unless (filter != NULL);
  g_signal_connect (G_OBJECT (filter), "rate-changed",
      G_CALLBACK (on_rate_changed), NULL);

  sink = gst_element_factory_make ("fakesink", NULL);
  fail_unless (sink != NULL);
  g_object_set (G_OBJECT (sink), "signal-handoffs", TRUE, NULL);
  g_signal_connect (G_OBJECT (sink), "handoff", G_CALLBACK (on_handoff), NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, filter, sink, NULL);
  fail_unless (gst_element_link_many (src, filter, sink, NULL));

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (on_message), loop);

  fail_if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE);

  g_main_loop_run (loop);

  fail_unless (have_data);
  fail_unless (have_eos);

  fail_unless (gst_element_set_state (pipeline,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  gst_bus_remove_signal_watch (bus);
  gst_object_unref (GST_OBJECT (bus));
  g_main_loop_unref (loop);
  gst_object_unref (pipeline);
}

GST_END_TEST;

static Suite *
audioiirfilter_suite (void)
{
  Suite *s = suite_create ("audioiirfilter");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_pipeline);

  return s;
}

GST_CHECK_MAIN (audioiirfilter);
