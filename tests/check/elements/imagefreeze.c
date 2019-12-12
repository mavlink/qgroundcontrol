/* GStreamer
 * Copyright (C) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#include <string.h>

#include <gst/check/gstcheck.h>
#include <gst/video/video.h>

static gboolean
bus_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (message->type) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_WARNING:
    case GST_MESSAGE_ERROR:{
      GError *gerror;
      gchar *debug;

      if (message->type == GST_MESSAGE_WARNING)
        gst_message_parse_warning (message, &gerror, &debug);
      else
        gst_message_parse_error (message, &gerror, &debug);
      g_error ("error from %s: %s (%s)\n",
          GST_ELEMENT_NAME (GST_MESSAGE_SRC (message)), gerror->message,
          GST_STR_NULL (debug));
      g_error_free (gerror);
      g_free (debug);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static GstElement *
setup_imagefreeze (const GstCaps * caps1, const GstCaps * caps2,
    GCallback sink_handoff, gpointer sink_handoff_data)
{
  GstElement *pipeline;
  GstElement *videotestsrc, *capsfilter1, *imagefreeze, *capsfilter2, *fakesink;

  pipeline = gst_pipeline_new ("pipeline");
  fail_unless (pipeline != NULL);

  videotestsrc = gst_element_factory_make ("videotestsrc", "src");
  fail_unless (videotestsrc != NULL);
  g_object_set (videotestsrc, "num-buffers", 1, NULL);

  capsfilter1 = gst_element_factory_make ("capsfilter", "filter1");
  fail_unless (capsfilter1 != NULL);
  g_object_set (capsfilter1, "caps", caps1, NULL);

  imagefreeze = gst_element_factory_make ("imagefreeze", "freeze");
  fail_unless (imagefreeze != NULL);

  capsfilter2 = gst_element_factory_make ("capsfilter", "filter2");
  fail_unless (capsfilter2 != NULL);
  g_object_set (capsfilter2, "caps", caps2, NULL);

  fakesink = gst_element_factory_make ("fakesink", "sink");
  fail_unless (fakesink != NULL);
  g_object_set (fakesink, "signal-handoffs", TRUE, "async", FALSE, NULL);

  if (sink_handoff)
    g_signal_connect (fakesink, "handoff", sink_handoff, sink_handoff_data);

  gst_bin_add_many (GST_BIN (pipeline), videotestsrc, capsfilter1, imagefreeze,
      capsfilter2, fakesink, NULL);

  fail_unless (gst_element_link_pads (videotestsrc, "src", capsfilter1,
          "sink"));
  fail_unless (gst_element_link_pads (capsfilter1, "src", imagefreeze, "sink"));
  fail_unless (gst_element_link_pads (imagefreeze, "src", capsfilter2, "sink"));
  fail_unless (gst_element_link_pads (capsfilter2, "src", fakesink, "sink"));

  return pipeline;
}

static void
sink_handoff_cb_0_1 (GstElement * object, GstBuffer * buffer, GstPad * pad,
    gpointer user_data)
{
  guint *n_buffers = (guint *) user_data;

  if (*n_buffers == G_MAXUINT)
    return;

  fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer), 0);
  fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer), GST_CLOCK_TIME_NONE);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (buffer), 0);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (buffer), 1);

  *n_buffers = *n_buffers + 1;
}

GST_START_TEST (test_imagefreeze_0_1)
{
  GstElement *pipeline;
  GstCaps *caps1, *caps2;
  GstBus *bus;
  GMainLoop *loop;
  guint n_buffers = G_MAXUINT;
  guint bus_watch = 0;
  GstVideoInfo i1, i2;

  gst_video_info_init (&i1);
  gst_video_info_set_format (&i1, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i1.fps_n = 25;
  i1.fps_d = 1;
  caps1 = gst_video_info_to_caps (&i1);

  gst_video_info_init (&i2);
  gst_video_info_set_format (&i2, GST_VIDEO_FORMAT_xRGB, 640, 480);
  caps2 = gst_video_info_to_caps (&i2);

  pipeline =
      setup_imagefreeze (caps1, caps2, G_CALLBACK (sink_handoff_cb_0_1),
      &n_buffers);

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  n_buffers = 0;
  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless_equals_int (n_buffers, 1);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  gst_caps_unref (caps1);
  gst_caps_unref (caps2);
  g_source_remove (bus_watch);
}

GST_END_TEST;

static void
sink_handoff_cb_25_1_0ms_400ms (GstElement * object, GstBuffer * buffer,
    GstPad * pad, gpointer user_data)
{
  guint *n_buffers = (guint *) user_data;

  if (*n_buffers == G_MAXUINT)
    return;

  fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer),
      *n_buffers * 40 * GST_MSECOND);
  fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer), 40 * GST_MSECOND);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (buffer), *n_buffers);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (buffer), *n_buffers + 1);

  *n_buffers = *n_buffers + 1;
}

GST_START_TEST (test_imagefreeze_25_1_0ms_400ms)
{
  GstElement *pipeline;
  GstCaps *caps1, *caps2;
  GstBus *bus;
  GMainLoop *loop;
  guint n_buffers = G_MAXUINT;
  guint bus_watch = 0;
  GstVideoInfo i1, i2;

  gst_video_info_init (&i1);
  gst_video_info_set_format (&i1, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i1.fps_n = 25;
  i1.fps_d = 1;
  caps1 = gst_video_info_to_caps (&i1);

  gst_video_info_init (&i2);
  gst_video_info_set_format (&i2, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i2.fps_n = 25;
  i2.fps_d = 1;
  caps2 = gst_video_info_to_caps (&i2);

  pipeline =
      setup_imagefreeze (caps1, caps2,
      G_CALLBACK (sink_handoff_cb_25_1_0ms_400ms), &n_buffers);

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PAUSED),
      GST_STATE_CHANGE_SUCCESS);

  fail_unless (gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME,
          GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET,
          400 * GST_MSECOND));

  n_buffers = 0;

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless_equals_int (n_buffers, 10);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  gst_caps_unref (caps1);
  gst_caps_unref (caps2);
  g_source_remove (bus_watch);
}

GST_END_TEST;

static void
sink_handoff_cb_25_1_200ms_400ms (GstElement * object, GstBuffer * buffer,
    GstPad * pad, gpointer user_data)
{
  guint *n_buffers = (guint *) user_data;

  if (*n_buffers == G_MAXUINT)
    return;

  fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer),
      200 * GST_MSECOND + *n_buffers * 40 * GST_MSECOND);
  fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer), 40 * GST_MSECOND);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (buffer), 5 + *n_buffers);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (buffer),
      5 + *n_buffers + 1);

  *n_buffers = *n_buffers + 1;
}

GST_START_TEST (test_imagefreeze_25_1_200ms_400ms)
{
  GstElement *pipeline;
  GstCaps *caps1, *caps2;
  GstBus *bus;
  GMainLoop *loop;
  guint n_buffers = G_MAXUINT;
  guint bus_watch = 0;
  GstVideoInfo i1, i2;

  gst_video_info_init (&i1);
  gst_video_info_set_format (&i1, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i1.fps_n = 25;
  i1.fps_d = 1;
  caps1 = gst_video_info_to_caps (&i1);

  gst_video_info_init (&i2);
  gst_video_info_set_format (&i2, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i2.fps_n = 25;
  i2.fps_d = 1;
  caps2 = gst_video_info_to_caps (&i2);

  pipeline =
      setup_imagefreeze (caps1, caps2,
      G_CALLBACK (sink_handoff_cb_25_1_200ms_400ms), &n_buffers);

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PAUSED),
      GST_STATE_CHANGE_SUCCESS);

  fail_unless (gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME,
          GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 200 * GST_MSECOND,
          GST_SEEK_TYPE_SET, 400 * GST_MSECOND));

  n_buffers = 0;

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless_equals_int (n_buffers, 5);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  gst_caps_unref (caps1);
  gst_caps_unref (caps2);
  g_source_remove (bus_watch);
}

GST_END_TEST;

static void
sink_handoff_cb_25_1_400ms_0ms (GstElement * object, GstBuffer * buffer,
    GstPad * pad, gpointer user_data)
{
  guint *n_buffers = (guint *) user_data;

  if (*n_buffers == G_MAXUINT)
    return;

  fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer),
      400 * GST_MSECOND - (*n_buffers + 1) * 40 * GST_MSECOND);
  fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer), 40 * GST_MSECOND);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (buffer), 10 - (*n_buffers + 1));
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (buffer),
      10 - (*n_buffers + 1) + 1);

  *n_buffers = *n_buffers + 1;
}

GST_START_TEST (test_imagefreeze_25_1_400ms_0ms)
{
  GstElement *pipeline;
  GstCaps *caps1, *caps2;
  GstBus *bus;
  GMainLoop *loop;
  guint n_buffers = G_MAXUINT;
  guint bus_watch = 0;
  GstVideoInfo i1, i2;

  gst_video_info_init (&i1);
  gst_video_info_set_format (&i1, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i1.fps_n = 25;
  i1.fps_d = 1;
  caps1 = gst_video_info_to_caps (&i1);

  gst_video_info_init (&i2);
  gst_video_info_set_format (&i2, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i2.fps_n = 25;
  i2.fps_d = 1;
  caps2 = gst_video_info_to_caps (&i2);

  pipeline =
      setup_imagefreeze (caps1, caps2,
      G_CALLBACK (sink_handoff_cb_25_1_400ms_0ms), &n_buffers);

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PAUSED),
      GST_STATE_CHANGE_SUCCESS);

  fail_unless (gst_element_seek (pipeline, -1.0, GST_FORMAT_TIME,
          GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET,
          400 * GST_MSECOND));

  n_buffers = 0;

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless_equals_int (n_buffers, 10);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  gst_caps_unref (caps1);
  gst_caps_unref (caps2);
  g_source_remove (bus_watch);
}

GST_END_TEST;

static void
sink_handoff_cb_25_1_220ms_380ms (GstElement * object, GstBuffer * buffer,
    GstPad * pad, gpointer user_data)
{
  guint *n_buffers = (guint *) user_data;

  if (*n_buffers == G_MAXUINT)
    return;

  if (*n_buffers == 0) {
    fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer),
        220 * GST_MSECOND);
    fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer), 20 * GST_MSECOND);
  } else if (*n_buffers == 4) {
    fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer),
        360 * GST_MSECOND);
    fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer), 20 * GST_MSECOND);
  } else {
    fail_unless_equals_uint64 (GST_BUFFER_TIMESTAMP (buffer),
        200 * GST_MSECOND + *n_buffers * 40 * GST_MSECOND);
    fail_unless_equals_uint64 (GST_BUFFER_DURATION (buffer), 40 * GST_MSECOND);
  }

  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (buffer), 5 + *n_buffers);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (buffer),
      5 + *n_buffers + 1);

  *n_buffers = *n_buffers + 1;
}

GST_START_TEST (test_imagefreeze_25_1_220ms_380ms)
{
  GstElement *pipeline;
  GstCaps *caps1, *caps2;
  GstBus *bus;
  GMainLoop *loop;
  guint n_buffers = G_MAXUINT;
  guint bus_watch = 0;
  GstVideoInfo i1, i2;

  gst_video_info_init (&i1);
  gst_video_info_set_format (&i1, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i1.fps_n = 25;
  i1.fps_d = 1;
  caps1 = gst_video_info_to_caps (&i1);

  gst_video_info_init (&i2);
  gst_video_info_set_format (&i2, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i2.fps_n = 25;
  i2.fps_d = 1;
  caps2 = gst_video_info_to_caps (&i2);

  pipeline =
      setup_imagefreeze (caps1, caps2,
      G_CALLBACK (sink_handoff_cb_25_1_220ms_380ms), &n_buffers);

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PAUSED),
      GST_STATE_CHANGE_SUCCESS);

  fail_unless (gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME,
          GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 220 * GST_MSECOND,
          GST_SEEK_TYPE_SET, 380 * GST_MSECOND));

  n_buffers = 0;

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless_equals_int (n_buffers, 5);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  gst_caps_unref (caps1);
  gst_caps_unref (caps2);
  g_source_remove (bus_watch);
}

GST_END_TEST;

static void
sink_handoff_cb_count_buffers (GstElement * object, GstBuffer * buffer,
    GstPad * pad, gpointer user_data)
{
  guint *n_buffers = (guint *) user_data;

  if (*n_buffers == G_MAXUINT)
    return;

  *n_buffers = *n_buffers + 1;
}

GST_START_TEST (test_imagefreeze_num_buffers)
{
  GstElement *pipeline;
  GstElement *imagefreeze;
  GstCaps *caps1, *caps2;
  GstBus *bus;
  GMainLoop *loop;
  guint n_buffers = G_MAXUINT;
  guint bus_watch = 0;
  GstVideoInfo i1, i2;

  gst_video_info_init (&i1);
  gst_video_info_set_format (&i1, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i1.fps_n = 25;
  i1.fps_d = 1;
  caps1 = gst_video_info_to_caps (&i1);

  gst_video_info_init (&i2);
  gst_video_info_set_format (&i2, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i2.fps_n = 25;
  i2.fps_d = 1;
  caps2 = gst_video_info_to_caps (&i2);

  pipeline =
      setup_imagefreeze (caps1, caps2,
      G_CALLBACK (sink_handoff_cb_count_buffers), &n_buffers);

  imagefreeze = gst_bin_get_by_name (GST_BIN (pipeline), "freeze");
  fail_unless (imagefreeze != NULL);

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  /* Check that 0 buffers have been pushed */
  g_object_set (imagefreeze, "num-buffers", 0, NULL);
  n_buffers = 0;

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless_equals_int (n_buffers, 0);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  /* Check that the exact number of buffers have been pushed */
  g_object_set (imagefreeze, "num-buffers", 100, NULL);
  n_buffers = 0;

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless_equals_int (n_buffers, 100);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (imagefreeze);
  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  gst_caps_unref (caps1);
  gst_caps_unref (caps2);
  g_source_remove (bus_watch);
}

GST_END_TEST;

GST_START_TEST (test_imagefreeze_eos)
{
  GstElement *pipeline;
  GstElement *src;
  GstCaps *caps1, *caps2;
  GstBus *bus;
  GMainLoop *loop;
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 position;
  guint bus_watch = 0;
  GstVideoInfo i1, i2;

  gst_video_info_init (&i1);
  gst_video_info_set_format (&i1, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i1.fps_n = 25;
  i1.fps_d = 1;
  caps1 = gst_video_info_to_caps (&i1);

  gst_video_info_init (&i2);
  gst_video_info_set_format (&i2, GST_VIDEO_FORMAT_xRGB, 640, 480);
  i2.fps_n = 25;
  i2.fps_d = 1;
  caps2 = gst_video_info_to_caps (&i2);

  pipeline = setup_imagefreeze (caps1, caps2, NULL, NULL);

  src = gst_bin_get_by_name (GST_BIN (pipeline), "src");
  fail_unless (src != NULL);
  g_object_set (src, "num-buffers", 100, NULL);

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PAUSED),
      GST_STATE_CHANGE_SUCCESS);

  fail_unless (gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME,
          GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET,
          400 * GST_MSECOND));

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless (gst_element_query_position (src, fmt, &position));
  fail_unless_equals_uint64 (position, 40 * GST_MSECOND);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (src);
  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  gst_caps_unref (caps1);
  gst_caps_unref (caps2);
  g_source_remove (bus_watch);
}

GST_END_TEST;

static Suite *
imagefreeze_suite (void)
{
  Suite *s = suite_create ("imagefreeze");
  TCase *tc_chain = tcase_create ("linear");

  /* time out after 120s, not the default 3 */
  tcase_set_timeout (tc_chain, 120);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_imagefreeze_0_1);
  tcase_add_test (tc_chain, test_imagefreeze_25_1_0ms_400ms);
  tcase_add_test (tc_chain, test_imagefreeze_25_1_200ms_400ms);
  tcase_add_test (tc_chain, test_imagefreeze_25_1_400ms_0ms);
  tcase_add_test (tc_chain, test_imagefreeze_25_1_220ms_380ms);

  tcase_add_test (tc_chain, test_imagefreeze_num_buffers);
  tcase_add_test (tc_chain, test_imagefreeze_eos);

  return s;
}

GST_CHECK_MAIN (imagefreeze);
