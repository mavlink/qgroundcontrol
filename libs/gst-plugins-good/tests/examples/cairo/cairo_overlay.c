/* GStreamer
 * Copyright (C) 2011 Jon Nordby <jononor@gmail.com>
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

/*
 * Example showing usage of the cairooverlay element
 */

#include <gst/gst.h>
#include <gst/video/video.h>

#include <cairo.h>
#include <cairo-gobject.h>

#include <glib.h>


static gboolean
on_message (GstBus * bus, GstMessage * message, gpointer user_data)
{
  GMainLoop *loop = (GMainLoop *) user_data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      g_critical ("Got ERROR: %s (%s)", err->message, GST_STR_NULL (debug));
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_WARNING:{
      GError *err = NULL;
      gchar *debug;

      gst_message_parse_warning (message, &err, &debug);
      g_warning ("Got WARNING: %s (%s)", err->message, GST_STR_NULL (debug));
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    default:
      break;
  }

  return TRUE;
}

/* Datastructure to share the state we are interested in between
 * prepare and render function. */
typedef struct
{
  gboolean valid;
  GstVideoInfo vinfo;
} CairoOverlayState;

/* Store the information from the caps that we are interested in. */
static void
prepare_overlay (GstElement * overlay, GstCaps * caps, gpointer user_data)
{
  CairoOverlayState *state = (CairoOverlayState *) user_data;

  state->valid = gst_video_info_from_caps (&state->vinfo, caps);
}

/* Draw the overlay. 
 * This function draws a cute "beating" heart. */
static void
draw_overlay (GstElement * overlay, cairo_t * cr, guint64 timestamp,
    guint64 duration, gpointer user_data)
{
  CairoOverlayState *s = (CairoOverlayState *) user_data;
  double scale;
  int width, height;

  if (!s->valid)
    return;

  width = GST_VIDEO_INFO_WIDTH (&s->vinfo);
  height = GST_VIDEO_INFO_HEIGHT (&s->vinfo);

  scale = 2 * (((timestamp / (int) 1e7) % 70) + 30) / 100.0;
  cairo_translate (cr, width / 2, (height / 2) - 30);

  /* FIXME: this assumes a pixel-aspect-ratio of 1/1 */
  cairo_scale (cr, scale, scale);

  cairo_move_to (cr, 0, 0);
  cairo_curve_to (cr, 0, -30, -50, -30, -50, 0);
  cairo_curve_to (cr, -50, 30, 0, 35, 0, 60);
  cairo_curve_to (cr, 0, 35, 50, 30, 50, 0);
  cairo_curve_to (cr, 50, -30, 0, -30, 0, 0);
  cairo_set_source_rgba (cr, 0.9, 0.0, 0.1, 0.7);
  cairo_fill (cr);
}

static GstElement *
setup_gst_pipeline (CairoOverlayState * overlay_state)
{
  GstElement *pipeline;
  GstElement *cairo_overlay;
  GstElement *source, *adaptor1, *adaptor2, *sink;

  pipeline = gst_pipeline_new ("cairo-overlay-example");

  /* Adaptors needed because cairooverlay only supports ARGB data */
  source = gst_element_factory_make ("videotestsrc", "source");
  adaptor1 = gst_element_factory_make ("videoconvert", "adaptor1");
  cairo_overlay = gst_element_factory_make ("cairooverlay", "overlay");
  adaptor2 = gst_element_factory_make ("videoconvert", "adaptor2");
  sink = gst_element_factory_make ("ximagesink", "sink");
  if (sink == NULL)
    sink = gst_element_factory_make ("autovideosink", "sink");

  /* If failing, the element could not be created */
  g_assert (cairo_overlay);

  /* Hook up the necessary signals for cairooverlay */
  g_signal_connect (cairo_overlay, "draw",
      G_CALLBACK (draw_overlay), overlay_state);
  g_signal_connect (cairo_overlay, "caps-changed",
      G_CALLBACK (prepare_overlay), overlay_state);

  gst_bin_add_many (GST_BIN (pipeline), source, adaptor1,
      cairo_overlay, adaptor2, sink, NULL);

  if (!gst_element_link_many (source, adaptor1,
          cairo_overlay, adaptor2, sink, NULL)) {
    g_warning ("Failed to link elements!");
  }

  return pipeline;
}

int
main (int argc, char **argv)
{
  GMainLoop *loop;
  GstElement *pipeline;
  GstBus *bus;
  CairoOverlayState *overlay_state;

  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* allocate on heap for pedagogical reasons, makes code easier to transfer */
  overlay_state = g_new0 (CairoOverlayState, 1);

  pipeline = setup_gst_pipeline (overlay_state);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (on_message), loop);
  gst_object_unref (GST_OBJECT (bus));

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  g_free (overlay_state);
  return 0;
}
