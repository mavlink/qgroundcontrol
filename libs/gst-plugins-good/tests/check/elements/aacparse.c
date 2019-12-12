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
#include <gst/check/check.h>
#include "parser.h"

#define SRC_CAPS_CDATA "audio/mpeg, mpegversion=(int)4, framed=(boolean)false, codec_data=(buffer)1190"
#define SRC_CAPS_TMPL  "audio/mpeg, framed=(boolean)false, mpegversion=(int){2,4}"
#define SRC_CAPS_RAW   "audio/mpeg, mpegversion=(int)4, framed=(boolean)true, stream-format=(string)raw, rate=(int)48000, channels=(int)2, codec_data=(buffer)1190"

#define SINK_CAPS \
    "audio/mpeg, framed=(boolean)true"
#define SINK_CAPS_MPEG2 \
    "audio/mpeg, framed=(boolean)true, mpegversion=2, rate=48000, channels=2"
#define SINK_CAPS_MPEG4 \
    "audio/mpeg, framed=(boolean)true, mpegversion=4, rate=96000, channels=2"
#define SINK_CAPS_TMPL  "audio/mpeg, framed=(boolean)true, mpegversion=(int){2,4}"

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
static guint8 adif_header[] = {
  'A', 'D', 'I', 'F'
};

static guint8 adts_frame_mpeg2[] = {
  0xff, 0xf9, 0x4c, 0x80, 0x01, 0xff, 0xfc, 0x21, 0x10, 0xd3, 0x20, 0x0c,
  0x32, 0x00, 0xc7
};

static guint8 adts_frame_mpeg4[] = {
  0xff, 0xf1, 0x4c, 0x80, 0x01, 0xff, 0xfc, 0x21, 0x10, 0xd3, 0x20, 0x0c,
  0x32, 0x00, 0xc7
};

static guint8 garbage_frame[] = {
  0xff, 0xff, 0xff, 0xff, 0xff
};

static guint8 raw_frame_short[] = {
  0x27, 0x00, 0x03, 0x20, 0x64, 0x1c
};

/*
 * Test if the parser pushes data with ADIF header properly and detects the
 * stream to MPEG4 properly.
 */
GST_START_TEST (test_parse_adif_normal)
{
  GstParserTest ptest;

  /* ADIF header */
  gst_parser_test_init (&ptest, adif_header, sizeof (adif_header), 1);
  /* well, no garbage, followed by random data */
  ptest.series[2].size = 100;
  ptest.series[2].num = 3;
  /* and we do not really expect output frames */
  ptest.framed = FALSE;
  /* Check that the negotiated caps are as expected */
  /* For ADIF parser assumes that data is always version 4 */
  ptest.sink_caps =
      gst_caps_from_string (SINK_CAPS_MPEG4 ", stream-format=(string)adif");

  gst_parser_test_run (&ptest, NULL);

  gst_caps_unref (ptest.sink_caps);
}

GST_END_TEST;


GST_START_TEST (test_parse_adts_normal)
{
  gst_parser_test_normal (adts_frame_mpeg4, sizeof (adts_frame_mpeg4));
}

GST_END_TEST;


GST_START_TEST (test_parse_adts_drain_single)
{
  gst_parser_test_drain_single (adts_frame_mpeg4, sizeof (adts_frame_mpeg4));
}

GST_END_TEST;


GST_START_TEST (test_parse_adts_drain_garbage)
{
  gst_parser_test_drain_garbage (adts_frame_mpeg4, sizeof (adts_frame_mpeg4),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


GST_START_TEST (test_parse_adts_split)
{
  gst_parser_test_split (adts_frame_mpeg4, sizeof (adts_frame_mpeg4));
}

GST_END_TEST;


GST_START_TEST (test_parse_adts_skip_garbage)
{
  gst_parser_test_skip_garbage (adts_frame_mpeg4, sizeof (adts_frame_mpeg4),
      garbage_frame, sizeof (garbage_frame));
}

GST_END_TEST;


/*
 * Test if the src caps are set according to stream format (MPEG version).
 */
GST_START_TEST (test_parse_adts_detect_mpeg_version)
{
  gst_parser_test_output_caps (adts_frame_mpeg2, sizeof (adts_frame_mpeg2),
      NULL,
      SINK_CAPS_MPEG2
      ", stream-format=(string)adts, level=(string)2, profile=(string)lc");
}

GST_END_TEST;

/*
 * Test if the parser correctly handles short raw frames and doesn't
 * concatenate them.
 */
GST_START_TEST (test_parse_raw_short)
{
  GstHarness *h = gst_harness_new ("aacparse");
  GstBuffer *in_buf;

  g_object_set (h->element, "disable-passthrough", TRUE, NULL);
  gst_harness_set_src_caps_str (h, SRC_CAPS_RAW);

  in_buf = gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
      raw_frame_short, sizeof raw_frame_short, 0, sizeof raw_frame_short,
      NULL, NULL);

  gst_harness_push (h, gst_buffer_ref (in_buf));
  fail_unless_equals_int (gst_harness_buffers_received (h), 1);

  gst_harness_push (h, in_buf);
  fail_unless_equals_int (gst_harness_buffers_received (h), 2);

  gst_harness_push_event (h, gst_event_new_eos ());
  fail_unless_equals_int (gst_harness_buffers_received (h), 2);

  gst_harness_teardown (h);
}

GST_END_TEST;

#define structure_get_int(s,f) \
    (g_value_get_int(gst_structure_get_value(s,f)))
#define fail_unless_structure_field_int_equals(s,field,num) \
    fail_unless_equals_int (structure_get_int(s,field), num)
/*
 * Test if the parser handles raw stream and codec_data info properly.
 */
GST_START_TEST (test_parse_handle_codec_data)
{
  GstCaps *caps;
  GstStructure *s;
  const gchar *stream_format;

  /* Push random data. It should get through since the parser should be
   * initialized because it got codec_data in the caps */
  caps = gst_parser_test_get_output_caps (NULL, 100, SRC_CAPS_CDATA);
  fail_unless (caps != NULL);

  /* Check that the negotiated caps are as expected */
  /* When codec_data is present, parser assumes that data is version 4 */
  GST_LOG ("aac output caps: %" GST_PTR_FORMAT, caps);
  s = gst_caps_get_structure (caps, 0);
  fail_unless (gst_structure_has_name (s, "audio/mpeg"));
  fail_unless_structure_field_int_equals (s, "mpegversion", 4);
  fail_unless_structure_field_int_equals (s, "channels", 2);
  fail_unless_structure_field_int_equals (s, "rate", 48000);
  fail_unless (gst_structure_has_field (s, "codec_data"));
  fail_unless (gst_structure_has_field (s, "stream-format"));
  stream_format = gst_structure_get_string (s, "stream-format");
  fail_unless (strcmp (stream_format, "raw") == 0);

  gst_caps_unref (caps);
}

GST_END_TEST;

GST_START_TEST (test_parse_proxy_constraints)
{
  GstCaps *caps, *resultcaps;
  GstElement *parse, *filter;
  GstPad *sinkpad;
  GstStructure *s;

  parse = gst_element_factory_make ("aacparse", NULL);
  filter = gst_element_factory_make ("capsfilter", NULL);

  /* constraint on rate and version */
  caps = gst_caps_from_string ("audio/mpeg,mpegversion=2,rate=44100");
  g_object_set (filter, "caps", caps, NULL);
  gst_caps_unref (caps);

  gst_element_link (parse, filter);

  sinkpad = gst_element_get_static_pad (parse, "sink");
  caps = gst_pad_query_caps (sinkpad, NULL);
  GST_LOG ("caps %" GST_PTR_FORMAT, caps);

  fail_unless (gst_caps_get_size (caps) == 1);

  /* getcaps should proxy the rate constraint */
  s = gst_caps_get_structure (caps, 0);
  fail_unless (gst_structure_has_name (s, "audio/mpeg"));
  fail_unless_structure_field_int_equals (s, "rate", 44100);
  gst_caps_unref (caps);

  /* should accept without the constraint */
  caps = gst_caps_from_string ("audio/mpeg,mpegversion=2");
  resultcaps = gst_pad_query_caps (sinkpad, caps);
  fail_if (gst_caps_is_empty (resultcaps));
  gst_caps_unref (resultcaps);
  gst_caps_unref (caps);

  /* should not accept with conflicting version */
  caps = gst_caps_from_string ("audio/mpeg,mpegversion=4");
  resultcaps = gst_pad_query_caps (sinkpad, caps);
  fail_unless (gst_caps_is_empty (resultcaps));
  gst_caps_unref (resultcaps);
  gst_caps_unref (caps);

  gst_object_unref (sinkpad);

  gst_object_unref (filter);
  gst_object_unref (parse);
}

GST_END_TEST;

static Suite *
aacparse_suite (void)
{
  Suite *s = suite_create ("aacparse");
  TCase *tc_chain = tcase_create ("general");

  /* init test context */
  ctx_factory = "aacparse";
  ctx_sink_template = &sinktemplate;
  ctx_src_template = &srctemplate;

  suite_add_tcase (s, tc_chain);
  /* ADIF tests */
  tcase_add_test (tc_chain, test_parse_adif_normal);

  /* ADTS tests */
  tcase_add_test (tc_chain, test_parse_adts_normal);
  tcase_add_test (tc_chain, test_parse_adts_drain_single);
  tcase_add_test (tc_chain, test_parse_adts_drain_garbage);
  tcase_add_test (tc_chain, test_parse_adts_split);
  tcase_add_test (tc_chain, test_parse_adts_skip_garbage);
  tcase_add_test (tc_chain, test_parse_adts_detect_mpeg_version);

  /* Raw tests */
  tcase_add_test (tc_chain, test_parse_raw_short);

  /* Other tests */
  tcase_add_test (tc_chain, test_parse_handle_codec_data);

  /* caps tests */
  tcase_add_test (tc_chain, test_parse_proxy_constraints);

  return s;
}


/*
 * TODO:
 *   - Both push- and pull-modes need to be tested
 *      * Pull-mode & EOS
 */
GST_CHECK_MAIN (aacparse);
