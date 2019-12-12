/* GStreamer
 *
 * unit test for matroskamux
 *
 * Copyright (C) <2005> Michal Benes <michal.benes@xeris.cz>
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
#include <gst/base/gstadapter.h>
#include <gst/check/gstharness.h>

#define AC3_CAPS_STRING "audio/x-ac3, " \
                        "channels = (int) 1, " \
                        "rate = (int) 8000"
#define VORBIS_TMPL_CAPS_STRING "audio/x-vorbis, " \
                                "channels = (int) 1, " \
                                "rate = (int) 8000, " \
                                "streamheader=(buffer)<10, 2020, 303030>"

static GstHarness *
setup_matroskamux_harness (const gchar * src_pad_str)
{
  GstHarness *h;

  h = gst_harness_new_with_padnames ("matroskamux", "audio_%u", "src");
  gst_harness_set_src_caps_str (h, src_pad_str);
  gst_harness_set_sink_caps_str (h, "video/x-matroska; audio/x-matroska");

  return h;
}

static gboolean
seekable_sinkpad_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean ret = FALSE;

  if (GST_QUERY_TYPE (query) == GST_QUERY_SEEKING) {
    gst_query_set_seeking (query, GST_FORMAT_BYTES, TRUE, 0, -1);
    ret = TRUE;
  }

  return ret;
}

#define compare_buffer_to_data(buffer, data, data_size)             \
G_STMT_START {                                                      \
fail_unless_equals_int (data_size, gst_buffer_get_size (buffer));   \
fail_unless (gst_buffer_memcmp (buffer, 0, data, data_size) == 0);  \
} G_STMT_END

static void
test_ebml_header_with_version (gint version,
    gconstpointer data, gsize data_size)
{
  GstHarness *h;
  GstBuffer *inbuffer, *outbuffer;

  h = setup_matroskamux_harness (AC3_CAPS_STRING);
  g_object_set (h->element, "version", version, NULL);

  inbuffer = gst_harness_create_buffer (h, 1);
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, inbuffer));
  fail_unless_equals_int (2, gst_harness_buffers_received (h));

  outbuffer = gst_harness_pull (h);
  compare_buffer_to_data (outbuffer, data, data_size);
  gst_buffer_unref (outbuffer);

  gst_harness_teardown (h);
}

GST_START_TEST (test_ebml_header_v1)
{
  guint8 data_v1[] = {
    0x1a, 0x45, 0xdf, 0xa3,     /* master ID */
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14,
    0x42, 0x82,                 /* doctype */
    0x89,                       /* 9 bytes */
    0x6d, 0x61, 0x74, 0x72, 0x6f, 0x73, 0x6b, 0x61, 0x00,       /* "matroska" */
    0x42, 0x87,                 /* doctypeversion */
    0x81,                       /* 1 byte */
    0x01,                       /* 1 */
    0x42, 0x85,                 /* doctypereadversion */
    0x81,                       /* 1 byte */
    0x01,                       /* 1 */
  };

  test_ebml_header_with_version (1, data_v1, sizeof (data_v1));
}

GST_END_TEST;

GST_START_TEST (test_ebml_header_v2)
{
  guint8 data_v2[] = {
    0x1a, 0x45, 0xdf, 0xa3,     /* master ID */
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14,
    0x42, 0x82,                 /* doctype */
    0x89,                       /* 9 bytes */
    0x6d, 0x61, 0x74, 0x72, 0x6f, 0x73, 0x6b, 0x61, 0x00,       /* "matroska" */
    0x42, 0x87,                 /* doctypeversion */
    0x81,                       /* 1 byte */
    0x02,                       /* 2 */
    0x42, 0x85,                 /* doctypereadversion */
    0x81,                       /* 1 byte */
    0x02,                       /* 2 */
  };

  test_ebml_header_with_version (2, data_v2, sizeof (data_v2));
}

GST_END_TEST;


GST_START_TEST (test_vorbis_header)
{
  GstHarness *h;
  GstBuffer *inbuffer, *outbuffer;
  gboolean vorbis_header_found = FALSE;
  gint j;
  gsize buffer_size;
  guint8 data[] =
      { 0x63, 0xa2, 0x89, 0x02, 0x01, 0x02, 0x10, 0x20, 0x20, 0x30, 0x30,
    0x30
  };

  h = setup_matroskamux_harness (VORBIS_TMPL_CAPS_STRING);

  inbuffer = gst_harness_create_buffer (h, 1);
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, inbuffer));

  outbuffer = gst_harness_pull (h);
  while (outbuffer != NULL) {
    buffer_size = gst_buffer_get_size (outbuffer);

    if (!vorbis_header_found && buffer_size >= sizeof (data)) {
      for (j = 0; j <= buffer_size - sizeof (data); j++) {
        if (gst_buffer_memcmp (outbuffer, j, data, sizeof (data)) == 0) {
          vorbis_header_found = TRUE;
          break;
        }
      }
    }

    ASSERT_BUFFER_REFCOUNT (outbuffer, "outbuffer", 1);
    gst_buffer_unref (outbuffer);

    outbuffer = gst_harness_try_pull (h);
  }

  fail_unless (vorbis_header_found);

  gst_harness_teardown (h);
}

GST_END_TEST;


static void
test_block_group_with_version (gint version,
    gconstpointer data0, gsize data0_size)
{
  GstHarness *h;
  GstBuffer *inbuffer, *outbuffer;
  guint8 data1[] = { 0x42 };

  h = setup_matroskamux_harness (AC3_CAPS_STRING);
  g_object_set (h->element, "version", version, NULL);

  /* Generate the header */
  inbuffer = gst_harness_create_buffer (h, 1);
  GST_BUFFER_TIMESTAMP (inbuffer) = 0;
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, inbuffer));
  fail_unless_equals_int (5, gst_harness_buffers_received (h));

  outbuffer = gst_harness_pull (h);
  fail_unless (outbuffer != NULL);
  while (outbuffer != NULL) {
    gst_buffer_unref (outbuffer);
    outbuffer = gst_harness_try_pull (h);
  }

  /* Now push a buffer */
  inbuffer = gst_harness_create_buffer (h, 1);
  gst_buffer_fill (inbuffer, 0, data1, sizeof (data1));
  GST_BUFFER_TIMESTAMP (inbuffer) = 1000000;

  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, inbuffer));

  outbuffer = gst_harness_pull (h);
  compare_buffer_to_data (outbuffer, data0, data0_size);
  gst_buffer_unref (outbuffer);

  outbuffer = gst_harness_pull (h);
  compare_buffer_to_data (outbuffer, data1, sizeof (data1));
  gst_buffer_unref (outbuffer);

  gst_harness_teardown (h);
}

GST_START_TEST (test_block_group_v1)
{
  guint8 data0_v1[] = { 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
    0xa1, 0x85,
    0x81, 0x00, 0x01, 0x00
  };

  test_block_group_with_version (1, data0_v1, sizeof (data0_v1));
}

GST_END_TEST;

GST_START_TEST (test_block_group_v2)
{
  guint8 data0_v2[] = { 0xa3, 0x85, 0x81, 0x00, 0x01, 0x00 };

  test_block_group_with_version (2, data0_v2, sizeof (data0_v2));
}

GST_END_TEST;

GST_START_TEST (test_reset)
{
  GstHarness *h;
  GstBuffer *inbuffer;
  GstBuffer *outbuffer;

  h = setup_matroskamux_harness (AC3_CAPS_STRING);

  inbuffer = gst_harness_create_buffer (h, 1);
  GST_BUFFER_TIMESTAMP (inbuffer) = 0;
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, inbuffer));
  fail_unless_equals_int (5, gst_harness_buffers_received (h));

  outbuffer = gst_harness_pull (h);
  fail_unless (outbuffer != NULL);
  while (outbuffer != NULL) {
    gst_buffer_unref (outbuffer);
    outbuffer = gst_harness_try_pull (h);
  }

  fail_unless_equals_int (GST_STATE_CHANGE_SUCCESS,
      gst_element_set_state (h->element, GST_STATE_NULL));

  gst_harness_play (h);

  inbuffer = gst_harness_create_buffer (h, 1);
  GST_BUFFER_TIMESTAMP (inbuffer) = 0;
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, inbuffer));

  outbuffer = gst_harness_pull (h);
  fail_unless (outbuffer != NULL);
  while (outbuffer != NULL) {
    gst_buffer_unref (outbuffer);
    outbuffer = gst_harness_try_pull (h);
  }

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_link_webmmux_webm_sink)
{
  GstHarness *h;

  h = gst_harness_new_with_padnames ("webmmux", "audio_%u", "src");
  fail_unless (h != NULL);

  gst_harness_set_sink_caps_str (h, "video/webm; audio/webm");

  gst_harness_play (h);

  fail_unless_equals_int (GST_STATE_CHANGE_SUCCESS,
      gst_element_set_state (h->element, GST_STATE_NULL));

  gst_harness_teardown (h);
}

GST_END_TEST;

static gint64 timecodescales[] = {
  GST_USECOND,
  GST_MSECOND,
  GST_MSECOND * 10,
  GST_MSECOND * 100,
  GST_MSECOND * 400,
  /* FAILS: ? GST_MSECOND * 500, a bug? */
};

GST_START_TEST (test_timecodescale)
{
  GstBuffer *inbuffer, *outbuffer;
  guint8 data_h0[] = {
    0xa3, 0x85, 0x81, 0x00, 0x00, 0x00,
  };
  guint8 data_h1[] = {
    0xa3, 0x85, 0x81, 0x00, 0x01, 0x00,
  };

  GstHarness *h = setup_matroskamux_harness (AC3_CAPS_STRING);
  gint64 timecodescale = timecodescales[__i__];

  g_object_set (h->element, "timecodescale", timecodescale, NULL);
  g_object_set (h->element, "version", 2, NULL);

  /* Buffer 0 */
  inbuffer = gst_harness_create_buffer (h, 1);
  GST_BUFFER_TIMESTAMP (inbuffer) = 0;
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, inbuffer));

  /* pull out headers */
  gst_buffer_unref (gst_harness_pull (h));
  gst_buffer_unref (gst_harness_pull (h));
  gst_buffer_unref (gst_harness_pull (h));

  /* verify header and drop the data */
  outbuffer = gst_harness_pull (h);
  compare_buffer_to_data (outbuffer, data_h0, sizeof (data_h0));
  gst_buffer_unref (outbuffer);
  gst_buffer_unref (gst_harness_pull (h));

  /* Buffer 1 */
  inbuffer = gst_harness_create_buffer (h, 1);
  GST_BUFFER_TIMESTAMP (inbuffer) = timecodescale;
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, inbuffer));

  /* verify header and drop the data */
  outbuffer = gst_harness_pull (h);
  compare_buffer_to_data (outbuffer, data_h1, sizeof (data_h1));
  gst_buffer_unref (outbuffer);
  gst_buffer_unref (gst_harness_pull (h));

  gst_harness_teardown (h);
}

GST_END_TEST;

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

/* Create a reference toc which includes a master edition entry */
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

/* Create a toc which includes chapters without edition entry */
static GstToc *
new_no_edition_toc (void)
{
  GstToc *ref_toc;
  GstTocEntry *toc_entry;

  ref_toc = gst_toc_new (GST_TOC_SCOPE_GLOBAL);

  toc_entry = new_chapter (1, 0 * GST_MSECOND, 2 * GST_MSECOND);
  gst_toc_append_entry (ref_toc, toc_entry);

  toc_entry = new_chapter (2, 2 * GST_MSECOND, 4 * GST_MSECOND);
  gst_toc_append_entry (ref_toc, toc_entry);

  return ref_toc;
}

static guint64
read_integer (GstMapInfo * info, gsize * index, guint64 len)
{
  guint64 total = 0;

  for (; len > 0; --len) {
    total = (total << 8) | GST_READ_UINT8 (info->data + *index);
    ++(*index);
  }

  return total;
}

static guint64
read_length (GstMapInfo * info, gsize * index)
{
  gint len_mask = 0x80, read = 1;
  guint64 total;
  guint8 b;

  b = GST_READ_UINT8 (info->data + *index);
  ++(*index);
  total = (guint64) b;
  while (read <= 8 && !(total & len_mask)) {
    read++;
    len_mask >>= 1;
  }
  total &= (len_mask - 1);

  for (; read > 1; --read) {
    total = (total << 8) | GST_READ_UINT8 (info->data + *index);
    ++(*index);
  }

  return total;
}

static gboolean
check_id (GstMapInfo * info, gsize * index,
    guint8 * tag, gint tag_len, guint64 * len)
{
  if (memcmp (info->data + *index, tag, tag_len) == 0) {
    *index += tag_len;
    *len = read_length (info, index);
    return TRUE;
  } else {
    return FALSE;
  }
}

static gboolean
check_id_read_int (GstMapInfo * info, gsize * index,
    guint8 * tag, gint tag_len, guint64 * value)
{
  guint64 len;

  if (check_id (info, index, tag, tag_len, &len)) {
    *value = read_integer (info, index, len);
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Check the toc entry against the muxed buffer
 * Returns the internal UID */
static void
check_chapter (GstTocEntry * toc_entry, GstTocEntry * internal_toc_entry,
    GstMapInfo * info, gsize * index, gint last_offset)
{
  guint64 len, value, uid;
  gint64 start_ref, end_ref;
  gchar s_uid[32];
  const gchar *str_uid;
  GstTocEntry *internal_chapter;
  GList *cur_sub_chap;
  GstTagList *tags;
  gchar *title;

  guint8 chapter_atom[] = { 0xb6 };
  guint8 chapter_uid[] = { 0x73, 0xc4 };
  guint8 chapter_str_uid[] = { 0x56, 0x54 };
  guint8 chapter_start[] = { 0x91 };
  guint8 chapter_end[] = { 0x92 };
  guint8 chapter_flag_hidden[] = { 0x98 };
  guint8 chapter_flag_enabled[] = { 0x45, 0x98 };
  guint8 chapter_segment_uid[] = { 0x6e, 0x67 };
  guint8 chapter_segment_edition_uid[] = { 0x6e, 0xbc };
  guint8 chapter_physical_equiv[] = { 0x63, 0xc3 };
  guint8 chapter_track[] = { 0x8f };
  guint8 chapter_track_nb[] = { 0x89 };
  guint8 chapter_display[] = { 0x80 };
  guint8 chapter_string[] = { 0x85 };
  guint8 chapter_language[] = { 0x43, 0x7c };

  fail_unless (check_id (info, index, chapter_atom,
          sizeof (chapter_atom), &len));

  fail_unless (check_id_read_int (info, index, chapter_uid,
          sizeof (chapter_uid), &uid));

  /* optional StringUID */
  if (check_id (info, index, chapter_str_uid, sizeof (chapter_str_uid), &len)) {
    str_uid = gst_toc_entry_get_uid (toc_entry);
    fail_unless (memcmp (info->data + *index, str_uid, strlen (str_uid)) == 0);
    *index += len;
  }

  gst_toc_entry_get_start_stop_times (toc_entry, &start_ref, &end_ref);

  fail_unless (check_id_read_int (info, index, chapter_start,
          sizeof (chapter_start), &value));
  fail_unless_equals_int (start_ref, value);

  /* optional chapter end */
  if (check_id_read_int (info, index, chapter_end,
          sizeof (chapter_end), &value)) {
    fail_unless_equals_int (end_ref, value);
  }

  fail_unless (check_id_read_int (info, index, chapter_flag_hidden,
          sizeof (chapter_flag_hidden), &value));

  fail_unless (check_id_read_int (info, index, chapter_flag_enabled,
          sizeof (chapter_flag_enabled), &value));

  /* optional segment UID */
  check_id_read_int (info, index, chapter_segment_uid,
      sizeof (chapter_segment_uid), &value);

  /* optional segment edition UID */
  check_id_read_int (info, index, chapter_segment_edition_uid,
      sizeof (chapter_segment_edition_uid), &value);

  /* optional physical equiv */
  check_id_read_int (info, index, chapter_physical_equiv,
      sizeof (chapter_physical_equiv), &value);

  /* optional chapter track */
  if (check_id (info, index, chapter_track, sizeof (chapter_track), &len)) {
    fail_unless (check_id_read_int (info, index, chapter_track_nb,
            sizeof (chapter_track_nb), &value));
  }

  /* FIXME: there can be several chapter displays */
  if (check_id (info, index, chapter_display, sizeof (chapter_display), &len)) {
    /* chapter display */
    fail_unless (check_id (info, index, chapter_string,
            sizeof (chapter_string), &len));

    tags = gst_toc_entry_get_tags (toc_entry);
    if (gst_tag_list_get_tag_size (tags, GST_TAG_TITLE) > 0) {
      gst_tag_list_get_string_index (tags, GST_TAG_TITLE, 0, &title);
      fail_unless (memcmp (info->data + *index, title, strlen (title)) == 0);
      g_free (title);
    }
    *index += len;

    fail_unless (check_id (info, index, chapter_language,
            sizeof (chapter_language), &len));
    /* TODO: define language - always "und" ATM */
    *index += len;
  }

  /* TODO: add remaining fields (not used in current matroska-mux) */

  g_snprintf (s_uid, sizeof (s_uid), "%" G_GINT64_FORMAT, uid);
  internal_chapter = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_CHAPTER, s_uid);
  gst_toc_entry_append_sub_entry (internal_toc_entry, internal_chapter);

  cur_sub_chap = gst_toc_entry_get_sub_entries (toc_entry);
  while (cur_sub_chap != NULL && *index < last_offset) {
    check_chapter (cur_sub_chap->data, internal_chapter, info,
        index, last_offset);
    cur_sub_chap = cur_sub_chap->next;
  }

  fail_unless (cur_sub_chap == NULL);
}

/* Check the reference toc against the muxed buffer */
static void
check_toc (GstToc * ref_toc, GstToc * internal_toc,
    GstMapInfo * info, gsize * index)
{
  guint64 len, value, uid;
  gchar s_uid[32];
  gint last_offset;
  GList *cur_entry, *cur_chapter;
  GstTocEntry *internal_edition;

  guint8 edition_entry[] = { 0x45, 0xb9 };
  guint8 edition_uid[] = { 0x45, 0xbc };
  guint8 edition_flag_hidden[] = { 0x45, 0xbd };
  guint8 edition_flag_default[] = { 0x45, 0xdb };
  guint8 edition_flag_ordered[] = { 0x45, 0xdd };

  /* edition entry */
  fail_unless (check_id (info, index, edition_entry,
          sizeof (edition_entry), &len));
  last_offset = *index + (gint) len;

  cur_entry = gst_toc_get_entries (ref_toc);
  while (cur_entry != NULL && *index < last_offset) {
    uid = 0;
    check_id_read_int (info, index, edition_uid, sizeof (edition_uid), &uid);
    g_snprintf (s_uid, sizeof (s_uid), "%" G_GINT64_FORMAT, uid);
    internal_edition = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_EDITION, s_uid);
    gst_toc_append_entry (internal_toc, internal_edition);

    fail_unless (check_id_read_int (info, index, edition_flag_hidden,
            sizeof (edition_flag_hidden), &value));

    fail_unless (check_id_read_int (info, index, edition_flag_default,
            sizeof (edition_flag_default), &value));

    /* optional */
    check_id_read_int (info, index, edition_flag_ordered,
        sizeof (edition_flag_ordered), &value);

    cur_chapter = gst_toc_entry_get_sub_entries (cur_entry->data);
    while (cur_chapter != NULL && *index < last_offset) {
      check_chapter (cur_chapter->data, internal_edition, info,
          index, last_offset);
      cur_chapter = cur_chapter->next;
    }
    fail_unless (cur_chapter == NULL);

    cur_entry = cur_entry->next;
  }

  fail_unless (cur_entry == NULL && *index == last_offset);
}

static GstTocEntry *
find_toc_entry (GstTocEntry * ref_toc_entry, GstTocEntry * internal_toc_entry,
    guint64 uid)
{
  GList *cur_ref_entry, *cur_internal_entry;
  guint64 internal_uid;
  GstTocEntry *result = NULL;

  internal_uid = g_ascii_strtoull (gst_toc_entry_get_uid (internal_toc_entry),
      NULL, 10);
  if (uid == internal_uid) {
    result = ref_toc_entry;
  } else {
    cur_ref_entry = gst_toc_entry_get_sub_entries (ref_toc_entry);
    cur_internal_entry = gst_toc_entry_get_sub_entries (internal_toc_entry);
    while (cur_ref_entry != NULL && cur_internal_entry != NULL) {
      result = find_toc_entry (cur_ref_entry->data, cur_internal_entry->data,
          uid);

      if (result != NULL) {
        break;
      }

      cur_ref_entry = cur_ref_entry->next;
      cur_internal_entry = cur_internal_entry->next;
    }
  }

  return result;
}

static void
find_and_check_tags (GstToc * ref_toc, GstToc * internal_toc, GstMapInfo * info,
    guint64 uid, gchar * tag_name, gchar * tag_string)
{
  GList *cur_ref_entry, *cur_internal_entry;
  GstTocEntry *ref_toc_entry = NULL;
  GstTagList *tags;
  const gchar *tag_type;
  gchar *cur_tag_string;

  /* find the reference toc entry matching the UID */
  cur_ref_entry = gst_toc_get_entries (ref_toc);
  cur_internal_entry = gst_toc_get_entries (internal_toc);
  while (cur_ref_entry != NULL && cur_internal_entry != NULL) {
    ref_toc_entry = find_toc_entry (cur_ref_entry->data,
        cur_internal_entry->data, uid);

    if (ref_toc_entry != NULL) {
      break;
    }

    cur_ref_entry = cur_ref_entry->next;
    cur_internal_entry = cur_internal_entry->next;
  }

  fail_unless (ref_toc_entry != NULL);

  if (g_strcmp0 (tag_name, "ARTIST") == 0) {
    tag_type = GST_TAG_ARTIST;
  } else if (g_strcmp0 (tag_name, "COMMENTS") == 0) {
    tag_type = GST_TAG_COMMENT;
  } else {
    tag_type = NULL;
  }

  fail_unless (tag_type != NULL);

  tags = gst_toc_entry_get_tags (ref_toc_entry);
  fail_unless (gst_tag_list_get_tag_size (tags, tag_type) > 0);
  gst_tag_list_get_string_index (tags, tag_type, 0, &cur_tag_string);
  fail_unless (g_strcmp0 (cur_tag_string, tag_string) == 0);
  g_free (cur_tag_string);
}

static void
check_tags (GstToc * ref_toc, GstToc * internal_toc,
    GstMapInfo * info, gsize * index)
{
  gboolean found_tags = FALSE, must_check_tag = FALSE;
  guint64 len, value, uid;
  gsize last_offset = 0;
  gsize next_tag;
  gchar *tag_name_str, *tag_string_str;
  guint8 tags[] = { 0x12, 0x54, 0xc3, 0x67 };
  guint8 tag[] = { 0x73, 0x73 };
  guint8 tag_targets[] = { 0x63, 0xc0 };
  guint8 tag_target_type_value[] = { 0x68, 0xca };
  guint8 tag_target_type[] = { 0x63, 0xca };
  guint8 tag_edition_uid[] = { 0x63, 0xc9 };
  guint8 tag_chapter_uid[] = { 0x63, 0xc4 };
  guint8 simple_tag[] = { 0x67, 0xc8 };
  guint8 tag_name[] = { 0x45, 0xa3 };
  guint8 tag_string[] = { 0x44, 0x87 };

  if (info->size > *index + sizeof (tags)) {
    for (; *index < info->size - sizeof (tags); ++(*index)) {
      if (memcmp (info->data + *index, tags, sizeof (tags)) == 0) {
        *index += sizeof (tags);

        len = read_length (info, index);
        last_offset = *index + len;

        found_tags = TRUE;
        break;
      }
    }
  }

  fail_unless (found_tags);

  while (*index < last_offset) {
    fail_unless (check_id (info, index, tag, sizeof (tag), &len));
    next_tag = *index + len;

    fail_unless (check_id (info, index, tag_targets,
            sizeof (tag_targets), &len));

    must_check_tag = FALSE;
    check_id_read_int (info, index, tag_target_type_value,
        sizeof (tag_target_type_value), &value);

    if (check_id (info, index, tag_target_type, sizeof (tag_target_type), &len)) {
      *index += len;
    }

    if (check_id_read_int (info, index, tag_chapter_uid,
            sizeof (tag_chapter_uid), &uid)) {
      must_check_tag = TRUE;
    } else if (check_id_read_int (info, index, tag_edition_uid,
            sizeof (tag_edition_uid), &uid)) {
      must_check_tag = TRUE;
    }

    if (must_check_tag) {
      fail_unless (check_id (info, index, simple_tag,
              sizeof (simple_tag), &len));

      fail_unless (check_id (info, index, tag_name, sizeof (tag_name), &len));
      tag_name_str = g_strndup ((gchar *) info->data + *index, len);
      *index += len;

      fail_unless (check_id (info, index, tag_string, sizeof (tag_string),
              &len));
      tag_string_str = g_strndup ((gchar *) info->data + *index, len);
      *index += len;

      find_and_check_tags (ref_toc, internal_toc, info, uid,
          tag_name_str, tag_string_str);

      g_free (tag_name_str);
      g_free (tag_string_str);
    }

    *index = next_tag;
  }
}

static void
check_segment (GstToc * ref_toc, GstToc * internal_toc,
    GstMapInfo * info, gsize * index)
{
  guint8 matroska_segment[] = { 0x18, 0x53, 0x80, 0x67 };
  guint8 matroska_seek_id_chapters[] = { 0x53, 0xab, 0x84,
    0x10, 0x43, 0xA7, 0x70
  };
  guint8 matroska_seek_id_tags[] = { 0x53, 0xab, 0x84,
    0x12, 0x54, 0xc3, 0x67
  };
  guint8 matroska_seek_pos[] = { 0x53, 0xac };
  guint8 matroska_chapters[] = { 0x10, 0x43, 0xA7, 0x70 };

  guint64 len, value, segment_offset;
  guint64 tags_offset = 0;
  guint64 chapters_offset = 0;
  gboolean found_chapters_declaration = FALSE, found_tags_declaration = FALSE;

  /* Segment */
  fail_unless (info->size > sizeof (matroska_segment));
  fail_unless (check_id (info, index, matroska_segment,
          sizeof (matroska_segment), &len));

  segment_offset = *index;

  /* Search chapter declaration in seek head */
  for (; *index < len - sizeof (matroska_seek_id_chapters); ++(*index)) {
    if (memcmp (info->data + *index, matroska_seek_id_chapters,
            sizeof (matroska_seek_id_chapters)) == 0) {
      *index += sizeof (matroska_seek_id_chapters);

      if (check_id_read_int (info, index, matroska_seek_pos,
              sizeof (matroska_seek_pos), &value)) {
        /* found chapter declaration */
        found_chapters_declaration = TRUE;
        chapters_offset = segment_offset + value;
        break;
      }
    }
  }

  fail_unless (found_chapters_declaration);

  *index = chapters_offset;
  if (check_id (info, index, matroska_chapters,
          sizeof (matroska_chapters), &len)) {
    check_toc (ref_toc, internal_toc, info, index);
  }

  /* Search tags declaration in seek head */
  for (*index = segment_offset; *index < len - sizeof (matroska_seek_id_tags);
      ++(*index)) {
    if (memcmp (info->data + *index, matroska_seek_id_tags,
            sizeof (matroska_seek_id_tags)) == 0) {
      *index += sizeof (matroska_seek_id_tags);

      if (check_id_read_int (info, index, matroska_seek_pos,
              sizeof (matroska_seek_pos), &value)) {
        /* found tags declaration */
        found_tags_declaration = TRUE;
        tags_offset = segment_offset + value;
        break;
      }
    }
  }

  fail_unless (found_tags_declaration);

  *index = tags_offset;
  check_tags (ref_toc, internal_toc, info, index);
}

static void
test_toc (gboolean with_edition)
{
  GstHarness *h;
  GstBuffer *inbuffer, *outbuffer, *merged_buffer;
  GstMapInfo info;
  guint64 len;
  gsize index;
  GstTocSetter *toc_setter;
  GstToc *test_toc, *ref_toc, *internal_toc;

  guint8 ebml_header[] = { 0x1a, 0x45, 0xdf, 0xa3 };

  h = setup_matroskamux_harness (AC3_CAPS_STRING);

  /* Make element seekable */
  gst_pad_set_query_function (h->sinkpad, seekable_sinkpad_query);

  toc_setter = GST_TOC_SETTER (h->element);
  fail_unless (toc_setter != NULL);

  if (with_edition) {
    test_toc = new_reference_toc ();
  } else {
    test_toc = new_no_edition_toc ();
  }
  gst_toc_setter_set_toc (toc_setter, test_toc);
  gst_toc_unref (test_toc);

  inbuffer = gst_harness_create_buffer (h, 1);
  gst_buffer_memset (inbuffer, 0, 0, 1);
  GST_BUFFER_TIMESTAMP (inbuffer) = 0;
  GST_BUFFER_DURATION (inbuffer) = 1 * GST_MSECOND;
  fail_unless_equals_int (GST_FLOW_OK, gst_harness_push (h, inbuffer));

  /* send eos to ensure everything is written */
  fail_unless (gst_harness_push_event (h, gst_event_new_eos ()));
  ASSERT_MINI_OBJECT_REFCOUNT (test_toc, "test_toc", 1);

  outbuffer = gst_harness_pull (h);
  fail_unless (outbuffer != NULL);

  /* Merge buffers */
  merged_buffer = gst_buffer_new ();
  while (outbuffer != NULL) {
    if (outbuffer->offset == gst_buffer_get_size (merged_buffer)) {
      gst_buffer_append_memory (merged_buffer,
          gst_buffer_get_all_memory (outbuffer));
    } else {
      fail_unless (gst_buffer_map (outbuffer, &info, GST_MAP_READ));
      gst_buffer_fill (merged_buffer, outbuffer->offset, info.data, info.size);
      gst_buffer_unmap (outbuffer, &info);
    }

    gst_buffer_unref (outbuffer);
    outbuffer = gst_harness_try_pull (h);
  }

  fail_unless (gst_buffer_map (merged_buffer, &info, GST_MAP_READ));
  index = 0;

  fail_unless (check_id (&info, &index, ebml_header,
          sizeof (ebml_header), &len));
  /* skip header */
  index += len;

  ref_toc = new_reference_toc ();
  internal_toc = gst_toc_new (GST_TOC_SCOPE_GLOBAL);
  check_segment (ref_toc, internal_toc, &info, &index);
  gst_toc_unref (internal_toc);
  gst_toc_unref (ref_toc);

  gst_buffer_unmap (merged_buffer, &info);
  gst_buffer_unref (merged_buffer);
  gst_harness_teardown (h);
}

GST_START_TEST (test_toc_with_edition)
{
  test_toc (TRUE);
}

GST_END_TEST;

GST_START_TEST (test_toc_without_edition)
{
  test_toc (FALSE);
}

GST_END_TEST;

static Suite *
matroskamux_suite (void)
{
  Suite *s = suite_create ("matroskamux");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_ebml_header_v1);
  tcase_add_test (tc_chain, test_ebml_header_v2);
  tcase_add_test (tc_chain, test_vorbis_header);
  tcase_add_test (tc_chain, test_block_group_v1);
  tcase_add_test (tc_chain, test_block_group_v2);

  tcase_add_test (tc_chain, test_reset);
  tcase_add_test (tc_chain, test_link_webmmux_webm_sink);
  tcase_add_loop_test (tc_chain, test_timecodescale,
      0, G_N_ELEMENTS (timecodescales));

  tcase_add_test (tc_chain, test_toc_with_edition);
  tcase_add_test (tc_chain, test_toc_without_edition);
  return s;
}

GST_CHECK_MAIN (matroskamux);
