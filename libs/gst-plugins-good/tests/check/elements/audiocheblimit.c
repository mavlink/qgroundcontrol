/* GStreamer
 *
 * Copyright (C) 2007 Sebastian Dröge <slomo@circular-chaos.org>
 *
 * audiocheblimit.c: Unit test for the audiocheblimit element
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

#define BUFFER_CAPS_STRING_32           \
    "audio/x-raw, "                     \
    "channels = (int) 1, "              \
    "rate = (int) 44100, "              \
    "layout = (string) interleaved, "   \
    "format = (string) " GST_AUDIO_NE(F32)

#define BUFFER_CAPS_STRING_64           \
    "audio/x-raw, "                     \
    "channels = (int) 1, "              \
    "rate = (int) 44100, "              \
    "layout = (string) interleaved, "   \
    "format = (string) " GST_AUDIO_NE(F64)

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "channels = (int) 1, "
        "rate = (int) 44100, "
        "layout = (string) interleaved, "
        "format = (string) { "
        GST_AUDIO_NE (F32) ", " GST_AUDIO_NE (F64) " }"));
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "channels = (int) 1, "
        "rate = (int) 44100, "
        "layout = (string) interleaved, "
        "format = (string) { "
        GST_AUDIO_NE (F32) ", " GST_AUDIO_NE (F64) " }"));

static GstElement *
setup_audiocheblimit (void)
{
  GstElement *audiocheblimit;

  GST_DEBUG ("setup_audiocheblimit");
  audiocheblimit = gst_check_setup_element ("audiocheblimit");
  mysrcpad = gst_check_setup_src_pad (audiocheblimit, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (audiocheblimit, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return audiocheblimit;
}

static void
cleanup_audiocheblimit (GstElement * audiocheblimit)
{
  GST_DEBUG ("cleanup_audiocheblimit");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (audiocheblimit);
  gst_check_teardown_sink_pad (audiocheblimit);
  gst_check_teardown_element (audiocheblimit);
}

/* Test if data containing only one frequency component
 * at 0 is preserved with lowpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type1_32_lp_0hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to lowpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gfloat), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gfloat *) map.data;
  for (i = 0; i < 128; i++)
    in[i] = 1.0;
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gfloat *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms >= 0.9);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is erased with lowpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type1_32_lp_22050hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to lowpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gfloat), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gfloat *) map.data;
  for (i = 0; i < 128; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gfloat *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms <= 0.1);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is erased with highpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type1_32_hp_0hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to highpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gfloat), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gfloat *) map.data;
  for (i = 0; i < 128; i++)
    in[i] = 1.0;
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gfloat *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms <= 0.1);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is preserved with highpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type1_32_hp_22050hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to highpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gfloat), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gfloat *) map.data;
  for (i = 0; i < 128; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gfloat *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms >= 0.9);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is preserved with lowpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type1_64_lp_0hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to lowpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gdouble), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 128; i++)
    in[i] = 1.0;
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms >= 0.9);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is erased with lowpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type1_64_lp_22050hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to lowpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gdouble), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 128; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms <= 0.1);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is erased with highpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type1_64_hp_0hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to highpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gdouble), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 128; i++)
    in[i] = 1.0;
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms <= 0.1);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is preserved with highpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type1_64_hp_22050hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to highpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gdouble), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 128; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms >= 0.9);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is preserved with lowpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type2_32_lp_0hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to lowpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 2, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gfloat), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gfloat *) map.data;
  for (i = 0; i < 128; i++)
    in[i] = 1.0;
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gfloat *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms >= 0.9);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is erased with lowpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type2_32_lp_22050hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to lowpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 2, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gfloat), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gfloat *) map.data;
  for (i = 0; i < 128; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gfloat *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms <= 0.1);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is erased with highpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type2_32_hp_0hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to highpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 2, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gfloat), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gfloat *) map.data;
  for (i = 0; i < 128; i++)
    in[i] = 1.0;
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gfloat *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms <= 0.1);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is preserved with highpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type2_32_hp_22050hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to highpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 2, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_allocate (NULL, 128 * sizeof (gfloat), NULL);
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gfloat *) map.data;
  for (i = 0; i < 128; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gfloat *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms >= 0.9);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is preserved with lowpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type2_64_lp_0hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to lowpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 2, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_and_alloc (128 * sizeof (gdouble));
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 128; i++)
    in[i] = 1.0;
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms >= 0.9);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is erased with lowpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type2_64_lp_22050hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to lowpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 2, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_and_alloc (128 * sizeof (gdouble));
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 128; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms <= 0.1);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is erased with highpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type2_64_hp_0hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to highpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 2, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_and_alloc (128 * sizeof (gdouble));
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 128; i++)
    in[i] = 1.0;
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms <= 0.1);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is preserved with highpass mode and a cutoff
 * at rate/4 */
GST_START_TEST (test_type2_64_hp_22050hz)
{
  GstElement *audiocheblimit;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;
  GstMapInfo map;

  audiocheblimit = setup_audiocheblimit ();
  /* Set to highpass */
  g_object_set (G_OBJECT (audiocheblimit), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "type", 2, NULL);
  g_object_set (G_OBJECT (audiocheblimit), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiocheblimit,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiocheblimit), "cutoff", 44100 / 4.0, NULL);
  inbuffer = gst_buffer_new_and_alloc (128 * sizeof (gdouble));
  gst_buffer_map (inbuffer, &map, GST_MAP_WRITE);
  in = (gdouble *) map.data;
  for (i = 0; i < 128; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }
  gst_buffer_unmap (inbuffer, &map);

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_check_setup_events (mysrcpad, audiocheblimit, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  res = (gdouble *) map.data;

  rms = 0.0;
  for (i = 0; i < 128; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 128.0);
  fail_unless (rms >= 0.9);

  gst_buffer_unmap (outbuffer, &map);

  /* cleanup */
  cleanup_audiocheblimit (audiocheblimit);
}

GST_END_TEST;


static Suite *
audiocheblimit_suite (void)
{
  Suite *s = suite_create ("audiocheblimit");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_type1_32_lp_0hz);
  tcase_add_test (tc_chain, test_type1_32_lp_22050hz);
  tcase_add_test (tc_chain, test_type1_32_hp_0hz);
  tcase_add_test (tc_chain, test_type1_32_hp_22050hz);
  tcase_add_test (tc_chain, test_type1_64_lp_0hz);
  tcase_add_test (tc_chain, test_type1_64_lp_22050hz);
  tcase_add_test (tc_chain, test_type1_64_hp_0hz);
  tcase_add_test (tc_chain, test_type1_64_hp_22050hz);
  tcase_add_test (tc_chain, test_type2_32_lp_0hz);
  tcase_add_test (tc_chain, test_type2_32_lp_22050hz);
  tcase_add_test (tc_chain, test_type2_32_hp_0hz);
  tcase_add_test (tc_chain, test_type2_32_hp_22050hz);
  tcase_add_test (tc_chain, test_type2_64_lp_0hz);
  tcase_add_test (tc_chain, test_type2_64_lp_22050hz);
  tcase_add_test (tc_chain, test_type2_64_hp_0hz);
  tcase_add_test (tc_chain, test_type2_64_hp_22050hz);
  return s;
}

GST_CHECK_MAIN (audiocheblimit);
