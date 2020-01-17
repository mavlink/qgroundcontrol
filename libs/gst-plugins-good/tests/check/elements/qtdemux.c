/* GStreamer
 *
 * unit test for qtdemux
 *
 * Copyright (C) <2016> Edward Hervey <edward@centricular.com>
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

#include "qtdemux.h"
#include <glib/gprintf.h>

typedef struct
{
  GstPad *srcpad;
  guint expected_size;
  GstClockTime expected_time;
} CommonTestData;

static GstPadProbeReturn
qtdemux_probe (GstPad * pad, GstPadProbeInfo * info, CommonTestData * data)
{
  if (GST_IS_EVENT (GST_PAD_PROBE_INFO_DATA (info))) {
    GstEvent *ev = GST_PAD_PROBE_INFO_EVENT (info);

    switch (GST_EVENT_TYPE (ev)) {
      case GST_EVENT_SEGMENT:
      {
        const GstSegment *segment;
        gst_event_parse_segment (ev, &segment);
        fail_unless (GST_CLOCK_TIME_IS_VALID (segment->format));
        fail_unless (GST_CLOCK_TIME_IS_VALID (segment->start));
        fail_unless (GST_CLOCK_TIME_IS_VALID (segment->base));
        fail_unless (GST_CLOCK_TIME_IS_VALID (segment->time));
        fail_unless (GST_CLOCK_TIME_IS_VALID (segment->position));
        break;
      }
        break;
      default:
        break;
    }

    return GST_PAD_PROBE_OK;
  } else if (GST_IS_BUFFER (GST_PAD_PROBE_INFO_DATA (info))) {
    GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER (info);

    fail_unless_equals_int (gst_buffer_get_size (buf), data->expected_size);
    fail_unless_equals_uint64 (GST_BUFFER_PTS (buf), data->expected_time);
  }

  return GST_PAD_PROBE_DROP;
}

static void
qtdemux_pad_added_cb (GstElement * element, GstPad * pad, CommonTestData * data)
{
  data->srcpad = pad;
  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM,
      (GstPadProbeCallback) qtdemux_probe, data, NULL);
}

GST_START_TEST (test_qtdemux_input_gap)
{
  GstElement *qtdemux;
  GstPad *sinkpad;
  CommonTestData data = { 0, };
  GstBuffer *inbuf;
  GstSegment segment;
  GstEvent *event;
  guint i, offset;
  GstClockTime pts;

  /* The goal of this test is to check that qtdemux can properly handle
   * fragmented input from dashdemux, with gaps in it.
   *
   * Input segment :
   *   - TIME
   * Input buffers :
   *   - The offset is set on buffers, it corresponds to the offset
   *     within the current fragment.
   *   - Buffer of the beginning of a fragment has the PTS set, others
   *     don't.
   *   - By extension, the beginning of a fragment also has an offset
   *     of 0.
   */

  qtdemux = gst_element_factory_make ("qtdemux", NULL);
  gst_element_set_state (qtdemux, GST_STATE_PLAYING);
  sinkpad = gst_element_get_static_pad (qtdemux, "sink");

  /* We'll want to know when the source pad is added */
  g_signal_connect (qtdemux, "pad-added", (GCallback) qtdemux_pad_added_cb,
      &data);

  /* Send the initial STREAM_START and segment (TIME) event */
  event = gst_event_new_stream_start ("TEST");
  GST_DEBUG ("Pushing stream-start event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);
  gst_segment_init (&segment, GST_FORMAT_TIME);
  event = gst_event_new_segment (&segment);
  GST_DEBUG ("Pushing segment event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);

  /* Feed the init buffer, should create the source pad */
  inbuf = gst_buffer_new_and_alloc (init_mp4_len);
  gst_buffer_fill (inbuf, 0, init_mp4, init_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  GST_DEBUG ("Pushing header buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);

  /* Now send the trun of the first fragment */
  inbuf = gst_buffer_new_and_alloc (seg_1_moof_size);
  gst_buffer_fill (inbuf, 0, seg_1_m4f, seg_1_moof_size);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  /* We are simulating that this fragment can happen at any point */
  GST_BUFFER_FLAG_SET (inbuf, GST_BUFFER_FLAG_DISCONT);
  GST_DEBUG ("Pushing trun buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_if (data.srcpad == NULL);

  /* We are now ready to send some buffers with gaps */
  offset = seg_1_sample_0_offset;
  pts = 0;

  GST_DEBUG ("Pushing gap'ed buffers");
  for (i = 0; i < 129; i++) {
    /* Let's send one every 3 */
    if ((i % 3) == 0) {
      GST_DEBUG ("Pushing buffer #%d offset:%" G_GUINT32_FORMAT, i, offset);
      inbuf = gst_buffer_new_and_alloc (seg_1_sample_sizes[i]);
      gst_buffer_fill (inbuf, 0, seg_1_m4f + offset, seg_1_sample_sizes[i]);
      GST_BUFFER_OFFSET (inbuf) = offset;
      GST_BUFFER_FLAG_SET (inbuf, GST_BUFFER_FLAG_DISCONT);
      data.expected_time =
          gst_util_uint64_scale (pts, GST_SECOND, seg_1_timescale);
      data.expected_size = seg_1_sample_sizes[i];
      fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
    }
    /* Finally move offset forward */
    offset += seg_1_sample_sizes[i];
    pts += seg_1_sample_duration;
  }

  gst_object_unref (sinkpad);
  gst_element_set_state (qtdemux, GST_STATE_NULL);
  gst_object_unref (qtdemux);
}

GST_END_TEST;

typedef struct
{
  GstPad *sinkpad;
  GstPad *pending_pad;
  GstEventType *expected_events;
  guint step;
  guint total_step;
  guint expected_num_srcpad;
  guint num_srcpad;
} ReconfigTestData;

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static gboolean
_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gst_event_unref (event);
  return TRUE;
}

static GstFlowReturn
_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  gst_buffer_unref (buffer);
  return GST_FLOW_OK;
}

static GstPadProbeReturn
qtdemux_block_for_reconfig (GstPad * pad, GstPadProbeInfo * info,
    ReconfigTestData * data)
{
  fail_unless (data->pending_pad);
  fail_unless (data->pending_pad == pad);

  GST_DEBUG_OBJECT (pad, "Unblock pad");

  if (gst_pad_is_linked (data->sinkpad)) {
    GstPad *peer = gst_pad_get_peer (data->sinkpad);
    fail_unless (peer);
    gst_pad_unlink (peer, data->sinkpad);
  }

  fail_unless (gst_pad_link (data->pending_pad, data->sinkpad) ==
      GST_PAD_LINK_OK);
  data->pending_pad = NULL;

  return GST_PAD_PROBE_REMOVE;
}

static GstPadProbeReturn
qtdemux_probe_for_reconfig (GstPad * pad, GstPadProbeInfo * info,
    ReconfigTestData * data)
{
  GstEvent *event = GST_PAD_PROBE_INFO_EVENT (info);
  GstEventType expected;

  if (data->step < data->total_step) {
    expected = data->expected_events[data->step];
  } else {
    expected = GST_EVENT_UNKNOWN;
  }

  GST_DEBUG ("Got event %p %s", event, GST_EVENT_TYPE_NAME (event));

  fail_unless (GST_EVENT_TYPE (event) == expected,
      "Received unexpected event: %s (expected: %s)",
      GST_EVENT_TYPE_NAME (event), gst_event_type_get_name (expected));
  data->step++;

  if (GST_EVENT_TYPE (event) == GST_EVENT_EOS && data->step < data->total_step) {
    /* If current EOS is for draining, there should be pending srcpad */
    fail_unless (data->pending_pad != NULL);
  }

  return GST_PAD_PROBE_OK;
}

static void
qtdemux_pad_added_cb_for_reconfig (GstElement * element, GstPad * pad,
    ReconfigTestData * data)
{
  data->num_srcpad++;

  fail_unless (data->num_srcpad <= data->expected_num_srcpad);
  fail_unless (data->pending_pad == NULL);

  GST_DEBUG_OBJECT (pad, "New pad added");

  data->pending_pad = pad;
  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
      (GstPadProbeCallback) qtdemux_block_for_reconfig, data, NULL);

  if (!data->sinkpad) {
    GstPad *sinkpad = gst_pad_new_from_static_template (&sinktemplate, "sink");

    gst_pad_set_event_function (sinkpad, _sink_event);
    gst_pad_set_chain_function (sinkpad, _sink_chain);

    gst_pad_add_probe (sinkpad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        (GstPadProbeCallback) qtdemux_probe_for_reconfig, data, NULL);
    gst_pad_set_active (sinkpad, TRUE);
    data->sinkpad = sinkpad;
  }
}

GST_START_TEST (test_qtdemux_duplicated_moov)
{
  GstElement *qtdemux;
  GstPad *sinkpad;
  ReconfigTestData data = { 0, };
  GstBuffer *inbuf;
  GstSegment segment;
  GstEvent *event;
  GstEventType expected[] = {
    GST_EVENT_STREAM_START,
    GST_EVENT_CAPS,
    GST_EVENT_SEGMENT,
    GST_EVENT_TAG,
    GST_EVENT_TAG,
    GST_EVENT_EOS
  };

  data.expected_events = expected;
  data.expected_num_srcpad = 1;
  data.total_step = G_N_ELEMENTS (expected);

  /* The goal of this test is to check that qtdemux can properly handle
   * duplicated moov without redundant events and pad exposing
   *
   * Testing step
   *  - Push events stream-start and segment to qtdemux
   *  - Push init and media data
   *  - Push the same init and media data again
   *
   * Expected behaviour
   *  - Expose srcpad only once
   *  - No additional downstream events when the second init and media data is
   *    pushed to qtdemux
   */

  qtdemux = gst_element_factory_make ("qtdemux", NULL);
  gst_element_set_state (qtdemux, GST_STATE_PLAYING);
  sinkpad = gst_element_get_static_pad (qtdemux, "sink");

  /* We'll want to know when the source pad is added */
  g_signal_connect (qtdemux, "pad-added", (GCallback)
      qtdemux_pad_added_cb_for_reconfig, &data);

  /* Send the initial STREAM_START and segment (TIME) event */
  event = gst_event_new_stream_start ("TEST");
  GST_DEBUG ("Pushing stream-start event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);
  gst_segment_init (&segment, GST_FORMAT_TIME);
  event = gst_event_new_segment (&segment);
  GST_DEBUG ("Pushing segment event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);

  /* Feed the init buffer, should create the source pad */
  inbuf = gst_buffer_new_and_alloc (init_mp4_len);
  gst_buffer_fill (inbuf, 0, init_mp4, init_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  GST_DEBUG ("Pushing moov buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_if (data.sinkpad == NULL);
  fail_unless_equals_int (data.num_srcpad, 1);

  /* Now send the moof and mdat of the first fragment */
  inbuf = gst_buffer_new_and_alloc (seg_1_m4f_len);
  gst_buffer_fill (inbuf, 0, seg_1_m4f, seg_1_m4f_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  GST_DEBUG ("Pushing moof and mdat buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);

  /* Resend the init, moof and mdat, no additional event and pad are expected */
  inbuf = gst_buffer_new_and_alloc (init_mp4_len);
  gst_buffer_fill (inbuf, 0, init_mp4, init_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  GST_BUFFER_FLAG_SET (inbuf, GST_BUFFER_FLAG_DISCONT);
  GST_DEBUG ("Pushing moov buffer again");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_if (data.sinkpad == NULL);
  fail_unless_equals_int (data.num_srcpad, 1);

  inbuf = gst_buffer_new_and_alloc (seg_1_m4f_len);
  gst_buffer_fill (inbuf, 0, seg_1_m4f, seg_1_m4f_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = init_mp4_len;
  GST_DEBUG ("Pushing moof and mdat buffer again");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_unless (gst_pad_send_event (sinkpad, gst_event_new_eos ()) == TRUE);
  fail_unless_equals_int (data.step, data.total_step);
  fail_unless (data.pending_pad == NULL);

  gst_object_unref (sinkpad);
  gst_pad_set_active (data.sinkpad, FALSE);
  gst_object_unref (data.sinkpad);
  gst_element_set_state (qtdemux, GST_STATE_NULL);
  gst_object_unref (qtdemux);
}

GST_END_TEST;

GST_START_TEST (test_qtdemux_stream_change)
{
  GstElement *qtdemux;
  GstPad *sinkpad;
  ReconfigTestData data = { 0, };
  GstBuffer *inbuf;
  GstSegment segment;
  GstEvent *event;
  const gchar *upstream_id;
  const gchar *stream_id = NULL;
  gchar *expected_stream_id = NULL;
  guint track_id;
  GstEventType expected[] = {
    /* 1st group */
    GST_EVENT_STREAM_START,
    GST_EVENT_CAPS,
    GST_EVENT_SEGMENT,
    GST_EVENT_TAG,
    GST_EVENT_TAG,
    /* 2nd group (track-id change without upstream stream-start) */
    GST_EVENT_EOS,
    GST_EVENT_STREAM_START,
    GST_EVENT_CAPS,
    GST_EVENT_SEGMENT,
    GST_EVENT_TAG,
    GST_EVENT_TAG,
    /* 3rd group (no track-id change with upstream stream-start) */
    GST_EVENT_EOS,
    GST_EVENT_STREAM_START,
    GST_EVENT_CAPS,
    GST_EVENT_SEGMENT,
    GST_EVENT_TAG,
    GST_EVENT_TAG,
    /* last group (track-id change with upstream stream-start) */
    GST_EVENT_EOS,
    GST_EVENT_STREAM_START,
    GST_EVENT_CAPS,
    GST_EVENT_SEGMENT,
    GST_EVENT_TAG,
    GST_EVENT_TAG,
    GST_EVENT_EOS
  };

  data.expected_events = expected;
  data.expected_num_srcpad = 4;
  data.total_step = G_N_ELEMENTS (expected);

  /* The goal of this test is to check that qtdemux can properly handle
   * stream change regardless of track-id change.
   * This test is simulating DASH bitrate switching (for both playbin and plabyin3)
   * and period-change for playbin3
   *
   * NOTE: During bitrate switching in DASH, track-id might be changed
   * NOTE: stream change with new stream-start to qtdemux is playbin3 specific behaviour,
   * because playbin configures new demux per period and existing demux never ever get
   * new stream-start again.
   *
   * Testing step
   *  [GROUP 1]
   *  - Push events stream-start and segment to qtdemux
   *  - Push init and media data to qtdemux
   *  [GROUP 2]
   *  - Push different (track-id change) init and media data to qtdemux
   *    without new downstream sticky events to qtdemux
   *  [GROUP 3]
   *  - Push events stream-start and segment to qtdemux again
   *  - Push the init and media data which are the same as GROUP 2
   *  [GROUP 4]
   *  - Push events stream-start and segment to qtdemux again
   *  - Push different (track-id change) init and media data to qtdemux
   *
   * Expected behaviour
   *  - Demux exposes srcpad four times, per test GROUP, regardless of track-id change
   *  - Whenever exposing new pads, downstream sticky events should be detected
   *    at demux srcpad
   */

  qtdemux = gst_element_factory_make ("qtdemux", NULL);
  gst_element_set_state (qtdemux, GST_STATE_PLAYING);
  sinkpad = gst_element_get_static_pad (qtdemux, "sink");

  /* We'll want to know when the source pad is added */
  g_signal_connect (qtdemux, "pad-added", (GCallback)
      qtdemux_pad_added_cb_for_reconfig, &data);

  /***************
   * TEST GROUP 1
   * (track-id: 2)
   **************/
  /* Send the initial STREAM_START and segment (TIME) event */
  upstream_id = "TEST-GROUP-1";
  track_id = 2;
  expected_stream_id = g_strdup_printf ("%s/%03u", upstream_id, track_id);
  event = gst_event_new_stream_start (upstream_id);
  GST_DEBUG ("Pushing stream-start event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);
  gst_segment_init (&segment, GST_FORMAT_TIME);
  event = gst_event_new_segment (&segment);
  GST_DEBUG ("Pushing segment event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);

  /* Feed the init buffer, should create the source pad */
  inbuf = gst_buffer_new_and_alloc (init_mp4_len);
  gst_buffer_fill (inbuf, 0, init_mp4, init_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  GST_DEBUG ("Pushing moov buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_if (data.sinkpad == NULL);
  fail_unless_equals_int (data.num_srcpad, 1);

  /* Check stream-id */
  event = gst_pad_get_sticky_event (data.sinkpad, GST_EVENT_STREAM_START, 0);
  fail_unless (event != NULL);
  gst_event_parse_stream_start (event, &stream_id);
  fail_unless_equals_string (stream_id, expected_stream_id);
  g_free (expected_stream_id);
  gst_event_unref (event);

  /* Now send the moof and mdat of the first fragment */
  inbuf = gst_buffer_new_and_alloc (seg_1_m4f_len);
  gst_buffer_fill (inbuf, 0, seg_1_m4f, seg_1_m4f_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = init_mp4_len;
  GST_DEBUG ("Pushing moof and mdat buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);


  /***************
   * TEST GROUP 2
   * (track-id: 1)
   * - track-id change without new upstream stream-start event
   **************/
  /* Resend the init */
  inbuf = gst_buffer_new_and_alloc (BBB_32k_init_mp4_len);
  gst_buffer_fill (inbuf, 0, BBB_32k_init_mp4, BBB_32k_init_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  GST_BUFFER_FLAG_SET (inbuf, GST_BUFFER_FLAG_DISCONT);
  GST_DEBUG ("Pushing moov buffer again");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_if (data.sinkpad == NULL);
  /* new srcpad should be exposed */
  fail_unless_equals_int (data.num_srcpad, 2);

  /* Check stream-id */
  upstream_id = "TEST-GROUP-1"; /* upstream-id does not changed from GROUP 1 */
  track_id = 1;                 /* track-id is changed from 2 to 1 */
  expected_stream_id = g_strdup_printf ("%s/%03u", upstream_id, track_id);
  event = gst_pad_get_sticky_event (data.sinkpad, GST_EVENT_STREAM_START, 0);
  fail_unless (event != NULL);
  gst_event_parse_stream_start (event, &stream_id);
  fail_unless_equals_string (stream_id, expected_stream_id);
  g_free (expected_stream_id);
  gst_event_unref (event);

  /* push the moof and mdat again */
  inbuf = gst_buffer_new_and_alloc (BBB_32k_1_mp4_len);
  gst_buffer_fill (inbuf, 0, BBB_32k_1_mp4, BBB_32k_1_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = BBB_32k_init_mp4_len;
  GST_DEBUG ("Pushing moof and mdat buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);

  /***************
   * TEST GROUP 3
   * (track-id: 1)
   * - Push new stream-start and segment to qtdemux
   * - Reuse init and media data of GROUP 2 (no track-id change)
   **************/
  /* Send STREAM_START and segment (TIME) event */
  upstream_id = "TEST-GROUP-3";
  track_id = 1;
  expected_stream_id = g_strdup_printf ("%s/%03u", upstream_id, track_id);
  event = gst_event_new_stream_start (upstream_id);
  GST_DEBUG ("Pushing stream-start event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);
  gst_segment_init (&segment, GST_FORMAT_TIME);
  event = gst_event_new_segment (&segment);
  GST_DEBUG ("Pushing segment event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);

  /* Resend the init */
  inbuf = gst_buffer_new_and_alloc (BBB_32k_init_mp4_len);
  gst_buffer_fill (inbuf, 0, BBB_32k_init_mp4, BBB_32k_init_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  GST_BUFFER_FLAG_SET (inbuf, GST_BUFFER_FLAG_DISCONT);
  GST_DEBUG ("Pushing moov buffer again");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_if (data.sinkpad == NULL);
  /* new srcpad should be exposed */
  fail_unless_equals_int (data.num_srcpad, 3);

  /* Check stream-id */
  event = gst_pad_get_sticky_event (data.sinkpad, GST_EVENT_STREAM_START, 0);
  fail_unless (event != NULL);
  gst_event_parse_stream_start (event, &stream_id);
  fail_unless_equals_string (stream_id, expected_stream_id);
  g_free (expected_stream_id);
  gst_event_unref (event);

  /* push the moof and mdat again */
  inbuf = gst_buffer_new_and_alloc (BBB_32k_1_mp4_len);
  gst_buffer_fill (inbuf, 0, BBB_32k_1_mp4, BBB_32k_1_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = BBB_32k_init_mp4_len;
  GST_DEBUG ("Pushing moof and mdat buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);

  /***************
   * TEST GROUP 4
   * (track-id: 2)
   * - Push new stream-start and segment to qtdemux
   * - track-id change from 1 to 2
   **************/
  /* Send STREAM_START and segment (TIME) event */
  upstream_id = "TEST-GROUP-4";
  track_id = 2;
  expected_stream_id = g_strdup_printf ("%s/%03u", upstream_id, track_id);
  event = gst_event_new_stream_start (upstream_id);
  GST_DEBUG ("Pushing stream-start event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);
  gst_segment_init (&segment, GST_FORMAT_TIME);
  event = gst_event_new_segment (&segment);
  GST_DEBUG ("Pushing segment event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);

  /* Resend the init */
  inbuf = gst_buffer_new_and_alloc (init_mp4_len);
  gst_buffer_fill (inbuf, 0, init_mp4, init_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  GST_BUFFER_FLAG_SET (inbuf, GST_BUFFER_FLAG_DISCONT);
  GST_DEBUG ("Pushing moov buffer again");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_if (data.sinkpad == NULL);
  /* new srcpad should be exposed */
  fail_unless_equals_int (data.num_srcpad, 4);

  /* Check stream-id */
  event = gst_pad_get_sticky_event (data.sinkpad, GST_EVENT_STREAM_START, 0);
  fail_unless (event != NULL);
  gst_event_parse_stream_start (event, &stream_id);
  fail_unless_equals_string (stream_id, expected_stream_id);
  g_free (expected_stream_id);
  gst_event_unref (event);

  /* push the moof and mdat again */
  inbuf = gst_buffer_new_and_alloc (seg_1_m4f_len);
  gst_buffer_fill (inbuf, 0, seg_1_m4f, seg_1_m4f_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = init_mp4_len;
  GST_DEBUG ("Pushing moof and mdat buffer again");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_unless (gst_pad_send_event (sinkpad, gst_event_new_eos ()) == TRUE);
  fail_unless_equals_int (data.step, data.total_step);
  fail_unless (data.pending_pad == NULL);

  gst_object_unref (sinkpad);
  gst_pad_set_active (data.sinkpad, FALSE);
  gst_object_unref (data.sinkpad);
  gst_element_set_state (qtdemux, GST_STATE_NULL);
  gst_object_unref (qtdemux);
}

GST_END_TEST;

static Suite *
qtdemux_suite (void)
{
  Suite *s = suite_create ("qtdemux");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_qtdemux_input_gap);
  tcase_add_test (tc_chain, test_qtdemux_duplicated_moov);
  tcase_add_test (tc_chain, test_qtdemux_stream_change);

  return s;
}

GST_CHECK_MAIN (qtdemux)
