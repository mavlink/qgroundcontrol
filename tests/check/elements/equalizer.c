/* GStreamer
 *
 * Copyright (C) 2008 Sebastian Dröge <slomo@circular-chaos.org>
 *
 * equalizer.c: Unit test for the equalizer element
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/base/gstbasetransform.h>
#include <gst/check/gstcheck.h>

#include <math.h>

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
GstPad *mysrcpad, *mysinkpad;

#define EQUALIZER_CAPS_STRING                     \
    "audio/x-raw, "                               \
    "format = (string) "GST_AUDIO_NE (F64) ", "   \
    "layout = (string) interleaved, "             \
    "channels = (int) 1, "                        \
    "rate = (int) 48000"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F64) ", "
        "layout = (string) interleaved, "
        "channels = (int) 1, " "rate = (int) 48000")
    );
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F64) ", "
        "layout = (string) interleaved, "
        "channels = (int) 1, " "rate = (int) 48000")
    );

static GstElement *
setup_equalizer (void)
{
  GstElement *equalizer;

  GST_DEBUG ("setup_equalizer");
  equalizer = gst_check_setup_element ("equalizer-nbands");
  mysrcpad = gst_check_setup_src_pad (equalizer, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (equalizer, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return equalizer;
}

static void
cleanup_equalizer (GstElement * equalizer)
{
  GST_DEBUG ("cleanup_equalizer");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (equalizer);
  gst_check_teardown_sink_pad (equalizer);
  gst_check_teardown_element (equalizer);
}

GST_START_TEST (test_equalizer_5bands_passthrough)
{
  GstElement *equalizer;
  GstBuffer *inbuffer;
  GstCaps *caps;
  gdouble *in, *res;
  gint i;
  GstMapInfo map;

  equalizer = setup_equalizer ();
  g_object_set (G_OBJECT (equalizer), "num-bands", 5, NULL);

  fail_unless_equals_int (gst_child_proxy_get_children_count (GST_CHILD_PROXY
          (equalizer)), 5);

  fail_unless (gst_element_set_state (equalizer,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 1024; i++)
    in[i] = g_random_double_range (-1.0, 1.0);
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (EQUALIZER_CAPS_STRING);
  gst_check_setup_events (mysrcpad, equalizer, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));
  /* ... and puts a new buffer on the global list */
  fail_unless (g_list_length (buffers) == 1);

  gst_buffer_map (GST_BUFFER (buffers->data), &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  for (i = 0; i < 1024; i++)
    fail_unless_equals_float (in[i], res[i]);
  gst_buffer_unmap (GST_BUFFER (buffers->data), &map);

  /* cleanup */
  cleanup_equalizer (equalizer);
}

GST_END_TEST;

GST_START_TEST (test_equalizer_5bands_minus_24)
{
  GstElement *equalizer;
  GstBuffer *inbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms_in, rms_out;
  gint i;
  GstMapInfo map;

  equalizer = setup_equalizer ();
  g_object_set (G_OBJECT (equalizer), "num-bands", 5, NULL);

  fail_unless_equals_int (gst_child_proxy_get_children_count (GST_CHILD_PROXY
          (equalizer)), 5);

  for (i = 0; i < 5; i++) {
    GObject *band =
        gst_child_proxy_get_child_by_index (GST_CHILD_PROXY (equalizer), i);
    fail_unless (band != NULL);

    g_object_set (band, "gain", -24.0, NULL);
    g_object_unref (band);
  }

  fail_unless (gst_element_set_state (equalizer,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 1024; i++)
    in[i] = g_random_double_range (-1.0, 1.0);
  gst_buffer_unmap (inbuffer, &map);

  rms_in = 0.0;
  for (i = 0; i < 1024; i++)
    rms_in += in[i] * in[i];
  rms_in = sqrt (rms_in / 1024);

  caps = gst_caps_from_string (EQUALIZER_CAPS_STRING);
  gst_check_setup_events (mysrcpad, equalizer, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));
  /* ... and puts a new buffer on the global list */
  fail_unless (g_list_length (buffers) == 1);

  gst_buffer_map (GST_BUFFER (buffers->data), &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms_out = 0.0;
  for (i = 0; i < 1024; i++)
    rms_out += res[i] * res[i];
  rms_out = sqrt (rms_out / 1024);
  gst_buffer_unmap (GST_BUFFER (buffers->data), &map);

  fail_unless (rms_in > rms_out);

  /* cleanup */
  cleanup_equalizer (equalizer);
}

GST_END_TEST;

GST_START_TEST (test_equalizer_5bands_plus_12)
{
  GstElement *equalizer;
  GstBuffer *inbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms_in, rms_out;
  gint i;
  GstMapInfo map;

  equalizer = setup_equalizer ();
  g_object_set (G_OBJECT (equalizer), "num-bands", 5, NULL);

  fail_unless_equals_int (gst_child_proxy_get_children_count (GST_CHILD_PROXY
          (equalizer)), 5);

  for (i = 0; i < 5; i++) {
    GObject *band =
        gst_child_proxy_get_child_by_index (GST_CHILD_PROXY (equalizer), i);
    fail_unless (band != NULL);

    g_object_set (band, "gain", 12.0, NULL);
    g_object_unref (band);
  }

  fail_unless (gst_element_set_state (equalizer,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 1024; i++)
    in[i] = g_random_double_range (-1.0, 1.0);
  gst_buffer_unmap (inbuffer, &map);

  rms_in = 0.0;
  for (i = 0; i < 1024; i++)
    rms_in += in[i] * in[i];
  rms_in = sqrt (rms_in / 1024);

  caps = gst_caps_from_string (EQUALIZER_CAPS_STRING);
  gst_check_setup_events (mysrcpad, equalizer, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()));
  /* ... and puts a new buffer on the global list */
  fail_unless (g_list_length (buffers) == 1);

  gst_buffer_map (GST_BUFFER (buffers->data), &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms_out = 0.0;
  for (i = 0; i < 1024; i++)
    rms_out += res[i] * res[i];
  rms_out = sqrt (rms_out / 1024);
  gst_buffer_unmap (GST_BUFFER (buffers->data), &map);

  fail_unless (rms_in < rms_out);

  /* cleanup */
  cleanup_equalizer (equalizer);
}

GST_END_TEST;

GST_START_TEST (test_equalizer_band_number_changing)
{
  GstElement *equalizer;
  gint i;

  equalizer = setup_equalizer ();

  g_object_set (G_OBJECT (equalizer), "num-bands", 5, NULL);
  fail_unless_equals_int (gst_child_proxy_get_children_count (GST_CHILD_PROXY
          (equalizer)), 5);

  for (i = 0; i < 5; i++) {
    GObject *band;

    band = gst_child_proxy_get_child_by_index (GST_CHILD_PROXY (equalizer), i);
    fail_unless (band != NULL);
    g_object_unref (band);
  }

  g_object_set (G_OBJECT (equalizer), "num-bands", 10, NULL);
  fail_unless_equals_int (gst_child_proxy_get_children_count (GST_CHILD_PROXY
          (equalizer)), 10);

  for (i = 0; i < 10; i++) {
    GObject *band;

    band = gst_child_proxy_get_child_by_index (GST_CHILD_PROXY (equalizer), i);
    fail_unless (band != NULL);
    g_object_unref (band);
  }

  /* cleanup */
  cleanup_equalizer (equalizer);
}

GST_END_TEST;

GST_START_TEST (test_equalizer_presets)
{
  GstElement *eq1, *eq2;
  gint type;
  gdouble gain, freq;

  eq1 = gst_check_setup_element ("equalizer-nbands");
  g_object_set (G_OBJECT (eq1), "num-bands", 3, NULL);

  /* set properties to non-defaults */
  gst_child_proxy_set ((GstChildProxy *) eq1,
      "band0::type", 0, "band0::gain", -3.0, "band0::freq", 100.0,
      "band1::type", 1, "band1::gain", +3.0, "band1::freq", 1000.0,
      "band2::type", 2, "band2::gain", +9.0, "band2::freq", 10000.0, NULL);

  /* save preset */
  gst_preset_save_preset ((GstPreset *) eq1, "_testpreset_");
  GST_INFO_OBJECT (eq1, "Preset saved");

  eq2 = gst_check_setup_element ("equalizer-nbands");
  g_object_set (G_OBJECT (eq2), "num-bands", 3, NULL);

  /* load preset */
  gst_preset_load_preset ((GstPreset *) eq2, "_testpreset_");
  GST_INFO_OBJECT (eq1, "Preset loaded");

  /* compare properties */
  gst_child_proxy_get ((GstChildProxy *) eq2,
      "band0::type", &type, "band0::gain", &gain, "band0::freq", &freq, NULL);
  ck_assert_int_eq (type, 0);
  fail_unless (gain == -3.0, NULL);
  fail_unless (freq == 100.0, NULL);
  gst_child_proxy_get ((GstChildProxy *) eq2,
      "band1::type", &type, "band1::gain", &gain, "band1::freq", &freq, NULL);
  ck_assert_int_eq (type, 1);
  fail_unless (gain == +3.0, NULL);
  fail_unless (freq == 1000.0, NULL);
  gst_child_proxy_get ((GstChildProxy *) eq2,
      "band2::type", &type, "band2::gain", &gain, "band2::freq", &freq, NULL);
  ck_assert_int_eq (type, 2);
  fail_unless (gain == +9.0, NULL);
  fail_unless (freq == 10000.0, NULL);

  gst_preset_delete_preset ((GstPreset *) eq1, "_testpreset_");
  gst_check_teardown_element (eq1);
  gst_check_teardown_element (eq2);
}

GST_END_TEST;


static Suite *
equalizer_suite (void)
{
  Suite *s = suite_create ("equalizer");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_equalizer_5bands_passthrough);
  tcase_add_test (tc_chain, test_equalizer_5bands_minus_24);
  tcase_add_test (tc_chain, test_equalizer_5bands_plus_12);
  tcase_add_test (tc_chain, test_equalizer_band_number_changing);
  tcase_add_test (tc_chain, test_equalizer_presets);

  return s;
}

GST_CHECK_MAIN (equalizer);
