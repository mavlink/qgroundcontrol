/* GStreamer unit test for matroskademux
 * Copyright (C) 2015 Tim-Philipp Müller <tim@centricular.com>
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

const gchar mkv_sub_base64[] =
    "GkXfowEAAAAAAAAUQoKJbWF0cm9za2EAQoeBAkKFgQIYU4BnAQAAAAAAAg0RTZt0AQAAAAAAAIxN"
    "uwEAAAAAAAASU6uEFUmpZlOsiAAAAAAAAACYTbsBAAAAAAAAElOrhBZUrmtTrIgAAAAAAAABEuya"
    "AQAAAAAAABJTq4QQQ6dwU6yI///////////smgEAAAAAAAASU6uEHFO7a1OsiP//////////TbsB"
    "AAAAAAAAElOrhBJUw2dTrIgAAAAAAAAB9xVJqWYBAAAAAAAAbnOkkDylQZJlrLziQo8+gsrZVtUq"
    "17GDD0JARImIQNGUAAAAAABNgJ9HU3RyZWFtZXIgcGx1Z2luIHZlcnNpb24gMS40LjUAV0GZR1N0"
    "cmVhbWVyIE1hdHJvc2thIG11eGVyAERhiAZfU0rcEwgAFlSuawEAAAAAAAA0rgEAAAAAAAAr14EB"
    "g4ERc8WIoWF8pYlELidTbolTdWJ0aXRsZQCGjFNfVEVYVC9VVEY4AB9DtnUBAAAAAAAAmeeCA+ig"
    "AQAAAAAAAA2bggfQoYeBAAAAZm9voAEAAAAAAAAUm4IH0KGOgQu4ADxpPmJhcjwvaT6gAQAAAAAA"
    "AA2bggfQoYeBF3AAYmF6oAEAAAAAAAAOm4IH0KGIgScQAGbDtgCgAQAAAAAAABWbggfQoY+BMsgA"
    "PGk+YmFyPC9pPgCgAQAAAAAAAA6bggfQoYiBPoAAYuR6ABJUw2cBAAAAAAAACnNzAQAAAAAAAAA=";

const gchar mkv_toc_base64[] =
    "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";

static void
pad_added_cb (GstElement * matroskademux, GstPad * pad, gpointer user_data)
{
  GstHarness *h = user_data;

  GST_LOG_OBJECT (pad, "got new source pad");
  gst_harness_add_element_src_pad (h, pad);
}

static void
pull_and_check_buffer (GstHarness * h, GstClockTime pts, GstClockTime duration,
    const gchar * output)
{
  GstMapInfo map;
  GstBuffer *buf;

  /* wait for buffer */
  buf = gst_harness_pull (h);

  /* Make sure there's no 0-terminator in there */
  fail_unless (gst_buffer_map (buf, &map, GST_MAP_READ));
  GST_MEMDUMP ("subtitle buffer", map.data, map.size);
  fail_unless (map.size > 0);
  fail_unless (map.data[map.size - 1] != '\0');
  if (output != NULL && memcmp (map.data, output, map.size) != 0) {
    g_printerr ("Got:\n");
    gst_util_dump_mem (map.data, map.size);;
    g_printerr ("Wanted:\n");
    gst_util_dump_mem ((guint8 *) output, strlen (output));
    g_error ("Did not get output expected.");
  }

  gst_buffer_unmap (buf, &map);

  fail_unless_equals_int64 (pts, GST_BUFFER_PTS (buf));
  fail_unless_equals_int64 (duration, GST_BUFFER_DURATION (buf));

  gst_buffer_unref (buf);
}

GST_START_TEST (test_sub_terminator)
{
  GstHarness *h;
  GstBuffer *buf;
  guchar *mkv_data;
  gsize mkv_size;

  h = gst_harness_new_with_padnames ("matroskademux", "sink", NULL);

  g_signal_connect (h->element, "pad-added", G_CALLBACK (pad_added_cb), h);

  mkv_data = g_base64_decode (mkv_sub_base64, &mkv_size);
  fail_unless (mkv_data != NULL);

  gst_harness_set_src_caps_str (h, "video/x-matroska");

  buf = gst_buffer_new_wrapped (mkv_data, mkv_size);
  GST_BUFFER_OFFSET (buf) = 0;

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, buf));
  gst_harness_push_event (h, gst_event_new_eos ());

  pull_and_check_buffer (h, 1 * GST_SECOND, 2 * GST_SECOND, "foo");
  pull_and_check_buffer (h, 4 * GST_SECOND, 2 * GST_SECOND, "<i>bar</i>");
  pull_and_check_buffer (h, 7 * GST_SECOND, 2 * GST_SECOND, "baz");
  pull_and_check_buffer (h, 11 * GST_SECOND, 2 * GST_SECOND, "f\303\266");
  pull_and_check_buffer (h, 14 * GST_SECOND, 2 * GST_SECOND, "<i>bar</i>");
  /* The input is invalid UTF-8 here, what happens might depend on locale */
  pull_and_check_buffer (h, 17 * GST_SECOND, 2 * GST_SECOND, NULL);

  fail_unless (gst_harness_try_pull (h) == NULL);

  gst_harness_teardown (h);
}

GST_END_TEST;

/* Recursively compare 2 toc entries */
static void
check_toc_entries (const GstTocEntry * original, const GstTocEntry * other)
{
  gint64 start, stop, other_start, other_stop;
  GstTocEntryType original_type, other_type;
  const gchar *original_string_uid = NULL, *other_string_uid = NULL;
  GstTagList *original_tags, *other_tags;
  GList *cur, *other_cur;

  original_type = gst_toc_entry_get_entry_type (original);
  other_type = gst_toc_entry_get_entry_type (other);
  fail_unless (original_type == other_type);

  if (original_type != GST_TOC_ENTRY_TYPE_EDITION) {
    original_string_uid = gst_toc_entry_get_uid (original);
    other_string_uid = gst_toc_entry_get_uid (other);
    fail_unless (g_strcmp0 (original_string_uid, other_string_uid) == 0);
  }

  if (original_type != GST_TOC_ENTRY_TYPE_EDITION) {
    gst_toc_entry_get_start_stop_times (original, &start, &stop);
    gst_toc_entry_get_start_stop_times (other, &other_start, &other_stop);

    fail_unless (start == other_start && stop == other_stop);
  }

  /* tags */
  original_tags = gst_toc_entry_get_tags (original);
  other_tags = gst_toc_entry_get_tags (other);
  fail_unless (gst_tag_list_is_equal (original_tags, other_tags));

  other_cur = gst_toc_entry_get_sub_entries (other);
  for (cur = gst_toc_entry_get_sub_entries (original); cur != NULL;
      cur = cur->next) {
    fail_unless (other_cur != NULL);

    check_toc_entries (cur->data, other_cur->data);

    other_cur = other_cur->next;
  }
}

/* Create a new chapter */
static GstTocEntry *
new_chapter (const guint chapter_nb, const gint64 start, const gint64 stop)
{
  GstTocEntry *toc_entry, *toc_sub_entry;
  GstTagList *tags;
  gchar title[32];
  gchar artist[32];
  gchar str_uid[32];

  g_snprintf (str_uid, sizeof (str_uid), "uid.%d", chapter_nb);
  toc_entry = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_CHAPTER, str_uid);
  gst_toc_entry_set_start_stop_times (toc_entry, start, stop);

  g_snprintf (title, sizeof (title), "chap.%d", chapter_nb);
  g_snprintf (artist, sizeof (artist), "art.%d", chapter_nb);
  tags = gst_tag_list_new (GST_TAG_TITLE, title, GST_TAG_ARTIST, artist, NULL);
  gst_toc_entry_set_tags (toc_entry, tags);

  g_snprintf (str_uid, sizeof (str_uid), "uid.%d.1", chapter_nb);
  toc_sub_entry = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_CHAPTER, str_uid);
  gst_toc_entry_set_start_stop_times (toc_sub_entry, start, (start + stop) / 2);

  g_snprintf (title, sizeof (title), "nested.%d.1", chapter_nb);
  g_snprintf (artist, sizeof (artist), "art.%d.1", chapter_nb);
  tags = gst_tag_list_new (GST_TAG_TITLE, title, GST_TAG_ARTIST, artist, NULL);
  gst_toc_entry_set_tags (toc_sub_entry, tags);

  gst_toc_entry_append_sub_entry (toc_entry, toc_sub_entry);

  g_snprintf (str_uid, sizeof (str_uid), "uid.%d.2", chapter_nb);
  toc_sub_entry = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_CHAPTER, str_uid);
  gst_toc_entry_set_start_stop_times (toc_sub_entry, (start + stop) / 2, stop);

  g_snprintf (title, sizeof (title), "nested/%d.2", chapter_nb);
  g_snprintf (artist, sizeof (artist), "art.%d.2", chapter_nb);
  tags = gst_tag_list_new (GST_TAG_TITLE, title, GST_TAG_ARTIST, artist, NULL);
  gst_toc_entry_set_tags (toc_sub_entry, tags);

  gst_toc_entry_append_sub_entry (toc_entry, toc_sub_entry);

  return toc_entry;
}

/* Create a reference toc which matches what is expected in mkv_toc_base64 */
static GstToc *
new_reference_toc (void)
{
  GstToc *ref_toc;
  GstTocEntry *toc_edition_entry, *toc_entry;
  GstTagList *tags;

  ref_toc = gst_toc_new (GST_TOC_SCOPE_GLOBAL);

  toc_edition_entry = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_EDITION, "00");
  tags = gst_tag_list_new (GST_TAG_COMMENT, "Ed", NULL);
  gst_toc_entry_set_tags (toc_edition_entry, tags);

  toc_entry = new_chapter (1, 0 * GST_MSECOND, 2 * GST_MSECOND);
  gst_toc_entry_append_sub_entry (toc_edition_entry, toc_entry);

  toc_entry = new_chapter (2, 2 * GST_MSECOND, 4 * GST_MSECOND);
  gst_toc_entry_append_sub_entry (toc_edition_entry, toc_entry);

  gst_toc_append_entry (ref_toc, toc_edition_entry);

  return ref_toc;
}

GST_START_TEST (test_toc_demux)
{
  GstHarness *h;
  GstBuffer *buf;
  guchar *mkv_data;
  gsize mkv_size;
  GstEvent *event;
  gboolean update;
  GstToc *ref_toc, *demuxed_toc = NULL;
  GList *ref_cur, *demuxed_cur;

  h = gst_harness_new_with_padnames ("matroskademux", "sink", NULL);

  g_signal_connect (h->element, "pad-added", G_CALLBACK (pad_added_cb), h);

  mkv_data = g_base64_decode (mkv_toc_base64, &mkv_size);
  fail_unless (mkv_data != NULL);

  gst_harness_set_src_caps_str (h, "audio/x-matroska");

  buf = gst_buffer_new_wrapped (mkv_data, mkv_size);
  GST_BUFFER_OFFSET (buf) = 0;

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, buf));
  gst_harness_push_event (h, gst_event_new_eos ());

  event = gst_harness_try_pull_event (h);
  fail_unless (event != NULL);

  while (event != NULL) {
    if (event->type == GST_EVENT_TOC) {
      gst_event_parse_toc (event, &demuxed_toc, &update);
      gst_event_unref (event);
      break;
    }
    gst_event_unref (event);
    event = gst_harness_try_pull_event (h);
  }

  fail_unless (demuxed_toc != NULL);
  ref_toc = new_reference_toc ();

  demuxed_cur = gst_toc_get_entries (demuxed_toc);
  for (ref_cur = gst_toc_get_entries (ref_toc); ref_cur != NULL;
      ref_cur = ref_cur->next) {
    fail_unless (demuxed_cur != NULL);

    check_toc_entries (ref_cur->data, demuxed_cur->data);
    demuxed_cur = demuxed_cur->next;
  }

  gst_toc_unref (ref_toc);
  gst_toc_unref (demuxed_toc);

  gst_harness_teardown (h);
}

GST_END_TEST;

static Suite *
matroskademux_suite (void)
{
  Suite *s = suite_create ("matroskademux");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_sub_terminator);
  tcase_add_test (tc_chain, test_toc_demux);

  return s;
}

GST_CHECK_MAIN (matroskademux);
