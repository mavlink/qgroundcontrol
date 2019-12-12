/* GStreamer unit test for splitmuxsrc/sink elements
 *
 * Copyright (C) 2007 David A. Schleef <ds@schleef.org>
 * Copyright (C) 2015 Jan Schmidt <jan@centricular.com>
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
#  include "config.h"
#endif

#include <glib/gstdio.h>

#include <gst/check/gstcheck.h>
#include <gst/app/app.h>
#include <stdlib.h>

gchar *tmpdir = NULL;
GstClockTime first_ts;
GstClockTime last_ts;
gdouble current_rate;

static void
tempdir_setup (void)
{
  const gchar *systmp = g_get_tmp_dir ();
  tmpdir = g_build_filename (systmp, "splitmux-test-XXXXXX", NULL);
  /* Rewrites tmpdir template input: */
  tmpdir = g_mkdtemp (tmpdir);
}

static void
tempdir_cleanup (void)
{
  GDir *d;
  const gchar *f;

  fail_if (tmpdir == NULL);

  d = g_dir_open (tmpdir, 0, NULL);
  fail_if (d == NULL);

  while ((f = g_dir_read_name (d)) != NULL) {
    gchar *fname = g_build_filename (tmpdir, f, NULL);
    fail_if (g_remove (fname) != 0, "Failed to remove tmp file %s", fname);
    g_free (fname);
  }
  g_dir_close (d);

  fail_if (g_remove (tmpdir) != 0, "Failed to delete tmpdir %s", tmpdir);

  g_free (tmpdir);
  tmpdir = NULL;
}

static guint
count_files (const gchar * target)
{
  GDir *d;
  const gchar *f;
  guint ret = 0;

  d = g_dir_open (target, 0, NULL);
  fail_if (d == NULL);

  while ((f = g_dir_read_name (d)) != NULL)
    ret++;
  g_dir_close (d);

  return ret;
}

static void
dump_error (GstMessage * msg)
{
  GError *err = NULL;
  gchar *dbg_info;

  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR);

  gst_message_parse_error (msg, &err, &dbg_info);

  g_printerr ("ERROR from element %s: %s\n",
      GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
  g_error_free (err);
  g_free (dbg_info);
}

static GstMessage *
run_pipeline (GstElement * pipeline)
{
  GstBus *bus = gst_element_get_bus (GST_ELEMENT (pipeline));
  GstMessage *msg;

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  msg = gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, -1);
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (bus);

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);

  return msg;
}

static void
seek_pipeline (GstElement * pipeline, gdouble rate, GstClockTime start,
    GstClockTime end)
{
  /* Pause the pipeline, seek to the desired range / rate, wait for PAUSED again, then
   * clear the tracking vars for start_ts / end_ts */
  gst_element_set_state (pipeline, GST_STATE_PAUSED);
  gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

  /* specific end time not implemented: */
  fail_unless (end == GST_CLOCK_TIME_NONE);

  gst_element_seek (pipeline, rate, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET, start,
      GST_SEEK_TYPE_END, 0);

  /* Wait for the pipeline to preroll again */
  gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

  GST_LOG ("Seeked pipeline. Rate %f time range %" GST_TIME_FORMAT " to %"
      GST_TIME_FORMAT, rate, GST_TIME_ARGS (start), GST_TIME_ARGS (end));

  /* Clear tracking variables now that the seek is complete */
  first_ts = last_ts = GST_CLOCK_TIME_NONE;
  current_rate = rate;
};

static void
receive_handoff (GstElement * object G_GNUC_UNUSED, GstBuffer * buf,
    GstPad * arg1 G_GNUC_UNUSED, gpointer user_data G_GNUC_UNUSED)
{
  GstClockTime start = GST_BUFFER_TIMESTAMP (buf);
  GstClockTime end = start;

  if (GST_BUFFER_DURATION_IS_VALID (buf))
    end += GST_BUFFER_DURATION (buf);

  GST_LOG ("Got buffer %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (start), GST_TIME_ARGS (end));

  /* Check time is moving in the right direction */
  if (current_rate > 0) {
    if (GST_CLOCK_TIME_IS_VALID (first_ts))
      fail_unless (start >= first_ts,
          "Timestamps went backward during forward play, %" GST_TIME_FORMAT
          " < %" GST_TIME_FORMAT, GST_TIME_ARGS (start),
          GST_TIME_ARGS (first_ts));
    if (GST_CLOCK_TIME_IS_VALID (last_ts))
      fail_unless (end >= last_ts,
          "Timestamps went backward during forward play, %" GST_TIME_FORMAT
          " < %" GST_TIME_FORMAT, GST_TIME_ARGS (end), GST_TIME_ARGS (last_ts));
  } else {
    fail_unless (start <= first_ts,
        "Timestamps went forward during reverse play, %" GST_TIME_FORMAT " > %"
        GST_TIME_FORMAT, GST_TIME_ARGS (start), GST_TIME_ARGS (first_ts));
    fail_unless (end <= last_ts,
        "Timestamps went forward during reverse play, %" GST_TIME_FORMAT " > %"
        GST_TIME_FORMAT, GST_TIME_ARGS (end), GST_TIME_ARGS (last_ts));
  }

  /* update the range of timestamps we've encountered */
  if (!GST_CLOCK_TIME_IS_VALID (first_ts) || start < first_ts)
    first_ts = start;
  if (!GST_CLOCK_TIME_IS_VALID (last_ts) || end > last_ts)
    last_ts = end;
}

static void
test_playback (const gchar * in_pattern, GstClockTime exp_first_time,
    GstClockTime exp_last_time, gboolean test_reverse)
{
  GstMessage *msg;
  GstElement *pipeline;
  GstElement *fakesink;
  GstElement *fakesink2;
  gchar *uri;

  pipeline = gst_element_factory_make ("playbin", NULL);
  fail_if (pipeline == NULL);

  fakesink = gst_element_factory_make ("fakesink", NULL);
  fail_if (fakesink == NULL);
  g_object_set (G_OBJECT (pipeline), "video-sink", fakesink, NULL);
  fakesink2 = gst_element_factory_make ("fakesink", NULL);
  fail_if (fakesink2 == NULL);
  g_object_set (G_OBJECT (pipeline), "audio-sink", fakesink2, NULL);

  uri = g_strdup_printf ("splitmux://%s", in_pattern);

  g_object_set (G_OBJECT (pipeline), "uri", uri, NULL);
  g_free (uri);

  g_signal_connect (fakesink, "handoff", (GCallback) receive_handoff, NULL);
  g_object_set (G_OBJECT (fakesink), "signal-handoffs", TRUE, NULL);

  /* test forwards */
  seek_pipeline (pipeline, 1.0, 0, -1);
  fail_unless (first_ts == GST_CLOCK_TIME_NONE);
  msg = run_pipeline (pipeline);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);

  /* Check we saw the entire range of values */
  fail_unless (first_ts == exp_first_time,
      "Expected start of playback range 0, got %" GST_TIME_FORMAT,
      GST_TIME_ARGS (first_ts));
  fail_unless (last_ts == exp_last_time,
      "Expected end of playback range 3s, got %" GST_TIME_FORMAT,
      GST_TIME_ARGS (last_ts));

  if (test_reverse) {
    /* Test backwards */
    seek_pipeline (pipeline, -1.0, 0, -1);
    msg = run_pipeline (pipeline);
    fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
    gst_message_unref (msg);
    /* Check we saw the entire range of values */
    fail_unless (first_ts == exp_first_time,
        "Expected start of playback range %" GST_TIME_FORMAT
        ", got %" GST_TIME_FORMAT, GST_TIME_ARGS (exp_first_time),
        GST_TIME_ARGS (first_ts));
    fail_unless (last_ts == exp_last_time,
        "Expected end of playback range %" GST_TIME_FORMAT
        ", got %" GST_TIME_FORMAT, GST_TIME_ARGS (exp_last_time),
        GST_TIME_ARGS (last_ts));
  }

  gst_object_unref (pipeline);
}

GST_START_TEST (test_splitmuxsrc)
{
  gchar *in_pattern =
      g_build_filename (GST_TEST_FILES_PATH, "splitvideo*.ogg", NULL);
  test_playback (in_pattern, 0, 3 * GST_SECOND, TRUE);
  g_free (in_pattern);
}

GST_END_TEST;

static gchar **
src_format_location_cb (GstElement * splitmuxsrc, gpointer user_data)
{
  gchar **result = g_malloc0_n (4, sizeof (gchar *));
  result[0] = g_build_filename (GST_TEST_FILES_PATH, "splitvideo00.ogg", NULL);
  result[1] = g_build_filename (GST_TEST_FILES_PATH, "splitvideo01.ogg", NULL);
  result[2] = g_build_filename (GST_TEST_FILES_PATH, "splitvideo02.ogg", NULL);
  return result;
}

GST_START_TEST (test_splitmuxsrc_format_location)
{
  GstMessage *msg;
  GstElement *pipeline;
  GstElement *src;
  GError *error = NULL;

  pipeline = gst_parse_launch ("splitmuxsrc name=splitsrc ! decodebin "
      "! fakesink", &error);
  g_assert_no_error (error);
  fail_if (pipeline == NULL);

  src = gst_bin_get_by_name (GST_BIN (pipeline), "splitsrc");
  g_signal_connect (src, "format-location",
      (GCallback) src_format_location_cb, NULL);
  g_object_unref (src);

  msg = run_pipeline (pipeline);

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);
  gst_object_unref (pipeline);
}

GST_END_TEST;

static gchar *
check_format_location (GstElement * object,
    guint fragment_id, GstSample * first_sample)
{
  GstBuffer *buf = gst_sample_get_buffer (first_sample);

  /* Must have a buffer */
  fail_if (buf == NULL);
  GST_LOG ("New file - first buffer %" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)));

  return NULL;
}

GST_START_TEST (test_splitmuxsink)
{
  GstMessage *msg;
  GstElement *pipeline;
  GstElement *sink;
  GstPad *splitmux_sink_pad;
  GstPad *enc_src_pad;
  gchar *dest_pattern;
  guint count;
  gchar *in_pattern;

  /* This pipeline has a small time cutoff - it should start a new file
   * every GOP, ie 1 second */
  pipeline =
      gst_parse_launch
      ("videotestsrc num-buffers=15 ! video/x-raw,width=80,height=64,framerate=5/1 ! videoconvert !"
      " queue ! theoraenc keyframe-force=5 ! splitmuxsink name=splitsink "
      " max-size-time=1000000 max-size-bytes=1000000 muxer=oggmux", NULL);
  fail_if (pipeline == NULL);
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (sink == NULL);
  g_signal_connect (sink, "format-location-full",
      (GCallback) check_format_location, NULL);
  dest_pattern = g_build_filename (tmpdir, "out%05d.ogg", NULL);
  g_object_set (G_OBJECT (sink), "location", dest_pattern, NULL);
  g_free (dest_pattern);
  g_object_unref (sink);

  msg = run_pipeline (pipeline);

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);

  /* unlink manually and release request pad to ensure that we *can* do that
   * - https://bugzilla.gnome.org/show_bug.cgi?id=753622 */
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (sink == NULL);
  splitmux_sink_pad = gst_element_get_static_pad (sink, "video");
  fail_if (splitmux_sink_pad == NULL);
  enc_src_pad = gst_pad_get_peer (splitmux_sink_pad);
  fail_if (enc_src_pad == NULL);
  fail_unless (gst_pad_unlink (enc_src_pad, splitmux_sink_pad));
  gst_object_unref (enc_src_pad);
  gst_element_release_request_pad (sink, splitmux_sink_pad);
  gst_object_unref (splitmux_sink_pad);
  /* at this point the pad must be released - try to find it again to verify */
  splitmux_sink_pad = gst_element_get_static_pad (sink, "video");
  fail_if (splitmux_sink_pad != NULL);
  g_object_unref (sink);

  gst_object_unref (pipeline);

  count = count_files (tmpdir);
  fail_unless (count == 3, "Expected 3 output files, got %d", count);

  in_pattern = g_build_filename (tmpdir, "out*.ogg", NULL);
  test_playback (in_pattern, 0, 3 * GST_SECOND, TRUE);
  g_free (in_pattern);
}

GST_END_TEST;

GST_START_TEST (test_splitmuxsink_multivid)
{
  GstMessage *msg;
  GstElement *pipeline;
  GstElement *sink;
  gchar *dest_pattern;
  guint count;
  gchar *in_pattern;

  /* This pipeline should start a new file every GOP, ie 1 second,
   * driven by the primary video stream and with 2 auxiliary video streams */
  pipeline =
      gst_parse_launch
      ("splitmuxsink name=splitsink "
      " max-size-time=1000000 max-size-bytes=1000000 muxer=qtmux "
      "videotestsrc num-buffers=15 ! video/x-raw,width=80,height=64,framerate=5/1 ! videoconvert !"
      " queue ! vp8enc keyframe-max-dist=5 ! splitsink.video "
      "videotestsrc num-buffers=15 pattern=snow ! video/x-raw,width=80,height=64,framerate=5/1 ! videoconvert !"
      " queue ! vp8enc keyframe-max-dist=6 ! splitsink.video_aux_0 "
      "videotestsrc num-buffers=15 pattern=ball ! video/x-raw,width=80,height=64,framerate=5/1 ! videoconvert !"
      " queue ! vp8enc keyframe-max-dist=8 ! splitsink.video_aux_1 ", NULL);
  fail_if (pipeline == NULL);
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (sink == NULL);
  g_signal_connect (sink, "format-location-full",
      (GCallback) check_format_location, NULL);
  dest_pattern = g_build_filename (tmpdir, "out%05d.m4v", NULL);
  g_object_set (G_OBJECT (sink), "location", dest_pattern, NULL);
  g_free (dest_pattern);
  g_object_unref (sink);

  msg = run_pipeline (pipeline);

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);

  gst_object_unref (pipeline);

  count = count_files (tmpdir);
  fail_unless (count == 3, "Expected 3 output files, got %d", count);

  in_pattern = g_build_filename (tmpdir, "out*.m4v", NULL);
  /* FIXME: Reverse playback works poorly with multiple video streams
   * in qtdemux (at least, maybe other demuxers) at the time this was
   * written, and causes test failures like buffers being output
   * multiple times by qtdemux as it loops through GOPs. Disable that
   * for now */
  test_playback (in_pattern, 0, 3 * GST_SECOND, FALSE);
  g_free (in_pattern);
}

GST_END_TEST;

GST_START_TEST (test_splitmuxsink_async)
{
  GstMessage *msg;
  GstElement *pipeline;
  GstElement *sink;
  GstPad *splitmux_sink_pad;
  GstPad *enc_src_pad;
  gchar *dest_pattern;
  guint count;
  gchar *in_pattern;

  pipeline =
      gst_parse_launch
      ("videotestsrc num-buffers=15 ! video/x-raw,width=80,height=64,framerate=5/1 ! videoconvert !"
      " queue ! theoraenc keyframe-force=5 ! splitmuxsink name=splitsink "
      " max-size-time=1000000000 async-finalize=true "
      " muxer-factory=matroskamux audiotestsrc num-buffers=15 samplesperbuffer=9600 ! "
      " audio/x-raw,rate=48000 ! splitsink.audio_%u", NULL);
  fail_if (pipeline == NULL);
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (sink == NULL);
  g_signal_connect (sink, "format-location-full",
      (GCallback) check_format_location, NULL);
  dest_pattern = g_build_filename (tmpdir, "matroska%05d.mkv", NULL);
  g_object_set (G_OBJECT (sink), "location", dest_pattern, NULL);
  g_free (dest_pattern);
  g_object_unref (sink);

  msg = run_pipeline (pipeline);

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);

  /* unlink manually and release request pad to ensure that we *can* do that
   * - https://bugzilla.gnome.org/show_bug.cgi?id=753622 */
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (sink == NULL);
  splitmux_sink_pad = gst_element_get_static_pad (sink, "video");
  fail_if (splitmux_sink_pad == NULL);
  enc_src_pad = gst_pad_get_peer (splitmux_sink_pad);
  fail_if (enc_src_pad == NULL);
  fail_unless (gst_pad_unlink (enc_src_pad, splitmux_sink_pad));
  gst_object_unref (enc_src_pad);
  gst_element_release_request_pad (sink, splitmux_sink_pad);
  gst_object_unref (splitmux_sink_pad);
  /* at this point the pad must be released - try to find it again to verify */
  splitmux_sink_pad = gst_element_get_static_pad (sink, "video");
  fail_if (splitmux_sink_pad != NULL);
  g_object_unref (sink);

  gst_object_unref (pipeline);

  count = count_files (tmpdir);
  fail_unless (count == 3, "Expected 3 output files, got %d", count);

  in_pattern = g_build_filename (tmpdir, "matroska*.mkv", NULL);
  test_playback (in_pattern, 0, 3 * GST_SECOND, TRUE);
  g_free (in_pattern);
}

GST_END_TEST;

static GstPadProbeReturn
intercept_stream_start (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  GstEvent *event = gst_pad_probe_info_get_event (info);

  if (GST_EVENT_TYPE (event) == GST_EVENT_STREAM_START) {
    GstStreamFlags flags;
    event = gst_event_make_writable (event);
    gst_event_parse_stream_flags (event, &flags);
    gst_event_set_stream_flags (event, flags | GST_STREAM_FLAG_SPARSE);
    GST_PAD_PROBE_INFO_DATA (info) = event;
  }

  return GST_PAD_PROBE_OK;
}

static GstFlowReturn
new_sample_verify_continuous_timestamps (GstAppSink * appsink,
    gpointer user_data)
{
  GstSample *sample;
  GstBuffer *buffer;
  GstClockTime *prev_ts = user_data;
  GstClockTime new_ts;

  sample = gst_app_sink_pull_sample (appsink);
  buffer = gst_sample_get_buffer (sample);

  new_ts = GST_BUFFER_PTS (buffer);
  if (GST_CLOCK_TIME_IS_VALID (*prev_ts)) {
    fail_unless (*prev_ts < new_ts,
        "%s: prev_ts (%" GST_TIME_FORMAT ") >= new_ts (%" GST_TIME_FORMAT ")",
        GST_OBJECT_NAME (appsink), GST_TIME_ARGS (*prev_ts),
        GST_TIME_ARGS (new_ts));
  }

  *prev_ts = new_ts;
  gst_sample_unref (sample);
  return GST_FLOW_OK;
}

static GstFlowReturn
new_sample_verify_1sec_offset (GstAppSink * appsink, gpointer user_data)
{
  GstSample *sample;
  GstBuffer *buffer;
  GstClockTime *prev_ts = user_data;
  GstClockTime new_ts;

  sample = gst_app_sink_pull_sample (appsink);
  buffer = gst_sample_get_buffer (sample);

  new_ts = GST_BUFFER_PTS (buffer);
  if (GST_CLOCK_TIME_IS_VALID (*prev_ts)) {
    fail_unless (new_ts > (*prev_ts + 900 * GST_MSECOND),
        "%s: prev_ts (%" GST_TIME_FORMAT ") + 0.9s >= new_ts (%"
        GST_TIME_FORMAT ")", GST_OBJECT_NAME (appsink),
        GST_TIME_ARGS (*prev_ts), GST_TIME_ARGS (new_ts));
  }

  *prev_ts = new_ts;
  gst_sample_unref (sample);
  return GST_FLOW_OK;
}

/* https://bugzilla.gnome.org/show_bug.cgi?id=761086 */
GST_START_TEST (test_splitmuxsrc_sparse_streams)
{
  GstElement *pipeline;
  GstElement *element;
  gchar *dest_pattern;
  GstElement *appsrc;
  GstPad *appsrc_src;
  GstBus *bus;
  GstMessage *msg;
  gint i;

  /* generate files */

  /* in this test, we have 5sec of data with files split at 1sec intervals */
  pipeline =
      gst_parse_launch
      ("videotestsrc num-buffers=75 !"
      "  video/x-raw,width=80,height=64,framerate=15/1 !"
      "  theoraenc keyframe-force=5 ! splitmuxsink name=splitsink"
      "    max-size-time=1000000000 muxer=matroskamux"
      " audiotestsrc num-buffers=100 samplesperbuffer=1024 !"
      "  audio/x-raw,rate=20000 ! vorbisenc ! splitsink.audio_%u"
      " appsrc name=appsrc format=time caps=text/x-raw,format=utf8 !"
      "  splitsink.subtitle_%u", NULL);
  fail_if (pipeline == NULL);

  element = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (element == NULL);
  dest_pattern = g_build_filename (tmpdir, "out%05d.ogg", NULL);
  g_object_set (G_OBJECT (element), "location", dest_pattern, NULL);
  g_clear_pointer (&dest_pattern, g_free);
  g_clear_object (&element);

  appsrc = gst_bin_get_by_name (GST_BIN (pipeline), "appsrc");
  fail_if (appsrc == NULL);

  /* add the SPARSE flag on the stream-start event of the subtitle stream */
  appsrc_src = gst_element_get_static_pad (appsrc, "src");
  fail_if (appsrc_src == NULL);
  gst_pad_add_probe (appsrc_src, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
      intercept_stream_start, NULL, NULL);
  g_clear_object (&appsrc_src);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* push subtitles, one per second, starting from t=100ms */
  for (i = 0; i < 5; i++) {
    GstBuffer *buffer = gst_buffer_new_allocate (NULL, 5, NULL);
    GstMapInfo info;

    gst_buffer_map (buffer, &info, GST_MAP_WRITE);
    strcpy ((char *) info.data, "test");
    gst_buffer_unmap (buffer, &info);

    GST_BUFFER_PTS (buffer) = i * GST_SECOND + 100 * GST_MSECOND;
    GST_BUFFER_DTS (buffer) = GST_BUFFER_PTS (buffer);

    fail_if (gst_app_src_push_buffer (GST_APP_SRC (appsrc), buffer)
        != GST_FLOW_OK);
  }
  fail_if (gst_app_src_end_of_stream (GST_APP_SRC (appsrc)) != GST_FLOW_OK);

  msg = gst_bus_timed_pop_filtered (bus, 5 * GST_SECOND, GST_MESSAGE_EOS);
  g_clear_pointer (&msg, gst_message_unref);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_clear_object (&appsrc);
  g_clear_object (&bus);
  g_clear_object (&pipeline);

  /* read and verify */

  pipeline =
      gst_parse_launch
      ("splitmuxsrc name=splitsrc"
      " splitsrc. ! theoradec ! appsink name=vsink sync=false emit-signals=true"
      " splitsrc. ! vorbisdec ! appsink name=asink sync=false emit-signals=true"
      " splitsrc. ! text/x-raw ! appsink name=tsink sync=false emit-signals=true",
      NULL);
  fail_if (pipeline == NULL);

  element = gst_bin_get_by_name (GST_BIN (pipeline), "splitsrc");
  fail_if (element == NULL);
  dest_pattern = g_build_filename (tmpdir, "out*.ogg", NULL);
  g_object_set (G_OBJECT (element), "location", dest_pattern, NULL);
  g_clear_pointer (&dest_pattern, g_free);
  g_clear_object (&element);

  {
    GstClockTime vsink_prev_ts = GST_CLOCK_TIME_NONE;
    GstClockTime asink_prev_ts = GST_CLOCK_TIME_NONE;
    GstClockTime tsink_prev_ts = GST_CLOCK_TIME_NONE;

    /* verify that timestamps are continuously increasing for audio + video.
     * if we hit bug 761086, timestamps will jump about -900ms after switching
     * to a new part, because this is the difference between the last subtitle
     * pts and the last audio/video pts */
    element = gst_bin_get_by_name (GST_BIN (pipeline), "vsink");
    g_signal_connect (element, "new-sample",
        (GCallback) new_sample_verify_continuous_timestamps, &vsink_prev_ts);
    g_clear_object (&element);

    element = gst_bin_get_by_name (GST_BIN (pipeline), "asink");
    g_signal_connect (element, "new-sample",
        (GCallback) new_sample_verify_continuous_timestamps, &asink_prev_ts);
    g_clear_object (&element);

    /* also verify that subtitle timestamps are increasing by about 1s.
     * if we hit bug 761086, timestamps will increase by exactly 100ms instead,
     * because this is the relative difference between a part's start time
     * (remember a new part starts every 1sec) and the subtitle's pts in that
     * part, which will be added to the max_ts of the previous part, which
     * equals the last subtitle's pts (and should not!) */
    element = gst_bin_get_by_name (GST_BIN (pipeline), "tsink");
    g_signal_connect (element, "new-sample",
        (GCallback) new_sample_verify_1sec_offset, &tsink_prev_ts);
    g_clear_object (&element);

    msg = run_pipeline (pipeline);
  }

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);

  g_clear_pointer (&msg, gst_message_unref);
  g_clear_object (&pipeline);
}

GST_END_TEST;

struct CapsChangeData
{
  guint count;
  GstElement *cf;
};

static GstPadProbeReturn
switch_caps (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  struct CapsChangeData *data = (struct CapsChangeData *) (user_data);

  if (data->count == 4) {
    GST_INFO ("Saw 5 buffers to the encoder. Switching caps");
    gst_util_set_object_arg (G_OBJECT (data->cf), "caps",
        "video/x-raw,width=160,height=128,framerate=10/1");
  }
  data->count++;
  return GST_PAD_PROBE_OK;
}

GST_START_TEST (test_splitmuxsrc_caps_change)
{
  GstMessage *msg;
  GstElement *pipeline;
  GstElement *sink;
  GstElement *cf;
  GstPad *sinkpad;
  gchar *dest_pattern;
  guint count;
  gchar *in_pattern;
  struct CapsChangeData data;

  /* This test creates a new file only by changing the caps, which
   * qtmux will reject (for now - if qtmux starts supporting caps
   * changes, this test will break and need fixing/disabling */
  pipeline =
      gst_parse_launch
      ("videotestsrc num-buffers=10 !"
      "  capsfilter name=c caps=video/x-raw,width=80,height=64,framerate=10/1 !"
      "  jpegenc ! splitmuxsink name=splitsink muxer=qtmux", NULL);
  fail_if (pipeline == NULL);
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (sink == NULL);
  g_signal_connect (sink, "format-location-full",
      (GCallback) check_format_location, NULL);
  dest_pattern = g_build_filename (tmpdir, "out%05d.mp4", NULL);
  g_object_set (G_OBJECT (sink), "location", dest_pattern, NULL);
  g_free (dest_pattern);
  g_object_unref (sink);

  cf = gst_bin_get_by_name (GST_BIN (pipeline), "c");
  sinkpad = gst_element_get_static_pad (cf, "sink");

  data.cf = cf;
  data.count = 0;

  gst_pad_add_probe (sinkpad, GST_PAD_PROBE_TYPE_BUFFER,
      switch_caps, &data, NULL);

  gst_object_unref (sinkpad);
  gst_object_unref (cf);

  msg = run_pipeline (pipeline);

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);

  gst_object_unref (pipeline);

  count = count_files (tmpdir);
  fail_unless (count == 2, "Expected 2 output files, got %d", count);

  in_pattern = g_build_filename (tmpdir, "out*.mp4", NULL);
  test_playback (in_pattern, 0, GST_SECOND, TRUE);
  g_free (in_pattern);
}

GST_END_TEST;

GST_START_TEST (test_splitmuxsrc_robust_mux)
{
  GstMessage *msg;
  GstElement *pipeline;
  GstElement *sink;
  gchar *dest_pattern;
  gchar *in_pattern;

  /* This test creates a new file only by changing the caps, which
   * qtmux will reject (for now - if qtmux starts supporting caps
   * changes, this test will break and need fixing/disabling */
  pipeline =
      gst_parse_launch
      ("videotestsrc num-buffers=10 !"
      "  video/x-raw,width=80,height=64,framerate=10/1 !"
      "  jpegenc ! splitmuxsink name=splitsink muxer=\"qtmux reserved-bytes-per-sec=200 reserved-moov-update-period=100000000 \" max-size-time=500000000 use-robust-muxing=true",
      NULL);
  fail_if (pipeline == NULL);
  sink = gst_bin_get_by_name (GST_BIN (pipeline), "splitsink");
  fail_if (sink == NULL);
  g_signal_connect (sink, "format-location-full",
      (GCallback) check_format_location, NULL);
  dest_pattern = g_build_filename (tmpdir, "out%05d.mp4", NULL);
  g_object_set (G_OBJECT (sink), "location", dest_pattern, NULL);
  g_free (dest_pattern);
  g_object_unref (sink);

  msg = run_pipeline (pipeline);

  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
    dump_error (msg);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);

  gst_object_unref (pipeline);

  /* Unlike other tests, we don't check an explicit file size, because the overflow detection
   * can be racy (depends on exactly when buffers get handed to the muxer and when it updates the
   * reserved duration property. All we care about is that the muxing didn't fail because space ran out */

  in_pattern = g_build_filename (tmpdir, "out*.mp4", NULL);
  test_playback (in_pattern, 0, GST_SECOND, TRUE);
  g_free (in_pattern);
}

GST_END_TEST;

/* For verifying bug https://bugzilla.gnome.org/show_bug.cgi?id=762893 */
GST_START_TEST (test_splitmuxsink_reuse_simple)
{
  GstElement *sink;
  GstPad *pad;

  sink = gst_element_factory_make ("splitmuxsink", NULL);
  pad = gst_element_get_request_pad (sink, "video");
  fail_unless (pad != NULL);
  g_object_set (sink, "location", "/dev/null", NULL);

  fail_unless (gst_element_set_state (sink,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_ASYNC);
  fail_unless (gst_element_set_state (sink,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);
  fail_unless (gst_element_set_state (sink,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_ASYNC);
  fail_unless (gst_element_set_state (sink,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  gst_element_release_request_pad (sink, pad);
  gst_object_unref (pad);
  gst_object_unref (sink);
}

GST_END_TEST;

GST_START_TEST (test_splitmuxsink_muxer_pad_map)
{
  GstElement *sink, *muxer;
  GstPad *muxpad;
  GstPad *pad1 = NULL, *pad2 = NULL;
  GstStructure *pad_map;

  pad_map = gst_structure_new ("x-pad-map",
      "video", G_TYPE_STRING, "video_100",
      "audio_0", G_TYPE_STRING, "audio_101", NULL);

  muxer = gst_element_factory_make ("qtmux", NULL);
  fail_if (muxer == NULL);
  sink = gst_element_factory_make ("splitmuxsink", NULL);
  fail_if (sink == NULL);

  g_object_set (sink, "muxer", muxer, "muxer-pad-map", pad_map, NULL);
  gst_structure_free (pad_map);

  pad1 = gst_element_get_request_pad (sink, "video");
  fail_unless (g_str_equal ("video", GST_PAD_NAME (pad1)));
  muxpad = gst_element_get_static_pad (muxer, "video_100");
  fail_unless (muxpad != NULL);
  gst_object_unref (muxpad);

  pad2 = gst_element_get_request_pad (sink, "audio_0");
  fail_unless (g_str_equal ("audio_0", GST_PAD_NAME (pad2)));
  muxpad = gst_element_get_static_pad (muxer, "audio_101");
  fail_unless (muxpad != NULL);
  gst_object_unref (muxpad);

  g_object_set (sink, "location", "/dev/null", NULL);

  fail_unless (gst_element_set_state (sink,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_ASYNC);
  fail_unless (gst_element_set_state (sink,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  gst_element_release_request_pad (sink, pad1);
  gst_object_unref (pad1);
  gst_element_release_request_pad (sink, pad2);
  gst_object_unref (pad2);
  gst_object_unref (sink);
}

GST_END_TEST;


static Suite *
splitmux_suite (void)
{
  Suite *s = suite_create ("splitmux");
  TCase *tc_chain = tcase_create ("general");
  TCase *tc_chain_basic = tcase_create ("basic");
  TCase *tc_chain_complex = tcase_create ("complex");
  TCase *tc_chain_mp4_jpeg = tcase_create ("caps_change");
  gboolean have_theora, have_ogg, have_vorbis, have_matroska, have_qtmux,
      have_jpeg, have_vp8;

  /* we assume that if encoder/muxer are there, decoder/demuxer will be a well */
  have_theora = gst_registry_check_feature_version (gst_registry_get (),
      "theoraenc", GST_VERSION_MAJOR, GST_VERSION_MINOR, 0);
  have_ogg = gst_registry_check_feature_version (gst_registry_get (),
      "oggmux", GST_VERSION_MAJOR, GST_VERSION_MINOR, 0);
  have_vorbis = gst_registry_check_feature_version (gst_registry_get (),
      "vorbisenc", GST_VERSION_MAJOR, GST_VERSION_MINOR, 0);
  have_matroska = gst_registry_check_feature_version (gst_registry_get (),
      "matroskamux", GST_VERSION_MAJOR, GST_VERSION_MINOR, 0);
  have_qtmux = gst_registry_check_feature_version (gst_registry_get (),
      "qtmux", GST_VERSION_MAJOR, GST_VERSION_MINOR, 0);
  have_jpeg = gst_registry_check_feature_version (gst_registry_get (),
      "jpegenc", GST_VERSION_MAJOR, GST_VERSION_MINOR, 0);
  have_vp8 = gst_registry_check_feature_version (gst_registry_get (),
      "vp8enc", GST_VERSION_MAJOR, GST_VERSION_MINOR, 0);

  suite_add_tcase (s, tc_chain);
  suite_add_tcase (s, tc_chain_basic);
  suite_add_tcase (s, tc_chain_complex);
  suite_add_tcase (s, tc_chain_mp4_jpeg);

  tcase_add_test (tc_chain_basic, test_splitmuxsink_reuse_simple);

  if (have_theora && have_ogg) {
    tcase_add_checked_fixture (tc_chain, tempdir_setup, tempdir_cleanup);

    tcase_add_test (tc_chain, test_splitmuxsrc);
    tcase_add_test (tc_chain, test_splitmuxsrc_format_location);
    tcase_add_test (tc_chain, test_splitmuxsink);

    if (have_matroska && have_vorbis) {
      tcase_add_checked_fixture (tc_chain_complex, tempdir_setup,
          tempdir_cleanup);

      tcase_add_test (tc_chain_complex, test_splitmuxsrc_sparse_streams);
      tcase_add_test (tc_chain, test_splitmuxsink_async);
    } else {
      GST_INFO ("Skipping tests, missing plugins: matroska and/or vorbis");
    }
  } else {
    GST_INFO ("Skipping tests, missing plugins: theora and/or ogg");
  }


  if (have_qtmux && have_jpeg) {
    tcase_add_checked_fixture (tc_chain_mp4_jpeg, tempdir_setup,
        tempdir_cleanup);
    tcase_add_test (tc_chain_mp4_jpeg, test_splitmuxsrc_caps_change);
    tcase_add_test (tc_chain_mp4_jpeg, test_splitmuxsrc_robust_mux);
    tcase_add_test (tc_chain_mp4_jpeg, test_splitmuxsink_muxer_pad_map);
  } else {
    GST_INFO ("Skipping tests, missing plugins: jpegenc or mp4mux");
  }

  if (have_qtmux && have_vp8) {
    tcase_add_test (tc_chain, test_splitmuxsink_multivid);
  } else {
    GST_INFO ("Skipping tests, missing plugins: vp8enc or mp4mux");
  }
  return s;
}

GST_CHECK_MAIN (splitmux);
