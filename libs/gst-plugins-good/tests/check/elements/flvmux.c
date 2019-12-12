/* GStreamer unit tests for flvmux
 *
 * Copyright (C) 2009 Tim-Philipp Müller  <tim centricular net>
 * Copyright (C) 2016 Havard Graff <havard@pexip.com>
 * Copyright (C) 2016 David Buchmann <david@pexip.com>
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
# include "config.h"
#endif

#ifdef HAVE_VALGRIND
# include <valgrind/valgrind.h>
#endif

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>

#include <gst/gst.h>

static GstBusSyncReply
error_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
    GError *err = NULL;
    gchar *dbg = NULL;

    gst_message_parse_error (msg, &err, &dbg);
    g_error ("ERROR: %s\n%s\n", err->message, dbg);
  }

  return GST_BUS_PASS;
}

static void
handoff_cb (GstElement * element, GstBuffer * buf, GstPad * pad,
    gint * p_counter)
{
  *p_counter += 1;
  GST_LOG ("counter = %d", *p_counter);
}

static void
mux_pcm_audio (guint num_buffers, guint repeat)
{
  GstElement *src, *sink, *flvmux, *conv, *pipeline;
  GstPad *sinkpad, *srcpad;
  gint counter;

  GST_LOG ("num_buffers = %u", num_buffers);

  pipeline = gst_pipeline_new ("pipeline");
  fail_unless (pipeline != NULL, "Failed to create pipeline!");

  /* kids, don't use a sync handler for this at home, really; we do because
   * we just want to abort and nothing else */
  gst_bus_set_sync_handler (GST_ELEMENT_BUS (pipeline), error_cb, NULL, NULL);

  src = gst_element_factory_make ("audiotestsrc", "audiotestsrc");
  fail_unless (src != NULL, "Failed to create 'audiotestsrc' element!");

  g_object_set (src, "num-buffers", num_buffers, NULL);

  conv = gst_element_factory_make ("audioconvert", "audioconvert");
  fail_unless (conv != NULL, "Failed to create 'audioconvert' element!");

  flvmux = gst_element_factory_make ("flvmux", "flvmux");
  fail_unless (flvmux != NULL, "Failed to create 'flvmux' element!");

  sink = gst_element_factory_make ("fakesink", "fakesink");
  fail_unless (sink != NULL, "Failed to create 'fakesink' element!");

  g_object_set (sink, "signal-handoffs", TRUE, NULL);
  g_signal_connect (sink, "handoff", G_CALLBACK (handoff_cb), &counter);

  gst_bin_add_many (GST_BIN (pipeline), src, conv, flvmux, sink, NULL);

  fail_unless (gst_element_link (src, conv));
  fail_unless (gst_element_link (flvmux, sink));

  /* now link the elements */
  sinkpad = gst_element_get_request_pad (flvmux, "audio");
  fail_unless (sinkpad != NULL, "Could not get audio request pad");

  srcpad = gst_element_get_static_pad (conv, "src");
  fail_unless (srcpad != NULL, "Could not get audioconvert's source pad");

  fail_unless_equals_int (gst_pad_link (srcpad, sinkpad), GST_PAD_LINK_OK);

  gst_object_unref (srcpad);
  gst_object_unref (sinkpad);

  do {
    GstStateChangeReturn state_ret;
    GstMessage *msg;

    GST_LOG ("repeat=%d", repeat);

    counter = 0;

    state_ret = gst_element_set_state (pipeline, GST_STATE_PAUSED);
    fail_unless (state_ret != GST_STATE_CHANGE_FAILURE);

    if (state_ret == GST_STATE_CHANGE_ASYNC) {
      GST_LOG ("waiting for pipeline to reach PAUSED state");
      state_ret = gst_element_get_state (pipeline, NULL, NULL, -1);
      fail_unless_equals_int (state_ret, GST_STATE_CHANGE_SUCCESS);
    }

    GST_LOG ("PAUSED, let's do the rest of it");

    state_ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    fail_unless (state_ret != GST_STATE_CHANGE_FAILURE);

    msg = gst_bus_poll (GST_ELEMENT_BUS (pipeline), GST_MESSAGE_EOS, -1);
    fail_unless (msg != NULL, "Expected EOS message on bus!");

    GST_LOG ("EOS");
    gst_message_unref (msg);

    /* should have some output */
    fail_unless (counter > 2);

    fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_NULL),
        GST_STATE_CHANGE_SUCCESS);

    /* repeat = test re-usability */
    --repeat;
  } while (repeat > 0);

  gst_object_unref (pipeline);
}

GST_START_TEST (test_index_writing)
{
  /* note: there's a magic 128 value in flvmux when doing index writing */
  mux_pcm_audio (__i__ * 33 + 1, 2);
}

GST_END_TEST;

static GstBuffer *
create_buffer (guint8 * data, gsize size,
    GstClockTime timestamp, GstClockTime duration)
{
  GstBuffer *buf = gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
      data, size, 0, size, NULL, NULL);
  GST_BUFFER_PTS (buf) = timestamp;
  GST_BUFFER_DTS (buf) = timestamp;
  GST_BUFFER_DURATION (buf) = duration;
  GST_BUFFER_OFFSET (buf) = 0;
  GST_BUFFER_OFFSET_END (buf) = 0;
  return buf;
}

guint8 speex_hdr0[] = {
  0x53, 0x70, 0x65, 0x65, 0x78, 0x20, 0x20, 0x20,
  0x31, 0x2e, 0x32, 0x72, 0x63, 0x31, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x50, 0x00, 0x00, 0x00, 0x80, 0x3e, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
  0x40, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

guint8 speex_hdr1[] = {
  0x1f, 0x00, 0x00, 0x00, 0x45, 0x6e, 0x63, 0x6f,
  0x64, 0x65, 0x64, 0x20, 0x77, 0x69, 0x74, 0x68,
  0x20, 0x47, 0x53, 0x74, 0x72, 0x65, 0x61, 0x6d,
  0x65, 0x72, 0x20, 0x53, 0x70, 0x65, 0x65, 0x78,
  0x65, 0x6e, 0x63, 0x00, 0x00, 0x00, 0x00, 0x01
};

guint8 speex_buf[] = {
  0x36, 0x9d, 0x1b, 0x9a, 0x20, 0x00, 0x01, 0x68,
  0xe8, 0xe8, 0xe8, 0xe8, 0xe8, 0xe8, 0xe8, 0x84,
  0x00, 0xb4, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74,
  0x74, 0x42, 0x00, 0x5a, 0x3a, 0x3a, 0x3a, 0x3a,
  0x3a, 0x3a, 0x3a, 0x21, 0x00, 0x2d, 0x1d, 0x1d,
  0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1b, 0x3b, 0x60,
  0xab, 0xab, 0xab, 0xab, 0xab, 0x0a, 0xba, 0xba,
  0xba, 0xba, 0xb0, 0xab, 0xab, 0xab, 0xab, 0xab,
  0x0a, 0xba, 0xba, 0xba, 0xba, 0xb7
};

guint8 h264_buf[] = {
  0x00, 0x00, 0x00, 0x0b, 0x67, 0x42, 0xc0, 0x0c,
  0x95, 0xa7, 0x20, 0x1e, 0x11, 0x08, 0xd4, 0x00,
  0x00, 0x00, 0x04, 0x68, 0xce, 0x3c, 0x80, 0x00,
  0x00, 0x00, 0x55, 0x65, 0xb8, 0x04, 0x0e, 0x7e,
  0x1f, 0x22, 0x60, 0x34, 0x01, 0xe2, 0x00, 0x3c,
  0xe1, 0xfc, 0x91, 0x40, 0xa6, 0x9e, 0x07, 0x42,
  0x56, 0x44, 0x73, 0x75, 0x40, 0x9f, 0x0c, 0x87,
  0x83, 0xc9, 0x52, 0x60, 0x6d, 0xd8, 0x98, 0x01,
  0x16, 0xbd, 0x0f, 0xa6, 0xaf, 0x75, 0x83, 0xdd,
  0xfa, 0xe7, 0x8f, 0xe3, 0x58, 0x10, 0x0f, 0x5c,
  0x18, 0x2f, 0x41, 0x40, 0x23, 0x0b, 0x03, 0x70,
  0x00, 0xff, 0xe4, 0xa6, 0x7d, 0x7f, 0x3f, 0x76,
  0x01, 0xd0, 0x98, 0x2a, 0x0c, 0xb8, 0x02, 0x32,
  0xbc, 0x56, 0xfd, 0x34, 0x4f, 0xcf, 0xfe, 0xa0,
};

GST_START_TEST (test_speex_streamable)
{
  GstBuffer *buf;
  GstMapInfo map = GST_MAP_INFO_INIT;


  GstCaps *caps = gst_caps_new_simple ("audio/x-speex",
      "rate", G_TYPE_INT, 16000,
      "channels", G_TYPE_INT, 1,
      NULL);

  const GstClockTime base_time = 123456789;
  const GstClockTime duration_ms = 20;
  const GstClockTime duration = duration_ms * GST_MSECOND;

  GstHarness *h = gst_harness_new_with_padnames ("flvmux", "audio", "src");
  gst_harness_set_src_caps (h, caps);
  g_object_set (h->element, "streamable", 1, NULL);

  /* push speex header0 */
  gst_harness_push (h, create_buffer (speex_hdr0,
          sizeof (speex_hdr0), base_time, 0));

  /* push speex header1 */
  gst_harness_push (h, create_buffer (speex_hdr1,
          sizeof (speex_hdr1), base_time, 0));

  /* push speex data */
  gst_harness_push (h, create_buffer (speex_buf,
          sizeof (speex_buf), base_time, duration));

  /* push speex data 2 */
  gst_harness_push (h, create_buffer (speex_buf,
          sizeof (speex_buf), base_time + duration, duration));

  /* pull out stream-start event */
  gst_event_unref (gst_harness_pull_event (h));

  /* pull out caps event */
  gst_event_unref (gst_harness_pull_event (h));

  /* pull out segment event and verify we are using GST_FORMAT_TIME */
  {
    GstEvent *event = gst_harness_pull_event (h);
    const GstSegment *segment;
    gst_event_parse_segment (event, &segment);
    fail_unless_equals_int (GST_FORMAT_TIME, segment->format);
    gst_event_unref (event);
  }

  /* pull FLV header buffer */
  buf = gst_harness_pull (h);
  gst_buffer_unref (buf);

  /* pull Metadata buffer */
  buf = gst_harness_pull (h);
  gst_buffer_unref (buf);

  /* pull header0 */
  buf = gst_harness_pull (h);
  fail_unless_equals_uint64 (base_time, GST_BUFFER_PTS (buf));
  fail_unless_equals_uint64 (GST_CLOCK_TIME_NONE, GST_BUFFER_DTS (buf));
  fail_unless_equals_uint64 (0, GST_BUFFER_DURATION (buf));
  gst_buffer_map (buf, &map, GST_MAP_READ);
  /* 0x08 means it is audio */
  fail_unless_equals_int (0x08, map.data[0]);
  /* timestamp should be starting from 0 */
  fail_unless_equals_int (0x00, map.data[6]);
  /* 0xb2 means Speex, 16000Hz, Mono */
  fail_unless_equals_int (0xb2, map.data[11]);
  /* verify content is intact */
  fail_unless_equals_int (0, memcmp (&map.data[12], speex_hdr0,
          sizeof (speex_hdr0)));
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  /* pull header1 */
  buf = gst_harness_pull (h);
  fail_unless_equals_uint64 (base_time, GST_BUFFER_PTS (buf));
  fail_unless_equals_uint64 (GST_CLOCK_TIME_NONE, GST_BUFFER_DTS (buf));
  fail_unless_equals_uint64 (0, GST_BUFFER_DURATION (buf));
  gst_buffer_map (buf, &map, GST_MAP_READ);
  /* 0x08 means it is audio */
  fail_unless_equals_int (0x08, map.data[0]);
  /* timestamp should be starting from 0 */
  fail_unless_equals_int (0x00, map.data[6]);
  /* 0xb2 means Speex, 16000Hz, Mono */
  fail_unless_equals_int (0xb2, map.data[11]);
  /* verify content is intact */
  fail_unless_equals_int (0, memcmp (&map.data[12], speex_hdr1,
          sizeof (speex_hdr1)));
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  /* pull data */
  buf = gst_harness_pull (h);
  fail_unless_equals_uint64 (base_time, GST_BUFFER_PTS (buf));
  fail_unless_equals_uint64 (GST_CLOCK_TIME_NONE, GST_BUFFER_DTS (buf));
  fail_unless_equals_uint64 (duration, GST_BUFFER_DURATION (buf));
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_NONE, GST_BUFFER_OFFSET (buf));
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_NONE,
      GST_BUFFER_OFFSET_END (buf));
  gst_buffer_map (buf, &map, GST_MAP_READ);
  /* 0x08 means it is audio */
  fail_unless_equals_int (0x08, map.data[0]);
  /* timestamp should be starting from 0 */
  fail_unless_equals_int (0x00, map.data[6]);
  /* 0xb2 means Speex, 16000Hz, Mono */
  fail_unless_equals_int (0xb2, map.data[11]);
  /* verify content is intact */
  fail_unless_equals_int (0, memcmp (&map.data[12], speex_buf,
          sizeof (speex_buf)));
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  /* pull data */
  buf = gst_harness_pull (h);
  fail_unless_equals_uint64 (base_time + duration, GST_BUFFER_PTS (buf));
  fail_unless_equals_uint64 (GST_CLOCK_TIME_NONE, GST_BUFFER_DTS (buf));
  fail_unless_equals_uint64 (duration, GST_BUFFER_DURATION (buf));
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_NONE, GST_BUFFER_OFFSET (buf));
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_NONE,
      GST_BUFFER_OFFSET_END (buf));
  gst_buffer_map (buf, &map, GST_MAP_READ);
  /* 0x08 means it is audio */
  fail_unless_equals_int (0x08, map.data[0]);
  /* timestamp should reflect the duration_ms */
  fail_unless_equals_int (duration_ms, map.data[6]);
  /* 0xb2 means Speex, 16000Hz, Mono */
  fail_unless_equals_int (0xb2, map.data[11]);
  /* verify content is intact */
  fail_unless_equals_int (0, memcmp (&map.data[12], speex_buf,
          sizeof (speex_buf)));
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  gst_harness_teardown (h);
}

GST_END_TEST;

static void
check_buf_type_timestamp (GstBuffer * buf, gint packet_type, gint timestamp)
{
  GstMapInfo map = GST_MAP_INFO_INIT;
  gst_buffer_map (buf, &map, GST_MAP_READ);
  fail_unless_equals_int (packet_type, map.data[0]);
  fail_unless_equals_int (timestamp, map.data[6]);
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);
}

static const gint AUDIO = 0x08;
static const gint VIDEO = 0x09;

GST_START_TEST (test_increasing_timestamp_when_pts_none)
{
  gint timestamp = 3;
  GstClockTime base_time = 42 * GST_SECOND;
  GstPad *audio_sink, *video_sink, *audio_src, *video_src;
  GstHarness *h, *audio, *video, *audio_q, *video_q;
  GstCaps *audio_caps, *video_caps;
  GstBuffer *buf;

  h = gst_harness_new_with_padnames ("flvmux", NULL, "src");
  audio = gst_harness_new_with_element (h->element, "audio", NULL);
  video = gst_harness_new_with_element (h->element, "video", NULL);
  audio_q = gst_harness_new ("queue");
  video_q = gst_harness_new ("queue");

  audio_sink = GST_PAD_PEER (audio->srcpad);
  video_sink = GST_PAD_PEER (video->srcpad);
  audio_src = GST_PAD_PEER (audio_q->sinkpad);
  video_src = GST_PAD_PEER (video_q->sinkpad);

  gst_pad_unlink (audio->srcpad, audio_sink);
  gst_pad_unlink (video->srcpad, video_sink);
  gst_pad_unlink (audio_src, audio_q->sinkpad);
  gst_pad_unlink (video_src, video_q->sinkpad);
  gst_pad_link (audio_src, audio_sink);
  gst_pad_link (video_src, video_sink);

  audio_caps = gst_caps_new_simple ("audio/x-speex",
      "rate", G_TYPE_INT, 16000, "channels", G_TYPE_INT, 1, NULL);
  gst_harness_set_src_caps (audio_q, audio_caps);
  video_caps = gst_caps_new_simple ("video/x-h264",
      "stream-format", G_TYPE_STRING, "avc", NULL);
  gst_harness_set_src_caps (video_q, video_caps);

  /* Push audio + video + audio with increasing DTS, but PTS for video is
   * GST_CLOCK_TIME_NONE
   */
  buf = gst_buffer_new ();
  GST_BUFFER_DTS (buf) = timestamp * GST_MSECOND + base_time;
  GST_BUFFER_PTS (buf) = timestamp * GST_MSECOND + base_time;
  gst_harness_push (audio_q, buf);

  buf = gst_buffer_new ();
  GST_BUFFER_DTS (buf) = (timestamp + 1) * GST_MSECOND + base_time;
  GST_BUFFER_PTS (buf) = GST_CLOCK_TIME_NONE;
  gst_harness_push (video_q, buf);

  buf = gst_buffer_new ();
  GST_BUFFER_DTS (buf) = (timestamp + 2) * GST_MSECOND + base_time;
  GST_BUFFER_PTS (buf) = (timestamp + 2) * GST_MSECOND + base_time;
  gst_harness_push (audio_q, buf);

  /* Pull two metadata packets out */
  gst_buffer_unref (gst_harness_pull (h));
  gst_buffer_unref (gst_harness_pull (h));

  /* Check that we receive the packets in monotonically increasing order and
   * that their timestamps are correct (should start at 0)
   */
  buf = gst_harness_pull (h);
  check_buf_type_timestamp (buf, AUDIO, 0);
  buf = gst_harness_pull (h);
  check_buf_type_timestamp (buf, VIDEO, 1);

  /* teardown */
  gst_harness_teardown (h);
  gst_harness_teardown (audio);
  gst_harness_teardown (video);
  gst_harness_teardown (audio_q);
  gst_harness_teardown (video_q);
}

GST_END_TEST;

typedef struct
{
  GstHarness *a_sink;
  GstHarness *v_sink;
} DemuxHarnesses;

static void
flvdemux_pad_added (GstElement * flvdemux, GstPad * srcpad, DemuxHarnesses * h)
{
  GstCaps *caps = gst_pad_get_current_caps (srcpad);
  const gchar *name = gst_structure_get_name (gst_caps_get_structure (caps, 0));

  if (h->a_sink && g_ascii_strncasecmp ("audio", name, 5) == 0)
    gst_harness_add_element_src_pad (h->a_sink, srcpad);
  else if (h->v_sink && g_ascii_strncasecmp ("video", name, 5) == 0)
    gst_harness_add_element_src_pad (h->v_sink, srcpad);
  else
    ck_abort_msg ("Unexpected demux pad: %s", GST_STR_NULL (name));

  gst_caps_unref (caps);
}

GST_START_TEST (test_video_caps_late)
{
  GstHarness *mux = gst_harness_new_with_padnames ("flvmux", NULL, "src");
  GstHarness *a_src =
      gst_harness_new_with_element (mux->element, "audio", NULL);
  GstHarness *v_src =
      gst_harness_new_with_element (mux->element, "video", NULL);
  GstHarness *demux = gst_harness_new_with_padnames ("flvdemux", "sink", NULL);
  GstHarness *a_sink =
      gst_harness_new_with_element (demux->element, NULL, NULL);
  GstHarness *v_sink =
      gst_harness_new_with_element (demux->element, NULL, NULL);
  DemuxHarnesses harnesses = { a_sink, v_sink };
  GstTestClock *tclock;

  g_object_set (mux->element, "streamable", TRUE,
      "latency", G_GUINT64_CONSTANT (1), NULL);
  gst_harness_use_testclock (mux);

  g_signal_connect (demux->element, "pad-added",
      G_CALLBACK (flvdemux_pad_added), &harnesses);
  gst_harness_add_sink_harness (mux, demux);

  gst_harness_set_src_caps_str (a_src,
      "audio/x-speex, rate=(int)16000, channels=(int)1");

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (a_src,
          gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
              speex_hdr0, sizeof (speex_hdr0), 0, sizeof (speex_hdr0), NULL,
              NULL)));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (a_src,
          gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
              speex_hdr1, sizeof (speex_hdr1), 0, sizeof (speex_hdr1), NULL,
              NULL)));

  /* Wait a little and make sure no clock was scheduled as this shouldn't happen
   * before the caps are set */
  g_usleep (40 * 1000);
  tclock = gst_harness_get_testclock (mux);
  fail_unless (gst_test_clock_get_next_entry_time (tclock) ==
      GST_CLOCK_TIME_NONE);

  gst_harness_set_src_caps_str (v_src,
      "video/x-h264, stream-format=(string)avc, alignment=(string)au, "
      "codec_data=(buffer)0142c00cffe1000b6742c00c95a7201e1108d401000468ce3c80");

  gst_harness_crank_single_clock_wait (mux);

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (a_src,
          gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
              speex_buf, sizeof (speex_buf), 0, sizeof (speex_buf), NULL,
              NULL)));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (v_src,
          gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
              h264_buf, sizeof (h264_buf), 0, sizeof (h264_buf), NULL, NULL)));

  gst_harness_crank_single_clock_wait (mux);
  gst_harness_crank_single_clock_wait (mux);
  gst_harness_crank_single_clock_wait (mux);


  /* push from flvmux to demux */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_sink_push_many (mux, 6));

  /* verify we got 2x audio and 1x video buffers out of flvdemux */
  gst_buffer_unref (gst_harness_pull (a_sink));
  gst_buffer_unref (gst_harness_pull (a_sink));
  gst_buffer_unref (gst_harness_pull (v_sink));

  fail_unless (gst_test_clock_get_next_entry_time (tclock) ==
      GST_CLOCK_TIME_NONE);

  g_clear_object (&tclock);
  gst_harness_teardown (a_src);
  gst_harness_teardown (v_src);
  gst_harness_teardown (mux);
  gst_harness_teardown (a_sink);
  gst_harness_teardown (v_sink);
}

GST_END_TEST;

guint8 raw_frame_short[] = {
  0x27, 0x00, 0x03, 0x20, 0x64, 0x1c
};

GST_START_TEST (test_video_caps_change_streamable)
{
  GstEvent *event;
  GstCaps *a_caps1, *v_caps1, *v_caps2, *ret_caps;
  GstHarness *mux = gst_harness_new_with_padnames ("flvmux", NULL, "src");
  GstHarness *a_src =
      gst_harness_new_with_element (mux->element, "audio", NULL);
  GstHarness *v_src =
      gst_harness_new_with_element (mux->element, "video", NULL);
  GstHarness *demux = gst_harness_new_with_padnames ("flvdemux", "sink", NULL);
  GstHarness *a_sink =
      gst_harness_new_with_element (demux->element, NULL, NULL);
  GstHarness *v_sink =
      gst_harness_new_with_element (demux->element, NULL, NULL);
  DemuxHarnesses harnesses = { a_sink, v_sink };

  const GstClockTime base_time = 123456789;
  const GstClockTime duration_ms = 20;
  const GstClockTime duration = duration_ms * GST_MSECOND;

  g_object_set (mux->element, "streamable", TRUE, NULL);

  g_signal_connect (demux->element, "pad-added",
      G_CALLBACK (flvdemux_pad_added), &harnesses);
  gst_harness_add_sink_harness (mux, demux);

  a_caps1 =
      gst_caps_from_string
      ("audio/mpeg, mpegversion=(int)4, framed=(boolean)true, stream-format=(string)raw, "
      "rate=(int)44100, channels=(int)1, codec_data=(buffer)1208");

  v_caps1 = gst_caps_from_string ("video/x-h264, stream-format=(string)avc, "
      "codec_data=(buffer)0142c00cffe1000b6742c00c95a7201e1108d401000468ce3c80");

  gst_harness_set_src_caps (a_src, gst_caps_ref (a_caps1));
  gst_harness_set_src_caps (v_src, gst_caps_ref (v_caps1));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (a_src,
          create_buffer (raw_frame_short, sizeof (raw_frame_short), base_time,
              duration)));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (v_src,
          create_buffer (h264_buf, sizeof (h264_buf), base_time, duration)));

  gst_harness_crank_single_clock_wait (mux);

  /* push from flvmux to demux */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_sink_push_many (mux, 6));

  /* should accept without the constraint */
  while ((event = gst_harness_try_pull_event (v_sink))) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
      gst_event_parse_caps (event, &ret_caps);
      GST_LOG ("v_caps1 %" GST_PTR_FORMAT ", ret caps %" GST_PTR_FORMAT,
          v_caps1, ret_caps);
      fail_unless (gst_caps_is_equal (v_caps1, ret_caps));
    }
    gst_event_unref (event);
  }

  /* caps change */
  v_caps2 = gst_caps_from_string ("video/x-h264, stream-format=(string)avc, "
      "codec_data=(buffer)0164001fffe1001c6764001facd9405005bb016a02020280000003008000001e478c18cb01000568ebecb22c");

  gst_harness_set_src_caps (v_src, gst_caps_ref (v_caps2));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (v_src,
          create_buffer (h264_buf, sizeof (h264_buf), base_time + duration,
              duration)));

  gst_harness_crank_single_clock_wait (mux);

  /* push from flvmux to demux */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_sink_push_many (mux, 2));

  /* should accept without the constraint */
  while ((event = gst_harness_try_pull_event (v_sink))) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
      gst_event_parse_caps (event, &ret_caps);
      GST_LOG ("v_caps2 %" GST_PTR_FORMAT ", ret caps %" GST_PTR_FORMAT,
          v_caps2, ret_caps);
      fail_unless (gst_caps_is_equal (v_caps2, ret_caps));
    }
    gst_event_unref (event);
  }

  /* verify we got 1x audio and 2x video buffers out of flvdemux */
  gst_buffer_unref (gst_harness_pull (a_sink));
  gst_buffer_unref (gst_harness_pull (v_sink));
  gst_buffer_unref (gst_harness_pull (v_sink));
  gst_caps_unref (a_caps1);
  gst_caps_unref (v_caps1);
  gst_caps_unref (v_caps2);

  gst_harness_teardown (a_src);
  gst_harness_teardown (v_src);
  gst_harness_teardown (mux);
  gst_harness_teardown (a_sink);
  gst_harness_teardown (v_sink);
}

GST_END_TEST;

GST_START_TEST (test_audio_caps_change_streamable)
{
  GstEvent *event;
  GstCaps *a_caps1, *a_caps2, *v_caps1, *ret_caps;
  GstHarness *mux = gst_harness_new_with_padnames ("flvmux", NULL, "src");
  GstHarness *a_src =
      gst_harness_new_with_element (mux->element, "audio", NULL);
  GstHarness *v_src =
      gst_harness_new_with_element (mux->element, "video", NULL);
  GstHarness *demux = gst_harness_new_with_padnames ("flvdemux", "sink", NULL);
  GstHarness *a_sink =
      gst_harness_new_with_element (demux->element, NULL, NULL);
  GstHarness *v_sink =
      gst_harness_new_with_element (demux->element, NULL, NULL);
  DemuxHarnesses harnesses = { a_sink, v_sink };

  const GstClockTime base_time = 123456789;
  const GstClockTime duration_ms = 20;
  const GstClockTime duration = duration_ms * GST_MSECOND;

  g_object_set (mux->element, "streamable", TRUE, NULL);

  g_signal_connect (demux->element, "pad-added",
      G_CALLBACK (flvdemux_pad_added), &harnesses);
  gst_harness_add_sink_harness (mux, demux);

  a_caps1 =
      gst_caps_from_string
      ("audio/mpeg, mpegversion=(int)4, framed=(boolean)true, stream-format=(string)raw, "
      "rate=(int)44100, channels=(int)1, codec_data=(buffer)1208");

  v_caps1 = gst_caps_from_string ("video/x-h264, stream-format=(string)avc, "
      "codec_data=(buffer)0142c00cffe1000b6742c00c95a7201e1108d401000468ce3c80");

  gst_harness_set_src_caps (a_src, gst_caps_ref (a_caps1));
  gst_harness_set_src_caps (v_src, gst_caps_ref (v_caps1));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (a_src,
          create_buffer (raw_frame_short, sizeof (raw_frame_short), base_time,
              duration)));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (v_src,
          create_buffer (h264_buf, sizeof (h264_buf), base_time, duration)));

  gst_harness_crank_single_clock_wait (mux);

  /* push from flvmux to demux */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_sink_push_many (mux, 6));

  /* should accept without the constraint */
  while ((event = gst_harness_try_pull_event (a_sink))) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
      gst_event_parse_caps (event, &ret_caps);
      GST_LOG ("a_caps1 %" GST_PTR_FORMAT ", ret caps %" GST_PTR_FORMAT,
          a_caps1, ret_caps);
      fail_unless (gst_caps_is_equal (a_caps1, ret_caps));
    }
    gst_event_unref (event);
  }

  /* caps change */
  a_caps2 =
      gst_caps_from_string
      ("audio/mpeg, mpegversion=(int)4, framed=(boolean)true, stream-format=(string)raw, "
      "rate=(int)22050, channels=(int)1, codec_data=(buffer)1388");

  gst_harness_set_src_caps (a_src, gst_caps_ref (a_caps2));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (a_src,
          create_buffer (raw_frame_short, sizeof (raw_frame_short),
              base_time + duration, duration)));

  gst_harness_crank_single_clock_wait (mux);

  /* push from flvmux to demux */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_sink_push_many (mux, 2));

  /* should accept without the constraint */
  while ((event = gst_harness_try_pull_event (a_sink))) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
      gst_event_parse_caps (event, &ret_caps);
      GST_LOG ("a_caps2 %" GST_PTR_FORMAT ", ret caps %" GST_PTR_FORMAT,
          a_caps2, ret_caps);
      fail_unless (gst_caps_is_equal (a_caps2, ret_caps));
    }
    gst_event_unref (event);
  }

  /* verify we got 2x audio and 1x video buffers out of flvdemux */
  gst_buffer_unref (gst_harness_pull (a_sink));
  gst_buffer_unref (gst_harness_pull (a_sink));
  gst_buffer_unref (gst_harness_pull (v_sink));
  gst_caps_unref (a_caps1);
  gst_caps_unref (a_caps2);
  gst_caps_unref (v_caps1);

  gst_harness_teardown (a_src);
  gst_harness_teardown (v_src);
  gst_harness_teardown (mux);
  gst_harness_teardown (a_sink);
  gst_harness_teardown (v_sink);
}

GST_END_TEST;

GST_START_TEST (test_video_caps_change_streamable_single)
{
  GstEvent *event;
  GstCaps *v_caps1, *v_caps2, *ret_caps;
  GstHarness *mux = gst_harness_new_with_padnames ("flvmux", NULL, "src");
  GstHarness *v_src =
      gst_harness_new_with_element (mux->element, "video", NULL);
  GstHarness *demux = gst_harness_new_with_padnames ("flvdemux", "sink", NULL);
  GstHarness *v_sink =
      gst_harness_new_with_element (demux->element, NULL, NULL);
  DemuxHarnesses harnesses = { NULL, v_sink };

  const GstClockTime base_time = 123456789;
  const GstClockTime duration_ms = 20;
  const GstClockTime duration = duration_ms * GST_MSECOND;

  g_object_set (mux->element, "streamable", TRUE, NULL);

  g_signal_connect (demux->element, "pad-added",
      G_CALLBACK (flvdemux_pad_added), &harnesses);
  gst_harness_add_sink_harness (mux, demux);

  v_caps1 = gst_caps_from_string ("video/x-h264, stream-format=(string)avc, "
      "codec_data=(buffer)0142c00cffe1000b6742c00c95a7201e1108d401000468ce3c80");

  gst_harness_set_src_caps (v_src, gst_caps_ref (v_caps1));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (v_src,
          create_buffer (h264_buf, sizeof (h264_buf), base_time, duration)));

  /* push from flvmux to demux */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_sink_push_many (mux, 4));

  /* should accept without the constraint */
  while ((event = gst_harness_try_pull_event (v_sink))) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
      gst_event_parse_caps (event, &ret_caps);
      GST_LOG ("v_caps1 %" GST_PTR_FORMAT ", ret caps %" GST_PTR_FORMAT,
          v_caps1, ret_caps);
      fail_unless (gst_caps_is_equal (v_caps1, ret_caps));
    }
    gst_event_unref (event);
  }

  /* caps change */
  v_caps2 = gst_caps_from_string ("video/x-h264, stream-format=(string)avc, "
      "codec_data=(buffer)0164001fffe1001c6764001facd9405005bb016a02020280000003008000001e478c18cb01000568ebecb22c");

  gst_harness_set_src_caps (v_src, gst_caps_ref (v_caps2));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (v_src,
          create_buffer (h264_buf, sizeof (h264_buf), base_time + duration,
              duration)));

  /* push from flvmux to demux */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_sink_push_many (mux, 2));

  /* should accept without the constraint */
  while ((event = gst_harness_try_pull_event (v_sink))) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
      gst_event_parse_caps (event, &ret_caps);
      GST_LOG ("v_caps2 %" GST_PTR_FORMAT ", ret caps %" GST_PTR_FORMAT,
          v_caps2, ret_caps);
      fail_unless (gst_caps_is_equal (v_caps2, ret_caps));
    }
    gst_event_unref (event);
  }

  /* verify we got 2x video buffers out of flvdemux */
  gst_buffer_unref (gst_harness_pull (v_sink));
  gst_buffer_unref (gst_harness_pull (v_sink));
  gst_caps_unref (v_caps1);
  gst_caps_unref (v_caps2);

  gst_harness_teardown (v_src);
  gst_harness_teardown (mux);
  gst_harness_teardown (v_sink);
}

GST_END_TEST;

GST_START_TEST (test_audio_caps_change_streamable_single)
{
  GstEvent *event;
  GstCaps *a_caps1, *a_caps2, *ret_caps;
  GstHarness *mux = gst_harness_new_with_padnames ("flvmux", NULL, "src");
  GstHarness *a_src =
      gst_harness_new_with_element (mux->element, "audio", NULL);
  GstHarness *demux = gst_harness_new_with_padnames ("flvdemux", "sink", NULL);
  GstHarness *a_sink =
      gst_harness_new_with_element (demux->element, NULL, NULL);
  DemuxHarnesses harnesses = { a_sink, NULL };

  const GstClockTime base_time = 123456789;
  const GstClockTime duration_ms = 20;
  const GstClockTime duration = duration_ms * GST_MSECOND;

  g_object_set (mux->element, "streamable", TRUE, NULL);

  g_signal_connect (demux->element, "pad-added",
      G_CALLBACK (flvdemux_pad_added), &harnesses);
  gst_harness_add_sink_harness (mux, demux);

  a_caps1 =
      gst_caps_from_string
      ("audio/mpeg, mpegversion=(int)4, framed=(boolean)true, stream-format=(string)raw, "
      "rate=(int)44100, channels=(int)1, codec_data=(buffer)1208");

  gst_harness_set_src_caps (a_src, gst_caps_ref (a_caps1));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (a_src,
          create_buffer (raw_frame_short, sizeof (raw_frame_short), base_time,
              duration)));

  /* push from flvmux to demux */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_sink_push_many (mux, 4));

  /* should accept without the constraint */
  while ((event = gst_harness_try_pull_event (a_sink))) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
      gst_event_parse_caps (event, &ret_caps);
      GST_LOG ("a_caps1 %" GST_PTR_FORMAT ", ret caps %" GST_PTR_FORMAT,
          a_caps1, ret_caps);
      fail_unless (gst_caps_is_equal (a_caps1, ret_caps));
    }
    gst_event_unref (event);
  }

  /* caps change */
  a_caps2 =
      gst_caps_from_string
      ("audio/mpeg, mpegversion=(int)4, framed=(boolean)true, stream-format=(string)raw, "
      "rate=(int)22050, channels=(int)1, codec_data=(buffer)1388");

  gst_harness_set_src_caps (a_src, gst_caps_ref (a_caps2));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (a_src,
          create_buffer (raw_frame_short, sizeof (raw_frame_short),
              base_time + duration, duration)));

  /* push from flvmux to demux */
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_sink_push_many (mux, 2));

  /* should accept without the constraint */
  while ((event = gst_harness_try_pull_event (a_sink))) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
      gst_event_parse_caps (event, &ret_caps);
      GST_LOG ("a_caps2 %" GST_PTR_FORMAT ", ret caps %" GST_PTR_FORMAT,
          a_caps2, ret_caps);
      fail_unless (gst_caps_is_equal (a_caps2, ret_caps));
    }
    gst_event_unref (event);
  }

  /* verify we got 2x audio out of flvdemux */
  gst_buffer_unref (gst_harness_pull (a_sink));
  gst_buffer_unref (gst_harness_pull (a_sink));
  gst_caps_unref (a_caps1);
  gst_caps_unref (a_caps2);

  gst_harness_teardown (a_src);
  gst_harness_teardown (mux);
  gst_harness_teardown (a_sink);
}

GST_END_TEST;

typedef struct
{
  guint media_type;
  gint ts;                      /* timestamp in ms */
  gint rt;                      /* running_time in ms */
} InputData;

GST_START_TEST (test_incrementing_timestamps)
{
  GstPad *audio_sink, *video_sink, *audio_src, *video_src;
  GstHarness *h, *audio, *video, *audio_q, *video_q;
  GstTestClock *tclock;
  guint i;
  guint32 prev_pts;
  InputData input[] = {
    {AUDIO, 155, 175},
    {VIDEO, 156, 191},
    {VIDEO, 190, 191},
    {AUDIO, 176, 195},
    {AUDIO, 197, 215},
  };

  /* setup flvmuxer with queues in front */
  h = gst_harness_new_with_padnames ("flvmux", NULL, "src");
  audio = gst_harness_new_with_element (h->element, "audio", NULL);
  video = gst_harness_new_with_element (h->element, "video", NULL);
  audio_q = gst_harness_new ("queue");
  video_q = gst_harness_new ("queue");
  audio_sink = GST_PAD_PEER (audio->srcpad);
  video_sink = GST_PAD_PEER (video->srcpad);
  audio_src = GST_PAD_PEER (audio_q->sinkpad);
  video_src = GST_PAD_PEER (video_q->sinkpad);
  gst_pad_unlink (audio->srcpad, audio_sink);
  gst_pad_unlink (video->srcpad, video_sink);
  gst_pad_unlink (audio_src, audio_q->sinkpad);
  gst_pad_unlink (video_src, video_q->sinkpad);
  gst_pad_link (audio_src, audio_sink);
  gst_pad_link (video_src, video_sink);
  g_object_set (h->element, "streamable", TRUE, NULL);

  gst_harness_set_src_caps_str (audio_q,
      "audio/mpeg, mpegversion=(int)4, "
      "rate=(int)44100, channels=(int)1, "
      "stream-format=(string)raw, codec_data=(buffer)1208");

  gst_harness_set_src_caps_str (video_q,
      "video/x-h264, stream-format=(string)avc, alignment=(string)au, "
      "codec_data=(buffer)0142c00dffe1000d6742c00d95a0507c807844235001000468ce3c80");

  tclock = gst_harness_get_testclock (h);

  for (i = 0; i < G_N_ELEMENTS (input); i++) {
    InputData *d = &input[i];
    GstBuffer *buf = gst_buffer_new ();
    GstClockTime now = d->rt * GST_MSECOND;
    GstClockID pending, res;

    GST_BUFFER_DTS (buf) = GST_BUFFER_PTS (buf) = d->ts * GST_MSECOND;
    gst_test_clock_set_time (tclock, now);

    if (d->media_type == AUDIO)
      gst_harness_push (audio_q, buf);
    else
      gst_harness_push (video_q, buf);

    gst_test_clock_wait_for_next_pending_id (tclock, &pending);
    res = gst_test_clock_process_next_clock_id (tclock);
    gst_clock_id_unref (pending);
    gst_clock_id_unref (res);
  }

  /* pull the flv metadata */
  gst_buffer_unref (gst_harness_pull (h));
  gst_buffer_unref (gst_harness_pull (h));
  gst_buffer_unref (gst_harness_pull (h));

  /* verify pts in the flvheader is increasing */
  prev_pts = 0;
  for (i = 0; i < G_N_ELEMENTS (input); i++) {
    GstBuffer *buf = gst_harness_pull (h);
    GstMapInfo map;
    guint32 pts;
    gst_buffer_map (buf, &map, GST_MAP_READ);
    pts = GST_READ_UINT24_BE (map.data + 4);
    GST_DEBUG ("media=%u, pts = %u\n", map.data[0], pts);
    fail_unless (pts >= prev_pts);
    prev_pts = pts;
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
  }

  /* teardown */
  gst_object_unref (tclock);
  gst_harness_teardown (h);
  gst_harness_teardown (audio);
  gst_harness_teardown (video);
  gst_harness_teardown (audio_q);
  gst_harness_teardown (video_q);
}

GST_END_TEST;

static Suite *
flvmux_suite (void)
{
  Suite *s = suite_create ("flvmux");
  TCase *tc_chain = tcase_create ("general");
  gint loop = 16;

  suite_add_tcase (s, tc_chain);

#ifdef HAVE_VALGRIND
  if (RUNNING_ON_VALGRIND) {
    loop = 1;
  }
#endif

  tcase_add_loop_test (tc_chain, test_index_writing, 0, loop);

  tcase_add_test (tc_chain, test_speex_streamable);
  tcase_add_test (tc_chain, test_increasing_timestamp_when_pts_none);
  tcase_add_test (tc_chain, test_video_caps_late);
  tcase_add_test (tc_chain, test_audio_caps_change_streamable);
  tcase_add_test (tc_chain, test_video_caps_change_streamable);
  tcase_add_test (tc_chain, test_audio_caps_change_streamable_single);
  tcase_add_test (tc_chain, test_video_caps_change_streamable_single);
  tcase_add_test (tc_chain, test_incrementing_timestamps);

  return s;
}

GST_CHECK_MAIN (flvmux)
