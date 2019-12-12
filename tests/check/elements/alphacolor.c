/* GStreamer unit test for the alphacolor element
 *
 * Copyright (C) 2007 Tim-Philipp Müller <tim centricular net>
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

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
GstPad *mysrcpad, *mysinkpad;

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("AYUV"))
    );
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ RGBA, RGB }"))
    );

static GstElement *
setup_alphacolor (void)
{
  GstElement *alphacolor;

  alphacolor = gst_check_setup_element ("alphacolor");
  mysrcpad = gst_check_setup_src_pad (alphacolor, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (alphacolor, &sinktemplate);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return alphacolor;
}

static void
cleanup_alphacolor (GstElement * alphacolor)
{
  GST_DEBUG ("cleaning up");

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (alphacolor);
  gst_check_teardown_sink_pad (alphacolor);
  gst_check_teardown_element (alphacolor);
}

#define WIDTH 3
#define HEIGHT 4

static GstCaps *
create_caps_rgb24 (void)
{
  GstCaps *caps;

  caps = gst_caps_new_simple ("video/x-raw",
      "width", G_TYPE_INT, 3,
      "height", G_TYPE_INT, 4,
      "framerate", GST_TYPE_FRACTION, 0, 1,
      "format", G_TYPE_STRING, "RGB", NULL);

  return caps;
}

static GstCaps *
create_caps_rgba32 (void)
{
  GstCaps *caps;

  caps = gst_caps_new_simple ("video/x-raw",
      "width", G_TYPE_INT, 3,
      "height", G_TYPE_INT, 4,
      "framerate", GST_TYPE_FRACTION, 0, 1,
      "format", G_TYPE_STRING, "RGBA", NULL);

  return caps;
}

static GstBuffer *
create_buffer_rgb24_3x4 (void)
{
  /* stride is rounded up to multiple of 4, so 3 bytes padding for each row */
  const guint8 rgb24_3x4_img[HEIGHT * GST_ROUND_UP_4 (WIDTH * 3)] = {
    0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
    0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00
  };
  guint rowstride = GST_ROUND_UP_4 (WIDTH * 3);
  GstBuffer *buf;
  GstMapInfo info;

  buf = gst_buffer_new_and_alloc (HEIGHT * rowstride);
  gst_buffer_map (buf, &info, GST_MAP_READWRITE);
  fail_unless_equals_int (info.size, sizeof (rgb24_3x4_img));
  memcpy (info.data, rgb24_3x4_img, sizeof (rgb24_3x4_img));

  gst_buffer_unmap (buf, &info);

  return buf;
}

static GstBuffer *
create_buffer_rgba32_3x4 (void)
{
  /* stride is rounded up to multiple of 4, so 3 bytes padding for each row */
  /* should be:  RED     BLUE    WHITE    where 'nothing' is fully transparent
   *             GREEN   RED     BLUE     and all other colours are fully
   *             NOTHING GREEN   RED      opaque.
   *             BLACK   NOTHING GREEN
   */
  const guint8 rgba32_3x4_img[HEIGHT * WIDTH * 4] = {
    0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff,
    0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff
  };
  guint rowstride = WIDTH * 4;
  GstBuffer *buf;
  GstMapInfo map;

  buf = gst_buffer_new_and_alloc (HEIGHT * rowstride);
  gst_buffer_map (buf, &map, GST_MAP_READWRITE);
  fail_unless_equals_int (map.size, sizeof (rgba32_3x4_img));
  memcpy (map.data, rgba32_3x4_img, sizeof (rgba32_3x4_img));

  gst_buffer_unmap (buf, &map);

  return buf;
}

GST_START_TEST (test_rgb24)
{
  GstElement *alphacolor;
  GstBuffer *inbuffer;
  GstCaps *incaps;

  incaps = create_caps_rgb24 ();
  alphacolor = setup_alphacolor ();

  fail_unless_equals_int (gst_element_set_state (alphacolor, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  gst_check_setup_events (mysrcpad, alphacolor, incaps, GST_FORMAT_TIME);

  inbuffer = create_buffer_rgb24_3x4 ();
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away reference; this should error out with a not-negotiated
   * error, alphacolor should only accept RGBA caps, not but plain RGB24 caps */
  GST_DEBUG ("push it");
  fail_unless_equals_int (gst_pad_push (mysrcpad, inbuffer),
      GST_FLOW_NOT_NEGOTIATED);
  GST_DEBUG ("pushed it");

  fail_unless (g_list_length (buffers) == 0);

  fail_unless_equals_int (gst_element_set_state (alphacolor, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);

  /* cleanup */
  GST_DEBUG ("cleanup alphacolor");
  cleanup_alphacolor (alphacolor);
  GST_DEBUG ("cleanup, unref incaps");
  ASSERT_CAPS_REFCOUNT (incaps, "incaps", 1);
  gst_caps_unref (incaps);
}

GST_END_TEST;

/* these macros assume WIDTH and HEIGHT is fixed to what's defined above */
#define fail_unless_ayuv_pixel_has_alpha(ayuv,x,y,a) \
    { \
        guint8 *pixel; \
        pixel = ((guint8*)(ayuv) + ((WIDTH * 4) * (y)) + ((x) * 4)); \
        fail_unless_equals_int (pixel[0], a); \
    }

GST_START_TEST (test_rgba32)
{
  GstElement *alphacolor;
  GstBuffer *inbuffer;
  GstBuffer *outbuffer;
  GstCaps *incaps;
  guint8 *ayuv;
  guint outlength;
  GstMapInfo map;

  incaps = create_caps_rgba32 ();
  alphacolor = setup_alphacolor ();

  fail_unless_equals_int (gst_element_set_state (alphacolor, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  gst_check_setup_events (mysrcpad, alphacolor, incaps, GST_FORMAT_TIME);

  inbuffer = create_buffer_rgba32_3x4 ();
  GST_DEBUG ("Created buffer of %" G_GSIZE_FORMAT " bytes",
      gst_buffer_get_size (inbuffer));
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away reference */
  GST_DEBUG ("push it");
  fail_unless_equals_int (gst_pad_push (mysrcpad, inbuffer), GST_FLOW_OK);
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

  /* check alpha values (0x00 = totally transparent, 0xff = totally opaque) */
  fail_unless_ayuv_pixel_has_alpha (ayuv, 0, 0, 0xff);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 1, 0, 0xff);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 2, 0, 0xff);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 0, 1, 0xff);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 1, 1, 0xff);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 2, 1, 0xff);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 0, 2, 0x00);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 1, 2, 0xff);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 2, 2, 0xff);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 0, 3, 0xff);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 1, 3, 0x00);
  fail_unless_ayuv_pixel_has_alpha (ayuv, 2, 3, 0xff);

  /* we don't check the YUV data, because apparently results differ slightly
   * depending on whether we run in valgrind or not */

  gst_buffer_unmap (outbuffer, &map);

  buffers = g_list_remove (buffers, outbuffer);
  gst_buffer_unref (outbuffer);

  fail_unless_equals_int (gst_element_set_state (alphacolor, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);

  /* cleanup */
  GST_DEBUG ("cleanup alphacolor");
  cleanup_alphacolor (alphacolor);
  GST_DEBUG ("cleanup, unref incaps");
  ASSERT_CAPS_REFCOUNT (incaps, "incaps", 1);
  gst_caps_unref (incaps);
}

GST_END_TEST;


static Suite *
alphacolor_suite (void)
{
  Suite *s = suite_create ("alphacolor");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_rgb24);
  tcase_add_test (tc_chain, test_rgba32);

  return s;
}

GST_CHECK_MAIN (alphacolor);
