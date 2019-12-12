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

/* This small sample application creates a bandpass FIR filter
 * by transforming the frequency response to the filter kernel.
 */

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <string.h>
#include <math.h>

#include <gst/gst.h>
#include <gst/fft/gstfftf64.h>

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
  GstFFTF64 *fft;
  GstFFTF64Complex frequency_response[17];
  gdouble tmp[32];
  gdouble filter_kernel[32];
  guint i;

  /* Create the frequency response: zero outside
   * a small frequency band */
  for (i = 0; i < 17; i++) {
    if (i < 5 || i > 11)
      frequency_response[i].r = 0.0;
    else
      frequency_response[i].r = 1.0;

    frequency_response[i].i = 0.0;
  }

  /* Calculate the inverse FT of the frequency response */
  fft = gst_fft_f64_new (32, TRUE);
  gst_fft_f64_inverse_fft (fft, frequency_response, tmp);
  gst_fft_f64_free (fft);

  /* Shift the inverse FT of the frequency response by 16,
   * i.e. the half of the kernel length to get the
   * impulse response. See http://www.dspguide.com/ch17/1.htm
   * for more information.
   */
  for (i = 0; i < 32; i++)
    filter_kernel[i] = tmp[(i + 16) % 32];

  /* Apply the hamming window to the impulse response to get
   * a better result than given from the rectangular window
   */
  for (i = 0; i < 32; i++)
    filter_kernel[i] *= (0.54 - 0.46 * cos (2 * G_PI * i / 32));

  va = g_value_array_new (1);

  g_value_init (&v, G_TYPE_DOUBLE);
  for (i = 0; i < 32; i++) {
    g_value_set_double (&v, filter_kernel[i]);
    g_value_array_append (va, &v);
    g_value_reset (&v);
  }
  g_object_set (G_OBJECT (element), "kernel", va, NULL);
  /* Latency is 1/2 of the kernel length for this method of
   * calculating a filter kernel from the frequency response
   */
  g_object_set (G_OBJECT (element), "latency", (gint64) (32 / 2), NULL);
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

  filter = gst_element_factory_make ("audiofirfilter", NULL);
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
