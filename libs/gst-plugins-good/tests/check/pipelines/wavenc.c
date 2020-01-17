/* GStreamer
 *
 * unit test for wavenc
 *
 * Copyright (C) <2010> Stefan Kost <ensonic@users.sf.net>
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

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <gst/check/gstcheck.h>
#include <gst/audio/audio.h>
#include <gst/audio/audio-enumtypes.h>

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

      gst_message_parse_error (message, &gerror, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      gst_message_unref (message);
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

/*
 * gst-launch-1.0 \
 * audiotestsrc freq=440 num-buffers=100 ! interleave name=i ! audioconvert ! wavenc ! filesink location=/tmp/mc.wav \
 * audiotestsrc freq=880 num-buffers=100 ! i.
 * ...
 *
 * http://www.microsoft.com/whdc/device/audio/multichaud.mspx
 */

static void
make_n_channel_wav (const gint channels, const GValueArray * arr)
{
  GstElement *pipeline;
  GstElement **audiotestsrc, *interleave, *wavenc, *conv, *fakesink;
  GstBus *bus;
  GMainLoop *loop;
  guint i;
  guint bus_watch = 0;

  audiotestsrc = g_new0 (GstElement *, channels);

  pipeline = gst_pipeline_new ("pipeline");
  fail_unless (pipeline != NULL);

  interleave = gst_element_factory_make ("interleave", NULL);
  fail_unless (interleave != NULL);
  g_object_set (interleave, "channel-positions", arr, NULL);
  gst_bin_add (GST_BIN (pipeline), interleave);

  if (G_BYTE_ORDER == G_BIG_ENDIAN) {
    /* we're not here to test orc; audioconvert misbehaves on ppc32 */
    g_setenv ("ORC_CODE", "backup", 1);
    conv = gst_element_factory_make ("audioconvert", NULL);
  } else {
    conv = gst_element_factory_make ("identity", NULL);
  }
  fail_unless (conv != NULL);
  gst_bin_add (GST_BIN (pipeline), conv);
  fail_unless (gst_element_link (interleave, conv));

  wavenc = gst_element_factory_make ("wavenc", NULL);
  fail_unless (wavenc != NULL);
  gst_bin_add (GST_BIN (pipeline), wavenc);
  fail_unless (gst_element_link (conv, wavenc));

  fakesink = gst_element_factory_make ("fakesink", NULL);
  fail_unless (fakesink != NULL);
  gst_bin_add (GST_BIN (pipeline), fakesink);
  fail_unless (gst_element_link (wavenc, fakesink));

  for (i = 0; i < channels; i++) {
    audiotestsrc[i] = gst_element_factory_make ("audiotestsrc", NULL);
    fail_unless (audiotestsrc[i] != NULL);
    g_object_set (G_OBJECT (audiotestsrc[i]), "wave", 0, "freq",
        440.0 * (i + 1), "num-buffers", 100, NULL);
    gst_bin_add (GST_BIN (pipeline), audiotestsrc[i]);
    fail_unless (gst_element_link (audiotestsrc[i], interleave));
  }

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);
  g_free (audiotestsrc);

  g_main_loop_unref (loop);
  g_source_remove (bus_watch);
}

GST_START_TEST (test_encode_stereo)
{
  GValueArray *arr;
  GValue val = { 0, };

  arr = g_value_array_new (2);
  g_value_init (&val, GST_TYPE_AUDIO_CHANNEL_POSITION);
  g_value_set_enum (&val, GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT);
  g_value_array_append (arr, &val);
  g_value_reset (&val);
  g_value_set_enum (&val, GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT);
  g_value_array_append (arr, &val);
  g_value_unset (&val);

  make_n_channel_wav (2, arr);
  g_value_array_free (arr);
}

GST_END_TEST;

#if 0
GST_START_TEST (test_encode_multichannel)
{
  GValueArray *arr;
  GValue val = { 0, };

  arr = g_value_array_new (6);
  g_value_init (&val, GST_TYPE_AUDIO_CHANNEL_POSITION);
  g_value_set_enum (&val, GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT);
  g_value_array_append (arr, &val);
  g_value_reset (&val);
  g_value_set_enum (&val, GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT);
  g_value_array_append (arr, &val);
  g_value_reset (&val);
  g_value_set_enum (&val, GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER);
  g_value_array_append (arr, &val);
  g_value_reset (&val);
  g_value_set_enum (&val, GST_AUDIO_CHANNEL_POSITION_LFE);
  g_value_array_append (arr, &val);
  g_value_reset (&val);
  g_value_set_enum (&val, GST_AUDIO_CHANNEL_POSITION_REAR_LEFT);
  g_value_array_append (arr, &val);
  g_value_reset (&val);
  g_value_set_enum (&val, GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT);
  g_value_array_append (arr, &val);
  g_value_unset (&val);

  make_n_channel_wav (6);
}

GST_END_TEST;
#endif

static Suite *
wavenc_suite (void)
{
  Suite *s = suite_create ("wavenc");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_encode_stereo);
  /* FIXME: improve wavenc
     tcase_add_test (tc_chain, test_encode_multichannel);
   */

  return s;
}

GST_CHECK_MAIN (wavenc);
