/* GStreamer
 *
 * unit test for gstrtpbin
 *
 * Copyright (C) <2009> Wim Taymans <wim.taymans@gmail.com>
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
#include <gst/check/gsttestclock.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/rtp/gstrtcpbuffer.h>

GST_START_TEST (test_pads)
{
  GstElement *element;
  GstPad *pad;

  element = gst_element_factory_make ("rtpsession", NULL);

  pad = gst_element_get_request_pad (element, "recv_rtcp_sink");
  gst_object_unref (pad);
  gst_object_unref (element);
}

GST_END_TEST;

GST_START_TEST (test_cleanup_send)
{
  GstElement *rtpbin;
  GstPad *rtp_sink, *rtp_src, *rtcp_src;
  GObject *session;
  gint count = 2;

  rtpbin = gst_element_factory_make ("rtpbin", "rtpbin");

  while (count--) {
    /* request session 0 */
    rtp_sink = gst_element_get_request_pad (rtpbin, "send_rtp_sink_0");
    fail_unless (rtp_sink != NULL);
    ASSERT_OBJECT_REFCOUNT (rtp_sink, "rtp_sink", 2);

    /* this static pad should be created automatically now */
    rtp_src = gst_element_get_static_pad (rtpbin, "send_rtp_src_0");
    fail_unless (rtp_src != NULL);
    ASSERT_OBJECT_REFCOUNT (rtp_src, "rtp_src", 2);

    /* we should be able to get an internal session 0 now */
    g_signal_emit_by_name (rtpbin, "get-internal-session", 0, &session);
    fail_unless (session != NULL);
    g_object_unref (session);

    /* get the send RTCP pad too */
    rtcp_src = gst_element_get_request_pad (rtpbin, "send_rtcp_src_0");
    fail_unless (rtcp_src != NULL);
    ASSERT_OBJECT_REFCOUNT (rtcp_src, "rtcp_src", 2);

    gst_element_release_request_pad (rtpbin, rtp_sink);
    /* we should only have our refs to the pads now */
    ASSERT_OBJECT_REFCOUNT (rtp_sink, "rtp_sink", 1);
    ASSERT_OBJECT_REFCOUNT (rtp_src, "rtp_src", 1);
    ASSERT_OBJECT_REFCOUNT (rtcp_src, "rtp_src", 2);

    /* the other pad should be gone now */
    fail_unless (gst_element_get_static_pad (rtpbin, "send_rtp_src_0") == NULL);

    /* internal session should still be there */
    g_signal_emit_by_name (rtpbin, "get-internal-session", 0, &session);
    fail_unless (session != NULL);
    g_object_unref (session);

    /* release the RTCP pad */
    gst_element_release_request_pad (rtpbin, rtcp_src);
    /* we should only have our refs to the pads now */
    ASSERT_OBJECT_REFCOUNT (rtp_sink, "rtp_sink", 1);
    ASSERT_OBJECT_REFCOUNT (rtp_src, "rtp_src", 1);
    ASSERT_OBJECT_REFCOUNT (rtcp_src, "rtp_src", 1);

    /* the session should be gone now */
    g_signal_emit_by_name (rtpbin, "get-internal-session", 0, &session);
    fail_unless (session == NULL);

    /* unref the request pad and the static pad */
    gst_object_unref (rtp_sink);
    gst_object_unref (rtp_src);
    gst_object_unref (rtcp_src);
  }

  gst_object_unref (rtpbin);
}

GST_END_TEST;

typedef struct
{
  guint16 seqnum;
  gboolean pad_added;
  GstPad *pad;
  GMutex lock;
  GCond cond;
  GstPad *sinkpad;
  GList *pads;
  GstCaps *caps;
} CleanupData;

static void
init_data (CleanupData * data)
{
  data->seqnum = 10;
  data->pad_added = FALSE;
  g_mutex_init (&data->lock);
  g_cond_init (&data->cond);
  data->pads = NULL;
  data->caps = NULL;
}

static void
clean_data (CleanupData * data)
{
  g_list_foreach (data->pads, (GFunc) gst_object_unref, NULL);
  g_list_free (data->pads);
  g_mutex_clear (&data->lock);
  g_cond_clear (&data->cond);
  if (data->caps)
    gst_caps_unref (data->caps);
}

static guint8 rtp_packet[] = { 0x80, 0x60, 0x94, 0xbc, 0x8f, 0x37, 0x4e, 0xb8,
  0x44, 0xa8, 0xf3, 0x7c, 0x06, 0x6a, 0x0c, 0xce,
  0x13, 0x25, 0x19, 0x69, 0x1f, 0x93, 0x25, 0x9d,
  0x2b, 0x82, 0x31, 0x3b, 0x36, 0xc1, 0x3c, 0x13
};

static GstFlowReturn
chain_rtp_packet (GstPad * pad, CleanupData * data)
{
  GstFlowReturn res;
  GstSegment segment;
  GstBuffer *buffer;
  GstMapInfo map;

  if (data->caps == NULL) {
    data->caps = gst_caps_from_string ("application/x-rtp,"
        "media=(string)audio, clock-rate=(int)44100, "
        "encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1");
    data->seqnum = 0;
  }

  gst_pad_send_event (pad, gst_event_new_stream_start (GST_OBJECT_NAME (pad)));
  gst_pad_send_event (pad, gst_event_new_caps (data->caps));
  gst_segment_init (&segment, GST_FORMAT_TIME);
  gst_pad_send_event (pad, gst_event_new_segment (&segment));

  buffer = gst_buffer_new_and_alloc (sizeof (rtp_packet));
  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  memcpy (map.data, rtp_packet, sizeof (rtp_packet));

  map.data[2] = (data->seqnum >> 8) & 0xff;
  map.data[3] = data->seqnum & 0xff;

  data->seqnum++;
  gst_buffer_unmap (buffer, &map);

  GST_BUFFER_DTS (buffer) = 0;

  res = gst_pad_chain (pad, buffer);

  return res;
}

static GstFlowReturn
dummy_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  gst_buffer_unref (buffer);

  return GST_FLOW_OK;
}

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp"));


static GstPad *
make_sinkpad (CleanupData * data)
{
  GstPad *pad;

  pad = gst_pad_new_from_static_template (&sink_factory, "sink");

  gst_pad_set_chain_function (pad, dummy_chain);
  gst_pad_set_active (pad, TRUE);

  data->pads = g_list_prepend (data->pads, pad);

  return pad;
}

static void
pad_added_cb (GstElement * rtpbin, GstPad * pad, CleanupData * data)
{
  GstPad *sinkpad;

  GST_DEBUG ("pad added %s:%s\n", GST_DEBUG_PAD_NAME (pad));

  if (GST_PAD_IS_SINK (pad))
    return;

  fail_unless (data->pad_added == FALSE);

  sinkpad = make_sinkpad (data);
  fail_unless (gst_pad_link (pad, sinkpad) == GST_PAD_LINK_OK);

  g_mutex_lock (&data->lock);
  data->pad_added = TRUE;
  data->pad = pad;
  g_cond_signal (&data->cond);
  g_mutex_unlock (&data->lock);
}

static void
pad_removed_cb (GstElement * rtpbin, GstPad * pad, CleanupData * data)
{
  GST_DEBUG ("pad removed %s:%s\n", GST_DEBUG_PAD_NAME (pad));

  if (data->pad != pad)
    return;

  fail_unless (data->pad_added == TRUE);

  g_mutex_lock (&data->lock);
  data->pad_added = FALSE;
  g_cond_signal (&data->cond);
  g_mutex_unlock (&data->lock);
}

GST_START_TEST (test_cleanup_recv)
{
  GstElement *rtpbin;
  GstPad *rtp_sink;
  CleanupData data;
  GstStateChangeReturn ret;
  GstFlowReturn res;
  gint count = 2;

  init_data (&data);

  rtpbin = gst_element_factory_make ("rtpbin", "rtpbin");

  g_signal_connect (rtpbin, "pad-added", (GCallback) pad_added_cb, &data);
  g_signal_connect (rtpbin, "pad-removed", (GCallback) pad_removed_cb, &data);

  ret = gst_element_set_state (rtpbin, GST_STATE_PLAYING);
  fail_unless (ret == GST_STATE_CHANGE_SUCCESS);

  while (count--) {
    /* request session 0 */
    rtp_sink = gst_element_get_request_pad (rtpbin, "recv_rtp_sink_0");
    fail_unless (rtp_sink != NULL);
    ASSERT_OBJECT_REFCOUNT (rtp_sink, "rtp_sink", 2);

    /* no sourcepads are created yet */
    fail_unless (rtpbin->numsinkpads == 1);
    fail_unless (rtpbin->numsrcpads == 0);

    res = chain_rtp_packet (rtp_sink, &data);
    GST_DEBUG ("res %d, %s\n", res, gst_flow_get_name (res));
    fail_unless (res == GST_FLOW_OK);

    res = chain_rtp_packet (rtp_sink, &data);
    GST_DEBUG ("res %d, %s\n", res, gst_flow_get_name (res));
    fail_unless (res == GST_FLOW_OK);

    /* we wait for the new pad to appear now */
    g_mutex_lock (&data.lock);
    while (!data.pad_added)
      g_cond_wait (&data.cond, &data.lock);
    g_mutex_unlock (&data.lock);

    /* sourcepad created now */
    fail_unless (rtpbin->numsinkpads == 1);
    fail_unless (rtpbin->numsrcpads == 1);

    /* remove the session */
    gst_element_release_request_pad (rtpbin, rtp_sink);
    gst_object_unref (rtp_sink);

    /* pad should be gone now */
    g_mutex_lock (&data.lock);
    while (data.pad_added)
      g_cond_wait (&data.cond, &data.lock);
    g_mutex_unlock (&data.lock);

    /* nothing left anymore now */
    fail_unless (rtpbin->numsinkpads == 0);
    fail_unless (rtpbin->numsrcpads == 0);
  }

  ret = gst_element_set_state (rtpbin, GST_STATE_NULL);
  fail_unless (ret == GST_STATE_CHANGE_SUCCESS);

  gst_object_unref (rtpbin);

  clean_data (&data);
}

GST_END_TEST;

GST_START_TEST (test_cleanup_recv2)
{
  GstElement *rtpbin;
  GstPad *rtp_sink;
  CleanupData data;
  GstStateChangeReturn ret;
  GstFlowReturn res;
  gint count = 2;

  init_data (&data);

  rtpbin = gst_element_factory_make ("rtpbin", "rtpbin");

  g_signal_connect (rtpbin, "pad-added", (GCallback) pad_added_cb, &data);
  g_signal_connect (rtpbin, "pad-removed", (GCallback) pad_removed_cb, &data);

  ret = gst_element_set_state (rtpbin, GST_STATE_PLAYING);
  fail_unless (ret == GST_STATE_CHANGE_SUCCESS);

  /* request session 0 */
  rtp_sink = gst_element_get_request_pad (rtpbin, "recv_rtp_sink_0");
  fail_unless (rtp_sink != NULL);
  ASSERT_OBJECT_REFCOUNT (rtp_sink, "rtp_sink", 2);

  while (count--) {
    /* no sourcepads are created yet */
    fail_unless (rtpbin->numsinkpads == 1);
    fail_unless (rtpbin->numsrcpads == 0);

    res = chain_rtp_packet (rtp_sink, &data);
    GST_DEBUG ("res %d, %s\n", res, gst_flow_get_name (res));
    fail_unless (res == GST_FLOW_OK);

    res = chain_rtp_packet (rtp_sink, &data);
    GST_DEBUG ("res %d, %s\n", res, gst_flow_get_name (res));
    fail_unless (res == GST_FLOW_OK);

    /* we wait for the new pad to appear now */
    g_mutex_lock (&data.lock);
    while (!data.pad_added)
      g_cond_wait (&data.cond, &data.lock);
    g_mutex_unlock (&data.lock);

    /* sourcepad created now */
    fail_unless (rtpbin->numsinkpads == 1);
    fail_unless (rtpbin->numsrcpads == 1);

    /* change state */
    ret = gst_element_set_state (rtpbin, GST_STATE_NULL);
    fail_unless (ret == GST_STATE_CHANGE_SUCCESS);

    /* pad should be gone now */
    g_mutex_lock (&data.lock);
    while (data.pad_added)
      g_cond_wait (&data.cond, &data.lock);
    g_mutex_unlock (&data.lock);

    /* back to playing for the next round */
    ret = gst_element_set_state (rtpbin, GST_STATE_PLAYING);
    fail_unless (ret == GST_STATE_CHANGE_SUCCESS);
  }

  /* remove the session */
  gst_element_release_request_pad (rtpbin, rtp_sink);
  gst_object_unref (rtp_sink);

  /* nothing left anymore now */
  fail_unless (rtpbin->numsinkpads == 0);
  fail_unless (rtpbin->numsrcpads == 0);

  ret = gst_element_set_state (rtpbin, GST_STATE_NULL);
  fail_unless (ret == GST_STATE_CHANGE_SUCCESS);

  gst_object_unref (rtpbin);

  clean_data (&data);
}

GST_END_TEST;

GST_START_TEST (test_request_pad_by_template_name)
{
  GstElement *rtpbin;
  GstPad *rtp_sink1, *rtp_sink2, *rtp_sink3;

  rtpbin = gst_element_factory_make ("rtpbin", "rtpbin");
  rtp_sink1 = gst_element_get_request_pad (rtpbin, "recv_rtp_sink_%u");
  fail_unless (rtp_sink1 != NULL);
  fail_unless_equals_string (GST_PAD_NAME (rtp_sink1), "recv_rtp_sink_0");
  ASSERT_OBJECT_REFCOUNT (rtp_sink1, "rtp_sink1", 2);

  rtp_sink2 = gst_element_get_request_pad (rtpbin, "recv_rtp_sink_%u");
  fail_unless (rtp_sink2 != NULL);
  fail_unless_equals_string (GST_PAD_NAME (rtp_sink2), "recv_rtp_sink_1");
  ASSERT_OBJECT_REFCOUNT (rtp_sink2, "rtp_sink2", 2);

  rtp_sink3 = gst_element_get_request_pad (rtpbin, "recv_rtp_sink_%u");
  fail_unless (rtp_sink3 != NULL);
  fail_unless_equals_string (GST_PAD_NAME (rtp_sink3), "recv_rtp_sink_2");
  ASSERT_OBJECT_REFCOUNT (rtp_sink3, "rtp_sink3", 2);


  gst_element_release_request_pad (rtpbin, rtp_sink2);
  gst_element_release_request_pad (rtpbin, rtp_sink1);
  gst_element_release_request_pad (rtpbin, rtp_sink3);
  ASSERT_OBJECT_REFCOUNT (rtp_sink3, "rtp_sink3", 1);
  ASSERT_OBJECT_REFCOUNT (rtp_sink2, "rtp_sink2", 1);
  ASSERT_OBJECT_REFCOUNT (rtp_sink1, "rtp_sink", 1);
  gst_object_unref (rtp_sink1);
  gst_object_unref (rtp_sink2);
  gst_object_unref (rtp_sink3);

  gst_object_unref (rtpbin);
}

GST_END_TEST;

static GstElement *
encoder_cb (GstElement * rtpbin, guint sessid, GstElement * bin)
{
  GstPad *srcpad, *sinkpad;

  fail_unless (sessid == 2);

  GST_DEBUG ("making encoder");
  sinkpad = gst_ghost_pad_new_no_target ("rtp_sink_2", GST_PAD_SINK);
  srcpad = gst_ghost_pad_new_no_target ("rtp_src_2", GST_PAD_SRC);

  gst_element_add_pad (bin, sinkpad);
  gst_element_add_pad (bin, srcpad);

  return gst_object_ref (bin);
}

static GstElement *
encoder_cb2 (GstElement * rtpbin, guint sessid, GstElement * bin)
{
  GstPad *srcpad, *sinkpad;

  fail_unless (sessid == 3);

  GST_DEBUG ("making encoder");
  sinkpad = gst_ghost_pad_new_no_target ("rtp_sink_3", GST_PAD_SINK);
  srcpad = gst_ghost_pad_new_no_target ("rtp_src_3", GST_PAD_SRC);

  gst_element_add_pad (bin, sinkpad);
  gst_element_add_pad (bin, srcpad);

  return gst_object_ref (bin);
}

GST_START_TEST (test_encoder)
{
  GstElement *rtpbin, *bin;
  GstPad *rtp_sink1, *rtp_sink2;
  gulong id;

  bin = gst_bin_new ("rtpenc");

  rtpbin = gst_element_factory_make ("rtpbin", "rtpbin");

  id = g_signal_connect (rtpbin, "request-rtp-encoder", (GCallback) encoder_cb,
      bin);

  rtp_sink1 = gst_element_get_request_pad (rtpbin, "send_rtp_sink_2");
  fail_unless (rtp_sink1 != NULL);
  fail_unless_equals_string (GST_PAD_NAME (rtp_sink1), "send_rtp_sink_2");
  ASSERT_OBJECT_REFCOUNT (rtp_sink1, "rtp_sink1", 2);

  g_signal_handler_disconnect (rtpbin, id);

  id = g_signal_connect (rtpbin, "request-rtp-encoder", (GCallback) encoder_cb2,
      bin);

  rtp_sink2 = gst_element_get_request_pad (rtpbin, "send_rtp_sink_3");
  fail_unless (rtp_sink2 != NULL);

  /* remove the session */
  gst_element_release_request_pad (rtpbin, rtp_sink1);
  gst_object_unref (rtp_sink1);

  gst_element_release_request_pad (rtpbin, rtp_sink2);
  gst_object_unref (rtp_sink2);

  /* nothing left anymore now */
  fail_unless (rtpbin->numsinkpads == 0);
  fail_unless (rtpbin->numsrcpads == 0);

  gst_object_unref (rtpbin);
  gst_object_unref (bin);
}

GST_END_TEST;

static GstElement *
decoder_cb (GstElement * rtpbin, guint sessid, gpointer user_data)
{
  GstElement *bin;
  GstPad *srcpad, *sinkpad;

  bin = gst_bin_new (NULL);

  GST_DEBUG ("making decoder");
  sinkpad = gst_ghost_pad_new_no_target ("rtp_sink", GST_PAD_SINK);
  srcpad = gst_ghost_pad_new_no_target ("rtp_src", GST_PAD_SRC);

  gst_element_add_pad (bin, sinkpad);
  gst_element_add_pad (bin, srcpad);

  return bin;
}

GST_START_TEST (test_decoder)
{
  GstElement *rtpbin;
  GstPad *rtp_sink1, *rtp_sink2;
  gulong id;


  rtpbin = gst_element_factory_make ("rtpbin", "rtpbin");

  id = g_signal_connect (rtpbin, "request-rtp-decoder", (GCallback) decoder_cb,
      NULL);

  rtp_sink1 = gst_element_get_request_pad (rtpbin, "recv_rtp_sink_2");
  fail_unless (rtp_sink1 != NULL);
  fail_unless_equals_string (GST_PAD_NAME (rtp_sink1), "recv_rtp_sink_2");
  ASSERT_OBJECT_REFCOUNT (rtp_sink1, "rtp_sink1", 2);

  rtp_sink2 = gst_element_get_request_pad (rtpbin, "recv_rtp_sink_3");
  fail_unless (rtp_sink2 != NULL);

  g_signal_handler_disconnect (rtpbin, id);

  /* remove the session */
  gst_element_release_request_pad (rtpbin, rtp_sink1);
  gst_object_unref (rtp_sink1);

  gst_element_release_request_pad (rtpbin, rtp_sink2);
  gst_object_unref (rtp_sink2);

  /* nothing left anymore now */
  fail_unless (rtpbin->numsinkpads == 0);
  fail_unless (rtpbin->numsrcpads == 0);

  gst_object_unref (rtpbin);
}

GST_END_TEST;

static GstElement *
aux_sender_cb (GstElement * rtpbin, guint sessid, gpointer user_data)
{
  GstElement *bin;
  GstPad *srcpad, *sinkpad;

  bin = (GstElement *) user_data;

  GST_DEBUG ("making AUX sender");
  sinkpad = gst_ghost_pad_new_no_target ("sink_2", GST_PAD_SINK);
  gst_element_add_pad (bin, sinkpad);

  srcpad = gst_ghost_pad_new_no_target ("src_2", GST_PAD_SRC);
  gst_element_add_pad (bin, srcpad);
  srcpad = gst_ghost_pad_new_no_target ("src_1", GST_PAD_SRC);
  gst_element_add_pad (bin, srcpad);
  srcpad = gst_ghost_pad_new_no_target ("src_3", GST_PAD_SRC);
  gst_element_add_pad (bin, srcpad);

  return bin;
}

GST_START_TEST (test_aux_sender)
{
  GstElement *rtpbin;
  GstPad *rtp_sink1, *rtp_src, *rtcp_src;
  gulong id;
  GstElement *aux_sender = gst_object_ref_sink (gst_bin_new ("aux-sender"));

  gst_object_ref (aux_sender);

  rtpbin = gst_element_factory_make ("rtpbin", "rtpbin");

  id = g_signal_connect (rtpbin, "request-aux-sender",
      (GCallback) aux_sender_cb, aux_sender);

  rtp_sink1 = gst_element_get_request_pad (rtpbin, "send_rtp_sink_2");
  fail_unless (rtp_sink1 != NULL);
  fail_unless_equals_string (GST_PAD_NAME (rtp_sink1), "send_rtp_sink_2");
  ASSERT_OBJECT_REFCOUNT (rtp_sink1, "rtp_sink1", 2);

  g_signal_handler_disconnect (rtpbin, id);

  rtp_src = gst_element_get_static_pad (rtpbin, "send_rtp_src_2");
  fail_unless (rtp_src != NULL);
  gst_object_unref (rtp_src);

  rtp_src = gst_element_get_static_pad (rtpbin, "send_rtp_src_1");
  fail_unless (rtp_src != NULL);
  gst_object_unref (rtp_src);

  rtcp_src = gst_element_get_request_pad (rtpbin, "send_rtcp_src_1");
  fail_unless (rtcp_src != NULL);
  gst_element_release_request_pad (rtpbin, rtcp_src);
  gst_object_unref (rtcp_src);

  rtp_src = gst_element_get_static_pad (rtpbin, "send_rtp_src_3");
  fail_unless (rtp_src != NULL);
  gst_object_unref (rtp_src);

  /* remove the session */
  gst_element_release_request_pad (rtpbin, rtp_sink1);
  gst_object_unref (rtp_sink1);

  /* We have sinked the initial reference before returning it
   * in the request callback, the ref count should now be 1 because
   * the return of the signal is transfer full, and rtpbin should
   * have released that reference by now, but we had taken an
   * extra reference to perform this check
   */
  ASSERT_OBJECT_REFCOUNT (aux_sender, "aux-sender", 1);

  gst_object_unref (aux_sender);
  gst_object_unref (rtpbin);
}

GST_END_TEST;

static GstElement *
aux_receiver_cb (GstElement * rtpbin, guint sessid, gpointer user_data)
{
  GstElement *bin;
  GstPad *srcpad, *sinkpad;

  bin = gst_bin_new (NULL);

  GST_DEBUG ("making AUX receiver");
  srcpad = gst_ghost_pad_new_no_target ("src_2", GST_PAD_SRC);
  gst_element_add_pad (bin, srcpad);

  sinkpad = gst_ghost_pad_new_no_target ("sink_2", GST_PAD_SINK);
  gst_element_add_pad (bin, sinkpad);
  sinkpad = gst_ghost_pad_new_no_target ("sink_1", GST_PAD_SINK);
  gst_element_add_pad (bin, sinkpad);
  sinkpad = gst_ghost_pad_new_no_target ("sink_3", GST_PAD_SINK);
  gst_element_add_pad (bin, sinkpad);

  return bin;
}

GST_START_TEST (test_aux_receiver)
{
  GstElement *rtpbin;
  GstPad *rtp_sink1, *rtp_sink2, *rtcp_sink;
  gulong id;

  rtpbin = gst_element_factory_make ("rtpbin", "rtpbin");

  id = g_signal_connect (rtpbin, "request-aux-receiver",
      (GCallback) aux_receiver_cb, NULL);

  rtp_sink1 = gst_element_get_request_pad (rtpbin, "recv_rtp_sink_2");
  fail_unless (rtp_sink1 != NULL);

  rtp_sink2 = gst_element_get_request_pad (rtpbin, "recv_rtp_sink_1");
  fail_unless (rtp_sink2 != NULL);

  g_signal_handler_disconnect (rtpbin, id);

  rtcp_sink = gst_element_get_request_pad (rtpbin, "recv_rtcp_sink_1");
  fail_unless (rtcp_sink != NULL);
  gst_element_release_request_pad (rtpbin, rtcp_sink);
  gst_object_unref (rtcp_sink);

  /* remove the session */
  gst_element_release_request_pad (rtpbin, rtp_sink1);
  gst_object_unref (rtp_sink1);
  gst_element_release_request_pad (rtpbin, rtp_sink2);
  gst_object_unref (rtp_sink2);

  gst_object_unref (rtpbin);
}

GST_END_TEST;

GST_START_TEST (test_sender_eos)
{
  GstElement *rtpsession;
  GstBuffer *rtp_buffer;
  GstBuffer *rtcp_buffer;
  GstRTPBuffer rtpbuf = GST_RTP_BUFFER_INIT;
  GstRTCPBuffer rtcpbuf = GST_RTCP_BUFFER_INIT;
  GstRTCPPacket rtcppacket;
  static GstStaticPadTemplate recv_tmpl =
      GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("ANY"));
  GstPad *send_rtp_sink;
  GstPad *recv_rtcp_sink;
  GstCaps *caps;
  GstSegment segment;
  GstPad *rtp_sink, *rtcp_sink;
  GstClock *clock;
  GstTestClock *tclock;
  GstStructure *s;
  guint ssrc = 1;
  guint32 ssrc_in, packet_count, octet_count;
  gboolean got_bye = FALSE;

  clock = gst_test_clock_new ();
  gst_system_clock_set_default (clock);
  tclock = GST_TEST_CLOCK (clock);
  gst_test_clock_set_time (tclock, 0);

  rtpsession = gst_element_factory_make ("rtpsession", NULL);
  send_rtp_sink = gst_element_get_request_pad (rtpsession, "send_rtp_sink");
  recv_rtcp_sink = gst_element_get_request_pad (rtpsession, "recv_rtcp_sink");


  rtp_sink = gst_check_setup_sink_pad_by_name (rtpsession, &recv_tmpl,
      "send_rtp_src");
  rtcp_sink = gst_check_setup_sink_pad_by_name (rtpsession, &recv_tmpl,
      "send_rtcp_src");

  gst_pad_set_active (rtp_sink, TRUE);
  gst_pad_set_active (rtcp_sink, TRUE);

  gst_element_set_state (rtpsession, GST_STATE_PLAYING);

  /* Send initial events */

  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_send_event (send_rtp_sink,
          gst_event_new_stream_start ("id")));
  fail_unless (gst_pad_send_event (send_rtp_sink,
          gst_event_new_segment (&segment)));

  fail_unless (gst_pad_send_event (recv_rtcp_sink,
          gst_event_new_stream_start ("id")));
  fail_unless (gst_pad_send_event (recv_rtcp_sink,
          gst_event_new_segment (&segment)));

  /* Get the suggested SSRC from the rtpsession */

  caps = gst_pad_query_caps (send_rtp_sink, NULL);
  s = gst_caps_get_structure (caps, 0);
  gst_structure_get (s, "ssrc", G_TYPE_UINT, &ssrc, NULL);
  gst_caps_unref (caps);

  /* Send a RTP packet */

  rtp_buffer = gst_rtp_buffer_new_allocate (10, 0, 0);
  gst_rtp_buffer_map (rtp_buffer, GST_MAP_READWRITE, &rtpbuf);
  gst_rtp_buffer_set_ssrc (&rtpbuf, 1);
  gst_rtp_buffer_set_seq (&rtpbuf, 0);
  gst_rtp_buffer_unmap (&rtpbuf);

  fail_unless (gst_pad_chain (send_rtp_sink, rtp_buffer) == GST_FLOW_OK);

  /* Make sure it went through */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_unless_equals_pointer (buffers->data, rtp_buffer);
  gst_check_drop_buffers ();

  /* Advance time and send a packet to prevent source sender timeout */
  gst_test_clock_set_time (tclock, 1 * GST_SECOND);

  /* Just send a send packet to prevent timeout */
  rtp_buffer = gst_rtp_buffer_new_allocate (10, 0, 0);
  gst_rtp_buffer_map (rtp_buffer, GST_MAP_READWRITE, &rtpbuf);
  gst_rtp_buffer_set_ssrc (&rtpbuf, 1);
  gst_rtp_buffer_set_seq (&rtpbuf, 1);
  gst_rtp_buffer_set_timestamp (&rtpbuf, 10);
  gst_rtp_buffer_unmap (&rtpbuf);

  fail_unless (gst_pad_chain (send_rtp_sink, rtp_buffer) == GST_FLOW_OK);

  /* Make sure it went through */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_unless_equals_pointer (buffers->data, rtp_buffer);
  gst_check_drop_buffers ();

  /* Advance clock twice and we should have one RTCP packet at least */
  gst_test_clock_crank (tclock);
  gst_test_clock_crank (tclock);

  g_mutex_lock (&check_mutex);
  while (buffers == NULL)
    g_cond_wait (&check_cond, &check_mutex);

  fail_unless (gst_rtcp_buffer_map (buffers->data, GST_MAP_READ, &rtcpbuf));

  fail_unless (gst_rtcp_buffer_get_first_packet (&rtcpbuf, &rtcppacket));

  fail_unless_equals_int (gst_rtcp_packet_get_type (&rtcppacket),
      GST_RTCP_TYPE_SR);
  gst_rtcp_packet_sr_get_sender_info (&rtcppacket, &ssrc_in, NULL, NULL,
      &packet_count, &octet_count);
  fail_unless_equals_int (packet_count, 2);
  fail_unless_equals_int (octet_count, 20);

  fail_unless (gst_rtcp_packet_move_to_next (&rtcppacket));
  fail_unless_equals_int (gst_rtcp_packet_get_type (&rtcppacket),
      GST_RTCP_TYPE_SDES);

  gst_rtcp_buffer_unmap (&rtcpbuf);
  gst_check_drop_buffers ();

  g_mutex_unlock (&check_mutex);


  /* Create and send a valid RTCP reply packet */
  rtcp_buffer = gst_rtcp_buffer_new (1500);
  gst_rtcp_buffer_map (rtcp_buffer, GST_MAP_READWRITE, &rtcpbuf);
  gst_rtcp_buffer_add_packet (&rtcpbuf, GST_RTCP_TYPE_RR, &rtcppacket);
  gst_rtcp_packet_rr_set_ssrc (&rtcppacket, ssrc + 1);
  gst_rtcp_packet_add_rb (&rtcppacket, ssrc, 0, 0, 0, 0, 0, 0);
  gst_rtcp_buffer_add_packet (&rtcpbuf, GST_RTCP_TYPE_SDES, &rtcppacket);
  gst_rtcp_packet_sdes_add_item (&rtcppacket, ssrc + 1);
  gst_rtcp_packet_sdes_add_entry (&rtcppacket, GST_RTCP_SDES_CNAME, 3,
      (guint8 *) "a@a");
  gst_rtcp_packet_sdes_add_entry (&rtcppacket, GST_RTCP_SDES_NAME, 2,
      (guint8 *) "aa");
  gst_rtcp_packet_sdes_add_entry (&rtcppacket, GST_RTCP_SDES_END, 0,
      (guint8 *) "");
  gst_rtcp_buffer_unmap (&rtcpbuf);
  fail_unless (gst_pad_chain (recv_rtcp_sink, rtcp_buffer) == GST_FLOW_OK);


  /* Send a EOS to trigger sending a BYE message */
  fail_unless (gst_pad_send_event (send_rtp_sink, gst_event_new_eos ()));

  /* Crank to process EOS and wait for BYE */
  for (;;) {
    gst_test_clock_crank (tclock);
    g_mutex_lock (&check_mutex);
    while (buffers == NULL)
      g_cond_wait (&check_cond, &check_mutex);

    fail_unless (gst_rtcp_buffer_map (g_list_last (buffers)->data, GST_MAP_READ,
            &rtcpbuf));
    fail_unless (gst_rtcp_buffer_get_first_packet (&rtcpbuf, &rtcppacket));

    while (gst_rtcp_packet_move_to_next (&rtcppacket)) {
      if (gst_rtcp_packet_get_type (&rtcppacket) == GST_RTCP_TYPE_BYE) {
        got_bye = TRUE;
        break;
      }
    }
    g_mutex_unlock (&check_mutex);
    gst_rtcp_buffer_unmap (&rtcpbuf);

    if (got_bye)
      break;
  }

  gst_check_drop_buffers ();


  fail_unless (GST_PAD_IS_EOS (rtp_sink));
  fail_unless (GST_PAD_IS_EOS (rtcp_sink));

  gst_pad_set_active (rtp_sink, FALSE);
  gst_pad_set_active (rtcp_sink, FALSE);

  gst_check_teardown_pad_by_name (rtpsession, "send_rtp_src");
  gst_check_teardown_pad_by_name (rtpsession, "send_rtcp_src");
  gst_element_release_request_pad (rtpsession, send_rtp_sink);
  gst_object_unref (send_rtp_sink);
  gst_element_release_request_pad (rtpsession, recv_rtcp_sink);
  gst_object_unref (recv_rtcp_sink);

  gst_check_teardown_element (rtpsession);

  gst_system_clock_set_default (NULL);
  gst_object_unref (clock);

}

GST_END_TEST;

static Suite *
rtpbin_suite (void)
{
  Suite *s = suite_create ("rtpbin");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_pads);
  tcase_add_test (tc_chain, test_cleanup_send);
  tcase_add_test (tc_chain, test_cleanup_recv);
  tcase_add_test (tc_chain, test_cleanup_recv2);
  tcase_add_test (tc_chain, test_request_pad_by_template_name);
  tcase_add_test (tc_chain, test_encoder);
  tcase_add_test (tc_chain, test_decoder);
  tcase_add_test (tc_chain, test_aux_sender);
  tcase_add_test (tc_chain, test_aux_receiver);
  tcase_add_test (tc_chain, test_sender_eos);

  return s;
}

GST_CHECK_MAIN (rtpbin);
