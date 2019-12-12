/* GStreamer
 * Copyright (C) 2009 Sebastian Droege <sebastian.droege@collabora.co.uk>
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

/* This small sample application creates a lowpass IIR filter
 * and applies it to white noise.
 * See http://www.dspguide.com/ch19/2.htm for a description
 * of the IIR filter that is used.
 */

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <string.h>
#include <math.h>

#include <gst/gst.h>

/* Cutoff of 4000 Hz */
#define CUTOFF (4000.0)

static gboolean
on_message (GstBus * bus, GstMessage * message, gpointer user_data)
{
  GMainLoop *loop = (GMainLoop *) user_data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      g_error ("Got ERROR");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_WARNING:
      g_warning ("Got WARNING");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    default:
      break;
  }

  return TRUE;
}

static void
on_rate_changed (GstElement * element, gint rate, gpointer user_data)
{
  GValueArray *va;
  GValue v = { 0, };
  gdouble x;

  if (rate / 2.0 > CUTOFF)
    x = exp (-2.0 * G_PI * (CUTOFF / rate));
  else
    x = 0.0;

  va = g_value_array_new (1);

  g_value_init (&v, G_TYPE_DOUBLE);
  g_value_set_double (&v, 1.0 - x);
  g_value_array_append (va, &v);
  g_value_reset (&v);
  g_object_set (G_OBJECT (element), "a", va, NULL);
  g_value_array_free (va);

  va = g_value_array_new (1);
  g_value_set_double (&v, x);
  g_value_array_append (va, &v);
  g_value_reset (&v);
  g_object_set (G_OBJECT (element), "b", va, NULL);
  g_value_array_free (va);
}

gint
main (gint argc, gchar * argv[])
{
  GstElement *pipeline, *src, *filter, *conv, *sink;
  GstBus *bus;
  GMainLoop *loop;

  gst_init (NULL, NULL);

  pipeline = gst_element_factory_make ("pipeline", NULL);

  src = gst_element_factory_make ("audiotestsrc", NULL);
  g_object_set (G_OBJECT (src), "wave", 5, NULL);

  filter = gst_element_factory_make ("audioiirfilter", NULL);
  g_signal_connect (G_OBJECT (filter), "rate-changed",
      G_CALLBACK (on_rate_changed), NULL);

  conv = gst_element_factory_make ("audioconvert", NULL);

  sink = gst_element_factory_make ("autoaudiosink", NULL);
  g_return_val_if_fail (sink != NULL, -1);

  gst_bin_add_many (GST_BIN (pipeline), src, filter, conv, sink, NULL);
  if (!gst_element_link_many (src, filter, conv, sink, NULL)) {
    g_error ("Failed to link elements");
    return -2;
  }

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (on_message), loop);
  gst_object_unref (GST_OBJECT (bus));

  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_error ("Failed to go into PLAYING state");
    return -3;
  }

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_main_loop_unref (loop);
  gst_object_unref (pipeline);

  return 0;
}
