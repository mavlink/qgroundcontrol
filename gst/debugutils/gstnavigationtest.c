/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
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
#include "config.h"
#endif

#include "gstnavigationtest.h"
#include <string.h>
#include <math.h>

#include <gst/video/video.h>

#ifdef _MSC_VER
#define rint(x) (floor((x)+0.5))
#endif

GST_DEBUG_CATEGORY_STATIC (navigationtest_debug);
#define GST_CAT_DEFAULT navigationtest_debug

static GstStaticPadTemplate gst_navigationtest_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("I420"))
    );

static GstStaticPadTemplate gst_navigationtest_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("I420"))
    );

#define gst_navigationtest_parent_class parent_class
G_DEFINE_TYPE (GstNavigationtest, gst_navigationtest, GST_TYPE_VIDEO_FILTER);

static gboolean
gst_navigationtest_src_event (GstBaseTransform * trans, GstEvent * event)
{
  GstVideoInfo *info;
  GstNavigationtest *navtest;
  const gchar *type;

  navtest = GST_NAVIGATIONTEST (trans);

  info = &GST_VIDEO_FILTER (trans)->in_info;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_NAVIGATION:
    {
      const GstStructure *s = gst_event_get_structure (event);
      gint fps_n, fps_d;

      fps_n = GST_VIDEO_INFO_FPS_N (info);
      fps_d = GST_VIDEO_INFO_FPS_D (info);

      type = gst_structure_get_string (s, "event");
      if (g_str_equal (type, "mouse-move")) {
        gst_structure_get_double (s, "pointer_x", &navtest->x);
        gst_structure_get_double (s, "pointer_y", &navtest->y);
      } else if (g_str_equal (type, "mouse-button-press")) {
        ButtonClick *click = g_new (ButtonClick, 1);

        gst_structure_get_double (s, "pointer_x", &click->x);
        gst_structure_get_double (s, "pointer_y", &click->y);
        click->images_left = (fps_n + fps_d - 1) / fps_d;
        /* green */
        click->cy = 150;
        click->cu = 46;
        click->cv = 21;
        navtest->clicks = g_slist_prepend (navtest->clicks, click);
      } else if (g_str_equal (type, "mouse-button-release")) {
        ButtonClick *click = g_new (ButtonClick, 1);

        gst_structure_get_double (s, "pointer_x", &click->x);
        gst_structure_get_double (s, "pointer_y", &click->y);
        click->images_left = (fps_n + fps_d - 1) / fps_d;
        /* red */
        click->cy = 76;
        click->cu = 85;
        click->cv = 255;
        navtest->clicks = g_slist_prepend (navtest->clicks, click);
      }
      break;
    }
    default:
      break;
  }
  return GST_BASE_TRANSFORM_CLASS (parent_class)->src_event (trans, event);
}

/* Useful macros */
#define GST_VIDEO_I420_Y_ROWSTRIDE(width) (GST_ROUND_UP_4(width))
#define GST_VIDEO_I420_U_ROWSTRIDE(width) (GST_ROUND_UP_8(width)/2)
#define GST_VIDEO_I420_V_ROWSTRIDE(width) ((GST_ROUND_UP_8(GST_VIDEO_I420_Y_ROWSTRIDE(width)))/2)

#define GST_VIDEO_I420_Y_OFFSET(w,h) (0)
#define GST_VIDEO_I420_U_OFFSET(w,h) (GST_VIDEO_I420_Y_OFFSET(w,h)+(GST_VIDEO_I420_Y_ROWSTRIDE(w)*GST_ROUND_UP_2(h)))
#define GST_VIDEO_I420_V_OFFSET(w,h) (GST_VIDEO_I420_U_OFFSET(w,h)+(GST_VIDEO_I420_U_ROWSTRIDE(w)*GST_ROUND_UP_2(h)/2))

#define GST_VIDEO_I420_SIZE(w,h)     (GST_VIDEO_I420_V_OFFSET(w,h)+(GST_VIDEO_I420_V_ROWSTRIDE(w)*GST_ROUND_UP_2(h)/2))

static void
draw_box_planar411 (GstVideoFrame * frame, int x, int y,
    guint8 colory, guint8 coloru, guint8 colorv)
{
  gint width, height;
  int x1, x2, y1, y2;
  guint8 *d;
  gint stride;

  width = GST_VIDEO_FRAME_WIDTH (frame);
  height = GST_VIDEO_FRAME_HEIGHT (frame);

  if (x < 0 || y < 0 || x >= width || y >= height)
    return;

  x1 = MAX (x - 5, 0);
  x2 = MIN (x + 5, width);
  y1 = MAX (y - 5, 0);
  y2 = MIN (y + 5, height);

  d = GST_VIDEO_FRAME_PLANE_DATA (frame, 0);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE (frame, 0);

  for (y = y1; y < y2; y++) {
    for (x = x1; x < x2; x++) {
      d[y * stride + x] = colory;
    }
  }

  d = GST_VIDEO_FRAME_PLANE_DATA (frame, 1);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE (frame, 1);

  x1 /= 2;
  x2 /= 2;
  y1 /= 2;
  y2 /= 2;
  for (y = y1; y < y2; y++) {
    for (x = x1; x < x2; x++) {
      d[y * stride + x] = coloru;
    }
  }

  d = GST_VIDEO_FRAME_PLANE_DATA (frame, 2);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE (frame, 2);

  for (y = y1; y < y2; y++) {
    for (x = x1; x < x2; x++) {
      d[y * stride + x] = colorv;
    }
  }
}

static GstFlowReturn
gst_navigationtest_transform_frame (GstVideoFilter * filter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
  GstNavigationtest *navtest = GST_NAVIGATIONTEST (filter);
  GSList *walk;

  gst_video_frame_copy (out_frame, in_frame);

  walk = navtest->clicks;
  while (walk) {
    ButtonClick *click = walk->data;

    walk = g_slist_next (walk);
    draw_box_planar411 (out_frame,
        rint (click->x), rint (click->y), click->cy, click->cu, click->cv);
    if (--click->images_left < 1) {
      navtest->clicks = g_slist_remove (navtest->clicks, click);
      g_free (click);
    }
  }
  draw_box_planar411 (out_frame,
      rint (navtest->x), rint (navtest->y), 0, 128, 128);

  return GST_FLOW_OK;
}

static GstStateChangeReturn
gst_navigationtest_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstNavigationtest *navtest = GST_NAVIGATIONTEST (element);

  if (GST_ELEMENT_CLASS (parent_class)->change_state)
    ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  /* downwards state changes */
  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
      g_slist_foreach (navtest->clicks, (GFunc) g_free, NULL);
      g_slist_free (navtest->clicks);
      navtest->clicks = NULL;
      break;
    }
    default:
      break;
  }

  return ret;
}

static void
gst_navigationtest_class_init (GstNavigationtestClass * klass)
{
  GstElementClass *element_class;
  GstBaseTransformClass *trans_class;
  GstVideoFilterClass *vfilter_class;

  element_class = (GstElementClass *) klass;
  trans_class = (GstBaseTransformClass *) klass;
  vfilter_class = (GstVideoFilterClass *) klass;

  element_class->change_state =
      GST_DEBUG_FUNCPTR (gst_navigationtest_change_state);

  gst_element_class_set_static_metadata (element_class, "Video navigation test",
      "Filter/Effect/Video",
      "Handle navigation events showing a black square following mouse pointer",
      "David Schleef <ds@schleef.org>");

  gst_element_class_add_static_pad_template (element_class,
      &gst_navigationtest_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_navigationtest_src_template);

  trans_class->src_event = GST_DEBUG_FUNCPTR (gst_navigationtest_src_event);

  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_navigationtest_transform_frame);
}

static void
gst_navigationtest_init (GstNavigationtest * navtest)
{
  navtest->x = -1;
  navtest->y = -1;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (navigationtest_debug, "navigationtest", 0,
      "navigationtest");

  return gst_element_register (plugin, "navigationtest", GST_RANK_NONE,
      GST_TYPE_NAVIGATIONTEST);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    navigationtest,
    "Template for a video filter",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
