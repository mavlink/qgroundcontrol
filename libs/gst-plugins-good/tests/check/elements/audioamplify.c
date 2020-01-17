/* GStreamer
 *
 * unit test for audioamplify
 *
 * Copyright (C) 2007 Sebastian Dröge <slomo@circular-chaos.org>
 *
 * Greatly based on the audiopanorama unit test
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
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

#include <gst/base/gstbasetransform.h>
#include <gst/check/gstcheck.h>
#include <gst/audio/audio.h>

gboolean have_eos = FALSE;

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
GstPad *mysrcpad, *mysinkpad;


#define AMPLIFY_CAPS_STRING    \
    "audio/x-raw, "                     \
    "channels = (int) 1, "              \
    "rate = (int) 44100, "              \
    "layout = (string) interleaved, "   \
    "format = (string) " GST_AUDIO_NE(S16)

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "channels = (int) 1, "
        "rate = (int) [ 1,  MAX ], "
        "layout = (string) interleaved, "
        "format = (string) " GST_AUDIO_NE (S16)));
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "channels = (int) 1, "
        "rate = (int) [ 1,  MAX ], "
        "layout = (string) interleaved, "
        "format = (string) " GST_AUDIO_NE (S16)));

static GstElement *
setup_amplify (void)
{
  GstElement *amplify;

  GST_DEBUG ("setup_amplify");
  amplify = gst_check_setup_element ("audioamplify");
  mysrcpad = gst_check_setup_src_pad (amplify, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (amplify, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return amplify;
}

static void
cleanup_amplify (GstElement * amplify)
{
  GST_DEBUG ("cleanup_amplify");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (amplify);
  gst_check_teardown_sink_pad (amplify);
  gst_check_teardown_element (amplify);
}

GST_START_TEST (test_passthrough)
{
  GstElement *amplify;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[6] = { 24576, -16384, 256, -128, 0, -24576 };
  gint16 res[6];

  amplify = setup_amplify ();
  fail_unless (gst_element_set_state (amplify,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (AMPLIFY_CAPS_STRING);
  gst_check_setup_events (mysrcpad, amplify, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, 12, 0, 12,
      NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 12) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... but it ends up being collected on the global buffer list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res, 12) == 12);
  GST_INFO
      ("expected %+5d %+5d %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d %+5d %+5d",
      in[0], in[1], in[2], in[3], in[4], in[5], res[0], res[1], res[2], res[3],
      res[4], res[5]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, in, 12) == 0);

  /* cleanup */
  cleanup_amplify (amplify);
}

GST_END_TEST;

GST_START_TEST (test_zero)
{
  GstElement *amplify;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[6] = { 24576, -16384, 256, -128, 0, -24576 };
  gint16 out[6] = { 0, 0, 0, 0, 0, 0 };
  gint16 res[6];

  amplify = setup_amplify ();
  g_object_set (G_OBJECT (amplify), "amplification", 0.0, NULL);
  fail_unless (gst_element_set_state (amplify,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (AMPLIFY_CAPS_STRING);
  gst_check_setup_events (mysrcpad, amplify, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, 12, 0, 12,
      NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 12) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res, 12) == 12);
  GST_INFO
      ("expected %+5d %+5d %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], out[4], out[5], res[0], res[1], res[2],
      res[3], res[4], res[5]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 12) == 0);

  /* cleanup */
  cleanup_amplify (amplify);
}

GST_END_TEST;

GST_START_TEST (test_050_clip)
{
  GstElement *amplify;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[6] = { 24576, -16384, 256, -128, 0, -24576 };
  gint16 out[6] = { 12288, -8192, 128, -64, 0, -12288 };
  gint16 res[6];

  amplify = setup_amplify ();
  g_object_set (G_OBJECT (amplify), "amplification", 0.5, NULL);
  fail_unless (gst_element_set_state (amplify,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (AMPLIFY_CAPS_STRING);
  gst_check_setup_events (mysrcpad, amplify, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, 12, 0, 12,
      NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 12) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res, 12) == 12);
  GST_INFO
      ("expected %+5d %+5d %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], out[4], out[5], res[0], res[1], res[2],
      res[3], res[4], res[5]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 12) == 0);

  /* cleanup */
  cleanup_amplify (amplify);
}

GST_END_TEST;

GST_START_TEST (test_200_clip)
{
  GstElement *amplify;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[6] = { 24576, -16384, 256, -128, 0, -24576 };
  gint16 out[6] = { G_MAXINT16, -32768, 512, -256, 0, G_MININT16 };
  gint16 res[6];

  amplify = setup_amplify ();
  g_object_set (G_OBJECT (amplify), "amplification", 2.0, NULL);
  fail_unless (gst_element_set_state (amplify,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (AMPLIFY_CAPS_STRING);
  gst_check_setup_events (mysrcpad, amplify, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, 12, 0, 12,
      NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 12) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res, 12) == 12);
  GST_INFO
      ("expected %+5d %+5d %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], out[4], out[5], res[0], res[1], res[2],
      res[3], res[4], res[5]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 12) == 0);

  /* cleanup */
  cleanup_amplify (amplify);
}

GST_END_TEST;

GST_START_TEST (test_050_wrap_negative)
{
  GstElement *amplify;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[6] = { 24576, -16384, 256, -128, 0, -24576 };
  gint16 out[6] = { 12288, -8192, 128, -64, 0, -12288 };
  gint16 res[6];

  amplify = setup_amplify ();
  g_object_set (G_OBJECT (amplify), "amplification", 0.5, NULL);
  g_object_set (G_OBJECT (amplify), "clipping-method", 1, NULL);
  fail_unless (gst_element_set_state (amplify,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (AMPLIFY_CAPS_STRING);
  gst_check_setup_events (mysrcpad, amplify, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, 12, 0, 12,
      NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 12) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res, 12) == 12);
  GST_INFO
      ("expected %+5d %+5d %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], out[4], out[5], res[0], res[1], res[2],
      res[3], res[4], res[5]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 12) == 0);

  /* cleanup */
  cleanup_amplify (amplify);
}

GST_END_TEST;

GST_START_TEST (test_200_wrap_negative)
{
  GstElement *amplify;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[6] = { 24576, -16384, 256, -128, 0, -24576 };
  gint16 out[6] = { -16384, -32768, 512, -256, 0, 16384 };
  gint16 res[6];

  amplify = setup_amplify ();
  g_object_set (G_OBJECT (amplify), "amplification", 2.0, NULL);
  g_object_set (G_OBJECT (amplify), "clipping-method", 1, NULL);
  fail_unless (gst_element_set_state (amplify,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (AMPLIFY_CAPS_STRING);
  gst_check_setup_events (mysrcpad, amplify, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, 12, 0, 12,
      NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 12) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res, 12) == 12);
  GST_INFO
      ("expected %+5d %+5d %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], out[4], out[5], res[0], res[1], res[2],
      res[3], res[4], res[5]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 12) == 0);

  /* cleanup */
  cleanup_amplify (amplify);
}

GST_END_TEST;

GST_START_TEST (test_050_wrap_positive)
{
  GstElement *amplify;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[6] = { 24576, -16384, 256, -128, 0, -24576 };
  gint16 out[6] = { 12288, -8192, 128, -64, 0, -12288 };
  gint16 res[6];

  amplify = setup_amplify ();
  g_object_set (G_OBJECT (amplify), "amplification", 0.5, NULL);
  g_object_set (G_OBJECT (amplify), "clipping-method", 2, NULL);
  fail_unless (gst_element_set_state (amplify,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (AMPLIFY_CAPS_STRING);
  gst_check_setup_events (mysrcpad, amplify, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, 12, 0, 12,
      NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 12) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res, 12) == 12);
  GST_INFO
      ("expected %+5d %+5d %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], out[4], out[5], res[0], res[1], res[2],
      res[3], res[4], res[5]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 12) == 0);

  /* cleanup */
  cleanup_amplify (amplify);
}

GST_END_TEST;

GST_START_TEST (test_200_wrap_positive)
{
  GstElement *amplify;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[6] = { 24576, -16384, 256, -128, 0, -24576 };
  gint16 out[6] = { 16382, -32768, 512, -256, 0, -16384 };
  gint16 res[6];

  amplify = setup_amplify ();
  g_object_set (G_OBJECT (amplify), "amplification", 2.0, NULL);
  g_object_set (G_OBJECT (amplify), "clipping-method", 2, NULL);
  fail_unless (gst_element_set_state (amplify,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  caps = gst_caps_from_string (AMPLIFY_CAPS_STRING);
  gst_check_setup_events (mysrcpad, amplify, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  inbuffer =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, in, 12, 0, 12,
      NULL, NULL);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 12) == 0);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  fail_unless (gst_buffer_extract (outbuffer, 0, res, 12) == 12);
  GST_INFO
      ("expected %+5d %+5d %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], out[4], out[5], res[0], res[1], res[2],
      res[3], res[4], res[5]);
  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 12) == 0);

  /* cleanup */
  cleanup_amplify (amplify);
}

GST_END_TEST;

static Suite *
amplify_suite (void)
{
  Suite *s = suite_create ("amplify");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_passthrough);
  tcase_add_test (tc_chain, test_zero);
  tcase_add_test (tc_chain, test_050_clip);
  tcase_add_test (tc_chain, test_200_clip);
  tcase_add_test (tc_chain, test_050_wrap_negative);
  tcase_add_test (tc_chain, test_200_wrap_negative);
  tcase_add_test (tc_chain, test_050_wrap_positive);
  tcase_add_test (tc_chain, test_200_wrap_positive);
  return s;
}

GST_CHECK_MAIN (amplify);
