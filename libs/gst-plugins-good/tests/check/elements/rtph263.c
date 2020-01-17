/* GStreamer
 *
 * Copyright (C) 2015 Pexip AS
 *   @author Stian Selnes <stian@pexip.com>
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

#include <gst/check/check.h>
#include <gst/check/gstharness.h>
#include <gst/rtp/gstrtpbuffer.h>

#define RTP_H263_CAPS_STR(p)                                            \
  "application/x-rtp,media=video,encoding-name=H263,clock-rate=90000,"  \
  "payload=" G_STRINGIFY(p)

#define H263P_RTP_CAPS_STR(p)                                           \
  "application/x-rtp,media=video,encoding-name=H263-1998,clock-rate=90000," \
  "payload="G_STRINGIFY(p)

static gboolean
have_element (const gchar * element_name)
{
  GstElement *element;
  gboolean ret;

  element = gst_element_factory_make (element_name, NULL);
  ret = element != NULL;

  if (element)
    gst_object_unref (element);

  return ret;
}

static GstBuffer *
create_rtp_buffer (guint8 * data, gsize size, guint ts, gint seqnum)
{
  GstBuffer *buf = gst_rtp_buffer_new_copy_data (data, size);
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  GST_BUFFER_PTS (buf) = (ts) * (GST_SECOND / 30);

  gst_rtp_buffer_map (buf, GST_MAP_WRITE, &rtp);
  gst_rtp_buffer_set_seq (&rtp, seqnum);
  gst_rtp_buffer_unmap (&rtp);

  return buf;
}

GST_START_TEST (test_h263depay_start_packet_too_small_mode_a)
{
  GstHarness *h = gst_harness_new ("rtph263depay");
  guint8 packet[] = {
    0x80, 0xa2, 0x17, 0x62, 0x57, 0xbb, 0x48, 0x98, 0x4a, 0x59, 0xe8, 0xdc,
    0x00, 0x00, 0x80, 0x00
  };

  gst_harness_set_src_caps_str (h, RTP_H263_CAPS_STR (34));
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, create_rtp_buffer (packet, sizeof (packet), 0, 0)));

  /* Packet should be dropped and depayloader not crash */
  fail_unless_equals_int (0, gst_harness_buffers_received (h));

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_h263depay_start_packet_too_small_mode_b)
{
  GstHarness *h = gst_harness_new ("rtph263depay");
  guint8 packet[] = {
    0x80, 0xa2, 0x17, 0x62, 0x57, 0xbb, 0x48, 0x98, 0x4a, 0x59, 0xe8, 0xdc,
    0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  gst_harness_set_src_caps_str (h, RTP_H263_CAPS_STR (34));
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, create_rtp_buffer (packet, sizeof (packet), 0, 0)));

  /* Packet should be dropped and depayloader not crash */
  fail_unless_equals_int (0, gst_harness_buffers_received (h));

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_h263depay_start_packet_too_small_mode_c)
{
  GstHarness *h = gst_harness_new ("rtph263depay");
  guint8 packet[] = {
    0x80, 0xa2, 0x17, 0x62, 0x57, 0xbb, 0x48, 0x98, 0x4a, 0x59, 0xe8, 0xdc,
    0xc0, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  gst_harness_set_src_caps_str (h, RTP_H263_CAPS_STR (34));
  fail_unless_equals_int (GST_FLOW_OK,
      gst_harness_push (h, create_rtp_buffer (packet, sizeof (packet), 0, 0)));

  /* Packet should be dropped and depayloader not crash */
  fail_unless_equals_int (0, gst_harness_buffers_received (h));

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_h263pay_mode_b_snow)
{
  /* Payloading one large frame (like snow) is more likely to use mode b and
   * trigger issues in valgrind seen previously like double free, invalid read
   * etc. */
  GstHarness *h;
  guint frames = 1;
  guint i;

  if (!have_element ("avenc_h263"))
    return;

  h = gst_harness_new_parse
      ("avenc_h263 rtp-payload-size=1 ! rtph263pay mtu=1350 ");
  gst_harness_add_src_parse (h,
      "videotestsrc pattern=snow is-live=1 ! "
      "capsfilter caps=\"video/x-raw,format=I420,width=176,height=144\"", TRUE);

  for (i = 0; i < frames; i++)
    gst_harness_push_from_src (h);
  fail_unless (gst_harness_buffers_received (h) >= frames);

  gst_harness_teardown (h);
}

GST_END_TEST;

/* gst_rtp_buffer_get_payload() may return a copy of the payload. This test
 * makes sure that the rtph263pdepay also produces the correct output in this
 * case. */
GST_START_TEST (test_h263pdepay_fragmented_memory_non_writable_buffer)
{
  GstHarness *h;
  GstBuffer *header_buf, *payload_buf, *buf;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint8 header[] = {
    0x04, 0x00
  };
  guint8 payload[] = {
    0x80, 0x02, 0x1c, 0xb8, 0x01, 0x00, 0x11, 0xe0, 0x44, 0xc4
  };
  guint8 frame[] = {
    0x00, 0x00, 0x80, 0x02, 0x1c, 0xb8, 0x01, 0x00, 0x11, 0xe0, 0x44, 0xc4
  };

  h = gst_harness_new ("rtph263pdepay");
  gst_harness_set_src_caps_str (h, "application/x-rtp, media=video, "
      "clock-rate=90000, encoding-name=H263-1998");

  /* Packet with M=1, P=1 */
  header_buf = gst_rtp_buffer_new_allocate (sizeof (header), 0, 0);
  gst_rtp_buffer_map (header_buf, GST_MAP_WRITE, &rtp);
  gst_rtp_buffer_set_marker (&rtp, TRUE);
  memcpy (gst_rtp_buffer_get_payload (&rtp), header, sizeof (header));
  gst_rtp_buffer_unmap (&rtp);

  payload_buf = gst_buffer_new_allocate (NULL, sizeof (payload), NULL);
  gst_buffer_fill (payload_buf, 0, payload, sizeof (payload));
  buf = gst_buffer_append (header_buf, payload_buf);

  gst_harness_push (h, gst_buffer_ref (buf));
  gst_buffer_unref (buf);

  buf = gst_harness_pull (h);
  fail_unless (gst_buffer_memcmp (buf, 0, frame, sizeof (frame)) == 0);
  gst_buffer_unref (buf);

  gst_harness_teardown (h);
}

GST_END_TEST;

/* gst_rtp_buffer_get_payload() may return a copy of the payload. This test
 * makes sure that the rtph263pdepay also produces the correct output in this
 * case. */
GST_START_TEST
    (test_h263pdepay_fragmented_memory_non_writable_buffer_split_frame) {
  GstHarness *h;
  GstBuffer *header_buf, *payload_buf, *buf;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  guint8 header[] = {
    0x04, 0x00
  };
  guint8 payload[] = {
    0x80, 0x02, 0x1c, 0xb8, 0x01, 0x00, 0x11, 0xe0, 0x44, 0xc4
  };
  guint8 frame[] = {
    0x00, 0x00, 0x80, 0x02, 0x1c, 0xb8, 0x01, 0x00, 0x11, 0xe0, 0x44, 0xc4
  };

  h = gst_harness_new ("rtph263pdepay");
  gst_harness_set_src_caps_str (h, "application/x-rtp, media=video, "
      "clock-rate=90000, encoding-name=H263-1998");

  /* First packet, M=0, P=1 */
  header_buf = gst_rtp_buffer_new_allocate (sizeof (header), 0, 0);
  gst_rtp_buffer_map (header_buf, GST_MAP_WRITE, &rtp);
  gst_rtp_buffer_set_marker (&rtp, FALSE);
  gst_rtp_buffer_set_seq (&rtp, 0);
  memcpy (gst_rtp_buffer_get_payload (&rtp), header, sizeof (header));
  gst_rtp_buffer_unmap (&rtp);

  payload_buf = gst_buffer_new_allocate (NULL, sizeof (payload), NULL);
  gst_buffer_fill (payload_buf, 0, payload, sizeof (payload));
  buf = gst_buffer_append (header_buf, payload_buf);

  gst_harness_push (h, gst_buffer_ref (buf));
  gst_buffer_unref (buf);
  fail_unless_equals_int (gst_harness_buffers_received (h), 0);

  /* Second packet, M=1, P=1 */
  header_buf = gst_rtp_buffer_new_allocate (sizeof (header), 0, 0);
  gst_rtp_buffer_map (header_buf, GST_MAP_WRITE, &rtp);
  gst_rtp_buffer_set_marker (&rtp, TRUE);
  gst_rtp_buffer_set_seq (&rtp, 1);
  memcpy (gst_rtp_buffer_get_payload (&rtp), header, sizeof (header));
  gst_rtp_buffer_unmap (&rtp);

  payload_buf = gst_buffer_new_allocate (NULL, sizeof (payload), NULL);
  gst_buffer_memset (payload_buf, 0, 0, 10);
  buf = gst_buffer_append (header_buf, payload_buf);

  gst_harness_push (h, gst_buffer_ref (buf));
  gst_buffer_unref (buf);
  fail_unless_equals_int (gst_harness_buffers_received (h), 1);

  buf = gst_harness_pull (h);
  fail_unless (gst_buffer_memcmp (buf, 0, frame, sizeof (frame)) == 0);
  gst_buffer_unref (buf);

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_h263pdepay_dont_push_empty_frame)
{
  GstHarness *h = gst_harness_new ("rtph263pdepay");

  /* Packet that only contains header information and an extra picture header
   * (PLEN > 0). Partly handcrafted packet. Originally this packet did not
   * have P=1 (hence it was not start of a picture). With both P=1 and M=1 we
   * only need one packet to reproduce the issue where trying to push an empty
   * frame when PLEN is set */
  guint8 packet[] = {
    0x80, 0xe8, 0xbc, 0xaa, 0x14, 0x12, 0x16, 0x5c, 0xb8, 0x4e, 0x39, 0x04,
    0x25, 0x00, 0x54, 0x39, 0xd0, 0x12, 0x06, 0x9e, 0xb5, 0x0a, 0xf5, 0xe8,
    0x32, 0xeb, 0xd0, 0x6b, 0xd6, 0xa2, 0xfa, 0xd4, 0x3d, 0xd7, 0xa0, 0x2b,
    0x24, 0x97, 0xc3, 0xbf, 0xc0, 0xbb, 0xd7, 0xa0,
  };

  gst_harness_set_src_caps_str (h, H263P_RTP_CAPS_STR (100));

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h,
          create_rtp_buffer (packet, sizeof (packet), 0, 0)));

  fail_unless_equals_int (gst_harness_buffers_received (h), 0);

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_h263ppay_non_fixed_caps)
{
  GstHarness *h;
  guint8 frame[] = {
    0x00, 0x00, 0x80, 0x06, 0x1c, 0xa8, 0x01, 0x04, 0x91, 0xe0, 0x37, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };

  h = gst_harness_new_parse ("rtph263ppay");

  /* Set non-fixed caps after payloader */
  gst_harness_set_caps_str (h, "video/x-h263, variant=(string)itu",
      "application/x-rtp, clock-rate=[1, MAX]");

  gst_harness_push (h, gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
          frame, sizeof (frame), 0, sizeof (frame), NULL, NULL));

  fail_unless_equals_int (1, gst_harness_buffers_received (h));

  gst_harness_teardown (h);
}

GST_END_TEST;

static Suite *
rtph263_suite (void)
{
  Suite *s = suite_create ("rtph263");
  TCase *tc_chain;

  suite_add_tcase (s, (tc_chain = tcase_create ("h263depay")));
  tcase_add_test (tc_chain, test_h263depay_start_packet_too_small_mode_a);
  tcase_add_test (tc_chain, test_h263depay_start_packet_too_small_mode_b);
  tcase_add_test (tc_chain, test_h263depay_start_packet_too_small_mode_c);

  suite_add_tcase (s, (tc_chain = tcase_create ("h263pay")));
  tcase_add_test (tc_chain, test_h263pay_mode_b_snow);

  suite_add_tcase (s, (tc_chain = tcase_create ("h263pdepay")));
  tcase_add_test (tc_chain,
      test_h263pdepay_fragmented_memory_non_writable_buffer);
  tcase_add_test (tc_chain,
      test_h263pdepay_fragmented_memory_non_writable_buffer_split_frame);
  tcase_add_test (tc_chain, test_h263pdepay_dont_push_empty_frame);

  suite_add_tcase (s, (tc_chain = tcase_create ("h263ppay")));
  tcase_add_test (tc_chain, test_h263ppay_non_fixed_caps);

  return s;
}

GST_CHECK_MAIN (rtph263);
