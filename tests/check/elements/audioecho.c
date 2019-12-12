/* GStreamer
 *
 * Copyright (C) 2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
#include <gst/audio/audio.h>

gboolean have_eos = FALSE;

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
GstPad *mysrcpad, *mysinkpad;

#define ECHO_CAPS_STRING    \
    "audio/x-raw, "               \
    "channels = (int) 2, "              \
    "channel-mask = (bitmask) 3, "      \
    "rate = (int) 100000, "             \
    "layout = (string) interleaved, "   \
    "format = (string) " GST_AUDIO_NE(F64)

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "channels = (int) [ 1, 2 ], "
        "rate = (int) [ 1,  MAX ], "
        "format = (string) { "
        GST_AUDIO_NE (F32) ", " GST_AUDIO_NE (F64) " }"));
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "channels = (int) [ 1, 2 ], "
        "rate = (int) [ 1,  MAX ], "
        "format = (string) { "
        GST_AUDIO_NE (F32) ", " GST_AUDIO_NE (F64) " }"));

static GstElement *
setup_echo (void)
{
  GstElement *echo;

  GST_DEBUG ("setup_echo");
  echo = gst_check_setup_element ("audioecho");
  mysrcpad = gst_check_setup_src_pad (echo, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (echo, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return echo;
}

static void
cleanup_echo (GstElement * echo)
{
  GST_DEBUG ("cleanup_echo");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (echo);
  gst_check_teardown_sink_pad (echo);
  gst_check_teardown_element (echo);
}

GST_START_TEST (test_passthrough)
{
  GstElement *echo;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble in[] = { 1.0, -1.0, 0.0, 0.5, -0.5, 0.0 };
  gdouble res[6];

  echo = setup_echo ();
  g_object_set (G_OBJECT (echo), "delay", (GstClockTime) 1, "intensity", 0.0,
      "feedback", 0.0, NULL);
  fail_unless (gst_element_set_state (echo,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (ECHO_CAPS_STRING);
  gst_check_setup_events (mysrcpad, echo, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, sizeof (in), 0,
      sizeof (in), NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, sizeof (in)) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... but it ends up being collected on the global buffer list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res,
          sizeof (res)) == sizeof (res));
  GST_INFO
      ("expected %+lf %+lf %+lf %+lf %+lf %+lf real %+lf %+lf %+lf %+lf %+lf %+lf",
      in[0], in[1], in[2], in[3], in[4], in[5], res[0], res[1], res[2], res[3],
      res[4], res[5]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, in, sizeof (in)) == 0);

  /* cleanup */
  cleanup_echo (echo);
}

GST_END_TEST;

GST_START_TEST (test_echo)
{
  GstElement *echo;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble in[] = { 1.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, };
  gdouble out[] = { 1.0, -1.0, 0.0, 0.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0 };
  gdouble res[10];

  echo = setup_echo ();
  g_object_set (G_OBJECT (echo), "delay", (GstClockTime) 20000, "intensity",
      1.0, "feedback", 0.0, NULL);
  fail_unless (gst_element_set_state (echo,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (ECHO_CAPS_STRING);
  gst_check_setup_events (mysrcpad, echo, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, sizeof (in), 0,
      sizeof (in), NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, sizeof (in)) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... but it ends up being collected on the global buffer list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res,
          sizeof (res)) == sizeof (res));
  GST_INFO
      ("expected %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf real %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf",
      out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8],
      out[9], res[0], res[1], res[2], res[3], res[4], res[5], res[6], res[7],
      res[8], res[9]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, sizeof (out)) == 0);

  /* cleanup */
  cleanup_echo (echo);
}

GST_END_TEST;

GST_START_TEST (test_feedback)
{
  GstElement *echo;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble in[] = { 1.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, };
  gdouble out[] = { 1.0, -1.0, 0.0, 0.0, 1.0, -1.0, 0.0, 0.0, 1.0, -1.0 };
  gdouble res[10];

  echo = setup_echo ();
  g_object_set (G_OBJECT (echo), "delay", (GstClockTime) 20000, "intensity",
      1.0, "feedback", 1.0, NULL);
  fail_unless (gst_element_set_state (echo,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (ECHO_CAPS_STRING);
  gst_check_setup_events (mysrcpad, echo, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, sizeof (in), 0,
      sizeof (in), NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, sizeof (in)) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... but it ends up being collected on the global buffer list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res,
          sizeof (res)) == sizeof (res));
  GST_INFO
      ("expected %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf real %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf %+lf",
      out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8],
      out[9], res[0], res[1], res[2], res[3], res[4], res[5], res[6], res[7],
      res[8], res[9]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, sizeof (out)) == 0);

  /* cleanup */
  cleanup_echo (echo);
}

GST_END_TEST;

static Suite *
audioecho_suite (void)
{
  Suite *s = suite_create ("audioecho");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_passthrough);
  tcase_add_test (tc_chain, test_echo);
  tcase_add_test (tc_chain, test_feedback);

  return s;
}

GST_CHECK_MAIN (audioecho);
