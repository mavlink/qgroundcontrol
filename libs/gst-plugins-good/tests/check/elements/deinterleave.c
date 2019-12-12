/* GStreamer unit tests for the interleave element
 * Copyright (C) 2008 Sebastian Dröge <slomo@circular-chaos.org>
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

#include <stdio.h>
#include <gst/audio/audio.h>
#include <gst/check/gstcheck.h>
#include <gst/audio/audio.h>

GST_START_TEST (test_create_and_unref)
{
  GstElement *deinterleave;

  deinterleave = gst_element_factory_make ("deinterleave", NULL);
  fail_unless (deinterleave != NULL);

  gst_element_set_state (deinterleave, GST_STATE_NULL);
  gst_object_unref (deinterleave);
}

GST_END_TEST;

static GstPad *mysrcpad, **mysinkpads;
static gint nsinkpads;
static GstBus *bus;
static GstElement *deinterleave;

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F32) ", "
        "channels = (int) 1, layout = (string) {interleaved, non-interleaved}, rate = (int) {32000, 48000}"));

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F32) ", "
        "channels = (int) { 2, 3 }, layout = (string) interleaved, rate = (int) {32000, 48000}"));

#define CAPS_32khz \
        "audio/x-raw, " \
        "format = (string) "GST_AUDIO_NE (F32) ", " \
        "channels = (int) 2, layout = (string) interleaved, " \
        "rate = (int) 32000"

#define CAPS_48khz \
        "audio/x-raw, " \
        "format = (string) "GST_AUDIO_NE (F32) ", " \
        "channels = (int) 2, layout = (string) interleaved, " \
        "rate = (int) 48000"

#define CAPS_48khz_3CH \
        "audio/x-raw, " \
        "format = (string) "GST_AUDIO_NE (F32) ", " \
        "channels = (int) 3, layout = (string) interleaved, " \
        "rate = (int) 48000"

static GstFlowReturn
deinterleave_chain_func (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  gint i;
  GstMapInfo map;
  gfloat *indata;

  fail_unless (GST_IS_BUFFER (buffer));
  gst_buffer_map (buffer, &map, GST_MAP_READ);
  indata = (gfloat *) map.data;
  fail_unless_equals_int (map.size, 48000 * sizeof (gfloat));
  fail_unless (indata != NULL);

  if (strcmp (GST_PAD_NAME (pad), "sink0") == 0) {
    for (i = 0; i < 48000; i++)
      fail_unless_equals_float (indata[i], -1.0);
  } else if (strcmp (GST_PAD_NAME (pad), "sink1") == 0) {
    for (i = 0; i < 48000; i++)
      fail_unless_equals_float (indata[i], 1.0);
  } else {
    g_assert_not_reached ();
  }
  gst_buffer_unmap (buffer, &map);
  gst_buffer_unref (buffer);

  return GST_FLOW_OK;
}

static void
deinterleave_pad_added (GstElement * src, GstPad * pad, gpointer data)
{
  gchar *name;
  gint link = GPOINTER_TO_INT (data);

  if (nsinkpads >= link)
    return;

  name = g_strdup_printf ("sink%d", nsinkpads);

  mysinkpads[nsinkpads] =
      gst_pad_new_from_static_template (&sinktemplate, name);
  g_free (name);
  fail_if (mysinkpads[nsinkpads] == NULL);

  gst_pad_set_chain_function (mysinkpads[nsinkpads], deinterleave_chain_func);
  fail_unless (gst_pad_link (pad, mysinkpads[nsinkpads]) == GST_PAD_LINK_OK);
  gst_pad_set_active (mysinkpads[nsinkpads], TRUE);
  nsinkpads++;
}

GST_START_TEST (test_2_channels)
{
  GstPad *sinkpad;
  gint i;
  GstBuffer *inbuf;
  GstCaps *caps;
  gfloat *indata;
  GstMapInfo map;
  guint64 channel_mask = 0;

  mysinkpads = g_new0 (GstPad *, 2);
  nsinkpads = 0;

  deinterleave = gst_element_factory_make ("deinterleave", NULL);
  fail_unless (deinterleave != NULL);

  mysrcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  fail_unless (mysrcpad != NULL);
  gst_pad_set_active (mysrcpad, TRUE);

  caps = gst_caps_from_string (CAPS_48khz);
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
  gst_caps_set_simple (caps, "channel-mask", GST_TYPE_BITMASK, channel_mask,
      NULL);

  gst_check_setup_events (mysrcpad, deinterleave, caps, GST_FORMAT_TIME);

  sinkpad = gst_element_get_static_pad (deinterleave, "sink");
  fail_unless (sinkpad != NULL);
  fail_unless (gst_pad_link (mysrcpad, sinkpad) == GST_PAD_LINK_OK);
  g_object_unref (sinkpad);

  g_signal_connect (deinterleave, "pad-added",
      G_CALLBACK (deinterleave_pad_added), GINT_TO_POINTER (2));

  bus = gst_bus_new ();
  gst_element_set_bus (deinterleave, bus);

  fail_unless (gst_element_set_state (deinterleave,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS);

  inbuf = gst_buffer_new_and_alloc (2 * 48000 * sizeof (gfloat));
  inbuf = gst_buffer_make_writable (inbuf);
  gst_buffer_map (inbuf, &map, GST_MAP_WRITE);
  indata = (gfloat *) map.data;
  for (i = 0; i < 2 * 48000; i += 2) {
    indata[i] = -1.0;
    indata[i + 1] = 1.0;
  }
  gst_buffer_unmap (inbuf, &map);

  fail_unless (gst_pad_push (mysrcpad, inbuf) == GST_FLOW_OK);

  fail_unless (gst_element_set_state (deinterleave,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  for (i = 0; i < nsinkpads; i++)
    g_object_unref (mysinkpads[i]);
  g_free (mysinkpads);
  mysinkpads = NULL;

  g_object_unref (deinterleave);
  gst_bus_set_flushing (bus, TRUE);
  g_object_unref (bus);
  gst_caps_unref (caps);
  gst_object_unref (mysrcpad);
}

GST_END_TEST;

GST_START_TEST (test_2_channels_1_linked)
{
  GstPad *sinkpad;
  gint i;
  GstBuffer *inbuf;
  GstCaps *caps;
  gfloat *indata;
  GstMapInfo map;
  guint64 channel_mask = 0;

  nsinkpads = 0;
  mysinkpads = g_new0 (GstPad *, 2);

  deinterleave = gst_element_factory_make ("deinterleave", NULL);
  fail_unless (deinterleave != NULL);

  mysrcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  fail_unless (mysrcpad != NULL);
  gst_pad_set_active (mysrcpad, TRUE);

  caps = gst_caps_from_string (CAPS_48khz);
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
  gst_caps_set_simple (caps, "channel-mask", GST_TYPE_BITMASK, channel_mask,
      NULL);

  gst_check_setup_events (mysrcpad, deinterleave, caps, GST_FORMAT_TIME);

  sinkpad = gst_element_get_static_pad (deinterleave, "sink");
  fail_unless (sinkpad != NULL);
  fail_unless (gst_pad_link (mysrcpad, sinkpad) == GST_PAD_LINK_OK);
  g_object_unref (sinkpad);

  g_signal_connect (deinterleave, "pad-added",
      G_CALLBACK (deinterleave_pad_added), GINT_TO_POINTER (1));

  bus = gst_bus_new ();
  gst_element_set_bus (deinterleave, bus);

  fail_unless (gst_element_set_state (deinterleave,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS);

  inbuf = gst_buffer_new_and_alloc (2 * 48000 * sizeof (gfloat));
  inbuf = gst_buffer_make_writable (inbuf);
  gst_buffer_map (inbuf, &map, GST_MAP_WRITE);
  indata = (gfloat *) map.data;
  for (i = 0; i < 2 * 48000; i += 2) {
    indata[i] = -1.0;
    indata[i + 1] = 1.0;
  }
  gst_buffer_unmap (inbuf, &map);

  fail_unless (gst_pad_push (mysrcpad, inbuf) == GST_FLOW_OK);

  fail_unless (gst_element_set_state (deinterleave,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  for (i = 0; i < nsinkpads; i++)
    g_object_unref (mysinkpads[i]);
  g_free (mysinkpads);
  mysinkpads = NULL;

  g_object_unref (deinterleave);
  gst_bus_set_flushing (bus, TRUE);
  g_object_unref (bus);
  gst_caps_unref (caps);
  gst_object_unref (mysrcpad);
}

GST_END_TEST;

GST_START_TEST (test_2_channels_caps_change)
{
  GstPad *sinkpad;
  GstCaps *caps, *caps2;
  GstCaps *ret_caps;
  gint i;
  GstBuffer *inbuf;
  gfloat *indata;
  GstMapInfo map;
  guint64 channel_mask;

  nsinkpads = 0;
  mysinkpads = g_new0 (GstPad *, 2);

  deinterleave = gst_element_factory_make ("deinterleave", NULL);
  fail_unless (deinterleave != NULL);

  mysrcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  fail_unless (mysrcpad != NULL);

  gst_pad_set_active (mysrcpad, TRUE);

  caps = gst_caps_from_string (CAPS_48khz);
  channel_mask = 0;
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
  gst_caps_set_simple (caps, "channel-mask", GST_TYPE_BITMASK, channel_mask,
      NULL);

  sinkpad = gst_element_get_static_pad (deinterleave, "sink");
  fail_unless (sinkpad != NULL);
  fail_unless (gst_pad_link (mysrcpad, sinkpad) == GST_PAD_LINK_OK);
  g_object_unref (sinkpad);

  ret_caps = gst_pad_peer_query_caps (mysrcpad, caps);
  fail_if (gst_caps_is_empty (ret_caps));
  fail_unless (gst_pad_peer_query_accept_caps (mysrcpad, caps));
  gst_caps_unref (ret_caps);
  gst_check_setup_events (mysrcpad, deinterleave, caps, GST_FORMAT_TIME);

  g_signal_connect (deinterleave, "pad-added",
      G_CALLBACK (deinterleave_pad_added), GINT_TO_POINTER (2));

  bus = gst_bus_new ();
  gst_element_set_bus (deinterleave, bus);

  fail_unless (gst_element_set_state (deinterleave,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS);

  inbuf = gst_buffer_new_and_alloc (2 * 48000 * sizeof (gfloat));
  inbuf = gst_buffer_make_writable (inbuf);
  gst_buffer_map (inbuf, &map, GST_MAP_WRITE);
  indata = (gfloat *) map.data;
  for (i = 0; i < 2 * 48000; i += 2) {
    indata[i] = -1.0;
    indata[i + 1] = 1.0;
  }
  gst_buffer_unmap (inbuf, &map);

  fail_unless (gst_pad_push (mysrcpad, inbuf) == GST_FLOW_OK);

  caps2 = gst_caps_from_string (CAPS_32khz);
  channel_mask = 0;
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
  gst_caps_set_simple (caps2, "channel-mask", GST_TYPE_BITMASK, channel_mask,
      NULL);
  ret_caps = gst_pad_peer_query_caps (mysrcpad, caps2);
  fail_if (gst_caps_is_empty (ret_caps));
  fail_unless (gst_pad_peer_query_accept_caps (mysrcpad, caps2));
  gst_caps_unref (ret_caps);
  gst_pad_set_caps (mysrcpad, caps2);

  inbuf = gst_buffer_new_and_alloc (2 * 48000 * sizeof (gfloat));
  inbuf = gst_buffer_make_writable (inbuf);
  gst_buffer_map (inbuf, &map, GST_MAP_WRITE);
  indata = (gfloat *) map.data;
  for (i = 0; i < 2 * 48000; i += 2) {
    indata[i] = -1.0;
    indata[i + 1] = 1.0;
  }
  gst_buffer_unmap (inbuf, &map);

  /* Should work fine because the caps changed in a compatible way */
  fail_unless (gst_pad_push (mysrcpad, inbuf) == GST_FLOW_OK);

  gst_caps_unref (caps2);

  caps2 = gst_caps_from_string (CAPS_48khz_3CH);
  channel_mask = 0;
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
  channel_mask |=
      G_GUINT64_CONSTANT (1) << GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER;
  gst_caps_set_simple (caps2, "channel-mask", GST_TYPE_BITMASK, channel_mask,
      NULL);
  ret_caps = gst_pad_peer_query_caps (mysrcpad, caps2);
  fail_unless (gst_caps_is_empty (ret_caps));
  gst_caps_unref (ret_caps);
  fail_if (gst_pad_peer_query_accept_caps (mysrcpad, caps2));
  gst_pad_set_caps (mysrcpad, caps2);

  inbuf = gst_buffer_new_and_alloc (3 * 48000 * sizeof (gfloat));
  inbuf = gst_buffer_make_writable (inbuf);
  gst_buffer_map (inbuf, &map, GST_MAP_WRITE);
  indata = (gfloat *) map.data;
  for (i = 0; i < 3 * 48000; i += 3) {
    indata[i] = -1.0;
    indata[i + 1] = 1.0;
    indata[i + 2] = 0.0;
  }
  gst_buffer_unmap (inbuf, &map);

  /* Should break because the caps changed in an incompatible way */
  fail_if (gst_pad_push (mysrcpad, inbuf) == GST_FLOW_OK);

  fail_unless (gst_element_set_state (deinterleave,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  for (i = 0; i < nsinkpads; i++)
    g_object_unref (mysinkpads[i]);
  g_free (mysinkpads);
  mysinkpads = NULL;

  g_object_unref (deinterleave);
  gst_bus_set_flushing (bus, TRUE);
  g_object_unref (bus);
  gst_caps_unref (caps);
  gst_caps_unref (caps2);
  gst_object_unref (mysrcpad);
}

GST_END_TEST;


#define SAMPLES_PER_BUFFER  10
#define NUM_CHANNELS        8
#define SAMPLE_RATE         44100

static guint pads_created;

static void
set_channel_positions (GstCaps * caps, int channels,
    GstAudioChannelPosition * channelpositions)
{
  int c;
  guint64 channel_mask = 0;

  for (c = 0; c < channels; c++)
    channel_mask |= G_GUINT64_CONSTANT (1) << channelpositions[c];

  gst_caps_set_simple (caps, "channel-mask", GST_TYPE_BITMASK, channel_mask,
      NULL);
}

static void
src_handoff_float32_8ch (GstElement * src, GstBuffer * buf, GstPad * pad,
    gpointer user_data)
{
  gfloat *data, *p;
  guint size, i, c;

  size = sizeof (gfloat) * SAMPLES_PER_BUFFER * NUM_CHANNELS;
  data = p = (gfloat *) g_malloc (size);

  for (i = 0; i < SAMPLES_PER_BUFFER; ++i) {
    for (c = 0; c < NUM_CHANNELS; ++c) {
      *p = (gfloat) ((i * NUM_CHANNELS) + c);
      ++p;
    }
  }

  if (gst_buffer_n_memory (buf)) {
    gst_buffer_replace_memory_range (buf, 0, -1,
        gst_memory_new_wrapped (0, data, size, 0, size, data, g_free));
  } else {
    gst_buffer_insert_memory (buf, 0,
        gst_memory_new_wrapped (0, data, size, 0, size, data, g_free));
  }
  GST_BUFFER_OFFSET (buf) = 0;
  GST_BUFFER_TIMESTAMP (buf) = 0;
}

static GstPadProbeReturn
src_event_probe (GstPad * pad, GstPadProbeInfo * info, gpointer userdata)
{
  GstAudioChannelPosition layout[NUM_CHANNELS];
  GstCaps *caps;
  guint i;

  if ((info->type & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM)
      && GST_EVENT_TYPE (info->data) == GST_EVENT_STREAM_START) {
    gst_pad_remove_probe (pad, info->id);

    caps = gst_caps_new_simple ("audio/x-raw",
        "format", G_TYPE_STRING, GST_AUDIO_NE (F32),
        "channels", G_TYPE_INT, NUM_CHANNELS,
        "layout", G_TYPE_STRING, "interleaved",
        "rate", G_TYPE_INT, SAMPLE_RATE, NULL);

    for (i = 0; i < NUM_CHANNELS; ++i)
      layout[i] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT + i;

    set_channel_positions (caps, NUM_CHANNELS, layout);
    gst_pad_set_caps (pad, caps);
    gst_caps_unref (caps);
  }

  return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn
float_buffer_check_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer userdata)
{
  GstMapInfo map;
  gfloat *data;
  guint padnum, numpads;
  guint num, i;
  GstCaps *caps;
  GstStructure *s;
  GstAudioChannelPosition *pos;
  gint channels;
  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);
  GstAudioInfo audio_info;
  guint pad_id = GPOINTER_TO_UINT (userdata);

  fail_unless_equals_int (sscanf (GST_PAD_NAME (pad), "src_%u", &padnum), 1);

  numpads = pads_created;

  /* Check caps */
  caps = gst_pad_get_current_caps (pad);
  fail_unless (caps != NULL);
  s = gst_caps_get_structure (caps, 0);
  fail_unless (gst_structure_get_int (s, "channels", &channels));
  fail_unless_equals_int (channels, 1);

  gst_audio_info_init (&audio_info);
  fail_unless (gst_audio_info_from_caps (&audio_info, caps));

  pos = audio_info.position;
  fail_unless (pos != NULL
      && pos[0] == GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT + pad_id);
  gst_caps_unref (caps);

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  data = (gfloat *) map.data;
  num = map.size / sizeof (gfloat);

  /* Check buffer content */
  for (i = 0; i < num; ++i) {
    guint val, rest;

    val = (guint) data[i];
    GST_LOG ("%s[%u]: %8f", GST_PAD_NAME (pad), i, data[i]);
    /* can't use the modulo operator in the assertion statement, since due to
     * the way it gets expanded it would be interpreted as a printf operator
     * in the failure case, which will result in segfaults */
    rest = val % numpads;
    /* check that the first channel is on pad src0, the second on src1 etc. */
    fail_unless_equals_int (rest, padnum);
  }
  gst_buffer_unmap (buffer, &map);

  return GST_PAD_PROBE_OK;      /* don't drop data */
}

static void
pad_added_setup_data_check_float32_8ch_cb (GstElement * deinterleave,
    GstPad * pad, GstElement * pipeline)
{
  GstElement *queue, *sink;
  GstPad *sinkpad;

  queue = gst_element_factory_make ("queue", NULL);
  fail_unless (queue != NULL);

  sink = gst_element_factory_make ("fakesink", NULL);
  fail_unless (sink != NULL);

  gst_bin_add_many (GST_BIN (pipeline), queue, sink, NULL);
  gst_element_link_pads_full (queue, "src", sink, "sink",
      GST_PAD_LINK_CHECK_NOTHING);

  sinkpad = gst_element_get_static_pad (queue, "sink");

  fail_unless_equals_int (gst_pad_link (pad, sinkpad), GST_PAD_LINK_OK);
  gst_object_unref (sinkpad);

  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER, float_buffer_check_probe,
      GUINT_TO_POINTER (pads_created), NULL);

  gst_element_set_state (sink, GST_STATE_PLAYING);
  gst_element_set_state (queue, GST_STATE_PLAYING);

  GST_LOG ("new pad: %s", GST_PAD_NAME (pad));
  ++pads_created;
}

static GstElement *
make_fake_src_8chans_float32 (void)
{
  GstElement *src;
  GstPad *pad;

  src = gst_element_factory_make ("fakesrc", "src");
  fail_unless (src != NULL, "failed to create fakesrc element");

  g_object_set (src, "num-buffers", 1, NULL);
  g_object_set (src, "signal-handoffs", TRUE, NULL);

  g_signal_connect (src, "handoff", G_CALLBACK (src_handoff_float32_8ch), NULL);

  pad = gst_element_get_static_pad (src, "src");
  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, src_event_probe,
      NULL, NULL);
  gst_object_unref (pad);

  return src;
}

GST_START_TEST (test_8_channels_float32)
{
  GstElement *pipeline, *src, *deinterleave;
  GstMessage *msg;

  pipeline = (GstElement *) gst_pipeline_new ("pipeline");
  fail_unless (pipeline != NULL, "failed to create pipeline");

  src = make_fake_src_8chans_float32 ();

  deinterleave = gst_element_factory_make ("deinterleave", "deinterleave");
  fail_unless (deinterleave != NULL, "failed to create deinterleave element");
  g_object_set (deinterleave, "keep-positions", TRUE, NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, deinterleave, NULL);

  fail_unless (gst_element_link (src, deinterleave),
      "failed to link src <=> deinterleave");

  g_signal_connect (deinterleave, "pad-added",
      G_CALLBACK (pad_added_setup_data_check_float32_8ch_cb), pipeline);

  pads_created = 0;

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  msg = gst_bus_poll (GST_ELEMENT_BUS (pipeline), GST_MESSAGE_EOS, -1);
  gst_message_unref (msg);

  fail_unless_equals_int (pads_created, NUM_CHANNELS);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
}

GST_END_TEST;

static Suite *
deinterleave_suite (void)
{
  Suite *s = suite_create ("deinterleave");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_set_timeout (tc_chain, 180);
  tcase_add_test (tc_chain, test_create_and_unref);
  tcase_add_test (tc_chain, test_2_channels);
  tcase_add_test (tc_chain, test_2_channels_1_linked);
  tcase_add_test (tc_chain, test_2_channels_caps_change);
  tcase_add_test (tc_chain, test_8_channels_float32);

  return s;
}

GST_CHECK_MAIN (deinterleave);
