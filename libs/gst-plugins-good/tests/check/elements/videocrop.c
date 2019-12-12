/* GStreamer unit test for the videocrop element
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
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

#ifdef HAVE_VALGRIND
# include <valgrind/valgrind.h>
#endif

#include <gst/check/gstcheck.h>
#include <gst/video/video.h>
#include <gst/base/gstbasetransform.h>

/* return a list of caps where we only need to set
 * width and height to get fixed caps */
static GList *
video_crop_get_test_caps (GstElement * videocrop)
{
  GstCaps *templ, *allowed_caps;
  GstPad *srcpad;
  GList *list = NULL;
  guint i;

  srcpad = gst_element_get_static_pad (videocrop, "src");
  fail_unless (srcpad != NULL);
  templ = gst_pad_get_pad_template_caps (srcpad);
  fail_unless (templ != NULL);

  allowed_caps = gst_caps_normalize (templ);

  for (i = 0; i < gst_caps_get_size (allowed_caps); ++i) {
    GstStructure *new_structure;
    GstCaps *single_caps;

    single_caps = gst_caps_new_empty ();
    new_structure =
        gst_structure_copy (gst_caps_get_structure (allowed_caps, i));
    gst_structure_set (new_structure, "framerate", GST_TYPE_FRACTION,
        1, 1, NULL);
    gst_structure_remove_field (new_structure, "width");
    gst_structure_remove_field (new_structure, "height");
    gst_caps_append_structure (single_caps, new_structure);

    GST_DEBUG ("have caps %" GST_PTR_FORMAT, single_caps);
    /* should be fixed without width/height */
    fail_unless (gst_caps_is_fixed (single_caps));

    list = g_list_prepend (list, single_caps);
  }

  gst_caps_unref (allowed_caps);
  gst_object_unref (srcpad);

  return list;
}

GST_START_TEST (test_unit_sizes)
{
  GstBaseTransformClass *csp_klass, *vcrop_klass;
  GstElement *videocrop, *csp;
  GList *caps_list, *l;

  videocrop = gst_element_factory_make ("videocrop", "videocrop");
  fail_unless (videocrop != NULL, "Failed to create videocrop element");
  vcrop_klass = GST_BASE_TRANSFORM_GET_CLASS (videocrop);

  csp = gst_element_factory_make ("videoconvert", "csp");
  fail_unless (csp != NULL, "Failed to create videoconvert element");
  csp_klass = GST_BASE_TRANSFORM_GET_CLASS (csp);

  caps_list = video_crop_get_test_caps (videocrop);

  for (l = caps_list; l != NULL; l = l->next) {
    const struct
    {
      gint width, height;
    } sizes_to_try[] = {
      {
      160, 120}, {
      161, 120}, {
      160, 121}, {
      161, 121}, {
      159, 120}, {
      160, 119}, {
      159, 119}, {
      159, 121}
    };
    GstStructure *s;
    GstCaps *caps;
    gint i;

    caps = gst_caps_copy (GST_CAPS (l->data));
    s = gst_caps_get_structure (caps, 0);
    fail_unless (s != NULL);

    for (i = 0; i < G_N_ELEMENTS (sizes_to_try); ++i) {
      gchar *caps_str;
      gsize csp_size = 0;
      gsize vc_size = 0;

      gst_structure_set (s, "width", G_TYPE_INT, sizes_to_try[i].width,
          "height", G_TYPE_INT, sizes_to_try[i].height, NULL);

      caps_str = gst_caps_to_string (caps);
      GST_INFO ("Testing unit size for %s", caps_str);

      /* skip if videoconvert doesn't support these caps
       * (only works with gst-plugins-base 0.10.9.1 or later) */
      if (!csp_klass->get_unit_size ((GstBaseTransform *) csp, caps, &csp_size)) {
        GST_INFO ("videoconvert does not support format %s", caps_str);
        g_free (caps_str);
        continue;
      }

      fail_unless (vcrop_klass->get_unit_size ((GstBaseTransform *) videocrop,
              caps, &vc_size));

      fail_unless (vc_size == csp_size,
          "videocrop and videoconvert return different unit sizes for "
          "caps %s: vc_size=%d, csp_size=%d", caps_str, vc_size, csp_size);

      g_free (caps_str);
    }

    gst_caps_unref (caps);
  }

  g_list_foreach (caps_list, (GFunc) gst_caps_unref, NULL);
  g_list_free (caps_list);

  gst_object_unref (csp);
  gst_object_unref (videocrop);
}

GST_END_TEST;

typedef struct
{
  GstElement *pipeline;
  GstElement *src;
  GstElement *filter;
  GstElement *crop;
  GstElement *filter2;
  GstElement *sink;
  GstBuffer *last_buf;
  GstCaps *last_caps;
} GstVideoCropTestContext;

static void
handoff_cb (GstElement * sink, GstBuffer * buf, GstPad * pad,
    GstVideoCropTestContext * ctx)
{
  GstCaps *caps;

  gst_buffer_replace (&ctx->last_buf, buf);
  caps = gst_pad_get_current_caps (pad);
  gst_caps_replace (&ctx->last_caps, caps);
  gst_caps_unref (caps);
}

static void
videocrop_test_cropping_init_context (GstVideoCropTestContext * ctx)
{
  fail_unless (ctx != NULL);

  ctx->pipeline = gst_pipeline_new ("pipeline");
  fail_unless (ctx->pipeline != NULL);
  ctx->src = gst_element_factory_make ("videotestsrc", "src");
  fail_unless (ctx->src != NULL, "Failed to create videotestsrc element");
  ctx->filter = gst_element_factory_make ("capsfilter", "filter");
  fail_unless (ctx->filter != NULL, "Failed to create capsfilter element");
  ctx->crop = gst_element_factory_make ("videocrop", "crop");
  fail_unless (ctx->crop != NULL, "Failed to create videocrop element");
  ctx->filter2 = gst_element_factory_make ("capsfilter", "filter2");
  fail_unless (ctx->filter2 != NULL,
      "Failed to create second capsfilter element");
  ctx->sink = gst_element_factory_make ("fakesink", "sink");
  fail_unless (ctx->sink != NULL, "Failed to create fakesink element");

  gst_bin_add_many (GST_BIN (ctx->pipeline), ctx->src, ctx->filter,
      ctx->crop, ctx->filter2, ctx->sink, NULL);
  gst_element_link_many (ctx->src, ctx->filter, ctx->crop, ctx->filter2,
      ctx->sink, NULL);

  /* set pattern to 'red' - for our purposes it doesn't matter anyway */
  g_object_set (ctx->src, "pattern", 4, NULL);

  g_object_set (ctx->sink, "signal-handoffs", TRUE, NULL);
  g_signal_connect (ctx->sink, "preroll-handoff", G_CALLBACK (handoff_cb), ctx);

  ctx->last_buf = NULL;
  ctx->last_caps = NULL;

  GST_LOG ("context inited");
}

static void
videocrop_test_cropping_deinit_context (GstVideoCropTestContext * ctx)
{
  GST_LOG ("deiniting context");

  gst_element_set_state (ctx->pipeline, GST_STATE_NULL);
  gst_object_unref (ctx->pipeline);
  gst_buffer_replace (&ctx->last_buf, NULL);
  gst_caps_replace (&ctx->last_caps, NULL);
  memset (ctx, 0x00, sizeof (GstVideoCropTestContext));
}

typedef void (*GstVideoCropTestBufferFunc) (GstBuffer * buffer, GstCaps * caps);

static void
videocrop_test_cropping (GstVideoCropTestContext * ctx, GstCaps * in_caps,
    GstCaps * out_caps, gint left, gint right, gint top, gint bottom,
    GstVideoCropTestBufferFunc func)
{
  GST_LOG ("lrtb = %03u %03u %03u %03u, in_caps = %" GST_PTR_FORMAT
      ", out_caps = %" GST_PTR_FORMAT, left, right, top, bottom, in_caps,
      out_caps);

  g_object_set (ctx->filter, "caps", in_caps, NULL);
  g_object_set (ctx->filter2, "caps", out_caps, NULL);

  g_object_set (ctx->crop, "left", left, "right", right, "top", top,
      "bottom", bottom, NULL);

  /* this will fail if videotestsrc doesn't support our format; we need
   * videotestsrc from -base CVS 0.10.9.1 with RGBA and AYUV support */
  fail_unless (gst_element_set_state (ctx->pipeline,
          GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);
  fail_unless (gst_element_get_state (ctx->pipeline, NULL, NULL,
          -1) == GST_STATE_CHANGE_SUCCESS);

  if (func != NULL) {
    func (ctx->last_buf, ctx->last_caps);
  }

  gst_element_set_state (ctx->pipeline, GST_STATE_NULL);
}

static void
check_1x1_buffer (GstBuffer * buf, GstCaps * caps)
{
  GstVideoInfo info;
  GstVideoFrame frame;
  /* the exact values we check for come from videotestsrc */
  static const guint yuv_values[] = { 81, 90, 240, 255 };
  static const guint rgb_values[] = { 0xff, 0, 0, 255 };
  static const guint gray8_values[] = { 0x51 };
  static const guint gray16_values[] = { 0x5151 };
  const guint *values;
  guint i;
  const GstVideoFormatInfo *finfo;

  fail_unless (buf != NULL);
  fail_unless (caps != NULL);

  fail_unless (gst_video_info_from_caps (&info, caps));
  fail_unless (gst_video_frame_map (&frame, &info, buf, GST_MAP_READ));

  finfo = info.finfo;

  if (GST_VIDEO_INFO_IS_YUV (&info))
    values = yuv_values;
  else if (GST_VIDEO_INFO_IS_GRAY (&info))
    if (GST_VIDEO_FORMAT_INFO_BITS (finfo) == 8)
      values = gray8_values;
    else
      values = gray16_values;
  else
    values = rgb_values;

  GST_MEMDUMP ("buffer", GST_VIDEO_FRAME_PLANE_DATA (&frame, 0), 8);

  for (i = 0; i < GST_VIDEO_FRAME_N_COMPONENTS (&frame); i++) {
    guint8 *data = GST_VIDEO_FRAME_COMP_DATA (&frame, i);

    GST_DEBUG ("W: %d", GST_VIDEO_FORMAT_INFO_W_SUB (finfo, i));
    GST_DEBUG ("H: %d", GST_VIDEO_FORMAT_INFO_H_SUB (finfo, i));

    if (GST_VIDEO_FORMAT_INFO_W_SUB (finfo,
            i) >= GST_VIDEO_FRAME_WIDTH (&frame))
      continue;
    if (GST_VIDEO_FORMAT_INFO_H_SUB (finfo,
            i) >= GST_VIDEO_FRAME_HEIGHT (&frame))
      continue;

    if (GST_VIDEO_FORMAT_INFO_BITS (finfo) == 8) {
      fail_unless_equals_int (data[0], values[i]);
    } else if (GST_VIDEO_FORMAT_INFO_BITS (finfo) == 16) {
      guint16 pixels, val;
      gint depth;

      if (GST_VIDEO_FORMAT_INFO_IS_LE (finfo))
        pixels = GST_READ_UINT16_LE (data);
      else
        pixels = GST_READ_UINT16_BE (data);

      depth = GST_VIDEO_FORMAT_INFO_DEPTH (finfo, i);
      val = pixels >> GST_VIDEO_FORMAT_INFO_SHIFT (finfo, i);
      val = val & ((1 << depth) - 1);

      GST_DEBUG ("val %08x %d : %d", pixels, i, val);
      if (depth <= 8) {
        fail_unless_equals_int (val, values[i] >> (8 - depth));
      } else {
        fail_unless_equals_int (val, values[i] >> (16 - depth));
      }
    } else {
    }
  }

  gst_video_frame_unmap (&frame);

  /*
     fail_unless_equals_int ((pixel & rmask) >> rshift, 0xff);
     fail_unless_equals_int ((pixel & gmask) >> gshift, 0x00);
     fail_unless_equals_int ((pixel & bmask) >> bshift, 0x00);
   */
}

GST_START_TEST (test_crop_to_1x1)
{
  GstVideoCropTestContext ctx;
  GList *caps_list, *node;

  videocrop_test_cropping_init_context (&ctx);

  caps_list = video_crop_get_test_caps (ctx.crop);

  for (node = caps_list; node != NULL; node = node->next) {
    GstStructure *s;
    GstCaps *caps;

    caps = gst_caps_copy (GST_CAPS (node->data));
    s = gst_caps_get_structure (caps, 0);
    fail_unless (s != NULL);

    GST_INFO ("testing format: %" GST_PTR_FORMAT, caps);

    gst_structure_set (s, "width", G_TYPE_INT, 160,
        "height", G_TYPE_INT, 160, NULL);

    videocrop_test_cropping (&ctx, caps, NULL, 159, 0, 159, 0,
        check_1x1_buffer);
    /* commented out because they don't really add anything useful check-wise:
       videocrop_test_cropping (&ctx, caps, NULL, 0, 159, 0, 159, check_1x1_buffer);
       videocrop_test_cropping (&ctx, caps, NULL, 159, 0, 0, 159, check_1x1_buffer);
       videocrop_test_cropping (&ctx, caps, NULL, 0, 159, 159, 0, check_1x1_buffer);
     */
    gst_caps_unref (caps);
  }
  g_list_foreach (caps_list, (GFunc) gst_caps_unref, NULL);
  g_list_free (caps_list);

  videocrop_test_cropping_deinit_context (&ctx);
}

GST_END_TEST;

GST_START_TEST (test_cropping)
{
  GstVideoCropTestContext ctx;
  struct
  {
    gint width, height;
  } sizes_to_try[] = {
    {
    160, 160}, {
    161, 160}, {
    160, 161}, {
    161, 161}, {
    159, 160}, {
    160, 159}, {
    159, 159}, {
    159, 161}
  };
  GList *caps_list, *node;
  gint i;

  videocrop_test_cropping_init_context (&ctx);

  caps_list = video_crop_get_test_caps (ctx.crop);
  node = g_list_nth (caps_list, __i__);

  if (node != NULL) {
    GstStructure *s;
    GstCaps *caps;

    caps = gst_caps_copy (GST_CAPS (node->data));
    s = gst_caps_get_structure (caps, 0);
    fail_unless (s != NULL);

    GST_INFO ("testing format: %" GST_PTR_FORMAT, caps);

    for (i = 0; i < G_N_ELEMENTS (sizes_to_try); ++i) {
      GstCaps *in_caps, *out_caps;

      GST_INFO (" - %d x %d", sizes_to_try[i].width, sizes_to_try[i].height);

      gst_structure_set (s, "width", G_TYPE_INT, sizes_to_try[i].width,
          "height", G_TYPE_INT, sizes_to_try[i].height, NULL);
      in_caps = gst_caps_copy (caps);

      gst_structure_set (s, "width", G_TYPE_INT, 1, "height", G_TYPE_INT, 1,
          NULL);
      out_caps = gst_caps_copy (caps);

      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 0, 0, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 1, 0, 0, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 1, 0, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 0, 1, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 0, 0, 1, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 63, 0, 0, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 63, 0, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 0, 63, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 0, 0, 63, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 63, 0, 0, 1, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 63, 1, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 1, 63, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 1, 0, 0, 63, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 0, 0, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 32, 0, 0, 128, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 32, 128, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 0, 128, 32, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 128, 0, 0, 32, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 1, 1, 1, 1, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 63, 63, 63, 63, NULL);
      videocrop_test_cropping (&ctx, in_caps, NULL, 64, 64, 64, 64, NULL);

      /* Dynamic cropping */
      videocrop_test_cropping (&ctx, in_caps, out_caps, -1, -1, -1, -1, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps, 0, -1, -1, -1, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps, -1, 0, -1, -1, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps, -1, -1, 0, -1, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps, -1, -1, -1, 0, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps, 10, -1, 10, -1, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps, -1, 10, -1, 10, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps,
          sizes_to_try[i].width - 1, -1, -1, -1, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps, -1,
          sizes_to_try[i].width - 1, -1, -1, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps, -1, -1,
          sizes_to_try[i].height - 1, -1, NULL);
      videocrop_test_cropping (&ctx, in_caps, out_caps, -1, -1, -1,
          sizes_to_try[i].height - 1, NULL);

      gst_caps_unref (in_caps);
      gst_caps_unref (out_caps);
    }

    gst_caps_unref (caps);
  } else {
    GST_INFO ("no caps #%d", __i__);
  }
  g_list_foreach (caps_list, (GFunc) gst_caps_unref, NULL);
  g_list_free (caps_list);

  videocrop_test_cropping_deinit_context (&ctx);
}

GST_END_TEST;


static GstPadProbeReturn
buffer_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer data)
{
  GstBuffer **p_buf = data;
  GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER (info);

  gst_buffer_replace (p_buf, buf);

  return GST_PAD_PROBE_OK;      /* keep data */
}

GST_START_TEST (test_passthrough)
{
  GstStateChangeReturn state_ret;
  GstVideoCropTestContext ctx;
  GstPad *srcpad;
  GstBuffer *gen_buf = NULL;    /* buffer generated by videotestsrc */

  videocrop_test_cropping_init_context (&ctx);

  g_object_set (ctx.src, "num-buffers", 1, NULL);

  srcpad = gst_element_get_static_pad (ctx.src, "src");
  fail_unless (srcpad != NULL);
  gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_BUFFER, buffer_probe_cb,
      &gen_buf, NULL);
  gst_object_unref (srcpad);

  g_object_set (ctx.crop, "left", 0, "right", 0, "top", 0, "bottom", 0, NULL);

  state_ret = gst_element_set_state (ctx.pipeline, GST_STATE_PAUSED);
  fail_unless (state_ret != GST_STATE_CHANGE_FAILURE,
      "couldn't set pipeline to PAUSED state");

  state_ret = gst_element_get_state (ctx.pipeline, NULL, NULL, -1);
  fail_unless (state_ret == GST_STATE_CHANGE_SUCCESS,
      "pipeline failed to go to PAUSED state");

  fail_unless (gen_buf != NULL);
  fail_unless (ctx.last_buf != NULL);

  /* pass through should do nothing */
  fail_unless (gen_buf == ctx.last_buf);

  videocrop_test_cropping_deinit_context (&ctx);

  fail_unless_equals_int (GST_MINI_OBJECT_REFCOUNT_VALUE (gen_buf), 1);
  gst_buffer_unref (gen_buf);
}

GST_END_TEST;

static gint
notgst_value_list_get_nth_int (const GValue * list_val, guint n)
{
  const GValue *v;

  fail_unless (GST_VALUE_HOLDS_LIST (list_val));
  fail_unless (n < gst_value_list_get_size (list_val));

  v = gst_value_list_get_value (list_val, n);
  fail_unless (G_VALUE_HOLDS_INT (v));
  return g_value_get_int (v);
}

GST_START_TEST (test_caps_transform)
{
  GstVideoCropTestContext ctx;
  GstBaseTransformClass *klass;
  GstBaseTransform *crop;
  const GValue *w_val;
  const GValue *h_val;
  GstCaps *caps, *adj_caps;

  videocrop_test_cropping_init_context (&ctx);

  crop = GST_BASE_TRANSFORM (ctx.crop);
  klass = GST_BASE_TRANSFORM_GET_CLASS (ctx.crop);
  fail_unless (klass != NULL);

  caps = gst_caps_new_simple ("video/x-raw",
      "format", G_TYPE_STRING, "I420",
      "framerate", GST_TYPE_FRACTION, 1, 1,
      "width", G_TYPE_INT, 200, "height", G_TYPE_INT, 100, NULL);

  /* by default, it should be no cropping and hence passthrough */
  adj_caps = klass->transform_caps (crop, GST_PAD_SRC, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_is_equal (adj_caps, caps));
  gst_caps_unref (adj_caps);

  adj_caps = klass->transform_caps (crop, GST_PAD_SINK, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_is_equal (adj_caps, caps));
  gst_caps_unref (adj_caps);

  /* make sure that's still true after changing properties back and forth */
  g_object_set (ctx.crop, "left", 1, "right", 3, "top", 5, "bottom", 7, NULL);
  g_object_set (ctx.crop, "left", 0, "right", 0, "top", 0, "bottom", 0, NULL);

  adj_caps = klass->transform_caps (crop, GST_PAD_SRC, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_is_equal (adj_caps, caps));
  gst_caps_unref (adj_caps);

  adj_caps = klass->transform_caps (crop, GST_PAD_SINK, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_is_equal (adj_caps, caps));
  gst_caps_unref (adj_caps);

  /* now check adjustments made ... */
  g_object_set (ctx.crop, "left", 1, "right", 3, "top", 5, "bottom", 7, NULL);

  /* ========= (1) fixed value ============================================= */

  /* sink => source, source must be bigger if we crop stuff off */
  adj_caps = klass->transform_caps (crop, GST_PAD_SRC, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_get_size (adj_caps) == 1);
  w_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "width");
  fail_unless (w_val != NULL);
  fail_unless (G_VALUE_HOLDS_INT (w_val));
  fail_unless_equals_int (g_value_get_int (w_val), 200 + (1 + 3));
  h_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "height");
  fail_unless (h_val != NULL);
  fail_unless (G_VALUE_HOLDS_INT (h_val));
  fail_unless_equals_int (g_value_get_int (h_val), 100 + (5 + 7));
  gst_caps_unref (adj_caps);

  /* source => sink becomes smaller */
  adj_caps = klass->transform_caps (crop, GST_PAD_SINK, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_get_size (adj_caps) == 1);
  w_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "width");
  fail_unless (w_val != NULL);
  fail_unless (G_VALUE_HOLDS_INT (w_val));
  fail_unless_equals_int (g_value_get_int (w_val), 200 - (1 + 3));
  h_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "height");
  fail_unless (h_val != NULL);
  fail_unless (G_VALUE_HOLDS_INT (h_val));
  fail_unless_equals_int (g_value_get_int (h_val), 100 - (5 + 7));
  gst_caps_unref (adj_caps);

  /* ========= (2) range (simple adjustment) =============================== */

  gst_structure_set (gst_caps_get_structure (caps, 0),
      "width", GST_TYPE_INT_RANGE, 1000, 2000,
      "height", GST_TYPE_INT_RANGE, 3000, 4000, NULL);

  /* sink => source, source must be bigger if we crop stuff off */
  adj_caps = klass->transform_caps (crop, GST_PAD_SRC, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_get_size (adj_caps) == 1);
  w_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "width");
  fail_unless (w_val != NULL);
  fail_unless (GST_VALUE_HOLDS_INT_RANGE (w_val));
  fail_unless_equals_int (gst_value_get_int_range_min (w_val), 1000 + (1 + 3));
  fail_unless_equals_int (gst_value_get_int_range_max (w_val), 2000 + (1 + 3));
  h_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "height");
  fail_unless (h_val != NULL);
  fail_unless (GST_VALUE_HOLDS_INT_RANGE (h_val));
  fail_unless_equals_int (gst_value_get_int_range_min (h_val), 3000 + (5 + 7));
  fail_unless_equals_int (gst_value_get_int_range_max (h_val), 4000 + (5 + 7));
  gst_caps_unref (adj_caps);

  /* source => sink becomes smaller */
  adj_caps = klass->transform_caps (crop, GST_PAD_SINK, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_get_size (adj_caps) == 1);
  w_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "width");
  fail_unless (w_val != NULL);
  fail_unless (GST_VALUE_HOLDS_INT_RANGE (w_val));
  fail_unless_equals_int (gst_value_get_int_range_min (w_val), 1000 - (1 + 3));
  fail_unless_equals_int (gst_value_get_int_range_max (w_val), 2000 - (1 + 3));
  h_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "height");
  fail_unless (h_val != NULL);
  fail_unless (GST_VALUE_HOLDS_INT_RANGE (h_val));
  fail_unless_equals_int (gst_value_get_int_range_min (h_val), 3000 - (5 + 7));
  fail_unless_equals_int (gst_value_get_int_range_max (h_val), 4000 - (5 + 7));
  gst_caps_unref (adj_caps);

  /* ========= (3) range (adjustment at boundary) ========================== */

  gst_structure_set (gst_caps_get_structure (caps, 0),
      "width", GST_TYPE_INT_RANGE, 2, G_MAXINT,
      "height", GST_TYPE_INT_RANGE, 2, G_MAXINT, NULL);

  /* sink => source, source must be bigger if we crop stuff off */
  adj_caps = klass->transform_caps (crop, GST_PAD_SRC, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_get_size (adj_caps) == 1);
  w_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "width");
  fail_unless (w_val != NULL);
  fail_unless (GST_VALUE_HOLDS_INT_RANGE (w_val));
  fail_unless_equals_int (gst_value_get_int_range_min (w_val), 2 + (1 + 3));
  fail_unless_equals_int (gst_value_get_int_range_max (w_val), G_MAXINT);
  h_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "height");
  fail_unless (h_val != NULL);
  fail_unless (GST_VALUE_HOLDS_INT_RANGE (h_val));
  fail_unless_equals_int (gst_value_get_int_range_min (h_val), 2 + (5 + 7));
  fail_unless_equals_int (gst_value_get_int_range_max (h_val), G_MAXINT);
  gst_caps_unref (adj_caps);

  /* source => sink becomes smaller */
  adj_caps = klass->transform_caps (crop, GST_PAD_SINK, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_get_size (adj_caps) == 1);
  w_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "width");
  fail_unless (w_val != NULL);
  fail_unless (GST_VALUE_HOLDS_INT_RANGE (w_val));
  fail_unless_equals_int (gst_value_get_int_range_min (w_val), 1);
  fail_unless_equals_int (gst_value_get_int_range_max (w_val),
      G_MAXINT - (1 + 3));
  h_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "height");
  fail_unless (h_val != NULL);
  fail_unless (GST_VALUE_HOLDS_INT_RANGE (h_val));
  fail_unless_equals_int (gst_value_get_int_range_min (h_val), 1);
  fail_unless_equals_int (gst_value_get_int_range_max (h_val),
      G_MAXINT - (5 + 7));
  gst_caps_unref (adj_caps);

  /* ========= (4) list of values ========================================== */

  {
    GValue list = { 0, };
    GValue ival = { 0, };

    g_value_init (&ival, G_TYPE_INT);
    g_value_init (&list, GST_TYPE_LIST);
    g_value_set_int (&ival, 2);
    gst_value_list_append_value (&list, &ival);
    g_value_set_int (&ival, G_MAXINT);
    gst_value_list_append_value (&list, &ival);
    gst_structure_set_value (gst_caps_get_structure (caps, 0), "width", &list);
    g_value_unset (&list);
    g_value_unset (&ival);

    g_value_init (&ival, G_TYPE_INT);
    g_value_init (&list, GST_TYPE_LIST);
    g_value_set_int (&ival, 5);
    gst_value_list_append_value (&list, &ival);
    g_value_set_int (&ival, 1000);
    gst_value_list_append_value (&list, &ival);
    gst_structure_set_value (gst_caps_get_structure (caps, 0), "height", &list);
    g_value_unset (&list);
    g_value_unset (&ival);
  }

  /* sink => source, source must be bigger if we crop stuff off */
  adj_caps = klass->transform_caps (crop, GST_PAD_SRC, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_get_size (adj_caps) == 1);
  w_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "width");
  fail_unless (w_val != NULL);
  fail_unless (GST_VALUE_HOLDS_LIST (w_val));
  fail_unless_equals_int (notgst_value_list_get_nth_int (w_val, 0),
      2 + (1 + 3));
  fail_unless_equals_int (notgst_value_list_get_nth_int (w_val, 1), G_MAXINT);
  h_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "height");
  fail_unless (h_val != NULL);
  fail_unless (GST_VALUE_HOLDS_LIST (h_val));
  fail_unless_equals_int (notgst_value_list_get_nth_int (h_val, 0),
      5 + (5 + 7));
  fail_unless_equals_int (notgst_value_list_get_nth_int (h_val, 1),
      1000 + (5 + 7));
  gst_caps_unref (adj_caps);

  /* source => sink becomes smaller */
  adj_caps = klass->transform_caps (crop, GST_PAD_SINK, caps, NULL);
  fail_unless (adj_caps != NULL);
  fail_unless (gst_caps_get_size (adj_caps) == 1);
  w_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "width");
  fail_unless (w_val != NULL);
  fail_unless (GST_VALUE_HOLDS_LIST (w_val));
  fail_unless_equals_int (notgst_value_list_get_nth_int (w_val, 0), 1);
  fail_unless_equals_int (notgst_value_list_get_nth_int (w_val, 1),
      G_MAXINT - (1 + 3));
  h_val =
      gst_structure_get_value (gst_caps_get_structure (adj_caps, 0), "height");
  fail_unless (h_val != NULL);
  fail_unless (GST_VALUE_HOLDS_LIST (h_val));
  fail_unless_equals_int (notgst_value_list_get_nth_int (h_val, 0), 1);
  fail_unless_equals_int (notgst_value_list_get_nth_int (h_val, 1),
      1000 - (5 + 7));
  gst_caps_unref (adj_caps);

  gst_caps_unref (caps);
  videocrop_test_cropping_deinit_context (&ctx);
}

GST_END_TEST;

static Suite *
videocrop_suite (void)
{
  Suite *s = suite_create ("videocrop");
  TCase *tc_chain = tcase_create ("general");

#ifdef HAVE_VALGRIND
  if (RUNNING_ON_VALGRIND) {
    /* our tests take quite a long time, so increase
     * timeout (~25 minutes on my 1.6GHz AMD K7) */
    tcase_set_timeout (tc_chain, 30 * 60);
  } else
#endif
  {
    /* increase timeout, these tests take a long time (60 secs here) */
    tcase_set_timeout (tc_chain, 2 * 60);
  }

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_crop_to_1x1);
  tcase_add_test (tc_chain, test_caps_transform);
  tcase_add_test (tc_chain, test_passthrough);
  tcase_add_test (tc_chain, test_unit_sizes);
  tcase_add_loop_test (tc_chain, test_cropping, 0, 25);

  return s;
}

GST_CHECK_MAIN (videocrop);
