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
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) I420, "
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
setup_vp8dec (const gchar * src_caps_str)
{
  GstElement *bin;
  GstElement *vp8enc, *vp8dec;
  GstCaps *srccaps = NULL;
  GstBus *bus;
  GstPad *ghostpad, *targetpad;

  if (src_caps_str) {
    srccaps = gst_caps_from_string (src_caps_str);
    fail_unless (srccaps != NULL);
  }

  bin = gst_bin_new ("bin");

  vp8enc = gst_check_setup_element ("vp8enc");
  fail_unless (vp8enc != NULL);
  vp8dec = gst_check_setup_element ("vp8dec");
  fail_unless (vp8dec != NULL);

  g_object_set (vp8enc, "name", "encoder", NULL);
  g_object_set (vp8dec, "name", "decoder", NULL);

  gst_bin_add_many (GST_BIN (bin), vp8enc, vp8dec, NULL);
  fail_unless (gst_element_link_pads (vp8enc, "src", vp8dec, "sink"));

  targetpad = gst_element_get_static_pad (vp8enc, "sink");
  fail_unless (targetpad != NULL);
  ghostpad = gst_ghost_pad_new ("sink", targetpad);
  fail_unless (ghostpad != NULL);
  gst_element_add_pad (bin, ghostpad);
  gst_object_unref (targetpad);

  targetpad = gst_element_get_static_pad (vp8dec, "src");
  fail_unless (targetpad != NULL);
  ghostpad = gst_ghost_pad_new ("src", targetpad);
  fail_unless (ghostpad != NULL);
  gst_element_add_pad (bin, ghostpad);
  gst_object_unref (targetpad);

  srcpad = gst_check_setup_src_pad (bin, &srctemplate);
  sinkpad = gst_check_setup_sink_pad (bin, &sinktemplate);
  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);
  gst_check_setup_events (srcpad, bin, srccaps, GST_FORMAT_TIME);

  bus = gst_bus_new ();
  gst_element_set_bus (bin, bus);

  fail_unless (gst_element_set_state (bin,
          GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
      "could not set to playing");

  if (srccaps)
    gst_caps_unref (srccaps);

  buffers = NULL;
  return bin;
}

static void
cleanup_vp8dec (GstElement * bin)
{
  GstBus *bus;

  /* Free parsed buffers */
  gst_check_drop_buffers ();

  bus = GST_ELEMENT_BUS (bin);
  gst_bus_set_flushing (bus, TRUE);
  gst_object_unref (bus);

  gst_pad_set_active (srcpad, FALSE);
  gst_pad_set_active (sinkpad, FALSE);

  gst_check_teardown_src_pad (bin);
  gst_check_teardown_sink_pad (bin);
  gst_check_teardown_element (bin);
}

static void
_gst_vp8_test_check_output_caps (gint width, gint height, gint fps_n,
    gint fps_d)
{
  GstCaps *caps;
  GstStructure *structure;
  gint caps_w, caps_h, caps_fpsn, caps_fpsd;

  caps = gst_pad_get_current_caps (sinkpad);
  fail_unless (caps != NULL);
  structure = gst_caps_get_structure (caps, 0);

  fail_unless (gst_structure_get_int (structure, "width", &caps_w));
  fail_unless (gst_structure_get_int (structure, "height", &caps_h));
  fail_unless (gst_structure_get_fraction (structure, "framerate", &caps_fpsn,
          &caps_fpsd));

  fail_unless (width == caps_w);
  fail_unless (height == caps_h);
  fail_unless (fps_n == caps_fpsn);
  fail_unless (fps_d == caps_fpsd);

  gst_caps_unref (caps);
}

GST_START_TEST (test_decode_simple)
{
  GstElement *bin;
  GstBuffer *buffer;
  gint i;
  GList *l;
  GstSegment seg;

  bin =
      setup_vp8dec
      ("video/x-raw,format=(string)I420,width=(int)320,height=(int)240,framerate=(fraction)25/1");

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

  fail_unless (gst_pad_push_event (srcpad, gst_event_new_eos ()));

  /* All buffers must be there now */
  fail_unless_equals_int (g_list_length (buffers), 20);

  for (l = buffers, i = 0; l; l = l->next, i++) {
    buffer = l->data;

    fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer),
        gst_util_uint64_scale (i, GST_SECOND, 25));
    fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer),
        gst_util_uint64_scale (1, GST_SECOND, 25));
  }

  cleanup_vp8dec (bin);
}

GST_END_TEST;


GST_START_TEST (test_decode_caps_change)
{
  GstElement *bin;
  GstBuffer *buffer;
  GstSegment seg;
  GstElement *encoder;
  GstCaps *caps;

  bin =
      setup_vp8dec
      ("video/x-raw,format=(string)I420,width=(int)320,height=(int)240,framerate=(fraction)25/1");

  gst_segment_init (&seg, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (srcpad, gst_event_new_segment (&seg)));

  buffer = gst_buffer_new_and_alloc (320 * 240 + 2 * 160 * 120);
  gst_buffer_memset (buffer, 0, 0, -1);

  GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (0, GST_SECOND, 25);
  GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale (1, GST_SECOND, 25);
  fail_unless (gst_pad_push (srcpad, buffer) == GST_FLOW_OK);

  /* at this point, the output caps should be the same as the input */
  _gst_vp8_test_check_output_caps (320, 240, 25, 1);
  fail_unless_equals_int (g_list_length (buffers), 1);
  g_list_free_full (buffers, (GDestroyNotify) gst_buffer_unref);
  buffers = NULL;

  /* now change the caps */
  encoder = gst_bin_get_by_name (GST_BIN (bin), "encoder");
  gst_element_set_state (encoder, GST_STATE_NULL);
  gst_element_sync_state_with_parent (encoder);
  gst_object_unref (encoder);
  caps = gst_caps_from_string
      ("video/x-raw,format=(string)I420,width=(int)64,"
      "height=(int)32,framerate=(fraction)30/1");
  fail_unless (gst_pad_push_event (srcpad, gst_event_new_caps (caps)));
  fail_unless (gst_pad_push_event (srcpad, gst_event_new_segment (&seg)));
  buffer = gst_buffer_new_and_alloc (64 * 32 + 2 * 32 * 16);
  gst_buffer_memset (buffer, 0, 0, -1);
  gst_caps_unref (caps);

  GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (0, GST_SECOND, 30);
  GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale (1, GST_SECOND, 30);
  fail_unless (gst_pad_push (srcpad, buffer) == GST_FLOW_OK);

  /* at this point, the output caps should be the same as the input */
  _gst_vp8_test_check_output_caps (64, 32, 30, 1);
  fail_unless_equals_int (g_list_length (buffers), 1);
  g_list_free_full (buffers, (GDestroyNotify) gst_buffer_unref);
  buffers = NULL;

  cleanup_vp8dec (bin);
}

GST_END_TEST;


static Suite *
vp8dec_suite (void)
{
  Suite *s = suite_create ("vp8dec");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_decode_simple);
  tcase_add_test (tc_chain, test_decode_caps_change);

  return s;
}

GST_CHECK_MAIN (vp8dec);
