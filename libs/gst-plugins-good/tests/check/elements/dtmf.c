/* GStreamer
 *
 * unit test for dtmf elements
 * Copyright (C) 2013 Collabora Ltd
 *   @author: Olivier Crete <olivier.crete@collabora.com>
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

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/check/gstcheck.h>
#include <gst/check/gsttestclock.h>
#include <gst/rtp/gstrtpbuffer.h>


/* Include this from the plugin to get the defines */

#include "../../gst/dtmf/gstdtmfcommon.h"

#define END_BIT (1<<7)

static GstStaticPadTemplate audio_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) \"" GST_AUDIO_NE (S16) "\", "
        "rate = " GST_AUDIO_RATE_RANGE ", " "channels = (int) 1")
    );

static GstStaticPadTemplate rtp_dtmf_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) [ 0, MAX ], "
        "encoding-name = (string) \"TELEPHONE-EVENT\"")
    );


static void
check_get_dtmf_event_message (GstBus * bus, gint number, gint volume)
{
  GstMessage *message;
  gboolean have_message = FALSE;

  while (!have_message &&
      (message = gst_bus_pop_filtered (bus, GST_MESSAGE_ELEMENT)) != NULL) {
    if (gst_message_has_name (message, "dtmf-event")) {
      const GstStructure *s = gst_message_get_structure (message);
      gint stype, snumber, smethod, svolume;

      fail_unless (gst_structure_get (s,
              "type", G_TYPE_INT, &stype,
              "number", G_TYPE_INT, &snumber,
              "method", G_TYPE_INT, &smethod,
              "volume", G_TYPE_INT, &svolume, NULL));

      fail_unless (stype == 1);
      fail_unless (smethod == 1);
      fail_unless (snumber == number);
      fail_unless (svolume == volume);
      have_message = TRUE;
    }
    gst_message_unref (message);
  }

  fail_unless (have_message);
}

static void
check_no_dtmf_event_message (GstBus * bus)
{
  GstMessage *message;
  gboolean have_message = FALSE;

  while (!have_message &&
      (message = gst_bus_pop_filtered (bus, GST_MESSAGE_ELEMENT)) != NULL) {
    if (gst_message_has_name (message, "dtmf-event") ||
        gst_message_has_name (message, "dtmf-event-processed") ||
        gst_message_has_name (message, "dtmf-event-dropped")) {
      have_message = TRUE;
    }
    gst_message_unref (message);
  }

  fail_unless (!have_message);
}

static void
check_buffers_duration (GstClockTime expected_duration)
{
  GstClockTime duration = 0;

  while (buffers) {
    GstBuffer *buf = buffers->data;

    buffers = g_list_delete_link (buffers, buffers);

    fail_unless (GST_BUFFER_DURATION_IS_VALID (buf));
    duration += GST_BUFFER_DURATION (buf);
    gst_buffer_unref (buf);
  }

  fail_unless (duration == expected_duration);
}

static void
send_rtp_packet (GstPad * src, guint timestamp, gboolean marker, gboolean end,
    guint number, guint volume, guint duration)
{
  GstBuffer *buf;
  GstRTPBuffer rtpbuf = GST_RTP_BUFFER_INIT;
  gchar *payload;
  static guint seqnum = 1;

  buf = gst_rtp_buffer_new_allocate (4, 0, 0);
  fail_unless (gst_rtp_buffer_map (buf, GST_MAP_READWRITE, &rtpbuf));
  gst_rtp_buffer_set_seq (&rtpbuf, seqnum++);
  gst_rtp_buffer_set_timestamp (&rtpbuf, timestamp);
  gst_rtp_buffer_set_marker (&rtpbuf, marker);
  payload = gst_rtp_buffer_get_payload (&rtpbuf);
  payload[0] = number;
  payload[1] = volume | (end ? END_BIT : 0);
  GST_WRITE_UINT16_BE (payload + 2, duration);
  gst_rtp_buffer_unmap (&rtpbuf);
  fail_unless (gst_pad_push (src, buf) == GST_FLOW_OK);
}

GST_START_TEST (test_rtpdtmfdepay)
{
  GstElement *dtmfdepay;
  GstPad *src, *sink;
  GstBus *bus;
  GstCaps *caps_in;
  GstCaps *expected_caps_out;
  GstCaps *caps_out;

  dtmfdepay = gst_check_setup_element ("rtpdtmfdepay");
  sink = gst_check_setup_sink_pad (dtmfdepay, &audio_sink_template);
  src = gst_check_setup_src_pad (dtmfdepay, &rtp_dtmf_src_template);

  bus = gst_bus_new ();
  gst_element_set_bus (dtmfdepay, bus);

  gst_pad_set_active (src, TRUE);
  gst_pad_set_active (sink, TRUE);
  gst_element_set_state (dtmfdepay, GST_STATE_PLAYING);


  caps_in = gst_caps_new_simple ("application/x-rtp",
      "encoding-name", G_TYPE_STRING, "TELEPHONE-EVENT",
      "media", G_TYPE_STRING, "audio",
      "clock-rate", G_TYPE_INT, 1000, "payload", G_TYPE_INT, 99, NULL);
  gst_check_setup_events (src, dtmfdepay, caps_in, GST_FORMAT_TIME);
  gst_caps_unref (caps_in);

  caps_out = gst_pad_get_current_caps (sink);
  expected_caps_out = gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, GST_AUDIO_NE (S16),
      "rate", G_TYPE_INT, 1000, "channels", G_TYPE_INT, 1, NULL);
  fail_unless (gst_caps_is_equal_fixed (caps_out, expected_caps_out));
  gst_caps_unref (expected_caps_out);
  gst_caps_unref (caps_out);

  /* Single packet DTMF */
  send_rtp_packet (src, 200, TRUE, TRUE, 1, 5, 250);
  check_get_dtmf_event_message (bus, 1, 5);
  check_buffers_duration (250 * GST_MSECOND);

  /* Two packet DTMF */
  send_rtp_packet (src, 800, TRUE, FALSE, 1, 5, 200);
  send_rtp_packet (src, 800, FALSE, TRUE, 1, 5, 400);
  check_buffers_duration (400 * GST_MSECOND);
  check_get_dtmf_event_message (bus, 1, 5);

  /* Long DTMF */
  send_rtp_packet (src, 3000, TRUE, FALSE, 1, 5, 200);
  check_get_dtmf_event_message (bus, 1, 5);
  check_buffers_duration (200 * GST_MSECOND);
  send_rtp_packet (src, 3000, FALSE, FALSE, 1, 5, 400);
  check_no_dtmf_event_message (bus);
  check_buffers_duration (200 * GST_MSECOND);
  send_rtp_packet (src, 3000, FALSE, FALSE, 1, 5, 600);
  check_no_dtmf_event_message (bus);
  check_buffers_duration (200 * GST_MSECOND);

  /* New without end to last */
  send_rtp_packet (src, 4000, TRUE, TRUE, 1, 5, 250);
  check_get_dtmf_event_message (bus, 1, 5);
  check_buffers_duration (250 * GST_MSECOND);

  check_no_dtmf_event_message (bus);
  fail_unless (buffers == NULL);
  gst_element_set_bus (dtmfdepay, NULL);
  gst_object_unref (bus);

  gst_pad_set_active (src, FALSE);
  gst_pad_set_active (sink, FALSE);
  gst_check_teardown_sink_pad (dtmfdepay);
  gst_check_teardown_src_pad (dtmfdepay);
  gst_check_teardown_element (dtmfdepay);
}

GST_END_TEST;


static GstStaticPadTemplate rtp_dtmf_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) 99, "
        "clock-rate = (int) 1000, "
        "seqnum-offset = (uint) 333, "
        "timestamp-offset = (uint) 666, "
        "ssrc = (uint) 999, "
        "maxptime = (uint) 20, encoding-name = (string) \"TELEPHONE-EVENT\"")
    );

GstElement *dtmfsrc;
GstPad *sink;
GstClock *testclock;
GstBus *bus;

static void
check_message_structure (GstStructure * expected_s)
{
  GstMessage *message;
  gboolean have_message = FALSE;

  while (!have_message &&
      (message = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
              GST_MESSAGE_ELEMENT)) != NULL) {
    if (gst_message_has_name (message, gst_structure_get_name (expected_s))) {
      const GstStructure *s = gst_message_get_structure (message);

      fail_unless (gst_structure_is_equal (s, expected_s));
      have_message = TRUE;
    }
    gst_message_unref (message);
  }

  fail_unless (have_message);

  gst_structure_free (expected_s);
}

static void
check_rtp_buffer (GstClockTime ts, GstClockTime duration, gboolean start,
    gboolean end, guint rtpts, guint ssrc, guint volume, guint number,
    guint rtpduration)
{
  GstBuffer *buffer;
  GstRTPBuffer rtpbuffer = GST_RTP_BUFFER_INIT;
  gchar *payload;

  g_mutex_lock (&check_mutex);
  while (buffers == NULL)
    g_cond_wait (&check_cond, &check_mutex);
  g_mutex_unlock (&check_mutex);
  fail_unless (buffers != NULL);

  buffer = buffers->data;
  buffers = g_list_delete_link (buffers, buffers);

  fail_unless (GST_BUFFER_PTS (buffer) == ts);
  fail_unless (GST_BUFFER_DURATION (buffer) == duration);

  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtpbuffer));
  fail_unless (gst_rtp_buffer_get_marker (&rtpbuffer) == start);
  fail_unless (gst_rtp_buffer_get_timestamp (&rtpbuffer) == rtpts);
  payload = gst_rtp_buffer_get_payload (&rtpbuffer);

  fail_unless (payload[0] == number);
  fail_unless ((payload[1] & 0x7F) == volume);
  fail_unless (! !(payload[1] & 0x80) == end);
  fail_unless (GST_READ_UINT16_BE (payload + 2) == rtpduration);

  gst_rtp_buffer_unmap (&rtpbuffer);
  gst_buffer_unref (buffer);
}

gint method;

static void
setup_rtpdtmfsrc (void)
{
  testclock = gst_test_clock_new ();
  bus = gst_bus_new ();

  method = 1;
  dtmfsrc = gst_check_setup_element ("rtpdtmfsrc");
  sink = gst_check_setup_sink_pad (dtmfsrc, &rtp_dtmf_sink_template);
  gst_element_set_bus (dtmfsrc, bus);
  fail_unless (gst_element_set_clock (dtmfsrc, testclock));

  gst_pad_set_active (sink, TRUE);
  fail_unless (gst_element_set_state (dtmfsrc, GST_STATE_PLAYING) ==
      GST_STATE_CHANGE_SUCCESS);
}

static void
teardown_dtmfsrc (void)
{
  gst_object_unref (testclock);
  gst_pad_set_active (sink, FALSE);
  gst_element_set_bus (dtmfsrc, NULL);
  gst_object_unref (bus);
  gst_check_teardown_sink_pad (dtmfsrc);
  gst_check_teardown_element (dtmfsrc);
}

GST_START_TEST (test_dtmfsrc_invalid_events)
{
  GstStructure *s;

  /* Missing start */
  s = gst_structure_new ("dtmf-event",
      "type", G_TYPE_INT, 1, "number", G_TYPE_INT, 3,
      "method", G_TYPE_INT, method, "volume", G_TYPE_INT, 8, NULL);
  fail_unless (gst_pad_push_event (sink,
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, s)) == FALSE);

  /* Missing volume */
  s = gst_structure_new ("dtmf-event",
      "type", G_TYPE_INT, 1, "number", G_TYPE_INT, 3,
      "method", G_TYPE_INT, method, "start", G_TYPE_BOOLEAN, TRUE, NULL);
  fail_unless (gst_pad_push_event (sink,
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, s)) == FALSE);

  /* Missing number */
  s = gst_structure_new ("dtmf-event",
      "type", G_TYPE_INT, 1, "method", G_TYPE_INT, method,
      "volume", G_TYPE_INT, 8, "start", G_TYPE_BOOLEAN, TRUE, NULL);
  fail_unless (gst_pad_push_event (sink,
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, s)) == FALSE);

  /* Missing type */
  s = gst_structure_new ("dtmf-event",
      "number", G_TYPE_INT, 3, "method", G_TYPE_INT, method,
      "volume", G_TYPE_INT, 8, "start", G_TYPE_BOOLEAN, TRUE, NULL);
  fail_unless (gst_pad_push_event (sink,
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, s)) == FALSE);

  /* Stop before start */
  s = gst_structure_new ("dtmf-event",
      "type", G_TYPE_INT, 1, "number", G_TYPE_INT, 3,
      "method", G_TYPE_INT, method, "volume", G_TYPE_INT, 8,
      "start", G_TYPE_BOOLEAN, FALSE, NULL);
  fail_unless (gst_pad_push_event (sink,
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, s)) == FALSE);

  gst_element_set_state (dtmfsrc, GST_STATE_NULL);
}

GST_END_TEST;

GST_START_TEST (test_rtpdtmfsrc_min_duration)
{
  GstStructure *s;
  GstClockID id;
  guint timestamp = 0;
  GstCaps *expected_caps, *caps;

  /* Minimum duration dtmf */

  s = gst_structure_new ("dtmf-event",
      "type", G_TYPE_INT, 1, "number", G_TYPE_INT, 3,
      "method", G_TYPE_INT, 1, "volume", G_TYPE_INT, 8,
      "start", G_TYPE_BOOLEAN, TRUE, NULL);
  fail_unless (gst_pad_push_event (sink,
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
              gst_structure_copy (s))));
  check_no_dtmf_event_message (bus);
  gst_test_clock_wait_for_next_pending_id (GST_TEST_CLOCK (testclock), NULL);
  fail_unless (buffers == NULL);
  id = gst_test_clock_process_next_clock_id (GST_TEST_CLOCK (testclock));
  fail_unless (gst_clock_id_get_time (id) == 0);
  gst_clock_id_unref (id);
  gst_structure_set_name (s, "dtmf-event-processed");
  check_message_structure (s);

  s = gst_structure_new ("dtmf-event",
      "type", G_TYPE_INT, 1, "method", G_TYPE_INT, 1,
      "start", G_TYPE_BOOLEAN, FALSE, NULL);
  fail_unless (gst_pad_push_event (sink,
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
              gst_structure_copy (s))));

  check_rtp_buffer (0, 20 * GST_MSECOND, TRUE, FALSE, 666, 999, 8, 3, 20);

  for (timestamp = 20; timestamp < MIN_PULSE_DURATION + 20; timestamp += 20) {
    gst_test_clock_advance_time (GST_TEST_CLOCK (testclock),
        20 * GST_MSECOND + 1);
    gst_test_clock_wait_for_next_pending_id (GST_TEST_CLOCK (testclock), NULL);
    fail_unless (buffers == NULL);
    id = gst_test_clock_process_next_clock_id (GST_TEST_CLOCK (testclock));
    fail_unless (gst_clock_id_get_time (id) == timestamp * GST_MSECOND);
    gst_clock_id_unref (id);

    if (timestamp < MIN_PULSE_DURATION) {
      check_rtp_buffer (timestamp * GST_MSECOND, 20 * GST_MSECOND, FALSE,
          FALSE, 666, 999, 8, 3, timestamp + 20);
      check_no_dtmf_event_message (bus);
    } else {
      gst_structure_set_name (s, "dtmf-event-processed");
      check_message_structure (s);
      check_rtp_buffer (timestamp * GST_MSECOND,
          (20 + MIN_INTER_DIGIT_INTERVAL) * GST_MSECOND, FALSE, TRUE, 666,
          999, 8, 3, timestamp + 20);
    }

    fail_unless (buffers == NULL);
  }


  fail_unless (gst_test_clock_peek_id_count (GST_TEST_CLOCK (testclock)) == 0);

  /* caps check */

  expected_caps = gst_caps_new_simple ("application/x-rtp",
      "encoding-name", G_TYPE_STRING, "TELEPHONE-EVENT",
      "media", G_TYPE_STRING, "audio",
      "clock-rate", G_TYPE_INT, 1000, "payload", G_TYPE_INT, 99,
      "seqnum-offset", G_TYPE_UINT, 333,
      "timestamp-offset", G_TYPE_UINT, 666,
      "ssrc", G_TYPE_UINT, 999, "ptime", G_TYPE_UINT, 20, NULL);
  caps = gst_pad_get_current_caps (sink);
  fail_unless (gst_caps_can_intersect (caps, expected_caps));
  gst_caps_unref (caps);
  gst_caps_unref (expected_caps);

  gst_element_set_state (dtmfsrc, GST_STATE_NULL);

  check_no_dtmf_event_message (bus);
}

GST_END_TEST;

static GstStaticPadTemplate audio_dtmfsrc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) \"" GST_AUDIO_NE (S16) "\", "
        "rate = (int) 8003, " "channels = (int) 1")
    );
static void
setup_dtmfsrc (void)
{
  testclock = gst_test_clock_new ();
  bus = gst_bus_new ();

  method = 2;
  dtmfsrc = gst_check_setup_element ("dtmfsrc");
  sink = gst_check_setup_sink_pad (dtmfsrc, &audio_dtmfsrc_sink_template);
  gst_element_set_bus (dtmfsrc, bus);
  fail_unless (gst_element_set_clock (dtmfsrc, testclock));

  gst_pad_set_active (sink, TRUE);
  fail_unless (gst_element_set_state (dtmfsrc, GST_STATE_PLAYING) ==
      GST_STATE_CHANGE_SUCCESS);
}


GST_START_TEST (test_dtmfsrc_min_duration)
{
  GstStructure *s;
  GstClockID id;
  GstClockTime timestamp = 0;
  GstCaps *expected_caps, *caps;
  guint interval;

  g_object_get (dtmfsrc, "interval", &interval, NULL);
  fail_unless (interval == 50);

  /* Minimum duration dtmf */
  gst_test_clock_set_time (GST_TEST_CLOCK (testclock), 0);

  s = gst_structure_new ("dtmf-event",
      "type", G_TYPE_INT, 1, "number", G_TYPE_INT, 3,
      "method", G_TYPE_INT, 2, "volume", G_TYPE_INT, 8,
      "start", G_TYPE_BOOLEAN, TRUE, NULL);
  fail_unless (gst_pad_push_event (sink,
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
              gst_structure_copy (s))));

  gst_test_clock_wait_for_next_pending_id (GST_TEST_CLOCK (testclock), NULL);
  id = gst_test_clock_process_next_clock_id (GST_TEST_CLOCK (testclock));
  fail_unless (gst_clock_id_get_time (id) == 0);
  gst_clock_id_unref (id);

  gst_structure_set_name (s, "dtmf-event-processed");
  check_message_structure (s);

  s = gst_structure_new ("dtmf-event",
      "type", G_TYPE_INT, 1, "method", G_TYPE_INT, 2,
      "start", G_TYPE_BOOLEAN, FALSE, NULL);
  fail_unless (gst_pad_push_event (sink,
          gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
              gst_structure_copy (s))));

  for (timestamp = interval * GST_MSECOND;
      timestamp < (MIN_PULSE_DURATION + MIN_INTER_DIGIT_INTERVAL) *
      GST_MSECOND; timestamp += GST_MSECOND * interval) {
    gst_test_clock_wait_for_next_pending_id (GST_TEST_CLOCK (testclock), NULL);
    gst_test_clock_advance_time (GST_TEST_CLOCK (testclock),
        interval * GST_MSECOND);

    id = gst_test_clock_process_next_clock_id (GST_TEST_CLOCK (testclock));
    fail_unless (gst_clock_id_get_time (id) == timestamp);
    gst_clock_id_unref (id);
  }

  gst_structure_set_name (s, "dtmf-event-processed");
  check_message_structure (s);

  check_buffers_duration ((MIN_PULSE_DURATION + MIN_INTER_DIGIT_INTERVAL) *
      GST_MSECOND);

  fail_unless (gst_test_clock_peek_id_count (GST_TEST_CLOCK (testclock)) == 0);

  /* caps check */

  expected_caps = gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, GST_AUDIO_NE (S16),
      "rate", G_TYPE_INT, 8003, "channels", G_TYPE_INT, 1, NULL);
  caps = gst_pad_get_current_caps (sink);
  fail_unless (gst_caps_can_intersect (caps, expected_caps));
  gst_caps_unref (caps);
  gst_caps_unref (expected_caps);

  gst_element_set_state (dtmfsrc, GST_STATE_NULL);

  check_no_dtmf_event_message (bus);
}

GST_END_TEST;

static Suite *
dtmf_suite (void)
{
  Suite *s = suite_create ("dtmf");
  TCase *tc;

  tc = tcase_create ("rtpdtmfdepay");
  tcase_add_test (tc, test_rtpdtmfdepay);
  suite_add_tcase (s, tc);

  tc = tcase_create ("rtpdtmfsrc");
  tcase_add_checked_fixture (tc, setup_rtpdtmfsrc, teardown_dtmfsrc);
  tcase_add_test (tc, test_dtmfsrc_invalid_events);
  tcase_add_test (tc, test_rtpdtmfsrc_min_duration);
  suite_add_tcase (s, tc);

  tc = tcase_create ("dtmfsrc");
  tcase_add_checked_fixture (tc, setup_dtmfsrc, teardown_dtmfsrc);
  tcase_add_test (tc, test_dtmfsrc_invalid_events);
  tcase_add_test (tc, test_dtmfsrc_min_duration);
  suite_add_tcase (s, tc);

  return s;
}


GST_CHECK_MAIN (dtmf);
