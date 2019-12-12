/* GStreamer unit test for the gdkpixbufoverlay element
 * Copyright (C) 2015 Tim-Philipp Müller <tim centricular com>
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

#include <gst/check/gstcheck.h>

GST_START_TEST (test_simple_overlay)
{
  GstElement *pipeline, *src, *overlay, *sink;
  GstMessage *msg;
  GstBus *bus;

  src = gst_element_factory_make ("videotestsrc", NULL);
  fail_unless (src != NULL);
  g_object_set (src, "num-buffers", 3, NULL);

  overlay = gst_element_factory_make ("gdkpixbufoverlay", NULL);
  fail_unless (overlay != NULL);

#define IMAGE_PATH GST_TEST_FILES_PATH G_DIR_SEPARATOR_S "image.jpg"
  g_object_set (overlay, "location", IMAGE_PATH, NULL);

  sink = gst_element_factory_make ("fakesink", NULL);
  fail_unless (sink != NULL);

  pipeline = gst_pipeline_new (NULL);
  gst_bin_add_many (GST_BIN (pipeline), src, overlay, sink, NULL);
  gst_element_link_many (src, overlay, sink, NULL);

  /* start prerolling */
  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_ASYNC);

  bus = gst_element_get_bus (pipeline);

  /* wait for EOS */
  msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);
  gst_message_unref (msg);

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);

  gst_object_unref (bus);
  gst_object_unref (pipeline);
}

GST_END_TEST;

static Suite *
gdkpixbufoverlay_suite (void)
{
  Suite *s = suite_create ("gdkpixbufoverlay");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_simple_overlay);

  return s;
}

GST_CHECK_MAIN (gdkpixbufoverlay);
