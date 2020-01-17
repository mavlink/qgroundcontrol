/* GStreamer WavParse unit tests
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

#include <gst/check/gstcheck.h>

#define SIMPLE_WAV_PATH GST_TEST_FILES_PATH G_DIR_SEPARATOR_S "audiotestsrc.wav"

static void
do_test_simple_file (GstPadMode mode)
{
  GstStateChangeReturn ret;
  GstElement *pipeline;
  GstElement *src, *q = NULL;
  GstElement *wavparse;
  GstElement *fakesink;
  GstMessage *msg;

  pipeline = gst_pipeline_new ("testpipe");
  src = gst_element_factory_make ("filesrc", NULL);
  fail_if (src == NULL);
  if (mode == GST_PAD_MODE_PUSH)
    q = gst_element_factory_make ("queue", NULL);
  wavparse = gst_element_factory_make ("wavparse", NULL);
  fail_if (wavparse == NULL);
  fakesink = gst_element_factory_make ("fakesink", NULL);
  fail_if (fakesink == NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, wavparse, fakesink, q, NULL);

  g_object_set (src, "location", SIMPLE_WAV_PATH, NULL);

  if (mode == GST_PAD_MODE_PUSH)
    fail_unless (gst_element_link_many (src, q, wavparse, fakesink, NULL));
  else
    fail_unless (gst_element_link_many (src, wavparse, fakesink, NULL));

  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  fail_unless_equals_int (ret, GST_STATE_CHANGE_ASYNC);

  ret = gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
  fail_unless_equals_int (ret, GST_STATE_CHANGE_SUCCESS);

  msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (pipeline),
      GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS | GST_MESSAGE_ERROR);

  fail_unless_equals_string (GST_MESSAGE_TYPE_NAME (msg), "eos");

  gst_message_unref (msg);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
}

GST_START_TEST (test_simple_file_pull)
{
  do_test_simple_file (TRUE);
}

GST_END_TEST;

GST_START_TEST (test_simple_file_push)
{
  do_test_simple_file (FALSE);
}

GST_END_TEST;

static void
do_test_empty_file (gboolean can_activate_pull)
{
  GstStateChangeReturn ret1, ret2;
  GstElement *pipeline;
  GstElement *src;
  GstElement *wavparse;
  GstElement *fakesink;

  /* Pull mode */
  pipeline = gst_pipeline_new ("testpipe");
  src = gst_element_factory_make ("fakesrc", NULL);
  fail_if (src == NULL);
  wavparse = gst_element_factory_make ("wavparse", NULL);
  fail_if (wavparse == NULL);
  fakesink = gst_element_factory_make ("fakesink", NULL);
  fail_if (fakesink == NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, wavparse, fakesink, NULL);
  g_object_set (src, "num-buffers", 0, "can-activate-pull", can_activate_pull,
      NULL);

  fail_unless (gst_element_link_many (src, wavparse, fakesink, NULL));

  ret1 = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret1 == GST_STATE_CHANGE_ASYNC)
    ret2 = gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
  else
    ret2 = ret1;

  /* should have gotten an error on the bus, no output to fakesink */
  fail_unless_equals_int (ret2, GST_STATE_CHANGE_FAILURE);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
}

GST_START_TEST (test_empty_file_pull)
{
  do_test_empty_file (TRUE);
}

GST_END_TEST;

GST_START_TEST (test_empty_file_push)
{
  do_test_empty_file (FALSE);
}

GST_END_TEST;

static Suite *
wavparse_suite (void)
{
  Suite *s = suite_create ("wavparse");
  TCase *tc_chain = tcase_create ("wavparse");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_empty_file_pull);
  tcase_add_test (tc_chain, test_empty_file_push);
  tcase_add_test (tc_chain, test_simple_file_pull);
  tcase_add_test (tc_chain, test_simple_file_push);
  return s;
}

GST_CHECK_MAIN (wavparse)
