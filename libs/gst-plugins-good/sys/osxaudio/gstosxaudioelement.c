/*
 * GStreamer
 * Copyright (C) 2006 Zaheer Abbas Merali <zaheerabbas at merali dot org>
 * Copyright (C) 2007 Pioneers of the Inevitable <songbird@songbirdnest.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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
 *
 * The development of this code was made possible due to the involvement of
 * Pioneers of the Inevitable, the creators of the Songbird Music player
 *
 */

#include "gstosxaudioelement.h"
#include <gst/gst.h>

static void
gst_osx_audio_element_interface_init (GstOsxAudioElementInterface * iface);

GType
gst_osx_audio_element_get_type (void)
{
  static GType gst_osxaudioelement_type = 0;

  if (!gst_osxaudioelement_type) {
    static const GTypeInfo gst_osxaudioelement_info = {
      sizeof (GstOsxAudioElementInterface),
      (GBaseInitFunc) gst_osx_audio_element_interface_init,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      NULL,
      NULL
    };

    gst_osxaudioelement_type = g_type_register_static (G_TYPE_INTERFACE,
        "GstOsxAudioElement", &gst_osxaudioelement_info, 0);
  }

  return gst_osxaudioelement_type;
}

static void
gst_osx_audio_element_interface_init (GstOsxAudioElementInterface * iface)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    initialized = TRUE;
  }

  /* default virtual functions */
  iface->io_proc = NULL;
}
