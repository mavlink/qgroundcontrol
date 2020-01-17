/* GStreamer
 *
 * Copyright (c) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-vp8, "
        "width = (int) [1, MAX], "
        "height = (int) [1, MAX], " "framerate = (fraction) [0, MAX]"));

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) I420, "
        "width = (int) [1, MAX], "
        "height = (int) [1, MAX], " "framerate = (fraction) [0, MAX]"));

static GstPad *sinkpad, *srcpad;

static GstElement *
setup_vp8enc (const gchar * src_caps_str)
{
  GstElement *vp8enc;
  GstCaps *srccaps = NULL;
  GstBus *bus;

  if (src_caps_str) {
    srccaps = gst_caps_from_string (src_caps_str);
    fail_unless (srccaps != NULL);
  }

  vp8enc = gst_check_setup_element ("vp8enc");
  fail_unless (vp8enc != NULL);
  srcpad = gst_check_setup_src_pad (vp8enc, &srctemplate);
  sinkpad = gst_check_setup_sink_pad (vp8enc, &sinktemplate);
  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);
  gst_check_setup_events (srcpad, vp8enc, srccaps, GST_FORMAT_TIME);

  bus = gst_bus_new ();
  gst_element_set_bus (vp8enc, bus);

  fail_unless (gst_element_set_state (vp8enc,
          GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
      "could not set to playing");

  if (srccaps)
    gst_caps_unref (srccaps);

  buffers = NULL;
  return vp8enc;
}

static void
cleanup_vp8enc (GstElement * vp8enc)
{
  GstBus *bus;

  /* Free parsed buffers */
  gst_check_drop_buffers ();

  bus = GST_ELEMENT_BUS (vp8enc);
  gst_bus_set_flushing (bus, TRUE);
  gst_object_unref (bus);

  gst_pad_set_active (srcpad, FALSE);
  gst_pad_set_active (sinkpad, FALSE);
  gst_check_teardown_src_pad (vp8enc);
  gst_check_teardown_sink_pad (vp8enc);
  gst_check_teardown_element (vp8enc);
}

GST_START_TEST (test_encode_simple)
{
  GstElement *vp8enc;
  GstBuffer *buffer;
  gint i;
  GList *l;
  GstCaps *outcaps;
  GstSegment seg;

  vp8enc =
      setup_vp8enc
      ("video/x-raw,format=(string)I420,width=(int)320,height=(int)240,framerate=(fraction)25/1");

  g_object_set (vp8enc, "lag-in-frames", 5, NULL);

  gst_segment_init (&seg, GST_FORMAT_TIME);
  seg.stop = gst_util_uint64_scale (20, GST_SECOND, 25);
  fail_unless (gst_pad_push_event (srcpad, gst_event_new_segment (&seg)));

  buffer = gst_buffer_new_and_alloc (320 * 240 + 2 * 160 * 120);
  gst_buffer_memset (buffer, 0, 0, -1);

  for (i = 0; i < 20; i++) {
    GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (i, GST_SECOND, 25);
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale (1, GST_SECOND, 25);
    fail_unless (gst_pad_push (srcpad, gst_buffer_ref (buffer)) == GST_FLOW_OK);
  }

  gst_buffer_unref (buffer);

  /* Only 5 buffers are allowed to be queued now */
  fail_unless (g_list_length (buffers) > 15);

  fail_unless (gst_pad_push_event (srcpad, gst_event_new_eos ()));


  /* All buffers must be there now */
  fail_unless_equals_int (g_list_length (buffers), 20);

  outcaps =
      gst_caps_from_string
      ("video/x-vp8,width=(int)320,height=(int)240,framerate=(fraction)25/1");

  for (l = buffers, i = 0; l; l = l->next, i++) {
    buffer = l->data;

    if (i == 0)
      fail_if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT));

    fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer),
        gst_util_uint64_scale (i, GST_SECOND, 25));
    fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer),
        gst_util_uint64_scale (1, GST_SECOND, 25));
  }

  gst_caps_unref (outcaps);

  cleanup_vp8enc (vp8enc);
}

GST_END_TEST;

static Suite *
vp8enc_suite (void)
{
  Suite *s = suite_create ("vp8enc");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_encode_simple);

  return s;
}

GST_CHECK_MAIN (vp8enc);
