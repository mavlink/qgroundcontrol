/* GStreamer plugin for forward error correction
 * Copyright (C) 2017 Pexip
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Mikhail Fludkov <misha@pexip.com>
 */

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>

#include "../../../gst/rtp/gstrtpstorage.h"
#include "../../../gst/rtp/rtpstorage.h"

#define RTP_CLOCK_RATE (90000)
#define RTP_FRAME_DUR (RTP_CLOCK_RATE / 30)

#define RTP_TSTAMP_BASE (0x11111111)
#define GST_TSTAMP_BASE (0x22222222)
#define RTP_TSTAMP(i) (RTP_FRAME_DUR * (i) + RTP_TSTAMP_BASE)
#define GST_TSTAMP(i) (RTP_PACKET_DUR * (i) + GST_TSTAMP_BASE)

#define RTP_PACKET_DUR (10 * GST_MSECOND)

static GstBufferList *
get_packets_for_recovery (GstHarness * h, gint fec_pt, guint32 ssrc,
    guint16 lost_seq)
{
  GstBufferList *res;
  RtpStorage *internal_storage;

  g_object_get (h->element, "internal-storage", &internal_storage, NULL);

  res =
      rtp_storage_get_packets_for_recovery (internal_storage, fec_pt, ssrc,
      lost_seq);

  g_object_unref (internal_storage);

  return res;
}

static void
put_recovered_packet (GstHarness * h, GstBuffer * buffer, guint8 pt,
    guint32 ssrc, guint16 seq)
{
  RtpStorage *internal_storage;

  g_object_get (h->element, "internal-storage", &internal_storage, NULL);

  rtp_storage_put_recovered_packet (internal_storage, buffer, pt, ssrc, seq);

  g_object_unref (internal_storage);
}

static GstBuffer *
create_rtp_packet (guint8 pt, guint32 ssrc, guint32 timestamp, guint16 seq)
{
  GstBuffer *buf = gst_rtp_buffer_new_allocate (0, 0, 0);
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  fail_unless (gst_rtp_buffer_map (buf, GST_MAP_WRITE, &rtp));
  gst_rtp_buffer_set_ssrc (&rtp, ssrc);
  gst_rtp_buffer_set_payload_type (&rtp, pt);
  gst_rtp_buffer_set_timestamp (&rtp, timestamp);
  gst_rtp_buffer_set_seq (&rtp, seq);
  GST_BUFFER_DTS (buf) = GST_TSTAMP (seq);
  gst_rtp_buffer_unmap (&rtp);

  return buf;
}

GST_START_TEST (rtpstorage_up_and_down)
{
  GstHarness *h = gst_harness_new ("rtpstorage");
  gst_harness_set_src_caps_str (h, "application/x-rtp");
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_resize)
{
  guint i, j;
  GstBuffer *bufin, *bufout, *bufs[10];
  GstHarness *h = gst_harness_new ("rtpstorage");

  gst_harness_set_src_caps_str (h, "application/x-rtp");

  g_object_set (h->element, "size-time", (guint64) 0, NULL);
  bufin = create_rtp_packet (96, 0xabe2b0b, 0x111111, 0);
  bufout = gst_harness_push_and_pull (h, bufin);
  fail_unless (bufin == bufout);
  fail_unless (gst_buffer_is_writable (bufout));

  g_object_set (h->element,
      "size-time", (guint64) (G_N_ELEMENTS (bufs) - 1) * RTP_PACKET_DUR, NULL);

  // Pushing 10 buffers all of them should have ref. count =2
  for (i = 0; i < G_N_ELEMENTS (bufs); ++i) {
    bufs[i] =
        gst_harness_push_and_pull (h, create_rtp_packet (96, 0xabe2b0b,
            0x111111, i));
    for (j = 0; j <= i; ++j)
      fail_unless (!gst_buffer_is_writable (bufs[j]));
  }

  // The next 10 buffers should expel the first 10
  for (i = 0; i < G_N_ELEMENTS (bufs); ++i) {
    gst_buffer_unref (gst_harness_push_and_pull (h, create_rtp_packet (96,
                0xabe2b0b, 0x111111, G_N_ELEMENTS (bufs) + i)));
    for (j = 0; j <= i; ++j)
      fail_unless (gst_buffer_is_writable (bufs[j]));
  }

  for (i = 0; i < G_N_ELEMENTS (bufs); ++i)
    gst_buffer_unref (bufs[i]);
  gst_buffer_unref (bufout);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_stop_redundant_packets)
{
  GstHarness *h = gst_harness_new ("rtpstorage");
  GstBuffer *bufinp;

  g_object_set (h->element, "size-time", (guint64) 2 * RTP_PACKET_DUR, NULL);
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  bufinp = create_rtp_packet (96, 0xabe2b0b, 0x111111, 0);
  GST_BUFFER_FLAG_SET (bufinp, GST_RTP_BUFFER_FLAG_REDUNDANT);
  gst_harness_push (h, bufinp);

  gst_buffer_unref (gst_harness_push_and_pull (h, create_rtp_packet (96,
              0xabe2b0b, 0x111111, 1)));

  fail_unless_equals_int (gst_harness_buffers_received (h), 1);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_unknown_ssrc)
{
  GstBufferList *bufs_out;
  GstHarness *h = gst_harness_new ("rtpstorage");
  g_object_set (h->element, "size-time", (guint64) RTP_PACKET_DUR, NULL);
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  /* No packets has been pushed through yet */
  bufs_out = get_packets_for_recovery (h, 100, 0xabe2b0b, 0);
  fail_unless (NULL == bufs_out);

  /* 1 packet with ssrc=0xabe2bob pushed. Asking for ssrc=0xdeadbeef */
  gst_buffer_unref (gst_harness_push_and_pull (h, create_rtp_packet (96,
              0xabe2b0b, 0x111111, 0)));
  bufs_out = get_packets_for_recovery (h, 100, 0xdeadbeef, 0);
  fail_unless (NULL == bufs_out);

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_packet_not_lost)
{
  GstBuffer *buf;
  GstBufferList *bufs_out;
  GstHarness *h = gst_harness_new ("rtpstorage");
  g_object_set (h->element, "size-time", (guint64) 10 * RTP_PACKET_DUR, NULL);
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  /* Pushing through 2 frames + 2 FEC */
  gst_buffer_unref (gst_harness_push_and_pull (h, create_rtp_packet (96,
              0xabe2b0b, RTP_TSTAMP (0), 0)));
  gst_buffer_unref (gst_harness_push_and_pull (h, (buf =
              create_rtp_packet (96, 0xabe2b0b, RTP_TSTAMP (1), 1))));
  gst_buffer_unref (gst_harness_push_and_pull (h, create_rtp_packet (96,
              0xabe2b0b, RTP_TSTAMP (1), 2)));
  gst_buffer_unref (gst_harness_push_and_pull (h, create_rtp_packet (96,
              0xabe2b0b, RTP_TSTAMP (1), 3)));

  /* Asking for a packet which was pushed before */
  bufs_out = get_packets_for_recovery (h, 100, 0xabe2b0b, 1);
  fail_unless (NULL != bufs_out);
  fail_unless_equals_int (1, gst_buffer_list_length (bufs_out));
  fail_unless (gst_buffer_list_get (bufs_out, 0) == buf);

  gst_buffer_list_unref (bufs_out);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtpstorage_put_recovered_packet)
{
  GstBuffer *bufs_in[4];
  GstBufferList *bufs_out;
  GstHarness *h = gst_harness_new ("rtpstorage");
  g_object_set (h->element, "size-time", (guint64) 10 * RTP_PACKET_DUR, NULL);
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  /* Pushing through 2 frames + 2 FEC
   * Packets with sequence numbers 1 and 2 are lost */
  bufs_in[0] = create_rtp_packet (96, 0xabe2b0b, RTP_TSTAMP (0), 0);
  bufs_in[1] = NULL;
  bufs_in[2] = NULL;
  bufs_in[3] = create_rtp_packet (100, 0xabe2b0b, RTP_TSTAMP (1), 3);
  gst_buffer_unref (gst_harness_push_and_pull (h, bufs_in[0]));
  gst_buffer_unref (gst_harness_push_and_pull (h, bufs_in[3]));

  /* 1 more frame + 1 FEC */
  gst_buffer_unref (gst_harness_push_and_pull (h, create_rtp_packet (96,
              0xabe2b0b, RTP_TSTAMP (2), 4)));
  gst_buffer_unref (gst_harness_push_and_pull (h, create_rtp_packet (100,
              0xabe2b0b, RTP_TSTAMP (2), 5)));

  /* Asking for the lost packet seq=1 */
  bufs_out = get_packets_for_recovery (h, 100, 0xabe2b0b, 1);
  fail_unless (NULL != bufs_out);
  fail_unless_equals_int (2, gst_buffer_list_length (bufs_out));
  fail_unless (gst_buffer_list_get (bufs_out, 0) == bufs_in[0]);
  fail_unless (gst_buffer_list_get (bufs_out, 1) == bufs_in[3]);
  gst_buffer_list_unref (bufs_out);

  /* During recovery the packet of a new frame has arrived */
  gst_buffer_unref (gst_harness_push_and_pull (h, create_rtp_packet (96,
              0xabe2b0b, RTP_TSTAMP (3), 6)));

  /* Say we recovered packet with seq=1 and put it back in the storage */
  bufs_in[1] = create_rtp_packet (96, 0xabe2b0b, RTP_TSTAMP (1), 1);
  put_recovered_packet (h, bufs_in[1], 96, 0xabe2b0b, 1);

  /* Asking for the lost packet seq=2 */
  bufs_out = get_packets_for_recovery (h, 100, 0xabe2b0b, 2);
  fail_unless (NULL != bufs_out);
  fail_unless_equals_int (3, gst_buffer_list_length (bufs_out));
  fail_unless (gst_buffer_list_get (bufs_out, 0) == bufs_in[0]);
  fail_unless (gst_buffer_list_get (bufs_out, 1) == bufs_in[1]);
  fail_unless (gst_buffer_list_get (bufs_out, 2) == bufs_in[3]);
  gst_buffer_list_unref (bufs_out);

  gst_harness_teardown (h);
}

GST_END_TEST;


static void
_single_ssrc_test (GstHarness * h, guint32 ssrc,
    guint16 seq_start, guint16 nth_to_loose,
    gsize expected_buf_size, gsize expected_first_buffer_idx)
{
  guint i;
  GPtrArray *bufs_in =
      g_ptr_array_new_with_free_func ((GDestroyNotify) gst_buffer_unref);
  GstBufferList *bufs_out;

  /* 2 frames + 2 FEC */
  g_ptr_array_add (bufs_in, create_rtp_packet (96, ssrc, RTP_TSTAMP (0),
          seq_start + 0));
  g_ptr_array_add (bufs_in, create_rtp_packet (96, ssrc, RTP_TSTAMP (1),
          seq_start + 1));
  g_ptr_array_add (bufs_in, create_rtp_packet (100, ssrc, RTP_TSTAMP (1),
          seq_start + 2));
  g_ptr_array_add (bufs_in, create_rtp_packet (100, ssrc, RTP_TSTAMP (1),
          seq_start + 3));
  /* 3 frames + 2 FEC */
  g_ptr_array_add (bufs_in, create_rtp_packet (96, ssrc, RTP_TSTAMP (2),
          seq_start + 4));
  g_ptr_array_add (bufs_in, create_rtp_packet (96, ssrc, RTP_TSTAMP (3),
          seq_start + 5));
  g_ptr_array_add (bufs_in, create_rtp_packet (96, ssrc, RTP_TSTAMP (4),
          seq_start + 6));
  g_ptr_array_add (bufs_in, create_rtp_packet (100, ssrc, RTP_TSTAMP (4),
          seq_start + 7));
  g_ptr_array_add (bufs_in, create_rtp_packet (100, ssrc, RTP_TSTAMP (4),
          seq_start + 8));
  g_ptr_array_add (bufs_in, create_rtp_packet (100, ssrc, RTP_TSTAMP (4),
          seq_start + 9));
  /* 2 frames + no FEC */
  g_ptr_array_add (bufs_in, create_rtp_packet (96, ssrc, RTP_TSTAMP (5),
          seq_start + 10));
  g_ptr_array_add (bufs_in, create_rtp_packet (96, ssrc, RTP_TSTAMP (6),
          seq_start + 11));

  /* Losing one */
  g_ptr_array_remove_index (bufs_in, nth_to_loose);

  /* Push all of them through */
  for (i = 0; i < bufs_in->len; ++i)
    gst_buffer_unref (gst_harness_push_and_pull (h,
            gst_buffer_ref (g_ptr_array_index (bufs_in, i))));

  bufs_out =
      get_packets_for_recovery (h, 100, ssrc,
      (guint16) (seq_start + nth_to_loose));
  if (0 == expected_buf_size) {
    fail_unless (NULL == bufs_out);
  } else {
    fail_unless (NULL != bufs_out);
    fail_unless_equals_int (expected_buf_size,
        gst_buffer_list_length (bufs_out));
    for (i = 0; i < gst_buffer_list_length (bufs_out); ++i)
      fail_unless (gst_buffer_list_get (bufs_out, i) ==
          g_ptr_array_index (bufs_in, expected_first_buffer_idx + i));
    gst_buffer_list_unref (bufs_out);
  }
  g_ptr_array_unref (bufs_in);
}

static void
_multiple_ssrcs_test (guint16 nth_to_loose,
    gsize expected_buf_size, gsize expected_first_buffer_idx)
{
  guint16 stream0_seq_start = 200;
  guint16 stream1_seq_start = 65529;
  GstHarness *h = gst_harness_new ("rtpstorage");
  g_object_set (h->element, "size-time", (guint64) 12 * RTP_PACKET_DUR, NULL);
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  _single_ssrc_test (h, 0x0abe2b0b, stream0_seq_start,
      nth_to_loose, expected_buf_size, expected_first_buffer_idx);
  _single_ssrc_test (h, 0xdeadbeef, stream1_seq_start,
      nth_to_loose, expected_buf_size, expected_first_buffer_idx);

  gst_harness_teardown (h);
}

GST_START_TEST (rtpstorage_loss_pattern0)
{
  _multiple_ssrcs_test (1, 3, 0);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_loss_pattern1)
{
  _multiple_ssrcs_test (2, 3, 0);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_loss_pattern2)
{
  _multiple_ssrcs_test (3, 6, 3);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_loss_pattern3)
{
  _multiple_ssrcs_test (4, 5, 4);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_loss_pattern4)
{
  _multiple_ssrcs_test (5, 5, 4);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_loss_pattern5)
{
  _multiple_ssrcs_test (6, 5, 4);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_loss_pattern6)
{
  _multiple_ssrcs_test (7, 5, 4);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_loss_pattern7)
{
  _multiple_ssrcs_test (8, 5, 4);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_loss_pattern8)
{
  _multiple_ssrcs_test (9, 0, 0);
}

GST_END_TEST;

GST_START_TEST (rtpstorage_loss_pattern9)
{
  _multiple_ssrcs_test (10, 0, 0);
}

GST_END_TEST;

#define STRESS_TEST_SSRCS (8)
#define STRESS_TEST_STORAGE_DEPTH (50)
typedef struct _StressTestData StressTestData;
struct _StressTestData
{
  guint16 seq[STRESS_TEST_SSRCS];
  guint32 ssrc[STRESS_TEST_SSRCS];
  gsize count[STRESS_TEST_SSRCS];
  GRand *rnd;
};

static GstBuffer *
rtpstorage_stress_prepare_buffer (GstHarness * h, gpointer data)
{
  static const guint8 fec_pt = 100;
  static const guint8 media_pt = 96;
  StressTestData *test_data = data;
  gsize ssrc_idx = g_rand_int_range (test_data->rnd, 0, STRESS_TEST_SSRCS);
  guint16 seq = test_data->seq[ssrc_idx];
  guint32 ssrc = test_data->ssrc[ssrc_idx];
  gboolean is_fec = test_data->count[ssrc_idx] > 0 && (seq % 5 == 0
      || seq % 5 == 1);
  guint8 pt = is_fec ? fec_pt : media_pt;

  GstBuffer *buf = create_rtp_packet (pt, ssrc, RTP_TSTAMP (0), seq);

  ++test_data->seq[ssrc_idx];
  ++test_data->count[ssrc_idx];
  return buf;
}

GST_START_TEST (rtpstorage_stress)
{
  GRand *rnd;
  GTimer *timer;
  GstCaps *caps;
  GstSegment segment;
  GstHarnessThread *ht;
  StressTestData test_data;
  guint seed, i, total, requested;
  GstHarness *h = gst_harness_new ("rtpstorage");
  g_object_set (h->element,
      "size-time", (guint64) STRESS_TEST_STORAGE_DEPTH * RTP_PACKET_DUR, NULL);

  /* The stress test pushes buffers with STRESS_TEST_SSRCS different
   * ssrcs from one thread and requests packets for FEC recovery from
   * another thread.
   * */
  memset (&test_data, 0, sizeof (test_data));
  seed = g_random_int ();
  test_data.rnd = g_rand_new_with_seed (seed);
  for (i = 0; i < STRESS_TEST_SSRCS; ++i) {
    test_data.ssrc[i] = 0x00112233 + i * 0x01000000;
    test_data.seq[i] = g_rand_int_range (test_data.rnd, 0, 0x10000);
  }

  gst_segment_init (&segment, GST_FORMAT_TIME);
  caps = gst_caps_from_string ("application/x-rtp");
  rnd = g_rand_copy (test_data.rnd);

  GST_INFO ("%u seed", seed);
  ht = gst_harness_stress_push_buffer_with_cb_start (h, caps, &segment,
      rtpstorage_stress_prepare_buffer, &test_data, NULL);

  requested = 0;
  timer = g_timer_new ();
  while (g_timer_elapsed (timer, NULL) < 2) {
    gsize ssrc_idx = g_rand_int_range (rnd, 0, STRESS_TEST_SSRCS);

    /* The following if statement is simply keeping the log
     * clean from ERROR messages */
    if (*((volatile gsize *) &test_data.count[ssrc_idx]) > 1) {
      guint16 lost_seq = *((volatile guint16 *) &test_data.seq[ssrc_idx]) - 5;

      GstBufferList *bufs_out = get_packets_for_recovery (h, 100,
          test_data.ssrc[ssrc_idx], lost_seq);
      if (bufs_out) {
        requested += gst_buffer_list_length (bufs_out);
        gst_buffer_list_unref (bufs_out);
      }
    }

    /* Having sleep here makes it hard to detect the race, but we need it to
     * allow another thread to push more buffers when running under valgrind */
    g_usleep (G_USEC_PER_SEC / 10000);
  }

  gst_harness_stress_thread_stop (ht);
  for (i = 0, total = 0; i < STRESS_TEST_SSRCS; ++i) {
    GST_INFO ("SSRC 0x%08x: %u packets", test_data.ssrc[i],
        (guint32) test_data.count[i]);
    total += test_data.count[i];
  }
  GST_INFO ("%u packets pushed through, %u requested", total, requested);

  g_rand_free (rnd);
  g_rand_free (test_data.rnd);
  gst_caps_unref (caps);
  g_timer_destroy (timer);
  gst_harness_teardown (h);
}

GST_END_TEST;

static Suite *
rtpstorage_suite (void)
{
  Suite *s = suite_create ("rtpstorage");
  TCase *tc_chain = tcase_create ("general");

  gst_element_register (NULL, "rtpstorage", GST_RANK_NONE,
      GST_TYPE_RTP_STORAGE);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, rtpstorage_up_and_down);
  tcase_add_test (tc_chain, rtpstorage_resize);
  tcase_add_test (tc_chain, rtpstorage_stop_redundant_packets);
  tcase_add_test (tc_chain, rtpstorage_unknown_ssrc);
  tcase_add_test (tc_chain, rtpstorage_packet_not_lost);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern0);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern1);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern2);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern3);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern4);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern5);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern6);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern7);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern8);
  tcase_add_test (tc_chain, rtpstorage_loss_pattern9);
  tcase_add_test (tc_chain, test_rtpstorage_put_recovered_packet);
  tcase_add_test (tc_chain, rtpstorage_stress);

  return s;
}

GST_CHECK_MAIN (rtpstorage)
