/*
 * GStreamer
 *
 * unit test for aacparse
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

#define SRC_CAPS_TMPL   "audio/mpeg, parsed=(boolean)false, mpegversion=(int)1"
#define SINK_CAPS_TMPL  "audio/mpeg, parsed=(boolean)true, mpegversion=(int)1"

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
static guint8 mp3_frame[384] = {
  0xff, 0xfb, 0x94, 0xc4, 0xff, 0x83, 0xc0, 0x00,
  0x01, 0xa4, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00,
  0x34, 0x80, 0x00, 0x00, 0x04, 0x00,
};

static guint8 garbage_frame[] = {
  0xff, 0xff, 0xff, 0xff, 0xff
};


GST_START_TEST (test_parse_normal)
{
  gst_parser_test_normal (mp3_frame, sizeof (mp3_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_drain_single)
{
  gst_parser_test_drain_single (mp3_frame, sizeof (mp3_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_drain_garbage)
{
  gst_parser_test_drain_garbage (mp3_frame, sizeof (mp3_frame),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_split)
{
  gst_parser_test_split (mp3_frame, sizeof (mp3_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_skip_garbage)
{
  gst_parser_test_skip_garbage (mp3_frame, sizeof (mp3_frame),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


#define structure_get_int(s,f) \
    (g_value_get_int(gst_structure_get_value(s,f)))
#define fail_unless_structure_field_int_equals(s,field,num) \
    fail_unless_equals_int (structure_get_int(s,field), num)

GST_START_TEST (test_parse_detect_stream)
{
  GstStructure *s;
  GstCaps *caps;

  caps = gst_parser_test_get_output_caps (mp3_frame, sizeof (mp3_frame), NULL);

  fail_unless (caps != NULL);

  GST_LOG ("mpegaudio output caps: %" GST_PTR_FORMAT, caps);
  s = gst_caps_get_structure (caps, 0);
  fail_unless (gst_structure_has_name (s, "audio/mpeg"));
  fail_unless_structure_field_int_equals (s, "mpegversion", 1);
  fail_unless_structure_field_int_equals (s, "layer", 3);
  fail_unless_structure_field_int_equals (s, "channels", 1);
  fail_unless_structure_field_int_equals (s, "rate", 48000);

  gst_caps_unref (caps);
}

GST_END_TEST;


static Suite *
mpegaudioparse_suite (void)
{
  Suite *s = suite_create ("mpegaudioparse");
  TCase *tc_chain = tcase_create ("general");


  /* init test context */
  ctx_factory = "mpegaudioparse";
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
GST_CHECK_MAIN (mpegaudioparse);
