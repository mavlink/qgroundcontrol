/* GStreamer
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

#include <gst/check/gstcheck.h>
#include <gst/base/gstadapter.h>

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
    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure *s = gst_message_get_structure (message);

      const gchar *name = gst_structure_get_name (s);

      fail_unless (strcmp (name, "imperfect-timestamp") != 0);
      fail_unless (strcmp (name, "imperfect-offset") != 0);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static gboolean had_first_buffer = FALSE;

static void
identity_handoff (GstElement * object, GstBuffer * buffer, gpointer user_data)
{
  GstAdapter *adapter = GST_ADAPTER (user_data);

  gst_adapter_push (adapter, gst_buffer_ref (buffer));
}

static void
fakesink_handoff (GstElement * object, GstBuffer * buffer, GstPad * pad,
    gpointer user_data)
{
  GstAdapter *adapter = GST_ADAPTER (user_data);

  /* Don't allow the second buffer with offset=0 as it's the decoded
   * rewrite of the first
   */
  if (had_first_buffer == FALSE && GST_BUFFER_OFFSET (buffer) == 0)
    had_first_buffer = TRUE;
  else if (GST_BUFFER_OFFSET (buffer) == 0)
    return;

  gst_adapter_push (adapter, gst_buffer_ref (buffer));
}

GST_START_TEST (test_encode_decode)
{
  GstElement *pipeline;
  GstElement *audiotestsrc, *identity1, *wavpackenc, *identity2, *wavpackdec,
      *identity3, *fakesink;
  GstAdapter *srcadapter, *sinkadapter;
  GstBus *bus;
  GMainLoop *loop;
  GstBuffer *in, *out;
  guint bus_watch = 0;
  GstMapInfo map;

  srcadapter = gst_adapter_new ();
  fail_unless (srcadapter != NULL);
  sinkadapter = gst_adapter_new ();
  fail_unless (sinkadapter != NULL);

  pipeline = gst_pipeline_new ("pipeline");
  fail_unless (pipeline != NULL);

  audiotestsrc = gst_element_factory_make ("audiotestsrc", "src");
  fail_unless (audiotestsrc != NULL);
  g_object_set (G_OBJECT (audiotestsrc), "wave", 0, "freq", 440.0,
      "num-buffers", 200, NULL);

  identity1 = gst_element_factory_make ("identity", "identity1");
  fail_unless (identity1 != NULL);
  g_object_set (G_OBJECT (identity1), "signal-handoffs", TRUE, NULL);
  g_signal_connect (G_OBJECT (identity1), "handoff",
      G_CALLBACK (identity_handoff), srcadapter);

  wavpackenc = gst_element_factory_make ("wavpackenc", "enc");
  fail_unless (wavpackenc != NULL);

  identity2 = gst_element_factory_make ("identity", "identity2");
  fail_unless (identity2 != NULL);
  g_object_set (G_OBJECT (identity2), "check-imperfect-timestamp", TRUE,
      "check-imperfect-offset", TRUE, NULL);

  wavpackdec = gst_element_factory_make ("wavpackdec", "dec");
  fail_unless (wavpackdec != NULL);

  identity3 = gst_element_factory_make ("identity", "identity3");
  fail_unless (identity3 != NULL);
  g_object_set (G_OBJECT (identity3), "check-imperfect-timestamp", TRUE,
      "check-imperfect-offset", TRUE, NULL);

  fakesink = gst_element_factory_make ("fakesink", "sink");
  fail_unless (fakesink != NULL);
  g_object_set (G_OBJECT (fakesink), "signal-handoffs", TRUE, NULL);
  g_signal_connect (G_OBJECT (fakesink), "handoff",
      G_CALLBACK (fakesink_handoff), sinkadapter);

  gst_bin_add_many (GST_BIN (pipeline), audiotestsrc, identity1, wavpackenc,
      identity2, wavpackdec, identity3, fakesink, NULL);

  fail_unless (gst_element_link_many (audiotestsrc, identity1, wavpackenc,
          identity2, wavpackdec, identity3, fakesink, NULL));

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  bus_watch = gst_bus_add_watch (bus, bus_handler, loop);
  gst_object_unref (bus);

  had_first_buffer = FALSE;
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);
  gst_element_set_state (pipeline, GST_STATE_NULL);

  fail_unless (had_first_buffer == TRUE);
  fail_unless (gst_adapter_available (srcadapter) > 0);
  fail_unless (gst_adapter_available (sinkadapter) > 0);
  fail_unless_equals_int (gst_adapter_available (srcadapter),
      gst_adapter_available (sinkadapter));

  in = gst_adapter_take_buffer (srcadapter, gst_adapter_available (srcadapter));
  fail_unless (in != NULL);
  out =
      gst_adapter_take_buffer (sinkadapter,
      gst_adapter_available (sinkadapter));
  fail_unless (out != NULL);

  fail_unless_equals_int (gst_buffer_get_size (in), gst_buffer_get_size (out));
  gst_buffer_map (out, &map, GST_MAP_READ);
  fail_unless (gst_buffer_memcmp (in, 0, map.data, map.size) == 0);
  gst_buffer_unmap (out, &map);

  gst_buffer_unref (in);
  gst_buffer_unref (out);
  g_object_unref (pipeline);
  g_main_loop_unref (loop);
  g_object_unref (srcadapter);
  g_object_unref (sinkadapter);
  g_source_remove (bus_watch);
}

GST_END_TEST;

static Suite *
wavpack_suite (void)
{
  Suite *s = suite_create ("Wavpack");
  TCase *tc_chain = tcase_create ("linear");

  /* time out after 60s, not the default 3 */
  tcase_set_timeout (tc_chain, 60);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_encode_decode);

  return s;
}

GST_CHECK_MAIN (wavpack);
