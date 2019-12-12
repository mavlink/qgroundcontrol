/* GStreamer
 *
 * unit test for wavpackenc
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

#include <unistd.h>

#include <gst/check/gstcheck.h>

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
static GstPad *mysrcpad, *mysinkpad;
static GstBus *bus;

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define AUDIO_FORMAT "S32BE"
#else
#define AUDIO_FORMAT "S32LE"
#endif

#define RAW_CAPS_STRING "audio/x-raw, " \
                        "format = (string) " AUDIO_FORMAT ", " \
                        "layout = (string) interleaved, " \
                        "channels = (int) 1, " \
                        "rate = (int) 44100"

#define WAVPACK_CAPS_STRING "audio/x-wavpack, " \
                            "depth = (int) 32, " \
                            "channels = (int) 1, " \
                            "rate = (int) 44100, " \
                            "framed = (boolean) true"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-wavpack, "
        "depth = (int) 32, "
        "channels = (int) 1, "
        "rate = (int) 44100, " "framed = (boolean) true"));

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " AUDIO_FORMAT ", "
        "layout = (string) interleaved, "
        "channels = (int) 1, " "rate = (int) 44100"));

static GstElement *
setup_wavpackenc (void)
{
  GstElement *wavpackenc;

  GST_DEBUG ("setup_wavpackenc");
  wavpackenc = gst_check_setup_element ("wavpackenc");
  mysrcpad = gst_check_setup_src_pad (wavpackenc, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (wavpackenc, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  fail_unless (gst_element_set_state (wavpackenc,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");
  bus = gst_bus_new ();

  gst_pad_push_event (mysrcpad, gst_event_new_stream_start ("test-silence"));

  return wavpackenc;
}

static void
cleanup_wavpackenc (GstElement * wavpackenc)
{
  GST_DEBUG ("cleanup_wavpackenc");

  gst_bus_set_flushing (bus, TRUE);
  gst_element_set_bus (wavpackenc, NULL);
  gst_object_unref (GST_OBJECT (bus));

  gst_element_set_state (wavpackenc, GST_STATE_NULL);

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (wavpackenc);
  gst_check_teardown_sink_pad (wavpackenc);
  gst_check_teardown_element (wavpackenc);
}

GST_START_TEST (test_encode_silence)
{
  GstSegment segment;
  GstElement *wavpackenc;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  GstEvent *eos = gst_event_new_eos ();
  int i, num_buffers;

  wavpackenc = setup_wavpackenc ();

  gst_segment_init (&segment, GST_FORMAT_TIME);

  inbuffer = gst_buffer_new_and_alloc (1000);
  gst_buffer_memset (inbuffer, 0, 0, 1000);

  caps = gst_caps_from_string (RAW_CAPS_STRING);
  fail_unless (gst_pad_set_caps (mysrcpad, caps));
  gst_caps_unref (caps);

  gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment));

  GST_BUFFER_TIMESTAMP (inbuffer) = 0;
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  gst_element_set_bus (wavpackenc, bus);

  fail_unless_equals_int (gst_pad_push (mysrcpad, inbuffer), GST_FLOW_OK);

  fail_if (gst_pad_push_event (mysrcpad, eos) != TRUE);

  /* check first buffer */
  outbuffer = GST_BUFFER (buffers->data);

  fail_if (outbuffer == NULL);

  fail_unless_equals_int (GST_BUFFER_TIMESTAMP (outbuffer), 0);
  fail_unless_equals_int (GST_BUFFER_DURATION (outbuffer), 5668934);

  fail_unless (gst_buffer_memcmp (outbuffer, 0, "wvpk", 4) == 0,
      "Failed to encode to valid Wavpack frames");

  /* free all buffers */
  num_buffers = g_list_length (buffers);

  for (i = 0; i < num_buffers; ++i) {
    outbuffer = GST_BUFFER (buffers->data);
    fail_if (outbuffer == NULL);

    buffers = g_list_remove (buffers, outbuffer);

    gst_buffer_unref (outbuffer);
    outbuffer = NULL;
  }

  g_list_free (buffers);
  buffers = NULL;

  cleanup_wavpackenc (wavpackenc);
}

GST_END_TEST;

static Suite *
wavpackenc_suite (void)
{
  Suite *s = suite_create ("wavpackenc");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_encode_silence);

  return s;
}

GST_CHECK_MAIN (wavpackenc);
