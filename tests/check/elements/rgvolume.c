/* GStreamer ReplayGain volume adjustment
 *
 * Copyright (C) 2007 Rene Stadler <mail@renestadler.de>
 *
 * rgvolume.c: Unit test for the rgvolume element
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

#include <gst/check/gstcheck.h>
#include <gst/audio/audio.h>

#include <math.h>

static GList *events = NULL;

/* For ease of programming we use globals to keep refs for our floating src and
 * sink pads we create; otherwise we always have to do get_pad, get_peer, and
 * then remove references in every test function */
static GstPad *mysrcpad, *mysinkpad;

#define RG_VOLUME_CAPS_TEMPLATE_STRING        \
  "audio/x-raw, "                             \
  "format = (string) "GST_AUDIO_NE (F32) ", " \
  "layout = (string) interleaved, "           \
  "channels = (int) [ 1, MAX ], "             \
  "rate = (int) [ 1, MAX ]"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (RG_VOLUME_CAPS_TEMPLATE_STRING)
    );
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (RG_VOLUME_CAPS_TEMPLATE_STRING)
    );

static GstBuffer *test_buffer_new (gfloat value);

/* gstcheck sets up a chain function that appends buffers to a global list.
 * This is our equivalent of that for event handling. */
static gboolean
event_func (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GST_DEBUG ("received event %p (%s)", event, GST_EVENT_TYPE_NAME (event));
  events = g_list_append (events, event);

  return TRUE;
}

static GstElement *
setup_rgvolume (void)
{
  GstElement *element;

  GST_DEBUG ("setup_rgvolume");
  element = gst_check_setup_element ("rgvolume");
  mysrcpad = gst_check_setup_src_pad (element, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (element, &sinktemplate);

  /* Capture events, to test tag filtering behavior: */
  gst_pad_set_event_function (mysinkpad, event_func);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return element;
}

static void
send_empty_buffer (void)
{
  GstBuffer *buf;

  buf = test_buffer_new (0.0);
  gst_buffer_resize (buf, 0, 0);
  GST_BUFFER_DURATION (buf) = 0;
  GST_BUFFER_OFFSET_END (buf) = GST_BUFFER_OFFSET (buf);
  fail_unless (gst_pad_push (mysrcpad, buf) == GST_FLOW_OK);

  fail_unless (g_list_length (buffers) == 1);
  fail_unless (buffers->data == buf);
  gst_mini_object_unref ((GstMiniObject *) buffers->data);
  buffers = g_list_remove (buffers, buf);
}

static void
cleanup_rgvolume (GstElement * element)
{
  GST_DEBUG ("cleanup_rgvolume");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  g_list_foreach (events, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (events);
  events = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (element);
  gst_check_teardown_sink_pad (element);
  gst_check_teardown_element (element);
}

static void
set_playing_state (GstElement * element)
{
  fail_unless (gst_element_set_state (element,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "Could not set state to PLAYING");
}

static void
set_null_state (GstElement * element)
{
  fail_unless (gst_element_set_state (element,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS,
      "Could not set state to NULL");
}

static void
send_flush_events (GstElement * element)
{
  gboolean res;

  res = gst_pad_push_event (mysrcpad, gst_event_new_flush_start ());
  fail_unless (res, "flush-start even not handled");

  res = gst_pad_push_event (mysrcpad, gst_event_new_flush_stop (TRUE));
  fail_unless (res, "flush-stop event not handled");
}

static void
send_stream_start_event (GstElement * element)
{
  gboolean res;

  res = gst_pad_push_event (mysrcpad, gst_event_new_stream_start ("test"));
  fail_unless (res, "STREAM_START event not handled");
}

static void
send_caps_event (GstElement * element)
{
  GstCaps *caps;
  gboolean res;

  caps = gst_caps_from_string ("audio/x-raw, format = " GST_AUDIO_NE (F32) ", "
      "layout = interleaved, rate = 8000, channels = 1");
  res = gst_pad_push_event (mysrcpad, gst_event_new_caps (caps));
  fail_unless (res, "CAPS event not handled");
  gst_caps_unref (caps);
}

static void
send_segment_event (GstElement * element)
{
  GstSegment segment;
  gboolean res;

  gst_segment_init (&segment, GST_FORMAT_TIME);
  res = gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment));
  fail_unless (res, "SEGMENT event not handled");
}

static void
send_eos_event (GstElement * element)
{
  GstEvent *event = gst_event_new_eos ();

  fail_unless (gst_pad_push_event (mysrcpad, event),
      "Pushing EOS event failed");
}

static GstEvent *
send_tag_event (GstElement * element, GstEvent * event)
{
  GList *l;
  GstTagList *tag_list;
  gdouble dummy;

  g_return_val_if_fail (event->type == GST_EVENT_TAG, NULL);

  fail_unless (gst_pad_push_event (mysrcpad, event),
      "Pushing tag event failed");

  event = NULL;

  for (l = g_list_last (events); l; l = l->prev) {
    if (GST_EVENT_TYPE (l->data) == GST_EVENT_TAG) {
      event = l->data;
      events = g_list_delete_link (events, l);
      break;
    }
  }

  /* Event got filtered out */
  if (event == NULL)
    return NULL;

  fail_unless (event->type == GST_EVENT_TAG);
  gst_event_parse_tag (event, &tag_list);

  /* The element is supposed to filter out ReplayGain related tags. */
  fail_if (gst_tag_list_get_double (tag_list, GST_TAG_TRACK_GAIN, &dummy),
      "tag event still contains track gain tag");
  fail_if (gst_tag_list_get_double (tag_list, GST_TAG_TRACK_PEAK, &dummy),
      "tag event still contains track peak tag");
  fail_if (gst_tag_list_get_double (tag_list, GST_TAG_ALBUM_GAIN, &dummy),
      "tag event still contains album gain tag");
  fail_if (gst_tag_list_get_double (tag_list, GST_TAG_ALBUM_PEAK, &dummy),
      "tag event still contains album peak tag");

  return event;
}

static GstBuffer *
test_buffer_new (gfloat value)
{
  GstBuffer *buf;
  GstMapInfo map;
  gfloat *data;
  gint i;

  buf = gst_buffer_new_and_alloc (8 * sizeof (gfloat));
  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gfloat *) map.data;
  for (i = 0; i < 8; i++)
    data[i] = value;
  gst_buffer_unmap (buf, &map);


  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);

  return buf;
}

#define MATCH_GAIN(g1, g2) ((g1 < g2 + 1e-6) && (g2 < g1 + 1e-6))

static void
fail_unless_target_gain (GstElement * element, gdouble expected_gain)
{
  gdouble prop_gain;

  g_object_get (element, "target-gain", &prop_gain, NULL);

  fail_unless (MATCH_GAIN (prop_gain, expected_gain),
      "Target gain is %.2f dB, expected %.2f dB", prop_gain, expected_gain);
}

static void
fail_unless_result_gain (GstElement * element, gdouble expected_gain)
{
  GstBuffer *input_buf, *output_buf;
  gfloat *data;
  gfloat input_sample, output_sample;
  gdouble gain, prop_gain;
  gboolean is_passthrough, expect_passthrough;
  gint i;
  GstMapInfo map;

  fail_unless (g_list_length (buffers) == 0);

  input_sample = 1.0;
  input_buf = test_buffer_new (input_sample);

  /* We keep an extra reference to detect passthrough mode. */
  gst_buffer_ref (input_buf);
  /* Pushing steals a reference. */
  fail_unless (gst_pad_push (mysrcpad, input_buf) == GST_FLOW_OK);
  gst_buffer_unref (input_buf);

  /* The output buffer ends up on the global buffer list. */
  fail_unless (g_list_length (buffers) == 1);
  output_buf = buffers->data;
  fail_if (output_buf == NULL);

  buffers = g_list_remove (buffers, output_buf);
  ASSERT_BUFFER_REFCOUNT (output_buf, "output_buf", 1);

  fail_unless_equals_int (gst_buffer_get_size (output_buf),
      8 * sizeof (gfloat));

  gst_buffer_map (output_buf, &map, GST_MAP_READ);
  data = (gfloat *) map.data;

  output_sample = *data;
  fail_if (output_sample == 0.0, "First output sample is zero");
  for (i = 1; i < 8; i++) {
    fail_unless (output_sample == data[i], "Output samples not uniform");
  };
  gst_buffer_unmap (output_buf, &map);

  gain = 20. * log10 (output_sample / input_sample);
  fail_unless (MATCH_GAIN (gain, expected_gain),
      "Applied gain is %.2f dB, expected %.2f dB", gain, expected_gain);
  g_object_get (element, "result-gain", &prop_gain, NULL);
  fail_unless (MATCH_GAIN (prop_gain, expected_gain),
      "Result gain is %.2f dB, expected %.2f dB", prop_gain, expected_gain);

  is_passthrough = (output_buf == input_buf);
  expect_passthrough = MATCH_GAIN (expected_gain, +0.00);
  fail_unless (is_passthrough == expect_passthrough,
      expect_passthrough
      ? "Expected operation in passthrough mode"
      : "Incorrect passthrough behaviour");

  gst_buffer_unref (output_buf);
}

static void
fail_unless_gain (GstElement * element, gdouble expected_gain)
{
  fail_unless_target_gain (element, expected_gain);
  fail_unless_result_gain (element, expected_gain);
}

/* Start of tests. */

GST_START_TEST (test_no_buffer)
{
  GstElement *element = setup_rgvolume ();

  set_playing_state (element);
  set_null_state (element);
  set_playing_state (element);
  send_eos_event (element);

  cleanup_rgvolume (element);
}

GST_END_TEST;

GST_START_TEST (test_events)
{
  GstElement *element = setup_rgvolume ();
  GstEvent *event;
  GstEvent *new_event;
  GstTagList *tag_list;
  gchar *artist;

  set_playing_state (element);
  send_stream_start_event (element);
  send_caps_event (element);
  send_segment_event (element);

  send_empty_buffer ();

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, +4.95, GST_TAG_TRACK_PEAK, 0.59463,
      GST_TAG_ALBUM_GAIN, -1.54, GST_TAG_ALBUM_PEAK, 0.693415,
      GST_TAG_ARTIST, "Foobar", NULL);
  event = gst_event_new_tag (tag_list);
  new_event = send_tag_event (element, event);
  gst_event_parse_tag (new_event, &tag_list);
  fail_unless (gst_tag_list_get_string (tag_list, GST_TAG_ARTIST, &artist));
  fail_unless (g_str_equal (artist, "Foobar"));
  g_free (artist);
  gst_event_unref (new_event);

  /* Same as above, but with a non-writable event. */

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, +4.95, GST_TAG_TRACK_PEAK, 0.59463,
      GST_TAG_ALBUM_GAIN, -1.54, GST_TAG_ALBUM_PEAK, 0.693415,
      GST_TAG_ARTIST, "Foobar", NULL);
  gst_tag_list_ref (tag_list);
  event = gst_event_new_tag (tag_list);
  new_event = send_tag_event (element, event);

  /* Make sure our tags weren't modified in place while we still got a ref */
  fail_unless_equals_int (5, gst_tag_list_n_tags (tag_list));
  gst_tag_list_unref (tag_list);

  gst_event_parse_tag (new_event, &tag_list);
  fail_unless (gst_tag_list_get_string (tag_list, GST_TAG_ARTIST, &artist));
  fail_unless (g_str_equal (artist, "Foobar"));
  g_free (artist);
  gst_event_unref (new_event);

  cleanup_rgvolume (element);
}

GST_END_TEST;

GST_START_TEST (test_simple)
{
  GstElement *element = setup_rgvolume ();
  GstTagList *tag_list;

  g_object_set (element, "album-mode", FALSE, "headroom", +0.00,
      "pre-amp", -6.00, "fallback-gain", +1.23, NULL);
  set_playing_state (element);
  send_stream_start_event (element);
  send_caps_event (element);
  send_segment_event (element);

  send_empty_buffer ();

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, -3.45, GST_TAG_TRACK_PEAK, 1.0,
      GST_TAG_ALBUM_GAIN, +2.09, GST_TAG_ALBUM_PEAK, 1.0, NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  fail_unless_gain (element, -9.45);    /* pre-amp + track gain */
  send_eos_event (element);

  g_object_set (element, "album-mode", TRUE, NULL);

  send_flush_events (element);
  send_segment_event (element);

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, -3.45, GST_TAG_TRACK_PEAK, 1.0,
      GST_TAG_ALBUM_GAIN, +2.09, GST_TAG_ALBUM_PEAK, 1.0, NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  fail_unless_gain (element, -3.91);    /* pre-amp + album gain */

  /* Switching back to track mode in the middle of a stream: */
  g_object_set (element, "album-mode", FALSE, NULL);
  fail_unless_gain (element, -9.45);    /* pre-amp + track gain */
  send_eos_event (element);

  cleanup_rgvolume (element);
}

GST_END_TEST;

/* If there are no gain tags at all, the fallback gain is used. */

GST_START_TEST (test_fallback_gain)
{
  GstElement *element = setup_rgvolume ();
  GstTagList *tag_list;

  /* First some track where fallback does _not_ apply. */

  g_object_set (element, "album-mode", FALSE, "headroom", 10.00,
      "pre-amp", -6.00, "fallback-gain", -3.00, NULL);
  set_playing_state (element);
  send_stream_start_event (element);
  send_caps_event (element);
  send_segment_event (element);

  send_empty_buffer ();

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, +3.5, GST_TAG_TRACK_PEAK, 1.0,
      GST_TAG_ALBUM_GAIN, -0.5, GST_TAG_ALBUM_PEAK, 1.0, NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  fail_unless_gain (element, -2.50);    /* pre-amp + track gain */
  send_eos_event (element);

  /* Now a track completely missing tags. */
  send_flush_events (element);
  send_segment_event (element);

  fail_unless_gain (element, -9.00);    /* pre-amp + fallback-gain */

  /* Changing the fallback gain in the middle of a stream, going to pass-through
   * mode: */
  g_object_set (element, "fallback-gain", +6.00, NULL);
  fail_unless_gain (element, +0.00);    /* pre-amp + fallback-gain */
  send_eos_event (element);

  /* Verify that result gain is set to +0.00 with pre-amp + fallback-gain >
   * +0.00 and no headroom. */
  send_flush_events (element);
  send_segment_event (element);

  g_object_set (element, "fallback-gain", +12.00, "headroom", +0.00, NULL);
  fail_unless_target_gain (element, +6.00);     /* pre-amp + fallback-gain */
  fail_unless_result_gain (element, +0.00);
  send_eos_event (element);

  cleanup_rgvolume (element);
}

GST_END_TEST;

/* If album gain is to be preferred but not available, the track gain is to be
 * taken instead. */

GST_START_TEST (test_fallback_track)
{
  GstElement *element = setup_rgvolume ();
  GstTagList *tag_list;

  g_object_set (element, "album-mode", TRUE, "headroom", +0.00,
      "pre-amp", -6.00, "fallback-gain", +1.23, NULL);
  set_playing_state (element);
  send_stream_start_event (element);
  send_caps_event (element);
  send_segment_event (element);

  send_empty_buffer ();

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, +2.11, GST_TAG_TRACK_PEAK, 1.0, NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  fail_unless_gain (element, -3.89);    /* pre-amp + track gain */

  send_eos_event (element);

  cleanup_rgvolume (element);
}

GST_END_TEST;

/* If track gain is to be preferred but not available, the album gain is to be
 * taken instead. */

GST_START_TEST (test_fallback_album)
{
  GstElement *element = setup_rgvolume ();
  GstTagList *tag_list;

  g_object_set (element, "album-mode", FALSE, "headroom", +0.00,
      "pre-amp", -6.00, "fallback-gain", +1.23, NULL);
  set_playing_state (element);
  send_stream_start_event (element);
  send_caps_event (element);
  send_segment_event (element);

  send_empty_buffer ();

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_ALBUM_GAIN, +3.73, GST_TAG_ALBUM_PEAK, 1.0, NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  fail_unless_gain (element, -2.27);    /* pre-amp + album gain */

  send_eos_event (element);

  cleanup_rgvolume (element);
}

GST_END_TEST;

GST_START_TEST (test_headroom)
{
  GstElement *element = setup_rgvolume ();
  GstTagList *tag_list;

  g_object_set (element, "album-mode", FALSE, "headroom", +0.00,
      "pre-amp", +0.00, "fallback-gain", +1.23, NULL);
  set_playing_state (element);
  send_stream_start_event (element);
  send_caps_event (element);
  send_segment_event (element);

  send_empty_buffer ();

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, +3.50, GST_TAG_TRACK_PEAK, 1.0, NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  fail_unless_target_gain (element, +3.50);     /* pre-amp + track gain */
  fail_unless_result_gain (element, +0.00);
  send_eos_event (element);

  send_flush_events (element);
  send_segment_event (element);

  g_object_set (element, "headroom", +2.00, NULL);
  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, +9.18, GST_TAG_TRACK_PEAK, 0.687149, NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  fail_unless_target_gain (element, +9.18);     /* pre-amp + track gain */
  /* Result is 20. * log10 (1. / peak) + headroom. */
  fail_unless_result_gain (element, 5.2589816238303335);
  send_eos_event (element);

  send_flush_events (element);
  send_segment_event (element);

  g_object_set (element, "album-mode", TRUE, NULL);
  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_ALBUM_GAIN, +5.50, GST_TAG_ALBUM_PEAK, 1.0, NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  fail_unless_target_gain (element, +5.50);     /* pre-amp + album gain */
  fail_unless_result_gain (element, +2.00);     /* headroom */
  send_eos_event (element);

  cleanup_rgvolume (element);
}

GST_END_TEST;

GST_START_TEST (test_reference_level)
{
  GstElement *element = setup_rgvolume ();
  GstTagList *tag_list;

  g_object_set (element,
      "album-mode", FALSE,
      "headroom", +0.00, "pre-amp", +0.00, "fallback-gain", +1.23, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (element);
  send_segment_event (element);

  send_empty_buffer ();

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, 0.00, GST_TAG_TRACK_PEAK, 0.2,
      GST_TAG_REFERENCE_LEVEL, 83., NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  /* Because our authoritative reference is 89 dB, we bump it up by +6 dB. */
  fail_unless_gain (element, +6.00);    /* pre-amp + track gain */
  send_eos_event (element);

  g_object_set (element, "album-mode", TRUE, NULL);

  /* Same as above, but with album gain. */
  send_flush_events (element);
  send_segment_event (element);

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
      GST_TAG_TRACK_GAIN, 1.23, GST_TAG_TRACK_PEAK, 0.1,
      GST_TAG_ALBUM_GAIN, 0.00, GST_TAG_ALBUM_PEAK, 0.2,
      GST_TAG_REFERENCE_LEVEL, 83., NULL);
  fail_unless (send_tag_event (element, gst_event_new_tag (tag_list)) == NULL);
  fail_unless_gain (element, +6.00);    /* pre-amp + album gain */

  cleanup_rgvolume (element);
}

GST_END_TEST;

static Suite *
rgvolume_suite (void)
{
  Suite *s = suite_create ("rgvolume");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_no_buffer);
  tcase_add_test (tc_chain, test_events);
  tcase_add_test (tc_chain, test_simple);
  tcase_add_test (tc_chain, test_fallback_gain);
  tcase_add_test (tc_chain, test_fallback_track);
  tcase_add_test (tc_chain, test_fallback_album);
  tcase_add_test (tc_chain, test_headroom);
  tcase_add_test (tc_chain, test_reference_level);

  return s;
}

GST_CHECK_MAIN (rgvolume);
