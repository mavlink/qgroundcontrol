/* GStreamer OSS4 audio tests
 * Copyright (C) 2007-2008 Tim-Philipp Müller <tim centricular net>
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

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <gst/gst.h>

static gboolean opt_show_mixer_messages = FALSE;

#define WAIT_TIME  60.0         /* in seconds */

static void
probe_pad (GstElement * element, const gchar * pad_name)
{
  GstCaps *caps = NULL;
  GstPad *pad;
  guint i;

  pad = gst_element_get_static_pad (element, pad_name);
  if (pad == NULL)
    return;

  caps = gst_pad_query_caps (pad, NULL);
  g_return_if_fail (caps != NULL);

  for (i = 0; i < gst_caps_get_size (caps); ++i) {
    gchar *s;

    s = gst_structure_to_string (gst_caps_get_structure (caps, i));
    g_print ("  %4s[%d]: %s\n", GST_PAD_NAME (pad), i, s);
    g_free (s);
  }
  gst_caps_unref (caps);
  gst_object_unref (pad);
}

static void
probe_details (GstElement * element)
{
  GstStateChangeReturn ret;

  ret = gst_element_set_state (element, GST_STATE_READY);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_print ("Could not set element %s to READY.", GST_ELEMENT_NAME (element));
    return;
  }

  probe_pad (element, "sink");
  probe_pad (element, "src");

  gst_element_set_state (element, GST_STATE_NULL);
}

static void
probe_element (const gchar * name)
{
  GstElement *element;
  gchar *devname = NULL;

  element = gst_element_factory_make (name, name);

  /* make sure we don't deadlock on GST_ELEMENT_ERROR or do other silly things
   * if we try to query the "device-name" property when the device isn't open */
  g_object_set (element, "device", "/dev/does/not/exist", NULL);
  g_object_get (element, "device-name", &devname, NULL);
  GST_LOG ("devname: '%s'", GST_STR_NULL (devname));
  g_assert (devname == NULL || *devname == '\0');

  /* and now for real */

  probe_details (element);

  gst_object_unref (element);
}

int
main (int argc, char **argv)
{
  GOptionEntry options[] = {
    {"show-mixer-messages", 'm', 0, G_OPTION_ARG_NONE, &opt_show_mixer_messages,
        "For mixer elements, wait 60 seconds and show any mixer messages "
          "(for debugging auto-notifications)", NULL},
    {NULL,}
  };
  GOptionContext *ctx;
  GError *err = NULL;

  ctx = g_option_context_new ("");
  g_option_context_add_main_entries (ctx, options, NULL);
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
    g_print ("Error initializing: %s\n", err->message);
    g_option_context_free (ctx);
    g_clear_error (&err);
    exit (1);
  }
  g_option_context_free (ctx);

  probe_element ("oss4sink");
  probe_element ("oss4src");

  return 0;
}
