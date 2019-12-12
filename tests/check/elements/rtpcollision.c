/* GStreamer
 *
 * Copyright (C) 2013 Collabora Ltd.
 *   @author Julien Isorce <julien.isorce@collabora.co.uk>
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
#include <gst/net/gstnetaddressmeta.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/rtp/gstrtcpbuffer.h>

static GMainLoop *main_loop;
static GstPad *srcpad;

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

static void
message_received (GstBus * bus, GstMessage * message, GstPipeline * bin)
{
  GST_INFO ("bus message from \"%" GST_PTR_FORMAT "\": %" GST_PTR_FORMAT,
      GST_MESSAGE_SRC (message), message);

  switch (message->type) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (main_loop);
      break;
    case GST_MESSAGE_WARNING:{
      GError *gerror;
      gchar *debug;

      gst_message_parse_warning (message, &gerror, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      g_error_free (gerror);
      g_free (debug);
      break;
    }
    case GST_MESSAGE_ERROR:{
      GError *gerror;
      gchar *debug;

      gst_message_parse_error (message, &gerror, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      g_error_free (gerror);
      g_free (debug);
      g_main_loop_quit (main_loop);
      break;
    }
    default:
      break;
  }
}

static GstBuffer *
create_rtcp_app (guint32 ssrc, guint count)
{
  GInetAddress *inet_addr_0;
  guint16 port = 5678 + count;
  GSocketAddress *socket_addr_0;
  GstBuffer *rtcp_buffer;
  GstRTCPPacket *rtcp_packet = NULL;
  GstRTCPBuffer rtcp = GST_RTCP_BUFFER_INIT;

  inet_addr_0 = g_inet_address_new_from_string ("192.168.1.1");
  socket_addr_0 = g_inet_socket_address_new (inet_addr_0, port);
  g_object_unref (inet_addr_0);

  rtcp_buffer = gst_rtcp_buffer_new (1400);
  gst_buffer_add_net_address_meta (rtcp_buffer, socket_addr_0);
  g_object_unref (socket_addr_0);

  /* need to begin with rr */
  gst_rtcp_buffer_map (rtcp_buffer, GST_MAP_READWRITE, &rtcp);
  rtcp_packet = g_slice_new0 (GstRTCPPacket);
  gst_rtcp_buffer_add_packet (&rtcp, GST_RTCP_TYPE_RR, rtcp_packet);
  gst_rtcp_packet_rr_set_ssrc (rtcp_packet, ssrc);
  g_slice_free (GstRTCPPacket, rtcp_packet);

  /* useful to make the rtcp buffer valid */
  rtcp_packet = g_slice_new0 (GstRTCPPacket);
  gst_rtcp_buffer_add_packet (&rtcp, GST_RTCP_TYPE_APP, rtcp_packet);
  g_slice_free (GstRTCPPacket, rtcp_packet);
  gst_rtcp_buffer_unmap (&rtcp);

  return rtcp_buffer;
}

static guint nb_ssrc_changes;
static guint ssrc_prev;

static GstPadProbeReturn
rtpsession_sinkpad_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  GstPadProbeReturn ret = GST_PAD_PROBE_OK;

  if (info->type == (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_PUSH)) {
    GstBuffer *buffer = GST_BUFFER (info->data);
    GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
    GstBuffer *rtcp_buffer = 0;
    guint ssrc = 0;

    /* retrieve current ssrc */
    gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp);
    ssrc = gst_rtp_buffer_get_ssrc (&rtp);
    gst_rtp_buffer_unmap (&rtp);

    /* if not first buffer, check that our ssrc has changed */
    if (ssrc_prev != -1 && ssrc != ssrc_prev)
      ++nb_ssrc_changes;

    /* update prev ssrc */
    ssrc_prev = ssrc;

    /* feint a collision on recv_rtcp_sink pad of gstrtpsession
     * (note that after being marked as collied the rtpsession ignores
     * all non bye packets)
     */
    rtcp_buffer = create_rtcp_app (ssrc, nb_ssrc_changes);

    /* push collied packet on recv_rtcp_sink */
    gst_pad_push (srcpad, rtcp_buffer);
  }

  return ret;
}

static GstFlowReturn
fake_udp_sink_chain_func (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  gst_buffer_unref (buffer);
  return GST_FLOW_OK;
}

/* This test build the pipeline audiotestsrc ! alawenc ! rtppcmapay ! \
 * rtpsession ! fakesink
 * It manually pushs buffer into rtpsession with same ssrc but different
 * ip so that collision can be detected
 * The test checks that the payloader change their ssrc
 */
GST_START_TEST (test_master_ssrc_collision)
{
  GstElement *bin, *src, *encoder, *rtppayloader, *rtpsession, *sink;
  GstBus *bus = NULL;
  gboolean res = FALSE;
  GstSegment segment;
  GstPad *sinkpad = NULL;
  GstPad *rtcp_sinkpad = NULL;
  GstPad *fake_udp_sinkpad = NULL;
  GstPad *rtcp_srcpad = NULL;
  GstStateChangeReturn state_res = GST_STATE_CHANGE_FAILURE;

  GST_INFO ("preparing test");

  nb_ssrc_changes = 0;
  ssrc_prev = -1;

  /* build pipeline */
  bin = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (bin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  src = gst_element_factory_make ("audiotestsrc", "src");
  g_object_set (src, "num-buffers", 5, NULL);
  encoder = gst_element_factory_make ("alawenc", NULL);
  rtppayloader = gst_element_factory_make ("rtppcmapay", NULL);
  g_object_set (rtppayloader, "pt", 8, NULL);
  rtpsession = gst_element_factory_make ("rtpsession", NULL);
  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (bin), src, encoder, rtppayloader,
      rtpsession, sink, NULL);

  /* link elements */
  res = gst_element_link (src, encoder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (encoder, rtppayloader);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link_pads_full (rtppayloader, "src",
      rtpsession, "send_rtp_sink", GST_PAD_LINK_CHECK_NOTHING);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link_pads_full (rtpsession, "send_rtp_src",
      sink, "sink", GST_PAD_LINK_CHECK_NOTHING);
  fail_unless (res == TRUE, NULL);

  /* add probe on rtpsession sink pad to induce collision */
  sinkpad = gst_element_get_static_pad (rtpsession, "send_rtp_sink");
  gst_pad_add_probe (sinkpad,
      (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_PUSH),
      (GstPadProbeCallback) rtpsession_sinkpad_probe, NULL, NULL);
  gst_object_unref (sinkpad);

  /* setup rtcp link */
  srcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  rtcp_sinkpad = gst_element_get_request_pad (rtpsession, "recv_rtcp_sink");
  fail_unless (gst_pad_link (srcpad, rtcp_sinkpad) == GST_PAD_LINK_OK, NULL);
  gst_object_unref (rtcp_sinkpad);
  res = gst_pad_set_active (srcpad, TRUE);
  fail_if (res == FALSE);
  res =
      gst_pad_push_event (srcpad,
      gst_event_new_stream_start ("my_rtcp_stream_id"));
  fail_if (res == FALSE);
  gst_segment_init (&segment, GST_FORMAT_TIME);
  res = gst_pad_push_event (srcpad, gst_event_new_segment (&segment));
  fail_if (res == FALSE);

  fake_udp_sinkpad = gst_pad_new_from_static_template (&sinktemplate, "sink");
  gst_pad_set_chain_function (fake_udp_sinkpad, fake_udp_sink_chain_func);
  rtcp_srcpad = gst_element_get_request_pad (rtpsession, "send_rtcp_src");
  fail_unless (gst_pad_link (rtcp_srcpad, fake_udp_sinkpad) == GST_PAD_LINK_OK,
      NULL);
  gst_object_unref (rtcp_srcpad);
  res = gst_pad_set_active (fake_udp_sinkpad, TRUE);
  fail_if (res == FALSE);

  /* connect messages */
  main_loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message::error", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::warning", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::eos", (GCallback) message_received, bin);

  state_res = gst_element_set_state (bin, GST_STATE_PLAYING);
  ck_assert_int_ne (state_res, GST_STATE_CHANGE_FAILURE);

  GST_INFO ("running main loop");
  g_main_loop_run (main_loop);

  state_res = gst_element_set_state (bin, GST_STATE_NULL);
  ck_assert_int_ne (state_res, GST_STATE_CHANGE_FAILURE);

  /* cleanup */
  gst_object_unref (srcpad);
  gst_object_unref (fake_udp_sinkpad);
  g_main_loop_unref (main_loop);
  gst_bus_remove_signal_watch (bus);
  gst_object_unref (bus);
  gst_object_unref (bin);

  /* check results */
  fail_unless_equals_int (nb_ssrc_changes, 4);
}

GST_END_TEST;

static guint ssrc_before;
static guint ssrc_after;
static guint rtx_ssrc_before;
static guint rtx_ssrc_after;

static GstPadProbeReturn
rtpsession_sinkpad_probe2 (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  GstPadProbeReturn ret = GST_PAD_PROBE_OK;

  if (info->type == (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_PUSH)) {
    GstBuffer *buffer = GST_BUFFER (info->data);
    GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
    guint payload_type = 0;

    static gint i = 0;

    /* retrieve current ssrc for retransmission stream only */
    gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp);
    payload_type = gst_rtp_buffer_get_payload_type (&rtp);
    if (payload_type == 99) {
      if (i < 3)
        rtx_ssrc_before = gst_rtp_buffer_get_ssrc (&rtp);
      else
        rtx_ssrc_after = gst_rtp_buffer_get_ssrc (&rtp);
    } else {
      /* ask to retransmit every packet */
      GstEvent *event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
          gst_structure_new ("GstRTPRetransmissionRequest",
              "seqnum", G_TYPE_UINT, gst_rtp_buffer_get_seq (&rtp),
              "ssrc", G_TYPE_UINT, gst_rtp_buffer_get_ssrc (&rtp),
              NULL));
      gst_pad_push_event (pad, event);

      if (i < 3)
        ssrc_before = gst_rtp_buffer_get_ssrc (&rtp);
      else
        ssrc_after = gst_rtp_buffer_get_ssrc (&rtp);
    }
    gst_rtp_buffer_unmap (&rtp);

    /* feint a collision on recv_rtcp_sink pad of gstrtpsession
     * (note that after being marked as collied the rtpsession ignores
     * all non bye packets)
     */
    if (i == 2) {
      GstBuffer *rtcp_buffer = create_rtcp_app (rtx_ssrc_before, 0);

      /* push collied packet on recv_rtcp_sink */
      gst_pad_push (srcpad, rtcp_buffer);
    }

    ++i;
  }

  return ret;
}

/* This test build the pipeline audiotestsrc ! alawenc ! rtppcmapay ! \
 * rtprtxsend ! rtpsession ! fakesink
 * It manually pushs buffer into rtpsession with same ssrc than rtx stream
 * but different ip so that collision can be detected
 * The test checks that the rtx elements changes its ssrc whereas
 * the payloader keeps its master ssrc
 */
GST_START_TEST (test_rtx_ssrc_collision)
{
  GstElement *bin, *src, *encoder, *rtppayloader, *rtprtxsend, *rtpsession,
      *sink;
  GstBus *bus = NULL;
  gboolean res = FALSE;
  GstSegment segment;
  GstPad *sinkpad = NULL;
  GstPad *rtcp_sinkpad = NULL;
  GstPad *fake_udp_sinkpad = NULL;
  GstPad *rtcp_srcpad = NULL;
  GstStateChangeReturn state_res = GST_STATE_CHANGE_FAILURE;
  GstStructure *pt_map;

  GST_INFO ("preparing test");

  /* build pipeline */
  bin = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (bin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  src = gst_element_factory_make ("audiotestsrc", "src");
  g_object_set (src, "num-buffers", 5, NULL);
  encoder = gst_element_factory_make ("alawenc", NULL);
  rtppayloader = gst_element_factory_make ("rtppcmapay", NULL);
  g_object_set (rtppayloader, "pt", 8, NULL);
  rtprtxsend = gst_element_factory_make ("rtprtxsend", NULL);
  pt_map = gst_structure_new ("application/x-rtp-pt-map",
      "8", G_TYPE_UINT, 99, NULL);
  g_object_set (rtprtxsend, "payload-type-map", pt_map, NULL);
  gst_structure_free (pt_map);
  rtpsession = gst_element_factory_make ("rtpsession", NULL);
  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (bin), src, encoder, rtppayloader, rtprtxsend,
      rtpsession, sink, NULL);

  /* link elements */
  res = gst_element_link (src, encoder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (encoder, rtppayloader);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (rtppayloader, rtprtxsend);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link_pads_full (rtprtxsend, "src",
      rtpsession, "send_rtp_sink", GST_PAD_LINK_CHECK_NOTHING);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link_pads_full (rtpsession, "send_rtp_src",
      sink, "sink", GST_PAD_LINK_CHECK_NOTHING);
  fail_unless (res == TRUE, NULL);

  /* add probe on rtpsession sink pad to induce collision */
  sinkpad = gst_element_get_static_pad (rtpsession, "send_rtp_sink");
  gst_pad_add_probe (sinkpad,
      (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_PUSH),
      (GstPadProbeCallback) rtpsession_sinkpad_probe2, NULL, NULL);
  gst_object_unref (sinkpad);

  /* setup rtcp link */
  srcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  rtcp_sinkpad = gst_element_get_request_pad (rtpsession, "recv_rtcp_sink");
  fail_unless (gst_pad_link (srcpad, rtcp_sinkpad) == GST_PAD_LINK_OK, NULL);
  gst_object_unref (rtcp_sinkpad);
  res = gst_pad_set_active (srcpad, TRUE);
  fail_if (res == FALSE);
  res =
      gst_pad_push_event (srcpad,
      gst_event_new_stream_start ("my_rtcp_stream_id"));
  fail_if (res == FALSE);
  gst_segment_init (&segment, GST_FORMAT_TIME);
  res = gst_pad_push_event (srcpad, gst_event_new_segment (&segment));
  fail_if (res == FALSE);

  fake_udp_sinkpad = gst_pad_new_from_static_template (&sinktemplate, "sink");
  gst_pad_set_chain_function (fake_udp_sinkpad, fake_udp_sink_chain_func);
  rtcp_srcpad = gst_element_get_request_pad (rtpsession, "send_rtcp_src");
  fail_unless (gst_pad_link (rtcp_srcpad, fake_udp_sinkpad) == GST_PAD_LINK_OK,
      NULL);
  gst_object_unref (rtcp_srcpad);
  res = gst_pad_set_active (fake_udp_sinkpad, TRUE);
  fail_if (res == FALSE);

  /* connect messages */
  main_loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message::error", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::warning", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::eos", (GCallback) message_received, bin);

  state_res = gst_element_set_state (bin, GST_STATE_PLAYING);
  ck_assert_int_ne (state_res, GST_STATE_CHANGE_FAILURE);

  GST_INFO ("running main loop");
  g_main_loop_run (main_loop);

  state_res = gst_element_set_state (bin, GST_STATE_NULL);
  ck_assert_int_ne (state_res, GST_STATE_CHANGE_FAILURE);

  /* cleanup */
  gst_object_unref (srcpad);
  gst_object_unref (fake_udp_sinkpad);
  g_main_loop_unref (main_loop);
  gst_bus_remove_signal_watch (bus);
  gst_object_unref (bus);
  gst_object_unref (bin);

  /* check results */
  fail_if (rtx_ssrc_before == rtx_ssrc_after);
  fail_if (ssrc_before != ssrc_after);
}

GST_END_TEST;

static Suite *
rtpcollision_suite (void)
{
  Suite *s = suite_create ("rtpcollision");
  TCase *tc_chain = tcase_create ("general");

  tcase_set_timeout (tc_chain, 10);

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_master_ssrc_collision);
  tcase_add_test (tc_chain, test_rtx_ssrc_collision);

  return s;
}

GST_CHECK_MAIN (rtpcollision);
