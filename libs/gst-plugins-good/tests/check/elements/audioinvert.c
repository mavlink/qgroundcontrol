/* GStreamer
 *
 * unit test for audioinvert
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

#include <gst/audio/audio.h>
#include <gst/base/gstbasetransform.h>
#include <gst/check/gstcheck.h>

gboolean have_eos = FALSE;

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
GstPad *mysrcpad, *mysinkpad;


#define INVERT_CAPS_STRING    \
    "audio/x-raw, "                             \
    "format = (string) "GST_AUDIO_NE(S16)", "   \
    "layout = (string) interleaved, "           \
    "channels = (int) 1, "                      \
    "rate = (int) 44100"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "channels = (int) 1, " "rate = (int) [ 1,  MAX ]")
    );
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "channels = (int) 1, " "rate = (int) [ 1,  MAX ]")
    );

static GstElement *
setup_invert (void)
{
  GstElement *invert;

  GST_DEBUG ("setup_invert");
  invert = gst_check_setup_element ("audioinvert");
  mysrcpad = gst_check_setup_src_pad (invert, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (invert, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return invert;
}

static void
cleanup_invert (GstElement * invert)
{
  GST_DEBUG ("cleanup_invert");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (invert);
  gst_check_teardown_sink_pad (invert);
  gst_check_teardown_element (invert);
}

GST_START_TEST (test_passthrough)
{
  GstElement *invert;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[4] = { 16384, -256, 128, -512 };
  gint16 *res;
  GstMapInfo map;

  invert = setup_invert ();
  fail_unless (gst_element_set_state (invert,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = gst_buffer_new_and_alloc (8);
  gst_buffer_fill (inbuffer, 0, in, 8);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 8) == 0);
  caps = gst_caps_from_string (INVERT_CAPS_STRING);
  gst_check_setup_events (mysrcpad, invert, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... but it ends up being collected on the global buffer list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gint16 *) map.data;
  GST_INFO ("expected %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d",
      in[0], in[1], in[2], in[3], res[0], res[1], res[2], res[3]);
  gst_buffer_unmap (outbuffer, &map);

  fail_unless (gst_buffer_memcmp (outbuffer, 0, in, 8) == 0);

  /* cleanup */
  cleanup_invert (invert);
}

GST_END_TEST;

GST_START_TEST (test_zero)
{
  GstElement *invert;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[4] = { 16384, -256, 128, -512 };
  gint16 out[4] = { 0, 0, 0, 0 };
  gint16 *res;
  GstMapInfo map;

  invert = setup_invert ();
  g_object_set (G_OBJECT (invert), "degree", 0.5, NULL);
  fail_unless (gst_element_set_state (invert,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = gst_buffer_new_and_alloc (8);
  gst_buffer_fill (inbuffer, 0, in, 8);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 8) == 0);
  caps = gst_caps_from_string (INVERT_CAPS_STRING);
  gst_check_setup_events (mysrcpad, invert, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gint16 *) map.data;
  GST_INFO ("expected %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], res[0], res[1], res[2], res[3]);
  gst_buffer_unmap (outbuffer, &map);

  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 8) == 0);

  /* cleanup */
  cleanup_invert (invert);
}

GST_END_TEST;

GST_START_TEST (test_full_inverse)
{
  GstElement *invert;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[4] = { 16384, -256, 128, -512 };
  gint16 out[4] = { -16385, 255, -129, 511 };
  gint16 *res;
  GstMapInfo map;

  invert = setup_invert ();
  g_object_set (G_OBJECT (invert), "degree", 1.0, NULL);
  fail_unless (gst_element_set_state (invert,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = gst_buffer_new_and_alloc (8);
  gst_buffer_fill (inbuffer, 0, in, 8);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 8) == 0);
  caps = gst_caps_from_string (INVERT_CAPS_STRING);
  gst_check_setup_events (mysrcpad, invert, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gint16 *) map.data;
  GST_INFO ("expected %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], res[0], res[1], res[2], res[3]);
  gst_buffer_unmap (outbuffer, &map);

  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 8) == 0);

  /* cleanup */
  cleanup_invert (invert);
}

GST_END_TEST;

GST_START_TEST (test_25_inverse)
{
  GstElement *invert;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gint16 in[4] = { 16384, -256, 128, -512 };
  gint16 out[4] = { 8191, -128, 63, -256 };
  gint16 *res;
  GstMapInfo map;

  invert = setup_invert ();
  g_object_set (G_OBJECT (invert), "degree", 0.25, NULL);
  fail_unless (gst_element_set_state (invert,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = gst_buffer_new_and_alloc (8);
  gst_buffer_fill (inbuffer, 0, in, 8);
  fail_unless (gst_buffer_memcmp (inbuffer, 0, in, 8) == 0);
  caps = gst_caps_from_string (INVERT_CAPS_STRING);
  gst_check_setup_events (mysrcpad, invert, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gint16 *) map.data;
  GST_INFO ("expected %+5d %+5d %+5d %+5d real %+5d %+5d %+5d %+5d",
      out[0], out[1], out[2], out[3], res[0], res[1], res[2], res[3]);
  gst_buffer_unmap (outbuffer, &map);

  fail_unless (gst_buffer_memcmp (outbuffer, 0, out, 8) == 0);

  /* cleanup */
  cleanup_invert (invert);
}

GST_END_TEST;

static Suite *
invert_suite (void)
{
  Suite *s = suite_create ("invert");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_passthrough);
  tcase_add_test (tc_chain, test_zero);
  tcase_add_test (tc_chain, test_full_inverse);
  tcase_add_test (tc_chain, test_25_inverse);

  return s;
}

GST_CHECK_MAIN (invert);
