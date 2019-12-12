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
#include <gst/check/gstharness.h>
#include <gst/rtp/gstrtpbuffer.h>

#define verify_buf(buf, is_rtx, expected_ssrc, expted_pt, expected_seqnum)       \
  G_STMT_START {                                                                 \
    GstRTPBuffer _rtp = GST_RTP_BUFFER_INIT;                                     \
    fail_unless (gst_rtp_buffer_map (buf, GST_MAP_READ, &_rtp));                 \
    fail_unless_equals_int (gst_rtp_buffer_get_ssrc (&_rtp), expected_ssrc);     \
    fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&_rtp), expted_pt); \
    if (!(is_rtx)) {                                                             \
      fail_unless_equals_int (gst_rtp_buffer_get_seq (&_rtp), expected_seqnum);  \
    } else {                                                                     \
      fail_unless_equals_int (GST_READ_UINT16_BE (gst_rtp_buffer_get_payload     \
              (&_rtp)), expected_seqnum);                                        \
    }                                                                            \
    gst_rtp_buffer_unmap (&_rtp);                                                \
  } G_STMT_END

#define pull_and_verify(h, is_rtx, expected_ssrc, expted_pt, expected_seqnum) \
  G_STMT_START {                                                              \
    GstBuffer *_buf = gst_harness_pull (h);                                   \
    verify_buf (_buf, is_rtx, expected_ssrc, expted_pt, expected_seqnum);     \
    gst_buffer_unref (_buf);                                                  \
  } G_STMT_END

#define push_pull_and_verify(h, buf, is_rtx, expected_ssrc, expted_pt, expected_seqnum) \
  G_STMT_START {                                                                        \
    gst_harness_push (h, buf);                                                          \
    pull_and_verify (h, is_rtx, expected_ssrc, expted_pt, expected_seqnum);             \
  } G_STMT_END

static GstEvent *
create_rtx_event (guint32 ssrc, guint8 payload_type, guint16 seqnum)
{
  return gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM,
      gst_structure_new ("GstRTPRetransmissionRequest",
          "seqnum", G_TYPE_UINT, seqnum,
          "ssrc", G_TYPE_UINT, ssrc,
          "payload-type", G_TYPE_UINT, payload_type, NULL));
}

static void
compare_rtp_packets (GstBuffer * a, GstBuffer * b)
{
  GstRTPBuffer rtp_a = GST_RTP_BUFFER_INIT;
  GstRTPBuffer rtp_b = GST_RTP_BUFFER_INIT;

  gst_rtp_buffer_map (a, GST_MAP_READ, &rtp_a);
  gst_rtp_buffer_map (b, GST_MAP_READ, &rtp_b);

  fail_unless_equals_int (gst_rtp_buffer_get_header_len (&rtp_a),
      gst_rtp_buffer_get_header_len (&rtp_b));
  fail_unless_equals_int (gst_rtp_buffer_get_version (&rtp_a),
      gst_rtp_buffer_get_version (&rtp_b));
  fail_unless_equals_int (gst_rtp_buffer_get_ssrc (&rtp_a),
      gst_rtp_buffer_get_ssrc (&rtp_b));
  fail_unless_equals_int (gst_rtp_buffer_get_seq (&rtp_a),
      gst_rtp_buffer_get_seq (&rtp_b));
  fail_unless_equals_int (gst_rtp_buffer_get_csrc_count (&rtp_a),
      gst_rtp_buffer_get_csrc_count (&rtp_b));
  fail_unless_equals_int (gst_rtp_buffer_get_marker (&rtp_a),
      gst_rtp_buffer_get_marker (&rtp_b));
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp_a),
      gst_rtp_buffer_get_payload_type (&rtp_b));
  fail_unless_equals_int (gst_rtp_buffer_get_timestamp (&rtp_a),
      gst_rtp_buffer_get_timestamp (&rtp_b));
  fail_unless_equals_int (gst_rtp_buffer_get_extension (&rtp_a),
      gst_rtp_buffer_get_extension (&rtp_b));

  fail_unless_equals_int (gst_rtp_buffer_get_payload_len (&rtp_a),
      gst_rtp_buffer_get_payload_len (&rtp_b));
  fail_unless_equals_int (memcmp (gst_rtp_buffer_get_payload (&rtp_a),
          gst_rtp_buffer_get_payload (&rtp_b),
          gst_rtp_buffer_get_payload_len (&rtp_a)), 0);

  gst_rtp_buffer_unmap (&rtp_a);
  gst_rtp_buffer_unmap (&rtp_b);
}

static GstBuffer *
create_rtp_buffer (guint32 ssrc, guint8 payload_type, guint16 seqnum)
{
  GstRTPBuffer rtpbuf = GST_RTP_BUFFER_INIT;
  guint payload_size = 29;
  guint64 timestamp = gst_util_uint64_scale_int (seqnum, 90000, 30);
  GstBuffer *buf = gst_rtp_buffer_new_allocate (payload_size, 0, 0);

  gst_rtp_buffer_map (buf, GST_MAP_WRITE, &rtpbuf);
  gst_rtp_buffer_set_ssrc (&rtpbuf, ssrc);
  gst_rtp_buffer_set_payload_type (&rtpbuf, payload_type);
  gst_rtp_buffer_set_seq (&rtpbuf, seqnum);
  gst_rtp_buffer_set_timestamp (&rtpbuf, (guint32) timestamp);
  memset (gst_rtp_buffer_get_payload (&rtpbuf), 0x29, payload_size);
  gst_rtp_buffer_unmap (&rtpbuf);
  return buf;
}

static GstBuffer *
create_rtp_buffer_with_timestamp (guint32 ssrc, guint8 payload_type,
    guint16 seqnum, guint32 timestamp)
{
  GstRTPBuffer rtpbuf = GST_RTP_BUFFER_INIT;
  GstBuffer *buf = create_rtp_buffer (ssrc, payload_type, seqnum);
  gst_rtp_buffer_map (buf, GST_MAP_WRITE, &rtpbuf);
  gst_rtp_buffer_set_timestamp (&rtpbuf, timestamp);
  gst_rtp_buffer_unmap (&rtpbuf);
  return buf;
}

GST_START_TEST (test_rtxsend_rtxreceive)
{
  const guint packets_num = 5;
  guint master_ssrc = 1234567;
  guint master_pt = 96;
  guint rtx_pt = 99;
  GstStructure *pt_map;
  GstBuffer *inbufs[5];
  GstHarness *hrecv = gst_harness_new ("rtprtxreceive");
  GstHarness *hsend = gst_harness_new ("rtprtxsend");
  gint i;

  pt_map = gst_structure_new ("application/x-rtp-pt-map",
      "96", G_TYPE_UINT, rtx_pt, NULL);
  g_object_set (hrecv->element, "payload-type-map", pt_map, NULL);
  g_object_set (hsend->element, "payload-type-map", pt_map, NULL);

  gst_harness_set_src_caps_str (hsend, "application/x-rtp, "
      "media = (string)video, payload = (int)96, "
      "ssrc = (uint)1234567, clock-rate = (int)90000, "
      "encoding-name = (string)RAW");
  gst_harness_set_src_caps_str (hrecv, "application/x-rtp, "
      "media = (string)video, payload = (int)96, "
      "ssrc = (uint)1234567, clock-rate = (int)90000, "
      "encoding-name = (string)RAW");

  /* Push 'packets_num' packets through rtxsend to rtxreceive */
  for (i = 0; i < packets_num; ++i) {
    inbufs[i] = create_rtp_buffer (master_ssrc, master_pt, 100 + i);
    gst_harness_push (hsend, gst_buffer_ref (inbufs[i]));
    gst_harness_push (hrecv, gst_harness_pull (hsend));
    pull_and_verify (hrecv, FALSE, master_ssrc, master_pt, 100 + i);
  }

  /* Getting rid of reconfigure event. Preparation before the next step */
  gst_event_unref (gst_harness_pull_upstream_event (hrecv));
  fail_unless_equals_int (gst_harness_upstream_events_in_queue (hrecv), 0);

  /* Push 'packets_num' RTX events through rtxreceive to rtxsend.
     Push RTX packets from rtxsend to rtxreceive and
     check that the packet produced out of RTX packet is the same
     as an original packet */
  for (i = 0; i < packets_num; ++i) {
    GstBuffer *outbuf;
    gst_harness_push_upstream_event (hrecv,
        create_rtx_event (master_ssrc, master_pt, 100 + i));
    gst_harness_push_upstream_event (hsend,
        gst_harness_pull_upstream_event (hrecv));
    gst_harness_push (hrecv, gst_harness_pull (hsend));

    outbuf = gst_harness_pull (hrecv);
    compare_rtp_packets (inbufs[i], outbuf);
    gst_buffer_unref (inbufs[i]);
    gst_buffer_unref (outbuf);
  }

  /* Check RTX stats */
  {
    guint rtx_requests;
    guint rtx_packets;
    guint rtx_assoc_packets;
    g_object_get (G_OBJECT (hsend->element),
        "num-rtx-requests", &rtx_requests,
        "num-rtx-packets", &rtx_packets, NULL);
    fail_unless_equals_int (rtx_packets, packets_num);
    fail_unless_equals_int (rtx_requests, packets_num);

    g_object_get (G_OBJECT (hrecv->element),
        "num-rtx-requests", &rtx_requests,
        "num-rtx-packets", &rtx_packets,
        "num-rtx-assoc-packets", &rtx_assoc_packets, NULL);
    fail_unless_equals_int (rtx_packets, packets_num);
    fail_unless_equals_int (rtx_requests, packets_num);
    fail_unless_equals_int (rtx_assoc_packets, packets_num);
  }

  gst_structure_free (pt_map);
  gst_harness_teardown (hrecv);
  gst_harness_teardown (hsend);
}

GST_END_TEST;

GST_START_TEST (test_rtxsend_rtxreceive_with_packet_loss)
{
  guint packets_num = 20;
  guint master_ssrc = 1234567;
  guint master_pt = 96;
  guint rtx_pt = 99;
  guint seqnum = 100;
  guint expected_rtx_packets = 0;
  GstStructure *pt_map;
  GstHarness *hrecv = gst_harness_new ("rtprtxreceive");
  GstHarness *hsend = gst_harness_new ("rtprtxsend");
  gint drop_nth_packet, i;

  pt_map = gst_structure_new ("application/x-rtp-pt-map",
      "96", G_TYPE_UINT, rtx_pt, NULL);
  g_object_set (hrecv->element, "payload-type-map", pt_map, NULL);
  g_object_set (hsend->element, "payload-type-map", pt_map, NULL);

  gst_harness_set_src_caps_str (hsend, "application/x-rtp, "
      "media = (string)video, payload = (int)96, "
      "ssrc = (uint)1234567, clock-rate = (int)90000, "
      "encoding-name = (string)RAW");
  gst_harness_set_src_caps_str (hrecv, "application/x-rtp, "
      "media = (string)video, payload = (int)96, "
      "ssrc = (uint)1234567, clock-rate = (int)90000, "
      "encoding-name = (string)RAW");

  /* Getting rid of reconfigure event. Making sure there is no upstream
     events in the queue. Preparation step before the test. */
  gst_event_unref (gst_harness_pull_upstream_event (hrecv));
  fail_unless_equals_int (gst_harness_upstream_events_in_queue (hrecv), 0);

  /* Push 'packets_num' packets through rtxsend to rtxreceive losing every
     'drop_every_n_packets' packet. When we loose the packet we send RTX event
     through rtxreceive to rtxsend, and verify the packet was retransmitted */
  for (drop_nth_packet = 2; drop_nth_packet < 10; ++drop_nth_packet) {
    for (i = 0; i < packets_num; ++i, ++seqnum) {
      GstBuffer *outbuf;
      GstBuffer *inbuf = create_rtp_buffer (master_ssrc, master_pt, seqnum);
      gboolean drop_this_packet = ((i + 1) % drop_nth_packet) == 0;

      gst_harness_push (hsend, gst_buffer_ref (inbuf));
      if (drop_this_packet) {
        /* Dropping original packet */
        gst_buffer_unref (gst_harness_pull (hsend));
        /* Requesting retransmission */
        gst_harness_push_upstream_event (hrecv,
            create_rtx_event (master_ssrc, master_pt, seqnum));
        gst_harness_push_upstream_event (hsend,
            gst_harness_pull_upstream_event (hrecv));
        /* Pushing RTX packet to rtxreceive */
        gst_harness_push (hrecv, gst_harness_pull (hsend));
        ++expected_rtx_packets;
      } else {
        gst_harness_push (hrecv, gst_harness_pull (hsend));
      }

      /* We making sure every buffer we pull is the same as original input
         buffer */
      outbuf = gst_harness_pull (hrecv);
      compare_rtp_packets (inbuf, outbuf);
      gst_buffer_unref (inbuf);
      gst_buffer_unref (outbuf);

      /*
         We should not have any packets in the harness queue by this point. It
         means rtxsend didn't send more packets than RTX events and rtxreceive
         didn't produce more than one packet per RTX packet.
       */
      fail_unless_equals_int (gst_harness_buffers_in_queue (hsend), 0);
      fail_unless_equals_int (gst_harness_buffers_in_queue (hrecv), 0);
    }
  }

  /* Check RTX stats */
  {
    guint rtx_requests;
    guint rtx_packets;
    guint rtx_assoc_packets;
    g_object_get (G_OBJECT (hsend->element),
        "num-rtx-requests", &rtx_requests,
        "num-rtx-packets", &rtx_packets, NULL);
    fail_unless_equals_int (rtx_packets, expected_rtx_packets);
    fail_unless_equals_int (rtx_requests, expected_rtx_packets);

    g_object_get (G_OBJECT (hrecv->element),
        "num-rtx-requests", &rtx_requests,
        "num-rtx-packets", &rtx_packets,
        "num-rtx-assoc-packets", &rtx_assoc_packets, NULL);
    fail_unless_equals_int (rtx_packets, expected_rtx_packets);
    fail_unless_equals_int (rtx_requests, expected_rtx_packets);
    fail_unless_equals_int (rtx_assoc_packets, expected_rtx_packets);
  }

  gst_structure_free (pt_map);
  gst_harness_teardown (hrecv);
  gst_harness_teardown (hsend);
}

GST_END_TEST;

typedef struct
{
  GstHarness *h;
  guint master_ssrc;
  guint master_pt;
  guint rtx_ssrc;
  guint rtx_pt;
  guint seqnum;
  guint expected_rtx_packets;
} RtxSender;

static GstStructure *
create_rtxsenders (RtxSender * senders, guint senders_num)
{
  GstStructure *recv_pt_map =
      gst_structure_new_empty ("application/x-rtp-pt-map");
  gint i;

  for (i = 0; i < senders_num; ++i) {
    gchar *master_pt_str;
    gchar *master_caps_str;
    GstStructure *send_pt_map;

    senders[i].h = gst_harness_new ("rtprtxsend");
    senders[i].master_ssrc = 1234567 + i;
    senders[i].rtx_ssrc = 7654321 + i;
    senders[i].master_pt = 80 + i;
    senders[i].rtx_pt = 20 + i;
    senders[i].seqnum = i * 1000;
    senders[i].expected_rtx_packets = 0;

    master_pt_str = g_strdup_printf ("%u", senders[i].master_pt);
    master_caps_str = g_strdup_printf ("application/x-rtp, "
        "media = (string)video, payload = (int)%u, "
        "ssrc = (uint)%u, clock-rate = (int)90000, "
        "encoding-name = (string)RAW",
        senders[i].master_pt, senders[i].master_ssrc);

    send_pt_map = gst_structure_new ("application/x-rtp-pt-map",
        master_pt_str, G_TYPE_UINT, senders[i].rtx_pt, NULL);
    gst_structure_set (recv_pt_map,
        master_pt_str, G_TYPE_UINT, senders[i].rtx_pt, NULL);

    g_object_set (senders[i].h->element, "payload-type-map", send_pt_map, NULL);
    gst_harness_set_src_caps_str (senders[i].h, master_caps_str);

    gst_structure_free (send_pt_map);
    g_free (master_pt_str);
    g_free (master_caps_str);
  }
  return recv_pt_map;
}

static guint
check_rtxsenders_stats_and_teardown (RtxSender * senders, guint senders_num)
{
  guint total_pakets_num = 0;
  gint i;

  for (i = 0; i < senders_num; ++i) {
    guint rtx_requests;
    guint rtx_packets;
    g_object_get (G_OBJECT (senders[i].h->element),
        "num-rtx-requests", &rtx_requests,
        "num-rtx-packets", &rtx_packets, NULL);
    fail_unless_equals_int (rtx_packets, senders[i].expected_rtx_packets);
    fail_unless_equals_int (rtx_requests, senders[i].expected_rtx_packets);
    total_pakets_num += rtx_packets;

    gst_harness_teardown (senders[i].h);
  }
  return total_pakets_num;
}

GST_START_TEST (test_multi_rtxsend_rtxreceive_with_packet_loss)
{
  guint senders_num = 5;
  guint packets_num = 10;
  guint total_pakets_num = senders_num * packets_num;
  guint total_dropped_packets = 0;
  RtxSender senders[5];
  GstStructure *pt_map;
  GstHarness *hrecv = gst_harness_new ("rtprtxreceive");
  gint drop_nth_packet, i, j;

  pt_map = create_rtxsenders (senders, 5);
  g_object_set (hrecv->element, "payload-type-map", pt_map, NULL);
  gst_harness_set_src_caps_str (hrecv, "application/x-rtp, "
      "media = (string)video, payload = (int)80, "
      "ssrc = (uint)1234567, clock-rate = (int)90000, "
      "encoding-name = (string)RAW");

  /* Getting rid of reconfigure event. Making sure there is no upstream
     events in the queue. Preparation step before the test. */
  gst_event_unref (gst_harness_pull_upstream_event (hrecv));
  fail_unless_equals_int (gst_harness_upstream_events_in_queue (hrecv), 0);

  /* We are going to push the 1st packet from the 1st sender, 2nd from the 2nd,
     3rd from the 3rd, etc. until all the senders will push 'packets_num' packets.
     We will drop every 'drop_nth_packet' packet and request its retransmission
     from all the senders. Because only one of them can produce RTX packet.
     We need to make sure that all other senders will ignore the RTX event they
     can't act upon.
   */
  for (drop_nth_packet = 2; drop_nth_packet < 5; ++drop_nth_packet) {
    for (i = 0; i < total_pakets_num; ++i) {
      RtxSender *sender = &senders[i % senders_num];
      gboolean drop_this_packet = ((i + 1) % drop_nth_packet) == 0;
      GstBuffer *outbuf, *inbuf;
      inbuf =
          create_rtp_buffer (sender->master_ssrc, sender->master_pt,
          sender->seqnum);

      gst_harness_push (sender->h, gst_buffer_ref (inbuf));
      if (drop_this_packet) {
        GstEvent *rtxevent;
        /* Dropping original packet */
        gst_buffer_unref (gst_harness_pull (sender->h));

        /* Pushing RTX event through rtxreceive to all the senders */
        gst_harness_push_upstream_event (hrecv,
            create_rtx_event (sender->master_ssrc, sender->master_pt,
                sender->seqnum));
        rtxevent = gst_harness_pull_upstream_event (hrecv);

        /* ... to all the senders */
        for (j = 0; j < senders_num; ++j)
          gst_harness_push_upstream_event (senders[j].h,
              gst_event_ref (rtxevent));
        gst_event_unref (rtxevent);

        /* Pushing RTX packet to rtxreceive */
        gst_harness_push (hrecv, gst_harness_pull (sender->h));
        ++sender->expected_rtx_packets;
        ++total_dropped_packets;
      } else {
        gst_harness_push (hrecv, gst_harness_pull (sender->h));
      }

      /* It should not matter whether the buffer was dropped (and retransmitted)
         or it went straight through rtxsend to rtxreceive. We should always pull
         the same buffer that was pushed */
      outbuf = gst_harness_pull (hrecv);
      compare_rtp_packets (inbuf, outbuf);
      gst_buffer_unref (inbuf);
      gst_buffer_unref (outbuf);

      /*
         We should not have any packets in the harness queue by this point. It
         means our senders didn't produce the packets for the unknown RTX event.
       */
      for (j = 0; j < senders_num; ++j)
        fail_unless_equals_int (gst_harness_buffers_in_queue (senders[j].h), 0);

      ++sender->seqnum;
    }
  }

  /* Check RTX stats */
  {
    guint total_rtx_packets;
    guint rtx_requests;
    guint rtx_packets;
    guint rtx_assoc_packets;

    total_rtx_packets =
        check_rtxsenders_stats_and_teardown (senders, senders_num);
    fail_unless_equals_int (total_rtx_packets, total_dropped_packets);

    g_object_get (G_OBJECT (hrecv->element),
        "num-rtx-requests", &rtx_requests,
        "num-rtx-packets", &rtx_packets,
        "num-rtx-assoc-packets", &rtx_assoc_packets, NULL);
    fail_unless_equals_int (rtx_packets, total_rtx_packets);
    fail_unless_equals_int (rtx_requests, total_rtx_packets);
    fail_unless_equals_int (rtx_assoc_packets, total_rtx_packets);
  }

  gst_structure_free (pt_map);
  gst_harness_teardown (hrecv);
}

GST_END_TEST;

static void
test_rtxsender_packet_retention (gboolean test_with_time)
{
  guint master_ssrc = 1234567;
  guint master_pt = 96;
  guint rtx_ssrc = 7654321;
  guint rtx_pt = 99;
  gint num_buffers = test_with_time ? 30 : 10;
  gint half_buffers = num_buffers / 2;
  guint timestamp_delta = 90000 / 30;
  guint timestamp = G_MAXUINT32 - half_buffers * timestamp_delta;
  GstHarness *h;
  GstStructure *pt_map = gst_structure_new ("application/x-rtp-pt-map",
      "96", G_TYPE_UINT, rtx_pt, NULL);
  GstStructure *ssrc_map = gst_structure_new ("application/x-rtp-ssrc-map",
      "1234567", G_TYPE_UINT, rtx_ssrc, NULL);
  gint i, j;

  h = gst_harness_new ("rtprtxsend");

  /* In both cases we want the rtxsend queue to store 'half_buffers'
     amount of buffers at most. In max-size-packets mode, it's trivial.
     In max-size-time mode, we specify almost half a second, which is
     the equivalent of 15 frames in a 30fps video stream.
   */
  g_object_set (h->element,
      "max-size-packets", test_with_time ? 0 : half_buffers,
      "max-size-time", test_with_time ? 499 : 0,
      "payload-type-map", pt_map, "ssrc-map", ssrc_map, NULL);

  gst_harness_set_src_caps_str (h, "application/x-rtp, "
      "media = (string)video, payload = (int)96, "
      "ssrc = (uint)1234567, clock-rate = (int)90000, "
      "encoding-name = (string)RAW");

  /* Now push all buffers and request retransmission every time for all of them */
  for (i = 0; i < num_buffers; ++i, timestamp += timestamp_delta) {
    /* Request to retransmit all the previous ones */
    for (j = 0; j < i; ++j) {
      guint rtx_seqnum = 0x100 + j;
      gst_harness_push_upstream_event (h,
          create_rtx_event (master_ssrc, master_pt, rtx_seqnum));

      /* Pull only the ones supposed to be retransmitted */
      if (j >= i - half_buffers)
        pull_and_verify (h, TRUE, rtx_ssrc, rtx_pt, rtx_seqnum);
    }
    /* Check there no extra buffers in the harness queue */
    fail_unless_equals_int (gst_harness_buffers_in_queue (h), 0);

    /* We create RTP buffers with timestamps that will eventually wrap around 0
       to be sure, rtprtxsend can handle it properly */
    push_pull_and_verify (h,
        create_rtp_buffer_with_timestamp (master_ssrc, master_pt, 0x100 + i,
            timestamp), FALSE, master_ssrc, master_pt, 0x100 + i);
  }

  gst_structure_free (pt_map);
  gst_structure_free (ssrc_map);
  gst_harness_teardown (h);
}

GST_START_TEST (test_rtxsender_max_size_packets)
{
  test_rtxsender_packet_retention (FALSE);
}

GST_END_TEST;

GST_START_TEST (test_rtxsender_max_size_time)
{
  test_rtxsender_packet_retention (TRUE);
}

GST_END_TEST;

static void
test_rtxqueue_packet_retention (gboolean test_with_time)
{
  guint ssrc = 1234567;
  guint pt = 96;
  gint num_buffers = test_with_time ? 30 : 10;
  gint half_buffers = num_buffers / 2;
  GstClockTime timestamp_delta = GST_SECOND / 30;
  GstClockTime timestamp = 0;
  GstBuffer *buf;
  GstHarness *h;
  gint i, j;

  h = gst_harness_new ("rtprtxqueue");

  /* In both cases we want the rtxqueue to store 'half_buffers'
     amount of buffers at most. In max-size-packets mode, it's trivial.
     In max-size-time mode, we specify almost half a second, which is
     the equivalent of 15 frames in a 30fps video stream.
   */
  g_object_set (h->element,
      "max-size-packets", test_with_time ? 0 : half_buffers,
      "max-size-time", test_with_time ? 498 : 0, NULL);

  gst_harness_set_src_caps_str (h, "application/x-rtp, "
      "media = (string)video, payload = (int)96, "
      "ssrc = (uint)1234567, clock-rate = (int)90000, "
      "encoding-name = (string)RAW");

  /* Now push all buffers and request retransmission every time for all of them.
   * Note that rtprtxqueue sends retransmissions in chain(), just before
   * pushing out the chained buffer, a differentiation from rtprtxsend above
   */
  for (i = 0; i < num_buffers; ++i, timestamp += timestamp_delta) {
    /* Request to retransmit all the previous ones */
    for (j = 0; j < i; ++j) {
      guint rtx_seqnum = 0x100 + j;
      gst_harness_push_upstream_event (h,
          create_rtx_event (ssrc, pt, rtx_seqnum));
    }

    /* push one packet */
    buf = create_rtp_buffer (ssrc, pt, 0x100 + i);
    GST_BUFFER_TIMESTAMP (buf) = timestamp;
    gst_harness_push (h, buf);

    /* Pull the ones supposed to be retransmitted */
    for (j = 0; j < i; ++j) {
      guint rtx_seqnum = 0x100 + j;
      if (j >= i - half_buffers)
        pull_and_verify (h, FALSE, ssrc, pt, rtx_seqnum);
    }

    /* There should be only one packet remaining in the queue now */
    fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

    /* pull the one that we just pushed (comes after the retransmitted ones) */
    pull_and_verify (h, FALSE, ssrc, pt, 0x100 + i);

    /* Check there no extra buffers in the harness queue */
    fail_unless_equals_int (gst_harness_buffers_in_queue (h), 0);
  }

  gst_harness_teardown (h);
}

GST_START_TEST (test_rtxqueue_max_size_packets)
{
  test_rtxqueue_packet_retention (FALSE);
}

GST_END_TEST;

GST_START_TEST (test_rtxqueue_max_size_time)
{
  test_rtxqueue_packet_retention (TRUE);
}

GST_END_TEST;

static Suite *
rtprtx_suite (void)
{
  Suite *s = suite_create ("rtprtx");
  TCase *tc_chain = tcase_create ("general");

  tcase_set_timeout (tc_chain, 120);

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_rtxsend_rtxreceive);
  tcase_add_test (tc_chain, test_rtxsend_rtxreceive_with_packet_loss);
  tcase_add_test (tc_chain, test_multi_rtxsend_rtxreceive_with_packet_loss);
  tcase_add_test (tc_chain, test_rtxsender_max_size_packets);
  tcase_add_test (tc_chain, test_rtxsender_max_size_time);
  tcase_add_test (tc_chain, test_rtxqueue_max_size_packets);
  tcase_add_test (tc_chain, test_rtxqueue_max_size_time);

  return s;
}

GST_CHECK_MAIN (rtprtx);
