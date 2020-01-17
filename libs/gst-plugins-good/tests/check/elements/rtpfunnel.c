/* GStreamer
 *
 * unit test for rtpfunnel
 *
 * Copyright (C) <2017> Pexip.
 *   Contact: Havard Graff <havard@pexip.com>
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
#include <gst/check/gstharness.h>

GST_START_TEST (rtpfunnel_ssrc_demuxing)
{
  GstHarness *h = gst_harness_new_with_padnames ("rtpfunnel", NULL, "src");
  GstHarness *h0 = gst_harness_new_with_element (h->element, "sink_0", NULL);
  GstHarness *h1 = gst_harness_new_with_element (h->element, "sink_1", NULL);

  gst_harness_set_src_caps_str (h0, "application/x-rtp, ssrc=(uint)123");
  gst_harness_set_src_caps_str (h1, "application/x-rtp, ssrc=(uint)321");

  /* unref latency events */
  gst_event_unref (gst_harness_pull_upstream_event (h0));
  gst_event_unref (gst_harness_pull_upstream_event (h1));
  fail_unless_equals_int (1, gst_harness_upstream_events_received (h0));
  fail_unless_equals_int (1, gst_harness_upstream_events_received (h1));

  /* send to pad 0 */
  gst_harness_push_upstream_event (h,
      gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
          gst_structure_new ("GstForceKeyUnit",
              "ssrc", G_TYPE_UINT, 123, NULL)));
  fail_unless_equals_int (2, gst_harness_upstream_events_received (h0));
  fail_unless_equals_int (1, gst_harness_upstream_events_received (h1));

  /* send to pad 1 */
  gst_harness_push_upstream_event (h,
      gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
          gst_structure_new ("GstForceKeyUnit",
              "ssrc", G_TYPE_UINT, 321, NULL)));
  fail_unless_equals_int (2, gst_harness_upstream_events_received (h0));
  fail_unless_equals_int (2, gst_harness_upstream_events_received (h1));

  /* unknown ssrc, we drop it */
  gst_harness_push_upstream_event (h,
      gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
          gst_structure_new ("GstForceKeyUnit",
              "ssrc", G_TYPE_UINT, 666, NULL)));
  fail_unless_equals_int (2, gst_harness_upstream_events_received (h0));
  fail_unless_equals_int (2, gst_harness_upstream_events_received (h1));

  /* no ssrc, we send to all */
  gst_harness_push_upstream_event (h,
      gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
          gst_structure_new_empty ("GstForceKeyUnit")));
  fail_unless_equals_int (3, gst_harness_upstream_events_received (h0));
  fail_unless_equals_int (3, gst_harness_upstream_events_received (h1));

  /* remove pad 0, and send an event referencing the now dead ssrc */
  gst_harness_teardown (h0);
  gst_harness_push_upstream_event (h,
      gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
          gst_structure_new ("GstForceKeyUnit",
              "ssrc", G_TYPE_UINT, 123, NULL)));
  fail_unless_equals_int (3, gst_harness_upstream_events_received (h1));

  gst_harness_teardown (h);
  gst_harness_teardown (h1);
}

GST_END_TEST
GST_START_TEST (rtpfunnel_ssrc_downstream_not_leaking_through)
{
  GstHarness *h = gst_harness_new_with_padnames ("rtpfunnel",
      "sink_0", "src");
  GstCaps *caps;
  const GstStructure *s;

  gst_harness_set_sink_caps_str (h, "application/x-rtp, ssrc=(uint)123");

  caps = gst_pad_peer_query_caps (h->srcpad, NULL);
  s = gst_caps_get_structure (caps, 0);

  fail_unless (!gst_structure_has_field (s, "ssrc"));

  gst_caps_unref (caps);
  gst_harness_teardown (h);
}

GST_END_TEST
GST_START_TEST (rtpfunnel_common_ts_offset)
{
  GstHarness *h = gst_harness_new_with_padnames ("rtpfunnel",
      "sink_0", "src");
  GstCaps *caps;
  const GstStructure *s;
  const guint expected_ts_offset = 12345;
  guint ts_offset;

  g_object_set (h->element, "common-ts-offset", expected_ts_offset, NULL);

  caps = gst_pad_peer_query_caps (h->srcpad, NULL);
  s = gst_caps_get_structure (caps, 0);

  fail_unless (gst_structure_get_uint (s, "timestamp-offset", &ts_offset));
  fail_unless_equals_int (expected_ts_offset, ts_offset);

  gst_caps_unref (caps);
  gst_harness_teardown (h);
}

GST_END_TEST
GST_START_TEST (rtpfunnel_stress)
{
  GstHarness *h = gst_harness_new_with_padnames ("rtpfunnel",
      "sink_0", "src");
  GstHarness *h1 = gst_harness_new_with_element (h->element, "sink_1", NULL);

  GstPadTemplate *templ =
      gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (h->element),
      "sink_%u");
  GstCaps *caps = gst_caps_from_string ("application/x-rtp, ssrc=(uint)123");
  GstBuffer *buf = gst_buffer_new_allocate (NULL, 0, NULL);
  GstSegment segment;
  GstHarnessThread *statechange, *push, *req, *push1;

  gst_check_add_log_filter ("GStreamer", G_LOG_LEVEL_WARNING,
      g_regex_new ("Got data flow before (stream-start|segment) event",
          (GRegexCompileFlags) 0, (GRegexMatchFlags) 0, NULL),
      NULL, NULL, NULL);
  gst_check_add_log_filter ("GStreamer", G_LOG_LEVEL_WARNING,
      g_regex_new ("Sticky event misordering",
          (GRegexCompileFlags) 0, (GRegexMatchFlags) 0, NULL),
      NULL, NULL, NULL);


  gst_segment_init (&segment, GST_FORMAT_TIME);

  statechange = gst_harness_stress_statechange_start (h);
  push = gst_harness_stress_push_buffer_start (h, caps, &segment, buf);
  req = gst_harness_stress_requestpad_start (h, templ, NULL, NULL, TRUE);
  push1 = gst_harness_stress_push_buffer_start (h1, caps, &segment, buf);

  gst_caps_unref (caps);
  gst_buffer_unref (buf);

  /* test-length */
  g_usleep (G_USEC_PER_SEC * 1);

  gst_harness_stress_thread_stop (push1);
  gst_harness_stress_thread_stop (req);
  gst_harness_stress_thread_stop (push);
  gst_harness_stress_thread_stop (statechange);

  gst_harness_teardown (h1);
  gst_harness_teardown (h);

  gst_check_clear_log_filter ();
}

GST_END_TEST;

static Suite *
rtpfunnel_suite (void)
{
  Suite *s = suite_create ("rtpfunnel");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, rtpfunnel_ssrc_demuxing);
  tcase_add_test (tc_chain, rtpfunnel_ssrc_downstream_not_leaking_through);
  tcase_add_test (tc_chain, rtpfunnel_common_ts_offset);

  tcase_add_test (tc_chain, rtpfunnel_stress);

  return s;
}

GST_CHECK_MAIN (rtpfunnel)
