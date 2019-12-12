/*
 * GStreamer
 *
 * unit test for ac3parse
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

#define SRC_CAPS_TMPL   "audio/x-ac3, framed=(boolean)false"
#define SINK_CAPS_TMPL  "audio/x-ac3, framed=(boolean)true"

GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SINK_CAPS_TMPL)
    );

GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SRC_CAPS_TMPL)
    );

/* some data */

static guint8 ac3_frame[512] = {
  0x0b, 0x77, 0xb6, 0xa8, 0x10, 0x40, 0x2f, 0x84,
  0x29, 0xcb, 0xfe, 0x75, 0x7c, 0xf9, 0xf3, 0xe7,
  0xcf, 0x9f, 0x3e, 0x7c, 0xf9, 0xf3, 0xe7, 0xcf,
  0x9f, 0x3e, 0x7c, 0xf9, 0xf3, 0xe7, 0xcf, 0x9f,
  0x3e, 0x7c, 0xf9, 0xf3, 0xe7, 0xcf, 0x9f, 0x3e,
  0x7c, 0xf9, 0xf3, 0xe7, 0xcf, 0x9f, 0x3e, 0x7c,
  0xf9, 0xf3, 0xe7, 0xcf, 0x9f, 0x3e, 0x7c, 0xf9,
  0xf3, 0xe7, 0xcf, 0x9f, 0x3e, 0x7c, 0xf9, 0xf3,
  0xe7, 0xcf, 0x9f, 0x3e, 0x7c, 0xf9, 0xf3, 0xe7,
  0xcf, 0x9f, 0x3e, 0x32, 0xd3, 0xff, 0xc0, 0x06,
  0xe9, 0x40, 0x00, 0x6e, 0x94, 0x00, 0x06, 0xe9,
  0x40, 0x00, 0x6e, 0x94, 0x00, 0x06, 0xe9, 0x40,
  0x00, 0x6e, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static guint8 garbage_frame[] = {
  0xff, 0xff, 0xff, 0xff, 0xff
};


GST_START_TEST (test_parse_normal)
{
  gst_parser_test_normal (ac3_frame, sizeof (ac3_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_drain_single)
{
  gst_parser_test_drain_single (ac3_frame, sizeof (ac3_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_drain_garbage)
{
  gst_parser_test_drain_garbage (ac3_frame, sizeof (ac3_frame),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_split)
{
  gst_parser_test_split (ac3_frame, sizeof (ac3_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_skip_garbage)
{
  gst_parser_test_skip_garbage (ac3_frame, sizeof (ac3_frame),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_detect_stream)
{
  gst_parser_test_output_caps (ac3_frame, sizeof (ac3_frame),
      NULL, SINK_CAPS_TMPL ",channels=1,rate=48000,alignment=frame");
}

GST_END_TEST;


static Suite *
ac3parse_suite (void)
{
  Suite *s = suite_create ("ac3parse");
  TCase *tc_chain = tcase_create ("general");

  /* init test context */
  ctx_factory = "ac3parse";
  ctx_sink_template = &sinktemplate;
  ctx_src_template = &srctemplate;

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_parse_normal);
  tcase_add_test (tc_chain, test_parse_drain_single);
  tcase_add_test (tc_chain, test_parse_drain_garbage);
  tcase_add_test (tc_chain, test_parse_split);
  tcase_add_test (tc_chain, test_parse_skip_garbage);
  tcase_add_test (tc_chain, test_parse_detect_stream);

  return s;
}


/*
 * TODO:
 *   - Both push- and pull-modes need to be tested
 *      * Pull-mode & EOS
 */
GST_CHECK_MAIN (ac3parse);
