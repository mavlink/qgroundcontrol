/*
 * icydemux.c - Test icydemux element
 * Copyright (C) 2006 Michael Smith <msmith@fluendo.com>
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/check/gstcheck.h>

/* Chunk of data: 8 bytes, followed by a metadata-length byte of 2, followed by
 * some metadata (32 bytes), then some more data.
 */
#define TEST_METADATA \
    "Test metadata"
#define ICY_METADATA \
    "StreamTitle='" TEST_METADATA "';\0\0\0\0"

#define EMPTY_ICY_STREAM_TITLE_METADATA \
    "StreamTitle='';\0"

#define ICY_DATA \
    "aaaaaaaa" \
    "\x02" \
    ICY_METADATA \
    "bbbbbbbb"

#define ICY_DATA_EMPTY_METADATA \
    ICY_DATA \
    "\x00" \
    "dddddddd" \
    "\x01" \
    EMPTY_ICY_STREAM_TITLE_METADATA \
    "cccccccc"

#define ICYCAPS "application/x-icy, metadata-interval = (int)8"

#define SRC_CAPS "application/x-icy, metadata-interval = (int)[0, MAX]"
#define SINK_CAPS  "ANY"

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SRC_CAPS)
    );

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SINK_CAPS)
    );

static GstElement *icydemux;
static GstBus *bus;
GstPad *srcpad, *sinkpad;

static GstStaticCaps typefind_caps =
GST_STATIC_CAPS ("application/octet-stream");

static gboolean fake_typefind_caps;     /* FALSE */

static void
typefind_succeed (GstTypeFind * tf, gpointer private)
{
  if (fake_typefind_caps) {
    gst_type_find_suggest (tf, GST_TYPE_FIND_MAXIMUM,
        gst_static_caps_get (&typefind_caps));
  }
}

static gboolean
test_event_func (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GST_LOG_OBJECT (pad, "%s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  /* a sink would post tag events as messages, so do the same here,
   * esp. since we're polling on the bus waiting for TAG messages.. */
  if (GST_EVENT_TYPE (event) == GST_EVENT_TAG) {
    GstTagList *taglist;

    gst_event_parse_tag (event, &taglist);

    gst_bus_post (bus, gst_message_new_tag (GST_OBJECT (pad),
            gst_tag_list_copy (taglist)));
  }

  gst_event_unref (event);
  return TRUE;
}

static void
icydemux_found_pad (GstElement * src, GstPad * pad, gpointer data)
{
  GST_DEBUG ("got new pad %" GST_PTR_FORMAT, pad);

  /* Turns out that this asserts a refcount which is wrong for this
   * case (adding the pad from a pad-added callback), so just do the same
   * thing inline... */
  /* sinkpad = gst_check_setup_sink_pad (icydemux, &sinktemplate, NULL); */
  sinkpad = gst_pad_new_from_static_template (&sinktemplate, "sink");
  fail_if (sinkpad == NULL, "Couldn't create sinkpad");
  srcpad = gst_element_get_static_pad (icydemux, "src");
  fail_if (srcpad == NULL, "Failed to get srcpad from icydemux");
  gst_pad_set_chain_function (sinkpad, gst_check_chain_func);
  gst_pad_set_event_function (sinkpad, test_event_func);

  GST_DEBUG ("checking srcpad %p refcount", srcpad);
  /* 1 from element, 1 from signal, 1 from us */
  ASSERT_OBJECT_REFCOUNT (srcpad, "srcpad", 3);

  GST_DEBUG ("linking srcpad");
  fail_unless (gst_pad_link (srcpad, sinkpad) == GST_PAD_LINK_OK,
      "Failed to link pads");
  gst_object_unref (srcpad);

  gst_pad_set_active (sinkpad, TRUE);
}

static GstElement *
create_icydemux (void)
{
  icydemux = gst_check_setup_element ("icydemux");
  srcpad = gst_check_setup_src_pad (icydemux, &srctemplate);

  gst_pad_set_active (srcpad, TRUE);

  g_signal_connect (icydemux, "pad-added", G_CALLBACK (icydemux_found_pad),
      NULL);

  bus = gst_bus_new ();
  gst_element_set_bus (icydemux, bus);

  fail_unless (gst_element_set_state (icydemux, GST_STATE_PLAYING) !=
      GST_STATE_CHANGE_FAILURE, "could not set to playing");

  return icydemux;
}

static void
cleanup_icydemux (void)
{
  gst_bus_set_flushing (bus, TRUE);
  gst_object_unref (bus);
  bus = NULL;

  gst_check_drop_buffers ();
  gst_check_teardown_src_pad (icydemux);
  if (sinkpad)
    gst_check_teardown_sink_pad (icydemux);
  gst_check_teardown_element (icydemux);

  srcpad = NULL;
  sinkpad = NULL;
  icydemux = NULL;
}

static void
push_data (const guint8 * data, int len, gint64 offset)
{
  GstFlowReturn res;
  GstBuffer *buffer = gst_buffer_new_and_alloc (len);

  gst_buffer_fill (buffer, 0, data, len);

  GST_BUFFER_OFFSET (buffer) = offset;

  res = gst_pad_push (srcpad, buffer);

  fail_unless (res == GST_FLOW_OK, "Failed pushing buffer: %d", res);
}

GST_START_TEST (test_demux)
{
  GstMessage *message;
  GstTagList *tags;
  const GValue *tag_val;
  const gchar *tag;
  GstCaps *caps;

  fail_unless (gst_type_find_register (NULL, "success", GST_RANK_PRIMARY,
          typefind_succeed, NULL, gst_static_caps_get (&typefind_caps), NULL,
          NULL));

  fake_typefind_caps = TRUE;

  caps = gst_caps_from_string (ICYCAPS);

  create_icydemux ();
  gst_check_setup_events (srcpad, icydemux, caps, GST_FORMAT_TIME);

  push_data ((guint8 *) ICY_DATA, sizeof (ICY_DATA), -1);

  message = gst_bus_poll (bus, GST_MESSAGE_TAG, -1);
  fail_unless (message != NULL);

  gst_message_parse_tag (message, &tags);
  fail_unless (tags != NULL);

  tag_val = gst_tag_list_get_value_index (tags, GST_TAG_TITLE, 0);
  fail_unless (tag_val != NULL);

  tag = g_value_get_string (tag_val);
  fail_unless (tag != NULL);

  fail_unless_equals_string (TEST_METADATA, (char *) tag);

  gst_tag_list_unref (tags);
  gst_message_unref (message);
  gst_caps_unref (caps);

  cleanup_icydemux ();

  fake_typefind_caps = FALSE;
}

GST_END_TEST;

GST_START_TEST (test_demux_empty_data)
{
  GstMessage *message;
  GstTagList *tags;
  const GValue *tag_val;
  const gchar *tag;
  GstCaps *caps;

  fail_unless (gst_type_find_register (NULL, "success", GST_RANK_PRIMARY,
          typefind_succeed, NULL, gst_static_caps_get (&typefind_caps), NULL,
          NULL));

  fake_typefind_caps = TRUE;

  caps = gst_caps_from_string (ICYCAPS);

  create_icydemux ();
  gst_check_setup_events (srcpad, icydemux, caps, GST_FORMAT_TIME);

  push_data ((guint8 *) ICY_DATA_EMPTY_METADATA,
      sizeof (ICY_DATA_EMPTY_METADATA), -1);

  message = gst_bus_poll (bus, GST_MESSAGE_TAG, -1);
  fail_unless (message != NULL);

  gst_message_parse_tag (message, &tags);
  fail_unless (tags != NULL);

  tag_val = gst_tag_list_get_value_index (tags, GST_TAG_TITLE, 0);
  fail_unless (tag_val != NULL);

  tag = g_value_get_string (tag_val);
  fail_unless (tag != NULL);

  fail_unless_equals_string (TEST_METADATA, (char *) tag);

  gst_tag_list_unref (tags);
  gst_message_unref (message);

  message = gst_bus_poll (bus, GST_MESSAGE_TAG, -1);
  fail_unless (message != NULL);

  gst_message_parse_tag (message, &tags);
  fail_unless (tags != NULL);

  tag_val = gst_tag_list_get_value_index (tags, GST_TAG_TITLE, 0);
  fail_unless (tag_val == NULL);

  gst_message_unref (message);

  /* Ensure that no further tag messages are received, i.e. the empty ICY tag
   * is skipped */
  message = gst_bus_poll (bus, GST_MESSAGE_TAG, 100000000);
  fail_unless (message == NULL);

  gst_tag_list_unref (tags);
  gst_caps_unref (caps);

  cleanup_icydemux ();

  fake_typefind_caps = FALSE;
}

GST_END_TEST;

/* run this test first before the custom typefind function is set up */
GST_START_TEST (test_first_buf_offset_when_merged_for_typefinding)
{
  const guint8 buf1[] = { 'M' };
  const guint8 buf2[] = { 'P', '+', 0xff, 0xfb, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  GstCaps *icy_caps;
  GstPad *icy_srcpad;

  fake_typefind_caps = FALSE;

  create_icydemux ();

  icy_caps = gst_caps_from_string (ICYCAPS);

  gst_check_setup_events (srcpad, icydemux, icy_caps, GST_FORMAT_TIME);

  push_data (buf1, G_N_ELEMENTS (buf1), 0);

  /* one byte isn't really enough for typefinding, can't have a srcpad yet */
  fail_unless (gst_element_get_static_pad (icydemux, "src") == NULL);

  push_data (buf2, G_N_ELEMENTS (buf2), -1);

  /* should have been enough to create a audio/x-musepack source pad .. */
  icy_srcpad = gst_element_get_static_pad (icydemux, "src");
  fail_unless (icy_srcpad != NULL);
  gst_object_unref (icy_srcpad);

  fail_unless (g_list_length (buffers) > 0);

  /* first buffer should have offset 0 even after it was merged with 2nd buf */
  fail_unless (GST_BUFFER_OFFSET (GST_BUFFER_CAST (buffers->data)) == 0);

  gst_caps_unref (icy_caps);

  cleanup_icydemux ();
}

GST_END_TEST;

GST_START_TEST (test_not_negotiated)
{
  GstBuffer *buf;
  GstSegment segment;

  create_icydemux ();

  gst_segment_init (&segment, GST_FORMAT_BYTES);
  gst_pad_push_event (srcpad, gst_event_new_stream_start ("test"));
  gst_pad_push_event (srcpad, gst_event_new_segment (&segment));

  buf = gst_buffer_new_and_alloc (0);
  GST_BUFFER_OFFSET (buf) = 0;

  fail_unless_equals_int (gst_pad_push (srcpad, buf), GST_FLOW_NOT_NEGOTIATED);
  buf = NULL;

  cleanup_icydemux ();
}

GST_END_TEST;

static Suite *
icydemux_suite (void)
{
  Suite *s = suite_create ("icydemux");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_demux);
  tcase_add_test (tc_chain, test_demux_empty_data);
  tcase_add_test (tc_chain, test_first_buf_offset_when_merged_for_typefinding);
  tcase_add_test (tc_chain, test_not_negotiated);

  return s;
}

GST_CHECK_MAIN (icydemux)
