/* GStreamer
 * unit test for the videobox element
 *
 * Copyright (C) 2006 Ravi Kiran K N <ravi.kiran@samsung.com>
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

typedef struct _GstVideoBoxTestContext
{
  GstElement *pipeline;
  GstElement *src;
  GstElement *filter;
  GstElement *box;
  GstElement *filter2;
  GstElement *sink;
} GstVideoBoxTestContext;

typedef struct _FormatConversion
{
  const gchar *in_caps;
  const gchar *out_caps;
  gboolean expected_result;
} FormatConversion;


/*
 * Update this table as and when the conversion is supported(or unsupported) in videobox
 */
static const FormatConversion conversion_table[] = {
  {"video/x-raw,format={RGBA}", "video/x-raw,format={AYUV}", TRUE},
  {"video/x-raw,format={AYUV}", "video/x-raw,format={RGBA}", TRUE},
  {"video/x-raw,format={I420}", "video/x-raw,format={AYUV}", TRUE},
  {"video/x-raw,format={AYUV}", "video/x-raw,format={I420}", TRUE},
  {"video/x-raw,format={I420}", "video/x-raw,format={YV12}", TRUE},
  {"video/x-raw,format={YV12}", "video/x-raw,format={AYUV}", TRUE},
  {"video/x-raw,format={YV12}", "video/x-raw,format={I420}", TRUE},
  {"video/x-raw,format={AYUV}", "video/x-raw,format={YV12}", TRUE},
  {"video/x-raw,format={AYUV}", "video/x-raw,format={xRGB}", TRUE},
  {"video/x-raw,format={xRGB}", "video/x-raw,format={xRGB}", TRUE},
  {"video/x-raw,format={xRGB}", "video/x-raw,format={AYUV}", TRUE},
  {"video/x-raw,format={GRAY8}", "video/x-raw,format={GRAY16_LE}", FALSE},
  {"video/x-raw,format={GRAY8}", "video/x-raw,format={GRAY16_BE}", FALSE},
  {"video/x-raw,format={Y444}", "video/x-raw,format={Y42B}", FALSE},
  {"video/x-raw,format={Y444}", "video/x-raw,format={Y41B}", FALSE},
  {"video/x-raw,format={Y42B}", "video/x-raw,format={Y41B}", FALSE}
};


static gboolean
bus_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (message->type) {
    case GST_MESSAGE_EOS:{
      GST_LOG ("EOS event received");
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_ERROR:{
      GError *gerror;
      gchar *debug;
      gst_message_parse_error (message, &gerror, &debug);
      g_error ("Error from %s: %s (%s)\n",
          GST_ELEMENT_NAME (GST_MESSAGE_SRC (message)), gerror->message,
          GST_STR_NULL (debug));
      g_error_free (gerror);
      g_free (debug);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_WARNING:{
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static void
videobox_test_init_context (GstVideoBoxTestContext * ctx)
{
  fail_unless (ctx != NULL);

  ctx->pipeline = gst_pipeline_new ("pipeline");
  fail_unless (ctx->pipeline != NULL);
  ctx->src = gst_element_factory_make ("videotestsrc", "src");
  fail_unless (ctx->src != NULL, "Failed to create videotestsrc element");
  ctx->filter = gst_element_factory_make ("capsfilter", "filter");
  fail_unless (ctx->filter != NULL, "Failed to create capsfilter element");
  ctx->box = gst_element_factory_make ("videobox", "box");
  fail_unless (ctx->box != NULL, "Failed to create videobox element");
  ctx->filter2 = gst_element_factory_make ("capsfilter", "filter2");
  fail_unless (ctx->filter2 != NULL,
      "Failed to create second capsfilter element");
  ctx->sink = gst_element_factory_make ("fakesink", "sink");
  fail_unless (ctx->sink != NULL, "Failed to create fakesink element");

  gst_bin_add_many (GST_BIN (ctx->pipeline), ctx->src, ctx->filter,
      ctx->box, ctx->filter2, ctx->sink, NULL);
  fail_unless (gst_element_link_many (ctx->src, ctx->filter, ctx->box,
          ctx->filter2, ctx->sink, NULL) == TRUE, "Can not link elements");

  fail_unless (gst_element_set_state (ctx->pipeline,
          GST_STATE_READY) != GST_STATE_CHANGE_FAILURE,
      "couldn't set pipeline to READY state");

  GST_LOG ("videobox context inited");
}

static void
videobox_test_deinit_context (GstVideoBoxTestContext * ctx)
{
  GST_LOG ("deiniting videobox context");

  gst_element_set_state (ctx->pipeline, GST_STATE_NULL);
  gst_object_unref (ctx->pipeline);
  memset (ctx, 0x00, sizeof (GstVideoBoxTestContext));
}

GST_START_TEST (test_caps_transform)
{
  GstStateChangeReturn state_ret;
  GstVideoBoxTestContext ctx;
  guint conversions_test_size;
  guint itr;
  gboolean link_res;
  GMainLoop *loop;
  GstBus *bus;

  videobox_test_init_context (&ctx);
  gst_util_set_object_arg (G_OBJECT (ctx.src), "num-buffers", "1");

  loop = g_main_loop_new (NULL, TRUE);
  fail_unless (loop != NULL);

  bus = gst_element_get_bus (ctx.pipeline);
  fail_unless (bus != NULL);

  gst_bus_add_watch (bus, bus_handler, loop);

  conversions_test_size = G_N_ELEMENTS (conversion_table);
  for (itr = 0; itr < conversions_test_size; itr++) {
    gst_element_unlink_many (ctx.src, ctx.filter, ctx.box, ctx.filter2,
        ctx.sink, NULL);
    gst_util_set_object_arg (G_OBJECT (ctx.filter), "caps",
        conversion_table[itr].in_caps);
    gst_util_set_object_arg (G_OBJECT (ctx.filter2), "caps",
        conversion_table[itr].out_caps);

    /* Link with new input and output format from conversion table */
    link_res =
        gst_element_link_many (ctx.src, ctx.filter, ctx.box, ctx.filter2,
        ctx.sink, NULL);

    /* Check if the specified format conversion is supported or not by videobox */
    fail_unless (link_res == conversion_table[itr].expected_result,
        "videobox can not convert from '%s'' to '%s'",
        conversion_table[itr].in_caps, conversion_table[itr].out_caps);

    if (link_res == FALSE) {
      GST_LOG ("elements linking failed");
      continue;
    }

    state_ret = gst_element_set_state (ctx.pipeline, GST_STATE_PLAYING);
    fail_unless (state_ret != GST_STATE_CHANGE_FAILURE,
        "couldn't set pipeline to PLAYING state");

    g_main_loop_run (loop);

    state_ret = gst_element_set_state (ctx.pipeline, GST_STATE_READY);
    fail_unless (state_ret != GST_STATE_CHANGE_FAILURE,
        "couldn't set pipeline to READY state");
  }

  gst_bus_remove_watch (bus);
  gst_object_unref (bus);
  g_main_loop_unref (loop);

  videobox_test_deinit_context (&ctx);
}

GST_END_TEST;


static Suite *
videobox_suite (void)
{
  Suite *s = suite_create ("videobox");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_caps_transform);

  return s;
}

GST_CHECK_MAIN (videobox);
