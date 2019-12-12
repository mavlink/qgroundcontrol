/* GStreamer gdkpixbufoverlay test app
 * Copyright (C) 2014 Tim-Philipp Müller <tim centricular com>

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

#include <gio/gio.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <math.h>

#define VIDEO_WIDTH 720
#define VIDEO_HEIGHT 480
#define VIDEO_FPS 50

/* GdkPixbuf RGBA C-Source image dump from gdk-pixbuf-csource --raw,
 * gzipped and then base64 encoded */
const gchar gzipped_pixdata_base64[] =
    "H4sICPX/Z1QAA2xvZ28ucGl4AO2dsZHrNhCG+ewK2II64ClyrhmnTtSBh4kLUOLQAUuwEhSgFtiA"
    "A7agwA2wBT5AXJ5w4P5LgKLEO97ezDf2SCAIAftjFwuQ7/c///ojy/77+8eP7Jcs+/WfLMv+t/zW"
    "dV32pSmK3HK6sXZbFOXV9PZfWipLbWksHdHQZxVpZP+Ee7u62/d7rt0fivIqimJnOX+w/zhauq68"
    "aWjevfdUR1h3vXq/KMqzufueFN1J1FE+stf8KfCzobZ3q/ePojyT3v8gDSyB09GFND7g/N014rpl"
    "41xF+Wz0/i817nwFjepP+Rb0PnDwOUOehePyQq1eurlrSkXZOkVx7OblbGK43upf+zcqyldhOT22"
    "5GvV9ynKXIriQDpKye242Ldcve2KskX6deaRWVeebnpdu32Koryc4t/iaKksmm9Vvg3W3neWk+Vs"
    "qS2tpSOu9NmZtLG4f7R15paS7jXct1q7XxTl2ZDd157dp1A/6q9I+1Wg+QH1g8pmcb7M0szUHseV"
    "/KTTtJg39XyudH/NASmbhfzOUtqTNHmhew1cglhT9ad8O16kv7m4eFT3/pVNQ77IX8shLgW/RnsW"
    "Li7V5y4UJaDo9wnOT9Sjq1fzn4oSQXHft4tZ00Vpr5jI3yiKwmO1sy/63GZKzNpS+dVyLsaYneVk"
    "Ud+rbIqi32M/kC7DtaXzn6vt9Vm95ZbS0lg64rx2nynKdyHQ3oC+A06ZxdtbkztWundpqS1fKo5j"
    "9OfQ8+jKJNbW987eye47QGO5WE6u/BPakJP2rt4927X7JhanNaBBzQUpEMbmU2hJk66O2ftt7lrS"
    "f8vc4yl+0P7llspyWapOl3th9Hdde4yVz4m17eMD2pP8ZBXjI0l3J7oG1Rf1DlH7dyBOpKuB2+dB"
    "2ZzKXS3ugw9+lnKZB49oH2bL1owGH9K4vX5P7Yg+0+O1/Uj/XSRm8ep9Ws6LclrRfe+N1+xYIxjz"
    "49Jrh6D+m68inSypPeQja7pXRZp3/z1P6M7XM+xX+7eznElHc3E63Hv2xWmoo89H4yLEnpDg+orq"
    "DsnJFq7oWqYtLhd7sbTg3i3dL2VO2Xlt5Oo7DzY1Uc8VtOng9eMZtP0ctpnmJa787Tcm/jbUtmHc"
    "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";

static GdkPixbuf *logo_pixbuf;

static GMainLoop *main_loop;
static gint count;

static GdkPixbuf *
create_overlay_pixbuf (void)
{
  GZlibDecompressor *decompress;
  GConverterResult decomp_res;
  guchar *gzipped_pixdata, *pixdata, *pixels_copy;
  gsize gzipped_size, bytes_read, pixdata_size;
  guint stride, width, height;
  GdkPixbuf *pixbuf;

  gzipped_pixdata = g_base64_decode (gzipped_pixdata_base64, &gzipped_size);
  g_assert (gzipped_pixdata != NULL);

  pixdata = g_malloc (64 * 1024);

  decompress = g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP);
  decomp_res = g_converter_convert (G_CONVERTER (decompress),
      gzipped_pixdata, gzipped_size, pixdata, 64 * 1024,
      G_CONVERTER_INPUT_AT_END, &bytes_read, &pixdata_size, NULL);
  g_assert (decomp_res == G_CONVERTER_FINISHED);
  g_assert (bytes_read == gzipped_size);
  g_free (gzipped_pixdata);
  g_object_unref (decompress);

  /* 0: Pixbuf magic (0x47646b50) */
  g_assert (GST_READ_UINT32_BE (pixdata) == 0x47646b50);

  /* pixdata length */
  pixdata_size = GST_READ_UINT32_BE (pixdata + 4);
  g_assert (pixdata_size > 4 + 4 + 4 + 4 + 4 + 4);

  /* raw, 8-bit depth, RGBA */
  g_assert (GST_READ_UINT32_BE (pixdata + 8) == 0x01010002);

  stride = GST_READ_UINT32_BE (pixdata + 12);
  width = GST_READ_UINT32_BE (pixdata + 16);
  height = GST_READ_UINT32_BE (pixdata + 20);

  g_assert (pixdata_size == 24 + height * stride);

  pixels_copy = g_memdup (pixdata + 24, height * stride);

  pixbuf =
      gdk_pixbuf_new_from_data (pixels_copy, GDK_COLORSPACE_RGB, TRUE, 8,
      width, height, stride, (GdkPixbufDestroyNotify) g_free, pixels_copy);

  g_assert (pixbuf != NULL);

  g_free (pixdata);

  return pixbuf;
}

static gboolean
bus_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GMainLoop *loop = user_data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *dbg;

      gst_message_parse_error (msg, &err, &dbg);
      gst_object_default_error (msg->src, err, dbg);
      g_clear_error (&err);
      g_free (dbg);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

#define SPEED_SCALE_FACTOR (VIDEO_FPS * 4)

/* nicked from videotestsrc's ball pattern renderer */
static void
calculate_position (gint * x, gint * y, guint logo_w, guint logo_h, guint n)
{
  guint r_x = logo_w / 2;
  guint r_y = logo_h / 2;
  guint w = VIDEO_WIDTH + logo_w;
  guint h = VIDEO_HEIGHT + logo_h;

  *x = r_x + (0.5 + 0.5 * sin (2 * G_PI * n / SPEED_SCALE_FACTOR))
      * (w - 2 * r_x);
  *y = r_y + (0.5 + 0.5 * sin (2 * G_PI * sqrt (2) * n / SPEED_SCALE_FACTOR))
      * (h - 2 * r_y);

  *x -= logo_w;
  *y -= logo_h;
}

static GstPadProbeReturn
buffer_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstElement *overlay = GST_PAD_PARENT (pad);
  gint x, y, w, h;

  w = gdk_pixbuf_get_width (logo_pixbuf);
  h = gdk_pixbuf_get_height (logo_pixbuf);

  calculate_position (&x, &y, w, h, ++count);

  GST_LOG ("%3d, %3d", x, y);

  g_object_set (overlay, "offset-x", x, "offset-y", y, NULL);

  return GST_PAD_PROBE_OK;
}

int
main (int argc, char **argv)
{
  GOptionEntry options[] = {
    {NULL}
  };
  GOptionContext *ctx;
  GError *err = NULL;
  GstElement *src, *q, *capsfilter, *overlay, *sink;
  GstElement *pipeline;
  GstPad *sink_pad;
  GstCaps *filter_caps;

  ctx = g_option_context_new ("");
  g_option_context_add_main_entries (ctx, options, GETTEXT_PACKAGE);
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
    g_print ("Error initializing: %s\n", err->message);
    g_option_context_free (ctx);
    g_clear_error (&err);
    return 1;
  }
  g_option_context_free (ctx);

  logo_pixbuf = create_overlay_pixbuf ();

  main_loop = g_main_loop_new (NULL, FALSE);

  pipeline = gst_pipeline_new ("pipeline");

  src = gst_element_factory_make ("videotestsrc", NULL);
  gst_util_set_object_arg (G_OBJECT (src), "pattern", "white");

  overlay = gst_element_factory_make ("gdkpixbufoverlay", NULL);

  /* set positioning-mode to absolute so we can set negative positions */
  g_object_set (overlay, "pixbuf", logo_pixbuf, "positioning-mode", 1, NULL);

  sink_pad = gst_element_get_static_pad (overlay, "sink");
  gst_pad_add_probe (sink_pad, GST_PAD_PROBE_TYPE_BUFFER, buffer_cb, NULL,
      NULL);
  gst_object_unref (sink_pad);

  q = gst_element_factory_make ("queue", NULL);

  capsfilter = gst_element_factory_make ("capsfilter", NULL);
  filter_caps = gst_caps_from_string ("video/x-raw, format = "
      GST_VIDEO_OVERLAY_COMPOSITION_BLEND_FORMATS);
  gst_caps_set_simple (filter_caps,
      "width", G_TYPE_INT, VIDEO_WIDTH,
      "height", G_TYPE_INT, VIDEO_HEIGHT,
      "framerate", GST_TYPE_FRACTION, VIDEO_FPS, 1, NULL);
  g_object_set (capsfilter, "caps", filter_caps, NULL);
  gst_caps_unref (filter_caps);

  sink = gst_element_factory_make ("ximagesink", NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, q, overlay, capsfilter, sink,
      NULL);

  gst_element_link_many (src, q, overlay, capsfilter, sink, NULL);

  count = 0;

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  gst_bus_add_watch (GST_ELEMENT_BUS (pipeline), bus_cb, main_loop);

  g_main_loop_run (main_loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  g_object_unref (logo_pixbuf);

  return 0;
}
