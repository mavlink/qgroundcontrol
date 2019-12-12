/* GStreamer unit test for the aspectratiocrop element
 * Copyright (C) 2009 Thijs Vermeir <thijsvermeir@gmail.com>
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
# include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/check/gstcheck.h>

#define ASPECT_RATIO_CROP_CAPS                      \
  GST_VIDEO_CAPS_MAKE ("{ RGBx, xRGB, BGRx, xBGR, " \
      "RGBA, ARGB, BGRA, ABGR, RGB, BGR, AYUV, "    \
      "YUY2, YVYU, UYVY, GRAY8, I420, YV12, RGB16, " \
      "RGB15 }")

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (ASPECT_RATIO_CROP_CAPS)
    );

static void
check_aspectratiocrop (const gchar * in_string, const gchar * out_string,
    gint in_size, gint out_size, gint ar_n, gint ar_d)
{
  GstElement *element;
  GstPad *pad_peer;
  GstPad *sink_pad = NULL;
  GstPad *src_pad;
  GstBuffer *new;
  GstBuffer *buffer;
  GstBuffer *buffer_out;
  GstCaps *incaps;
  GstCaps *outcaps;

  incaps = gst_caps_from_string (in_string);
  buffer = gst_buffer_new_and_alloc (in_size);
  outcaps = gst_caps_from_string (out_string);
  buffer_out = gst_buffer_new_and_alloc (out_size);

  /* check that there are no buffers waiting */
  gst_check_drop_buffers ();

  /* create the element */
  element = gst_check_setup_element ("aspectratiocrop");

  /* set the requested aspect ratio */
  g_object_set (G_OBJECT (element), "aspect-ratio", ar_n, ar_d, NULL);

  /* create the src pad */
  src_pad = gst_pad_new (NULL, GST_PAD_SRC);
  gst_pad_set_active (src_pad, TRUE);
  GST_DEBUG ("setting caps %s %" GST_PTR_FORMAT, in_string, incaps);
  gst_check_setup_events (src_pad, element, incaps, GST_FORMAT_TIME);

  pad_peer = gst_element_get_static_pad (element, "sink");
  fail_if (pad_peer == NULL);
  fail_unless (gst_pad_link (src_pad, pad_peer) == GST_PAD_LINK_OK,
      "Could not link source and %s sink pads", GST_ELEMENT_NAME (element));
  gst_object_unref (pad_peer);

  /* create the sink pad */
  pad_peer = gst_element_get_static_pad (element, "src");
  sink_pad = gst_pad_new_from_static_template (&sinktemplate, "sink");
  fail_unless (gst_pad_link (pad_peer, sink_pad) == GST_PAD_LINK_OK,
      "Could not link sink and %s source pads", GST_ELEMENT_NAME (element));
  gst_object_unref (pad_peer);
  gst_pad_set_chain_function (sink_pad, gst_check_chain_func);
  gst_pad_set_active (sink_pad, TRUE);

  /* configure the sink pad */
  fail_unless (gst_element_set_state (element,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  fail_unless (gst_pad_push (src_pad, buffer) == GST_FLOW_OK,
      "Failed to push buffer");
  fail_unless (gst_element_set_state (element,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS, "could not set to null");

  /* check the result */
  fail_unless (g_list_length (buffers) == 1);
  new = GST_BUFFER (buffers->data);
  buffers = g_list_remove (buffers, new);
  fail_unless (gst_buffer_get_size (buffer_out) == gst_buffer_get_size (new),
      "size of the buffers are not the same");
  {
    GstCaps *sinkpad_caps;

    sinkpad_caps = gst_pad_get_current_caps (sink_pad);

    gst_check_caps_equal (sinkpad_caps, outcaps);

    gst_caps_unref (sinkpad_caps);
  }
  gst_buffer_unref (new);
  gst_buffer_unref (buffer_out);
  gst_caps_unref (outcaps);
  gst_caps_unref (incaps);

  /* teardown the element and pads */
  gst_pad_set_active (src_pad, FALSE);
  gst_check_teardown_src_pad (element);
  gst_pad_set_active (sink_pad, FALSE);
  gst_check_teardown_sink_pad (element);
  gst_check_teardown_element (element);
}

GST_START_TEST (test_no_cropping)
{
  check_aspectratiocrop
      ("video/x-raw, format=(string)YUY2, width=(int)320, height=(int)240, framerate=(fraction)30/1",
      "video/x-raw, format=(string)YUY2, width=(int)320, height=(int)240, framerate=(fraction)30/1",
      153600, 153600, 4, 3);
  check_aspectratiocrop
      ("video/x-raw, format=(string)YUY2, width=(int)320, height=(int)320, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)4/3",
      "video/x-raw, format=(string)YUY2, width=(int)320, height=(int)320, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)4/3",
      204800, 204800, 4, 3);
}

GST_END_TEST;

GST_START_TEST (test_autocropping)
{
  check_aspectratiocrop
      ("video/x-raw, format=(string)YUY2, width=(int)320, height=(int)240, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)4/3",
      "video/x-raw, format=(string)YUY2, width=(int)240, height=(int)240, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)4/3",
      153600, 115200, 4, 3);

  check_aspectratiocrop
      ("video/x-raw, format=(string)YUY2, width=(int)320, height=(int)240, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)16/9",
      "video/x-raw, format=(string)YUY2, width=(int)180, height=(int)240, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)16/9",
      153600, 86400, 4, 3);

  check_aspectratiocrop
      ("video/x-raw, format=(string)YUY2, width=(int)320, height=(int)240, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)16/15",
      "video/x-raw, format=(string)YUY2, width=(int)320, height=(int)192, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)16/15",
      153600, 122880, 16, 9);

}

GST_END_TEST;

static Suite *
aspectratiocrop_suite (void)
{
  Suite *s = suite_create ("aspectratiocrop");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_no_cropping);
  tcase_add_test (tc_chain, test_autocropping);

  return s;
}

GST_CHECK_MAIN (aspectratiocrop);
