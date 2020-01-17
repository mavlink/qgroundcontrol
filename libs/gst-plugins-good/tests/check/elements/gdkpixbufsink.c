/* GStreamer unit test for the gdkpixbufsink element
 * Copyright (C) 2008 Tim-Philipp Müller <tim centricular net>
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

#include <gst/check/gstcheck.h>
#include <gst/video/video.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#define CAPS_RGB "video/x-raw, format=RGB"
#define CAPS_RGBA "video/x-raw, format=RGBA"
#define WxH ",width=(int)319,height=(int)241"

#define N_BUFFERS 5

typedef struct
{
  GstElement *pipe;
  GstElement *src;
  GstElement *filter;
  GstElement *sink;
} GstGdkPixbufSinkTestContext;

static void
gdkpixbufsink_init_test_context (GstGdkPixbufSinkTestContext * ctx,
    const gchar * filter_caps_string, gint num_buffers)
{
  GstCaps *caps;

  fail_unless (ctx != NULL);

  ctx->pipe = gst_pipeline_new ("pipeline");
  fail_unless (ctx->pipe != NULL);
  ctx->src = gst_element_factory_make ("videotestsrc", "src");
  fail_unless (ctx->src != NULL, "Failed to create videotestsrc element");
  ctx->filter = gst_element_factory_make ("capsfilter", "filter");
  fail_unless (ctx->filter != NULL, "Failed to create capsfilter element");
  ctx->sink = gst_element_factory_make ("gdkpixbufsink", "sink");
  fail_unless (ctx->sink != NULL, "Failed to create gdkpixbufsink element");

  caps = gst_caps_from_string (filter_caps_string);
  fail_unless (caps != NULL);
  g_object_set (ctx->filter, "caps", caps, NULL);
  gst_caps_unref (caps);

  if (num_buffers > 0)
    g_object_set (ctx->src, "num-buffers", num_buffers, NULL);

  fail_unless (gst_bin_add (GST_BIN (ctx->pipe), ctx->src));
  fail_unless (gst_bin_add (GST_BIN (ctx->pipe), ctx->filter));
  fail_unless (gst_bin_add (GST_BIN (ctx->pipe), ctx->sink));
  fail_unless (gst_element_link (ctx->src, ctx->filter));
  fail_unless (gst_element_link (ctx->filter, ctx->sink));
}

static void
gdkpixbufsink_unset_test_context (GstGdkPixbufSinkTestContext * ctx)
{
  gst_element_set_state (ctx->pipe, GST_STATE_NULL);
  gst_object_unref (ctx->pipe);
  memset (ctx, 0, sizeof (GstGdkPixbufSinkTestContext));
}

static gboolean
check_last_pixbuf (GstGdkPixbufSinkTestContext * ctx, gpointer pixbuf)
{
  gpointer last_pb = NULL;
  gboolean ret;

  g_object_get (ctx->sink, "last-pixbuf", &last_pb, NULL);

  ret = (last_pb == pixbuf);

  if (last_pb)
    g_object_unref (last_pb);

  return ret;
}

/* doesn't return a ref to the pixbuf */
static GdkPixbuf *
check_message_pixbuf (GstMessage * msg, const gchar * name, gint channels,
    gboolean has_alpha)
{
  GdkPixbuf *pixbuf;
  const GstStructure *s;

  fail_unless (gst_message_get_structure (msg) != NULL);

  s = gst_message_get_structure (msg);
  fail_unless_equals_string (gst_structure_get_name (s), name);

  fail_unless (gst_structure_has_field (s, "pixbuf"));
  fail_unless (gst_structure_has_field_typed (s, "pixel-aspect-ratio",
          GST_TYPE_FRACTION));
  pixbuf =
      GDK_PIXBUF (g_value_get_object (gst_structure_get_value (s, "pixbuf")));
  fail_unless (GDK_IS_PIXBUF (pixbuf));
  fail_unless_equals_int (gdk_pixbuf_get_n_channels (pixbuf), channels);
  fail_unless_equals_int (gdk_pixbuf_get_has_alpha (pixbuf), has_alpha);
  fail_unless_equals_int (gdk_pixbuf_get_width (pixbuf), 319);
  fail_unless_equals_int (gdk_pixbuf_get_height (pixbuf), 241);

  return pixbuf;
}

GST_START_TEST (test_rgb)
{
  GstGdkPixbufSinkTestContext ctx;
  GstMessage *msg;
  GdkPixbuf *pixbuf;
  GstBus *bus;
  gint i;

  gdkpixbufsink_init_test_context (&ctx, CAPS_RGB WxH, N_BUFFERS);

  fail_unless (check_last_pixbuf (&ctx, NULL));

  /* start prerolling */
  fail_unless_equals_int (gst_element_set_state (ctx.pipe, GST_STATE_PAUSED),
      GST_STATE_CHANGE_ASYNC);

  /* wait until prerolled */
  fail_unless_equals_int (gst_element_get_state (ctx.pipe, NULL, NULL, -1),
      GST_STATE_CHANGE_SUCCESS);

  bus = gst_element_get_bus (ctx.pipe);

  /* find element message from our gdkpixbufsink, ignore element messages from
   * other elemements (which just seems prudent to do, we don't expect any) */
  while (1) {
    msg = gst_bus_pop_filtered (bus, GST_MESSAGE_ELEMENT);
    fail_if (msg == NULL, "Expected element message from gdkpixbufsink");

    if (msg->src == GST_OBJECT (ctx.sink))
      break;
  }

  pixbuf = check_message_pixbuf (msg, "preroll-pixbuf", 3, FALSE);
  fail_unless (check_last_pixbuf (&ctx, pixbuf));
  gst_message_unref (msg);
  pixbuf = NULL;

  /* and go! */
  fail_unless_equals_int (gst_element_set_state (ctx.pipe, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  /* This is racy, supposed to make sure locking and refcounting works at
   * least to some extent */
  for (i = 0; i < 10000; ++i) {
    gpointer obj = NULL;

    g_object_get (ctx.sink, "last-pixbuf", &obj, NULL);
    fail_unless (obj == NULL || GDK_IS_PIXBUF (obj));
    if (obj)
      g_object_unref (obj);
  }

  /* there should be as many pixbuf messages as buffers */
  for (i = 0; i < N_BUFFERS; ++i) {
    /* find element message from our gdkpixbufsink, ignore element messages from
     * other elemements (which just seems prudent to do, we don't expect any) */
    while (1) {
      msg = gst_bus_timed_pop_filtered (bus, -1, GST_MESSAGE_ELEMENT);
      fail_if (msg == NULL, "Expected element message from gdkpixbufsink");
      if (msg->src == GST_OBJECT (ctx.sink))
        break;
    }

    pixbuf = check_message_pixbuf (msg, "pixbuf", 3, FALSE);
    gst_message_unref (msg);
  }

  /* note: we don't hold a ref to pixbuf any longer here, but it should be ok */
  fail_unless (check_last_pixbuf (&ctx, pixbuf));
  pixbuf = NULL;

  gdkpixbufsink_unset_test_context (&ctx);
  gst_object_unref (bus);
}

GST_END_TEST;

GST_START_TEST (test_rgba)
{
  GstGdkPixbufSinkTestContext ctx;
  GstMessage *msg;
  GdkPixbuf *pixbuf;
  GstBus *bus;
  gint i;

  gdkpixbufsink_init_test_context (&ctx, CAPS_RGBA WxH, N_BUFFERS);

  fail_unless (check_last_pixbuf (&ctx, NULL));

  /* start prerolling */
  fail_unless_equals_int (gst_element_set_state (ctx.pipe, GST_STATE_PAUSED),
      GST_STATE_CHANGE_ASYNC);

  /* wait until prerolled */
  fail_unless_equals_int (gst_element_get_state (ctx.pipe, NULL, NULL, -1),
      GST_STATE_CHANGE_SUCCESS);

  bus = gst_element_get_bus (ctx.pipe);

  /* find element message from our gdkpixbufsink, ignore element messages from
   * other elemements (which just seems prudent to do, we don't expect any) */
  while (1) {
    msg = gst_bus_pop_filtered (bus, GST_MESSAGE_ELEMENT);
    fail_if (msg == NULL, "Expected element message from gdkpixbufsink");

    if (msg->src == GST_OBJECT (ctx.sink))
      break;
  }

  pixbuf = check_message_pixbuf (msg, "preroll-pixbuf", 4, TRUE);
  fail_unless (check_last_pixbuf (&ctx, pixbuf));
  gst_message_unref (msg);
  pixbuf = NULL;

  /* and go! */
  fail_unless_equals_int (gst_element_set_state (ctx.pipe, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  /* This is racy, supposed to make sure locking and refcounting works at
   * least to some extent */
  for (i = 0; i < 10000; ++i) {
    gpointer obj = NULL;

    g_object_get (ctx.sink, "last-pixbuf", &obj, NULL);
    fail_unless (obj == NULL || GDK_IS_PIXBUF (obj));
    if (obj)
      g_object_unref (obj);
  }

  /* there should be as many pixbuf messages as buffers */
  for (i = 0; i < N_BUFFERS; ++i) {
    /* find element message from our gdkpixbufsink, ignore element messages from
     * other elemements (which just seems prudent to do, we don't expect any) */
    while (1) {
      msg = gst_bus_timed_pop_filtered (bus, -1, GST_MESSAGE_ELEMENT);
      fail_if (msg == NULL, "Expected element message from gdkpixbufsink");
      if (msg->src == GST_OBJECT (ctx.sink))
        break;
    }

    pixbuf = check_message_pixbuf (msg, "pixbuf", 4, TRUE);
    gst_message_unref (msg);
  }

  /* note: we don't hold a ref to pixbuf any longer here, but it should be ok */
  fail_unless (check_last_pixbuf (&ctx, pixbuf));
  pixbuf = NULL;

  gdkpixbufsink_unset_test_context (&ctx);
  gst_object_unref (bus);
}

GST_END_TEST;

static Suite *
gdkpixbufsink_suite (void)
{
  Suite *s = suite_create ("gdkpixbufsink");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_rgb);
  tcase_add_test (tc_chain, test_rgba);

  return s;
}

GST_CHECK_MAIN (gdkpixbufsink);
