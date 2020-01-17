/* GStreamer
 *
 * unit test for wavpackparse
 *
 * Copyright (c) 2006 Sebastian Dröge <slomo@circular-chaos.org>
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

/* Wavpack file with 2 frames of silence */
guint8 test_file[] = {
  0x77, 0x76, 0x70, 0x6B, 0x62, 0x00, 0x00, 0x00,       /* first frame */
  0x04, 0x04, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00,       /* include RIFF header */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
  0x05, 0x18, 0x80, 0x04, 0xFF, 0xAF, 0x80, 0x60,
  0x21, 0x16, 0x52, 0x49, 0x46, 0x46, 0x24, 0x90,
  0x01, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D,
  0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x01, 0x00, 0x44, 0xAC, 0x00, 0x00, 0x88, 0x58,
  0x01, 0x00, 0x02, 0x00, 0x10, 0x00, 0x64, 0x61,
  0x74, 0x61, 0x00, 0x90, 0x01, 0x00, 0x02, 0x00,
  0x03, 0x00, 0x04, 0x00, 0x05, 0x03, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x65, 0x02, 0x00, 0x00,
  0x00, 0x00, 0x8A, 0x02, 0x00, 0x00, 0xFF, 0x7F,
  0x00, 0xE4,
  0x77, 0x76, 0x70, 0x6B, 0x2E, 0x00, 0x00, 0x00,       /* second frame */
  0x04, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
  0x05, 0x18, 0x80, 0x04, 0xFF, 0xAF, 0x80, 0x60,
  0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x03,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8A, 0x02,
  0x00, 0x00, 0xFF, 0x7F, 0x00, 0xE4,
};

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-wavpack, "
        "depth = (int) 16, "
        "channels = (int) 1, "
        "rate = (int) 44100, " "framed = (boolean) TRUE"));

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-wavpack"));

static GstElement *
setup_wavpackparse (void)
{
  GstElement *wavpackparse;

  GST_DEBUG ("setup_wavpackparse");

  wavpackparse = gst_check_setup_element ("wavpackparse");
  mysrcpad = gst_check_setup_src_pad (wavpackparse, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (wavpackparse, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);
  gst_check_setup_events (mysrcpad, wavpackparse, NULL, GST_FORMAT_BYTES);

  return wavpackparse;
}

static void
cleanup_wavpackparse (GstElement * wavpackparse)
{
  GST_DEBUG ("cleanup_wavpackparse");
  gst_element_set_state (wavpackparse, GST_STATE_NULL);

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (wavpackparse);
  gst_check_teardown_sink_pad (wavpackparse);
  gst_check_teardown_element (wavpackparse);
}

GST_START_TEST (test_parsing_valid_frames)
{
  GstElement *wavpackparse;
  GstBuffer *inbuffer, *outbuffer;
  int i, num_buffers;
  GstFormat format = GST_FORMAT_TIME;
  gint64 pos;

  wavpackparse = setup_wavpackparse ();
  fail_unless (gst_element_set_state (wavpackparse,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = gst_buffer_new_and_alloc (sizeof (test_file));
  gst_buffer_fill (inbuffer, 0, test_file, sizeof (test_file));

  /* should decode the buffer without problems */
  fail_unless_equals_int (gst_pad_push (mysrcpad, inbuffer), GST_FLOW_OK);

  /* inform of no further data */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));

  num_buffers = g_list_length (buffers);
  /* should get 2 buffers, each one complete wavpack frame */
  fail_unless_equals_int (num_buffers, 2);

  for (i = 0; i < num_buffers; ++i) {
    outbuffer = GST_BUFFER (buffers->data);
    fail_if (outbuffer == NULL);

    fail_unless (gst_buffer_memcmp (outbuffer, 0, "wvpk", 4) == 0,
        "Buffer contains no Wavpack frame");
    fail_unless_equals_int (GST_BUFFER_DURATION (outbuffer), 580498866);

    switch (i) {
      case 0:{
        fail_unless_equals_int (GST_BUFFER_TIMESTAMP (outbuffer), 0);
        break;
      }
      case 1:{
        fail_unless_equals_int (GST_BUFFER_TIMESTAMP (outbuffer), 580498866);
        break;
      }
    }

    buffers = g_list_remove (buffers, outbuffer);

    gst_buffer_unref (outbuffer);
    outbuffer = NULL;
  }

  fail_unless (gst_element_query_position (wavpackparse, format, &pos),
      "Position query failed");
  fail_unless_equals_int64 (pos, 580498866 * 2);
  fail_unless (gst_element_query_duration (wavpackparse, format, NULL),
      "Duration query failed");

  g_list_free (buffers);
  buffers = NULL;

  cleanup_wavpackparse (wavpackparse);
}

GST_END_TEST;

GST_START_TEST (test_parsing_invalid_first_header)
{
  GstElement *wavpackparse;
  GstBuffer *inbuffer, *outbuffer;
  int i, num_buffers;

  wavpackparse = setup_wavpackparse ();
  fail_unless (gst_element_set_state (wavpackparse,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = gst_buffer_new_and_alloc (sizeof (test_file));
  gst_buffer_fill (inbuffer, 0, test_file, sizeof (test_file));
  gst_buffer_memset (inbuffer, 0, 'k', 1);

  /* should decode the buffer without problems */
  fail_unless_equals_int (gst_pad_push (mysrcpad, inbuffer), GST_FLOW_OK);

  /* inform of no further data */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));

  num_buffers = g_list_length (buffers);

  /* should get 1 buffers, the second non-broken one */
  fail_unless_equals_int (num_buffers, 1);

  for (i = 0; i < num_buffers; ++i) {
    outbuffer = GST_BUFFER (buffers->data);
    fail_if (outbuffer == NULL);

    fail_unless (gst_buffer_memcmp (outbuffer, 0, "wvpk", 4) == 0,
        "Buffer contains no Wavpack frame");
    fail_unless_equals_int (GST_BUFFER_DURATION (outbuffer), 580498866);

    switch (i) {
      case 0:{
        fail_unless_equals_int (GST_BUFFER_TIMESTAMP (outbuffer), 580498866);
        break;
      }
    }

    buffers = g_list_remove (buffers, outbuffer);

    gst_buffer_unref (outbuffer);
    outbuffer = NULL;
  }

  g_list_free (buffers);
  buffers = NULL;

  cleanup_wavpackparse (wavpackparse);
}

GST_END_TEST;


static Suite *
wavpackparse_suite (void)
{
  Suite *s = suite_create ("wavpackparse");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_parsing_valid_frames);
  tcase_add_test (tc_chain, test_parsing_invalid_first_header);

  return s;
}

GST_CHECK_MAIN (wavpackparse);
