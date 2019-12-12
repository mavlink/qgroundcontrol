/* GStreamer unit test for the autodetect elements
 *
 * Copyright (C) <2006> Tim-Philipp Müller <tim centricular net>
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gst/audio/audio.h>
#include <gst/base/gstbasesink.h>
#include <gst/check/gstcheck.h>
#include <gst/video/video.h>

/* dummy sink elements */

#define GST_TYPE_FAKE_AUDIO_SINK (gst_fake_audio_sink_get_type ())
typedef GstBaseSink GstFakeAudioSink;
typedef GstBaseSinkClass GstFakeAudioSinkClass;
GType gst_fake_audio_sink_get_type (void);
G_DEFINE_TYPE (GstFakeAudioSink, gst_fake_audio_sink, GST_TYPE_BASE_SINK);

static void
gst_fake_audio_sink_class_init (GstFakeAudioSinkClass * klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  static GstStaticPadTemplate pad_template = GST_STATIC_PAD_TEMPLATE ("sink",
      GST_PAD_SINK, GST_PAD_ALWAYS,
      GST_STATIC_CAPS (GST_AUDIO_CAPS_MAKE (GST_AUDIO_FORMATS_ALL) "; "));

  gst_element_class_set_static_metadata (gstelement_class, "Fake Audio Sink",
      "Sink/Audio", "Audio sink fake for testing", "Stefan Sauer");
  gst_element_class_add_static_pad_template (gstelement_class, &pad_template);
}

static void
gst_fake_audio_sink_init (GstFakeAudioSink * sink)
{
}

#define GST_TYPE_FAKE_VIDEO_SINK (gst_fake_video_sink_get_type ())
typedef GstBaseSink GstFakeVideoSink;
typedef GstBaseSinkClass GstFakeVideoSinkClass;
GType gst_fake_video_sink_get_type (void);
G_DEFINE_TYPE (GstFakeVideoSink, gst_fake_video_sink, GST_TYPE_BASE_SINK);

static void
gst_fake_video_sink_class_init (GstFakeVideoSinkClass * klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  static GstStaticPadTemplate pad_template = GST_STATIC_PAD_TEMPLATE ("sink",
      GST_PAD_SINK, GST_PAD_ALWAYS,
      GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (GST_VIDEO_FORMATS_ALL) "; "));

  gst_element_class_set_static_metadata (gstelement_class, "Fake Video Sink",
      "Sink/Video", "Video sink fake for testing", "Stefan Sauer");
  gst_element_class_add_static_pad_template (gstelement_class, &pad_template);
}

static void
gst_fake_video_sink_init (GstFakeVideoSink * sink)
{
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "fakeaudiosink", G_MAXINT,
          GST_TYPE_FAKE_AUDIO_SINK))
    return FALSE;
  if (!gst_element_register (plugin, "fakevideosink", G_MAXINT,
          GST_TYPE_FAKE_VIDEO_SINK))
    return FALSE;
  return TRUE;
}

/* tests */

static void
test_plugs_best (GstElement * sink, GType expected)
{
  GstStateChangeReturn state_ret;
  GList *child;

  state_ret = gst_element_set_state (sink, GST_STATE_READY);
  fail_unless (state_ret == GST_STATE_CHANGE_SUCCESS, NULL);

  child = GST_BIN_CHILDREN (sink);
  fail_unless (child != NULL, "no elements plugged");
  ck_assert_int_eq (g_list_length (child), 1);
  fail_unless (G_OBJECT_TYPE (child), expected);

  /* clean up */
  gst_element_set_state (sink, GST_STATE_NULL);
  gst_object_unref (sink);
}

GST_START_TEST (test_autovideosink_plugs_best)
{
  GstElement *sink = gst_element_factory_make ("autovideosink", NULL);

  test_plugs_best (sink, GST_TYPE_FAKE_VIDEO_SINK);
}

GST_END_TEST;

GST_START_TEST (test_autoaudiosink_plugs_best)
{
  GstElement *sink = gst_element_factory_make ("autoaudiosink", NULL);

  test_plugs_best (sink, GST_TYPE_FAKE_AUDIO_SINK);
}

GST_END_TEST;

static void
test_ghostpad_error_case (GstCaps * caps, GstElement * sink,
    const gchar * fake_sink_name)
{
  GstStateChangeReturn state_ret;
  GstElement *pipeline, *src, *filter, *fakesink;

  pipeline = gst_pipeline_new ("pipeline");
  src = gst_element_factory_make ("fakesrc", NULL);
  filter = gst_element_factory_make ("capsfilter", NULL);

  g_object_set (filter, "caps", caps, NULL);
  gst_caps_unref (caps);

  gst_bin_add_many (GST_BIN (pipeline), src, filter, sink, NULL);

  fail_unless (gst_element_link (src, filter), "Failed to link src to filter");
  fail_unless (gst_element_link (filter, sink),
      "Failed to link filter to sink");

  /* this should fail, there's no such format */
  state_ret = gst_element_set_state (pipeline, GST_STATE_PAUSED);
  if (state_ret == GST_STATE_CHANGE_ASYNC) {
    /* make sure we wait for the actual success/failure to happen */
    GstState state;
    state_ret =
        gst_element_get_state (pipeline, &state, &state, GST_CLOCK_TIME_NONE);
  }
  fakesink = gst_bin_get_by_name (GST_BIN (sink), fake_sink_name);
  if (fakesink != NULL) {
    /* no real sink available */
    fail_unless (state_ret == GST_STATE_CHANGE_SUCCESS,
        "pipeline _set_state() to PAUSED failed");
    gst_object_unref (fakesink);
  } else {
    /* autovideosink contains a real video sink */
    fail_unless (state_ret == GST_STATE_CHANGE_FAILURE,
        "pipeline _set_state() to PAUSED succeeded but should have failed");

    /* so, we hit an error and try to shut down the pipeline; this shouldn't
     * deadlock or block anywhere when autovideosink resets the ghostpad
     * targets etc. */
  }
  state_ret = gst_element_set_state (pipeline, GST_STATE_NULL);
  fail_unless (state_ret == GST_STATE_CHANGE_SUCCESS,
      "State change on pipeline failed");

  /* clean up */
  gst_object_unref (pipeline);
}

GST_START_TEST (test_autovideosink_ghostpad_error_case)
{
  GstElement *sink = gst_element_factory_make ("autovideosink", NULL);
  GstCaps *caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING,
      "ABCD", NULL);

  test_ghostpad_error_case (caps, sink, "fake-video-sink");
}

GST_END_TEST;

GST_START_TEST (test_autoaudiosink_ghostpad_error_case)
{
  GstElement *sink = gst_element_factory_make ("autoaudiosink", NULL);
  GstCaps *caps = gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING,
      "ABCD", NULL);

  test_ghostpad_error_case (caps, sink, "fake-audio-sink");
}

GST_END_TEST;

static Suite *
autodetect_suite (void)
{
  Suite *s = suite_create ("autodetect");
  TCase *tc_chain = tcase_create ("general");

  gst_plugin_register_static (GST_VERSION_MAJOR,
      GST_VERSION_MINOR,
      "autodetect-test",
      "autodetect test elements",
      plugin_init,
      VERSION, "LGPL", PACKAGE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_autovideosink_plugs_best);
  tcase_add_test (tc_chain, test_autoaudiosink_plugs_best);
  tcase_add_test (tc_chain, test_autovideosink_ghostpad_error_case);
  tcase_add_test (tc_chain, test_autoaudiosink_ghostpad_error_case);

  return s;
}

GST_CHECK_MAIN (autodetect);
