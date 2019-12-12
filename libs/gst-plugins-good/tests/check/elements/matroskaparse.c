/* GStreamer unit tests for matroskaparse
 * Copyright (C) 2011 Tim-Philipp Müller  <tim centricular net>
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

#include <gst/check/gstcheck.h>

#include <gst/gst.h>

static void
run_check_for_file (const gchar * file_name, gboolean push_mode)
{
  GstStateChangeReturn state_ret;
  GstMessage *msg;
  GstElement *src, *sep, *sink, *matroskaparse, *pipeline;
  GstBus *bus;
  gchar *path;

  pipeline = gst_pipeline_new ("pipeline");
  fail_unless (pipeline != NULL, "Failed to create pipeline!");

  bus = gst_element_get_bus (pipeline);

  src = gst_element_factory_make ("filesrc", "filesrc");
  fail_unless (src != NULL, "Failed to create 'filesrc' element!");

  if (push_mode) {
    sep = gst_element_factory_make ("queue", "queue");
    fail_unless (sep != NULL, "Failed to create 'queue' element");
  } else {
    sep = gst_element_factory_make ("identity", "identity");
    fail_unless (sep != NULL, "Failed to create 'identity' element");
  }

  matroskaparse = gst_element_factory_make ("matroskaparse", "matroskaparse");
  fail_unless (matroskaparse != NULL, "Failed to create matroskaparse element");

  sink = gst_element_factory_make ("fakesink", "fakesink");
  fail_unless (sink != NULL, "Failed to create 'fakesink' element!");

  gst_bin_add_many (GST_BIN (pipeline), src, sep, matroskaparse, sink, NULL);

  fail_unless (gst_element_link (src, sep));
  fail_unless (gst_element_link (sep, matroskaparse));
  fail_unless (gst_element_link (matroskaparse, sink));

  path = g_build_filename (GST_TEST_FILES_PATH, file_name, NULL);
  GST_LOG ("reading file '%s'", path);
  g_object_set (src, "location", path, NULL);

  state_ret = gst_element_set_state (pipeline, GST_STATE_PAUSED);
  fail_unless (state_ret != GST_STATE_CHANGE_FAILURE);

  if (state_ret == GST_STATE_CHANGE_ASYNC) {
    GST_LOG ("waiting for pipeline to reach PAUSED state");
    state_ret = gst_element_get_state (pipeline, NULL, NULL, -1);
    fail_unless_equals_int (state_ret, GST_STATE_CHANGE_SUCCESS);
  }

  GST_LOG ("PAUSED, let's play a little..");
  state_ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  fail_unless (state_ret != GST_STATE_CHANGE_FAILURE);

  msg = gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, -1);
  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
    GError *err;
    gchar *dbg;

    gst_message_parse_error (msg, &err, &dbg);
    gst_object_default_error (GST_MESSAGE_SRC (msg), err, dbg);
    g_error ("%s (%s)", err->message, dbg);
    g_error_free (err);
    g_free (dbg);
  }
  gst_message_unref (msg);
  gst_object_unref (bus);

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);
  gst_object_unref (pipeline);

  g_free (path);
}

#if 0
GST_START_TEST (test_parse_file_pull)
{
  run_check_for_file ("pinknoise-vorbis.mkv", TRUE);
}

GST_END_TEST;
#endif

GST_START_TEST (test_parse_file_push)
{
  run_check_for_file ("pinknoise-vorbis.mkv", FALSE);
}

GST_END_TEST;

static Suite *
matroskaparse_suite (void)
{
  Suite *s = suite_create ("matroskaparse");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
#if 0
  tcase_add_test (tc_chain, test_parse_file_pull);
#endif
  tcase_add_test (tc_chain, test_parse_file_push);

  return s;
}

GST_CHECK_MAIN (matroskaparse)
