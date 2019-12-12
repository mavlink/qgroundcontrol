/* GStreamer
 *
 * unit test for avisubtitle
 *
 * Copyright (C) <2007> Thijs Vermeir <thijsvermeir@gmail.com>
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
/* Element-Checklist-Version: 5 */

#include <gst/gst.h>
#include <gst/check/gstcheck.h>

GstPad *mysinkpad;
GstPad *mysrcpad;

guint8 avisub_utf_8_with_bom[] = {
  0x47, 0x41, 0x42, 0x32, 0x00, 0x02, 0x00, 0x10,
  0x00, 0x00, 0x00, 0x45, 0x00, 0x6e, 0x00, 0x67,
  0x00, 0x6c, 0x00, 0x69, 0x00, 0x73, 0x00, 0x68,
  0x00, 0x00, 0x00, 0x04, 0x00, 0x8e, 0x00, 0x00,
  0x00, 0xef, 0xbb, 0xbf, 0x31, 0x0d, 0x0a, 0x30,
  0x30, 0x3a, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x2c,
  0x31, 0x30, 0x30, 0x20, 0x2d, 0x2d, 0x3e, 0x20,
  0x30, 0x30, 0x3a, 0x30, 0x30, 0x3a, 0x30, 0x32,
  0x2c, 0x30, 0x30, 0x30, 0x0d, 0x0a, 0x3c, 0x62,
  0x3e, 0x41, 0x6e, 0x20, 0x55, 0x54, 0x46, 0x38,
  0x20, 0x53, 0x75, 0x62, 0x74, 0x69, 0x74, 0x6c,
  0x65, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x42,
  0x4f, 0x4d, 0x3c, 0x2f, 0x62, 0x3e, 0x0d, 0x0a,
  0x0d, 0x0a, 0x32, 0x0d, 0x0a, 0x30, 0x30, 0x3a,
  0x30, 0x30, 0x3a, 0x30, 0x32, 0x2c, 0x31, 0x30,
  0x30, 0x20, 0x2d, 0x2d, 0x3e, 0x20, 0x30, 0x30,
  0x3a, 0x30, 0x30, 0x3a, 0x30, 0x34, 0x2c, 0x30,
  0x30, 0x30, 0x0d, 0x0a, 0x53, 0x6f, 0x6d, 0x65,
  0x74, 0x68, 0x69, 0x6e, 0x67, 0x20, 0x6e, 0x6f,
  0x6e, 0x41, 0x53, 0x43, 0x49, 0x49, 0x20, 0x2d,
  0x20, 0xc2, 0xb5, 0xc3, 0xb6, 0xc3, 0xa4, 0xc3,
  0xbc, 0xc3, 0x9f, 0x0d, 0x0a, 0x0d, 0x0a
};

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-subtitle")
    );

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-subtitle-avi")
    );

static GstElement *
setup_avisubtitle (void)
{
  GstElement *avisubtitle;
  GstCaps *srccaps;

  GST_DEBUG ("setup_avisubtitle");
  avisubtitle = gst_check_setup_element ("avisubtitle");
  mysinkpad = gst_check_setup_sink_pad (avisubtitle, &sink_template);
  mysrcpad = gst_check_setup_src_pad (avisubtitle, &src_template);
  gst_pad_set_active (mysinkpad, TRUE);
  gst_pad_set_active (mysrcpad, TRUE);
  srccaps = gst_caps_new_empty_simple ("application/x-subtitle-avi");
  gst_check_setup_events (mysrcpad, avisubtitle, srccaps, GST_FORMAT_TIME);
  gst_caps_unref (srccaps);
  return avisubtitle;
}

static void
cleanup_avisubtitle (GstElement * avisubtitle)
{
  gst_pad_set_active (mysinkpad, FALSE);
  gst_pad_set_active (mysrcpad, FALSE);
  gst_check_teardown_sink_pad (avisubtitle);
  gst_check_teardown_src_pad (avisubtitle);
  gst_check_teardown_element (avisubtitle);
}

static void
check_wrong_buffer (guint8 * data, guint length)
{
  GstBuffer *buffer = gst_buffer_new_allocate (NULL, length, 0);
  GstElement *avisubtitle = setup_avisubtitle ();

  gst_buffer_fill (buffer, 0, data, length);
  fail_unless (gst_element_set_state (avisubtitle,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");
  gst_buffer_ref (buffer);
  ASSERT_BUFFER_REFCOUNT (buffer, "inbuffer", 2);
  /* push the broken buffer */
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_ERROR,
      "accepted a broken buffer");
  /* check if we have unreffed this buffer on failure */
  ASSERT_BUFFER_REFCOUNT (buffer, "inbuffer", 1);
  gst_buffer_unref (buffer);
  fail_unless (gst_element_set_state (avisubtitle,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS, "could not set to null");
  cleanup_avisubtitle (avisubtitle);
}

static void
check_correct_buffer (guint8 * src_data, guint src_size, guint8 * dst_data,
    guint dst_size)
{
  GstBuffer *buffer = gst_buffer_new_allocate (NULL, src_size, 0);
  GstBuffer *newBuffer;
  GstElement *avisubtitle = setup_avisubtitle ();
  GstEvent *event;

  fail_unless (g_list_length (buffers) == 0, "Buffers list needs to be empty");
  gst_buffer_fill (buffer, 0, src_data, src_size);
  fail_unless (gst_element_set_state (avisubtitle,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");
  ASSERT_BUFFER_REFCOUNT (buffer, "inbuffer", 1);
  event = gst_event_new_seek (1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, 2 * GST_SECOND, GST_SEEK_TYPE_SET, 5 * GST_SECOND);
  fail_unless (gst_element_send_event (avisubtitle, event) == FALSE,
      "Seeking is not possible when there is no buffer yet");
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK,
      "not accepted a correct buffer");
  /* we gave away our reference to the buffer, don't assume anything */
  buffer = NULL;
  /* a new buffer is created in the list */
  fail_unless (g_list_length (buffers) == 1,
      "No new buffer in the buffers list");
  event = gst_event_new_seek (1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, 2 * GST_SECOND, GST_SEEK_TYPE_SET, 5 * GST_SECOND);
  fail_unless (gst_element_send_event (avisubtitle, event) == TRUE,
      "seeking should be working now");
  fail_unless (g_list_length (buffers) == 2,
      "After seeking we need another buffer in the buffers");
  newBuffer = GST_BUFFER (buffers->data);
  buffers = g_list_remove (buffers, newBuffer);
  fail_unless (g_list_length (buffers) == 1, "Buffers list needs to be empty");
  fail_unless (gst_buffer_get_size (newBuffer) == dst_size,
      "size of the new buffer is wrong ( %d != %d)",
      gst_buffer_get_size (newBuffer), dst_size);
  fail_unless (gst_buffer_memcmp (newBuffer, 0, dst_data, dst_size) == 0,
      "data of the buffer is not correct");
  gst_buffer_unref (newBuffer);
  /* free the buffer from seeking */
  gst_buffer_unref (GST_BUFFER (buffers->data));
  buffers = g_list_remove (buffers, buffers->data);
  fail_unless (gst_element_set_state (avisubtitle,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS, "could not set to null");
  cleanup_avisubtitle (avisubtitle);
}


GST_START_TEST (test_avisubtitle_negative)
{
  guint8 wrong_magic[] =
      { 0x47, 0x41, 0x41, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
  };
  guint8 wrong_fixed_word_2[] = {
    0x47, 0x41, 0x42, 0x32, 0x00, 0x02, 0x01, 0x10,
    0x00, 0x00, 0x00, 0x45, 0x00, 0x6e, 0x00, 0x67,
    0x00, 0x6c, 0x00, 0x69, 0x00, 0x73, 0x00, 0x68
  };
  guint8 wrong_length_after_name[] = {
    0x47, 0x41, 0x42, 0x32, 0x00, 0x02, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x45, 0x00, 0x6e, 0x00, 0x67,
    0x00, 0x6c, 0x00, 0x69, 0x00, 0x73, 0x00, 0x68
  };
  guint8 wrong_fixed_word_4[] = {
    0x47, 0x41, 0x42, 0x32, 0x00, 0x02, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x45, 0x00, 0x6e, 0x00, 0x67,
    0x00, 0x6c, 0x00, 0x69, 0x00, 0x73, 0x00, 0x68,
    0x00, 0x00, 0x00, 0x04, 0x01, 0x8e, 0x00, 0x00,
    0x00, 0xef, 0xbb, 0xbf, 0x31, 0x0d, 0x0a, 0x30
  };
  guint8 wrong_total_length[] = {
    0x47, 0x41, 0x42, 0x32, 0x00, 0x02, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x45, 0x00, 0x6e, 0x00, 0x67,
    0x00, 0x6c, 0x00, 0x69, 0x00, 0x73, 0x00, 0x68,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x8e, 0x00, 0x00,
    0x00, 0xef, 0xbb, 0xbf, 0x31, 0x0d, 0x0a, 0x30
  };
  /* size of the buffer must be larger than 11 */
  check_wrong_buffer (avisub_utf_8_with_bom, 11);
  /* buffer must start with 'GAB2\0' */
  check_wrong_buffer (wrong_magic, 14);
  /* next word must be 2 */
  check_wrong_buffer (wrong_fixed_word_2, 24);
  /* length must be larger than the length of the name + 17 */
  check_wrong_buffer (wrong_length_after_name, 24);
  /* next word must be 4 */
  check_wrong_buffer (wrong_fixed_word_4, 36);
  /* check wrong total length */
  check_wrong_buffer (wrong_total_length, 36);
}

GST_END_TEST;

GST_START_TEST (test_avisubtitle_positive)
{
  guint8 avisub_utf_8_without_bom[] = {
    0x47, 0x41, 0x42, 0x32, 0x00, 0x02, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x45, 0x00, 0x6e, 0x00, 0x67,
    0x00, 0x6c, 0x00, 0x69, 0x00, 0x73, 0x00, 0x68,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x8b, 0x00, 0x00,
    0x00, 0x31, 0x0d, 0x0a, 0x30,
    0x30, 0x3a, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x2c,
    0x31, 0x30, 0x30, 0x20, 0x2d, 0x2d, 0x3e, 0x20,
    0x30, 0x30, 0x3a, 0x30, 0x30, 0x3a, 0x30, 0x32,
    0x2c, 0x30, 0x30, 0x30, 0x0d, 0x0a, 0x3c, 0x62,
    0x3e, 0x41, 0x6e, 0x20, 0x55, 0x54, 0x46, 0x38,
    0x20, 0x53, 0x75, 0x62, 0x74, 0x69, 0x74, 0x6c,
    0x65, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x42,
    0x4f, 0x4d, 0x3c, 0x2f, 0x62, 0x3e, 0x0d, 0x0a,
    0x0d, 0x0a, 0x32, 0x0d, 0x0a, 0x30, 0x30, 0x3a,
    0x30, 0x30, 0x3a, 0x30, 0x32, 0x2c, 0x31, 0x30,
    0x30, 0x20, 0x2d, 0x2d, 0x3e, 0x20, 0x30, 0x30,
    0x3a, 0x30, 0x30, 0x3a, 0x30, 0x34, 0x2c, 0x30,
    0x30, 0x30, 0x0d, 0x0a, 0x53, 0x6f, 0x6d, 0x65,
    0x74, 0x68, 0x69, 0x6e, 0x67, 0x20, 0x6e, 0x6f,
    0x6e, 0x41, 0x53, 0x43, 0x49, 0x49, 0x20, 0x2d,
    0x20, 0xc2, 0xb5, 0xc3, 0xb6, 0xc3, 0xa4, 0xc3,
    0xbc, 0xc3, 0x9f, 0x0d, 0x0a, 0x0d, 0x0a
  };
  check_correct_buffer (avisub_utf_8_with_bom, 175, avisub_utf_8_with_bom + 36,
      139);
  check_correct_buffer (avisub_utf_8_without_bom, 172,
      avisub_utf_8_without_bom + 33, 139);
}

GST_END_TEST;

static Suite *
avisubtitle_suite (void)
{
  Suite *s = suite_create ("avisubtitle");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_avisubtitle_negative);
  tcase_add_test (tc_chain, test_avisubtitle_positive);

  return s;
}

GST_CHECK_MAIN (avisubtitle);
