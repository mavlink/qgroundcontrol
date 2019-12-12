/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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
#include <config.h>
#endif

#include <string.h>

#include <gst/gst.h>

#include "gstjpeg.h"
#include "gstjpegdec.h"
#include "gstjpegenc.h"
#if 0
#include "gstsmokeenc.h"
#include "gstsmokedec.h"
#endif

GType
gst_idct_method_get_type (void)
{
  static GType idct_method_type = 0;
  static const GEnumValue idct_method[] = {
    {JDCT_ISLOW, "Slow but accurate integer algorithm", "islow"},
    {JDCT_IFAST, "Faster, less accurate integer method", "ifast"},
    {JDCT_FLOAT, "Floating-point: accurate, fast on fast HW", "float"},
    {0, NULL, NULL},
  };

  if (!idct_method_type) {
    idct_method_type = g_enum_register_static ("GstIDCTMethod", idct_method);
  }
  return idct_method_type;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

  if (!gst_element_register (plugin, "jpegenc", GST_RANK_PRIMARY,
          GST_TYPE_JPEGENC))
    return FALSE;

  if (!gst_element_register (plugin, "jpegdec", GST_RANK_PRIMARY,
          GST_TYPE_JPEG_DEC))
    return FALSE;

#if 0
  if (!gst_element_register (plugin, "smokeenc", GST_RANK_PRIMARY,
          GST_TYPE_SMOKEENC))
    return FALSE;

  if (!gst_element_register (plugin, "smokedec", GST_RANK_PRIMARY,
          GST_TYPE_SMOKEDEC))
    return FALSE;
#endif

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    jpeg,
    "JPeg plugin library",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
