/*
 * GStreamer
 *
 * unit test for amrparse
 *
 * Copyright (C) 2008 Nokia Corporation. All rights reserved.
 *
 * Contact: Stefan Kost <stefan.kost@nokia.com>
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
#include "parser.h"

#define SRC_CAPS_NB  "audio/x-amr-nb-sh"
#define SRC_CAPS_WB  "audio/x-amr-wb-sh"
#define SRC_CAPS_ANY "ANY"

#define SINK_CAPS_NB  "audio/AMR, rate=8000 , channels=1"
#define SINK_CAPS_WB  "audio/AMR-WB, rate=16000 , channels=1"
#define SINK_CAPS_ANY "ANY"

#define AMR_FRAME_DURATION (GST_SECOND/50)

static GstStaticPadTemplate sinktemplate_nb = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SINK_CAPS_NB)
    );

static GstStaticPadTemplate sinktemplate_wb = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SINK_CAPS_WB)
    );

static GstStaticPadTemplate srctemplate_nb = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SRC_CAPS_NB)
    );

static GstStaticPadTemplate srctemplate_wb = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SRC_CAPS_WB)
    );

static GstCaps *input_caps = NULL;

/* some data */

static guint8 frame_data_nb[] = {
  0x0c, 0x56, 0x3c, 0x52, 0xe0, 0x61, 0xbc, 0x45,
  0x0f, 0x98, 0x2e, 0x01, 0x42, 0x02
};

static guint8 frame_data_wb[] = {
  0x08, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
  0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};

static guint8 frame_hdr_nb[] = {
  '#', '!', 'A', 'M', 'R', '\n'
};

static guint8 frame_hdr_wb[] = {
  '#', '!', 'A', 'M', 'R', '-', 'W', 'B', '\n'
};

static guint8 garbage_frame[] = {
  0xff, 0xff, 0xff, 0xff, 0xff
};

static void
setup_amrnb (void)
{
  ctx_factory = "amrparse";
  ctx_sink_template = &sinktemplate_nb;
  ctx_src_template = &srctemplate_nb;
  input_caps = gst_caps_from_string (SRC_CAPS_NB);
  g_assert (input_caps);
  ctx_input_caps = input_caps;
}

static void
setup_amrwb (void)
{
  ctx_factory = "amrparse";
  ctx_sink_template = &sinktemplate_wb;
  ctx_src_template = &srctemplate_wb;
  input_caps = gst_caps_from_string (SRC_CAPS_WB);
  g_assert (input_caps);
  ctx_input_caps = input_caps;
}

static void
teardown (void)
{
  gst_caps_unref (input_caps);
}

GST_START_TEST (test_parse_nb_normal)
{
  gst_parser_test_normal (frame_data_nb, sizeof (frame_data_nb));
}

GST_END_TEST;


GST_START_TEST (test_parse_nb_drain_single)
{
  gst_parser_test_drain_single (frame_data_nb, sizeof (frame_data_nb));
}

GST_END_TEST;


GST_START_TEST (test_parse_nb_drain_garbage)
{
  gst_parser_test_drain_garbage (frame_data_nb, sizeof (frame_data_nb),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_nb_split)
{
  gst_parser_test_split (frame_data_nb, sizeof (frame_data_nb));
}

GST_END_TEST;


GST_START_TEST (test_parse_nb_skip_garbage)
{
  gst_parser_test_skip_garbage (frame_data_nb, sizeof (frame_data_nb),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_nb_detect_stream)
{
  GstParserTest ptest;
  GstCaps *old_ctx_caps;

  /* no input caps, override ctx */
  old_ctx_caps = ctx_input_caps;
  ctx_input_caps = NULL;

  /* AMR-NB header */
  gst_parser_test_init (&ptest, frame_hdr_nb, sizeof (frame_hdr_nb), 1);
  /* well, no garbage, followed by real data */
  ptest.series[2].data = frame_data_nb;
  ptest.series[2].size = sizeof (frame_data_nb);
  ptest.series[2].num = 10;
  /* header gets dropped, so ... */
  /* buffer count will not match */
  ptest.framed = FALSE;
  /* total size a bit less */
  ptest.dropped = sizeof (frame_hdr_nb);

  /* Check that the negotiated caps are as expected */
  ptest.sink_caps = gst_caps_from_string (SINK_CAPS_NB);

  gst_parser_test_run (&ptest, NULL);

  gst_caps_unref (ptest.sink_caps);

  ctx_input_caps = old_ctx_caps;
}

GST_END_TEST;


GST_START_TEST (test_parse_wb_normal)
{
  gst_parser_test_normal (frame_data_wb, sizeof (frame_data_wb));
}

GST_END_TEST;


GST_START_TEST (test_parse_wb_drain_single)
{
  gst_parser_test_drain_single (frame_data_wb, sizeof (frame_data_wb));
}

GST_END_TEST;


GST_START_TEST (test_parse_wb_drain_garbage)
{
  gst_parser_test_drain_garbage (frame_data_wb, sizeof (frame_data_wb),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_wb_split)
{
  gst_parser_test_split (frame_data_wb, sizeof (frame_data_wb));
}

GST_END_TEST;


GST_START_TEST (test_parse_wb_skip_garbage)
{
  gst_parser_test_skip_garbage (frame_data_wb, sizeof (frame_data_wb),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_wb_detect_stream)
{
  GstParserTest ptest;
  GstCaps *old_ctx_caps;

  /* no input caps, override ctx */
  old_ctx_caps = ctx_input_caps;
  ctx_input_caps = NULL;

  /* AMR-WB header */
  gst_parser_test_init (&ptest, frame_hdr_wb, sizeof (frame_hdr_wb), 1);
  /* well, no garbage, followed by real data */
  ptest.series[2].data = frame_data_wb;
  ptest.series[2].size = sizeof (frame_data_wb);
  ptest.series[2].num = 10;
  /* header gets dropped, so ... */
  /* buffer count will not match */
  ptest.framed = FALSE;
  /* total size a bit less */
  ptest.dropped = sizeof (frame_hdr_wb);

  /* Check that the negotiated caps are as expected */
  ptest.sink_caps = gst_caps_from_string (SINK_CAPS_WB);

  gst_parser_test_run (&ptest, NULL);

  gst_caps_unref (ptest.sink_caps);

  ctx_input_caps = old_ctx_caps;
}

GST_END_TEST;

/*
 * TODO:
 *   - Both push- and pull-modes need to be tested
 *      * Pull-mode & EOS
 */

static Suite *
amrparse_suite (void)
{
  Suite *s = suite_create ("amrparse");
  TCase *tc_chain;

  /* AMR-NB tests */
  tc_chain = tcase_create ("amrnb");
  tcase_add_checked_fixture (tc_chain, setup_amrnb, teardown);
  tcase_add_test (tc_chain, test_parse_nb_normal);
  tcase_add_test (tc_chain, test_parse_nb_drain_single);
  tcase_add_test (tc_chain, test_parse_nb_drain_garbage);
  tcase_add_test (tc_chain, test_parse_nb_split);
  tcase_add_test (tc_chain, test_parse_nb_detect_stream);
  tcase_add_test (tc_chain, test_parse_nb_skip_garbage);
  suite_add_tcase (s, tc_chain);

  /* AMR-WB tests */
  tc_chain = tcase_create ("amrwb");
  tcase_add_checked_fixture (tc_chain, setup_amrwb, teardown);
  tcase_add_test (tc_chain, test_parse_wb_normal);
  tcase_add_test (tc_chain, test_parse_wb_drain_single);
  tcase_add_test (tc_chain, test_parse_wb_drain_garbage);
  tcase_add_test (tc_chain, test_parse_wb_split);
  tcase_add_test (tc_chain, test_parse_wb_detect_stream);
  tcase_add_test (tc_chain, test_parse_wb_skip_garbage);
  suite_add_tcase (s, tc_chain);

  return s;
}

GST_CHECK_MAIN (amrparse)
