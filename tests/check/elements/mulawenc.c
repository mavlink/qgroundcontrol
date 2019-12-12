/* GStreamer MulawEnc unit tests
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
#include "config.h"
#endif

#include <gst/check/gstcheck.h>
#include <gst/audio/audio.h>
#include <string.h>

static GstPad *mysrcpad, *mysinkpad;
static GstElement *mulawenc = NULL;

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-mulaw," "rate = (int) 8000," "channels = (int) 1")
    );

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw,"
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "rate = (int) 8000, "
        "channels = (int) 1, " "layout = (string)interleaved")
    );

static void
mulawenc_setup (void)
{
  GstCaps *src_caps;

  src_caps = gst_caps_from_string ("audio/x-raw,"
      "format = (string) " GST_AUDIO_NE (S16) ", "
      "rate = (int) 8000, "
      "channels = (int) 1, " "layout = (string)interleaved");

  GST_DEBUG ("%s", __FUNCTION__);

  mulawenc = gst_check_setup_element ("mulawenc");

  mysrcpad = gst_check_setup_src_pad (mulawenc, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (mulawenc, &sinktemplate);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  gst_check_setup_events (mysrcpad, mulawenc, src_caps, GST_FORMAT_TIME);
  gst_caps_unref (src_caps);
}

static void
buffer_unref (void *buffer, void *user_data)
{
  gst_buffer_unref (GST_BUFFER (buffer));
}

static void
mulawenc_teardown (void)
{
  /* free encoded buffers */
  g_list_foreach (buffers, buffer_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (mulawenc);
  gst_check_teardown_sink_pad (mulawenc);
  gst_check_teardown_element (mulawenc);
  mulawenc = NULL;
}

static gboolean
check_for_maximum_bitrate (GstPad * pad, GstEvent ** eventp, gpointer user_data)
{
  gboolean *found_maximum_bitrate = (gboolean *) user_data;
  GstEvent *event = *eventp;

  if (event->type == GST_EVENT_TAG) {
    GstTagList *taglist = NULL;
    guint value = 0;
    gst_event_parse_tag (event, &taglist);

    fail_unless (taglist != NULL);

    fail_unless (gst_tag_list_get_uint (taglist, GST_TAG_MAXIMUM_BITRATE,
            &value));

    /* bitrate needs to be exactly sample rate * channels * 8 */
    fail_unless (value == 8000 * 1 * 8);

    *found_maximum_bitrate = TRUE;
  }

  return TRUE;
}

GST_START_TEST (test_one_buffer)
{
  GstBuffer *buffer;
  gint buf_size = 4096;
  guint8 *dp;

  fail_unless (gst_element_set_state (mulawenc, GST_STATE_PLAYING) ==
      GST_STATE_CHANGE_SUCCESS, "could not change state to playing");

  buffer = gst_buffer_new ();
  dp = g_malloc0 (buf_size);
  gst_buffer_append_memory (buffer,
      gst_memory_new_wrapped (0, dp, buf_size, 0, buf_size, dp, g_free));
  ASSERT_BUFFER_REFCOUNT (buffer, "buffer", 1);

  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

  fail_unless (g_list_length (buffers) == 1);
  fail_unless (gst_buffer_get_size (GST_BUFFER (g_list_first (buffers)->data)));
}

GST_END_TEST;

GST_START_TEST (test_tags)
{
  GstBuffer *buffer;
  gint buf_size = 4096;
  guint8 *dp;
  gboolean found_maximum_bitrate = FALSE;

  fail_unless (gst_element_set_state (mulawenc, GST_STATE_PLAYING) ==
      GST_STATE_CHANGE_SUCCESS, "could not change state to playing");

  buffer = gst_buffer_new ();
  dp = g_malloc0 (buf_size);
  gst_buffer_append_memory (buffer,
      gst_memory_new_wrapped (0, dp, buf_size, 0, buf_size, dp, g_free));
  ASSERT_BUFFER_REFCOUNT (buffer, "buffer", 1);

  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);
  gst_pad_sticky_events_foreach (mysinkpad, check_for_maximum_bitrate,
      &found_maximum_bitrate);
  fail_unless (found_maximum_bitrate);
}

GST_END_TEST;

static Suite *
mulawenc_suite (void)
{
  Suite *s = suite_create ("mulawenc");
  TCase *tc_chain = tcase_create ("mulawenc");

  tcase_add_checked_fixture (tc_chain, mulawenc_setup, mulawenc_teardown);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_one_buffer);
  tcase_add_test (tc_chain, test_tags);
  return s;
}

GST_CHECK_MAIN (mulawenc)
