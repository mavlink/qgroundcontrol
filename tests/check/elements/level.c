/* GStreamer
 *
 * unit test for level
 *
 * Copyright (C) <2005> Thomas Vander Stichele <thomas at apestaart dot org>
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

/* suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <gst/audio/audio.h>
#include <gst/check/gstcheck.h>

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
static GstPad *mysrcpad, *mysinkpad;

#define LEVEL_CAPS_TEMPLATE_STRING \
  "audio/x-raw, " \
    "format = (string) { "GST_AUDIO_NE(S16)", "GST_AUDIO_NE(F32)" }, " \
    "layout = (string) interleaved, " \
    "rate = (int) [ 1, MAX ], " \
    "channels = (int) [ 1, 8 ]"

/* we use rate = 1000 here for easy buffer size calculations */
#define LEVEL_S16_CAPS_STRING \
  "audio/x-raw, " \
    "format = (string) "GST_AUDIO_NE(S16)", " \
    "layout = (string) interleaved, " \
    "rate = (int) 1000, " \
    "channels = (int) 2, "  \
    "channel-mask = (bitmask) 3"

#define LEVEL_F32_CAPS_STRING \
  "audio/x-raw, " \
    "format = (string) "GST_AUDIO_NE(F32)", " \
    "layout = (string) interleaved, " \
    "rate = (int) 1000, " \
    "channels = (int) 2, "  \
    "channel-mask = (bitmask) 3"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (LEVEL_CAPS_TEMPLATE_STRING)
    );
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (LEVEL_CAPS_TEMPLATE_STRING)
    );

/* takes over reference for outcaps */
static GstElement *
setup_level (const gchar * caps_str)
{
  GstElement *level;
  GstCaps *caps;

  GST_DEBUG ("setup_level");
  level = gst_check_setup_element ("level");
  mysrcpad = gst_check_setup_src_pad (level, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (level, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  caps = gst_caps_from_string (caps_str);
  gst_check_setup_events (mysrcpad, level, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  return level;
}

static void
cleanup_level (GstElement * level)
{
  GST_DEBUG ("cleanup_level");

  gst_check_drop_buffers ();
  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (level);
  gst_check_teardown_sink_pad (level);
  gst_check_teardown_element (level);
}

/* create a 0.1 sec buffer stereo buffer */
static GstBuffer *
create_s16_buffer (gint16 val_l, gint16 val_r)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (2 * 100 * sizeof (gint16));
  GstMapInfo map;
  gint j;
  gint16 *data;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gint16 *) map.data;
  for (j = 0; j < 100; ++j) {
    *(data++) = val_l;
    *(data++) = val_r;
  }
  gst_buffer_unmap (buf, &map);
  GST_BUFFER_TIMESTAMP (buf) = G_GUINT64_CONSTANT (0);
  return buf;
}

static GstBuffer *
create_f32_buffer (gfloat val_l, gfloat val_r)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (2 * 100 * sizeof (gfloat));
  GstMapInfo map;
  gint j;
  gfloat *data;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gfloat *) map.data;
  for (j = 0; j < 100; ++j) {
    *(data++) = val_l;
    *(data++) = val_r;
  }
  gst_buffer_unmap (buf, &map);
  GST_BUFFER_TIMESTAMP (buf) = G_GUINT64_CONSTANT (0);
  return buf;
}

/* tests */

GST_START_TEST (test_ref_counts)
{
  GstElement *level;
  GstBuffer *inbuffer, *outbuffer;
  GstBus *bus;
  GstMessage *message;

  level = setup_level (LEVEL_S16_CAPS_STRING);
  g_object_set (level, "post-messages", TRUE,
      "interval", (guint64) GST_SECOND / 10, NULL);
  fail_unless (gst_element_set_state (level,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");
  /* create a bus to get the level message on */
  bus = gst_bus_new ();
  ASSERT_OBJECT_REFCOUNT (bus, "bus", 1);
  gst_element_set_bus (level, bus);
  ASSERT_OBJECT_REFCOUNT (bus, "bus", 2);

  /* create a fake 0.1 sec buffer with a half-amplitude block signal */
  inbuffer = create_s16_buffer (16536, 16536);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... but it ends up being collected on the global buffer list */
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);
  fail_unless (inbuffer == outbuffer);

  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, -1);
  ASSERT_OBJECT_REFCOUNT (message, "message", 1);

  fail_unless (message != NULL);
  fail_unless (GST_MESSAGE_SRC (message) == GST_OBJECT (level));
  fail_unless (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ELEMENT);

  /* clean up */
  /* flush current messages,and future state change messages */
  gst_bus_set_flushing (bus, TRUE);

  /* message has a ref to the element */
  ASSERT_OBJECT_REFCOUNT (level, "level", 2);
  gst_message_unref (message);
  ASSERT_OBJECT_REFCOUNT (level, "level", 1);

  gst_element_set_bus (level, NULL);
  ASSERT_OBJECT_REFCOUNT (bus, "bus", 1);
  gst_object_unref (bus);
  fail_unless (gst_element_set_state (level,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS, "could not set to null");
  ASSERT_OBJECT_REFCOUNT (level, "level", 1);
  cleanup_level (level);
}

GST_END_TEST;

GST_START_TEST (test_message_is_valid)
{
  GstElement *level;
  GstBuffer *inbuffer;
  GstBus *bus;
  GstMessage *message;
  const GstStructure *structure;
  GstClockTime endtime, ts, duration;

  level = setup_level (LEVEL_S16_CAPS_STRING);
  g_object_set (level, "post-messages", TRUE,
      "interval", (guint64) GST_SECOND / 10, NULL);
  gst_element_set_state (level, GST_STATE_PLAYING);
  /* create a bus to get the level message on */
  bus = gst_bus_new ();
  gst_element_set_bus (level, bus);

  /* create a fake 0.1 sec buffer with a half-amplitude block signal and push */
  inbuffer = create_s16_buffer (16536, 16536);
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);

  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, -1);
  fail_unless (message != NULL);
  structure = gst_message_get_structure (message);
  fail_if (structure == NULL);
  fail_unless_equals_string (gst_structure_get_name (structure), "level");
  fail_unless (gst_structure_get_clock_time (structure, "endtime", &endtime));
  fail_unless (gst_structure_get_clock_time (structure, "timestamp", &ts));
  fail_unless (gst_structure_get_clock_time (structure, "duration", &duration));

  /* clean up */
  /* flush current messages,and future state change messages */
  gst_bus_set_flushing (bus, TRUE);
  gst_message_unref (message);
  gst_element_set_bus (level, NULL);
  gst_object_unref (bus);
  gst_element_set_state (level, GST_STATE_NULL);
  cleanup_level (level);
}

GST_END_TEST;

GST_START_TEST (test_int16)
{
  GstElement *level;
  GstBuffer *inbuffer, *outbuffer;
  GstBus *bus;
  GstMessage *message;
  const GstStructure *structure;
  gint i, j;
  const GValue *list, *value;
  gdouble dB;
  const gchar *fields[3] = { "rms", "peak", "decay" };

  level = setup_level (LEVEL_S16_CAPS_STRING);
  g_object_set (level, "post-messages", TRUE,
      "interval", (guint64) GST_SECOND / 10, NULL);
  gst_element_set_state (level, GST_STATE_PLAYING);
  /* create a bus to get the level message on */
  bus = gst_bus_new ();
  gst_element_set_bus (level, bus);

  /* create a fake 0.1 sec buffer with a half-amplitude block signal */
  inbuffer = create_s16_buffer (16536, 16536);

  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);
  fail_unless (inbuffer == outbuffer);

  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, -1);
  structure = gst_message_get_structure (message);

  /* block wave of half amplitude has -5.94 dB for rms, peak and decay */
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < 3; ++j) {
      GValueArray *arr;

      list = gst_structure_get_value (structure, fields[j]);
      arr = g_value_get_boxed (list);
      value = g_value_array_get_nth (arr, i);
      dB = g_value_get_double (value);
      GST_DEBUG ("%s is %lf", fields[j], dB);
      fail_if (dB < -6.1);
      fail_if (dB > -5.9);
    }
  }

  /* clean up */
  /* flush current messages,and future state change messages */
  gst_bus_set_flushing (bus, TRUE);
  gst_message_unref (message);
  gst_element_set_bus (level, NULL);
  gst_object_unref (bus);
  gst_element_set_state (level, GST_STATE_NULL);
  cleanup_level (level);
}

GST_END_TEST;

GST_START_TEST (test_int16_panned)
{
  GstElement *level;
  GstBuffer *inbuffer, *outbuffer;
  GstBus *bus;
  GstMessage *message;
  const GstStructure *structure;
  gint i, j;
  const GValue *list, *value;
  gdouble dB;
  const gchar *fields[3] = { "rms", "peak", "decay" };

  level = setup_level (LEVEL_S16_CAPS_STRING);
  g_object_set (level, "post-messages", TRUE,
      "interval", (guint64) GST_SECOND / 30, NULL);
  gst_element_set_state (level, GST_STATE_PLAYING);
  /* create a bus to get the level message on */
  bus = gst_bus_new ();
  gst_element_set_bus (level, bus);

  /* create a fake 0.1 sec buffer with a half-amplitude block signal */
  inbuffer = create_s16_buffer (0, 16536);

  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);
  fail_unless (inbuffer == outbuffer);

  /* do multiple messages per buffer, to verify that the inner loop in level
   * advances the read-index correctly, see
   * https://bugzilla.gnome.org/show_bug.cgi?id=754144
   */
  for (i = 0; i < 3; i++) {
    GST_DEBUG ("get message number %d", i);
    message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, -1);
    structure = gst_message_get_structure (message);

    /* silence has 0 dB for rms, peak and decay */
    for (j = 0; j < 3; ++j) {
      GValueArray *arr;

      list = gst_structure_get_value (structure, fields[j]);
      arr = g_value_get_boxed (list);
      value = g_value_array_get_nth (arr, 0);
      dB = g_value_get_double (value);
      GST_DEBUG ("%s[0] is %lf", fields[j], dB);
#ifdef HAVE_ISINF
      fail_unless (isinf (dB));
#elif defined (HAVE_FPCLASS)
      fail_unless (fpclass (dB) == FP_NINF);
#endif
    }
    /* block wave of half amplitude has -5.94 dB for rms, peak and decay */
    for (j = 0; j < 3; ++j) {
      GValueArray *arr;

      list = gst_structure_get_value (structure, fields[j]);
      arr = g_value_get_boxed (list);
      value = g_value_array_get_nth (arr, 1);
      dB = g_value_get_double (value);
      GST_DEBUG ("%s[1] is %lf", fields[j], dB);
      fail_if (dB < -6.1);
      fail_if (dB > -5.9);
    }
    gst_message_unref (message);
  }

  /* clean up */
  /* flush current messages,and future state change messages */
  gst_bus_set_flushing (bus, TRUE);
  gst_element_set_bus (level, NULL);
  gst_object_unref (bus);
  gst_element_set_state (level, GST_STATE_NULL);
  cleanup_level (level);
}

GST_END_TEST;

GST_START_TEST (test_float)
{
  GstElement *level;
  GstBuffer *inbuffer, *outbuffer;
  GstBus *bus;
  GstMessage *message;
  const GstStructure *structure;
  gint i, j;
  const GValue *list, *value;
  gdouble dB;
  const gchar *fields[3] = { "rms", "peak", "decay" };

  level = setup_level (LEVEL_F32_CAPS_STRING);
  g_object_set (level, "post-messages", TRUE,
      "interval", (guint64) GST_SECOND / 10, NULL);
  gst_element_set_state (level, GST_STATE_PLAYING);
  /* create a bus to get the level message on */
  bus = gst_bus_new ();
  gst_element_set_bus (level, bus);

  /* create a fake 0.1 sec buffer with a half-amplitude block signal */
  inbuffer = create_f32_buffer (0.5, 0.5);

  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);
  fail_unless (inbuffer == outbuffer);

  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, -1);
  structure = gst_message_get_structure (message);

  /* block wave of half amplitude has -5.94 dB for rms, peak and decay */
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < 3; ++j) {
      GValueArray *arr;

      list = gst_structure_get_value (structure, fields[j]);
      arr = g_value_get_boxed (list);
      value = g_value_array_get_nth (arr, i);
      dB = g_value_get_double (value);
      GST_DEBUG ("%s is %lf", fields[j], dB);
      fail_if (dB < -6.1);
      fail_if (dB > -5.9);
    }
  }

  /* clean up */
  /* flush current messages,and future state change messages */
  gst_bus_set_flushing (bus, TRUE);
  gst_message_unref (message);
  gst_element_set_bus (level, NULL);
  gst_object_unref (bus);
  gst_element_set_state (level, GST_STATE_NULL);
  cleanup_level (level);
}

GST_END_TEST;

GST_START_TEST (test_message_on_eos)
{
  GstElement *level;
  GstBuffer *inbuffer, *outbuffer;
  GstEvent *event;
  GstBus *bus;
  GstMessage *message;
  const GstStructure *structure;
  gint i, j;
  const GValue *list, *value;
  gdouble dB;

  level = setup_level (LEVEL_S16_CAPS_STRING);
  g_object_set (level, "post-messages", TRUE,
      "interval", (guint64) GST_SECOND / 5, NULL);
  gst_element_set_state (level, GST_STATE_PLAYING);
  /* create a bus to get the level message on */
  bus = gst_bus_new ();
  gst_element_set_bus (level, bus);

  /* create a fake 0.1 sec buffer with a half-amplitude block signal */
  inbuffer = create_s16_buffer (16536, 16536);

  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);
  fail_unless (inbuffer == outbuffer);

  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, 0);

  event = gst_event_new_eos ();
  fail_unless (gst_pad_push_event (mysrcpad, event) == TRUE);

  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, 0);
  fail_unless (message != NULL);
  structure = gst_message_get_structure (message);
  fail_unless_equals_string (gst_structure_get_name (structure), "level");

  /* block wave of half amplitude has -5.94 dB for rms, peak and decay */
  for (i = 0; i < 2; ++i) {
    const gchar *fields[3] = { "rms", "peak", "decay" };
    for (j = 0; j < 3; ++j) {
      GValueArray *arr;

      list = gst_structure_get_value (structure, fields[j]);
      arr = g_value_get_boxed (list);
      value = g_value_array_get_nth (arr, i);
      dB = g_value_get_double (value);
      GST_DEBUG ("%s is %lf", fields[j], dB);
      fail_if (dB < -6.1);
      fail_if (dB > -5.9);
    }
  }

  /* clean up */
  /* flush current messages,and future state change messages */
  gst_bus_set_flushing (bus, TRUE);
  gst_message_unref (message);
  gst_element_set_bus (level, NULL);
  gst_object_unref (bus);
  gst_element_set_state (level, GST_STATE_NULL);
  cleanup_level (level);
}

GST_END_TEST;

GST_START_TEST (test_message_count)
{
  GstElement *level;
  GstBuffer *inbuffer, *outbuffer;
  GstBus *bus;
  GstMessage *message;

  level = setup_level (LEVEL_S16_CAPS_STRING);
  g_object_set (level, "post-messages", TRUE,
      "interval", (guint64) GST_SECOND / 20, NULL);
  gst_element_set_state (level, GST_STATE_PLAYING);
  /* create a bus to get the level message on */
  bus = gst_bus_new ();
  gst_element_set_bus (level, bus);

  /* create a fake 0.1 sec buffer with a half-amplitude block signal */
  inbuffer = create_s16_buffer (16536, 16536);

  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);
  fail_unless (inbuffer == outbuffer);

  /* we should get two messages per buffer */
  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, -1);
  fail_unless (message != NULL);
  gst_message_unref (message);
  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, -1);
  fail_unless (message != NULL);
  gst_message_unref (message);

  gst_element_set_bus (level, NULL);
  gst_object_unref (bus);
  gst_element_set_state (level, GST_STATE_NULL);
  cleanup_level (level);
}

GST_END_TEST;

GST_START_TEST (test_message_timestamps)
{
  GstElement *level;
  GstBuffer *inbuffer, *outbuffer;
  GstBus *bus;
  GstMessage *message;
  const GstStructure *structure;
  GstClockTime ts1, dur1, ts2;

  level = setup_level (LEVEL_S16_CAPS_STRING);
  g_object_set (level, "post-messages", TRUE,
      "interval", (guint64) GST_SECOND / 20, NULL);
  gst_element_set_state (level, GST_STATE_PLAYING);
  /* create a bus to get the level message on */
  bus = gst_bus_new ();
  gst_element_set_bus (level, bus);

  /* create a fake 0.1 sec buffer with a half-amplitude block signal */
  inbuffer = create_s16_buffer (16536, 16536);

  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);
  fail_unless (inbuffer == outbuffer);

  /* check that timestamp + duration is contiguous to the next timestamp */
  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, -1);
  structure = gst_message_get_structure (message);
  gst_structure_get_clock_time (structure, "timestamp", &ts1);
  gst_structure_get_clock_time (structure, "duration", &dur1);
  gst_message_unref (message);

  message = gst_bus_poll (bus, GST_MESSAGE_ELEMENT, -1);
  structure = gst_message_get_structure (message);
  gst_structure_get_clock_time (structure, "timestamp", &ts2);
  gst_message_unref (message);

  fail_unless_equals_int64 (ts1 + dur1, ts2);

  gst_element_set_bus (level, NULL);
  gst_object_unref (bus);
  gst_element_set_state (level, GST_STATE_NULL);
  cleanup_level (level);
}

GST_END_TEST;

static Suite *
level_suite (void)
{
  Suite *s = suite_create ("level");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_ref_counts);
  tcase_add_test (tc_chain, test_message_is_valid);
  tcase_add_test (tc_chain, test_int16);
  tcase_add_test (tc_chain, test_int16_panned);
  tcase_add_test (tc_chain, test_float);
  tcase_add_test (tc_chain, test_message_on_eos);
  tcase_add_test (tc_chain, test_message_count);
  tcase_add_test (tc_chain, test_message_timestamps);

  return s;
}

GST_CHECK_MAIN (level);
