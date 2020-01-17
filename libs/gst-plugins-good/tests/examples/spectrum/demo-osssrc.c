/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
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

/* TODO: We use gdk_cairo_create() and others, which are deprecated */
#define GDK_DISABLE_DEPRECATION_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <gtk/gtk.h>

#ifndef DEFAULT_AUDIOSRC
#define DEFAULT_AUDIOSRC "alsasrc"
#endif

static guint spect_height = 64;
static guint spect_bands = 256;
static gfloat height_scale = 1.0;

static GtkWidget *drawingarea = NULL;
static GstClock *sync_clock = NULL;

static void
on_window_destroy (GObject * object, gpointer user_data)
{
  drawingarea = NULL;
  gtk_main_quit ();
}

static gboolean
on_configure_event (GtkWidget * widget, GdkEventConfigure * event,
    gpointer user_data)
{
  GstElement *spectrum = GST_ELEMENT (user_data);

  /*GST_INFO ("%d x %d", event->width, event->height); */
  spect_height = event->height;
  height_scale = event->height / 64.0;
  spect_bands = event->width;

  g_object_set (G_OBJECT (spectrum), "bands", spect_bands, NULL);
  return FALSE;
}

/* draw frequency spectrum as a bunch of bars */
static void
draw_spectrum (gfloat * data)
{
  gint i;
  GdkRectangle rect = { 0, 0, spect_bands, spect_height };
  cairo_t *cr;

  if (!drawingarea)
    return;

  gdk_window_begin_paint_rect (gtk_widget_get_window (drawingarea), &rect);
  cr = gdk_cairo_create (gtk_widget_get_window (drawingarea));

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_rectangle (cr, 0, 0, spect_bands, spect_height);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 1, 1, 1);
  for (i = 0; i < spect_bands; i++) {
    cairo_rectangle (cr, i, -data[i], 1, spect_height + data[i]);
    cairo_fill (cr);
  }
  cairo_destroy (cr);

  gdk_window_end_paint (gtk_widget_get_window (drawingarea));
}

/* process delayed message */
static gboolean
delayed_idle_spectrum_update (gpointer user_data)
{
  draw_spectrum ((gfloat *) user_data);
  g_free (user_data);
  return (FALSE);
}

static gboolean
delayed_spectrum_update (GstClock * sync_clock, GstClockTime time,
    GstClockID id, gpointer user_data)
{
  if (GST_CLOCK_TIME_IS_VALID (time))
    g_idle_add (delayed_idle_spectrum_update, user_data);
  else
    g_free (user_data);
  return (TRUE);
}

/* receive spectral data from element message */
static gboolean
message_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  if (message->type == GST_MESSAGE_ELEMENT) {
    const GstStructure *s = gst_message_get_structure (message);
    const gchar *name = gst_structure_get_name (s);

    if (strcmp (name, "spectrum") == 0) {
      GstElement *spectrum = GST_ELEMENT (GST_MESSAGE_SRC (message));
      GstClockTime timestamp, duration;
      GstClockTime waittime = GST_CLOCK_TIME_NONE;

      if (gst_structure_get_clock_time (s, "running-time", &timestamp) &&
          gst_structure_get_clock_time (s, "duration", &duration)) {
        /* wait for middle of buffer */
        waittime = timestamp + duration / 2;
      } else if (gst_structure_get_clock_time (s, "endtime", &timestamp)) {
        waittime = timestamp;
      }
      if (GST_CLOCK_TIME_IS_VALID (waittime)) {
        GstClockID clock_id;
        GstClockTime basetime = gst_element_get_base_time (spectrum);
        gfloat *spect = g_new (gfloat, spect_bands);
        const GValue *list;
        const GValue *value;
        guint i;

        list = gst_structure_get_value (s, "magnitude");
        for (i = 0; i < spect_bands; ++i) {
          value = gst_value_list_get_value (list, i);
          spect[i] = height_scale * g_value_get_float (value);
        }

        clock_id =
            gst_clock_new_single_shot_id (sync_clock, waittime + basetime);
        gst_clock_id_wait_async (clock_id, delayed_spectrum_update,
            (gpointer) spect, NULL);
        gst_clock_id_unref (clock_id);
      }
    }
  }
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GstElement *bin;
  GstElement *src, *spectrum, *sink;
  GstBus *bus;
  GtkWidget *appwindow;

  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);

  bin = gst_pipeline_new ("bin");

  src = gst_element_factory_make (DEFAULT_AUDIOSRC, "src");

  spectrum = gst_element_factory_make ("spectrum", "spectrum");
  g_object_set (G_OBJECT (spectrum), "bands", spect_bands, "threshold", -80,
      "post-messages", TRUE, NULL);

  sink = gst_element_factory_make ("fakesink", "sink");

  gst_bin_add_many (GST_BIN (bin), src, spectrum, sink, NULL);
  if (!gst_element_link_many (src, spectrum, sink, NULL)) {
    fprintf (stderr, "can't link elements\n");
    exit (1);
  }

  bus = gst_element_get_bus (bin);
  gst_bus_add_watch (bus, message_handler, NULL);
  gst_object_unref (bus);

  sync_clock = gst_pipeline_get_clock (GST_PIPELINE (bin));

  appwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (appwindow), "destroy",
      G_CALLBACK (on_window_destroy), NULL);

  drawingarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawingarea, spect_bands, spect_height);
  g_signal_connect (G_OBJECT (drawingarea), "configure-event",
      G_CALLBACK (on_configure_event), (gpointer) spectrum);
  gtk_container_add (GTK_CONTAINER (appwindow), drawingarea);
  gtk_widget_show_all (appwindow);

  gst_element_set_state (bin, GST_STATE_PLAYING);
  gtk_main ();
  gst_element_set_state (bin, GST_STATE_NULL);

  gst_object_unref (sync_clock);
  gst_object_unref (bin);

  return 0;
}
