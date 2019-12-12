/* GStreamer unit test for the alpha element
 *
 * Copyright (C) 2007 Ravi Kiran K N <ravi.kiran@samsung.com>
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
#include <gst/video/video.h>


GstPad *srcpad, *sinkpad;

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("AYUV"))
    );
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ AYUV, "
            "ARGB, BGRA, ABGR, RGBA, Y444, xRGB, BGRx, xBGR, "
            "RGBx, RGB, BGR, Y42B, YUY2, YVYU, UYVY, I420, YV12, Y41B } "))
    );


typedef enum
{
  FILL_GREEN,
  FILL_BLUE
}
FillColor;

static GstElement *
setup_alpha (void)
{
  GstElement *alpha;

  alpha = gst_check_setup_element ("alpha");
  srcpad = gst_check_setup_src_pad (alpha, &srctemplate);
  sinkpad = gst_check_setup_sink_pad (alpha, &sinktemplate);

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  return alpha;
}

static void
cleanup_alpha (GstElement * alpha)
{
  gst_pad_set_active (srcpad, FALSE);
  gst_pad_set_active (sinkpad, FALSE);
  gst_check_teardown_src_pad (alpha);
  gst_check_teardown_sink_pad (alpha);
  gst_check_teardown_element (alpha);
}

#define WIDTH 3
#define HEIGHT 4

static GstCaps *
create_caps_rgba32 (void)
{
  GstCaps *caps;

  caps = gst_caps_new_simple ("video/x-raw",
      "width", G_TYPE_INT, WIDTH,
      "height", G_TYPE_INT, HEIGHT,
      "framerate", GST_TYPE_FRACTION, 0, 1,
      "format", G_TYPE_STRING, "RGBA", NULL);

  return caps;
}

static GstBuffer *
create_buffer_rgba32 (FillColor color)
{
  guint8 rgba32_img[HEIGHT * WIDTH * 4];
  guint32 *rgba32 = (guint32 *) rgba32_img;

  GstBuffer *buf;
  GstMapInfo map;
  guint32 rgba_col;
  int i;

  if (color == FILL_GREEN)
    rgba_col = 0xff00ff00;      /* GREEN */
  else
    rgba_col = 0xffff0000;      /* BLUE */

  for (i = 0; i < HEIGHT * WIDTH; i++)
    rgba32[i] = rgba_col;

  buf = gst_buffer_new_and_alloc (HEIGHT * WIDTH * 4);
  gst_buffer_map (buf, &map, GST_MAP_READWRITE);
  fail_unless_equals_int (map.size, sizeof (rgba32_img));
  memcpy (map.data, rgba32_img, sizeof (rgba32_img));

  gst_buffer_unmap (buf, &map);

  return buf;
}


GST_START_TEST (test_chromakeying)
{
  GstElement *alpha;
  GstBuffer *inbuffer;
  GstBuffer *outbuffer;
  GstCaps *incaps;
  guint8 *ayuv;
  guint outlength;
  GstMapInfo map;
  int i;

  incaps = create_caps_rgba32 ();

  alpha = setup_alpha ();

  g_object_set (alpha, "method", 1, NULL);      /* Chroma-keying GREEN */

  fail_unless_equals_int (gst_element_set_state (alpha, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  gst_check_setup_events (srcpad, alpha, incaps, GST_FORMAT_TIME);

  inbuffer = create_buffer_rgba32 (FILL_GREEN);
  GST_DEBUG ("Created buffer of %" G_GSIZE_FORMAT " bytes",
      gst_buffer_get_size (inbuffer));
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  fail_unless_equals_int (gst_pad_push (srcpad, inbuffer), GST_FLOW_OK);

  fail_unless (g_list_length (buffers) == 1);
  outbuffer = (GstBuffer *) buffers->data;
  fail_if (outbuffer == NULL);
  fail_unless (GST_IS_BUFFER (outbuffer));

  ASSERT_BUFFER_REFCOUNT (outbuffer, "outbuffer", 1);
  outlength = WIDTH * HEIGHT * 4;       /* output is AYUV */
  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  fail_unless_equals_int (map.size, outlength);

  ayuv = map.data;

  /* check chroma keying GREEN */
  for (i = 0; i < HEIGHT * WIDTH; i += 4)
    fail_unless_equals_int (ayuv[i], 0x00);

  gst_buffer_unmap (outbuffer, &map);

  buffers = g_list_remove (buffers, outbuffer);
  gst_buffer_unref (outbuffer);

  fail_unless_equals_int (gst_element_set_state (alpha, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);

  /* cleanup */
  cleanup_alpha (alpha);
  ASSERT_CAPS_REFCOUNT (incaps, "incaps", 1);
  gst_caps_unref (incaps);

}

GST_END_TEST;



GST_START_TEST (test_alpha)
{
  GstElement *alpha;
  GstBuffer *inbuffer;
  GstBuffer *outbuffer;
  GstCaps *incaps;
  guint8 *ayuv;
  guint outlength;
  GstMapInfo map;
  int i;

  incaps = create_caps_rgba32 ();

  alpha = setup_alpha ();

  g_object_set (alpha, "alpha", 0.5, NULL);     /* Alpha value 0.5 */

  fail_unless_equals_int (gst_element_set_state (alpha, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  gst_check_setup_events (srcpad, alpha, incaps, GST_FORMAT_TIME);

  inbuffer = create_buffer_rgba32 (FILL_BLUE);
  GST_DEBUG ("Created buffer of %" G_GSIZE_FORMAT " bytes",
      gst_buffer_get_size (inbuffer));
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away reference */
  GST_DEBUG ("push it");
  fail_unless_equals_int (gst_pad_push (srcpad, inbuffer), GST_FLOW_OK);
  GST_DEBUG ("pushed it");

  /* ... and puts a new buffer on the global list */
  fail_unless (g_list_length (buffers) == 1);
  outbuffer = (GstBuffer *) buffers->data;
  fail_if (outbuffer == NULL);
  fail_unless (GST_IS_BUFFER (outbuffer));

  ASSERT_BUFFER_REFCOUNT (outbuffer, "outbuffer", 1);
  outlength = WIDTH * HEIGHT * 4;       /* output is AYUV */
  gst_buffer_map (outbuffer, &map, GST_MAP_READ);
  fail_unless_equals_int (map.size, outlength);

  ayuv = map.data;

  for (i = 0; i < HEIGHT * WIDTH; i += 4)
    fail_unless_equals_int (ayuv[i], 0x7F);

  gst_buffer_unmap (outbuffer, &map);

  buffers = g_list_remove (buffers, outbuffer);
  gst_buffer_unref (outbuffer);

  fail_unless_equals_int (gst_element_set_state (alpha, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);

  /* cleanup */
  GST_DEBUG ("cleanup alpha");
  cleanup_alpha (alpha);
  GST_DEBUG ("cleanup, unref incaps");
  ASSERT_CAPS_REFCOUNT (incaps, "incaps", 1);
  gst_caps_unref (incaps);

}

GST_END_TEST;


static Suite *
alpha_suite (void)
{
  Suite *s = suite_create ("alpha");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_alpha);
  tcase_add_test (tc_chain, test_chromakeying);

  return s;
}

GST_CHECK_MAIN (alpha);
