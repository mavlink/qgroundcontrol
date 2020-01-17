/* GStreamer ReplayGain plugin
 *
 * Copyright (C) 2006 Rene Stadler <mail@renestadler.de>
 * 
 * replaygain.c: Plugin providing ReplayGain related elements
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>

#include "gstrganalysis.h"
#include "gstrglimiter.h"
#include "gstrgvolume.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "rganalysis", GST_RANK_NONE,
          GST_TYPE_RG_ANALYSIS))
    return FALSE;

  if (!gst_element_register (plugin, "rglimiter", GST_RANK_NONE,
          GST_TYPE_RG_LIMITER))
    return FALSE;

  if (!gst_element_register (plugin, "rgvolume", GST_RANK_NONE,
          GST_TYPE_RG_VOLUME))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, replaygain,
    "ReplayGain volume normalization", plugin_init, VERSION, GST_LICENSE,
    GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
