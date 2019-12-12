/* GStreamer
 *
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
#include <string.h>

typedef struct
{
  GMainLoop *loop;
  gboolean eos;
} OnMessageUserData;

static void
on_message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
  OnMessageUserData *d = user_data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING:
      g_assert_not_reached ();
      break;
    case GST_MESSAGE_EOS:
      g_main_loop_quit (d->loop);
      d->eos = TRUE;
      break;
    default:
      break;
  }
}

static void
run_test (const gchar * pipeline_string)
{
  GstElement *pipeline;
  GstBus *bus;
  GMainLoop *loop;
  OnMessageUserData omud = { NULL, };
  GstStateChangeReturn ret;

  GST_DEBUG ("Testing pipeline '%s'", pipeline_string);

  pipeline = gst_parse_launch (pipeline_string, NULL);
  fail_unless (pipeline != NULL);
  g_object_set (G_OBJECT (pipeline), "async-handling", TRUE, NULL);

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  gst_bus_add_signal_watch (bus);

  omud.loop = loop;
  omud.eos = FALSE;

  g_signal_connect (bus, "message", (GCallback) on_message_cb, &omud);


  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  fail_unless (ret == GST_STATE_CHANGE_SUCCESS
      || ret == GST_STATE_CHANGE_ASYNC);

  g_main_loop_run (loop);

  fail_unless (gst_element_set_state (pipeline,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  fail_unless (omud.eos == TRUE);

  gst_bus_remove_signal_watch (bus);
  gst_object_unref (bus);
  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
}

#define CREATE_TEST(element) \
GST_START_TEST (test_##element) \
{ \
  gchar *pipeline; \
  \
  pipeline = g_strdup_printf ("videotestsrc num-buffers=100 ! " \
      "videoconvert ! " \
      " %s ! " \
      " fakesink", #element); \
  \
  run_test (pipeline); \
  g_free (pipeline); \
} \
\
GST_END_TEST;

CREATE_TEST (agingtv);
CREATE_TEST (dicetv);
CREATE_TEST (edgetv);
CREATE_TEST (optv);
CREATE_TEST (quarktv);
CREATE_TEST (radioactv);
CREATE_TEST (revtv);
CREATE_TEST (rippletv);
CREATE_TEST (shagadelictv);
CREATE_TEST (streaktv);
CREATE_TEST (vertigotv);
CREATE_TEST (warptv);

static Suite *
effectv_suite (void)
{
  Suite *s = suite_create ("effectv");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_set_timeout (tc_chain, 180);

  tcase_add_test (tc_chain, test_agingtv);
  tcase_add_test (tc_chain, test_dicetv);
  tcase_add_test (tc_chain, test_edgetv);
  tcase_add_test (tc_chain, test_optv);
  tcase_add_test (tc_chain, test_quarktv);
  tcase_add_test (tc_chain, test_radioactv);
  tcase_add_test (tc_chain, test_revtv);
  tcase_add_test (tc_chain, test_rippletv);
  tcase_add_test (tc_chain, test_shagadelictv);
  tcase_add_test (tc_chain, test_streaktv);
  tcase_add_test (tc_chain, test_vertigotv);
  tcase_add_test (tc_chain, test_warptv);

  return s;
}

GST_CHECK_MAIN (effectv);
