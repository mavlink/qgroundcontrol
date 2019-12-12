/*
 *  GStreamer pulseaudio plugin
 *
 *  Copyright (c) 2004-2008 Lennart Poettering
 *
 *  gst-pulse is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  gst-pulse is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with gst-pulse; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 *  USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst-i18n-plugin.h>

#include "pulsesink.h"
#include "pulsesrc.h"
#include "pulsedeviceprovider.h"

GST_DEBUG_CATEGORY (pulse_debug);

static gboolean
plugin_init (GstPlugin * plugin)
{
#ifdef ENABLE_NLS
  GST_DEBUG ("binding text domain %s to locale dir %s", GETTEXT_PACKAGE,
      LOCALEDIR);
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  if (!gst_element_register (plugin, "pulsesink", GST_RANK_PRIMARY + 10,
          GST_TYPE_PULSESINK))
    return FALSE;

  if (!gst_element_register (plugin, "pulsesrc", GST_RANK_PRIMARY + 10,
          GST_TYPE_PULSESRC))
    return FALSE;

  if (!gst_device_provider_register (plugin, "pulsedeviceprovider",
          GST_RANK_PRIMARY, GST_TYPE_PULSE_DEVICE_PROVIDER))
    return FALSE;

  GST_DEBUG_CATEGORY_INIT (pulse_debug, "pulse", 0, "PulseAudio elements");
  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    pulseaudio,
    "PulseAudio plugin library",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
