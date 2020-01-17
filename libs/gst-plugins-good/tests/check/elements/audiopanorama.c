/* GStreamer
 *
 * unit test for audiopanorama
 *
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
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

#include <gst/audio/audio.h>
#include <gst/base/gstbasetransform.h>
#include <gst/check/gstcheck.h>

gboolean have_eos = FALSE;

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
GstPad *mysrcpad, *mysinkpad;


#define PANORAMA_S16_MONO_CAPS_STRING   \
    "audio/x-raw, "                     \
    "channels = (int) 1, "              \
    "rate = (int) 44100, "              \
    "layout = (string) interleaved, "   \
    "format = (string) " GST_AUDIO_NE(S16)

#define PANORAMA_S16_STEREO_CAPS_STRING \
    "audio/x-raw, "                     \
    "channels = (int) 2, "              \
    "channel-mask = (bitmask) 3, "      \
    "rate = (int) 44100, "              \
    "layout = (string) interleaved, "   \
    "format = (string) " GST_AUDIO_NE(S16)

#define PANORAMA_F32_MONO_CAPS_STRING   \
    "audio/x-raw, "                     \
    "channels = (int) 1, "              \
    "rate = (int) 44100, "              \
    "layout = (string) interleaved, "   \
    "format = (string) " GST_AUDIO_NE(F32)

#define PANORAMA_F32_STEREO_CAPS_STRING \
    "audio/x-raw, "                     \
    "channels = (int) 2, "              \
    "channel-mask = (bitmask) 3, "      \
    "rate = (int) 44100, "              \
    "layout = (string) interleaved, "   \
    "format = (string) " GST_AUDIO_NE(F32)

#define PANORAMA_WRONG_CAPS_STRING  \
    "audio/x-raw, "                     \
    "channels = (int) 5, "              \
    "rate = (int) 44100, "              \
    "layout = (string) interleaved, "   \
    "format = (string) " GST_AUDIO_NE(U16)


static GstStaticPadTemplate sinktemplate[2] = {
  GST_STATIC_PAD_TEMPLATE ("sink",
      GST_PAD_SINK,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("audio/x-raw, "
          "channels = (int) 2, "
          "rate = (int) [ 1,  MAX ], "
          "layout = (string) interleaved, "
          "format = (string) " GST_AUDIO_NE (S16))
      ),
  GST_STATIC_PAD_TEMPLATE ("sink",
      GST_PAD_SINK,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("audio/x-raw, "
          "channels = (int) 2, "
          "rate = (int) [ 1,  MAX ], "
          "layout = (string) interleaved, "
          "format = (string) " GST_AUDIO_NE (F32))
      ),
};

static GstStaticPadTemplate msrctemplate[2] = {
  GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("audio/x-raw, "
          "channels = (int) 1, "
          "rate = (int) [ 1,  MAX ], "
          "layout = (string) interleaved, "
          "format = (string) " GST_AUDIO_NE (S16))
      ),
  GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("audio/x-raw, "
          "channels = (int) 1, "
          "rate = (int) [ 1,  MAX ], "
          "layout = (string) interleaved, "
          "format = (string) " GST_AUDIO_NE (F32))
      ),
};

static GstStaticPadTemplate ssrctemplate[2] = {
  GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("audio/x-raw, "
          "channels = (int) 2, "
          "rate = (int) [ 1,  MAX ], "
          "layout = (string) interleaved, "
          "format = (string) " GST_AUDIO_NE (S16))
      ),
  GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("audio/x-raw, "
          "channels = (int) 2, "
          "rate = (int) [ 1,  MAX ], "
          "layout = (string) interleaved, "
          "format = (string) " GST_AUDIO_NE (F32))
      ),
};

static GstElement *
setup_panorama (GstStaticPadTemplate * srctemplate, gint fmt,
    const gchar * caps_str)
{
  GstElement *panorama;

  GST_DEBUG ("setup_panorama");
  panorama = gst_check_setup_element ("audiopanorama");
  mysrcpad = gst_check_setup_src_pad (panorama, &srctemplate[fmt]);
  mysinkpad = gst_check_setup_sink_pad (panorama, &sinktemplate[fmt]);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  if (caps_str) {
    GstCaps *caps = gst_caps_from_string (caps_str);
    gst_check_setup_events (mysrcpad, panorama, caps, GST_FORMAT_TIME);
    gst_caps_unref (caps);
  }
  return panorama;
}

static GstElement *
setup_panorama_s16_m (gint method, gfloat pan)
{
  GstElement *panorama;
  panorama = setup_panorama (msrctemplate, 0, PANORAMA_S16_MONO_CAPS_STRING);
  g_object_set (G_OBJECT (panorama), "method", method, "panorama", pan, NULL);
  gst_element_set_state (panorama, GST_STATE_PLAYING);
  GST_DEBUG ("panorama(mono) ready");

  return panorama;
}

static GstElement *
setup_panorama_f32_m (gint method, gfloat pan)
{
  GstElement *panorama;
  panorama = setup_panorama (msrctemplate, 1, PANORAMA_F32_MONO_CAPS_STRING);
  g_object_set (G_OBJECT (panorama), "method", method, "panorama", pan, NULL);
  gst_element_set_state (panorama, GST_STATE_PLAYING);
  GST_DEBUG ("panorama(mono) ready");

  return panorama;
}

static GstElement *
setup_panorama_s16_s (gint method, gfloat pan)
{
  GstElement *panorama;
  panorama = setup_panorama (ssrctemplate, 0, PANORAMA_S16_STEREO_CAPS_STRING);
  g_object_set (G_OBJECT (panorama), "method", method, "panorama", pan, NULL);
  gst_element_set_state (panorama, GST_STATE_PLAYING);
  GST_DEBUG ("panorama(stereo) ready");

  return panorama;
}

static GstElement *
setup_panorama_f32_s (gint method, gfloat pan)
{
  GstElement *panorama;
  panorama = setup_panorama (ssrctemplate, 1, PANORAMA_F32_STEREO_CAPS_STRING);
  g_object_set (G_OBJECT (panorama), "method", method, "panorama", pan, NULL);
  gst_element_set_state (panorama, GST_STATE_PLAYING);
  GST_DEBUG ("panorama(stereo) ready");

  return panorama;
}

static void
cleanup_panorama (GstElement * panorama)
{
  GST_DEBUG ("cleaning up");
  gst_element_set_state (panorama, GST_STATE_NULL);

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (panorama);
  gst_check_teardown_sink_pad (panorama);
  gst_check_teardown_element (panorama);
}

static GstBuffer *
setup_buffer (gpointer data, gsize size)
{
  return gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, data, size, 0,
      size, NULL, NULL);
}

static void
do_panorama (gpointer in_data, gsize in_size, gpointer out_data, gsize out_size)
{
  GstBuffer *in, *out;

  in = setup_buffer (in_data, in_size);
  fail_unless (gst_pad_push (mysrcpad, in) == GST_FLOW_OK);
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((out = (GstBuffer *) buffers->data) == NULL);
  fail_unless (gst_buffer_extract (out, 0, out_data, out_size) == out_size);
}

/* the tests */

GST_START_TEST (test_ref_counts)
{
  GstElement *panorama;
  GstBuffer *inbuffer, *outbuffer;
  gint16 in[2] = { 16384, -256 };

  panorama = setup_panorama (msrctemplate, 0, PANORAMA_S16_MONO_CAPS_STRING);
  fail_unless (gst_element_set_state (panorama,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  inbuffer = setup_buffer (in, sizeof (in));
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... but it ends up being collected on the global buffer list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);
  fail_if (inbuffer == outbuffer);

  /* cleanup */
  fail_unless (gst_element_set_state (panorama,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS, "could not set to null");
  ASSERT_OBJECT_REFCOUNT (panorama, "panorama", 1);
  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_wrong_caps)
{
  GstElement *panorama;
  GstBuffer *inbuffer;
  gint16 in[2] = { 16384, -256 };
  GstBus *bus;
  GstMessage *message;
  GstCaps *caps;

  panorama = setup_panorama (msrctemplate, 0, NULL);
  bus = gst_bus_new ();
  gst_element_set_state (panorama, GST_STATE_PLAYING);

  inbuffer = setup_buffer (in, sizeof (in));
  gst_buffer_ref (inbuffer);

  /* set a bus here so we avoid getting state change messages */
  gst_element_set_bus (panorama, bus);

  caps = gst_caps_from_string (PANORAMA_WRONG_CAPS_STRING);
  /* this actually succeeds, because the caps event is sticky */
  gst_check_setup_events (mysrcpad, panorama, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  /* pushing gives an error because it can't negotiate with wrong caps */
  fail_unless_equals_int (gst_pad_push (mysrcpad, inbuffer),
      GST_FLOW_NOT_NEGOTIATED);
  /* ... and the buffer would have been lost if we didn't ref it ourselves */
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);
  gst_buffer_unref (inbuffer);
  fail_unless_equals_int (g_list_length (buffers), 0);

  /* panorama_set_caps should not have been called since basetransform caught
   * the negotiation problem */
  fail_if ((message = gst_bus_pop (bus)) != NULL);

  /* cleanup */
  gst_element_set_bus (panorama, NULL);
  gst_object_unref (GST_OBJECT (bus));
  gst_element_set_state (panorama, GST_STATE_NULL);
  cleanup_panorama (panorama);
}

GST_END_TEST;

/* processing for method=psy */

GST_START_TEST (test_s16_mono_middle)
{
  gint16 in[2] = { 16384, -256 };
  gint16 out[4] = { 8192, 8192, -128, -128 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_m (0, 0.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_mono_left)
{
  gint16 in[2] = { 16384, -256 };
  gint16 out[4] = { 16384, 0, -256, 0 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_m (0, -1.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_mono_right)
{
  gint16 in[2] = { 16384, -256 };
  gint16 out[4] = { 0, 16384, 0, -256 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_m (0, 1.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_stereo_middle)
{
  gint16 in[4] = { 16384, -256, 8192, 128 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_s (0, 0.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", in[0], in[1], in[2], in[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, in, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_stereo_left)
{
  gint16 in[4] = { 16384, -256, 8192, 128 };
  gint16 out[4] = { 16384 - 256, 0, 8192 + 128, 0 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_s (0, -1.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_stereo_right)
{
  gint16 in[4] = { 16384, -256, 8192, 128 };
  gint16 out[4] = { 0, -256 + 16384, 0, 128 + 8192 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_s (0, 1.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_mono_middle)
{
  gfloat in[2] = { 0.5, -0.2 };
  gfloat out[4] = { 0.25, 0.25, -0.1, -0.1 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_m (0, 0.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_mono_left)
{
  gfloat in[2] = { 0.5, -0.2 };
  gfloat out[4] = { 0.5, 0.0, -0.2, 0.0 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_m (0, -1.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_mono_right)
{
  gfloat in[2] = { 0.5, -0.2 };
  gfloat out[4] = { 0.0, 0.5, 0.0, -0.2 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_m (0, 1.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_stereo_middle)
{
  gfloat in[4] = { 0.5, -0.2, 0.25, 0.1 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_s (0, 0.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", in[0], in[1], in[2], in[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == in[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_stereo_left)
{
  gfloat in[4] = { 0.5, -0.2, 0.25, 0.1 };
  gfloat out[4] = { 0.5 - 0.2, 0.0, 0.25 + 0.1, 0.0 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_s (0, -1.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_stereo_right)
{
  gfloat in[4] = { 0.5, -0.2, 0.25, 0.1 };
  gfloat out[4] = { 0.0, -0.2 + 0.5, 0.0, 0.1 + 0.25 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_s (0, 1.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

/* processing for method=simple */

GST_START_TEST (test_s16_mono_middle_simple)
{
  gint16 in[2] = { 16384, -256 };
  gint16 out[4] = { 16384, 16384, -256, -256 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_m (1, 0.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_mono_left_simple)
{
  gint16 in[2] = { 16384, -256 };
  gint16 out[4] = { 16384, 0, -256, 0 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_m (1, -1.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_mono_right_simple)
{
  gint16 in[2] = { 16384, -256 };
  gint16 out[4] = { 0, 16384, 0, -256 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_m (1, 1.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_stereo_middle_simple)
{
  gint16 in[4] = { 16384, -256, 8192, 128 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_s (1, 0.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", in[0], in[1], in[2], in[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, in, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_stereo_left_simple)
{
  gint16 in[4] = { 16384, -256, 8192, 128 };
  gint16 out[4] = { 16384, 0, 8192, 0 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_s (1, -1.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_s16_stereo_right_simple)
{
  gint16 in[4] = { 16384, -256, 8192, 128 };
  gint16 out[4] = { 0, -256, 0, 128 };
  gint16 res[4];
  GstElement *panorama = setup_panorama_s16_s (1, 1.0);

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+5d %+5d %+5d %+5d", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+5d %+5d %+5d %+5d", res[0], res[1], res[2], res[3]);
  fail_unless (memcmp (res, out, sizeof (res)) == 0);

  cleanup_panorama (panorama);
}

GST_END_TEST;

//-----------------------------------------------------------------------------

GST_START_TEST (test_f32_mono_middle_simple)
{
  gfloat in[2] = { 0.5, -0.2 };
  gfloat out[4] = { 0.5, 0.5, -0.2, -0.2 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_m (1, 0.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_mono_left_simple)
{
  gfloat in[2] = { 0.5, -0.2 };
  gfloat out[4] = { 0.5, 0.0, -0.2, 0.0 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_m (1, -1.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_mono_right_simple)
{
  gfloat in[2] = { 0.5, -0.2 };
  gfloat out[4] = { 0.0, 0.5, 0.0, -0.2 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_m (1, 1.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_stereo_middle_simple)
{
  gfloat in[4] = { 0.5, -0.2, 0.25, 0.1 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_s (1, 0.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", in[0], in[1], in[2], in[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == in[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_stereo_left_simple)
{
  gfloat in[4] = { 0.5, -0.2, 0.25, 0.1 };
  gfloat out[4] = { 0.5, 0.0, 0.25, 0.0 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_s (1, -1.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;

GST_START_TEST (test_f32_stereo_right_simple)
{
  gfloat in[4] = { 0.5, -0.2, 0.25, 0.1 };
  gfloat out[4] = { 0.0, -0.2, 0.0, 0.1 };
  gfloat res[4];
  GstElement *panorama = setup_panorama_f32_s (1, 1.0);
  gint i;

  do_panorama (in, sizeof (in), res, sizeof (res));

  GST_INFO ("exp. %+4.2f %+4.2f %+4.2f %+4.2f", out[0], out[1], out[2], out[3]);
  GST_INFO ("real %+4.2f %+4.2f %+4.2f %+4.2f", res[0], res[1], res[2], res[3]);
  for (i = 0; i < 4; i++)
    fail_unless (res[i] == out[i], "difference at pos=%d", i);

  cleanup_panorama (panorama);
}

GST_END_TEST;


static Suite *
panorama_suite (void)
{
  Suite *s = suite_create ("panorama");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_ref_counts);
  tcase_add_test (tc_chain, test_wrong_caps);
  /* processing for method=psy */
  tcase_add_test (tc_chain, test_s16_mono_middle);
  tcase_add_test (tc_chain, test_s16_mono_left);
  tcase_add_test (tc_chain, test_s16_mono_right);
  tcase_add_test (tc_chain, test_s16_stereo_middle);
  tcase_add_test (tc_chain, test_s16_stereo_left);
  tcase_add_test (tc_chain, test_s16_stereo_right);
  tcase_add_test (tc_chain, test_f32_mono_middle);
  tcase_add_test (tc_chain, test_f32_mono_left);
  tcase_add_test (tc_chain, test_f32_mono_right);
  tcase_add_test (tc_chain, test_f32_stereo_middle);
  tcase_add_test (tc_chain, test_f32_stereo_left);
  tcase_add_test (tc_chain, test_f32_stereo_right);
  /* processing for method=simple */
  tcase_add_test (tc_chain, test_s16_mono_middle_simple);
  tcase_add_test (tc_chain, test_s16_mono_left_simple);
  tcase_add_test (tc_chain, test_s16_mono_right_simple);
  tcase_add_test (tc_chain, test_s16_stereo_middle_simple);
  tcase_add_test (tc_chain, test_s16_stereo_left_simple);
  tcase_add_test (tc_chain, test_s16_stereo_right_simple);
  tcase_add_test (tc_chain, test_f32_mono_middle_simple);
  tcase_add_test (tc_chain, test_f32_mono_left_simple);
  tcase_add_test (tc_chain, test_f32_mono_right_simple);
  tcase_add_test (tc_chain, test_f32_stereo_middle_simple);
  tcase_add_test (tc_chain, test_f32_stereo_left_simple);
  tcase_add_test (tc_chain, test_f32_stereo_right_simple);

  return s;
}

GST_CHECK_MAIN (panorama);
