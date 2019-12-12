/* GStreamer interactive test for the gdkpixbufsink element
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

#include <gst/gst.h>
#include <gtk/gtk.h>

typedef struct
{
  GstElement *pipe;
  GstElement *sink;
  gboolean got_video;

  GtkWidget *win;
  GtkWidget *img;
  GtkWidget *slider;
  GtkWidget *accurate_cb;

  gboolean accurate;            /* whether to try accurate seeks */

  gint64 cur_pos;
  gboolean prerolled;
} AppInfo;

static void seek_to (AppInfo * info, gdouble percent);

static GstElement *
create_element (const gchar * factory_name)
{
  GstElement *element;

  element = gst_element_factory_make (factory_name, NULL);

  if (element == NULL)
    g_error ("Failed to create '%s' element", factory_name);

  return element;
}

static void
new_decoded_pad (GstElement * dec, GstPad * new_pad, AppInfo * info)
{
  const gchar *sname;
  GstElement *csp, *scale, *filter;
  GstStructure *s;
  GstCaps *caps;
  GstPad *sinkpad;

  /* already found a video stream? */
  if (info->got_video)
    return;

  /* FIXME: is this racy or does decodebin make sure caps are always
   * negotiated at this point? */
  caps = gst_pad_query_caps (new_pad, NULL);
  g_return_if_fail (caps != NULL);

  s = gst_caps_get_structure (caps, 0);
  sname = gst_structure_get_name (s);
  if (!g_str_has_prefix (sname, "video/x-raw"))
    goto not_video;

  csp = create_element ("videoconvert");
  scale = create_element ("videoscale");
  filter = create_element ("capsfilter");
  info->sink = create_element ("gdkpixbufsink");
  g_object_set (info->sink, "qos", FALSE, "max-lateness", (gint64) - 1, NULL);

  gst_bin_add_many (GST_BIN (info->pipe), csp, scale, filter, info->sink, NULL);

  sinkpad = gst_element_get_static_pad (csp, "sink");
  if (GST_PAD_LINK_FAILED (gst_pad_link (new_pad, sinkpad)))
    g_error ("Can't link new decoded pad to videoconvert's sink pad");
  gst_object_unref (sinkpad);

  if (!gst_element_link (csp, scale))
    g_error ("Can't link videoconvert to videoscale");
  if (!gst_element_link (scale, filter))
    g_error ("Can't link videoscale to capsfilter");
  if (!gst_element_link (filter, info->sink))
    g_error ("Can't link capsfilter to gdkpixbufsink");

  gst_element_set_state (info->sink, GST_STATE_PAUSED);
  gst_element_set_state (filter, GST_STATE_PAUSED);
  gst_element_set_state (scale, GST_STATE_PAUSED);
  gst_element_set_state (csp, GST_STATE_PAUSED);

  info->got_video = TRUE;
  return;

not_video:
  return;
}

static void
no_more_pads (GstElement * decodebin, AppInfo * info)
{
  if (!info->got_video) {
    g_error ("This file does not contain a video track, or you do not have "
        "the necessary decoder(s) installed");
  }
}

static void
bus_message_cb (GstBus * bus, GstMessage * msg, AppInfo * info)
{
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ASYNC_DONE:{
      /* only interested in async-done messages from the top-level pipeline */
      if (msg->src != GST_OBJECT_CAST (info->pipe))
        break;

      if (!info->prerolled) {
        /* make slider visible if it's not visible already */
        gtk_widget_show (info->slider);

        /* initial frame is often black, so seek to beginning plus a bit */
        seek_to (info, 0.001);
        info->prerolled = TRUE;
      }

      /* update position */
      if (!gst_element_query_position (info->pipe, GST_FORMAT_TIME,
              &info->cur_pos))
        info->cur_pos = -1;
      break;
    }
    case GST_MESSAGE_ELEMENT:{
      const GValue *val;
      GdkPixbuf *pixbuf = NULL;
      const GstStructure *structure;

      /* only interested in element messages from our gdkpixbufsink */
      if (msg->src != GST_OBJECT_CAST (info->sink))
        break;

      /* only interested in these two messages */
      if (!gst_message_has_name (msg, "preroll-pixbuf") &&
          !gst_message_has_name (msg, "pixbuf")) {
        break;
      }

      g_print ("pixbuf\n");
      structure = gst_message_get_structure (msg);
      val = gst_structure_get_value (structure, "pixbuf");
      g_return_if_fail (val != NULL);

      pixbuf = GDK_PIXBUF (g_value_dup_object (val));
      gtk_image_set_from_pixbuf (GTK_IMAGE (info->img), pixbuf);
      g_object_unref (pixbuf);
      break;
    }
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *dbg = NULL;

      gst_message_parse_error (msg, &err, &dbg);
      g_error ("Error: %s\n%s\n", err->message, (dbg) ? dbg : "");
      g_clear_error (&err);
      g_free (dbg);
      break;
    }
    default:
      break;
  }
}

static gboolean
create_pipeline (AppInfo * info, const gchar * filename)
{
  GstElement *src, *dec;
  GstBus *bus;

  info->pipe = gst_pipeline_new ("pipeline");
  src = create_element ("filesrc");
  g_object_set (src, "location", filename, NULL);

  dec = create_element ("decodebin");

  gst_bin_add_many (GST_BIN (info->pipe), src, dec, NULL);
  if (!gst_element_link (src, dec))
    g_error ("Can't link filesrc to decodebin");

  g_signal_connect (dec, "pad-added", G_CALLBACK (new_decoded_pad), info);

  g_signal_connect (dec, "no-more-pads", G_CALLBACK (no_more_pads), info);

  /* set up bus */
  bus = gst_element_get_bus (info->pipe);
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message", G_CALLBACK (bus_message_cb), info);
  gst_object_unref (bus);

  return TRUE;
}

static void
seek_to (AppInfo * info, gdouble percent)
{
  GstSeekFlags seek_flags;
  gint64 seek_pos, dur = -1;

  if (!gst_element_query_duration (info->pipe, GST_FORMAT_TIME, &dur)
      || dur <= 0) {
    g_printerr ("Could not query duration\n");
    return;
  }

  seek_pos = gst_gdouble_to_guint64 (gst_guint64_to_gdouble (dur) * percent);
  g_print ("Seeking to %" GST_TIME_FORMAT ", accurate: %d\n",
      GST_TIME_ARGS (seek_pos), info->accurate);

  seek_flags = GST_SEEK_FLAG_FLUSH;

  if (info->accurate)
    seek_flags |= GST_SEEK_FLAG_ACCURATE;
  else
    seek_flags |= GST_SEEK_FLAG_KEY_UNIT;

  if (!gst_element_seek_simple (info->pipe, GST_FORMAT_TIME, seek_flags,
          seek_pos)) {
    g_printerr ("Seek failed.\n");
    return;
  }
}

static void
slider_cb (GtkRange * range, AppInfo * info)
{
  gdouble val;

  val = gtk_range_get_value (range);
  seek_to (info, val);
}

static gchar *
slider_format_value_cb (GtkScale * scale, gdouble value, AppInfo * info)
{
  gchar s[64];

  if (info->cur_pos < 0)
    return g_strdup_printf ("%0.1g%%", value * 100.0);

  g_snprintf (s, 64, "%" GST_TIME_FORMAT, GST_TIME_ARGS (info->cur_pos));
  s[10] = '\0';
  return g_strdup (s);
}

static void
accurate_toggled_cb (GtkToggleButton * toggle, AppInfo * info)
{
  info->accurate = gtk_toggle_button_get_active (toggle);
}

static void
run_gui (const gchar * filename)
{
  GtkWidget *vbox, *hbox;
  AppInfo *info;

  info = g_new0 (AppInfo, 1);

  /* create pipeline */
  if (!create_pipeline (info, filename))
    goto done;

  /* create window */
  info->win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (info->win, "delete-event", G_CALLBACK (gtk_main_quit),
      NULL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (info->win), vbox);

  info->img = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (vbox), info->img, FALSE, FALSE, 6);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 6);

  info->accurate_cb = gtk_check_button_new_with_label ("accurate seek "
      "(might not work reliably with all demuxers)");
  gtk_box_pack_start (GTK_BOX (hbox), info->accurate_cb, FALSE, FALSE, 6);
  g_signal_connect (info->accurate_cb, "toggled",
      G_CALLBACK (accurate_toggled_cb), info);

  info->slider = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
      0.0, 1.0, 0.001);
  gtk_box_pack_start (GTK_BOX (vbox), info->slider, FALSE, FALSE, 6);
  g_signal_connect (info->slider, "value-changed",
      G_CALLBACK (slider_cb), info);
  g_signal_connect (info->slider, "format-value",
      G_CALLBACK (slider_format_value_cb), info);

  /* and go! */
  gst_element_set_state (info->pipe, GST_STATE_PAUSED);

  gtk_widget_show_all (info->win);
  gtk_widget_hide (info->slider);       /* hide until we're prerolled */
  gtk_main ();

done:

  g_free (info);
}

static gchar **filenames = NULL;

int
main (int argc, char **argv)
{
  static const GOptionEntry test_goptions[] = {
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL},
    {NULL, '\0', 0, 0, NULL, NULL, NULL}
  };
  GOptionContext *ctx;
  GError *opt_err = NULL;

  gtk_init (&argc, &argv);

  /* command line option parsing */
  ctx = g_option_context_new (" VIDEOFILE");
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  g_option_context_add_main_entries (ctx, test_goptions, NULL);

  if (!g_option_context_parse (ctx, &argc, &argv, &opt_err)) {
    g_error ("Error parsing command line options: %s", opt_err->message);
    g_option_context_free (ctx);
    g_clear_error (&opt_err);
    return -1;
  }

  if (filenames == NULL || filenames[0] == NULL || filenames[0][0] == '\0') {
    g_printerr ("Please specify a path to a video file\n\n");
    return -1;
  }

  run_gui (filenames[0]);

  g_free (filenames);
  g_option_context_free (ctx);

  return 0;
}
