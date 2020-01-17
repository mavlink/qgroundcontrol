/* GStreamer GdkPixbuf plugin
 * Copyright (C) 1999-2001 Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2003 David A. Schleef <ds@schleef.org>
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

#include <gst/gst.h>

#include "gstgdkpixbufdec.h"
#include "gstgdkpixbufoverlay.h"
#include "gstgdkpixbufsink.h"


#if 0
static void gst_gdk_pixbuf_type_find (GstTypeFind * tf, gpointer ignore);

#define GST_GDK_PIXBUF_TYPE_FIND_SIZE 1024

static void
gst_gdk_pixbuf_type_find (GstTypeFind * tf, gpointer ignore)
{
  guint8 *data;
  GdkPixbufLoader *pixbuf_loader;
  GdkPixbufFormat *format;

  data = gst_type_find_peek (tf, 0, GST_GDK_PIXBUF_TYPE_FIND_SIZE);
  if (data == NULL)
    return;

  GST_DEBUG ("creating new loader");

  pixbuf_loader = gdk_pixbuf_loader_new ();

  gdk_pixbuf_loader_write (pixbuf_loader, data, GST_GDK_PIXBUF_TYPE_FIND_SIZE,
      NULL);

  format = gdk_pixbuf_loader_get_format (pixbuf_loader);

  if (format != NULL) {
    GstCaps *caps;
    gchar **p;
    gchar **mlist = gdk_pixbuf_format_get_mime_types (format);

    for (p = mlist; *p; ++p) {
      GST_DEBUG ("suggesting mime type %s", *p);
      caps = gst_caps_new_simple (*p, NULL);
      gst_type_find_suggest (tf, GST_TYPE_FIND_MINIMUM, caps);
      gst_caps_free (caps);
    }
    g_strfreev (mlist);
  }

  GST_DEBUG ("closing pixbuf loader, hope it doesn't hang ...");
  /* librsvg 2.4.x has a bug where it triggers an endless loop in trying
     to close a gzip that's not an svg; fixed upstream but no good way
     to work around it */
  gdk_pixbuf_loader_close (pixbuf_loader, NULL);
  GST_DEBUG ("closed pixbuf loader");
  g_object_unref (G_OBJECT (pixbuf_loader));
}
#endif

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "gdkpixbufdec", GST_RANK_SECONDARY,
          GST_TYPE_GDK_PIXBUF_DEC))
    return FALSE;

#if 0
  gst_type_find_register (plugin, "image/*", GST_RANK_MARGINAL,
      gst_gdk_pixbuf_type_find, NULL, GST_CAPS_ANY, NULL);
#endif

  if (!gst_element_register (plugin, "gdkpixbufoverlay", GST_RANK_NONE,
          GST_TYPE_GDK_PIXBUF_OVERLAY))
    return FALSE;

  if (!gst_element_register (plugin, "gdkpixbufsink", GST_RANK_NONE,
          GST_TYPE_GDK_PIXBUF_SINK))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    gdkpixbuf,
    "GdkPixbuf-based image decoder, overlay and sink",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
