/*
 * GStreamer
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

#include <gst/gst.h>

#include "audiopanorama.h"
#include "audioinvert.h"
#include "audiokaraoke.h"
#include "audioamplify.h"
#include "audiodynamic.h"
#include "audiocheblimit.h"
#include "audiochebband.h"
#include "audioiirfilter.h"
#include "audiowsincband.h"
#include "audiowsinclimit.h"
#include "audiofirfilter.h"
#include "audioecho.h"
#include "gstscaletempo.h"
#include "gststereo.h"

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and pad templates
 * register the features
 */
static gboolean
plugin_init (GstPlugin * plugin)
{
  return (gst_element_register (plugin, "audiopanorama", GST_RANK_NONE,
          GST_TYPE_AUDIO_PANORAMA) &&
      gst_element_register (plugin, "audioinvert", GST_RANK_NONE,
          GST_TYPE_AUDIO_INVERT) &&
      gst_element_register (plugin, "audiokaraoke", GST_RANK_NONE,
          GST_TYPE_AUDIO_KARAOKE) &&
      gst_element_register (plugin, "audioamplify", GST_RANK_NONE,
          GST_TYPE_AUDIO_AMPLIFY) &&
      gst_element_register (plugin, "audiodynamic", GST_RANK_NONE,
          GST_TYPE_AUDIO_DYNAMIC) &&
      gst_element_register (plugin, "audiocheblimit", GST_RANK_NONE,
          GST_TYPE_AUDIO_CHEB_LIMIT) &&
      gst_element_register (plugin, "audiochebband", GST_RANK_NONE,
          GST_TYPE_AUDIO_CHEB_BAND) &&
      gst_element_register (plugin, "audioiirfilter", GST_RANK_NONE,
          GST_TYPE_AUDIO_IIR_FILTER) &&
      gst_element_register (plugin, "audiowsinclimit", GST_RANK_NONE,
          GST_TYPE_AUDIO_WSINC_LIMIT) &&
      gst_element_register (plugin, "audiowsincband", GST_RANK_NONE,
          GST_TYPE_AUDIO_WSINC_BAND) &&
      gst_element_register (plugin, "audiofirfilter", GST_RANK_NONE,
          GST_TYPE_AUDIO_FIR_FILTER) &&
      gst_element_register (plugin, "audioecho", GST_RANK_NONE,
          GST_TYPE_AUDIO_ECHO) &&
      gst_element_register (plugin, "scaletempo", GST_RANK_NONE,
          GST_TYPE_SCALETEMPO) &&
      gst_element_register (plugin, "stereo", GST_RANK_NONE, GST_TYPE_STEREO));
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    audiofx,
    "Audio effects plugin",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
