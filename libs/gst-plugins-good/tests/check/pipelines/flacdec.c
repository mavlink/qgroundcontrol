/* GStreamer
 * Copyright (C) 2009 Thomas Vander Stichele <thomas at apestaart dot org>
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
#include <glib/gstdio.h>

static guint16
_get_first_sample (GstSample * sample)
{
  GstAudioInfo info;
  GstCaps *caps;
  GstBuffer *buf;
  GstMapInfo map;
  guint16 res;

  fail_unless (sample != NULL, "NULL sample");

  caps = gst_sample_get_caps (sample);
  fail_unless (caps != NULL, "sample without caps");

  buf = gst_sample_get_buffer (sample);
  GST_DEBUG ("buffer with size=%" G_GSIZE_FORMAT ", caps=%" GST_PTR_FORMAT,
      gst_buffer_get_size (buf), caps);

  gst_buffer_map (buf, &map, GST_MAP_READ);
  /* log buffer details */
  GST_MEMDUMP ("buffer data from decoder", map.data, map.size);

  /* make sure it's the format we expect */
  fail_unless (gst_audio_info_from_caps (&info, caps));

  fail_unless_equals_int (GST_AUDIO_INFO_WIDTH (&info), 16);
  fail_unless_equals_int (GST_AUDIO_INFO_DEPTH (&info), 16);
  fail_unless_equals_int (GST_AUDIO_INFO_RATE (&info), 44100);
  fail_unless_equals_int (GST_AUDIO_INFO_CHANNELS (&info), 1);

  if (GST_AUDIO_INFO_IS_LITTLE_ENDIAN (&info))
    res = GST_READ_UINT16_LE (map.data);
  else
    res = GST_READ_UINT16_BE (map.data);

  gst_buffer_unmap (buf, &map);

  return res;
}

GST_START_TEST (test_decode)
{
  GstElement *pipeline;
  GstElement *appsink;
  GstSample *sample = NULL;
  guint16 first_sample = 0;
  guint size = 0;
  gchar *path =
      g_build_filename (GST_TEST_FILES_PATH, "audiotestsrc.flac", NULL);
  gchar *pipe_desc =
      g_strdup_printf
      ("filesrc location=\"%s\" ! flacparse ! flacdec ! appsink name=sink",
      path);

  pipeline = gst_parse_launch (pipe_desc, NULL);
  fail_unless (pipeline != NULL);

  g_free (path);
  g_free (pipe_desc);

  appsink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");
  fail_unless (appsink != NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  do {
    g_signal_emit_by_name (appsink, "pull-sample", &sample);
    if (sample == NULL)
      break;
    if (first_sample == 0)
      first_sample = _get_first_sample (sample);

    size += gst_buffer_get_size (gst_sample_get_buffer (sample));

    gst_sample_unref (sample);
    sample = NULL;
  }
  while (TRUE);

  /* audiotestsrc with samplesperbuffer 1024 and 10 num-buffers */
  fail_unless_equals_int (size, 20480);
  fail_unless_equals_int (first_sample, 0x066a);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);
  g_object_unref (appsink);
}

GST_END_TEST;

GST_START_TEST (test_decode_seek_full)
{
  GstElement *pipeline;
  GstElement *appsink;
  GstEvent *event;
  GstSample *sample = NULL;
  guint16 first_sample = 0;
  guint size = 0;
  gchar *path =
      g_build_filename (GST_TEST_FILES_PATH, "audiotestsrc.flac", NULL);
  gchar *pipe_desc =
      g_strdup_printf
      ("filesrc location=\"%s\" ! flacparse ! flacdec ! appsink name=sink",
      path);

  pipeline = gst_parse_launch (pipe_desc, NULL);
  fail_unless (pipeline != NULL);

  g_free (pipe_desc);
  g_free (path);

  appsink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");
  fail_unless (appsink != NULL);

  gst_element_set_state (pipeline, GST_STATE_PAUSED);
  gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

  /* do a seek that should give us the complete output */
  event = gst_event_new_seek (1.0, GST_FORMAT_DEFAULT, GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, 20480);
  fail_unless (gst_element_send_event (appsink, event));

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  do {
    g_signal_emit_by_name (appsink, "pull-sample", &sample);
    if (sample == NULL)
      break;
    if (first_sample == 0)
      first_sample = _get_first_sample (sample);
    size += gst_buffer_get_size (gst_sample_get_buffer (sample));

    gst_sample_unref (sample);
    sample = NULL;
  }
  while (TRUE);

  /* file was generated with audiotestsrc
   * with 1024 samplesperbuffer and 10 num-buffers in 16 bit audio */
  fail_unless_equals_int (size, 20480);
  fail_unless_equals_int (first_sample, 0x066a);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_object_unref (pipeline);
  g_object_unref (appsink);
}

GST_END_TEST;

GST_START_TEST (test_decode_seek_partial)
{
  GstElement *pipeline;
  GstElement *appsink;
  GstEvent *event;
  GstSample *sample = NULL;
  guint size = 0;
  guint16 first_sample = 0;
  gchar *path =
      g_build_filename (GST_TEST_FILES_PATH, "audiotestsrc.flac", NULL);
  gchar *pipe_desc =
      g_strdup_printf
      ("filesrc location=\"%s\" ! flacparse ! flacdec ! appsink name=sink",
      path);

  pipeline = gst_parse_launch (pipe_desc, NULL);
  fail_unless (pipeline != NULL);

  g_free (path);
  g_free (pipe_desc);

  appsink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");
  fail_unless (appsink != NULL);

  gst_element_set_state (pipeline, GST_STATE_PAUSED);
  gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

  /* do a partial seek to get the first 1024 samples or 2048 bytes */
  event = gst_event_new_seek (1.0, GST_FORMAT_DEFAULT, GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, 1024);
  GST_DEBUG ("seeking");
  fail_unless (gst_element_send_event (appsink, event));
  GST_DEBUG ("seeked");

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  do {
    GST_DEBUG ("pulling sample");
    g_signal_emit_by_name (appsink, "pull-sample", &sample);
    GST_DEBUG ("pulled sample %p", sample);
    if (sample == NULL)
      break;
    if (first_sample == 0) {
      first_sample = _get_first_sample (sample);
    }
    size += gst_buffer_get_size (gst_sample_get_buffer (sample));

    gst_sample_unref (sample);
    sample = NULL;
  }
  while (TRUE);

  /* allow for sample round-up clipping effect */
  fail_unless (size == 2048 || size == 2050);
  fail_unless_equals_int (first_sample, 0x066a);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_object_unref (pipeline);
  g_object_unref (appsink);
}

GST_END_TEST;

static Suite *
flacdec_suite (void)
{
  Suite *s = suite_create ("flacdec");

  TCase *tc_chain = tcase_create ("linear");

  /* time out after 60s, not the default 3 */
  tcase_set_timeout (tc_chain, 60);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_decode);
  tcase_add_test (tc_chain, test_decode_seek_full);
  tcase_add_test (tc_chain, test_decode_seek_partial);

  return s;
}

GST_CHECK_MAIN (flacdec);
