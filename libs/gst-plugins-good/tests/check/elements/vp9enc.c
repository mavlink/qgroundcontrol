/* GStreamer
 *
 * Copyright (c) 2016 Stian Selnes <stian@pexip.com>
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
#include <gst/check/gstharness.h>
#include <gst/check/gstcheck.h>
#include <gst/video/video.h>

GST_START_TEST (test_encode_lag_in_frames)
{
  GstHarness *h = gst_harness_new_parse ("vp9enc lag-in-frames=5 cpu-used=8 "
      "deadline=1");
  gint i;

  gst_harness_add_src_parse (h, "videotestsrc is-live=true pattern=black ! "
      "capsfilter caps=\"video/x-raw,width=320,height=240,framerate=25/1\"",
      TRUE);

  /* Push 20 buffers into the encoder */
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_src_crank_and_push_many (h, 20, 20));

  /* Only 5 buffers are allowed to be queued now */
  fail_unless (gst_harness_buffers_received (h) > 15);

  /* EOS will cause the remaining buffers to be drained */
  fail_unless (gst_harness_push_event (h, gst_event_new_eos ()));
  fail_unless_equals_int (gst_harness_buffers_received (h), 20);

  for (i = 0; i < 20; i++) {
    GstBuffer *buffer = gst_harness_pull (h);

    if (i == 0)
      fail_if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT));

    fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer),
        gst_util_uint64_scale (i, GST_SECOND, 25));
    fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer),
        gst_util_uint64_scale (1, GST_SECOND, 25));

    gst_buffer_unref (buffer);
  }

  gst_harness_teardown (h);
}

GST_END_TEST;


static Suite *
vp9enc_suite (void)
{
  Suite *s = suite_create ("vp9enc");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_encode_lag_in_frames);

  return s;
}

GST_CHECK_MAIN (vp9enc);
