/*
 * GStreamer
 * Copyright (C) 1999 Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2007,2008 Pioneers of the Inevitable <songbird@songbirdnest.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstosxaudioelement.h"
#include "gstosxaudiosink.h"
#include "gstosxaudiosrc.h"
#ifndef HAVE_IOS
#include "gstosxaudiodeviceprovider.h"
#endif

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "osxaudiosink", GST_RANK_PRIMARY,
          GST_TYPE_OSX_AUDIO_SINK)) {
    return FALSE;
  }
  if (!gst_element_register (plugin, "osxaudiosrc", GST_RANK_PRIMARY,
          GST_TYPE_OSX_AUDIO_SRC)) {
    return FALSE;
  }
#ifndef HAVE_IOS
  if (!gst_device_provider_register (plugin, "osxaudiodeviceprovider",
          GST_RANK_PRIMARY, GST_TYPE_OSX_AUDIO_DEVICE_PROVIDER))
    return FALSE;
#endif

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    osxaudio,
    "OSX (Mac OS X) audio support for GStreamer",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
