/* GStreamer
 *
 * unit test for jpegenc
 *
 * Copyright (C) <2010> Thiago Santos <thiago.sousa.santos@collabora.co.uk>
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
#include <gst/app/gstappsink.h>

/* For ease of programming we use globals to keep refs for our floating
 * sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
static GstPad *mysinkpad;
static GstPad *mysrcpad;

#define JPEG_CAPS_STRING "image/jpeg"

#define JPEG_CAPS_RESTRICTIVE "image/jpeg, " \
    "width = (int) [100, 200], " \
    "framerate = (fraction) 25/1, " \
    "extraparameter = (string) { abc, def }"

static GstStaticPadTemplate jpeg_sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (JPEG_CAPS_STRING));

static GstStaticPadTemplate any_sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate jpeg_restrictive_sinktemplate =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (JPEG_CAPS_RESTRICTIVE));

static GstStaticPadTemplate any_srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);


static GstElement *
setup_jpegenc (GstStaticPadTemplate * sinktemplate)
{
  GstElement *jpegenc;

  GST_DEBUG ("setup_jpegenc");
  jpegenc = gst_check_setup_element ("jpegenc");
  mysinkpad = gst_check_setup_sink_pad (jpegenc, sinktemplate);
  mysrcpad = gst_check_setup_src_pad (jpegenc, &any_srctemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return jpegenc;
}

static void
cleanup_jpegenc (GstElement * jpegenc)
{
  GST_DEBUG ("cleanup_jpegenc");
  gst_element_set_state (jpegenc, GST_STATE_NULL);

  gst_check_drop_buffers ();
  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_sink_pad (jpegenc);
  gst_check_teardown_src_pad (jpegenc);
  gst_check_teardown_element (jpegenc);
}

static GstBuffer *
create_video_buffer (GstCaps * caps)
{
  GstElement *pipeline;
  GstElement *cf;
  GstElement *sink;
  GstSample *sample;
  GstBuffer *buffer;

  pipeline =
      gst_parse_launch
      ("videotestsrc num-buffers=1 ! capsfilter name=cf ! appsink name=sink",
      NULL);
  g_assert (pipeline != NULL);

  cf = gst_bin_get_by_name (GST_BIN (pipeline), "cf");
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");

  g_object_set (G_OBJECT (cf), "caps", caps, NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  sample = gst_app_sink_pull_sample (GST_APP_SINK (sink));

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  gst_object_unref (sink);
  gst_object_unref (cf);

  buffer = gst_sample_get_buffer (sample);
  gst_buffer_ref (buffer);

  gst_sample_unref (sample);

  return buffer;
}


GST_START_TEST (test_jpegenc_getcaps)
{
  GstElement *jpegenc;
  GstPad *sinkpad;
  GstCaps *caps;
  GstStructure *structure;
  gint fps_n;
  gint fps_d;
  const GValue *value;

  /* we are going to do some get caps to confirm it doesn't return non-subset
   * caps */

  jpegenc = setup_jpegenc (&any_sinktemplate);
  sinkpad = gst_element_get_static_pad (jpegenc, "sink");
  /* this should assert if non-subset */
  caps = gst_pad_query_caps (sinkpad, NULL);
  gst_caps_unref (caps);
  gst_object_unref (sinkpad);
  cleanup_jpegenc (jpegenc);

  jpegenc = setup_jpegenc (&jpeg_sinktemplate);
  sinkpad = gst_element_get_static_pad (jpegenc, "sink");
  /* this should assert if non-subset */
  caps = gst_pad_query_caps (sinkpad, NULL);
  gst_caps_unref (caps);
  gst_object_unref (sinkpad);
  cleanup_jpegenc (jpegenc);

  /* now use a more restricted one and check the resulting caps */
  jpegenc = setup_jpegenc (&jpeg_restrictive_sinktemplate);
  sinkpad = gst_element_get_static_pad (jpegenc, "sink");
  /* this should assert if non-subset */
  caps = gst_pad_query_caps (sinkpad, NULL);
  structure = gst_caps_get_structure (caps, 0);

  /* check the width */
  value = gst_structure_get_value (structure, "width");
  fail_unless (gst_value_get_int_range_min (value) == 100);
  fail_unless (gst_value_get_int_range_max (value) == 200);

  fail_unless (gst_structure_get_fraction (structure, "framerate", &fps_n,
          &fps_d));
  fail_unless (fps_n == 25);
  fail_unless (fps_d == 1);

  gst_caps_unref (caps);
  gst_object_unref (sinkpad);
  cleanup_jpegenc (jpegenc);
}

GST_END_TEST;


GST_START_TEST (test_jpegenc_different_caps)
{
  GstElement *jpegenc;
  GstBuffer *buffer;
  GstCaps *caps;
  GstCaps *allowed_caps;

  /* now use a more restricted one and check the resulting caps */
  jpegenc = setup_jpegenc (&any_sinktemplate);
  gst_element_set_state (jpegenc, GST_STATE_PLAYING);

  /* push first buffer with 800x600 resolution */
  caps = gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT,
      800, "height", G_TYPE_INT, 600, "framerate",
      GST_TYPE_FRACTION, 1, 1, "format", G_TYPE_STRING, "I420", NULL);
  gst_check_setup_events (mysrcpad, jpegenc, caps, GST_FORMAT_TIME);
  fail_unless ((buffer = create_video_buffer (caps)) != NULL);
  gst_caps_unref (caps);
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

  /* check the allowed caps to see if a second buffer with a different
   * caps could be negotiated */
  allowed_caps = gst_pad_get_allowed_caps (mysrcpad);

  /* the caps we want to negotiate to */
  caps = gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT,
      640, "height", G_TYPE_INT, 480, "framerate",
      GST_TYPE_FRACTION, 1, 1, "format", G_TYPE_STRING, "I420", NULL);
  fail_unless (gst_caps_can_intersect (allowed_caps, caps));
  fail_unless (gst_pad_set_caps (mysrcpad, caps));

  /* push second buffer with 640x480 resolution */
  buffer = create_video_buffer (caps);
  gst_caps_unref (caps);
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

  gst_caps_unref (allowed_caps);
  gst_element_set_state (jpegenc, GST_STATE_NULL);
  cleanup_jpegenc (jpegenc);
}

GST_END_TEST;

static Suite *
jpegenc_suite (void)
{
  Suite *s = suite_create ("jpegenc");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_jpegenc_getcaps);
  tcase_add_test (tc_chain, test_jpegenc_different_caps);

  return s;
}

GST_CHECK_MAIN (jpegenc);
