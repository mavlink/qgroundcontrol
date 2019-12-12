/* GStreamer
 *
 * unit test for capssetter
 *
 * Copyright (C) <2009> Mark Nauwelaerts <mnauw@users.sourceforge.net>
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


/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
static GstPad *mysrcpad, *mysinkpad;


static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstElement *
setup_capssetter (void)
{
  GstElement *capssetter;

  GST_DEBUG ("setup_capssetter");

  capssetter = gst_check_setup_element ("capssetter");
  mysrcpad = gst_check_setup_src_pad (capssetter, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (capssetter, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return capssetter;
}

static void
cleanup_capssetter (GstElement * capssetter)
{
  GST_DEBUG ("cleanup_capssetter");

  gst_check_drop_buffers ();
  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (capssetter);
  gst_check_teardown_sink_pad (capssetter);
  gst_check_teardown_element (capssetter);
}

static void
push_and_test (GstCaps * prop_caps, gboolean join, gboolean replace,
    GstCaps * in_caps, GstCaps * out_caps)
{
  GstElement *capssetter;
  GstBuffer *buffer;
  GstCaps *current_out;

  capssetter = setup_capssetter ();
  fail_unless (gst_element_set_state (capssetter,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (capssetter, "join", join, NULL);
  g_object_set (capssetter, "replace", replace, NULL);
  g_object_set (capssetter, "caps", prop_caps, NULL);
  gst_caps_unref (prop_caps);

  buffer = gst_buffer_new_and_alloc (4);
  ASSERT_BUFFER_REFCOUNT (buffer, "buffer", 1);
  gst_buffer_fill (buffer, 0, "data", 4);

  gst_check_setup_events (mysrcpad, capssetter, in_caps, GST_FORMAT_TIME);
  gst_caps_unref (in_caps);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK,
      "Failed pushing buffer to capssetter");

  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()) == TRUE);

  /* ... but it should end up being collected on the global buffer list */
  fail_unless (g_list_length (buffers) == 1);
  buffer = g_list_first (buffers)->data;
  ASSERT_BUFFER_REFCOUNT (buffer, "buffer", 1);

  current_out = gst_pad_get_current_caps (mysinkpad);
  fail_unless (gst_caps_is_equal (out_caps, current_out));
  gst_caps_unref (current_out);
  gst_caps_unref (out_caps);

  /* cleanup */
  cleanup_capssetter (capssetter);
}

#define SRC_WIDTH   8
#define SRC_HEIGHT 12

static GstCaps *
make_src_caps (void)
{
  return gst_caps_new_simple ("video/x-foo", "width", G_TYPE_INT, SRC_WIDTH,
      "height", G_TYPE_INT, SRC_HEIGHT, NULL);
}

/* don't try these caps mutations at home, folks */

GST_START_TEST (test_n_join_n_replace)
{
  GstCaps *in_caps, *prop_caps, *out_caps;

  in_caps = make_src_caps ();
  prop_caps = gst_caps_new_simple ("video/x-bar",
      "width", G_TYPE_INT, 2 * SRC_WIDTH, NULL);
  out_caps = gst_caps_new_simple ("video/x-bar",
      "width", G_TYPE_INT, 2 * SRC_WIDTH,
      "height", G_TYPE_INT, SRC_HEIGHT, NULL);
  push_and_test (prop_caps, FALSE, FALSE, in_caps, out_caps);
}

GST_END_TEST;

GST_START_TEST (test_n_join_replace)
{
  GstCaps *in_caps, *prop_caps, *out_caps;

  in_caps = make_src_caps ();
  prop_caps = gst_caps_new_simple ("video/x-bar",
      "width", G_TYPE_INT, 2 * SRC_WIDTH, NULL);
  out_caps = gst_caps_copy (prop_caps);
  push_and_test (prop_caps, FALSE, TRUE, in_caps, out_caps);
}

GST_END_TEST;

GST_START_TEST (test_join_n_replace_n_match)
{
  GstCaps *in_caps, *prop_caps, *out_caps;

  /* non joining caps */
  in_caps = make_src_caps ();
  prop_caps = gst_caps_new_simple ("video/x-bar",
      "width", G_TYPE_INT, 2 * SRC_WIDTH, NULL);
  out_caps = gst_caps_copy (in_caps);
  push_and_test (prop_caps, TRUE, FALSE, in_caps, out_caps);
}

GST_END_TEST;

GST_START_TEST (test_join_n_replace_match)
{
  GstCaps *in_caps, *prop_caps, *out_caps;

  /* joining caps */
  in_caps = make_src_caps ();
  prop_caps = gst_caps_new_simple ("video/x-foo",
      "width", G_TYPE_INT, 2 * SRC_WIDTH, NULL);
  out_caps = gst_caps_new_simple ("video/x-foo",
      "width", G_TYPE_INT, 2 * SRC_WIDTH,
      "height", G_TYPE_INT, SRC_HEIGHT, NULL);
  push_and_test (prop_caps, TRUE, FALSE, in_caps, out_caps);
}

GST_END_TEST;

GST_START_TEST (test_join_replace_n_match)
{
  GstCaps *in_caps, *prop_caps, *out_caps;

  /* non joining caps */
  in_caps = make_src_caps ();
  prop_caps = gst_caps_new_simple ("video/x-bar",
      "width", G_TYPE_INT, 2 * SRC_WIDTH, NULL);
  out_caps = gst_caps_copy (in_caps);
  push_and_test (prop_caps, TRUE, TRUE, in_caps, out_caps);
}

GST_END_TEST;

GST_START_TEST (test_join_replace_match)
{
  GstCaps *in_caps, *prop_caps, *out_caps;

  /* joining caps */
  in_caps = make_src_caps ();
  prop_caps = gst_caps_new_simple ("video/x-foo",
      "width", G_TYPE_INT, 2 * SRC_WIDTH, NULL);
  out_caps = gst_caps_copy (prop_caps);
  push_and_test (prop_caps, TRUE, TRUE, in_caps, out_caps);
}

GST_END_TEST;

static Suite *
capssetter_suite (void)
{
  Suite *s = suite_create ("capssetter");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_n_join_n_replace);
  tcase_add_test (tc_chain, test_n_join_replace);
  tcase_add_test (tc_chain, test_join_n_replace_n_match);
  tcase_add_test (tc_chain, test_join_n_replace_match);
  tcase_add_test (tc_chain, test_join_replace_n_match);
  tcase_add_test (tc_chain, test_join_replace_match);

  return s;
}

GST_CHECK_MAIN (capssetter);
