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

#include <gst/check/gstharness.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/check/gstcheck.h>

#define PT_RED 100
#define PT_MEDIA 96
#define CLOCKRATE 8000
#define TIMESTAMP_BASE (1000)
#define TIMESTAMP_DIFF (40 * CLOCKRATE / 1000)
#define TIMESTAMP_NTH(i) (TIMESTAMP_BASE + (i) * TIMESTAMP_DIFF)
#define xstr(s) str(s)
#define str(s) #s
#define GST_RTP_RED_ENC_CAPS_STR "application/x-rtp, payload=" xstr(PT_MEDIA)

#define _check_red_received(h, expected)                     \
  G_STMT_START {                                             \
    guint received;                                          \
    g_object_get ((h)->element, "received", &received, NULL);\
    fail_unless_equals_int (expected, received);             \
  } G_STMT_END

#define _check_red_sent(h, expected)                 \
  G_STMT_START {                                     \
    guint sent;                                      \
    g_object_get ((h)->element, "sent", &sent, NULL);\
    fail_unless_equals_int (expected, sent);         \
  } G_STMT_END

#define _check_caps(_h_, _nth_, _expected_payload_)               \
  G_STMT_START {                                                  \
    GstEvent *_ev_;                                               \
    gint _pt_ = -1, _i_;                                          \
    GstCaps *_caps_ = NULL;                                       \
                                                                  \
    for (_i_ = 0; _i_ < _nth_; ++_i_)                             \
      gst_event_unref (gst_harness_pull_event (_h_));             \
                                                                  \
    _ev_ = gst_harness_pull_event (_h_);                          \
    fail_unless (NULL != _ev_);                                   \
    fail_unless_equals_string ("caps", GST_EVENT_TYPE_NAME(_ev_));\
                                                                  \
    gst_event_parse_caps (_ev_, &_caps_);                         \
                                                                  \
    gst_structure_get_int (                                       \
        gst_caps_get_structure (_caps_, 0), "payload", &_pt_);    \
    fail_unless_equals_int (_expected_payload_, _pt_);            \
    gst_event_unref (_ev_);                                       \
  } G_STMT_END

#define _check_nocaps(_h_)                                     \
  G_STMT_START {                                               \
    GstEvent *_ev_;                                            \
    while (NULL != (_ev_ = gst_harness_try_pull_event (_h_))) {\
      fail_unless (GST_EVENT_TYPE (_ev_) != GST_EVENT_CAPS,    \
          "Don't expect to receive caps event");               \
      gst_event_unref (_ev_);                                  \
    }                                                          \
  } G_STMT_END

static GstBuffer *
_new_rtp_buffer (gboolean marker, guint8 csrc_count, guint8 pt, guint16 seqnum,
    guint32 timestamp, guint32 ssrc, guint payload_len)
{
  GstBuffer *buf = gst_rtp_buffer_new_allocate (payload_len, 0, csrc_count);
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  fail_unless (gst_rtp_buffer_map (buf, GST_MAP_READ, &rtp));
  gst_rtp_buffer_set_marker (&rtp, marker);
  gst_rtp_buffer_set_payload_type (&rtp, pt);
  gst_rtp_buffer_set_seq (&rtp, seqnum);
  gst_rtp_buffer_set_timestamp (&rtp, timestamp);
  gst_rtp_buffer_set_ssrc (&rtp, ssrc);
  gst_rtp_buffer_unmap (&rtp);

  return buf;
}

GST_START_TEST (rtpreddec_passthrough)
{
  GstBuffer *bufinp, *bufout;
  GstHarness *h = gst_harness_new ("rtpreddec");
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  /* Passthrough when pt is not set */
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 0, TIMESTAMP_NTH (0), 0xabe2b0b, 0);
  bufout = gst_harness_push_and_pull (h, bufinp);
  fail_unless (bufout == bufinp);
  fail_unless (gst_buffer_is_writable (bufout));
  gst_buffer_unref (bufout);

  /* Now pt is set */
  g_object_set (h->element, "pt", PT_RED, NULL);

  /* Passthrough when not RED. RED pt = 100, pushing pt 99 */
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_MEDIA, 1, TIMESTAMP_NTH (1), 0xabe2b0b, 0);
  bufout = gst_harness_push_and_pull (h, bufinp);
  fail_unless (bufout == bufinp);
  fail_unless (gst_buffer_is_writable (bufout));
  gst_buffer_unref (bufout);

  /* Passthrough when not RTP buffer */
  bufinp = gst_buffer_new_wrapped (g_strdup ("hello"), 5);
  bufout = gst_harness_push_and_pull (h, bufinp);
  fail_unless (bufout == bufinp);
  fail_unless (gst_buffer_is_writable (bufout));
  gst_buffer_unref (bufout);

  _check_red_received (h, 0);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpreddec_main_block)
{
  GstHarness *h = gst_harness_new ("rtpreddec");
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint8 out_data[] = { 0xa, 0xa, 0xa, 0xa, 0xa };
  guint8 red_in[] = { PT_MEDIA, 0xa, 0xa, 0xa, 0xa, 0xa };
  guint gst_ts = 3454679;
  guint csrc_count = 2;
  guint seq = 549;
  GstBuffer *bufinp, *bufout;
  guint bufinp_flags;

  g_object_set (h->element, "pt", PT_RED, NULL);
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  /* RED buffer has Marker bit set, has CSRCS and flags */
  bufinp =
      _new_rtp_buffer (TRUE, csrc_count, PT_RED, seq, TIMESTAMP_NTH (0),
      0xabe2b0b, sizeof (red_in));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &red_in, sizeof (red_in));
  gst_rtp_buffer_set_csrc (&rtp, 0, 0x1abe2b0b);
  gst_rtp_buffer_set_csrc (&rtp, 1, 0x2abe2b0b);
  GST_BUFFER_TIMESTAMP (bufinp) = gst_ts;
  GST_BUFFER_FLAG_SET (bufinp, GST_RTP_BUFFER_FLAG_RETRANSMISSION);
  GST_BUFFER_FLAG_SET (bufinp, GST_BUFFER_FLAG_DISCONT);
  bufinp_flags = GST_BUFFER_FLAGS (bufinp);
  gst_rtp_buffer_unmap (&rtp);

  /* Checking that pulled buffer has keeps everything from RED buffer */
  bufout = gst_harness_push_and_pull (h, bufinp);
  fail_unless (gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtp));
  fail_unless_equals_int (GST_BUFFER_TIMESTAMP (bufout), gst_ts);
  fail_unless_equals_int (GST_BUFFER_FLAGS (bufout), bufinp_flags);
  fail_unless_equals_int (gst_buffer_get_size (bufout),
      gst_rtp_buffer_calc_packet_len (sizeof (out_data), 0, csrc_count));
  fail_unless_equals_int (gst_rtp_buffer_get_timestamp (&rtp),
      TIMESTAMP_NTH (0));
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), PT_MEDIA);
  fail_unless_equals_int (gst_rtp_buffer_get_seq (&rtp), seq);
  fail_unless_equals_int (gst_rtp_buffer_get_csrc_count (&rtp), csrc_count);
  fail_unless_equals_int (gst_rtp_buffer_get_ssrc (&rtp), 0x0abe2b0b);
  fail_unless_equals_int (gst_rtp_buffer_get_csrc (&rtp, 0), 0x1abe2b0b);
  fail_unless_equals_int (gst_rtp_buffer_get_csrc (&rtp, 1), 0x2abe2b0b);
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  fail_unless (!memcmp (gst_rtp_buffer_get_payload (&rtp), out_data,
          sizeof (out_data)));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (bufout);

  _check_red_received (h, 1);
  gst_harness_teardown (h);
}

GST_END_TEST;

static void
_push_and_check_didnt_go_through (GstHarness * h, GstBuffer * bufinp)
{
  gst_harness_push (h, bufinp);
  /* Making sure it didn't go through */
  fail_unless_equals_int (gst_harness_buffers_received (h), 0);
}

static void
_push_and_check_cant_pull_twice (GstHarness * h,
    GstBuffer * bufinp, guint buffers_received)
{
  gst_buffer_unref (gst_harness_push_and_pull (h, bufinp));
  /* Making sure only one buffer was pushed through */
  fail_unless_equals_int (gst_harness_buffers_received (h), buffers_received);
}

static void
_push_and_check_redundant_packet (GstHarness * h, GstBuffer * bufinp,
    guint seq, guint timestamp, guint payload_len, gconstpointer payload)
{
  GstBuffer *bufout = gst_harness_push_and_pull (h, bufinp);
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  fail_unless (gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtp));
  fail_unless (GST_BUFFER_FLAG_IS_SET (bufout, GST_RTP_BUFFER_FLAG_REDUNDANT));
  fail_unless_equals_int (gst_buffer_get_size (bufout),
      gst_rtp_buffer_calc_packet_len (payload_len, 0, 0));
  fail_unless_equals_int (gst_rtp_buffer_get_timestamp (&rtp), timestamp);
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), PT_MEDIA);
  fail_unless_equals_int (gst_rtp_buffer_get_seq (&rtp), seq);
  fail_unless_equals_int (gst_rtp_buffer_get_ssrc (&rtp), 0x0abe2b0b);
  fail_unless (!memcmp (gst_rtp_buffer_get_payload (&rtp), payload,
          payload_len));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (bufout);
  gst_buffer_unref (gst_harness_pull (h));
}

GST_START_TEST (rtpreddec_redundant_block_not_pushed)
{
  GstHarness *h = gst_harness_new ("rtpreddec");
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  /* Redundant block has valid tsoffset but we have not seen any buffers before */
  guint16 ts_offset = TIMESTAMP_DIFF;
  guint8 red_in[] = {
    0x80 | PT_MEDIA,
    (guint8) (ts_offset >> 6),
    (guint8) (ts_offset & 0x3f) << 2, 1,        /* Redundant block size = 1 */
    PT_MEDIA, 0xa, 0xa          /* Main block size = 1 */
  };
  GstBuffer *bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 2, TIMESTAMP_NTH (2), 0xabe2b0b,
      sizeof (red_in));

  g_object_set (h->element, "pt", PT_RED, NULL);
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &red_in, sizeof (red_in));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_cant_pull_twice (h, bufinp, 1);

  /* Redundant block has too large tsoffset */
  ts_offset = TIMESTAMP_DIFF * 4;
  red_in[1] = ts_offset >> 6;
  red_in[2] = (ts_offset & 0x3f) << 2;
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 3, TIMESTAMP_NTH (3), 0xabe2b0b,
      sizeof (red_in));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &red_in, sizeof (red_in));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_cant_pull_twice (h, bufinp, 2);

  /* TS offset is too small */
  ts_offset = TIMESTAMP_DIFF / 2;
  red_in[1] = ts_offset >> 6;
  red_in[2] = (ts_offset & 0x3f) << 2;
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 4, TIMESTAMP_NTH (4), 0xabe2b0b,
      sizeof (red_in));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &red_in, sizeof (red_in));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_cant_pull_twice (h, bufinp, 3);

  /* Now we ts_offset points to the previous buffer we didn't loose */
  ts_offset = TIMESTAMP_DIFF;
  red_in[1] = ts_offset >> 6;
  red_in[2] = (ts_offset & 0x3f) << 2;
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 5, TIMESTAMP_NTH (5), 0xabe2b0b,
      sizeof (red_in));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &red_in, sizeof (red_in));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_cant_pull_twice (h, bufinp, 4);

  _check_red_received (h, 4);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpreddec_redundant_block_pushed)
{
  GstHarness *h = gst_harness_new ("rtpreddec");
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint16 ts_offset = TIMESTAMP_DIFF;
  guint8 red_in[] = {
    0x80 | PT_MEDIA,
    (guint8) (ts_offset >> 6),
    (guint8) (ts_offset & 0x3f) << 2, 5,        /* Redundant block size = 5 */
    PT_MEDIA, 0x01, 0x02, 0x03, 0x4, 0x5, 0xa   /* Main block size = 1 */
  };
  GstBuffer *bufinp;

  g_object_set (h->element, "pt", PT_RED, NULL);
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  /* Pushing seq=0 */
  gst_buffer_unref (gst_harness_push_and_pull (h, _new_rtp_buffer (FALSE, 0,
              PT_MEDIA, 0, TIMESTAMP_NTH (0), 0xabe2b0b, 0)));

  /* Pushing seq=2, recovering seq=1 (fec distance 1) */

  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 2, TIMESTAMP_NTH (2), 0xabe2b0b,
      sizeof (red_in));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &red_in, sizeof (red_in));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_redundant_packet (h, bufinp, 1, TIMESTAMP_NTH (1), 5,
      red_in + 5);

  /* Pushing seq=5, recovering seq=3 (fec distance 2) */
  ts_offset = TIMESTAMP_DIFF * 2;
  red_in[1] = ts_offset >> 6;
  red_in[2] = (ts_offset & 0x3f) << 2;
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 5, TIMESTAMP_NTH (5), 0xabe2b0b,
      sizeof (red_in));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &red_in, sizeof (red_in));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_redundant_packet (h, bufinp, 3, TIMESTAMP_NTH (3), 5,
      red_in + 5);

  /* Pushing seq=9, recovering seq=6 (fec distance 3) */
  ts_offset = TIMESTAMP_DIFF * 3;
  red_in[1] = ts_offset >> 6;
  red_in[2] = (ts_offset & 0x3f) << 2;
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 9, TIMESTAMP_NTH (9), 0xabe2b0b,
      sizeof (red_in));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &red_in, sizeof (red_in));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_redundant_packet (h, bufinp, 6, TIMESTAMP_NTH (6), 5,
      red_in + 5);

  /* Pushing seq=14, recovering seq=10 (fec distance 4) */
  ts_offset = TIMESTAMP_DIFF * 4;
  red_in[1] = ts_offset >> 6;
  red_in[2] = (ts_offset & 0x3f) << 2;
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 14, TIMESTAMP_NTH (14), 0xabe2b0b,
      sizeof (red_in));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &red_in, sizeof (red_in));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_redundant_packet (h, bufinp, 10, TIMESTAMP_NTH (10), 5,
      red_in + 5);

  _check_red_received (h, 4);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpreddec_invalid)
{
  GstBuffer *bufinp;
  GstHarness *h = gst_harness_new ("rtpreddec");
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  /* 2 block RED packets should have at least 4 bytes for redundant block
   * header and 1 byte for the main block header. */
  guint8 data[] = {
    0x80 | PT_MEDIA, 0, 0, 1,   /* 1st block header (redundant block) size=1, timestmapoffset=0 */
    PT_MEDIA,                   /* 2nd block header (main block) size=0 */
  };

  g_object_set (h->element, "pt", PT_RED, NULL);
  gst_harness_set_src_caps_str (h, "application/x-rtp");

  /* Single block RED packets should have at least 1 byte of payload to be
   * considered valid. This buffer does not have any payload */
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 0, TIMESTAMP_NTH (0), 0xabe2b0b, 0);
  _push_and_check_didnt_go_through (h, bufinp);

  /* Only the first byte with F bit set (indication of redundant block) */
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 1, TIMESTAMP_NTH (1), 0xabe2b0b, 1);
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &data, sizeof (data));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_didnt_go_through (h, bufinp);

  /* Full 1st block header only */
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 2, TIMESTAMP_NTH (2), 0xabe2b0b, 4);
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &data, sizeof (data));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_didnt_go_through (h, bufinp);

  /* Both blocks, missing 1 byte of payload for redundant block */
  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_RED, 3, TIMESTAMP_NTH (3), 0xabe2b0b, 5);
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &data, sizeof (data));
  gst_rtp_buffer_unmap (&rtp);
  _push_and_check_didnt_go_through (h, bufinp);

  _check_red_received (h, 4);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpredenc_passthrough)
{
  GstBuffer *bufinp, *bufout;
  GstHarness *h = gst_harness_new ("rtpredenc");

  g_object_set (h->element, "allow-no-red-blocks", FALSE, NULL);
  gst_harness_set_src_caps_str (h, GST_RTP_RED_ENC_CAPS_STR);

  bufinp =
      _new_rtp_buffer (FALSE, 0, PT_MEDIA, 0, TIMESTAMP_NTH (0), 0xabe2b0b, 0);
  bufout = gst_harness_push_and_pull (h, bufinp);

  _check_caps (h, 1, PT_MEDIA);
  fail_unless (bufout == bufinp);
  fail_unless (gst_buffer_is_writable (bufout));
  gst_buffer_unref (bufout);

  /* Setting pt and allowing RED packets without redundant blocks */
  g_object_set (h->element, "pt", PT_RED, "allow-no-red-blocks", TRUE, NULL);

  /* Passthrough when not RTP buffer */
  bufinp = gst_buffer_new_wrapped (g_strdup ("hello"), 5);
  bufout = gst_harness_push_and_pull (h, bufinp);

  _check_nocaps (h);
  fail_unless (bufout == bufinp);
  fail_unless (gst_buffer_is_writable (bufout));
  gst_buffer_unref (bufout);

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpredenc_payloadless_rtp)
{
  GstHarness *h = gst_harness_new ("rtpredenc");
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint8 out_data[] = { PT_MEDIA };
  GstBuffer *bufout;

  g_object_set (h->element, "pt", PT_RED, "allow-no-red-blocks", TRUE, NULL);
  gst_harness_set_src_caps_str (h, GST_RTP_RED_ENC_CAPS_STR);

  bufout =
      gst_harness_push_and_pull (h, _new_rtp_buffer (TRUE, 0, PT_MEDIA, 0,
          TIMESTAMP_NTH (0), 0xabe2b0b, 0));

  _check_caps (h, 1, PT_RED);
  fail_unless (gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtp));
  fail_unless_equals_int (gst_buffer_get_size (bufout),
      gst_rtp_buffer_calc_packet_len (sizeof (out_data), 0, 0));
  fail_unless_equals_int (gst_rtp_buffer_get_timestamp (&rtp),
      TIMESTAMP_NTH (0));
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), PT_RED);
  fail_unless_equals_int (gst_rtp_buffer_get_seq (&rtp), 0);
  fail_unless_equals_int (gst_rtp_buffer_get_csrc_count (&rtp), 0);
  fail_unless_equals_int (gst_rtp_buffer_get_ssrc (&rtp), 0x0abe2b0b);
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  fail_unless (!memcmp (gst_rtp_buffer_get_payload (&rtp), out_data,
          sizeof (out_data)));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (bufout);

  _check_red_sent (h, 1);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpredenc_without_redundant_block)
{
  GstHarness *h = gst_harness_new ("rtpredenc");
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint8 in_data[] = { 0xa, 0xa, 0xa, 0xa, 0xa };
  guint8 out_data[] = { PT_MEDIA, 0xa, 0xa, 0xa, 0xa, 0xa };
  guint gst_ts = 3454679;
  guint csrc_count = 2;
  guint seq = 549;
  guint bufinp_flags;
  GstBuffer *bufinp, *bufout;

  g_object_set (h->element, "pt", PT_RED, "allow-no-red-blocks", TRUE, NULL);
  gst_harness_set_src_caps_str (h, GST_RTP_RED_ENC_CAPS_STR);

  /* Media buffer has Marker bit set, has CSRCS and flags */
  bufinp =
      _new_rtp_buffer (TRUE, csrc_count, PT_MEDIA, seq, TIMESTAMP_NTH (0),
      0xabe2b0b, sizeof (in_data));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data, sizeof (in_data));
  gst_rtp_buffer_set_csrc (&rtp, 0, 0x1abe2b0b);
  gst_rtp_buffer_set_csrc (&rtp, 1, 0x2abe2b0b);
  gst_rtp_buffer_unmap (&rtp);
  GST_BUFFER_TIMESTAMP (bufinp) = gst_ts;
  GST_BUFFER_FLAG_SET (bufinp, GST_RTP_BUFFER_FLAG_RETRANSMISSION);
  GST_BUFFER_FLAG_SET (bufinp, GST_BUFFER_FLAG_DISCONT);
  bufinp_flags = GST_BUFFER_FLAGS (bufinp);
  bufout = gst_harness_push_and_pull (h, bufinp);

  /* Checking that pulled buffer has keeps everything from Media buffer */
  _check_caps (h, 1, PT_RED);
  fail_unless (gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtp));
  fail_unless_equals_int (GST_BUFFER_TIMESTAMP (bufout), gst_ts);
  fail_unless_equals_int (GST_BUFFER_FLAGS (bufout), bufinp_flags);
  fail_unless_equals_int (gst_buffer_get_size (bufout),
      gst_rtp_buffer_calc_packet_len (sizeof (out_data), 0, csrc_count));
  fail_unless_equals_int (gst_rtp_buffer_get_timestamp (&rtp),
      TIMESTAMP_NTH (0));
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), PT_RED);
  fail_unless_equals_int (gst_rtp_buffer_get_seq (&rtp), seq);
  fail_unless_equals_int (gst_rtp_buffer_get_csrc_count (&rtp), csrc_count);
  fail_unless_equals_int (gst_rtp_buffer_get_ssrc (&rtp), 0x0abe2b0b);
  fail_unless_equals_int (gst_rtp_buffer_get_csrc (&rtp, 0), 0x1abe2b0b);
  fail_unless_equals_int (gst_rtp_buffer_get_csrc (&rtp, 1), 0x2abe2b0b);
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  fail_unless (!memcmp (gst_rtp_buffer_get_payload (&rtp), out_data,
          sizeof (out_data)));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (bufout);

  _check_red_sent (h, 1);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpredenc_with_redundant_block)
{
  GstHarness *h = gst_harness_new ("rtpredenc");
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint8 in_data0[] = { 0xa, 0xa, 0xa, 0xa, 0xa };
  guint8 in_data1[] = { 0xb, 0xb, 0xb, 0xb, 0xb };
  guint8 in_data2[] = { 0xc, 0xc, 0xc, 0xc, 0xc };
  guint timestmapoffset0 = TIMESTAMP_NTH (1) - TIMESTAMP_NTH (0);
  guint timestmapoffset1 = TIMESTAMP_NTH (2) - TIMESTAMP_NTH (0);
  guint8 out_data0[] = {
    /* Redundant block header */
    0x80 | PT_MEDIA,            /* F=1 | pt=PT_MEDIA */
    timestmapoffset0 >> 6,      /* timestamp hi 8 bits */
    timestmapoffset0 & 0x3f,    /* timestamp lo 6 bits | length hi = 0 */
    sizeof (in_data0),          /* length lo 8 bits */
    /* Main block header */
    PT_MEDIA,                   /* F=0 | pt=PT_MEDIA */
    /* Redundant block data */
    0xa, 0xa, 0xa, 0xa, 0xa,
    /* Main block data */
    0xb, 0xb, 0xb, 0xb, 0xb
  };

  guint8 out_data1[] = {
    /* Redundant block header */
    0x80 | PT_MEDIA,            /* F=1 | pt=PT_MEDIA */
    timestmapoffset1 >> 6,      /* timestamp hi 8 bits */
    timestmapoffset1 & 0x3f,    /* timestamp lo 6 bits | length hi = 0 */
    sizeof (in_data0),          /* length lo 8 bits */
    /* Main block header */
    PT_MEDIA,                   /* F=0 | pt=PT_MEDIA */
    /* Redundant block data */
    0xa, 0xa, 0xa, 0xa, 0xa,
    /* Main block data */
    0xc, 0xc, 0xc, 0xc, 0xc
  };
  guint seq = 549;
  GstBuffer *bufinp, *bufout;

  g_object_set (h->element,
      "pt", PT_RED, "distance", 2, "allow-no-red-blocks", FALSE, NULL);
  gst_harness_set_src_caps_str (h, GST_RTP_RED_ENC_CAPS_STR);

  bufinp =
      _new_rtp_buffer (TRUE, 0, PT_MEDIA, seq, TIMESTAMP_NTH (0), 0xabe2b0b,
      sizeof (in_data0));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data0, sizeof (in_data0));
  gst_rtp_buffer_unmap (&rtp);
  bufout = gst_harness_push_and_pull (h, bufinp);

  /* The first buffer should go through,
   * there were no redundant data to create RED packet */
  _check_caps (h, 1, PT_MEDIA);
  fail_unless (bufout == bufinp);
  fail_unless (gst_buffer_is_writable (bufout));
  fail_unless (gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtp));
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), PT_MEDIA);
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (bufout);

  bufinp =
      _new_rtp_buffer (TRUE, 0, PT_MEDIA, seq + 1, TIMESTAMP_NTH (1), 0xabe2b0b,
      sizeof (in_data1));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data1, sizeof (in_data1));
  gst_rtp_buffer_unmap (&rtp);
  bufout = gst_harness_push_and_pull (h, bufinp);

  /* The next buffer is RED referencing previous packet */
  _check_caps (h, 1, PT_RED);
  fail_unless (gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtp));
  fail_unless_equals_int (gst_buffer_get_size (bufout),
      gst_rtp_buffer_calc_packet_len (sizeof (out_data0), 0, 0));
  fail_unless_equals_int (gst_rtp_buffer_get_timestamp (&rtp),
      TIMESTAMP_NTH (1));
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), PT_RED);
  fail_unless_equals_int (gst_rtp_buffer_get_seq (&rtp), seq + 1);
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  fail_unless (!memcmp (gst_rtp_buffer_get_payload (&rtp), out_data0,
          sizeof (out_data0)));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (bufout);

  bufinp =
      _new_rtp_buffer (TRUE, 0, PT_MEDIA, seq + 2, TIMESTAMP_NTH (2), 0xabe2b0b,
      sizeof (in_data2));
  fail_unless (gst_rtp_buffer_map (bufinp, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data2, sizeof (in_data2));
  gst_rtp_buffer_unmap (&rtp);
  bufout = gst_harness_push_and_pull (h, bufinp);

  /* The next buffer is RED referencing the packet before the previous */
  _check_nocaps (h);
  fail_unless (gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtp));
  fail_unless_equals_int (gst_buffer_get_size (bufout),
      gst_rtp_buffer_calc_packet_len (sizeof (out_data1), 0, 0));
  fail_unless_equals_int (gst_rtp_buffer_get_timestamp (&rtp),
      TIMESTAMP_NTH (2));
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), PT_RED);
  fail_unless_equals_int (gst_rtp_buffer_get_seq (&rtp), seq + 2);
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  fail_unless (!memcmp (gst_rtp_buffer_get_payload (&rtp), out_data1,
          sizeof (out_data1)));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (bufout);

  _check_red_sent (h, 2);
  gst_harness_teardown (h);
}

GST_END_TEST;

static void
rtpredenc_cant_create_red_packet_base_test (GstBuffer * buffer0,
    GstBuffer * buffer1)
{
  /* The test configures PexRtpRedEnc to produce RED packets only with redundant
   * blocks. The first packet we pull should not be RED just because it is the
   * very first one. The second should not be RED because it was impossible
   * to create a RED packet for varios reasons:
   * - too large redundant block size
   * - too large timestamp offset
   * - negative timestamp offset */
  GstBuffer *bufout;
  GstHarness *h = gst_harness_new ("rtpredenc");
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  g_object_set (h->element,
      "pt", PT_RED, "distance", 1, "allow-no-red-blocks", FALSE, NULL);
  gst_harness_set_src_caps_str (h, GST_RTP_RED_ENC_CAPS_STR);

  /* Checking the first pulled buffer is media packet */
  bufout = gst_harness_push_and_pull (h, buffer0);
  _check_caps (h, 1, PT_MEDIA);
  fail_unless (gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtp));
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), PT_MEDIA);
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (bufout);

  /* The next buffer should be media packet too */
  bufout = gst_harness_push_and_pull (h, buffer1);
  _check_nocaps (h);
  fail_unless (gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtp));
  fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), PT_MEDIA);
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (bufout);

  _check_red_sent (h, 0);
  gst_harness_teardown (h);
}

GST_START_TEST (rtpredenc_negative_timestamp_offset)
{
  gboolean with_warping;
  guint16 seq0, seq1;
  guint32 timestamp0, timestamp1;
  GstBuffer *buffer0, *buffer1;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint8 in_data[] = { 0xa, 0xa, 0xa, 0xa, 0xa };

  with_warping = __i__ != 0;
  timestamp0 =
      with_warping ? (0xffffffff - TIMESTAMP_DIFF / 2) : TIMESTAMP_BASE;
  timestamp1 = timestamp0 + TIMESTAMP_DIFF;
  seq0 = with_warping ? 0xffff : 0;
  seq1 = seq0 + 1;

  /* Two buffers have negative timestamp difference */
  buffer0 =
      _new_rtp_buffer (TRUE, 0, PT_MEDIA, seq0, timestamp1, 0xabe2b0b,
      sizeof (in_data));
  buffer1 =
      _new_rtp_buffer (TRUE, 0, PT_MEDIA, seq1, timestamp0, 0xabe2b0b,
      sizeof (in_data));

  fail_unless (gst_rtp_buffer_map (buffer0, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data, sizeof (in_data));
  gst_rtp_buffer_unmap (&rtp);

  fail_unless (gst_rtp_buffer_map (buffer1, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data, sizeof (in_data));
  gst_rtp_buffer_unmap (&rtp);

  rtpredenc_cant_create_red_packet_base_test (buffer0, buffer1);
}

GST_END_TEST;

GST_START_TEST (rtpredenc_too_large_timestamp_offset)
{
  gboolean with_warping;
  guint16 seq0, seq1;
  guint32 timestamp0, timestamp1, timestamp_diff;
  GstBuffer *buffer0, *buffer1;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint8 in_data[] = { 0xa, 0xa, 0xa, 0xa, 0xa };

  with_warping = __i__ != 0;
  timestamp_diff = 0x4000;
  timestamp0 =
      with_warping ? (0xffffffff - timestamp_diff / 2) : TIMESTAMP_BASE;
  timestamp1 = timestamp0 + timestamp_diff;

  seq0 = with_warping ? 0xffff : 0;
  seq1 = seq0 + 1;

  /* Two buffers have timestamp difference > 14bit long */
  buffer0 =
      _new_rtp_buffer (TRUE, 0, PT_MEDIA, seq0, timestamp0, 0xabe2b0b,
      sizeof (in_data));
  buffer1 =
      _new_rtp_buffer (TRUE, 0, PT_MEDIA, seq1, timestamp1, 0xabe2b0b,
      sizeof (in_data));
  fail_unless (gst_rtp_buffer_map (buffer0, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data, sizeof (in_data));
  gst_rtp_buffer_unmap (&rtp);

  fail_unless (gst_rtp_buffer_map (buffer1, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data, sizeof (in_data));
  gst_rtp_buffer_unmap (&rtp);

  rtpredenc_cant_create_red_packet_base_test (buffer0, buffer1);
}

GST_END_TEST;

GST_START_TEST (rtpredenc_too_large_length)
{
  gboolean with_warping;
  guint16 seq0, seq1;
  guint32 timestamp0, timestamp1;
  GstBuffer *buffer0, *buffer1;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint8 in_data0[1024] = { 0, };
  guint8 in_data1[] = { 0xa, 0xa, 0xa, 0xa, 0xa };

  with_warping = __i__ != 0;
  timestamp0 =
      with_warping ? (0xffffffff - TIMESTAMP_DIFF / 2) : TIMESTAMP_BASE;
  timestamp1 = timestamp0 + TIMESTAMP_DIFF;
  seq0 = with_warping ? 0xffff : 0;
  seq1 = seq0 + 1;

  /* The first buffer is too large to use as a redundant block */
  buffer0 =
      _new_rtp_buffer (TRUE, 0, PT_MEDIA, seq0, timestamp0, 0xabe2b0b,
      sizeof (in_data0));
  buffer1 =
      _new_rtp_buffer (TRUE, 0, PT_MEDIA, seq1, timestamp1, 0xabe2b0b,
      sizeof (in_data1));
  fail_unless (gst_rtp_buffer_map (buffer0, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data0, sizeof (in_data0));
  gst_rtp_buffer_unmap (&rtp);

  fail_unless (gst_rtp_buffer_map (buffer1, GST_MAP_WRITE, &rtp));
  memcpy (gst_rtp_buffer_get_payload (&rtp), &in_data1, sizeof (in_data1));
  gst_rtp_buffer_unmap (&rtp);

  rtpredenc_cant_create_red_packet_base_test (buffer0, buffer1);
}

GST_END_TEST;

static Suite *
rtpred_suite (void)
{
  Suite *s = suite_create ("rtpred");
  TCase *tc_chain = tcase_create ("decoder");
  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, rtpreddec_passthrough);
  tcase_add_test (tc_chain, rtpreddec_main_block);
  tcase_add_test (tc_chain, rtpreddec_redundant_block_not_pushed);
  tcase_add_test (tc_chain, rtpreddec_redundant_block_pushed);
  tcase_add_test (tc_chain, rtpreddec_invalid);

  tc_chain = tcase_create ("encoder");
  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, rtpredenc_passthrough);
  tcase_add_test (tc_chain, rtpredenc_payloadless_rtp);
  tcase_add_test (tc_chain, rtpredenc_without_redundant_block);
  tcase_add_test (tc_chain, rtpredenc_with_redundant_block);
  tcase_add_loop_test (tc_chain, rtpredenc_negative_timestamp_offset, 0, 2);
  tcase_add_loop_test (tc_chain, rtpredenc_too_large_timestamp_offset, 0, 2);
  tcase_add_loop_test (tc_chain, rtpredenc_too_large_length, 0, 2);

  return s;
}

GST_CHECK_MAIN (rtpred)
