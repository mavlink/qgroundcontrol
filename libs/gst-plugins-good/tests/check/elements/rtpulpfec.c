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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "packets.h"

#include <gst/check/gstharness.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/check/gstcheck.h>

#define RTP_PACKET_DUR (10 * GST_MSECOND)

typedef struct
{
  guint8 pt;
  guint32 ssrc;
  guint16 seq;
} RecoveredPacketInfo;

static void
check_rtpulpfecdec_stats (GstHarness * h, guint packets_recovered,
    guint packets_unrecovered)
{
  guint packets_recovered_out;
  guint packets_unrecovered_out;
  gst_harness_get (h, "rtpulpfecdec",
      "recovered", &packets_recovered_out,
      "unrecovered", &packets_unrecovered_out, NULL);

  fail_unless_equals_int (packets_recovered, packets_recovered_out);
  fail_unless_equals_int (packets_unrecovered, packets_unrecovered_out);
}

static void
push_lost_event (GstHarness * h, guint32 seqnum,
    guint64 timestamp, guint64 duration, gboolean event_goes_through)
{
  GstEvent *packet_loss_in = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
      gst_structure_new ("GstRTPPacketLost",
          "seqnum", G_TYPE_UINT, seqnum,
          "timestamp", G_TYPE_UINT64, timestamp,
          "duration", G_TYPE_UINT64, duration, NULL));
  GstEvent *it, *packet_loss_out = NULL;

  fail_unless_equals_int (TRUE, gst_harness_push_event (h, packet_loss_in));

  while (NULL != (it = gst_harness_try_pull_event (h))) {
    if (GST_EVENT_TYPE (it) != GST_EVENT_CUSTOM_DOWNSTREAM ||
        !gst_event_has_name (it, "GstRTPPacketLost")) {
      gst_event_unref (it);
    } else {
      packet_loss_out = it;
      break;
    }
  }

  if (event_goes_through) {
    const GstStructure *s = gst_event_get_structure (packet_loss_out);
    guint64 tscopy, durcopy;
    gboolean might_have_been_fec;

    fail_unless (gst_structure_has_name (s, "GstRTPPacketLost"));
    fail_if (gst_structure_has_field (s, "seqnum"));
    fail_unless (gst_structure_get_uint64 (s, "timestamp", &tscopy));
    fail_unless (gst_structure_get_uint64 (s, "duration", &durcopy));
    fail_unless (gst_structure_get_boolean (s, "might-have-been-fec",
            &might_have_been_fec));

    fail_unless_equals_uint64 (timestamp, tscopy);
    fail_unless_equals_uint64 (duration, durcopy);
    fail_unless (might_have_been_fec == TRUE);
    gst_event_unref (packet_loss_out);
  } else {
    fail_unless (NULL == packet_loss_out);
  }
}

static void
lose_and_recover_test (GstHarness * h, guint16 lost_seq,
    gconstpointer recbuf, gsize recbuf_size)
{
  guint64 duration = 222222;
  guint64 timestamp = 111111;
  GstBuffer *bufout;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstRTPBuffer rtpout = GST_RTP_BUFFER_INIT;
  GstBuffer *wrap;
  gpointer reccopy = g_malloc (recbuf_size);

  memcpy (reccopy, recbuf, recbuf_size);

  push_lost_event (h, lost_seq, timestamp, duration, FALSE);

  bufout = gst_harness_pull (h);
  fail_unless_equals_int (gst_buffer_get_size (bufout), recbuf_size);
  fail_unless_equals_int (GST_BUFFER_PTS (bufout), timestamp);
  wrap = gst_buffer_new_wrapped (reccopy, recbuf_size);
  gst_rtp_buffer_map (wrap, GST_MAP_WRITE, &rtp);
  gst_rtp_buffer_map (bufout, GST_MAP_READ, &rtpout);
  gst_rtp_buffer_set_seq (&rtp, gst_rtp_buffer_get_seq (&rtpout));
  fail_unless (gst_buffer_memcmp (bufout, 0, reccopy, recbuf_size) == 0);
  gst_rtp_buffer_unmap (&rtp);
  gst_rtp_buffer_unmap (&rtpout);
  fail_unless (!GST_BUFFER_FLAG_IS_SET (bufout, GST_RTP_BUFFER_FLAG_REDUNDANT));
  gst_buffer_unref (bufout);
  gst_buffer_unref (wrap);

  /* Pushing the next buffer with discont flag set */
  bufout = gst_rtp_buffer_new_allocate (0, 0, 0);
  GST_BUFFER_FLAG_SET (bufout, GST_BUFFER_FLAG_DISCONT);
  bufout = gst_harness_push_and_pull (h, bufout);
  /* Checking the flag was unset */
  fail_unless (!GST_BUFFER_IS_DISCONT (bufout));
  gst_buffer_unref (bufout);
}

static void
push_data (GstHarness * h, gconstpointer rtp, gsize rtp_length)
{
  GstBuffer *buf = gst_rtp_buffer_new_copy_data (rtp, rtp_length);
  GstBuffer *bufout;

  bufout = gst_harness_push_and_pull (h, buf);
  if (bufout)
    gst_buffer_unref (bufout);
}

static GstHarness *
harness_rtpulpfecdec (guint32 ssrc, guint8 lost_pt, guint8 fec_pt)
{
  GstHarness *h =
      gst_harness_new_parse ("rtpstorage ! rtpulpfecdec ! identity");
  GObject *internal_storage;
  gchar *caps_str =
      g_strdup_printf ("application/x-rtp,ssrc=(uint)%u,payload=(int)%u",
      ssrc, lost_pt);

  gst_harness_set (h, "rtpstorage", "size-time", (guint64) 200 * RTP_PACKET_DUR,
      NULL);
  gst_harness_get (h, "rtpstorage", "internal-storage", &internal_storage,
      NULL);
  gst_harness_set (h, "rtpulpfecdec", "storage", internal_storage, "pt", fec_pt,
      NULL);
  g_object_unref (internal_storage);

  gst_harness_set_src_caps_str (h, caps_str);
  g_free (caps_str);

  return h;
}

static void
packet_recovered_cb (GObject * internal_storage, GstBuffer * buffer,
    GList * infos)
{
  gboolean found = FALSE;
  GstRTPBuffer rtp = { NULL };
  GList *it;

  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));

  for (it = infos; it; it = it->next) {
    RecoveredPacketInfo *info = it->data;
    if (gst_rtp_buffer_get_seq (&rtp) == info->seq) {
      fail_unless_equals_int (gst_rtp_buffer_get_payload_type (&rtp), info->pt);
      fail_unless_equals_int (gst_rtp_buffer_get_ssrc (&rtp), info->ssrc);
      found = TRUE;
      break;
    }
  }

  gst_rtp_buffer_unmap (&rtp);
  fail_unless (found);
}

static GList *
expect_recovered_packets (GstHarness * h, RecoveredPacketInfo * infos,
    guint infos_len)
{
  GObject *internal_storage;
  guint i = 0;
  GList *res = NULL;

  for (i = 0; i < infos_len; i++)
    res = g_list_prepend (res, &infos[i]);

  gst_harness_get (h, "rtpstorage", "internal-storage", &internal_storage,
      NULL);
  g_signal_connect (internal_storage, "packet-recovered",
      G_CALLBACK (packet_recovered_cb), res);

  g_object_unref (internal_storage);

  return res;
}

GST_START_TEST (rtpulpfecdec_up_and_down)
{
  GstHarness *h = harness_rtpulpfecdec (0, 0, 0);
  gst_harness_set_src_caps_str (h, "application/x-rtp");
  check_rtpulpfecdec_stats (h, 0, 0);
  gst_harness_teardown (h);
}

GST_END_TEST;

static void
_recovered_from_fec_base (guint32 ssrc, guint8 fec_pt,
    guint8 lost_pt, guint16 lost_seq,
    gconstpointer fec_packet, gsize fec_packet_size,
    gconstpointer lost_packet, gsize lost_packet_size)
{
  GstHarness *h = harness_rtpulpfecdec (ssrc, lost_pt, 123);
  RecoveredPacketInfo info = {.pt = lost_pt,.ssrc = ssrc,.seq = lost_seq };
  GList *expected = expect_recovered_packets (h, &info, 1);

  push_data (h, fec_packet, fec_packet_size);
  lose_and_recover_test (h, lost_seq, lost_packet, lost_packet_size);

  check_rtpulpfecdec_stats (h, 1, 0);

  g_list_free (expected);
  gst_harness_teardown (h);
}

GST_START_TEST (rtpulpfecdec_recovered_from_fec)
{
  _recovered_from_fec_base (3536077562UL, 123, 100, 36921,
      SAMPLE_ULPFEC0_FEC, sizeof (SAMPLE_ULPFEC0_FEC) - 1,
      SAMPLE_ULPFEC0_MEDIA, sizeof (SAMPLE_ULPFEC0_MEDIA) - 1);
}

GST_END_TEST;

GST_START_TEST (rtpulpfecdec_recovered_from_fec_long)
{
  GstHarness *h = harness_rtpulpfecdec (3536077562UL, 100, 123);
  RecoveredPacketInfo info = {.pt = 100,.ssrc = 3536077562UL,.seq = 36921 };
  const guint8 fec[] = SAMPLE_ULPFEC0_FEC;
  guint8 *feclongmask = NULL;
  GList *expected = expect_recovered_packets (h, &info, 1);

  /* Long FEC mask needs 4 more bytes */
  feclongmask = g_malloc (sizeof (fec) - 1 + 4);

  /* Copying the beginning of FEC packet:
   * RTP header (12 bytes) + RtpUlpFecHeader (10 bytes) + RtpUlpFecLevelHeader (4 bytes) */
  memcpy (feclongmask, fec, 26);

  // Changing L bit: 0 -> 1
  feclongmask[12] |= 64;
  // Changing fec_seq_base: 36921 -> 36874
  feclongmask[14] = 0x90;
  feclongmask[15] = 0x0a;
  // Changing fec_mask: 0x800000000000 -> 0x000000000001
  feclongmask[24] = 0;
  feclongmask[25] = 0;
  feclongmask[26] = 0;
  feclongmask[27] = 0;
  feclongmask[28] = 0;
  feclongmask[29] = 1;
  memcpy (feclongmask + 30, fec + 26, sizeof (fec) - 1 - 26);

  push_data (h, feclongmask, sizeof (fec) - 1 + 4);
  lose_and_recover_test (h, 36921, SAMPLE_ULPFEC0_MEDIA,
      sizeof (SAMPLE_ULPFEC0_MEDIA) - 1);

  g_free (feclongmask);
  check_rtpulpfecdec_stats (h, 1, 0);
  g_list_free (expected);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpulpfecdec_recovered_from_many)
{
  GstHarness *h = harness_rtpulpfecdec (578322839UL, 126, 22);
  static const gchar *packets[] = {
    SAMPLE_ULPFEC1_MEDIA0,
    SAMPLE_ULPFEC1_MEDIA1,
    SAMPLE_ULPFEC1_MEDIA2,
    SAMPLE_ULPFEC1_MEDIA3,
    SAMPLE_ULPFEC1_FEC0,
    SAMPLE_ULPFEC1_FEC1,
  };
  static gsize packets_size[] = {
    sizeof (SAMPLE_ULPFEC1_MEDIA0) - 1,
    sizeof (SAMPLE_ULPFEC1_MEDIA1) - 1,
    sizeof (SAMPLE_ULPFEC1_MEDIA2) - 1,
    sizeof (SAMPLE_ULPFEC1_MEDIA3) - 1,
    sizeof (SAMPLE_ULPFEC1_FEC0) - 1,
    sizeof (SAMPLE_ULPFEC1_FEC1) - 1,
  };
  guint lost_seq = __i__ + 8476;
  const gchar *lost_packet = packets[__i__];
  gsize lost_packet_size = packets_size[__i__];
  RecoveredPacketInfo info = {.pt = 126,.ssrc = 578322839UL,.seq = lost_seq };
  GList *expected = expect_recovered_packets (h, &info, 1);
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (packets); ++i) {
    if (i != (gsize) __i__)
      push_data (h, packets[i], packets_size[i]);
  }

  lose_and_recover_test (h, lost_seq, lost_packet, lost_packet_size);

  check_rtpulpfecdec_stats (h, 1, 0);
  g_list_free (expected);
  gst_harness_teardown (h);
}

GST_END_TEST;

typedef struct _SampleRTPPacket SampleRTPPacket;
struct _SampleRTPPacket
{
  const gchar *data;
  gsize len;
};

SampleRTPPacket avmcu_media_packets[] = {
  {SAMPLE_AVMCU2_MEDIA0, sizeof (SAMPLE_AVMCU2_MEDIA0) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA1, sizeof (SAMPLE_AVMCU2_MEDIA1) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA2, sizeof (SAMPLE_AVMCU2_MEDIA2) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA3, sizeof (SAMPLE_AVMCU2_MEDIA3) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA4, sizeof (SAMPLE_AVMCU2_MEDIA4) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA5, sizeof (SAMPLE_AVMCU2_MEDIA5) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA6, sizeof (SAMPLE_AVMCU2_MEDIA6) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA7, sizeof (SAMPLE_AVMCU2_MEDIA7) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA8, sizeof (SAMPLE_AVMCU2_MEDIA8) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA9, sizeof (SAMPLE_AVMCU2_MEDIA9) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA10, sizeof (SAMPLE_AVMCU2_MEDIA10) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA11, sizeof (SAMPLE_AVMCU2_MEDIA11) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA12, sizeof (SAMPLE_AVMCU2_MEDIA12) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA13, sizeof (SAMPLE_AVMCU2_MEDIA13) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA14, sizeof (SAMPLE_AVMCU2_MEDIA14) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA15, sizeof (SAMPLE_AVMCU2_MEDIA15) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA16, sizeof (SAMPLE_AVMCU2_MEDIA16) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA17, sizeof (SAMPLE_AVMCU2_MEDIA17) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA18, sizeof (SAMPLE_AVMCU2_MEDIA18) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA19, sizeof (SAMPLE_AVMCU2_MEDIA19) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA20, sizeof (SAMPLE_AVMCU2_MEDIA20) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA21, sizeof (SAMPLE_AVMCU2_MEDIA21) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA22, sizeof (SAMPLE_AVMCU2_MEDIA22) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA23, sizeof (SAMPLE_AVMCU2_MEDIA23) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA24, sizeof (SAMPLE_AVMCU2_MEDIA24) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA25, sizeof (SAMPLE_AVMCU2_MEDIA25) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA26, sizeof (SAMPLE_AVMCU2_MEDIA26) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA27, sizeof (SAMPLE_AVMCU2_MEDIA27) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA28, sizeof (SAMPLE_AVMCU2_MEDIA28) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA29, sizeof (SAMPLE_AVMCU2_MEDIA29) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA30, sizeof (SAMPLE_AVMCU2_MEDIA30) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA31, sizeof (SAMPLE_AVMCU2_MEDIA31) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA32, sizeof (SAMPLE_AVMCU2_MEDIA32) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA33, sizeof (SAMPLE_AVMCU2_MEDIA33) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA34, sizeof (SAMPLE_AVMCU2_MEDIA34) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA35, sizeof (SAMPLE_AVMCU2_MEDIA35) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA36, sizeof (SAMPLE_AVMCU2_MEDIA36) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA37, sizeof (SAMPLE_AVMCU2_MEDIA37) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA38, sizeof (SAMPLE_AVMCU2_MEDIA38) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA39, sizeof (SAMPLE_AVMCU2_MEDIA39) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA40, sizeof (SAMPLE_AVMCU2_MEDIA40) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA41, sizeof (SAMPLE_AVMCU2_MEDIA41) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA42, sizeof (SAMPLE_AVMCU2_MEDIA42) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA43, sizeof (SAMPLE_AVMCU2_MEDIA43) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA44, sizeof (SAMPLE_AVMCU2_MEDIA44) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA45, sizeof (SAMPLE_AVMCU2_MEDIA45) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA46, sizeof (SAMPLE_AVMCU2_MEDIA46) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA47, sizeof (SAMPLE_AVMCU2_MEDIA47) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA48, sizeof (SAMPLE_AVMCU2_MEDIA48) - 1}
  ,
  {SAMPLE_AVMCU2_MEDIA49, sizeof (SAMPLE_AVMCU2_MEDIA49) - 1}
  ,
};

SampleRTPPacket avmcu_fec_packets[] = {
  {SAMPLE_AVMCU2_FEC0, sizeof (SAMPLE_AVMCU2_FEC0) - 1}
  ,
  {SAMPLE_AVMCU2_FEC1, sizeof (SAMPLE_AVMCU2_FEC1) - 1}
  ,
  {SAMPLE_AVMCU2_FEC2, sizeof (SAMPLE_AVMCU2_FEC2) - 1}
  ,
  {SAMPLE_AVMCU2_FEC3, sizeof (SAMPLE_AVMCU2_FEC3) - 1}
  ,
  {SAMPLE_AVMCU2_FEC4, sizeof (SAMPLE_AVMCU2_FEC4) - 1}
  ,
  {SAMPLE_AVMCU2_FEC5, sizeof (SAMPLE_AVMCU2_FEC5) - 1}
  ,
  {SAMPLE_AVMCU2_FEC6, sizeof (SAMPLE_AVMCU2_FEC6) - 1}
  ,
};


GST_START_TEST (rtpulpfecdec_recovered_using_recovered_packet)
{
  GstHarness *h = harness_rtpulpfecdec (578322839UL, 126, 22);
  RecoveredPacketInfo info[3] = {
    {.pt = 126,.ssrc = 578322839UL,.seq = 8477}
    ,
    {.pt = 126,.ssrc = 578322839UL,.seq = 8476}
    ,
    {.pt = 126,.ssrc = 578322839UL,.seq = 8479}
  };
  GList *expected = expect_recovered_packets (h, info, 3);

  // To recover the packet we want we need to recover 2 packets
  push_data (h, SAMPLE_ULPFEC1_MEDIA2, sizeof (SAMPLE_ULPFEC1_MEDIA2) - 1);
  push_data (h, SAMPLE_ULPFEC1_FEC0, sizeof (SAMPLE_ULPFEC1_FEC0) - 1);
  push_data (h, SAMPLE_ULPFEC1_FEC1, sizeof (SAMPLE_ULPFEC1_FEC1) - 1);
  push_data (h, SAMPLE_ULPFEC1_FEC2, sizeof (SAMPLE_ULPFEC1_FEC2) - 1);

  lose_and_recover_test (h, 8479, SAMPLE_ULPFEC1_MEDIA3,
      sizeof (SAMPLE_ULPFEC1_MEDIA3) - 1);

  check_rtpulpfecdec_stats (h, 1, 0);
  g_list_free (expected);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpulpfecdec_recovered_from_storage)
{
  GstHarness *h = harness_rtpulpfecdec (578322839UL, 126, 22);

  // The packet we want to recover is already in the storage
  push_data (h, SAMPLE_ULPFEC1_MEDIA0, sizeof (SAMPLE_ULPFEC1_MEDIA0) - 1);
  push_data (h, SAMPLE_ULPFEC1_MEDIA1, sizeof (SAMPLE_ULPFEC1_MEDIA1) - 1);
  push_data (h, SAMPLE_ULPFEC1_MEDIA2, sizeof (SAMPLE_ULPFEC1_MEDIA2) - 1);
  push_data (h, SAMPLE_ULPFEC1_MEDIA3, sizeof (SAMPLE_ULPFEC1_MEDIA3) - 1);
  push_data (h, SAMPLE_ULPFEC1_FEC0, sizeof (SAMPLE_ULPFEC1_FEC0) - 1);
  push_data (h, SAMPLE_ULPFEC1_FEC1, sizeof (SAMPLE_ULPFEC1_FEC1) - 1);
  push_data (h, SAMPLE_ULPFEC1_FEC2, sizeof (SAMPLE_ULPFEC1_FEC2) - 1);

  lose_and_recover_test (h, 8479, SAMPLE_ULPFEC1_MEDIA3,
      sizeof (SAMPLE_ULPFEC1_MEDIA3) - 1);

  check_rtpulpfecdec_stats (h, 1, 0);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpulpfecdec_recovered_push_failed)
{
  GstHarness *h = harness_rtpulpfecdec (3536077562UL, 100, 123);
  RecoveredPacketInfo info = {.pt = 100,.ssrc = 3536077562UL,.seq = 36921 };
  GList *expected = expect_recovered_packets (h, &info, 1);

  // the harness is already PLAYING because there are no src pads, which
  // means the error-after counter isn't set, so reset and start again.
  gst_element_set_state (h->element, GST_STATE_NULL);
  gst_harness_set (h, "identity", "error-after", 2, NULL);
  gst_harness_play (h);

  push_data (h, SAMPLE_ULPFEC0_FEC, sizeof (SAMPLE_ULPFEC0_FEC) - 1);
  push_lost_event (h, 36921, 1111, 2222, FALSE);

  /* gst_pad_push for the recovered packet has failed,
   * making sure the error will be propagated from the chain function*/
  fail_unless_equals_int (gst_harness_push (h, gst_buffer_new ()),
      GST_FLOW_ERROR);

  g_list_free (expected);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpulpfecdec_invalid_fec_size_mismatch)
{
  static const guint packet_sizes[] = { 21, 25,
    sizeof (SAMPLE_ULPFEC0_FEC) - 2, sizeof (SAMPLE_ULPFEC0_FEC)
  };
  GstHarness *h = harness_rtpulpfecdec (3536077562UL, 100, 123);

  push_data (h, SAMPLE_ULPFEC0_FEC, packet_sizes[__i__]);
  push_lost_event (h, 36921, 1111, 2222, TRUE);

  check_rtpulpfecdec_stats (h, 0, 1);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpulpfecdec_invalid_fec_ebit_not_zero)
{
  const guint8 fec[] = SAMPLE_ULPFEC0_FEC;
  guint8 *fec_ebit_not_zero = NULL;
  GstHarness *h = harness_rtpulpfecdec (3536077562UL, 100, 123);

  // Changing E bit: 0 -> 1
  fec_ebit_not_zero = g_malloc (sizeof (fec) - 1);
  memcpy (fec_ebit_not_zero, fec, sizeof (fec) - 1);
  fec_ebit_not_zero[12] |= 128;

  push_data (h, fec_ebit_not_zero, sizeof (fec) - 1);
  push_lost_event (h, 36921, 1111, 2222, TRUE);

  g_free (fec_ebit_not_zero);
  check_rtpulpfecdec_stats (h, 0, 1);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpulpfecdec_invalid_recovered)
{
  GstHarness *h = harness_rtpulpfecdec (3536077562UL, 50, 22);

  push_data (h, SAMPLE_ISSUE_7049, sizeof (SAMPLE_ISSUE_7049) - 1);
  push_lost_event (h, 36921, 1111, 2222, TRUE);

  check_rtpulpfecdec_stats (h, 0, 1);
  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpulpfecdec_invalid_recovered_pt_mismatch)
{
  GstHarness *h = harness_rtpulpfecdec (3536077562UL, 100, 123);
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstBuffer *modified;
  GstBuffer *bufout;

  gst_harness_set_src_caps_str (h, "application/x-rtp,ssrc=(uint)3536077562");

  /* Payload type can be derivered neither from the caps nor from the media packets */
  push_data (h, SAMPLE_ULPFEC0_FEC, sizeof (SAMPLE_ULPFEC0_FEC) - 1);
  push_lost_event (h, 36921, 1111, 2222, TRUE);
  check_rtpulpfecdec_stats (h, 0, 1);

  /* Payload type can only be derivered from the caps (pt=50). Recovered packet
   * with pt=100 should be ignored */
  gst_harness_set_src_caps_str (h,
      "application/x-rtp,ssrc=(uint)3536077562,payload=(int)50");
  push_lost_event (h, 36921, 1111, 2222, TRUE);
  check_rtpulpfecdec_stats (h, 0, 2);

  modified =
      gst_rtp_buffer_new_copy_data (SAMPLE_ULPFEC0_MEDIA,
      sizeof (SAMPLE_ULPFEC0_MEDIA));
  fail_unless (gst_rtp_buffer_map (modified, GST_MAP_READ, &rtp));
  gst_rtp_buffer_set_payload_type (&rtp, 50);
  gst_rtp_buffer_set_seq (&rtp, 36920);
  gst_rtp_buffer_unmap (&rtp);
  /* Now we have media packet with pt=50 and caps with pt=50. */
  bufout = gst_harness_push_and_pull (h, modified);
  if (bufout)
    gst_buffer_unref (bufout);
  push_lost_event (h, 36921, 1111, 2222, TRUE);
  check_rtpulpfecdec_stats (h, 0, 3);

  /* Removing payload type from the caps */
  gst_harness_set_src_caps_str (h, "application/x-rtp,ssrc=(uint)3536077562");
  push_lost_event (h, 36921, 1111, 2222, TRUE);
  check_rtpulpfecdec_stats (h, 0, 4);

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (rtpulpfecdec_fecstorage_gives_no_buffers)
{
  GstHarness *h = harness_rtpulpfecdec (3536077562UL, 100, 123);

  push_lost_event (h, 36921, 1111, 2222, TRUE);

  check_rtpulpfecdec_stats (h, 0, 1);

  gst_harness_teardown (h);
}

GST_END_TEST;

static Suite *
rtpfec_suite (void)
{
  Suite *s = suite_create ("rtpfec");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, rtpulpfecdec_up_and_down);

  tcase_add_test (tc_chain, rtpulpfecdec_recovered_from_fec);
  tcase_add_test (tc_chain, rtpulpfecdec_recovered_from_fec_long);
  tcase_add_loop_test (tc_chain, rtpulpfecdec_recovered_from_many, 0, 4);
  tcase_add_test (tc_chain, rtpulpfecdec_recovered_using_recovered_packet);
  tcase_add_test (tc_chain, rtpulpfecdec_recovered_from_storage);
  tcase_add_test (tc_chain, rtpulpfecdec_recovered_push_failed);

  tcase_add_loop_test (tc_chain, rtpulpfecdec_invalid_fec_size_mismatch, 0, 4);
  tcase_add_test (tc_chain, rtpulpfecdec_invalid_fec_ebit_not_zero);
  tcase_add_test (tc_chain, rtpulpfecdec_invalid_recovered);
  tcase_add_test (tc_chain, rtpulpfecdec_invalid_recovered_pt_mismatch);
  tcase_add_test (tc_chain, rtpulpfecdec_fecstorage_gives_no_buffers);
  return s;
}

GST_CHECK_MAIN (rtpfec)
