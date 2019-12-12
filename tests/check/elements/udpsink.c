/* GStreamer udpsink unit tests
 * Copyright (C) 2009 Axis Communications <dev-gstreamer@axis.com>
 * @author Ognyan Tonchev <ognyan@axis.com>
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
#include <gst/base/gstbasesink.h>
#include <gio/gio.h>
#include <stdlib.h>

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

#define RTP_HEADER_SIZE 12
#define RTP_PAYLOAD_SIZE 1024

/*
 * Number of bytes received in the render function when using buffer lists
 */
static guint render_list_bytes_received;

/*
 * Render function for testing udpsink with buffer lists
 */
static GstFlowReturn
udpsink_render_list (GstBaseSink * sink, GstBufferList * list)
{
  guint i, num;

  num = gst_buffer_list_length (list);
  for (i = 0; i < num; ++i) {
    GstBuffer *buf = gst_buffer_list_get (list, i);
    gsize size = gst_buffer_get_size (buf);

    GST_DEBUG ("rendered %" G_GSIZE_FORMAT " bytes", size);
    render_list_bytes_received += size;
  }

  return GST_FLOW_OK;
}

static void
set_render_list_function (GstElement * bsink)
{
  GstBaseSinkClass *bsclass;

  bsclass = GST_BASE_SINK_GET_CLASS ((GstBaseSink *) bsink);

  /* Add callback function for the buffer list tests */
  bsclass->render_list = udpsink_render_list;
}

static GstBufferList *
create_buffer_list (guint * data_size)
{
  GstBufferList *list;
  GstBuffer *rtp_buffer;
  GstBuffer *data_buffer;

  list = gst_buffer_list_new ();

  /*** First group, i.e. first packet. **/

  /* Create the RTP header buffer */
  rtp_buffer = gst_buffer_new_allocate (NULL, RTP_HEADER_SIZE, NULL);
  gst_buffer_memset (rtp_buffer, 0, 0, RTP_HEADER_SIZE);

  /* Create the buffer that holds the payload */
  data_buffer = gst_buffer_new_allocate (NULL, RTP_PAYLOAD_SIZE, NULL);
  gst_buffer_memset (data_buffer, 0, 0, RTP_PAYLOAD_SIZE);

  /* Create a new group to hold the rtp header and the payload */
  gst_buffer_list_add (list, gst_buffer_append (rtp_buffer, data_buffer));

  /***  Second group, i.e. second packet. ***/

  /* Create the RTP header buffer */
  rtp_buffer = gst_buffer_new_allocate (NULL, RTP_HEADER_SIZE, NULL);
  gst_buffer_memset (rtp_buffer, 0, 0, RTP_HEADER_SIZE);

  /* Create the buffer that holds the payload */
  data_buffer = gst_buffer_new_allocate (NULL, RTP_PAYLOAD_SIZE, NULL);
  gst_buffer_memset (data_buffer, 0, 0, RTP_PAYLOAD_SIZE);

  /* Create a new group to hold the rtp header and the payload */
  gst_buffer_list_add (list, gst_buffer_append (rtp_buffer, data_buffer));

  /* Calculate the size of the data */
  *data_size = 2 * RTP_HEADER_SIZE + 2 * RTP_PAYLOAD_SIZE;

  return list;
}

static void
udpsink_test (gboolean use_buffer_lists)
{
  GstSegment segment;
  GstElement *udpsink;
  GstPad *srcpad;
  GstBufferList *list;
  guint data_size;

  list = create_buffer_list (&data_size);

  udpsink = gst_check_setup_element ("udpsink");
  if (use_buffer_lists)
    set_render_list_function (udpsink);

  srcpad = gst_check_setup_src_pad_by_name (udpsink, &srctemplate, "sink");

  gst_element_set_state (udpsink, GST_STATE_PLAYING);
  gst_pad_set_active (srcpad, TRUE);

  gst_pad_push_event (srcpad, gst_event_new_stream_start ("hey there!"));

  gst_segment_init (&segment, GST_FORMAT_TIME);
  gst_pad_push_event (srcpad, gst_event_new_segment (&segment));

  fail_unless_equals_int (gst_pad_push_list (srcpad, list), GST_FLOW_OK);

  gst_check_teardown_pad_by_name (udpsink, "sink");
  gst_check_teardown_element (udpsink);

  if (use_buffer_lists)
    fail_unless_equals_int (data_size, render_list_bytes_received);
}

GST_START_TEST (test_udpsink)
{
  udpsink_test (FALSE);
}

GST_END_TEST;


GST_START_TEST (test_udpsink_bufferlist)
{
  udpsink_test (TRUE);
}

GST_END_TEST;

GST_START_TEST (test_udpsink_client_add_remove)
{
  GstElement *udpsink;

  /* Note: keep in mind that these are in addition to the client added by
   * the host/port properties (by default 'localhost:5004' */

  udpsink = gst_check_setup_element ("udpsink");
  g_signal_emit_by_name (udpsink, "remove", "localhost", 5004, NULL);
  gst_object_unref (udpsink);

  udpsink = gst_check_setup_element ("udpsink");
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5554, NULL);
  gst_object_unref (udpsink);

  udpsink = gst_check_setup_element ("udpsink");
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5554, NULL);
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5554, NULL);
  gst_object_unref (udpsink);

  udpsink = gst_check_setup_element ("udpsink");
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5554, NULL);
  g_signal_emit_by_name (udpsink, "remove", "127.0.0.1", 5554, NULL);
  gst_object_unref (udpsink);

  udpsink = gst_check_setup_element ("udpsink");
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5554, NULL);
  g_signal_emit_by_name (udpsink, "remove", "127.0.0.1", 5555, NULL);
  gst_object_unref (udpsink);

  udpsink = gst_check_setup_element ("udpsink");
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5554, NULL);
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5555, NULL);
  gst_object_unref (udpsink);

  udpsink = gst_check_setup_element ("udpsink");
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5554, NULL);
  g_signal_emit_by_name (udpsink, "add", "10.2.0.1", 5554, NULL);
  gst_object_unref (udpsink);

  udpsink = gst_check_setup_element ("udpsink");
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5554, NULL);
  g_signal_emit_by_name (udpsink, "add", "10.2.0.1", 5554, NULL);
  g_signal_emit_by_name (udpsink, "remove", "127.0.0.1", 5554, NULL);
  gst_object_unref (udpsink);
}

GST_END_TEST;

GST_START_TEST (test_udpsink_dscp)
{
  GstElement *udpsink;
  GError *error = NULL;
  GSocket *sock4, *sock6;

  sock4 =
      g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM,
      G_SOCKET_PROTOCOL_UDP, &error);
  fail_unless (sock4 != NULL && error == NULL);
  sock6 =
      g_socket_new (G_SOCKET_FAMILY_IPV6, G_SOCKET_TYPE_DATAGRAM,
      G_SOCKET_PROTOCOL_UDP, &error);
  fail_unless (sock6 != NULL && error == NULL);

  udpsink = gst_check_setup_element ("udpsink");
  g_signal_emit_by_name (udpsink, "add", "127.0.0.1", 5554, NULL);
  g_object_set (udpsink, "socket", sock4, NULL);
  g_object_set (udpsink, "socket-v6", sock6, NULL);

  ASSERT_SET_STATE (udpsink, GST_STATE_READY, GST_STATE_CHANGE_SUCCESS);

  g_object_set (udpsink, "qos-dscp", 0, NULL);
  g_object_set (udpsink, "qos-dscp", 63, NULL);

  ASSERT_SET_STATE (udpsink, GST_STATE_NULL, GST_STATE_CHANGE_SUCCESS);

  gst_object_unref (udpsink);
  g_object_unref (sock4);
  g_object_unref (sock6);
}

GST_END_TEST;

static Suite *
udpsink_suite (void)
{
  Suite *s = suite_create ("udpsink_test");
  TCase *tc_chain = tcase_create ("linear");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_udpsink);
  tcase_add_test (tc_chain, test_udpsink_bufferlist);
  tcase_add_test (tc_chain, test_udpsink_client_add_remove);
  tcase_add_test (tc_chain, test_udpsink_dscp);

  return s;
}

GST_CHECK_MAIN (udpsink)
